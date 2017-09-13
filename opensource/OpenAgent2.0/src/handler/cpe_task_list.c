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

#include "cpe_task_list.h"
#include "../tools/dl_conf.h"
#include "./methods/upload.h"
#include "./methods/methods.h"
#include "../event/event.h"

int handle_download(TR_cpe_task_list *task_list);
int handle_reboot(TR_cpe_task_list *task_list);
int handle_upload(TR_cpe_task_list *task_list);

/*
 *********************************************************************
 * Function name: init_task_list
 * Description: init the head pointer of the task list
 * Parameter: none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int init_task_list()
{
    INIT_LIST_HEAD(&cpe_task_list_head);
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: add_task_list
 * Description: add the unfinished task into the task list
 * Parameter: char *func_name: the pointer point to the task name; void *func_param_struct: the pointer point to the task structure
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int add_task_list(char *func_name, void *func_param_struct)
{  
    int reboot_exist = 0;
    int down_exist = 0;
    TR_cpe_task_list     *task_list_traval = NULL;
    TR_cpe_task_list     *task_list = NULL;  
    TR_cpe_task_list     *reboot_task = NULL;
    TR_cpe_task_list     *down_task = NULL; 

    //add by frankj
    if(!list_empty(&cpe_task_list_head))
    {
        list_for_each_entry(task_list_traval, &cpe_task_list_head, node)
        {
            if(strcmp(task_list_traval->func_name, func_name) == 0)
            {
                LOG(m_handler, DEBUG, "node %s already exists in task_list.\n", func_name);
                return SUCCESS;
            }
            if(strcmp(task_list_traval->func_name, "dev_reboot") == 0)
            {
                reboot_exist = 1;
                reboot_task = task_list_traval;
            }
            if(strcmp(task_list_traval->func_name, "trf_download") == 0)
            {
                down_exist = 1;
                down_task = task_list_traval;
            }
        }
    }

    //end by frankj 
    if(( task_list = (TR_cpe_task_list *)malloc(sizeof(TR_cpe_task_list))) == NULL)
    {
        LOG(m_handler, ERROR, "Can't allocate enough memory for cpe_task_list\n");
        return FAIL;
    }
 
    strcpy(task_list->func_name,  func_name);
    task_list->func_param_struct = func_param_struct;
    //add by frankj
    if(strcmp(func_name, "dev_reboot") == 0)
    {
        list_add_tail(&task_list->node, &cpe_task_list_head);  
    }
    else if(strcmp(func_name, "trf_download") == 0)
    {
        if(reboot_exist == 1)
        {
            task_list->node.prev = reboot_task->node.prev;
            task_list->node.next = &reboot_task->node;
            reboot_task->node.prev->next = &task_list->node;
            reboot_task->node.prev = &task_list->node;
        }
        else
        {
            list_add_tail(&task_list->node, &cpe_task_list_head);  
        }
    }
    else
    {
        if(down_exist == 1)
        {
            task_list->node.prev = down_task->node.prev;
            task_list->node.next = &down_task->node;
            down_task->node.prev->next = &task_list->node;
            down_task->node.prev = &task_list->node;                
        }
        else if(down_exist == 0 && reboot_exist == 1)
        {
            task_list->node.prev = reboot_task->node.prev;
            task_list->node.next = &reboot_task->node;
            reboot_task->node.prev->next = &task_list->node;
            reboot_task->node.prev = &task_list->node;                
        }
        else
        {
            list_add_tail(&task_list->node, &cpe_task_list_head);  
        }
    }
    //end by frankj

    LOG(m_handler, DEBUG, "Add %s to task list success\n", func_name);
     
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: count_task_list
 * Description: counts the number of tasks in the task list
 * Parameter: struct list_head *_lst: the head pointer of a task list
 * Return value: the number of tasks in the task list
 *********************************************************************
*/
int count_task_list(struct list_head *_lst)
{
    struct   list_head * _p = NULL;
    int   count = 0;
    
    list_for_each(_p, _lst)
    {
        count++;
    }
    
    return count;
}

