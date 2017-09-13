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
 * param_attri.c    
 * $Author: andyy $
 * $Date: 2007-06-08 02:31:28 $
 */

/***********************************************************************
*
*	include file
*
***********************************************************************/
#include "param_attri.h"
#include "../soap/soap.h"
#include "../../device/TRF_param.h"

/***********************************************************************
*
*	function prototype
*
***********************************************************************/
/* about attri.conf */
static int set_first_item(int num);
static int add_element_attr_conf(TR_param_attr add_element);
static int change_element_attr_conf(TR_param_attr array[], int num);
//static int dele_element_attr_conf(TR_param_attr array[], int num, int i);
static int param_in_attri_conf(char * param_name);

/* about set parameter attribute */
static int parse_set_param_attr_struct(TRF_node *attr_struct_node, TR_set_para_attr  *spattr);
static int handle_noti_list(char *fullname, TR_set_para_attr *spattr);
static int handle_conf(int F_conf_N, int F_conf_A, char *fullname, int notification, char *accesslist);
/* about generating response */

static int gen_get_fault_code(TRF_node *soap_body, int fault_code);
//static int change_noti(char *param_name);
static TRF_node *gen_soap_part(TRF_node *xmlroot);
static TRF_node *gen_get_para_attr_list(TRF_node *method);
static void gen_get_para_attr_list_attr(TRF_node *para_list, int size);
static TRF_node *gen_get_para_attr_struct(TRF_node *para_list);
static int gen_get_para_attr_name(TRF_node *para_struct, char *val);
static int gen_get_para_attr_noti(TRF_node *para_struct,  int val);
static int gen_get_para_attr_acce(TRF_node *para_struct, char *val);
static TRF_node *gen_para_attr_struct(TRF_node *para_list_node, TR_param_attr *param_attr);
static int get_partialpath_attri(TRF_node *para_list_node, struct TR_object *object, char *param_name, int param_num);

/*
 ************************************************************************
 * Function name: set_first_item()
 * Description: Set the first item, which indicate how many array in this configure file
 * Parameter: num:    The number of elements in array
 * Return Value: Successful FAIL
 *************************************************************************
 */
int set_first_item(int num)
{
    FILE *fp = NULL;
    
    if((fp = fopen(attri_conf_path, "rb+")) == NULL){
        LOG(m_handler, ERROR, "Can't open %s \n", attri_conf_path);
        return FAIL;
    }
    
    if(fwrite(&num, sizeof(int), 1, fp) != 1){
        LOG(m_handler, ERROR, "Can't write num into attri.conf file\n");
        fclose(fp);
        return FAIL;
    }
  
    LOG(m_handler, DEBUG, "Now, there are %d parameters in %s\n", num, attri_conf_path);
    fclose(fp);
    return SUCCESS;
}
/*
 **************************************************************************
 * Function name: add_element_attr_conf()
 * Description: Add one TR_para_attr structure element to configure file
 * Parameter: add_element: The element which be added to configure file 
 * Returen Value: Successfull FAIL
 **************************************************************************
 */
int add_element_attr_conf(TR_param_attr  add_element)
{
    FILE *fp = NULL;

    if((fp = fopen(attri_conf_path, "ab+")) == NULL) {
        LOG(m_handler, ERROR, "Fault code = %d\n", PROCESS_ATTRI_CONF_ERROR);
        return FAIL;
    }
    if(fwrite(&add_element, sizeof(TR_param_attr), 1 ,fp) != 1) {
        LOG(m_handler, ERROR, "Can't add parameter and attributes into attri.conf\n");
        fclose(fp);
        return FAIL;
    }
    LOG(m_handler, DEBUG, "Add parameter and attributes successfully\n");
    fclose(fp);
    return SUCCESS;
}
/*
 *************************************************************************
 * Function name: change_element_attr_conf()
 * Description: When the parameter's attributes is changed , save to attri.conf
 * Parameter: array[]: the array of TR_param_attr
 *            num    : the number of elements in array 
 * Return Value:  Successful FAIL
 *************************************************************************
 */
int change_element_attr_conf(TR_param_attr  array[], int num)
{
    FILE *fp = NULL;
    int i;
    
    if((fp = fopen(attri_conf_path, "wb+")) == NULL) {
        LOG(m_handler, ERROR, "Can't open %s \n", attri_conf_path);
        return FAIL;
    }
    if(fwrite(&num, sizeof(int), 1, fp) != 1) {
        LOG(m_handler, ERROR, "Can't write the first item into attri.conf file\n");
        fclose(fp);
        return FAIL;
    }
    for(i = 0; i < num; i++) {
        if(fwrite(&array[i], sizeof(TR_param_attr), 1 ,fp) != 1) {
            LOG(m_handler, ERROR, "Can't write  parameters and attributes into attri.conf\n");
            fclose(fp);
            return FAIL;
        }
    }
    LOG(m_handler, DEBUG, "Change parameter's notification in attri.conf successfully\n");
    fclose(fp);
    return SUCCESS;
}

/*
 **************************************************************************
 * Function name: dele_element_attr_conf()
 * Description: This function will delete one item whose index is i
 * Parameter:
 *       array[]: The array of TR_param_attr
 *       num    : The number of elements in array
 *       i:       The index of array, the array[i] will be deleted from configure file
 * Return Value: Successful FAIL
 ***************************************************************************
 *
int dele_element_attr_conf(TR_param_attr  array[], int num, int i)
{
    FILE *fp;
    int j, surplus_num;
    
    surplus_num = num - 1;
    
    if((fp = fopen(attri_conf_path, "wb+")) == NULL) {
        LOG(m_handler, ERROR, "Can't open %s \n", attri_conf_path);
        return FAIL;
    }
    if(fwrite(&surplus_num, sizeof(int), 1, fp) != 1) {
        LOG(m_handler, ERROR, "Can't write the first item into attri.conf file\n");
        fclose(fp);
        return FAIL;
    }
    for(j = 0; j < num; j++) {    
        if(j != i) {
            if(fwrite(&array[j], sizeof(TR_param_attr), 1 ,fp) != 1) {
                LOG(m_handler, ERROR, "Can't write  parameters and attributes into attri.conf\n");
                fclose(fp);
                return FAIL;
            }
    	}
    }
    LOG(m_handler, DEBUG, "Delete parameter's notification in attri.conf successfully\n");
    fclose(fp);
    return SUCCESS;
}*/
/*
 *************************************************************************
 * Function name: param_in_attri_conf()
 * Description: Judge whether the parameter is in attri.conf
 * Parameter:
 *       param_name: parameter name
 * Return Value: index in attri.conf, -1 Unsuccessful, -2 this parameter is not in attri.conf
 *************************************************************************** 
 */
