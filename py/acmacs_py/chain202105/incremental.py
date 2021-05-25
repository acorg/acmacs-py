from acmacs_py import *
from .chain_base import ChainBase, MapMaker, extract_column_bases
from .individual import IndividualTableMaps, IndividualMapMaker, IndividualMapWithMergeColumnBasesMaker
from .error import WrongFirstChartInIncrementalChain
from .log import info
import acmacs

# ----------------------------------------------------------------------

class IncrementalChain (ChainBase):

    def __init__(self, tables :list[Path], name :str, **kwargs):
        super().__init__(name=name, **kwargs)
        self.tables = tables

    def run(self, runner, chain_setup):
        with runner.log_path("incremetal.log").open("a") as log:
            map_path = self.first_map(runner=runner, chain_setup=chain_setup)
            for table_no, table in enumerate(self.tables[1:], start=1):
                merger = IncrementalMergeMaker(chain_setup)
                merger.make(previous_merge=map_path, new_table=table, output_dir=self.output_directory(), output_prefix=self.output_prefix(table_no), runner=runner)
                individual_merge_cb = IndividualMapWithMergeColumnBasesMaker(chain_setup)
                individual_merge_cb.prepare(source=table, merge_column_bases=merger.column_bases, output_dir=self.output_directory(), output_prefix=self.output_prefix(table_no))
                commands = [cmd for cmd in (
                    individual_merge_cb.source and individual_merge_cb.command(source=individual_merge_cb.source, target=individual_merge_cb.target),
                    #   incremental
                    #   scratch
                    ) if cmd]
                runner.run(commands=commands, log=log, add_threads_to_commands=MapMaker.add_threads_to_commands)
                if individual_merge_cb.source:
                    individual_merge_cb.source.unlink()
                break

    def first_map(self, runner, chain_setup):
        chart  = acmacs.Chart(self.tables[0])
        if chart.titers().number_of_layers() < 2:
            source_target = IndividualTableMaps(tables=[self.tables[0]], output_root_dir=self.output_root_dir).run(runner=runner, chain_setup=chain_setup)
            first_map_filename = source_target[0][1]
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
            self.column_bases = extract_column_bases(merge)
        else:
            info(f"""{self.output_path} up to date""")
            self.column_bases = extract_column_bases(acmacs.Chart(self.output_path))
        # pprint.pprint(self.column_bases)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
