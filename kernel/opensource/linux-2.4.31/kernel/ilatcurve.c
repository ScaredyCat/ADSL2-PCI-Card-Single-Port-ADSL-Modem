/*
 * FILE NAME kernel/ilatcurve.c
 *
 * BRIEF MODULE DESCRIPTION
 * This file implements a sample based interrupt latency curve for Linux.
 *
 * Author: David Singleton
 *	MontaVista Software, Inc.
 *      support@mvista.com
 *
 * Copyright 2001 MontaVista Software Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <asm/system.h>
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/ilatency.h>
#include <asm/preem_latency.h>
#ifdef CONFIG_SMP
#include <linux/smp.h>
#endif

unsigned long bucketlog[BUCKETS];
unsigned long maximum_off = 0;
unsigned int ilatency_start = 0;
unsigned int ilatency_stop = 0;
unsigned int ilatency_delta = 0;
extern interrupt_latency_log_t latency;

unsigned int clock_diff(unsigned int start, unsigned int stop)
{
#ifdef CONFIG_CLOCK_COUNTS_DOWN
#ifdef CONFIG_24BIT_COUNTER
	if (stop < start) {
		stop = 0xffffff - stop; 
	}
#endif /* CONFIG_24BIT_COUNTER */
	return (start - stop);
#else	/* CONFIG_CLOCK_COUNTS_DOWN */
#ifdef CONFIG_24BIT_COUNTER
	if (start < stop) {
		start = 0xffffff - start; 
	}
#endif /* CONFIG_24BIT_COUNTER */
	return (stop - start);
#endif /* CLOCK_COUNTS_DOWN */
}

void  inline interrupt_overhead_start()
{

	if (latency.logging == OFF) {
		return;
	}
	readclock(ilatency_start);
}

void  inline interrupt_overhead_stop()
{
	unsigned int delta;

	if (latency.logging == OFF) {
		return;
	}
	readclock(ilatency_stop);
	delta = clock_to_usecs(clock_diff(ilatency_start, ilatency_stop));
	if (ilatency_delta < delta) {
		ilatency_delta = delta;
	}
}


unsigned long total_ilat_samples; /* total number of samples collected */

asmlinkage void inthoff_logentry(unsigned diff)
{
	unsigned sampletime = diff;
	sampletime /= BUCKET_SIZE;
	if (maximum_off < diff) {
		maximum_off = diff;
	}

	if (sampletime < 0) {
		bucketlog[0]++;
	} else if (sampletime > LAST_BUCKET) {
		bucketlog[LAST_BUCKET]++;
	} else {
		bucketlog[sampletime]++;
	}
	total_ilat_samples++;
	return;
}

/*
 * Writing to /proc/maxoff resets the counters
 */
static ssize_t 
ilatcurve_write_proc(struct file * file, const char * buf, size_t count,
loff_t *ppos)
{
	extern interrupt_latency_log_t latency;

	latency.logging = OFF;
	total_ilat_samples = 0;
	ilatency_delta = 0;
	maximum_off = 0;
        memset(&bucketlog, 0, sizeof(unsigned long) * BUCKETS);
        memset(&latency, 0, sizeof(interrupt_latency_log_t));
	latency.logging = ON;
	return count;
}

static int ilatcurve_read_proc(
	char *page_buffer,
	char **my_first_byte,
	off_t offset,
	int length,
	int *eof,
	void *data)
{
	int len = 0;
	char * const my_base = page_buffer;
	static int bucket = 0;

	if (offset == 0) {
		/* 
		 * Just been opened so display the header information 
		 */
		len += sprintf(my_base + len,
		    "#%lu samples logged\n#timer measured %u ticks per usec.\n"
		    "#worst case interrupt overhead %u microseconds\n"
		    "#maximum interrupt off time %lu microseconds\n"
		    "#usecs  samples", total_ilat_samples, 
		    ticks_per_usec, ilatency_delta, maximum_off);
		    bucket = 0;
	} else if (bucket == -1) {
		 *eof = 1;
		 return 0;
	}

	/* dump the sample log on the screen */
	for (bucket; bucket < BUCKETS && len < length; bucket++) {
		len += sprintf(my_base + len,
		    "\n%5u\t%8lu", (bucket + 1) * BUCKET_SIZE,
		    bucketlog[bucket]);
	}
	if (bucket >= BUCKETS) {
		len += sprintf(my_base + len, " (greater than this time)\n");
		bucket = -1;
	}
	*my_first_byte = page_buffer;
	return  len;
}

int  __init
ilatcurve_init(void)
{
	struct proc_dir_entry *entry;

	readclock_init();
#ifndef CONFIG_PPC
	ticks_per_usec = TICKS_PER_USEC;
#endif
#ifdef CONFIG_SMP
	printk("Interrupt holdoff times are not supported on SMP systems.\n");
#else
	printk("Interrupt holdoff times measurement enabled at %u ticks per usec.\n", ticks_per_usec);
	/*
	 * this is where each platform must turn the
	 * delta 'ticks' into microseconds.  Each architecture
	 * does it differently.
	 */
#ifdef CONFIG_PROC_FS
	entry = create_proc_read_entry("ilatcurve", S_IWUSR | S_IRUGO, NULL,
	    ilatcurve_read_proc, 0);
	if (entry) {
		entry->write_proc = (write_proc_t *)ilatcurve_write_proc;
	}

#endif
#endif
	return 0;
}

__initcall(ilatcurve_init);


