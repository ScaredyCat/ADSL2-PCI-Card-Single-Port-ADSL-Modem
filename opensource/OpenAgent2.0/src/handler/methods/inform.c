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
 
/***********************************************************************
*
*	include file
*
***********************************************************************/
  
#include "inform.h"

/***********************************************************************
*
*	function prototype
*
***********************************************************************/
static void modify_list();
static int gen_inform_method(TR_inform *info, TRF_node *xmlroot);
static int get_maxenvelopes(TRF_node *method);
static int gen_anytype(TRF_node *val, TR_para_val_struct *pvs);
static int get_para_val(int i, TR_inform *info);
static int get_deviceid_struct(TR_dev_id_struct *deviceid);
static int dev_dll_func(char *buf, char dev_fun_name[]);//one parameter, char[]
static int process_inform(TR_inform *info); 


/*
************************************************************************
* Function name: modify_list
* Description: Modify the event_list and changed_param_name_list after receive the inform response successfully
* Parameter: void
* Return value: void
* 
***********************************************************************
*/
void modify_list()
{
    /* Modify event_list, delete the node sent except periodic node */
    modify_event_list ();
    LOG(m_handler, DEBUG, "Invoke modify_event_list successful.\n");

    /* Modify changed_param_name_list, delete all nodes */
    flush_changed_param_list ();
    LOG(m_handler, DEBUG, "Invoke flush_changed_param_list successful.\n");
}

/*
***************************************************************************
* Function name: gen_devicdid() 
* Description : Generate device id node
* Parameter: DeviceId 
*        method
* return value: 
*    success return SUCCESS, else return FAIL
***************************************************************************
*/
 
int gen_deviceid(TR_dev_id_struct *deviceid, TRF_node *method)
{
    TRF_node *devid = NULL, *mf = NULL, *oui = NULL, *pc = NULL, *sn = NULL;

    /*generate deviceid*/
    devid = mxmlNewElement (method, "DeviceId");
    if (!devid) 
        return FAIL;
   
    mf = mxmlNewElement(devid, "Manufacturer");
    if (!mf) 
        return FAIL;

    if (mxmlNewText (mf, 0, deviceid->manufacturer) == NULL)
        return FAIL;

    oui = mxmlNewElement (devid, "OUI");
    if (!oui)
        return FAIL;
   
    if (mxmlNewText (oui, 0, deviceid->oui) == NULL) 
        return FAIL;
  
    pc = mxmlNewElement (devid, "ProductClass");
    if (!pc)
        return FAIL;

    if (mxmlNewText(pc, 0, deviceid->product_class) == NULL) {
        return FAIL;
    }

    sn = mxmlNewElement (devid, "SerialNumber");
    if (!sn) {
        return FAIL;
    }
    if (mxmlNewText (sn, 0, deviceid->serial_number) == NULL) 
        return FAIL;
   
    return SUCCESS;
}

/*
**************************************************************************
* Function name: gen_event()
* Description: generate event node
* Parameter: Event
*            method
* Return Value:
*    success return SUCCESS, else return FAIL
**************************************************************************
*/
 
int gen_event (TR_arr_event_struct *event, TRF_node *method)
{
    TRF_node *evn = NULL, *es = NULL, *ec = NULL, *ck = NULL;
    char arraytype[EVENT_LEN];
    int i = 0; 

    sprintf (arraytype, "cwmp:EventStruct[%d]", event->_size);
    
    evn = mxmlNewElement(method, "Event");
    if (!evn) 
        return FAIL;
    
    mxmlElementSetAttr(evn, "xsi:type", "SOAP-ENC:Array");
    mxmlElementSetAttr(evn, "SOAP-ENC:arrayType", arraytype);

    /*This for loop to add event node*/
    for (i = 0; i < event->_size; i++) {
        es = mxmlNewElement(evn, "EventStruct");
        if (!es) 
            return FAIL;
    
        ec = mxmlNewElement(es, "EventCode");
        if (!ec) 
            return FAIL;
      
        if (mxmlNewText(ec, 0, event->_ptr[i].event_code) == NULL) 
            return FAIL;
     
        ck = mxmlNewElement(es, "CommandKey");
        if (!ck) 
            return FAIL;

        if (mxmlNewText(ck, 0, event->_ptr[i].command_key) == NULL)
            return FAIL;
     
    }
   
    return SUCCESS;
}

