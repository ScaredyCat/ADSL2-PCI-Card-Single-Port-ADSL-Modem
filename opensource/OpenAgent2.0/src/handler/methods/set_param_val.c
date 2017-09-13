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
 * Include necessary file......
 */

#include "methods.h"
#include "../../device/TRF_param.h"
#include "../cpe_task_list.h"
#include <dlfcn.h>


/*
 * Variable .......
 */
static int param_struct_num;
static unsigned short reboot_flag;
static unsigned short have_fault_code;


TRF_node *pk;
typedef struct {
        TRF_node *name;
        TRF_node *val;
        struct TR_param *param;
        int fault_code;
}TR_param_val_struct;

TR_param_val_struct *param_struct;

/*
 * Declaration function .......
 */

static int gen_setparamval_resp(int status);
static int get_struct_num(TRF_node *node);
static int get_param_value(TRF_node *method);
static int get_pl_val(TRF_node *pl);
static int valid_param();
static int set_param_val();
static int gen_fault_response();
static void free_param_struct();
static int verifyNumber(int type, const char *value);
//static void transfer_type(char param_type, char *type);

/*
 ********************************************************************
 * Function: gen_setparamval_resp
 * Description: 
 * Parameter: 
 * Return value:
 *     success return 0, else return -1
 ********************************************************************
 */

static int gen_setparamval_resp(int status)
{
    char name[] = "SetParameterValuesResponse";
    TRF_node *method = NULL, *st = NULL;

    //generate soap frame
    method = gen_soap_frame(name);
    if (method == NULL) {
        return -1;
    }
    
    st = mxmlNewElement(method, "Status");
    if (!st) {
        return -1;
    }
    if (mxmlNewInteger(st, status) == NULL) {
        return -1;
    }
    
    LOG(m_handler, DEBUG, "generate response success\n");
    
    return 0;
}

/*
 *******************************************************************
 * Function:
 * Description:
 * Parameter:
 * Return Values:
 * 
 ******************************************************************
 */
int call_dev_func(char *func_name, int opt_flag, void *value, int locate[])
{
    int ret = -1;
    void *handle;
    int (*dev_func)(int, void *, int[]);
    const char *error;
    
    handle = dlopen(dev_lib_path, RTLD_LAZY);
    if (!handle) {
        LOG(m_handler, ERROR, "dlopen dll failed\n");
        return ret;
    }
    
    dev_func = dlsym(handle, func_name);
    if ((error = dlerror()) != NULL) {
        LOG(m_handler, ERROR, "ERROR : %s\n", error);
        dlclose(handle);
        return ret;
    }
    
    ret = dev_func(opt_flag, value, locate);
    
    if (dlclose(handle) != 0) {
        LOG(m_handler, ERROR, "Close handle failed\n");
        ret = -1;
    }
    
    return ret;
}

/*
 ************************************************************************
 * Function: get_struct_num
 * Description: get the number of the parameter value struct
 * Parameter: TRF_node *node - pointer the node of parameter list
 * Return Value:
 *     parameter struct number - success
 *     -1                      - failed
 ************************************************************************
 */
static int get_struct_num(TRF_node *node)
{
    char *attrval = NULL, *p = NULL, *q = NULL;
    
    attrval = (char *)mxmlElementGetAttr(node, ":arrayType");
    if (attrval == NULL) {
        return -1;
    }
    
    q = strchr(attrval, '[');
    if (q == NULL) {
        return -1;
    }
    
    p = q++;
    
    for (;;) {
        if (*p == ']') {
            p = '\0';
            break;
        }
        if (*p == '\0') {
            return -1;
        }
        p++;
    }
    
    return atoi(q);
}

/*
 *********************************************************************
 * Function: get_param_value
 * Description: get the parameter's name and value
 *              parameter key value
 * Parameter: TRF_node *method - pointer the node of RPC method
 * Return Value:
 *     
 *     
 *********************************************************************
 */
