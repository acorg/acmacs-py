from acmacs_py import *
from .chain_base import ChainBase, MapMaker
from .error import RunFailed

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


# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
