# -*- Python -*-
# ======================================================================

from pathlib import Path

# ----------------------------------------------------------------------

def read(filename :Path):
    """read fasta file and return dict {name: sequence}"""
    data = {}
    name = None
    sequence = ""
    for line in filename.open("r"):
        line = line.strip()
        if not line:
            pass
        elif line[0] == ">":
            if name:
                data[name] = sequence
            name = line[1:]
            sequence = ""
            # print(name)
        else:
            sequence += line
    if name:
        data[name] = sequence
    return data

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