static int get_param_value(TRF_node *method)
{
    TRF_node *node, *pl;
    int j = 0;
    
    node = method->child;
    if (node == NULL) {
        return MTH_RET_FAILED;
    }
    LOG(m_handler, DEBUG, "Start get name value\n");
    do {
        if (!strcmp(node->value.element.name, "ParameterList")) {
            LOG(m_handler, DEBUG, "Find parameter list\n");
            param_struct_num = get_struct_num(node);
            if (param_struct_num < 0) {
                //return XML_FORMAT_ERROR;
                return MTH_RET_FAILED;
            } else if (param_struct_num > 0) {
                pl = node->child;
                param_struct = (TR_param_val_struct *)malloc(param_struct_num * sizeof(TR_param_val_struct));
                if (param_struct == NULL) {
                    LOG(m_handler, ERROR, "Memory is not enough\n");
                    //return FATAL_ERROR;
                    return MTH_RET_FAILED;
                }
                //init param struct value
                for (j = 0; j < param_struct_num; j++) {
                    param_struct[j].name = NULL;
                    param_struct[j].val  = NULL;
                    param_struct[j].param = NULL;
                    param_struct[j].fault_code = 0;
                }
                
                if (get_pl_val(pl) != 0) {
                    //return XML_FORMAT_ERROR;
                    return MTH_RET_FAILED;
                }
                
            }
        } else if (!strcmp(node->value.element.name, "ParameterKey")) {
            LOG(m_handler, DEBUG, "Find Parameter key node\n");
            //get parameterkey pointer
            pk = node;
            
        }
        node = node->next;
    } while (node != NULL);

    return MTH_RET_SUCCESS;
}

/*
 ***********************************************************************
 * Function: get_pl_val
 * Description: get the parameter's name and value
 * Parameter: TRF_node *pl
 *            TR_param_val_struct
 * Return Value: 
 *     XML_FORMAT_ERROR - failed
 *     0                - success
 ***********************************************************************
 */
static int get_pl_val(TRF_node *pl)
{
    TRF_node *node;
    int i = 0;

    while (pl != NULL) {
        if (i >= param_struct_num) {
            //return XML_FORMAT_ERROR;
            return MTH_RET_FAILED;
        }
        if (!strcmp(pl->value.element.name, "ParameterValueStruct")) {
            node = pl->child;
            while (node != NULL) {
                if (!strcmp(node->value.element.name, "Name")) {
                    param_struct[i].name = node;
                } else if (!strcmp(node->value.element.name, "Value")) {
                    param_struct[i].val = node;
                }
                node = node->next;
            }
            i++;
        }
        pl = pl->next;
    }
    
    return 0;
}

/*
 **********************************************************************
 * Function:
 * Description:
 * Parameter:
 * Return Values:
 *
 **********************************************************************
 */
/*static void transfer_type(char *param_type, char *type)
{
    if (!strcmp(type, "string")) {
        param_type = 's';
    } else if (!strcmp(type, "unsignedInt")) {
        param_type = 'u';
    } else if (!strcmp(type, "int")) {
        param_type = 'i';
    } else if (!strcmp(type, "boolean")) {
        param_type = 'b';
    } else if (!strcmp(type, "dateTime")) {
        param_type = 'd';
    } else if (!strcmp(type, "base64")) {
        param_type = 'B';
    }

    return;
}
*/

/*
 ************************************************************************
 * Function: valid_param
 * Description: check parameter name and value
 * Parameter: TR_param_val_struct *ps
 * Return value:
 *     0 - success
 ************************************************************************
 */
static int valid_param()
{
    int i = 0, len = 0;
    struct TR_param *param = NULL;
    TRF_node *node;
    char *attrval = NULL, *type = NULL;
    char param_type = '\0';
    
    for (i = 0; i < param_struct_num; i++) {
        //check parameter name
        node = param_struct[i].name->child;
        if (node == NULL) {
            param_struct[i].fault_code = INVALID_PARAM_NAME;
            have_fault_code = 1;
            continue;
        }
        LOG(m_handler, DEBUG, "Parameter: %s\n", node->value.opaque);
        len = strlen(node->value.opaque);
        if (node->value.opaque[len - 1] == '.') {
            param_struct[i].fault_code = INVALID_PARAM_NAME;
            have_fault_code = 1;
            continue;
        }
        //search param
        param = (struct TR_param *)param_search(node->value.opaque);
        if (param == NULL) {
            param_struct[i].fault_code = INVALID_PARAM_NAME;
            have_fault_code = 1;
            continue;
        }
        param_struct[i].param = param;
        
        /*node = param_struct[i].val->child;
        if (node == NULL) {
            param_struct[i].fault_code = INVALID_PARAM_VAL;
            have_fault_code = 1;
            continue;
        }*/
        //get value type
        attrval = (char *)mxmlElementGetAttr(param_struct[i].val, ":type");
        if (attrval == NULL) {
            param_struct[i].fault_code = INVALID_PARAM_TYPE;
            have_fault_code = 1;
            continue;
        }
        type = strchr(attrval, ':');
        type++;
        //check value type
        LOG(m_handler, DEBUG, "param type: %s\n", type);
        //transfer parameter type
        if (!strcmp(type, "string")) {
            param_type = 's';
        } else if (!strcmp(type, "unsignedInt")) {
            param_type = 'u';
        } else if (!strcmp(type, "int")) {
            param_type = 'i';
        } else if (!strcmp(type, "boolean")) {
            param_type = 'b';
        } else if (!strcmp(type, "dateTime")) {
            param_type = 'd';
        } else if (!strcmp(type, "base64")) {
            param_type = 'B';
        } else {
            LOG(m_handler, DEBUG, "Don't support this data type\n");
            param_struct[i].fault_code = INVALID_PARAM_TYPE;
            have_fault_code = 1;
            continue;
        }
        LOG(m_handler, DEBUG, "short type: %c\n", param_type);
        LOG(m_handler, DEBUG, "short type: %c\n", param->param_type);
        if (param_type != param->param_type) {
            param_struct[i].fault_code = INVALID_PARAM_TYPE;
            have_fault_code = 1;
            continue;
        }
        //check value non-writeable
        LOG(m_handler, DEBUG, "param writeable: %d\n", param->writable);
        if (param->writable == 0) {
            param_struct[i].fault_code = SET_NOT_WRITEABLE;
            have_fault_code = 1;
        }
        
    }
    return 0;
}

