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
*	init.c - Initialize global resource for Agent
*
*	$Author: joinsonj $
*
*	history: $Date: 2007-06-08 02:23:35 $, initial version by simonl
*
***********************************************************************/


/***********************************************************************
*
*	include file
*
***********************************************************************/

#include "init.h"
/***********************************************************************
*
*       function declare
*
***********************************************************************/


/*
***********************************************************************
* Function name: init_cpe  
* Description:  Initialize global variables
* Parameter: 
* Return value: 0:SUCCESS  -1:FAIL
***********************************************************************
*/
int init_cpe()
{
    int res;

    /* Initialize the path of all configure file */
    res = init_conf_path();
    if(res != 0) {
        printf("Invoke init_conf_path() failed\n");
        return FAIL;
    }
    strcpy(log_file_path, conf_dir);
    strcat(log_file_path, "cpe.log");

    /* Initialize logger */
    res = init_logger();
    if(res != 0) {
        printf("initialize logger failed\n");
        return FAIL;
    }
    /* For debug */
    log_out_mode = 0;
    log_size = 1;
    log_module = all;

    /* Initialize event_list_head */
    res = init_event_list();
    if(res != 0) {
        LOG(m_init, ERROR, "Initialize event_list_head failed\n");
        return FAIL;    
    }
    
    /* Initialize changed_param_name_list_head */
    res = init_changed_param_name_list();
    if(res != 0) {
        LOG(m_init, ERROR, "Initialize changed_param_name_list_head failed\n");
        return FAIL;    
    }
    /* Initialize object tree */
    if(pthread_mutex_init(&tree_lock, NULL) != 0) {
        LOG(m_init, ERROR, "Init Mutex tree_lock failed.\n");
        return FAIL;
    }
    LOG(m_init, DEBUG, "Init tree_lock success.\n");
    res = init_object_tree();
    if(res != 0) {
        LOG(m_init, ERROR,"initialize object tree failed\n");
        return FAIL;
    }
    LOG(m_init, DEBUG, "Invoke init_object_tree() success.\n");

    /* Initialize agent.conf */
    res = init_agent_conf();
    if (res != 0) {
        LOG(m_init, ERROR, "Initialize agent_conf failed.\n");
        return FAIL;
    }
    res = init_iv();
    if (res != 0) {
        LOG(m_init, ERROR, "Invoke init_iv() failed.\n");
        return FAIL;
    }
    
    res = init_timer();
    if (res != 0) {
        LOG (m_init, ERROR, "Initialize thread p_timer failed\n");
        return FAIL;    
    }
    LOG(m_init, DEBUG, "Invoke init_timer() success.\n");

    /* Initialize try_time */
    res = init_try_time();  
    if(res != 0) {
        LOG(m_init, ERROR, "Invoke init_try_time() failed\n");
        return FAIL;    
    }

    /* Initialize empty_qbuf_head, send_qbuf_head and recv_qbuf_head */
    res = init_list_member(&empty_qbuf_list);
    if(res != 0) {
        LOG(m_init, ERROR, "Initialize empty_qbuf_head failed\n");
        return FAIL;    
    }
    res = init_list_member(&send_qbuf_list);
    if(res != 0) {
        LOG(m_init, ERROR, "Initialize send_qbuf_head failed\n");
        return FAIL;    
    }
    res = init_list_member(&recv_qbuf_list);
    if(res != 0) {
        LOG(m_init, ERROR, "Initialize recv_qbuf_head failed\n");
        return FAIL;    
    }
    res = empty_qbuf_list.create(&empty_qbuf_list, 6);
    if(res != 0) {
        LOG(m_init, ERROR, "Initialize empty_qbuf_head failed.\n");
        return FAIL;    
    }
    
    /* Initialize cpe_req_list_head */
    res = init_request();
    if(res != 0) {
        LOG(m_init, ERROR, "Initialize cpe_req_list_head failed\n");
        return FAIL;    
    }

    /* Initialize SEM_SEND */
    res = sem_init(&SEM_SEND, 0, 0);
    if(res != 0) {
        LOG(m_init, ERROR, "Semaphore SEM_SEND initialization failed\n");
        return FAIL;    
    }
    
    /* Initialize SEM_RECV */
    res = sem_init(&SEM_RECV, 0, 0);
    if(res != 0) {
        LOG(m_init, ERROR, "Semaphore SEM_RECV initialization failed\n");
        return FAIL;    
    }
    /* Initialize SEM_INFORM */
    res = sem_init (&SEM_INFORM, 0, 0);
    if (res != 0) {
        LOG (m_init, ERROR, "Semaphore SEM_INFORM initialization failed\n");
        return FAIL;    
    }
    /* Initialize SEM_CONNECTED */
    res = sem_init (&SEM_CONNECTED, 0, 1);
    if (res != 0) {
        LOG (m_init, ERROR, "Semaphore SEM_CONNECTED initialization failed\n");
        return FAIL;    
    }
    /* Initialize SEM_HANDLER_ABORT */
    res = sem_init (&SEM_HANDLER_ABORT, 0, 0);
    if (res != 0) {
        LOG (m_init, ERROR, "Semaphore SEM_HANDLER_ABORT initialization failed\n");
        return FAIL;    
    }
    return SUCCESS;
    	
}