int param_in_attri_conf(char *param_name)
{
    TR_param_attr   *attr_conf_array = NULL;
    
    FILE *fp = NULL;
    int attr_conf_num;
    int attr_count;
    int i = 0;
    
    //Operation about attri.conf
    if((fp = fopen(attri_conf_path, "rb+")) == NULL) {
        LOG(m_handler, ERROR, "Can't open %s \n", attri_conf_path);
        return FAIL;
    }
    LOG(m_handler, DEBUG, "Open %s successfully\n", attri_conf_path);
                                
    if(fread(&attr_conf_num, sizeof(int), 1,fp) != 1) {
        LOG(m_handler, ERROR, "Can't read \"num\" from %s\n", attri_conf_path);
        fclose(fp);
        return FAIL;
    }
    LOG(m_handler, DEBUG, "attr_conf_num = %d\n", attr_conf_num);
    attr_conf_array = (TR_param_attr *)malloc(attr_conf_num * sizeof(TR_param_attr));
    if(attr_conf_array == NULL) {
        LOG(m_handler, ERROR, "Allocate memory for attr_conf_array unsuccessfully\n ");
        fclose(fp);
        return FAIL;
    }    
    for(attr_count = 0; attr_count < attr_conf_num; attr_count++) {
        if(fread(&attr_conf_array[attr_count], sizeof(TR_param_attr), 1 ,fp) != 1) {
            LOG(m_handler, ERROR, "Can't read initial parameters and attributes from attri.conf\n");
            fclose(fp);
            free(attr_conf_array);
            return FAIL;
        }
    }
    for(attr_count = 0; attr_count < attr_conf_num; attr_count++) {
        if(!strcmp(attr_conf_array[attr_count].name, param_name)) {
            LOG(m_handler, DEBUG, "Parameter: %s is in attri.conf\n", param_name);
            break;
        }
        i++;        
    }
    if(i == attr_conf_num) {
        LOG(m_handler, DEBUG, "Parameter: %s is not in attri.conf\n", param_name);
        fclose(fp);
        free(attr_conf_array);
        return NOT_IN_CONF;
    }
    free(attr_conf_array);
    fclose(fp);
    return attr_count;
}

/*
 ************************************************************************
 * Function: change_noti()
 * Description :Judge whether the parameter could be set Notfication
 * Parameter:param_name: The parameter name
 * Retrun Value:0 , this parameter coulde be set Notification, -1 ,this parmameter
 *              can't be set Notification   
 ************************************************************************
 *
int change_noti(char *param_name)
{
    if(!strcmp(DEVICEINFOSOFTWAREVERSION, param_name)) {
        LOG(m_handler, DEBUG, "This parameter couldn't be set Notification\n");
        return CANT_BE_SET;
    }
    if(!strcmp(DEVICEINFOPROVISIONINGCODE, param_name)) {
        LOG(m_handler, DEBUG, "This parameter couldn't be set Notification\n");
        return CANT_BE_SET;
    }
    if(!strcmp(MANAGEMENTSERVERCONREQURL, param_name)) {
	LOG(m_handler, DEBUG, "This parameter couldn't be set Notification\n");
        return CANT_BE_SET;
    }
    if(!strcmp(wan_para, param_name)) {
	LOG(m_handler, DEBUG, "This parameter couldn't be set Notification\n");
        return CANT_BE_SET;
    }
*    if(!strcmp(WANDEVICEPPPEXTERNALADDRESS, param_name)) {
	LOG(m_handler, DEBUG, "This parameter couldn't be set Notification\n");
        return CANT_BE_SET;
    }*     
    LOG(m_handler, DEBUG, "This parameter can be set Nofication\n");
    return SUCCESS;
}*/
/*
 **************************************************************************
 * Function name: gen_soap_part()
 * Description: Generate SOAP partion  <SOAP-ENV:Envelope> <SOAP-ENV:Header> <SOAP-ENV:Body>
 * Param: xmlroot: point to xml root node
 * Return Value  : Success Point to <SOAP-ENV:Body> node  else return NULL
 **************************************************************************
 */
TRF_node *gen_soap_part(TRF_node *xmlroot)
{
    TRF_node *soap_env = NULL, *soap_body = NULL;

    if ((soap_env = gen_env(xmlroot)) == NULL) {
    	LOG(m_handler, ERROR, "Generate <SOAP-ENV:Envelope> error\n");
        return NULL;
    }
    LOG(m_handler, DEBUG, "Generate <SOAP-ENV:Envelope> successfully\n");
    
    if(gen_soap_header(soap_env) == NULL) {
        LOG(m_handler, ERROR, "Generate <SOAP-ENV:Header> error\n");
        return NULL;
    }
    if ((soap_body = gen_soap_body(soap_env)) == NULL) {
    	LOG(m_handler, ERROR, "SetParameteratttributesResponse: <SOAP-ENV:Body> error\n");
    	return NULL;
    }
    LOG(m_handler, DEBUG, "Generate soap body successfully\n");
    
    return soap_body;
}

/*
 **************************************************************************
 * Function name: gen_get_fault_code()
 * Description: generate fault xml for other errors
 * Parameter: soap_body: pointer to soap body, fault_code
 * Return: SUCCESS FAIL
 *************************************************************************
 */
int gen_get_fault_code(TRF_node *soap_body, int fault_code)
{
    TRF_node *soap_fault_node = NULL;
    TRF_node *cwmp_fault_node = NULL;

    char metd_name[] = "GetParameterAttributes";
    unsigned int cwmp_fault_code;

    cwmp_fault_code = fault_code;
    soap_fault_node = gen_soap_fault(soap_body);
    if(soap_fault_node == NULL) {
	LOG(m_handler, ERROR, "Can't generate <SOAP-ENV:Fault> node\n");
	return FAIL;
    }
    cwmp_fault_node = gen_cwmp_fault(soap_fault_node, cwmp_fault_code, metd_name);
    if(cwmp_fault_node == NULL) {
        LOG(m_handler, ERROR, "Can't generate <cwmp:Fault> node\n");
	return FAIL;
    }
    return SUCCESS;
}
/*
 ****************************************************************************
 * Function name: parse_set_param_attr_struct()
 * Description: parse struct
 * Param: attr_struct_node, spattr
 * Return: SUCCESS:0, FAIL:-1, gen fault response: 1. 
 ************************************************************************((
 */
