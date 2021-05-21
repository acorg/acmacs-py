from acmacs_py import *

class IndividualTableMaps:

    def __init__(self, tables :[Path]):
        self.tables = tables

    def run(output_dir :Path, chain_setup):
        output_dir.mkdir(parents=True, exist_ok=True)
        
# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
