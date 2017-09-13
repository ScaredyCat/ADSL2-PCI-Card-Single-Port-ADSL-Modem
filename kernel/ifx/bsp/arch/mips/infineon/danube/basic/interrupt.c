/*
 *  Gary Jennejohn (C) 2003 <gj@denx.de>
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
 * Routines for generic manipulation of the interrupts found on the 
 * DANUBE boards.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>

#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/traps.h>
#ifdef CONFIG_KGDB
#include <asm/gdb-stub.h>
#endif

extern asmlinkage void mipsIRQ(void);
#if 0
extern void do_IRQ(int irq, struct pt_regs *regs);
#endif
#undef DANUBE_INT_DEBUG_MSG
#ifdef DANUBE_INT_DEBUG_MSG
#define DANUBE_INT_DMSG(fmt,args...) printk("%s: " fmt, __FUNCTION__ , ##args)
#else
#define DANUBE_INT_DMSG(x...)
#endif 

void disable_danube_irq(unsigned int irq_nr)
{
  DANUBE_INT_DMSG("disable_danube_irq: %d\n", irq_nr);
  /* have to access the correct register here */
  if (irq_nr <= INT_NUM_IM0_IRL31 && irq_nr >= INT_NUM_IM0_IRL0)
  {
    /* access IM0 */
    *DANUBE_ICU_IM0_IER &= (~DANUBE_ICU_IM0_IER_IR(irq_nr));

  }else if (irq_nr <= INT_NUM_IM1_IRL31 && irq_nr >= INT_NUM_IM1_IRL0)
  {
    /* access IM1 */
    *DANUBE_ICU_IM1_IER &= (~DANUBE_ICU_IM1_IER_IR(irq_nr - 32));
  }else if (irq_nr <= INT_NUM_IM2_IRL31 && irq_nr >= INT_NUM_IM2_IRL0)
  {
    /* access IM2 */
    *DANUBE_ICU_IM2_IER &= (~DANUBE_ICU_IM2_IER_IR(irq_nr - 64));
  }else if (irq_nr <= INT_NUM_IM3_IRL31 && irq_nr >= INT_NUM_IM3_IRL0)
  {
    /* access IM3 */
    *DANUBE_ICU_IM3_IER &= (~DANUBE_ICU_IM3_IER_IR((irq_nr - 96)));
  }else if (irq_nr <= INT_NUM_IM4_IRL31 && irq_nr >= INT_NUM_IM4_IRL0)
  {
    /* access IM4 */
    *DANUBE_ICU_IM4_IER &= (~DANUBE_ICU_IM4_IER_IR((irq_nr - 128)));
  }

}
EXPORT_SYMBOL(disable_danube_irq);
void mask_and_ack_danube_irq(unsigned int irq_nr)
{
  DANUBE_INT_DMSG("mask_and_ack_danube_irq: %d\n", irq_nr);
  /* have to access the correct register here */
  if (irq_nr <= INT_NUM_IM0_IRL31 && irq_nr >= INT_NUM_IM0_IRL0)
  {
    /* access IM0 */
    /* mask */
    *DANUBE_ICU_IM0_IER &= ~DANUBE_ICU_IM0_IER_IR(irq_nr);
    /* ack */
    *DANUBE_ICU_IM0_ISR = DANUBE_ICU_IM0_ISR_IR(irq_nr);
  }else if (irq_nr <= INT_NUM_IM1_IRL31 && irq_nr >= INT_NUM_IM1_IRL0)
  {
    /* access IM1 */
    /* mask */
    *DANUBE_ICU_IM1_IER &= ~DANUBE_ICU_IM1_IER_IR(irq_nr - 32);
    /* ack */
    *DANUBE_ICU_IM1_ISR = DANUBE_ICU_IM1_ISR_IR(irq_nr - 32);
  }else if (irq_nr <= INT_NUM_IM2_IRL31 && irq_nr >= INT_NUM_IM2_IRL0)
  {
    /* access IM2 */
    /* mask */
    *DANUBE_ICU_IM2_IER &= ~DANUBE_ICU_IM2_IER_IR(irq_nr - 64);
    /* ack */
    *DANUBE_ICU_IM2_ISR = DANUBE_ICU_IM2_ISR_IR(irq_nr - 64);
  }else if (irq_nr <= INT_NUM_IM3_IRL31 && irq_nr >= INT_NUM_IM3_IRL0)
  {
    /* access IM3 */
    /* mask */
    *DANUBE_ICU_IM3_IER &= ~DANUBE_ICU_IM3_IER_IR(irq_nr - 96);
    /* ack */
    *DANUBE_ICU_IM3_ISR = DANUBE_ICU_IM3_ISR_IR(irq_nr - 96);
  }else if (irq_nr <= INT_NUM_IM4_IRL31 && irq_nr >= INT_NUM_IM4_IRL0)
  {
    /* access IM4 */
    /* mask */		
    *DANUBE_ICU_IM4_IER &= ~DANUBE_ICU_IM4_IER_IR(irq_nr - 128);
    /* ack */
    *DANUBE_ICU_IM4_ISR = DANUBE_ICU_IM4_ISR_IR(irq_nr - 128);
  }
}
EXPORT_SYMBOL(mask_and_ack_danube_irq);
void enable_danube_irq(unsigned int irq_nr)
{
  DANUBE_INT_DMSG("enable_danube_irq: %d\n", irq_nr);
  /* have to access the correct register here */
  if (irq_nr <= INT_NUM_IM0_IRL31 && irq_nr >= INT_NUM_IM0_IRL0){
    /* access IM0 */
    *DANUBE_ICU_IM0_IER |= DANUBE_ICU_IM0_IER_IR(irq_nr);
  }else if (irq_nr <= INT_NUM_IM1_IRL31 && irq_nr >= INT_NUM_IM1_IRL0)
  {
    /* access IM1 */
    *DANUBE_ICU_IM1_IER |= DANUBE_ICU_IM1_IER_IR(irq_nr - 32);
  }else if (irq_nr <= INT_NUM_IM2_IRL31 && irq_nr >= INT_NUM_IM2_IRL0)
  {
    /* access IM2 */
    *DANUBE_ICU_IM2_IER |= DANUBE_ICU_IM2_IER_IR(irq_nr - 64);
  }else if (irq_nr <= INT_NUM_IM3_IRL31 && irq_nr >= INT_NUM_IM3_IRL0)
  {
    /* access IM3 */
    *DANUBE_ICU_IM3_IER |= DANUBE_ICU_IM3_IER_IR((irq_nr - 96));
  }else if (irq_nr <= INT_NUM_IM4_IRL31 && irq_nr >= INT_NUM_IM4_IRL0)
  {
    /* access IM4 */
    *DANUBE_ICU_IM4_IER |= DANUBE_ICU_IM4_IER_IR((irq_nr - 128));
  }

}
EXPORT_SYMBOL(enable_danube_irq);

