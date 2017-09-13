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


#include "http.h"
#include "../../auth/auth.h"
#include "../../res/qbuf.h"
#include "../../device/TRF_param.h"
#include "../methods/methods.h"
#include "../../tools/agent_conf.h"
/*
 * Declear function .......
 */
static int gen_http_header(char *sendbuf);
static int gen_auth_info(char *auth);
static void gen_cookie_info(char *session_cookie_info);
static int xml_to_buf(char *sendbuf, TRF_node *xmlroot, int httpheadsize);
static int gen_http_pkg(TRF_node *xmlroot, char *sendbuf);
static int revise_http_cnt_len(char *sendbuf, int xmlsize);

/*
 **************************************************************************
 * Function:  gen_http_header
 * Description:  Generate http header for http package
 * Parameter:
 *      char *sendbuf - Point to string which save data to it
 * Return Value: 
 *      size of http head - success 
 *      HTTP_GEN_FAILED - falied
 **************************************************************************
 */
static int gen_http_header(char *sendbuf)
{
    int httpheadsize = 0;
    
    //Add POST value to http head
    sprintf(sendbuf, "%s %s %s\r\n", HTTP_METHOD, acs_path, HTTP_VERSION);
    
    //Add HOST value to http head
    strcat(sendbuf, "Host:");
    strcat(sendbuf, acs_ip);
    strcat(sendbuf, "\r\n");
    
    //Add User_agent value to http head
    strcat(sendbuf, "User-Agent:");
    strcat(sendbuf, USER_AGENT);
    strcat(sendbuf, "\r\n");

    //Add Content length value to http head
    strcat(sendbuf, "Content-Length:");
    strcat(sendbuf, CONTENT_LENGTH);
    strcat(sendbuf, "\r\n");

    //Add Connection value to http head
    strcat(sendbuf, "Connection:");
    strcat(sendbuf, CONNECTION);
    strcat(sendbuf, "\r\n");

    //Add Content type value to http head
    strcat(sendbuf, "Content-Type:");
    strcat(sendbuf, CONN_TYPE);
    strcat(sendbuf, "\r\n");

    //Add Soap action value to http head
    strcat(sendbuf, "SOAPAction:");
    strcat(sendbuf, SOAP_ACTION);
    
    //Add Cookie
    if (have_cookie_flag == 1) {
        strcat(sendbuf, "\r\n");
        gen_cookie_info(sendbuf + strlen(sendbuf));
    }
    //Add auth head
    if (auth_status != NO_AUTH) {
        strcat(sendbuf, "\r\n");
        if (gen_auth_info(sendbuf + strlen(sendbuf)) == HTTP_GEN_FAILED) {
            return HTTP_GEN_FAILED;
        }
    }
    
    strcat(sendbuf, "\r\n\r\n");
    //get the size of http head
    httpheadsize = strlen(sendbuf);
    
    return httpheadsize;
}

/*
 *****************************************************************************
 * Function: gen_auth_info
 * Description: Generate basic or digest authnication information
 * Parameter: 
 *     char *auth - be used to store the auth info
 * Return Value:
 *     HTTP_GEN_SUCCESS - success 
 *     HTTP_GEN_FAILED - failed
 *****************************************************************************
 */

