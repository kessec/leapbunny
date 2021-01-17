	.file	"bounds.c"
@ GNU C version 4.1.1 (arm-linux-uclibcgnueabi)
@	compiled by GNU C version 4.1.2 (Ubuntu 4.1.2-0ubuntu4).
@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -nostdinc -Iinclude
@ -I/vol/hdd0/Git/leapbunny/LF1000/linux-2.6/arch/arm/include
@ -Iarch/arm/mach-lf1000/include -D__KERNEL__ -D__LINUX_ARM_ARCH__=5 -Uarm
@ -DKBUILD_STR(s)=#s -DKBUILD_BASENAME=KBUILD_STR(bounds)
@ -DKBUILD_MODNAME=KBUILD_STR(bounds) -isystem -include -MD -mlittle-endian
@ -marm -mabi=aapcs-linux -mno-thumb-interwork -mcpu=arm926ej-s
@ -msoft-float -auxbase-strip -g -O3 -Wall -Wundef -Wstrict-prototypes
@ -Wno-trigraphs -Werror-implicit-function-declaration -Wno-format-security
@ -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-aliasing
@ -fno-common -fno-delete-null-pointer-checks -funwind-tables
@ -fno-stack-protector -fomit-frame-pointer -fverbose-asm
@ options enabled:  -falign-loops -fargument-alias -fbranch-count-reg
@ -fcaller-saves -fcprop-registers -fcrossjumping -fcse-follow-jumps
@ -fcse-skip-blocks -fdefer-pop -fearly-inlining
@ -feliminate-unused-debug-types -fexpensive-optimizations -ffunction-cse
@ -fgcse -fgcse-after-reload -fgcse-lm -fguess-branch-probability -fident
@ -fif-conversion -fif-conversion2 -finline-functions
@ -finline-functions-called-once -fipa-pure-const -fipa-reference
@ -fipa-type-escape -fivopts -fkeep-static-consts -fleading-underscore
@ -floop-optimize -floop-optimize2 -fmath-errno -fmerge-constants
@ -fomit-frame-pointer -foptimize-register-move -foptimize-sibling-calls
@ -fpeephole -fpeephole2 -freg-struct-return -fregmove -freorder-blocks
@ -freorder-functions -frerun-cse-after-loop -frerun-loop-opt
@ -fsched-interblock -fsched-spec -fsched-stalled-insns-dep
@ -fschedule-insns -fschedule-insns2 -fshow-column -fsplit-ivs-in-unroller
@ -fstrength-reduce -fthread-jumps -ftrapping-math -ftree-ccp -ftree-ch
@ -ftree-copy-prop -ftree-copyrename -ftree-dce -ftree-dominator-opts
@ -ftree-dse -ftree-fre -ftree-loop-im -ftree-loop-ivcanon
@ -ftree-loop-optimize -ftree-lrs -ftree-pre -ftree-salias -ftree-sink
@ -ftree-sra -ftree-store-ccp -ftree-store-copy-prop -ftree-ter
@ -ftree-vect-loop-version -ftree-vrp -funit-at-a-time -funswitch-loops
@ -funwind-tables -fvar-tracking -fverbose-asm -fzero-initialized-in-bss
@ -mlittle-endian -msched-prolog

	.section	.debug_abbrev,"",%progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.text
.Ltext0:
@ Compiler executable checksum: 89a4a3ee2786ab13cec23902f7c85b41

	.align	2
	.global	foo
	.type	foo, %function
foo:
	.fnstart
.LFB2:
	.file 1 "kernel/bounds.c"
	.loc 1 14 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	@ lr needed for prologue	@
	.loc 1 16 0
#APP
	
->NR_PAGEFLAGS #23 __NR_PAGEFLAGS	@
	.loc 1 17 0
	
->MAX_NR_ZONES #2 __MAX_NR_ZONES	@
	.loc 1 19 0
	bx	lr	@
.LFE2:
	.fnend
	.size	foo, .-foo
	.section	.debug_frame,"",%progbits
.Lframe0:
	.4byte	.LECIE0-.LSCIE0
