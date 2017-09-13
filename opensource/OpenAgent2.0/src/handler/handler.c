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
 * Include head file.
 */

#include "handler.h"
#include "cpe_task_list.h"
#include "../res/global_res.h"
#include "../res/qbuf.h"
#include "soap/http.h"
#include "methods/inform.h"

/*
 * Declear function.....
 */

static void handle_hold_req();
static void handle_head_method();
static void handle_termination();
static void *handle_recv_data();
static int trigger_inform();

/*
 * Define Global variable
 */

Qbuf_node *sbuf, *rbuf;
int req_num;                      //The number of request list
TR_cpe_req_list *cpereqnode;

/*
 ***************************************************************************
 * Function name: init_handler() 
 * Description: create a thread to call trigger inform 
 * Return Value:
 *     success return 0, else return -1
 ***************************************************************************
 */
 
int init_handler()
{
    int ret = INIT_HANDLE_FAILED;
    pthread_attr_t thread_attr;
    
    //init thread attr
    if (pthread_attr_init(&thread_attr) == 0) {
        LOG(m_handler, DEBUG, "pthread_attr_init success.\n");
	
        //set thread attr
        if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) == 0) {
            LOG(m_handler, DEBUG, "pthread_attr_setdetachstate success.\n");
            
            //Creat thread
            if (pthread_create(&p_handler, &thread_attr, handle_recv_data, NULL) == 0) {
                LOG(m_handler, DEBUG, "Init handler successful\n");
                ret = INIT_HANDLE_SUCCESS;
            } else {
                switch(errno) {
                    case EAGAIN:
                        LOG(m_handler, ERROR, "pthread_create(p_handler) failed, show cause: ? too much thread numbers.\n");
                        break;
                    case EINVAL:
                        LOG(m_handler, ERROR, "pthread_create(p_handler) failed, show cause: ? thread id illegality.\n");
                        break;
                    default :
                        LOG(m_handler, ERROR, "pthread_create(p_handler) failed, show cause: ? unknown.\n");
                        break;
                }
            }
        } else {
            LOG(m_handler, ERROR, "pthread_attr_setdetachstate Failed.\n");
        }
    } else {
        LOG(m_handler, ERROR, "pthread_attr_init failed .\n");
    }
    
    return ret;
}

/*
 ***************************************************************************
 * Function: save_send_data()
 * Description: save data to the buffer
 * Param: Void
 * Return value:
 *     handle status
 ***************************************************************************
 */
void save_send_data()
{
    int ret;
    int retry_get = 0;
    char *pData = NULL; 
    
    //get empty buffer
    do {
        sbuf = empty_qbuf_list.head(&empty_qbuf_list);
        if (sbuf != NULL) {
            break;
        } else {
    	    //buf_snap_shots();
            //monitor_network_status();
            LOG(m_handler, ERROR, "empty buffer is NULL\n");
            sleep(1);
        }
        retry_get++;
    } while (retry_get < 3);
    
    if (retry_get >= 3) {
        //Delete xmlroot tree
        mxmlDelete(xmlroot);
        LOG(m_handler, ERROR, "Can't get empty buffer\n");
        handle_termination();
    }
    LOG(m_handler, DEBUG, "Get empty buf successful\n");

    pData = sbuf->get_data_ptr(sbuf);
    //save to empty buffer
    ret = save_to_sendbuf(xmlroot, pData);
    if (ret == HTTP_GEN_FAILED) {
        LOG(m_handler, ERROR, "savd send data error\n");
        empty_qbuf_list.append(&empty_qbuf_list, sbuf);
        handle_termination();
    }
    
    //LOG(m_handler, DEBUG, "Send Data: %s\n", pData);
    //init cpe envelope number
    cpe_env_num = 0;
    
    //insert the sbuf to send_qbuf
    send_qbuf_list.append(&send_qbuf_list, sbuf);
    LOG(m_handler, DEBUG, "Insert send_qbuf successful \n");
    
    return;
}

/*
 ************************************************************************
 * Function:handle_hold_req()
 * Description: add cpe request to the buffer
 * Param:
 *     Void
 * Return value:
 *     Void
 ************************************************************************
 */

