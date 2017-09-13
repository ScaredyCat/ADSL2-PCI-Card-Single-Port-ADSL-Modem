/******************************************************************************
**
** FILE NAME    : danube_wdt.h
** PROJECT      : Danube
** MODULES      : WDT
**
** DATE         : 4 Aug 2005
** AUTHOR       : Huang Xiaogang
** DESCRIPTION  : Danube Watchdog Timer driver
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
** 01 Jan 2006  Huang Xiaogang  modification & verification on Danube chip
*******************************************************************************/

#ifndef DANUBE_WDT_H
#define DANUBE_WDT_H

#ifdef WDT_DEBUG

unsigned int danube_wdt_register[3] = { 0x00000001,
	0,
	0x00000002
};

#define DANUBE_WDT_REG32(addr) 		 (danube_wdt_register[(addr - DANUBE_BIU_WDT_BASE)/4])

#else

#define DANUBE_WDT_REG32(addr) 		 (*((volatile u32*)(addr)))

#endif

/* Define for device driver code */
#define DEVICE_NAME "wdt"

/* Danube wdt ioctl control */
#define DANUBE_WDT_IOC_MAGIC             0xc0
#define DANUBE_WDT_IOC_START            _IOW(DANUBE_WDT_IOC_MAGIC, 0, int)
#define DANUBE_WDT_IOC_STOP             _IO(DANUBE_WDT_IOC_MAGIC, 1)
#define DANUBE_WDT_IOC_PING             _IO(DANUBE_WDT_IOC_MAGIC, 2)
#define DANUBE_WDT_IOC_SET_PWL          _IOW(DANUBE_WDT_IOC_MAGIC, 3, int)
#define DANUBE_WDT_IOC_SET_DSEN         _IOW(DANUBE_WDT_IOC_MAGIC, 4, int)
#define DANUBE_WDT_IOC_SET_LPEN         _IOW(DANUBE_WDT_IOC_MAGIC, 5, int)
#define DANUBE_WDT_IOC_GET_STATUS       _IOR(DANUBE_WDT_IOC_MAGIC, 6, int)
#define DANUBE_WDT_IOC_SET_CLKDIV	_IOW(DANUBE_WDT_IOC_MAGIC, 7, int)

#define DANUBE_WDT_PW1 0x000000BE /**< First password for access */
#define DANUBE_WDT_PW2 0x000000DC /**< Second password for access */

#define DANUBE_WDT_CLKDIV0_VAL 1
#define DANUBE_WDT_CLKDIV1_VAL 64
#define DANUBE_WDT_CLKDIV2_VAL 4096
#define DANUBE_WDT_CLKDIV3_VAL 262144
#define DANUBE_WDT_CLKDIV0 0
#define DANUBE_WDT_CLKDIV1 1
#define DANUBE_WDT_CLKDIV2 2
#define DANUBE_WDT_CLKDIV3 3

#endif //DANUBE_WDT_H
