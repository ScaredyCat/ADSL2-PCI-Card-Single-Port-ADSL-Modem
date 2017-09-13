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
 * 
 */

#ifndef SOAP_H_
#define SOAP_H_

/*
 * Include head file......
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../xmltool/mxml.h"
#include "../xmltool/config.h"
#include "../../tools/logger.h"
#include "../../auth/auth.h"

/*
 * Define SOAP URI
 */

#define ENVURI "http://schemas.xmlsoap.org/soap/envelope/"
#define ENCURI "http://schemas.xmlsoap.org/soap/encoding/"
#define XSIURI "http://www.w3.org/2001/XMLSchema-instance"
#define XSDURI "http://www.w3.org/2001/XMLSchema"
#define CWMPURN  "urn:dslforum-org:cwmp-1-0"
#define ENSTY  "http://schemas.xmlsoap.org/soap/encoding/"

/*
 * Define Header ID value ......
 */

#define HEADER_ID_LEN 256
#define INFORM_ID     "inform"
#define TRAN_COMP_ID  "transfercomplete"

/*
 * Define falut code
 */

#define MTHD_NOT_SUPPORT     9000
#define REQUEST_DENY         9001
#define INTERNAL_ERROR       9002
#define INVALID_ARGUMENT     9003
#define RESOURCE_EXCEED      9004
#define INVALID_PARAM_NAME   9005
#define INVALID_PARAM_TYPE   9006
#define INVALID_PARAM_VAL    9007
#define SET_NOT_WRITEABLE    9008
#define NOTIFY_REQ_REJECT    9009
#define DOWNLOAD_FAILURE     9010
#define UPLOAD_FAILURE       9011
#define FILE_TRAN_AUTH_FAIL  9012
#define UNSUPPORT_PROTOCOL   9013

#define UNKNOW_MTHD          9800
#define TOO_MANY_ENV         9801
#define STR_TOO_LONG         9802
#define INVALID_TIME_ZONE    9803

#define RETRY_REQUEST        8005


/* Fault code description */
#define MSG9000  "Method not supported"
#define MSG9001  "Request denied"
#define MSG9002  "Internal error"
#define MSG9003  "Invalid arguments"
#define MSG9004  "Resources exceeded"
#define MSG9005  "Invalid parameter name"
#define MSG9006  "Invalid parameter type"
#define MSG9007  "Invalid parameter value"
#define MSG9008  "Attempt to set non_writable parameter"
#define MSG9009  "notification request rejected"
#define MSG9010  "Download failure"
#define MSG9011  "Upload failure"
#define MSG9012  "File Transfer server authentication failure"
#define MSG9013  "Unsupported protocol for file transfer"

#define MSG9800  "Unknow method in envelop"
#define MSG9801  "Too many envelopes"
#define MSG9802  "String too long"
#define MSG9803  "Not a valid time zone value"


#define SOAP_FAULT_CODE  "Client"
#define SOAP_FAULT_STRING "CWMP fault"
#define CWMPFAULT         "cwmp:Fault"

/*
 * Define return values ....
 */
#define SOAP_RET_SUCCESS 0
#define SOAP_RET_FAILED  -1


/*
 * Define fault struct
 */

#define MAX_FAULT_SIZE   18
#define FAULT_STRING_LEN 256
typedef struct {
    unsigned int fault_code;
    char         fault_string[FAULT_STRING_LEN + 1];
}TR_fault_struct;

/*
 * Define soap Header variable
 */

unsigned short have_soap_head;                     // 0 -- No soap header, 1 -- Have soap header
unsigned short hold_req_val;                       // 0 -- Hold request is false, 1 -- Hold request is true
unsigned short acs_no_more_req_val;                // 0 -- ACS have request to cpe, 1 -- ACS no more request to cpe
unsigned short cpe_no_more_req_val;                // 0 -- CPE have request to ACS, 1 -- CPE no more request to ACS
char  header_id_val[HEADER_ID_LEN + 1];            // The value of ID in soap header

/*
 * Define CPE and ACS MAXENVELOPE
 */

#define CPE_MAX_ENVELOPES  1                  // Max envelopes CPE can handle
unsigned int ACS_MAX_ENVELOPES;               // Max envelopes ACS can handle

/*
 * Define Envelope Number
 */
 
int cpe_env_num;             //Record Envelope number in one http package

/*
 * Define xml node type
 */
 
typedef mxml_node_t TRF_node;


typedef struct {
    TRF_node *head;
    TRF_node *method;
}method_array;

method_array p[CPE_MAX_ENVELOPES + 1];

/*
 * Define root node
 */

TRF_node *xmlroot;         //root node for send
TRF_node *xmltop;          //root node for recv

/*
 * Define generate xml Function ......
 */

extern TRF_node *gen_xml_header();
extern TRF_node *gen_env(TRF_node *xmlroot);
extern TRF_node *gen_soap_header(TRF_node *ENV);
extern TRF_node *gen_soap_body(TRF_node *ENV);
extern TRF_node *gen_method_name(char *name, TRF_node *soap_body);
extern TRF_node *gen_soap_frame(char *method_name);
extern int save_to_sendbuf(TRF_node *xmlroot, char *sendbuf);

extern int format_time(time_t temp, char *tm);
extern time_t strtimetosec(char *strtm);

extern TRF_node * gen_soap_fault(TRF_node *soap_body);
extern TRF_node * gen_cwmp_fault(TRF_node *deta, unsigned int code, char *methname);
extern TRF_node * gen_method_fault(TRF_node *xmlroot, char *methname, unsigned int fault_code);
extern TRF_node * gen_para_fault(TRF_node *cwmpf, char *meth_name, char *para_name, unsigned int fault_code);
extern int get_fault_string(int fault_code, char *fault_string);

/*
 * Define parse function
 */
extern int get_head_method(char *recvbuf);
extern void process_soap_head(TRF_node *head);
extern int process_method(TRF_node *method);

#endif  /* SOAP_H_ */

