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
*	event.c - Event module
*
*	$Author: joinsonj $
*
*	history: $Date: 2007-06-08 02:23:18 $, initial version by simonl
*
***********************************************************************/


/***********************************************************************
*
*	include file
*
***********************************************************************/

#include "event.h"

/************************************************************************
*
*	global var define
*
************************************************************************/


/***********************************************************************
*
*	function prototype
*
***********************************************************************/
static int start_session();
static void *func_timer(void *arg);
/*
 ********************************************************************
 * Function name: start_session();
 * Description: Trigger handler and comm module.
 * Parameter: 
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int start_session()
{
    int res = 0;
    res = start_comm();
    if (res != 0) {
       LOG (m_event, ERROR, "Initialize comm failed.\n");
       return FAIL;
    } else {
        LOG (m_event, DEBUG, "Initialize comm success.\n");
    }
    res = init_handler();
    if (res != 0) {
       LOG (m_event, ERROR, "Initialize handler failed.\n");
       return FAIL;
    } else {
       LOG (m_event, DEBUG, "Initialize handler success.\n");
    }
    return SUCCESS;
}
/*
 ********************************************************************
 * Function name: func_timer();
 * Description: timer thread for periodic.
 * Parameter: 
 * Return value: void
 *********************************************************************
 */
void *func_timer(void *arg)
{
    timer_t cur_time;
    struct event_list *event_list_node;
    int res;
    
    flag_periodtime_change = 0;
    init_first_time();
    while(1) {
        LOG(m_event, DEBUG, "Thread timer is here.\n");
        cur_time = dev_get_cur_time();
        if (g_first_time > cur_time) {
            res = set_timer(g_first_time - cur_time);
            if (res == 0) {
                LOG(m_event, DEBUG, "Invoke set_timer() success.\n");
            } else {
                LOG(m_event, ERROR, "Invoke set_timer() failed.\n");
            }
        }
        while(1) {
             if (get_g_period_enable() == PERIOD_ENABLE){
                LOG(m_event, DEBUG, "Periodic inform is enabled\n");
                pthread_mutex_lock(&event_list_lock);
                if(!list_empty(&event_list_head)) {
                    list_for_each_entry(event_list_node, &event_list_head, node) {
                        if(strcmp(event_list_node->event_code, "2 PERIODIC") == 0) {
                            LOG(m_device, DEBUG, "2 PERIODIC find out.\n");
                            event_list_node -> sent_flag = 1;
                        }
                    }
                }
                pthread_mutex_unlock(&event_list_lock);
            
                LOG(m_event, DEBUG, "post SEM_INFORM.\n"); 
                sem_post(&SEM_INFORM);
            } else {
                LOG(m_event, DEBUG, "Time out, but priodic is disable.\n"); 
               // res = set_timer(10);
            }
            res = set_timer(get_g_period_iv());
            if (res == 0) {
                LOG(m_event, DEBUG, "Invoke set_timer() success.\n");
                LOG(m_event, DEBUG, "Periodic come now.\n");
            } else {
                LOG(m_event, ERROR, "Invoke set_timer() failed.\n");
            }
            if((get_flag_periodtime_change()) == PERIOD_TIME_CHANGE) {
                init_first_time();
                set_flag_periodtime_change(PERIOD_TIME_NOT_CHANGE);

                break;
            }
        }
    }
    pthread_exit(NULL);
}
/*
***********************************************************************
* Function name:  init_timer
* Description:  The entrance of timer module. Create thread p_timer.
* Parameter: None
* Return value: SUCCESS  FAIL
***********************************************************************
*/
int init_timer()
{
    int res;
    pthread_t p_timer;
    pthread_attr_t p_timer_attr;

    res = pthread_attr_init(&p_timer_attr);
    if(res != 0) {
        LOG(m_event, ERROR, "Thread attribute creation failed\n");
        return FAIL;
    } 
    res = pthread_attr_setdetachstate(&p_timer_attr, PTHREAD_CREATE_DETACHED);
    if(res != 0) {
        LOG(m_event, ERROR, "Set detached attribute failed\n");
        return FAIL;
    } 
    res = pthread_create(&p_timer, &p_timer_attr, func_timer, NULL);
    if(res != 0) {
        LOG(m_event, ERROR, "Create thread p_timer failed\n");
        return FAIL;
    } 
    
    (void)pthread_attr_destroy(&p_timer_attr);
    
    return SUCCESS;
}
/*
***********************************************************************
* Function name:  init_iv
* Description:  Initialize some variable of periodic interval.
* Parameter: None
* Return value: SUCCESS  FAIL
***********************************************************************
*/
int init_iv()
{
    int res;
    agent_conf pconf;

    res = get_agent_conf(&pconf);
    if (res != 0) {
        LOG(m_event, ERROR, "Invoke get_agent_conf failed.\n");
        return FAIL;
    }
    set_g_period_iv(pconf.periodic_inform_interval);
    set_g_period_enable(pconf.periodic_inform_enable);
    if(strcmp(pconf.periodic_inform_time, "0000-00-00T00:00:00") == 0) {
        strcpy(pconf.periodic_inform_time, "2001-01-01T00:00:00");
    }
    LOG(m_event, DEBUG, "time:%s\n", pconf.periodic_inform_time);
    g_period_time = strtimetosec(pconf.periodic_inform_time);
    if (g_period_time == -1){
      return FAIL;
    }
    strcpy(reboot_cmd_key, pconf.command_key);
    flag_reboot = pconf.flag_reboot;
    max_try_time = pconf.retry_times;
    LOG(m_event, DEBUG, "max_try_time = %d\n", max_try_time);  
    LOG(m_event, DEBUG, "g_period_iv = %d\tg_period_enable = %d\tg_period_time = %d\n", g_period_iv, g_period_enable, g_period_time);
    LOG(m_event, DEBUG, "flag_reboot = %d\t reboot_cmd_key = %s\n", flag_reboot, reboot_cmd_key);        
    return SUCCESS;


}