int parse_set_param_attr_struct(TRF_node *attr_struct_node, TR_set_para_attr  *spattr)
{
    TRF_node *name_node=NULL, *noti_change_node=NULL, *notification_node=NULL, *acc_change_node=NULL, *accesslist_node=NULL;
    struct TR_object  *obj = NULL;
    struct TR_param *tr_param = NULL, *tmp_param = NULL;
    int len;
    //char fullname[NAME_LEN];
    char metd_name[] = "SetParameterAttributes";

    name_node = attr_struct_node->child;
    noti_change_node = name_node->next;
    if(noti_change_node == NULL) {
        if ((gen_method_fault(xmlroot, metd_name, INVALID_ARGUMENT)) == NULL){
            LOG(m_handler, ERROR, "Gen_set_param_name_fault_code failed.\n");
            return FAIL;
        }
        return GEN_FAULT_CODE;
    }
    notification_node = noti_change_node->next;
    if(notification_node == NULL) {
        if ((gen_method_fault(xmlroot, metd_name, INVALID_ARGUMENT)) == NULL){
            LOG(m_handler, ERROR, "Gen_set_param_name_fault_code failed.\n");
            return FAIL;
        }
        return GEN_FAULT_CODE;
    }
    acc_change_node = notification_node->next;
    if(acc_change_node == NULL) {
        if ((gen_method_fault(xmlroot, metd_name, INVALID_ARGUMENT)) == NULL){
            LOG(m_handler, ERROR, "Gen_set_param_name_fault_code failed.\n");
            return FAIL;
        }
        return GEN_FAULT_CODE;
    }
    accesslist_node = acc_change_node->next;
    if(accesslist_node == NULL) {
        if ((gen_method_fault(xmlroot, metd_name, INVALID_ARGUMENT)) == NULL){
            LOG(m_handler, ERROR, "Gen_set_param_name_fault_code failed.\n");
            return FAIL;
        }
        return GEN_FAULT_CODE;
    }

    /*parse <Name> first*/
    if (name_node->child != NULL) {
        LOG(m_handler, DEBUG, "It is not NULL between <Name> and </Name>\n");
        strcpy(spattr->name, name_node->child->value.opaque);
        len = strlen(spattr->name);
        if(spattr->name[len-1] == '.') {
            LOG(m_handler, DEBUG, "This is a partial path name\n");    
            obj = param_search (spattr->name);
            if (obj == NULL) {
                LOG(m_handler, ERROR, "Error parameter name.\n");
                if ((gen_method_fault(xmlroot, metd_name, INVALID_PARAM_NAME)) == NULL){
                    LOG(m_handler, ERROR, "Gen_set_param_name_fault_code failed.\n");
                    return FAIL;
                }
                return GEN_FAULT_CODE;  
            } 
            tmp_param = obj->param;
            while(tmp_param != NULL){
                if (tmp_param -> noti_rw == TR_NOTI_RDONLY) {
                    if ((gen_method_fault(xmlroot, metd_name, NOTIFY_REQ_REJECT)) == NULL){
                        LOG(m_handler, ERROR, "Gen_method_code failed.\n");
                        return FAIL;
                    }
                    return GEN_FAULT_CODE;
                }
                tmp_param = tmp_param->next;
            } 
        } else {
            LOG(m_handler, DEBUG, "It is a full name:%s\n", spattr->name);
            tr_param = param_search (spattr->name);
            if (tr_param == NULL) {
                LOG(m_handler, ERROR, "Error parameter name.\n");
                if ((gen_method_fault(xmlroot, metd_name, INVALID_PARAM_NAME)) == NULL){
                    LOG(m_handler, ERROR, "Gen_set_param_name_fault_code failed.\n");
                    return FAIL;
                }
                return GEN_FAULT_CODE;
            }
            if (tr_param -> noti_rw == TR_NOTI_RDONLY) {
                if ((gen_method_fault(xmlroot, metd_name, NOTIFY_REQ_REJECT)) == NULL){
                    LOG(m_handler, ERROR, "Gen_method_code failed.\n");
                    return FAIL;
                }
                return GEN_FAULT_CODE;
            }
            LOG(m_handler, DEBUG, "Find out parameter:%s\n", spattr->name); 
        }
    } else {
        strcpy(spattr->name, INTERNETGATEWAYDEVICE);
    }
    /*parse <NotificationChange> second */
    if(!strcmp(noti_change_node->child->value.opaque, "true") || !strcmp(noti_change_node->child->value.opaque, "1")) {
        spattr->noti_change = 1;
        //parse <Notification> third -- anna
        spattr->notification = atoi(notification_node->child->value.opaque);   
        if (spattr->notification == 0 || spattr->notification == 1 || spattr->notification == 2) {
            LOG(m_handler, DEBUG, "The value of Notification is: %d\n", spattr->notification);
        } else {
            LOG(m_handler, ERROR, "Error value of Notification: %d\n", spattr->notification);
            if ((gen_method_fault(xmlroot, metd_name, INVALID_ARGUMENT)) == NULL){   
                LOG(m_handler, ERROR, "Gen_method_code failed.\n");
                return FAIL;
            }
            return GEN_FAULT_CODE;  
        }
    } else if (!strcmp(noti_change_node->child->value.opaque, "false") || !strcmp(noti_change_node->child->value.opaque, "0")) {
         spattr->noti_change = 0;
    } else {
         LOG(m_handler, ERROR, "Error value of NotificationChange: %s\n", noti_change_node->child->value.opaque);
         if ((gen_method_fault(xmlroot, metd_name, INVALID_ARGUMENT)) == NULL){   
             LOG(m_handler, ERROR, "Gen_method_code failed.\n");
             return FAIL;
         }
         return GEN_FAULT_CODE;            				 
    }
    LOG(m_handler, DEBUG, "The value of NotificationChange is : %s\n", noti_change_node->child->value.opaque);
             
    /*parse <AccessListChange> forth*/
    if(!strcmp(acc_change_node->child->value.opaque, "true") || !strcmp(acc_change_node->child->value.opaque, "1")) {
        spattr->access_list_change = 1;
        if(accesslist_node != NULL) {
            if(accesslist_node->child != NULL) {
                strcpy(spattr->accesslist, accesslist_node->child->value.opaque);
            } else {
                strcpy(spattr->accesslist, "");
            }
        } else {
            LOG(m_handler, ERROR, "Accelist error.\n")
            if ((gen_method_fault(xmlroot, metd_name, INVALID_ARGUMENT)) == NULL){   
                LOG(m_handler, ERROR, "Gen_method_code failed.\n");
                return FAIL;
            }
            return GEN_FAULT_CODE;
        }   
    } else if (!strcmp(acc_change_node->child->value.opaque, "false") || !strcmp(acc_change_node->child->value.opaque, "0")) {
        spattr->access_list_change = 0;
    } else {
         LOG(m_handler, ERROR, "Accelist_change error: %s.\n", acc_change_node->child->value.opaque)
         if ((gen_method_fault(xmlroot, metd_name, INVALID_ARGUMENT)) == NULL){   
             LOG(m_handler, ERROR, "Gen_method_code failed.\n");
             return FAIL;
         }
         return GEN_FAULT_CODE;            				 
    }
    LOG(m_handler, DEBUG, "The value of AccessListChange is:%d\n", spattr->access_list_change);
    return SUCCESS;             
}
/*
 **************************************************************************
 * Function name: handle_noti_list()
 * Description: handle set attributes
 * Param: fullname: parameter's full name,spattr: pointer to TR_set_para_attr
 * Return: SUCCESS FAIL 
 **************************************************************************
 */
