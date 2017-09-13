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

#include "../res/global_res.h"
#include "../tools/logger.h"
//#include "./methods/download.h"
#include "./methods/methods.h"
#include "handler.h"

/**********************************************************************
*
*Description:    Add a pending CPE request to cpe_req_list
*param :      
*                req_name:          point to the request method   name
*                req_param_struct:  point to request argument        
*Return Value:   0 sucess, -1 failure
*
***********************************************************************/
int add_req(char *req_name, void *req_param_struct )
{
    int F_exist = 0;
    TR_cpe_req_list     *req_list_traval;   
    TR_cpe_req_list     *req_list;  
    
    //add by frankj
    if(!list_empty(&cpe_req_list_head))
    {
        list_for_each_entry(req_list_traval, &cpe_req_list_head, node)
        {
            if(strcmp(req_list_traval->req_name, req_name) == 0)
            {
                F_exist = 1;
                break;
            }
        }
    }
    if(F_exist == 0)
    {
        //end by frankj
        if(( req_list = (TR_cpe_req_list *)malloc(sizeof(TR_cpe_req_list))) == NULL){
           LOG(m_handler, ERROR, "Can't allocate enough memory for cpe_req_list\n");
           return (-1);
        }
 
        strcpy(req_list->req_name,  req_name);
        req_list->req_param_struct = req_param_struct;
        req_list->retry_count = 0;
        list_add_tail(&req_list->node, &cpe_req_list_head);  
    }
 
    LOG(m_handler, DEBUG, "Add %s to request list success\n", req_name);
    
    return (0);
}

/********************************************************************************
*
* Description:   Calculate how many items in a list
* Param:        
*                *_lst:  use linux kernel list structure
* Return Value:  0  Sucess       -1  failure
*
*********************************************************************************/
int count_req_list(struct list_head *_lst)
{
    struct   list_head * _p = NULL;
    int   count = 0;
    
    list_for_each(_p, _lst)
    {
        count++;
    }
    
    return count;
}

/********************************************************************************
*
* Description:  Delete a node from cpe_req_list
* Param:        
*               req: one node in cpe_req_list
* Return Value: 0 
*
*******************************************************************************/
int del_req(TR_cpe_req_list *req)
{       
    list_del(&req->node);
    free(req->req_param_struct);
    free(req);
         
    return 0;    
}

/*******************************************************************************
*Initial the cpe_req_list
*
*********************************************************************************/
int init_request()
{
    INIT_LIST_HEAD(&cpe_req_list_head);
    return 0;
}

/*
 * Description:  Insert a request to xmlroot for send
 * Param:
 *       req:    point to a cpe request structure
 *       xmlroot:point to a root node of xml
 * Return Value: 0 sucess   -1 failure    
 */
int insert_request(TR_cpe_req_list *req, TRF_node *xmlroot)
{
	
    if(strcmp(req->req_name, "TransferComplete") == 0)
    {
        if(gen_tran_comp_method(req->req_param_struct, xmlroot) != 0){
			
            LOG(m_handler, ERROR, "Insert TransferComplete method failure");
            return (-1);
        }
		
    }
	
	LOG(m_handler, DEBUG, "Insert TransferComplete method to xmlroot sucessfullly\n");
    return (0);
}	

/*
 * Description:  search a request node in request list
 * Param:
 *     req_name:          point to the request method   name
 *     req_list:          point to one node in cpe_req_list 
 * Return Value: 
 *     
 */

TR_cpe_req_list *search_req_node(char *req_name)
{
    int num;
    TR_cpe_req_list *req_list = NULL;
    struct list_head * _p = NULL;
    
    //Get the number of request in request list
    num = count_req_list(&cpe_req_list_head);
    if (num <= 0){
        LOG(m_handler, DEBUG, "requset list have no request node\n");
        return NULL;
    }
    
    //Find request node in request list
    list_for_each(_p, &cpe_req_list_head)
    {
        req_list = list_entry((&cpe_req_list_head)->next, TR_cpe_req_list, node);

        if (!strcmp(req_list->req_name, req_name)){
            LOG(m_handler, DEBUG, "Get %s node in request list sucessfullly\n", req_name);
            break;
        }
        (&cpe_req_list_head)->next = ((&cpe_req_list_head)->next)->next;
    }
       
    return req_list;
}
