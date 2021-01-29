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

import micromon
from micromon.config import Config
from micromon.log import Log
import sys
import os
from binascii import crc32
from time import sleep

_loader_signature    = '\xE6Mon'

LOADER_NOP          = 0
LOADER_GO_MAIN      = 1
LOADER_SET_BAUDRATE = 2
LOADER_LOAD_REST    = 3

class Loader:
    def __init__(self, target):
        self._target = target

        powerup_delay = Config.get('target.powerup_delay')

        try:
            core = self._read_core()
            while True:
                self._target.set_baudrate(19200)

                self._target.set_uart_boot(True)
                if self._target.get_power_state():
                    self._target.reset()
                else:
                    print 'Please power on the target'
                    while not self._target.get_power_state():
                        sleep(0.1)
                    self._target.purge_input()
                    sleep(powerup_delay)
                self._target.set_uart_boot(False)

                if self._init_core(core):
                    break
            self._check_crc(core)
        except Exception as inst:
            Log.fatal(str(inst)).single()
            sys.exit(1)

    def _read_core(self):
        fp = file(micromon.CORE_BIN, 'rb')
        fp.seek(0, os.SEEK_END)
        size = fp.tell()
        if size > micromon.CORE_SIZE:
            raise Exception, 'Core binary is too big'
        fp.seek(0, os.SEEK_SET)

        core = fp.read(size)
        core += '\x00' * (micromon.CORE_SIZE - size)
        return core

    def _init_core(self, core):
        uart_boot_size = Config.get('monitor.uart_boot_size')

        if uart_boot_size in ['detect', '512']:
            if not self._write(core[:512]):
                return False
            if self._loader_signature():
                if not self._loader_512(core):
                    return False
            elif uart_boot_size == 'detect':
                if not self._write(core[512:]):
                    return False
                if self._loader_signature():
                    self._loader_16k()
                else:
                    raise Exception, 'Detect boot failed'
            else:
                raise Exception, '512 boot failed'

        elif uart_boot_size == '16k':
            if not self._write(core):
                return False
            if self._loader_signature():
                self._loader_16k()
            else:
                raise Exception, '16k boot failed'

        return True

    def _loader_signature(self):
        response = self._target.read(4)
        if not response:
            return False
        
        if response == _loader_signature:
            return True
        else:
            raise Exception, 'Communication error occured'

    def _loader_512(self, core):
        self._set_baudrate()
        if not self._load_rest(core[512:]):
            return False
        self._go_main()
        return True

    def _loader_16k(self):
        self._set_baudrate()
        self._go_main()
        return True

    def _set_baudrate(self):
        baudrate = Config.get('monitor.baudrate')

        with Log.debug('Setting target baudrate to %(baudrate)d',
                       baudrate=baudrate):
            self._target.write_u8(LOADER_SET_BAUDRATE)
            self._target.write_u32(baudrate)
            response = self._target.read_u8()
            if response == None:
                raise Exception, 'Target did not respond'
            elif response:
                raise Exception, 'Target error'

            self._target.set_baudrate(baudrate)
            self._target.write_u16(0xAA55)

            response = self._target.read_u8()
            if response == None:
                raise Exception, 'Target did not respond'
            elif response:
                raise Exception, 'Target error'

    def _load_rest(self, rest):
        if len(rest) != micromon.CORE_SIZE - 512:
            raise Exception, 'Incorrect data length'

        self._target.write_u8(LOADER_LOAD_REST)

        if not self._write(rest):
            return False

        response = self._target.read_u8()
        if response == None:
            raise Exception, 'Target did not respond'
        elif response:
            raise Exception, 'Target error'

        return True

    def _go_main(self):
        self._target.write_u8(LOADER_GO_MAIN)

    def _check_crc(self, core):
        remote_crc = self._target.read_u32()
        if remote_crc == None:
            raise Exception, 'Target did not respond'
        Log.debug('Remote CRC32: 0x%(remote_crc)08X',
                  remote_crc=remote_crc).single()
        local_crc = crc32(core) & 0xFFFFFFFF
        Log.debug('Local CRC32: 0x%(local_crc)08X',
                  local_crc=local_crc).single()
        if remote_crc != local_crc:
            raise Exception, 'CRC32 check failed'

    def _write(self, data):
        size = len(data)
        blocksize = 512
        blocks = (size + blocksize-1) / blocksize
        block = 0

        log_entry = Log.info('Sending block %(block)d of %(blocks)d block(s)',
                             block=block, blocks=blocks)
        with log_entry:
            while block < blocks:
                log_entry.update(block=block+1)
                if not self._target.get_power_state():
                    with Log.warning('Power lost'):
                        return False
                offset = block * blocksize
                self._target.write(data[offset:offset+blocksize])
                block += 1

        return True

