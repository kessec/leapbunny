#!/usr/bin/python
# vim: set ts=4 sw=4 expandtab
#
#  Copyright (C) 2011 Jeff Kent <jakent@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

import sys
from micromon import *
from micromon.util import terminal

# TODO: Integrate into the Micromon shell

def main():
    filename = sys.argv[1]

    exec_baud = None
    if len(sys.argv) > 2:
        exec_baud = int(sys.argv[2])

    load_address = 0
    if len(sys.argv) > 3:
        load_address = int(sys.argv[3], 16)

    exec_address = load_address
    if len(sys.argv) > 4:
        exec_address = int(sys.argv[4], 16)

    target = Target()
    core = Core(target)

    fp = open(filename, 'rb')
    data = fp.read()
    fp.close()

    assert core.mem_write(load_address, data)
    if exec_baud:
        assert core.set_baudrate(exec_baud)
    core.run(exec_address)

    terminal(target.sp)

if __name__ == '__main__':
    main()
