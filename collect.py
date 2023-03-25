from pathlib import Path
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import Section, SymbolTableSection
from elftools.elf.relocation import RelocationSection
from io import BytesIO
import argparse
import subprocess
import re
from db import DB
# https://github.com/vidstige/ar/
import ar


# https://refspecs.linuxbase.org/elf


def fix_reloc(bcodes, offset):
    offset *= 2
    for i in range(8):
        bcodes[offset + i] = '.'
    return bcodes


def to_pattern_str(hex_list):
    s = ''.join(hex_list).strip('.')
    for n in range(20, 0, -1):
        n *= 8
        s = s.replace('.' * n, f'.{{{n//2}}}')
    s = re.sub(r'([0-9a-f]{2})', r'\\x\1', s)
    return s


def demangle(name):
    if name.startswith('_Z'):
        s = subprocess.check_output(['c++filt', '-p', name])
        return s.strip().decode('ascii')
    else:
        return name


def demangle_names(names):
    buf = '\n'.join(names).encode('ascii')
    proc = subprocess.run('c++filt -p', stdout=subprocess.PIPE, input=buf)
    names = proc.stdout.decode('ascii').split('\r\n')
    return names


def find_functions(io):
    texts = {}  # key is section index, value is hex string
    symbols = []
    relocs = {}  # key is target section index, value is reloc section

    elf = ELFFile(io)
    for i, section in enumerate(elf.iter_sections()):
        if isinstance(section, SymbolTableSection):
            for symbol in section.iter_symbols():
                if symbol['st_info'].type in ('STT_FUNC', 'STT_LOPROC'):
                    symbols.append(symbol)
        elif isinstance(section, RelocationSection):
            relocs[section['sh_info']] = section
        elif isinstance(section, Section):
            texts[i] = list(section.data().hex())

    for i, section in relocs.items():
        if i in texts:
            for reloc in section.iter_relocations():
                texts[i] = fix_reloc(texts[i], reloc['r_offset'])

    results = {}
    names = [s.name for s in symbols]
    names = demangle_names(names)
    for i, symbol in enumerate(symbols):
        start = symbol['st_value'] * 2
        if symbol['st_size'] > 0:
            end = start + symbol['st_size'] * 2
        else:
            end = None
        buf = to_pattern_str(texts[symbol['st_shndx']][start:end])
        if buf in results:
            results[buf] += f' / {names[i]}'
        else:
            results[buf] = names[i]

    return results


def update(io):
    global results
    for buf, name in find_functions(io).items():
        if buf in results:
            if name not in results[buf]:
                results[buf] += f' / {name}'
        else:
            results[buf] = name


def check_short(v):
    return not(len(v[0]) <= 8 * 4 and v[1].count('/') > 2)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('path', action='store', nargs=1)
    parser.add_argument('name', action='store', nargs='?')
    args = parser.parse_args()

    if args.name is None:
        name = args.path[0] + '.bin'
    else:
        name = args.name

    results = {}
    for path in Path(args.path[0]).rglob('*'):
        print(path)
        if path.suffix == '.a':
            with open(path, 'rb') as fp:
                for archive in ar.Archive(fp):
                    # print(' ', archive.name)
                    update(BytesIO(archive.get_stream(fp).read()))
        elif path.suffix == '.o':
            update(open(path, 'rb'))

    db = DB([(n, p) for p, n in filter(check_short, results.items())])
    db.sort(key=lambda x: len(x[1]), reverse=True)
    db.save(open(name, 'wb'))
