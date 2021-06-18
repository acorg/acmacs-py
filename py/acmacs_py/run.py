import sys, logging, argparse, traceback
from .error import KnownError

def execute_this_script(main, arguments, description=None):
    try:
        parser = argparse.ArgumentParser(description=description or sys.modules[main.__module__].__doc__)
        for name_or_flags, kwa in arguments.items():
            if isinstance(name_or_flags, str):
                parser.add_argument(name_or_flags, **kwa)
            else:
                parser.add_argument(*name_or_flags, **kwa)

        parser.add_argument('--debug', action='store_const', dest='loglevel', const=logging.DEBUG, default=logging.INFO, help='Enable debugging output.')
        args = parser.parse_args()
        logging.basicConfig(level=args.loglevel, format="%(levelname)s %(asctime)s: %(message)s")
        exit(main(args) or 0)
    except KnownError as err:
        print(f"""> ERROR: {err}""", file=sys.stderr)
        exit(1)
    except Exception as err:
        print(f"""> ERROR: {err}\n{traceback.format_exc()}""", file=sys.stderr)
        exit(67)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
