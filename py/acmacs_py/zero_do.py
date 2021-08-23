# 0do.py support, e.g. ssm report custom

import sys, os, time, datetime, subprocess, json, argparse, pprint, traceback
from pathlib import Path
from typing import List, Union, Callable
import acmacs

# ----------------------------------------------------------------------

class Locals: pass
class Error (Exception): pass

class ErrorJSON (Error):

    def __init__(self, filename: Union[str,Path], err: json.decoder.JSONDecodeError):
        self.message = f"{filename}:{err.lineno}:{err.colno}: {err.msg}"

    def __str__(self) -> str:
        return self.message

# ----------------------------------------------------------------------

class Do:

    def __init__(self, chart_filename: Path = None, draw_final=False, default_command="do", loop=True, make_index_html=True, mapi_filename: Path = None, page_title: str = None, self_name: str = "zd"):
        self.painter = None
        self.set_chart(chart_filename)
        self.draw_final = draw_final
        self.make_index_html = make_index_html
        self._title = True
        self._mark_sera = True
        self._mapi_filename = mapi_filename
        self._mapi_key = None
        self._first_reset = True
        self._html_data = []
        self._page_title = page_title
        self.self_name = self_name
        self.self_mtime = self._get_argv0_mtime()
        command = parse_command_line(default_command=default_command)
        self._loop(command=command, loop=loop)

    def reset(self, reset_draw: bool = True, reset_plot: bool = True):
        self._html_data = []
        if reset_draw:
            self.painter = acmacs.ChartDraw(self._chart) if self._chart else None
        if reset_plot:
            self.reset_plot()
            if self.painter:
                self.mark_clades(names_to_report=10 if self._first_reset else 0)
            self._first_reset = False

    def snapshot(self, infix: str = None, title: str = None, overwrite: bool = True, reset=False, export_ace: bool = True, open: bool = True, new_section=None, pc_with: Path = None):
        "draw, export ace (optionally), make html entry"
        pdf_filename = self.draw(infix=infix, overwrite=overwrite, reset=reset, open=open)
        ace_filename = self.chart_filename.with_suffix(f".{infix}.ace") if infix else self.chart_filename
        if export_ace and (overwrite or not ace_filename.exists()):
            self.chart().export(ace_filename)
        if new_section:
            self.html_new_section(title=new_section)
        elif not self._html_data:
            self.html_new_section(title="")
        self._html_data[-1]["pdfs"].append({"title": title, "pdf": pdf_filename, "ace": ace_filename})
        if pc_with:
            pc_pdf_filename = self.draw(infix=f"{infix}-pc" if infix else "pc", overwrite=overwrite, reset=reset, pc_with=pc_with, open=open)
            self._html_data[-1]["pdfs"].append({"title": "procrustes", "pdf": pc_pdf_filename})

    def html_new_section(self, title: str):
        self._html_data.append({"title": title, "pdfs": []})

    def draw(self, infix: str = None, overwrite: bool = True, reset: bool = False, pc_with: Path = None, open: bool = True):
        output_filename = self.chart_filename.with_suffix((f".{infix}" if infix else "") + ".pdf")
        if overwrite or not output_filename.exists():
            if reset:
                self.reset(reset_draw=not self.painter, reset_plot=True)
            if pc_with:
                secondary_chart = acmacs.Chart(pc_with)
                self.painter.procrustes_arrows(common=acmacs.CommonAntigensSera(self.chart(), secondary_chart), secondary_chart=secondary_chart, threshold=0.3)
                self.painter.legend(offset=[-10, -10])
            elif self._title:
                self.painter.title(lines=["{lab} {virus-type/lineage-subset} {assay-no-hi-cap} " + f"{self.chart().projection(0).stress(recalculate=True):.4f}"], remove_all_lines=True)
                self.painter.legend(offset=[10, 40])
            else:
                self.painter.legend(offset=[10, 10])
            self.painter.calculate_viewport()
            self.painter.draw(output_filename, open=open)
        return output_filename

    def chart(self):
        return self.painter.chart()

    def set_chart(self, filename: Union[Path,None], chart: Union[acmacs.Chart,None] = None):
        self.chart_filename = filename
        self._chart = chart or (acmacs.Chart(self.chart_filename) if self.chart_filename else None)
        self.painter = None
        return self

    def reset_plot(self, test_antigen_size=10, reference_antigen_size=None, serum_size=None, grey="#D0D0D0"):
        if reference_antigen_size is None:
            reference_antigen_size = test_antigen_size * 1.5
        if serum_size is None:
            serum_size = test_antigen_size * 1.5
        if self.painter:
            self.painter.modify(self.chart().select_antigens(lambda ag: ag.antigen.reference()), fill="transparent", outline=grey, outline_width=1, size=reference_antigen_size)
            self.painter.modify(self.chart().select_antigens(lambda ag: not ag.antigen.reference()), fill=grey, outline=grey, outline_width=1, size=test_antigen_size)
            self.painter.modify(self.chart().select_antigens(lambda ag: ag.passage.is_egg()), shape="egg")
            self.painter.modify(self.chart().select_antigens(lambda ag: bool(ag.reassortant)), rotation=0.5)
            self.painter.modify(self.chart().select_all_sera(), fill="transparent", outline=grey, outline_width=1, size=serum_size)
            self.painter.modify(self.chart().select_sera(lambda sr: sr.passage.is_egg()), shape="uglyegg")

    def mark_clades(self, names_to_report=10):
        if self._mapi_filename:
            try:
                data = json.load(self._mapi_filename.open())[self._mapi_key or self._mapi_clades_key_vr()]
            except json.decoder.JSONDecodeError as err:
                raise ErrorJSON(self._mapi_filename, err)
            marked = {"ag": [], "sr": []}
            for en in data:
                if en.get("N") == "antigens":
                    args = {
                        "fill": en.get("fill", "").replace("{clade-pale}", ""),
                        "outline": en.get("outline", "").replace("{clade-pale}", ""),
                        "outline_width": en.get("outline_width"),
                        "order": en.get("order"),
                        "legend": en.get("legend") and acmacs.PointLegend(format=en["legend"].get("label"), show_if_none_selected=en["legend"].get("show_if_none_selected"))
                    }

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

                    selected = self.chart().select_antigens(sel_ag)
                    marked["ag"].append({"selected": selected, "selector": selector, "modify_args": args})
                    # print(f"AGs {selected.size()} {selector} {args}")
                    self.painter.modify(selected, **{k: v for k, v in args.items() if v})

                    if self._mark_sera:
                        args_sera = {
                            "outline": args["fill"],
                            "outline_width": 3,
                        }
                        selected = self.chart().select_sera(sel_sr)
                        marked["sr"].append({"selected": selected, "selector": selector, "modify_args": args_sera})
                        # print(f"SRs {selected.size()} {selector} {args_sera}")
                        self.painter.modify(selected, **{k: v for k, v in args_sera.items() if v})
            self._report_marked(title="Marked by clade", marked=marked, names_to_report=names_to_report)

    def _report_marked(self, title, marked, names_to_report):
        if names_to_report:
            # pprint.pprint(marked)
            for ag_sr in ["ag", "sr"]:
                if marked[ag_sr]:
                    print(f'{ag_sr.upper()} {title} ({len(marked[ag_sr])})')
                    for en in marked[ag_sr]:
                        print(f'{en["selected"].size():6d}  {en["selector"]} {en["modify_args"]}')
                        # reported = en["selected"].report_list(format="{AG_SR} {no0} {full_name}") # [:max_names_to_report]
                        reported = en["selected"].report_list(format="{ag_sr} {no0:5d} {full_name}")[:names_to_report]
                        for rep in reported:
                            print("     ", rep)

    def show_title(self, show):
        self._title = show
        return self

    def mark_sera(self, mark):
        self._mark_sera = mark
        return self

    # def mapi_filename(self, mapi_filename: Path):
    #     self._mapi_filename = mapi_filename
    #     return self

    def mapi_key(self, mapi_key: str):
        self._mapi_key = mapi_key
        return self

    # ----------------------------------------------------------------------

    @classmethod
    def glob_bash(cls, pattern) -> List[Path]:
        "return [Path] by matching using bash, e.g. ~/ac/whocc-tables/h3-hint-cdc/h3-hint-cdc-{2020{0[4-9],1},2021}*.ace"
        return sorted(Path(fn) for fn in subprocess.check_output(f"ls -1 {pattern}", text=True, shell=True).strip().split("\n"))

    @classmethod
    def chart_merge(cls, sources: List[Path], output_filename: Path, match: str = "strict", incremental: bool = False):
        if not output_filename.exists():
            subprocess.check_call(["chart-merge",
                                   "--match", match,
                                   "--merge-type", "incremental" if incremental else "simple",
                                   "-o", str(output_filename),
                                   *(str(src) for src in sources)])
        return output_filename

    def relax(self, source_filename: Path, mcb: str="none", num_optimizations: int = 1000, num_dimensions: int = 2, keep_projections: int =10, grid: bool = True,
              reorient: Union[str, Path, acmacs.Chart] = None, incremental: bool = False, draw_relaxed: bool = True, add_to_painter: bool = True, populate_seqdb: bool = False,
              disconnect_antigens: Callable[[acmacs.SelectionDataAntigen], bool] = None, disconnect_sera: Callable[[acmacs.SelectionDataSerum], bool] = None,
              output_infix: str = None, slurm: bool = False):
        """disconnect_antigens, disconnect_antigens: callable, e.g. lambda ag"""
        infix = output_infix or f"{mcb}-{num_optimizations/1000}k"
        result_filename = source_filename.with_suffix(f".{infix}.ace")
        if not result_filename.exists():
            if slurm:
                if incremental:
                    raise Error("relax incremental is not supported with slurm=True")
                reorient_args = ["--reorient", str(reorient)] if reorient else []
                grid_args = ["--grid"] if grid else []
                no_draw_args = [] if draw_relaxed else ["--no-draw"]
                subprocess.check_call(["slurm-relax", *no_draw_args, "-o", str(result_filename), str(source_filename), "-n", str(num_optimizations), "-d", str(num_dimensions), "-m", mcb, "-k", str(keep_projections), *grid_args, *reorient_args])
                if add_to_painter:
                    self.set_chart(result_filename)
            else:
                chart = acmacs.Chart(source_filename)
                antigens_to_disconnect = sera_to_disconnect = None
                if disconnect_antigens or disconnect_sera:
                    if incremental:
                        raise Error("relax incremental cannot handle disconnected points")
                    print("disconnecting antigens/sera", file=sys.stderr)
                    antigens_to_disconnect = chart.select_antigens(disconnect_antigens, report=True) if disconnect_antigens else None
                    sera_to_disconnect = chart.select_sera(disconnect_sera, report=True) if disconnect_sera else None
                if populate_seqdb:
                    chart.populate_from_seqdb()
                print(f"relaxing chart {chart.description()}")
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
                if add_to_painter or draw_relaxed:
                    self.set_chart(filename=result_filename, chart=chart)
                    if draw_relaxed:
                        self.reset()
        elif add_to_painter or draw_relaxed:
            self.set_chart(result_filename)
            if draw_relaxed:
                self.reset()
        return result_filename

    # ----------------------------------------------------------------------

    def _make_index_html(self, open=False):
        # pprint.pprint(self._html_data)
        if self._html_data:
            with Path("index.html").open("w") as ff:
                ff.write(sHtmlHeader % {"title": self._page_title})
                for section in self._html_data:
                    print(f'<h3>{section["title"]}</h3>', file=ff)
                    print(f'<table><tr>', file=ff)
                    for en in section["pdfs"]:
                        if en.get("ace") and en["ace"].exists():
                            print(f'  <td>\n    <div class="ac-tabs">', file=ff)
                            print(f'      <div class="tabcontent pdf tab-default" title="{en["title"]}" src="{en["pdf"]}"></div>', file=ff)
                            print(f'      <div class="tabcontent ace-view-widget" title="Interactive viewer" src="{en["ace"]}"></div>', file=ff)
                            print(f'    </div>\n  </td>', file=ff)
                        else:
                            print(f'  <td>\n    <b>{en["title"]}</b><br />\n    <div class="pdf" src="{en["pdf"]}"></div>\n  </td>', file=ff)
                    print(f'</tr></table>', file=ff)
                ff.write(sHtmlFooter)
            if open:
                subprocess.run(["open-and-back-to-emacs", "index.html"], check=False)

    # ----------------------------------------------------------------------

    def _loop(self, command: str, loop: bool):
        for iter_no in range(1000000):
            try:
                mod = self._reload(iter_no)
                self.reset()
                getattr(mod, command)()
                if self.draw_final and self.painter:
                    self.draw(infix=command + ".final", overwrite=True)
                if self.make_index_html:
                    self._make_index_html()
                if not loop:
                    break
            except KeyboardInterrupt:
                print(">> KeyboardInterrupt", file=sys.stderr)
                submarine()
                time.sleep(0.5)
                sys.exit(2)
            except Error as err:
                print(err, file=sys.stderr)
                blow()
            except Exception as err:
                print(f"> {type(err)}: {err}\n{traceback.format_exc()}", file=sys.stderr)
                blow()

    def _reload(self, iter_no):
        if iter_no:
            print(f">>> waiting {iter_no} {datetime.datetime.now()}")
            self._wait_until_updated()
            print(f">>> reloading {datetime.datetime.now()}")
        locls = Locals()
        # globls = {**vars(sys.modules["__main__"])}
        # globls.update({**globals(), "__name__": Path(sys.argv[0]).name, self.self_name: self})
        globls = {**vars(sys.modules["__main__"]), **globals(), "__name__": Path(sys.argv[0]).name, self.self_name: self}
        exec(open(sys.argv[0]).read(), globls, locls.__dict__)
        return locls

    def _wait_until_updated(self):
        current_self_mtime = self._get_argv0_mtime()
        while self.self_mtime >= current_self_mtime:
            time.sleep(0.3)
            current_self_mtime = self._get_argv0_mtime()
        self.self_mtime = current_self_mtime

    def _get_argv0_mtime(self):
        return os.stat(sys.argv[0]).st_mtime

    # ----------------------------------------------------------------------

    def _mapi_clades_key_vr(self):
        stl = self.chart().subtype_lineage().lower()
        if stl == "h1":
            return "loc:clade-155-156-A(H1N1)2009pdm"
        elif stl == "h3":
            return "loc:clades-A(H3N2)-all"
        elif stl == "bvictoria":
            return "loc:clades-B/Vic"
        elif stl == "byamagata":
            return "loc:clades-B/Yam"
        else:
            raise RuntimeError(f"_mapi_clades_key_vr: unsupported subtype_lineage \"{stl}\"")

