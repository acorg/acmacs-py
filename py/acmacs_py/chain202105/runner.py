import socket
from acmacs_py import *
from .error import RunFailed
from .log import error, info

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
        error(f"""Logs of {len(self.failures)} failed commands:\n    {messages}""")

# ----------------------------------------------------------------------

class RunnerLocal (_RunnerBase):

    @classmethod
    def enabled(cls):
        return True

    def run(self, commands :list[list], log_file_name :str, **kwargs):
        log_file_path = self.log_dir.joinpath(log_file_name)
        with log_file_path.open("w") as log_file:
            for command in commands:
                command = [str(elt) for elt in command]
                comman_to_report = " ".join(command)
                info(comman_to_report)
                log_file.write(f"""$ {comman_to_report}\n\n""")
                log_file.flush()
                status = subprocess.run(command, stdout=log_file, stderr=subprocess.STDOUT)
                if status.returncode != 0:
                    self.failures.append(f"{socket.gethostname()}:{log_file_path}")
                log_file.write(f"""\n\n{"=" * 70}\n\n""")

            if self.failures:
                raise RunFailed()

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

    def run(self, commands :list[list], add_threads_to_commands, **kwargs):
        commands = add_threads_to_commands(threads=self.threads, commands=commands)
        info("RunnerSLURM.run:\n    {}".format("\n    ".join(" ".join(command) for command in commands)))

        if self.failures:
            raise RunFailed()

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
