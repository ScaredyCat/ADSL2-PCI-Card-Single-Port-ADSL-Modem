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

#include "soap.h"


/*
 * Initialize faultstruct array
 */

TR_fault_struct faultstruct[MAX_FAULT_SIZE] = {{9000, MSG9000},
	                                              {9001, MSG9001},
                                                      {9002, MSG9002},
                                                      {9003, MSG9003},
                                                      {9004, MSG9004},
                                                      {9005, MSG9005},
                                                      {9006, MSG9006},
                                                      {9007, MSG9007},
                                                      {9008, MSG9008},
                                                      {9009, MSG9009},
                                                      {9010, MSG9010},
                                                      {9011, MSG9011},
                                                      {9012, MSG9012},
                                                      {9013, MSG9013}, 
                                                      {9800, MSG9800},
			                              {9801, MSG9801},
			                              {9802, MSG9802},
			                              {9803, MSG9803}}; 

static inline unsigned long ewt_mktime(struct tm *tm);
extern char *strptime (__const char *__restrict __s,
                       __const char *__restrict __fmt, struct tm *__tp)
     __THROW;

/*
 ****************************************************************
 * Function: gen_xml_header()
 * Description: Generate a root node for xml
 * Return value:
 *     success return xmlroot node, else return NULL
 ****************************************************************
 */

TRF_node *gen_xml_header()
{   
    //generate xmlroot node
    xmlroot = mxmlNewElement(MXML_NO_PARENT, "?xml");
    if (!xmlroot) {
        LOG (m_handler, ERROR, "gen xml header failed\n");
        return NULL;
    }
    
    //Add attr to xmlroot
    mxmlElementSetAttr(xmlroot, "version", "1.0");
    mxmlElementSetAttr(xmlroot, "encoding", "UTF-8");
    
    return xmlroot;
}

/*
 **************************************************************
 * Function: gen_env()
 * Description: generate soap envelope
 * Param: TRF_node *xmlroot point to xml root node 
 * Return Value: 
 *     success return ENV node, else return NULL
 **************************************************************
 */

TRF_node *gen_env(TRF_node *xmlroot)
{
    TRF_node *ENV;
    
    //generate envelope node
    ENV = mxmlNewElement(xmlroot, "SOAP-ENV:Envelope");
    if (!ENV) {
        LOG (m_handler, ERROR, "gen xml envenvelope failed");
        return NULL;
    }
    
    //Add attr to envelope node
    mxmlElementSetAttr(ENV, "xmlns:SOAP-ENV", ENVURI);
    mxmlElementSetAttr(ENV, "xmlns:SOAP-ENC", ENCURI);
    mxmlElementSetAttr(ENV, "xmlns:xsi", XSIURI);
    mxmlElementSetAttr(ENV, "xmlns:xsd", XSDURI);
    mxmlElementSetAttr(ENV, "xmlns:cwmp", CWMPURN);
    
    return ENV;
}

/*
 ***************************************************************
 * Function: gen_soap_header()
 * Description: generate soap header
 * Param: TRF_node *ENV point to SOAP-ENV node
 * Return Value: 
 *     success return SOAP Header node, else return NULL
 ***************************************************************
 */
 
TRF_node *gen_soap_header(TRF_node *ENV)
{
    TRF_node *HEADER;
    TRF_node *ID, *NOMOREREQ;
    
    //check need to add soap head
    if (have_soap_head == 0) {
    	return ENV;
    }
    
    //generate header node
    HEADER = mxmlNewElement(ENV, "SOAP-ENV:Header");
    if (!HEADER) {
        LOG (m_handler, ERROR, "Gen soap header failed\n");
        return NULL;
    }
    
    //Add header ID value
    ID = mxmlNewElement(HEADER, "cwmp:ID");
    if (!ID) {
        LOG (m_handler, ERROR, "Gen ID node failed\n");
        return NULL;
    }
    mxmlElementSetAttr(ID, "SOAP-ENV:mustUnderstand", "1");
    if (mxmlNewText(ID, 0, header_id_val) == NULL) {
        LOG (m_handler, ERROR, "Gen ID value failed\n");
        return NULL;
    }
   
    //Add header no more request value
    if (cpe_no_more_req_val != 0) {
        NOMOREREQ = mxmlNewElement(HEADER, "cwmp:NoMoreRequests");
        if (!NOMOREREQ){
            LOG (m_handler, ERROR, "Gen No more request node failed\n");
            return NULL;
        }
        if (!mxmlNewInteger(NOMOREREQ, cpe_no_more_req_val)){
            LOG (m_handler, ERROR, "gen No more request value falied\n");
            return NULL;
        }
        
    }
    
    return HEADER;
}