/*
*******************************************************************
* Function name: gen_agentmaxenv()
* Description: Generate max envelope node
* Parameter: CPEMaxEnvelopes 
* Return Value:
*    success return SUCCESS, else return FAIL
******************************************************************
 */
 
int gen_agentmaxenv(int CPEMaxEnvelopes, TRF_node *method)
{
    TRF_node *maxenv = NULL;

    maxenv = mxmlNewElement(method, "MaxEnvelopes");
    if (!maxenv) 
        return FAIL;
   
    if (mxmlNewInteger(maxenv, CPEMaxEnvelopes) == NULL) 
        return FAIL;
   
    return SUCCESS;
}

/*
******************************************************************
* Function name: gen_cur_time()
* Description: generate current time node
* Parameter: method
* Return Value:
*    success return SUCCESS, else return FAIL
******************************************************************
*/
 
int gen_cur_time(TRF_node *method)
{
    TRF_node *ct = NULL;
    char timebuf[TIMEBUF_LEN];
    time_t tp;
    
    /*get current time*/
    time(&tp);
    format_time (tp, timebuf);
    
    ct = mxmlNewElement (method, "CurrentTime");
    if (!ct) 
        return FAIL;
   
    if (mxmlNewText (ct, 0, timebuf) == NULL)
        return FAIL;

    return SUCCESS;
}

/*
*******************************************************************
* Function name: gen_retry_count()
* Description: generate retry count node
* Parameter: RetryCount
*        method
* Return Value:
*     success return SUCCESS, else return FAIL
*******************************************************************
*/
 
int gen_retry_count(int retrycount, TRF_node *method)
{
    TRF_node *rc = NULL;

    rc = mxmlNewElement(method, "RetryCount");
    if (!rc) 
        return FAIL;
 
    if (mxmlNewInteger(rc, retrycount) == NULL)
        return FAIL;
 
    return SUCCESS;
}

/*
*******************************************************************
* Function name: gen_para_list()
* Description: generate parameter list node
* Parameter: ParaList
*        method
* Return Value:
*     success return SUCCESS, else return FAIL
*******************************************************************
*/
  
int gen_para_list(TR_arr_para_val_struct *paralist, TRF_node *method)
{
    TRF_node *pl = NULL, *pvs = NULL, *name = NULL, *val = NULL;
    char arraytype[PARAM_LEN];
    int i = 0, res = 0;

    sprintf(arraytype, "cwmp:ParameterValueStruct[%d]", paralist->_size);
    
    pl = mxmlNewElement(method, "ParameterList");
    if (!pl) 
        return FAIL;
    
    mxmlElementSetAttr(pl, "xsi:type", "SOAP-ENC:Array");
    mxmlElementSetAttr(pl, "SOAP-ENC:arrayType", arraytype);

    for (i = 0; i < paralist->_size; i++) {
        pvs = mxmlNewElement(pl, "ParameterValueStruct");
        if (!pvs) 
            return FAIL;
      
        name = mxmlNewElement(pvs, "Name");
        if (!name) 
          return FAIL;
     
        LOG(m_handler, DEBUG, "Paralist name is : %s\n", paralist->_ptr[i].name);
        if (mxmlNewText(name, 0, paralist->_ptr[i].name) == NULL) 
            return FAIL;
      
        val = mxmlNewElement(pvs, "Value");
        if (!val) 
            return FAIL;
   
        res = gen_anytype(val, &paralist->_ptr[i]);
        if (res != 0)
            return FAIL;
      
    }
    return SUCCESS;
}

/*
*********************************************************************
* Function name: gen_inform_argument()
* Description: generate infor argument
* Parameter: info
*            method
* Return Value:
*     success return SUCCESS, else return FAIL
*********************************************************************
*/
 
