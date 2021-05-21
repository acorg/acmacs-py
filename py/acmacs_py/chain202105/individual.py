from acmacs_py import *
from .chain_base import ChainBase

class IndividualTableMaps (ChainBase):

    def __init__(self, tables :[Path], **kwargs):
        super().__init__(**kwargs)
        self.tables = tables

    def output_dir(self):
        return self.output_root_dir.joinpath("i")

    def run(self, submitter, chain_setup):
        output_dir = self.output_dir()
        output_dir.mkdir(parents=True, exist_ok=True)
        for table in self.tables:
            output_path = output_dir.joinpath(table.name)
            if self.older_than(output_path, table):
                options = [
                    "-n", chain_setup.number_of_optimizations(),
                    "-d", chain_setup.number_of_dimensions(),
                    "-m", chain_setup.minimum_column_basis(),
                    "--keep-projections", chain_setup.projections_to_keep(),
                    ]
                reorient_to = chain_setup.reorient_to()
                if reorient_to:
                    options.extend(["--reorient", reorient_to])
                if not chain_setup.disconnect_having_few_titers():
                    options.append("--no-disconnect-having-few-titers")
                submitter.submit(["chart-relax-grid", *options, table, output_path])

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
