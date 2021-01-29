CROSS_COMPILE ?= arm-linux-uclibcgnueabi-
OPT=-Os
CFLAGS = -ggdb -std=gnu99 -mcpu=arm926ej-s -mlittle-endian -msoft-float -ffreestanding $(OPT) -Wall -Werror -pedantic -Wno-variadic-macros -D_LF1000_BOOTLOADER -DSELF_BOOTSTRAP -fPIC -fno-builtin -I./include
LDFLAGS = -static --architecture armv5te -EL -M -Map boot.map -e StartUp -T emerald-boot.lds

# nand_utils.o and nand_boot.o must be in the first 2K.
# params.o must be in the first 16K, but not in the first 2K!!.  See source
OBJS = startup.o copyNand.o nand_boot.o nand_utils.o nand.o nand_ecc.o \
	clock.o gpio.o string.o \
	debug.o params.o global.o \
	timer.o cbf.o adc.o ram.o \
	bootUsb.o usbDescriptors-common.o lib1funcs.o \
	buttons.o \
	spi.o displays/hx8238.o displays/nt39016.o displays/ili9322.o displays/ili6480g2.o \
	i2c.o \
	lfp100.o \
	mmc.o disk.o \
	graphics/display.o graphics/dpc.o pwm.o

SCREENS_PATH = "../packages"
GENERIC_SCREENS=ATTENTION_NEEDED.32.rle.h DOWNLOAD_IN_PROGRESS.32.rle.h \
	LOW_BATTERY.32.rle.h VISIT.32.rle.h VISIT_FR.32.rle.h  VISIT_ES.32.rle.h
LEX_SCREENS=$(GENERIC_SCREENS:%=Lex-%)
LPAD_SCREENS=$(GENERIC_SCREENS:%=Lpad-%)

# HNS_SIZE = `cat $(SCREENS_PATH)/screens/FIRST.32.rle | wc -c` 
HNS_SIZE = `cat $(SCREENS_PATH)/screens/Madrid-Boot-logo.32.rle | wc -c` 

LEX_TARGET = emerald-boot
LPAD_TARGET = madrid-boot

LEX_OBJS = $(OBJS) usbDescriptors-emerald.o lex_screens.o rle.o mlc.o
LPAD_OBJS = $(OBJS) usbDescriptors-madrid.o lpad_screens.o rle.o mlc.o

LEX_RES = -DFB_VIRTUAL_XRES=320 -DFB_VIRTUAL_YRES=240
LPAD_RES = -DFB_VIRTUAL_XRES=480 -DFB_VIRTUAL_YRES=272


## Enable debugging for now...
# CFLAGS += -DMMC_DEBUG -DDEBUG

# debug printing support
ifneq ($(DEBUG),)
CFLAGS += -DDEBUG
endif

# debug stopwatch support
ifneq ($(DEBUG_STOPWATCH),)
CFLAGS += -DDEBUG_STOPWATCH
endif

# RAMDISK bootloader support (enumerate as MSC device with a ramdisk)
# (otherwise enumerate as a MSC device w/o media)
ifeq ($(RAMDISK),1)
CFLAGS += -DRAMDISK
OBJS += ramdisk.o
endif

CFLAGS += -DMMC_SUPPORT -DMMC_CHANNEL=0

ifeq ($(SDCARD_DEBUG),1)
CFLAGS += -DMMC_DEBUG
endif

# Bits per pixel (BBP) setting
CFLAGS += -DBPP=4

# Force support for integer division for nand_ecc
CFLAGS += -DCONFIG_AEABI

# FIXME
HT=../host_tools/rle/

all: $(LPAD_TARGET).bin $(LEX_TARGET).bin
	echo "Done!"

$(LPAD_TARGET).bin: $(OBJS) lpad_screens.o usbDescriptors-madrid.o
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -DHNS_SIZE=$(HNS_SIZE) $(LPAD_RES) main.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $(LPAD_RES) rle.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c global.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $(LPAD_RES) graphics/mlc.c -o mlc.o
	$(CROSS_COMPILE)ld $(LDFLAGS) $(LPAD_OBJS) main.o -o $(LPAD_TARGET)
	mv boot.map $(LPAD_TARGET).map
	$(CROSS_COMPILE)objcopy -S -I elf32-littlearm -O binary $(LPAD_TARGET) $(LPAD_TARGET).bin
	chmod 644 $(LPAD_TARGET).bin
	$(CROSS_COMPILE)objdump -D -b binary -EL -m armv5te $(LPAD_TARGET).bin > $(LPAD_TARGET).dis
	du --apparent-size -b $(LPAD_TARGET).bin

$(LEX_TARGET).bin: $(OBJS) lex_screens.o usbDescriptors-emerald.o
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -DHNS_SIZE=$(HNS_SIZE) $(LEX_RES) main.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $(LEX_RES) rle.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c global.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $(LEX_RES) graphics/mlc.c -o mlc.o
	$(CROSS_COMPILE)ld $(LDFLAGS) $(LEX_OBJS) main.o -o $(LEX_TARGET)
	mv boot.map $(LEX_TARGET).map
	$(CROSS_COMPILE)objcopy -S -I elf32-littlearm -O binary $(LEX_TARGET) $(LEX_TARGET).bin
	chmod 644 $(LEX_TARGET).bin
	$(CROSS_COMPILE)objdump -D -b binary -EL -m armv5te $(LEX_TARGET).bin > $(LEX_TARGET).dis
	du --apparent-size -b $(LEX_TARGET).bin

startup.o: startup.S
	$(CROSS_COMPILE)gcc $(CFLAGS) -Wa,--defsym,StartUp=0x0000 -D__ASSEMBLY__ -c startup.S

lib1funcs.o: lib1funcs.S
	$(CROSS_COMPILE)gcc $(CFLAGS) -D__ASSEMBLY__ -c lib1funcs.S

lpad_screens.o: $(LPAD_SCREENS) lpad_screens.c

lex_screens.o: $(LEX_SCREENS) lex_screens.c

displays/%.o:
	$(CROSS_COMPILE)gcc $(CFLAGS) -c displays/$*.c -o $@

graphics/%.o:
	$(CROSS_COMPILE)gcc $(CFLAGS) -c graphics/$*.c -o $@

%.o:
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $*.c

%.png:
	./link_pngs.py	$(SCREENS_PATH)

%.32.rle.h:	%.png
	$(HT)pngto32bpp $*.png $*.32
	$(HT)rle32.py $*.32
	./dump32.py $*.32.rle
	rm $*.32 $*.32.rle

install: all
	cp -v *.bin /tftpboot

clean:
	rm -f *.o displays/*.o graphics/*.o *.rle.h *.bin *.dis *.map \
	$(LEX_TARGET) $(LPAD_TARGET)
