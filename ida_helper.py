import idc
import ida_kernwin

path = ida_kernwin.ask_file(False, 'txt file|*.txt;*.sym', 'Choose a symbols file')


if path:
    names = {}
    for line in open(path):
        addr, name = line.strip().split(maxsplit=1)
        if name in names:
            names[name] += 1
            name = name + str(names[name])
        else:
            names[name] = -1
        addr = int(addr, 16)
        idc.create_insn(addr)
        idc.set_name(addr, name.replace(' ', '_').replace('/', ':'), idc.SN_CHECK)