/*
 *******************************************************************
 * Function: set_param_val
 * Description: set parameter value to device
 * Parameter: TR_param_val_struct *ps
 * Return value:
 *     0 - success
 *******************************************************************
 */
static int set_param_val()
{
    int ret = -1, i = 0; 
    int intval;
    unsigned int uintval;
    char *param_value = NULL;
    int locate[4];

    //Only for param vlaue is NULL
    char strval[10];
    memset(strval, 0, sizeof(strval));
    
    for (i = 0; i < param_struct_num; i++) {
        //check parameter value
        if (param_struct[i].val->child == NULL) {
            if (param_struct[i].param->param_type != 's' && param_struct[i].param->param_type != 'B') {
                param_struct[i].fault_code = INVALID_PARAM_VAL;
                have_fault_code = 1;
                continue;
            } else {
                param_value = strval;
            }
        } else {
            param_value = param_struct[i].val->child->value.opaque;
        }
        
        switch (param_struct[i].param->param_type) {
            case 's':
                ret = call_dev_func(param_struct[i].param->dev_func, SET_OPT, param_value, param_struct[i].param->parent->locate);
                break;
            case 'i':
                //verify value
                if (verifyNumber(1, param_value) == -1) {
                    ret = SET_INVALID_PARAM_VAL;
                    break;
                }
                intval = atoi(param_value);
                ret = call_dev_func(param_struct[i].param->dev_func, SET_OPT, &intval, param_struct[i].param->parent->locate);
                break;
            case 'u':
                //verify value
                if (verifyNumber(0, param_value) == -1) {
                    ret = SET_INVALID_PARAM_VAL;
                    break;
                }
                uintval = atoi(param_value);
                ret = call_dev_func(param_struct[i].param->dev_func, SET_OPT, &uintval, param_struct[i].param->parent->locate);
                break;
            case 'b':
                if (!strcmp(param_value, "true") || !strcmp(param_value, "1")) {
                    intval = 1;
                } else if (!strcmp(param_value, "false") || !strcmp(param_value, "0")) {
                    intval = 0;
                } else {
                    ret = SET_INVALID_PARAM_VAL;
                    break;
                }
                ret = call_dev_func(param_struct[i].param->dev_func, SET_OPT, &intval, param_struct[i].param->parent->locate);
                break;
            case 'd':
                //verify value
                if (strtimetosec(param_value) == FAIL) {
                    ret = SET_INVALID_PARAM_VAL;
                    break;
                }
                ret = call_dev_func(param_struct[i].param->dev_func, SET_OPT, param_value, param_struct[i].param->parent->locate);
                break;
            case 'B':
                ret = call_dev_func(param_struct[i].param->dev_func, SET_OPT, param_value, param_struct[i].param->parent->locate);
                break;
            default:
                LOG(m_handler, ERROR, "Don't support this data type\n");
                ret = INTERNAL_ERROR;
                break;
        }
        //LOG(m_handler, DEBUG, "DEV_FUNC: %s\n", param_struct[i].param->dev_func);
        //ret = call_dev_func(param_struct[i].param->dev_func, SET_OPT, param_value, param_struct[i].param->parent->locate);
        switch (ret) {
            case SET_VAL_SUCCESS:
                break;
            case SET_NEED_REBOOT:
                ret = add_task_list("dev_reboot", NULL);
                if(ret != 0) {
                    LOG(m_handler, ERROR, "add task list error\n");
                    param_struct[i].fault_code = INTERNAL_ERROR;
                }
                reboot_flag = NEED_REBOOT;
                break;
            case SET_INVALID_PARAM_VAL:
                param_struct[i].fault_code = INVALID_PARAM_VAL;
                have_fault_code = 1;
                break;
            default:
                param_struct[i].fault_code = INTERNAL_ERROR;
                have_fault_code = 1;
                break;
        }
    }
    
    //set parameter value
    if (have_fault_code == 0) {
        LOG(m_handler, DEBUG, "Set parameter value\n");
        if (pk->child != NULL) {
            call_dev_func(pk_dev_func, SET_OPT, pk->child->value.opaque, locate);
        } else {
            call_dev_func(pk_dev_func, SET_OPT, "", locate);
        }
    }
    
    return 0;
}

