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
*	CLI.h - Command Line Interface
*
*	$Author: joinsonj $
*
*	history: $Date: 2007-06-08 02:22:53 $, initial version by simonl
*
***********************************************************************/


/***********************************************************************
*
*	include file
*
***********************************************************************/

#include "CLI.h"

/**********************************************************************
*
*	function prototype
*
***********************************************************************/
static int read_msg(int mqid, long type, char *text);
static int creat_msgq();
static int send_msg(int mqid, long type, char *text);
static void *func_CLI(void *arg);
static int process_mq(char *mq_buf);
static int conn_req();
static int ACL_changed();
static int val_changed(char *params[], int n);
static int add_changed_param(char *param);

/*
 ********************************************************************
 * Function name: send_msg();
 * Description: Send message queue.
 * Parameter: int mqid:id of mq; long type: type of send mq; char *text:text
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int send_msg(int mqid, long type, char *text)
{
    int res = 0;

    struct msgbuf {
        long mtype;
        char mtext[MAX_SEND_SIZE];
    }msg;
    msg.mtype = type;
    strcpy(msg.mtext, text);
    res = msgsnd(mqid, &msg, strlen(msg.mtext) + 1, 0);
    if(res == -1) {
        LOG(m_CLI, ERROR, "msgsnd failed.\n");
        return FAIL;
    }
    return SUCCESS;
}
/*
 ********************************************************************
 * Function name: read_msg();
 * Description: Read message queue.
 * Parameter: int mqid:id of mq; long type: type of recv mq; char *text:text
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int read_msg(int mqid, long type, char *text)
{
    int res = 0;
    int flag = 0;
    struct msgbuf {
        long mtype;
        char mtext[MAX_READ_SIZE];
    }msg;    
//    msg.mtype = type;
    res = msgrcv(mqid, &msg, MAX_READ_SIZE, type, flag); 
    if (res == -1) {
        LOG(m_CLI, ERROR, "msgrcv failed.\n");
        return FAIL;
    }
    strcpy(text, msg.mtext);
    return SUCCESS;
}
/*
 ********************************************************************
 * Function name: Creat_msg();
 * Description: Creat message queue.
 * Parameter: 
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
 */
