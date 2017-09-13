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
 * Include necessary file ......
 */
#include "soap.h"
#include "../methods/methods.h"
#include "../methods/inform.h"
#include "../methods/object.h"
#include "../methods/param_attri.h"
#include "../methods/upload.h"
#include "../methods/download.h"

/*
 * Declear function.....
 */

 
/*
 *****************************************************************************
 * Function name: get_head_method()
 * Description:  get the soap head and method's pointer in xml
 * Param: char *recvbuf point to the string of recv buffer
 * Return Value:
 *     success return SUCCESS, else return FAIL
 *****************************************************************************
 */
int get_head_method(char *recvbuf)
{
    int i;
    TRF_node *node; 
    
    //init the method_array
    for (i = 0; i < CPE_MAX_ENVELOPES + 1; i++) {
       p[i].head = NULL;
       p[i].method = NULL;
    }

    //find xml data
    char *q = NULL;
    q = strstr(recvbuf, "<?xml");
    if (q == NULL) {
        //LOG(m_handler, ERROR, "XML FORMAT ERROR: Can't Find <?xml>\n");
        q = strstr(recvbuf, "<SOAP-ENV:Envelope");
        if (q == NULL) {
            LOG(m_handler, ERROR, "XML FORMAT ERROR: Can't Find <?xml>\n");
            return FAIL;
        }
        strcpy(recvbuf, "<?xml version=\"1.0\" ?>");
        strcat(recvbuf, q);
        q = recvbuf;    
    }
    
    //load the data to a tree
    xmltop = mxmlLoadString(NULL, q, MXML_OPAQUE_CALLBACK);
    if (xmltop == NULL) {
       LOG(m_handler, ERROR, "mxmlloadstring: unable get enough memory\n");
       return FAIL;
    }
    
    //pointer to the xml root node
    node = xmltop;
    
    //This loop for get head and method pointer
    for (i = 0; i < CPE_MAX_ENVELOPES; i++) {
        //Get envelope node
        node = mxmlFindElement(node, xmltop, ":Envelope", NULL, NULL, MXML_DESCEND);
        if (node == NULL) {
            break;
        }

        //Point to the child node
        node = node->child;
        
        while (node != NULL) {
            if (strstr(node->value.element.name, ":Header")) {
                p[i].head = node;
                node = node->next;
                LOG(m_handler, DEBUG, "%s\n", node->value.element.name);
                continue;
            }
            if (strstr(node->value.element.name, ":Body")) {
                node = node->child;
                continue;
            }
            if (strstr(node->value.element.name, ":")) {
                p[i].method = node;
                LOG(m_handler, DEBUG, "%s\n", node->value.element.name);
                break;
            }
            //for comment node in xml
            node = node->child;
        }
    }

    return SUCCESS;
}

/*
 *******************************************************************
 * Function name: process_soap_head()  
 * Description: process the soap header in xml
 * Param: TRF_node *head which point to the soap header in xml
 * Return Value:
 *      void
 *******************************************************************
 */

void process_soap_head(TRF_node *head)
{
    //init soap header
    have_soap_head = 0;
    strcpy(header_id_val, "");
    hold_req_val = 0;
    acs_no_more_req_val = 0;
    
    //check header and get header value
    if (head != NULL) {
	have_soap_head = 1;
	LOG(m_handler, DEBUG, "Have SOAP Header\n");
	head = head->child;
	
	//parse header
        while (head != NULL) {
	    if (strstr(head->value.element.name, ":ID")) {
                if (head->child != NULL) {
		    strcpy(header_id_val, head->child->value.opaque);
                }
	    } else if (strstr(head->value.element.name, ":HoldRequests")) {
                if (head->child != NULL) {
                    hold_req_val = atoi(head->child->value.opaque);
                }
	    } else if (strstr(head->value.element.name, ":NoMoreRequests")) {
                if (head->child != NULL) {
		    acs_no_more_req_val = atoi(head->child->value.opaque);
                }
	    }
            //LOG(m_handler, DEBUG, "%s\n", head->child->value.opaque);
	    head = head->next;
	}
    }
}

