/*
 *  linux/kernel/timer.c
 *
 *  Kernel internal timers, kernel timekeeping, basic process system calls
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  1997-01-28  Modified by Finn Arne Gangstad to make timers scale better.
 *
 *  1997-09-10  Updated NTP code according to technical memorandum Jan '96
 *              "A Kernel Model for Precision Timekeeping" by Dave Mills
 *  1998-12-24  Fixed a xtime SMP race (we need the xtime_lock rw spinlock to
 *              serialize accesses to xtime/lost_ticks).
 *                              Copyright (C) 1998  Andrea Arcangeli
 *  1999-03-10  Improved NTP compatibility by Ulrich Windl
 *  2002-08-13  High res timers code by George Anzinger 
 *                  Copyright (C)2002 by MontaVista Software.

 */

#include <linux/config.h>
#include <linux/mm.h>
#include <linux/timex.h>
#include <linux/delay.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>

#include <linux/posix-timers-bp.h>

#include <linux/trace.h>

#define _INCLUDED_FROM_TIMER_C
#include <linux/hrtime.h>
#include <linux/compiler.h>
#include <asm/signal.h>
#include <asm/uaccess.h>

struct kernel_stat kstat;

/*
 * Timekeeping variables
 */

long tick = (1000000 + HZ/2) / HZ;	/* timer interrupt period */

/* 
 * The current time 
 * wall_to_monotonic is what we need to add to xtime (or xtime corrected 
 * for sub jiffie times) to get to monotonic time.  Monotonic is pegged at zero
 * at zero at system boot time, so wall_to_monotonic will be negative,
 * however, we will ALWAYS keep the tv_nsec part positive so we can use
 * the usual normalization.
 */
struct timeval xtime __attribute__ ((aligned (16)));
struct timespec wall_to_monotonic __attribute__ ((aligned (16)));

/* Don't completely fail for HZ > 500.  */
int tickadj = 500/HZ ? : 1;		/* microsecs */

DECLARE_TASK_QUEUE(tq_timer);
DECLARE_TASK_QUEUE(tq_immediate);

/*
 * phase-lock loop variables
 */
/* TIME_ERROR prevents overwriting the CMOS clock */
int time_state = TIME_OK;		/* clock synchronization status	*/
int time_status = STA_UNSYNC;		/* clock status bits		*/
long time_offset;			/* time adjustment (us)		*/
long time_constant = 2;			/* pll time constant		*/
long time_tolerance = MAXFREQ;		/* frequency tolerance (ppm)	*/
long time_precision = 1;		/* clock precision (us)		*/
long time_maxerror = NTP_PHASE_LIMIT;	/* maximum error (us)		*/
long time_esterror = NTP_PHASE_LIMIT;	/* estimated error (us)		*/
long time_phase;			/* phase offset (scaled us)	*/
long time_freq = ((1000000 + HZ/2) % HZ - HZ/2) << SHIFT_USEC;
					/* frequency offset (scaled ppm)*/
long time_adj;				/* tick adjust (scaled 1 / HZ)	*/
long time_reftime;			/* time at last adjustment (s)	*/

long time_adjust;
long time_adjust_step;

unsigned long event;

extern int do_setitimer(int, struct itimerval *, struct itimerval *);

/*
 * The 64-bit value is not volatile - you MUST NOT read it
 * without holding read_lock_irq(&xtime_lock).
 * jiffies is defined in the linker script...
 */
u64 jiffies_64;

unsigned int * prof_buffer;
unsigned long prof_len;
unsigned long prof_shift;

/*
 * This spinlock protect us from races in SMP while playing with xtime. -arca
 */
rwlock_t xtime_lock = RW_LOCK_UNLOCKED;

/*
 * Event timer code
 */
#ifdef CONFIG_HIGH_RES_TIMERS
/*
 * ifdef eliminator macro...
 */
#define IF_HIGH_RES(a) a
#else
#define IF_HIGH_RES(a)
#endif

#ifndef CONFIG_NEW_TIMER_LISTSIZE
#ifdef IFX_SMALL_FOOTPRINT
#define CONFIG_NEW_TIMER_LISTSIZE 128
#warning CONFIG_NEW_TIMER_LISTSIZE 128
#else
#define CONFIG_NEW_TIMER_LISTSIZE 512
#warning CONFIG_NEW_TIMER_LISTSIZE 512
#endif
#endif

