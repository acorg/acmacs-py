"""
In the chain directory create Setup.py with the following sample content:

----------------------------------------------------------------------
from acmacs_py.chain202105.chain_setup import ChainSetupDefault

class ChainSetup (ChainSetupDefault):

    # def number_of_optimizations(self):
    #     return 1000
    #
    # def number_of_dimensions(self):
    #     return 2
    #
    # def minimum_column_basis(self):
    #     return "none"

"""

# ----------------------------------------------------------------------

class ChainSetupDefault:

    def __init__(self):
        pass

    def number_of_optimizations(self):
        return 1000

    def number_of_dimensions(self):
        return 2

    def minimum_column_basis(self):
        return "none"


# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
