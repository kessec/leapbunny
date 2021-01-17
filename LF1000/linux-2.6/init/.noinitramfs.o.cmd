cmd_init/noinitramfs.o := arm-linux-gcc -Wp,-MD,init/.noinitramfs.o.d  -nostdinc -isystem /opt/RidgeRun/arm-eabi-uclibc/lib/gcc/arm-linux-uclibcgnueabi/4.1.1/include -Iinclude  -I/vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-lf1000/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O3 -marm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=5 -mcpu=arm926ej-s -msoft-float -Uarm -fno-stack-protector -fomit-frame-pointer -g -Wdeclaration-after-statement -Wno-pointer-sign   -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(noinitramfs)"  -D"KBUILD_MODNAME=KBUILD_STR(noinitramfs)"  -c -o init/noinitramfs.o init/noinitramfs.c

deps_init/noinitramfs.o := \
  init/noinitramfs.c \
  include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
  include/linux/stat.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/stat.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/posix_types.h \
  include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
  include/linux/cache.h \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/spinlock/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /opt/RidgeRun/arm-eabi-uclibc/lib/gcc/arm-linux-uclibcgnueabi/4.1.1/include/stdarg.h \
  include/linux/linkage.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/linkage.h \
  include/linux/bitops.h \
    $(wildcard include/config/generic/find/first/bit.h) \
    $(wildcard include/config/generic/find/last/bit.h) \
    $(wildcard include/config/generic/find/next/bit.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/bitops.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/system.h \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/fa526.h) \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/sa110.h) \
    $(wildcard include/config/cpu/32v6k.h) \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
    $(wildcard include/config/x86.h) \
  include/linux/typecheck.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/irqflags.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/hwcap.h \
  include/asm-generic/cmpxchg-local.h \
  include/asm-generic/cmpxchg.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/lock.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/ratelimit.h \
  include/linux/param.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/param.h \
    $(wildcard include/config/hz.h) \
  include/linux/dynamic_debug.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/swab.h \
  include/linux/byteorder/generic.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  include/asm-generic/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/div64.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/cache.h \
    $(wildcard include/config/aeabi.h) \
  include/linux/seqlock.h \
  include/linux/spinlock.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/generic/lockbreak.h) \
    $(wildcard include/config/preempt.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
  include/linux/preempt.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  include/linux/thread_info.h \
    $(wildcard include/config/compat.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/thread_info.h \
    $(wildcard include/config/arm/thumbee.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/fpstate.h \
    $(wildcard include/config/vfpv3.h) \
    $(wildcard include/config/iwmmxt.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
    $(wildcard include/config/mmu.h) \
  include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  include/linux/poison.h \
  include/linux/prefetch.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/processor.h \
  include/linux/stringify.h \
  include/linux/bottom_half.h \
  include/linux/spinlock_types.h \
  include/linux/spinlock_types_up.h \
  include/linux/lockdep.h \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/lock/stat.h) \
    $(wildcard include/config/generic/hardirqs.h) \
  include/linux/spinlock_up.h \
  include/linux/spinlock_api_up.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/atomic.h \
  include/asm-generic/atomic-long.h \
  include/linux/math64.h \
  include/linux/kdev_t.h \
  include/linux/syscalls.h \
    $(wildcard include/config/ftrace/syscalls.h) \
    $(wildcard include/config/ppc64.h) \
    $(wildcard include/config/alpha.h) \
    $(wildcard include/config/mips.h) \
    $(wildcard include/config/have/syscall/wrappers.h) \
    $(wildcard include/config/ia64.h) \
  include/linux/aio_abi.h \
  include/linux/capability.h \
    $(wildcard include/config/security/file/capabilities.h) \
  include/linux/sem.h \
    $(wildcard include/config/sysvipc.h) \
  include/linux/ipc.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/ipcbuf.h \
  include/linux/kref.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/sembuf.h \
  include/linux/rcupdate.h \
    $(wildcard include/config/classic/rcu.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/preempt/rcu.h) \
  include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  include/linux/cpumask.h \
    $(wildcard include/config/disable/obsolete/cpumask/functions.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
  include/linux/bitmap.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/string.h \
  include/linux/completion.h \
  include/linux/wait.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/current.h \
  include/linux/rcuclassic.h \
    $(wildcard include/config/rcu/cpu/stall/detector.h) \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/siginfo.h \
  include/asm-generic/siginfo.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/signal.h \
  include/asm-generic/signal-defs.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/sigcontext.h \
  include/linux/quota.h \
  include/linux/errno.h \
  /vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include/asm/errno.h \
  include/asm-generic/errno.h \
  include/asm-generic/errno-base.h \
  include/linux/mutex.h \
    $(wildcard include/config/debug/mutexes.h) \
  include/linux/rwsem.h \
    $(wildcard include/config/rwsem/generic/spinlock.h) \
  include/linux/rwsem-spinlock.h \
  include/linux/dqblk_xfs.h \
  include/linux/dqblk_v1.h \
  include/linux/dqblk_v2.h \
  include/linux/dqblk_qtree.h \
  include/linux/key.h \
    $(wildcard include/config/keys.h) \
    $(wildcard include/config/sysctl.h) \
  include/linux/rbtree.h \
  include/linux/sysctl.h \
  include/trace/syscall.h \

init/noinitramfs.o: $(deps_init/noinitramfs.o)

$(deps_init/noinitramfs.o):
