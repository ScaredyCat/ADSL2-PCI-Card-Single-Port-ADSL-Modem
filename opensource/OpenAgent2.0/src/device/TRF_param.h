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
 * $Author: joinsonj $ : $Date: 2007-06-08 02:17:00 $
 * @ function:
 *          receive parameters form command line, and write to fifo
 * @ return value:
 *    0 means right
 *    1 means wrong
 */

#ifndef TRF_PARAM_H_
#define TRF_PARAM_H_

/*macro defination*/
#define TR_RDONLY 0                      //read only
#define TR_WRONLY 1                      //write only
#define TR_RDWR 2                        //both read and write
#define DEVICE "InternetGatewayDevice"
#define MULTI_NUM 4                      //the number of the multi object
#define TR_NOTI_PASSIVE  1               //passive notification
#define TR_NOTI_ACTICE   2               //active notification
#define TR_NOTI_OFF      1               //notification off
#define TR_NOTI_RW       1               //both read and write
#define TR_NOTI_RDONLY   0               //read only

#define GET_OPT 0                        //get parameter value flag
#define SET_OPT 1                        //set parameter value flag

#define ADD_OPT 0                        //add object in device
#define DEL_OPT 1                        //delete object in device

#define SET_VAL_SUCCESS        0         //set parameter value success and doesn't need reboot
#define SET_NEED_REBOOT        1         //set parameter value success and need reboot
#define SET_INVALID_PARAM_VAL -2         //parameter value invalid
#define SET_VAL_FAILED        -1         //set parameter value fail

#define GET_VAL_SUCCESS        0         //get parameter value success 
#define GET_VAL_FAILED        -1         //get parameter value fail

#define ADD_OBJ_SUCCESS        0         //add object success and doesn't need reboot     
#define ADD_NEED_REBOOT        1         //add object success and need reboot
#define ADD_VAL_FAILED        -1         //add object fail

#define DEL_OBJ_SUCCESS        0         //del object success and doesn't need reboot
#define DEL_NEED_REBOOT        1         //del object success and need reboot
#define DEL_VAL_FAILED        -1         //del object fail

#define PARAM_NAME            60         //define character string length         
#define PARAM_DEV_FUNC_LEN    150

#define OBJECT_NAME           28
#define OBJ_DEV_FUNC_LEN      30
#define OBJ_INS_FUNC_LEN      30

#define TRF_ELE_TRF                         "trf"
#define TRF_ELE_OBJECT                      "obj"
#define TRF_ELE_PARAMETER                   "param"

#define TRF_ATTR_OBJECT_NAME                "name"
#define TRF_ATTR_OBJECT_WRITABLE            "rw"
#define TRF_ATTR_OBJECT_MAXINSTANCE         "max"
#define TRF_ATTR_OBJECT_OBJ_DEV_METHOD      "obj_dev_func"
#define TRF_ATTR_OBJECT_OBJ_INSTANCE_METHOD "obj_instance_func"

#define TRF_ATTR_PARAMETER_NAME             "name"
#define TRF_ATTR_PARAMETER_WRITABLE         "rw"
#define TRF_ATTR_PARAMETER_TYPE             "type"
#define TRF_ATTR_PARAMETER_DEV_METHOD       "dev_func"
#define TRF_ATTR_PARAMETER_NOTIFICATION     "noti"
#define TRF_ATTR_PARAMETER_NOTI_RW          "noti_rw"

#define TRF_ELE_LIB                         "lib"
#define TRF_ELE_INSTALL                     "install"
#define TRF_ELE_MANU                        "manu"
#define TRF_ELE_OUI                         "oui"
#define TRF_ELE_CLASS                       "class"
#define TRF_ELE_SERIAL                      "serial"
#define TRF_ELE_PARAMKEY                    "paramkey"
#define TRF_ATTR_OBJECT_WAN_DEV_FUNC        "wan_dev_func"
#define TRF_ELE_UPFIG                       "upfig"
#define TRF_ELE_UPLOG                       "uplog"

//SSL cret
#define TRF_ELE_CAERT                       "ca_cert"
#define TRF_ELE_CLERT                       "cl_cert"
#define TRF_ELE_CLKEY                       "cl_key"
//download
#define TRF_ELE_DOWNSYS                     "downsys"
#define TRF_ELE_DOWNKIL                     "downkil"
#define TRF_ELE_DOWNPAR                     "downpar"

//tasklist
#define TRF_ELE_TASKWST                     "taskwst"
#define TRF_ELE_TASKWEB                     "taskweb"
#define TRF_ELE_TASKFLA                     "taskfla"
#define TRF_ELE_TASKREB                     "taskreb"
#define TRF_ELE_TRCMD                       "trcmd"
//wandevice 
#define TRF_ELE_WANDEV                      "wandev"
#define NAME                                "name"

/*object tree root node*/
struct TR_object root;


/*parameter structure*/
struct TR_param {
    char name[PARAM_NAME];
    char param_type;
    int writable;
    int notification;                                // attributes of parameter
    int noti_rw;
    char dev_func[PARAM_DEV_FUNC_LEN];               // device devive function name,get/set parameter value
    char accesslist[65];
    struct TR_param* next;                           //point to the same layer parameter
    struct TR_object* parent;                        //point to parameter parent 
};

/*TRF object structure*/
struct TR_object{
    char name[OBJECT_NAME];
    int writable;
    int max_instance;
    int locate[MULTI_NUM];                            //the position of the object in the tree
    struct TR_param *param;                           //point to object's parameter
    struct TR_object *parent;                         //point to object parent
    struct TR_object *next_layer;                     //point to next layer object
    struct TR_object *next;
    char obj_dev_func[OBJ_DEV_FUNC_LEN];              //device function name ,add/delete object 
    char obj_instance_func[OBJ_INS_FUNC_LEN];         //device function name, get instance number
};

/*lock object tree*/
pthread_mutex_t tree_lock;

/*function offered by TRF_param.c*/
int init_object_tree(void);
void * param_search(char *);
void tr_free_obj(struct TR_object *obj_node);
int add_object_tree(int instance_num, struct TR_object* obj);
void del_object(struct TR_object *pre_layer_node, struct TR_object *del_node);
int del_attri_conf(char *path, struct TR_object *del_node);
#endif


