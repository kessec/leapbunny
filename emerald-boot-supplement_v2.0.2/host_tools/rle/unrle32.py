#!/usr/bin/python
import sys
import struct
import os.path

u32 = struct.Struct('<I')

def main():
    infile = open(sys.argv[1], 'rb')
    outfile = open(os.path.splitext(sys.argv[1])[0], 'wb')

    while True:
        chunk = infile.read(4)
        if not chunk:
            break
        data = u32.unpack(chunk)[0]
        times = data >> 24
        chunk = u32.pack(data | (0xff << 24))
        for i in xrange(times):
            outfile.write(chunk)

    infile.close()
    outfile.close()

if __name__ == '__main__':
    main()

