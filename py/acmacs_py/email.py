# -*- Python -*-
# license
# license.
# ----------------------------------------------------------------------

import logging; module_logger = logging.getLogger(__name__)
import sys, os, subprocess, socket, traceback, datetime
from contextlib import contextmanager

# ----------------------------------------------------------------------

def send(**kwargs):
    args = {"to": "eu@antigenic-cartography.org", "subject": f"{socket.gethostname()}: {os.getcwd()}", "body": traceback.format_stack()}
    args.update(kwargs)
    subprocess.check_call(["/usr/bin/mail", "-s", args["subject"], args["to"]], input=args["body"])

# ----------------------------------------------------------------------

@contextmanager
def send_after(**kwargs):
    start = datetime.datetime.utcnow()
    args = {"to": "eu@antigenic-cartography.org", "subject": f"{socket.gethostname()}: {sys.argv[0]} {os.getcwd()}", "body": f"{sys.argv}"}
    args.update(kwargs)
    try:
        yield
    except Exception as err:
        elapsed = datetime.datetime.utcnow() - start
        args["subject"] += f"FAILED in {elapsed}"
        args["body"] += f"\n\FAILED in {elapsed}: {err}\n\n{traceback.format_exc()}"
        send(**args)
    else:
        elapsed = datetime.datetime.utcnow() - start
        args["subject"] += f"completed in {elapsed}"
        args["body"] += f"\n\ncompleted in {elapsed}\n"
        send(**args)

# ----------------------------------------------------------------------

# def send(to, subject, body):
#     module_logger.info('About to send email to {} subject: {}'.format(to, subject))
#     msg = MIMEText(body)
#     msg["Subject"] = subject
#     msg["To"] = to
#     msg["From"] = "{}@{}".format(getpass.getuser(), socket.getfqdn())
#     s = smtplib.SMTP("localhost")
#     s.send_message(msg)
#     s.quit()

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
