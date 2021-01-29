#!/usr/bin/env python

import os
import sys
import array
import struct
import getopt

CBF_MAGIC = -0x65432110
CBF_VERSION = 1
NAME_LEN = 64
BLOCK_SIZE = 0x20000
KERNEL_JUMP = 0x100000

try:
	project = os.environ['PROJECT_PATH']
except:
	print "Error: PROJECT_PATH not set.  Please set it."
	sys.exit(1)

try:
	kerneldir = os.environ['KERNELDIR']
except:
	kerneldir = project+'/linux-2.6'

KERNEL_PATH = kerneldir+'/arch/arm/boot/zImage'
KERNEL_LOAD = KERNEL_JUMP
IMAGE="kernel.cbf"

buffer = ""
summary = ""

def pack_file(path):
	global summary
	global buffer

	name = path[path.rfind('/')+1:]
	if len(name) >= NAME_LEN:
		print "error: \"%s\", name is too long" % name
		return False
	if not os.path.exists(path):
		print "error: %s not found" % path
		return False
	h = open(path)
	data = h.read()
	h.close()
	size = len(data)
	# rem = BLOCK_SIZE - (size % BLOCK_SIZE)
	buffer += data # + '\xFF'*rem
	return size

def crc(buf):
	a = array.array ('i', buf)
	crc = 0;
	for c in a:
		crc = 1 + (crc ^ c)
		# if len(buf) < 100:
		#	print "ordc=%d crc=%d\n" % (ord(c), crc)
	return crc

if __name__ == '__main__':
	try:
		opts = getopt.getopt(sys.argv[1:], "cuewbo:")
	except getopt.GetoptError:
		pass

	add_uboot = False
	for o,a in opts[0]:
		if o == '-o':
			IMAGE=a
		if o == '-c':
			try:
				os.unlink(IMAGE)
			except OSError:
				pass
		if o == '-u':
			add_uboot = True

	# pack files
	size = pack_file(KERNEL_PATH);
	if not size:
		sys.exit(1)
	# MAGIC, VERSION, load, jump, len, crc, <data>, crc
	summary = struct.pack('iiiii', CBF_MAGIC, CBF_VERSION, 
			      KERNEL_LOAD, KERNEL_JUMP, size)
	# Tack on crcs
	summary_crc = crc(summary)
	summary += struct.pack('i', summary_crc)
	buffer_crc = crc(buffer);
	buffer += struct.pack('i', buffer_crc)
	print "summary crc=%08x buffer crc=%08x buffer len=%08x" \
	    % (summary_crc, buffer_crc, size)

	# pad out to erase block size
	rem = BLOCK_SIZE - ((len(summary) + len(buffer)) % BLOCK_SIZE)
	pad = '\xFF' * rem

	# write out
	h = open(IMAGE, "w")
	h.write(summary)
	h.write(buffer)
	if rem > 0:
		h.write(pad)
	h.close()