static unsigned int startup_danube_irq(unsigned int irq)
{
	enable_danube_irq(irq);
	return 0; /* never anything pending */
}

#define shutdown_danube_irq	disable_danube_irq

static void end_danube_irq(unsigned int irq)
{
  if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS))){
    enable_danube_irq(irq);
  }
}

static struct hw_interrupt_type danube_irq_type = {
	"DANUBE",
	startup_danube_irq,
	shutdown_danube_irq,
	enable_danube_irq,
	disable_danube_irq,
	mask_and_ack_danube_irq,
	end_danube_irq,
	NULL
};

#define IVEC_BUG_FIX
#ifdef IVEC_BUG_FIX
/**
 * Find least significant bit.
 * This function searches for the least significant bit in 32bit value.
 *
 * \param  x - The value to be checked.
 * \return Position of the least significant bit (0..31)
 */
static inline int ls1bit32(unsigned int x)
{
  int b = 31, s;

  s = 16; if (x << 16 == 0) s = 0; b -= s; x <<= s;
  s =  8; if (x <<  8 == 0) s = 0; b -= s; x <<= s;
  s =  4; if (x <<  4 == 0) s = 0; b -= s; x <<= s;
  s =  2; if (x <<  2 == 0) s = 0; b -= s; x <<= s;
  s =  1; if (x <<  1 == 0) s = 0; b -= s;

  return b;
}
#endif /* IVEC_BUG_FIX */
/* Cascaded interrupts from IM0 */
void danube_hw0_irqdispatch(struct pt_regs *regs)
{
  u32 irq;

#ifdef IVEC_BUG_FIX
  irq = (*DANUBE_ICU_IM0_IOSR); 
  /* if int_status == 0, then the interrupt has already been cleared */
  if (irq == 0) {
    return;
  }
  irq = ls1bit32(irq);
#else
  irq = (*DANUBE_ICU_IM_VEC) & DANUBE_ICU_IM0_VEC_MASK;
  if (irq ==0) return;
  irq--;
#endif /*IVEC_BUG_FIX*/
  if (irq == 22){
    /*clear EBU interrupt */
    *(DANUBE_EBU_PCC_ISTAT) |= 0x10;
  }
  DANUBE_INT_DMSG("danube_hw0_irqdispatch: irq=%d\n", irq);

  do_IRQ((int)irq, regs);

  return;
}

