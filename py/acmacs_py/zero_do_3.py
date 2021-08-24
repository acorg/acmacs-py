# 0do.py support, e.g. ssm report custom

import sys, os, time, datetime, subprocess, json, pprint, contextlib, traceback
from pathlib import Path
from typing import List, Union, Callable
import acmacs

# ======================================================================

def main():

    def main_commands():
        return [name for name, value in vars(sys.modules["__main__"]).items() if name[0] != "_" and name != "Path" and callable(value)]

    def parse_command_line():
        import argparse
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument("--command-list", action='store_true', default=False)
        parser.add_argument("command", nargs='?')
        args = parser.parse_args()
        if args.command_list:
            print("\n".join(main_commands()))
            exit(0)
        if args.command:
            return args.command
        else:
            return main_commands()[0]

    command = parse_command_line()
    try:
        zd = Zd()
        cmd = getattr(sys.modules["__main__"], command)
        zd.snapshot_data.section(cmd)
        return cmd(zd)
    except Error as err:
        print(f"> {err}", file=sys.stderr)
        return 1
    except Exception as err:
        print(f"> {type(err)}: {err}\n{traceback.format_exc()}", file=sys.stderr)
        return 2

# ======================================================================

class Zd:

    subtype_lineage_to_mapi_name = {"H1": "h1pdm.mapi", "H3": "h3.mapi", "BVICTORIA": "bvic.mapi", "BYAMAGATA": "byam.mapi"}
    subtype_lineage_to_mapi_key = {"H1": "loc:clade-155-156-A(H1N1)2009pdm", "H3": "loc:clades-A(H3N2)-all", "BVICTORIA": "loc:clades-B/Vic", "BYAMAGATA": "loc:clades-B/Yam"}

    test_antigen_size = 10
    reference_antigen_size = test_antigen_size * 1.5
    serum_size = test_antigen_size * 1.5
    grey = "#D0D0D0"

    def __init__(self):
        self.mapi_key = None
        self.mapi_data = None
        self.snapshot_data = Snapshot()

    def open(self, filename: Path) -> acmacs.Chart:
        chart = acmacs.Chart(filename)
        self.load_mapi(chart)
        pdf_filename = Path(filename.name).with_suffix(f".{self.snapshot_data.number_of_images()}.pdf")
        with self.draw(chart, pdf=pdf_filename, ace=filename, overwrite=False, export=False): pass
        return chart

    @contextlib.contextmanager
    def draw(self, chart: acmacs.Chart, pdf: Path, ace: Path = None, overwrite: bool = True, export: bool = True, add_image: bool = True):
        if overwrite or not pdf.exists():
            painter = acmacs.ChartDraw(chart)
            self.draw_reset(painter)
            self.draw_mark_with_mapi(painter)
            painter.title(lines=["{lab} {virus-type/lineage-subset} {assay-no-hi-cap} " + f"{painter.chart().projection(0).stress(recalculate=True):.4f}"], remove_all_lines=True)
            painter.legend(offset=[10, 40])
            yield painter
            painter.calculate_viewport()
            painter.draw(pdf)
            if export:
                painter.chart().export(ace)
        else:
            yield None          # contextmanager requirement
        if add_image:
            self.snapshot_data.add_image(pdf=pdf, ace=ace)

    def draw_reset(self, painter: acmacs.ChartDraw):
        pchart = painter.chart()
        painter.modify(pchart.select_antigens(lambda ag: ag.antigen.reference()), fill="transparent", outline=self.grey, outline_width=1, size=self.reference_antigen_size)
        painter.modify(pchart.select_antigens(lambda ag: not ag.antigen.reference()), fill=self.grey, outline=self.grey, outline_width=1, size=self.test_antigen_size)
        painter.modify(pchart.select_antigens(lambda ag: ag.passage.is_egg()), shape="egg")
        painter.modify(pchart.select_antigens(lambda ag: bool(ag.reassortant)), rotation=0.5)
        painter.modify(pchart.select_all_sera(), fill="transparent", outline=self.grey, outline_width=1, size=self.serum_size)
        painter.modify(pchart.select_sera(lambda sr: sr.passage.is_egg()), shape="uglyegg")

    def draw_mark_with_mapi(self, painter: acmacs.ChartDraw, mark_sera: bool = True, report: bool = False):
        pchart = painter.chart()
        marked = {"ag": [], "sr": []}
        for en in self.mapi_data:
            selector = en["select"]

            def clade_match(clade, clades):
                if clade[0] != "!":
                    return clade in clades
                else:
                    return clade[1:] not in clades

            def sel_ag_sr(ag_sr):
                good = True
                if good and selector.get("sequenced"):
                    good = ag_sr.sequenced()
                if good and (clade := selector.get("clade")):
                    good = clade_match(clade, ag_sr.clades())
                if good and (clade_all := selector.get("clade-all")):
                    good = all(clade_match(clade, ag_sr.clades()) for clade in clade_all)
                if good and (aas := selector.get("amino-acid") or selector.get("amino_acid")):
                    good = ag_sr.sequence_aa().matches_all(aas)
                return good

            def sel_ag(ag):
                return sel_ag_sr(ag.antigen)

            def sel_sr(sr):
                return sel_ag_sr(sr.serum)

            selected = pchart.select_antigens(sel_ag)
            marked["ag"].append({"selected": selected, "selector": selector, "modify_args": en["modify_antigens"]})
            painter.modify(selected, **{k: v for k, v in en["modify_antigens"].items() if v})
            if mark_sera:
                selected = pchart.select_sera(sel_sr)
                marked["sr"].append({"selected": selected, "selector": selector, "modify_args": en["modify_sera"]})
                painter.modify(selected, **{k: v for k, v in en["modify_sera"].items() if v})

        def report_marked(marked, names_to_report):
            if names_to_report:
                for ag_sr in ["ag", "sr"]:
                    if marked[ag_sr]:
                        print(f'{ag_sr.upper()} ({len(marked[ag_sr])})')
                        for en in marked[ag_sr]:
                            print(f'{en["selected"].size():6d}  {en["selector"]} {en["modify_args"]}')
                            # reported = en["selected"].report_list(format="{AG_SR} {no0} {full_name}") # [:max_names_to_report]
                            reported = en["selected"].report_list(format="{ag_sr} {no0:5d} {full_name}")[:names_to_report]
                            for rep in reported:
                                print("     ", rep)

        if report:
            report_marked(marked=marked, names_to_report=10)

    def load_mapi(self, chart: acmacs.Chart):
        if not self.mapi_data:
            mapi_filename = Path(os.getcwd()).parents[1].joinpath(self.subtype_lineage_to_mapi_name.get(chart.subtype_lineage(), "unknown"))
            if mapi_filename.exists():
                if not self.mapi_key:
                    self.mapi_key = self.subtype_lineage_to_mapi_key.get(chart.subtype_lineage())
                if self.mapi_key:
                    try:
                        data = json.load(mapi_filename.open())[self.mapi_key]
                    except json.decoder.JSONDecodeError as err:
                        raise ErrorJSON(mapi_filename, err)

                    def make_mapi_entry(en: dict) -> dict:
                        return {
                            "select": en["select"],
                            "modify_antigens": {
                                "fill": en.get("fill", "").replace("{clade-pale}", ""),
                                "outline": en.get("outline", "").replace("{clade-pale}", ""),
                                "outline_width": en.get("outline_width"),
                                "order": en.get("order"),
                                "legend": en.get("legend") and acmacs.PointLegend(format=en["legend"].get("label"), show_if_none_selected=en["legend"].get("show_if_none_selected")),
                            },
                            "modify_sera": {
                                "outline": en.get("fill", "").replace("{clade-pale}", ""),
                                "outline_width": 3,
                            },
                        }
                    self.mapi_data = [make_mapi_entry(en) for en in data if en.get("N") == "antigens"]
                    # pprint.pprint(self.mapi_data, width=200)

