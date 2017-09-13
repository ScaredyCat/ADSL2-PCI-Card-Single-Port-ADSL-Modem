#ifndef __DANUBE_GPTU_DEV_H__2005_07_26__10_19__
#define __DANUBE_GPTU_DEV_H__2005_07_26__10_19__


/******************************************************************************
**
** FILE NAME    : danube_gptu.h
** PROJECT      : Danube
** MODULES     	: GPTU
**
** DATE         : 26 JUL 2005
** AUTHOR       : Xu Liang
** DESCRIPTION  : General Purpose Timer Unit (GPTU) Driver Header File
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
** 26 JUL 2005  Xu Liang        Initiate Version
** 23 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/


/*
 * ####################################
 *              Definition
 * ####################################
 */

/*
 *  Available Timer/Counter Index
 */
#define TIMER(n, X)                     (n * 2 + (X ? 1 : 0))
#define TIMER_ANY                       0x00
#define TIMER1A                         TIMER(1, 0)
#define TIMER1B                         TIMER(1, 1)
#define TIMER2A                         TIMER(2, 0)
#define TIMER2B                         TIMER(2, 1)
#define TIMER3A                         TIMER(3, 0)
#define TIMER3B                         TIMER(3, 1)

/*
 *  Flag of Timer/Counter
 *  These flags specify the way in which timer is configured.
 */
/*  Bit size of timer/counter.                      */
#define TIMER_FLAG_16BIT                0x0000
#define TIMER_FLAG_32BIT                0x0001
/*  Switch between timer and counter.               */
#define TIMER_FLAG_TIMER                0x0000
#define TIMER_FLAG_COUNTER              0x0002
/*  Stop or continue when overflowing/underflowing. */
#define TIMER_FLAG_ONCE                 0x0000
#define TIMER_FLAG_CYCLIC               0x0004
/*  Count up or counter down.                       */
#define TIMER_FLAG_UP                   0x0000
#define TIMER_FLAG_DOWN                 0x0008
/*  Count on specific level or edge.                */
#define TIMER_FLAG_HIGH_LEVEL_SENSITIVE 0x0000
#define TIMER_FLAG_LOW_LEVEL_SENSITIVE  0x0040
#define TIMER_FLAG_RISE_EDGE            0x0010
#define TIMER_FLAG_FALL_EDGE            0x0020
#define TIMER_FLAG_ANY_EDGE             0x0030
/*  Signal is syncronous to module clock or not.    */
#define TIMER_FLAG_UNSYNC               0x0000
#define TIMER_FLAG_SYNC                 0x0080
/*  Different interrupt handle type.                */
#define TIMER_FLAG_NO_HANDLE            0x0000
#if defined(__KERNEL__)
    #define TIMER_FLAG_CALLBACK_IN_IRQ  0x0100
#endif  //  defined(__KERNEL__)
#define TIMER_FLAG_SIGNAL               0x0300
/*  Internal clock source or external clock source  */
#define TIMER_FLAG_INT_SRC              0x0000
#define TIMER_FLAG_EXT_SRC              0x1000


/*
 *  ioctl Command
 */
#define GPTU_REQUEST_TIMER              0x01    /*  General method to setup timer/counter.  */
#define GPTU_FREE_TIMER                 0x02    /*  Free timer/counter.                     */
#define GPTU_START_TIMER                0x03    /*  Start or resume timer/counter.          */
#define GPTU_STOP_TIMER                 0x04    /*  Suspend timer/counter.                  */
#define GPTU_GET_COUNT_VALUE            0x05    /*  Get current count value.                */
#define GPTU_CALCULATE_DIVIDER          0x06    /*  Calculate timer divider from given freq.*/
#define GPTU_SET_TIMER                  0x07    /*  Simplified method to setup timer.       */
#define GPTU_SET_COUNTER                0x08    /*  Simplified method to setup counter.     */

/*
 *  Data Type Used to Call ioctl
 */
struct gptu_ioctl_param {
    unsigned int                        timer;  /*  In command GPTU_REQUEST_TIMER, GPTU_SET_TIMER, and  *
                                                 *  GPTU_SET_COUNTER, this field is ID of expected      *
                                                 *  timer/counter. If it's zero, a timer/counter would  *
                                                 *  be dynamically allocated and ID would be stored in  *
                                                 *  this field.                                         *
                                                 *  In command GPTU_GET_COUNT_VALUE, this field is      *
                                                 *  ignored.                                            *
                                                 *  In other command, this field is ID of timer/counter *
                                                 *  allocated.                                          */
    unsigned int                        flag;   /*  In command GPTU_REQUEST_TIMER, GPTU_SET_TIMER, and  *
                                                 *  GPTU_SET_COUNTER, this field contains flags to      *
                                                 *  specify how to configure timer/counter.             *
                                                 *  In command GPTU_START_TIMER, zero indicate start    *
                                                 *  and non-zero indicate resume timer/counter.         *
                                                 *  In other command, this field is ignored.            */
    unsigned long                       value;  /*  In command GPTU_REQUEST_TIMER, this field contains  *
                                                 *  init/reload value.                                  *
                                                 *  In command GPTU_SET_TIMER, this field contains      *
                                                 *  frequency (0.001Hz) of timer.                       *
                                                 *  In command GPTU_GET_COUNT_VALUE, current count      *
                                                 *  value would be stored in this field.                *
                                                 *  In command GPTU_CALCULATE_DIVIDER, this field       *
                                                 *  contains frequency wanted, and after calculation,   *
                                                 *  divider would be stored in this field to overwrite  *
                                                 *  the frequency.                                      *
                                                 *  In other command, this field is ignored.            */
    int                                 pid;    /*  In command GPTU_REQUEST_TIMER and GPTU_SET_TIMER,   *
                                                 *  if signal is required, this field contains process  *
                                                 *  ID to which signal would be sent.                   *
                                                 *  In other command, this field is ignored.            */
    int                                 sig;    /*  In command GPTU_REQUEST_TIMER and GPTU_SET_TIMER,   *
                                                 *  if signal is required, this field contains signal   *
                                                 *  number which would be sent.                         *
                                                 *  In other command, this field is ignored.            */
};

/*
 * ####################################
 *              Data Type
 * ####################################
 */
#if defined(__KERNEL__)
    typedef void (*timer_callback)(unsigned long arg);
#endif  //  defined(__KERNEL__)


/*
 * ####################################
 *             Declaration
 * ####################################
 */

#if defined(__KERNEL__)
    extern int request_timer(unsigned int, unsigned int, unsigned long, unsigned long, unsigned long);
    extern int free_timer(unsigned int);
    extern int start_timer(unsigned int, int);
    extern int stop_timer(unsigned int);
    extern int get_count_value(unsigned int, unsigned long *);

    extern unsigned long cal_divider(unsigned long);

    extern int set_timer(unsigned int, unsigned int, int, int, unsigned int, unsigned long, unsigned long);
    extern int set_counter(unsigned int, int, int, int, unsigned int, unsigned int, unsigned long, unsigned long);
#endif  //  defined(__KERNEL__)


#endif  //  __DANUBE_GPTU_DEV_H__2005_07_26__10_19__
