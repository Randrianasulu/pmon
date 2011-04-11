/*	$Id: start.S,v 1.1.1.1 2006/09/14 01:59:08 root Exp $ */

/*
 * Copyright (c) 2001 Opsycon AB  (www.opsycon.se)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Opsycon AB, Sweden.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _KERNEL
#define _KERNEL
#endif

#include <asm.h>
#include <regnum.h>
#include <cpu.h>
#include <pte.h>

#include "pmon/dev/ns16550.h"
#include "target/prid.h"
#include "target/sbd.h"
#include "target/fcr.h"
#include "target/via686b.h"
#include "target/i8254.h"
#include "target/isapnpreg.h"
#define DEBUG_LOCORE

#ifndef BOOT_FROM_EJTAG      
#define  BOOT_FROM_EJTAG
#endif
#undef BOOT_FROM_EJTAG

#ifdef DEBUG_LOCORE
#define	TTYDBG(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#else
#define TTYDBG(x)
#endif

#define	PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop


#define tmpsize		s1
#define msize		s2
#define bonito		s4
#define dbg			s5
#define sdCfg		s6

#define CP0_CONFIG $16
#define CP0_TAGLO  $28
#define CP0_TAGHI  $29


/*
 *   Register usage:
 *
 *	s0	link versus load offset, used to relocate absolute adresses.
 *	s1	free
 *	s2	memory size.
 *	s3	free.
 *	s4	Bonito base address.
 *	s5	dbg.
 *	s6	sdCfg.
 *	s7	rasave.
 *	s8	L3 Cache size.
 */


	.set	noreorder
	.globl	_start
	.globl	start
	.globl	__main
_start:
start:
	.globl	stack
stack = start - 0x4000		/* Place PMON stack below PMON start in RAM */

/* NOTE!! Not more that 16 instructions here!!! Right now it's FULL! */
	mtc0	zero, COP_0_STATUS_REG
	mtc0	zero, COP_0_CAUSE_REG
	li	t0, SR_BOOT_EXC_VEC	/* Exception to Boostrap Location */
	mtc0	t0, COP_0_STATUS_REG
	la	sp, stack
	la	gp, _gp

//	bal	uncached		/* Switch to uncached address space */
	nop

	bal	locate			/* Get current execute address */
	nop

uncached:
	or	ra, UNCACHED_MEMORY_ADDR
	j	ra
	nop

/*
 *  Reboot vector usable from outside pmon.
 */
	.align	9
ext_map_and_reboot:
	bal	CPU_TLBClear
	nop

	li	a0, 0xc0000000
	li	a1, 0x40000000
	bal	CPU_TLBInit
	nop
	la	v0, tgt_reboot
	la	v1, start
	subu	v0, v1
	lui	v1, 0xffc0
	addu	v0, v1
	jr	v0
	nop

 /* */
	nop
	b	exc_common

	.align	7			/* bfc00280 */
	la	a0, v280_msg
	bal	stringserial
	nop
	b	exc_common

/* Cache error */
	.align	8			/* bfc00300 */
	PRINTSTR("\r\nPANIC! Unexpected Cache Error exception! ")
	mfc0	a0, COP_0_CACHE_ERR
	bal	hexserial
	nop
	b	exc_common

/* General exception */
	.align	7			/* bfc00380 */
	la	a0, v380_msg
	bal	stringserial
	nop
	b	exc_common
	
	.align	9			/* bfc00400 */
	la	a0, v400_msg
	bal	stringserial
	nop
	b	exc_common

	.align	7			/* bfc00480 */
	la	a0, v480_msg
	bal	stringserial
	nop
	b	exc_common

	
	.align	8			/* bfc00500 */
exc_common:
	PRINTSTR("\r\nCAUSE=")
	mfc0	a0, COP_0_CAUSE_REG
	bal	hexserial
	nop
	PRINTSTR("\r\nSTATUS=")
	mfc0	a0, COP_0_STATUS_REG
	bal	hexserial
	nop
	PRINTSTR("\r\nERRORPC=")
	mfc0	a0, COP_0_ERROR_PC
	bal	hexserial
	nop
	PRINTSTR("\r\nEPC=")
	mfc0	a0, COP_0_EXC_PC
	bal	hexserial
	nop

	bal mydebug_main
	nop
