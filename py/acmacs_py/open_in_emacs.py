import os, subprocess
from pathlib import Path

# ----------------------------------------------------------------------

def open_in_emacs(filename :Path):
    emacsclient = f"""{os.environ["HOME"]}/bin/emacsclient"""
    if os.environ.get("INSIDE_EMACS") and Path(emacsclient).exists():
        subprocess.run([emacsclient, "-n", str(filename)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
