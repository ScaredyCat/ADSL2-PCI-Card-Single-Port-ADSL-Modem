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
 ******************************************************************************
 * $Author: joinsonj $
 * $Date: 2007-06-08 06:50:31 $
 ******************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "../../tools/logger.h"
#include "../soap/soap.h"
#include "../xmltool/mxml.h"
#include "../../device/TRF_param.h"
#include "../cpe_task_list.h"
#include "../../handler/methods/methods.h"
 
/*
 ************************************************************************
 * Function name: int gen_addobj_resp()
 * Description: This function will gen response xml data for add object
 * Parameter: 
 *    int instance_num
 *    int status
 * Retunr value:
 *    SUCCESS is success, FAIL is fail
 ************************************************************************
 */

static int gen_addobj_resp(int instance_num, int status)
{
    TRF_node *obj_method = NULL, *status_node = NULL, *instance_num_node = NULL;
    char name[] = "AddObjectResponse";
    
    //generate envelope
    obj_method = gen_soap_frame(name);
    if (obj_method == NULL) {
        return FAIL;
    }

    //generate method's xml
    instance_num_node = mxmlNewElement (obj_method, "InstanceNumber");
    if (instance_num_node == NULL) {
        LOG (m_handler, ERROR, "build new element for xml tree failed\n");
        return FAIL;
    }
    
    //generate instance num element node
    if (mxmlNewInteger(instance_num_node,instance_num) == NULL) {
        LOG (m_handler, ERROR, "build new text failed\n");
        return FAIL;
    }
    
    status_node = mxmlNewElement (obj_method, "Status");
    if (status_node == NULL) {
        LOG (m_handler, ERROR, "build new element for xml tree\n");
        return FAIL;
    }
    if (mxmlNewInteger (status_node, status) == NULL) {
        LOG (m_handler, ERROR, "Build new element(status) for xml tree\n");
        return FAIL;
    }
    
    LOG (m_handler, DEBUG, "Generate add object response success\n");
    
    return SUCCESS;

}

 /*
 *********************************************************************************
 * Function name: int get_argument_val()
 * Description: get the argument value of request from ACS
 * Parameter:
 *    TRF_node *method - pointer to method node
 *    char *objname - pointer to string for store object name
 *    char *param_key - pointer to string for store param key
 * Return value:
 *     if error return FAIL , successful return SUCCESS
 ********************************************************************************
 */

static int get_argument_val(TRF_node *method, char *objname, char *param_key)
{
    int flag1 = -1, flag2 = -1;
    
    //check method
    if (method -> child == NULL) {
        LOG (m_handler, DEBUG, "empty method body\n");
        return FAIL;
    }

    //get add object argument value
    method = method -> child;
    while (method -> value.element.name != NULL ) {
        if (!(strcmp (method -> value.element.name, "ObjectName"))) {
            if (method -> child == NULL) {
                LOG (m_handler, DEBUG, "argument ObjectName is empty\n");
                return FAIL;
            }
            else
                strncpy (objname, method -> child -> value.opaque, PARAM_FULL_NAME_LEN);
            flag1 = 1;
        }

        if (!(strcmp (method -> value.element.name, "ParameterKey"))) {
            if (method -> child == NULL)
                strcpy(param_key, "");
            else
                strncpy(param_key, method -> child-> value.opaque, PK_LEN);
            flag2 = 1;
        }
        if (flag1 == 1 && flag2 == 1)
            break;
        //get the next node pointer
        method = method -> next;
    }
    //compare falag value
    if (flag1 == -1 || flag2 == -1) {
        LOG (m_handler, DEBUG, "Only have one argument or none in ACS request\n");
        return FAIL;
    }
    
    return SUCCESS;
}

 /*
 *********************************************************************************
 * Function name: int get_argument_val()
 * Description:   set parameter key into device
 * Parameter:     int *status
 *                char param_key[]
 *                char method[]
 * Return value:
 *     if error return FAIL , successful return SUCCESS, have fault code return HAVE_FAULT_CODE. 
 ********************************************************************************
 */

static int set_param_key_device(int status, char param_key[], char method[])
{  
    int res = 0, fault_code = 0, have_fault_code = 0;

    res = call_dev_func(pk_dev_func, SET_OPT, param_key, NULL);
    switch (res) {
            case SET_VAL_SUCCESS:
                LOG(m_handler, DEBUG, "set param_key to device success and doesn't need reboot!\n")
                break;
            case SET_NEED_REBOOT:
                if (status == 0) {
                    res = add_task_list("dev_reboot", NULL);
                    if (res != 0) {
                        LOG(m_handler, ERROR, "add task list error\n");
                        fault_code = INTERNAL_ERROR;
                        have_fault_code = 1;
                    }
                }
                LOG(m_handler, DEBUG, "set param_key to device success and need reboot!\n");
                break;
            case SET_INVALID_PARAM_VAL:
                LOG(m_handler, DEBUG, "set param_key value invalid !\n");
                fault_code = INVALID_PARAM_VAL;
                have_fault_code = 1;
                break;
            default:
                LOG(m_handler, DEBUG, "set param_key unknown error!\n");
                fault_code = INTERNAL_ERROR;
                have_fault_code = 1;
                break;
    }

    if (have_fault_code) {
        if (gen_method_fault(xmlroot, method, fault_code) == NULL) {
            return FAIL;
        }
        return HAVE_FAULT_CODE;
    }
    return SUCCESS;
}

 /*
 *******************************************************************************
 * Function name: int process_addobj():
 * Description: parse ACS AddObject request, and generate response soap package
 * Parameter:
 *         method - the AddObject method node in xml tree   
 * return value:
 *         SUCCESS      process Addobject sucsess
 *         FAIL         process Addobject failure
 *******************************************************************************
 */

