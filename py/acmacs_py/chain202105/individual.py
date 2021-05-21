from acmacs_py import *
from .chain_base import ChainBase

class IndividualTableMaps (ChainBase):

    def __init__(self, tables :[Path], **kwargs):
        super().__init__(**kwargs)
        self.tables = tables

    # def run(output_dir :Path, chain_setup):
    #     output_dir.mkdir(parents=True, exist_ok=True)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