/*
***********************************************************************
* Function name:  init_event
* Description:  Initialize some events when init agent
* Parameter: None
* Return value: SUCCESS  FAIL
***********************************************************************
*/
int init_event()
{
    int res;
    char event_code[EVENT_CODE_LEN ];
    char cmd_key[CMDKEY_LEN];
    agent_conf a_conf; 
           
    memset(event_code, '\0', sizeof(event_code));
    memset(cmd_key, '\0', sizeof(cmd_key));
    
    /* Add periodic node in event_list */
    strcpy(event_code, "2 PERIODIC");
    strcpy(cmd_key, "");
    res = add_event(event_code, cmd_key);
    if(res != 0) {
        LOG(m_event, ERROR, "Add periodic node in event_list failed\n");
        return FAIL;
    } else {
        LOG(m_event, DEBUG, "Add periodic node in event_list success\n");
    }
        
    /* Judge whether first installation and IP ready */
 
    res = dev_func(first_install_func);
    if (res != 0) {
        LOG(m_event, DEBUG, "Device is not first install.\n");
    } else {
        LOG(m_event, DEBUG, "Device is first install. Add 0 BOOTSTRAP event to event_list\n");
        strcpy(event_code, "0 BOOTSTRAP");
        strcpy(cmd_key, "");
        res = add_event(event_code, cmd_key);
        if (res != 0){
            LOG(m_event, ERROR, "Add BOOTSTRAP node in event list failed\n");
            return FAIL;
        }
     }
    /* Judge whether Boot because Call Reboot Method */
    if(flag_reboot == NEED_REBOOT) {
        strcpy(event_code, "M Reboot");
        res = add_event(event_code, reboot_cmd_key);
        if (res != 0) {
            LOG(m_event, ERROR, "Add Reboot node in event list failed\n");
            return FAIL;
        }
        res = get_agent_conf(&a_conf);
        if (res != 0) {
            LOG(m_handler, ERROR, "Invoke get_agent_conf failed.\n");
            return FAIL;
        }
        a_conf.flag_reboot = NOT_NEED_REBOOT;
        res = set_agent_conf(&a_conf);
        if(res != 0) {
            LOG(m_handler, ERROR, "Invoke get_agent_conf failed.\n");
            return FAIL;
        }
    }
    /* Judge whether Boot and IP ready */
    strcpy(event_code, "1 BOOT");
    strcpy(cmd_key, "");
    res = add_event(event_code, cmd_key);
    if (res != 0){
        LOG(m_event, ERROR, "Add BOOT node in event_list failed\n");
        return FAIL;
    }
    LOG(m_event, DEBUG, "Post Inform.\n");
    sem_post(&SEM_INFORM);

    /* Circularly set timer and call event_session() */
    while(1) {
        res = event_session();
        if(res == 0) {
            LOG(m_event, DEBUG, "call event_session() success and wait for timer interval to timeout.\n");
        } else {
            LOG(m_event, ERROR, "call event_session() failed.\n");
        }

    }
    return SUCCESS;
}
/*
***********************************************************************
* Function name:  init_first_time 
* Description:  Initialize first periodic time when init agent
* Parameter: None
* Return value: SUCCESS  FAIL
***********************************************************************
*/
int init_first_time() 
{
    time_t cur_time;
    int i = 0;
    cur_time = dev_get_cur_time();

    if (g_period_time < cur_time) {
        i = 1;
        while (((g_period_time + g_period_iv * i) - cur_time) < 0) 
            i++;
        g_first_time = g_period_time + g_period_iv * i;

        LOG(m_event, DEBUG, "g_first_time = %d\n", g_first_time);
        return SUCCESS;
                
    } else if (g_period_time == cur_time) {
        g_first_time = cur_time;
        return SUCCESS;
    } else {
        if ((g_period_time - cur_time) == g_period_iv) {
            g_first_time = cur_time;
        } else if ((g_period_time - cur_time) < g_period_iv) {
            g_first_time = g_period_time;
	    LOG(m_event, DEBUG, "g_first_time = %d\n", g_first_time);
        } else {
            i = 0;
            while (((int)g_period_time - i * (int)g_period_iv - (int)cur_time) > 0) {
                i++;
            }
            g_first_time = g_period_time - (i - 1) * g_period_iv;
            LOG(m_event, DEBUG, "g_first_time = %d\n", g_first_time);
        }
        return SUCCESS;
    }

}
/*
***********************************************************************
* Function name:  set_flag_periodtime_change 
* Description:  Set global variable flag_periodtime_change
* Parameter: the status of if changed periodtime
* Return value: void
***********************************************************************
*/
void set_flag_periodtime_change(int flag)
{
    pthread_mutex_lock(&flag_periodtime_change_lock);
    flag_periodtime_change = flag;
    pthread_mutex_unlock(&flag_periodtime_change_lock);
    
}
/*
***********************************************************************
* Function name: get_flag_periodtime_change 
* Description:  Get the value whether periodic time is changed
* Parameter: none
* Return value: SUCCESS FAIL
***********************************************************************
*/
int get_flag_periodtime_change()
{
    int flag;

    pthread_mutex_lock(&flag_periodtime_change_lock);
    flag = flag_periodtime_change;
    pthread_mutex_unlock(&flag_periodtime_change_lock);

    return flag;
}
/*
***********************************************************************
* Function name: set_g_period_iv 
* Description:  Set global variable period_iv
* Parameter: the interval of periodic inform
* Return value: none
***********************************************************************
*/
void set_g_period_iv(unsigned int iv)
{
    pthread_mutex_lock(&g_period_iv_lock);
    g_period_iv = iv;
    pthread_mutex_unlock(&g_period_iv_lock);
    
}
/*
***********************************************************************
* Function name: get_period_iv 
* Description:  Get global variable period_iv
* Parameter: none
* Return value: the interval of periodic inform
***********************************************************************
*/
unsigned int get_g_period_iv()
{
    unsigned int iv;

    pthread_mutex_lock(&g_period_iv_lock);
    iv = g_period_iv;
    pthread_mutex_unlock(&g_period_iv_lock);

    return iv;
}
/*
***********************************************************************
* Function name: set_period_enable 
* Description:set periodic enable with global variable periodic inform enable
* Parameter: periodic enable
* Return value: none
***********************************************************************
*/
void set_g_period_enable(int enable)
{
    pthread_mutex_lock(&g_period_enable_lock);
    g_period_enable = enable;
    pthread_mutex_unlock(&g_period_enable_lock);
}
/*
***********************************************************************
* Function name: get_period_enable 
* Description:get periodic enable with global variable periodic inform enable
* Parameter: none
* Return value: periodic enable
***********************************************************************
*/
int get_g_period_enable()
{
    int enable;

    pthread_mutex_lock(&g_period_enable_lock);
    enable = g_period_enable;
    pthread_mutex_unlock(&g_period_enable_lock);

    return enable;
}
/*
***********************************************************************
* Function name: set_period_time 
* Description:set periodic time
* Parameter: periodic time
* Return value: none
***********************************************************************
*/
void set_g_period_time(time_t temp)
{
    pthread_mutex_lock(&g_period_time_lock);
    g_period_time = temp;
    pthread_mutex_unlock(&g_period_time_lock);
}
/*
***********************************************************************
* Function name: get_period_time 
* Description:get periodic time
* Parameter: none
* Return value: periodic time
***********************************************************************
*/
time_t get_g_period_time()
{
    time_t temp;

    pthread_mutex_lock(&g_period_time_lock);
    temp = g_period_time;
    pthread_mutex_unlock(&g_period_time_lock);

    return temp;
}
/*
***********************************************************************
* Function name: set_timer 
* Description: Set timer with global variable timer_iv
* Parameter: interval
* Return value: SUCCESS FAIL
***********************************************************************
*/
int set_timer(int interval)
{
    struct timeval timeout;
    
    timeout.tv_sec = interval;
    timeout.tv_usec = 0;
    
    if (select(0, (fd_set *)NULL, (fd_set *)NULL, (fd_set *)NULL, &timeout) == -1){
        perror("Select error:");
        return FAIL;
    }
    return SUCCESS;
}
/*
***********************************************************************
* Function name: event_session 
* Description: When SEM_INFORM be post, this function will be trigger
* Parameter: none
* Return value: SUCCESS FAIL
***********************************************************************
*/
int event_session()
{
    int res;

    pthread_mutex_lock(&event_session_lock);

    sem_wait(&SEM_CONNECTED);
    sem_post(&SEM_CONNECTED);                 //for comm module
    LOG(m_event, DEBUG, "Wait SEM......\n");

    sem_wait(&SEM_INFORM);
    LOG(m_event, DEBUG, "SEM POST.\n");

    res = check_event();
    if(res == 0) {
       LOG(m_event, DEBUG, "check event_list and determine to send inform\n");
        
       res = start_session();
       if (res != 0) {
           LOG(m_event, ERROR, "Start session failed.\n");
           pthread_mutex_unlock(&event_session_lock);
           return FAIL;
       }
    } else {
        LOG(m_event, DEBUG, "check event_list and determine not to send inform\n");
    }

    pthread_mutex_unlock(&event_session_lock);

    return SUCCESS;
}
/*
***********************************************************************
* Function name: init_event_list
* Description: Initialize event_list_head and event_list_lock
* Parameter: none
* Return value: SUCCESS FAIL
***********************************************************************
*/