1:
	b 1b
	nop


	PRINTSTR("\r\nDERR0=")
	cfc0	a0, COP_0_DERR_0
	bal	hexserial
	nop
	PRINTSTR("\r\nDERR1=")
	cfc0	a0, COP_0_DERR_1
	bal	hexserial
	nop

	b	ext_map_and_reboot
	nop

	.align 9
		nop
	.align 8
	.word read
	.word write
	.word open
	.word close
	.word nullfunction
	.word printf
	.word vsprintf
	.word nullfunction
	.word nullfunction
	.word getenv
	.word nullfunction
	.word nullfunction
	.word nullfunction
	.word nullfunction


/*
 *  We get here from executing a bal to get the PC value of the current execute
 *  location into ra. Check to see if we run from ROM or if this is ramloaded.
 */
locate:
	la	s0,uncached
	subu	s0,ra,s0

	#bal	CPU_TLBClear
	#nop
	li	t0,SR_BOOT_EXC_VEC
	mtc0	t0,COP_0_STATUS_REG
        mtc0    zero,COP_0_CAUSE_REG
	.set noreorder
/*spi speed*/
    li  t0, 0xbfe80000
    li  t1, 0x17  // div 4, fast_read + burst_en + memory_en
    sb  t1, 0x4(t0) // sfc_param

#ifdef LS1FSOC/* to adjust the DDR frequency */
    li  a0, 0xbfe78030
#define DDRCFG_DATA (0x8888|(CPU_MULT-1)|((DDR_MULT-3)<<8))
    li  a1, DDRCFG_DATA   // 1fboard 25MHz
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
    sw  a1, 0x0(a0)
    nop
#else
    li  a0, 0xbfe78030
    /*31:dc_div_en,30-26:dc_div,25:cpu_div_en,24-20:cpu_div,19:ddr_div_en,18-14:ddr_div*/
    li v1,(1<<31)|(4<<26)|(1<<25)|(1<<20)|(1<<19)|(2<<14)
    sw v1,4(a0)
    li  a1, 4
    sw a1,(a0);

1:
   lw v1,(a0)
   bne a1,v1,1b
   nop
	
#endif


	bal	initserial
	nop
	

	PRINTSTR("\r\nPMON2000 MIPS Initializing. Standby...\r\n")
	PRINTSTR("ERRORPC=")
	mfc0	a0, COP_0_ERROR_PC
	bal	hexserial
	nop

	PRINTSTR(" CONFIG=")
	mfc0	a0, COP_0_CONFIG
	bal	hexserial
	nop
	PRINTSTR("\r\n")
	
	PRINTSTR(" PRID=")
	mfc0	a0, COP_0_PRID
	bal	hexserial
	nop
	PRINTSTR("\r\n")

	bnez s0,1f
	nop
	li a0,128
	jal initmips
	nop
1:

// use only 8wins
#define CPU_WIN_BASE 0xbfd00000
#define CPU_WIN_MASK 0xbfd00040
#define CPU_WIN_MMAP 0xbfd00080

#define set_cpu_window(id, base, mask, mmap) \
        li      t0, CPU_WIN_BASE          ;  \
        sw      $0, 0x80+id*8(t0)         ;  \
        li      t1, base                  ;  \
        sw      t1, 0x00+id*8(t0)         ;  \
        sw      $0, 0x04+id*8(t0)         ;  \
        li      t1, mask                  ;  \
        sw      t1, 0x40+id*8(t0)         ;  \
        sw      $0, 0x44+id*8(t0)         ;  \
        li      t1, mmap                  ;  \
        sw      t1, 0x80+id*8(t0)         ;  \
        sw      $0, 0x84+id*8(t0)

#if 1 /* fixup cpu window */
cpu_win_fixup:
        //
        // hit         = (paddr & mask) == (mmap & mask)
        // mapped_addr =  paddr &~mask | mmap & mask
        //
        // mmap[7] -> enable
        // mmap[4] -> cachable
        // mmap[1:0] -> destination
        //
        // NOTE: the address windows has priority, win0 > win1 > ... > win7
        set_cpu_window(0, 0x1fc00000, 0xfff00000, 0x1fc000f3) // boot rom
        set_cpu_window(1, 0x10000000, 0xf8000000, 0x10000001) // PCI mem0, mem1,    disabled
        set_cpu_window(2, 0x18000000, 0xfc000000, 0x18000001) // PCI mem2           disabled
        set_cpu_window(3, 0x1c000000, 0xffe00000, 0x1c000001) // PCI cfg/IO/header  disabled
        set_cpu_window(4, 0x1c200000, 0xffe00000, 0x1c2000d2) // gpu 1c2 /dc 1c3
        set_cpu_window(5, 0x1f000000, 0xff000000, 0x1f0000d3) // AXIMUX
        set_cpu_window(6, 0x40000000, 0xc0000000, 0x000000f0) // DDR 1GB
        set_cpu_window(7, 0x00000000, 0x00000000, 0x000000f0) // everything else
        // after this fixup, the kernel code should be compiled with
        // uncached instruction fetch patch
