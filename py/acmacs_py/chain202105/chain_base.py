from acmacs_py import *
from .log import info

class ChainBase:

    def __init__(self, name=None, output_root_dir=None, **kwargs):
        self.output_root_dir = output_root_dir
        self.name = name

    def set_output_root_dir(self, output_root_dir :Path):
        self.output_root_dir = output_root_dir

    def output_directory(self):
        if not self.name:
            raise RuntimeError(f"""{self.__class__}: invalid self.name""")
        output_dir = self.output_root_dir.joinpath(self.name)
        output_dir.mkdir(parents=True, exist_ok=True)
        return output_dir

# ----------------------------------------------------------------------

class MapMaker:

    def __init__(self, chain_setup):
        self.chain_setup = chain_setup

    def individual_map_directory_name(self):
        return f"i-{self.chain_setup.minimum_column_basis()}"

    def command(self, source :Path, target :Path):
        """returns command (list) or None if making is not necessary (already made)"""
        target.parent.mkdir(parents=True, exist_ok=True)
        if older_than(target, source):
            return [self.command_name(), *self.command_args(), source, target]
        else:
            info(f"""{target} up to date""")
            return None

    def command_name(self):
        return "chart-relax-grid"

    def command_args(self):
        return [
            "-n", self.chain_setup.number_of_optimizations(),
            "-d", self.chain_setup.number_of_dimensions(),
            "-m", self.chain_setup.minimum_column_basis(),
            *self.args_keep_projections(),
            *self.args_reorient(),
            *self.args_disconnect()
            ]

    def args_keep_projections(self):
        return ["--keep-projections", self.chain_setup.projections_to_keep()]

    def args_reorient(self):
        reorient_to = self.chain_setup.reorient_to()
        if reorient_to:
            return ["--reorient", reorient_to]
        else:
            return []

    def args_disconnect(self):
        if not self.chain_setup.disconnect_having_few_titers():
            return ["--no-disconnect-having-few-titers"]
        else:
            return []

    @classmethod
    def add_threads_to_commands(cls, threads :int, commands :list[list]):
        """Modifies commands to make it limit threads number. Returns modified command"""
        return [command + ["--threads", threads] for command in commands]

# ----------------------------------------------------------------------

def extract_column_bases(chart):
    return {serum.name_full(): chart.column_basis(sr_no) for sr_no, serum in chart.select_all_sera()}

# ----------------------------------------------------------------------

def older_than(first :Path, second :Path):
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
