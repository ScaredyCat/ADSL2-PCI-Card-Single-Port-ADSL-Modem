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

#ifndef INFORM_H_
#define INFORM_H_
 
/***********************************************************************
*
*	include file
*
***********************************************************************/
#include "../../res/global_res.h"
#include "../soap/soap.h"
#include "../../event/event.h"
#include "../../CLI/CLI.h"
#include "methods.h"
#include "../../device/TRF_param.h"

/************************************************************************
*
*	global var define
*
************************************************************************/

int inform_flag;                             // 0 -- inform unsuccessful  1 -- inform successful

/************************************************************************
*
*	macrodefinition
*
************************************************************************/
#define SPECVERSION                  "InternetGatewayDevice.DeviceInfo.SpecVersion"
#define HARDWAREVERSION            "InternetGatewayDevice.DeviceInfo.HardwareVersion"
#define SOFTWAREVERSION             "InternetGatewayDevice.DeviceInfo.SoftwareVersion"
#define PROVISIONINGCODE            "InternetGatewayDevice.DeviceInfo.ProvisioningCode"
#define CONNECTIONREQUESTURL      "InternetGatewayDevice.ManagementServer.ConnectionRequestURL"
#define PARAMTERKEY                 "InternetGatewayDevice.ManagementServer.ParameterKey"

#define NUM_PARAM     7
#define EVENT_LEN        256
#define TIMEBUF_LEN      20 
#define PARAM_LEN        256
#define WAN_PARAM_LEN  100
#define MANU_LEN        65
#define OUI_LEN          7
#define CLASS_LEN	 65
#define SERIAL_LEN       65
#define TYPE_CHAR_LEN   256

#define EVENT_CODE_LEN   65
#define CMDKEY_LEN       33

/************************************************************************
*
*	structure define
*
************************************************************************/
 
typedef struct deviceidstruct           //DeviceIdStruct
{
    char   manufacturer[MANU_LEN];            // string(64) 
    char   oui[OUI_LEN];                               // string(6) 
    char   product_class[CLASS_LEN];           // string(64) 
    char   serial_number[SERIAL_LEN];          // string(64) 
}TR_dev_id_struct;

typedef struct eventstruct            //EventStruct
{
    char   event_code[EVENT_CODE_LEN];                 // string(64) 
    char   command_key[CMDKEY_LEN];            // string(32) 
}TR_event_struct;

typedef struct arrayofeventstruct     
{
    TR_event_struct   *_ptr;
    int   _size;
}TR_arr_event_struct;

typedef union anytype
{
    char  chararg[TYPE_CHAR_LEN];                // string 
    int    intarg;                           // int 
    unsigned int   uintarg;            // unsigned int 
    time_t    timearg;                  // dateTime 
    int boolean;                     //boolean
   // base
}TR_anytype;

typedef struct paravalstruct          //ParameterValueStruct
{
    char   name[PARAM_LEN+1];                  //  string(256)  
    TR_anytype     value;             //  anytpe       
    char           type;                   //  s -- string , i -- int , u -- unsignedint , t -- datetime
                                     //    (example : if the datatype is string, the type value is s)
}TR_para_val_struct;

typedef struct arrayofparaValstruct    //ArrayOfParameterValueStruct
{
    TR_para_val_struct   *_ptr;
    int                  _size;
}TR_arr_para_val_struct;

typedef struct inform
{
	TR_dev_id_struct      device_id;	        //DeviceIdStruct
	TR_arr_event_struct   event; 	        //EventStruct [16]
	unsigned int   max_envelopes;	        // unsignedInt
	time_t   cur_time;	                             //dateTime 
	unsigned int   retry_count;	               // unsignedInt 
       TR_arr_para_val_struct   para_list;	 // ParameterValueStruct[] 
}TR_inform;

/***********************************************************************
*
*	function declare
*
***********************************************************************/
int process_inform_resp(TRF_node *method);
int init_inform();

#endif  /* INFORM_H_ */

