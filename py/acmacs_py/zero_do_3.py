# 0do.py support, e.g. ssm report custom

import sys, os, time, datetime, subprocess, json, pprint, traceback
from pathlib import Path
from typing import List, Union, Callable
import acmacs

# ======================================================================

def main():

    def main_commands():
        return [name for name, value in vars(sys.modules["__main__"]).items() if name[0] != "_" and name != "Path" and callable(value)]

    def parse_command_line():
        import argparse
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument("--command-list", action='store_true', default=False)
        parser.add_argument("command", nargs='?')
        args = parser.parse_args()
        if args.command_list:
            print("\n".join(main_commands()))
            exit(0)
        if args.command:
            return args.command
        else:
            return main_commands()[0]

    command = parse_command_line()
    try:
        zd = Zd()
        cmd = getattr(sys.modules["__main__"], command)
        zd.snapshot_section = zd.snapshot_data.section(cmd)
        return cmd(zd)
    except Error as err:
        print(f"> {err}", file=sys.stderr)
        return 1
    except Exception as err:
        print(f"> {type(err)}: {err}\n{traceback.format_exc()}", file=sys.stderr)
        return 2

# ======================================================================

class Zd:

    def __init__(self):
        self.mapi_key = None
        self.snapshot_data = Snapshot()
        self.snapshot_section = None

# ======================================================================

class Snapshot:

    def __init__(self):
        self.filename = Path("snapshot.json")
        if self.filename.exists():
            self.data = json.load(self.filename.open())
        else:
            self.data = {"sections": []}

    def __del__(self):
        self.save()
        self.generate_html()

    def save(self):
        json.dump(self.data, self.filename.open("w"), indent=2)

    def section(self, cmd):
        for sec in self.data["sections"]:
            if sec["name"] == cmd.__name__:
                return sec
        sec = {"name": cmd.__name__, "doc": cmd.__doc__, "images": []}
        self.data["sections"].append(sec)
        return sec

    def generate_html(self):
        pass

# ======================================================================

class Error (Exception):
    pass

# ----------------------------------------------------------------------

class ErrorJSON (Error):

    def __init__(self, filename: Union[str,Path], err: json.decoder.JSONDecodeError):
        self.message = f"{filename}:{err.lineno}:{err.colno}: {err.msg}"

    def __str__(self) -> str:
        return self.message

# ----------------------------------------------------------------------
