import datetime, concurrent.futures, socket
from acmacs_py import KnownError, Path
from .submitter import submitter_factory

# ----------------------------------------------------------------------

def run(chain_dir :Path):
    # with email.send_after():
    ChainRunner(chain_dir=chain_dir).run()

# ----------------------------------------------------------------------

class ChainRunner:

    def __init__(self, chain_dir :Path):
        self.chain_dir = chain_dir.resolve()
        self.chain_setup = None
        self.log_dir = None

    def run(self):
        self.load_setup()
        self.setup_log()
        self.submitter = submitter_factory()
        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures = [executor.submit(self.run_chain, chain=chain) for chain in self.chain_setup.chains()]
            for future in concurrent.futures.as_completed(futures):
                print(future.result())
        if self.submitter.is_failed():
            self.submitter.report_failures()
            raise KnownError(f"Parts of chains FAILED, see {socket.gethostname()}:{self.log_dir}")

    def run_chain(self, chain):
        chain.set_output_root_dir(self.chain_dir)
        chain.run(submitter=self.submitter, chain_setup=self.chain_setup, log_dir=self.log_dir)

    def setup_log(self):
        self.log_dir = self.chain_dir.joinpath("log", datetime.datetime.now().strftime("%y%m%d-%H%M%S"))
        self.log_dir.mkdir(parents=True)
        from acmacs_py.redirect_stdout import redirect_stdout
        redirect_stdout(stdout=self.log_dir.joinpath("Out.log"), stderr=self.log_dir.joinpath("Err.log"))

    def load_setup(self):
        locls = {}
        setup_path = self.chain_dir.joinpath("Setup.py")
        try:
            exec(setup_path.open().read(), globals(), locls)
        except FileNotFoundError:
            raise KnownError(f"invalid chain dir: no {setup_path}")
        try:
            chain_setup_cls = locls["ChainSetup"]
        except KeyError:
            raise KnownError(f"invalid chain setup ({setup_path}): ChainSetup class no defined")
        self.chain_setup = chain_setup_cls()


# ----------------------------------------------------------------------

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