# ======================================================================

sHtmlHeader = """<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8" />
    <style>
      table td { padding: 0 1em; }
    </style>
    <script src="/js/acmacs-d/map-draw/pdf.js"></script> <!-- <div class="pdf" src="image.pdf"></div> -->
    <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
    <script src="/js/acmacs-d/map-draw/tabs.js"></script>
    <link rel="stylesheet" href="/js/acmacs-d/map-draw/tabs.css">
    <script src="/js/acmacs-d/map-draw/ace-view/201807/widget.js"></script>
    <title>%(title)s</title>
  </head>
  <body>
    <h2>%(title)s</h2>
"""

sHtmlFooter = """
  </body>
</html>
"""

# ======================================================================

def parse_command_line(default_command):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--command-list", action='store_true', default=False)
    parser.add_argument("command", nargs='?', default=default_command)
    args = parser.parse_args()
    if args.command_list:
        print("\n".join(name for name, value in vars(sys.modules["__main__"]).items() if name[0] != "_" and name != "Path" and callable(value)))
        exit(0)
    return args.command

def submarine():
    subprocess.run(["aiff-play", "/System/Library/Sounds/Submarine.aiff"], stderr=subprocess.DEVNULL, check=False)

def blow():
    subprocess.run(["aiff-play", "/System/Library/Sounds/Blow.aiff"], stderr=subprocess.DEVNULL, check=False)