#define NEW_TVEC_SIZE CONFIG_NEW_TIMER_LISTSIZE
#define NEW_TVEC_MASK (NEW_TVEC_SIZE - 1)

static struct list_head new_tvec[NEW_TVEC_SIZE];

void init_timervecs (void)
{
	int i;

        for (i = 0; i < NEW_TVEC_SIZE; i++)
                INIT_LIST_HEAD( new_tvec + i);
}

static unsigned long timer_jiffies;

/* Initialize both explicitly - let's try to have them in the same cache line */
spinlock_t timerlist_lock = SPIN_LOCK_UNLOCKED;

#if defined( CONFIG_SMP) ||  defined(CONFIG_HIGH_RES_TIMERS)
volatile struct timer_list * volatile running_timer;
#define timer_enter(t) do { running_timer = t; mb(); } while (0)
#define timer_exit() do { running_timer = NULL; } while (0)
#define timer_is_inactive() (running_timer == NULL)
#else
#define timer_enter(t)		do { } while (0)
#define timer_exit()		do { } while (0)
#define timer_is_inactive()     1
#endif

#ifdef CONFIG_SMP
#define timer_is_running(t) (running_timer == t)
#define timer_synchronize(t) while (timer_is_running(t)) barrier()
#endif



static inline unsigned long internal_add_timer(struct timer_list *timer)
{
	/*
	 * must be cli-ed when calling this
	 */
	unsigned long expires = timer->expires;
	IF_HIGH_RES(int sub_expires = timer->sub_expires;)
                int indx;
	struct list_head *pos,*list_start;

	if ( time_before(expires, timer_jiffies) ){
		/*
		 * already expired, schedule for next tick 
		 * would like to do better here
		 * Actually this now works just fine with the
		 * back up of timer_jiffies in "run_timer_list".
		 * Note that this puts the timer on a list other
		 * than the one it idexes to.  We don't want to
		 * change the expires value in the timer as it is
		 * used by the repeat code in setitimer and the
		 * POSIX timers code.
		 */
		expires = timer_jiffies;
		IF_HIGH_RES(sub_expires = 0);
	}
                        
	indx =  expires & NEW_TVEC_MASK;
	if ((expires - timer_jiffies) <= NEW_TVEC_SIZE) {
#ifdef CONFIG_HIGH_RES_TIMERS
		unsigned long jiffies_f;
		/*
		 * The high diff bits are the same, goes to the head of 
		 * the list, sort on sub_expire.
		 */
		for (pos = (list_start = &new_tvec[indx])->next; 
		     pos != list_start; 
		     pos = pos->next){
			struct timer_list *tmr = 
				list_entry(pos,
					   struct timer_list,
					   list);
			if ((tmr->sub_expires >= sub_expires) ||
			    (tmr->expires != expires)){
				break;
			}
		}
		list_add_tail(&timer->list, pos);
		/*
		 * Notes to me.  Use jiffies here instead of
		 * timer_jiffies to prevent adding unneeded interrupts.
		 * Timer_is_inactive() is false if we are currently
		 * activly dispatching timers.  Since we are under the
		 * same spin lock, it being false means that it has
		 * dropped the spinlock to call the timer function,
		 * which could well be who called us.  In any case, we
		 * don't need a new interrupt as the timer dispach code
		 * (run_timer_list) will pick this up when the function
		 * it is calling returns.
		 */
		if ( expires == (jiffies_f = jiffies) && 
		     list_start->next == &timer->list &&
		     timer_is_inactive()) 
			if (schedule_hr_timer_int(jiffies_f, sub_expires))
				mark_bh(TIMER_BH);
#else
		pos = (&new_tvec[indx])->next;
		list_add_tail(&timer->list, pos);
#endif
	}else{
		/*
		 * The high diff bits differ, search from the tail
		 * The for will pick up an empty list.
		 */
		for (pos = (list_start = &new_tvec[indx])->prev; 
		     pos != list_start; 
		     pos = pos->prev){
			struct timer_list *tmr = 
				list_entry(pos,
					   struct timer_list,
					   list);
			if (time_after(tmr->expires, expires)){
				continue;
			}
#ifdef CONFIG_HIGH_RES_TIMERS
			if ((tmr->expires != expires) ||
			    (tmr->sub_expires < sub_expires)) 
#endif
				break;
		}
		list_add(&timer->list, pos);
	}
	return 0;
}

