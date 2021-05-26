from acmacs_py import *
from . import maps, error
import acmacs

# ----------------------------------------------------------------------

class ChainBase:

    def __init__(self, name="noname", minimum_column_basis="none", output_root_dir=None, **kwargs):
        self.output_root_dir = output_root_dir
        self.name = f"{name}-{minimum_column_basis}"
        self.minimum_column_basis = minimum_column_basis

    def set_output_root_dir(self, output_root_dir :Path):
        self.output_root_dir = output_root_dir

    def output_directory(self):
        if not self.name:
            raise RuntimeError(f"""{self.__class__}: invalid self.name""")
        output_dir = self.output_root_dir.joinpath(self.name)
        output_dir.mkdir(parents=True, exist_ok=True)
        return output_dir

# ----------------------------------------------------------------------

class IndividualTableMapChain (ChainBase):

    def __init__(self, tables :list[Path], **kwargs):
        super().__init__(**kwargs)
        self.tables = tables

    def run(self, runner, chain_setup):
        with runner.log_path("individual.log").open("a") as log:
            maker = maps.IndividualMapMaker(chain_setup, minimum_column_basis=self.minimum_column_basis)
            source_target = [[table, self.output_root_dir.joinpath(maker.individual_map_directory_name(), table.name)] for table in self.tables]
            commands = [cmd for cmd in (maker.command(source=source, target=target) for source, target in source_target) if cmd]
            try:
                runner.run(commands, log=log, add_threads_to_commands=maps.IndividualMapMaker.add_threads_to_commands)
            except error.RunFailed:
                pass            # ignore failures, they will be reported upon making all other maps
            return source_target

# ----------------------------------------------------------------------

class IncrementalChain (ChainBase):

    def __init__(self, tables :list[Path], **kwargs):
        super().__init__(**kwargs)
        self.tables = tables

    def run(self, runner, chain_setup):
        with runner.log_path("incremetal.log").open("a") as log:
            map_path = self.first_map(runner=runner, chain_setup=chain_setup)
            for table_no, table in enumerate(self.tables[1:], start=1):
                merger = IncrementalMergeMaker(chain_setup)
                merger.make(previous_merge=map_path, new_table=table, output_dir=self.output_directory(), output_prefix=self.output_prefix(table_no))
                individual_merge_cb = maps.IndividualMapWithMergeColumnBasesMaker(chain_setup, minimum_column_basis=self.minimum_column_basis)
                individual_merge_cb.prepare(source=table, merge_column_bases=merger.column_bases, output_dir=self.output_directory(), output_prefix=self.output_prefix(table_no))
                commands = [cmd for cmd in (
                    individual_merge_cb.source and individual_merge_cb.command(source=individual_merge_cb.source, target=individual_merge_cb.target),
                    maps.IncrementalMapMaker(chain_setup, minimum_column_basis=self.minimum_column_basis).command(source=merger.output_path, target=merger.output_path.parent.joinpath(merger.output_path.name.replace(".merge.", ".incremental."))),
                    maps.MapMaker(chain_setup, minimum_column_basis=self.minimum_column_basis).command(source=merger.output_path, target=merger.output_path.parent.joinpath(merger.output_path.name.replace(".merge.", ".scratch."))),
                    ) if cmd]
                runner.run(commands=commands, log=log, add_threads_to_commands=maps.MapMaker.add_threads_to_commands)
                if individual_merge_cb.source:
                    individual_merge_cb.source.unlink()
                # avidity test
                # choose incremental vs. scratch
                # degradation check
                break

    def first_map(self, runner, chain_setup):
        chart  = acmacs.Chart(self.tables[0])
        if chart.titers().number_of_layers() < 2:
            source_target = IndividualTableMapChain(tables=[self.tables[0]], output_root_dir=self.output_root_dir).run(runner=runner, chain_setup=chain_setup)
            first_map_filename = source_target[0][1]
        elif chart.number_of_projections() > 0:
            first_map_filename = self.tables[0]
        else:
            raise error.WrongFirstChartInIncrementalChain(f"""{self.tables[0]} has layers but no projections""")
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
            raise error.WrongFirstChartInIncrementalChain(f"""{link} is not a symlink (needs to be symlink to {target}""")
        if link.readlink() != target:
            link.unlink()
            return True
        return False            # link points to target

# ----------------------------------------------------------------------

class IncrementalMergeMaker:

    def __init__(self, chain_setup):
        self.chain_setup = chain_setup
        self.output_path = None

    def make(self, previous_merge :Path, new_table :Path, output_dir :Path, output_prefix :str):
        previous_chart = acmacs.Chart(previous_merge)
        chart_to_add = acmacs.Chart(new_table)
        if previous_chart.titers().number_of_layers() < 2:
            merge_date = f"""{previous_chart.date()}-{chart_to_add.date()}"""
        else:
            merge_date = f"""{previous_chart.date().split("-")[0]}-{chart_to_add.date()}"""
        self.output_path = output_dir.joinpath(f"{output_prefix}{merge_date}.merge{previous_merge.suffix}")
        if not self.output_path.exists():
            merge, report = acmacs.merge(previous_chart, chart_to_add, type="incremental", combine_cheating_assays=self.chain_setup.combine_cheating_assays())
            print(report)
            merge.export(self.output_path, sys.argv[0])
            self.column_bases = maps.extract_column_bases(merge)
        else:
            log.info(f"""{self.output_path} up to date""")
            self.column_bases = maps.extract_column_bases(acmacs.Chart(self.output_path))
        # pprint.pprint(self.column_bases)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