int handle_noti_list(char *fullname, TR_set_para_attr *spattr)
{
    int F_conf_N = 0, F_conf_A = 0;
    struct TR_param *tr_param = NULL;

    tr_param = param_search(fullname);
    if (spattr->noti_change == 1) {
       // LOG(m_handler, DEBUG, "name= %s****Notification = %d\n", fullname, spattr->notification);
        if (spattr->notification == tr_param->notification)
            F_conf_N = NOT_CHANGE_CONF;
        else
            F_conf_N = NEED_CHANGE_CONF;
     
        tr_param->notification = spattr->notification;  //write to parameter tree  
    }
    if (spattr->access_list_change == 1) {
        if (!strcmp(spattr->accesslist,tr_param->accesslist))
            F_conf_A = NEED_CHANGE_CONF;
        else
            F_conf_A = NOT_CHANGE_CONF;

        strcpy(tr_param->accesslist , spattr->accesslist); //wirte to parameter tree
    }
   handle_conf(F_conf_N, F_conf_A, fullname, tr_param -> notification, tr_param->accesslist);  //write to attr.conf 
    return SUCCESS;
}

/*
 ************************************************************************************************************************
 * Function name: handle_conf()
 * Description: set attributes in attri.conf
 * Parameter:  int F_conf_N, int F_conf_A: flag of notification or attribute;
               char *fullname: parameter's full name 
               int notification :notification
               char *accesslist :accesslist
 * Return: SUCCESS, FAIL 
 *************************************************************************************************************************
 */
int handle_conf(int F_conf_N, int F_conf_A, char *fullname, int notification, char *accesslist)
{
    int res = 0, locate;
    TR_param_attr add_element, *attr_conf_array = NULL;
    int attr_conf_num, i;
    FILE *fp = NULL;

    if ((fp = fopen(attri_conf_path, "rb+")) == NULL) {
        LOG(m_handler, ERROR, "Can't open %s\n", attri_conf_path);
        return FAIL;
    }
    if (fread(&attr_conf_num, sizeof(int), 1, fp) != 1) {
        LOG(m_handler, ERROR, "Can't read\"num\"from %s\n",attri_conf_path);
        fclose(fp);
        return FAIL;
    }
 
    attr_conf_array = (TR_param_attr *)malloc(attr_conf_num * sizeof(TR_param_attr));
    for(i = 0; i < attr_conf_num; i ++) {
        if(fread(&attr_conf_array[i], sizeof(TR_param_attr), 1, fp) != 1) {
            LOG(m_handler, ERROR, "Can't read initial parameters and attributes from attri.conf\n");
            fclose(fp);
            free(attr_conf_array);
            return FAIL;
        }
    }
    fclose(fp);

    if (F_conf_N == 1 || F_conf_A == 1) {
        res = param_in_attri_conf(fullname);
        if (res >= 0) {
            LOG(m_handler, DEBUG, "Find out parameter in the attr.conf.\n");
            locate = res;
            attr_conf_array[locate].notification = notification;
            strcpy(attr_conf_array[locate].accesslist, accesslist);
            change_element_attr_conf(attr_conf_array, attr_conf_num);
            LOG(m_handler, DEBUG, "Change element in attr_conf ok.\n");
            free(attr_conf_array);
	    return SUCCESS;
        } else if(res == NOT_IN_CONF){
            LOG(m_handler, DEBUG, "Parameter isnot in the attr.conf.\n");
            strcpy(add_element.name, fullname);
            add_element.notification = notification;
            strcpy(add_element.accesslist, accesslist);

            attr_conf_num = attr_conf_num + 1;
            res = set_first_item(attr_conf_num);
            if (res != 0) {
                LOG(m_handler, ERROR, "Add new element to attri.conf failed.\n");
                free(attr_conf_array);
                return FAIL;
            }
            res = add_element_attr_conf(add_element);
            if (res != 0) {
                LOG(m_handler, ERROR, "Add new element to attri.conf failed.\n");
                free(attr_conf_array);
                return FAIL;
            }
            LOG(m_handler, DEBUG, "Add element to attr_conf ok.\n");
            free(attr_conf_array);
            return SUCCESS;
        } else {
            LOG(m_handler, ERROR, "Find out failed.\n");
            free(attr_conf_array);
            return FAIL;
        }
    } /*else {
        res = param_in_attri_conf(fullname);
        if (res >= 0) {
            LOG(m_handler, DEBUG, "Find out parameter in the attr.conf.\n");
            locate = res;
            res = dele_element_attr_conf(attr_conf_array, attr_conf_num, locate);
            if (res == -1) {
                LOG(m_handler, ERROR, "Del element in attri.conf fail.\n");
                free(attr_conf_array);
                return FAIL;
            }
            LOG(m_handler, DEBUG, "Del element in attr_conf ok\n");
            free(attr_conf_array);
            return SUCCESS;
        } else if(res == NOT_IN_CONF){
            LOG(m_handler, DEBUG, "Parameter isnot in the attr.conf.\n");
            free(attr_conf_array);
            return SUCCESS;
        } else {
            LOG(m_handler, ERROR, "Find out failed.\n");
            free(attr_conf_array);
            return FAIL;
        }
    }*/
    return SUCCESS;
}
/*
 **************************************************************************
 * Function name: process_set_para_attr()
 * Description:  parse SetParameterAttribute method 
 * Param: method: point to the <cwmp:SetParameterAttributes> node
 * Return Value: SUCCESS FAIL
 ************************************************************************** 
 */
