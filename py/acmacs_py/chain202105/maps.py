from acmacs_py import *
from . import utils
from .log import Log
import acmacs

# ----------------------------------------------------------------------

class MapMaker:

    def __init__(self, chain_setup, minimum_column_basis, log :Log):
        self.chain_setup = chain_setup
        self.minimum_column_basis = minimum_column_basis
        self.log = log

    def individual_map_directory_name(self):
        return f"i-{self.minimum_column_basis}"

    def command(self, source :Path, target :Path):
        """returns command (list) or None if making is not necessary (already made)"""
        target.parent.mkdir(parents=True, exist_ok=True)
        if utils.older_than(target, source):
            return [self.command_name(), *self.command_args(), "--grid-json", target.with_suffix(".grid.json"), source, target]
        else:
            self.log.info(f"{target} up to date")
            return None

    def command_name(self):
        return "chart-relax-grid"

    def command_args(self):
        return [
            "-n", self.chain_setup.number_of_optimizations(),
            "-d", self.chain_setup.number_of_dimensions(),
            "-m", self.minimum_column_basis,
            *self.args_keep_projections(),
            *self.args_reorient(),
            *self.args_disconnect()
            ]

    def args_keep_projections(self):
        return ["--keep-projections", self.chain_setup.projections_to_keep()]

    def args_reorient(self):
        reorient_to = self.chain_setup.reorient_to()
        if reorient_to:
            return ["--reorient", reorient_to]
        else:
            return []

    def args_disconnect(self):
        if not self.chain_setup.disconnect_having_few_titers():
            return ["--no-disconnect-having-few-titers"]
        else:
            return []

    @classmethod
    def add_threads_to_commands(cls, threads :int, commands :list[list]):
        """Modifies commands to make it limit threads number. Returns modified command"""
        return [command + ["--threads", threads] for command in commands]

# ----------------------------------------------------------------------

class IndividualMapMaker (MapMaker):

    pass

# ----------------------------------------------------------------------

class IndividualMapWithMergeColumnBasesMaker (MapMaker):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # self.output_dir_name = output_dir_name
        self.source = None      # nothing to do
        self.target = None      # nothing to do

    def prepare(self, source :Path, merge_column_bases :dict, merge_path :Path, output_dir :Path, output_prefix :str):
        self.log.info(f"Individual table map ({source.name}) with column bases from the merge ({merge_path.name})")
        chart = acmacs.Chart(source)
        mcb_source = output_dir.joinpath(f"{output_prefix}{chart.date()}.mcb-table{source.suffix}")
        mcb_target = output_dir.joinpath(f"{output_prefix}{chart.date()}.mcb{source.suffix}")
        if utils.older_than(mcb_target, source):
            cb = chart.column_bases(self.minimum_column_basis)
            orig_cb = str(cb)
            updated = False
            for sr_no, serum in chart.select_all_sera():
                mcb = merge_column_bases.get(serum.name_full())
                if mcb is None:
                    message = f"No column basis for {serum.name_full()} in the merge column bases (source: {source.name}:\n{pprint.pformat(merge_column_bases, width=200)}"
                    self.log.info(f"ERROR {message}")
                    raise RuntimeError(message)
                if mcb != cb[sr_no]:
                    if mcb < cb[sr_no]:
                        message = f"Column basis for {serum.name_full()} in the merge ({mcb}) is less than in the individual table ({cb[sr_no]})"
                        self.log.info(f"ERROR {message}")
                        raise RuntimeError(message)
                    cb[sr_no] = mcb
                    updated = True
            if updated:
                chart.column_bases(cb)
                self.log.info(f"{mcb_source} <-- {source}: column basis updated from merge:\n    orig: {orig_cb}\n     new: {cb}")
                self.source = mcb_source
                self.target = mcb_target
                chart.export(self.source, program_name=sys.argv[0])
            else:
                self.log.info("column basis in the merge are the same as in the original individual table")
        else:
            self.log.info(f"{mcb_source} up to date")
        self.log.separator(newlines_before=1)

# ----------------------------------------------------------------------

class IncrementalMapMaker (MapMaker):

    def command_name(self):
        return "chart-relax-incremental"

    def command_args(self):
        return [
            "-n", self.chain_setup.number_of_optimizations(),
            "--grid-test",
            "--remove-source-projection",
            *self.args_keep_projections(),
            # *self.args_reorient(),
            *self.args_disconnect()
            ]

# ----------------------------------------------------------------------

def extract_column_bases(chart):
    return {serum.name_full(): chart.column_basis(sr_no) for sr_no, serum in chart.select_all_sera()}

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