int creat_msgq()
{
    key_t mqkey;
    int oflag,mqid;

    char filenm[] = "agent_mq";
    oflag = IPC_CREAT;

    mqkey = ftok(filenm,PROJID);
    mqid = msgget(mqkey, oflag);
    if (mqid == -1){
        LOG(m_CLI, ERROR, "msgget failed.\n");
        return FAIL;
    }
    return mqid;
}
/*
***********************************************************************
* Function name:  cli_conn_req
* Description:  Write "0" to FIFO file.
* Parameter: 
* Return value: SUCCESS  FAIL
***********************************************************************
*/
int cli_conn_req()
{
    int mqid;
    int res;
    int type = 1;
    char text[256];

    strcpy(text, CONN_REQ);

    res = creat_msgq();
    if(res == -1) {
        LOG(m_CLI, DEBUG, "Creat message queue failed.\n");
        return FAIL;
    }
    mqid = res;
    res = send_msg (mqid, type, text);
    if(res == -1) {
        LOG(m_CLI, DEBUG, "Send message queue failed.\n");
        return FAIL;
    }
    return SUCCESS;    
}
/*
********************************************************************
* Function name:  init_CLI
* Description:  The entrance of CLI module. 
* Parameter: 
* Return value: 0:SUCCESS  -1:FAIL
************************************************************************
*/
int init_CLI()
{
    int res;
    pthread_t p_CLI;
    pthread_attr_t p_CLI_attr;
    
    res = pthread_attr_init(&p_CLI_attr);
    if(res != 0) {
        LOG(m_CLI, ERROR, "Thread attribute creation failed\n");
        return FAIL;
    } 
    res = pthread_attr_setdetachstate(&p_CLI_attr, PTHREAD_CREATE_DETACHED);
    if(res != 0) {
        LOG(m_CLI, ERROR, "Set detached attribute failed\n");
        return FAIL;
    } 
    res = pthread_create(&p_CLI, &p_CLI_attr, func_CLI, NULL);
    if(res != 0) {
        LOG(m_CLI, ERROR, "Create thread p_CLI failed\n");
        return FAIL;
    } 
    
    (void)pthread_attr_destroy(&p_CLI_attr);
    
    return SUCCESS;
}
/*
********************************************************************
* Function name:  init_changed_param_name_list
* Description:   Initialize changed_param_name_list_head and changed_param_name_list_lock 
* Parameter: 
* Return value: 0:SUCCESS  -1:FAIL
************************************************************************
*/
int init_changed_param_name_list()
{
    int res;

    /* Initialize changed_param_name_list_head */
    INIT_LIST_HEAD(&changed_param_name_list_head);

    /* Initialize changed_param_name_list_lock */
    res = pthread_mutex_init(&changed_param_name_list_lock, NULL);
    if(res != 0) {
        LOG(m_CLI, ERROR, "Mutex changed_param_name_list_lock initialization failed.\n");
        return FAIL;
    }
    return SUCCESS;
}
/*
********************************************************************
* Function name:  display_changed_param_name_list
* Description:   Display all nodes in changed_param_name_list.
* Parameter: 
* Return value: 0:SUCCESS  -1:FAIL
************************************************************************
*/
int display_changed_param_name_list()
{
    struct changed_param_name_list *param_list_node;
    int i = 0;

    LOG(m_CLI, DEBUG, "\nThe following are the nodes in changed_param_name_list:\n\n");
    pthread_mutex_lock(&changed_param_name_list_lock);
    if(!list_empty(&changed_param_name_list_head)) {
        list_for_each_entry(param_list_node, &changed_param_name_list_head, node) {
            i++;
            LOG(m_CLI, DEBUG, "node %d is %s\n", i, param_list_node->name);
        }
    }
    pthread_mutex_unlock(&changed_param_name_list_lock);
    return SUCCESS;
}
/*
********************************************************************
* Function name:  flush_changed_param_list
* Description:   Delete all nodes in changed_param_name_list.
* Parameter: 
* Return value: 0:SUCCESS  -1:FAIL
************************************************************************
*/
int flush_changed_param_list()
{
    struct list_head *p;
    struct changed_param_name_list *param_list_node;

    pthread_mutex_lock(&changed_param_name_list_lock);
    if(!list_empty(&changed_param_name_list_head)) {
        list_for_each_entry(param_list_node, &changed_param_name_list_head, node) {
            p = param_list_node->node.prev;
            list_del(&param_list_node->node);
            free(param_list_node);
            param_list_node = list_entry(p, struct changed_param_name_list, node);
        }
    }
    pthread_mutex_unlock(&changed_param_name_list_lock);
    
    return SUCCESS;
}
/*
********************************************************************
* Function name:  func_CLI
* Description:    The start routine of thread p_CLI.
* Parameter: 
* Return value: void
************************************************************************
*/
void *func_CLI(void *arg)
{
    int res;
    char text[MAX_READ_SIZE];
    int mqid = 0;
    long type = 0;
    int sockfd;
    struct sockaddr_in cli_addr;

    LOG(m_CLI, DEBUG, "Thread p_CLI is running.\n");
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        LOG(m_CLI, ERROR, "Create socket failed.\n");
        pthread_exit(NULL);
    } 
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(CLI_PORT);
    cli_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(cli_addr.sin_zero), 8);
    for(;;){
        res = bind(sockfd, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr));
        if (res == -1) {
            LOG(m_CLI, ERROR, "Bind sockfd failed.\n");
        //    pthread_exit(NULL);
            sleep(3);
            continue;
        }
        break;
    }
    res = listen(sockfd, BACKLOG);
    if (res == -1) {
        LOG(m_CLI, ERROR, "Listen sockfd failed.\n");
        pthread_exit(NULL);
    }
    LOG(m_CLI, DEBUG, "listen ok.\n");

    res = creat_msgq();
    if(res == -1) {
        LOG(m_CLI, DEBUG, "Creat message queue failed.\n");
        pthread_exit(NULL);
    }
    mqid = res;
    while(1){
        memset(text, '\0', MAX_READ_SIZE);
        res = read_msg (mqid, type, text);
        if(res == -1) {
            LOG(m_CLI, DEBUG, "Read message queue failed.\n");
        }
 
        LOG(m_CLI, DEBUG, "text = %s\n", text);
        pthread_mutex_lock(&tree_lock);
        res = process_mq(text);
        pthread_mutex_unlock(&tree_lock);
        if(res == 0) {
             LOG(m_CLI, DEBUG, "Invoke process_mq() success\n");
        } else {
             LOG(m_CLI, ERROR, "Invoke process_mq() failed\n");
         }
    }    
    pthread_exit(NULL);
}
/*
********************************************************************
* Function name:  process_mq()
* Description:    process text of message queue
* Parameter: mq_buf: content of message queue
* Return value: SUCCESS, FAIL
************************************************************************
*/
int process_mq(char *mq_buf)
{
    int res = 0, i = 0;
    char *delim = " ", *name[100]; //name is be nued to store param name or obj name pointer
    char *first_char;
    char param_name[128];

    first_char = strtok(mq_buf, delim);
    if(strcmp(first_char, CONN_REQ) == 0) {
        res = conn_req();
        if(res == 0) {
            LOG(m_CLI, DEBUG, "call conn_req() success.\n");
        } else {
            LOG(m_CLI, ERROR, "call conn_req() failed.\n");
            return FAIL;
        }
        sem_post(&SEM_INFORM);

    } else if(strcmp(first_char, ACL_CHANGED) == 0) {	
        res = ACL_changed();
        if(res == 0) {
            LOG(m_CLI, DEBUG, "call ACL_changed() success.\n");
        } else {
            LOG(m_CLI, ERROR, "call ACL_changed() failed.\n");
            return FAIL;
        }
        sem_post(&SEM_INFORM);

    } else if(strcmp(first_char, SET_LOGGER) == 0) {
        /*read level, mode, file_size and module from FIFO string*/
        int level = atoi(strtok(NULL, delim));
        int mode = atoi(strtok(NULL, delim));
        int file_size = atoi(strtok(NULL, delim));
        int module = atoi(strtok(NULL, delim));

        res = set_logger(level, mode, file_size, module);
        if(res == 0) {
            LOG(m_CLI, DEBUG, "call set_logger() success.\n");
        } else {
            LOG(m_CLI, ERROR, "call set_logger() failed.\n");
            return FAIL;
        }
    } else if(strcmp(first_char, VAL_CHANGED) == 0) {
        i = 0;
        while((name[i] = strtok(NULL, delim))) {
            i++;
        }
        res = val_changed(name, i);
        if(res == -1){
            LOG(m_CLI, ERROR, "call val_changed() failed.\n");
            return FAIL;
        } else if (res == 2) {
            sem_post(&SEM_INFORM);
            return SUCCESS;
        } else {
            LOG(m_CLI, DEBUG, "Notificaion is not 2, do nothing.\n");  
            return SUCCESS;
        }    
    } else if(strcmp(first_char, ADD_OBJ) == 0) {
        i = 0;
        int  j = 0;
        int instance_num[100];
      
        while((name[i] = strtok(NULL, delim))) {  
	   instance_num[i] = atoi(strtok(NULL,delim));
           LOG(m_CLI, DEBUG, "name = %s\n", name[i]);
           LOG(m_CLI, DEBUG, "instance_num = %d\n", instance_num[i]);
           
           i++;
        }
        for (j = 0; j < i; j++) {
            
            res = add_multi_obj(name[j], instance_num[j]);
            if(res != 0) {
                LOG(m_CLI, ERROR, "Add object %s failed\n", name[j]);
                return FAIL;
            }
        }
            
    } else if(strcmp(first_char, DEL_OBJ) == 0) {
        i = 0;
        /*read the objects name from FIFO string*/
        while((name[i] = strtok(NULL, delim))) {
            res = del_multi_obj(name[i]);
            if (res != 0){
                LOG(m_CLI, ERROR, "Del object %s failed\n", name[i]);
                return FAIL;
            }
            i++;
        }
    } else if(strcmp(first_char, SET_RETRY_TIMES) == 0) {
        /*read interval from FIFO string*/
        int retrytimes;
        retrytimes = atoi(strtok(NULL, delim));
     
        set_max_try_time(retrytimes);  //comm module 
    } else if(strcmp(first_char, SET_PERIOD_IV) == 0) {
        /*read interval from FIFO string*/
        int interval = atoi(strtok(NULL, delim));
        set_g_period_iv(interval);
        strcpy(param_name, PERIODINTERVAL);
        name[0] = param_name;
        res = val_changed(name, 1);
        if(res == -1){
            LOG(m_CLI, ERROR, "call val_changed() failed.\n");
            return SUCCESS;
        } else if (res == 2) {
            sem_post(&SEM_INFORM);
            return SUCCESS;
        } else
            return SUCCESS;
    } else if (strcmp(first_char, SET_PERIOD_ENABLE) == 0) {
        int enableval = atoi(strtok(NULL, delim));

        set_g_period_enable(enableval);
        strcpy(param_name, PERIODENABLE);
        name[0] = param_name;
        
        res = val_changed(name, 1);
        if(res == -1){
            LOG(m_CLI, ERROR, "call val_changed() failed.\n");
            return FAIL;
        } else if (res == 2) {
            sem_post(&SEM_INFORM);
            return SUCCESS;
        } else
            return SUCCESS;
        
    } else if (strcmp(first_char, SET_PERIOD_TIME) == 0) {
        time_t tmp = atoi(strtok(NULL, delim));
  
        set_g_period_time(tmp);
        set_flag_periodtime_change(PERIOD_TIME_CHANGE);
        strcpy(param_name, PERIODTIME);
        name[0] = param_name;
        res = val_changed(name, 1);
        if(res == -1){
            LOG(m_CLI, ERROR, "call val_changed() failed.\n");
            return FAIL;
        } else if (res == 2) {
            sem_post(&SEM_INFORM);
            return SUCCESS;
        } else
            return SUCCESS;
       
    } else {
        LOG(m_CLI, ERROR, "Receive wrong MQ string.\n");
    }
    return SUCCESS;
}
/*
********************************************************************
* Function name:  conn_req
* Description:    Add event "6 CONNECTION REQUEST" to event list and post sem to trigger inform
* Parameter: 
* Return value: SUCCESS, FAIL
************************************************************************
*/
int conn_req()
{
    int res; 
    char event_code[65];
    char cmd_key[33];

    /* add_event(Connection Request) */
    strcpy(event_code, "6 CONNECTION REQUEST");
    strcpy(cmd_key, "");

    res = add_event(event_code, cmd_key);
    if(res != 0) {
        LOG(m_CLI, ERROR, "Add Connection Request node in event_list failed\n");
        return FAIL;
    } else {
        LOG(m_CLI, DEBUG, "Add Connection Request node in event_list success\n");
    }
    return SUCCESS;
}
/*
********************************************************************
* Function name:  ACL_changed
* Description:    Add a BOOTSTRAP node in event list and trigger inform
* Parameter: 
* Return value: SUCCESS, FAIL
************************************************************************
*/
int ACL_changed()
{
    int res;
    char event_code[65]; 
    char cmd_key[33]; 

    /*add BOOTSTRAP node in event_list*/
    
    strcpy(event_code, "0 BOOTSTRAP");
    strcpy(cmd_key, "");
    res = add_event(event_code, cmd_key);
    if(res != 0) {
        LOG(m_CLI, ERROR, "Add BOOTSTRAP node in event_list failed\n");
        return FAIL;
    } else {
        LOG(m_CLI, DEBUG, "Add BOOTSTRAP node in event_list success\n");
    }
    return SUCCESS;
}
/*
********************************************************************
* Function name:  val_changed
* Description:    Get the attributes of the changed parameter(s) in 
*                   parameters tree. Determine whether the parameter(s) 
*                   are allowed to changed and whether to notify ACS 
*                   server if allowed.
* Parameter: params is an array of changed parameters name string. 
*            n is the quantity of changed parameters
* Return value: -1: FAIL   0: not inform  1: inform next time  2: inform immediately
************************************************************************
*/
int val_changed(char *params[], int n)
{
    int i, res;
    int F_val_changed = 0;
    int F_inform = 0;
    struct TR_param *p_param = NULL;
    char event_code[65];
    char cmd_key[33];

    for(i = 0; i < n; i++) {
        p_param = (struct TR_param *)param_search(params[i]);
        if(p_param == NULL)
        {
            LOG(m_CLI, ERROR, "Not find %s.\n", params[i]);
            return FAIL;
        }
        /* Judge whether the parameter is allow to changed according 
         to accesslist, to be enhanced in the future */

        /* Determine whether to notify ACS server according to notification */       
        switch(p_param->notification) {
            case 0:
                F_inform = 0;
                F_val_changed = 0;
                break;
            case 1:
                res = add_changed_param(params[i]);
                if(res != 0) {
                    LOG(m_CLI, ERROR, "Invoke add_changed_param(%s) failed\n", params[i]);
                    return FAIL;
                } else {
                    LOG(m_CLI, DEBUG, "Invoke add_changed_param(%s) success\n", params[i]);
                }
                F_val_changed = 2;
                F_inform = 1;

                break;
            case 2:
                res = add_changed_param(params[i]);
                if(res != 0) {
                    LOG(m_CLI, ERROR, "Invoke add_changed_param(%s) failed\n", params[i]);
                    return FAIL;
                } else {
                    LOG(m_CLI, DEBUG, "Invoke add_changed_param(%s) success\n", params[i]);
                }

                F_val_changed = 2;
                F_inform = 2; //timer_action at once
                LOG(m_CLI, DEBUG, "p_param->notification = %d\n",p_param->notification);

                break;
            default:
                LOG(m_CLI, ERROR, "Wrong notification value(%d) of %s\n", p_param->notification, p_param->name);
        }
    }
   // display_changed_param_name_list();
    if(F_val_changed == 2) {
       
        //add a VALUE CHANGED node in event_list
        strcpy(event_code, "4 VALUE CHANGE");
        strcpy(cmd_key, "");
        res = add_event(event_code, cmd_key);
        if(res != 0) {
            LOG(m_CLI, ERROR, "Add VALUE CHANGED node in event_list failed\n");
            return FAIL;
        } else {
            LOG(m_CLI, DEBUG, "Add VALUE CHANGED node in event_list success\n");
        }
    }
   return F_inform;
}
/*
********************************************************************
* Function name:  add_changed_param
* Description:    Add a node whose name is param into changed_param_name_list.
* Parameter: param is the name of changed parameter.
* Return value: SUCCESS, FAIL
************************************************************************
*/
int add_changed_param(char *param)
{
    int F_exist = 0;
    struct changed_param_name_list *param_list_node_traval;
    struct changed_param_name_list *param_list_node;

    pthread_mutex_lock(&changed_param_name_list_lock);

    if(!list_empty(&changed_param_name_list_head)) {
        list_for_each_entry(param_list_node_traval, &changed_param_name_list_head, node)  {
            if(strcmp(param_list_node_traval->name, param) == 0) {
                F_exist = 1;
                break;
            }
        }
    }
    if(F_exist == 0) {
        param_list_node = (struct changed_param_name_list *)malloc(sizeof(struct changed_param_name_list));
        if(param_list_node != NULL) {
            LOG(m_CLI, DEBUG, "malloc for param_list_node success\n");
        } else {
            LOG(m_CLI, ERROR, "malloc for param_list_node failed\n");
            pthread_mutex_unlock(&changed_param_name_list_lock);
            return FAIL;
        }
        strcpy(param_list_node->name, param);
        list_add_tail(&param_list_node->node, &changed_param_name_list_head);
        LOG(m_CLI, DEBUG, "Add node %s into changed_param_name_list success\n", param);
    } else {
        LOG(m_CLI, DEBUG, "node %s already exists in changed_param_name_list\n", param);
    }
    pthread_mutex_unlock(&changed_param_name_list_lock);
   
    return SUCCESS;
}