static int gen_auth_info(char *auth)
{
    unsigned char nc_value[STR_NC_VALUE_LEN + 1];
    //int ret = 0, locate[4];
    char username[ACS_USERNAME_LEN], password[ACS_PASSWORD_LEN];
    agent_conf a_conf;
    
    if (auth_status == BASIC) {    //check auth type
        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));
        
        /*ret = call_dev_func(acsuser_dev_func, GET_OPT, username, locate);
        if (ret != GET_VAL_SUCCESS) {
            return HTTP_GEN_FAILED;
        }
        ret = call_dev_func(acspwd_dev_func, GET_OPT, password, locate);
        if (ret != GET_VAL_SUCCESS) {
            return HTTP_GEN_FAILED;
        }*/
        //agent_conf a_conf;
        if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
            return HTTP_GEN_FAILED;
        }
        strcpy(username, a_conf.acs_username);
        strcpy(password, a_conf.acs_password);
        
        if (get_basic_auth_info(auth, username, password) != 0) {
            return HTTP_GEN_FAILED;
        }
    } else {
        memset(digest_auth.szNonceCount, '0', sizeof(digest_auth.szNonceCount));
        /*if (!strcmp(digest_auth.pszUser, "")) {
            ret = call_dev_func(acsuser_dev_func, GET_OPT, digest_auth.pszUser, locate);
            if (ret != GET_VAL_SUCCESS) {
                return HTTP_GEN_FAILED;
            }
        }
        if (!strcmp(digest_auth.pszPass, "")) {
            ret = call_dev_func(acspwd_dev_func, GET_OPT, digest_auth.pszPass, locate);
            if (ret != GET_VAL_SUCCESS) {
                return HTTP_GEN_FAILED;
            }
        }*/
        //get username and password
        //agent_conf a_conf;
        if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
            return HTTP_GEN_FAILED;
        }
        strcpy(digest_auth.pszUser, a_conf.acs_username);
        strcpy(digest_auth.pszPass, a_conf.acs_password);
        
        nonce_count++;
        sprintf(nc_value, "%x", nonce_count);
        strcpy((digest_auth.szNonceCount + (STR_NC_VALUE_LEN - strlen(nc_value))), nc_value);
        //calculate the auth information
        if (get_digest_auth_info(&digest_auth, auth) != 0) {
            return HTTP_GEN_FAILED;
        }
    }

    return HTTP_GEN_SUCCESS;
}

/*
 *******************************************************************
 * Function: gen_cookie_info
 * Description: generate session cookie header for http request
 * Parameter:
 *      char *session_cookie_info - used to store cookie information
 * Return Value:
 *      void
 *******************************************************************
 */
static void gen_cookie_info(char *session_cookie_info)
{
    int i;
                                                                                                                             
    //generate the cookie information
    sprintf(session_cookie_info, "Cookie: $Version=\"%u\"", cookie_version);
                                                                                                                             
    for (i = 0; i < SUPPORT_MAX_COOKIE_NUM; i++) {
        if (!strcmp(session_cookie_array[i].name, "")) {
            break;
        }
        //add cookie name
        strcat(session_cookie_info, ";");
        strcat(session_cookie_info, session_cookie_array[i].name);
        strcat(session_cookie_info, "=");
        strcat(session_cookie_info, session_cookie_array[i].value);
                                                                                                                             
        //add cookie path
        if (strcmp(session_cookie_array[i].path, "")) {
            strcat(session_cookie_info, ";");
            strcat(session_cookie_info, "$Path=");
            strcat(session_cookie_info, session_cookie_array[i].path);
        }
                                                                                                                             
        //add cookie domain
        if (strcmp(session_cookie_array[i].domain, "")) {
            strcat(session_cookie_info, ";");
            strcat(session_cookie_info, "$Domain=");
            strcat(session_cookie_info, session_cookie_array[i].domain);
        }
        //add cookie port
        if (strcmp(session_cookie_array[i].port, "")) {
            strcat(session_cookie_info, ";");
            strcat(session_cookie_info, "$Port=");
            strcat(session_cookie_info, session_cookie_array[i].port);
        }
    }
                                                                                                                             
    return;
}

/*
 ********************************************************************
 * Function:  xml_to_buf()  
 * Description: save the xml to sendbuf
 * Parameters: 
 *        char *sendbuf - point to a string data will save to it
 *        TRF_node *xmlroot - the node of xml root node
 *        int httpheadsize - the size of the http head size
 * Return value: 
 *     Xml string size - success
 *     HTTP_GEN_FAILED - failed
 ********************************************************************
 */
