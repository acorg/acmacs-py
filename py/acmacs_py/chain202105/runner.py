import socket, tempfile, concurrent.futures
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

    def sync_nfs(self, *output_name :Path):
        for dir in set(fn.parent for fn in output_name if output_name):
            with tempfile.TemporaryFile(dir=dir) as fp:
                pass

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
            log.message(command_start, f"$ {comman_to_report}", "", status.stdout, now(), timestamp=False)
            log.separator()
        if self.failures:
            raise RunFailed()

# ----------------------------------------------------------------------

class RunnerSLURM (_RunnerBase):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.threads = 32
        self.run_no = 0
        self.log_sep = "-" * 140

    @classmethod
    def enabled(cls):
        try:
            return (subprocess.check_output(["srun", "-V"]).decode("ascii").split()[1] > "19"
                    and subprocess.check_output(["sbatch", "-V"]).decode("ascii").split()[1] > "19")
        except:
            return False

    def run(self, commands :list, log :Log, add_threads_to_commands, wait_for_output=[], wait_for_output_timeout=60, job_name_prefix="", **kwargs):
        commands = add_threads_to_commands(threads=self.threads, commands=commands)
        chain_dir = Path(self.log_prefix).parents[1]
        start = datetime.datetime.now()
        if job_name_prefix and job_name_prefix[-1] != " ":
            job_name_prefix = job_name_prefix + " "
        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures = [executor.submit(self.run_command, command=command, chain_dir=chain_dir, log=log, job_name_prefix=job_name_prefix) for command in commands]
            for future in concurrent.futures.as_completed(futures):
                future.result()
        self.sync_nfs(wait_for_output and wait_for_output[0])
        self.wait_for_output(wait_for_output=wait_for_output, wait_for_output_timeout=wait_for_output_timeout)
        if self.failures:
            raise RunFailed()

    def wait_for_output(self, wait_for_output, wait_for_output_timeout):
        # due to strange NFS issues (?) sometimes
        # output files appear much later (in 20 seconds), list
        # expected output files to wait for them no longer than
        # wait_for_output_timeout seconds
        if not self.is_failed() and wait_for_output:
            start_wait_for_output = datetime.datetime.now()
            while not all(fn.exists() for fn in wait_for_output) and (datetime.datetime.now() - start_wait_for_output).seconds < wait_for_output_timeout:
                time.sleep(1)
            if (datetime.datetime.now() - start_wait_for_output).seconds > 1:
                log.message(f"output files appeared in {datetime.datetime.now() - start_wait_for_output}")

    def run_command(self, command, chain_dir :Path, log :Log, job_name_prefix):
        self.run_no += 1
        cmd = ["srun", "--ntasks=1", "--nodes=1", f"--cpus-per-task={self.threads}", f"--job-name={job_name_prefix}{command[0]} {chain_dir.name} {self.run_no}", *(str(part) for part in command)]
        start = datetime.datetime.now()
        status = subprocess.run(cmd, cwd=chain_dir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if status.returncode == 0:
            finished_message = f"completed in {datetime.datetime.now() - start}"
        else:
            self.failures.append(" ".join(cmd))
            finished_message = f"FAILED in {datetime.datetime.now() - start}"
        log.message(" ".join(cmd), status.stdout, finished_message, flush=True)

# ----------------------------------------------------------------------

class RunnerSLURM_sbatch (_RunnerBase):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.threads = 32
        self.run_no = 0
        self.log_sep = "-" * 140

    @classmethod
    def enabled(cls):
        try:
            return (subprocess.check_output(["srun", "-V"]).decode("ascii").split()[1] > "19"
                    and subprocess.check_output(["sbatch", "-V"]).decode("ascii").split()[1] > "19")
        except:
            return False

    def run(self, commands :list, log :Log, add_threads_to_commands, wait_for_output=[], wait_for_output_timeout=60, job_name_prefix="", **kwargs):
        # wait_for_output: due to strange NFS issues (?) sometimes
        # output files appear much later (in 20 seconds), list
        # expected output files to wait for them no longer than
        # wait_for_output_timeout seconds

        self.run_no += 1
        post_commands = [] # [["ls", "-l", cmd[-1]] for cmd in commands]
        commands = add_threads_to_commands(threads=self.threads, commands=commands)
        chain_dir = Path(self.log_prefix).parents[1]
        log_file_name = self.log_path(log_suffix=f"{self.run_no:03d}-slurm.log")
        batch = self.sBatchTemplate.format(
            job_name=f"{job_name_prefix} chain-202105 {chain_dir.name}",
            chdir=chain_dir,
            log_file_name=log_file_name,
            threads=self.threads,
            ntasks=len(commands),
            commands="\n".join(f"srun -n1 -N1 -c{self.threads} '" + "' '".join(str(part) for part in cmd) + "' &" for cmd in commands),
            post_commands="\n".join("'" + "' '".join(str(part) for part in cmd) + "'" for cmd in post_commands)
            )
        log.message("SBATCH", batch)
        log.flush()
        start = datetime.datetime.now()
        status = subprocess.run(["sbatch"], input=batch, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        self.sync_nfs(log_file_name, wait_for_output and wait_for_output[0])
        log.message(f"SBATCH {'completed' if status.returncode == 0 else 'FAILED ' + str(status.returncode)} in {datetime.datetime.now() - start}", self.log_sep, "sbatch log", self.log_sep, log_file_name.open().read(), self.log_sep, "")
        log_file_name.unlink()
        if status.returncode == 0 and wait_for_output:
            start_wait_for_output = datetime.datetime.now()
            while not all(fn.exists() for fn in wait_for_output) and (datetime.datetime.now() - start_wait_for_output).seconds < wait_for_output_timeout:
                time.sleep(1)
            if (datetime.datetime.now() - start_wait_for_output).seconds > 1:
                log.message(f"output files appeared in {datetime.datetime.now() - start_wait_for_output}")
        if status.returncode != 0:
            self.failures.append("sbatch")
        log.flush()
        if self.failures:
            raise RunFailed()

    sBatchTemplate = """#! /bin/bash
#SBATCH --mail-user=eu@antigenic-cartography.org
#SBATCH --mail-type=FAIL
#SBATCH --job-name="{job_name}"
#SBATCH --chdir="{chdir}"
#SBATCH --output="{log_file_name}"
#SBATCH --error="{log_file_name}"
#SBATCH --ntasks={ntasks}
#SBATCH --cpus-per-task={threads}
#SBATCH -N1-1000
#SBATCH --wait

#xSBATCH --ntasks-per-node=4

printf "SLURM Job:$SLURM_JOBID Node:$SLURMD_NODENAME\n\n"

{commands}

wait

{post_commands}

exit 0
"""

# ======================================================================
