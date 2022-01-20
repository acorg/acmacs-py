from ..error import KnownError

# ----------------------------------------------------------------------

class ChainFailed (KnownError):
    pass

# ----------------------------------------------------------------------

class RunFailed (ChainFailed):
    pass

# ----------------------------------------------------------------------

class WrongFirstChartInIncrementalChain (ChainFailed):

    def __init__(self, message):
        super().__init__(f"""WrongFirstChartInIncrementalChain: {message}""")

# ======================================================================
