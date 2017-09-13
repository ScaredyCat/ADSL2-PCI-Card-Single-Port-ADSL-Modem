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
 * $Date: 2007-06-08 02:17:29 $
 ******************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include "TRF_param.h"
#include "../handler/xmltool/mxml.h"
#include "../tools/logger.h"
#include "../handler/methods/param_attri.h"
#include "../res/global_res.h"
#include "../handler/methods/param_attri.h"

//declare global variable
mxml_node_t* node;
struct TR_object root;

 /*
 **************************************************************************
 * function:     Loadfile_to_memory
 * description:  load param.xml config file into memeory 
 *               and generate an XML node tree
 * argument:     mxml_node_t*  top    
 *               FILE*         fp     
 *               mxml_type_t (*cb)(mxml_node_t *node)  //callback function pointer
 * return:       mxml_node_t node pointer    success
 *               NULL                        fail
 ***************************************************************************
 */

static mxml_node_t* Loadfile_to_memory(mxml_node_t* top, FILE* fp, mxml_type_t (*cb)(mxml_node_t *node))
{
    int fd = 0, res = 0, readCnt = 0;
    struct stat theStat;
    char* tmp1 = NULL, *ptr = NULL;
    char* tmp2 = NULL, *chars = NULL, *str = NULL;
                                  
    fd = fileno(fp);
    res = fstat(fd, &theStat);                                                                                           
    if (res != 0 ) {
        LOG(m_device, ERROR, "get parameter xml file size fail!\n");
        return NULL;
    }

    chars = (char*)malloc(theStat.st_size);
    if (chars == NULL) {
        LOG(m_device, ERROR, "malloc chars space fail!\n");
        return NULL;
    }
    memset(chars, 0, theStat.st_size);

    str=(char*)malloc(theStat.st_size);
    if (str == NULL){
        LOG(m_device, ERROR, "malloc str space fail!\n");
        return NULL;
    }
    memset(str, 0, theStat.st_size);


    readCnt = fread(chars, 1,theStat.st_size, fp);
    if (readCnt < theStat.st_size) {
        LOG(m_device, DEBUG, "lose some.%d %d", readCnt, theStat.st_size);
    }
    tmp1 = strstr(chars, "<!--Param.xml");
    tmp2 = strstr(tmp1, "</TR069>");
    tmp2 = tmp2 + 7;

    ptr = str;
    while (tmp1 != tmp2) {
        if (*tmp1 == '>') {
            *str = *tmp1;
            str++;
            tmp1++;
            while ((*tmp1) != '<') {
                tmp1++;
            }
        } else {
              *str = *tmp1;
              str++;
              tmp1++;
        }
    }
    *(str++) = '>';
    *(str+2) = 0;
    LOG(m_device, DEBUG, "string of xml:\n%s\n",ptr);
    
    mxml_node_t * node = mxmlLoadString(0, ptr, cb);
    if (node == NULL) {
        LOG(m_device, ERROR, "load string to xml tree fail!\n");
        return NULL;
    }
    free(ptr);
    free(chars);
    return node;
}

 /*
 **************************************************************************
 * function:     populateTRFObject
 * description:  populate object's attributes
 * argument:     struct TR_object* tr_obj
 *               mxml_node_t* obj
 * return:       SUCCESS      success
 *               FAIL         fail
 ***************************************************************************
 */

