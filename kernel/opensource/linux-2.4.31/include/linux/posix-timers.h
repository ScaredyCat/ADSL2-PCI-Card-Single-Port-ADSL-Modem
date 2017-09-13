#ifndef _linux_POSIX_TIMERS_H
#define _linux_POSIX_TIMERS_H

/*
 * include/linux/posix-timers.h
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
#include <linux/config.h>
#include <linux/hrtime.h>
#include <linux/posix-timers-bp.h>

struct k_clock {
	int res;		/* in nano seconds */
	int (*clock_set) (struct timespec * tp);
	int (*clock_get) (struct timespec * tp);
	int (*nsleep) (int flags,
		       struct timespec * new_setting,
		       struct itimerspec * old_setting);
	int (*timer_set) (struct k_itimer * timr, int flags,
			  struct itimerspec * new_setting,
			  struct itimerspec * old_setting);
	int (*timer_del) (struct k_itimer * timr);
	void (*timer_get) (struct k_itimer * timr,
			   struct itimerspec * cur_setting);
};

#ifdef CONFIG_HIGH_RES_TIMERS
struct now_struct {
	unsigned long jiffies;
	long sub_jiffie;
};

/*
 * The following locking assumes that irq off.
 */
static inline void
posix_get_now(struct now_struct *now)
{
	(now)->jiffies = jiffies;
	read_lock(&xtime_lock);
	(now)->sub_jiffie = get_arch_cycles((now)->jiffies);
	read_unlock(&xtime_lock);	
	while (unlikely(((now)->sub_jiffie - arch_cycles_per_jiffy) > 0)) {
		(now)->sub_jiffie = (now)->sub_jiffie - arch_cycles_per_jiffy;
		(now)->jiffies++;
	}
}

#define posix_time_before(timer, now) \
         ( {long diff = (long)(timer)->expires - (long)(now)->jiffies;  \
           (diff < 0) ||                                      \
	   ((diff == 0) && ((timer)->sub_expires < (now)->sub_jiffie)); })

#define posix_bump_timer(timr) do { \
          (timr)->it_timer.expires += (timr)->it_incr; \
          (timr)->it_timer.sub_expires += (timr)->it_sub_incr; \
          if (((timr)->it_timer.sub_expires - arch_cycles_per_jiffy) >= 0){ \
		  (timr)->it_timer.sub_expires -= arch_cycles_per_jiffy; \
		  (timr)->it_timer.expires++; \
	  }                                 \
          (timr)->it_overrun++;               \
        }while (0)

#else
struct now_struct {
	unsigned long jiffies;
};

#define posix_get_now(now) (now)->jiffies = jiffies;
#define posix_time_before(timer, now) \
                      time_before((timer)->expires, (now)->jiffies)

#define posix_bump_timer(timr) do { \
                        (timr)->it_timer.expires += (timr)->it_incr; \
                        (timr)->it_overrun++;               \
                       }while (0)
#endif				/* CONFIG_HIGH_RES_TIMERS */
#endif
