# -*- Python -*-
# license
# license.
# ----------------------------------------------------------------------

import sys, os, subprocess, socket, traceback, datetime
from pathlib import Path
from contextlib import contextmanager

# ----------------------------------------------------------------------

HOSTNAME = socket.gethostname()

DEFAULT_ARGS = {
    "to": "whocc-chain@antigenic-cartography.org",
    "subject": f"{HOSTNAME}: {Path(sys.argv[0]).name} {os.getcwd()}",
    "body": traceback.format_stack()
}

# ----------------------------------------------------------------------

def send(**kwargs):
    args = {**DEFAULT_ARGS, "body": traceback.format_stack(), **kwargs}
    subprocess.run(["/usr/bin/mail", "-s", args["subject"], args["to"]], input=args["body"].encode("utf-8"), check=True)

# ----------------------------------------------------------------------

@contextmanager
def send_after(**kwargs):
    start = datetime.datetime.utcnow()
    args = {**DEFAULT_ARGS, "body": f"""'{"' '".join(sys.argv)}'""", **kwargs}
    try:
        yield args
    except Exception as err:
        elapsed = datetime.datetime.utcnow() - start
        args["subject"] = f"FAILED on {HOSTNAME} {args['subject']}"
        args["body"] = f"{HOSTNAME} FAILED in {elapsed}: {err}" + "\n\n" + args["body"] + "\n\n" + traceback.format_exc()
        send(**args)
        raise
    else:
        elapsed = datetime.datetime.utcnow() - start
        args["subject"] += f" completed on {HOSTNAME} -- {args['subject']}"
        args["body"] = f"{HOSTNAME} completed in {elapsed}" + "\n\n" + args["body"]
        send(**args)

# ======================================================================
