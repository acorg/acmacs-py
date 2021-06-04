import socket
from acmacs_py import *
from .error import RunFailed
from .log import Log, error, now

# ----------------------------------------------------------------------

def runner_factory(log_prefix :str, force_local=False):
    if force_local:
        return RunnerLocal(log_prefix=log_prefix)
    for runner_class in [RunnerSLURM, RunnerLocal]:
        if runner_class.enabled():
            return runner_class(log_prefix=log_prefix)
    raise KnownError("No runner enabled")

# ----------------------------------------------------------------------

class _RunnerBase:           # must begin with _

    def __init__(self, log_prefix :str):
        self.failures = []
        self.log_prefix = log_prefix

    @classmethod
    def enabled(cls):
        return False

    def is_failed(self):
        return len(self.failures) != 0

    def report_failures(self):
        messages = "\n    ".join(self.failures)
        error(f"{len(self.failures)} failed commands:\n    {messages}")

    def log_path(self, log_suffix :str):
        return Path(self.log_prefix + log_suffix)

# ----------------------------------------------------------------------

class RunnerLocal (_RunnerBase):

    @classmethod
    def enabled(cls):
        return True

    def run(self, commands :list, log :Log, **kwargs):
        for command in commands:
            command = [str(elt) for elt in command]
            comman_to_report = " ".join(command)
            command_start = now()
            status = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            if status.returncode != 0:
                self.failures.append(comman_to_report)
            log.message(f"{command_start}\n$ {comman_to_report}\n\n{status.stdout}\n{now()}\n")
            log.separator()
        if self.failures:
            raise RunFailed()

# ----------------------------------------------------------------------

class RunnerSLURM (_RunnerBase):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.threads = 16
        self.run_no = 0

    @classmethod
    def enabled(cls):
        try:
            return (subprocess.check_output(["srun", "-V"]).decode("ascii").split()[1] > "19"
                    and subprocess.check_output(["sbatch", "-V"]).decode("ascii").split()[1] > "19")
        except:
            return False

    def run(self, commands :list, log :Log, add_threads_to_commands, **kwargs):
        self.run_no += 1
        commands = add_threads_to_commands(threads=self.threads, commands=commands)
        chain_dir = Path(self.log_prefix).parents[1]
        batch = self.sBatchTemplate.format(
            job_name=f"chain-202105 {chain_dir.name}",
            chdir=chain_dir,
            log_file_name=self.log_path(log_suffix=f"{self.run_no:03d}-slurm.log"),
            threads=self.threads,
            ntasks=len(commands),
            commands="\n".join("srun -n1 -N1 '" + "' '".join(str(part) for part in cmd) + "' &" for cmd in commands)
            )
        log.message(now() + ": SBATCH\n" + batch)
        status = subprocess.run(["sbatch"], input=batch, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if status.returncode != 0:
            self.failures.append("sbatch")

        if self.failures:
            raise RunFailed()

    sBatchTemplate = """#! /bin/bash
#SBATCH --mail-user=eu@antigenic-cartography.org
#SBATCH --mail-type=FAIL
#SBATCH --job-name="{job_name}"
#SBATCH --chdir="{chdir}"
#SBATCH --output="{log_file_name}"
#SBATCH --error="{log_file_name}"
#SBATCH --cpus-per-task={threads}
#SBATCH --ntasks={ntasks}
#SBATCH -N1-1000
#x SBATCH --time=01:00:00
{commands}
wait
exit 0
"""

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
