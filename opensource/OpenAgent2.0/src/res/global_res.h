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
*	global_res.h - module routines for Agent
*
*
*	history: 2005-12-27, initial version by leon.liu
*
***********************************************************************/

#ifndef GLOBAL_RES_H_
#define GLOBAL_RES_H_

/***********************************************************************
*
*	include file
*
***********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <dirent.h>
#include "../list/ewt_list.h"
#include "../tools/logger.h"

/************************************************************************
*
*	struct define
*
************************************************************************/



/************************************************************************
*
*	macrodefinition
*
************************************************************************/

#define	SIG_ABORT   SIGABRT                /* The signal that indicates to disconnect */

#define	SIG_TIMEOUT SIGUSR1                /* The signal that indicates socket timeout */

#define DF_WORK_DIR "/tmp/"         /* The absolute path of agent working directory, this should be adapt to your system */

#define DF_CONF_DIR DF_WORK_DIR "conf/"             /* The path of directory /conf */


#define SUCCESS 0
#define FAIL    -1
#define FAIL_DL    -3

#define DEV_LENGTH   50

#define PATH_LEN         100
#define ACS_IP_LEN        33
#define ACS_PATH_LEN     100
#define HTTP_PORT_LEN    64

/*used by agent_conf.h*/
#define ACSURL_LEN   256
#define ACSUSER_LEN  256
#define ACSPWD_LEN  256
#define PERIODTIME_LEN 24
#define PK_LEN       33
#define CRS_USER_LEN  256
#define CRS_PWD_LEN   256
#define CK_LEN      33
/************************************************************************
*
*	global var define
*
************************************************************************/

sem_t SEM_SEND;               /* indicates that send_qbuf has data that should be sent out */

sem_t SEM_RECV;               /* indicates that recv_qbuf has received data that needed to be processed */

sem_t SEM_INFORM;             /* indicates that agent should send inform to acs now */

sem_t SEM_CONNECTED;         /* indicates that agent connected is over now*/

sem_t SEM_HANDLER_ABORT;    /* indicates that handler module is over now*/

pthread_t p_handler;              /* The id of thread p_handler */

pthread_t p_send;                 /* The id of thread p_send */

pthread_t p_recv;                 /* The id of thread p_recv */

int max_try_time;                 /* Times of max retry */

pthread_mutex_t try_time_lock;      /* Lock of try_time */

int retry_interval;                  /* Interval of try machenism */
int inform_retry;                   /* Times of inform retry */

/*The following are the path of all configure file*/

char conf_dir[PATH_LEN];

char dl_conf_path[PATH_LEN];

char rbc_conf_path[PATH_LEN];

char attri_conf_path[PATH_LEN];

char conn_req_conf_path[PATH_LEN];

char agent_conf_path[PATH_LEN];

char ca_cert_path[DEV_LENGTH];

char client_cert_path[DEV_LENGTH];

char client_key_path[DEV_LENGTH];

char log_file_path[PATH_LEN];

char log_file_bak[PATH_LEN];

char cli_conf_path[PATH_LEN];

char param_conf_path[PATH_LEN];

char acs_ip[ACS_IP_LEN];
char acs_path[ACS_PATH_LEN];

char http_port[HTTP_PORT_LEN];

/*dev function name array*/

//used in inform.c
char manu_dev_func[DEV_LENGTH];
char oui_dev_func[DEV_LENGTH];
char class_dev_func[DEV_LENGTH];
char serial_dev_func[DEV_LENGTH];

char wan_dev_func[DEV_LENGTH];

char pk_dev_func[DEV_LENGTH];

char get_upfig[DEV_LENGTH];      //upload
char get_uplog[DEV_LENGTH];

char first_install_func[DEV_LENGTH];   //init_event
char reboot_cmdkey_func[DEV_LENGTH]; //init_event

/*init.c(init_CRS)*/
char get_req_port_func[DEV_LENGTH]; 

char dev_lib_path[DEV_LENGTH];    //save dll lib name

/*download&tasklist*/
char dev_sysflashsizeget_func[DEV_LENGTH];
char dev_killallapps_func[DEV_LENGTH];
char dev_parseimagedata_func[DEV_LENGTH];

char dev_writestreamtoflash_func[DEV_LENGTH];
char dev_writestreamtoflash_web_func[DEV_LENGTH];
char dev_flashimage_func[DEV_LENGTH];
char dev_reboot_func[DEV_LENGTH];

char dev_sys_cmd[DEV_LENGTH];

/***********************************************************************
*
*	function declare
*
***********************************************************************/

int post_signal(pthread_t pth, int sig); 
int init_try_time();
int get_max_try_time();
void set_max_try_time(int time);
int reset_sys_sem();   //this func reset SEM_SEND and SEM_RECV
int init_conf_path();
time_t dev_get_cur_time();

#endif /* GLOBAL_RES_H_ */