#endif 


	/* 
	 * Now determine DRAM configuration and size by
	 * reading the I2C EEROM on the DIMMS
	 */



#ddr2config by cww 20090901
        li      msize,0x0f000000
        PRINTSTR("DDR2 config begin_2\r\n")
        bal     ddr2_config_old
        nop
        PRINTSTR("DDR2 config begin_2\r\n")
        bal     ddr2_config
        nop
        PRINTSTR("DDR2 config end\r\n")


#define CF_7_SE         (1 << 3)        /* Secondary cache enable */
#define CF_7_SC         (1 << 31)       /* Secondary cache not present */
#define CF_7_TE         (1 << 12)       /* Tertiary cache enable */
#define CF_7_TC         (1 << 17)       /* Tertiary cache not present */
#define CF_7_TS         (3 << 20)       /* Tertiary cache size */
#define CF_7_TS_AL      20              /* Shift to align */
#define NOP8 nop;nop;nop;nop;nop;nop;nop;nop
do_caches:
	TTYDBG("Init caches...\r\n")

	li	s7, 0                   /* no L2 cache */
	li	s8, 0                   /* no L3 cache */

	TTYDBG("godson1 caches found\r\n")
    bal     cache_init
    nop

	TTYDBG("Init caches done, cfg = ")
	mfc0	a0, COP_0_CONFIG
	bal	hexserial
	nop
	TTYDBG("\r\n\r\n")


    mfc0   a0,COP_0_CONFIG
    and    a0,a0,~((1<<12) | 3)
	or     a0,a0,2
    mtc0   a0,COP_0_CONFIG


PRINTSTR("begin test\r\n");

###lcd soft_reset and panel config&timing
	li a0,0xbc301240
	li a1,0x00100000
	sw a1,0x0(a0)
	li a1,0x00000000
	sw a1,0x0(a0)

	li a0,0xbc3013c0
	li a1,0x80001111
	sw a1,0x0(a0)
	li a1,0x33333333
	sw a1,0x20(a0)



#include "newtest.32/mydebug.S"
bootnow:
PRINTSTR("end test\r\n");
#define	FCALL_PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; la v0, stringserial; addu v0,s0;jalr v0; nop	

#undef BAL
#define BAL(x) \
	la v0,x; \
	addu v0,s0; \
	jalr v0; \
	nop;





/*copy copy program to sdram to make copy fast*/

	la t0,121f
	addu t0,s0
	la t1,122f
	addu t1,s0
	
	li t2,0xa0000000
1:
	lw  v0,(t0)
	sw v0,(t2)
	addu t0,4
	addu t2,4
	ble t0,t1,1b
	nop

	li t0,0xa0000000
   	jr t0	
	nop		

.align 3
121: 
	FCALL_PRINTSTR("Copy PMON to execute location...\r\n")
	la	a0, start
	addu a1,a0,s0
	la	a2, _edata
        or      a0, 0xa0000000
        or      a2, 0xa0000000
	subu	t1, a2, a0
	srl	t1, t1, 2

	move	t0, a0
	move	t1, a1
	move	t2, a2

	/* copy text section */
	
1:	and	t3,t0,0x0000ffff
	bnez	t3,2f
	nop
	move	a0,t0
	BAL(hexserial)
	nop
	li	a0,'\r'
	BAL(tgt_putchar)
	nop
2:	lw	t3, 0(t1)
	nop
	sw	t3, 0(t0)
	addu	t0, 4
	addu	t1, 4
	bne	t2, t0, 1b
	nop

	FCALL_PRINTSTR("\ncopy text section done.\r\n")
	
	/* Clear BSS */
	la	a0, _edata
	la	a2, _end
