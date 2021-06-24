import sys, datetime
from pathlib import Path

# ----------------------------------------------------------------------

class Log:

    def __init__(self, path :Path):
        existed = path.exists()
        self.file = path.open("a")
        if not existed:
            self.file.write("-*- log-chain202105 -*-\n")
            self.file.flush()

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.file.close()

    def info(self, before_newline :str = None, after_newline :str = None):
        self.file.write(f"{now()}")
        if before_newline:
            self.file.write(f" {before_newline}")
        self.file.write("\n")
        if after_newline:
            self.file.write(f"{after_newline}\n")

    def message(self, *message_lines, timestamp=True, flush=False):
        if timestamp:
            self.file.write(now())
            if message_lines and message_lines[0]:
                self.file.write(" ")
        self.file.write("\n".join(message_lines))
        self.file.write("\n")
        if flush:
            self.file.flush()

    def newline(self):
        self.file.write("\n")

    def name(self):
        return self.file.name

    def separator(self, symbol="=", length = 150, newlines_before = 0, newlines_after = 2):
        self.file.write("\n" * newlines_before)
        self.file.write(symbol * length)
        self.file.write("\n" * newlines_after)

    def flush(self):
        self.file.flush()

# ----------------------------------------------------------------------

def info(message):
    print(f"""{now()} {message}""")

def warning(message):
    msg = f"""{now()} WARNING {message}"""
    print(msg)
    print(msg, file=sys.stderr)

def error(message):
    msg = f"""{now()} ERROR {message}"""
    print(msg)
    print(msg, file=sys.stderr)

def now():
    return f"""<{datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")}>"""

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