/*
***********************************************************************
* Function name: init_download
* Description:  Initialize for download
* Parameter: 
* Return value: 0:SUCCESS  -1:FAIL
***********************************************************************
*/
int init_download()
{
    FILE *fp;
    int res;
    int flag;
    char event_code[EVENT_CODE_LEN];
    char cmd_key[CMDKEY_LEN];

    TR_tran_comp *transf;
    TR_download *down;
    void *param_ptr;

    if((fp = fopen(dl_conf_path, "rb")) == NULL) {
        LOG(m_init, DEBUG, "File %s not exist, create it\n", dl_conf_path);
        if((fp = fopen(dl_conf_path, "wb")) == NULL) {
            LOG(m_init, ERROR, "Can't create file %s\n", dl_conf_path);
            return FAIL;
        } else {
            LOG(m_init, DEBUG, "create file %s successfullly\n", dl_conf_path);
	     fclose(fp);
             return SUCCESS;
        }
    } else {
        LOG(m_init, DEBUG, "ropen file %s successfully\n", dl_conf_path);
        fclose(fp);

        /* read the flag of file dl_conf */
        flag = flag_dl_conf(dl_conf_path);
        if(flag == -1) {
            LOG(m_init, ERROR, "Invoke flag_dl_conf() failed\n");
	     return FAIL;
        } else if(flag == -2) {
            LOG(m_init, DEBUG, "File %s is empty\n", dl_conf_path);
        } else if(flag == 0) {
            LOG(m_init, DEBUG, "The flag of %s is 0 (Download)\n", dl_conf_path);

            /* read_dl_conf(0) */
            param_ptr = (void *)malloc(sizeof(TR_download));
            res = read_dl_conf(dl_conf_path, param_ptr);
            if(res != 0) {
                LOG(m_init, ERROR, "Invode read_dl_conf() failed\n");
		  free(param_ptr);
		  return FAIL;
            }
            down = (TR_download *)param_ptr;
            res = handle_download_task(down);
            if(res == FAIL) {
                LOG(m_handler, DEBUG, "handle_download_task failed");
                return FAIL;
            } else if(res == SUCCESS) {
                return REBOOT_NOW;
            } else {
                return GO_ON;
            }
        } else {
            LOG(m_init, DEBUG, "The flag of %s is 1 (TransferComplete)\n", dl_conf_path);

            /* read_dl_conf(1) */
            param_ptr = (void *)malloc(sizeof(TR_tran_comp));
            res = read_dl_conf(dl_conf_path, param_ptr);
            if(res != 0) {
                LOG(m_init, ERROR, "Invode read_dl_conf() failed\n");
		  free(param_ptr);
		  return FAIL;
            }
            transf = (TR_tran_comp *)param_ptr;

            /* add_req(TransferComplete) */
            res = add_req("TransferComplete", transf);
            if(res != 0) {
                LOG(m_init, ERROR, "Add TransferComplete request failed\n");
		free(transf);
                return FAIL;
            }

            /* add_event(TransferComplete) */

            strcpy(event_code, "7 TRANSFER COMPLETE");
            strcpy(cmd_key, transf->command_key);
            res = add_event(event_code, cmd_key);
            if(res != 0) {
                LOG(m_init, ERROR, "Add Transfer Complete node in event_list failed\n");
		free(transf);
                return FAIL;
            }
           
        }
    }
    return SUCCESS;
}