2:	sw	zero, 0(a0)
	bne	a2, a0, 2b
	addu	a0, 4


	FCALL_PRINTSTR("Copy PMON to execute location done.\r\n")
	FCALL_PRINTSTR("sp=");
	move a0, sp
	BAL(hexserial)
	nop

	li	a0, 0
	sw	a0, CpuTertiaryCacheSize /* Set L3 cache size */


	move	a0,msize
	srl	a0,20


	la	v0, initmips
	jalr	v0
	nop

.align 3
122:

stuck:
	b	stuck
	nop

/*
 *  Clear the TLB. Normally called from start.S.
 */
#if __mips64
#define MTC0 dmtc0
#else 
#define MTC0 mtc0
#endif
LEAF(CPU_TLBClear)
	li	a3, 0			# First TLB index.

	li	a2, PG_SIZE_4K
	MTC0   a2, COP_0_TLB_PG_MASK   # Whatever...

1:
	MTC0   zero, COP_0_TLB_HI	# Clear entry high.
	MTC0   zero, COP_0_TLB_LO0	# Clear entry low0.
	MTC0   zero, COP_0_TLB_LO1	# Clear entry low1.

	mtc0    a3, COP_0_TLB_INDEX	# Set the index.
	addiu	a3, 1
	li	a2, 64
	nop
	nop
	tlbwi				# Write the TLB

	bne	a3, a2, 1b
	nop

	jr	ra
	nop
END(CPU_TLBClear)

/*
 *  Set up the TLB. Normally called from start.S.
 */
LEAF(CPU_TLBInit)
	li	a3, 0			# First TLB index.

	li	a2, PG_SIZE_16M
	MTC0   a2, COP_0_TLB_PG_MASK   # All pages are 16Mb.

1:
	and	a2, a0, PG_SVPN
	MTC0   a2, COP_0_TLB_HI	# Set up entry high.

	move	a2, a0
	srl	a2, a0, PG_SHIFT 
	and	a2, a2, PG_FRAME
	ori	a2, PG_IOPAGE
	MTC0   a2, COP_0_TLB_LO0	# Set up entry low0.
	addu	a2, (0x01000000 >> PG_SHIFT)
	MTC0   a2, COP_0_TLB_LO1	# Set up entry low1.

	mtc0    a3, COP_0_TLB_INDEX	# Set the index.
	addiu	a3, 1
	li	a2, 0x02000000
	subu	a1, a2
	nop
	tlbwi				# Write the TLB

	bgtz	a1, 1b
	addu	a0, a2			# Step address 32Mb.

	jr	ra
	nop
END(CPU_TLBInit)

LEAF(stringserial)
	nop
	move	a2, ra
	addu	a1, a0, s0
	lbu	a0, 0(a1)
1:
	beqz	a0, 2f
	nop
	bal	tgt_putchar
	addiu	a1, 1
	b	1b
	lbu	a0, 0(a1)

2:
	j	a2
	nop
END(stringserial)

LEAF(outstring)
	move	a2, ra
	move	a1, a0
	lbu	a0, 0(a1)
1:
	beqz	a0, 2f
	nop
	bal	tgt_putchar
	addiu	a1, 1
	b	1b
	lbu	a0, 0(a1)

2:
	j	a2
	nop
END(outstring)

LEAF(hexserial)
	nop
	move	a2, ra
	move	a1, a0
	li	a3, 7
1:
	rol	a0, a1, 4
	move	a1, a0
	and	a0, 0xf
	la	v0, hexchar
	addu	v0, s0
	addu	v0, a0
	bal	tgt_putchar
	lbu	a0, 0(v0)

	bnez	a3, 1b
	addu	a3, -1

	j	a2
	nop
END(hexserial)

LEAF(tgt_putchar)
	nop
	la	v0, COM1_BASE_ADDR
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb	a0, NSREG(NS16550_DATA)(v0)
	j	ra
	nop	
END(tgt_putchar)

LEAF(tgt_testchar)
	la	v0, COM1_BASE_ADDR
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v0,v1, LSR_RXRDY
	jr ra
	nop
END(tgt_testchar)

LEAF(tgt_getchar)
	la	v0, COM1_BASE_ADDR
1:
	lbu	v1, NSREG(NS16550_LSR)(v0)
	and	v1, LSR_RXRDY
	beqz	v1, 1b
	nop
	lb	v0, NSREG(NS16550_DATA)(v0)
	jr ra
	nop