/* Cascaded interrupts from IM1 */
void danube_hw1_irqdispatch(struct pt_regs *regs)
{
  u32 irq;

#ifdef IVEC_BUG_FIX
  irq = (*DANUBE_ICU_IM1_IOSR); 
  /* if int_status == 0, then the interrupt has already been cleared */
  if (irq == 0) {
    return;
  }
  irq = ls1bit32(irq)+32;
#else
  irq = ((*DANUBE_ICU_IM_VEC) & DANUBE_ICU_IM4_VEC_MASK) >>6;
  if (irq ==0) return;
  irq+=31;
#endif /*IVEC_BUG_FIX*/
  DANUBE_INT_DMSG("danube_hw1_irqdispatch: irq=%d\n", irq);

  do_IRQ((int)irq, regs);

  return;
}
/* Cascaded interrupts from IM2 */
void danube_hw2_irqdispatch(struct pt_regs *regs)
{
  u32 irq;

#ifdef IVEC_BUG_FIX
  irq = (*DANUBE_ICU_IM2_IOSR); 
  /* if int_status == 0, then the interrupt has already been cleared */
  if (irq == 0) {
    return;
  }
  irq = ls1bit32(irq)+64;
#else
  irq = ((*DANUBE_ICU_IM_VEC) & DANUBE_ICU_IM4_VEC_MASK) >>12;
  if (irq ==0) return;
  irq+=63;
#endif /*IVEC_BUG_FIX*/
  DANUBE_INT_DMSG("danube_hw2_irqdispatch: irq=%d\n", irq);

  do_IRQ((int)irq, regs);

  return;
}
/* Cascaded interrupts from IM3 */
void danube_hw3_irqdispatch(struct pt_regs *regs)
{
  u32 irq;

#ifdef IVEC_BUG_FIX
  irq = (*DANUBE_ICU_IM3_IOSR); 
  /* if int_status == 0, then the interrupt has already been cleared */
  if (irq == 0) {
    return;
  }
  irq = ls1bit32(irq)+96;
#else
  irq = ((*DANUBE_ICU_IM_VEC) & DANUBE_ICU_IM4_VEC_MASK) >>18;
  if (irq ==0) return;
  irq+=95;
#endif /*IVEC_BUG_FIX*/
  DANUBE_INT_DMSG("danube_hw3_irqdispatch: irq=%d\n", irq);

  do_IRQ((int)irq, regs);

  return;
}
/* Cascaded interrupts from IM4 */
void danube_hw4_irqdispatch(struct pt_regs *regs)
{
  u32 irq;

#ifdef IVEC_BUG_FIX
  irq = (*DANUBE_ICU_IM4_IOSR); 
  /* if int_status == 0, then the interrupt has already been cleared */
  if (irq == 0) {
    return;
  }
  irq = ls1bit32(irq)+128;
#else
  irq = ((*DANUBE_ICU_IM_VEC) & DANUBE_ICU_IM4_VEC_MASK) >>24;
  if (irq ==0) return;
  irq+=127;
#endif /*IVEC_BUG_FIX*/
  DANUBE_INT_DMSG("danube_hw4_irqdispatch: irq=%d\n", irq);

  do_IRQ((int)irq, regs);

  return;
}
int danube_be_handler(struct pt_regs *regs, int is_fixup)
{
	volatile u32 reg1,reg2,reg3,reg4,reg5;
  /*TODO: bus error */
  printk("TODO: BUS error\n");
	return MIPS_BE_FATAL;
}


/* Function for careful CP0 interrupt mask access */

void __init init_IRQ(void)
{
  int i;

  DANUBE_INT_DMSG("init_IRQ\n");

  board_be_handler = &danube_be_handler;

  init_generic_irq();

  /* mask all interrupt sources */
  *DANUBE_ICU_IM0_IER = 0;
  *DANUBE_ICU_IM1_IER = 0;
  *DANUBE_ICU_IM2_IER = 0;
  *DANUBE_ICU_IM3_IER = 0;
  *DANUBE_ICU_IM4_IER = 0;


  /* Now safe to set the exception vector. */
  set_except_vector(0, mipsIRQ);

  for (i = 0; i <= INT_NUM_IM4_IRL31; i++) {
    irq_desc[i].status	= IRQ_DISABLED;
    irq_desc[i].action	= 0;
    irq_desc[i].depth	= 1;
    irq_desc[i].handler	= &danube_irq_type;
  }

  set_c0_status(IE_IRQ0 | IE_IRQ1 | IE_IRQ2 | IE_IRQ3 | IE_IRQ4 | IE_IRQ5);

#ifdef CONFIG_KGDB
  set_debug_traps();
  breakpoint();
#endif
}
