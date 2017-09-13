#ifndef __DANUBE_PPE_DEV_H__2005_08_04__11_23__
#define __DANUBE_PPE_DEV_H__2005_08_04__11_23__


/******************************************************************************
**
** FILE NAME    : danube_ppe.h
** PROJECT      : Danube
** MODULES     	: ATM (ADSL)
**
** DATE         : 1 AUG 2005
** AUTHOR       : Xu Liang
** DESCRIPTION  : ATM Driver Header File
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
**  4 AUG 2005  Xu Liang        Initiate Version
**  2 OCT 2006  Xu Liang        Add config option and register function for
**                              set_cell_rate and adsl_led functions.
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
#define PPE_ATM_IOC_MAGIC       'o'
#define PPE_ATM_MIB_CELL        _IOW(PPE_ATM_IOC_MAGIC, 0, atm_cell_ifEntry_t)
#define PPE_ATM_MIB_AAL5        _IOW(PPE_ATM_IOC_MAGIC, 1, atm_aal5_ifEntry_t)
#define PPE_ATM_MIB_VCC         _IOWR(PPE_ATM_IOC_MAGIC, 2, atm_aal5_vcc_x_t)
#define PPE_ATM_IOC_MAXNR       3


/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*
 *  Data Type Used to Call ioctl
 */
typedef struct {
	int             vpi;
	int             vci;
	atm_aal5_vcc_t  mib_vcc;
} atm_aal5_vcc_x_t;


/*
 * ####################################
 *             Declaration
 * ####################################
 */

#if defined(__KERNEL__)
  extern void ifx_atm_set_cell_rate(int, u32);
  extern int IFX_ATM_LED_Callback_Register(void (*)(void));
  extern int IFX_ATM_LED_Callback_Unregister( void (*)(void));
#endif


#endif  //  __DANUBE_PPE_DEV_H__2005_08_04__11_23__