/*
 ***********************************************************************
 * Function: int add_object() 
 * Description: This function will add object to object tree
 * Param:
 *     char *objname - pointer to object name string
       int instance_num - which object will be added
 * Return value:
 *     -1 is error, 0 is successful
 ************************************************************************
 */
int add_multi_obj(char *objname, int instance_num)
{
    struct TR_object *object;
    int res = 0;

    object = (struct TR_object *)param_search(objname);

    if (object == NULL){
        LOG(m_CLI, ERROR, "search object %s failed\n", objname);
        return FAIL;
    }
    res = add_object_tree(instance_num, object);
    if (res == -1) {
        LOG(m_CLI, ERROR, "Add obj to parameter tree failed.\n");
        return FAIL;
    }   
    return SUCCESS;
}

/*
 ******************************************************************
 * Function: del_multi_object(char *objname)
 * Description: This function will delete object to object tree
 * Param:
 *     char *objname - pointer to object name string
 * Return value:
 *     -1 is error, 0 is successful
 *******************************************************************
 */
int del_multi_obj(char *objname) //need instant_num 
{
    struct TR_object *object, *pre_layer_node;
    char path[256];
    int res;    
    LOG(m_CLI, DEBUG, "objname = %s\n", objname);
    object = (struct TR_object *)param_search(objname);
    if (object == NULL){
        LOG(m_CLI, ERROR, "search object %s failed\n", objname);
        return FAIL;
    }
    strcpy(path, objname);
    if (path[strlen(path) - 3] != '.'){
        LOG(m_CLI, ERROR, "object is not a instance of multi object\n");
        return FAIL;
    }
    
    path[strlen(path) - 2] = '\0';
    LOG(m_CLI, DEBUG, "The path object is %s\n", path);
    
    /*Search path from param */
    pre_layer_node = (struct TR_object *)param_search(path);
    if (pre_layer_node == NULL){
        LOG(m_CLI, ERROR, "Search object %s failed\n", path);
        return FAIL;
    }
    res = del_attri_conf(objname, object);
    if (res != 0) {
        LOG(m_CLI, ERROR, "Del attri.conf failed.\n");
        return FAIL;
    }    
    /*delete object from object tree*/
    del_object(pre_layer_node, object);

    
    return SUCCESS;
}