/*
 ********************************************************************
 * Function: gen_soap_body()
 * Description: generate soap body
 * Param: TRF_node *ENV  point to soap ENV node
 * Return Value: 
 *     success return soap_body node, else return NULL
 ********************************************************************
 */

TRF_node *gen_soap_body(TRF_node *ENV)
{
    TRF_node *soap_body;
    
    //generate body node
    soap_body = mxmlNewElement(ENV, "SOAP-ENV:Body");
    if (!soap_body){
        LOG(m_handler, ERROR, "Gen soap body node failed\n");
        return NULL;
    }
    
    //Add attr to body node
    mxmlElementSetAttr(soap_body, "SOAP-ENV:encodingStyle", ENSTY);
    
    return soap_body;
}

/*
 ***************************************************************************************
 * Function name: gen_method_name()
 * Description: generate RPC method name
 * Param: char *name point to method name, TRF_node *soap_body point to soap_body node 
 * Return Value: 
 *     success return method node, else return NULL
 ***************************************************************************************
 */

TRF_node *gen_method_name(char *name, TRF_node *soap_body)
{
    TRF_node *method;
    char methodval[72] = "cwmp:";
    
    strcat(methodval, name);
    
    //generate method node
    method = mxmlNewElement(soap_body, methodval);
    if (!method) {
        LOG (m_handler, ERROR, "gen method name node failed\n");
        return NULL;
    }

    return method;
}

/*
 *************************************************************************
 * Function name: gen_soap_frame
 * Description: Gen soap framework for tr069 request or response
 * Param:
 *       char *method_name - the RPC method name of TR069
 * Return value:
 *       success return method node pointer, else return NULL
 *************************************************************************
 */
TRF_node *gen_soap_frame(char *method_name)
{
    TRF_node *env = NULL, *soap_header = NULL, *soap_body = NULL, *method = NULL;
                                                                                                                             
    //gen env node
    env = gen_env (xmlroot);
    if (!env) {
        return NULL;
    }
                                                                                                                             
    //gen soap header node
    soap_header = gen_soap_header (env);
    if (!soap_header) {
        return NULL;
    }
    
    //gen soap body node
    soap_body = gen_soap_body (env);
    if (!soap_body) {
        return NULL;
    }
    
    //gen method node
    method = gen_method_name (method_name, soap_body);
    if (!method) {
        return NULL;
    }
    
    LOG(m_handler, DEBUG, "Generate soap frame success\n");

    return method;
}

/*
 *************************************************************************
 * Function name: fault_search()
 * Description: Find fault code
 * param:
 *       array[]: the array of struct faultcodestruct
 *       n:       the size of array
 *       value:   the value you want to find
 * Return value:  the location of value in array[]
 ************************************************************************
 */
int fault_search(TR_fault_struct array[], int n, int value)
{
    int i,j;
    int find = 0;
    
    if ((0 <= (i = (value - 9000)))&&((i =(value - 9000)) <= 13)) {
        find = 1;
    }
    if ((0 <= (j = (value - 9800)))&&((j = (value - 9800)) <= 3)) {
        find = 1;
        i = j+14;
    }
    if (!find) {
        i = -1;
        LOG(m_handler, DEBUG, "Not find the value you want\n");
    }
    return (i);
}

/*
 **************************************************************************
 * Function name : gen_soap_fault()
 * Descrition Generate soap fault items :faultcode, faultstring, detail
 * Param:
 *       soap_body:  point to the <soap-ENV:body> node
 * Return Value:  pointer that point to <detail> node , else return NULL
 **************************************************************************
 */
TRF_node * gen_soap_fault(TRF_node *soap_body)
{
    TRF_node *soapfault, *fcode, *fstring, *deta;
    
    //Generate <SOAP-ENV:Fault> node
    soapfault = mxmlNewElement(soap_body, "SOAP-ENV:Fault");
    
    if (soapfault == NULL) {
        return NULL;
    }
    
    //Generate <faultcode> node
    fcode = mxmlNewElement(soapfault, "faultcode");
    
    if (fcode == NULL) {
        return NULL;
    }
    
    if (mxmlNewText(fcode, 0, SOAP_FAULT_CODE) == NULL) {
        return NULL;
    }
    
    //Generate <faultstring> node
    fstring = mxmlNewElement(soapfault, "faultstring");
    
    if (fstring == NULL) {
        return NULL;
    }
    
    if (mxmlNewText(fstring, 0, SOAP_FAULT_STRING) == NULL) {
        return NULL;
    }
    
    //Generate <detail> node
    deta = mxmlNewElement(soapfault, "detail");
    
    if(deta == NULL) {
        return NULL;
    }
    
    return deta;
}

