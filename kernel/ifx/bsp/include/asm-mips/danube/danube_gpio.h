/******************************************************************************
**
** FILE NAME    : danube_gpio.h
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
 
#ifndef DANUBE_GPIO_H
#define DANUBE_GPIO_H
	
#define OK				0
	
#define DANUBE_PORT_OUT_REG		0x00000010
#define DANUBE_PORT_IN_REG		0x00000014
#define DANUBE_PORT_DIR_REG		0x00000018
#define DANUBE_PORT_ALTSEL0_REG		0x0000001C
#define DANUBE_PORT_ALTSEL1_REG		0x00000020
#define DANUBE_PORT_OD_REG		0x00000024
#define DANUBE_PORT_STOFF_REG		0x00000028
#define DANUBE_PORT_PUDSEL_REG		0x0000002C
#define DANUBE_PORT_PUDEN_REG		0x00000030
	
#define PORT_MODULE_MEI_JTAG		0x1
#define PORT_MODULE_ID	0xff
	
#define NOPS				asm("nop;nop;nop;nop;nop");
	
#undef GPIO_DEBUG
	
#ifdef GPIO_DEBUG
unsigned int gpio_global_register[25] =
	{ 0, 0, 0, 0, 0x00000004, 0x00000005, 0x00000006, 0x00000007,
	0x00000008, 0x00000009, 0x0000000A, 0x0000000B, 0x0000000C,
		0x0000000D, 0x0000000E,
	0x0000000F, 0x00000010, 0x00000011, 0x00000012, 0x00000013,
		0x00000014, 0x00000015,
	0x00000016, 0x00000017, 0x00000018 
};


#define PORT_WRITE_REG(reg,value)       gpio_global_register[((u32)reg - DANUBE_GPIO)/4] = (u32)value;
#define PORT_READ_REG(reg, value)       value = (gpio_global_register[((u32)reg - DANUBE_GPIO)/4]);	 
#else /*  */
	
#define PORT_WRITE_REG(reg, value)  	*((volatile u32*)(reg)) = (u32)value; 
#define PORT_READ_REG(reg, value)       value = (u32)*((volatile u32 *)(reg));
	
#endif /*  */
	
#define PORT_IOC_CALL(ret,port,pin,func) 	\
	ret = danube_port_reserve_pin (port, pin, PORT_MODULE_ID); \
if (ret == 0)\
	ret = func (port, pin, PORT_MODULE_ID);\
if (ret == 0)\
	ret = danube_port_free_pin (port, pin, PORT_MODULE_ID);

#endif	/* DANUBE_GPIO */
