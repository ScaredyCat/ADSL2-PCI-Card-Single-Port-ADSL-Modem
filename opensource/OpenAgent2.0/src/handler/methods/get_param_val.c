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

#include "../soap/soap.h"
#include "../../device/TRF_param.h"
#include "methods.h"


/*
 * Global variable....
 */

TRF_node *soap_env;             // The node of Envelope
TRF_node *param_list;           // The node of parameter list
int param_num;                  // Parameter value struct number

#define PARTIAL_NAME 1
#define FULL_NAME    2

static int get_partialpath_value(TRF_node *pl, struct TR_object *object, char *param_name);
static int get_param_val(TRF_node *val, struct TR_param *param);
static int gen_param_struct(TRF_node *pl, struct TR_param *param, char *param_name);
static int gen_param_node(TRF_node *pl, char *param_name);
static int gen_getparamval_resp();

/*
 ***********************************************************************
 * Function:get_param_val
 * Description: 
 * Parameter: 
 * Return Value:
 *     Success return 0, else return -1;
 ***********************************************************************
 */
static int get_param_val(TRF_node *val, struct TR_param *param)
{
    int ret = MTH_RET_SUCCESS;
    char chararg[GET_PARAM_VAL_LEN];
    int intarg = 0;
    //time_t timearg = 0;
    unsigned int uintarg = 0; 
    
    memset(chararg, 0, sizeof(chararg));
    switch (param->param_type) {
        case 's':
            ret = call_dev_func(param->dev_func, GET_OPT, chararg, param->parent->locate);
            if (ret != GET_VAL_SUCCESS) {
                return INTERNAL_ERROR;
            }
            mxmlElementSetAttr(val, "xsi:type", "xsd:string");
            if (mxmlNewText(val, 0, chararg) == NULL) {
                return INTERNAL_ERROR;
            }
            break;
        case 'i':
            ret = call_dev_func(param->dev_func, GET_OPT, &intarg, param->parent->locate);
            if (ret != GET_VAL_SUCCESS) {
                return INTERNAL_ERROR;
            }
            mxmlElementSetAttr(val, "xsi:type", "xsd:int");
            if (mxmlNewInteger(val, intarg) == NULL){
                return INTERNAL_ERROR;
            }
            break;
        case 'u':
            ret = call_dev_func(param->dev_func, GET_OPT, &uintarg, param->parent->locate);
            if (ret != GET_VAL_SUCCESS) {
                return INTERNAL_ERROR;
            }
            mxmlElementSetAttr(val, "xsi:type", "xsd:unsignedInt");
            if (mxmlNewInteger(val, uintarg) == NULL) {
                return INTERNAL_ERROR;
            }
            break;
        case 'b':
            ret = call_dev_func(param->dev_func, GET_OPT, &intarg, param->parent->locate);
            if (ret != GET_VAL_SUCCESS) {
                return INTERNAL_ERROR;
            }
            mxmlElementSetAttr(val, "xsi:type", "xsd:boolean");
            if (mxmlNewInteger(val, intarg) == NULL) {
                return INTERNAL_ERROR;
            }
            break;
        case 'd':
            ret = call_dev_func(param->dev_func, GET_OPT, &chararg, param->parent->locate);
            if (ret != GET_VAL_SUCCESS) {
                return INTERNAL_ERROR;
            }
            //format_time(timearg, chararg);
            mxmlElementSetAttr(val, "xsi:type", "xsd:dateTime");
            if (mxmlNewText(val, 0, chararg) == NULL) {
                return INTERNAL_ERROR;
            }
            break;
        case 'B':
            ret = call_dev_func(param->dev_func, GET_OPT, &chararg, param->parent->locate);
            if (ret != GET_VAL_SUCCESS) {
                return INTERNAL_ERROR;
            }
            mxmlElementSetAttr(val, "xsi:type", "xsd:base64");
            if (mxmlNewText(val, 0, chararg) == NULL){
                return INTERNAL_ERROR;
            }
            break;
        default:
            LOG(m_handler, ERROR, "Don't support this data type\n");
            return INTERNAL_ERROR;
            
    }
    
    return ret;
}

/*
 ********************************************************************
 * Function: gen_param_struct
 * Description: generate parameter struct node
 * Parameter: 
 * Return Values: 
 *     MTH_RET_SUCCESS - success
 *     INTERNAL_ERROR  - failed
 ********************************************************************
 */
