/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999, 2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * Interrupt exception dispatch code.
 *
 */
#include <linux/config.h>

#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>

	.text
	.set	noreorder
	.set	noat
	.align	5
	NESTED(mipsIRQ, PT_SIZE, sp)
	SAVE_ALL
	CLI
	.set	at

	mfc0	s0, CP0_CAUSE		# get irq mask
	mfc0	t1, CP0_STATUS
	and	s0, t1

	/* First we check for r4k counter/timer IRQ. */
	andi	a0, s0, CAUSEF_IP7
	beq	a0, zero, 1f
	nop

	/* Wheee, a timer interrupt. */
	move	a0, sp
	/* NOTE: this may just acknowledge the interrupt */
	jal	danube_timer_interrupt
	 nop

	j	ret_from_irq
	 nop

1:
	andi	a0, s0, CAUSEF_IP2	# delay slot, check hw0 interrupt
	beq	a0, zero, 1f
	nop
	/* Wheee, combined hardware level zero interrupt. */
	jal	danube_hw0_irqdispatch
	 move	a0, sp			# delay slot

	j	ret_from_irq
	 nop				# delay slot

1:
	andi	a0, s0, CAUSEF_IP3	# delay slot, check hw1 interrupt
	beq	a0, zero, 1f
	nop
	/* Wheee, combined hardware level one interrupt. */
	jal	danube_hw1_irqdispatch
	 move	a0, sp			# delay slot

	j	ret_from_irq
	 nop				# delay slot

1:
	andi	a0, s0, CAUSEF_IP4	# delay slot, check hw2 interrupt
	beq	a0, zero, 1f
	nop
	/* Wheee, combined hardware level two interrupt. */
	jal	danube_hw2_irqdispatch
	 move	a0, sp			# delay slot

	j	ret_from_irq
	 nop				# delay slot
1:
        andi    a0, s0, CAUSEF_IP5      # delay slot, check hw3 interrupt
        beq     a0, zero, 1f
        nop
        /* Wheee, combined hardware level three interrupt. */
        jal     danube_hw3_irqdispatch
         move   a0, sp                  # delay slot
                                                                                                         
        j       ret_from_irq
         nop                            # delay slot
                                                                                                         
1:
        andi    a0, s0, CAUSEF_IP6      # delay slot, check hw4 interrupt
        beq     a0, zero, 1f
        nop
        /* Wheee, combined hardware level three interrupt. */
        jal     danube_hw4_irqdispatch
         move   a0, sp                  # delay slot
                                                                                                         
        j       ret_from_irq
         nop                            # delay slot

1:
	/*
	 * Here by mistake?  This is possible, what can happen is that by the
	 * time we take the exception the IRQ pin goes low, so just leave if
	 * this is the case.
	 */
	move	a1,s0
	PRINT("Got interrupt: c0_cause = %08x\n")
	mfc0	a1, CP0_EPC
	PRINT("c0_epc = %08x\n")

	j	ret_from_irq
	 nop
	END(mipsIRQ)
