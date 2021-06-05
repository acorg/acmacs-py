import sys, datetime
from pathlib import Path

# ----------------------------------------------------------------------

class Log:

    def __init__(self, path :Path):
        self.file = path.open("a")

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.file.close()

    def info(self, before_newline :str = None, after_newline :str = None):
        self.file.write(f"{now()}:")
        if before_newline:
            self.file.write(f" {before_newline}")
        self.file.write("\n")
        if after_newline:
            self.file.write(f"{after_newline}\n")

    def message(self, message):
        self.file.write(f"{message}\n")

    def newline(self):
        self.file.write("\n")

    def name(self):
        return self.file.name

    def separator(self, length = 70, newlines_before = 0, newlines_after = 2):
        self.file.write("\n" * newlines_before)
        self.file.write("=" * length)
        self.file.write("\n" * newlines_after)

    def flush(self):
        self.file.flush()

# ----------------------------------------------------------------------

def info(message):
    print(f"""{now()}: {message}""")

def warning(message):
    msg = f"""{now()}: WARNING {message}"""
    print(msg)
    print(msg, file=sys.stderr)

def error(message):
    msg = f"""{now()}: ERROR {message}"""
    print(msg)
    print(msg, file=sys.stderr)

def now():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