/*
 *********************************************************************
 * Function name: exec_task
 * Description: executes the tasks in the task list one bye one and delete them from the task list
 * Parameter: none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int exec_task()
{
    int res = -1;
    int   count = 0;
    TR_cpe_task_list   *task_list = NULL;
    
    //Check task list 
    count = count_task_list(&cpe_task_list_head);
    if(count == 0)
    {
        LOG(m_handler, DEBUG, "The cpe_task_list is NULL\n");
        return TASK_LIST_NULL;
    }
    LOG(m_handler, DEBUG, "There are %d nodes in task list\n", count);
    
    //This while process task list
    while(count > 0)
    {
        task_list = list_entry((&cpe_task_list_head)->next, TR_cpe_task_list, node);
        
        //process download
        if(!strcmp(task_list->func_name, "trf_download"))
        {
            LOG(m_handler, DEBUG, "Start process trf_download\n");
                        
            res = handle_download(task_list);
            if(res != SUCCESS)
                return res; 
        }
        //Process Reboot 
    	if(!strcmp(task_list->func_name, "dev_reboot"))
    	{
    	    LOG(m_handler, DEBUG, "Start process dev_reboot\n");
            
            res = handle_reboot(task_list);
            if(res != SUCCESS)
                return res;
    	}
    	
    	//process upload
    	if (!strcmp(task_list->func_name, "trf_upload"))
        {
    	    LOG(m_handler, DEBUG, "start process trf_upload\n");
    	    
    	    res = handle_upload(task_list);
            if(res != SUCCESS)
                return res;
    	}

    	count--;
    }
    
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: handle_download
 * Description: handles the download unfinished task in the task list
 * Parameter: TR_cpe_task_list *task_list: the pointer point to the task structure
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int handle_download(TR_cpe_task_list *task_list)
{
    int res;
    TR_download  *down = NULL;
    void *handle = NULL;
    char *d_error = NULL;
    int (*dev_writestreamtoflash)(char*, int) = NULL;
    int (*dev_writestreamtoflash_web)(char*, int) = NULL;
    int (*dev_flashimage)(char *, int) = NULL;
    int (*dev_reboot)() = NULL;

    if(download_at_once_flag)
    {
        LOG(m_handler, DEBUG, "at once\n");
        handle = dlopen(dev_lib_path, RTLD_LAZY);
        if (!handle)
        {
            LOG(m_handler, ERROR, "%s\n", dlerror());
            list_del(&task_list->node);
            free(task_list->func_param_struct);
            free(task_list);
            LOG(m_handler, DEBUG, "Delete download task success\n");
            return FAIL;
        }
 
        dev_writestreamtoflash = dlsym(handle, dev_writestreamtoflash_func);
        if((d_error = dlerror()) != NULL)
        {
            LOG(m_handler, ERROR, "%s\n", d_error);
            dlclose(handle);

            list_del(&task_list->node);
            free(task_list->func_param_struct); 
            free(task_list);
            LOG(m_handler, DEBUG, "Delete download task success\n");
            return FAIL;
        }

        dev_writestreamtoflash_web = dlsym(handle, dev_writestreamtoflash_web_func);
        if((d_error = dlerror()) != NULL)
        {
            LOG(m_handler, ERROR, "%s\n", d_error);
            dlclose(handle);
 
            list_del(&task_list->node);
            free(task_list->func_param_struct);
            free(task_list);
            LOG(m_handler, DEBUG, "Delete download task success\n");
            return FAIL;
        }
    
        dev_flashimage = dlsym(handle, dev_flashimage_func);
        if((d_error = dlerror()) != NULL) 
        {
            LOG(m_handler, ERROR, "%s\n", d_error); 
            dlclose(handle);

            list_del(&task_list->node);
            free(task_list->func_param_struct); 
            free(task_list);
            LOG(m_handler, DEBUG, "Delete download task success\n");
            return FAIL;
        }

        dev_reboot = dlsym(handle, dev_reboot_func);
        if((d_error = dlerror()) != NULL) 
        {
            LOG(m_handler, ERROR, "%s\n", d_error); 
            dlclose(handle);

            list_del(&task_list->node);
            free(task_list->func_param_struct); 
            free(task_list);
            LOG(m_handler, DEBUG, "Delete download task success\n");
            return FAIL;
        }

        if (imagetype == CONFIG_FORMAT)
        {
            LOG(m_handler, DEBUG, "Start write configure file to flash\n");
            res = dev_writestreamtoflash(imageptr, dl_len);
            if (res != 0)
            {
                LOG(m_handler, DEBUG, "Write configure file to flash faild\n");
            }
       
            LOG(m_handler, DEBUG, "Write configure file to flash success\n");
            
        }
        else if (imagetype == WEB_FORMAT)
        {
            LOG(m_handler, DEBUG, "Start write web file to flash\n");
            res = dev_writestreamtoflash_web(imageptr, dl_len);
            if (res != 0)
            {
                LOG(m_handler, DEBUG, "Write web file to flash faild\n");
            }
 
            LOG(m_handler, DEBUG, "Write web file to flash success\n");
 
        }
        else if (imagetype == IMAGE_FORMAT)
        {
            if (dev_flashimage(imageptr, dl_len) != 0)
            {
                LOG(m_handler, DEBUG, "Write to flash faild\n");
            }
            LOG(m_handler, DEBUG, "Write to flash successful\n");
        }
                
        LOG(m_handler, DEBUG, "reboot system\n");

        dev_reboot();
        dlclose(handle);
    }
    else
    {
        LOG(m_handler, DEBUG, "not at once\n");
        down = (TR_download *)(task_list->func_param_struct);
        if(down == NULL)
        {
            LOG(m_handler, DEBUG, "download task error");
            list_del(&task_list->node); 
            free(task_list->func_param_struct); 
            free(task_list);

            return FAIL;
        }

        res = handle_download_task(down);
        if(res != SUCCESS)
        {
            LOG(m_handler, DEBUG, "handle_download_task failed");
            list_del(&task_list->node); 
            free(task_list->func_param_struct); 
            free(task_list);

            return FAIL;
        }
    }

    list_del(&task_list->node);
    free(task_list->func_param_struct);
    free(task_list);
    LOG(m_handler, DEBUG, "Delete download task success\n");

    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: handle_download_task
 * Description: handles the download unfinished task in the task list
 * Parameter: TR_download  *down: the pointer point to the TR_download structure
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int handle_download_task(TR_download  *down)
{
    TR_tran_comp *transfer = NULL;

    time_t starttime;
    time_t completetime;

    char event_code[65];
    char cmd_key[33];

    int res;

    void *handle = NULL;
    char *d_error = NULL;
    int (*dev_writestreamtoflash)(char*, int) = NULL;
    int (*dev_writestreamtoflash_web)(char*, int) = NULL;
    int (*dev_flashimage)(char *, int) = NULL;
    int (*dev_reboot)() = NULL;

    handle = dlopen(dev_lib_path, RTLD_LAZY);
    if (!handle)
    {
        LOG(m_handler, ERROR, "%s\n", dlerror());
        return FAIL;
    }
 
    dev_writestreamtoflash = dlsym(handle, dev_writestreamtoflash_func);
    if((d_error = dlerror()) != NULL)
    {
        LOG(m_handler, ERROR, "%s\n", d_error);
        dlclose(handle);
        return FAIL;
    }

    dev_writestreamtoflash_web = dlsym(handle, dev_writestreamtoflash_web_func);
    if((d_error = dlerror()) != NULL)
    {
        LOG(m_handler, ERROR, "%s\n", d_error);
        dlclose(handle);
        return FAIL;
    }
    
    dev_flashimage = dlsym(handle, dev_flashimage_func);
    if((d_error = dlerror()) != NULL) 
    {
        LOG(m_handler, ERROR, "%s\n", d_error); 
        dlclose(handle);
        return FAIL;
    }

    dev_reboot = dlsym(handle, dev_reboot_func);
    if((d_error = dlerror()) != NULL) 
    {
        LOG(m_handler, ERROR, "%s\n", d_error); 
        dlclose(handle);
        return FAIL;
    }

    
            
    //Debug for download
    LOG(m_handler, DEBUG, "down.command_key : %s\n, down.delay_seconds: %d\n, down.failure_url: %s\n, down.file_type : %s\n, down.file_size : %d\n, down.password : %s\n, down.success_url: %s\n, down.target_file_name: %s\n, down.url : %s\n, down.username: %s\n", down->command_key, down->delay_seconds, down->failure_url, down->file_type, down->file_size, down->password, down->success_url, down->target_file_name, down->url, down->username);
            
    res = trf_download(down, &starttime, &completetime); 
            
    transfer = (TR_tran_comp *)malloc(sizeof(TR_tran_comp));
            
    if (transfer == NULL)
    {
        LOG(m_handler, ERROR, "Unable allocate memory for transfer\n");
        dlclose(handle);
        return FAIL;
    }
            
    if (res != SUCCESS)
    {
        LOG(m_handler, DEBUG, "Download unsuccessful\n");
            	
        //Add transfercmplete request
        strcpy(transfer->command_key, down->command_key);
        transfer->start_time = starttime;
        transfer->complete_time = completetime;
        transfer->fault_struct.fault_code = res;
        res = get_fault_string(transfer->fault_struct.fault_code, transfer->fault_struct.fault_string);
        if (res != 0)
        {
            LOG(m_handler, ERROR, "Unknow this fault code\n");
            free(transfer);
            dlclose(handle);
            return FAIL;
        }
                
        //Debug for add transfer
        LOG(m_handler, DEBUG, "transfer command_key: %s\n, transfer.start_time: %d\n, transfer.complete_time: %d\n, transfer.fault_struct.fault_code: %d\n, transfer.fault_struct.fault_string: %s\n", transfer->command_key, transfer->start_time, transfer->complete_time, transfer->fault_struct.fault_code, transfer->fault_struct.fault_string);

        res = add_req("TransferComplete", transfer);
        if (res != 0)
        {
            free(transfer);
            dlclose(handle);
            return FAIL;
        }
                
        strcpy(event_code, "7 TRANSFER COMPLETE");
        strcpy(cmd_key, down->command_key);
                
        //Debug for add event
        LOG(m_handler, DEBUG, "event_code: %s\n, cmd_key : %s\n", event_code, cmd_key);
                
        res = add_event(event_code, cmd_key);
        if(res != 0)
        {
            dlclose(handle);
            return FAIL;
        }       

        dlclose(handle);
        sem_post(&SEM_INFORM);

        return FAIL_DL;
    }
    else
    {
        LOG(m_handler, DEBUG, "Download successful\n");
        //Write Configeure File dl_conf_path
        strcpy(transfer->command_key, down->command_key);
        transfer->start_time = starttime;
        transfer->complete_time = completetime;
        transfer->fault_struct.fault_code = 0;
        strcpy(transfer->fault_struct.fault_string, "");
                
        //Debug for add transfer
        LOG(m_handler, DEBUG, "transfer command_key: %s\n, transfer.start_time: %d\n, transfer.complete_time: %d\n, transfer.fault_struct.fault_code: %d\n, transfer.fault_struct.fault_string: %s\n", transfer->command_key, transfer->start_time, transfer->complete_time, transfer->fault_struct.fault_code, transfer->fault_struct.fault_string);
                
        //Add
        if (imagetype == CONFIG_FORMAT)
        {
            LOG(m_handler, DEBUG, "Start write configure file to flash\n");
            res = dev_writestreamtoflash(imageptr, dl_len);
            if (res != 0)
            {
                //Define error code ?
                transfer->fault_struct.fault_code = 9002;
                strcpy(transfer->fault_struct.fault_string, "Internal error");
                LOG(m_handler, DEBUG, "write configure file to flash failed\n");
            }
            else
            {
                LOG(m_handler, DEBUG, "Write configure file to flash success\n");
            }
            res = write_dl_conf(dl_conf_path, 1, transfer);
            if (res != 0)
            {
                 LOG(m_handler, ERROR, "Write transfer to dl_conf failed\n");
                 free(transfer);
                 dlclose(handle);
                 return FAIL;
            }
            LOG(m_handler, DEBUG, "Write transfer to dl_conf success\n");
            free(transfer);
        }
        else if (imagetype == WEB_FORMAT)
        {
            LOG(m_handler, DEBUG, "Start web file to flash\n");
            res = dev_writestreamtoflash_web(imageptr, dl_len);
            if (res != 0)
            {
                //Define error code ?
                transfer->fault_struct.fault_code = 9002;
                strcpy(transfer->fault_struct.fault_string, "Internal error");
                LOG(m_handler, DEBUG, "write web file to flash failed\n");
            }
            else
            {
                LOG(m_handler, DEBUG, "Write web file to flash success\n");
            }
            res = write_dl_conf(dl_conf_path, 1, transfer);
            if (res != 0)
            {
                 LOG(m_handler, ERROR, "Write transfer to dl_conf failed\n");
                 free(transfer);
                 dlclose(handle);
                 return FAIL;
            }
            LOG(m_handler, DEBUG, "Write transfer to dl_conf success\n");
            free(transfer);
        }
        else if (imagetype == IMAGE_FORMAT)
        {
            res = write_dl_conf(dl_conf_path, 1, transfer);
            if (res != 0)
            {
                LOG(m_handler, ERROR, "Write transfer to dl_conf failed\n");
                free(transfer);
                dlclose(handle);
                return FAIL;
            }
            LOG(m_handler, DEBUG, "Write transfer to dl_conf success\n");

            if (dev_flashimage(imageptr, dl_len) != 0)
            {
                LOG(m_handler, DEBUG, "Write flash image failed\n");
                //eeror code? define
                transfer->fault_struct.fault_code = 9002;
                strcpy(transfer->fault_struct.fault_string, "Internal error");
                res = write_dl_conf(dl_conf_path, 1, transfer);
                if (res != 0)
                {
                    LOG(m_handler, ERROR, "Write transfer to dl_conf failed\n");
                    free(transfer);
                    dlclose(handle);
                    return FAIL;
                }
                LOG(m_handler, DEBUG, "Write transfer to dl_conf success\n");
                free(transfer);
            }
        }
                
        LOG(m_handler, DEBUG, "Success download then will reboot system\n");

        dev_reboot();
        dlclose(handle);
    }
    
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: handle_reboot
 * Description: handles the reboot unfinished task in the task list
 * Parameter: TR_cpe_task_list *task_list:  the pointer point to the task structure
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int handle_reboot(TR_cpe_task_list *task_list)
{
    void *handle = NULL;
    char *d_error = NULL;
    int (*dev_reboot)() = NULL;

    handle = dlopen(dev_lib_path, RTLD_LAZY);
    if (!handle)
    {
        LOG(m_handler, ERROR, "%s\n", dlerror());
        list_del(&task_list->node);
        free(task_list->func_param_struct);
        free(task_list);
        return FAIL;
    }
 
    dev_reboot = dlsym(handle, dev_reboot_func);
    if((d_error = dlerror()) != NULL)
    {
        LOG(m_handler, ERROR, "%s\n", d_error);
        dlclose(handle);
        list_del(&task_list->node);
        free(task_list->func_param_struct);
        free(task_list);
        return FAIL;
    }

    list_del(&task_list->node);
    free(task_list->func_param_struct);
    free(task_list);
    dev_reboot();
    dlclose(handle);

    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: handle_upload
 * Description: handles the upload unfinished task in the task list
 * Parameter: TR_cpe_task_list *task_list: the pointer point to the task structure
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int handle_upload(TR_cpe_task_list *task_list)
{
    TR_upload *up = NULL;
    TR_tran_comp *transfer = NULL;
 
    time_t starttime;
    time_t completetime;
 
    char event_code[65];
    char cmd_key[33];
 
    int res;

    up = (TR_upload *)(task_list->func_param_struct);
            
    //Debug for upload
    LOG(m_handler, DEBUG, "up.command_key : %s\n, up.delay_seconds: %d\n, up.file_type : %s\n, up.password : %s\n, up.url : %s\n, up.username: %s\n", up->command_key, up->delay_seconds, up->file_type, up->password, up->url, up->username);

    res = trf_upload(up, &starttime, &completetime);
    	    
    transfer = (TR_tran_comp *)malloc(sizeof(TR_tran_comp));
            
    if (transfer == NULL)
    {
        LOG(m_handler, ERROR, "Unable allocate memory for transfer\n");
        //delete task node
        list_del(&task_list->node);    
        free(task_list->func_param_struct);    
        free(task_list);
        LOG(m_handler, DEBUG, "Delete upload task success\n");
        return FAIL;
    }
            
    if (res != 0)
    {
        LOG(m_handler, DEBUG, "Upload Unsuccessful\n");
                
        //Add transfercmplete request
        strcpy(transfer->command_key, up->command_key);
        transfer->start_time = starttime;
        transfer->complete_time = completetime;
        transfer->fault_struct.fault_code = res;
        res = get_fault_string(transfer->fault_struct.fault_code, transfer->fault_struct.fault_string);
        if (res != 0)
        {
            LOG(m_handler, ERROR, "Unknow this fault code\n");
            free(transfer);
            //delete task node
            list_del(&task_list->node);     
            free(task_list->func_param_struct);     
            free(task_list);
            LOG(m_handler, DEBUG, "Delete upload task success\n");
            return FAIL;
        }
        LOG(m_handler, DEBUG, "get fault string success\n");
                
    }
    else
    {
        LOG(m_handler, DEBUG, "upload Successful\n");
        strcpy(transfer->command_key, up->command_key);
        transfer->start_time = starttime;
        transfer->complete_time = completetime;
        transfer->fault_struct.fault_code = 0;
        strcpy(transfer->fault_struct.fault_string, "");
    }
            
    //Debug for add transfer
    LOG(m_handler, DEBUG, "transfer command_key: %s\n, transfer.start_time: %d\n, transfer.complete_time: %d\n, transfer.fault_struct.fault_code: %d\n, transfer.fault_struct.fault_string: %s\n", transfer->command_key, transfer->start_time, transfer->complete_time, transfer->fault_struct.fault_code, transfer->fault_struct.fault_string);
    
    res = add_req("TransferComplete", transfer);
    if (res != 0)
    {
        free(transfer);
        //delete task node
        list_del(&task_list->node);     
        free(task_list->func_param_struct);     
        free(task_list);
        LOG(m_handler, DEBUG, "Delete upload task success\n");
        return FAIL;
    }
                
    strcpy(event_code, "7 TRANSFER COMPLETE");
    strcpy(cmd_key, up->command_key);
                
    //Debug for add event
    LOG(m_handler, DEBUG, "event_code: %s\n, cmd_key : %s\n", event_code, cmd_key);
                
    res = add_event(event_code, cmd_key);
    if(res != 0)
    {
        free(transfer);
        //delete task node
        list_del(&task_list->node);     
        free(task_list->func_param_struct);     
        free(task_list);
        LOG(m_handler, DEBUG, "Delete upload task success\n");
        return FAIL;
    }    
    sem_post(&SEM_INFORM);
            
    //delete task node
    list_del(&task_list->node);
    free(task_list->func_param_struct);
    free(task_list);
    LOG(m_handler, DEBUG, "Delete upload task success\n");

    return SUCCESS;
}
