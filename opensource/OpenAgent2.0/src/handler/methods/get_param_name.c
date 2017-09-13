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
 ************************************************
 *
 * $Author: joinsonj $
 * $Date: 2007-06-08 06:53:02 $
 *
 ************************************************
 */

#include <string.h>
#include <stdlib.h>
#include "../../device/TRF_param.h"
#include "../../tools/logger.h"
#include "../../handler/soap/soap.h"
#include "methods.h"

// gloable variable, ACS request argument stored in it
typedef struct get_param_name {
    char param_path[PARAM_FULL_NAME_LEN+1];
    int next_level;
}TR_get_param_name;

// response arguments structure
typedef struct param_info_struct {
    char name[PARAM_FULL_NAME_LEN+1];
    int writable;
}TR_param_info_struct;


int size = 0;                   //the number of paramter
static TR_get_param_name arg;
static TR_param_info_struct param_info;

 /*
 ******************************************************************************
 * function:   gen_param_name_xml
 * dscription: generate xml paramlist struct
 * arguments:  TRF_node *ML
 * return values:
 *      SUCCESS        get arg success
 *      FAIL           get arg failure
 ******************************************************************************
 */

static int gen_param_name_xml(TRF_node *ML)
{
    TRF_node* Method = NULL, *param_name = NULL, *param_writable = NULL;

    Method = mxmlNewElement(ML, "ParameterInfoStruct");
    if (Method == NULL) {
        return FAIL;
    } 
    param_name = mxmlNewElement(Method, "Name");
    if (param_name == NULL) {
       return FAIL;
    }

    if (mxmlNewText(param_name, 0, param_info.name) == NULL) {
       return FAIL;
    }

    param_writable = mxmlNewElement(Method, "Writable");
    if (param_writable == NULL) {
       return FAIL;
    }
    
    if(mxmlNewInteger(param_writable, param_info.writable) == NULL) {
       return FAIL;
    }
    return SUCCESS;
}

 /*
 ******************************************************************************
 * function:   get_param_obj_path
 * dscription: get parmeter object path and writable
 * arguments:  struct TR_object* obj
 *             char *param_path 
 *             TRF_node* ML
 * return values:
 *      SUCCESS        get arg success
 *      FAIL           get arg failure
 ******************************************************************************
 */

int get_param_obj_path(struct TR_object* obj, char *param_path, TRF_node* ML)
{
    int res = 0;
    if (obj != NULL) {
        if (obj->name[0] == '0')
        obj = obj->next;
        if (obj == NULL) {
            return SUCCESS;
        } 
        char path[PARAM_FULL_NAME_LEN+1];
        char str[PARAM_FULL_NAME_LEN+1];
        struct TR_param* param_tmp = NULL;
        
        strcpy(str, param_path);
        strcpy(param_info.name, param_path);
        strcat(param_info.name, obj->name);
        strcat(param_info.name, ".");
        if (obj->writable > 0) 
            param_info.writable = 1;
        else 
            param_info.writable = 0;
        LOG(m_handler, DEBUG, "%s\n", param_info.name);

        res = gen_param_name_xml(ML);
        if (res != 0){
            return INTERNAL_ERROR;
        }
        size++;
        
        strcpy(path, param_info.name);
        param_tmp = obj->param;
        while (param_tmp != NULL) {
            size++;
            strcpy(param_info.name, path);
            strcat(param_info.name, param_tmp->name);
            if (param_tmp->writable > 0)
                param_info.writable = 1;
            else
                param_info.writable = 0;
  
            res = gen_param_name_xml(ML);
            if (res != 0){
                return INTERNAL_ERROR;
            }
            param_tmp = param_tmp->next;
        }            
        //strcpy(param_info.name, path);
        if (obj->next != NULL) {
            get_param_obj_path(obj->next, str, ML);
            LOG(m_handler, DEBUG, "obj->next is not NULL!\n");
        }

        if (obj->next_layer != NULL) {
            LOG(m_handler, DEBUG, "obj->next_layer: %s\n", path);    
            get_param_obj_path(obj->next_layer, path, ML);
        }
    }    
    return SUCCESS;
}

 /*
 ******************************************************************************
 * function:     get_param_names 
 * description:  get request arguments tansferred form ACS 
 * arguments:
 *      TRF_node*  method   - the method node in XML tree
 * return values:
 *      SUCCESS        get arg success
 *      FAIL           get arg failure
 ******************************************************************************
 */
