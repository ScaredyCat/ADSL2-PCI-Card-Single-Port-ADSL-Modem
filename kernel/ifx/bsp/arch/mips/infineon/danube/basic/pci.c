/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999, 2000 MIPS Technologies, Inc.  All rights reserved.
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
 */

/*
 * Copyright (C) 2004 Infineon Technologies AG
 * Author IFAP DC COM SD
 * History:
 *  -Use 33M clock pliu20060613
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/pci_channel.h>
#include <asm/addrspace.h>

#if defined(CONFIG_PCI) && defined(CONFIG_DANUBE_CHIP_A11)
  #if defined(CONFIG_DANUBE_EBU_PCI_SOFTWARE_ARBITOR)
    #define EBU_PCI_SOFTWARE_ARBITOR
  #endif
  #if defined(CONFIG_DANUBE_PPE_PCI_SOFTWARE_ARBITOR)
    #define USE_FIX_FOR_PCI_PPE
  #endif
#endif

#define DANUBE_PCI_REG32( addr )		  (*(volatile u32 *)(addr))
#define DANUBE_PCI_TEST

extern struct pci_ops danube_pci_ops;

static void __init danube_pci_startup(void)
{
  /*initialize the first PCI device--danube itself*/
  u32 temp_buffer;


  /* make sure PCI clock is slower than FPI clock */
  extern unsigned int danube_get_fpi_hz(void);
  //pliu20060613
  //if ( danube_get_fpi_hz() < CLOCK_60M){
    /*use 33.3M */
    /*TODO: trigger reset */
    DANUBE_PCI_REG32(DANUBE_CGU_IFCCR) &= (~(0xf00000));
    DANUBE_PCI_REG32(DANUBE_CGU_IFCCR) |= ((0x800000));
  //pliu20060613
  //printk("PCI clock 33.3MHz\n");
  //}
  /* PCIS of IF_CLK of CGU   : 1 =>PCI Clock output
     0 =>clock input
     PADsel of PCI_CR of CGU : 1 =>From CGU
     : 0 =>From pad
   */
  DANUBE_PCI_REG32(DANUBE_CGU_IFCCR) |= ((1<<16));
  //removed: pliu20060613
  //DANUBE_PCI_REG32(DANUBE_CGU_PCICR) = (1<<31);
  DANUBE_PCI_REG32(DANUBE_CGU_PCICR) = ((1<<31)|(1<<30));

  /* prepare GPIO */
  /* PCI_RST: P1.5 ALT 01*/
  //pliu20060613: start
  *DANUBE_GPIO_P1_OUT |= ((1<<5));
  *DANUBE_GPIO_P1_OD |=  ((1<<5));
  *DANUBE_GPIO_P1_DIR |=  ((1<<5));
  *DANUBE_GPIO_P1_ALTSEL1 &= (~(1<<5));
  *DANUBE_GPIO_P1_ALTSEL0 |= ((1<<5));
  //pliu20060613: end
  /* PCI_REQ1: P1.13 ALT 01*/
  /* PCI_GNT1: P1.14 ALT 01*/
  *DANUBE_GPIO_P1_DIR &= (~(0x2000));
  *DANUBE_GPIO_P1_DIR |= ((0x4000));
  *DANUBE_GPIO_P1_ALTSEL1 &= (~(0x6000));
  *DANUBE_GPIO_P1_ALTSEL0 |= ((0x6000));
  /* PCI_REQ2: P1.15 ALT 10*/
  /* PCI_GNT2: P1.7 ALT 10*/
#ifdef CONFIG_USE_EMULATOR
  /*change to internal reset*/
  *DANUBE_CGU_PCICR= *DANUBE_CGU_PCICR | 0x40000000;
  /* GPIO21 PCI reset */
  /* PCI reset:1*/
  *DANUBE_GPIO_P1_OUT = *DANUBE_GPIO_P1_OUT & (~(0x20));
  *DANUBE_GPIO_P1_OUT = *DANUBE_GPIO_P1_OUT | ((0x20));
#endif
  /* enable auto-switching between PCI and EBU */
  DANUBE_PCI_REG32(PCI_CR_CLK_CTRL_REG) = 0xA;
  /* busy, i.e. configuration is not done, PCI access has to be retried */
  DANUBE_PCI_REG32(PCI_CR_PCI_MOD_REG) &= (~(1<<24));
  wmb();
  /* BUS Master/IO/MEM access*/
  DANUBE_PCI_REG32(PCI_CS_STS_CMD_REG) |= (7);

  temp_buffer = DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG);
#ifdef EBU_PCI_SOFTWARE_ARBITOR
  temp_buffer &= (~(0x1<<16)); // enable 1st external pci master only
#else
  /* enable external 2 PCI masters */
  temp_buffer &= (~(0xf<<16));
#endif
  /* enable internal arbiter */
  temp_buffer |= (1<< INTERNAL_ARB_ENABLE_BIT);
  /* enable internal PCI master reqest*/
  temp_buffer &= (~(3<< PCI_MASTER0_REQ_MASK_2BITS));