static int populateTRFObject(mxml_node_t* obj, struct TR_object* tr_obj)
{
    const char* tmp = NULL;
    char ch;
    int num = 0, i = 0;
    tmp = mxmlElementGetAttr(obj, TRF_ATTR_OBJECT_MAXINSTANCE);
    if (tmp)
        tr_obj->max_instance = atoi(tmp);
    else
        tr_obj->max_instance = 0;
                                                                                
                                                                                
    tmp = mxmlElementGetAttr(obj, TRF_ATTR_OBJECT_WRITABLE);
    if (tmp) {
        tr_obj->writable = atoi(tmp);
    } else {
          LOG(m_device, ERROR, "parameter config file doesn't have writable content!\n")
          return FAIL;
    }
                                                                        
    tmp = mxmlElementGetAttr(obj, TRF_ATTR_OBJECT_NAME);
    if (tmp) {
        i = strlen(tmp);
        if (i >= OBJECT_NAME) {
            LOG(m_device, ERROR, "Object name length must <= %d object name %s!\n", OBJECT_NAME-1, tr_obj->name);
            return FAIL;
        }
        strncpy(tr_obj->name, tmp, OBJECT_NAME);
        ch = tr_obj->name[0];
        if (tr_obj->parent == NULL) {
            LOG(m_device, DEBUG, "object name is :%s\n", tr_obj->name);
            for (i = 0; i < MULTI_NUM; i++) {
                tr_obj->locate[i] = 0;
            }
        } else if ((ch >= 'A')&& (ch <= 'Z')) {
            for (i = 0; i < MULTI_NUM; i++) {
                tr_obj->locate[i] = tr_obj->parent->locate[i];
            }
        } else {
            for (i = 0; i < MULTI_NUM; i++) {
                tr_obj->locate[i] = tr_obj->parent->locate[i];
            }
            num = atoi(tr_obj->name);
            for (i = 0; i < MULTI_NUM; i++) {
                if (tr_obj->locate[i] == 0) {
                    tr_obj->locate[i] = num;
                    break;
                }
            }
        }

    } else {
        LOG(m_device, ERROR, "parameter configfile lose object name!\n");
        return FAIL;
    }
    
    tmp = mxmlElementGetAttr(obj, TRF_ATTR_OBJECT_OBJ_DEV_METHOD);
    if (tmp) {
        i = strlen(tmp);
        if (i >= OBJ_DEV_FUNC_LEN) {
            LOG(m_device, ERROR, "Add Object dev func name length must <= %d object name %s!\n", OBJ_DEV_FUNC_LEN -1, tr_obj->name);
            return FAIL;
        }
        strncpy(tr_obj->obj_dev_func, tmp, OBJ_DEV_FUNC_LEN);
    }

    tmp = mxmlElementGetAttr(obj, TRF_ATTR_OBJECT_OBJ_INSTANCE_METHOD);
    if (tmp) {
        i = strlen(tmp);
        if (i >= OBJ_INS_FUNC_LEN) {
            LOG(m_device, ERROR, "Get Object dev func name length must <= %d object name %s!\n", OBJ_INS_FUNC_LEN-1, tr_obj->name);
            return FAIL;
        }
        strncpy(tr_obj->obj_instance_func, tmp, OBJ_INS_FUNC_LEN);
    }

    return SUCCESS;
}

 /*
 **************************************************************************
 * function:    populateTRFparam
 * description: evaluate parameter attributes
 * argument:    mxml_node_t* obj
 *              struct TR_param* obj_param
 * return:      SUCCESS      success
 *              FAIL         fail
 **************************************************************************
 */

static int populateTRFparam(mxml_node_t* obj, struct TR_param* obj_param)
{
    int i = 0;
    const char* tmp = NULL;

 //   obj_param->notification = 0;
    strcpy(obj_param->accesslist, "Subscriber");

    tmp = mxmlElementGetAttr(obj, TRF_ATTR_PARAMETER_NAME);
    if (tmp) { 
        i = strlen(tmp);
        if (i >= PARAM_NAME) {
            LOG(m_device, ERROR, "Parameter name length must <= %d parameter name %s!\n", PARAM_NAME-1, obj_param->name);
            return FAIL;
        }
        strncpy(obj_param->name, tmp, PARAM_NAME);
    } else {
        LOG(m_device, ERROR, "paramter configfile lose parameter name!\n");
        return FAIL;
    }
                                                                                
    tmp = mxmlElementGetAttr(obj, TRF_ATTR_PARAMETER_TYPE);
    if (tmp) { 
        obj_param->param_type = tmp[0];      
    } else {
        LOG(m_device, ERROR, "paramter configfile lose parameter type!\n");
        return FAIL;
    }

    tmp = mxmlElementGetAttr(obj, TRF_ATTR_PARAMETER_WRITABLE);
    if (tmp) {
        obj_param->writable = atoi(tmp);
    } else {
        LOG(m_device, ERROR, "paramter configfile lose parameter writable!\n");
        return FAIL;
    }

    tmp = mxmlElementGetAttr(obj, TRF_ATTR_PARAMETER_DEV_METHOD);
    if (tmp) {
        i = strlen(tmp);
        if (i >= PARAM_DEV_FUNC_LEN) {
            LOG(m_device, ERROR, "Param dev_funv name length must <= %d parameter name %s!\n", PARAM_DEV_FUNC_LEN-1, obj_param->name);
            return FAIL;
        }
        strncpy(obj_param->dev_func, tmp, PARAM_DEV_FUNC_LEN);
    } else {
        LOG(m_device, ERROR, "paramter configfile lose parameter dev_func!\n");
        return FAIL;
    }
    tmp = mxmlElementGetAttr(obj, TRF_ATTR_PARAMETER_NOTIFICATION);
    if(tmp) {
        obj_param->notification = atoi(tmp);
    } else {
        obj_param->notification = 0;
    }
    tmp = mxmlElementGetAttr(obj, TRF_ATTR_PARAMETER_NOTI_RW);
    if(tmp) {
        obj_param->noti_rw = atoi(tmp);
    } else {
        obj_param->noti_rw = TR_NOTI_RW;
    }

    return SUCCESS;
}

 /*
 ********************************************************************************
 * function:     create_param_tree
 * description:  create a object tree, so you can search parameter or object in it              
 * argument:     struct TR_object* obj
 *               mxml_node_t* xml_node
 * return        SUCCESS      success
 *               FAIL         fail
 ********************************************************************************
 */