static int get_param_names(TRF_node * method)
{       
    int flag1 = -1, flag2 = -1;
    
    LOG(m_handler, DEBUG, "GetParameterNames start to get arguments\n");
    method = method -> child;
    while (method ->value.element.name != NULL ) {
        if (!(strcmp (method ->value.element.name, "ParameterPath"))) {
            if (method -> child == NULL)
                arg.param_path[0] = '\0';
            else
                strcpy(arg.param_path, method -> child-> value.opaque);

            flag1 = 1;
        }
 
        if (!(strcmp (method ->value.element.name, "NextLevel"))) {
            if (method -> child == NULL) {
               LOG(m_handler, ERROR, "no NextLevel in ACS request");
               return FAIL;
            }
            if (!strcmp(method->child->value.opaque, "true") || !strcmp(method->child->value.opaque, "1")) {
                arg.next_level = 1;
            } else { 
                  if (!strcmp(method->child->value.opaque, "false") || !strcmp(method->child->value.opaque, "0")) {
                      arg.next_level = 0;
                  } else {
                      return FAIL;
                  }
            }
            //arg.next_level =strtol(method -> child-> value.opaque, NULL, 10); 
            flag2 = 1;
        }
        
        if (flag1 == 1 && flag2 == 1)
            break;

        method =  method -> next;
    }

    // judge the parameter is valid
    if (flag1 == -1 || flag2 == -1) {      
       LOG(m_handler, WARN, "there is no correct parameter for GetParameterNames\n");
       return FAIL;
    }

    if (arg.next_level != 0 && arg.next_level != 1) {
        LOG(m_handler, ERROR, "ACS argument NextLevel is invalid\n");
        return FAIL;
    }

    LOG(m_handler, DEBUG, "param_path=%s**nextlevel=%d, GetParameterNames end\n", arg.param_path, arg.next_level);

    return SUCCESS;
}

 /*
 ******************************************************************************
 * function:     gen_param_list 
 * description:  according to ACS' request, generate the parameter name list
 * arguments:
 *               TRF_node *ML
 * return value:
 *           SUCCESS      generate param list success
 *           FAIL         failed to generate param list
 ******************************************************************************
 */

