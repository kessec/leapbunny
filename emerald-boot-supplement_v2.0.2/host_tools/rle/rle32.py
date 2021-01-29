#!/usr/bin/python
import sys
import struct

u32 = struct.Struct('<I')

def main():
    infile = open(sys.argv[1], 'rb')
    outfile = open(sys.argv[1] + '.rle', 'wb')

    times = -1
    lastdata = None
    while True:
        chunk = infile.read(4)
        times += 1
        if not chunk:
            break

        data = u32.unpack(chunk)[0] & 0xffffff

        if lastdata != None and lastdata != data:
            outfile.write(u32.pack(lastdata | (times << 24)))
            times = 0

        if times == 255:
            outfile.write(u32.pack(data | (times << 24)))
            times = 0

        lastdata = data

    if times:
        outfile.write(u32.pack(data | (times << 24)))

    outfile.write(u32.pack(0))
        
    infile.close()
    outfile.close()

if __name__ == '__main__':
    main()

