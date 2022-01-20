# 0do.py v5 support, e.g. ssm report custom

import sys, os, json, subprocess, re, pprint, traceback, inspect
from pathlib import Path
from contextlib import contextmanager
from collections.abc import Callable
import acmacs

# ======================================================================

class Error (Exception):
    pass

# ----------------------------------------------------------------------

class ErrorJSON (Error):

    def __init__(self, filename: str|Path, err: json.decoder.JSONDecodeError):
        self.message = f"{filename}:{err.lineno}:{err.colno}: {err.msg}"

    def __str__(self) -> str:
        return self.message

# ======================================================================

class Zd:

    def __init__(self, cmd):
        self.num_slots = 0
        self.section(cmd)

    def section(self, cmd):
        pass # print("section", cmd)

    # ----------------------------------------------------------------------

    def slot(self, func: Callable[[any], dict]) -> dict:
        slot_name = func.__qualname__.replace("<locals>", f"{self.num_slots:02d}")
        self.num_slots += 1
        with self.slot_context(slot_name) as sl:
            return func(sl)

    @contextmanager
    def slot_context(self, slot_name: str):
        slot = Slot(self, slot_name)
        try:
            yield slot
        finally:
            slot.finalize()

# ----------------------------------------------------------------------

class Slot:

    def __init__(self, zd: Zd, slot_name: str):
        self.zd = zd
        self.slot_name = slot_name
        self.step = 0
        # chart
        self.chart_filename = None
        self.chart = None
        self.chart_draw = None
        # settings
        self.open_final_plot = True
        # self.export_step_ace = True
        self.export_final_ace = True
        self.final_step = 99
        self.chart_link_infix = None # for print_final_ace_link

    def finalize(self):
        if self.chart or self.chart_filename:
            self.plot(step=self.final_step, open=self.open_final_plot)
            if self.export_final_ace and (chart := self.final_chart()):
                ace = self.final_ace()
                chart.export(ace)
                self.print_final_ace_link(comment=f"final {self.slot_name}")

    # ----------------------------------------------------------------------

    def modify(self, selected: acmacs.SelectedAntigens|acmacs.SelectedSera, fill: str = None, outline: str = None, outline_width: float = None, show: bool = None, shape: str = None, size: float = None, aspect: float = None, rotation: float = None, order: str = None, label: dict = None, legend: dict = None):
        """legend: {"format": "", "show_if_none_selected": False, "replace": False, "show": True}
        """
        self.make_chart_draw()
        kwargs = {arg: value for arg, value in locals().items() if arg not in ["self", "selected"] and value is not None}
        if "label" in kwargs:
            print(f">> slot.modify label is not implemented {kwargs['label']}")
        if "legend" in kwargs:
            kwargs["legend"] = acmacs.PointLegend(**kwargs["legend"])
            # print(f">> slot.modify legend is not implemented {kwargs['legend']}")
        self.chart_draw.modify(select=selected, **kwargs)

    def move(self, selected: acmacs.SelectedAntigens|acmacs.SelectedSera, to: list[float] = None, flip_over_line: list[list[float]]|acmacs.Figure = None, snapshot: bool = True):
        self.make_chart_draw()
        kwargs = {arg: value for arg, value in locals().items() if arg not in ["self", "selected", "snapshot"] and value is not None}
        if isinstance(kwargs.get("flip_over_line"), list):
            kwargs["flip_over_line"] = self.chart_draw.figure(vertices=kwargs["flip_over_line"], close=False, coordinates_relative_to="viewport-origin")
        self.chart_draw.move(select=selected, **kwargs)
        if snapshot:
            self.plot()

    def path(self, path: list[list[float]], outline: str = None, outline_width: float = 1.0, fill: str = None, close: bool = True, coordinates_relative_to: str = "viewport-origin") -> acmacs.Figure:
        self.make_chart_draw()
        fig = self.chart_draw.figure(vertices=path, close=close, coordinates_relative_to=coordinates_relative_to)
        if outline or fill:
            self.chart_draw.path(fig, outline=outline or fill, outline_width=outline_width, fill=fill or "transparent")
        return fig

    def select_antigens(self, predicate: Callable = None, report: bool|int = 20, modify: dict = None, snapshot: bool = True):
        "if predicate=None (default), select all"
        return self._select_ag_sr("antigens", predicate=predicate, report=report, modify=modify, snapshot=snapshot)

    def select_sera(self, predicate: Callable = None, report: bool|int = 20, modify: dict = None, snapshot: bool = True):
        "if predicate=None (default), select all"
        return self._select_ag_sr("sera", predicate=predicate, report=report, modify=modify, snapshot=snapshot)

    def _select_ag_sr(self, ag_sr: str, predicate: Callable, report: bool|int, modify: dict, snapshot: bool):
        "if predicate=None, select all"
        self.make_chart_draw()
        if predicate is None:
            selected = getattr(self.chart_draw.chart(), "select_all_" + ag_sr)()
            print(f">>> {len(selected)} {ag_sr} all selected")
        else:
            selected = getattr(self.chart_draw.chart(), "select_" + ag_sr)(predicate=predicate, report=False)
            print(f">>> {len(selected)} {ag_sr} selected using [{inspect.getsource(predicate).strip()}]")
        if report:
            for no, ag in enumerate(selected):
                if not isinstance(report, bool) and isinstance(report, int) and no >= report: # bool value is instance of int
                    print(f"    ... {len(selected) - report} {ag_sr} more")
                    break
                print(f"    {no:3d} {ag[0]:5d} {ag[1].name_full()}")
        if modify:
            self.modify(selected=selected, **{k: v for k, v in modify.items() if k[0] != "?"})
        if snapshot:
            self.plot()
        return selected

    def legend(self, **args):
        """offset=[-10, -10], show=True, label_size=10, point_size=8, title=[], type="legend-point-label"|"continent-map" """
        self.make_chart_draw()
        self.chart_draw.legend(**args)

    def title(self, **args):
        """lines=[], show=True, text_color="black", remove_all_lines=False
        subtitutions: {name} {virus} {virus-type} {lineage} {lineage-cap} {subset} {subset-up} {virus-type/lineage} {virus-type/lineage-subset} {virus-type-lineage-subset-short-low}
                     {assay-full} {assay-cap} {assay-low} {assay-no-hi-low} {assay-no-hi-cap} {lab} {lab-low} {rbc} {assay-rbc} {assay-low} {table-date} {num-ag} {num-sr} {num-layers}
                     {minimum-column-basis} {mcb} {stress}
        """
        self.make_chart_draw()
        self.chart_draw.title(**args)

    def viewport(self, x: float, y: float, size: float):
        self.make_chart_draw()
        self.chart_draw.viewport(x=x, y=y, size=size)

    # ----------------------------------------------------------------------

    def rotate(self, angle: float):
        self.make_chart_draw()
        self.chart_draw.rotate(angle)

    def flip(self, direction: str = "ew"):
        self.make_chart_draw()
        self.chart_draw.flip(direction)

    # ----------------------------------------------------------------------

    test_antigen_size = 10
    reference_antigen_size = test_antigen_size * 1.5
    serum_size = test_antigen_size * 1.5
    grey = "#D0D0D0"

    def reset_plot_spec(self, snapshot: bool = False):
        self.make_chart_draw()
        self.chart_draw.legend(show=False) # remove old legend stuff
        self.chart_draw.remove_paths_circles()
        self.select_antigens(lambda ag: ag.antigen.reference(), modify={"fill": "transparent", "outline": self.grey, "outline_width": 1, "size": self.reference_antigen_size}, report=False, snapshot=False)
        self.select_antigens(lambda ag: not ag.antigen.reference(), modify={"fill": self.grey, "outline": self.grey, "outline_width": 1, "size": self.test_antigen_size}, report=False, snapshot=False)
        self.select_antigens(lambda ag: ag.passage.is_egg(), modify={"shape": "egg"}, report=False, snapshot=False)
        self.select_antigens(lambda ag: bool(ag.reassortant), modify={"rotation": 0.5}, report=False, snapshot=False)
        self.select_sera(modify={"fill": "transparent", "outline": self.grey, "outline_width": 1, "size": self.serum_size}, report=False, snapshot=False)
        self.select_sera(lambda sr: sr.passage.is_egg(), modify={"shape": "uglyegg"}, report=False, snapshot=snapshot)

    def color_by_clade(self, mapi_dir: Path = None, snapshot: bool = False):
        if mapi_dir is not None:
            MarkWithMapi(mapi_dir=mapi_dir, subtype=self.chart.subtype_lineage()).mark(self.chart_draw)
        if snapshot:
            self.plot()

    # ----------------------------------------------------------------------

    def relax(self, snapshot: bool = True):
        self.make_chart_draw()
        self.chart_draw.projection().relax()
        if snapshot:
            self.plot()

    def merge(self, sources: list[Path], match: str = "strict", incremental: bool = False, combine_cheating_assays: bool = True, populate_seqdb: bool = True) -> Path:
        sources = [fn.expanduser() for fn in sources]
        first_chart = acmacs.Chart(sources[0])
        last_chart = acmacs.Chart(sources[-1])
        name_prefix = f"{last_chart.subtype_lineage()[:4].lower()}-{last_chart.assay_rbc().lower()}-{last_chart.lab().lower()}"
        name_dates = f"{first_chart.date().split('-')[0]}-{last_chart.date().split('-')[-1]}"
        output_filename = self.subdir().joinpath(f"{name_prefix}-{name_dates}.ace")
        if incremental:
            merge_type = "incremental"
        else:
            merge_type = "simple"
        if combine_cheating_assays:
            cca = "--combine-cheating-assays"
        else:
            cca = "--no-combine-cheating-assays"
        if not output_filename.exists():
            subprocess.check_call(["chart-merge",
                                   "--match", match,
                                   "--merge-type", merge_type,
                                   cca,
                                   "-o", str(output_filename),
                                   *(str(src) for src in sources)])
            if populate_seqdb:
                self.populate_from_seqdb4(output_filename)
            print(f">>> {output_filename}")
        return output_filename

    def relax_chart(self, source_filename: Path, mcb: str="none", num_optimizations: int = 1000, num_dimensions: int = 2, keep_projections: int = 10, grid: bool = True,
              reorient: str|Path|acmacs.Chart = None, incremental: bool = False, populate_seqdb: bool = True,
              disconnect_antigens: Callable[[acmacs.SelectionDataAntigen], bool] = None, disconnect_sera: Callable[[acmacs.SelectionDataSerum], bool] = None,
              slurm: bool = False):
        """disconnect_antigens, disconnect_antigens: callable, e.g. lambda ag"""
        infix = f"{mcb}-{num_optimizations//1000}k"
        result_filename = source_filename.with_suffix(f".{infix}.ace")
        if not result_filename.exists():
            if populate_seqdb:
                self.populate_from_seqdb4(source_filename)
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

    def orient_to(self, master: Path|acmacs.Chart):
        self.make_chart_draw()
        if isinstance(master, (str, Path)):
            master = acmacs.Chart(master)
        self.chart_draw.chart().orient_to(master)

    def glob_bash(self, pattern) -> list[Path]:
        "return [Path] by matching using bash, e.g. ~/ac/whocc-tables/h3-hint-cdc/h3-hint-cdc-{2020{0[4-9],1},2021}*.ace"
        return sorted(Path(fn) for fn in subprocess.check_output(f"ls -1 {pattern}", text=True, shell=True).strip().split("\n"))

    # ----------------------------------------------------------------------

    def plot(self, step: int = None, infix: str = None, open: bool = False):
        self.make_chart_draw()
        if self.chart_draw:
            self.chart_draw.calculate_viewport()
            if step is None:
                step = self.step
                self.step += 1
            if infix:
                infix = f".{infix}"
            else:
                infix = ""
            pdf = self.subdir().joinpath(f"{step:02d}{infix}.pdf")
            self.chart_draw.draw(pdf, open=open)
            print(f">>> {pdf}")

    def procrustes(self, secondary_chart_file: Path = None, threshold: float = 0.3, open: bool = False):
        self.make_chart_draw()
        if self.chart_draw:
            if secondary_chart_file:
                secondary_chart = acmacs.Chart(secondary_chart_file)
            else:
                secondary_chart = acmacs.Chart(self.chart_filename)
            self.chart_draw.procrustes_arrows(common=acmacs.CommonAntigensSera(self.chart_draw.chart(), secondary_chart), secondary_chart=secondary_chart, threshold=threshold)
            self.plot(infix="pc", open=open)
            self.chart_draw.remove_procrustes_arrows()
            self.chart_draw.title(remove_all_lines=True)

    def print_final_ace_link(self, comment: str = None):
        self.make_chart_draw()
        source_path = re.sub(r"^.+/custom/", "../custom/", str(self.final_ace().resolve()))
        cmnt = f"[{comment}] " if comment else ""
        subtype_lineage = self.chart.subtype_lineage().lower()
        if subtype_lineage[:1] == "b":
            subtype_lineage = subtype_lineage[:4]
        if self.chart_link_infix:
            infix = self.chart_link_infix
            if infix[0] != '.':
                infix = f".{infix}"
        else:
            infix = ""
        if subtype_lineage == "h1":
            subtype_lineage = "h1pdm"
        print(f">>> {cmnt}ln -sf {source_path} {subtype_lineage}-{self.chart.assay_rbc().lower()}-{self.chart.lab().lower()}{infix}.ace")

    def final_ace(self) -> Path:
        return self.subdir().joinpath(f"{self.final_step:02d}.ace")

    def final_chart(self) -> acmacs.Chart:
        if self.chart_draw:
            return self.chart_draw.chart()
        else:
            return self.chart

    def make_chart_draw(self):
        if not self.chart_draw:
            if self.chart is None:
                if self.chart_filename is None:
                    raise RuntimeError("slot: chart_filename is not set")
                self.chart = acmacs.Chart(self.chart_filename)
            if self.chart.number_of_projections() > 0:
                self.chart_draw = acmacs.ChartDraw(self.chart)
            elif self.chart_filename:
                print(f">> \"{self.chart_filename}\" has no projections")
        return self.chart_draw

    def subdir(self):
        subd = Path(self.slot_name)
        subd.mkdir(parents=False, exist_ok=True)
        return subd

    def populate_from_seqdb4(self, chart_filename: Path):
        subprocess.check_call([os.path.join(os.environ["AE_ROOT"], "bin", "seqdb-chart-populate"), str(chart_filename)])