static int gen_param_list(TRF_node *ML)
{
    int res = 0;
    struct TR_object *_node = NULL, *obj_tmp = NULL;
    struct TR_param *_param = NULL;
    char path[257];
    
    strcpy(path, arg.param_path);

    // parameter path is empty string
    if (strlen(arg.param_path) == 0) {   
        _node = &root;
       
        LOG(m_handler, INFO, "parameter path at top level\n");
        if (arg.next_level == 1) {

            strcpy(param_info.name, DEVICE);
            strcat(param_info.name, "."); 
            if (_node->writable > 0)
                param_info.writable = 1;
            else
                param_info.writable = 0;
            LOG(m_handler, DEBUG, "writable:%d\n", _node->writable);

            res = gen_param_name_xml(ML);        
            if (res != 0){
                return INTERNAL_ERROR;
            }
            size = 1;
            return SUCCESS;
        } else {
              size = 0;
              res = get_param_obj_path(_node, arg.param_path, ML);
              if (res != 0)
                  return res;
        }
    } else if (arg.param_path[strlen(arg.param_path) - 1] != '.') { 
          LOG(m_handler, INFO, "ACS request parameter path is full path\n");    
          _param =(struct TR_param *)param_search(path); 
          if (_param == NULL) {
              LOG(m_handler, ERROR, "full path search parameter path failure\n");
              return INVALID_PARAM_NAME;
          }
          
          if (arg.next_level == 0) {
              strcpy(param_info.name, arg.param_path);
              if (_param->writable > 0)
                  param_info.writable = 1;
              else
                  param_info.writable = 0;

              res = gen_param_name_xml(ML);
              if (res != 0){
                  return INTERNAL_ERROR;
              }
              size = 1;
              return SUCCESS;
          } else 
                return INVALID_ARGUMENT;
    } else {
          LOG(m_handler, DEBUG, "parameter path is partial path\n");
          _node =(struct TR_object *)param_search(path);
         if (_node == NULL) {
             LOG(m_handler, ERROR, "partial path search parameter path failure\n");
             return INVALID_PARAM_NAME;
         }
         
         if (arg.next_level == 1) {
             LOG(m_handler, DEBUG, "arg next_level is 1 and object name: %s!\n", _node->name);
             size = 0;
             _param = _node->param;
             while (_param != NULL) {
                 size++;
                 strcpy(param_info.name, arg.param_path);
                 strcat(param_info.name, _param->name);
                 if (_param->writable > 0)
                     param_info.writable = 1;
                 else
                     param_info.writable = 0;

                 res = gen_param_name_xml(ML);
                 if (res != 0){
                     return INTERNAL_ERROR;
                 }
                 _param = _param->next;
             }

             if (_node->next_layer != NULL) {
                 obj_tmp = _node->next_layer;
                 if (obj_tmp->name[0] == '0') 
                     obj_tmp = obj_tmp->next;

                 while ((obj_tmp != NULL)) {
                      size++;
                      strcpy(param_info.name, arg.param_path);
                      strcat(param_info.name, obj_tmp->name);
                      strcat(param_info.name, ".");
                      if (obj_tmp->writable > 0)
                          param_info.writable = 1;
                      else
                          param_info.writable = 0;
  
                      res = gen_param_name_xml(ML);
                      if (res != 0){
                          return INTERNAL_ERROR;
                      }
                      obj_tmp = obj_tmp->next;
                 }
             }    
         } else {
                size = 0;
                strcpy(param_info.name, arg.param_path);
                if (_node->writable > 0)
                    param_info.writable = 1;
                else
                    param_info.writable = 0;

                res = gen_param_name_xml(ML);
                if (res != 0){
                    return INTERNAL_ERROR;
                }
                size++;

                _param = _node->param;
                while (_param != NULL) {
                    size++;
                    strcpy(param_info.name, arg.param_path);
                    strcat(param_info.name, _param->name);
                    if (_param->writable > 0)
                        param_info.writable = 1;
                    else
                        param_info.writable = 0;

                    res = gen_param_name_xml(ML);
                    if (res != 0){
                        return INTERNAL_ERROR;
                    }
                    _param = _param->next;
                }

                res = get_param_obj_path(_node->next_layer, arg.param_path, ML);
                if (res != 0)
                    return res;
         }
    }
    return SUCCESS;
}

 /*
 ******************************************************************************
 * function:     process_getparamname 
 * description:  get argumets form ACS request, and generate response 
 * arguemt: 
 *       TRF_node* method   GetParameterNames this methods' node in xml tree 
 * return value:
 *       SUCCESS         generate GetParameterNames response success
 *       FAIL            generate GetParameterNames response failure
 ******************************************************************************
 */
int process_getparamname(TRF_node * method)
{
    int res;
    char arraytype[257], name[] = "GetParameterNamesResponse";
    TRF_node *ML, *Method, *soap_env, *soap_body, *soap_header;
 
    res = get_param_names(method);
    if (res == -1) {
        LOG(m_handler, DEBUG, "Get param names fault, then generate soap fault\n");
        if (gen_method_fault(xmlroot, "GetParameterNames", INVALID_PARAM_NAME) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
  
    //method = gen_soap_frame(name);
    soap_env = gen_env(xmlroot);
    if (!soap_env) {
        return MTH_RET_FAILED;
    }

    soap_header = gen_soap_header(soap_env);
    if (!soap_header) {
        return MTH_RET_FAILED;
    }

    soap_body = gen_soap_body(soap_env);
    if (!soap_body) {
        return MTH_RET_FAILED;
    }

    Method = gen_method_name(name, soap_body);
    if (!Method) {
        return MTH_RET_FAILED;
    }
    

    //generate method's xml
    LOG(m_handler, DEBUG, "Start to generate GetParameterNames Response xml\n");

    ML = mxmlNewElement(Method, "ParameterList");
    if (ML == NULL) {
        return FAIL;
    }
    
    //sprintf(arraytype, "cwmp:ParameterInfoStruct[%d]", size);
    mxmlElementSetAttr(ML, "xsi:type", "SOAP-ENC:Array");
   
    res = gen_param_list(ML);
    if (res != 0) {
        mxmlDelete(soap_env);
        LOG(m_handler, DEBUG, "generate parameter list failed, then generate soap fault\n");
        if (gen_method_fault(xmlroot, "GetParameterNames", res) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    } 
      
    sprintf(arraytype, "cwmp:ParameterInfoStruct[%d]", size);
    mxmlElementSetAttr(ML, "SOAP-ENC:arrayType", arraytype);
    LOG(m_handler, DEBUG, "END generate GetParameterNames Response xml\n");
    return SUCCESS;
}


