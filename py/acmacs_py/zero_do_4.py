# 0do.py v4 support, e.g. ssm report custom

import sys, os, json, subprocess, re, pprint, traceback
from pathlib import Path
from contextlib import contextmanager
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
        parser.add_argument("--help-api", action='store_true', default=False)
        parser.add_argument("command", nargs='?')
        args = parser.parse_args()
        if args.command_list:
            print("\n".join(main_commands()))
            exit(0)
        if args.help_api:
            help(Zd)
            help(Painter)
            help(Snapshot)
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


    test_antigen_size = 10
    reference_antigen_size = test_antigen_size * 1.5
    serum_size = test_antigen_size * 1.5
    grey = "#D0D0D0"

    def __init__(self, zd, chart: acmacs.Chart, chart_filename: Path, mapi_key: Union[str, None], legend_offset: List[float] = [-10, -10]):
        super().__init__(chart)
        self.orig_chart_filename = chart_filename
        self.zd = zd
        self.mapi_key = mapi_key
        self.draw_reset(mark_with_mapi=True)
        self.legend(offset=legend_offset)

    def __bool__(self):
        "return if not done"
        return not self.is_done()

    def make(self, pdf: Path, ace: Path = None, title: bool = True, open: bool = False):
        if title:
            self.title(lines=["{lab} {virus-type/lineage-subset} {assay-no-hi-cap} " + f"{self.chart().projection(0).stress(recalculate=True):.4f}"], remove_all_lines=True)
        self.calculate_viewport()
        self.draw(pdf, open=open)
        print(f">>> {pdf}")
        if ace:
            self.chart().export(ace)
            print(f">>> {ace}")

    def move(self, *args, pre_snapshot: bool = True, **kwargs):
        if pre_snapshot:
            self.snapshot()
        super().move(*args, **kwargs)

    def relax(self, pre_snapshot: bool = True):
        if pre_snapshot:
            self.snapshot()
        self.projection().relax()

    def draw_reset(self, mark_with_mapi: bool = True, clade_pale: Union[str, None] = None, mark_sera: bool = True, report: bool = False):
        self.legend(show=False) # remove old legend stuff
        pchart = self.chart()
        self.modify(pchart.select_antigens(lambda ag: ag.antigen.reference()), fill="transparent", outline=self.grey, outline_width=1, size=self.reference_antigen_size)
        self.modify(pchart.select_antigens(lambda ag: not ag.antigen.reference()), fill=self.grey, outline=self.grey, outline_width=1, size=self.test_antigen_size)
        self.modify(pchart.select_antigens(lambda ag: ag.passage.is_egg()), shape="egg")
        self.modify(pchart.select_antigens(lambda ag: bool(ag.reassortant)), rotation=0.5)
        self.modify(pchart.select_all_sera(), fill="transparent", outline=self.grey, outline_width=1, size=self.serum_size)
        self.modify(pchart.select_sera(lambda sr: sr.passage.is_egg()), shape="uglyegg")
        if mark_with_mapi and (mapi := self.mapi()):
            mapi.mark(painter=self, chart=pchart, clade_pale=clade_pale, mark_sera=mark_sera, report=report)

    def mapi(self):
        return self.zd.mapi.get(self.mapi_key)

    def snapshot(self, overwrite: bool = True, export_ace: bool = True, open: bool = False, done: bool = False) -> Path:
        """returns ace filename, even if export_ace==False"""
        pdf, ace_filename = self.zd.generate_filenames(done=done)
        stck = "".join(traceback.format_stack())
        if overwrite or not pdf.exists():
            self.make(pdf=pdf, ace=ace_filename if export_ace and self.zd.export_ace else None, open=open)
        self.zd.snapshot_data.add_image(pdf=pdf, ace=ace_filename)
        return ace_filename

    def procrustes(self, secondary: Union[Path, None] = None, threshold: float = 0.3, overwrite: bool = True, open: bool = False):
        pdf, ace = self.zd.generate_filenames(infix=f"pc-{secondary.stem if secondary else 'orig'}")
        if overwrite or not pdf.exists():
            secondary_chart = self.zd.get_chart(secondary or self.orig_chart_filename, load_mapi=False)[0]
            self.procrustes_arrows(common=acmacs.CommonAntigensSera(self.chart(), secondary_chart), secondary_chart=secondary_chart, threshold=threshold)
            self.make(pdf=pdf, title=False, open=open)
            self.remove_procrustes_arrows()
            self.title(remove_all_lines=True)
        self.zd.snapshot_data.add_image(pdf=pdf, ace=ace)

    def compare_sequences(self, set1, set2, overwrite: bool = False, open: bool = True) -> Path:
        fn = self.zd.generate_filenames(suffixes=[".html"])[0]
        print(f">>> {fn}  (compare_sequences)")
        if overwrite or not fn.exists():
            super().compare_sequences(set1=set1, set2=set2, output=fn, open=open)
        self.zd.snapshot_data.add_image(html=fn)
        return fn

    def remove_done(self):
        pdf, ace_filename = self.zd.generate_filenames(done=True)
        pdf.unlink(missing_ok=True)
        ace_filename.unlink(missing_ok=True)

    def is_done(self) -> bool:
        pdf, ace_filename = self.zd.generate_filenames(done=True)
        return ace_filename.exists()

    def final_ace(self) -> Path:
        return self.zd.generate_filenames(done=True)[1]

    def link(self, comment=None):
        source_path = re.sub(r"^.+/custom/", "../custom/", str(self.final_ace().resolve()))
        cmnt = f"[{comment}] " if comment else ""
        print(f">> {cmnt}ln -sf {source_path} {self.chart().subtype_lineage().lower()}-{self.chart().assay_rbc().lower()}-{self.chart().lab().lower()}.ace")

