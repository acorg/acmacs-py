# -*- Python -*-
# license
# license.
# ======================================================================

import logging; module_logger = logging.getLogger(__name__)

# ----------------------------------------------------------------------

def encode(name):
    for char in "% :()!*';@&=+$,?#[]": # the samae as in ../cc/name-encode.hh
        name = name.replace(char, '%{:02X}'.format(ord(char)))
    return name

# ----------------------------------------------------------------------

def decode(name):
    return name

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
