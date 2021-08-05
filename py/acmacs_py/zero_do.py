# 0do.py support, e.g. ssm report custom

import sys, os, time, datetime, subprocess, json, traceback
from pathlib import Path
import acmacs

# ----------------------------------------------------------------------

def draw(draw, output_filename :Path, overwrite=True, reset_plotspec=False, mapi_filename :Path = None, mapi_key="vr"):
    if overwrite or not output_filename.exists():
        if reset_plotspec:
            reset(draw)
            clades(draw, mapi_filename=mapi_filename, mapi_key=mapi_key)
        draw.calculate_viewport()
        draw.draw(output_filename)

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

def clades(draw, mapi_filename :Path, mapi_key="vr"):
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

            def sel(ag):
                good = True
                if good and selector.get("sequenced"):
                    good = ag.antigen.sequenced()
                if good and (clade := selector.get("clade")):
                    good = clade_match(clade, ag.antigen.clades())
                if good and (clade_all := selector.get("clade-all")):
                    good = all(clade_match(clade, ag.antigen.clades()) for clade in clade_all)
                if good and (aas := selector.get("amino-acid") or selector.get("amino_acid")):
                    good = ag.antigen.sequence_aa().matches_all(aas)
                return good
            selected = chart.select_antigens(sel)
            print(f"===== {selected.size()} {selector} {args}")
            draw.modify(selected, **{k: v for k, v in args.items() if v})

# ----------------------------------------------------------------------

def mapi_clades_key_vr(chart):
    stl = chart.subtype_lineage().lower()
    if stl == "h1":
        return "loc:clades-155-156-A(H1N1)2009pdm"
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
    if not output_filename.exists():
        subprocess.check_call(["chart-merge", "--match", match, "-o", str(output_filename), *(str(src) for src in sources)])
    return output_filename

# ----------------------------------------------------------------------

def glob_bash(pattern):
    "return [Path] by matching using bash, e.g. ~/ac/whocc-tables/h3-hint-cdc/h3-hint-cdc-{2020{0[4-9],1},2021}*.ace"
    return sorted(Path(fn) for fn in subprocess.check_output(f"ls -1 {pattern}", text=True, shell=True).strip().split("\n"))

# ======================================================================

def main_loop(chart_filename :Path = None, draw_final=False, default_command="do", command_choices=None):
    if not command_choices:
        command_choices = [default_command]
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", nargs='?', default=default_command, choices=command_choices)
    args = parser.parse_args()

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

class Locals: pass

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

# ----------------------------------------------------------------------

def submarine():
    subprocess.run(["aiff-play", "/System/Library/Sounds/Submarine.aiff"], stderr=subprocess.DEVNULL)

def blow():
    subprocess.run(["aiff-play", "/System/Library/Sounds/Blow.aiff"], stderr=subprocess.DEVNULL)

# ----------------------------------------------------------------------

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
