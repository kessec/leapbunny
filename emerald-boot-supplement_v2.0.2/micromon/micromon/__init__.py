from os import path
import sys

MICROMON_ROOT = path.normpath(path.join(path.dirname(path.abspath(__file__)), '..'))
CONFIG_PATH = path.normpath(path.join(MICROMON_ROOT, 'micromon.cfg'))
CORE_BIN = path.normpath(path.join(MICROMON_ROOT, 'core', 'core.bin'))
CORE_SIZE = (1 << 14) # 16k

# need to iterate over path and see if we're in there currently
sys.path.append(MICROMON_ROOT)

from core import Core
from target import Target
__all__ = ['Target', 'Core']