# ======================================================================

class Mapi:

    subtype_lineage_to_mapi_name = {"H1": "h1pdm.mapi", "H3": "h3.mapi", "BVICTORIA": "bvic.mapi", "BYAMAGATA": "byam.mapi"}
    subtype_lineage_to_mapi_key = {"H1": "loc:clade-155-156-A(H1N1)2009pdm", "H3": "loc:clades-A(H3N2)-all", "BVICTORIA": "loc:clades-B/Vic", "BYAMAGATA": "loc:clades-B/Yam"}

    def __init__(self, filename: Path, key: str):
        if filename.exists() and key:
            try:
                data = json.load(filename.open())[key]
            except json.decoder.JSONDecodeError as err:
                raise ErrorJSON(filename, err)

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
            self.data = [make_mapi_entry(en) for en in data if en.get("N") == "antigens"]
            # pprint.pprint(self.data)
        else:
            print(f">> {filename} does not exist")
            self.data = None

    def mark(self, painter: Painter, chart: acmacs.Chart, clade_pale: Union[str, None] = None, mark_sera: bool = True, selected_antigens=None, selected_sera=None, report: bool = True, names_to_report: int = 10):
        marked = {"ag": [], "sr": []}
        for en in (self.data or []):
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
                return (not selected_antigens or ag.no in selected_antigens.indexes()) and sel_ag_sr(ag.antigen)

            def sel_sr(sr):
                return (not selected_sera or sr.no in selected_sera.indexes()) and sel_ag_sr(sr.serum)

            def apply_clade_pale(key, value):
                if clade_pale and key in ["fill", "outline"] and value != "transparent":
                    value += clade_pale
                return value

            selected = chart.select_antigens(sel_ag)
            marked["ag"].append({"selected": selected, "selector": selector, "modify_args": en["modify_antigens"]})
            painter.modify(selected, **{k: apply_clade_pale(k, v) for k, v in en["modify_antigens"].items() if v})
            if mark_sera:
                selected = chart.select_sera(sel_sr)
                marked["sr"].append({"selected": selected, "selector": selector, "modify_args": en["modify_sera"]})
                painter.modify(selected, **{k: apply_clade_pale(k, v) for k, v in en["modify_sera"].items() if v})

        def report_marked(marked):
            if names_to_report:
                for ag_sr in ["ag", "sr"]:
                    if marked[ag_sr]:
                        print(f'{ag_sr.upper()} ({len(marked[ag_sr])})')
                        for en in marked[ag_sr]:
                            print(f'{en["selected"].size():6d}  {en["selector"]} {en["modify_args"]}')
                            # reported = en["selected"].report_list(format="{AG_SR} {no0} {full_name}") # [:max_names_to_report]
                            reported = en["selected"].report_list(format="{ag_sr} {no0:5d} {full_name} [{date}]")[:names_to_report]
                            for rep in reported:
                                print("     ", rep)

        if report:
            report_marked(marked=marked)

# ======================================================================