# ======================================================================
# old interface (2021-08-06)
# ======================================================================

def draw(draw, output_filename :Path, overwrite=True, reset_plotspec=False, mapi_filename :Path = None, mapi_key="vr", mark_sera=False, title=True, show_legend=True, open=True):
    if overwrite or not output_filename.exists():
        if reset_plotspec:
            reset(draw)
            clades(draw, mapi_filename=mapi_filename, mapi_key=mapi_key, mark_sera=mark_sera)
        if title:
            if isinstance(title, str):
                draw.title(lines=[title], remove_all_lines=True)
            else:
                draw.title(lines=["{lab} {virus-type/lineage-subset} {assay-no-hi-cap} " + f"{draw.chart().projection(0).stress(recalculate=True):.4f}"], remove_all_lines=True)
            if show_legend:
                draw.legend(offset=[10, 40])
        if not show_legend:
            draw.legend(show=False)
        draw.calculate_viewport()
        draw.draw(output_filename, open=open)

# ----------------------------------------------------------------------

def reset(draw, test_antigen_size=10, reference_antigen_size=None, serum_size=None, grey="#D0D0D0"):
    chart = draw.chart()
    if reference_antigen_size is None:
        reference_antigen_size = test_antigen_size * 1.5
    if serum_size is None:
        serum_size = test_antigen_size * 1.5
    draw.modify(chart.select_antigens(lambda ag: ag.antigen.reference()), fill="transparent", outline=grey, outline_width=1, size=reference_antigen_size)
    draw.modify(chart.select_antigens(lambda ag: not ag.antigen.reference()), fill=grey, outline=grey, outline_width=1, size=test_antigen_size)
    draw.modify(chart.select_antigens(lambda ag: ag.passage.is_egg()), shape="egg")
    draw.modify(chart.select_antigens(lambda ag: bool(ag.reassortant)), rotation=0.5)
    draw.modify(chart.select_all_sera(), fill="transparent", outline=grey, outline_width=1, size=serum_size)
    draw.modify(chart.select_sera(lambda sr: sr.passage.is_egg()), shape="uglyegg")
    draw.legend(offset=[10, 10])