int process_set_para_attr(TRF_node *method)
{
    TRF_node *para_list_node = NULL, *attr_struct_node = NULL, *tmpnode = NULL;
    TRF_node *method_node = NULL;
    TR_set_para_attr  *spattr = NULL;
    struct TR_object  *obj = NULL;
    struct TR_param   *tmp_param = NULL;
       
    char partial_name[NAME_LEN];
    int len, res = 0;
    int i = 0, struct_num = 0;
    
    char fullname[NAME_LEN];
    char resp_name[] = "SetParameterAttributesResponse";
//    char metd_name[] = "SetParameterAttributes";

    para_list_node = mxmlFindElement(method, xmltop, "ParameterList", NULL, NULL, MXML_DESCEND);//Find <ParameterList> node
   
    if(para_list_node == NULL) {
        LOG(m_handler, DEBUG, "Can't find ParameterList node\n");
        method_node = gen_soap_frame(resp_name);
        if(method_node == NULL) {
    	    LOG(m_handler, ERROR, "Generate SetParameterAttributesResponse error\n");
    	    return FAIL;
        }
        return SUCCESS;
    }
    attr_struct_node = mxmlFindElement(para_list_node, xmltop, "SetParameterAttributesStruct", NULL, NULL, MXML_DESCEND);  //Find <SetParameterAttributesStruct> node
    
    if(attr_struct_node == NULL || para_list_node->child == NULL) {
        LOG(m_handler, DEBUG, "The ParameterList of SetParameterAttributes request has no parameters in this envelope\n");
        method_node = gen_soap_frame(resp_name);
        if(method_node == NULL) {
    	    LOG(m_handler, ERROR, "Generate SetParameterAttributesResponse error\n");
    	    return FAIL;
        }
        return SUCCESS;
    }  
         
    /*Calculate how many items in SetParameterAttributesStruct[]*/
    for(tmpnode = attr_struct_node; tmpnode != NULL; tmpnode = tmpnode->next) {
        struct_num ++;
    }
    LOG(m_handler, DEBUG, "The num of SetParameterAttributesStruct is :%d\n", struct_num);
    spattr = (TR_set_para_attr  *)malloc(struct_num * sizeof(TR_set_para_attr));
    if (spattr == NULL) {
        LOG(m_handler, ERROR, "Malloc memory for TR_set_para_attr_struct failed.\n");
        return FAIL;
    }
    for(i = 0; i < struct_num; i++) {   
    	/*Whether it is null between <SetParameterAttributesStruct> and </SetParameterAttributesStruct>*/
    	if(attr_struct_node->child != NULL) {    
            res = parse_set_param_attr_struct(attr_struct_node, &spattr[i]);
    	     if (res == 0) {
                 LOG(m_handler, DEBUG, "Parse set parameter attributes struct successful.\n");
             } else if(res == -1) {
                 LOG(m_handler, ERROR, "Parse set parameter attributes struct failed.\n");
                 free(spattr);
                 return FAIL;
             } else if(res == GEN_FAULT_CODE) {
                LOG(m_handler, DEBUG, "Generate fault code already.\n");
                free(spattr);
                return SUCCESS;
            }
        } //end if(attr_struct_node->child != NULL)
        attr_struct_node = attr_struct_node->next;
    }
    for(i = 0; i < struct_num; i++) {
            len = strlen(spattr[i].name);
            if(spattr[i].name[len-1] == '.') {
                LOG(m_handler, DEBUG, "This is a partial path name\n");    
                strcpy(partial_name, spattr[i].name);

                obj = param_search(partial_name);
                tmp_param = obj->param;
                while(tmp_param != NULL) {
                    strcpy(fullname, partial_name);
                    strcat(fullname, tmp_param->name);
                    LOG(m_handler, DEBUG, "The full name is %s\n", fullname);       
                    handle_noti_list(fullname, &spattr[i]);    
                    tmp_param = tmp_param->next;
                }
            } else {
        	LOG(m_handler, DEBUG, "This is a full parameter name\n");
                strcpy(fullname, spattr[i].name);
                handle_noti_list(fullname, &spattr[i]); 
            }
        attr_struct_node = attr_struct_node->next;
        
    } //end for
    free(spattr);
    method_node = gen_soap_frame(resp_name);
    if(method_node == NULL) {
    	LOG(m_handler, ERROR, "Generate SetParameterAttributesResponse error\n");
    	return FAIL;
    }
    return SUCCESS;
}

/*
 ***************************************************************************
 * Function name: gen_get_para_attr_list
 * Description: Generate <ParameterList> node
 * Param: method: point to <cwmp:GetParameterAttributesResponse> node
 * Return Value : if successful, point to the <ParameterList> node
 *                else NULL
 ************************************************************************
 */
TRF_node *gen_get_para_attr_list(TRF_node *method)
{
    TRF_node *para_list = NULL;
   
    para_list = mxmlNewElement(method, "ParameterList");
    
    if(para_list == NULL) {
        LOG(m_handler, ERROR, "Can't generate <ParameterList> node\n");
        return NULL;
    }
    return para_list;
}

