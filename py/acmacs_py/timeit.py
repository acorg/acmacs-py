# -*- Python -*-
# license
# license.
# ======================================================================

import datetime
import logging; module_logger = logging.getLogger(__name__)
from contextlib import contextmanager

@contextmanager
def timeit(name, logging_level=logging.DEBUG):
    start = datetime.datetime.utcnow()
    try:
        yield
    except Exception as err:
        module_logger.warning('{} <{}> with error {}'.format(name, datetime.datetime.utcnow() - start, err))
        raise
    else:
        module_logger.log(logging_level, '{} <{}>'.format(name, datetime.datetime.utcnow() - start))

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
