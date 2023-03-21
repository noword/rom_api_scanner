from pathlib import Path
import ar
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection
from io import BytesIO
import os
import pickle
import gzip


def find_codes(io):
    symbols = []
    elf = ELFFile(io)
    for section in elf.iter_sections():
        if isinstance(section, SymbolTableSection):
            for symbol in section.iter_symbols():
                if symbol['st_info'].type == 'STT_FUNC' and symbol['st_size'] > 0:
                    symbols.append(symbol)
    results = {}
    for symbol in symbols:
        section = elf.get_section(symbol['st_shndx'])
        io.seek(section['sh_offset'] + symbol['st_value'], os.SEEK_SET)
        buf = io.read(symbol['st_size'])
        results[buf] = symbol.name
    return results


def update(io):
    global results
    for buf, name in find_codes(io).items():
        if buf in results:
            if name not in results[buf]:
                results[buf] += f' / {name}'
        else:
            results[buf] = name


if __name__ == '__main__':
    results = {}
    for path in Path('libs').rglob('*'):
        print(path)
        if path.suffix == '.a':
            with open(path, 'rb') as fp:
                for archive in ar.Archive(fp):
                    print(' ', archive.name)
                    update(BytesIO(archive.get_stream(fp).read()))
        elif path.suffix == '.o':
            update(open(path, 'rb'))

    pickle.dump(results, gzip.open('libs.pickle.gz', 'wb'))