END(tgt_getchar)


/* baud rate definitions, matching include/termios.h */
#define B0      0
#define B50     50      
#define B75     75
#define B110    110
#define B134    134
#define B150    150
#define B200    200
#define B300    300
#define B600    600
#define B1200   1200
#define B1800   1800
#define B2400   2400
#define B4800   4800
#define B9600   9600
#define B19200  19200
#define B38400  38400
#define B57600  57600
#define B115200 115200

LEAF(initserial)
	la	v0, COM1_BASE_ADDR
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
#ifdef LS1FSOC
 	li	v1, ((APB_CLK*DDR_MULT)/(2*16*115200))     // 8MHz crystal,   M[7:3]=6     1fboard
#else
	li	a0,0xbfe78030
	lw	a1,(a0)
	andi	a2,a1,0x3f
	addiu	a2,12
	sll     a2,10
	srl a1,8
	andi	a1,0x3ff
	addu	a1,a2
	li	a2,(33333333>>11)
	multu   a1,a2
	mflo    v1
	lw	a1,4(a0)
	srl	a1,14
	andi    a2,a1,0x20
	beqz    a2,1f
	nop
	andi    a1,0x1f
	divu    v1,a1
	mflo    v1
	b 2f
	nop
1:
	srl v1,1	
2:
#if 1
/*save div,for serial debug*/
	.set mips32
	mtc0 v1,$25,1
	.set mips0
#endif
	li a1,2*16*9600
	divu v1,v1,a1
#endif
	sb	v1, NSREG(NS16550_DATA)(v0)
	srl	v1, 8
	sb	v1, NSREG(NS16550_IER)(v0)
	li	v1, CFCR_8BITS
	sb	v1, NSREG(NS16550_CFCR)(v0)
	li	v1, MCR_DTR|MCR_RTS
	sb	v1, NSREG(NS16550_MCR)(v0)
	li	v1, 0x0
	sb	v1, NSREG(NS16550_IER)(v0)
        
        #disable all interrupt
        li      v1, 0x0
        sb      v1, NSREG(NS16550_IER)(v0)
	j	ra
	nop
END(initserial)

__main:
	j	ra
	nop


	.rdata
transmit_pat_msg:
	.asciz	"\r\nInvalid transmit pattern.  Must be DDDD or DDxDDx\r\n"
v200_msg:
	.asciz	"\r\nPANIC! Unexpected TLB refill exception!\r\n"
v280_msg:
	.asciz	"\r\nPANIC! Unexpected XTLB refill exception!\r\n"
v380_msg:
	.asciz	"\r\nPANIC! Unexpected General exception!\r\n"
v400_msg:
	.asciz	"\r\nPANIC! Unexpected Interrupt exception!\r\n"
v480_msg:
	.asciz	"\r\nPANIC! You have been in the Ejtag Debug MOde Trap is 0!\r\n"
hexchar:
	.ascii	"0123456789abcdef"

	.text
	.align	2
/*
 *   I2C Functions used in early startup code to get SPD info from
 *   SDRAM modules. This code must be entirely PIC and RAM independent.
 */

/* Delay macro */
#define	DELAY(count)	\
	li v0, count;	\
99:			\
	bnz	vo, 99b;\
	addiu	v0, -1



#define Index_Store_Tag_D			0x09
#define Index_Invalidate_I			0x00
#define Index_Writeback_Inv_D			0x01

LEAF(nullfunction)
	jr ra
	nop
END(nullfunction)

	.ent	cache_init
    .global cache_init
    .set noreorder
cache_init:
        move t1,ra
####part 2####
cache_detect_4way:
    .set mips32
        mfc0    t4, CP0_CONFIG,1
#        mfc0    $12, $16,1
	lui	t5, 0x7
        and    t5, t4, t5    
	srl	t5, t5, 16    #ic
        andi    t6, t4, 0x0380
        srl     t6, t6, 7     #dc
        addiu   t8, $0, 1
                                #set dcache way
        beq     t6, $0,  cache_d1way
        addiu   t7, $0, 1       #1 way
        beq     t6, t8,  cache_d2way
        addiu   t7, $0, 2       #2 way
        beq     $0, $0, cache_d4way
        addiu   t7, $0, 4       #4 way
cache_d1way:
        beq     $0, $0, 1f
        addiu   t6, t6, 12      #1 way