/*
 **********************************************************************************
 * Function name : gen_cwmp_fault()
 * Description: Generate content of detail: Faultcode, Faultstring
 * Param:
 *        deta: point to the "detail" node
 *        fault_code: FaultCode(9000, 9001...)
 * Return Value: pointer that point to "cwmp:Fault" node
 **********************************************************************************
 */
TRF_node * gen_cwmp_fault(TRF_node *deta, unsigned int fault_code, char *meth_name)
{
    TRF_node *cwmpf, *fcode, *fstring;
    
    int i, size;
    char msg[65];
    
    size = MAX_FAULT_SIZE;
    
    if (meth_name == NULL) {
        memset(msg, 0, sizeof(msg));
    } else {
        strcpy(msg, meth_name);
    }
    
    //Create <cwmp:Fault> node
    cwmpf = mxmlNewElement(deta, CWMPFAULT);
    
    if (cwmpf == NULL) {
        return NULL;
    }  
    
    //Create <FaultCode> node
    fcode = mxmlNewElement(cwmpf, "FaultCode");
    
    if (fcode == NULL) {
        return NULL;
    }
    
    if (mxmlNewInteger(fcode, fault_code) == NULL) {
        return NULL;
    }
    
    //Create <FaultString> node
    fstring = mxmlNewElement(cwmpf, "FaultString");
    
    if (fstring == NULL) {
        return NULL;
    }  
    
    i = fault_search(faultstruct, size, fault_code);
    
    if (i == 0) {
        strcat(msg, " ");
        strcat(msg, faultstruct[i].fault_string);
        mxmlNewText(fstring, 0, msg);
    } else if (i < 0) {
        LOG(m_handler, DEBUG, "Can't find your fault code value\n");
        return NULL;
    } else {
        mxmlNewText(fstring, 0, faultstruct[i].fault_string);
    }

    return cwmpf;
}

/*
 *****************************************************************************
 * Function name : gen_method_fault()
 * Generate a soap envelope containing a fault response for a method
 * Param:
 *       xmlroot:  point to the xml root: <?xml..>
 *       methname: point to the method name
 *       fault_code: the FaultCode for a method
 * Return Value: 0 successfully, NULL unsuccessfully
 *****************************************************************************
 */
TRF_node * gen_method_fault(TRF_node *xmlroot, char *meth_name, unsigned int fault_code)
{
    TRF_node *ENV, *soap_body, *deta, *cwmpf;

    if ((ENV = gen_env(xmlroot)) == NULL) {
        return NULL;
    }
       
    if ((gen_soap_header(ENV) == NULL)) {
        return NULL;
    }
    
    if ((soap_body = gen_soap_body(ENV)) == NULL) {
        return NULL;
    }
    
    if ((deta = gen_soap_fault(soap_body)) == NULL) {
        return NULL;
    }
     
    if ((cwmpf = gen_cwmp_fault(deta, fault_code, meth_name)) == NULL) {
        return NULL;
    }
    
    return cwmpf;
}

/*
 ***********************************************************************************************
 * Function name: gen_param_fault()
 * Description: Generate SetParameterValuesFault element
 * Param:
 *       cwmpf:     point to the node <cwmp:Fault>
 *       meth_name: the method name, for example: "SetParameterValues", "GetParameterValues"
 *       para_name: point to the parameter name
 *       fault_code: fault code
 * Return Value: point to the "[methode name]Fault" node, for example: <SetParameterValuesFault>
 ************************************************************************************************
 */