static int create_param_tree (struct TR_object* obj, mxml_node_t* xml_node)
{
    int res = 0, ret = 0;
    if (xml_node != NULL) {
        res = populateTRFObject(xml_node, obj);   //call poulateTRFObject function to inital TR_object and TR_param
        if (res == -1) {
            LOG(m_device, ERROR, "Populate TRF object fault!\n");
            ret = FAIL;
        }
 
        mxml_node_t* xml_node_tmp = xml_node->child;      /*everytime when run into this creat_param_tree function
                                                          we must declare this variable*/
        if (xml_node_tmp !=NULL) {
            obj->param = NULL;
            struct TR_param* obj_param = NULL;
            while (!strcmp(xml_node_tmp->value.element.name,"param")) {
                obj_param = (struct TR_param* )malloc(1 * sizeof(struct TR_param));          
                if (obj_param == NULL) {
                    LOG(m_device, ERROR, "malloc parameter memory failed!\n");
	            ret = FAIL;
                }
                memset(obj_param, 0, sizeof(struct TR_param));
                //add parameter parent
                obj_param->parent = obj;

                res = populateTRFparam(xml_node_tmp, obj_param);
                if (res == -1) {
                    LOG(m_device, ERROR, "Populate TRF parameter fault!\n");
                    ret = FAIL;
                }

                if (obj->param == NULL) {
                    obj->param = obj_param;
                    obj_param->next=NULL;
                } else {
                       obj_param->next = obj->param->next;
                       obj->param->next = obj_param;
                }
                //obj->param_size++;
                xml_node_tmp = xml_node_tmp->next;
                if (xml_node_tmp == NULL)
                    break;
            }
            if (xml_node_tmp != NULL) {
                if (!strcmp(xml_node_tmp->value.element.name, "obj")){
                     struct TR_object* obj_child = (struct TR_object*)malloc(1 * sizeof(struct TR_object));
 		     if (obj_child == NULL) {
                         LOG(m_device, ERROR, "malloc object child memory failed\n");
                         ret = FAIL;
                     }
                     memset(obj_child, 0, sizeof(struct TR_object));

                     obj->next_layer = obj_child;
                     obj_child->parent = obj;              //add object parent  
                     res = create_param_tree(obj_child, xml_node_tmp);
                     if (res != 0) {
                         LOG(m_device, ERROR, "create parameter tree fail!\n");
                         ret = FAIL;
                     }

                }
            }
        }
        if (xml_node->next != NULL) {
            struct TR_object* obj_next = (struct TR_object*)malloc(1 * sizeof(struct TR_object));
            if (obj_next == NULL) {
                LOG(m_device, ERROR, "malloc object next layer memory failed\n");
                ret = FAIL;
            }
            memset(obj_next, 0, sizeof(struct TR_object));

            obj->next = obj_next;
            obj_next->parent = obj->parent;     //add obj parent node
            res = create_param_tree(obj_next, xml_node->next);
            if (res != 0) {
                LOG(m_device, ERROR, "create parameter tree fail!\n");
                ret = FAIL;
            }
        }
    }
    return ret;                                                           
}

 /*
 ********************************************************************************
 * function:     dev_object_func
 * description:  call device get instance number
 * argument:     int* instance_num
 *               char dev_instance_func[]
 *               int *locate
 * return        SUCCESS      success
 *               FAIL         fail
 ********************************************************************************
 */

static int dev_object_func(int* instance_num, char dev_instance_func[], int *locate)
{
    int res;
    void *handle;
    int (*func)(int*, int*);
    char *error;
    
    handle = dlopen (dev_lib_path, RTLD_LAZY);
    if (!handle) {
        LOG(m_device, ERROR, "%s\n", dlerror());
        return FAIL;
    }

    func = dlsym(handle, dev_instance_func);
    if ((error = dlerror()) != NULL) {
        LOG(m_device, ERROR,  "%s\n", error);
        dlclose(handle);
        return FAIL;
    }
    //call vendor function
    res = (*func)(instance_num, locate);
    if (res != 0) {
        LOG(m_device, ERROR, "call vendor device func fail!\n");
        dlclose(handle);
        return FAIL;
    }
    dlclose(handle);
    return SUCCESS;
}


 /*
 ****************************************************************
 * function:    init_multi_obj
 * description: init multi object instance name
 * argument:    struct TR_object* obj
 * return:      SUCCESS     success
 *              FAIL        fail
 ****************************************************************
 */

