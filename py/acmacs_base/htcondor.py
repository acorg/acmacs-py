# -*- Python -*-
# license
# license.
# ======================================================================

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import re, collections, subprocess, time, datetime, pprint

# ----------------------------------------------------------------------

class JobError (Exception): pass

class Job:

    sJobStatus = {'1': 'pending', '2': 'running', '3': 'removed', '4': 'completed', '5': 'held', '6': 'transferring output', '7': 'suspended'}

    def __init__(self, clusters :dict, condor_log :Path):
        """clusters - cluster_id to number of jobs mapping"""
        self.clusters = clusters
        self.condor_log = condor_log

    def __str__(self):
        return str(self.clusters)

    # https://htcondor.readthedocs.io/en/latest/codes-other-values/job-event-log-codes.html
    sReLogEvent = re.compile(r"^(\d\d\d)\s+\(([\d\.]+)\)")
    sReExitStatus = re.compile(r"^\s+\(\d+\)\s+Normal\s+termination\s+\(return\s+value\s+(\d+)\)", re.I)
    sReExitStatus11 = re.compile(r"^\s+\(\d+\)\s+Abnormal\s+termination\s+", re.I)
    def state(self):
        jobs = {"submitted": [], "running": [], "failed": [], "completed": [], "unrecognized": [], "FAILED": False, "DONE": False, "PERCENT": 0}
        event_to_job = {"000": "submitted", "001": "running", "002": "failed", "005": "completed", "006": "ignore", "009": "failed"}
        job_terminated = False
        for line in Path(self.condor_log).open():
            if job_terminated:  # check exit status
                exit_status_match = self.sReExitStatus.match(line)
                if exit_status_match:
                    if exit_status_match.group(1) == "0":
                        jobs["completed"].append(cluster)
                    else:
                        jobs["failed"].append(cluster)
                elif self.sReExitStatus11.match(line):
                    jobs["failed"].append(cluster)
                else:
                    module_logger.error(f"No exit status found for job {cluster} in {line.strip()!r}")
                job_terminated = False
            else:
                event_title_match = self.sReLogEvent.match(line)
                if event_title_match:
                    event, cluster = event_title_match.groups()
                    if event == "005": # terminated
                        job_terminated = True # check next line for exit status
                    else:
                        try:
                            jobs[event_to_job[event]].append(cluster)
                        except KeyError:
                            jobs["unrecognized"].append(line.strip())
        if len(jobs["completed"]) == len(jobs["submitted"]):
            jobs["DONE"] = True
        elif (len(jobs["failed"]) + len(jobs["completed"])) == len(jobs["submitted"]):
            jobs["FAILED"] = True
        jobs["PERCENT"] = int((len(jobs["failed"]) + len(jobs["completed"])) / len(jobs["submitted"]) * 100.0)
        # module_logger.debug(f"{pprint.pformat(jobs)}")
        return jobs

    def status(self):
        # in reality needs to look into history file to check for failed jobs but it is slow
        still_running = collections.defaultdict(int)
        errors = []
        output = _run("condor_q", *list(self.clusters), "-autoformat:tl", "ClusterId", "ProcId", "JobStatus", "HoldReason")
        for line in output.splitlines():
            # module_logger.info('Status line {}'.format(line))
            state = dict(f.split(' = ') for f in line.split('\t'))
            state["JobStatus"] = self.sJobStatus[state["JobStatus"]]
            if state["JobStatus"] not in ["pending", "running", "transferring output", "completed", "suspended"]:
                errors.append(state)
            else:
                still_running[state["ClusterId"]] += 1
        if errors:
            raise JobError(errors)
        r = {}
        for cluster, jobs in self.clusters.items():
            cluster_running = still_running.get(cluster, 0)
            if cluster_running:
                r[cluster] = {"state": "running", "done": (jobs - cluster_running) / jobs}
            else:
                r[cluster] = {"state": "completed"}
        all_jobs = sum(self.clusters.values())
        all_running = sum(still_running.values())
        r["all"] = {"state": "running" if all_running else "completed", "done": (all_jobs - all_running) / all_jobs}
        return r

    def wait(self, timeout=None, verbose=False):
        start = datetime.datetime.now()
        cmd = ["condor_wait"]
        if timeout is not None:
            cmd.extend(["-wait", str(timeout)])
        cmd.append(str(self.condor_log))
        if verbose:
            cmd.append("-echo")
        # output = _run(*cmd)
        output = subprocess.run(cmd, env={"LD_LIBRARY_PATH": ""}, check=False, stdout=subprocess.PIPE).stdout.decode("utf-8") # ignore exit code, condor_wait exits with 1 on timeout
        if "All jobs done." in output:
            status = "done"
        elif "Time expired." in output:
            status = "timeout"
        else:
            status = "unknown"
            module_logger.warning('Unrecognized condor_wait output:\n' + output)
        if verbose:
            module_logger.info('condor_wait echo:\n' + output)
        # if status != "timeout":
        #     module_logger.info('{} in {}'.format(status, datetime.datetime.now() - start))
        return status

    def kill_tasks(self, tasks):
        cmd = ["condor_rm", *("{}.{}".format(list(self.clusters)[0], t) for t in tasks)]
        module_logger.info(str(cmd))
        subprocess.run(cmd, env={"LD_LIBRARY_PATH": ""}, check=False, stdout=subprocess.DEVNULL) # ignore exit code, condor_rm exits with 1 when all the tasks to remove have already completed

    # def wait_old(self, check_interval_in_seconds=30, verbose=True):
    #     start = datetime.datetime.now()
    #     while True:
    #         time.sleep(check_interval_in_seconds)
    #         status = self.status()
    #         if status["all"]["state"] == "completed":
    #             break
    #         if verbose:
    #             module_logger.info('Percent done: {:.1f}%'.format(status["all"]["done"] * 100.0))
    #     if verbose:
    #         module_logger.info('All done in {}'.format(datetime.datetime.now() - start))