class Snapshot:

    def __init__(self):
        self.filename = Path("snapshot.json")
        if self.filename.exists():
            self.data = json.load(self.filename.open())
        else:
            self.data = {"sections": []} # {"sections": [{"name": str, "doc": str, "pnt": [{"images": [{"pdf": str, "ace": str, "html": str}, ...]} ...]}, ...]}
        self.current_section = None

    def __del__(self):
        self.save()
        self.generate_html()

    def save(self):
        json.dump(self.data, self.filename.open("w"), indent=2)

    def section(self, cmd = None) -> str:
        if cmd:
            for sec in self.data["sections"]:
                if sec["name"] == cmd.__name__:
                    sec["pnt"] = []
                    self.current_section = sec
                    self.current_pnt = None
            if not self.current_section:
                self.current_section = {"name": cmd.__name__, "doc": cmd.__doc__, "pnt": []}
                self.data["sections"].append(self.current_section)
            Path(self.current_section["name"]).mkdir(exist_ok=True)
        return self.current_section["name"]

    def add_pnt(self) -> Path:
        self.current_section["pnt"].append({"images": []})
        self.current_pnt = len(self.current_section["pnt"]) - 1
        pnt_dir = self.pnt_dir()
        pnt_dir.mkdir(exist_ok=True)
        return pnt_dir

    def pnt_dir(self):
        if self.current_pnt is None:
            raise RuntimeError("Snapshot: no current_pnt")
        return Path(self.current_section["name"], f"{self.current_pnt:02d}")

    def number_of_images(self) -> int:
        return len(self.current_section["pnt"][self.current_pnt]["images"])

    def generate_filenames(self, infix: str, suffixes: List[str] = [".pdf", ".ace"], done: bool = False) -> List[Path]:
        if done:
            stem = "99"
        else:
            stem = f"{self.number_of_images():02d}"
        if infix:
            stem += f".{infix}"
        pnt_dir = self.pnt_dir()
        return [pnt_dir.joinpath(stem + suffix) for suffix in suffixes]

    def add_image(self, **args): # pdf: Union[Path, None] = None, ace: Union[Path, None] = None, html: Union[Path, None] = None):
        self.current_section["pnt"][self.current_pnt]["images"].append({k: str(v) for k, v in args.items() if v})

    def generate_html(self):
        pass

# ======================================================================

class Zd:

    def __init__(self, cmd):
        self.mapi_data = None
        self.snapshot_data = Snapshot()
        self.chart_filename = None
        self.export_ace = True
        self.section(cmd)
        self._chart_cache = {}  # {Path: Chart}
        self.mapi = {} # {key: Mapi}

    @contextmanager
    def open(self, filename: Path, mapi_filename: Union[Path, None, bool] = None, mapi_key: Union[str, None] = None, legend_offset: List[float] = [-10, -10], not_done: bool = False, open_final: bool = True, populate_seqdb: bool = True):
        chart, mapi_key = self.get_chart(filename=filename, mapi_filename=mapi_filename, mapi_key=mapi_key, populate_seqdb=populate_seqdb)
        self.snapshot_data.add_pnt()
        pnt = Painter(zd=self, chart=chart, chart_filename=filename, mapi_key=mapi_key, legend_offset=legend_offset)
        if not_done:
            pnt.remove_done()
        yield pnt
        if not pnt.is_done():
            pnt.snapshot(done=True, open=open_final)

    def section(self, cmd):
        self.snapshot_data.section(cmd)

    def chart_merge(self, sources: List[Path], output_infix: str = None, match: str = "strict", incremental: bool = False, combine_cheating_assays: bool = True):
        sources = [fn.expanduser() for fn in sources]
        first_chart = acmacs.Chart(sources[0])
        last_chart = acmacs.Chart(sources[-1])
        output_filename = Path(self.snapshot_data.section(), f"{last_chart.subtype_lineage()[:4].lower()}-{last_chart.assay_rbc().lower()}-{last_chart.lab().lower()}-{first_chart.date().split('-')[0]}-{last_chart.date().split('-')[-1]}{output_infix or ''}.ace")
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

    def get_chart(self, filename: Path, mapi_filename: Union[Path, None, bool] = None, mapi_key: Union[str, None] = None, load_mapi: bool = True, populate_seqdb: bool = True) -> tuple[acmacs.Chart, Union[str, None]]:
        self.chart_filename = filename
        try:
            chart = self._chart_cache[filename]
        except KeyError:
            chart = self._chart_cache[filename] = acmacs.Chart(filename)
            if populate_seqdb:
                chart.populate_from_seqdb()
        if load_mapi:
            subtype_lineage = chart.subtype_lineage()
            if mapi_filename is not False:
                mapi_filename = mapi_filename or Path(os.getcwd()).parents[1].joinpath(Mapi.subtype_lineage_to_mapi_name.get(subtype_lineage, "unknown"))
                mapi_key = mapi_key or Mapi.subtype_lineage_to_mapi_key.get(subtype_lineage)
                if mapi_key and not self.mapi.get(mapi_key):
                    self.mapi[mapi_key] = Mapi(filename=mapi_filename, key=mapi_key)
        return chart, mapi_key

    def generate_filenames(self, infix: str = None, suffixes: List[str] = [".pdf", ".ace"], done: bool = False) -> List[Path]:
        return self.snapshot_data.generate_filenames(infix=infix, suffixes=suffixes, done=done)

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
