# -*- Python -*-
# license
# license.
# ======================================================================

import os, socket, subprocess, tempfile, shutil
from pathlib import Path
from contextlib import contextmanager
import logging; module_logger = logging.getLogger(__name__)

# ----------------------------------------------------------------------

if Path("/r/ramdisk-id").exists():
    TEMP_FILE_DIR = "/r/T"
    Path(TEMP_FILE_DIR).mkdir(parents=True, exist_ok=True)
    BACKUP_ROOT_PATH = Path("/r/backup")
    BACKUP_ROOT_PATH.mkdir(parents=True, exist_ok=True)
else:
    TEMP_FILE_DIR = None
    BACKUP_ROOT_PATH = None

# ----------------------------------------------------------------------

def backup_file(path :Path):
    if path.exists():
        backup_dir = _backup_dir(path)

        version = 1
        def gen_name():
            nonlocal version, backup_dir, path
            version += 1
            return backup_dir.joinpath("{}.~{:03d}~{}".format(path.stem, version - 1, path.suffix))

        while True:
            newname = gen_name()
            # module_logger.debug('backup_file %s %r', newname, newname.exists())
            if not newname.exists():
                # path.rename(newname)
                module_logger.debug('backup_file %s -> %s', path, newname)
                shutil.copy(str(path), str(newname))
                break

# ----------------------------------------------------------------------

def _backup_dir(path :Path):
    parent = path.resolve().parent
    if BACKUP_ROOT_PATH is not None:
        if len(parent.parents) > 3 and str(parent.parents[len(parent.parents) - 3]) == "/Users/eu":
            backup_dir = BACKUP_ROOT_PATH.joinpath(*parent.parts[3:])
        else:
            backup_dir = BACKUP_ROOT_PATH.joinpath(*parent.parts[1:])
    else:
        backup_dir = parent.joinpath(".backup")
    backup_dir.mkdir(parents=True, exist_ok=True)

    backup_link_name = ".backup"
    if not parent.joinpath(backup_link_name).exists():
        parent.joinpath(backup_link_name).symlink_to(backup_dir)
    return backup_dir

# ----------------------------------------------------------------------

def read_pydata(path :Path):
    text = read_text(path)
    if text.strip()[0] == '{':
        text = 'data = ' + text
    data = {}
    exec(text, data)
    return data['data']

# ----------------------------------------------------------------------

def read_json(path :Path):
    from .json import loads
    return loads(read_text(path))

# ----------------------------------------------------------------------

def read_text(path :Path):
    import lzma, bz2
    filename = str(path)
    for opener in [lzma.LZMAFile, bz2.BZ2File, open]:
        f = opener(filename, "rb")
        try:
            f.peek(1)
            break
        except:
            pass
    else:
        raise RuntimeError("Cannot read " + filename)
    return f.read().decode("utf-8")

# ----------------------------------------------------------------------

def write_binary(path :Path, data :bytes):
    if isinstance(path, str):
        path = Path(path)
    if path.suffix == ".xz":
        import lzma
        opener = lzma.LZMAFile
    elif path.suffix == ".bz2":
        import bz2
        opener = bz2.BZ2File
    else:
        opener = open
    if isinstance(data, str):
        data = data.encode("utf-8")
    with opener(str(path), "wb") as f:
        f.write(data)

# ----------------------------------------------------------------------

@contextmanager
def temp_output(output=None, make_temp_output=True, suffix=".pdf"):
    remove_output = not output
    try:
        output = output or (make_temp_output and tempfile.mkstemp(suffix=suffix, dir=TEMP_FILE_DIR)[1])
        yield output
    finally:
        if remove_output and output:
            try:
                if os.path.isdir(output):
                    shutil.rmtree(output)
                else:
                    os.remove(output)
            except Exception as err:
                module_logger.error(err)

# ----------------------------------------------------------------------

def open_image(filename):
    hostname = socket.gethostname()
    if os.environ.get('USER') == 'eu' and hostname[:4] == 'jagd':
        if os.path.isdir(filename):
            import glob
            subprocess.run("qlmanage -p '{}'".format("' '".join(glob.glob(os.path.join(filename, "*.pdf")))), shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            # os.system("open '{}'".format(filename))
            subprocess.run("qlmanage -p '{}'".format(filename), shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
