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

if sys.platform == 'win32':
    import msvcrt
    from time import sleep
else:
    from select import select
    import termios
    import tty


def stdin_gen(timeout=0.05):
    """Generator returns keystrokes from stdin"""
    if sys.platform == 'win32':
        do_timeout = True
        while msvcrt.kbhit():
            do_timeout = False
            c = msvcrt.getch()
            yield c
        if do_timeout:
            sleep(timeout)
    else:
        while select([sys.stdin.fileno()],[],[],timeout)[0]:
            c = sys.stdin.read(1)
            yield c


class raw_tty:
    """Decorator for wrapping functions needing raw tty"""
    def __init__(self, f):  
        self.f = f

    def __call__(self, *args):
        ret = None
        if sys.platform != 'win32':
            try:
                self.saved = termios.tcgetattr(sys.stdin)
                tty.setraw(sys.stdin.fileno())
                ret = self.f(*args)
            finally:
                termios.tcsetattr(sys.stdin, termios.TCSADRAIN,
                                  self.saved)
        else:
            ret = self.f(*args)
        return ret


class cbreak_tty:
    """Decorator for wrapping functions needing cbreak tty"""
    def __init__(self, f):  
        self.f = f

    def __call__(self, *args):
        ret = None
        if sys.platform != 'win32':
            try:
                self.saved = termios.tcgetattr(sys.stdin)
                tty.setcbreak(sys.stdin.fileno())
                ret = self.f(*args)
            finally:
                termios.tcsetattr(sys.stdin, termios.TCSADRAIN,
                                  self.saved)
        else:
            ret = self.f(*args)
        return ret


@raw_tty
def terminal(sp):
    """Interactive serial terminal.

    ^C two times in a row stops the session.
    """

    break_count = 0
    while break_count < 2:
        serial_input = sp.read(sp.inWaiting())
        if serial_input:
            sys.stdout.write(serial_input)
            sys.stdout.flush()

        user_input = ''
        for c in stdin_gen():
            if c == '\x03':
                break_count += 1
                if break_count >= 2:
                    break   
            else:
                break_count = 0
            user_input += c
        if user_input:
            sp.write(user_input)
            sp.flush()

    sys.stdout.write('\n\r')


if __name__ == '__main__':
    from serial import Serial
    sp = Serial('/dev/ttyUSB0', 115200, timeout=0.25)
    terminal(sp)

