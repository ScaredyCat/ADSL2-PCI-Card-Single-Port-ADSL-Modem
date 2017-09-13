/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
//-----------------------------------------------------------------------
//Description:	
// DANUBE specific setup
//-----------------------------------------------------------------------
//Author:	peng.liu@infineon.com
//Created:	9-April-2004
//-----------------------------------------------------------------------
/* History
 * Last changed on: 22-Feb-2005
 * Last changed by: peng.liu@infineon.com
 * Last changed for: PCI MB & ADSL link down notification
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <asm/time.h>

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/danube/model.h>
#include <asm/danube/serial.h>
#include <asm/danube/emulation.h>
#include <asm/danube/danube_mei.h>
#include <asm/mipsregs.h>

static unsigned int r4k_offset; /* Amount to increment compare reg each time */
static unsigned int r4k_cur;    /* What counter should be at next timer irq */

extern void danube_reboot_setup(void); 
extern int kgdb_serial_init(void);
void prom_printf(const char * fmt, ...);
void __init bus_error_init(void) { /* nothing */ }

int (*adsl_link_notify)(int);

/* modify danube register with a small delay after writing to the register */
/* this delay is located in the same cache line as the write access, to prevent */
/* erroneous updates of the caches (seen with CGU_DIVCR register !) */
/* Input: reg = address of register */
/*        andmask = bits to clear in the register */
/*        ormask  = bits to set in the register */
static inline unsigned long modify_danube_reg(volatile unsigned int *reg, 
                                            unsigned long andmask, 
                                            unsigned long ormask)
{
    int count=15000;
    unsigned long regValue = 0;

    /* equivalent C-Code: (*reg) = ((*reg) & (~andmask)) | (ormask); */
    asm volatile (" .set noreorder\n"
                  "\tlw\t%2,(%3)\n"
                  "\tand\t%2,%2,%5\n"
                  "\tor\t%2,%2,%6\n"
                  "\t.align\t4\n" /* align on cache line boundary */
                  "\tsw\t%2,(%3)\n"
                  "\t0:\n"
                  "\tbnez\t%4,0b\n"
                  "\taddiu\t%4,%4,-1\n" /* branch delay */
                  "\t.set\treorder\n"
                  :
                  "=r" (regValue), /* %0 - output */
                  "=r" (count)  /* %1 - count is modified */
                  :
                  "0" (regValue),    /* %2 - same register as %0 */
                  "r" (reg),    /* %3 - address to modify */
                  "1" (count),  /* %4 - number of delay cycles */
                  "r" (~andmask), /* %5 - value to 'and' with on-chip register */
                  "r" (ormask)  /* %6 - value to 'or' with on-chip register */
/*                    : */
                  /* nothing clobbered */
        );

    return regValue;
}

unsigned int danube_get_ddr_hz(void)
{
	switch((*DANUBE_CGU_SYS) & 0x3){
		case 0:
			return CLOCK_167M;
		case 1:
			return CLOCK_133M;
		case 2:
			return CLOCK_111M;
	}
  return CLOCK_83M;
}
/* the CPU clock rate - lifted from u-boot */
unsigned int
danube_get_cpu_hz(void)
{
#ifdef CONFIG_USE_EMULATOR
	return EMULATOR_CPU_SPEED;
#else //NOT CONFIG_USE_EMULATOR
	unsigned int ddr_clock=danube_get_ddr_hz();
	switch((*DANUBE_CGU_SYS) & 0xc){
		case 0:
			return CLOCK_333M;
		case 4:
			return ddr_clock;
    case 0xc:
      /*reserved: TODO */
	}
	return ddr_clock << 1;
#endif	
}

/* the FPI clock rate - lifted from u-boot */
unsigned int
danube_get_fpi_hz(void)
{

#ifdef CONFIG_USE_EMULATOR
	unsigned int  clkCPU;
	clkCPU = danube_get_cpu_hz();
	return clkCPU >> 2;
#else //NOT CONFIG_USE_EMULATOR
	unsigned int ddr_clock=danube_get_ddr_hz();
	if ((*DANUBE_CGU_SYS) & 0x40){
		return ddr_clock >> 1;
	}
	return ddr_clock;
#endif
}

/* get the CPU version number  - based on sysLib.c from VxWorks sources */
/* this doesn't really belong here, but it's a convenient location */
unsigned int
danube_get_cpu_ver(void)
{
	static unsigned int cpu_ver = 0;

	if (cpu_ver == 0)
		cpu_ver = *DANUBE_MCD_CHIPID & 0xFFFFF000;
	return cpu_ver;
}



EXPORT_SYMBOL(danube_get_cpu_hz);
EXPORT_SYMBOL(danube_get_fpi_hz);
EXPORT_SYMBOL(danube_get_cpu_ver);

void danube_time_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,31) 
	#ifdef CONFIG_USE_EMULATOR
        //we do some hacking here to give illusion we have a faster counter frequency so that
        // the time interrupt happends less frequently
        mips_hpt_frequency = danube_get_cpu_hz()/2 * 25;
	#else //not CONFIG_USE_EMULATOR
        mips_hpt_frequency = danube_get_cpu_hz()/2;
	#endif //not CONFIG_USE_EMULATOR
        r4k_offset = mips_hpt_frequency/HZ;
        printk("mips_hpt_frequency:%d\n",mips_hpt_frequency);
        printk("r4k_offset: %08x(%d)\n",r4k_offset,r4k_offset);
        printk("CGU= %x\n",*DANUBE_CGU_SYS);
