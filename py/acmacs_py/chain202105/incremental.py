from acmacs_py import *
from .chain_base import ChainBase, IndividualMapMaker
from .error import WrongFirstChartInIncrementalChain
import acmacs

# ----------------------------------------------------------------------

class IncrementalChain (ChainBase):

    def __init__(self, tables :list[Path], name :str, **kwargs):
        super().__init__(name=name, **kwargs)
        self.tables = tables

    def run(self, runner, chain_setup):
        map_path = self.first_map(runner=runner, chain_setup=chain_setup)
        for table_no, table in enumerate(self.tables[1:], start=1):
            merger = IncrementalMergeMaker(chain_setup)
            merger.make(previous_merge=map_path, new_table=table, output_dir=self.output_directory(), output_prefix=self.output_prefix(table_no), runner=runner)
            # run in parallel:
            #   individual with col bases from merge
            #   incremental
            #   scratch
            break

    def first_map(self, runner, chain_setup):
        chart  = acmacs.Chart(self.tables[0])
        if chart.titers().number_of_layers() < 2:
            first_map_filename = IndividualMapMaker(chain_setup).make(source=self.tables[0], output_root_dir=self.output_root_dir, runner=runner)
        elif chart.number_of_projections() > 0:
            first_map_filename = self.tables[0]
        else:
            raise WrongFirstChartInIncrementalChain(f"""{self.tables[0]} has layers but no projections""")
        output_dir = self.output_directory()
        first_map_path = Path(os.path.relpath(first_map_filename, output_dir))
        map_path = output_dir.joinpath(f"""{self.output_prefix(0)}{chart.date()}{self.tables[0].suffix}""")
        if self.unlink_if_wrong_symlink(map_path, first_map_path):
            map_path.symlink_to(first_map_path)
        return map_path

    def output_prefix(self, table_no):
        return f"""{table_no:03d}."""

    def unlink_if_wrong_symlink(self, link, target):
        """returns if link was removed or did not exist"""
        if not link.exists():
            return True
        if not link.is_symlink():
            raise WrongFirstChartInIncrementalChain(f"""{link} is not a symlink (needs to be symlink to {target}""")
        if link.readlink() != target:
            link.unlink()
            return True
        return False            # link points to target

# ----------------------------------------------------------------------

class IncrementalMergeMaker:

    def __init__(self, chain_setup):
        self.chain_setup = chain_setup
        self.output_path = None

    def make(self, previous_merge :Path, new_table :Path, output_dir :Path, output_prefix :str, runner):
        previous_chart = acmacs.Chart(previous_merge)
        chart_to_add = acmacs.Chart(new_table)
        if previous_chart.titers().number_of_layers() < 2:
            merge_date = f"""{previous_chart.date()}-{chart_to_add.date()}"""
        else:
            merge_date = f"""{previous_chart.date().split("-")[0]}-{chart_to_add.date()}"""
        self.output_path = output_dir.joinpath(output_dir, f"""{output_prefix}{merge_date}{previous_merge.suffix}""")
        if not self.output_path.exists():
            merge, report = acmacs.merge(previous_chart, chart_to_add, type="incremental", combine_cheating_assays=self.chain_setup.combine_cheating_assays())
            print(report)
            merge.export(self.output_path, sys.argv[0])
        else:
            print(f"""{self.output_path} up to date""")
        # extract column bases

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