# ----------------------------------------------------------------------

def clades(draw, mapi_filename :Path, mapi_key="vr", mark_sera=False):
    chart = draw.chart()
    data = json.load(mapi_filename.open())[mapi_clades_key_vr(chart) if mapi_key == "vr" else mapi_key]
    for en in data:
        if en.get("N") == "antigens":
            args = {
                "fill": en.get("fill", "").replace("{clade-pale}", ""),
                "outline": en.get("outline", "").replace("{clade-pale}", ""),
                "outline_width": en.get("outline_width"),
                "order": en.get("order"),
                "legend": en.get("legend") and acmacs.PointLegend(format=en["legend"].get("label"), show_if_none_selected=en["legend"].get("show_if_none_selected"))
            }

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

            selected = chart.select_antigens(sel_ag)
            print(f"AGs {selected.size()} {selector} {args}")
            draw.modify(selected, **{k: v for k, v in args.items() if v})

            if mark_sera:
                args_sera = {
                    "outline": args["fill"],
                    "outline_width": 3,
                }
                selected = chart.select_sera(sel_sr)
                print(f"SRs {selected.size()} {selector} {args}")
                draw.modify(selected, **{k: v for k, v in args_sera.items() if v})

# ----------------------------------------------------------------------

def mapi_clades_key_vr(chart):
    stl = chart.subtype_lineage().lower()
    if stl == "h1":
        return "loc:clade-155-156-A(H1N1)2009pdm"
    elif stl == "h3":
        return "loc:clades-A(H3N2)-all"
    elif stl == "bvictoria":
        return "loc:clades-B/Vic"
    elif stl == "byamagata":
        return "loc:clades-B/Yam"
    else:
        raise RuntimeError(f"mapi_clades_key_vr: unsupported subtype_lineage \"{stl}\"")

