from acmacs_py import *
from . import maps, error
from .log import Log, info
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

    def __init__(self, tables :list, **kwargs):
        super().__init__(**kwargs)
        self.tables = tables

    def run(self, runner, chain_setup):
        with Log(runner.log_path("individual.log")) as log:
            start = datetime.datetime.now()
            try:
                maker = maps.IndividualMapMaker(chain_setup, minimum_column_basis=self.minimum_column_basis, ignore_tables_with_too_few_sera=chain_setup.ignore_tables_with_too_few_sera(), log=log)
                source_target = [[table, self.output_root_dir.joinpath(maker.individual_map_directory_name(), table.name)] for table in self.tables]
                if commands := [cmd for cmd in (maker.command(source=source, target=target) for source, target in source_target) if cmd]:
                    try:
                        runner.run(commands, log=log, add_threads_to_commands=maps.IndividualMapMaker.add_threads_to_commands, wait_for_output=[source_target[0][1]]) # [st[1] for st in source_target])
                    except error.RunFailed:
                        pass            # ignore failures, they will be reported upon making all other maps
                return source_target
            finally:
                log.info(f"chain run time: {datetime.datetime.now() - start}")

# ----------------------------------------------------------------------

class IncrementalChain (ChainBase):

    def __init__(self, tables :list, **kwargs):
        super().__init__(**kwargs)
        self.tables = tables

    def run(self, runner, chain_setup):
        with Log(runner.log_path("incremental.log")) as log:
            start = datetime.datetime.now()
            try:
                previous_merge_path = self.first_map(runner=runner, chain_setup=chain_setup)
                for table_no, table in enumerate(self.tables[1:], start=1):
                    merger = IncrementalMergeMaker(chain_setup, log=log)
                    merger.make(previous_merge=previous_merge_path, new_table=table, output_dir=self.output_directory(), output_prefix=self.output_prefix(table_no))

                    individual_merge_cb = maps.IndividualMapWithMergeColumnBasesMaker(chain_setup, minimum_column_basis=self.minimum_column_basis, ignore_tables_with_too_few_sera=chain_setup.ignore_tables_with_too_few_sera(), log=log)
                    individual_merge_cb.prepare(source=table, merge_column_bases=merger.column_bases, merge_path=merger.output_path, output_dir=self.output_directory(), output_prefix=self.output_prefix(table_no))

                    incremental_map_output = merger.output_path.parent.joinpath(merger.output_path.name.replace(".merge.", ".incremental."))
                    scratch_map_output = merger.output_path.parent.joinpath(merger.output_path.name.replace(".merge.", ".scratch."))
                    commands = [cmd for cmd in (
                        individual_merge_cb.source and individual_merge_cb.command(source=individual_merge_cb.source, target=individual_merge_cb.target),
                        maps.IncrementalMapMaker(chain_setup, minimum_column_basis=self.minimum_column_basis, log=log).command(source=merger.output_path, target=incremental_map_output),
                        maps.MapMaker(chain_setup, minimum_column_basis=self.minimum_column_basis, log=log).command(source=merger.output_path, target=scratch_map_output),
                        ) if cmd]
                    if commands:
                        runner.run(commands=commands, log=log, add_threads_to_commands=maps.MapMaker.add_threads_to_commands, wait_for_output=[incremental_map_output, scratch_map_output])
                    if individual_merge_cb.source:
                        individual_merge_cb.source.unlink()
                    # TODO: avidity test
                    previous_merge_path = self.choose_between_incremental_scratch(incremental_map_output, scratch_map_output, log=log)
                    # TODO: degradation check
                    log.flush()
            finally:
                log.info(f"chain run time: {datetime.datetime.now() - start}")

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

    def choose_between_incremental_scratch(self, incremental_map_output :Path, scratch_map_output :Path, log :Log):
        if incremental_map_output and scratch_map_output:
            incremental_chart = acmacs.Chart(incremental_map_output)
            incremental_stress = incremental_chart.projection(0).stress()
            scratch_chart = acmacs.Chart(scratch_map_output)
            scratch_stress = scratch_chart.projection(0).stress()
            log.info(f"choosing between incremental ({incremental_map_output.name}) and from scratch ({scratch_map_output.name})", f"incremental stress: {incremental_stress}\n   scratch stress: {scratch_stress}")
            if incremental_stress <= scratch_stress:
                previous_merge_path = incremental_map_output
                log.info(f"using incremental map ({incremental_map_output.name}) for the next step")
            else:
                previous_merge_path = scratch_map_output
                log.info(f"using map from scratch ({scratch_map_output.name}) for the next step")
            log.separator(newlines_before=1)
            return previous_merge_path
        else:
            raise NotImplementedError("IncrementalChain with just incremental or scratch map not implemented")

    def output_prefix(self, table_no):
        return f"""{table_no:03d}."""

    def unlink_if_wrong_symlink(self, link, target):
        """returns if link was removed or did not exist"""
        if not link.exists():
            return True
        if not link.is_symlink():
            raise error.WrongFirstChartInIncrementalChain(f"""{link} is not a symlink (needs to be symlink to {target}""")
        # print(f"unlink_if_wrong_symlink {Path(os.readlink(link))} == {target} -> {Path(os.readlink(link)) == target}", file=sys.stderr)
        if Path(os.readlink(link)) != target:
            link.unlink()
            return True
        return False            # link points to target

# ----------------------------------------------------------------------

class IncrementalMergeMaker:

    def __init__(self, chain_setup, log :Log):
        self.chain_setup = chain_setup
        self.output_path = None
        self.log = log

    def make(self, previous_merge :Path, new_table :Path, output_dir :Path, output_prefix :str):
        self.log.info(f"merging {previous_merge.name} and {new_table.name} (incrementally)")
        previous_chart = acmacs.Chart(previous_merge)
        chart_to_add = acmacs.Chart(new_table)
        if previous_chart.titers().number_of_layers() < 2:
            merge_date = f"""{previous_chart.date()}-{chart_to_add.date()}"""
        else:
            merge_date = f"""{previous_chart.date().split("-")[0]}-{chart_to_add.date()}"""
        self.output_path = output_dir.joinpath(f"{output_prefix}{merge_date}.merge{previous_merge.suffix}")
        if not self.output_path.exists():
            merge, report = acmacs.merge(previous_chart, chart_to_add, type="incremental", match="strict", combine_cheating_assays=self.chain_setup.combine_cheating_assays())
            self.log.info(after_newline=report.common())
            # self.log.info(after_newline=report.titer_merge(merge))
            merge.export(self.output_path, sys.argv[0])
            self.column_bases = maps.extract_column_bases(merge)
        else:
            # self.log.info(f"""{self.output_path} up to date\n""")
            self.column_bases = maps.extract_column_bases(acmacs.Chart(self.output_path))
        # pprint.pprint(self.column_bases)
        self.log.separator()
        return self

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
