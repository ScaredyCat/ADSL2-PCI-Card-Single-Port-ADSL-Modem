/******************************************************************************
**
** FILE NAME    : danube_pmu.h
** PROJECT      : Danube
** MODULES      : PMU
**
** DATE         : 1 Jan 2006
** AUTHOR       : Huang Xiaogang
** DESCRIPTION  : Danube Power Management Unit driver header file
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

#ifndef DANUBE_PMU_H
#define DANUBE_PMU_H

typedef struct pmu_dev {
	char name[16];
	int major;
	int minor;
	int occupied;
	int count;
	char buff[10];
} pmu_dev;

#define DANUBE_PMU_IOC_MAGIC             	0xe0
#define DANUBE_PMU_IOC_SET_USBPHY       	_IOW( DANUBE_PMU_IOC_MAGIC, 0, int)
#define DANUBE_PMU_IOC_SET_FPI1         	_IOW( DANUBE_PMU_IOC_MAGIC, 1, int)
#define DANUBE_PMU_IOC_SET_VMIPS        	_IOW( DANUBE_PMU_IOC_MAGIC, 2, int)
#define DANUBE_PMU_IOC_SET_VODEC        	_IOW( DANUBE_PMU_IOC_MAGIC, 3, int)
#define DANUBE_PMU_IOC_SET_PCI         		_IOW( DANUBE_PMU_IOC_MAGIC, 4, int)
#define DANUBE_PMU_IOC_SET_DMA         		_IOW( DANUBE_PMU_IOC_MAGIC, 5, int)
#define DANUBE_PMU_IOC_SET_USB         		_IOW( DANUBE_PMU_IOC_MAGIC, 6, int)
#define DANUBE_PMU_IOC_SET_UART0        	_IOW( DANUBE_PMU_IOC_MAGIC, 7, int)
#define DANUBE_PMU_IOC_SET_SPI         		_IOW( DANUBE_PMU_IOC_MAGIC, 8, int)
#define DANUBE_PMU_IOC_SET_DSL         		_IOW( DANUBE_PMU_IOC_MAGIC, 9, int)
#define DANUBE_PMU_IOC_SET_EBU         		_IOW( DANUBE_PMU_IOC_MAGIC, 10, int)
#define DANUBE_PMU_IOC_SET_LEDC         	_IOW( DANUBE_PMU_IOC_MAGIC, 11, int)
#define DANUBE_PMU_IOC_SET_GPTC         	_IOW( DANUBE_PMU_IOC_MAGIC, 12, int)
#define DANUBE_PMU_IOC_SET_PPE         		_IOW( DANUBE_PMU_IOC_MAGIC, 13, int)
#define DANUBE_PMU_IOC_SET_FPI0         	_IOW( DANUBE_PMU_IOC_MAGIC, 14, int)
#define DANUBE_PMU_IOC_SET_AHB         		_IOW( DANUBE_PMU_IOC_MAGIC, 15, int)
#define DANUBE_PMU_IOC_SET_SDIO         	_IOW( DANUBE_PMU_IOC_MAGIC, 16, int)
#define DANUBE_PMU_IOC_SET_UART1        	_IOW( DANUBE_PMU_IOC_MAGIC, 17, int)
#define DANUBE_PMU_IOC_SET_WDT0         	_IOW( DANUBE_PMU_IOC_MAGIC, 18, int)
#define DANUBE_PMU_IOC_SET_WDT1         	_IOW( DANUBE_PMU_IOC_MAGIC, 19, int)
#define DANUBE_PMU_IOC_SET_DEU          	_IOW( DANUBE_PMU_IOC_MAGIC, 20, int)
#define DANUBE_PMU_IOC_SET_PPE_TC        	_IOW( DANUBE_PMU_IOC_MAGIC, 21, int)
#define DANUBE_PMU_IOC_SET_PPE_ENET1        	_IOW( DANUBE_PMU_IOC_MAGIC, 22, int)
#define DANUBE_PMU_IOC_SET_PPE_ENET0        	_IOW( DANUBE_PMU_IOC_MAGIC, 23, int)
#define DANUBE_PMU_IOC_SET_PPE_UTP              _IOW( DANUBE_PMU_IOC_MAGIC, 24, int)
#define DANUBE_PMU_IOC_SET_PPE_TDM        	_IOW( DANUBE_PMU_IOC_MAGIC, 25, int)
#define DANUBE_PMU_IOC_SET_PWDCR        	_IOW( DANUBE_PMU_IOC_MAGIC, 26, int)
#define DANUBE_PMU_IOC_GET_STATUS       	_IOR( DANUBE_PMU_IOC_MAGIC, 27, int)

#endif