void add_timer(struct timer_list *timer)
{
	unsigned long flags;

        spin_lock_irqsave(&timerlist_lock, flags);
	if (timer_pending(timer))
		goto bug;
	internal_add_timer(timer);
	spin_unlock_irqrestore(&timerlist_lock, flags);
	return;
bug:
	spin_unlock_irqrestore(&timerlist_lock, flags);
	printk("bug: kernel timer added twice at %p.\n",
			__builtin_return_address(0));
}

static inline int detach_timer (struct timer_list *timer)
{
	if (!timer_pending(timer))
		return 0;
	list_del(&timer->list);
	return 1;
}

#ifdef CONFIG_HIGH_RES_TIMERS
int mod_timer_hr(struct timer_list *timer, 
		 unsigned long expires, 
		 long sub_expires)
#else
int mod_timer(struct timer_list *timer, 
		 unsigned long expires)
#endif
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&timerlist_lock, flags);
	timer->expires = expires;
	ret = detach_timer(timer);
	IF_HIGH_RES(timer->sub_expires = sub_expires);
	internal_add_timer(timer);
	spin_unlock_irqrestore(&timerlist_lock, flags);
	return ret;
}

int del_timer(struct timer_list * timer)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&timerlist_lock, flags);
	ret = detach_timer(timer);
	timer->list.next = timer->list.prev = NULL;
	spin_unlock_irqrestore(&timerlist_lock, flags);
	return ret;
}

#ifdef CONFIG_HIGH_RES_TIMERS
int mod_timer(struct timer_list *timer, unsigned long expires)
{
	return mod_timer_hr(timer, expires, timer->sub_expires);
}
#endif

#ifdef CONFIG_SMP
void sync_timers(void)
{
	spin_unlock_wait(&global_bh_lock);
}

/*
 * SMP specific function to delete periodic timer.
 * Caller must disable by some means restarting the timer
 * for new. Upon exit the timer is not queued and handler is not running
 * on any CPU. It returns number of times, which timer was deleted
 * (for reference counting).
 */

int del_timer_sync(struct timer_list * timer)
{
	int ret = 0;

	for (;;) {
		unsigned long flags;
		int running;

		spin_lock_irqsave(&timerlist_lock, flags);
		ret += detach_timer(timer);
		timer->list.next = timer->list.prev = 0;
		running = timer_is_running(timer);
		spin_unlock_irqrestore(&timerlist_lock, flags);

		if (!running)
			break;

		timer_synchronize(timer);
	}

	return ret;
}
#endif



/*
 * run_timer_list is ALWAYS called from softirq which calls with irq enabled.
 * We may assume this and not save the flags.
 */

