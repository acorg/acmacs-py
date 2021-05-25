from acmacs_py import *
from .chain_base import ChainBase, MapMaker, older_than
from .error import RunFailed
from .log import info
import acmacs

# ----------------------------------------------------------------------

class IndividualTableMaps (ChainBase):

    def __init__(self, tables :list[Path], **kwargs):
        super().__init__(**kwargs)
        self.tables = tables

    def run(self, runner, chain_setup):
        with runner.log_path("individual.log").open("a") as log:
            maker = IndividualMapMaker(chain_setup)
            source_target = [[table, self.output_root_dir.joinpath(maker.individual_map_directory_name(), table.name)] for table in self.tables]
            commands = [cmd for cmd in (maker.command(source=source, target=target) for source, target in source_target) if cmd]
            try:
                runner.run(commands, log=log, add_threads_to_commands=IndividualMapMaker.add_threads_to_commands)
            except RunFailed:
                pass            # ignore failures, they will be reported upon making all other maps
            return source_target

# ----------------------------------------------------------------------

class IndividualMapMaker (MapMaker):

    pass

# ----------------------------------------------------------------------

class IndividualMapWithMergeColumnBasesMaker (MapMaker):

    def __init__(self, chain_setup): # , output_dir_name :str):
        super().__init__(chain_setup)
        # self.output_dir_name = output_dir_name
        self.source = None      # nothing to do
        self.target = None      # nothing to do

    def prepare(self, source :Path, merge_column_bases :dict, output_dir :Path, output_prefix :str):
        chart = acmacs.Chart(source)
        mcb_source = output_dir.joinpath(f"{output_prefix}{chart.date()}.mcb-table{source.suffix}")
        mcb_target = output_dir.joinpath(f"{output_prefix}{chart.date()}.mcb{source.suffix}")
        if older_than(mcb_target, source):
            cb = chart.column_bases(self.chain_setup.minimum_column_basis())
            orig_cb = str(cb)
            updated = False
            for sr_no, serum in chart.select_all_sera():
                mcb = merge_column_bases.get(serum.name_full())
                if mcb is None:
                    raise RuntimeError(f"""No column basis for {serum.name_full()} in the merge column bases""")
                if mcb != cb[sr_no]:
                    if mcb < cb[sr_no]:
                        raise RuntimeError(f"""Column basis for {serum.name_full()} in the merge ({mcb}) is less than in the individual table ({cb[sr_no]})""")
                    cb[sr_no] = mcb
                    updated = True
            if updated:
                chart.column_bases(cb)
                info(f"{mcb_source} <-- {source}: column basis updated from merge:\n    orig: {orig_cb}\n     new: {cb}")
                self.source = mcb_source
                self.target = mcb_target
                chart.export(self.source, program_name=sys.argv[0])
            else:
                info("column basis in the merge are the same as in the original individual table")
        else:
            info(f"{mcb_source} up to date")

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