static int xml_to_buf(char *sendbuf, TRF_node *xmlroot, int httpheadsize)
{
    int xmlsize = 0;
    char *buf;
    TRF_node *node;
    int fault_code;

    //get current pointer of sendbuf
    buf = sendbuf + httpheadsize;
    
    //Judge the xmlroot tree whether have data or not
    node = mxmlFindElement(xmlroot, xmlroot, "SOAP-ENV:Envelope", NULL, NULL, MXML_DESCEND);
    if (node == NULL) {
        mxmlDelete(xmlroot);
        return xmlsize;
    }
    
    xmlsize = mxmlSaveString(xmlroot, buf, MAX_DATA_LEN - httpheadsize, MXML_NO_CALLBACK);
    if (xmlsize < 0) {
        mxmlDelete(xmlroot);
        //handle error
        xmlroot = gen_xml_header();
        if (xmlsize == -1)     //internal error
            fault_code = 9001;
        else                    //buffer is not have enough space
            fault_code = 9004;

        node = gen_method_fault(xmlroot, "", fault_code);
        if (node == NULL) {
            mxmlDelete(xmlroot);
            return HTTP_GEN_FAILED;
        }
        xmlsize = mxmlSaveString(xmlroot, buf, MAX_DATA_LEN - httpheadsize, MXML_NO_CALLBACK);
        if (xmlsize < 0) {
            mxmlDelete(xmlroot);
            return HTTP_GEN_FAILED;
        }        
    }
    // delete tree
    mxmlDelete(xmlroot);
     
    return xmlsize;
}

/*
 ***********************************************************************
 * Function:  gen_http_pkg()  
 * Description : generate the http data package 
 * Parameters:
 *        TRF_node *xmlroot - the node of xml root node
 *        char *sendbuf - point to a string data will save to it
 * Return Value: 
 *        Xml string size - success
 *        HTTP_GEN_FAILED - failed
 ***********************************************************************
 */
static int gen_http_pkg(TRF_node *xmlroot, char *sendbuf)
{
    int xmlsize, httpheadsize;

    //generate http head and put it into buffer
    httpheadsize = gen_http_header(sendbuf);
    if (httpheadsize == HTTP_GEN_FAILED) {
        return HTTP_GEN_FAILED;
    }
    
    //xml data into buffer
    xmlsize = xml_to_buf(sendbuf, xmlroot, httpheadsize);
    if(xmlsize == HTTP_GEN_FAILED) {
        return HTTP_GEN_FAILED;
    }
    return xmlsize;
}

/*
 ***************************************************************************
 * Function: revise_http_cnt_len() 
 * Description: revise the content length in HTTP head
 * Parameter: 
 *        char *sendbuf - point to a string data will save to it
 *        int xmlsize   - the size of xml data
 * Return Value: 
 *        HTTP_GEN_SUCCESS - success
 *        HTTP_GEN_FAILED  - failed
 **************************************************************************
 */

static int revise_http_cnt_len(char *sendbuf, int xmlsize)
{
    char *size_locate;
    
    // Find the place of Content length
    size_locate = strstr(sendbuf, CONTENT_LENGTH);
    if (size_locate == NULL) {
        LOG(m_handler, ERROR, "revise_http_cnt_len: Can't find the \
            place of content-length\n");
        return HTTP_GEN_FAILED;
    }
    
    //Fill the content-length
    sprintf(size_locate, "%d", xmlsize);
    strcat(sendbuf, size_locate + STR_CONTENT_LENGTH_LEN);
    
    return HTTP_GEN_SUCCESS;
}

/*
 ****************************************************************************
 * Function: save_to_sendbuf() 
 * Description: save the http data package to sendbuf
 * Parameter: 
 *        TRF_node *xmlroot - the node of xml root node
 *        char *sendbuf     - point to a string data will save to it
 * Return Value:
 *        Http data size  - success
 *        HTTP_GEN_FAILED - failed
 ****************************************************************************
 */
int save_to_sendbuf(TRF_node *xmlroot, char *sendbuf)
{
    int xmlsize;
	
    //generate http package
    xmlsize = gen_http_pkg(xmlroot, sendbuf);
    if (xmlsize == HTTP_GEN_FAILED) {
    	return HTTP_GEN_FAILED;
    }
    
    //revise the content_length of http package
    if (revise_http_cnt_len(sendbuf, xmlsize) == HTTP_GEN_FAILED) {
        return HTTP_GEN_FAILED;
    }
    
    LOG(m_handler, DEBUG, "%s\n", sendbuf);
    
    return strlen(sendbuf); 
}

