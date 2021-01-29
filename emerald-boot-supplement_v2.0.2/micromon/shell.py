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

from cmd import Cmd
import sys
from time import sleep
from micromon import *
from micromon.registers import REGS, Registers
from micromon.util import stdin_gen, cbreak_tty

def int2bin(n, count=32):
    """returns the binary of integer n, using count number of digits"""
    return ''.join([str((n >> y) & 1) for y in range(count-1, -1, -1)])


class CommandParser(Cmd):
    def __init__(self):
        Cmd.__init__(self)
        self.prompt = '> '
        self.target = None
    
    def preloop(self):
        self.target = Target()
        self.core = Core(self.target)
        self.regs = Registers(self.core)
        self.do_power('on')

    def emptyline(self):
        pass

    def do_regread(self, s):
        """regread [reg]

        Read the value of a register.
        """
        l = s.split()
        if len(l) != 1:
            print '*** Invalid number of arguments'
            return
        
        reg = l[0]
        try:
            r = Registers.lookup(reg)
        except:
            print '*** Register is unknown'
            return

        value = self.regs.read(r)
        if value is None:    
            print '*** Error reading'
            return

        value_bin = int2bin(value, r['bits'])
        value_bin = ' '.join([value_bin[x:x+4] for x in range(0, 32, 4)])
        print '0x%08X      %s'  % (value, value_bin)

    def do_regwrite(self, s):
        """regwrite [reg] [value]

        Write a hex value to a register.
        """
        l = s.split()
        if len(l) != 2:
            print '*** Invalid number of arguments'
            return
    
        reg = l[0]
        try:
            r = Registers.lookup(reg)
        except:
            print '*** Register is unknown'
            return
        
        s = l[1].lower()
        value = None
        try:        
            if s.startswith('0b'):
                value = int(s, 2)
            elif s.startswith('0x'):
                value = int(s, 16)
            else:
                value = int(s)
        except:
            print '*** Invalid value'
            return

        self.regs.write(r, value)

    def do_reglist(self, s):
        """reglist [[group]]

        List registers grouped by function.
        """
        group = None
        l = s.split()
        if len(l) == 1:
            group = l[0].upper()
        elif len(l) > 2:
            print '*** Invalid number of arugments'
            return

        groups = []
        regs = {}

        for r in REGS:
            r_group = r['group'].upper()
            if r_group.upper() not in groups:
                groups.append(r_group)
                regs[r_group] = []
            regs[r_group].append(r)

        if group is None:
            print ''
            self.print_topics('Register groups', groups, 15, 80)
        else:
            if group in groups:
                print ''
                for r in regs[group]:
                    print '0x%08X      %-24s' % (r['addr'], r['name'])
                print ''
            else:
                print '*** Unknown register group'
                return

    # TODO:
    #def do_reginfo(self, s):
    #    """reginfo [regname]
    #
    #    Provide detailed information on a register.
    #    """

    def do_gpioscan(self, s):
        """gpioscan [A|B|C] [[mask]]

        Helps identify GPIOs.  Use with great trepedation."""

        l = s.split()
        if len(l) > 2:
            print '*** Invalid number of arguments'
            return

        port = l[0].upper()
        if port not in ['A', 'B', 'C']:
            print '*** Unknown GPIO port'
            return

        mask = 0xFFFFFFFF
        if len(l) >= 2:
            try:
                mask = int(l[1],16)
                assert mask >= 0 and mask <= 0xFFFFFFFF
            except:
                print '*** Bad mask'
                return

        first = 0
        last = 31
        split = 16

        if port == 'A':
            self.regs.write('GPIO%sALTFN0' % port, 0x00010000)
        else:
            self.regs.write('GPIO%sALTFN0' % port, 0x00000000)
        self.regs.write('GPIO%sALTFN1' % port, 0x00000000)
        self.regs.write('GPIO%sOUTENB' % port, 0xFFFFFFFF & mask)
        
        @cbreak_tty
        def toggle():
            bits = 0
            for i in xrange(first, last+1):
                bits |= 1 << i

            state = False
            while True:
                try:
                    return stdin_gen(0.1).next().lower()
                except StopIteration:
                    if state:
                        self.regs.write('GPIO%sOUT' % port, 0xFFFFFFFF)
                    else:
                        self.regs.write('GPIO%sOUT' % port, ~bits & 0xFFFFFFFF)
                    state = not state

        print 'Find a pin that is toggling, and press any key to continue.'
        toggle()
        while split >= 1:
            first += split
            print 'Is it toggling now? [y/n]'
            key = None
            while key not in ['y', 'n']:
                key = toggle()
            if key == 'n':
                first -= split
                last -= split
            split /= 2
        print 'You found GPIO%s%d' % (port, first)

        self.regs.write('GPIO%sOUTENB' % port, 0x00000000)

    def do_memtoggle(self, s):
        """memtoggle [addr] [bits] [mask]"""
        l = s.split()
        if len(l) != 3:
            print '*** Invalid number of arguments'
            return

        addr = None
        bits = None
        value = None
        try:
            addr = int(l[0],16)
            bits = int(l[1])
            mask = int(l[2],16)
            assert bits in [8, 16, 32]
        except:
            print '*** Bad values'
            return

        if bits == 8:
            read = lambda addr: self.core.read_u8(addr)
            write = lambda addr, value: self.core.write_u8(addr, value)
        elif bits == 16:
            read = lambda addr: self.core.read_u16(addr)
            write = lambda addr, value: self.core.write_u16(addr, value)
        elif bits == 32:
            read = lambda addr: self.core.read_u32(addr)
            write = lambda addr, value: self.core.write_u32(addr, value)

        orig = read(addr)
        value = orig

        print 'Press any key to stop...'
        while not getkey():
            value ^= mask
            write(addr, value)
            sleep(0.05)

        write(addr, orig)

    def do_adc(self, s):
        """adc [channel]"""

        l = s.split()
        if len(l) != 1:
            print '*** Invalid number of arguments'
            return

        try:
            channel = int(l[0])
            assert channel in xrange(0,8)
        except:
            print '*** Invalid channel'
            return

        def init():
            self.regs.write('ADCCLKENB', 1 << 3)
            self.regs.write('ADCCON', 0)
            adccon = 126 << 6
            self.regs.write('ADCCON', adccon)
            adccon |= 1 << 14
            self.regs.write('ADCCON', adccon)

        def read(channel):
            adccon = self.regs.read('ADCCON') & ~(0x7 << 3)
            adccon |= (1 << 0) | (channel << 3)
            self.regs.write('ADCCON', adccon)

            while (self.regs.read('ADCCON') & 0x1):
                pass

            value = self.regs.read('ADCDAT') & 0x3FF
            return value

        def shutdown():
            self.regs.write('ADCCON', 0)
            self.regs.write('ADCCLKENB', 0)

        init()
        value = read(channel)
        print value
        shutdown()

    def do_power(self, s):
        """power [on|off]

        Changes the ALIVEGPIO VDDPWRON bit.
        """
        if s.lower() in ['1', 'on']:
            self.regs.write('ALIVEPWRGATEREG', 1)
            self.regs.write('ALIVEGPIORSTREG', 0)
            self.regs.write('ALIVEGPIOSETREG', 0x80)
            self.regs.write('ALIVEGPIOSETREG', 0)
            self.regs.write('ALIVEPWRGATEREG', 0)
        elif s.lower() in ['0', 'off']:
            self.regs.write('ALIVEPWRGATEREG', 1)
            self.regs.write('ALIVEGPIOSETREG', 0)
            self.regs.write('ALIVEGPIORSTREG', 0x80)
            self.regs.write('ALIVEGPIORSTREG', 0)
            self.regs.write('ALIVEPWRGATEREG', 0)
            return True
        else:
            print self.do_power.__doc__

    def do_quit(self, s):
        """quit

        Exit the program.
        """
        return True
    do_q = do_quit
    
if __name__ == '__main__':
    print '\nMicromon Shell 0.1\n'

    cp = CommandParser()
    cp.cmdloop('Type \'help\' for assistance.')

