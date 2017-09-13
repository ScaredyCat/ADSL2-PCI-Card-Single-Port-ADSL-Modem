/******************************************************************************
**
** FILE NAME    : port_defs.h
** PROJECT      : Danube
** MODULES      : GPIO
**
** DATE         : 21 Jun 2004
** AUTHOR       : btxu
** DESCRIPTION  : Global DANUBE_GPIO driver header file
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
** 21 Jun 2004   btxu            Generate from INCA-IP project
** 21 Jun 2005   Jin-Sze.Sow     Comments edited
** 01 Jan 2006   Huang Xiaogang  Modification & verification on Danube chip
*******************************************************************************/

#ifndef PORT_DEFS_H
#define PORT_DEFS_H

#define OK	0

#define DANUBE_PORT_OUT_REG		0x00000010
#define DANUBE_PORT_IN_REG		0x00000014
#define DANUBE_PORT_DIR_REG		0x00000018
#define DANUBE_PORT_ALTSEL0_REG		0x0000001C
#define DANUBE_PORT_ALTSEL1_REG		0x00000020
#define DANUBE_PORT_OD_REG		0x00000024
#define DANUBE_PORT_STOFF_REG		0x00000028
#define DANUBE_PORT_PUDSEL_REG		0x0000002C
#define DANUBE_PORT_PUDEN_REG		0x00000030

#define PORT_MODULE_ID	0xff

#define NOPS	asm("nop;nop;nop;nop;nop")
#define PORT_WRITE_REG(reg, value)   \
	*((volatile u32*)(reg)) = (u32)value;
#define PORT_READ_REG(reg, value)    \
	value = (u32)*((volatile u32*)(reg));

#define PORT_IOC_CALL(ret,port,pin,func) 	\
	ret=danube_port_reserve_pin(port,pin,PORT_MODULE_ID); \
	if (ret == 0) ret=func(port,pin,PORT_MODULE_ID); \
	if (ret == 0) ret=danube_port_free_pin(port,pin,PORT_MODULE_ID);

#endif /* PORT_DEFS_H */
