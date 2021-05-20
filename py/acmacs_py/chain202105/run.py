import os
from pathlib import Path

# ----------------------------------------------------------------------

def run(chain_dir :Path):
    # with email.send_after():
    os.chdir(chain_dir)
    load_setup()

def load_setup():
    try:
        exec(Path("Setup.py").open().read())
    except FileNotFoundError:
        raise RuntimeError(f"invalid chain dir (no Setup.py): {os.getcwd()}")

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
