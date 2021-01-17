#!/usr/bin/python

import sys
import array

fin=sys.argv[1]
fout=fin + ".h"
hin=open(fin)
hout=open(fout, "w")
raw=hin.read()
hin.close()
data=array.array ('L', raw)
j=0
for i in data:
    hout.write("0x%08x," % i)
    if j==7:
        hout.write("\n")
        j=0
    else:
        j=j+1
hout.write ("\n")
hout.close