int gen_inform_argument(TR_inform *info, TRF_node *method)
{
    int res = 0;
    
    res = gen_deviceid(&info->device_id, method);
    if (res != 0) {
        LOG (m_handler, ERROR, "generate deviceid failed\n");
        return FAIL;
    }
    
    res = gen_event(&info->event, method);
    if (res != 0) {
        LOG (m_handler, DEBUG, "generate event failed\n");
        return FAIL;
    }
        
    res = gen_agentmaxenv(info->max_envelopes, method);
    if (res != 0) {
        LOG(m_handler, ERROR, "generate agent max envelope failed\n");
        return FAIL;
    }
        
    res = gen_cur_time(method);
    if (res != 0) {
        LOG (m_handler, ERROR, "generate current time failed\n");
        return FAIL;
    }
        
    res = gen_retry_count(info->retry_count, method);
    if (res != 0) {
        LOG (m_handler, ERROR, "generate retry count failed\n");
        return FAIL;
    }
    
    res = gen_para_list(&info->para_list, method);
    if (res != 0) {
        LOG (m_handler, ERROR, "generate parameter list failed\n");
        return FAIL;
    }
    return SUCCESS;

}

/*
*******************************************************************
* Function name: gen_inform_method()
* Description: generate inform method
* Parameter: info
*            xmlroot
* Return Value:
*     success reutrn 0, else reutrn -1
*******************************************************************
*/
 
int gen_inform_method(TR_inform *info, TRF_node *xmlroot)
{
    char name[] = "Inform";
    TRF_node *method = NULL;
    int res;

   // LOG(m_handler, DEBUG, "start generate inform method\n");
    have_soap_head = 1;
    strcpy(header_id_val, INFORM_ID);

    method = gen_soap_frame(name);
    if (method == NULL) {
        LOG(m_handler, ERROR, "Generate soap frame fail.\n");
        return FAIL;
    }

    res = gen_inform_argument(info, method);
    if (res != 0)
        return FAIL;
    
    //LOG(m_handler, DEBUG, "generate inform method success\n");
    
    return SUCCESS;
}

/*
***************************************************************************
* Function name: get_maxenvlopes()
* Description:  parse response get acs max envelopes
* Parameter: method
* Return Value:
*    success return SUCCESS, else return FAIL
***************************************************************************
*/
 
int get_maxenvelopes(TRF_node *method)
{	
    /*init maxenvelope*/
    ACS_MAX_ENVELOPES = 0;
    
    /*handler header*/
    if (strcmp(header_id_val, INFORM_ID)) {
        LOG(m_handler, ERROR, "ID value is error in inform response\n");
        return FAIL;
    }
    have_soap_head = 0;
    strcpy(header_id_val, "");
    
    if (!method) 
        return FAIL;
  
    method = method->child;
    
    /*This while loop to find maxenvelope node*/
    while (method->value.element.name != NULL) {
        if (!strcmp(method->value.element.name, "MaxEnvelopes")) {
            if (method->child != NULL) {
                ACS_MAX_ENVELOPES = atoi(method->child->value.opaque);
                break;
            }
            else {
                LOG (m_handler, ERROR, "No maxenvelopes in informresponse.\n"); 
                return FAIL;
            } //end modify      
        }
        method = method->child;
    }
    if (ACS_MAX_ENVELOPES == 0) {
        ACS_MAX_ENVELOPES = 1;
        return SUCCESS;
    }
    if (ACS_MAX_ENVELOPES < 0) {
        LOG(m_handler, ERROR, "ACS_MAX_ENVELOPES is : %d\n", ACS_MAX_ENVELOPES);
        return FAIL;
    }
    
    return SUCCESS;
}

/*
****************************************************************************
* Function: process_inform_resp()
* Decription: process the data of inform response from ACS
* Parameter : TRF_node *method
* Return value:
*     success return SUCCESS, else return FAIL
****************************************************************************
*/
int process_inform_resp(TRF_node *method)
{   
    /*Get the acs max envelopes*/
    if (get_maxenvelopes(method) != 0) {
        LOG (m_handler, ERROR, "process inform response failed\n");
        return FAIL;
    }
    LOG (m_handler, DEBUG, " ACS_MAX_ENVELOPES : %d\n", ACS_MAX_ENVELOPES);

    /*Change inform flag (successful)*/
    inform_flag = 1;
    inform_retry = 0;
    /*modify event list*/
    modify_list ();
    return SUCCESS;
}

