from acmacs_py import *

# ----------------------------------------------------------------------

def submitter_factory():
    for submitter_class in [SubmitterSLURM, SubmitterLocal]:
        if submitter_class.enabled():
            return submitter_class()
    raise KnownError("No submitter enabled")

# ----------------------------------------------------------------------

class _SubmitterBase:           # must begin with _ to avoid selecting by list_submitters()

    def __init__(self):
        self.failures = 0

    @classmethod
    def enabled(cls):
        return False

    def is_failed(self):
        return self.failures != 0

# ----------------------------------------------------------------------

class SubmitterLocal (_SubmitterBase):

    @classmethod
    def enabled(cls):
        return True

    def submit(self, command, log_file :Path, **kwargs):
        command = [str(elt) for elt in command]
        print(" ".join(command))
        status = subprocess.run(command, stdout=log_file.open("w"), stderr=subprocess.STDOUT)
        if status.returncode != 0:
            self.failures += 1

# ----------------------------------------------------------------------

class SubmitterSLURM (_SubmitterBase):

    def __init__(self):
        self.threads = 16

    @classmethod
    def enabled(cls):
        try:
            return (subprocess.check_output(["srun", "-V"]).decode("ascii").split()[1] > "19"
                    and subprocess.check_output(["sbatch", "-V"]).decode("ascii").split()[1] > "19")
        except:
            return False

    def submit(self, command :[str, Path], add_threads_to_command, **kwargs):
        command = [str(elt) for elt in add_threads_to_command(self.threads, command)]
        print(f"""SubmitterSLURM.submit: '{"' '".join(command)}'""")

# ----------------------------------------------------------------------

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
