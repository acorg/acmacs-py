# -*- Python -*-
# license
# license.

"""
API to access AcmacsWeb server
"""

import sys, os, urllib.request, hashlib, json, traceback
import logging; module_logger = logging.getLogger(__name__)

# ----------------------------------------------------------------------

sApi = None

def acmacs_api(session=None, user=None, url_prefix="https://acmacs-web.antigenic-cartography.org"):
    global sApi
    if sApi is None:
        sApi = API(url_prefix=url_prefix)
        if (session is not None):
            sApi.session = session
        elif user is not None:
            import getpass
            sApi._login(user=user, password=getpass(prompt="AcmacsWeb password:"))
    if url_prefix is not None:
        sApi.url_prefix = url_prefix
    return sApi

# ----------------------------------------------------------------------

class CommandError (Exception):
    """Raised by api._execute if command resposne contains error and raise_error flag is set."""

class LoginFailed (Exception):
    """Raised on login failure"""

# ======================================================================

class API:

    def __init__(self, url_prefix):
        self.url_prefix = url_prefix
        # module_logger.info('acmacs url_prefix {}'.format(self.url_prefix), stack_info=True)

    def command(self, command):
        cmd = {"S": self.session, **command}
        return self._execute(cmd)

    def _execute(self, command):
        if self.url_prefix:
            response = self._execute_http(command)
        else:
            raise ValueError('No url_prefix: ' + repr(self.url_prefix))
        if isinstance(response, dict) and response.get('E'):
            raise CommandError(response['E'])
        return response

    def _login(self, user, password):
        response = self.execute(command=dict(F='acmacs-base', C='login_nonce', user=user), print_response=False)
        if response.output.get('E'):
            raise LoginFailed(response.output['E'])
        # module_logger.debug('login_nonce user:{} nonce:{}'.format(user, response.output))
        digest = self._.hash_password(user=user, password=password)
        cnonce = '{:X}'.format(random.randrange(0xFFFFFFFF))
        password = self._hash_nonce_digest(nonce=response.output['nonce'], cnonce=cnonce, digest=digest)
        response = self._execute(command=dict(F='acmacs-base', C='login', user=user, cnonce=cnonce, password=password, application=sys.argv[0]), print_response=False)
        module_logger.debug('response {}'.format(response))
        if response.output.get('E'):
            raise LoginFailed(response.output['E'])
        self.session = response.output['S']
        module_logger.info('--session={}'.format(self.session))

    def _execute_http(self, command):
        command['F'] = 'json'
        if True: # "localhost" in self.url_prefix:
            import ssl
            context = ssl.create_default_context()
            context.check_hostname = False
            context.verify_mode = ssl.CERT_NONE
        else:
            context = None
        # print(command)
        response = urllib.request.urlopen(url='{}/api'.format(self.url_prefix), data=json.dumps(command).encode('utf-8'), context=context).read()
        return json.loads(response.decode('utf-8'))

    def _hash_password(self, user, password):
        m = hashlib.md5()
        m.update(';'.join((user, 'acmacs-web', password)).encode('utf-8'))
        return m.hexdigest()

    def _hash_nonce_digest(self, nonce, cnonce, digest):
        m = hashlib.md5()
        m.update(';'.join((nonce, cnonce, digest)).encode('utf-8'))
        return m.hexdigest()

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
