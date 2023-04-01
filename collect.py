from pathlib import Path
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import Section, SymbolTableSection
from elftools.elf.relocation import RelocationSection
from io import BytesIO
import argparse
import subprocess
import re
import os

# https://github.com/vidstige/ar/
import ar


# https://refspecs.linuxbase.org/elf

# ENUM_RELOC_TYPE_ARM is defined in elftools.elf.enums
ARM_RELOC_BYTES = {
    0: 0,  # R_ARM_NONE
    1: 3,  # R_ARM_PC24
    2: 4,  # R_ARM_ABS32
    10: 4,  # R_ARM_THM_CALL
    28: 3,  # R_ARM_CALL
    29: 3,  # R_ARM_JUMP24
    102: 2,  # R_ARM_THM_JUMP11
}

PPC_RELOC_BYTES = {
    0: 0,  # R_PPC_NONE
    4: 2,  # R_PPC_ADDR16_LO
    5: 2,  # R_PPC_ADDR16_HI
    6: 2,  # R_PPC_ADDR16_HA
    10: 0,  # R_PPC_REL24
    11: 0,  # R_PPC_REL14
}

RELOC_BYTES = {
    'ARM': ARM_RELOC_BYTES,
    'PowerPC': PPC_RELOC_BYTES
}


def fix_reloc(arch, bcodes, offset, reloc_type):
    size = RELOC_BYTES[arch][reloc_type] * 2
    if size > 0:
        offset *= 2
        for i in range(size):
            bcodes[offset + i] = '.'
    return bcodes


def to_pattern_str(hex_list):
    s = ''.join(hex_list).rstrip('.')
    s = re.sub(r'\.{2}', '.', s)
    s = re.sub(r'([0-9a-f]{2})', r'\\x\1', s)
    # for dots in re.findall(r'\.+', s):
    #     s = s.replace(dots, '.{%d}' % len(dots))
    return s


def demangle(name):
    if name.startswith('_Z'):
        s = subprocess.check_output(['c++filt', '-p', name])
        return s.strip().decode('ascii')
    else:
        return name


def __demangle_names(names):
    buf = '\n'.join(names).encode('ascii')
    if b'_Z' in buf:
        proc = subprocess.run('c++filt -p', stdout=subprocess.PIPE, input=buf)
        output = proc.stdout.decode('ascii')
        sep = '\r\n' if '\r\n' in output else '\n'
        names = output.split(sep)
    return names


BATCH_SIZE = 100


def demangle_names(names):
    out = []
    for i in range(0, len(names), BATCH_SIZE):
        out.extend(__demangle_names(names[i:i + BATCH_SIZE]))
    return out


TMP_NAME = 'tmp.txt'


def demangle_ghs_names(names):
    open(TMP_NAME, 'w').write('\n'.join(names))
    # https://github.com/Chadderz121/ghs-demangle
    output = subprocess.check_output(['ghs-demangle', TMP_NAME]).decode('ascii')
    os.remove(TMP_NAME)
    sep = '\r\n' if '\r\n' in output else '\n'
    return output.split(sep)


def check_supported(elf):
    for arch, le in (('ARM', True), ('PowerPC', False)):
        if elf.get_machine_arch() == arch and elf.little_endian == le:
            return True
    return False


def find_functions(io):
    texts = {}  # key is section index, value is hex string
    symbols = []
    relocs = {}  # key is target section index, value is reloc section

    elf = ELFFile(io)
    if not check_supported(elf):
        raise TypeError(f'unsupport arch: {elf.get_machine_arch()}')
    for i, section in enumerate(elf.iter_sections()):
        if isinstance(section, SymbolTableSection):
            for symbol in section.iter_symbols():
                if symbol['st_size'] > 0 and \
                    symbol['st_info'].type in ('STT_FUNC', 'STT_LOPROC') and \
                        isinstance(symbol['st_shndx'], int):
                    symbols.append(symbol)
        elif isinstance(section, RelocationSection):
            relocs[section['sh_info']] = section
        elif isinstance(section, Section):
            texts[i] = list(section.data().hex())

    needed_texts_index = [symbol['st_shndx'] for symbol in symbols]

    for i, section in relocs.items():
        if i in needed_texts_index:
            for reloc in section.iter_relocations():
                # verboseprint(i, reloc['r_info_type'], '%x' % reloc['r_offset'])
                texts[i] = fix_reloc(elf.get_machine_arch(), texts[i], reloc['r_offset'], reloc['r_info_type'])

    results = {}
    names = [s.name for s in symbols]
    if elf.get_machine_arch() == 'PowerPC':
        names = demangle_ghs_names(names)
    else:
        names = demangle_names(names)
    for i, symbol in enumerate(symbols):
        start = symbol['st_value']
        if elf.get_machine_arch() == 'ARM':
            start &= 0xfffffffe
        start *= 2
        end = start + symbol['st_size'] * 2
        # print(symbol['st_shndx'], symbol.name, symbol['st_info'].type, symbol['st_size'])
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
    return not((len(v[0]) <= 32 and v[1].count('/') > 2) or len(v[0]) < 16)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('path', action='store', nargs=1)
    parser.add_argument('out_path', action='store', nargs='?')
    parser.add_argument('name', action='store', nargs='?')
    parser.add_argument('--level', action='store', type=int, nargs='?')
    parser.add_argument('--verbose', action='store_true', default=False)
    args = parser.parse_args()

    verboseprint = print if args.verbose else lambda *a, **k: None

    if args.out_path is None:
        out_path = Path(args.path[0]).name + '.txt'
    else:
        out_path = args.out_path

    if args.level is not None:
        out_path = f'{args.level}_{out_path}'

    if args.name is None:
        name = str(Path(out_path).stem)
    else:
        name = args.name

    results = {}
    for path in Path(args.path[0]).rglob('*'):
        if not path.is_file():
            continue
        print(path)
        if path.suffix == '.a':
            with open(path, 'rb') as fp:
                for archive in ar.Archive(fp):
                    verboseprint(' ', archive.name)
                    update(BytesIO(archive.get_stream(fp).read()))
        elif path.suffix == '.o':
            update(open(path, 'rb'))

    db = list(filter(check_short, results.items()))
    db.sort(key=lambda x: len(x[0]), reverse=True)
    out = [name, ]
    for p, n in db:
        out.append(n)
        out.append(p)

    open(out_path, 'w').write('\n'.join(out))
