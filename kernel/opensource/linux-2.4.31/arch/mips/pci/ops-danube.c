/******************************************************************************
**
** FILE NAME    : ops-danube.c
** PROJECT      : Danube
** MODULES      : PCI
**
** DATE         : 7 JUL 2006
** AUTHOR       : Liu Peng
** DESCRIPTION  : PCI Basic Read/Write Operation
** COPYRIGHT    :       Copyright (c) 2006
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
**  7 JUL 2006  Liu Peng        Initiate Version
** 23 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/danube/irq.h>
#include <asm/danube/danube.h>
#include <asm/pci_channel.h>

#define DANUBE_PCI_MEM_BASE    0x18000000
#define DANUBE_PCI_MEM_SIZE    0x02000000
#define DANUBE_PCI_IO_BASE     0x1AE00000
#define DANUBE_PCI_IO_SIZE     0x00200000

#define DANUBE_PCI_CFG_BUSNUM_SHF 16
#define DANUBE_PCI_CFG_DEVNUM_SHF 11
#define DANUBE_PCI_CFG_FUNNUM_SHF 8

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

#if 0
#define DBP(fmt, args...) printk(  fmt, ## args)
#else
#define DBP(fmt, args...)
#endif

static struct resource pci_io_resource = {
	"io pci IO space",
	DANUBE_PCI_IO_BASE,
	DANUBE_PCI_IO_BASE + DANUBE_PCI_IO_SIZE,
	IORESOURCE_IO
};
static struct resource pci_mem_resource = {
	"ext pci memory space",
	DANUBE_PCI_MEM_BASE,
	DANUBE_PCI_MEM_BASE + DANUBE_PCI_MEM_SIZE,
	IORESOURCE_MEM
};

static int
danube_pci_config_access (unsigned char access_type,
			  struct pci_dev *dev, unsigned char where,
			  u32 * data)
{
	unsigned char bus = dev->bus->number;
	unsigned char dev_fn = dev->devfn;
	u32 pci_addr;

	/* Danube support slot from 0 to 15 */
	/* dev_fn 0&0x68 (AD29) is danube itself */
	if ((bus != 0) || ((dev_fn & 0xf8) > 0x78) || ((dev_fn & 0xf8) == 0)
	    || ((dev_fn & 0xf8) == 0x68)) {
		return 1;
	}

	DBP ("\nbus=%x dev_fn=%x %s", bus, dev_fn,
	     (access_type == PCI_ACCESS_WRITE) ? "WRITE" : "READ");
	pci_addr =
		PCI_CFG_BASE | bus << DANUBE_PCI_CFG_BUSNUM_SHF | dev_fn <<
		DANUBE_PCI_CFG_FUNNUM_SHF | (where & ~0x3);

	DBP (" address=%x", pci_addr);
	/* Perform access */
	if (access_type == PCI_ACCESS_WRITE) {
#ifdef CONFIG_DANUBE_PCI_HW_SWAP
		*(volatile u32 *) pci_addr = swab32 (*data);
#else
		*(volatile u32 *) pci_addr = *data;
#endif
	}
	else {
		*data = *(volatile u32 *) pci_addr;
#ifdef CONFIG_DANUBE_PCI_HW_SWAP
		*data = swab32 (*data);
#endif
	}
	DBP (" %x", *data);
	u32 temp;
	/* clean possible Master abort */
	/* use PCI access to clear it */
	pci_addr = PCI_CFG_BASE | (0x0 << DANUBE_PCI_CFG_FUNNUM_SHF) + 4;
	temp = *((volatile u32 *) pci_addr);
#ifdef CONFIG_DANUBE_PCI_HW_SWAP
	temp = swab32 (temp);
#endif /*CONFIG_DANUBE_PCI_HW_SWAP */
	pci_addr = PCI_CFG_BASE | (0x68 << DANUBE_PCI_CFG_FUNNUM_SHF) + 4;
	*(volatile u32 *) pci_addr = temp;

	//TODO: non-existed device
	if ((*data) == 0xffffffff) {
		return 1;
	}
	return 0;
}