static int gen_param_struct(TRF_node *pl, struct TR_param *param, char *param_name)
{
    TRF_node *name = NULL, *value = NULL, *pvs = NULL;
    int ret = INTERNAL_ERROR;
    
    pvs = mxmlNewElement(pl, "ParameterValueStruct");
    if (pvs != NULL) {
        name = mxmlNewElement(pvs, "Name");
        if (name != NULL) {
            if (mxmlNewText(name, 0, param_name) != NULL) {
                value = mxmlNewElement(pvs, "Value");
                if (value != NULL) {
                    ret = get_param_val(value, param);
                    if (ret != GET_VAL_SUCCESS) {
                        LOG(m_handler, ERROR, "get parameter %s value failed\n", param_name);
                    } else {
                        ret = MTH_RET_SUCCESS;
                        LOG(m_handler, DEBUG, "get parameter %s value success\n", param_name);
                    }
                }    
            }    
        }
    }
    
    return ret;
}

/*
 *******************************************************************
 * Function: gen_param_node
 * Description:
 * Parameter: 
 * Return Value:
 *     MTH_RET_SUCCESS - success
 *     INTERNAL_ERROR  - failed
 *******************************************************************
 */
static int gen_param_node(TRF_node *pl, char *param_name)
{
    struct TR_object *object = NULL;
    struct TR_param *param = NULL;
    int ret, param_type;
    char param_name_bk2[PARAM_FULL_NAME_LEN + 1], object_name[PARAM_FULL_NAME_LEN + 1];

    //check parameter type
    if (param_name == NULL) {
        param_type = PARTIAL_NAME;
    } else if (param_name[strlen(param_name) - 1] == '.') {
        param_type = PARTIAL_NAME;
    } else {
        param_type = FULL_NAME;
    }
    
    //handle parameter
    switch (param_type) {
        case PARTIAL_NAME:
            memset(param_name_bk2, 0, sizeof(param_name_bk2));
            memset(object_name, 0, sizeof(object_name));
            
            LOG(m_handler, DEBUG, "Partial name : %s\n", param_name);
            object = (struct TR_object *)param_search(param_name);
            LOG(m_handler, DEBUG, "Search success\n");
            if (object == NULL) {
                LOG(m_handler, ERROR, "Find Object failed\n");
                ret = INVALID_PARAM_NAME;
                break;
            }
            if (param_name == NULL) {
                strcpy(object_name, object->name);
            } else {
                strcpy(object_name, param_name);
            }
            
            param = object->param;

            while (param != NULL) {
                //check param is read able
                if (param->writable == TR_WRONLY) {
                    LOG(m_handler, DEBUG, "Unreadable\n");
                    return INTERNAL_ERROR;
                }
                //get param full name
                strcpy(param_name_bk2, object_name);
                strcat(param_name_bk2, param->name);

                if (gen_param_struct(pl, param, param_name_bk2) != MTH_RET_SUCCESS) {
                    return INTERNAL_ERROR;
                }
                LOG(m_handler, DEBUG, "Gen parameter struct success\n");
                param_num++;
                param = param->next;
            }
            if (get_partialpath_value(pl, object->next_layer, object_name) != MTH_RET_SUCCESS) {
                ret = INTERNAL_ERROR;
                return ret;
            }
            
            ret =  MTH_RET_SUCCESS;
            break;
        case FULL_NAME:
            LOG(m_handler, DEBUG, "Full parameter name : %s\n", param_name);
            param = (struct TR_param *)param_search(param_name);
            if (param == NULL){
                LOG(m_handler, ERROR, "Find parameter: %s failed\n", param_name);
                ret = INVALID_PARAM_NAME;
                break;
            }
            //check parameter
            if (param->writable == TR_WRONLY) {
                LOG(m_handler, DEBUG, "Unreadable\n");
                return INTERNAL_ERROR;
            }
            
            if (gen_param_struct(pl, param, param_name) != MTH_RET_SUCCESS) {
                ret = INTERNAL_ERROR;
                break;
            }

            param_num++;

            ret = MTH_RET_SUCCESS;
            break;
        default:
            ret = INTERNAL_ERROR;
    }
    
    return ret;
}

/*
 **********************************************************************
 * Function: 
 * Description: 
 * Parameter: 
 * Return value: 
 * 
 **********************************************************************
 */
