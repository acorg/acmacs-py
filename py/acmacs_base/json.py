# -*- Python -*-
# license
# license.
# ======================================================================

import logging; module_logger = logging.getLogger(__name__)
from pathlib import Path
import json

# ----------------------------------------------------------------------

def _json_simple(d):
    r = True
    if isinstance(d, dict):
        r = not any(isinstance(v, (list, tuple, set, dict)) for v in d.values()) and len(d) < 17
    elif isinstance(d, (tuple, list)):
        r = not any(isinstance(v, (list, tuple, set, dict)) for v in d)
    return r

# ----------------------------------------------------------------------

def dumps(data, separators=[',', ': '], indent=None, compact=True, sort_keys=False, simple=_json_simple, one_line_max_width=200, object_fields_sorting_key=None):
    # module_logger.info('json.dumps: {!r}'.format(data))
    if indent is not None and compact:
        data = _json_dumps(data, indent=indent, indent_increment=indent, simple=simple, one_line_max_width=one_line_max_width, object_fields_sorting_key=object_fields_sorting_key)
    else:
        data = json.dumps(data, separators=separators, indent=indent, sort_keys=sort_keys)
        if indent:
            data = "{{{:<{}s}\"_\":\"-*- js-indent-level: {} -*-\",".format("", indent - 1, indent) + data[1:]
    return data

# ----------------------------------------------------------------------

class JSONEncoder (json.JSONEncoder):

    def default(self, o):
        if isinstance(o, Path):
            r = str(o)
        # elif hasattr(o, "json"):
        #     r = o.json()
        else:
            r = "<" + str(type(o)) + " : " + repr(o) + ">"
        return r

# ----------------------------------------------------------------------

def _json_dumps(data, indent=2, indent_increment=None, simple=_json_simple, toplevel=True, one_line_max_width=200, object_fields_sorting_key=None):
    """More compact dumper with wide lines."""

    def end(symbol, indent):
        if indent > indent_increment:
            r = "{:{}s}{}".format("", indent - indent_increment, symbol)
        else:
            r = symbol
        return r

    def make_one_line(data):
        if isinstance(data, set):
            s = json.dumps(sorted(data, key=object_fields_sorting_key), cls=JSONEncoder)
        elif isinstance(data, dict):
            s = "{"
            for no, k in enumerate(sorted(data, key=object_fields_sorting_key), start=1):
                comma = ", " if no < len(data) else ""
                s += "{}: {}{}".format(json.dumps(k, cls=JSONEncoder), _json_dumps(data[k], indent=0, indent_increment=None, simple=simple, toplevel=False, object_fields_sorting_key=object_fields_sorting_key), comma)
            s += "}"
        else:
            s = json.dumps(data, sort_keys=True, cls=JSONEncoder)
        return s

    def make_object(data):
        if toplevel:
            r = ["{{{:<{}s}\"_\":\"-*- js-indent-level: {} -*-\",".format("", indent_increment - 1, indent_increment)]
        else:
            r = ["{"]
        for no, k in enumerate(sorted(data, key=object_fields_sorting_key), start=1):
            comma = "," if no < len(data) else ""
            r.append("{:{}s}{}: {}{}".format("", indent, json.dumps(k, cls=JSONEncoder), _json_dumps(data[k], indent + indent_increment, indent_increment, simple=simple, toplevel=False, object_fields_sorting_key=object_fields_sorting_key), comma))
        r.append(end("}", indent))
        return r

    # --------------------------------------------------

    if indent_increment is None:
        indent_increment = indent
    if indent == 0 or simple(data):
        s = make_one_line(data)
    else:
        r = []
        if isinstance(data, dict):
            r.extend(make_object(data))
        elif isinstance(data, (tuple, list)):
            r.append("[")
            for no, v in enumerate(data, start=1):
                comma = "," if no < len(data) else ""
                r.append("{:{}s}{}{}".format("", indent, _json_dumps(v, indent + indent_increment, indent_increment, simple=simple, toplevel=False, object_fields_sorting_key=object_fields_sorting_key), comma))
            r.append(end("]", indent))
        else:
            raise ValueError("Cannot serialize: {!r}".format(data))
        s = "\n".join(r)
        if "\n" in s and len(s) < one_line_max_width:
            s = make_one_line(data)
    return s

# ----------------------------------------------------------------------

loads = json.loads

# ----------------------------------------------------------------------

def read(path :Path):
    from .files import read_text
    try:
        return loads(read_text(path))
    except json.decoder.JSONDecodeError as err:
        raise RuntimeError(f"cannot parse json in \"{path}\": {err}")

read_json = read

# ----------------------------------------------------------------------

def write(path :Path, data, indent=2, compact=True, object_fields_sorting_key=None):
    from .files import backup_file
    backup_file(path)
    from .files import write_binary
    write_binary(path=path, data=(dumps(data, indent=indent, compact=compact, sort_keys=True, object_fields_sorting_key=object_fields_sorting_key) + "\n").encode("utf-8"))

write_json = write

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
