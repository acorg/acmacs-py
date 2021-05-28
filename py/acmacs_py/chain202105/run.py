import sys, datetime, concurrent.futures, socket
from . import error
from acmacs_py import KnownError, Path, open_in_emacs
from .runner import runner_factory

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
        start = datetime.datetime.now()
        self.load_setup()
        self.setup_log()
        try:
            self.runner = runner_factory(log_prefix=self.log_prefix)
            with concurrent.futures.ThreadPoolExecutor() as executor:
                futures = [executor.submit(self.run_chain, chain=chain) for chain in self.chain_setup.chains()]
                for future in concurrent.futures.as_completed(futures):
                    future.result()
            if self.runner.is_failed():
                self.runner.report_failures()
                raise KnownError(f"Parts of chains FAILED, see {socket.gethostname()}:{self.log_dir}")
        except:
            sys.stderr.flush()
            open_in_emacs(self.stderr_file.parent)
            raise
        finally:
            print(f"chain run time: {datetime.datetime.now() - start}")
            
    def run_chain(self, chain):
        chain.set_output_root_dir(self.chain_dir)
        try:
            chain.run(runner=self.runner, chain_setup=self.chain_setup)
        except error.RunFailed:
            pass                # will be reported by self.run() and self.runner upon completion of other threads

    def setup_log(self):
        self.log_dir = self.chain_dir.joinpath("log") #, datetime.datetime.now().strftime("%y%m%d-%H%M%S"))
        self.log_dir.mkdir(parents=True, exist_ok=True)
        self.log_prefix = str(self.log_dir.joinpath(datetime.datetime.now().strftime("%y%m%d-%H%M%S-")))
        self.stdout_file = Path(self.log_prefix + "Out.log")
        self.stderr_file = Path(self.log_prefix + "Err.log")
        print(f"{self.stdout_file.parent}")
        from acmacs_py.redirect_stdout import redirect_stdout
        redirect_stdout(stdout=self.stdout_file, stderr=self.stderr_file)

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