int process_addobj(TRF_node *method)
{
    int res = 0, status = 0, fault_code = 0, have_fault_code = 0;
    int instance_num = 0, check_num = 0;
    char add_method[] = "AddObject";
    struct __object add_obj;
    struct TR_object *obj_node = NULL, *obj_tmp = NULL;

    //get ACS request arguments
    res = get_argument_val (method, add_obj.object_name, add_obj.param_key);
    if (res != 0) {
        if (gen_method_fault (xmlroot, "AddObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    
    //check object name is validate
    if (add_obj.object_name[strlen(add_obj.object_name) - 1] != '.') {
        LOG (m_handler, DEBUG, "ObjectName is not a partial name\n");
        if (gen_method_fault (xmlroot, "AddObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    
    obj_node = (struct TR_object *)param_search(add_obj.object_name);
    if (obj_node == NULL) {
        if (gen_method_fault (xmlroot, "AddObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }

    if (isdigit(obj_node -> name[0])) {
        LOG(m_handler, ERROR, "Object path is invalid!\n");
        if (gen_method_fault (xmlroot, "AddObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }

    if (obj_node->writable == 0) {
        if (gen_method_fault (xmlroot, "AddObject", REQUEST_DENY) == NULL){
            return FAIL;
        }
        return SUCCESS;
    }

    //check device multi object whether is less than max_instance
    LOG(m_handler, DEBUG, "max instance num is %d\n", obj_node -> max_instance);
    obj_tmp = obj_node->next_layer;
    LOG(m_handler, DEBUG, "object name is %s\n", obj_tmp->name);
    obj_tmp = obj_tmp -> next;
    while (obj_tmp != NULL) {
        check_num ++;
        obj_tmp = obj_tmp -> next;
    }
    LOG(m_handler, DEBUG, "device exist multi num is %d\n", check_num);         
    if (check_num >= obj_node -> max_instance) {
        LOG(m_handler, ERROR, "device exist multi object num is equal to or more than max isntance!\n");
        if (gen_method_fault (xmlroot, "AddObject", RESOURCE_EXCEED) == NULL){
            return FAIL;
        }
        return SUCCESS;
    }   
 
    LOG (m_handler, DEBUG, "Start call func to add object\n");
    res = call_dev_func(obj_node->obj_dev_func, ADD_OPT, &instance_num, obj_node->locate);
    switch (res) {
            case ADD_OBJ_SUCCESS:
                LOG(m_handler, DEBUG, "add object to device success and doesn't need reboot!\n");
                break;
            case ADD_NEED_REBOOT:
                res = add_task_list("dev_reboot", NULL);
                if (res != 0) {
                    LOG(m_handler, ERROR, "add task list error\n");
                    fault_code = INTERNAL_ERROR;
                    have_fault_code = 1;
                }
                status = 1;                              //reboot_flag = NEED_REBOOT;
                LOG(m_handler, DEBUG, "add object to device success and need reboot!\n");
                break;
            default:
                fault_code = INTERNAL_ERROR;
                have_fault_code = 1;
                LOG(m_handler, ERROR, "add object to device fail!\n");
                break;
    }

    if (have_fault_code){
        if (gen_method_fault (xmlroot, "AddObject", fault_code) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    
    res = add_object_tree(instance_num, obj_node);
    if (res != 0) {
        if (gen_method_fault (xmlroot, "AddObject", INTERNAL_ERROR) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    
    LOG(m_handler, DEBUG, "status:%d\n", status);
 
    //write parameter key value to configure file
    res = set_param_key_device(status, add_obj.param_key, add_method);
    if (res == 1) {
        LOG(m_device, DEBUG, "have fault code !\n");
        return SUCCESS;
    } else if (res == -1) {
          return FAIL;
    }
  
    //Generate add object response
    res = gen_addobj_resp (instance_num, status);
    if (res != 0) {
        if (gen_method_fault(xmlroot, "AddObject", INTERNAL_ERROR) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    return SUCCESS;
}

 /*
 ************************************************************************
 * Function name: int gen_delobj_resp()
 * Description: Generate delete object response data
 * Parameter:
 *     TRF_node *method - poniter of method name
 *     int status - status value
 * Return value:
 *     FAIL is error , SUCCESS is successful
 ***********************************************************************
 */

static int gen_delobj_resp(int status_val)
{
    TRF_node *obj_method = NULL, *stats = NULL;
    char name[] = "DeleteObjectResponse";

    //generate envelope
    obj_method = gen_soap_frame(name);
    if (obj_method == NULL) {
        return FAIL;
    }   
    //generate method's xml
    stats = mxmlNewElement (obj_method, "Status");
    if (stats == NULL) {
        LOG(m_handler, ERROR, "build new element for xml tree fail!\n");
        return FAIL;
    }

    if (mxmlNewInteger (stats, status_val) == NULL) {
        LOG(m_handler, ERROR, "build new text failed\n");
        return FAIL;
    }
    return SUCCESS;
}

 /*
 *******************************************************************************
 * Function name : process_delobj
 * Description:    parse ACS DeleteObject request, and generate response soap package
 * parameter:
 *         method - the DeleteObject method node in xml tree
 * return value:
 *         SUCCESS      process deleteobject sucsess
 *         FAIL         process deleteObject failure
 ********************************************************************************
 */

int process_delobj(TRF_node *method)
{
    int res = 0, status = 0, fault_code = 0, have_fault_code = 0;
    int instance_num = 0;
    char path[256];
    char del_method[] = "DeleteObject";
    struct __object del_obj;
    struct TR_object* obj_node = NULL, *prev_node = NULL;

    //get argument ACS del object request
    res = get_argument_val (method, del_obj.object_name, del_obj.param_key);
    if (res != 0) {
        if (gen_method_fault (xmlroot, "DeleteObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    
    if (del_obj.object_name[strlen(del_obj.object_name) - 1] != '.') {
        if (gen_method_fault (xmlroot, "DeleteObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    
    path[0] = '\0';
    strcpy (path, del_obj.object_name);

    obj_node = (struct TR_object *)param_search(del_obj.object_name);
    if (obj_node == NULL) {                                                                                                  
        if (gen_method_fault (xmlroot, "DeleteObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    
    if (isdigit(obj_node -> name[0]) == 0) {
        LOG(m_handler, ERROR, "delete object path is invalid!\n");
        if (gen_method_fault (xmlroot, "DeleteObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }

    instance_num = atoi(obj_node->name);
        
    //get current object last layer object
    if (path[strlen(path) - strlen(obj_node->name)-2] != '.') {
        if (gen_method_fault (xmlroot, "DeleteObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    else {
        path[strlen(path) - strlen(obj_node->name) - 1] = '\0';
    }
    
    prev_node = (struct TR_object *)param_search(path);
    if (prev_node == NULL) {
        if (gen_method_fault (xmlroot, "DeleteObject", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }

    //check object whether writeable or not
    if (prev_node->writable == 0) {
        if (gen_method_fault (xmlroot, "DeleteObject", REQUEST_DENY) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    
    //call device function to del object
    LOG(m_handler, DEBUG, "Start call func to delete object\n");
    res = call_dev_func(prev_node->obj_dev_func, DEL_OPT, &instance_num, obj_node->locate);
    switch (res) {
            case DEL_OBJ_SUCCESS:
                LOG(m_handler, DEBUG, "Delete object to device success and doesn't need reboot!\n");
                break;
            case ADD_NEED_REBOOT:
                res = add_task_list("dev_reboot", NULL);
                if (res != 0) {
                    LOG(m_handler, ERROR, "add task list error\n");
                    fault_code = INTERNAL_ERROR;
                    have_fault_code = 1;
                }
                status = 1;                              //reboot_flag = NEED_REBOOT;
                LOG(m_handler, DEBUG, "Delete object to device success and need reboot!\n");
                break;
            default:
                fault_code = INTERNAL_ERROR;
                have_fault_code = 1;
                LOG(m_handler, ERROR, "Delete object to device fail\n");
                break;
    }

    LOG(m_handler, DEBUG, "status:%d\n", status);

    if (have_fault_code){
        if (gen_method_fault (xmlroot, "DeleteObject", fault_code) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
   
    res = set_param_key_device(status, del_obj.param_key, del_method);
    if (res == 1) {
        LOG(m_device, DEBUG, "have fault code !\n");
        return SUCCESS;
    } else if (res == -1) {
          return FAIL;
    }
    
    //delete attribute in the attri_conf
    res = del_attri_conf(del_obj.object_name, obj_node);
    if (res !=0 ) {
        LOG(m_handler, ERROR, "delete parameter path in attri_conf fail!\n");
        return -1;
    }
    
    //delete the object
    del_object (prev_node, obj_node);
    
    //generate response data
    res = gen_delobj_resp (status);
    if (res != 0) {
        return FAIL;
    }
    return SUCCESS;
}

