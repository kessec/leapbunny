cmd_arch/arm/kernel/entry-armv.o := arm-linux-gcc -Wp,-MD,arch/arm/kernel/.entry-armv.o.d  -nostdinc -isystem /opt/RidgeRun/arm-eabi-uclibc/lib/gcc/arm-linux-uclibcgnueabi/4.1.1/include -Iinclude  -I/vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-lf1000/include -Iarch/arm/plat-mes/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=5 -mcpu=arm926ej-s -msoft-float -gdwarf-2       -c -o arch/arm/kernel/entry-armv.o arch/arm/kernel/entry-armv.S

deps_arch/arm/kernel/entry-armv.o := \
  arch/arm/kernel/entry-armv.S \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/local/timers.h) \
    $(wildcard include/config/kprobes.h) \
    $(wildcard include/config/aeabi.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/needs/syscall/for/cmpxchg.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/neon.h) \
    $(wildcard include/config/cpu/arm610.h) \
    $(wildcard include/config/cpu/arm710.h) \
    $(wildcard include/config/iwmmxt.h) \
    $(wildcard include/config/crunch.h) \
    $(wildcard include/config/vfp.h) \
    $(wildcard include/config/cpu/32v6k.h) \
    $(wildcard include/config/has/tls/reg.h) \
    $(wildcard include/config/tls/reg/emul.h) \
    $(wildcard include/config/arm/thumb.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/memory.h \
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/runtime/phys/offset.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/sparsemem.h) \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/const.h \
  arch/arm/mach-lf1000/include/mach/memory.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/sizes.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/glue.h \
    $(wildcard include/config/cpu/abrt/lv4t.h) \
    $(wildcard include/config/cpu/abrt/ev4.h) \
    $(wildcard include/config/cpu/abrt/ev4t.h) \
    $(wildcard include/config/cpu/abrt/ev5tj.h) \
    $(wildcard include/config/cpu/abrt/ev5t.h) \
    $(wildcard include/config/cpu/abrt/ev6.h) \
    $(wildcard include/config/cpu/abrt/ev7.h) \
    $(wildcard include/config/cpu/pabrt/ifar.h) \
    $(wildcard include/config/cpu/pabrt/noifar.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/vfpmacros.h \
    $(wildcard include/config/vfpv3.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/vfp.h \
  arch/arm/mach-lf1000/include/mach/entry-macro.S \
  arch/arm/mach-lf1000/include/mach/hardware.h \
  arch/arm/mach-lf1000/include/mach/platform.h \
    $(wildcard include/config/mach/me/lf1000.h) \
    $(wildcard include/config/mach/lf/lf1000.h) \
    $(wildcard include/config/cpu/speed/385024000.h) \
    $(wildcard include/config/cpu/speed/393216000.h) \
    $(wildcard include/config/cpu/speed/532480000.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/hardware/vic.h \
    $(wildcard include/config/arch/lf1000.h) \
  arch/arm/mach-lf1000/include/mach/ic.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/thread_notify.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/unwind.h \
    $(wildcard include/config/arm/unwind.h) \
  arch/arm/kernel/entry-header.S \
    $(wildcard include/config/frame/pointer.h) \
    $(wildcard include/config/alignment/trap.h) \
  include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  include/linux/linkage.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/linkage.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/ptrace.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/hwcap.h \
  include/asm/asm-offsets.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/errno.h \
  include/asm-generic/errno.h \
  include/asm-generic/errno-base.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/thread_info.h \
    $(wildcard include/config/arm/thumbee.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/fpstate.h \

arch/arm/kernel/entry-armv.o: $(deps_arch/arm/kernel/entry-armv.o)

$(deps_arch/arm/kernel/entry-armv.o):