/*
 * We can't address 8 and 16 bit words directly.  Instead we have to
 * read/write a 32bit word and mask/modify the data we actually want.
 */
static int
danube_pci_read_config_byte (struct pci_dev *dev, int where, u8 * val)
{
	u32 data = 0;

	if (danube_pci_config_access (PCI_ACCESS_READ, dev, where, &data)) {
		return -1;
	}

	*val = (data >> ((where & 3) << 3)) & 0xff;

	return PCIBIOS_SUCCESSFUL;
}

static int
danube_pci_read_config_word (struct pci_dev *dev, int where, u16 * val)
{
	u32 data = 0;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (danube_pci_config_access (PCI_ACCESS_READ, dev, where, &data)) {
		*val = 0xffff;
		return -1;
	}

	*val = (data >> ((where & 3) << 3)) & 0xffff;

	return PCIBIOS_SUCCESSFUL;
}

static int
danube_pci_read_config_dword (struct pci_dev *dev, int where, u32 * val)
{
	u32 data = 0;

	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (danube_pci_config_access (PCI_ACCESS_READ, dev, where, &data))
		return -1;

	*val = data;

	return PCIBIOS_SUCCESSFUL;
}

static int
danube_pci_write_config_byte (struct pci_dev *dev, int where, u8 val)
{
	u32 data = 0;

	if (danube_pci_config_access (PCI_ACCESS_READ, dev, where, &data))
		return -1;

	data = (data & ~(0xff << ((where & 3) << 3))) |
		(val << ((where & 3) << 3));

	if (danube_pci_config_access (PCI_ACCESS_WRITE, dev, where, &data))
		return -1;

	return PCIBIOS_SUCCESSFUL;
}

static int
danube_pci_write_config_word (struct pci_dev *dev, int where, u16 val)
{
	u32 data = 0;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (danube_pci_config_access (PCI_ACCESS_READ, dev, where, &data))
		return -1;

	data = (data & ~(0xffff << ((where & 3) << 3))) |
		(val << ((where & 3) << 3));

	if (danube_pci_config_access (PCI_ACCESS_WRITE, dev, where, &data))
		return -1;

	return PCIBIOS_SUCCESSFUL;
}

static int
danube_pci_write_config_dword (struct pci_dev *dev, int where, u32 val)
{
	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (danube_pci_config_access (PCI_ACCESS_WRITE, dev, where, &val))
		return -1;

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops danube_pci_ops = {
	danube_pci_read_config_byte,
	danube_pci_read_config_word,
	danube_pci_read_config_dword,
	danube_pci_write_config_byte,
	danube_pci_write_config_word,
	danube_pci_write_config_dword
};

struct pci_channel mips_pci_channels[] = {
	{&danube_pci_ops, &pci_io_resource, &pci_mem_resource, 0x08, 0x7f},
	{NULL, NULL, NULL, 0, 0}
};

void __init
pcibios_fixup_irqs (void)
{
	struct pci_dev *dev;
	u8 pin;

	pci_for_each_dev (dev) {
		dev->irq = 0;
		pci_read_config_byte (dev, PCI_INTERRUPT_PIN, &pin);
		switch (pin) {
		case 0:
			break;
		case 1:
			//PCI_INTA--shared with EBU
			//falling edge level triggered:0x4, low level:0xc, rising edge:0x2
			(*DANUBE_EBU_PCC_CON) |= 0xc;
			/* enable interrupt only */
			(*DANUBE_EBU_PCC_IEN) |= 0x10;
			dev->irq = INT_NUM_IM0_IRL22;
			pci_write_config_byte (dev, PCI_INTERRUPT_LINE,
					       dev->irq);
			break;
		case 2:
		case 3:
		case 4:
			printk ("WARNING: interrupt pin %d not supported yet!\n", pin);
		default:
			printk ("WARNING: invalid interrupt pin %d\n", pin);
		}
	}
}

void __init
pcibios_fixup (void)
{
}
