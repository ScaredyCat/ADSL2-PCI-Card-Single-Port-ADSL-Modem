/******************************************************************************
**
** FILE NAME    : danube_bcu.h
** PROJECT      : Danube
** MODULES      : BCU
**
** DATE         : 04 July 2005
** AUTHOR       : Huang Xiaogang
** DESCRIPTION  : Danube Bcu driver header file
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

#ifndef DANUBE_BCU_H
#define DANUBE_BCU_H

#ifndef BCU_DEBUG
#define DANUBE_BCU_REG32(addr)	(*((volatile unsigned int*)(addr)))
#else
unsigned int bcu_global_register[65];
#define DANUBE_BCU_REG32(addr)	(bcu_global_register[(addr - DANUBE_BCU_BASE_ADDR)/4])
#endif

#define DANUBE_BCU_IOC_MAGIC             0xd0
#define DANUBE_BCU_IOC_SET_PS    	_IOW(DANUBE_BCU_IOC_MAGIC, 0,  int)
#define DANUBE_BCU_IOC_SET_DBG   	_IOW(DANUBE_BCU_IOC_MAGIC, 1,  int)
#define DANUBE_BCU_IOC_SET_TOUT         _IOW(DANUBE_BCU_IOC_MAGIC, 2,  int)
#define DANUBE_BCU_IOC_GET_PS    	_IOR(DANUBE_BCU_IOC_MAGIC, 3,  int)
#define DANUBE_BCU_IOC_GET_DBG   	_IOR(DANUBE_BCU_IOC_MAGIC, 4,  int)
#define DANUBE_BCU_IOC_GET_TOUT         _IOR(DANUBE_BCU_IOC_MAGIC, 5, int)
#define DANUBE_BCU_IOC_GET_BCU_ERR      _IOR(DANUBE_BCU_IOC_MAGIC, 6, int)
#define DANUBE_BCU_IOC_IRNEN	        _IOW(DANUBE_BCU_IOC_MAGIC, 7, int)
#define DANUBE_BCU_IOC_SET_PM           _IOW(DANUBE_BCU_IOC_MAGIC, 8,  int)
#define DANUBE_BCU_IOC_MAXNR            9

#endif //DANUBE_EBU_H