/*
 ********************************************************************
 * Function process_method()
 * Description:   process TR-069 method
 * Param: RF_node *head which point to the TR-069 method in xml
 * Return Value:
 *   success return SUCCESS, method pointer is NULL return FAIL.
 ********************************************************************
 */
 
int process_method(TRF_node *method)
{   
    char *method_name;
    int res = FAIL;

    if (method == NULL) {
        return res;
    }
    
    //Get the method name
    method_name = strstr(method->value.element.name, ":");
    if (method_name == NULL){
        return res;
    }
    method_name++;
    LOG(m_handler, DEBUG, "Method name : %s\n", method_name);

    //parse request and generate response
    if (!strcmp(method_name, "InformResponse")) {
        if (process_inform_resp(method) == SUCCESS) {
            res = SUCCESS;
        }
    } else if (!strcmp(method_name, "Download")) {
        if (process_dl(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
    } else if (!strcmp(method_name, "SetParameterValues")) {
        //add by frankj
        pthread_mutex_lock(&tree_lock);
        //end by frankj
        LOG(m_handler, DEBUG, "PROCESS set parameter value\n");
        if (process_set_param_val(method) == MTH_RET_SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
        //add by frankj
        pthread_mutex_unlock(&tree_lock);
        //end by frankj
    } else if (!strcmp(method_name, "GetParameterValues")) {
        //add by frankj
        pthread_mutex_lock(&tree_lock);
        //end by frankj
        if (process_get_param_val(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
        //add by frankj
        pthread_mutex_unlock(&tree_lock);
        //end by frankj
    } else if (!strcmp(method_name, "Reboot")) {
        if (process_reboot(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
    } else if (!strcmp(method_name, "GetRPCMethods")) {
        if (process_getrpc(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
    } else if (!strcmp(method_name, "GetParameterNames")) {
        //add by frankj
        pthread_mutex_lock(&tree_lock);
        //end by frankj
        if (process_getparamname(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
        //add by frankj
        pthread_mutex_unlock(&tree_lock);
        //end by frankj*/
    } else if (!strcmp(method_name, "SetParameterAttributes")) {
        //add by frankj
        pthread_mutex_lock(&tree_lock);
        //end by frankj
        if (process_set_para_attr(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
        //add by frankj
        pthread_mutex_unlock(&tree_lock);
        //end by frankj
    } else if (!strcmp(method_name, "GetParameterAttributes")) {
        //add by frankj
        pthread_mutex_lock(&tree_lock);
        //end by frankj
        if (process_get_para_attr(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
        //add by frankj
        pthread_mutex_unlock(&tree_lock);
        //end by frankj
    } else if (!strcmp(method_name, "AddObject")) {
        //add by frankj
        pthread_mutex_lock(&tree_lock);
        //end by frankj
        if (process_addobj(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
        //add by frankj
        pthread_mutex_unlock(&tree_lock);
        //end by frankj
    } else if (!strcmp(method_name, "DeleteObject")) {
        //add by frankj
        pthread_mutex_lock(&tree_lock);
        //end by frankj
        if (process_delobj(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
        //add by frankj
        pthread_mutex_unlock(&tree_lock);
        //end by frankj
    } else if (!strcmp(method_name, "TransferCompleteResponse")) {
        if (process_tran_comp_resp(method) == SUCCESS) {
            res = SUCCESS;
        }
    } else if (!strcmp(method_name, "Upload")) {
        if (process_ul(method) == SUCCESS) {
            cpe_env_num++;
            res = SUCCESS;
        }
    } else if (!strcmp(method_name, "Fault")) {
        if (process_soap_fault(method) == SUCCESS) {
            res = SUCCESS;
        }
    } else {
        //Method name can't match all support method , Generate soap fault
        LOG(m_handler, DEBUG, "CPE can't support this RPC method at present, will generate soap fault\n");
        if (gen_method_fault(xmlroot, method_name, 9000) != NULL) {
            cpe_env_num++;
            res = SUCCESS;
        }
    }

    return res;
}