static int get_partialpath_value(TRF_node *pl, struct TR_object *object, char *param_name)
{
    struct TR_param *param = NULL;
    int ret = MTH_RET_SUCCESS;
    char param_name_bk1[PARAM_FULL_NAME_LEN + 1], param_name_bk2[PARAM_FULL_NAME_LEN + 1];
    
    //init param name
    memset(param_name_bk1, 0, sizeof(param_name_bk1));
    memset(param_name_bk2, 0, sizeof(param_name_bk2));
    
    if (object) {
        if (object->name[0] == '0') {
            object = object->next;
            if (object == NULL) {
                return ret;
            }
        }
        
        strcpy(param_name_bk1, param_name);
        strcat(param_name_bk1, object->name);
        strcat(param_name_bk1, ".");
        
        param = object->param;
        while (param != NULL) {
            //check param is read able
            if (param->writable == TR_WRONLY) {
                LOG(m_handler, DEBUG, "Unreadable\n");
                return INTERNAL_ERROR;
            }
            //get param name
            strcpy(param_name_bk2, param_name_bk1);
            strcat(param_name_bk2, param->name);
            if (gen_param_struct(pl, param, param_name_bk2) != MTH_RET_SUCCESS) {
                ret = INTERNAL_ERROR;
                return ret;
            }
            LOG(m_handler, DEBUG, "Gen parameter struct success\n");
            param_num++;
            param = param->next;
        }
        ret = get_partialpath_value(pl, object->next, param_name);
        if (ret != 0) {
            return ret;
        }
        ret = get_partialpath_value(pl, object->next_layer, param_name_bk1);
        if (ret != 0) {
            return ret;
        }
    }
    
    return ret;
}

/*
 ************************************************************************
 * Function: gen_getparamval_resp
 * Description: 
 * Parameter: 
 * Return Value:
 *     MTH_RET_SUCCESS - success 
 *     MTH_RET_FAILED  - failed
 ************************************************************************
 */
static int gen_getparamval_resp()
{
    char name[] = "GetParameterValuesResponse";
    TRF_node *soap_header, *soap_body, *method;

    soap_env = gen_env(xmlroot);
    if (!soap_env){
        return MTH_RET_FAILED;
    }
    
    soap_header = gen_soap_header(soap_env);
    if (!soap_header){
        return MTH_RET_FAILED;
    }
    
    soap_body = gen_soap_body(soap_env);
    if (!soap_body){
        return MTH_RET_FAILED;
    }
    
    method = gen_method_name(name, soap_body);
    if (!method){
        return MTH_RET_FAILED;
    }

    param_list = mxmlNewElement(method, "ParameterList");
    if (param_list == NULL){
        LOG(m_handler, ERROR, "generate parameter list node failed\n");
        return MTH_RET_FAILED;
    }
    
    return MTH_RET_SUCCESS;
}

/*
 **********************************************************************
 * Function:process_get_param_val
 * Description:
 * Parameter:
 * Return Value:
 *     MTH_RET_SUCCESS - success
 *     MTH_RET_FAILED  - failed
 **********************************************************************
 */
int process_get_param_val(TRF_node *method)
{
    TRF_node *node;
    int ret; 
    char arraytype[64];
    
    //init parameter value struct number
    param_num = 0;
    
    //generate get parameter value response
    ret = gen_getparamval_resp();
    if (ret != MTH_RET_SUCCESS) {
        return ret;
    }
    
    method = method->child;
    while (method != NULL) {
        // Find parameter name node
        if (!strcmp(method->value.element.name, "ParameterNames")) {
            node = method->child;
            // This while process parameter values
            while (node != NULL) {
                LOG(m_handler, DEBUG, "node->value.element.anme : %s\n", node->value.element.name);
                if (!strcmp(node->value.element.name, "string")) {
                    if (node->child == NULL) {
                        ret = gen_param_node(param_list, NULL);
                    } else {
                        ret = gen_param_node(param_list, node->child->value.opaque);
                    }
                    if (ret != MTH_RET_SUCCESS) {
                        LOG(m_handler, ERROR, "generate parameter value node failed\n");
                        if (ret == INVALID_PARAM_NAME || ret == INTERNAL_ERROR) {
                            //delete the envelope node
                            mxmlDelete(soap_env);
                            //generate method fault response
                            if (gen_method_fault(xmlroot, "GetParameterValues", ret) == NULL){
                                return MTH_RET_FAILED;
                            }
                            return MTH_RET_SUCCESS;
                        }
                        return MTH_RET_FAILED;
                    }
                    LOG(m_handler, DEBUG, "generate parameter value node success\n");
                }
                //get the next node
                node = node->next;
            }
            break;
        }
        
        method = method->child;
    }
    
    //add attribute to param_list node
    sprintf(arraytype, "cwmp:ParameterValueStruct[%d]", param_num);
    mxmlElementSetAttr(param_list, "xsi:type", "SOAP-ENC:Array");
    mxmlElementSetAttr(param_list, "SOAP-ENC:arrayType", arraytype);
    
    return MTH_RET_SUCCESS;
}

