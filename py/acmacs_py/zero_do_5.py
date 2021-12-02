# 0do.py v5 support, e.g. ssm report custom

import sys, os, json, subprocess, re, pprint, traceback
from pathlib import Path
from contextlib import contextmanager
from collections.abc import Callable
import acmacs

# ======================================================================

class Error (Exception):
    pass

# ----------------------------------------------------------------------

class ErrorJSON (Error):

    def __init__(self, filename: str|Path, err: json.decoder.JSONDecodeError):
        self.message = f"{filename}:{err.lineno}:{err.colno}: {err.msg}"

    def __str__(self) -> str:
        return self.message

# ======================================================================

class Zd:

    def __init__(self, cmd):
        self.num_slots = 0
        self.section(cmd)

    def section(self, cmd):
        print("section", cmd)

    # ----------------------------------------------------------------------

    def slot(self, func: Callable[[any], dict]) -> dict:
        slot_name = func.__qualname__.replace("<locals>", f"{self.num_slots:02d}")
        with self.slot_context(slot_name) as sl:
            return func(sl)

    @contextmanager
    def slot_context(self, slot_name: str):
        slot = Slot(self, slot_name)
        try:
            yield slot
        finally:
            slot.finalize()

# ----------------------------------------------------------------------

class Slot:

    def __init__(self, zd: Zd, slot_name: str):
        self.zd = zd
        print("slot init", slot_name)

    def finalize(self):
        print("slot finalize")

# ======================================================================

def main():

    def main_commands():
        return [name for name, value in vars(sys.modules["__main__"]).items() if name[0] != "_" and name != "Path" and callable(value)]

    def parse_command_line():
        import argparse
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument("--command-list", action='store_true', default=False)
        parser.add_argument("--help-api", action='store_true', default=False)
        parser.add_argument("command", nargs='?')
        args = parser.parse_args()
        if args.command_list:
            print("\n".join(main_commands()))
            exit(0)
        if args.help_api:
            help(Zd)
            help(Painter)
            help(Snapshot)
            exit(0)
        if args.command:
            return args.command
        else:
            return main_commands()[0]

    command = parse_command_line()
    try:
        cmd = getattr(sys.modules["__main__"], command)
        zd = Zd(cmd)
        return cmd(zd)
    except Error as err:
        print(f"> {err}", file=sys.stderr)
        return 1
    except Exception as err:
        print(f"> {type(err)}: {err}\n{traceback.format_exc()}", file=sys.stderr)
        return 2

# ======================================================================
