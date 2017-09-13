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
*	tr069_cli.h - Event module
*
*	$Author: joinsonj $
*
*	history: $Date: 2007-06-08 07:39:02 $, initial version by simonl
*
***********************************************************************/


/***********************************************************************
*
*	include file
*
***********************************************************************/
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <sys/msg.h>
#include <error.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>

/************************************************************************
*
*	macrodefinition
*
************************************************************************/
#define MAX_SEND_SIZE      256
#define SNDMSG              1
#define PROJID             0xFF
#define CLI_PORT           1234

#define MAX_SEND_SIZE      256
#define SNDMSG              1
#define PROJID             0xFF
#define CLI_PORT           1234

#define  REQURL    "InternetGatewayDevice.ManagementServer.ConnectionRequestURL"
#define  REQPWD    "InternetGatewayDevice.ManagementServer.ConnectionRequestPassword"
#define  REQUSER   "InternetGatewayDevice.ManagementServer.ConnectionRequestUsername"
#define  ACSPWD    "InternetGatewayDevice.ManagementServer.Password"
#define  ACSUSER   "InternetGatewayDevice.ManagementServer.Username"
#define  AGENT_CONF_PATH  "/etc/conf/agent.conf"
#define ACSURL_LEN   256
#define ACSUSER_LEN  256
#define ACSPWD_LEN  256
#define PERIODTIME_LEN 24
#define PK_LEN       33
#define CRS_USER_LEN  256
#define CRS_PWD_LEN   256
#define CK_LEN      33

#define ACS_URL "http://172.31.0.56/comserver/node1/tr069"
#define ACS_USERNAME "012345678912"
#define ACS_PASSWORD "TR069"
#define PERIODIC_INFORM_ENABLE 1
#define PERIODIC_INFORM_INTERVAL 300
#define PERIODIC_INFORM_TIME "0000-00-00T00:00:00"
#define PARAMETERKEY ""
#define CONN_REQ_SER_PORT 8099
#define CONN_REQ_SER_USERNAME "admin"
#define CONN_REQ_SER_PASSWORD "admin"
#define COMMAND_KEY ""
#define RETRY_TIMES 3
#define FLAG_REBOOT 0
#define SUCCESS 0
#define FAIL  -1

/************************************************************************
*
*	structure define
*
************************************************************************/
typedef struct {
    unsigned char acs_url[ACSURL_LEN];
    unsigned char acs_username[ACSUSER_LEN];
    unsigned char acs_password[ACSPWD_LEN];
    unsigned short periodic_inform_enable;
    unsigned int periodic_inform_interval;
    unsigned char periodic_inform_time[PERIODTIME_LEN];
    unsigned char parameterkey[PK_LEN];
    unsigned int conn_req_ser_port;             // 0 ~ 65535
    unsigned char conn_req_ser_username[CRS_USER_LEN];
    unsigned char conn_req_ser_password[CRS_PWD_LEN];
    unsigned int retry_times;
    unsigned char command_key[CK_LEN];
    unsigned int flag_reboot;
}agent_conf;