static inline void run_timer_list(void)
{
#ifdef CONFIG_HIGH_RES_TIMERS
	unsigned long jiffies_f;
	long sub_jiff;
	long sub_jiffie_f;
	spin_lock_irq(&timerlist_lock);
run_timer_list_again:
	read_lock(&xtime_lock);
	jiffies_f = jiffies;
	sub_jiffie_f = get_arch_cycles(jiffies_f);
	read_unlock(&xtime_lock);
	sub_jiff = 0;
/*
 * Lets not go beyond the current jiffies.  It tends to confuse some 
 * folks using short timers causing then to loop forever...
 * If sub_jiffie_f > cycles_per_jiffies we will just clean out all
 * timers at jiffies_f and quit.  We get the rest on the next go round.

	while ( unlikely(sub_jiffie_f >= arch_cycles_per_jiffy)){
		sub_jiffie_f -= arch_cycles_per_jiffy;
                jiffies_f++;
        }
*/
	while ((long)(jiffies_f - timer_jiffies) >= 0) 
#else
	spin_lock_irq(&timerlist_lock);
	while ((long)(jiffies - timer_jiffies) >= 0) 
#endif
	{
		struct list_head *head, *curr;
		head = new_tvec + (timer_jiffies & NEW_TVEC_MASK);
                /*
                 * Note that we never move "head" but continue to
                 * pick the first entry from it.  This allows new
                 * entries to be inserted while we unlock for the
                 * function call below.
                 */
repeat:
		curr = head->next;
		if (curr != head) {
			struct timer_list *timer;
			void (*fn)(unsigned long);
			unsigned long data;

			timer = list_entry(curr, struct timer_list, list);
#ifdef CONFIG_HIGH_RES_TIMERS
                        /*
                         * This would be simpler if we never got behind
                         * i.e. if timer_jiffies == jiffies, we could
                         * drop one of the tests.  Since we may get 
                         * behind, (in fact we don't up date until
                         * we are behind to allow sub_jiffie entries)
                         * we need a way to negate the sub_jiffie
                         * test in that case...
                         */
                        if (time_before(timer->expires, jiffies_f)||
                            ((timer->expires == jiffies_f) &&
                             timer->sub_expires <= sub_jiffie_f))
#else
                        if (time_before_eq(timer->expires, jiffies))
#endif
                                {fn = timer->function;
                                data= timer->data;
  
                                detach_timer(timer);
                                timer->list.next = timer->list.prev = NULL;
                                timer_enter(timer);
                                spin_unlock_irq(&timerlist_lock);
                                fn(data);
                                spin_lock_irq(&timerlist_lock);
                                timer_exit();
                                goto repeat;
				}
#ifdef CONFIG_HIGH_RES_TIMERS
                        else {
                                /*
                                 * The new timer list is not always emptied
                                 * here as it contains:
                                 * a.) entries (list size)^N*jiffies out and
                                 * b.) entries that match in jiffies, but have
                                 *     sub_expire times further out than now.
                                 */
                                 if (timer->expires == jiffies_f ){
					 sub_jiff = timer->sub_expires;
					 if (schedule_hr_timer_int(jiffies_f, 
								   sub_jiff))
						 /*
						  * If schedule_hr_timer_int
						  * says the time has
						  * passed we go round
						  * again
						  */
						 goto run_timer_list_again;
				 }
					 /*
					  * Otherwise, we need a jiffies
					  * interrupt next...
					  */

			}
#endif
	        }
		++timer_jiffies; 
	}
        /*
         * It is faster to back out the last bump, than to prevent it.
         * This allows zero time inserts as well as sub_jiffie values in
         * the current jiffie.
         */

        --timer_jiffies;

#ifdef CONFIG_HIGH_RES_TIMERS
	if (!sub_jiff && schedule_jiffies_int(jiffies_f)) {
		spin_unlock_irq(&timerlist_lock);
                /*
                 * If we are here, jiffies time has passed.  We should be
                 * safe waiting for jiffies to change.
                 */
                while ((volatile unsigned long)jiffies == jiffies_f) {
                        cpu_relax();
                }
		spin_lock_irq(&timerlist_lock);
		goto run_timer_list_again;
	}
#endif
        spin_unlock_irq(&timerlist_lock);
}

spinlock_t tqueue_lock = SPIN_LOCK_UNLOCKED;

void tqueue_bh(void)
{
	run_task_queue(&tq_timer);
}

void immediate_bh(void)
{
	run_task_queue(&tq_immediate);
}

/*
 * this routine handles the overflow of the microsecond field
 *
 * The tricky bits of code to handle the accurate clock support
 * were provided by Dave Mills (Mills@UDEL.EDU) of NTP fame.
 * They were originally developed for SUN and DEC kernels.
 * All the kudos should go to Dave for this stuff.
 *
 */