/*
 *************************************************************************
 * Function name: gen_get_para_attr_list_attr()
 * Description: Generate <ParameterList> node 's attribute and attribute value
 * Param: para_list: point to <ParameterList> node
 *       size:      the item number of ParameterList array
 * Return Value : Nothing
 *************************************************************************
 */
void gen_get_para_attr_list_attr(TRF_node *para_list, int size)
{
    char para_attr_struct[64];
    
    sprintf(para_attr_struct, "cwmp:ParameterAttributeStruct[%d]", size);
    mxmlElementSetAttr(para_list, "SOAP-ENC:arrayType", para_attr_struct);   
}
 
/*
 *************************************************************************
 * Function name: gen_get_para_attr_struct()
 * Description: Generate <ParameterAttribute> node
 * Parameter: para_list: point to <ParameterList> node
 * Return Value : if successful, point to the <ParameterAttribute> node
 *                else NULL
 ************************************************************************
 */
TRF_node *gen_get_para_attr_struct(TRF_node *para_list)
{
    TRF_node *para_struct = NULL;
    
    para_struct = mxmlNewElement(para_list, "ParameterAttributeStruct");
    
    if(para_struct == NULL) {
        LOG(m_handler, ERROR, "Can't generate <ParameterAttriubteStruct> node\n");
        return NULL;
    }
    return para_struct;
}

/*
 ************************************************************************
 * Function name: gen_get_para_attr_name()
 * Description: Generate <Name> node
 * Parameter:  para_list: point to <ParameterAttributeStruct> node
 * Return Value : if successful, point to the <Name> node
 *                else  NULL
 ************************************************************************
 */   
int  gen_get_para_attr_name(TRF_node *para_struct, char *val)
{
    TRF_node *para_name = NULL, *name_val = NULL;
	
    para_name = mxmlNewElement(para_struct, "Name");
    if(para_name == NULL) {
        LOG(m_handler, ERROR, "Can't generate <Name> node\n");
        return FAIL;
    } 
    name_val = mxmlNewText(para_name, 0, val);
    if(name_val == NULL) {
        LOG(m_handler, ERROR, "Can't generate <Name>'s child node\n");
        return FAIL;
    }
    return SUCCESS;
}
    
/*
 *********************************************************************
 * Function name: gen_get_para_attr_noti()
 * Description: Generate <Notification> node
 * Parameter:
 *       para_list: point to <ParameterAttributeStruct> node
 * Return Value : if successful, point to the <Notification> node
 *                else NULL
 *********************************************************************
 */
