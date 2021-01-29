cmd_arch/arm/mm/abort-ev5tj.o := arm-linux-gcc -Wp,-MD,arch/arm/mm/.abort-ev5tj.o.d  -nostdinc -isystem /opt/RidgeRun/arm-eabi-uclibc/lib/gcc/arm-linux-uclibcgnueabi/4.1.1/include -Iinclude  -I/vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-lf1000/include -Iarch/arm/plat-mes/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=5 -mcpu=arm926ej-s -msoft-float -gdwarf-2       -c -o arch/arm/mm/abort-ev5tj.o arch/arm/mm/abort-ev5tj.S

deps_arch/arm/mm/abort-ev5tj.o := \
  arch/arm/mm/abort-ev5tj.S \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/linkage.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/smp.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/hwcap.h \
  arch/arm/mm/abort-macro.S \

arch/arm/mm/abort-ev5tj.o: $(deps_arch/arm/mm/abort-ev5tj.o)

$(deps_arch/arm/mm/abort-ev5tj.o):
