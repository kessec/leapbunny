#-------------------------------------------------------------------------------
# NOTE: emerald-boot is built with support for usb-boot.
#       By default it is not built with support for a ramdisk.  If you want
#       to support a ramdisk, specify RAMDISK=1 on the ./install.sh command
#       line.
#-------------------------------------------------------------------------------
CROSS_COMPILE ?= arm-linux-uclibcgnueabi-
OPT=-Os
CFLAGS = -ggdb -std=gnu99 -mcpu=arm926ej-s -mlittle-endian -msoft-float -ffreestanding $(OPT) -Wall -Werror -pedantic -D_LF1000_BOOTLOADER -DSELF_BOOTSTRAP -fPIC -fno-builtin
LDFLAGS = -static --architecture armv5te -EL -M -Map emerald-boot.map -e StartUp -T emerald-boot.lds

ifneq ($(KERNELDIR),)
CFLAGS += -I$(KERNELDIR)/arch/arm/mach-lf1000/include -I$(KERNELDIR)/include/linux
endif

# build NFS booting support for the development boards (but not for the Form
# Factor / final boards as they don't have Ethernet)
ifneq ($(TARGET_MACH),LF_LF1000)
CFLAGS += -DNFS_SUPPORT
endif

# debug printing support
ifneq ($(DEBUG),)
CFLAGS += -DDEBUG
endif

# debug stopwatch support
ifneq ($(DEBUG_STOPWATCH),)
CFLAGS += -DDEBUG_STOPWATCH
endif

# boot button support
ifeq ($(BUTTON_BOOT),1)
CFLAGS += -DBUTTON_BOOT_SUPPORT
endif

# u-boot bootloader support
ifeq ($(UBOOT_SUPPORT),1)
CFLAGS += -DUBOOT_SUPPORT
endif

# nand_utils.o and nand_boot.o must be in the first 2K.
# params.o must be in the first 16K, but not in the first 2K!!.  See source
OBJS = startup.o copyNand.o nand_utils.o nand_boot.o nand.o nand_ecc.o \
       copyNor.o gpio.o string.o \
       debug.o params.o \
       timer.o cbf.o adc.o ram.o rle.o \
       bootUsb.o lib1funcs.o

SCREENS = ATTENTION_NEEDED.32.rle.h DOWNLOAD_IN_PROGRESS.32.rle.h LOW_BATTERY.32.rle.h VISIT.32.rle.h
HNS_SIZE = `cat ../packages/screens/HEALTH_AND_SAFETY.32.rle | wc -c` 

TARGET = emerald-boot

# RAMDISK bootloader support (enumerate as MSC device with a ramdisk)
# (otherwise enumerate as a MSC device w/o media)
ifeq ($(RAMDISK),1)
CFLAGS += -DRAMDISK
OBJS += ramdisk.o
endif

OBJS += screens.o

# button mapping support
OBJS += buttons.o

# LCD module support
OBJS += spi.o hx8238.o nt39016.o ili9322.o

ifeq ($(I2C_SUPPORT),1)
OBJS += i2c.o
endif

# MMC card support
MMC_OBJS=mmc.o diskio.o pff.o
ifeq ($(SDCARD),0)
OBJS += $(MMC_OBJS)
CFLAGS += -DMMC_SUPPORT -DMMC_CHANNEL=0
endif
ifeq ($(SDCARD),1)
OBJS += $(MMC_OBJS)
CFLAGS += -DMMC_SUPPORT -DMMC_CHANNEL=1
endif

ifeq ($(SDCARD_DEBUG),1)
CFLAGS += -DMMC_DEBUG
endif

# Bits per pixel (BBP) setting
CFLAGS += -DBPP=4

# Boot Splash Screen support
OBJS += display.o dpc.o mlc.o pwm.o

# Kernel bad-block test support
ifeq ($(TEST_BOOT_WITH_KERNEL_BAD_BLOCKS),1)
CFLAGS += -DTEST_BOOT_WITH_KERNEL_BAD_BLOCKS
OBJS += bbtest.o
endif

# Configure CPU and bus clocks
CFLAGS += -DCONFIGURE_CLOCKS

# Perform CRC32 checks on kernel image
#CFLAGS += -DCHECK_KERNEL_CRC

# Force support for integer division for nand_ecc
CFLAGS += -DCONFIG_AEABI

LINUX_FILES=include/autoconf.h include/mach-types.h include/dpc_hal.h \
	    include/mlc_hal.h include/pwm_hal.h

HT=../host_tools/rle

all: TAGS $(OBJS)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -DHNS_SIZE=$(HNS_SIZE) main.c
	$(CROSS_COMPILE)ld $(LDFLAGS) $(OBJS) main.o -o $(TARGET)
	$(CROSS_COMPILE)objcopy -S -I elf32-littlearm -O binary $(TARGET) $(TARGET).bin
	chmod 644 $(TARGET).bin
	$(CROSS_COMPILE)objdump -D -b binary -EL -m armv5te $(TARGET).bin > $(TARGET).dis
	du --apparent-size -b $(TARGET).bin

TAGS: *.c include/*.h
	etags *.c include/*.h

startup.o: startup.S
	$(CROSS_COMPILE)gcc $(CFLAGS) -Wa,--defsym,StartUp=0x0000 -D__ASSEMBLY__ -c startup.S

lib1funcs.o: lib1funcs.S
	$(CROSS_COMPILE)gcc $(CFLAGS) -D__ASSEMBLY__ -c lib1funcs.S

screens.o: $(SCREENS) screens.c

%.o:
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $*.c

%.32.rle.h:	%.png
	$(HT)/pngto32bpp $*.png $*.32
	$(HT)/rle32.py $*.32
	./dump32.py $*.32.rle
	rm $*.32 $*.32.rle

install: all
	cp $(TARGET).bin /tftpboot/

clean:
	rm -f *.o *.rle.h $(TARGET).bin $(TARGET).dis $(TARGET).map $(TARGET) $(LINUX_FILES)


