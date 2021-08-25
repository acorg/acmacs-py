# 0do.py support, e.g. ssm report custom

import sys, os, json, subprocess, pprint, traceback
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

    def __init__(self, chart: acmacs.Chart, mapi_filename: Path = None, mapi_key: str = None, legend_offset: List[float] = [-10, -10]):
        super().__init__(chart)
        self.mapi_filename = mapi_filename
        self.mapi_key = mapi_key
        self.draw_reset()
        self.draw_mark_with_mapi()
        self.legend(offset=legend_offset)

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
        print(f">>> loading mapi from {mapi_filename}")
        if mapi_filename.exists():
            if not self.mapi_key:
                self.mapi_key = self.subtype_lineage_to_mapi_key.get(subtype_lineage)
            print(f">>> mapi key {self.mapi_key}")
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
                mapi_data = [make_mapi_entry(en) for en in data if en.get("N") == "antigens"]
                # pprint.pprint(mapi_data)
                return mapi_data
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

    def open(self, filename: Path, mapi_filename: Path = None, mapi_key: str = None, legend_offset: List[float] = [-10, -10], export_ace: bool = False, open_pdf: bool = False) -> Painter:
        self.chart_filename = filename
        chart = acmacs.Chart(filename)
        chart.populate_from_seqdb()
        self.painter = Painter(chart=chart, mapi_filename=mapi_filename, mapi_key=mapi_key, legend_offset=legend_offset)
        self.snapshot(overwrite=False, export_ace=export_ace, open=open_pdf)
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
        pdf, ace = self.snapshot_data.generate_filename(ace=self.chart_filename, infix=infix, infix2=f"pc-{secondary.stem}")
        if overwrite or not pdf.exists():
            secondary_chart = acmacs.Chart(secondary)
            self.painter.procrustes_arrows(common=acmacs.CommonAntigensSera(self.painter.chart(), secondary_chart), secondary_chart=secondary_chart, threshold=threshold)
            self.painter.make(pdf=pdf, title=False, open=open)
            self.painter.remove_procrustes_arrows()
            self.painter.title(remove_all_lines=True)
        self.snapshot_data.add_image(pdf=pdf, ace=ace)

    def chart_merge(cls, sources: List[Path], output_infix: str = None, match: str = "strict", incremental: bool = False, combine_cheating_assays: bool = True):
        first_chart = acmacs.Chart(sources[0])
        last_chart = acmacs.Chart(sources[-1])
        output_filename = Path(f"{last_chart.subtype_lineage()[:4].lower()}-{last_chart.assay_rbc().lower()}-{last_chart.lab().lower()}-{first_chart.date().split('-')[0]}-{last_chart.date().split('-')[-1]}{output_infix or ''}.ace")
        if not output_filename.exists():
            subprocess.check_call(["chart-merge",
                                   "--match", match,
                                   "--merge-type", "incremental" if incremental else "simple",
                                   "--combine-cheating-assays" if combine_cheating_assays else "--no-combine-cheating-assays",
                                   "-o", str(output_filename),
                                   *(str(src) for src in sources)])
            print(f">>> {output_filename}")
        return output_filename

    def glob_bash(self, pattern) -> List[Path]:
        "return [Path] by matching using bash, e.g. ~/ac/whocc-tables/h3-hint-cdc/h3-hint-cdc-{2020{0[4-9],1},2021}*.ace"
        return sorted(Path(fn) for fn in subprocess.check_output(f"ls -1 {pattern}", text=True, shell=True).strip().split("\n"))

    def relax(self, source_filename: Path, mcb: str="none", num_optimizations: int = 1000, num_dimensions: int = 2, keep_projections: int = 10, grid: bool = True,
              reorient: Union[str, Path, acmacs.Chart] = None, incremental: bool = False, populate_seqdb: bool = False,
              disconnect_antigens: Callable[[acmacs.SelectionDataAntigen], bool] = None, disconnect_sera: Callable[[acmacs.SelectionDataSerum], bool] = None,
              output_infix: str = None, slurm: bool = False):
        """disconnect_antigens, disconnect_antigens: callable, e.g. lambda ag"""
        infix = output_infix or f"{mcb}-{num_optimizations//1000}k"
        result_filename = source_filename.with_suffix(f".{infix}.ace")
        if not result_filename.exists():
            if slurm:
                if incremental:
                    raise Error("relax incremental is not supported with slurm=True")
                reorient_args = ["--reorient", str(reorient)] if reorient else []
                grid_args = ["--grid"] if grid else []
                no_draw_args = ["--no-draw"]
                subprocess.check_call(["slurm-relax", *no_draw_args, "-o", str(result_filename), str(source_filename), "-n", str(num_optimizations), "-d", str(num_dimensions), "-m", mcb, "-k", str(keep_projections), *grid_args, *reorient_args])
            else:
                chart = acmacs.Chart(source_filename)
                antigens_to_disconnect = sera_to_disconnect = None
                if disconnect_antigens or disconnect_sera:
                    if incremental:
                        raise Error("relax incremental cannot handle disconnected points")
                    print(">>> disconnecting antigens/sera", file=sys.stderr)
                    antigens_to_disconnect = chart.select_antigens(disconnect_antigens, report=True) if disconnect_antigens else None
                    sera_to_disconnect = chart.select_sera(disconnect_sera, report=True) if disconnect_sera else None
                if populate_seqdb:
                    chart.populate_from_seqdb()
                print(f">>> relaxing chart {chart.description()} in {num_dimensions}d mcb:{mcb} {num_optimizations} times")
                if incremental:
                    chart.relax_incremental(number_of_optimizations=num_optimizations, remove_source_projection=True)
                else:
                    chart.relax(number_of_dimensions=num_dimensions, number_of_optimizations=num_optimizations, minimum_column_basis=mcb, disconnect_antigens=antigens_to_disconnect, disconnect_sera=sera_to_disconnect)
                if grid:
                    chart.grid_test()
                chart.keep_projections(keep_projections)
                if reorient:
                    if isinstance(reorient, (str, Path)):
                        reorient = acmacs.Chart(reorient)
                    chart.orient_to(master=reorient)
                chart.export(result_filename)
            print(f">>> {result_filename}")
        return result_filename

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
