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
*	CLI.h - Command Line Interface
*
*	$Author: joinsonj $
*
*	history: $Date: 2007-06-08 02:22:57 $, initial version by simonl
*
***********************************************************************/

#ifndef CLI_H_
#define CLI_H_

/***********************************************************************
*
*	include file
*
***********************************************************************/

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "../res/global_res.h"
#include "../event/event.h"
#include "../device/TRF_param.h"
#include "../handler/methods/methods.h"
#include "../tools/agent_conf.h"
#include "../handler/methods/object.h"
#include "../handler/soap/soap.h"

/************************************************************************
*
*	structure define
*
************************************************************************/

typedef struct changed_param_name_list 
{
    char name[256]; 
    struct list_head node;
}TR_changed_param_name_list;

/************************************************************************
*
*	macrodefinition
*
************************************************************************/

#define CONN_REQ "0"                                       /* MQ flag for connection request */ 

#define ACL_CHANGED "1"                                 /* MQ flag for ACS URL changed */ 

#define VAL_CHANGED "2"                                 /* MQ flag for value changed */ 

#define SET_LOGGER "3"                                    /* MQ flag for setting level and mode */ 

#define SET_RETRY_TIMES "4"                                 /* MQ flag for setting timer_iv */ 

#define SET_PERIOD_IV "5"                               /* MQ flag for setting period_iv */

#define ADD_OBJ "6"                                     /* MQ flag for add object*/

#define DEL_OBJ "7"                                     /* MQ flag for del object*/

#define SET_PERIOD_ENABLE "8"                           /* MQ flag for periodic inform enable */

#define SET_PERIOD_TIME "9"                             /* MQ flag for period inform time */

#define PROJID 0xFF

#define MAX_READ_SIZE 256
#define MAX_SEND_SIZE 256

#define CLI_PORT 1234    /* The port tr069_cli will be connected to */
#define BACKLOG 1        /* how many pending connections queue will hold */

/************************************************************************
*
*	global var define
*
************************************************************************/

struct list_head changed_param_name_list_head;     /* The head of changed_param_name_list */

pthread_mutex_t changed_param_name_list_lock;     /* The lock of changed_param_name_list */


/***********************************************************************
*
*	function declare
*
***********************************************************************/
int init_CLI();
int init_changed_param_name_list();

int cli_conn_req();

int display_changed_param_name_list();

int flush_changed_param_list();

int add_multi_obj(char *objname, int instance_num);

int del_multi_obj(char *objname);

#endif /* CLI_H_ */