# ======================================================================

class Snapshot:

    def __init__(self):
        self.filename = Path("snapshot.json")
        if self.filename.exists():
            self.data = json.load(self.filename.open())
        else:
            self.data = {"sections": []}
        self.current_section = None

    def __del__(self):
        self.save()
        self.generate_html()

    def save(self):
        json.dump(self.data, self.filename.open("w"), indent=2)

    def section(self, cmd):
        for sec in self.data["sections"]:
            if sec["name"] == cmd.__name__:
                sec["images"] = []
                self.current_section = sec
        if not self.current_section:
            self.current_section = {"name": cmd.__name__, "doc": cmd.__doc__, "images": []}
        self.data["sections"].append(self.current_section)

    def number_of_images(self) -> int:
        return len(self.current_section["images"])

    def add_image(self, pdf: Path, ace: Path):
        self.current_section["images"].append({"pdf": str(pdf), "ace": str(ace)})

    def generate_html(self):
        pass

# ======================================================================

class Error (Exception):
    pass

# ----------------------------------------------------------------------

class ErrorJSON (Error):

    def __init__(self, filename: Union[str,Path], err: json.decoder.JSONDecodeError):
        self.message = f"{filename}:{err.lineno}:{err.colno}: {err.msg}"

    def __str__(self) -> str:
        return self.message

# ----------------------------------------------------------------------