static void second_overflow(void)
{
    long ltemp;

    /* Bump the maxerror field */
    time_maxerror += time_tolerance >> SHIFT_USEC;
    if ( time_maxerror > NTP_PHASE_LIMIT ) {
	time_maxerror = NTP_PHASE_LIMIT;
	time_status |= STA_UNSYNC;
    }

    /*
     * Leap second processing. If in leap-insert state at
     * the end of the day, the system clock is set back one
     * second; if in leap-delete state, the system clock is
     * set ahead one second. The microtime() routine or
     * external clock driver will insure that reported time
     * is always monotonic. The ugly divides should be
     * replaced.
     */
    switch (time_state) {

    case TIME_OK:
	if (time_status & STA_INS)
	    time_state = TIME_INS;
	else if (time_status & STA_DEL)
	    time_state = TIME_DEL;
	break;

    case TIME_INS:
	if (xtime.tv_sec % 86400 == 0) {
	    xtime.tv_sec--;
	    wall_to_monotonic.tv_sec++;
	    time_state = TIME_OOP;
            clock_was_set();
	    printk(KERN_NOTICE "Clock: inserting leap second 23:59:60 UTC\n");
	}
	break;

    case TIME_DEL:
	if ((xtime.tv_sec + 1) % 86400 == 0) {
	    xtime.tv_sec++;
	    wall_to_monotonic.tv_sec--;
	    time_state = TIME_WAIT;
            clock_was_set();
	    printk(KERN_NOTICE "Clock: deleting leap second 23:59:59 UTC\n");
	}
	break;

    case TIME_OOP:
	time_state = TIME_WAIT;
	break;

    case TIME_WAIT:
	if (!(time_status & (STA_INS | STA_DEL)))
	    time_state = TIME_OK;
    }

    /*
     * Compute the phase adjustment for the next second. In
     * PLL mode, the offset is reduced by a fixed factor
     * times the time constant. In FLL mode the offset is
     * used directly. In either mode, the maximum phase
     * adjustment for each second is clamped so as to spread
     * the adjustment over not more than the number of
     * seconds between updates.
     */
    if (time_offset < 0) {
	ltemp = -time_offset;
	if (!(time_status & STA_FLL))
	    ltemp >>= SHIFT_KG + time_constant;
	if (ltemp > (MAXPHASE / MINSEC) << SHIFT_UPDATE)
	    ltemp = (MAXPHASE / MINSEC) << SHIFT_UPDATE;
	time_offset += ltemp;
	time_adj = -ltemp << (SHIFT_SCALE - SHIFT_HZ - SHIFT_UPDATE);
    } else {
	ltemp = time_offset;
	if (!(time_status & STA_FLL))
	    ltemp >>= SHIFT_KG + time_constant;
	if (ltemp > (MAXPHASE / MINSEC) << SHIFT_UPDATE)
	    ltemp = (MAXPHASE / MINSEC) << SHIFT_UPDATE;
	time_offset -= ltemp;
	time_adj = ltemp << (SHIFT_SCALE - SHIFT_HZ - SHIFT_UPDATE);
    }

    /*
     * Compute the frequency estimate and additional phase
     * adjustment due to frequency error for the next
     * second. When the PPS signal is engaged, gnaw on the
     * watchdog counter and update the frequency computed by
     * the pll and the PPS signal.
     */
    pps_valid++;
    if (pps_valid == PPS_VALID) {	/* PPS signal lost */
	pps_jitter = MAXTIME;
	pps_stabil = MAXFREQ;
	time_status &= ~(STA_PPSSIGNAL | STA_PPSJITTER |
			 STA_PPSWANDER | STA_PPSERROR);
    }
    ltemp = time_freq + pps_freq;
    if (ltemp < 0)
	time_adj -= -ltemp >>
	    (SHIFT_USEC + SHIFT_HZ - SHIFT_SCALE);
    else
	time_adj += ltemp >>
	    (SHIFT_USEC + SHIFT_HZ - SHIFT_SCALE);

#if HZ == 100
    /* Compensate for (HZ==100) != (1 << SHIFT_HZ).
     * Add 25% and 3.125% to get 128.125; => only 0.125% error (p. 14)
     */
    if (time_adj < 0)
	time_adj -= (-time_adj >> 2) + (-time_adj >> 5);
    else
	time_adj += (time_adj >> 2) + (time_adj >> 5);
#endif
}

/* in the NTP reference this is called "hardclock()" */
static void update_wall_time_one_tick(void)
{
	if ( (time_adjust_step = time_adjust) != 0 ) {
	    /* We are doing an adjtime thing. 
	     *
	     * Prepare time_adjust_step to be within bounds.
	     * Note that a positive time_adjust means we want the clock
	     * to run faster.
	     *
	     * Limit the amount of the step to be in the range
	     * -tickadj .. +tickadj
	     */
	     if (time_adjust > tickadj)
		time_adjust_step = tickadj;
	     else if (time_adjust < -tickadj)
		time_adjust_step = -tickadj;
	     
	    /* Reduce by this step the amount of time left  */
	    time_adjust -= time_adjust_step;
	}
	xtime.tv_usec += tick + time_adjust_step;
	/*
	 * Advance the phase, once it gets to one microsecond, then
	 * advance the tick more.
	 */
	time_phase += time_adj;
	if (time_phase <= -FINEUSEC) {
		long ltemp = -time_phase >> SHIFT_SCALE;
		time_phase += ltemp << SHIFT_SCALE;
		xtime.tv_usec -= ltemp;
	}
	else if (time_phase >= FINEUSEC) {
		long ltemp = time_phase >> SHIFT_SCALE;
		time_phase -= ltemp << SHIFT_SCALE;
		xtime.tv_usec += ltemp;
	}
}

/*
 * Using a loop looks inefficient, but "ticks" is
 * usually just one (we shouldn't be losing ticks,
 * we're doing this this way mainly for interrupt
 * latency reasons, not because we think we'll
 * have lots of lost timer ticks
 */
