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
        commands = [cmd for cmd in (IndividualMapMaker(chain_setup).command(source=table, output_root_dir=self.output_root_dir) for table in self.tables) if cmd]
        try:
            runner.run(commands, log_file_name=f"individual-table-maps.{self.tables[0].name}.log", add_threads_to_commands=IndividualMapMaker.add_threads_to_commands)
        except RunFailed:
            pass            # ignore failures, they will be reported upon making all other maps

# ----------------------------------------------------------------------

class IndividualMapMaker (MapMaker):

    def output_directory_name(self):
        return self.individual_map_directory_name()

# ----------------------------------------------------------------------

class IndividualMapWithMergeColumnBasesMaker (MapMaker):

    def __init__(self, chain_setup, output_dir_name :str):
        super().__init__(chain_setup)
        self.output_dir_name = output_dir_name
        self.source = None      # nothing to do

    def output_directory_name(self):
        return self.output_dir_name

    def prepare(self, source :Path, merge_column_bases :dict, output_dir :Path, output_prefix :str):
        chart = acmacs.Chart(source)
        mcb_source = output_dir.joinpath(f"{output_prefix}{chart.date()}.mcb{source.suffix}")
        if older_than(mcb_source, source):
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
                chart.export(self.source, program_name=sys.argv[0])
            else:
                info("column basis in the merge are the same as in the original individual table")
        else:
            info(f"{mcb_source} up to date")

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
