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
 * Include necessary headers...
 */
 
#include "methods.h"
#include "../handler.h"

/*
 *************************************************************************************
 * Function name: gen_tran_comp_argument
 * Description: generate tranfer completer argument
 * Parameter: TRF_node *method
 *            TR_tran_comp *tran
 * Return Value:
 *     sucess return 0, else return -1
 ************************************************************************************
 */

static int gen_tran_comp_argument(TRF_node *method, TR_tran_comp *tran)
{
    TRF_node *ck = NULL, *fs = NULL, *fc = NULL, *fst = NULL, *st = NULL, *ct = NULL;
    char timebuf[20];
    
    // Add commandkey node
    ck = mxmlNewElement(method, "CommandKey");
    if (!ck) {
        return -1;
    }
    if (mxmlNewText(ck, 0, tran->command_key) == NULL) {
        return -1;
    }
    
    //Add fault struct node
    fs = mxmlNewElement(method, "FaultStruct");
    if (!fs) {
        return -1;
    }
    
    fc = mxmlNewElement(fs, "FaultCode");
    if (!fc) {
        return -1;
    }
    if (mxmlNewInteger(fc, tran->fault_struct.fault_code) == NULL) {
        return -1;
    }
    
    fst = mxmlNewElement(fs, "FaultString");
    if (!fst) {
        return -1;
    }
    if (mxmlNewText(fst, 0, tran->fault_struct.fault_string) == NULL) {
        return -1;
    }
    
    //Add start time node
    st = mxmlNewElement(method, "StartTime");
    if (!st) {
        return -1;
    }
    format_time(tran->start_time, timebuf);
    if (mxmlNewText(st, 0, timebuf) == NULL) {
        return -1;
    }
    
    //Add complete time node
    ct = mxmlNewElement(method, "CompleteTime");
    if (!ct) {
        return -1;
    }
    format_time(tran->start_time, timebuf);
    if (mxmlNewText(ct, 0, timebuf) == NULL) {
        return -1;
    }

    return 0;
}

/*
 ******************************************************************************
 * Function name: gen_tran_comp_method
 * Description: generate tranfer complete method
 * Parameter: TR_tran_comp *tran
 *            TRF_node *xmlroot
 * Return Value:
 *     MTH_RET_SUCCESS - success
 *     MTH_RET_FAILED  - failed
 ******************************************************************************
 */
 
int gen_tran_comp_method(TR_tran_comp *tran, TRF_node *xmlroot)
{
    char name[] = "TransferComplete";
    TRF_node *method = NULL;
    int ret = 0;
    
    //init header element value
    have_soap_head = 1;
    strcpy(header_id_val, TRAN_COMP_ID);
    
    //generate soap frame
    method = gen_soap_frame(name);
    if (method == NULL) {
        return MTH_RET_FAILED;
    }

    //generate method argument
    ret = gen_tran_comp_argument(method, tran);
    if (ret != 0) {
        LOG(m_handler, ERROR, "generate transfercomplete argument failed\n");
        return MTH_RET_FAILED;
    }
    
    return MTH_RET_SUCCESS;
}

/*
 ********************************************************************************
 * Function name:process_tran_comp_resp(TRF_node *method)
 * Description: process tranfer complete response
 * Parameter:
 *     TRF_node *method
 * Return Value:
 *     MTH_RET_SUCCESS - success
 *     MTH_RET_FAILED  - failed
 *******************************************************************************
 */
int process_tran_comp_resp(TRF_node *method)
{
    TR_cpe_req_list *req_list = NULL;
    
    if (strcmp(header_id_val, TRAN_COMP_ID)) {
        LOG(m_handler, ERROR, "ID value is error in transfercompleter response\n");
        return MTH_RET_FAILED;
    }
    
    have_soap_head = 0;
    strcpy(header_id_val, "");
    
    //Find TransferComplete node in request list
    req_list = search_req_node("TransferComplete");
    if (req_list == NULL) {
        LOG(m_handler, ERROR, "Can't Find TransferComplete node\n");
        return MTH_RET_FAILED;
    }
    
    //delete transfer complete from request list
    del_req(req_list);
    LOG(m_handler, DEBUG, "Delete transfercomplete req_list successfully\n");
    
    return MTH_RET_SUCCESS;
}

