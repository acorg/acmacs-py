import sys, json
from pathlib import Path
try:
    import acmacs
except:
    import pprint
    print(f"> ERROR: cannot import acmacs\n{pprint.pformat(sys.path)}", file=sys.stderr)
    raise

# ======================================================================

sGrey = "#D0D0D0"
sTestAntigenSize = 10
sReferenceAntigenSize = sTestAntigenSize * 1.5
sSerumSize = sReferenceAntigenSize

class Error (RuntimeError): pass

# ======================================================================

class MapiSettings:

    def __init__(self, *sources):
        self.data = {}
        self.load(*sources)

    def load(self, *sources):
        for src in sources:
            self.data.update(json.load(Path(src).open()))

    def names(self):
        return self.data.keys()

    def chart_draw_modify(self, drw :acmacs.ChartDraw, mapi_key :str):
        if data := self.data.get(mapi_key):
            for en in data:
                if isinstance(en, str):
                    self.chart_draw_modify(drw, en)
                elif isinstance(en, dict):
                    if en.get("N") == "antigens":
                        args = {
                            "fill": en.get("fill", "").replace("{clade-pale}", ""),
                            "outline": en.get("outline", "").replace("{clade-pale}", ""),
                            "outline_width": en.get("outline_width"),
                            "order": en.get("order"),
                            "legend": en.get("legend") and acmacs.PointLegend(format=en["legend"].get("label"), show_if_none_selected=en["legend"].get("show_if_none_selected")),
                        }

                        selector = en["select"]

                        def clade_match(clade, cldes):
                            if clade[0] != "!":
                                return clade in cldes
                            else:
                                return clade[1:] not in cldes

                        def sel(ag):
                            good = True
                            if good and selector.get("sequenced"):
                                good = ag.antigen.sequenced()
                            if good and (clade := selector.get("clade")):
                                good = clade_match(clade, ag.antigen.clades())
                            if good and (clade_all := selector.get("clade-all") or selector.get("clade_all")):
                                good = all(clade_match(clade, ag.antigen.clades()) for clade in clade_all)
                            if good and (aas := selector.get("amino-acid") or selector.get("amino_acid")):
                                good = ag.antigen.sequence_aa().matches_all(aas)
                            return good
                        selected = drw.chart().select_antigens(sel)
                        # print(f"===== {selected.size()} {selector} {args}")
                        drw.modify(selected, **{k: v for k, v in args.items() if v})
                else:
                    raise Error(f"mapi key \"{mapi_key}\": unrecognized entry: {en}")

    def chart_draw_reset(self, drw :acmacs.ChartDraw, grey :str = sGrey, test_antigen_size :float = sTestAntigenSize, reference_antigen_size :float = sReferenceAntigenSize, serum_size :float = sSerumSize):
        chart = drw.chart()
        drw.modify(chart.select_antigens(lambda ag: ag.antigen.reference()), fill="transparent", outline=grey, size=reference_antigen_size)
        drw.modify(chart.select_antigens(lambda ag: not ag.antigen.reference()), fill=grey, outline=grey, size=test_antigen_size)
        drw.modify(chart.select_antigens(lambda ag: ag.passage.is_egg()), shape="egg")
        drw.modify(chart.select_antigens(lambda ag: bool(ag.reassortant)), rotation=0.5)
        drw.modify(chart.select_all_sera(), fill="transparent", outline=grey, size=serum_size)
        drw.modify(chart.select_sera(lambda sr: sr.passage.is_egg()), shape="uglyegg")

# ======================================================================