TRF_node *gen_para_fault(TRF_node *cwmpf, char *meth_name, char *para_name, unsigned int fault_code)
{
    TRF_node *paraname, *fcode, *fstring;
    TRF_node *para_fault = NULL;
    
    int i, size;
    
    size = MAX_FAULT_SIZE;
    
    if (!strcmp("SetParameterValues", meth_name)) {       
        para_fault = mxmlNewElement(cwmpf, "SetParameterValuesFault");
        
        if (para_fault == NULL) {
            return NULL;
        }
    }
    
    if (!strcmp("GetParameterValues", meth_name)) {
        para_fault = mxmlNewElement(cwmpf, "GetParamterValuesFault");
        
        if (para_fault == NULL) {
            return NULL;
        }
    }    
    
    if (!strcmp("GetParameterNames", meth_name)) {
        para_fault = mxmlNewElement(cwmpf, "GetParameterNamesFault");
        
        if(para_fault == NULL) {
            return NULL;
        }
    }
    
    if (!strcmp("SetParameterAttributes", meth_name)) {
    	para_fault = mxmlNewElement(cwmpf, "SetParameterAttributesFault");
    	
    	if (para_fault == NULL) {
    	    return NULL;
    	}
    }
    
    if (!strcmp("GetParameterAttributes", meth_name)) {
        para_fault = mxmlNewElement(cwmpf, "GetParameterAttributesFault");
        
        if (para_fault == NULL) {
            return NULL;
        }
    }
    
    //Generate <ParameterNames> node
    paraname = mxmlNewElement(para_fault, "ParameterName");
    
    if (paraname  == NULL) {
        return NULL;
    }
    
    if (mxmlNewText(paraname, 0 , para_name) == NULL) {
        return NULL;
    }
    
    //Generate <FaultCode> node
    fcode = mxmlNewElement(para_fault, "FaultCode");
    
    if (fcode == NULL) {
        return NULL;
    }
    
    if (mxmlNewInteger(fcode, fault_code) == NULL) {
        return NULL;
    }
    
    //Generate <FaultString> node 
    fstring = mxmlNewElement(para_fault, "FaultSting");
    
    if (fstring == NULL) {
        return NULL;
    }
    
    i = fault_search(faultstruct, size, fault_code);
    
    if (i < 0) {
        LOG(m_handler, ERROR, "Can't find your fault code: %d\n", fault_code);
        return NULL;
    }
    
    if (mxmlNewText(fstring, 0, faultstruct[i].fault_string) == NULL) {
        return NULL;
    }
    
    return para_fault;
}

/*
 ***********************************************************************************
 * Function: get_fault_string()
 * Description: get the description of fault code
 * Param:
 *      int fault_code: fault code in tr-069
 *      char *fault_string: the description of fault code
 * Return value:
 *      success return SUCCESS, else return FAIL
 ***********************************************************************************
 */

int get_fault_string(int fault_code, char *fault_string)
{
    int i;

    i = fault_search(faultstruct, MAX_FAULT_SIZE, fault_code);
    if (i == -1) {
        return FAIL;
    }
    strcpy(fault_string, faultstruct[i].fault_string);

    return SUCCESS;
}



/*
 **********************************************************************************
 * Function: format_time()
 * Description: format the time to the soap data time type
 * Prarm: time_t temp 
 *        char *tm
 * Return Value:
 *     success return SUCCESS, else return FAIL
 *********************************************************************************
 */

int format_time(time_t temp, char *tm)
{
    struct tm *tp;
    char *format = "%Y-%m-%dT%T";
       
    tp = gmtime(&temp);
    
    //format time
    if (strftime(tm, 20, format, tp) == 0) {
        LOG(m_handler, ERROR, "Don't copy any string to buffer\n");
        return FAIL;
    }
    
    return SUCCESS;
}

/*
 ********************************************************************************
 * Function: time_t strtimetosec(char *strtm)
 * Description: tran the time to second
 * Prarm: char *strtm string store time
 * Return : success return seconds else return FAIL
 *******************************************************************************
 */

time_t strtimetosec(char *strtm)
{
    char format[] = "%Y-%m-%dT%T";
    struct tm tm;
    time_t temp;

    if (strptime(strtm, format, &tm) == 0){
        LOG(m_handler, ERROR, "strptime error\n");
        return FAIL;
    }
    
    temp = ewt_mktime(&tm);
    
    return temp;
}

/*
 *******************************************************************************
 * Function: ewt_mktime(struct tm *tm)
 * Description: calculate the seconds of given time since 1970
 * Param: struct tm *tm
 * Return:
 *     return seconds since the year of 1970
 *******************************************************************************
 */

static inline unsigned long ewt_mktime(struct tm *tm)
{
    tm->tm_year += 1900;
    tm->tm_mon += 1;

    if (0 >= (int) (tm->tm_mon -= 2)) {  //1..12 -> 11,12,1..10 
        tm->tm_mon += 12;                // Puts Feb last since it has leap day
        tm->tm_year -= 1;
    }

    return ((( (unsigned long) (tm->tm_year/4 - tm->tm_year/100 + tm->tm_year/400 \
           + 367*tm->tm_mon/12 + tm->tm_mday) + tm->tm_year*365 - 719499 )*24 + \
           tm->tm_hour)*60 + tm->tm_min)*60 + tm->tm_sec;
}