/*
***************************************************************************
* Function name: gen_anytype()
* Description: gen data type in xml data package
* Parameter: TRF_node *val: data type attr and value add to it
*            TR_para_val_struct *pvs : which contain datatype and value
* Return Value:
*        success return SUCCESS, else return FAIL
**************************************************************************
*/
int gen_anytype(TRF_node *val, TR_para_val_struct *pvs)
{
    char timebuf[TIMEBUF_LEN];
    
    switch(pvs->type) {   
        
        case 's':    // s -- the data type of string
            mxmlElementSetAttr(val, "xsi:type", "xsd:string");
            if (mxmlNewText(val, 0, pvs->value.chararg) == NULL) {
               LOG(m_handler, DEBUG, "Paralist value is :%s\n", pvs->value.chararg);
               return FAIL;
            }
            break;
        
        case 'i':   // i -- the data type of int
            mxmlElementSetAttr(val, "xsi:type", "xsd:int");
            if (mxmlNewInteger(val, pvs->value.intarg) == NULL) {
		LOG(m_handler, DEBUG, "Paralist value is :%d\n", pvs->value.intarg);
                return FAIL;
            }
            break;
        
        case 'u':   // u -- the data type of unsignedint
            mxmlElementSetAttr(val, "xsi:type", "xsd:unsignedInt");
            if (mxmlNewInteger(val, pvs->value.uintarg) == NULL) {
		LOG(m_handler, DEBUG, "Paralist value is :%u\n", pvs->value.uintarg);
                return FAIL;
            }
            break;
        
        case 'd':   // d -- the data type of datetime--
            format_time(pvs->value.timearg, timebuf);
            mxmlElementSetAttr(val, "xsi:type", "xsd:dateTime");
            if (mxmlNewText(val, 0, timebuf) == NULL) {
		LOG(m_handler, DEBUG, "Paralist value is :%s\n", timebuf);
                return FAIL;
            }
            break;
        
        case 'b':    // b -- the data type of boolean--
            mxmlElementSetAttr(val, "xsi:type", "xsd:boolean");
            if (mxmlNewInteger(val, pvs->value.boolean) == NULL) {
		LOG(m_handler, DEBUG, "Paralist value is :%d\n", pvs->value.boolean);
                return FAIL;
            }
            break;
        default :
            LOG(m_handler, ERROR, "Don't support this Data type: %s\n", pvs->type);
            return FAIL;
    }
    return SUCCESS;
}
/*
*************************************************************************************
* Function: get_deviceid_struct()
* Description: get value of deviceid
* Parameter: TR_inform *info
* Return value:
*     Success return SUCCESS, else return FAIL
*************************************************************************************
 */
int get_deviceid_struct(TR_dev_id_struct *deviceid) 
{
   int res;
   int locate[4];

   res = call_dev_func(manu_dev_func, GET_OPT, deviceid->manufacturer, locate);
   if (res != 0) {
       LOG(m_handler, ERROR, "Invoke dev_func failed.\n");
       return -1;
   }
   res = call_dev_func(oui_dev_func, GET_OPT, deviceid->oui, locate);
   if (res != 0) {
       LOG(m_handler, ERROR, "Invoke dev_func failed.\n");
       return -1;
   }
   res = call_dev_func(class_dev_func, GET_OPT, deviceid->product_class, locate);
   if (res != 0) {
       LOG(m_handler, ERROR, "Invoke dev_func failed.\n");
       return -1;
   }
   res = call_dev_func(serial_dev_func, GET_OPT, deviceid->serial_number, locate);
   if (res != 0) {
       LOG(m_handler, ERROR, "Invoke dev_func failed.\n");
       return -1;
   }
   return SUCCESS;
}
     

/*
*************************************************************************************
* Function: init_inform()
* Description: init inform data
* Parameter: void
* Return value:
*     Success return SUCCESS, else return FAIL
*************************************************************************************
 */
