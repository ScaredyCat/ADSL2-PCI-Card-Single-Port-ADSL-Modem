#ifndef __DANUBE_CGU_DEV_H__2005_07_20__14_26__
#define __DANUBE_CGU_DEV_H__2005_07_20__14_26__

/******************************************************************************
**
** FILE NAME    : danube_cgu.h
** PROJECT      : Danube
** MODULES     	: CGU
**
** DATE         : 20 JUL 2005
** AUTHOR       : Xu Liang
** DESCRIPTION  : Clock Generation Unit (CGU) Driver Header File
** COPYRIGHT    : 	Copyright (c) 2006
**			Infineon Technologies AG
**			Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 20 JUL 2005  Xu Liang        Initiate Version
** 23 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/

/*
 * ####################################
 *              Definition
 * ####################################
 */

/*
 *  ioctl Command
 */
#define CGU_IOC_MAGIC                   'u'
#define CGU_GET_CLOCK_RATES             _IOR(CGU_IOC_MAGIC, 0, struct cgu_clock_rates)
#define CGU_IOC_MAXNR                   1

/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*
 *  Data Type Used to Call ioctl(GET_CLOCK_RATES)
 */
struct cgu_clock_rates {
	u32 mips0;
	u32 mips1;
	u32 cpu;
	u32 io_region;
	u32 fpi_bus1;
	u32 fpi_bus2;
	u32 pp32;
	u32 pci;
	u32 mii0;
	u32 mii1;
	u32 usb;
	u32 clockout0;
	u32 clockout1;
	u32 clockout2;
	u32 clockout3;
};

/*
 * ####################################
 *             Declaration
 * ####################################
 */

#if defined(__KERNEL__)
extern u32 cgu_get_mips_clock (int);
extern u32 cgu_get_cpu_clock (void);
extern u32 cgu_get_io_region_clock (void);
extern u32 cgu_get_fpi_bus_clock (int);
extern u32 cgu_get_pp32_clock (void);
extern u32 cgu_get_pci_clock (void);
extern u32 cgu_get_ethernet_clock (int);
extern u32 cgu_get_usb_clock (void);
extern u32 cgu_get_clockout (int);
#endif //  defined(__KERNEL__)

#endif //  __DANUBE_CGU_DEV_H__2005_07_20__14_26__
