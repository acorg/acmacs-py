import sys, datetime

# ----------------------------------------------------------------------

def info(message):
    print(f"""{now()} INFO {message}""")

def warning(message):
    msg = f"""{now()} WARNING {message}"""
    print(msg)
    print(msg, file=sys.stderr)

def error(message):
    msg = f"""{now()} ERROR {message}"""
    print(msg)
    print(msg, file=sys.stderr)

def now():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
