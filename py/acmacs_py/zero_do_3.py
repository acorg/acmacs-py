# 0do.py support, e.g. ssm report custom

import sys, os, json, pprint, traceback
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
        cmd = getattr(sys.modules["__main__"], command)
        zd = Zd(cmd)
        return cmd(zd)
    except Error as err:
        print(f"> {err}", file=sys.stderr)
        return 1
    except Exception as err:
        print(f"> {type(err)}: {err}\n{traceback.format_exc()}", file=sys.stderr)
        return 2

# ======================================================================

class Painter (acmacs.ChartDraw):

    subtype_lineage_to_mapi_name = {"H1": "h1pdm.mapi", "H3": "h3.mapi", "BVICTORIA": "bvic.mapi", "BYAMAGATA": "byam.mapi"}
    subtype_lineage_to_mapi_key = {"H1": "loc:clade-155-156-A(H1N1)2009pdm", "H3": "loc:clades-A(H3N2)-all", "BVICTORIA": "loc:clades-B/Vic", "BYAMAGATA": "loc:clades-B/Yam"}

    test_antigen_size = 10
    reference_antigen_size = test_antigen_size * 1.5
    serum_size = test_antigen_size * 1.5
    grey = "#D0D0D0"

    def __init__(self, chart: acmacs.Chart, mapi_filename: Path = None, mapi_key: str = None):
        super().__init__(chart)
        self.mapi_filename = mapi_filename
        self.mapi_key = mapi_key
        self.draw_reset()
        self.draw_mark_with_mapi()
        self.legend(offset=[-10, -10])

    def make(self, pdf: Path, ace: Path = None, title: bool = True, open: bool = False):
        if title:
            self.title(lines=["{lab} {virus-type/lineage-subset} {assay-no-hi-cap} " + f"{self.chart().projection(0).stress(recalculate=True):.4f}"], remove_all_lines=True)
        self.calculate_viewport()
        self.draw(pdf, open=open)
        print(f">>> {pdf}")
        if ace:
            self.chart().export(ace)
            print(f">>> {ace}")

    def relax(self):
        self.projection().relax()

    def draw_reset(self):
        pchart = self.chart()
        self.modify(pchart.select_antigens(lambda ag: ag.antigen.reference()), fill="transparent", outline=self.grey, outline_width=1, size=self.reference_antigen_size)
        self.modify(pchart.select_antigens(lambda ag: not ag.antigen.reference()), fill=self.grey, outline=self.grey, outline_width=1, size=self.test_antigen_size)
        self.modify(pchart.select_antigens(lambda ag: ag.passage.is_egg()), shape="egg")
        self.modify(pchart.select_antigens(lambda ag: bool(ag.reassortant)), rotation=0.5)
        self.modify(pchart.select_all_sera(), fill="transparent", outline=self.grey, outline_width=1, size=self.serum_size)
        self.modify(pchart.select_sera(lambda sr: sr.passage.is_egg()), shape="uglyegg")

    def draw_mark_with_mapi(self, mark_sera: bool = True, report: bool = False):
        pchart = self.chart()
        marked = {"ag": [], "sr": []}
        for en in self.load_mapi():
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
            self.modify(selected, **{k: v for k, v in en["modify_antigens"].items() if v})
            if mark_sera:
                selected = pchart.select_sera(sel_sr)
                marked["sr"].append({"selected": selected, "selector": selector, "modify_args": en["modify_sera"]})
                self.modify(selected, **{k: v for k, v in en["modify_sera"].items() if v})

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

    def load_mapi(self):
        subtype_lineage = self.chart().subtype_lineage()
        mapi_filename = self.mapi_filename or Path(os.getcwd()).parents[1].joinpath(self.subtype_lineage_to_mapi_name.get(subtype_lineage, "unknown"))
        if mapi_filename.exists():
            if not self.mapi_key:
                self.mapi_key = self.subtype_lineage_to_mapi_key.get(subtype_lineage)
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
                return [make_mapi_entry(en) for en in data if en.get("N") == "antigens"]
        return []

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

    def section(self, cmd = None):
        if cmd:
            for sec in self.data["sections"]:
                if sec["name"] == cmd.__name__:
                    sec["images"] = []
                    self.current_section = sec
            if not self.current_section:
                self.current_section = {"name": cmd.__name__, "doc": cmd.__doc__, "images": []}
                self.data["sections"].append(self.current_section)
        return self.current_section["name"]

    def number_of_images(self) -> int:
        return len(self.current_section["images"])

    def generate_filename(self, ace: Path, infix: bool, infix2: str = None) -> tuple[Path, Path]:
        s_infix = self.section()
        if infix:
            s_infix += f".{self.number_of_images():02d}"
        if infix2:
            s_infix += f".{infix2}"
        prefix = Path(ace.name)
        return prefix.with_suffix(f".{s_infix}.pdf"), prefix.with_suffix(f".{s_infix}.ace")

    def add_image(self, pdf: Path, ace: Path):
        self.current_section["images"].append({"pdf": str(pdf), "ace": str(ace)})

    def generate_html(self):
        pass

# ======================================================================

class Zd:

    def __init__(self, cmd):
        self.mapi_key = None
        self.mapi_data = None
        self.snapshot_data = Snapshot()
        self.chart_filename = None
        self.painter = None
        self.export_ace = True
        self.section(cmd)

    def open(self, filename: Path, mapi_filename: Path = None, mapi_key: str = None) -> Painter:
        self.chart_filename = filename
        self.painter = Painter(chart=acmacs.Chart(filename), mapi_filename=mapi_filename, mapi_key=mapi_key)
        self.snapshot(overwrite=False, export_ace=False)
        return self.painter

    def section(self, cmd):
        self.snapshot_data.section(cmd)

    def snapshot(self, overwrite: bool = True, infix: bool = True, export_ace: bool = True, open: bool = False):
        pdf, ace = self.snapshot_data.generate_filename(ace=self.chart_filename, infix=infix)
        if overwrite or not pdf.exists():
            self.painter.make(pdf=pdf, ace=ace if export_ace and self.export_ace else None, open=open)
        self.snapshot_data.add_image(pdf=pdf, ace=ace)
        return ace

    def snapshot_procrustes(self, secondary: Path, threshold: float = 0.3, overwrite: bool = True, infix: bool = True, open: bool = False):
        pdf, ace = self.snapshot_data.generate_filename(ace=self.chart_filename, infix=infix, infix2="pc")
        if overwrite or not pdf.exists():
            secondary_chart = acmacs.Chart(secondary)
            self.painter.procrustes_arrows(common=acmacs.CommonAntigensSera(self.painter.chart(), secondary_chart), secondary_chart=secondary_chart, threshold=threshold)
            self.painter.make(pdf=pdf, title=False, open=open)
            self.painter.remove_procrustes_arrows()
        self.snapshot_data.add_image(pdf=pdf, ace=ace)

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
