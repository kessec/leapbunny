# vim: set ts=4 sw=4 noexpandtab
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

CC			= $(CROSS_COMPILE)gcc
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
SIZE		= $(CROSS_COMPILE)size

CFLAGS		= -std=gnu99 -mcpu=arm926ej-s -mlittle-endian -msoft-float \
			  -ffreestanding -Os -Wall -pedantic -nostartfiles -nodefaultlibs
LDFLAGS		= -static -lgcc

INCLUDE		= -I./include/ -I../include/
CFLAGS		+= -D_LF1000_BOOTLOADER -DCONFIG_MACH_LF_LF1000 \
			   -DCONFIG_CPU_SPEED_393216000 -DCONFIG_RAM_18MB

TARGET = core
OUTPUT = $(TARGET).bin
SFILES = $(filter-out $(wildcard *.lds.S),$(wildcard *.S))
ARM_CFILES = $(wildcard *.arm.c)
THUMB_CFILES = $(filter-out $(ARM_CFILES),$(wildcard *.c))
OFILES = $(SFILES:.S=.o) $(THUMB_CFILES:.c=.o) $(ARM_CFILES:.c=.o)

.SECONDARY:
.PHONY: all
all: $(OUTPUT)

-include $(wildcard *.d)

%.lds: %.lds.S
	$(CC) -E -P -x c $(INCLUDE) $*.lds.S -o $*.lds
	$(CC) -MM $(INCLUDE) $*.lds.S > $*.lds.d
	@mv -f $*.lds.d $*.lds.d.tmp
	@sed -e 's/\.o:/:/' < $*.lds.d.tmp > $*.lds.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.lds.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.lds.d
	@rm -f $*.lds.d.tmp

%.o: %.S
	$(CC) -c $(CFLAGS) $(INCLUDE) $*.S -o $*.o
	$(CC) -MM $(CFLAGS) $(INCLUDE) $*.S > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

%.arm.o: %.arm.c
	$(CC) -c $(CFLAGS) -marm $(INCLUDE) $*.arm.c -o $*.arm.o
	$(CC) -MM $(CFLAGS) -marm $(INCLUDE) $*.arm.c > $*.arm.d
	@cp -f $*.arm.d $*.arm.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.arm.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.arm.d
	@rm -f $*.arm.d.tmp

%.o: %.c
	$(CC) -c $(CFLAGS) -mthumb $(INCLUDE) $*.c -o $*.o
	$(CC) -MM $(CFLAGS) -mthumb $(INCLUDE) $*.c > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

%.bin: %.elf
	$(OBJDUMP) -d -m armv5te $< > $*.dis
	$(OBJCOPY) -S -I elf32-littlearm -O binary $< $@
	@echo
	@$(SIZE) --target=binary $@

%.elf: $(TARGET).lds $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-M,-Map,$(TARGET).map -T $^ -o $@ -lgcc

.PHONY: distclean
distclean:
	rm -f *.o *.d *.dis *.map *.lds

.PHONY: clean
clean: distclean
	rm -f *.bin *.elf

