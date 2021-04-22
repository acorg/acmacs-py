# -*- Python -*-
# license
# license.
# ----------------------------------------------------------------------

import logging; module_logger = logging.getLogger(__name__)
module_logger.warning(f"{__module__.__name__} obsolete, use utils.email")

from ..utils.email import *

# ----------------------------------------------------------------------

# import smtplib, socket, getpass
# from email.mime.text import MIMEText

# # ----------------------------------------------------------------------

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
