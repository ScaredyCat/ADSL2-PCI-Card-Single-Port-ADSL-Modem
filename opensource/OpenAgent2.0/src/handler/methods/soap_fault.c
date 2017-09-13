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
 * Include files .....
 */
#include "../soap/soap.h"
#include "../handler.h"
/*
 ***************************************************************
 * Function: process_soap_fault
 * Description: process the soap fault from ACS
 * Parameter: TRF_node *method
 * Return Value:
 *     MTH_RET_SUCCESS - success
 *     MTH_RET_FAILED  - failed
 ***************************************************************
 */
int process_soap_fault(TRF_node *method)
{
    TRF_node *node;
    int ret, fault_code;
    TR_cpe_req_list *req_list = NULL;

    //get fault code
    node = mxmlFindElement(method, xmltop, "FaultCode", NULL, NULL, MXML_DESCEND);
    if (node == NULL) {
        LOG(m_handler, ERROR, "Can't FInd Fault Code\n");
        return MTH_RET_FAILED;
    }
    if (node->child == NULL) {
        LOG(m_handler, ERROR, "Fault Code is NULL\n");
        return MTH_RET_FAILED;
    }
    fault_code = atoi(node->child->value.opaque);
    LOG(m_handler, DEBUG, "Fault Code: %d\n", fault_code);

    if (fault_code == RETRY_REQUEST) {
        //init global variable value
        have_soap_head = 0;
        strcpy(header_id_val, "");
        //process inform and transfer complete fault response
        if (!strcmp(header_id_val, INFORM_ID)) {
            LOG(m_handler, DEBUG, "Inform fault response\n");
            inform_retry++;
            if (inform_retry >= max_try_time) {
                LOG(m_handler, ERROR, "Retry times is full\n ");
                inform_retry = 0;
                return MTH_RET_FAILED;
            }
            ret = init_inform();
            if (ret != MTH_RET_SUCCESS) {
                return MTH_RET_FAILED;
            }
        } else if (!strcmp(header_id_val, TRAN_COMP_ID)){
            LOG(m_handler, DEBUG, "Transfer complete fault response");
            //Find transfercomplete node in request list
            req_list = search_req_node("TransferComplete");
            if (req_list == NULL) {
                LOG(m_handler, ERROR, "Can't Find TransferComplete node\n");
                return MTH_RET_FAILED;
            }
            //check retry count
            if (req_list->retry_count >= max_try_time) {
                LOG(m_handler, DEBUG, "Transfer request Retry times full\n");
                //Delete node from request list
                del_req(req_list);
                LOG(m_handler, DEBUG, "Delete transfer success\n");
            } else {
                req_list->retry_count++;
            }
        } else {
            LOG(m_handler, DEBUG, "UNKNOW SOAP Header\n");
            return MTH_RET_FAILED;
        }
    } else {
        return MTH_RET_FAILED;
    }

    return MTH_RET_SUCCESS;
}


