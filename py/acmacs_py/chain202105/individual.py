from acmacs_py import *
from .chain_base import ChainBase, IndividualMapMaker

class IndividualTableMaps (ChainBase):

    def __init__(self, tables :list[Path], **kwargs):
        super().__init__(**kwargs)
        self.tables = tables

    def run(self, runner, chain_setup):
        for table in self.tables:
            IndividualMapMaker(chain_setup).make(source=table, output_root_dir=self.output_root_dir, runner=runner)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