void handle_hold_req()
{
    int res;
    // This while handle pending request when the request list have data
    do {
        if (cpe_env_num == 0) {
            //Generate xmlroot node
            xmlroot = gen_xml_header();
            if (xmlroot == NULL) {
                LOG(m_handler, ERROR, "Generate xml root failed\n");
                handle_termination();
            }
            LOG(m_handler, DEBUG, "Generate xml header successs \n");
        }
        
        while (cpe_env_num < ACS_MAX_ENVELOPES) {
            //Get the pointer of the request list node
            cpereqnode = list_entry((&cpe_req_list_head)->next, TR_cpe_req_list, node);
            //Insert a cpe request to xmlroot for send
            res = insert_request(cpereqnode, xmlroot);
            if (res != 0) {
                LOG(m_handler, ERROR, "Insert request to xml failed\n");
                handle_termination();
            }
            LOG(m_handler, DEBUG, "Insert request to xml successful\n");
            cpe_env_num++;
            
            //Delete cpe request list node
            //del_req(cpereqnode);
            //LOG(m_handler, DEBUG, "Delete request list succesfull\n");
        
            (&cpe_task_list_head)->next = ((&cpe_task_list_head)->next)->next;
        
            req_num--;
            //Check CPE request list is NULL 
            if (req_num <= 0)
                break;
        }
                   
        //save the send data to the buffer
        save_send_data();
        //Post Semphore to send thread
        sem_post(&SEM_SEND);
        LOG(m_handler, DEBUG, "Post SEM_SEND to send thread\n");
    } while (req_num > 0);
    
}

/*
 *******************************************************************************
 * Function name: handle_head_method()
 * Description: handler soap header and method , generate new data
 * Parameter:
 *     void
 * Return value:
 *     void
 ******************************************************************************
 */

void handle_head_method()
{
    int i = 0;

    //This while start process soap header and method
    do {
        if (gen_xml_header() == NULL) {
            handle_termination();
        }
        cpe_env_num = 0;
        do {
            LOG(m_handler, DEBUG, "Procrss soap head\n");
            process_soap_head(p[i].head);
            LOG(m_handler, DEBUG, "Process method\n");
            if (process_method(p[i].method) == -1) {
                LOG(m_handler, ERROR, "Process method error\n");

                LOG(m_handler, DEBUG, "Now delete the xmltop\n");
                mxmlDelete(xmltop);

                //Handle termination
                handle_termination();
            }
            i++;

            if (p[i].head == NULL && p[i].method == NULL) {
                //handle hold request
                LOG(m_handler, DEBUG, "Process data package end, NOW start check cpe request list\n");
                req_num = count_req_list(&cpe_req_list_head);
                LOG(m_handler, DEBUG, "Hold request number = %d\n", req_num);
                if (req_num > 0 && hold_req_val == 0 && inform_flag == 1)
                    handle_hold_req();
                else {
                    save_send_data();
                    //Post Semphore to send thread
                    sem_post(&SEM_SEND);
                    LOG(m_handler, DEBUG, "Post SEM_SEND to send thread\n");
                }
                return;
            }
        } while (cpe_env_num < ACS_MAX_ENVELOPES);
        
        //Save send data to buffer
        save_send_data();
        //Post Semphore to send thread
        sem_post(&SEM_SEND);
        LOG(m_handler, DEBUG, "Post SEM_SEND to send thread\n");
        

    } while (p[i].head != NULL || p[i].method != NULL);

}

/*
 ********************************************************************************
 * Function name :handle_termination() 
 * Description: tell comm moudle  when the session will termination
 * Param : 
 *      Void
 * Return value:
 *      Void
 *******************************************************************************
 */

void handle_termination()
{   
    //Check task list and execute task
    LOG(m_handler, DEBUG, "Start exec task\n");
    exec_task();
    LOG(m_handler, DEBUG, "Exec task successful\n");
    
    //buf_snap_shots();
    empty_qbuf_list.display(&empty_qbuf_list);
    send_qbuf_list.display(&send_qbuf_list);
    recv_qbuf_list.display(&recv_qbuf_list);
    //monitor_network_status();

    //Post signal SIG_ABORT to p_send
    post_signal(p_send, SIG_ABORT);
    sleep(1);
    post_signal(p_recv, SIG_ABORT);
    sleep(1);
    LOG(m_handler, DEBUG, "Post signal SIG_ABORT to p_send p_recv\n");
 
    //The send thread will wait when it wait sem send, it can't exit
    sem_post(&SEM_SEND);
    sem_post(&SEM_HANDLER_ABORT);

    LOG(m_handler, DEBUG, "Handler thread will closed\n");
    pthread_exit(NULL);

}

/*
 *****************************************************************************
 * Function:
 * Description:
 * Parameter:
 * Return Value:
 *****************************************************************************
 */

int trigger_inform()
{
    int ret;

    xmlroot = gen_xml_header();
    if (xmlroot == NULL) {
        return -1;
    }
    
    ret = init_inform();
    if (ret != 0) {
        return -1;
    }

    save_send_data();
    
    return 0;
}

/*
 *****************************************************************************
 * Function: handle_recv_data()
 * Description: parse recv data and generate new send data
 * Return value:
 *     Void
 *****************************************************************************
 */