int init_inform()
{
    int res;
    TR_inform info;

    res = process_inform(&info);
    if (res != 0) {
        if (gen_method_fault(xmlroot, "Inform", INTERNAL_ERROR) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    LOG(m_handler, DEBUG, "Invoke process_inform success!\n");
    res = gen_inform_method(&info, xmlroot);
    if (res != 0) {
        LOG(m_handler, ERROR, "Generate inform method failed\n");
        free(info.event._ptr);
        free(info.para_list._ptr);
        return FAIL;
    }
    free(info.event._ptr);
    free(info.para_list._ptr);
    LOG(m_handler, DEBUG, "Generate Inform method Success.\n");
    return SUCCESS;
}
int process_inform(TR_inform *info)
{
    int i = 0, j = 0, res = 0;
    int event_size = 0;         //The number of event_list nodes to be sent
    int param_size = 0;         //The number of changed_param_name_list nodes to be sent
   
    char wan_para[WAN_PARAM_LEN]; 

    struct event_list *event_list_node;
    struct changed_param_name_list *param_list_node;

    res = get_deviceid_struct(&info->device_id); 
    if (res != 0) {
        LOG(m_handler, ERROR, "Invoke get_deviceid_struct failed.\n");
        return FAIL;
    }
    info->max_envelopes = CPE_MAX_ENVELOPES;
    info->retry_count = inform_retry;
    
    /* read event_list and add to info.event */

    pthread_mutex_lock(&event_list_lock);

    list_for_each_entry(event_list_node, &event_list_head, node) {
       if (strcmp(event_list_node->event_code, "M RETRY CONNECT") != 0) {
           if (strcmp(event_list_node->event_code, "2 PERIODIC") == 0) {
                LOG(m_handler, DEBUG, "event_list_node->sent_flag:%d\n", event_list_node->sent_flag);
                if (event_list_node->sent_flag == 1)
                    event_size++;
            } else {
                event_size++;
            }
        }
    }
    info->event._size = event_size;
    info->event._ptr = (TR_event_struct *)malloc(event_size*sizeof(TR_event_struct));
    if (info->event._ptr == NULL) {
        LOG (m_handler, ERROR, "Malloc memory for info.event._ptr failed!\n");
        pthread_mutex_unlock(&event_list_lock);
        return FAIL;
    }
    i = 0;
    
    list_for_each_entry(event_list_node, &event_list_head, node) {
        if (strcmp(event_list_node->event_code, "M RETRY CONNECT") != 0) {
            if (strcmp(event_list_node->event_code, "2 PERIODIC") == 0) {
                if (event_list_node->sent_flag == 1) {
                    strcpy(info->event._ptr[i].command_key, event_list_node->cmd_key);
                    strcpy(info->event._ptr[i].event_code, event_list_node->event_code);
                //event_list_node->sent_flag == 0;
                    i++;
                }
            
            } else {
                strcpy(info->event._ptr[i].command_key, event_list_node->cmd_key);
                strcpy(info->event._ptr[i].event_code, event_list_node->event_code);
	        event_list_node->sent_flag = 1;
                i++;
            }
        } else {
            event_list_node->sent_flag = 1;
        }
    }

    pthread_mutex_unlock(&event_list_lock);
     
    /*get wan parameter 7; */
    res = dev_dll_func(wan_para, wan_dev_func);
    if (res != 0) {
        LOG(m_event, ERROR, "Invoke del_dll_func failed.\n");
        free(info->event._ptr);
        return FAIL;
    }
    
    /* read changed_param_name_list and add to info.para_list*/
    pthread_mutex_lock(&changed_param_name_list_lock);

    if (!list_empty(&changed_param_name_list_head)) {
        list_for_each_entry(param_list_node, &changed_param_name_list_head, node) {

                if((strcmp(param_list_node->name, SPECVERSION) != 0)
                && (strcmp(param_list_node->name, HARDWAREVERSION) != 0)	 
                && (strcmp(param_list_node->name, SOFTWAREVERSION) != 0)	 
                && (strcmp(param_list_node->name, PROVISIONINGCODE) != 0)	 
                && (strcmp(param_list_node->name, CONNECTIONREQUESTURL) != 0)	 
                && (strcmp(param_list_node->name, PARAMTERKEY) != 0)	 
                && (strcmp(param_list_node->name, wan_para) != 0)) {
                param_size++;
            }
        }
    }
    pthread_mutex_unlock(&changed_param_name_list_lock);
    LOG (m_handler, DEBUG, "param_size = %d\n", param_size);
    info->para_list._size = param_size + 7;
    info->para_list._ptr = (TR_para_val_struct *)malloc((param_size + 7)*sizeof(TR_para_val_struct));
    if (info->para_list._ptr == NULL) {
        LOG (m_handler, ERROR, "Malloc memory for info.para_list._ptr failed!\n");
        free(info->event._ptr);
        return FAIL;
    }
    LOG (m_handler, DEBUG, "Malloc memory for info.para_list._ptr success!\n");
    //7 parameters
    strcpy(info->para_list._ptr[0].name, SPECVERSION);
    strcpy(info->para_list._ptr[1].name, HARDWAREVERSION); 
    strcpy(info->para_list._ptr[2].name, SOFTWAREVERSION); 
    strcpy(info->para_list._ptr[3].name, PROVISIONINGCODE); 
    strcpy(info->para_list._ptr[4].name, CONNECTIONREQUESTURL); 
    strcpy(info->para_list._ptr[5].name, PARAMTERKEY); 
    strcpy(info->para_list._ptr[6].name, wan_para); 

/*First add the 7 required inform parameters to para_list*/
    for(i = 0; i < 7; i++) {
        if(get_para_val(i, info) != 0) {
            LOG(m_handler, ERROR, "Get_parameter's type and value failed! i=%d\n", i);
            free(info->event._ptr);
            free(info->para_list._ptr);
            return FAIL;
        }
        LOG(m_handler, DEBUG, "Get_parameter's type and value success! i=%d\n", i);
    }

/*Then Add the nodes in changed_param_name_list and add to info*/

    if (param_size > 0) {
        i = 0;
        pthread_mutex_lock(&changed_param_name_list_lock);
        list_for_each_entry(param_list_node, &changed_param_name_list_head, node) {
          //  LOG(m_handler, DEBUG, "param_list_node->name:%s\n", param_list_node->name);
            j = 0;
            while( j <= 6 && (strcmp(param_list_node->name, info->para_list._ptr[j].name) != 0)){
                LOG(m_handler, DEBUG, "Not same with %d parameter:%s.\n", j, &info->para_list._ptr[j].name);
                j++;
            }
            if (j >= 7) {
                strcpy(info->para_list._ptr[i + 7].name, param_list_node->name);
                if (get_para_val(i+7, info) != 0) {
                   LOG(m_handler, ERROR, "Get_parameter's type and value failed!\n");
                   free(info->event._ptr);
                   free(info->para_list._ptr);
                   pthread_mutex_unlock(&changed_param_name_list_lock);
                   return FAIL;
                }
                i++;
           }
        }
        pthread_mutex_unlock(&changed_param_name_list_lock);
    }//end if(param_size > 0)

    return SUCCESS;
}
/*
*************************************************************************************
* Function: get_para_val()
* Description: 
* Parameter: int i, TR_inform *info
* Return value:
*     Success return SUCCESS, else return FAIL
*************************************************************************************
 */
int get_para_val(int i, TR_inform *info)
{
    int res;
    struct TR_param *param_pointer;
 
    param_pointer = (struct TR_param *)param_search(info->para_list._ptr[i].name);  //get pointer of the param
    if (param_pointer == NULL) {
        LOG(m_handler, ERROR, "Do not find match parameter\n");
        LOG(m_handler, DEBUG, "name = %s\n", info->para_list._ptr[i].name);
        return FAIL;
    } else {
        //get para type 
        info->para_list._ptr[i].type = param_pointer->param_type;
        res = call_dev_func (param_pointer->dev_func, GET_OPT, info->para_list._ptr[i].value.chararg, param_pointer->parent->locate);
        if (res == GET_VAL_FAILED) {
           LOG(m_handler, ERROR, "Get parameter value from dev_func failed.\n");
           return FAIL;
        }
    }
    return SUCCESS;
}
/*
***********************************************************************
* Function name: dev_dll_unc
* Description: call a function of dll
* Parameter:  dev_func_name, buf
* Return value: SUCCESS FAIL
***********************************************************************
*/
int dev_dll_func(char *buf, char dev_func_name[])
{
    int res;
    void *handle;
    int (*func)(char*);
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
    res = (*func)(buf);
    if (res == 0) {
        LOG(m_event, DEBUG, "Call dll device func success.\n");
        dlclose(handle);
        return SUCCESS;
    } 
    dlclose(handle);
    LOG(m_event, ERROR, "Call dll device func failed.\n");
    return FAIL;
}
