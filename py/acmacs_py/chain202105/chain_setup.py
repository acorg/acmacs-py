from acmacs_py import *

"""
In the chain directory create Setup.py with the following sample content:

----------------------------------------------------------------------
from acmacs_py import *
from acmacs_py.chain202105 import ChainSetupDefault, IndividualTableMaps

class ChainSetup (ChainSetupDefault):

    def chains(self):
        global IndividualTableMaps
        tables = self.collect_individual_tables()
        return [IndividualTableMaps(tables[1:])]

    def collect_individual_tables(self):
        import acmacs
        return sorted(fn for fn in self.source_dir().glob("*.ace") if acmacs.Chart(fn).number_of_sera() > 2)

    def source_dir(self):
        return Path("/syn/eu/ac/whocc-tables/h3-hint-cdc")

    def reorient_to(self):
        return None

    # def number_of_optimizations(self):
    #     return 1000
    #
    # def number_of_dimensions(self):
    #     return 2
    #
    # def minimum_column_basis(self):
    #     return "none"
    #
    # def projections_to_keep(self):
    #     return 10
    # def disconnect_having_few_titers(self):
    #     return True


"""

# ----------------------------------------------------------------------

class ChainSetupDefault:

    def __init__(self):
        pass

    def chains(self):
        raise KnownError("override ChainSetup.chains() in Setup.py")

    def number_of_optimizations(self):
        return 1000

    def number_of_dimensions(self):
        return 2

    def minimum_column_basis(self):
        return "none"

    def reorient_to(self):
        return None

    def projections_to_keep(self):
        return 10

    def disconnect_having_few_titers(self):
        return True

# class ThisIncrementalChain (IncrementalChain):

#     sReInclude = re.compile(r"-(20191119|2019112|201912|202)", re.I)
#     sReExclude = re.compile(r"exclude", re.I)

#     def source_tables(self):
#         "returns [Path]"
#         whocc_tables = Path(os.environ["HOME"], "ac", "whocc-tables")
#         source_dir = whocc_tables.joinpath("h3-hi-guinea-pig-vidrl")
#         return sorted(pathname for pathname in source_dir.iterdir()
#                       if pathname.suffix == ".ace"
#                       and self.sReInclude.search(pathname.name)
#                       and not self.sReExclude.search(pathname.name))

#     def incremental(self):
#         "returns if making incremetal merge map at each step requested"
#         return True

#     def scratch(self):
#         "returns if making map from scratch at each step requested"
#         return True

#     def prefer_for_incremental_merge(self, depends):
#         "returns one of depends to force choosing incremental or from scratch merge from the previous step. returns None to choose based on stress"
#         # print(f"prefer_for_incremental_merge: {depends}")
#         return None

#     # def chart_loaded(self, chart):
#     #     "hook to preprocess chart on loading, e.g. remove antigens/sera of wrong lineage"
#     #     if chart.number_of_projections() == 0:
#     #         wrong_lineage = None
#     #         chart.remove_antigens_sera(antigens=chart.antigen_indexes().filter_lineage(wrong_lineage), sera=chart.serum_indexes().filter_lineage(wrong_lineage))
#     #     return chart

#     # def combine_cheating_assays(self):
#     #     return False

#     # def threads(self):
#     #     return 16

#     # def number_of_optimizations_per_run(self):
#     #     return 56

#     # def email(self):
#     #     return "eu@antigenic-cartography.org"

#     # def state_filename(self):
#     #     return Path("state.json")

#     # def sleep_interval_when_not_ready(self):
#     #     "in seconds (htcondor only)"
#     #     return 10 # in seconds

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
