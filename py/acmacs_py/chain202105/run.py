from acmacs_py import *
from .submitter import submitter_factory

# ----------------------------------------------------------------------

def run(chain_dir :Path):
    # with email.send_after():
    chain_dir = chain_dir.resolve()
    chain_setup = load_setup(chain_dir)
    submitter = submitter_factory()
    for chain in chain_setup.chains():
        chain.set_output_root_dir(chain_dir)
        chain.run(submitter=submitter, chain_setup=chain_setup)

def load_setup(chain_dir :Path):
    locls = {}
    setup_path = chain_dir.joinpath("Setup.py")
    try:
        exec(setup_path.open().read(), globals(), locls)
    except FileNotFoundError:
        raise KnownError(f"invalid chain dir: no {setup_path}")
    try:
        chain_setup_cls = locls["ChainSetup"]
    except KeyError:
        raise KnownError(f"invalid chain setup ({setup_path}): ChainSetup class no defined")
    return chain_setup_cls()

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
