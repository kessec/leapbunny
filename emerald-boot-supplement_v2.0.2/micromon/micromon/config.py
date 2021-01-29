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
import ConfigParser

class ConfigSingleton:
    options = {
        'target.serial_port':
            {'type': str},
        'target.data_timeout':
            {'type': float, 'default': 1.0},
        'target.uart_boot_pin':
            {'type': str, 'default': 'none',
             'values': ['none', 'rts', '!rts', 'dtr', '!dtr']},
        'target.reset_pin':
            {'type': str, 'default': 'none',
             'values': ['none', 'rts', '!rts', 'dtr', '!dtr']},
        'target.reset_delay':
            {'type': float, 'default': 0.5},
        'target.power_detect_pin':
            {'type': str, 'default': 'none',
             'values': ['none', 'cts', '!cts', 'dsr', '!dsr', 'cd', '!cd',
                        'ri', '!ri']},
        'target.powerup_delay':
            {'type': float, 'default': 1.0},
        'monitor.uart_boot_size':
            {'type': str, 'default': 'detect',
             'values': ['detect', '512', '16k']},
        'monitor.baudrate':
            {'type': int, 'default': 115200, 
             'values': [19200, 38400, 57600, 115200, 230400, 460800, 614400, 
                        921600, 1500000]},
    }

    def __init__(self):
        self.config = ConfigParser.SafeConfigParser()
        self.config.readfp(open(micromon.CONFIG_PATH))

    def load(self, filename):
        self.config.readfp(open(filename), filename)
        
    def get(self, name):
        if name not in self.options:
            raise ValueError, 'unknown setting: %s' % name

        option = self.options[name]
        l = name.split('.', 1)

        l[1] = l[1].replace('_', ' ')

        value = None
        try:
            if option['type'] == str:
                value = self.config.get(l[0], l[1])
            elif option['type'] == int:
                value = self.config.getint(l[0], l[1])
            elif option['type'] == float:
                value = self.config.getfloat(l[0], l[1])
            elif option['type'] == bool:
                value = self.config.getboolean(l[0], l[1])
        except:
            if option.has_key('default'):
                return option['default']
            raise ValueError, 'mandatory setting \'%s\' is not set' % name

        if option.has_key('values'):
            if value not in option['values']:
                raise ValueError, 'invalid setting for \'%s\'' % name

        return value

Config = ConfigSingleton()