static void update_wall_time(unsigned long ticks)
{
	do {
		ticks--;
		update_wall_time_one_tick();
	} while (ticks);

	if (xtime.tv_usec >= 1000000) {
	    xtime.tv_usec -= 1000000;
	    xtime.tv_sec++;
	    second_overflow();
	}
}

static inline void do_process_times(struct task_struct *p,
	unsigned long user, unsigned long system)
{
	unsigned long psecs;

	psecs = (p->times.tms_utime += user);
	psecs += (p->times.tms_stime += system);
	if (psecs / HZ > p->rlim[RLIMIT_CPU].rlim_cur) {
		/* Send SIGXCPU every second.. */
		if (!(psecs % HZ))
			send_sig(SIGXCPU, p, 1);
		/* and SIGKILL when we go over max.. */
		if (psecs / HZ > p->rlim[RLIMIT_CPU].rlim_max)
			send_sig(SIGKILL, p, 1);
	}
}

static inline void do_it_virt(struct task_struct * p, unsigned long ticks)
{
	unsigned long it_virt = p->it_virt_value;

	if (it_virt) {
		it_virt -= ticks;
		if (!it_virt) {
			it_virt = p->it_virt_incr;
			send_sig(SIGVTALRM, p, 1);
		}
		p->it_virt_value = it_virt;
	}
}

static inline void do_it_prof(struct task_struct *p)
{
	unsigned long it_prof = p->it_prof_value;

	if (it_prof) {
		if (--it_prof == 0) {
			it_prof = p->it_prof_incr;
			send_sig(SIGPROF, p, 1);
		}
		p->it_prof_value = it_prof;
	}
}

void update_one_process(struct task_struct *p, unsigned long user,
			unsigned long system, int cpu)
{
	p->per_cpu_utime[cpu] += user;
	p->per_cpu_stime[cpu] += system;
	do_process_times(p, user, system);
	do_it_virt(p, user);
	do_it_prof(p);
}	

/*
 * Called from the timer interrupt handler to charge one tick to the current 
 * process.  user_tick is 1 if the tick is user time, 0 for system.
 */
void update_process_times(int user_tick)
{
	struct task_struct *p = current;
	int cpu = smp_processor_id(), system = user_tick ^ 1;

	update_one_process(p, user_tick, system, cpu);
	scheduler_tick(user_tick, system);
}

/*
 * Nr of active tasks - counted in fixed-point numbers
 */
static unsigned long count_active_tasks(void)
{
	return (nr_running() + nr_uninterruptible()) * FIXED_1;
}

/*
 * Hmm.. Changed this, as the GNU make sources (load.c) seems to
 * imply that avenrun[] is the standard name for this kind of thing.
 * Nothing else seems to be standardized: the fractional size etc
 * all seem to differ on different machines.
 */
unsigned long avenrun[3];

static inline void calc_load(unsigned long ticks)
{
	unsigned long active_tasks; /* fixed-point */
	static int count = LOAD_FREQ;

	count -= ticks;
	if (count < 0) {
		count += LOAD_FREQ;
		active_tasks = count_active_tasks();
		CALC_LOAD(avenrun[0], EXP_1, active_tasks);
		CALC_LOAD(avenrun[1], EXP_5, active_tasks);
		CALC_LOAD(avenrun[2], EXP_15, active_tasks);
	}
}

/* jiffies at the most recent update of wall time */
unsigned long wall_jiffies;


static inline unsigned long update_times(void)
{
	unsigned long ticks;

	/*
	 * update_times() is run from the raw timer_bh handler so we
	 * just know that the irqs are locally enabled and so we don't
	 * need to save/restore the flags of the local CPU here. -arca
	 */
	write_lock_irq(&xtime_lock);
	vxtime_lock();

	ticks = jiffies - wall_jiffies;
	if (ticks) {
		wall_jiffies += ticks;
		update_wall_time(ticks);
	}
	vxtime_unlock();
	write_unlock_irq(&xtime_lock);
	calc_load(ticks);
}

#ifdef CONFIG_HIGH_RES_TIMERS
void update_real_wall_time(void)
{
 	unsigned long ticks;
       /*
         * To get the time of day really right, we need to make sure 
         * every one is on the same jiffie. (Because of adj_time, etc.)
         * So we provide this for the high res code.  Must be called 
         * under the write(xtime_lock).  (External locking allows the
         * caller to include sub jiffies in the lock region.)
         */
 	ticks = jiffies - wall_jiffies;
	if (ticks) {
		wall_jiffies += ticks;
		update_wall_time(ticks);
	}
}
#endif
void timer_bh(void)
{
	TRACE_EVENT(TRACE_EV_KERNEL_TIMER, NULL);
	update_times();
	run_timer_list();
}

