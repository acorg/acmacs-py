# -*- Python -*-
# license
# license.
# ----------------------------------------------------------------------

import smtplib, socket, getpass
from email.mime.text import MIMEText
import logging; module_logger = logging.getLogger(__name__)

# ----------------------------------------------------------------------

def send(to, subject, body):
    module_logger.info('About to send email to {} subject: {}'.format(to, subject))
    msg = MIMEText(body)
    msg["Subject"] = subject
    msg["To"] = to
    msg["From"] = "{}@{}".format(getpass.getuser(), socket.getfqdn())
    s = smtplib.SMTP("localhost")
    s.send_message(msg)
    s.quit()

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