# ======================================================================

def relax_slurm(source :Path, mcb="none", num_optimizations=1000, num_dimensions=2, keep_projections=10, grid=True, reorient=None, draw_relaxed=True):
    output_filename = source.with_suffix(f".{mcb}-relaxed.ace")
    if not output_filename.exists():
        if reorient:
            reorient_args = ["--reorient", str(reorient)]
        else:
            reorient_args = []
        if grid:
            grid_args = ["--grid"]
        else:
            grid_args = []
        if draw_relaxed:
            no_draw_args = []
        else:
            no_draw_args = ["--no-draw"]
        subprocess.check_call(["slurm-relax", *no_draw_args, str(source), "-n", str(num_optimizations), "-d", str(num_dimensions), "-m", mcb, "-k", str(keep_projections), *grid_args, *reorient_args])
    return output_filename

# ----------------------------------------------------------------------

def chart_merge(sources :[Path], output_filename :Path, match="strict"):
    return Do.chart_merge(sources=sources, output_filename=output_filename, match=match)

# ----------------------------------------------------------------------

def glob_bash(pattern):
    "return [Path] by matching using bash, e.g. ~/ac/whocc-tables/h3-hint-cdc/h3-hint-cdc-{2020{0[4-9],1},2021}*.ace"
    return Do.glob_bash(pattern)

