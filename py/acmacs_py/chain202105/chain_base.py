from acmacs_py import *
from .log import info

class ChainBase:

    def __init__(self, name=None, **kwargs):
        self.output_root_dir = None
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
        self.output_path = None

    def individual_map_directory_name(self):
        return f"i-{self.chain_setup.minimum_column_basis()}"

    def command(self, source :Path, output_root_dir :Path):
        """returns command (list) or None if making is not necessary (already made)"""
        self.output_path = output_root_dir.joinpath(self.output_directory_name(), source.name)
        self.output_path.parent.mkdir(parents=True, exist_ok=True)
        if self.older_than(self.output_path, source):
            options = [
                "-n", self.chain_setup.number_of_optimizations(),
                "-d", self.chain_setup.number_of_dimensions(),
                "-m", self.chain_setup.minimum_column_basis(),
                "--keep-projections", self.chain_setup.projections_to_keep(),
                ]
            reorient_to = self.chain_setup.reorient_to()
            if reorient_to:
                options.extend(["--reorient", reorient_to])
            if not self.chain_setup.disconnect_having_few_titers():
                options.append("--no-disconnect-having-few-titers")
            return ["chart-relax-grid", *options, source, self.output_path]
        else:
            info(f"""{self.output_path} up to date""")
            return None

    # def make(self, source :Path, output_root_dir :Path, runner):
    #     cmd = self.command(source=source, output_root_dir=output_root_dir)
    #     if cmd:
    #         runner.run(cmd, log_file_name=f"i-{source.name}.log", add_threads_to_command=self.add_threads_to_command)
    #     return self.output_path

    @classmethod
    def add_threads_to_commands(cls, threads :int, commands :list[list]):
        """Modifies commands to make it limit threads number. Returns modified command"""
        return [command + ["--threads", threads] for command in commands]

    def output_directory_name(self):
        raise RuntimeError(f"override in derived: {self.__class__}")

    def older_than(self, first :Path, second :Path):
        "returns if first file is older than second or first does not exist. raises if second does not exist"
        try:
            first_mtime = first.stat().st_mtime
        except FileNotFoundError:
            return True
        return second.stat().st_mtime > first_mtime

# ----------------------------------------------------------------------

def extract_column_bases(chart):
    return {serum.name_full(): chart.column_basis(sr_no) for sr_no, serum in chart.select_all_sera()}

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