# ----------------------------------------------------------------------

def prepare_submission(program, program_args :list, description :str, current_dir :Path, environment :dict={}, request_memory=None, capture_stdout=False, email=None, notification="Error", machines :list = None, priority :int = 0, request_cpus :int = 1):
    current_dir = Path(current_dir).resolve()
    current_dir.chmod(0o777)        # to allow remote processes runinnig under user nobody to write files
    condor_log = Path(current_dir, "condor.log")
    desc = [
        ["universe", "vanilla"],
        ["executable", str(Path(program).resolve())],
        ["priority", str(priority)],
        ["should_transfer_files", "NO"],
        ["notify_user", email or ""],
        ["notification", notification if email else "Never"],
        ["Requirements", "({})".format(" || ".join('machine == "{}"'.format(fix_machine_name(m)) for m in machines)) if machines else None],
        ["request_memory", str(request_memory) if request_memory is not None else "2000"],
        ["request_cpus", str(request_cpus)],
        ["initialdir", str(current_dir)],
        ["log", str(condor_log)],
        ["description", "{} {}".format(description, current_dir)],
        ["environment", " ".join(f"{key}='{value}'" for key, value in environment.items())],
        ["coresize", "-1"],
        [""],
        ]
    stderr_files = [Path(current_dir, "output", "{:04d}.stderr".format(no)) for no, args in enumerate(program_args)]
    for f in stderr_files:
        f.parent.mkdir(parents=True, exist_ok=True)
        f.touch()
        f.chmod(0o777)
    if capture_stdout:
        stdout_files = [Path(current_dir, "output", "{:04d}.stdout".format(no)) for no, args in enumerate(program_args)]
        for f in stdout_files:
            f.touch()
            f.chmod(0o777)
    else:
        stdout_files = [None] * len(program_args)
    for no, args in enumerate(program_args):
        desc.extend([
            ["arguments", " ".join(str(a) for a in args)],
            ["error", stderr_files[no] and str(stderr_files[no])],
            ["output", stdout_files[no] and str(stdout_files[no])],
            ["queue"],
            [""],
            ])
    desc_s = "\n".join((" = ".join(e) if len(e) == 2 else e[0]) for e in desc if len(e) != 2 or e[1])
    desc_filename = Path(current_dir, "condor.desc")
    with desc_filename.open("w") as f:
        f.write(desc_s)
    desc_filename = desc_filename.resolve()
    # module_logger.info('HTCondor desc {}'.format(desc_filename))
    return desc_filename, condor_log

# ----------------------------------------------------------------------

sReCondorProc = re.compile(r'\*\*\s+Proc\s+(\d+)\.(\d+):')

def submit(desc_filename):
    output = _run("condor_submit", "-verbose", str(desc_filename))
    cluster = collections.defaultdict(int)
    for line in output.splitlines():
        m = sReCondorProc.match(line)
        if m:
            cluster[m.group(1)] += 1
    if not cluster:
        logging.error(output)
        raise RuntimeError("cluster id not found in the submission results {}".format(cluster))
    return dict(cluster)

# ----------------------------------------------------------------------

sMachineName = {"albertine": "i19", "odette": "i18", "i18": "i18"}
def fix_machine_name(machine):
    return sMachineName.get(machine, machine)

# ----------------------------------------------------------------------

def _run(*program_and_args):
    return subprocess.check_output(program_and_args, env={"LD_LIBRARY_PATH": ""}).decode("utf-8")

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