static int init_multi_obj(struct TR_object* trObj)
{ 
    int ret = 0;
    int res = 0, tmp = 0, max_instance = 0, i = 0;
    char ch, tr = '0';
    int* instance_num = NULL;

    if (trObj != NULL) {
        tmp = trObj->writable;
        ch = trObj->name[0];
        if (trObj->parent != NULL) {
            tr = trObj->parent->name[0];
        }

        //if ((tmp == 2) && ((ch < '0') || (ch > '9'))) {          //if the tree form change ,the condition must change
        if ((tmp == 2) && (tr != '0') && !(isdigit(ch))) {

            max_instance = trObj->max_instance;
            LOG(m_device, DEBUG, "object parent name %s,object name %s\n", trObj->parent->name, trObj->name);

            instance_num = (int*)malloc(max_instance * sizeof(int));
            if (instance_num == NULL) {
                LOG(m_device, ERROR, "malloc memory fail\n");
                ret = FAIL;
            }
            memset(instance_num, 0, max_instance * sizeof(int));

            LOG(m_device, DEBUG, "obj_instance func:%s\n", trObj->obj_instance_func);
   
            //debug for locate
            for (i = 0; i < MULTI_NUM; i++) {
                LOG(m_device, DEBUG, "locate[%d] = %d\n",i, trObj->locate[i]);
            }

            //call device function
            res = dev_object_func(instance_num, trObj->obj_instance_func, trObj->locate);
            if (res != 0) {
                LOG(m_device, ERROR, "get instance_num fail!\n");
                ret = FAIL;
            }

            for (i = 0; i < max_instance; i++) {
                 LOG(m_device, DEBUG, "get instance_num :%d\n",instance_num[i]);
            }

            for (i = 0; i < max_instance; i++) {
                if (instance_num[i] != 0) {
                    LOG(m_device, DEBUG, "new add instance name %d !\n", instance_num[i]);
                    res = add_object_tree(instance_num[i], trObj);
                    if (res == 0) {
                        LOG(m_device, DEBUG,"add object %d success!\n", instance_num[i]);
                    } else {
                           LOG(m_device, ERROR,"add object %d fail!\n", instance_num[i]);
                           ret = FAIL;
                    }
                }
            }

            free(instance_num);
        }
 
        res = init_multi_obj(trObj->next_layer);
        if (res != 0) {
            LOG(m_device, ERROR,"add object fail!\n");
            ret = FAIL;
        }
        res = init_multi_obj(trObj->next);
        if (res != 0) {
            LOG(m_device, ERROR,"add object fail!\n");
            ret = FAIL;
        }
    }
    return ret;
}

 /*
 ***************************************************************************
 * function:    param_search 
 * description: locate the param_path location in object tree
 * arguments:   char *name     //full parameter path or partial path you want to use
 * return:      void* pointer   success
 *              NULL            fail
 * **************************************************************************
 */

void * param_search(char *name)
{
    int i = 0, size = -1;
    char *string =NULL, path_bak[256], path[256];
    char *delim = ".";
    int path_type = -1;
    int layer_num = 0;
    int obj_mark = 0;
    struct TR_object *node;
    struct TR_param *full_param = NULL, *tmp_param = NULL;
    node = &root;
    if (name == NULL) 
        return node;
    strcpy(path, name);                             //back up name
    strcpy(path_bak, path);                         //back up path
    size = strlen(path);

    string = strtok(path, delim);                   //split path string, compare every layer string
    if (strcmp(string, DEVICE)) {
        LOG(m_device, ERROR, "Device is not match\n");
        return NULL;
    }
    strcpy(path,path_bak);
                                                                                
    for (i = 1; i < size; i++) {                     // get the layer numbers of the param path
         if ((path[i] == path[i-1] && path[i] == '.') || path[0] == '.') {
            LOG(m_device, ERROR, "wrong parameter path\n");
            return NULL;
        }
        if (path[i] == '.')
            layer_num++;
    }
 
    if (path[size - 1] == '.') {
        path_type = 0;
    } else {
        path_type = 1;
    }

    if (layer_num == 1) {
        if (path_type == 0) {
           return node;
        } else {
            string = strchr(path, '.');
            string++;
            tmp_param = node->param;
            while (tmp_param!= NULL) {
                if (!strcmp(string, tmp_param->name)) {
                    full_param = tmp_param;
                    break;
                }
                tmp_param = tmp_param->next;
            }

            if (full_param == NULL) {
                LOG(m_device, ERROR, "parameter in full is not found\n");
                 return NULL;
            } else {
                return full_param;
                }
        }
    }
    string = strtok(path, delim);
    layer_num--;
    while ( node -> next_layer && layer_num > 0) {           //search the last object node, including full or partial path
        obj_mark = 0;
        node = node -> next_layer;
        string = strtok(NULL, delim);
        do {
            if ((strcmp(string, node -> name) == 0)&&(strcmp(string, "0") != 0)) {
                LOG(m_device, DEBUG, "string:%s\n", string);
                obj_mark = 1;                                // mark object found
                break;
            }
            if (node -> next != NULL)                        //same layer match
                node = node -> next;
            else
                break;
        } while (1);

        if (obj_mark == 0)                                   //if no object match, quit search loop
            break;
        layer_num--;
     }
                    
    if (layer_num >=1) { 
        LOG(m_device, ERROR, "layer search parameter path error\n");
        return NULL;
    }
    /**********************************************************                                                       
    if (obj_mark == 0) {                                      
        LOG(m_device, ERROR, "search parameter path error\n");
        return NULL;
    }
    ***********************************************************/                                                                            
    string = strtok(NULL, delim);
    if (obj_mark && path_type) {
        //search full parameter
        tmp_param = node->param;
        while (tmp_param!= NULL) {
            if (!strcmp(string, tmp_param->name)) {
                full_param = tmp_param;
                break;
            }
            tmp_param = tmp_param->next;
        }
                                                                                
        if (full_param == NULL) {
            LOG(m_device, ERROR, "parameter in full path is not found\n");
            return NULL;
        } else {
	      LOG(m_device, DEBUG,"parameter in full path has find\n");
              return full_param;
          }
    } else if (obj_mark) {
               LOG(m_device, DEBUG, "object search success\n");
               return node;
           }
   return NULL;
}

 /*
 ***************************************************************************
 * function:    get_attr_value
 * description: get element attribute value
 * arguments:   mxml_node_t* theNode
 *              char *path
 * return:      SUCCESS     success
 *              FAIL        fail
 * **************************************************************************
 */

