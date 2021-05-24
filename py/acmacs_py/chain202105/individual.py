from acmacs_py import *
from .chain_base import ChainBase, IndividualMapMaker
from .error import RunFailed

# ----------------------------------------------------------------------

class IndividualTableMaps (ChainBase):

    def __init__(self, tables :list[Path], **kwargs):
        super().__init__(**kwargs)
        self.tables = tables

    def run(self, runner, chain_setup):
        for table in self.tables:
            try:
                IndividualMapMaker(chain_setup).make(source=table, output_root_dir=self.output_root_dir, runner=runner)
            except RunFailed:
                pass            # ignore failures, they will be reported upon making all other maps

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