# ======================================================================

class MarkWithMapi:

    subtype_lineage_to_mapi_name = {"H1": "h1pdm.mapi", "H3": "h3.mapi", "BVICTORIA": "bvic.mapi", "BYAMAGATA": "byam.mapi"}
    subtype_lineage_to_mapi_key = {"H1": "loc:clade-155-156-A(H1N1)2009pdm", "H3": "loc:clade-v2-A(H3N2)", "BVICTORIA": "loc:clade-v1-B/Vic", "BYAMAGATA": "loc:clades-B/Yam"}

    def __init__(self, mapi_dir: Path = None, subtype: str = None, mapi_file: Path = None, mapi_key: str = None):
        if not mapi_file and mapi_dir and subtype:
            mapi_file = mapi_dir.joinpath(self.subtype_lineage_to_mapi_name[subtype])
        if not mapi_key and subtype:
            mapi_key = self.subtype_lineage_to_mapi_key[subtype]
        if mapi_file and mapi_file.exists() and mapi_key:
            try:
                print(f">>> loading {mapi_file} and extracting \"{mapi_key}\"")
                data = json.load(mapi_file.open())[mapi_key]
            except json.decoder.JSONDecodeError as err:
                raise ErrorJSON(mapi_file, err)

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
            print(f">> \"{mapi_file}\" does not exist or mapi_key is not passed")
            self.data = None

    def mark(self, chart_draw: acmacs.ChartDraw, mark_sera: bool = True): # painter: Painter, chart: acmacs.Chart, clade_pale: Union[str, None] = None, selected_antigens=None, selected_sera=None, report: bool = True, names_to_report: int = 10):
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
                # return (not selected_antigens or ag.no in selected_antigens.indexes()) and sel_ag_sr(ag.antigen)
                return sel_ag_sr(ag.antigen)

            def sel_sr(sr):
                # return (not selected_sera or sr.no in selected_sera.indexes()) and sel_ag_sr(sr.serum)
                return sel_ag_sr(sr.serum)

            def apply_clade_pale(key, value):
                # if clade_pale and key in ["fill", "outline"] and value != "transparent":
                #     value += clade_pale
                return value

            selected = chart_draw.chart().select_antigens(sel_ag)
            marked["ag"].append({"selected": selected, "selector": selector, "modify_args": en["modify_antigens"]})
            chart_draw.modify(selected, **{k: apply_clade_pale(k, v) for k, v in en["modify_antigens"].items() if v})
            if mark_sera:
                selected = chart_draw.chart().select_sera(sel_sr)
                marked["sr"].append({"selected": selected, "selector": selector, "modify_args": en["modify_sera"]})
                chart_draw.modify(selected, **{k: apply_clade_pale(k, v) for k, v in en["modify_sera"].items() if v})

        # def report_marked(marked):
        #     if names_to_report:
        #         for ag_sr in ["ag", "sr"]:
        #             if marked[ag_sr]:
        #                 print(f'{ag_sr.upper()} ({len(marked[ag_sr])})')
        #                 for en in marked[ag_sr]:
        #                     print(f'{en["selected"].size():6d}  {en["selector"]} {en["modify_args"]}')
        #                     # reported = en["selected"].report_list(format="{AG_SR} {no0} {full_name}") # [:max_names_to_report]
        #                     reported = en["selected"].report_list(format="{ag_sr} {no0:5d} {full_name} [{date}]")[:names_to_report]
        #                     for rep in reported:
        #                         print("     ", rep)

        # if report:
        #     report_marked(marked=marked)


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
            help(Slot)
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
