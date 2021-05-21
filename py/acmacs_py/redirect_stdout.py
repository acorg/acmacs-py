import sys, os
from pathlib import Path

def redirect_stdout(stdout :Path, stderr :Path = None):
    original_stdout_fd = sys.stdout.fileno()
    # sys.stdout.close()
    original_stderr_fd = sys.stderr.fileno()
    # sys.stderr.close()
    new_stdout = stdout.open("w")   # do not open inside os.dup2 call to avoid immediate closing of the file
    if stderr is not None:
        new_stderr = stderr.open("w")   # do not open inside os.dup2 call to avoid immediate closing of the file
    else:
        new_stderr = new_stdout
    os.dup2(new_stdout.fileno(), original_stdout_fd)
    os.dup2(new_stderr.fileno(), original_stderr_fd)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
