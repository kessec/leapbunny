cmd_arch/arm/kernel/debug.o := arm-linux-gcc -Wp,-MD,arch/arm/kernel/.debug.o.d  -nostdinc -isystem /opt/RidgeRun/arm-eabi-uclibc/lib/gcc/arm-linux-uclibcgnueabi/4.1.1/include -Iinclude  -I/vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-lf1000/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=5 -mcpu=arm926ej-s -msoft-float -gdwarf-2       -c -o arch/arm/kernel/debug.o arch/arm/kernel/debug.S

deps_arch/arm/kernel/debug.o := \
  arch/arm/kernel/debug.S \
    $(wildcard include/config/debug/icedcc.h) \
    $(wildcard include/config/cpu/v6.h) \
    $(wildcard include/config/cpu/xscale.h) \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/linkage.h \
  arch/arm/mach-lf1000/include/mach/debug-macro.S \
  arch/arm/mach-lf1000/include/mach/platform.h \
    $(wildcard include/config/mach/me/lf1000.h) \
    $(wildcard include/config/mach/lf/lf1000.h) \
    $(wildcard include/config/cpu/speed/385024000.h) \
    $(wildcard include/config/cpu/speed/393216000.h) \
    $(wildcard include/config/cpu/speed/532480000.h) \
  arch/arm/mach-lf1000/include/mach/uart.h \

arch/arm/kernel/debug.o: $(deps_arch/arm/kernel/debug.o)

$(deps_arch/arm/kernel/debug.o):
