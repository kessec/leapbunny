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

levels = ('FATAL', 'ERROR', 'WARNING', 'INFO', 'DEBUG')

class BaseLogger:
    def __init__(self, max_level):
        self._max_level = levels.index(max_level)
        self._entry = {}

    def begin_entry(self, n, level, format, **values):
        if self._max_level >= level:
            entry = {'level': level, 'format': format, 'values': values}
            self._entry[n] = entry
            return entry

    def update_entry(self, n, format, **values):
        if self._entry.has_key(n):
            entry = self._entry[n]
            entry['format'] = format
            entry['values'] = values
            return entry
    
    def end_entry(self, n):
        if self._entry.has_key(n):
            del self._entry[n]
            return True


class StdoutLogger(BaseLogger):
    def begin_entry(self, n, level, format, **values):
        entry = BaseLogger.begin_entry(self, n, level, format, **values)
        if entry:
            for key in self._entry.keys():
                if key != n:
                    self.end_entry(key)
            sys.stdout.write(entry['format'] % entry['values'])
            sys.stdout.flush()

    def update_entry(self, n, format, **values):
        entry = BaseLogger.update_entry(self, n, format, **values)
        if entry:
            sys.stdout.write('\r' + entry['format'] % entry['values'])
            sys.stdout.flush()
        
    def end_entry(self, n):
        if BaseLogger.end_entry(self, n):
            sys.stdout.write('\n')


def _make_context_manager(level_name):
    level = levels.index(level_name)
    class context_manager:
        def __init__(self, format, **values):
            Log.last_n += 1
            self._n = Log.last_n
            self._format = format
            self._values = values
            for logger in Log.loggers:
                logger.begin_entry(self._n, level, self._format, **self._values)
        def __enter__(self):
            return self
        def __exit__(self, type, value, tb):
            for logger in Log.loggers:
                logger.end_entry(self._n)
        def update(self, format=None, **values):
            if format:
                self._format = format
            self._values.update(values)
            for logger in Log.loggers:
                logger.update_entry(self._n, self._format, **self._values)
        def single(self):
            for logger in Log.loggers:
                logger.end_entry(self._n)
    return context_manager

class Log:
    loggers = [StdoutLogger('DEBUG')]
    last_n = 0

    fatal = _make_context_manager('FATAL')
    error = _make_context_manager('ERROR')
    warning = _make_context_manager('WARNING')
    info = _make_context_manager('INFO')
    debug = _make_context_manager('DEBUG')