void do_timer(struct pt_regs *regs)
{
   	  (*(u64 *)&jiffies_64)++;
//        (*(unsigned long *)&jiffies)++;
#ifndef CONFIG_SMP
	/* SMP process accounting uses the local APIC timer */

	update_process_times(user_mode(regs));
#endif
	mark_bh(TIMER_BH);
	if (TQ_ACTIVE(tq_timer))
		mark_bh(TQUEUE_BH);
}

#if !defined(__alpha__) && !defined(__ia64__)

/*
 * For backwards compatibility?  This can be done in libc so Alpha
 * and all newer ports shouldn't need it.
 */
asmlinkage unsigned long sys_alarm(unsigned int seconds)
{
	struct itimerval it_new, it_old;
	unsigned int oldalarm;

	it_new.it_interval.tv_sec = it_new.it_interval.tv_usec = 0;
	it_new.it_value.tv_sec = seconds;
	it_new.it_value.tv_usec = 0;
	do_setitimer(ITIMER_REAL, &it_new, &it_old);
	oldalarm = it_old.it_value.tv_sec;
	/* ehhh.. We can't return 0 if we have an alarm pending.. */
	/* And we'd better return too much than too little anyway */
	if (it_old.it_value.tv_usec)
		oldalarm++;
	return oldalarm;
}

#endif

#ifndef __alpha__

/*
 * The Alpha uses getxpid, getxuid, and getxgid instead.  Maybe this
 * should be moved into arch/i386 instead?
 */

/**
 * sys_getpid - return the thread group id of the current process
 *
 * Note, despite the name, this returns the tgid not the pid.  The tgid and
 * the pid are identical unless CLONE_THREAD was specified on clone() in
 * which case the tgid is the same in all threads of the same group.
 *
 * This is SMP safe as current->tgid does not change.
 */
asmlinkage long sys_getpid(void)
{
	return current->tgid;
}

/*
 * This is not strictly SMP safe: p_opptr could change
 * from under us. However, rather than getting any lock
 * we can use an optimistic algorithm: get the parent
 * pid, and go back and check that the parent is still
 * the same. If it has changed (which is extremely unlikely
 * indeed), we just try again..
 *
 * NOTE! This depends on the fact that even if we _do_
 * get an old value of "parent", we can happily dereference
 * the pointer: we just can't necessarily trust the result
 * until we know that the parent pointer is valid.
 *
 * The "mb()" macro is a memory barrier - a synchronizing
 * event. It also makes sure that gcc doesn't optimize
 * away the necessary memory references.. The barrier doesn't
 * have to have all that strong semantics: on x86 we don't
 * really require a synchronizing instruction, for example.
 * The barrier is more important for code generation than
 * for any real memory ordering semantics (even if there is
 * a small window for a race, using the old pointer is
 * harmless for a while).
 */
asmlinkage long sys_getppid(void)
{
	int pid;
	struct task_struct * me = current;
	struct task_struct * parent;

	parent = me->p_opptr;
	for (;;) {
		pid = parent->pid;
#if CONFIG_SMP
{
		struct task_struct *old = parent;
		mb();
		parent = me->p_opptr;
		if (old != parent)
			continue;
}
#endif
		break;
	}
	return pid;
}

asmlinkage long sys_getuid(void)
{
	/* Only we change this so SMP safe */
	return current->uid;
}

asmlinkage long sys_geteuid(void)
{
	/* Only we change this so SMP safe */
	return current->euid;
}

asmlinkage long sys_getgid(void)
{
	/* Only we change this so SMP safe */
	return current->gid;
}

asmlinkage long sys_getegid(void)
{
	/* Only we change this so SMP safe */
	return  current->egid;
}

#endif

static void process_timeout(unsigned long __data)
{
        TRACE_TIMER(TRACE_EV_TIMER_EXPIRED, 0, 0, 0);
	wake_up_process((task_t *)__data);
}

