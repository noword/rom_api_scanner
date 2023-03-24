from pathlib import Path
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import Section, SymbolTableSection
from elftools.elf.relocation import RelocationSection
from io import BytesIO
from struct import pack
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
        s = s.replace('.' * n, f'.{{{n}}}')
    return s


def find_functions(io):
    texts = {}
    symbols = []
    relocs = {}

    elf = ELFFile(io)
    for i, section in enumerate(elf.iter_sections()):
        if isinstance(section, SymbolTableSection):
            for symbol in section.iter_symbols():
                if symbol['st_info'].type == 'STT_FUNC' and symbol['st_size'] > 0:
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
    for symbol in symbols:
        start = symbol['st_value'] * 2
        end = start + symbol['st_size'] * 2
        buf = to_pattern_str(texts[symbol['st_shndx']][start:end])
        results[buf] = symbol.name

    return results


def update(io):
    global results
    for buf, name in find_functions(io).items():
        if buf in results:
            if name not in results[buf]:
                results[buf] += f' / {name}'
        else:
            results[buf] = name


def save_to_bin(io, data):
    io.write(pack('I', len(data)))
    for p, name in data:
        p = p.encode('ascii')
        name = name.encode('ascii')
        io.write(pack('I', len(name) + 1))
        io.write(name + b'\x00')
        io.write(pack('I', len(p) + 1))
        io.write(p + b'\x00')


if __name__ == '__main__':
    results = {}
    for path in Path('libs').rglob('*'):
        print(path)
        if path.suffix == '.a':
            with open(path, 'rb') as fp:
                for archive in ar.Archive(fp):
                    # print(' ', archive.name)
                    update(BytesIO(archive.get_stream(fp).read()))
        elif path.suffix == '.o':
            update(open(path, 'rb'))

    def check_short(v):
        return not(len(v[0]) <= 16 and v[1].count('/') > 2)

    db = DB(filter(check_short, results.items()))
    db.sort(key=lambda x: len(x[0]), reverse=True)
    db.save(open('libs.bin', 'wb'))
