/*
 * Copyright(c) 2006-2007, Works Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution. 
 * 3. Neither the name of the vendors nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**********************************************************************
*
*	event.h - Event module
*
*	$Author: joinsonj $
*
*	history: $Date: 2007-06-08 02:23:22 $, initial version by simonl
*
***********************************************************************/


#ifndef EVENT_H_
#define EVENT_H_
/***********************************************************************
*
*	include file
*
***********************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include "../res/global_res.h"
#include "../comm/comm.h"
#include "../handler/handler.h"
#include "../tools/agent_conf.h"
#include "../handler/soap/soap.h"

/************************************************************************
*
*	macrodefinition
*
************************************************************************/
#define EVENT_CODE_LEN   65
#define CMDKEY_LEN       33

#define DF_TIMER_IV 10
#define DF_PERIOD_IV 1800

#define PERIOD_ENABLE 1
#define PERIOD_TIME_CHANGE 1
#define PERIOD_TIME_NOT_CHANGE 0

#define PERIODINTERVAL   "InternetGatewayDevice.ManagementServer.PeriodicInformInterval"
#define PERIODTIME        "InternetGatewayDevice.ManagementServer.PeriodicInformTime"
#define PERIODENABLE     "InternetGatewayDevice.ManagementServer.PeriodicInformEnable"

#define NEED_REBOOT 1
#define NOT_NEED_REBOOT 0

/************************************************************************
*
*	structure define
*
************************************************************************/

typedef struct event_list 
{
    char event_code[EVENT_CODE_LEN]; 
    char cmd_key[CMDKEY_LEN]; 
    int sent_flag;             /* 1 means this node has been sent in inform */
    struct list_head node;     /* use linux kernel link list which is defined in file linux/list.h */
}TR_event_list;


/************************************************************************
*
*	global var define
*
************************************************************************/

struct list_head event_list_head;      /* The head of event_list */

pthread_mutex_t event_list_lock;       /* The lock of event_list */
pthread_mutex_t event_session_lock;

pthread_mutex_t g_period_enable_lock;
pthread_mutex_t g_period_time_lock;
pthread_mutex_t g_period_iv_lock;
pthread_mutex_t flag_periodtime_change_lock;

time_t g_first_time;           /*time of first inform because of periosdic*/
unsigned int g_period_iv;             /* The interval of periodic informs */

int g_period_enable;          /* whether or not  periodic to call inform method*/

time_t g_period_time;         /* periodic inform time */

int flag_periodtime_change;
int flag_reboot;

char reboot_cmd_key[CMDKEY_LEN];

/***********************************************************************
*
*	function declare
*
***********************************************************************/
int init_event();

int init_first_time();
int init_timer();

int set_timer(int interval);

void set_flag_periodtime_change(int flag);
int get_flag_periodtime_change();

/* About period_iv */
void set_g_period_iv(unsigned int iv);

unsigned int get_g_period_iv();

/* About periodic inform enable */

int get_g_period_enable();
void set_g_period_enable(int enable);

void set_g_period_time(time_t temp);
time_t get_g_period_time();

int event_session();

/* About event_list */
int init_event_list();

int destroy_event_list();

int modify_event_list();

int add_event(char event_code[], char cmd_key[]);

int del_event(char event_code[]);

int check_event();

//int dev_dll_func(char *buf, char dev_fun_name[]);//one parameter, char[]

int dev_func(char dev_func_name[]);//no parameter, first_install and reboot

int init_iv();

#endif /* EVENT_H_ */

