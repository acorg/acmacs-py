from acmacs_py import *

class ChainBase:

    def __init__(self):
        self.output_root_dir = None

    def set_output_root_dir(self, output_root_dir :Path):
        self.output_root_dir = output_root_dir

    def older_than(self, first :Path, second :Path):
        "returns if first file is older than second or first does not exist. raises if second does not exist"
        try:
            first_mtime = first.stat().st_mtime
        except FileNotFoundError:
            return True
        return second.stat().st_mtime > first_mtime

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