int get_attr_value(mxml_node_t* theNode, char *path)
{
    int i = 0;   
    const char* tmp = 0;
    tmp = mxmlElementGetAttr(theNode, NAME);
    if (tmp == NULL) {
        LOG(m_device, ERROR, "Find Name node in the xml tree fail\n");
        return FAIL;
    }
    i = strlen(tmp);
    if (i >= DEV_LENGTH) {
        LOG(m_device, ERROR, "Input func name characters number must < %d\n", DEV_LENGTH);
        return FAIL;
    }
    strncpy(path, tmp, DEV_LENGTH);
    LOG(m_device, DEBUG, "Globe argument dev func name is %s\n", path);
    return SUCCESS;
}

 /*
 ***************************************************************************
 * function:    get_dev_func_name
 * description: get device function name
 * arguments:   void
 * return:      SUCCESS     success
 *              FAIL        fail
 * **************************************************************************
 */

int get_dev_func_name(void)
{
    int res = 0;
    mxml_node_t* theNode;
    theNode = mxmlFindElement(node, node, TRF_ELE_LIB, NULL, NULL, MXML_DESCEND);
    if (theNode == NULL) {
        LOG(m_device, ERROR, "Find Lib node in the xml tree fail\n");
        return FAIL;
    }
    while (theNode) {
        LOG(m_device, DEBUG, "name %s\n", theNode->value.element.name);
        if (!strcmp(theNode->value.element.name, TRF_ELE_LIB)) {
            res = get_attr_value(theNode, dev_lib_path);      
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_INSTALL)) {
            res = get_attr_value(theNode, first_install_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_MANU)) {
            res = get_attr_value(theNode, manu_dev_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_OUI)) {
            res = get_attr_value(theNode, oui_dev_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_CLASS)) {
            res = get_attr_value(theNode, class_dev_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_SERIAL)) {
            res = get_attr_value(theNode, serial_dev_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_PARAMKEY)) {
            res = get_attr_value(theNode, pk_dev_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_UPFIG)) {
            res = get_attr_value(theNode, get_upfig);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_UPLOG)) {
            res = get_attr_value(theNode, get_uplog);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_DOWNSYS)) {
            res = get_attr_value(theNode, dev_sysflashsizeget_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_DOWNKIL)) {
            res = get_attr_value(theNode, dev_killallapps_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_DOWNPAR)) {
            res = get_attr_value(theNode, dev_parseimagedata_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_TASKWST)) {
            res = get_attr_value(theNode, dev_writestreamtoflash_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_TASKWEB)) {
            res = get_attr_value(theNode, dev_writestreamtoflash_web_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_TASKFLA)) {
            res = get_attr_value(theNode, dev_flashimage_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_TASKREB)) {
            res = get_attr_value(theNode, dev_reboot_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_CAERT)) {
            res = get_attr_value(theNode, ca_cert_path);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_CLERT)) {
            res = get_attr_value(theNode, client_cert_path);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_CLKEY)) {
            res = get_attr_value(theNode, client_key_path);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_WANDEV)) {
            res = get_attr_value(theNode, wan_dev_func);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else if (!strcmp(theNode->value.element.name, TRF_ELE_TRCMD)) {
            res = get_attr_value(theNode, dev_sys_cmd);
            if (res != 0) {
                LOG(m_device, ERROR, "get attr value fail\n");
                return FAIL;
            }
        } else {
            LOG(m_device, ERROR, "name %s is wrong!\n", theNode->value.element.name);
            return FAIL;
        }
        
    theNode = theNode->next;
    }
    return SUCCESS;
}

 /*
 ****************************************************************************
 * function:    init_object_tree 
 * description: initial a tree structure of object
 * argument:    void
 * return:      SUCCESS       success
 *              FAIL          fail
 *****************************************************************************
 */

