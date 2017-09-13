/*
 * High-resolution timer header file for MIPS.
 * 
 * Copyright (C) 2003 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#ifndef _ASM_HRTIMER_H
#define _ASM_HRTIMER_H

#include <linux/config.h>
#include <linux/sc_math.h>

#if defined(CONFIG_SOME_WACKY_BOARD)
/*
 * [jsun] This is the place for specific board HRT hookups.
 * E.g.
 *   	#if defined(CONFIG_TI_XBOARD)
 *   	#include "asm/ti_xboard_hrtimers.h"
 */

#else	/* MIPS common case, CPU counter system timer */

#include <asm/mipsregs.h>
#include <asm/bitops.h>

/* [jsun] Default case for UP systems with CPU counter system timer. */

#if defined(CONFIG_SMP)
#error "High resolution timers not supported on SMP with MIPS common HRT!"
#endif


extern int schedule_hr_timer_int(unsigned long ref_jiffies, int cycles); 
extern int get_arch_cycles(unsigned long ref_jiffies);

extern unsigned long  cycles_per_jiffy;
#define	arch_cycles_per_jiffy	((int)cycles_per_jiffy)

extern int nsec_to_arch_cycle(int nsecs);
extern int arch_cycle_to_nsec(int cycles);

extern int hr_time_resolution;

extern int schedule_jiffies_int(unsigned long ref_jiffies);

#endif	/* MIPS common case, CPU counter system timer */

#endif /* _ASM_HRTIMER_H */