#ifdef EBU_PCI_SOFTWARE_ARBITOR
 #if 1
  *DANUBE_GPIO_P0_OUT &= (~(1<<7));
  *DANUBE_GPIO_P0_OD |=  ((1<<7));
  *DANUBE_GPIO_P0_DIR |=  ((1<<7));
  *DANUBE_GPIO_P0_ALTSEL1 &= (~(1<<7));
  *DANUBE_GPIO_P0_ALTSEL0 &= (~(1<<7));
 #endif
  /* enable ebu */
  temp_buffer &= (~(3<< PCI_MASTER1_REQ_MASK_2BITS));
  DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG) = temp_buffer;

  /* enable Config space access */
  DANUBE_PCI_REG32(0xBE105430) = 0x00000103;

  DANUBE_PCI_REG32(0xBE105400) &= ~0x2;
  temp_buffer = DANUBE_PCI_REG32(0xB700006C);
  temp_buffer &= (~0x00000001);
  DANUBE_PCI_REG32(0xB700006C) = temp_buffer;
  temp_buffer = DANUBE_PCI_REG32(0xB700006C); // do dummy read
 #if 1
  printk("\nwaiting ebu ownnership");
  while ((DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG) & 0x400000) != 0) {
    *DANUBE_GPIO_P0_OUT |= ((1<<7));
    printk("\n.\n");
  }
  *DANUBE_GPIO_P0_OUT &= (~(1<<7));
  printk("done\n");
 #endif
  /* disable ebu */
  DANUBE_PCI_REG32(0xBE105400) |= 0x2;
  temp_buffer = DANUBE_PCI_REG32(0xB700006C);
  temp_buffer |= (0x00000001);
  DANUBE_PCI_REG32(0xB700006C) = temp_buffer;

  /* disable Config space access */
  DANUBE_PCI_REG32(0xBE105430) = 0x01000103;

  temp_buffer = DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG);
  temp_buffer |= ((3<< PCI_MASTER1_REQ_MASK_2BITS));
#else
  /* enable EBU reqest*/
  temp_buffer &= (~(3<< PCI_MASTER1_REQ_MASK_2BITS));
#endif

  /* enable all external masters request*/
  temp_buffer &= (~(3<< PCI_MASTER2_REQ_MASK_2BITS));
  DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG) = temp_buffer;

  wmb();

  /* FPI ==> PCI MEM address mapping */
  /* base: 0xb8000000 == > 0x18000000 */
  /* size: 8x4M = 32M*/
  DANUBE_PCI_REG32(PCI_CR_FCI_ADDR_MAP0_REG) = 0x18000000;
  DANUBE_PCI_REG32(PCI_CR_FCI_ADDR_MAP1_REG) = 0x18400000;
  DANUBE_PCI_REG32(PCI_CR_FCI_ADDR_MAP2_REG) = 0x18800000;
  DANUBE_PCI_REG32(PCI_CR_FCI_ADDR_MAP3_REG) = 0x18c00000;
  DANUBE_PCI_REG32(PCI_CR_FCI_ADDR_MAP4_REG) = 0x19000000;
  DANUBE_PCI_REG32(PCI_CR_FCI_ADDR_MAP5_REG) = 0x19400000;
  DANUBE_PCI_REG32(PCI_CR_FCI_ADDR_MAP6_REG) = 0x19800000;
  DANUBE_PCI_REG32(PCI_CR_FCI_ADDR_MAP7_REG) = 0x19c00000;

  /* FPI ==> PCI IO address mapping */
  /* base: 0xbAE00000 == > 0xbAE00000 */
  /* size: 2M */
  DANUBE_PCI_REG32(PCI_CR_FCI_ADDR_MAP11hg_REG) = 0xbae00000;

  /* PCI ==> FPI address mapping */
  /* base: 0x0 ==> 0x0 */
  /* size: 32M */
  /* BAR1 32M map to SDR address */
  DANUBE_PCI_REG32(PCI_CR_BAR11MASK_REG) = 0x0e000008;
  DANUBE_PCI_REG32(PCI_CR_PCI_ADDR_MAP11_REG) = 0x0;
  DANUBE_PCI_REG32(PCI_CS_BASE_ADDR1_REG) = 0x0;
#ifdef CONFIG_DANUBE_PCI_HW_SWAP
  /* both TX and RX endian swap are enabled */
  DANUBE_PCI_REG32(PCI_CR_PCI_EOI_REG) |= 3;
  wmb();
#endif
  /*TODO: disable BAR2 & BAR3*/
  DANUBE_PCI_REG32(PCI_CR_BAR12MASK_REG) |= 0x80000000;
  DANUBE_PCI_REG32(PCI_CR_BAR13MASK_REG) |= 0x80000000;

  /*use 8 dw burse length */
  DANUBE_PCI_REG32(PCI_CR_FCI_BURST_LENGTH_REG) = 0x303;

  DANUBE_PCI_REG32(PCI_CR_PCI_MOD_REG) |= ((1<<24));
  wmb();
  //pliu20060613: start
  *DANUBE_GPIO_P1_OUT &= (~(1<<5));
  wmb();
  mdelay(1);
  *DANUBE_GPIO_P1_OUT |= (1<<5);
  //pliu20060613: end
}

