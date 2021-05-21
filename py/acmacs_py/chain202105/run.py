import os, pprint
from pathlib import Path
from acmacs_py.error import KnownError

# ----------------------------------------------------------------------

def run(chain_dir :Path):
    # with email.send_after():
    os.chdir(chain_dir)
    chain_setup = load_setup()

def load_setup():
    locls = {}
    setup_path = Path("Setup.py").resolve()
    try:
        exec(setup_path.open().read(), globals(), locls)
    except FileNotFoundError:
        raise KnownError(f"invalid chain dir (no {setup_path}): {os.getcwd()}")
    try:
        chain_setup_cls = locls["ChainSetup"]
    except KeyError:
        raise KnownError(f"invalid chain setup ({setup_path.resolve()}): ChainSetup class no defined")
    return chain_setup_cls()

# ----------------------------------------------------------------------

# def setup(args):
#     settings = object()
#     settings.whocc_tables_root = Path(f"/syn/eu/ac/whocc-tables")
#     settings.whocc_chains_root = Path(f"/syn/eu/ac/results/chains-202104")
#     return settings

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