int init_event_list()
{
    int res;

    /* Initialize event_list_head */
    INIT_LIST_HEAD(&event_list_head);

    /* Initialize event_list_lock */
    res = pthread_mutex_init(&event_list_lock, NULL);
    if(res != 0) {
        LOG(m_event, ERROR, "Mutex event_list_lock initialization failed.\n");
        return FAIL;
    }

    res = pthread_mutex_init(&event_session_lock, NULL);
    if(res != 0)
    {
        LOG(m_event, ERROR, "Mutex event_session_lock initialization failed.\n");
        return FAIL;
    }
    /* Init period_enable_lock */
    res = pthread_mutex_init(&g_period_enable_lock, NULL);
    if (res != 0){
        LOG(m_event, ERROR, "Mutex g_period_enable_lock init failed\n");
        return FAIL;
    }

    
     /* Init period_time_lock */
    res = pthread_mutex_init(&g_period_time_lock, NULL);
    if (res != 0){
        LOG(m_event, ERROR, "Mutex g_period_time_lock init failed\n");
        return FAIL;
    }
    
     /* Init period_time_lock */
    res = pthread_mutex_init(&g_period_iv_lock, NULL);
    if (res != 0){
        LOG(m_event, ERROR, "Mutex g_period_iv_lock init failed\n");
        return FAIL;
    }
 
    return SUCCESS;
}
/*
***********************************************************************
* Function name: destroy_event_list
* Description: Destroy event_list_head and event_list_lock
* Parameter: none
* Return value: SUCCESS FAIL
***********************************************************************
*/
int destroy_event_list()
{
    struct event_list *event_list_node;
    struct list_head *p;
    int res;

    pthread_mutex_lock(&event_list_lock);
    if(!list_empty(&event_list_head)) {
        list_for_each_entry(event_list_node, &event_list_head, node) {
            p = event_list_node->node.prev;               
            list_del(&event_list_node->node);        
            free(event_list_node);               
            event_list_node = list_entry(p, struct event_list, node);       
        }
    }
    pthread_mutex_unlock(&event_list_lock);

    res = pthread_mutex_destroy(&event_list_lock);
    if(res != 0) {
        LOG(m_event, ERROR, "Mutex event_list_lock destroy failed.\n");
        return FAIL;
    }

    return SUCCESS;
}
/*
***********************************************************************
* Function name: modify_event_list
* Description: Modify event_list_head, delete the node sent except periodic node
* Parameter:
* Return value: SUCCESS FAIL
***********************************************************************
*/
int modify_event_list()
{
    struct list_head *p;
    struct event_list *event_list_node;

    pthread_mutex_lock(&event_list_lock);

    if(!list_empty(&event_list_head)) {
        list_for_each_entry(event_list_node, &event_list_head, node) {
            if(event_list_node->sent_flag == 1) {
                
                if(strcmp(event_list_node->event_code, "2 PERIODIC") == 0) {
                    event_list_node->sent_flag = 0;
                } else {
                    p = event_list_node->node.prev;
                    list_del(&event_list_node->node);
                    free(event_list_node);
                    event_list_node = list_entry(p, struct event_list, node);
                }
           }
        }
    }
    /*******display event_list*********/
    if(!list_empty(&event_list_head)) {
        list_for_each_entry(event_list_node, &event_list_head, node) {   
            LOG(m_event, DEBUG, "%s existed in event_list\n", event_list_node->event_code);
        }
    } 
    pthread_mutex_unlock(&event_list_lock);

    return SUCCESS;
}

