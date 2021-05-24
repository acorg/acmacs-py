from acmacs_py import *

class ChainBase:

    def __init__(self):
        self.output_root_dir = None

    def set_output_root_dir(self, output_root_dir :Path):
        self.output_root_dir = output_root_dir

# ----------------------------------------------------------------------

class MapMaker:

    def __init__(self, chain_setup):
        self.chain_setup = chain_setup

    def individual_map_directory_name(self):
        return f"i-{self.chain_setup.minimum_column_basis()}"

    def make(self, source :Path, output_root_dir :Path, runner):
        output_path = output_root_dir.joinpath(self.output_directory_name(), source.name)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        if self.older_than(output_path, source):
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
            runner.run(["chart-relax-grid", *options, source, output_path], log_file_name=f"i-{source.name}.log", add_threads_to_command=self.add_threads_to_command)
        else:
            print(f"""{output_path} up to date""")

    def add_threads_to_command(self, threads : int, command : list):
        """Modifies command to make it limit threads number. Returns modified command"""
        return command + ["--threads", threads]

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

class IndividualMapMaker (MapMaker):

    def output_directory_name(self):
        return self.individual_map_directory_name()


# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