int init_object_tree(void)
{
    int res = 0, i = 0, num = 0;
    FILE *fp = NULL;
    mxml_node_t* theNode;
    struct TR_param *param_pointer = NULL;
    TR_param_attr* attr_array = NULL;

    fp = fopen(param_conf_path, "r");
    if (fp == NULL) {
        LOG(m_device, ERROR, "open param.xml file failed\n");
        return FAIL;
    }
    LOG(m_device, DEBUG, "open param.xml success\n");

    node = Loadfile_to_memory(NULL, fp, MXML_NO_CALLBACK);
    if (node == NULL) {
        LOG(m_device, ERROR, "load param.xml file failed\n");
        return FAIL;
    }   
    fclose(fp);

    theNode = mxmlFindElement(node, node, TRF_ELE_OBJECT, NULL, NULL, MXML_DESCEND);
    if (theNode == NULL) {
        LOG(m_device, ERROR, "Find OBJ node in the xml tree fail\n");
        return FAIL;
    }
    // initial object tree
    res = create_param_tree (&root, theNode);
    if (res == -1) {
        LOG(m_device, ERROR, "build object structure failed\n");
        return FAIL;
    }

    res = get_dev_func_name();
    if (res != 0) {
        return FAIL;
    }

    mxmlDelete(node);                                      //free mxml tree
    res = init_multi_obj(&root);   
    if (res != 0) {
        LOG(m_device, ERROR, "init multi object fail!\n");
        return FAIL;
    }
    res = init_attr();                                     // initial parameter arribute config file
    if (res == -1) {  
        LOG(m_device, ERROR, "initial attr_conf file failed\n");
        return FAIL;
    }

    fp = fopen(attri_conf_path, "rb");                     // read parameter attributes form config file
    if (fp == NULL) {
        LOG(m_device, ERROR, "open attr_conf file failed\n");
        return FAIL;
    }
    
    res = fread(&num, sizeof(int), 1, fp);
    if (res != 1) {
        LOG(m_device, ERROR, "Can not read the number of attributes form the config file\n");
        fclose(fp);
        return FAIL;
    }

    attr_array = (TR_param_attr *)malloc(num * sizeof(TR_param_attr));
    if (attr_array == NULL) {
        LOG(m_device, ERROR, "malloc space for TR_param_attr failed\n ");
        fclose(fp);
        return FAIL;
    }
    memset(attr_array, 0, num * sizeof(TR_param_attr));
 
    for(i = 0; i < num; i++) {
        res = fread(&attr_array[i], sizeof(TR_param_attr), 1, fp);
        if (res != 1 ) {
            LOG(m_device, ERROR, "Can not read attribute form attr_conf\n");
            fclose(fp);
            return FAIL;
        }
    }

    fclose(fp);
    for (i = 0; i < num; i++) {
        param_pointer = (struct TR_param *)param_search(attr_array[i].name);
        if (param_pointer == NULL) {
            LOG(m_device, ERROR, "Do not find match parameter");
            return FAIL;
        }
        param_pointer -> notification = attr_array[i].notification;
        strcpy(param_pointer -> accesslist, attr_array[i].accesslist);
    }
    free(attr_array);
    return SUCCESS;
}


 /*
 ***************************************************************************
 * Function:    del_object
 * Description: delete the object node in the tree
 * Parameter:   struct TR_object *pre_layer_node
 *              struct TR_object *del_node
 * Return:     void
 *************************************************************************
 */

void del_object(struct TR_object *pre_layer_node, struct TR_object *del_node)
{
    int num = 0;
    struct TR_object *tmp_node;

    tmp_node = pre_layer_node;                                //back up pointer value
    num = atoi(del_node->name);
    
    if (del_node->next_layer != NULL){
        tr_free_obj(del_node->next_layer);
    }

    if (tmp_node->next_layer != del_node) {
        tmp_node = tmp_node -> next_layer;
        while(tmp_node->next != del_node) {
            tmp_node = tmp_node -> next;
        }
        tmp_node -> next = del_node -> next;
    }
    LOG(m_device, DEBUG, "have deleted object!\n");
    free(del_node->param);
    free(del_node);
}

 /*
 ***************************************************************************
 * Function:    add_sub_object
 * Description: add sub layer object
 * Parameter:   struct TR_object *obj
 *              struct TR_object *trobj
 * Return:      SUCCESS     success 
 *              FAIL        FAIL
 *************************************************************************
 */