/*
 ******************************************************************
 * Function: verifyNumber
 * Description: 
 * Parameter: 
 * Return value: 
 ******************************************************************
 */
static int verifyNumber(int type, const char *value)
{
    int i = 0, size = 0;
    int status = -1;

    if (value == NULL) {
        return status;
    }

    size = strlen(value);

    if (type == 1) { //signed int
        if (value[i] == '-') {
            i++;
        }
        if (value[i] == '\0') {
            return status;
        }
    }
    while (i < size) {
        if (!isdigit(value[i]))
            break;
        i++;
    }
    if(size > 0 && i == size)
        status = 0;
    return status;
}

/*
 ******************************************************************
 * Function:
 * Description:
 * Parameter:
 * Return Value:
 *    
 ******************************************************************
 */
static int gen_fault_response()
{
    TRF_node *cwmpf = NULL, *paraf = NULL;
    int i = 0;
    
    cwmpf = gen_method_fault(xmlroot, "SetParameterValues", INVALID_ARGUMENT);
    if (cwmpf == NULL) {
        return -1;
    }

    for (i = 0; i < param_struct_num; i++) {
        if (param_struct[i].fault_code != 0) {
            if (param_struct[i].name->child == NULL) {
                paraf = gen_para_fault(cwmpf, "SetParameterValues",  "", param_struct[i].fault_code);
            } else {
            paraf = gen_para_fault(cwmpf, "SetParameterValues",  param_struct[i].name->child->value.opaque, param_struct[i].fault_code);
            }
            if (paraf == NULL) {
                return -1;
            }
        }
    }
    
    LOG(m_handler, DEBUG, "Gen fault response success\n");
    
    return 0;
}

/*
 ***********************************************************
 * Function: free_param_struct
 * Description: free parameter value struct
 * Parameter: TRF_param_val_struct *ps
 * Return value:
 *    Void
 **********************************************************
 */
static void free_param_struct()
{
    //int i = 0;
    
    LOG(m_handler, DEBUG, "param struct num : %d\n", param_struct_num);
    free(param_struct);
    /*for (i = 0; i < param_struct_num; i++) {
        free(&param_struct[i]);
    }*/
    
    return;
}
/*
 **********************************************************************
 * Function: process_set_param_val
 * Description: process set parameter value method
 * Parameter: TRF_node *method - point to the method node
 * Return Value:
 *     success return 0, else return -1
 **********************************************************************
 */
int process_set_param_val(TRF_node *method)
{
    int ret;
    
    LOG(m_handler, DEBUG, "start process set parameter value\n");
    //init valiable
    param_struct_num = 0;
    reboot_flag = DONT_NEED_REBOOT;
    have_fault_code = 0;
    pk = NULL;
    param_struct = NULL;
   

    LOG(m_handler, DEBUG, "start get param name and value\n"); 
    ret = get_param_value(method);
    if (ret != 0) {
        free_param_struct();
        return MTH_RET_FAILED;
    }
    LOG(m_handler, DEBUG, "finished get param name and value\n");
    
    //check parameter
    valid_param();
    
    //check have soap fault
    if (have_fault_code == 0) {
        //set parameter value to device
        set_param_val();    
    }
    if (have_fault_code == 1) {
        if (gen_fault_response() != 0) {
            free_param_struct();
            return MTH_RET_FAILED;
        }
        LOG(m_handler, DEBUG, "Start free param struct\n");
        free_param_struct();
        LOG(m_handler, DEBUG, "Free param struct success\n");
    } else {
        //check need reboot(read task list)
        ret = gen_setparamval_resp(reboot_flag);
        if (ret != 0) {
            return MTH_RET_FAILED;
        }
    }
    
    return MTH_RET_SUCCESS;
}

