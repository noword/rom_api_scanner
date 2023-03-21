import pickle
import gzip
from struct import unpack, pack

LIBS_NAME = 'libs.pickle.gz'


class NdsApiScanner:
    def __init__(self, lib_name=LIBS_NAME):
        self.libs = pickle.load(gzip.open(lib_name, 'rb'))

    def scan(self, bs_or_name, voffset=0):
        if isinstance(bs_or_name, str):
            bs_or_name = open(bs_or_name, 'rb').read()

        buf = self._pre_process(bs_or_name)

        results = []
        for b, name in self.libs.items():
            offset = buf.find(b)
            if offset > 0:
                results.append((offset + voffset, name))

        results.sort(key=lambda x: x[0])

        return results

    def _pre_process(self, buf):
        # clean all address for bl(c) instruction
        data = list(unpack(f'{len(buf)//4}I', buf))
        for i, d in enumerate(data):
            if d & 0x0f000000 == 0x0b000000:
                data[i] &= 0xff000000
        return pack(f'{len(data)}I', *data)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('name', action='store', nargs=1)
    parser.add_argument('voffset', action='store', nargs='?', type=lambda x: int(x, 0), default=0x02000000)
    args = parser.parse_args()

    scanner = NdsApiScanner()
    results = scanner.scan(args.name[0], args.voffset)

    for offset, name in results:
        print(f'{offset:08x} {name}')