.LSCIE0:
	.4byte	0xffffffff
	.byte	0x1
	.ascii	"\000"
	.uleb128 0x1
	.sleb128 -4
	.byte	0xe
	.byte	0xc
	.uleb128 0xd
	.uleb128 0x0
	.align	2
.LECIE0:
.LSFDE0:
	.4byte	.LEFDE0-.LASFDE0
.LASFDE0:
	.4byte	.Lframe0
	.4byte	.LFB2
	.4byte	.LFE2-.LFB2
	.align	2
.LEFDE0:
	.text
.Letext0:
	.section	.debug_info
	.4byte	0x90
	.2byte	0x2
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.Ldebug_line0
	.4byte	.Letext0
	.4byte	.Ltext0
	.4byte	.LASF11
	.byte	0x1
	.4byte	.LASF12
	.4byte	.LASF13
	.uleb128 0x2
	.4byte	.LASF0
	.byte	0x1
	.byte	0x6
	.uleb128 0x2
	.4byte	.LASF1
	.byte	0x1
	.byte	0x8
	.uleb128 0x2
	.4byte	.LASF2
	.byte	0x2
	.byte	0x5
	.uleb128 0x2
	.4byte	.LASF3
	.byte	0x2
	.byte	0x7
	.uleb128 0x3
	.ascii	"int\000"
	.byte	0x4
	.byte	0x5
	.uleb128 0x2
	.4byte	.LASF4
	.byte	0x4
	.byte	0x7
	.uleb128 0x2
	.4byte	.LASF5
	.byte	0x8
	.byte	0x5
	.uleb128 0x2
	.4byte	.LASF6
	.byte	0x8
	.byte	0x7
	.uleb128 0x2
	.4byte	.LASF7
	.byte	0x4
	.byte	0x7
	.uleb128 0x2
	.4byte	.LASF4
	.byte	0x4
	.byte	0x7
	.uleb128 0x2
	.4byte	.LASF8
	.byte	0x1
	.byte	0x8
	.uleb128 0x2
	.4byte	.LASF9
	.byte	0x4
	.byte	0x5
	.uleb128 0x2
	.4byte	.LASF10
	.byte	0x1
	.byte	0x2
	.uleb128 0x4
	.byte	0x1
	.ascii	"foo\000"
	.byte	0x1
	.byte	0xe
	.byte	0x1
	.4byte	.LFB2
	.4byte	.LFE2
	.byte	0x1
	.byte	0x5d
	.byte	0x0
	.section	.debug_abbrev
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x10
	.uleb128 0x6
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.byte	0x0
	.byte	0x0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.byte	0x0
	.byte	0x0
	.uleb128 0x4
	.uleb128 0x2e
	.byte	0x0
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",%progbits
	.4byte	0x16
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x94
	.4byte	0x80
	.ascii	"foo\000"
	.4byte	0x0
	.section	.debug_aranges,"",%progbits
	.4byte	0x1c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0x0
	.2byte	0x0
	.2byte	0x0
	.4byte	.Ltext0
	.4byte	.Letext0-.Ltext0
	.4byte	0x0
	.4byte	0x0
	.section	.debug_str,"MS",%progbits,1
.LASF2:
	.ascii	"short int\000"
.LASF13:
	.ascii	"/vol/hdd0/Git/leapbunny/LF1000/linux-2.6\000"
.LASF5:
	.ascii	"long long int\000"
.LASF9:
	.ascii	"long int\000"
.LASF11:
	.ascii	"GNU C 4.1.1\000"
.LASF1:
	.ascii	"unsigned char\000"
.LASF0:
	.ascii	"signed char\000"
.LASF6:
	.ascii	"long long unsigned int\000"
.LASF4:
	.ascii	"unsigned int\000"
.LASF3:
	.ascii	"short unsigned int\000"
.LASF8:
	.ascii	"char\000"
.LASF10:
	.ascii	"_Bool\000"
.LASF7:
	.ascii	"long unsigned int\000"
.LASF12:
	.ascii	"kernel/bounds.c\000"
	.ident	"GCC: (GNU) 4.1.1"