extern int pciauto_assign_resources(int busno, struct pci_channel * hose);
extern void pcibios_fixup(void);
extern void pcibios_fixup_irqs(void);

#ifdef DANUBE_PCI_TEST
static void dump_cs(u32 base)
{
  int i;
  u32 data;
  for(i=0;i<64;i++){
    data = DANUBE_PCI_REG32(((base) | (i*4)));
#ifdef CONFIG_DANUBE_PCI_HW_SWAP
    data = swab32(data);
#endif
    printk("%8x ", data);
    if (i % 8 == 7) printk("\n");
  }
  printk("\n");
}
#endif

void __init pcibios_init(void)
{
  struct pci_channel *p;
  int busno;

  printk("PCI: Probing PCI hardware on host bus 0.\n");

#ifdef CONFIG_USE_EMULATOR
  *DANUBE_GPIO_P1_ALTSEL0 = *DANUBE_GPIO_P1_ALTSEL0 & ~(1<<7);
  *DANUBE_GPIO_P1_ALTSEL1 = *DANUBE_GPIO_P1_ALTSEL1 & ~(1<<7);
  *DANUBE_GPIO_P1_DIR = *DANUBE_GPIO_P1_DIR | (1<<7) ;
  *DANUBE_GPIO_P1_OD = *DANUBE_GPIO_P1_OD | (1<<7) ;
  *DANUBE_GPIO_P1_OUT = *DANUBE_GPIO_P1_OUT | (1<<7) ;

  *DANUBE_GPIO_P0_ALTSEL0 = *DANUBE_GPIO_P0_ALTSEL0 & (~0x1);
  *DANUBE_GPIO_P0_ALTSEL1 = *DANUBE_GPIO_P0_ALTSEL1 & (~0x1);
  //*DANUBE_GPIO_P0_DIR |= 0x81;
  *DANUBE_GPIO_P0_DIR |= 0x01;
  *DANUBE_GPIO_P0_OUT |= 0x1;

  *DANUBE_GPIO_P1_DIR = *DANUBE_GPIO_P1_DIR | 0x20;
  *DANUBE_GPIO_P1_OD = *DANUBE_GPIO_P1_OD | 0x20;

  /* GPIO 0 reset amazon */
  *DANUBE_GPIO_P0_OUT = *DANUBE_GPIO_P0_OUT &(~(1<<0));
  *DANUBE_GPIO_P0_OUT = *DANUBE_GPIO_P0_OUT |((1<<0));

  /* PCI reset:1*/
  *DANUBE_GPIO_P1_OUT = *DANUBE_GPIO_P1_OUT | (0x20);

#endif

  danube_pci_startup();
#ifdef CONFIG_PCI_AUTO
  /* assign resources */
  busno=0;
  for (p= mips_pci_channels; p->pci_ops != NULL;p++) {
    busno =pciauto_assign_resources(busno,p) + 1;
  }
#endif
  pci_scan_bus(0, &danube_pci_ops, NULL);
#ifdef DANUBE_PCI_TEST
  dump_cs(EXT_PCI14_CONFIG_SPACE_BASE_ADDR);
#endif
  DANUBE_PCI_REG32(PCI_CR_CLK_CTRL_REG) &= (~8);

  /* machine dependent fixups */
  pcibios_fixup();
  /* fixup irqs (board specific routines) */
  pcibios_fixup_irqs();
}

struct pci_fixup pcibios_fixups[] = {
	{ 0 }
};

/*
 *  Called after each bus is probed, but before its children
 *  are examined.
 */
void __devinit pcibios_fixup_bus(struct pci_bus *b)
{
	pci_read_bridge_bases(b);
}

unsigned int pcibios_assign_all_busses(void)
{
	return 1;
}

#ifdef  USE_FIX_FOR_PCI_PPE
/** Brief:  disable external pci aribtor request
  * Details:
      blocking call, i.e. only return when there is no external PCI bus activities
  */
void danube_disable_external_pci(void)
{
 #ifdef EBU_PCI_SOFTWARE_ARBITOR
  DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG)  |= (1<< 16);
 #else
  DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG)  |= ((3<< PCI_MASTER2_REQ_MASK_2BITS));
 #endif

  wmb();

 #ifdef EBU_PCI_SOFTWARE_ARBITOR
  /* check IRDY is high only */
  while( (DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG) & 0x100000 ) != 0x100000);
 #else
  /* make sure EBUSY is low && Frame Ird is high) */
  while( (DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG) & 0x700000 ) != 0x300000);
 #endif
}
/** Brief:  enable external pci aribtor request
  * Details:
      non-blocking call
  */
void danube_enable_external_pci(void)
{
 #ifdef EBU_PCI_SOFTWARE_ARBITOR
  DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG)  &= (~(1<< 16));
 #else
  /* enable all external masters request*/
  DANUBE_PCI_REG32(PCI_CR_PC_ARB_REG)  &= (~(3<< PCI_MASTER2_REQ_MASK_2BITS));
 #endif
  wmb();
}
#endif

