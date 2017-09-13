#ifndef __DANUBE_LED_DEV_H__2005_07_22__13_37__
#define __DANUBE_LED_DEV_H__2005_07_22__13_37__


/******************************************************************************
**
** FILE NAME    : danube_led.h
** PROJECT      : Danube
** MODULES     	: LED
**
** DATE         : 22 JUL 2005
** AUTHOR       : Xu Liang
** DESCRIPTION  : LED Controller Driver Header File
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
** 22 JUL 2005  Xu Liang        Initiate Version
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
#define LED_CONFIG                      0x01

/*
 *  Definition of Operation MASK
 */
#define CONFIG_OPERATION_UPDATE_SOURCE  0x0001
#define CONFIG_OPERATION_BLINK          0x0002
#define CONFIG_OPERATION_UPDATE_CLOCK   0x0004
#define CONFIG_OPERATION_STORE_MODE     0x0008
#define CONFIG_OPERATION_SHIFT_CLOCK    0x0010
#define CONFIG_OPERATION_DATA_OFFSET    0x0020
#define CONFIG_OPERATION_NUMBER_OF_LED  0x0040
#define CONFIG_OPERATION_DATA           0x0080
#define CONFIG_OPERATION_MIPS0_ACCESS   0x0100
#define CONFIG_DATA_CLOCK_EDGE          0x0200


/*
 *  Data Type Used to Call ioctl
 */
struct led_config_param {
    unsigned long   operation_mask;         //  Select operations to be performed
    unsigned long   led;                    //  LED to change update source (LED or ADSL)
    unsigned long   source;                 //  Corresponding update source (LED or ADSL)
    unsigned long   blink_mask;             //  LEDs to set blink mode
    unsigned long   blink;                  //  Set to blink mode or normal mode
    unsigned long   update_clock;           //  Select the source of update clock
    unsigned long   fpid;                   //  If FPI is the source of update clock, set the divider
                                            //  else if GPT is the source, set the frequency
    unsigned long   store_mode;             //  Set clock mode or single pulse mode for store signal
    unsigned long   fpis;                   //  FPI is the source of shift clock, set the divider
    unsigned long   data_offset;            //  Set cycles to be inserted before data is transmitted
    unsigned long   number_of_enabled_led;  //  Total number of LED to be enabled
    unsigned long   data_mask;              //  LEDs to set value
    unsigned long   data;                   //  Corresponding value
    unsigned long   mips0_access_mask;      //  LEDs to set access right
    unsigned long   mips0_access;           //  1: the corresponding data is output from MIPS0, 0: MIPS1
    unsigned long   f_data_clock_on_rising; //  1: data clock on rising edge, 0: data clock on falling edge
};


/*
 * ####################################
 *             Declaration
 * ####################################
 */

#if defined(__KERNEL__)
    extern int danube_led_set_blink(unsigned int, unsigned int);
    extern int danube_led_set_data(unsigned int, unsigned int);
    extern int danube_led_config(struct led_config_param *);
#endif  //  defined(__KERNEL__)


#endif  //  __DANUBE_LED_DEV_H__2005_07_22__13_37__
