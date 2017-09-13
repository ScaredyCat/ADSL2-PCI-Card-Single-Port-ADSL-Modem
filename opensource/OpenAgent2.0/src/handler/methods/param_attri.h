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

/*
 * Prevent multiple inclusion...
 */
#ifndef PARAM_ATTRI_H_
#define PARAM_ATTRI_H_

/***********************************************************************
*
*	include file
*
***********************************************************************/

#include "../soap/soap.h"
#include "../../res/global_res.h"
#include "methods.h"
#include "../../device/TRF_param.h"
//char wan_para[128];
/************************************************************************
*
*	macrodefinition
*
************************************************************************/

/*#define ATTRI_CONF_INITSIZE 4

#define DEVICEINFOSOFTWAREVERSION  "InternetGatewayDevice.DeviceInfo.SoftwareVersion"
#define DEVICEINFOPROVISIONINGCODE "InternetGatewayDevice.DeviceInfo.ProvisioningCode"
#define MANAGEMENTSERVERCONREQURL  "InternetGatewayDevice.ManagementServer.ConnectionRequestURL"*/
//#define WANDEVICEIPEXTERNALADDRESS   "InternetGatewayDevice.WANDevice.1.WANConnectionDevice.1.WANIPConnection.1.ExternalIPAddress"
//#define WANDEVICEPPPEXTERNALADDRESS   "InternetGatewayDevice.WANDevice.1.WANConnectionDevice.1.WANPPPConnection.1.ExternalIPAddress"

#define INTERNETGATEWAYDEVICE "InternetGatewayDevice."
 
#define SETPARAATTR    "cwmp:SetParameterAttributes"
#define GETPARAATTR    "cwmp:GetParameterAttributes"

/****FAULT CODE*******/
#define PARAM_ATTRI_BASE                          8100
#define PROCESS_ATTRI_CONF_ERROR                 PARAM_ATTRI_BASE+1
#define GEN_XML_FAIL                               PARAM_ATTRI_BASE+2 
#define PARSE_SET_ARGUMENT_ERROR                PARAM_ATTRI_BASE+3

/*****RETURN CODE*******/
#define NOT_IN_CONF   -2
#define CANT_BE_SET   -1
#define GEN_FAULT_CODE  1

#define F_SET  0
#define F_GET  1

#define NAME_LEN        257
#define ACCESSLIST_LEN   65

#define NOT_CHANGE_CONF  0
#define NEED_CHANGE_CONF 1 

/************************************************************************
*
*	structure define
*
************************************************************************/

typedef struct setparaattrstruct
{
    char   name[NAME_LEN];
    int    noti_change;
    int    notification;
    int    access_list_change;
    char   accesslist[ACCESSLIST_LEN];
}TR_set_para_attr;

typedef struct 
{
    char   name[NAME_LEN];
    int    notification;
  //  int    accesslist_num;
    char   accesslist[ACCESSLIST_LEN];
}TR_param_attr;
  

/***********************************************************************
*
*	function declare
*
***********************************************************************/
int process_set_para_attr(TRF_node *method);

int process_get_para_attr(TRF_node *method);

int init_attr();

#endif /*PARAM_ATTRI_H_*/