cache_d2way:
        beq     $0, $0, 1f
        addiu   t6, t6, 11      #2 way
cache_d4way:
        addiu   t6, t6, 10      #4 way (10), 2 way(11), 1 way(12)
1:                              #set icache way
        beq     t5, $0,  cache_i1way
        addiu   t3, $0, 1       #1 way
        beq     t5, t8,  cache_i2way
        addiu   t3, $0, 2       #2 way
        beq     $0, $0, cache_i4way
        addiu   t3, $0, 4       #4 way
cache_i1way:
        beq     $0, $0, 1f
        addiu   t5, t5, 12
cache_i2way:
        beq     $0, $0, 1f
        addiu   t5, t5, 11
cache_i4way:
        addiu   t5, t5, 10      #4 way (10), 2 way(11), 1 way(12)

1:      addiu   t4, $0, 1
        sllv    t6, t4, t6
        sllv    t5, t4, t5


#if 0
    la	t0, memvar
	sw	t7, 0x0(t0) #ways
	sw	t5, 0x4(t0) #icache size
	sw	t6, 0x8(t0) #dcache size
#endif
####part 3####
#	.set	mips3
	lui	a0, 0x8000
	addu	a1, $0, t5
	addu	a2, $0, t6
cache_init_d2way:
#a0=0x80000000, a1=icache_size, a2=dcache_size
#a3, v0 and v1 used as local registers
	mtc0	$0, CP0_TAGHI
	addu	v0, $0, a0
	addu	v1, a0, a2
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	mtc0	$0, CP0_TAGLO
	beq	t7, 1, 4f
	cache	Index_Store_Tag_D, 0x0(v0)	# 1 way
	beq	t7, 2 ,4f
	cache	Index_Store_Tag_D, 0x1000(v0)	# 2 way
	cache	Index_Store_Tag_D, 0x2000(v0)	# 4 way
	cache	Index_Store_Tag_D, 0x3000(v0)
4:	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_i2way:
	addu	v0, $0, a0
	addu	v1, a0, a1
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	beq	t3, 1, 4f
	cache	Index_Invalidate_I, 0x0(v0)	# 1 way
	beq	t3, 2, 4f
	cache	Index_Invalidate_I, 0x1000(v0)	# 2 way
	cache	Index_Invalidate_I, 0x2000(v0)
	cache	Index_Invalidate_I, 0x3000(v0)	# 4 way
4:	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
1:
cache_flush_d2way:
	addu	v0, $0, a0
	addu	v1, a0, a2
1:	slt	a3, v0, v1
	beq	a3, $0, 1f
	nop
	beq	t7, 1, 4f
	cache	Index_Writeback_Inv_D, 0x0(v0) 	#1 way
	beq	t7, 2, 4f
	cache	Index_Writeback_Inv_D, 0x1000(v0)	# 2 way
	cache	Index_Writeback_Inv_D, 0x2000(v0)
	cache	Index_Writeback_Inv_D, 0x3000(v0)	# 4 way
4:	beq	$0, $0, 1b
	addiu	v0, v0, 0x20
	.set	mips0

1:
cache_init_finish:
    jr	t1
    nop
    .set reorder
	.end 	cache_init

#define REG_ADDRESS 0x0
#define CONFIG_BASE 0xaffffe00

        .global ddr2_config_old
        .ent    ddr2_config_old
        .set    noreorder
       # .set    mips3
ddr2_config_old:
    la      t0, ddr2_reg_data_old
    addu    t0, t0, s0
    li      t1, 0x1d
    li      t2, CONFIG_BASE

reg_write_old:
    ld      a1, 0x0(t0)
    sd      a1, REG_ADDRESS(t2)
    subu    t1, t1, 0x1
    addiu     t0, t0, 0x8
    bne     t1, $0, reg_write_old
    addiu     t2, t2, 0x10
    
    ############start##########
    li      t2, CONFIG_BASE
    la		t0,DDR2_CTL_start_DATA_LO_old
    #la		t0,ddr2_start_reg
    addu    	t0, t0, s0
    ld      a1, 0x0(t0)
    sd      a1, 0x30(t2)

    jr      ra
    nop
    .end    ddr2_config_old
#ddr2 by cww 20090901
#if 1
#include "ddr2fconfig.S"
#include "ddr2fconfig.old.S"
#endif

