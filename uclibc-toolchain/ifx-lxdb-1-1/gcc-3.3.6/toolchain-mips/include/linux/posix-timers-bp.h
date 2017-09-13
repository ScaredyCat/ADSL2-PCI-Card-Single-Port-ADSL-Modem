#ifndef _linux_POSIX_TIMERS_BP_H
#define _linux_POSIX_TIMERS_BP_H
/*
 * include/linux/posix_timers-bp.h
 *
 *
 * 2003-7-7  Posix Clocks & timers 
 *                           by George Anzinger george@mvista.com
 *
 *			     Copyright (C) 2003 by MontaVista Software.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * MontaVista Software | 1237 East Arques Avenue | Sunnyvale | CA 94085 | USA 
 */
/*
 * This bit of code is to take care of back port issues.  We hope to
 * contain all the 2.5 vs 2.4 issues here so we can use pure 2.5 code
 * in posix-timers.c
 */
#include <linux/timex.h>
#define TICK_NSEC ((NSEC_PER_SEC +HZ/2) / HZ)

/* Suppose we want to devide two numbers NOM and DEN: NOM/DEN, the we can
 * improve accuracy by shifting LSH bits, hence calculating:
 *     (NOM << LSH) / DEN
 * This however means trouble for large NOM, because (NOM << LSH) may no
 * longer fit in 32 bits. The following way of calculating this gives us
 * some slack, under the following conditions:
 *   - (NOM / DEN) fits in (32 - LSH) bits.
 *   - (NOM % DEN) fits in (32 - LSH) bits.
 */
#define SH_DIV(NOM,DEN,LSH) (   ((NOM / DEN) << LSH)                    \
                             + (((NOM % DEN) << LSH) + DEN / 2) / DEN)

/*
 * We want to do realistic conversions of time so we need to use the same
 * values the update wall clock code uses as the jiffie size.  This value
 * is: TICK_NSEC (both of which are defined in timex.h).  This 
 * is a constant and is in nanoseconds.  We will used scaled math and
 * with a scales defined here as SEC_JIFFIE_SC,  USEC_JIFFIE_SC and 
 * NSEC_JIFFIE_SC.  Note that these defines contain nothing but
 * constants and so are computed at compile time.  SHIFT_HZ (computed in
 * timex.h) adjusts the scaling for different HZ values.
 */
#define SEC_JIFFIE_SC (30 - SHIFT_HZ)
#define NSEC_JIFFIE_SC (SEC_JIFFIE_SC + 30)
#define USEC_JIFFIE_SC (SEC_JIFFIE_SC + 20)
#define SEC_CONVERSION ((unsigned long)(((u64)NSEC_PER_SEC << SEC_JIFFIE_SC) /\
				(u64)TICK_NSEC))
#define NSEC_CONVERSION ((unsigned long)(((u64)1 << NSEC_JIFFIE_SC) /\
				(u64)TICK_NSEC))
#define USEC_CONVERSION ((unsigned long)(((u64)NSEC_PER_USEC << USEC_JIFFIE_SC)/\
				(u64)TICK_NSEC))
#if BITS_PER_LONG < 64
# define MAX_SEC_IN_JIFFIES \
	(long)((u64)((u64)MAX_JIFFY_OFFSET * TICK_NSEC) / NSEC_PER_SEC)
#else	/* take care of overflow on 64 bits machines */
# define MAX_SEC_IN_JIFFIES \
	(SH_DIV((MAX_JIFFY_OFFSET >> SEC_JIFFIE_SC) * TICK_NSEC, NSEC_PER_SEC, 1) - 1)
#endif

#define send_group_sig_info send_sig_info

#define read_seqbegin(lock) ({read_lock(lock); 0;})

#define read_seqretry(lock,seq) ({read_unlock(lock); 0;})

#define read_seqbegin_irqsave(lock, flags)	\
	({ local_irq_save(flags);   read_seqbegin(lock); })

#define read_seqretry_irqrestore(lock, iv, flags)			\
	({								\
		int ret = read_seqretry(lock, iv);			\
		local_irq_restore(flags);				\
		ret;							\
	})

extern rwlock_t xtime_lock;
extern u64 jiffies_64;

static inline u64 get_jiffies_64(void) 
{
	u64 jiff; 
	unsigned long flags;
	unsigned int seq; 
	do {
		seq = read_seqbegin_irqsave(&xtime_lock, flags);
		jiff = jiffies_64;				
	} while(read_seqretry_irqrestore(&xtime_lock, seq, flags));

	return jiff;
}

#define TIF_SIGPENDING 1
#define test_thread_flag(flag) (current->sigpending)


/* this is iffy, but the member name in struct timer_list has changed */
#define entry list 
#define __user

#define current_thread_info() current

static inline void finish_wait(wait_queue_head_t *q, wait_queue_t *wait)
{
	unsigned long flags;

	__set_current_state(TASK_RUNNING);
	if (!list_empty(&wait->task_list)) {
		spin_lock_irqsave(&q->lock, flags);
		list_del_init(&wait->task_list);
		spin_unlock_irqrestore(&q->lock, flags);
	}
}

extern long do_no_restart_syscall(struct restart_block *parm);

#endif
