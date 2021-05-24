import socket
from acmacs_py import *

# ----------------------------------------------------------------------

def runner_factory(log_dir :Path):
    for runner_class in [RunnerSLURM, RunnerLocal]:
        if runner_class.enabled():
            return runner_class(log_dir=log_dir)
    raise KnownError("No runner enabled")

# ----------------------------------------------------------------------

class _RunnerBase:           # must begin with _

    def __init__(self, log_dir :Path):
        self.failures = []
        self.log_dir = log_dir

    @classmethod
    def enabled(cls):
        return False

    def is_failed(self):
        return len(self.failures) != 0

    def report_failures(self):
        messages = "\n    ".join(self.failures)
        print(f"""> Logs of {len(self.failures)} failed commands:\n    {messages}""", file=sys.stderr)

# ----------------------------------------------------------------------

class RunnerLocal (_RunnerBase):

    @classmethod
    def enabled(cls):
        return True

    def run(self, command, log_file_name :str, **kwargs):
        command = [str(elt) for elt in command]
        print(" ".join(command))
        log_file = self.log_dir.joinpath(log_file_name)
        status = subprocess.run(command, stdout=log_file.open("w"), stderr=subprocess.STDOUT)
        if status.returncode != 0:
            self.failures.append(f"{socket.gethostname()}:{log_file}")

# ----------------------------------------------------------------------

class RunnerSLURM (_RunnerBase):

    def __init__(self):
        self.threads = 16

    @classmethod
    def enabled(cls):
        try:
            return (subprocess.check_output(["srun", "-V"]).decode("ascii").split()[1] > "19"
                    and subprocess.check_output(["sbatch", "-V"]).decode("ascii").split()[1] > "19")
        except:
            return False

    def run(self, command, log_file_name :str, add_threads_to_command, **kwargs):
        command = [str(elt) for elt in add_threads_to_command(self.threads, command)]
        print(f"""RunnerSLURM.run: '{"' '".join(command)}'""")

# ----------------------------------------------------------------------

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
