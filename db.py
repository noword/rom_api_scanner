from struct import unpack, pack


class DB(list):
    def __read_str(self, io, encoding='ascii'):
        size, = unpack('I', io.read(4))
        return io.read(size).decode(encoding).strip('\x00')

    def __write_str(self, io, s, encoding='ascii'):
        s = (s + '\x00').encode(encoding)
        io.write(pack('I', len(s)))
        io.write(s)

    def load(self, io):
        num, = unpack('I', io.read(4))
        for _ in range(num):
            name = self.__read_str(io)
            pattern = self.__read_str(io)
            self.append((name, pattern))

    def save(self, io):
        io.write(pack('I', len(self)))
        for name, pattern in self:
            self.__write_str(io, name)
            self.__write_str(io, pattern)


if __name__ == '__main__':
    import sys
    db = DB()
    db.load(open(sys.argv[1], 'rb'))
    for name, pattern in db:
        print(name, pattern)
