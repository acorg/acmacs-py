import sys, datetime, concurrent.futures, socket
from . import error
from acmacs_py import KnownError, Path, open_in_emacs
from .runner import runner_factory
from .log import info

# ----------------------------------------------------------------------

def run(chain_dir :Path, force_local_runner :bool):
    # with email.send_after():
    ChainRunner(chain_dir=chain_dir).run(force_local_runner=force_local_runner)

# ----------------------------------------------------------------------

class ChainRunner:

    def __init__(self, chain_dir :Path):
        self.chain_dir = chain_dir.resolve()
        self.chain_setup = None
        self.log_dir = None

    def run(self, force_local_runner):
        start = datetime.datetime.now()
        self.load_setup()
        self.setup_log()
        try:
            self.runner = runner_factory(log_prefix=self.log_prefix, force_local=force_local_runner)
            self.main_log(f"chain started with {type(self.runner)}")
            with concurrent.futures.ThreadPoolExecutor() as executor:
                futures = [executor.submit(self.run_chain, chain=chain) for chain in self.chain_setup.chains()]
                for future in concurrent.futures.as_completed(futures):
                    info(f"future completed {future}")
                    future.result()
            if self.runner.is_failed():
                self.runner.report_failures()
                raise KnownError(f"Parts of chains FAILED, see {socket.gethostname()}:{self.log_dir}")
        except:
            self.main_log(f"chain FAILED in: {datetime.datetime.now() - start}")
            sys.stderr.flush()
            open_in_emacs(self.stderr_file.parent)
            raise
        else:
            self.main_log(f"chain completed in: {datetime.datetime.now() - start}")

    def run_chain(self, chain):
        chain.set_output_root_dir(self.chain_dir)
        try:
            chain.run(runner=self.runner, chain_setup=self.chain_setup)
        except error.RunFailed:
            pass                # will be reported by self.run() and self.runner upon completion of other threads

    def setup_log(self):
        self.log_dir = self.chain_dir.joinpath("log") #, datetime.datetime.now().strftime("%y%m%d-%H%M%S"))
        self.log_dir.mkdir(parents=True, exist_ok=True)
        self.main_log_file = self.log_dir.joinpath("Out.log").open("a")
        self.log_prefix = str(self.log_dir.joinpath(datetime.datetime.now().strftime("%y%m%d-%H%M%S-")))
        self.stdout_file = Path(self.log_prefix + "z1-out.log")
        self.stderr_file = Path(self.log_prefix + "z2-err.log")
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


    def main_log(self, message, stderr=False, timestamp=True):
        if timestamp:
            message = f"{datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}: {message}"
        print(message)
        if stderr:
            print(message, file=sys.stderr)
        print(message, file=self.main_log_file, flush=True)

# ----------------------------------------------------------------------

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