#else
	#ifdef CONFIG_USE_EMULATOR
	//we do some hacking here to give illusion we have a faster counter frequency so that
	// the time interrupt happends less frequently
	mips_counter_frequency = danube_get_cpu_hz()/2 * 25;
	#else //not CONFIG_USE_EMULATOR
	mips_counter_frequency = danube_get_cpu_hz()/2;
	#endif //not CONFIG_USE_EMULATOR
	r4k_offset = mips_counter_frequency/HZ;
	printk("mips_counter_frequency:%d\n",mips_counter_frequency);
	printk("r4k_offset: %08x(%d)\n",r4k_offset,r4k_offset);
	printk("CGU= %x\n",*DANUBE_CGU_SYS);
#endif /* kernel version */
}




#ifdef CONFIG_HIGH_RES_TIMERS
extern int hr_time_resolution;

/* ISR GPTU Timer 6 for high resolution timer */
void danube_timer6_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	timer_interrupt(DANUBE_TIMER6_INT, NULL, regs);
}

static struct irqaction hrt_irqaction = {
	danube_timer6_interrupt,
	SA_INTERRUPT,
	0,
	"hrt",
	NULL,
	NULL
};

#endif //CONFIG_HIGH_RES_TIMERS

/*
 * THe CPU counter for System timer, set to HZ
 * GPTU Timer 6 for high resolution timer, set to hr_time_resolution
 * Also misuse this routine to print out the CPU type and clock.
 */
void danube_timer_setup(struct irqaction *irq)
{
	/* cpu counter for timer interrupts */
	irq->handler = no_action;     /* we use our own handler */
	setup_irq(MIPS_CPU_TIMER_IRQ, irq);
	 /* to generate the first timer interrupt */
	r4k_cur = (read_c0_count() + r4k_offset);
	write_c0_compare(r4k_cur);

#ifdef CONFIG_HIGH_RES_TIMERS	
	/* GPTU timer 6 */
	int retval;
	if ( hr_time_resolution > 200000000 || hr_time_resolution < 40) {
		prom_printf("hr_time_resolution is out of range, HIGH_RES_TIMER is diabled.\n");
		return;
	}
	
	/* enable the timer in the PMU */
        *(DANUBE_PMU_PWDCR) = (*(DANUBE_PMU_PWDCR))| DANUBE_PMU_PWDCR_GPT|DANUBE_PMU_PWDCR_FPI;
	/* setup the GPTU for timer tick  f_fpi == f_gptu*/
	*(DANUBE_GPTU_CLC) = 0x100;

#ifdef CONFIG_USE_EMULATOR
	//reload value = fpi/(HZ * P), timer mode, Prescaler = 4 ( T6I = 000, T6BPS2 = 0)	
	*(DANUBE_GPTU_CAPREL) = (danube_get_fpi_hz()*hr_time_resolution/1000000000)>>2;
	*(DANUBE_GPTU_T6CON) = 0x80C0;
#else //not CONFIG_USE_EMULATOR
	*(DANUBE_GPTU_CAPREL) = 0xffff;
	*(DANUBE_GPTU_T6CON) = 0x80C0;
#endif //not CONFIG_USE_EMULATOR
	retval = setup_irq(DANUBE_TIMER6_INT,&hrt_irqaction);
	if (retval){
		prom_printf("reqeust_irq failed %d. HIGH_RES_TIMER is diabled\n",DANUBE_TIMER6_INT);		
	}
#endif //CONFIG_HIGH_RES_TIMERS		

}

/* this gets called straight from danubeIRQ.S */
asmlinkage void danube_timer_interrupt(struct pt_regs *regs)
{
	timer_interrupt(MIPS_CPU_TIMER_IRQ, NULL, regs);
}

static char buf[1024];

void prom_printf(const char * fmt, ...)
{
	va_list args;
	int l;
	char *p, *buf_end;

	/* Low level, brute force, not SMP safe... */
	va_start(args, fmt);
	l = vsprintf(buf, fmt, args); /* hopefully i < sizeof(buf) */
	va_end(args);
	buf_end = buf + l;
	
	for (p = buf; p < buf_end; p++) {
		/* Wait for FIFO to empty */
#ifdef CONFIG_IFX_ASC_CONSOLE_ASC0
		while ((((*DANUBE_ASC0_FSTAT)& ASCFSTAT_TXFFLMASK) >> ASCFSTAT_TXFFLOFF) != 0x00) ; 
		/* Crude cr/nl handling is better than none */
		if(*p == '\n') *DANUBE_ASC0_TBUF=('\r');
		*DANUBE_ASC0_TBUF=(*p);
#else
		while ((((*DANUBE_ASC1_FSTAT)& ASCFSTAT_TXFFLMASK) >> ASCFSTAT_TXFFLOFF) != 0x00) ; 
		/* Crude cr/nl handling is better than none */
		if(*p == '\n') *DANUBE_ASC1_TBUF=('\r');
		*DANUBE_ASC1_TBUF=(*p);
#endif
	}

}

void __init danube_setup(void)
{	

	u32 status;
	//TODO
	prom_printf("TODO: chip version\n");	
	/* clear RE bit*/
	status = read_c0_status();
	status &= (~(1<<25));
	write_c0_status(status);

	danube_reboot_setup();
	board_time_init = danube_time_init;
	board_timer_setup = danube_timer_setup;
#ifdef CONFIG_KGDB
	kgdb_serial_init();
	prom_printf("\n===>Please connect GDB to console tty0\n");
#endif
}