static int add_sub_object(struct TR_object* obj, struct TR_object* trobj, struct TR_object* pre_obj, int instance_num)
{   
    int i = 0, res = 0, ret = 0;
    if (obj != NULL) {
        *trobj = *obj;                     //evaluate walue
        
        trobj->parent = pre_obj;
        trobj->next = NULL;
        trobj->next_layer = NULL;
        trobj->param = NULL;
        
        for (i = 0; i < MULTI_NUM; i++) {
            trobj->locate[i] = pre_obj->locate[i];
        }
        if (instance_num != 0) {
            LOG(m_device, DEBUG, "Instance number is %d\n", instance_num);
            for (i = 0; i < MULTI_NUM; i++) {
                if (trobj->locate[i] == 0) {
                    trobj->locate[i] = instance_num;
                    break;
                }
            }
        }

        struct TR_param* param_bk = NULL;
        param_bk = obj->param;

        while (obj->param != NULL) {
            struct TR_param* param_tmp = NULL;
            param_tmp = (struct TR_param*)malloc(1 * sizeof(struct TR_param));
            if (param_tmp == NULL) {      
                LOG(m_device, ERROR, "malloc memory param_tmp fail!\n");
                ret = FAIL;
            }
            memset(param_tmp, 0, sizeof(struct TR_param));
            *param_tmp = *obj->param;              //why must write in here   because param_tmp->next pointer is error
            
            //add parameter parent
            param_tmp->parent = trobj;
            if (trobj->param == NULL) {
                trobj->param = param_tmp;
                param_tmp->next = NULL;
            } else {
                  param_tmp->next = trobj->param->next;
                  trobj->param->next = param_tmp;
            }
            //*param_tmp = *obj->param;
            obj->param = obj->param->next;
        }
        obj->param = param_bk;

        if ((obj->name[0] != '0')&& (obj->next != NULL)) {
            struct TR_object* obj_tmp = NULL;
            obj_tmp = (struct TR_object*)malloc(1 * sizeof(struct TR_object));
            if (obj_tmp == NULL) {
                LOG(m_device, ERROR, "malloc memory obj_tmp fail!\n");
                ret = FAIL;
            }
            memset(obj_tmp, 0, sizeof(struct TR_object));
            trobj->next = obj_tmp;
            res = add_sub_object(obj->next, obj_tmp, trobj->parent, 0);
            if (res != 0) {
                LOG(m_device, ERROR, "add sub object fail!\n");
                ret = FAIL;
            }
        }

        if (obj->next_layer != NULL) {
            struct TR_object* trbj_tmp = NULL;
            trbj_tmp = (struct TR_object*)malloc(1 * sizeof(struct TR_object));
            if (trbj_tmp == NULL) {
                LOG(m_device, ERROR, "malloc memory trobj_tmp fail!\n");
                ret = FAIL;
            }
            memset(trbj_tmp, 0, sizeof(struct TR_object));
            trobj->next_layer = trbj_tmp;
            res = add_sub_object(obj->next_layer, trbj_tmp, trobj, 0);
            if (res != 0) {
                LOG(m_device, ERROR, "add sub object fail!\n");
                ret = FAIL;
            }

        }
    }
    return ret;
}

 /*
 **********************************************************************************
 * Function:    add_object_tree
 * Description: add multi object instance 
 * Parameter:   int instance_num
 *              struct TR_object *obj 
 * Return:      SUCCESS      success
 *              FAIL         fail
 ***********************************************************************************
 */

int add_object_tree(int instance_num, struct TR_object* obj)
{
    int res = 0;
    struct TR_object* obj_bk = NULL, *obj_tmp = NULL; 
    obj_bk = obj;
    obj_tmp = (struct TR_object*)malloc(1 * sizeof(struct TR_object));
    if (obj_tmp == NULL) {
        LOG(m_device, ERROR, "malloc memory obj_tmp fail!\n");
        return FAIL;
    }
    memset(obj_tmp, 0, sizeof(struct TR_object));

    if ((obj->next_layer == NULL) || (obj->next_layer->name[0]!= '0')) {
        LOG(m_device, ERROR, "config file fault first name is '0'!\n");
        return FAIL;
    } else {
          obj = obj->next_layer;
          //obj_param_bk = obj->param;      //back obj->param pointer
          while (obj->next != NULL) {
              obj = obj->next;
              if ((atoi(obj->name)) == instance_num) {
                  LOG(m_device, ERROR, "This instance number is eixst in tree:%d!\n", instance_num);
                  return FAIL;
              }
          }
          obj->next = obj_tmp;
    }

    obj = obj_bk->next_layer;

    res = add_sub_object(obj, obj_tmp, obj_bk, instance_num);
    if (res == -1) {
        LOG(m_device, ERROR, "call add sub object fail!\n");
        return FAIL;
    }
    sprintf(obj_tmp->name, "%d", instance_num);
    return SUCCESS;
}

 /*
 ********************************************************************
 * Function:    tr_free_object
 * Description: free object node
 * Parameter:   struct TR_object *objnode - pointer to node which will be delete
 * Return:      void
 ********************************************************************
 */

