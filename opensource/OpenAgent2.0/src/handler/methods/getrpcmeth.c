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
 ******************************************************************************
 * 
 * $Author: andyy $
 * $Date: 2007-06-08 02:31:28 $
 *
 *****************************************************************************
 */

#include "../soap/soap.h"
#include "../xmltool/mxml.h"
#include "../../tools/logger.h"
#include "methods.h"

#define VENDOR
typedef struct MethodList {
    char *Method[20];
    int size;
}TR_MethodListStruct;

/*
 ******************************************************************************
 * gen_MethodList function: generate MethodList xml code
 * arguments:
 *    MethodList--a pointer point to TR_MethodListStruct
 *    method -----xml tree node, methods new node's hung node
 * return value
 *    xml node hung node
 ****************************************************************************** 
 */

static TRF_node *gen_MethodList(TR_MethodListStruct *MethodList, TRF_node *method)
{
    int i;
    char arraytype[257];
    TRF_node *ML, *Method;

#ifdef VENDOR
    if (m11)
        MethodList->size++;
    if (m12)
        MethodList->size++;
    if (m13)
        MethodList->size++;
    if (m14)
        MethodList->size++;
    if (m15)
        MethodList->size++;
    if (m16)
        MethodList->size++;
    if (m17)
        MethodList->size++;
    if (m18)
        MethodList->size++;
    if (m19)
        MethodList->size++;
#endif   

    sprintf(arraytype, "cwmp:string[%d]", MethodList->size);
    ML = mxmlNewElement(method, "MethodList");
    if (ML == NULL) {
        return NULL;
    }
    mxmlElementSetAttr(ML, "xsi:type", "SOAP-ENC:Array");
    mxmlElementSetAttr(ML, "SOAP-ENC:arrayType", arraytype);
   
    for (i =0; i < MethodList -> size; i++) {
        Method = mxmlNewElement(ML, "string");
        if (mxmlNewText(Method, 0, MethodList -> Method[i]) == NULL) {
            return NULL;
        }
    }

    return ML;
}

/*
 ******************************************************************************
 * gen_getprc_method function: generate MethodList soap code
 * arguments:
 *    MethodList--a pointer point to TR_MethodListStruct
 *    method -----xml tree root node
 * return value
 *    soap code create success return 0, else return -1
 *****************************************************************************
 */              

static int gen_getrpc_method(TR_MethodListStruct *MethodList, TRF_node *method)
{
    char name[] = "GetRPCMethodsResponse";
    //TRF_node *ENV, *soap_body;

    /*if ((ENV = gen_env(xmlroot)) == NULL) {
        return -1;
    }
    gen_soap_header(ENV);
    if ((soap_body = gen_soap_body(ENV)) == NULL) {
         return -1;
    }
    if ((method = gen_method_name(name, soap_body)) == NULL) {
         return -1;
    }*/
    //generate soap frame
    method = gen_soap_frame(name);
    if (method == NULL) {
        return -1;
    }

    if ( gen_MethodList(MethodList, method) == NULL) {
        LOG(m_handler, ERROR, "add RPC methodlist error\n"); 
        return -1;
    }   
    return 0;
}
/*
 ******************************************************************************
 * process_getrpc function: add message to the TR_MethodListStruct  structure
 * arguments: 
 *    method -----xml tree node, methods new node's hung node
 * return value
 *    sucess return 0, else return -1
 ******************************************************************************
 */

int process_getrpc(TRF_node *method)
{
    TR_MethodListStruct MethodList; 
  
    MethodList.size = 11;
    MethodList.Method[0] = m0;
    MethodList.Method[1] = m1;
    MethodList.Method[2] = m2;
    MethodList.Method[3] = m3;
    MethodList.Method[4] = m4;
    MethodList.Method[5] = m5;
    MethodList.Method[6] = m6;
    MethodList.Method[7] = m7;
    MethodList.Method[8] = m8;
    MethodList.Method[9] = m9;
    MethodList.Method[10] = m10;

#ifdef VENDOR
    if (m11)
        MethodList.Method[11] = m11;
    if (m12)
        MethodList.Method[12] = m12;
    if (m13)
        MethodList.Method[13] = m13;
    if (m14)
        MethodList.Method[14] = m14;
    if (m15)
        MethodList.Method[15] = m15;
    if (m16)
        MethodList.Method[16] = m16;
    if (m17)
        MethodList.Method[17] = m17;
    if (m18)
        MethodList.Method[18] = m18;
    if (m19)
        MethodList.Method[19] = m19;
#endif   
                           
    if (gen_getrpc_method(&MethodList, method) == -1) {
        LOG(m_handler, ERROR, "generate rpcmethods xml date failure\n");
        return -1;
    }
   
    return 0;
}