/**
 * schedule_timeout - sleep until timeout
 * @timeout: timeout value in jiffies
 *
 * Make the current task sleep until @timeout jiffies have
 * elapsed. The routine will return immediately unless
 * the current task state has been set (see set_current_state()).
 *
 * You can set the task state as follows -
 *
 * %TASK_UNINTERRUPTIBLE - at least @timeout jiffies are guaranteed to
 * pass before the routine returns. The routine will return 0
 *
 * %TASK_INTERRUPTIBLE - the routine may return early if a signal is
 * delivered to the current task. In this case the remaining time
 * in jiffies will be returned, or 0 if the timer expired in time
 *
 * The current task state is guaranteed to be TASK_RUNNING when this 
 * routine returns.
 *
 * Specifying a @timeout value of %MAX_SCHEDULE_TIMEOUT will schedule
 * the CPU away without a bound on the timeout. In this case the return
 * value will be %MAX_SCHEDULE_TIMEOUT.
 *
 * In all cases the return value is guaranteed to be non-negative.
 */
signed long schedule_timeout(signed long timeout)
{
	struct timer_list timer;
	unsigned long expire;

	switch (timeout)
	{
	case MAX_SCHEDULE_TIMEOUT:
		/*
		 * These two special cases are useful to be comfortable
		 * in the caller. Nothing more. We could take
		 * MAX_SCHEDULE_TIMEOUT from one of the negative value
		 * but I' d like to return a valid offset (>=0) to allow
		 * the caller to do everything it want with the retval.
		 */
		schedule();
		goto out;
	default:
		/*
		 * Another bit of PARANOID. Note that the retval will be
		 * 0 since no piece of kernel is supposed to do a check
		 * for a negative retval of schedule_timeout() (since it
		 * should never happens anyway). You just have the printk()
		 * that will tell you if something is gone wrong and where.
		 */
		if (timeout < 0)
		{
			printk(KERN_ERR "schedule_timeout: wrong timeout "
			       "value %lx from %p\n", timeout,
			       __builtin_return_address(0));
			current->state = TASK_RUNNING;
			goto out;
		}
	}

        TRACE_TIMER(TRACE_EV_TIMER_SETTIMEOUT, 0, timeout, 0);

	expire = timeout + jiffies;

	init_timer(&timer);
	timer.expires = expire;
	timer.data = (unsigned long) current;
	timer.function = process_timeout;

	add_timer(&timer);
	schedule();
	del_timer_sync(&timer);

	timeout = expire - jiffies;

 out:
	return timeout < 0 ? 0 : timeout;
}

/* Thread ID - the internal kernel "pid" */
asmlinkage long sys_gettid(void)
{
	return current->pid;
}

#ifndef FOLD_NANO_SLEEP_INTO_CLOCK_NANO_SLEEP

static long nanosleep_restart(struct restart_block *restart)
{
	unsigned long expire = restart->arg0, now = jiffies;
	struct timespec *rmtp = (struct timespec *) restart->arg1;
	long ret;

	/* Did it expire while we handled signals? */
	if (!time_after(expire, now))
		return 0;

	current->state = TASK_INTERRUPTIBLE;
	expire = schedule_timeout(expire - now);

	ret = 0;
	if (expire) {
		struct timespec t;
		jiffies_to_timespec(expire, &t);

		ret = -ERESTART_RESTARTBLOCK;
		if (rmtp && copy_to_user(rmtp, &t, sizeof(t)))
			ret = -EFAULT;
		/* The 'restart' block is already filled in */
	}
	return ret;
}

asmlinkage long sys_nanosleep(struct timespec *rqtp, struct timespec *rmtp)
{
	struct timespec t;
	unsigned long expire;
	long ret;

	if (copy_from_user(&t, rqtp, sizeof(t)))
		return -EFAULT;

	if ((t.tv_nsec >= 1000000000L) || (t.tv_nsec < 0) || (t.tv_sec < 0))
		return -EINVAL;

	expire = timespec_to_jiffies(&t) + (t.tv_sec || t.tv_nsec);
	current->state = TASK_INTERRUPTIBLE;
	expire = schedule_timeout(expire);

	ret = 0;
	if (expire) {
		struct restart_block *restart;
		jiffies_to_timespec(expire, &t);
		if (rmtp && copy_to_user(rmtp, &t, sizeof(t)))
			return -EFAULT;

		restart = &current_thread_info()->restart_block;
		restart->fn = nanosleep_restart;
		restart->arg0 = jiffies + expire;
		restart->arg1 = (unsigned long) rmtp;
		ret = -ERESTART_RESTARTBLOCK;
	}
	return ret;
}
#endif // ! FOLD_NANO_SLEEP_INTO_CLOCK_NANO_SLEEP