static void free_param(struct TR_param* param)
{
    if (param) {
        free_param(param->next);
        free(param);
    }
}

 /*
 *******************************************************************
 * Function:    tr_free_object
 * Description: free object node
 * Parameter:   struct TR_object *objnode - pointer to node which will be delete
 * Return:      void
 ********************************************************************
 */

void tr_free_obj(struct TR_object *obj_node)
{
    if (obj_node) {
        tr_free_obj(obj_node->next_layer);
        tr_free_obj(obj_node->next);
        if (obj_node->param) {
            free_param(obj_node->param);
        }
        free(obj_node);
        obj_node = NULL;
    }
}

/*
 *******************************************************************
 * Function:    del_attri_conf
 * Description: delete parameter attribute in attri.conf 
 * Parameter:   struct TR_object *del_node  - pointer to node which will be delete
 *              char *path                  - the delete object path
 * Return:      SUCCESS  -success
 *              FAIL     -failed
 ********************************************************************
 */

int del_attri_conf(char *path, struct TR_object *del_node) 
{
    int i = 0, n = 0, num = 0;
    int attr_conf_num = 0;
    FILE *fp = NULL;
    char *sub_path = NULL;
    int *visited = NULL;
    TR_param_attr *attr_conf_array = NULL;

    LOG(m_device, DEBUG, "delete object path is :%s\n", path);
   
    if ((fp = fopen(attri_conf_path, "rb+")) == NULL) {
        LOG(m_device, ERROR, "Can't open %s\n", attri_conf_path);
        return FAIL;
    }
    if (fread(&attr_conf_num, sizeof(int), 1, fp) != 1) {
        LOG(m_device, ERROR, "Can't read\"num\"from %s\n",attri_conf_path);
        fclose(fp);
        return FAIL;
    }
    LOG(m_device, DEBUG, "exist num attribute in attri.conf: %d\n", attr_conf_num);
    // malloc visited memory
    num = attr_conf_num;
    visited = (int *) malloc (num * sizeof(int)); 
    if (visited == NULL) {
        LOG(m_device, ERROR, "doesn't have enough memory foe malloc!\n");
        fclose(fp);
        return FAIL;
    }
   
    memset(visited, -1, num * sizeof(int));

    attr_conf_array = (TR_param_attr *)malloc(attr_conf_num * sizeof(TR_param_attr));
    memset(attr_conf_array, 0, attr_conf_num * sizeof(TR_param_attr));
    for (i = 0; i < attr_conf_num; i ++) {
        if (fread(&attr_conf_array[i], sizeof(TR_param_attr), 1, fp) != 1) {
            LOG(m_device, ERROR, "Can't read initial parameters and attributes from attri.conf\n");
            fclose(fp);
            free(visited);
            free(attr_conf_array);
            return FAIL;
        } else {
            LOG(m_device, DEBUG, "exist attribute path in attri.conf :%s\n", attr_conf_array[i].name);
            sub_path = strstr(attr_conf_array[i].name, path);
            if (sub_path != NULL) {
                visited[n] = i;
                n++;
                LOG(m_device, DEBUG, "need to delete attribute path: %s\n", sub_path);
            }
        }
    }  
    if (n == 0) {
        LOG(m_device, DEBUG, "doesn't have any this object parameter ,so doesn't need delete any path in attr.conf!\n");
        fclose(fp);
        free(visited);
        free(attr_conf_array);
        return SUCCESS;
    } 
    fclose(fp);
    /********************debug*****************************
    LOG(m_device, DEBUG, "exist number is : %d\n", n);
    for (i = 0; i < n; i ++) 
        LOG(m_device, DEBUG, "visited id :%d\n", visited[i]);
    *******************************************************/
   
    //set new atrr.conf element number 
    attr_conf_num -= n;

    //reset n value
    n = 0;
    
    //wirte new content to attri.conf 
    if ((fp = fopen(attri_conf_path, "wb+")) == NULL) {
        LOG(m_device, ERROR, "Can't open %s\n", attri_conf_path);
        return FAIL;
    }

    if (fwrite(&attr_conf_num, sizeof(int), 1, fp) != 1) {
        LOG(m_device, ERROR, "Can't write the first item into attri.conf file\n");
        fclose(fp);
        free(visited);
        free(attr_conf_array);
        return FAIL;
    }
   
    for (i = 0; i < num; i++) {
        if (visited[n] != i) {
            LOG(m_device, DEBUG, "after delete attribute path :%s\n", attr_conf_array[i].name);
            if (fwrite(&attr_conf_array[i], sizeof(TR_param_attr), 1 ,fp) != 1) {
                LOG(m_handler, ERROR, "Can't write  parameters and attributes into attri.conf\n");
                free(visited);
                free(attr_conf_array);
                fclose(fp);
                return FAIL;
            }
        } else 
            n++;
    }
    fclose(fp);
    free(visited);
    free(attr_conf_array);
    return SUCCESS;
}
