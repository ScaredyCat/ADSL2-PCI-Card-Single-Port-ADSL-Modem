/******************************************************************************
**
** FILE NAME    : port.h
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

#ifndef PORT_H
#define PORT_H

struct danube_port_ioctl_parm {
	int port;
	int pin;
	int value;
};
#define DANUBE_PORT_IOC_MAGIC	0xbf
#define DANUBE_PORT_IOCOD	_IOW( DANUBE_PORT_IOC_MAGIC,0,struct danube_port_ioctl_parm)
#define DANUBE_PORT_IOCPUDSEL	_IOW( DANUBE_PORT_IOC_MAGIC,1,struct danube_port_ioctl_parm)
#define DANUBE_PORT_IOCPUDEN	_IOW( DANUBE_PORT_IOC_MAGIC,2,struct danube_port_ioctl_parm)
#define DANUBE_PORT_IOCSTOFF	_IOW( DANUBE_PORT_IOC_MAGIC,3,struct danube_port_ioctl_parm)
#define DANUBE_PORT_IOCDIR	_IOW( DANUBE_PORT_IOC_MAGIC,4,struct danube_port_ioctl_parm)
#define DANUBE_PORT_IOCOUTPUT	_IOW( DANUBE_PORT_IOC_MAGIC,5,struct danube_port_ioctl_parm)
#define DANUBE_PORT_IOCINPUT	_IOWR(DANUBE_PORT_IOC_MAGIC,6,struct danube_port_ioctl_parm)
#define DANUBE_PORT_IOCALTSEL0  _IOW( DANUBE_PORT_IOC_MAGIC,7,struct danube_port_ioctl_parm)
#define DANUBE_PORT_IOCALTSEL1  _IOW( DANUBE_PORT_IOC_MAGIC,8,struct danube_port_ioctl_parm)

int danube_port_reserve_pin (int port, int pin, int module_id);
int danube_port_free_pin (int port, int pin, int module_id);
int danube_port_set_open_drain (int port, int pin, int module_id);
int danube_port_clear_open_drain (int port, int pin, int module_id);
int danube_port_set_pudsel (int port, int pin, int module_id);
int danube_port_clear_pudsel (int port, int pin, int module_id);
int danube_port_set_puden (int port, int pin, int module_id);
int danube_port_clear_puden (int port, int pin, int module_id);
int danube_port_set_stoff (int port, int pin, int module_id);
int danube_port_clear_stoff (int port, int pin, int module_id);
int danube_port_set_dir_out (int port, int pin, int module_id);
int danube_port_set_dir_in (int port, int pin, int module_id);
int danube_port_set_output (int port, int pin, int module_id);
int danube_port_clear_output (int port, int pin, int module_id);
int danube_port_get_input (int port, int pin, int module_id);

int danube_port_set_altsel0 (int port, int pin, int module_id);
int danube_port_clear_altsel0 (int port, int pin, int module_id);
int danube_port_set_altsel1 (int port, int pin, int module_id);
int danube_port_clear_altsel1 (int port, int pin, int module_id);

#endif /* PORT_H */