int gen_get_para_attr_noti(TRF_node *para_struct,  int val)
{
    TRF_node *notification = NULL, *noti_val = NULL;
	
    notification = mxmlNewElement(para_struct, "Notification");
    if(notification == NULL) {
        LOG(m_handler, ERROR, "Can't generate <Notification> node\n");
        return FAIL;
    } 
    noti_val = mxmlNewInteger(notification, val);
    if(noti_val == NULL) {
        LOG(m_handler, ERROR, "Can't generate <Name>'s child node\n");
        return FAIL;
    }
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: gen_get_para_attr_acce()
 * Description:     Generate <AccessList> node
 * Parameter:
 *       para_list: point to <ParameterAttributeStruct> node
 * Return Value:    if successful, point to the <AccessList> node
 *                  else NULL 
 *********************************************************************
 */
int gen_get_para_attr_acce(TRF_node *para_struct, char *val)
{
    TRF_node *acce_list = NULL;
    TRF_node *acce_list_sub = NULL, *acce_list_sub_val = NULL;
    char string[64];
    int size = 1;
		
    acce_list = mxmlNewElement(para_struct, "AccessList");
    
    if(acce_list == NULL) {
        LOG(m_handler, ERROR, "Can't generate <AccessList> node\n");
        return FAIL;
    } 
    sprintf(string, "xsd:string[%d]", size);
    mxmlElementSetAttr(acce_list, "SOAP-ENC:arrayType", string);

    acce_list_sub = mxmlNewElement(acce_list, "string");
    if(acce_list_sub == NULL) {
        LOG(m_handler, ERROR, "Can't generate Accesslist item node\n");
        return FAIL;
    }
    acce_list_sub_val = mxmlNewText(acce_list_sub, 0, val);
    if(acce_list_sub_val == NULL) {
        LOG(m_handler, ERROR, "Can't generate <Name>'s child node\n");
        return FAIL;
    }
    return SUCCESS;
}
/*
 ************************************************************************
 * Function name: gen_para_attr_struct()
 * Description: Generate <ParameterAttributeStruct>...</ParameterAttributeStruct>
 * Parameter:
 *       para_list_node: point to <ParameterList> node 
 *       param_attr    : an pointer of TR_param_attr structure
 *       acce_list_count: the number of elements in ParameterList array
 * Return Value: SUCCESS FAIL
 ************************************************************************
 */
TRF_node *gen_para_attr_struct(TRF_node *para_list_node, TR_param_attr *param_attr)
{
    TRF_node *attr_struct_node = NULL;
    int res ;
    
    attr_struct_node = gen_get_para_attr_struct(para_list_node);
    if(attr_struct_node == NULL) {
        LOG(m_handler, ERROR, "Generate <GetParameterAttributeStruct> Unsuccessfully\n");
        return NULL;
    }              
                                                       
    /*Generate <Name>,,,</Name>*/
    res = gen_get_para_attr_name(attr_struct_node, param_attr->name);
    if(res == -1) {
        return NULL;
    }
              
    /*Generate <Notification>...</Notification>*/
    res = gen_get_para_attr_noti(attr_struct_node,  param_attr->notification);
    if(res == -1) {
        return NULL;
    }
                          
    /*Generate <AccessList SOAP-ENV:array="xsd:string[]">
                 <AccessList>...<AccessList>
             </AccessList> */      
    res = gen_get_para_attr_acce(attr_struct_node, param_attr->accesslist);
    if(res == -1) {
        return NULL;
    }
    return attr_struct_node;
}
/*
 **************************************************************************************************************
 * Function name: get_partialpath_attri()
 * Description: 
 * Parameters: para_list_node, object, param_name, param_num
 * Return: SUCCESS FAIL       
 **************************************************************************************************************
*/
int get_partialpath_attri(TRF_node *para_list_node, struct TR_object *object, char *param_name, int param_num)
{
    struct TR_param *param = NULL;
    char param_name_bk1[PARAM_FULL_NAME_LEN + 1], param_name_bk2[PARAM_FULL_NAME_LEN + 1];
    TR_param_attr  param_attr;
    TRF_node *attr_struct_node = NULL;
    int res, num;
    //init param name
    memset(param_name_bk1, 0, sizeof(param_name_bk1));
    memset(param_name_bk2, 0, sizeof(param_name_bk2));
    num = param_num;
    if (object) {
        if (object->name[0] == '0') {
            object = object->next;
            if (object == NULL) {
                return SUCCESS;
            }
        }
        
        strcpy(param_name_bk1, param_name);
        strcat(param_name_bk1, object->name);
        strcat(param_name_bk1, ".");
    //    LOG(m_handler, DEBUG, "param_name_bk1:%s\n", param_name_bk1);        
        param = object->param;
        while (param != NULL) {
            //get param name
            strcpy(param_name_bk2, param_name_bk1);
            strcat(param_name_bk2, param->name);

            strcpy(param_attr.name , param_name_bk2);     
            param_attr.notification = param->notification;
    //        LOG(m_handler, DEBUG, "notification = %d\n",param->notification);
            strcpy(param_attr.accesslist, param->accesslist);
            attr_struct_node = gen_para_attr_struct(para_list_node, &param_attr);
            if(attr_struct_node == NULL) {
                LOG(m_handler, ERROR, "Generate <ParameterAttributeStrcut>...</ParameterAttributeStruct> unsuccessfully\n");
                return -1;
            }

            num++;
            param = param->next;
        }
        res = get_partialpath_attri(para_list_node, object->next, param_name, num);
        if (res == -1) {
            //LOG(m_handler, ERROR, "I'm here.\n");
            return FAIL;
        } else {
            num = res;
        }
        res = get_partialpath_attri(para_list_node, object->next_layer, param_name_bk1, num);
        if (res == -1) {
            //LOG(m_handler, ERROR, "I'm there.\n");
            return FAIL;
        } else {
            num = res;
        }
   }
  // LOG(m_handler, DEBUG, "num = %d\n", num);
   return num;     
}
    
/*
 ************************************************************************
 * Function name: process_get_para_attr()
 * Description: Parse GetParameterAttributes method
 * Param: method: point to the <cwmp:GetParameterAttributes> node
 * Return: SUCESS FAIL
 ************************************************************************
 */
int process_get_para_attr(TRF_node *method)
{
    TRF_node *para_name = NULL, *para_name_child = NULL, *tmpnode = NULL, *soap_body = NULL;
    
    struct TR_object  *obj = NULL;
    struct TR_param   *tr_param= NULL, *tmp_param = NULL;
    TR_param_attr  param_attr;
    char parameter_name[257];   
    int  len;
    int i = 0, struct_num = 0, res = 0;
    
    /*Arguments about generating GetParameterAttributesResponse*/
    TRF_node *method_node= NULL;
    TRF_node *para_list_node = NULL;
    TRF_node *attr_struct_node = NULL;

    char metd_resp_name[] = "GetParameterAttributesResponse";
    int para_attr_count = 0;
  
    LOG(m_handler, DEBUG, "Before generate SOAP part\n");

    if((soap_body = gen_soap_part(xmlroot)) == NULL) {
        LOG(m_handler, ERROR, "Generate SOAP part unsuccessfully\n");
        return FAIL;
    }
    if ((method_node = gen_method_name(metd_resp_name, soap_body)) == NULL) {
    	LOG(m_handler, ERROR, "Generate GetParameterAttributesResponse method name unsuccessfully\n");
    	return FAIL;
    }
    /*Judege whether it is NULL between <GetParameterAttributes> and  </GetParameterAttributes>*/
    if(method->child == NULL) {
        LOG(m_handler, DEBUG, "This GetParameterAttributes request have no arguments nodes, only have method node\n");
        return SUCCESS;
    }   
    /*Find <ParameterNames> node*/
    para_name = mxmlFindElement(method, xmltop, "ParameterNames", NULL, NULL, MXML_DESCEND);
    if(para_name == NULL) {
        LOG(m_handler, DEBUG,"Not find <ParameterNames> node in GetParameterAttributes request soap envelope\n");
        return SUCCESS;
    } else {
        LOG(m_handler, DEBUG, "Find <ParameterNames> node in GetParameterAttributes request soap envelope\n");
    }
    para_name_child = para_name->child;
    if(para_name_child == NULL) {
        LOG(m_handler, DEBUG, "The ParameterNames of GetParameterAttributes has no parameters\n");
        
        /*Judeg whether it is NULL between <ParameterNames> and </ParameterNames>*/
        para_list_node = gen_get_para_attr_list(method_node);
        if(para_list_node == NULL) {
            LOG(m_handler, ERROR, "Generate <ParameterList> node unsuccessfully\n");
            return FAIL;
        }
        gen_get_para_attr_list_attr(para_list_node, 0);
        return SUCCESS;
    }
    tmpnode = para_name_child;
    /*Calculate how many items in ParameterNames Array*/  
    for(; tmpnode != NULL; tmpnode = tmpnode->next) {
        struct_num ++;
    }
    LOG(m_handler, DEBUG, "struct_num = %d\n", struct_num);
    /*if(struct_num >= 10) { 
        mxmlDelete(method_node);
        res = gen_get_fault_code(soap_body, RESOURCE_EXCEED); //anna
        if (res == -1) {
            LOG(m_handler, ERROR, "Gen_fault_code failed.\n");
            return FAIL;
        }
        return SUCCESS;
    }*/
    para_list_node = gen_get_para_attr_list(method_node);
    if(para_list_node == NULL) {
        LOG(m_handler, ERROR, "Generate <ParameterList> node unsuccessfully\n");
        return FAIL;
    }
    for(i = 0; i < struct_num; i ++) {
    	if(para_name_child->child == NULL) {
    	    LOG(m_handler, DEBUG, "An empty string\n");
    	    strcpy(parameter_name, INTERNETGATEWAYDEVICE);
        } else {
            strcpy(parameter_name, para_name_child->child->value.opaque);
        }
        len = strlen(parameter_name);
        if(parameter_name[len-1] == '.') {  
            LOG(m_handler, DEBUG, "This partial path name is :%s\n", parameter_name);
            obj = (struct TR_object *)param_search(parameter_name);
            if(obj == NULL) {
                LOG(m_handler, DEBUG, "Not find parameter:%s\n", parameter_name);           
                mxmlDelete(method_node);
                res = gen_get_fault_code(soap_body, INVALID_PARAM_NAME);
 	        if (res == -1) {
	            LOG(m_handler, DEBUG, "Generate parameter name fault code failed.\n");
	            return FAIL;
	        }
                return SUCCESS;              
            } else {  //obj != NULL
                LOG(m_handler, DEBUG, "Find  out parameter:%s\n", parameter_name);

                tmp_param = obj->param;
                while(tmp_param != NULL) {
                    //Generate <ParameterAttributeStruct>...</ParameterAttributeStruct> 
                    strcpy(param_attr.name, parameter_name);
                    strcat(param_attr.name, tmp_param->name);
                    param_attr.notification = tmp_param->notification;
                    strcpy(param_attr.accesslist, tmp_param->accesslist);

                    attr_struct_node = gen_para_attr_struct(para_list_node, &param_attr);
                    if(attr_struct_node == NULL) {
                        LOG(m_handler, ERROR, "Generate <ParameterAttributeStrcut>...</ParameterAttributeStruct> unsuccessfully\n");
                        return FAIL;
                    }
                    para_attr_count++;
                    tmp_param=tmp_param->next;
                }           
                res = get_partialpath_attri(para_list_node, obj->next_layer, parameter_name, para_attr_count);
                if (res == -1) {
                    mxmlDelete(method_node);
                    res = gen_get_fault_code(soap_body, INVALID_PARAM_NAME);
                    if (res == -1) {
                        LOG(m_handler, DEBUG, "Generate parameter name fault code failed.\n");
                        return FAIL;
                    }
                }
                para_attr_count = res;
             }
         } else {
             LOG(m_handler, DEBUG, "This is a full parameter name\n");
             strcpy(param_attr.name, parameter_name);
             LOG(m_handler, DEBUG, "The full parameter name : %s\n", param_attr.name);
                
             tr_param = (struct TR_param *)param_search(param_attr.name);
             if(tr_param == NULL) {   
                 LOG(m_handler, DEBUG, "Can't find parameter:%s\n", param_attr.name);
		 mxmlDelete(method_node);
                 res = gen_get_fault_code(soap_body, INVALID_PARAM_NAME);
		 if (res == -1) {
		     LOG(m_handler, DEBUG, "Generate parameter name fault code failed.\n");
		     return FAIL;
		 }       
                 return SUCCESS;            
             } else { //tr_param != NULL
                 LOG(m_handler, DEBUG, "Find out parameter:%s\n", param_attr.name);
                 /*Generate <ParameterAttributeStruct>...</ParameterAttributeStruct>*/
                 param_attr.notification = tr_param->notification;
                 strcpy(param_attr.accesslist, tr_param->accesslist);

                 attr_struct_node = gen_para_attr_struct(para_list_node, &param_attr);
                 if(attr_struct_node == NULL) {
                     LOG(m_handler, ERROR, "Generate <ParameterAttributeStrcut>...</ParameterAttributeStruct> unsuccessfully\n");                            
                     return FAIL;
                 }                                   
                 para_attr_count++;
             }//end else
         }
         para_name_child = para_name_child->next;
    }//end for
    /* Generate the attirubute and attribute value of <ParameterList> node */
    gen_get_para_attr_list_attr(para_list_node, para_attr_count);
    return SUCCESS;
}
/*
 **************************************************************************
 * Function name: init_attr()
 * Description:Initialize attri.conf
 * Parameter: void
 * Return Value: SUCCESS FAIL
 *************************************************************************
 */ 
int init_attr()
{
/*    TR_param_attr init_array[ATTRI_CONF_INITSIZE];
    int res;

    strcpy(init_array[0].name, DEVICEINFOSOFTWAREVERSION);
    init_array[0].notification = 2;
    strcpy(init_array[0].accesslist, "Subscriber");
    
    strcpy(init_array[1].name, DEVICEINFOPROVISIONINGCODE);
    init_array[1].notification = 2;
    strcpy(init_array[1].accesslist, "Subscriber");
    
    strcpy(init_array[2].name, MANAGEMENTSERVERCONREQURL);
    init_array[2].notification = 2;
    strcpy(init_array[2].accesslist, "Subscriber");
    
    res = dev_dll_func(wan_para, wan_dev_func);
    if (res != 0) {
        LOG(m_event, ERROR, "Invoke del_dll_func failed.\n");
        return FAIL;
    }

    strcpy(init_array[3].name, wan_para);
    init_array[3].notification = 2;
    strcpy(init_array[3].accesslist, "Subscriber");
  */  
   /* strcpy(init_array[4].name, WANDEVICEPPPEXTERNALADDRESS);
    init_array[4].notification = 2;

    strcpy(init_array[4].accesslist, "Subscriber");
     */ 
    /*get default wan connection ; */
 
    //int i;
    //int init_size = ATTRI_CONF_INITSIZE;
    
    FILE *fp = NULL;
    int init_size = 0;
    /* Initialize attri.conf */
    if(access(attri_conf_path, F_OK) == -1) {
        LOG(m_handler, DEBUG, "The configure file %s doesn't exit, create it\n", attri_conf_path);
        
        if((fp = fopen(attri_conf_path, "wb+")) == NULL){
            LOG(m_handler, ERROR, "Can't initialize attri.conf file\n");
            return FAIL;
        }
        if(fwrite(&init_size, sizeof(int), 1, fp) != 1){
            LOG(m_handler, ERROR, "Can't write INITSIZE into attri.conf file\n");
            fclose(fp);
            return FAIL;
        }
    /*    for(i = 0; i < init_size; i++) {
            if(fwrite(&init_array[i], sizeof(TR_param_attr), 1 ,fp) != 1) {
                LOG(m_handler, ERROR, "Can't write initial parameters and attributes into attri.conf\n");
                fclose(fp);
                return FAIL;
            }
        }*/
        fclose(fp);
        return SUCCESS;
    } else {
        LOG(m_handler, DEBUG, "The file: %s has been created.\n", attri_conf_path);
        return SUCCESS;
    }
}