/*
***********************************************************************
* Function name: add_event
* Description: Add a node in event_list.
* Parameter: event_code, cmd_key
* Return value: SUCCESS FAIL
***********************************************************************
*/
int add_event(char event_code[], char cmd_key[])
{
    struct event_list *event_list_node = NULL; 
    int F_exist = 0;
    struct event_list *event_list_node_traval = NULL;
    
    pthread_mutex_lock(&event_list_lock);
    if(!list_empty(&event_list_head)) {
        list_for_each_entry(event_list_node_traval, &event_list_head, node) {
            if(strcmp(event_list_node_traval->event_code, event_code) == 0)  {
                F_exist = 1;
                break;
            }
        }
    }
    if(F_exist == 0) {
        event_list_node = (struct event_list *)malloc(sizeof(struct event_list));
        if(event_list_node == NULL) {
            LOG(m_event, ERROR, "malloc for event_list_node failed\n");
            pthread_mutex_unlock(&event_list_lock);
            return SUCCESS;
        }
        strcpy(event_list_node->event_code, event_code);
        strcpy(event_list_node->cmd_key, cmd_key);
        event_list_node->sent_flag = 0;

        list_add_tail(&event_list_node->node, &event_list_head);
    } else {
        LOG(m_event, DEBUG, "node %s already exists in event_list\n", event_code);
    }
    pthread_mutex_unlock(&event_list_lock);
    
    return SUCCESS;
}
/*
***********************************************************************
* Function name: del_event
* Description: delete a node in event_list.
* Parameter: event_code
* Return value: SUCCESS FAIL
***********************************************************************
*/
int del_event(char event_code[])
{
    struct event_list *event_list_node;
    pthread_mutex_lock(&event_list_lock);
    if(!list_empty(&event_list_head)) {
        list_for_each_entry(event_list_node, &event_list_head, node) {
            if ((strcmp(event_list_node->event_code, event_code)) == 0) {
                list_del(&event_list_node->node);
                free(event_list_node);
                pthread_mutex_unlock(&event_list_lock);
                return 0;
            }
        }
    }
    pthread_mutex_unlock(&event_list_lock);
    return -1;     
}
/*
***********************************************************************
* Function name: check_event
* Description: Check event_list to determine whether to send inform
* Parameter: 
* Return value: 0:need trigger inform, -1: neednot trigger inform
***********************************************************************
*/
int check_event()
{
    int ret = -1;
    struct event_list *event_list_node;

    pthread_mutex_lock(&event_list_lock);
    if(!list_empty(&event_list_head)) {
        list_for_each_entry(event_list_node, &event_list_head, node) {
            if (strcmp(event_list_node->event_code, "M RETRY CONNECT") == 0) {  
                LOG(m_event, DEBUG, "RETRY_CONNECT event existed in the event_list.\n");
                LOG(m_event, DEBUG, "retry_interval = %d\n", retry_interval);
               // if (event_list_node->sent_flag == 0) {
                    set_timer(retry_interval);
                    pthread_mutex_unlock(&event_list_lock);
                    return 0;
              //  }
            } else if (strcmp(event_list_node->event_code, "2 PERIODIC") == 0) {
                LOG(m_event, DEBUG, "PERIODIC event existed in the event_list.\n");
                LOG(m_event, DEBUG, "g_period_iv = %d\n", g_period_iv);
                if (event_list_node->sent_flag == 1) {
                 //   pthread_mutex_unlock(&event_list_lock);
                    ret = 0;
                }
            } else{
                  LOG(m_event, DEBUG, "other event in the event_list.\n"); 
              //  if (event_list_node->sent_flag == 0) {
                    LOG(m_event, DEBUG, "event_code = %s is in event list\n", event_list_node->event_code);            
                 //     pthread_mutex_unlock(&event_list_lock);
                    ret = 0 ;   
             //   }
            }
        }
    }
    pthread_mutex_unlock(&event_list_lock);

    return ret;
}
/*
***********************************************************************
* Function name: dev_func
* Description: call a function of dll
* Parameter:  dev_func_name
* Return value: SUCCESS FAIL
***********************************************************************
*/
int dev_func(char dev_func_name[])
{
    int res;
    void *handle;
    int (*func)();
    char *error;
 
    handle = dlopen(dev_lib_path, RTLD_LAZY);
    if(!handle) { 
        LOG(m_event, ERROR, "%s\n", dlerror());
        return FAIL;
    }
    func = dlsym(handle, dev_func_name);
    if((error = dlerror()) != NULL) {
        LOG(m_event, ERROR, "%s\n", error);
        dlclose(handle);
        return FAIL;
    }
    res = (*func)();
    if (res != -1) {
        LOG(m_event, DEBUG, "Call dll device func success.\n");
        dlclose(handle);
        return res;
    } 
    dlclose(handle);
    LOG(m_event, ERROR, "Call dll device func failed.\n");
    return FAIL;
}
