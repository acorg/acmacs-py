from acmacs_py import *
from .chain_base import ChainBase, IndividualMapMaker

# ----------------------------------------------------------------------

class IncrementalChain (ChainBase):

    def __init__(self, tables :list[Path], name :str, **kwargs):
        super().__init__(**kwargs)
        self.tables = tables
        self.name = name

    def run(self, runner, chain_setup):
        IndividualMapMaker(chain_setup).make(source=self.tables[0], output_root_dir=self.output_root_dir, runner=runner)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