void *handle_recv_data()
{
    int i = 0;
    HTTP_PARSE_RESULT ret;
    
    //init global variable
    hold_req_val = 0;
    ACS_MAX_ENVELOPES = 1;
    cpereqnode = NULL;
    inform_flag = 0;
    //inform_retry = 0;
    cpe_env_num = 0;
    have_cookie_flag = 0;
    nonce_count = 0; 

    //init session array
    for (i = 0; i < SUPPORT_MAX_COOKIE_NUM; i++) {
        memset(&session_cookie_array[i], 0, sizeof(TR_session_cookie));
    }
    //memset(session_cookie_array, 0, sizeof(session_cookie_array));
    
    char *pData = NULL;
    
    //init task
    init_task_list();

    //trigger inform
    LOG(m_handler, DEBUG, "Send Indorm \n");
    if (trigger_inform() == -1){
        LOG(m_handler, ERROR, "Trigger Inform Failed\n");
        post_retry();
        handle_termination();
    }
    sem_post(&SEM_SEND);
    
    // Handler receive data package and generate new data package
    do {
        //Wait semphore from recv thread
        LOG(m_handler, DEBUG, "WAITTING SEM_RECV ........\n");
        sem_wait(&SEM_RECV);
        LOG(m_handler, DEBUG, "GET SEM_RECV Successful\n");
        
        //Get data from recv_qbuf
        rbuf = recv_qbuf_list.head(&recv_qbuf_list);
        if (rbuf == NULL) {
            LOG(m_handler, DEBUG, "Recv buffer is NULL\n");
            handle_termination();
        }
        
        //Check the data length
        pData = rbuf->get_data_ptr(rbuf);
        if (strlen(pData) == 0) {
            LOG(m_handler, DEBUG, "Need to retry\n");
            empty_qbuf_list.append(&empty_qbuf_list, rbuf);
            post_retry();
            handle_termination();
        }
        LOG(m_handler, DEBUG, "Recv Data : %s\n", pData);
        
        //parse http header
        LOG(m_handler, DEBUG, "Start Parse http header\n");
        ret = parse_http_header(pData);
        
        switch (ret) {
            case STATUS_OK:
                //Http body is not NULL, then process it
                if (get_head_method(pData) == -1) {
                    LOG(m_handler, ERROR, "get_head_method: can't get memory avliable");
                    empty_qbuf_list.append(&empty_qbuf_list, rbuf);
                    handle_termination();
                }

                //release recv buffer
                empty_qbuf_list.append(&empty_qbuf_list, rbuf);
                LOG(m_handler, DEBUG, "Release buf successful\n");

                //handler soap header and method and generate response data
                handle_head_method();
                
                //delete xmltop for recv
                LOG(m_handler, DEBUG, "Now delete the xmltop\n");
                mxmlDelete(xmltop);

                break;
            case STATUS_NO_CONTENT:
                LOG(m_handler, DEBUG, "HTTP data is NULL\n");
                empty_qbuf_list.append(&empty_qbuf_list, rbuf);
                LOG(m_handler, DEBUG, "Release buf successful\n");
                //handle cpe hold request
                req_num = count_req_list(&cpe_req_list_head);
                LOG(m_handler, DEBUG, "Hold request number = %d\n", req_num);
                if (req_num > 0 && hold_req_val == 0 && inform_flag == 1) {
                    handle_hold_req();
                } else {
                    handle_termination();
                }
                break;
            case STATUS_NEED_AUTH:
                //TODO
                LOG(m_handler, DEBUG, "NEED AUTH\n");
                empty_qbuf_list.append(&empty_qbuf_list, rbuf);
                //sem_post(&SEM_INFORM);
                post_retry();
                LOG(m_handler, DEBUG, "POST SEM INFORM \n");
                handle_termination();
                break;
            case STATUS_NEED_REDIRECTD:
                //TODO
                empty_qbuf_list.append(&empty_qbuf_list, rbuf);
                handle_termination();
                break;
            case UNKNOW_STATUS_CODE:
                empty_qbuf_list.append(&empty_qbuf_list, rbuf);
                handle_termination();
                break;
            case FORMAT_ERROR:
                empty_qbuf_list.append(&empty_qbuf_list, rbuf);
                handle_termination();
                break;
            case PARSER_ERROR:
                LOG(m_handler, DEBUG, "HTTP PARSER ERROR\n");
                empty_qbuf_list.append(&empty_qbuf_list, rbuf);
                handle_termination();
                break;
            default:
                empty_qbuf_list.append(&empty_qbuf_list, rbuf);
                handle_termination();
                break;
            
        }
    } while (1);
    

    
    return NULL;
}