# ======================================================================

def main_loop(chart_filename :Path = None, draw_final=False, default_command="do", command_choices=None):
    if not command_choices:
        command_choices = [default_command]
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--command-list", action='store_true', default=False)
    parser.add_argument("command", nargs='?', default=default_command, choices=command_choices)
    args = parser.parse_args()

    if args.command_list:
        print("\n".join(name for name, value in vars(sys.modules["__main__"]).items() if name[0] != "_" and name != "Path" and callable(value)))
        exit(0)

    chart = acmacs.Chart(chart_filename) if chart_filename else None
    while True:
        try:
            mod = reload()
            draw = acmacs.ChartDraw(chart) if chart else None
            getattr(mod, args.command)(draw)
            if draw_final and draw:
                sys.modules[__name__].draw(draw, output_filename=chart_filename.with_suffix(".0do.pdf").name, overwrite=True)
        except KeyboardInterrupt:
            print(">> KeyboardInterrupt")
            exit(2)
        except Exception as err:
            print(f"> {type(err)}: {err}\n{traceback.format_exc()}", file=sys.stderr)
            blow()

# ----------------------------------------------------------------------

def reload():
    print(f">>> waiting {datetime.datetime.now()}")
    wait_until_updated()
    print(f">>> reloading {datetime.datetime.now()}")
    locls = Locals()
    globls = {**globals(), "__name__": sys.argv[0]}
    exec(open(sys.argv[0]).read(), globls, locls.__dict__)
    return locls

# ----------------------------------------------------------------------

self_mtime = None

def wait_until_updated():
    global self_mtime
    current_self_mtime = os.stat(sys.argv[0]).st_mtime
    while self_mtime and self_mtime >= current_self_mtime:
        time.sleep(0.3)
        current_self_mtime = os.stat(sys.argv[0]).st_mtime
    self_mtime = current_self_mtime

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
