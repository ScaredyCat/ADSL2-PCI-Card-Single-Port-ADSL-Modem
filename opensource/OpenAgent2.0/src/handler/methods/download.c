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
 * Include necessary head file .....
 */

#include "download.h"
#include "../../tools/dl_conf.h"
#include "../../auth/auth.h"
#include "../../comm/comm.h"
#include "../cpe_task_list.h"
#include "../../tools/logger.h"

/*
 *********************************************************************
 * Function name: get_dl_param
 * Description: get the parameter of the download method
 * Parameter: TRF_node *method: the pointer point to the node of the method; TR_download *dl: the pointer pointer to the download parameter structure which you want save param to it
 * Return value: none
 *********************************************************************
*/
void get_dl_param(TRF_node *method, TR_download *dl)
{
    method = method->child;
    
    while (method != NULL) {
        if( ! strcmp (method->value.element.name, "CommandKey") ) {
            if (method->child != NULL) {
                strcpy (dl->command_key, method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
        
        if ( ! strcmp (method->value.element.name, "FileType") ) {
            if (method->child != NULL) {
                strcpy (dl->file_type, method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
        if ( ! strcmp(method->value.element.name, "URL") ) {
            if (method->child != NULL) {
                strcpy (dl->url, method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
        if ( ! strcmp (method->value.element.name, "Username") ) {
            if (method->child != NULL) {
                strcpy (dl->username, method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
        if ( ! strcmp (method->value.element.name, "Password") ) {
            if (method->child != NULL) {
                strcpy (dl->password, method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
        if ( ! strcmp (method->value.element.name, "FileSize") ) {
            if (method->child != NULL) {
                dl->file_size = atoi (method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
        if ( ! strcmp (method->value.element.name, "TargetFileName") ) {
            if (method->child != NULL) {
                strcpy (dl->target_file_name, method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
        if ( ! strcmp (method->value.element.name, "DelaySeconds") ) {
            if (method->child != NULL) {
                dl->delay_seconds = atoi (method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
        if ( ! strcmp (method->value.element.name, "SuccessURL") ) {
            if (method->child != NULL) {
                strcpy (dl->success_url, method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
        if ( ! strcmp (method->value.element.name, "FailureURL") ) {
            if (method->child != NULL) {
                strcpy (dl->failure_url, method->child->value.opaque);
            }
            method = method->next;
            continue;
        }
    }
}

/*
 *********************************************************************
 * Function name: gen_dl_resp_argu
 * Description: generate download response argument
 * Parameter: TR_download_resp *dlr: the pointer point to the download response argument; TRF_node *method: the pointer pointer to the node of the method
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/ 
int gen_dl_resp_argu(TR_download_resp *dlr, TRF_node *method)
{
    TRF_node *status = NULL, *st = NULL, *ct = NULL;
    char timebuf[20];
    
    //generate status node
    status = mxmlNewElement (method, "Status");
    if (!status) {
        return FAIL;
    }
    if (mxmlNewInteger (status, dlr->status) == NULL) {
        return FAIL;
    }

    //genarate starttime node 
    st = mxmlNewElement (method, "StartTime");
    if (!st) {
        return FAIL;
    }
    
    if (dlr->start_time != 0) {
        format_time (dlr->start_time, timebuf);
        if (mxmlNewText(st, 0, timebuf) == NULL) {
            return FAIL;
        }
    }

    //genarate completetime node
    ct = mxmlNewElement (method, "CompleteTime");
    if (!ct) {
        return FAIL;
    }
    
    if (dlr->complete_time != 0) {
        format_time (dlr->start_time, timebuf);
        if (mxmlNewText (ct, 0, timebuf) == NULL) {
            return FAIL;
        }
    }
    
    return SUCCESS;
    
}

/*
 *********************************************************************
 * Function name: gen_dl_resp
 * Description: generate download response data
 * Parameter: TR_download_resp *dlr: the pointer point to the download response argument
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int gen_dl_resp(TR_download_resp *dlr)
{
    char name[] = "DownloadResponse";
    TRF_node *method = NULL;
    int res = 0;

    //generate soap frame
    method = gen_soap_frame(name);
    if (method == NULL) {
        return FAIL;
    }
    
    res = gen_dl_resp_argu (dlr, method);
    if (res != 0){
        LOG (m_handler, ERROR, "generate download response argnment failed\n");
        return FAIL;
    }
   
    LOG (m_handler, DEBUG, "generate download response success\n");
    
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: process_dl
 * Description: process the download request
 * Parameter: TRF_node *method: the pointer point to method node
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int process_dl(TRF_node *method)
{
    int res = 0;

    time_t start_time;
    time_t complete_time;

    
    TR_download *down = NULL;
    TR_download_resp dlr;
    
    down = (TR_download *)malloc(sizeof(TR_download));
    if (!down) {
        LOG (m_handler, ERROR, "Unable allocate memory for down\n");
        return FAIL;
    }
    
    //Init download struct
    strcpy (down->command_key, "");
    down->delay_seconds = 0;
    strcpy (down->failure_url, "");
    strcpy (down->file_type, "");
    down->file_size = 0;
    strcpy (down->password, "");
    strcpy (down->success_url, "");
    strcpy (down->target_file_name, "");
    strcpy (down->url, "");
    strcpy (down->username, "");
    
    
    //get download parameter
    get_dl_param (method, down);
    LOG (m_handler, DEBUG, "Get Download param finished\n");
    
    //Debug write download param to dl_conf_path
    LOG (m_handler, DEBUG, "down.command_key : %s\n, down.delay_seconds: %d\n, down.failure_url: %s\n, down.file_type : %s\n, down.file_size : %d\n, down.password : %s\n, down.success_url: %s\n, down.target_file_name: %s\n, down.url : %s\n, down.username: %s\n", down->command_key, down->delay_seconds, down->failure_url, down->file_type, down->file_size, down->password, down->success_url, down->target_file_name, down->url, down->username);
    
    if (down->file_size < filesize && down->delay_seconds < delayseconds) {
        download_at_once_flag = 1;

        res = trf_download (down, &start_time, &complete_time);
        free (down);
        if (res != 0) {
            LOG (m_handler, DEBUG, "Download unsuccessful\n");
            gen_method_fault (xmlroot, "download", res);
            return SUCCESS;
        }
        else {
            LOG (m_handler, DEBUG, "Download successful\n");
        	//generate download response(status: 0)
            dlr.status = 0;
            dlr.start_time = start_time;
            dlr.complete_time = complete_time;
            
            res = gen_dl_resp (&dlr);
            if (res != 0) {
                return FAIL;
            }
            
            //Add reboot to task list
            res = add_task_list ("trf_download", NULL);
            if (res != 0) {
                return FAIL;
            }
            return SUCCESS; 
        }
    }
    
    //generate download reasponse (status: 1)
    download_at_once_flag = 0;

    dlr.status = 1;
    dlr.start_time = 0;
    dlr.complete_time = 0;
    
    res = gen_dl_resp (&dlr);
    if (res != 0) {
        return FAIL;
    }
    
    if (!first_reboot) {
        //First download
        res = add_task_list ("trf_download", down);
        if (res != 0) {
            free (down);
            return FAIL;
        }
    }
    else {
        //First reboot, write dl_conf_path
        
        res = write_dl_conf (dl_conf_path, 0, down);
        
        if (res != 0) {
            LOG (m_handler, ERROR, "Write doamload to dl_conf failed\n");
            free (down);
            return FAIL;
        }
        LOG (m_handler, DEBUG, "Write download to dl_conf success\n");

        free (down);
        
        //Add reboot to task list
        res = add_task_list ("dev_reboot", NULL);
        
        if (res != 0) {
            return FAIL;
        }

    }
    
    return SUCCESS;
    
}

/*
 *********************************************************************
 * Function name: get_file_ser_path
 * Description: get path in given url
 * Parameter: char *url: the whole url; char *path: the result
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int get_file_ser_path(char *url, char *path)
{
    char *ptr = NULL;
    char buf[128];
   
    strcpy (buf, url); 
    ptr = strstr (buf, "http://");
    if (ptr == NULL) {
        LOG (m_handler, ERROR, "There is no \"http://\" in url.\n");

        ptr = strstr(buf, "https://");

        if(ptr == NULL)
        {
            LOG(m_device, DEBUG, "There is no \"https://\" in given url.\n");
            return FAIL;
        }
        else
        {
            ptr = ptr + 8;
            strcpy(buf, ptr);
        }
    }
    else
    {
        ptr = ptr + 7;
        strcpy (buf, ptr);
    }
    
    ptr = strstr (buf, "/");
    if (ptr == NULL) {
        strcpy (path, "/");
        return SUCCESS;
    }
    else {
        strcpy (path, ptr);
        return SUCCESS;
    }
}

/*
 *********************************************************************
 * Function name: get_down_file_name
 * Description: get download file name
 * Parameter: TR_download *param: the pointer pointe to the TR_download structure; char *file_name: the result
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
void get_down_file_name(TR_download *param, char *file_name)
{
    char buf[128];
    char *p = NULL;
    
    strcpy (buf, param->url);
    p = buf;
    p = p + 7;
    p =  strrchr (p, '/');
    if ((p != NULL) && (*(p + 1) != '\0')) {
        strcpy (file_name, p + 1);
    }
    else if (strcmp (param->target_file_name, "")) {
        strcpy (file_name, param->target_file_name);
    }
    else {
        strcpy (file_name, TEMP_FILE_NAME);
    }
    LOG (m_handler, DEBUG, "The download file name is %s\n", file_name);

    return;
}

/*
 *********************************************************************
 * Function name: get_digest_param_value
 * Description: get digest authentication parameter value
 * Parameter: char *dg: the digest string; TR_digest_auth *pdigest_auth: the pointer point to the TR_digest_auth structure; char *url: the url; char *user: the username; char *pwd: the password
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int get_digest_param_value(char *dg, TR_digest_auth *pdigest_auth, char *url,char *user, char *pwd)
{
    char *pc1 = NULL, *pc2 = NULL;
    char name[50];
    char val[500];
    int res = 0;
    
    pc2 = dg;
    pc1 = strstr (dg, "=");
    
    //This for loop get pszRealm,pszNonce,pszAlg,pszQop values
    while (*pc2 != '\n' && *pc2 != '\r') {
        memset (name, 0, sizeof (name));
        memset (val, 0, sizeof (val));
        
        //get parameter name
        while (*pc2 == ' ') {
            pc2++;
        }
                                                                                                                             
        while (*(pc1 - 1)  == ' ') {
            pc1--;
        }
        
        strncpy (name, pc2, pc1 - pc2);
        LOG (m_handler, DEBUG, "digest parameter : %s\n", name);
        
        //get parameter value
        pc2 = strstr (pc1, "=");
        pc2++;
        
        while (*pc2 == ' ') {
            pc2++;
        }
        if (*pc2 == '"') {
            pc2++;
            pc1 = strstr (pc2, "\"");
            strncpy (val, pc2, pc1-pc2);
            pc1 = pc1 + 2;
        }
        else {
            pc1 = strstr (pc2, ",");                                                                                                     strncpy (val, pc2, pc1-pc2);
            pc1 = pc1 + 1;
        }
        
        //Move pointer
        pc2 = pc1;
        while (*pc2 == ' ') {
            pc2++;
        }
        pc1 = strstr (pc2, "=");
                                                                                                                                     
        if ( ! strcmp (name, "realm") ) {
            strcpy (pdigest_auth->pszRealm, val);
            continue;
        }
        if ( ! strcmp (name, "nonce") ) {
            strcpy (pdigest_auth->pszNonce, val);
            continue;
        }
        if ( ! strcmp (name, "algorithm") ) {
            strcpy (pdigest_auth->pszAlg, val);
            continue;
        }
        if ( ! strcmp(name, "qop") ) {
            strcpy (pdigest_auth->pszQop, val);
            continue;
        }
        
    }
    
    //get pszCNonce value
    strcpy (pdigest_auth->pszCNonce, "58dde74d9b47ca4f8e410cb170dd47dc");
    
    //get user name value
    strcpy (pdigest_auth->pszUser, user);
    
    //get password value
    strcpy (pdigest_auth->pszPass, pwd);

    if ( ! strcmp(pdigest_auth->pszPass, "") ) {
        LOG (m_handler, DEBUG, "Password is empty\n");
        return FAIL;
    }
    
    //get szNonceCount value
    strcpy (pdigest_auth->szNonceCount, "00000001");
    
    //get pszMethod value
    strcpy (pdigest_auth->pszMethod, "GET");
    
    //get pszURI value
    res = get_file_ser_path (url, pdigest_auth->pszURI);
    if (res != 0) {
        LOG (m_handler, ERROR, "Can't get URI in url in download method.\n");
        return FAIL;
    }

    LOG (m_handler, DEBUG, "relem is : %s\n, Nonce is : %s\n, pszAlg is : %s\n, pszQop is : %s\n, user is : %s\n, Password is : %s\n, pszURI is : %s\n", pdigest_auth->pszRealm, pdigest_auth->pszNonce, pdigest_auth->pszAlg, pdigest_auth->pszQop, pdigest_auth->pszUser, pdigest_auth->pszPass, pdigest_auth->pszURI);
    
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: get_auth_info
 * Description: get get authentication information from http head
 * Parameter: char *recvbuf: the pointer point the receive data; char *auth_info: the authentication string; char *url: the url; char *user: the username; char *pwd: the password
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int get_auth_info(char *recvbuf, char *auth_info, char *url, char *user, char *pwd)
{
    char *sp = NULL, *dg = NULL;
    int res = 0;
    TR_digest_auth digest_info;
    
    //init auth_head
    memset(auth_info, 0, sizeof(auth_info));
    
    //init digest_auth
    memset (&digest_info, 0, sizeof(TR_digest_auth));
    
    sp = strstr (recvbuf, "WWW-Authenticate");
    if (sp == NULL) {
        LOG (m_handler, ERROR, "WWW-Authenticate: can't find WWW-Authenticate\n");
        return FAIL;
    }
    
    sp = strtok (sp, " ");
    sp = strtok (NULL, " ");
    
    dg = strtok (NULL, "");
    dg = strtok (dg, "\r\n");
    
    LOG (m_handler, DEBUG, "%s\n", dg);
    
    if ( ! strcmp (sp, "Basic") ) {
        return -2;
    }
    if ( ! strcmp (sp, "Digest") ) {
    	//get digest authcation value
    	res = get_digest_param_value (dg, &digest_info, url, user, pwd);
    	if (res != 0) {
    	    LOG (m_handler, ERROR, "Get digest parameter values failed\n");
    	    return FAIL;
    	}
    	
        get_digest_auth_info (&digest_info, auth_info);
        
        LOG (m_handler, DEBUG, "Get auth info success, Auth-Head : %s\n", auth_info);
    }       
    return SUCCESS;    
}

/*
 *********************************************************************
 * Function name: dev_download_file
 * Description: download file from server
 * Parameter: TR_download *param: the pointer point to the TR_download structure
 * Return value: 0:SUCCESS  other:FAIL
 *********************************************************************
*/
int dev_download_file(TR_download *param)
{
    int sockfd, nrecv, f, n, res;
    int i = 0, j = 0, basicflag = 0;
    char recvbuf[MAX_DATA_SIZE];

    char *p = NULL;
    char page[128]; 
    char data[128];
    char sdbuf[1024];
    char tmp_buf[64];
    char file_ser_ip[33];
    char file_ser_port[64];
    char file_ser_name[128];
    int  fd = 0;
    
    char input[128];
    char output[128];
    char auth_info[512];
    int length;
    
    int ssl_conn_flag;
    int ssl_conn_mode;
    int dl_no_port_flag;
    #ifdef USE_SSL
    SSL *dl_ssl;
    SSL_CTX *dl_ctx;
    #endif

    void *handle = NULL;
    char *d_error = NULL;
    int (*dev_sysflashsizeget)() = NULL;
    void (*dev_killallapps)() = NULL; 
    int (*dev_parseimagedata)(char*, int, int) = NULL;

    //Add
    char *curptr = NULL;
    dl_len = 0;
    int flash_size = 0;
    imageptr = NULL;

    //begin
    handle = dlopen(dev_lib_path, RTLD_LAZY);
    if (!handle)
    {
        LOG(m_handler, ERROR, "%s\n", dlerror());
        return INTERNAL_ERROR;
    }

    dev_sysflashsizeget = dlsym(handle, dev_sysflashsizeget_func);
    if((d_error = dlerror()) != NULL)
    {
        LOG(m_handler, ERROR, "%s\n", d_error);
        dlclose(handle);
        return INTERNAL_ERROR;
    }

    dev_killallapps = dlsym(handle, dev_killallapps_func);
    if((d_error = dlerror()) != NULL)
    {
        LOG(m_handler, ERROR, "%s\n", d_error);
        dlclose(handle);
        return INTERNAL_ERROR;
    }

    dev_parseimagedata = dlsym(handle, dev_parseimagedata_func);
    if((d_error = dlerror()) != NULL)
    {
        LOG(m_handler, ERROR, "%s\n", d_error);
        dlclose(handle);
        return INTERNAL_ERROR;
    }
    //end

    if(param->file_size == 0)
    {
        dlclose(handle);
        return INTERNAL_ERROR;
    }

    //getflsah size
    flash_size = dev_sysflashsizeget () + DOWNLOAD_OVERHEAD;
    //Judge have the enough space to store the file
    LOG (m_handler, DEBUG, "Device flash size is : %d\n", flash_size);

    if (param->file_size > flash_size) {
        LOG (m_handler, ERROR, "File Size is over by %d\n", param->file_size - flash_size);
        dlclose(handle);
        return REQUEST_DENY;
    }

    LOG (m_handler, DEBUG, "Download start with len = %d with flash size = %d\n", param->file_size, flash_size);

    //END ADD
 
    memset (recvbuf, 0 , sizeof(recvbuf));
 
    res = dev_get_acs_ip_path(param->url, file_ser_name, file_ser_ip, file_ser_port, page, &ssl_conn_flag, 1, &dl_no_port_flag);
    if(res != 0)
    {
         LOG (m_handler, ERROR, "Get file server ip and name unsuccessfully\n");
         dlclose(handle);
         return INTERNAL_ERROR;
    }
    LOG(m_handler, DEBUG, "The ssl flag is %d\n", ssl_conn_flag);
    
    get_down_file_name (param, tmp_buf);
    
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0))== -1) {
        LOG (m_handler, ERROR, "Create Socket Unsuccessfully\r\n");
        dlclose(handle);
        return INTERNAL_ERROR;
    }
    
     #ifdef USE_SSL
        res = connect_auth(file_ser_ip, atoi(file_ser_port), &sockfd, (void **)(&dl_ssl), (void **)(&dl_ctx), ca_cert_path, client_cert_path, client_key_path, &ssl_conn_flag, &ssl_conn_mode, &dl_no_port_flag);
    #else
        res = connect_auth(file_ser_ip, atoi(file_ser_port), &sockfd, NULL, NULL, NULL, NULL, NULL, &ssl_conn_flag, &ssl_conn_mode, &dl_no_port_flag); 
    #endif

    LOG(m_handler, DEBUG, "Download flag is %d, conn_mode is %d\n", ssl_conn_flag, ssl_conn_mode);
 
    if(res != SUCCESS)
    {
        LOG (m_handler, ERROR, "Download connect auth Unsuccessfully\r\n");
        dlclose(handle);
        return INTERNAL_ERROR;
    }
    LOG (m_handler, ERROR, "Download connect auth successfully\r\n");
    
    if (!strcmp(param->username, "") || !strcmp(param->password, "")) {
        sprintf (sdbuf,"GET %s HTTP/1.1\r\n", page);
        sprintf (data, "Host: %s\r\n", file_ser_name);
        strcat (sdbuf, data);
        strcat (sdbuf, "Accept: */*\r\n");
        strcat (sdbuf, "User-Agent: Mozilla/4.0\r\n");
        strcat (sdbuf, "Pragma: no-cache\r\n");
        strcat (sdbuf, "Cache-Control: no-cache\r\n");
        strcat (sdbuf, "Connection: close\r\n");
        strcat (sdbuf, "\r\n\0");
        LOG (m_handler, DEBUG, "Now the sendbuf is\n%s\n", sdbuf); 
                     
        //Send
        #ifdef USE_SSL
            res = sock_send(&sockfd, (void **)(&dl_ssl), sdbuf, DOWN_TIME_OUT, ssl_conn_mode);
        #else
            res = sock_send(&sockfd, NULL, sdbuf, DOWN_TIME_OUT, ssl_conn_mode);
        #endif
 
        if(res != SUCCESS)
        {
            LOG (m_handler, ERROR, "Send unsuccessfully\r\n");
            if(ssl_conn_mode != 0)
            {
                #ifdef USE_SSL
                destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
 
            close (sockfd);
            dlclose(handle);
            return INTERNAL_ERROR;
        }
        LOG (m_handler, DEBUG, "Send success!\r\n");
        
        //Recevie
        while (1) {
            #ifdef USE_SSL
                nrecv = sock_recv(&sockfd, (void**)(&dl_ssl), recvbuf, MAX_DATA_SIZE, DOWN_TIME_OUT , ssl_conn_mode);
            #else
                nrecv = sock_recv(&sockfd, NULL, recvbuf, MAX_DATA_SIZE, DOWN_TIME_OUT, ssl_conn_mode);
            #endif

            switch(nrecv)
            {
                case SUCCESS:
                    break;
                case COMM_ERROR_RW:
                    LOG (m_handler, ERROR, "Recv data from file server unsuccessfully\r\n");
                    if(ssl_conn_mode != 0)
                    {
                        #ifdef USE_SSL
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }
 
                    close (sockfd);
                    dlclose(handle);
                    return DOWNLOAD_FAILURE;

                case COMM_ERROR_LIB:
                    if(ssl_conn_mode != 0)
                    { 
                        #ifdef USE_SSL 
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close (sockfd);
                    dlclose(handle);
                    return INTERNAL_ERROR;
                case COMM_ERROR_FDSET:
                    LOG (m_handler, ERROR, "FD_ISSET error!\r\n");
                    if(ssl_conn_mode != 0)
                    { 
                        #ifdef USE_SSL 
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close (sockfd);
                    dlclose(handle);
                    return INTERNAL_ERROR;
                case COMM_ERROR_TIMEOUT:
                    LOG (m_handler, DEBUG, "Receive error (maybe timeout).\r\n");
                    if(ssl_conn_mode != 0)
                    { 
                        #ifdef USE_SSL 
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close (sockfd);
                    dlclose(handle);
                    return DOWNLOAD_FAILURE;
            }

            if (i == 0) {
                if ( strstr (recvbuf, "200 OK") == NULL ) {
                    LOG (m_handler, ERROR, "Download file unsuccessfully\n.");
                    if(ssl_conn_mode != 0)
                    { 
                        #ifdef USE_SSL 
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close (sockfd);
                    dlclose(handle);
                    return DOWNLOAD_FAILURE;
                }
                //ADD
                //Judge if download firmware
                if (strcmp (param->file_type, "1 Firmware Upgrade Image") == 0) {
                    dev_killallapps();
                    LOG (m_handler, DEBUG, "Done removing processes\n");
                }
                LOG (m_handler, DEBUG, "Allocating %d bytes buffer\n", param->file_size);

                curptr = (char *)malloc(param->file_size);
                if (curptr == NULL) {
                    LOG (m_handler, ERROR, "Failed to allocate memory for the image. size required : [%d].\n", 
                    param->file_size);
                    dlclose(handle);
                    return INTERNAL_ERROR;
                }
                imageptr = curptr;
                LOG (m_handler, DEBUG, "Memory allocated %d\n", param->file_size);
                //END ADD
                        
                if (((p = strstr(recvbuf, "\r\n\r\n")) != NULL) || ((p = strstr(recvbuf, "\n\r\n\r")) != NULL)) {
                    p = p + 4;
                    n = nrecv-(p-recvbuf);
                    if (n != 0) {
                        //strcpy(recvbuf, p);
                    }
                    recvbuf[MAX_DATA_SIZE] = '\0';
                }
                else {                           
                    LOG (m_handler, ERROR, "Can't find the begining of data.\n");
                    if(ssl_conn_mode != 0)
                    { 
                        #ifdef USE_SSL 
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close (sockfd);
                    free(imageptr);
                    dlclose(handle);
                    return DOWNLOAD_FAILURE;                            
                }
                    
                //ADD
                if (n != 0) {
                    memcpy (curptr, p, n);
                    curptr += n;
                    dl_len += n;
                    if (dl_len > param->file_size) {
                        LOG(m_handler, ERROR, "Falied on Over size image\n");
                        close(sockfd);
                        free(imageptr);
                        dlclose(handle);
                        return INTERNAL_ERROR;
                    }
                }
                //END ADD

            }
            else {   //i != 0 
                //ADD
                memcpy (curptr, recvbuf, nrecv);
                curptr += nrecv;
                dl_len += nrecv;
                if (dl_len > param->file_size) {
                    LOG (m_handler, ERROR, "Failed on over size image\n");
                    close(sockfd);
                    free(imageptr);
                    dlclose(handle);
                    return INTERNAL_ERROR;
                }
                //END ADD
            }
            i = 1;
            LOG (m_handler, DEBUG, "The size of recv data is %d\r\n", nrecv);
            if (nrecv == 0)
                break;
        }
    }
    else {
    	sprintf (sdbuf,"GET %s HTTP/1.1\r\n", page);
        sprintf (data, "Host: %s\r\n", file_ser_name);
        strcat (sdbuf, data);
        strcat (sdbuf, "Accept: */*\r\n");
        strcat (sdbuf, "User-Agent: Mozilla/4.0\r\n");
        strcat (sdbuf, "Pragma: no-cache\r\n");
        strcat (sdbuf, "Cache-Control: no-cache\r\n");
        strcat (sdbuf, "Connection: close\r\n");
        
        sprintf (input, "%s:%s", param->username, param->password);

        //Encode "username:password" using base64
        length = Base64Enc ((unsigned char *)output, (const unsigned char *)input, strlen(input));
        LOG (m_handler, DEBUG, "The encoded string is %s length of %d\n", output, length);
        sprintf (auth_info, "Authorization: Basic %s", output);
        
        strcat (sdbuf, auth_info);
        strcat (sdbuf, "\015\012\015\012\0");
        LOG (m_handler, DEBUG, "Now the sendbuf is\n%s\n", sdbuf);
               
        //Send
        #ifdef USE_SSL
            res = sock_send(&sockfd, (void **)(&dl_ssl), sdbuf, DOWN_TIME_OUT, ssl_conn_mode);
        #else
            res = sock_send(&sockfd, NULL, sdbuf, DOWN_TIME_OUT, ssl_conn_mode);
        #endif

        if(res != SUCCESS)
        {
            LOG (m_handler, ERROR, "Send unsuccessfully\r\n");
            if(ssl_conn_mode != 0)
            { 
                #ifdef USE_SSL 
                destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }

            close (sockfd);
            dlclose(handle);
            return INTERNAL_ERROR;
        }
        LOG (m_handler, DEBUG, "Send success!\r\n");
        
        //Receive
        while (1) {
            #ifdef USE_SSL
                nrecv = sock_recv(&sockfd, (void**)(&dl_ssl), recvbuf, MAX_DATA_SIZE, DOWN_TIME_OUT , ssl_conn_mode);
            #else
                nrecv = sock_recv(&sockfd, NULL, recvbuf, MAX_DATA_SIZE, DOWN_TIME_OUT, ssl_conn_mode);
            #endif

            switch(nrecv)
            {
                case SUCCESS:
                    break;
                case COMM_ERROR_RW:
                    LOG (m_handler, ERROR, "Recv data from file server unsuccessfully\r\n");
                    if(ssl_conn_mode != 0)
                    { 
                        #ifdef USE_SSL 
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close (sockfd);
                    dlclose(handle);
                    return DOWNLOAD_FAILURE;

                case COMM_ERROR_LIB:
                    if(ssl_conn_mode != 0)
                    { 
                        #ifdef USE_SSL 
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close (sockfd);
                    dlclose(handle);
                    return INTERNAL_ERROR;
                case COMM_ERROR_FDSET:
                    LOG (m_handler, ERROR, "FD_ISSET error!\r\n");
                    if(ssl_conn_mode != 0)
                    { 
                        #ifdef USE_SSL 
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close (sockfd);
                    dlclose(handle);
                    return INTERNAL_ERROR;
                case COMM_ERROR_TIMEOUT:
                    LOG (m_handler, DEBUG, "Receive error (maybe timeout).\r\n");
                    if(ssl_conn_mode != 0)
                    { 
                        #ifdef USE_SSL 
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close (sockfd);
                    dlclose(handle);
                    return DOWNLOAD_FAILURE;
            }

            if (i == 0) {
                if (strstr(recvbuf, "401 Unauthorized") != NULL) {
                    if(ssl_conn_mode != 0)
                    {
                        #ifdef USE_SSL
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close(sockfd);
                    basicflag = 1;
                    break;                         
                }
                else {  //There is no 401 in http head
                    if (strstr(recvbuf, "200 OK") == NULL) {
                        LOG (m_handler, ERROR, "Download file unsuccessfully\n.");
                        if(ssl_conn_mode != 0)
                        { 
                            #ifdef USE_SSL 
                            destroy_ssl_res(&dl_ssl, &dl_ctx);
                            #endif
                        }

                        close (sockfd);
                        dlclose(handle);
                        return DOWNLOAD_FAILURE;
                    }
                    //ADD
                    //Judge if download firmware
                    if (strcmp(param->file_type, "1 Firmware Upgrade Image") == 0) {
                        dev_killallapps();
                        LOG(m_handler, DEBUG, "Done removing processes\n");
                    }
                    LOG(m_handler, DEBUG, "Allocating %d bytes buffer\n", param->file_size);

                    curptr = (char *)malloc(param->file_size);
                    if (curptr == NULL) {
                        LOG (m_handler, ERROR, "Failed to allocate memory for the image. size required : [%d].\n", param->file_size);
                        dlclose(handle);
                        return INTERNAL_ERROR;
                    }
                    imageptr = curptr;
                    LOG (m_handler, DEBUG, "Memory allocated %d\n", param->file_size);
                    //END ADD
                            
                    if (((p = strstr(recvbuf, "\r\n\r\n")) != NULL) || ((p = strstr(recvbuf, "\n\r\n\r")) != NULL)) {
                        p = p + 4;
                        n = nrecv-(p-recvbuf);
                        if (n != 0) {
                            //strcpy(recvbuf, p);
                        }
                        recvbuf[MAX_DATA_SIZE] = '\0';
                    }
                    else {                       
                        LOG (m_handler, ERROR, "Can't find the begining of data.\n");
                        if(ssl_conn_mode != 0)
                        { 
                            #ifdef USE_SSL 
                            destroy_ssl_res(&dl_ssl, &dl_ctx);
                            #endif
                        }

                        close (sockfd);
                        free(imageptr);
                        dlclose(handle);
                        return DOWNLOAD_FAILURE;
                    }
                    
                    if (n != 0) {
                        memcpy (curptr, p, n);
                        curptr += n;
                        dl_len += n;
                        if (dl_len > param->file_size) {
                            LOG(m_handler, ERROR, "Failed on over size image\n");
                            close(sockfd);
                            free(imageptr);
                            dlclose(handle);
                            return INTERNAL_ERROR;
                        }
                    }
                    //END ADD
                }
            }
            else
            {
                //ADD
                memcpy(curptr, recvbuf, nrecv);
                curptr += nrecv;
                dl_len += nrecv;
                if (dl_len > param->file_size) {
                    LOG (m_handler, ERROR, "Failed on over size image\n");
                    close(sockfd);
                    free(imageptr);
                    dlclose(handle);
                    return INTERNAL_ERROR;
                }
                //END ADD
            }
            i = 1;
            LOG (m_handler, DEBUG, "The size of recv data is %d\r\n", nrecv);
            if (nrecv == 0)
                break;               
        }
        // Digest Authentication
        if (basicflag == 1) {
            LOG (m_handler, DEBUG, "The recvbuf is \n%s\n", recvbuf);
        
            if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
                LOG (m_handler, ERROR, "Create Socket Unsuccessfully\r\n");
                dlclose(handle);
                return INTERNAL_ERROR;
            }
    
            #ifdef USE_SSL
                res = connect_auth(file_ser_ip, atoi(file_ser_port), &sockfd, (void **)(&dl_ssl), (void **)(&dl_ctx), ca_cert_path, client_cert_path, client_key_path, &ssl_conn_flag, &ssl_conn_mode, &dl_no_port_flag);
            #else
                res = connect_auth(file_ser_ip, atoi(file_ser_port), &sockfd, NULL, NULL, NULL, NULL, NULL, &ssl_conn_flag, &ssl_conn_mode, &dl_no_port_flag); 

                LOG(m_handler, DEBUG, "Download flag is %d, conn_mode is %d\n", ssl_conn_flag, ssl_conn_mode);
            #endif
 
            if(res != SUCCESS)
            {
                LOG (m_handler, ERROR, "Download connect auth Unsuccessfully\r\n");

                if(ssl_conn_mode != 0)
                {
                    #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                    #endif
                }
                close(sockfd);

                dlclose(handle);
                return INTERNAL_ERROR;
            }
            LOG (m_handler, ERROR, "Download connect auth successfully\r\n");
        
            f = get_auth_info (recvbuf, auth_info, param->url, param->username, param->password);
            if (f != SUCCESS) {
                if (f == -2) {
                    LOG(m_handler, ERROR, "Basic authentication unsuccessfully\n");
                    if(ssl_conn_mode != 0)
                    {
                        #ifdef USE_SSL
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

                    close(sockfd);
                    dlclose(handle);
                    return FILE_TRAN_AUTH_FAIL;
                }
                else {
             	    LOG(m_handler, ERROR, "Get authorization information unsuccessfully\n");
                    if(ssl_conn_mode != 0)
                    {
                        #ifdef USE_SSL
                        destroy_ssl_res(&dl_ssl, &dl_ctx);
                        #endif
                    }

             	    close(sockfd);
                    dlclose(handle);
                    return INTERNAL_ERROR;
                }
            }
            sprintf(sdbuf,"GET %s HTTP/1.1\r\n", page);
            sprintf(data, "Host: %s\r\n", file_ser_name);
            strcat(sdbuf, data);
            strcat(sdbuf, "Accept: */*\r\n");
            strcat(sdbuf, "User-Agent: Mozilla/4.0\r\n");
            strcat(sdbuf, "Pragma: no-cache\r\n");
            strcat(sdbuf, "Cache-Control: no-cache\r\n");
            strcat(sdbuf, "Connection: close\r\n");
            
            strcat(sdbuf, auth_info);
            strcat(sdbuf, "\r\n\r\n\0");
            LOG(m_handler, DEBUG, "Now the sendbuf is\n%s\n", sdbuf);
            
            //Send
            #ifdef USE_SSL
                res = sock_send(&sockfd, (void **)(&dl_ssl), sdbuf, DOWN_TIME_OUT, ssl_conn_mode);
            #else
                res = sock_send(&sockfd, NULL, sdbuf, DOWN_TIME_OUT, ssl_conn_mode);
            #endif
 
            if(res != SUCCESS)
            {
                LOG (m_handler, ERROR, "Send unsuccessfully\r\n");
                if(ssl_conn_mode != 0)
                { 
                    #ifdef USE_SSL 
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                    #endif
                }

                close (sockfd);
                dlclose(handle);
                return INTERNAL_ERROR;
            }
            LOG (m_handler, DEBUG, "Send success!\r\n");
            
            //Receive
            while (1) {
                #ifdef USE_SSL
                    nrecv = sock_recv(&sockfd, (void**)(&dl_ssl), recvbuf, MAX_DATA_SIZE, DOWN_TIME_OUT , ssl_conn_mode);
                #else
                    nrecv = sock_recv(&sockfd, NULL, recvbuf, MAX_DATA_SIZE, DOWN_TIME_OUT, ssl_conn_mode);
                #endif

                switch(nrecv)
                {
                    case SUCCESS:
                        break;
                    case COMM_ERROR_RW:
                        LOG (m_handler, ERROR, "Recv data from file server unsuccessfully\r\n");
                        if(ssl_conn_mode != 0)
                        { 
                            #ifdef USE_SSL 
                            destroy_ssl_res(&dl_ssl, &dl_ctx);
                            #endif
                        }

                        close (sockfd);
                        dlclose(handle);
                        return INTERNAL_ERROR;

                    case COMM_ERROR_LIB:
                        if(ssl_conn_mode != 0)
                        { 
                            #ifdef USE_SSL 
                            destroy_ssl_res(&dl_ssl, &dl_ctx);
                            #endif
                        }

                        close (sockfd);
                        dlclose(handle);
                        return INTERNAL_ERROR;
                    case COMM_ERROR_FDSET:
                        LOG (m_handler, ERROR, "FD_ISSET error!\r\n");
                        if(ssl_conn_mode != 0)
                        { 
                            #ifdef USE_SSL 
                            destroy_ssl_res(&dl_ssl, &dl_ctx);
                            #endif
                        }

                        close (sockfd);
                        dlclose(handle);
                        return INTERNAL_ERROR;
                    case COMM_ERROR_TIMEOUT:
                        LOG (m_handler, DEBUG, "Receive error (maybe timeout).\r\n");
                        if(ssl_conn_mode != 0)
                        { 
                            #ifdef USE_SSL 
                            destroy_ssl_res(&dl_ssl, &dl_ctx);
                            #endif
                        }

                        close (sockfd);
                        dlclose(handle);
                        return DOWNLOAD_FAILURE;
                }

                if (j == 0) {
                    if (strstr(recvbuf, "200 OK") == NULL) {
                        if (strstr(recvbuf, "401 Unauthorized") != NULL) {
                            LOG(m_handler, ERROR, "Digest authentication unsccessfully.\n");
                            if(ssl_conn_mode != 0)
                            {
                                #ifdef USE_SSL
                                destroy_ssl_res(&dl_ssl, &dl_ctx);
                                #endif
                            }

                            close(sockfd);
                            dlclose(handle);
                            return FILE_TRAN_AUTH_FAIL;
                        }
                        else {
                            LOG(m_handler, ERROR, "Download file unsuccessfully.\n.");
                            if(ssl_conn_mode != 0)
                            {
                                #ifdef USE_SSL
                                destroy_ssl_res(&dl_ssl, &dl_ctx);
                                #endif
                            }

                            close(sockfd);
                            dlclose(handle);
                            return DOWNLOAD_FAILURE;
                        }
                    }
                    //ADD
                    //Judge if download firmware
                    if (strcmp(param->file_type, "1 Firmware Upgrade Image") == 0) {
                        dev_killallapps();
                        LOG(m_handler, DEBUG, "Done removing processes\n");
                    }
                    LOG(m_handler, DEBUG, "Allocating %d bytes buffer\n", param->file_size);

                    curptr = (char *)malloc(param->file_size);
                    if (curptr == NULL) {
                        LOG(m_handler, ERROR, "Failed to allocate memory for the image. size required : [%d].\n", param->file_size);
                        dlclose(handle);
                        return INTERNAL_ERROR;
                    }
                    imageptr = curptr;
                    LOG(m_handler, DEBUG, "Memory allocated %d\n", param->file_size);
                    //END ADD
                             
                    if(((p = strstr(recvbuf, "\r\n\r\n")) != NULL) || ((p = strstr(recvbuf, "\n\r\n\r")) != NULL)) {
                        p = p + 4;
                                                
                        n = nrecv-(p-recvbuf);
                        if(n != 0){
                            //strcpy(recvbuf, p);
                        }
                        recvbuf[MAX_DATA_SIZE] = '\0';
                    }
                    else {                                               
                        LOG(m_handler, ERROR, "Can't find the begining of data.\n");
                        if(ssl_conn_mode != 0)
                        {
                            #ifdef USE_SSL
                            destroy_ssl_res(&dl_ssl, &dl_ctx);
                            #endif
                        }

                        close(sockfd);
                        free(imageptr);
                        dlclose(handle);
                        return DOWNLOAD_FAILURE;
                    }
                    
                    //ADD
                    if (n != 0) {
                        memcpy(curptr, p, n);
                        curptr += n;
                        dl_len += n;
                        if (dl_len > param->file_size) {
                            LOG(m_handler, ERROR, "Filed over on size\n");
                            close(sockfd);
                            free(imageptr);
                            dlclose(handle);
                            return INTERNAL_ERROR;
                        }
                    }
                    //END ADD

                }
                else {   // j != 0
                    //ADD
                    memcpy(curptr, recvbuf, nrecv);
                    curptr += nrecv;
                    dl_len += nrecv;
                    if (dl_len > param->file_size) {
                        LOG(m_handler, ERROR, "Failed over on size\n");
                        close(sockfd);
                        free(imageptr);
                        dlclose(handle);
                        return INTERNAL_ERROR;
                    }
                    //END ADD
                }
                j = 1;

                LOG(m_handler, DEBUG, "The size of recv data is %d\n", nrecv);
                if(nrecv == 0)
                    break;                 
            }
        }
    }
                              
    close(fd);

    if(ssl_conn_mode != 0)
    {
        #ifdef USE_SSL
        destroy_ssl_res(&dl_ssl, &dl_ctx);
        #endif
    }

    close(sockfd);

    //ADD
    LOG(m_handler, DEBUG, "Download size is : %d\n", dl_len);
    
    //Parse Image Data
    if (strcmp(param->file_type, "1 Firmware Upgrade Image") == 0) {
        LOG(m_handler, DEBUG, "File type is 1 firmware upgrade image, will call function parse image data\n");
        imagetype = dev_parseimagedata(imageptr, dl_len, TRF_DOWNLOAD_IMAGE);
    }
    else {
        if (strcmp(param->file_type, "2 Web Content") == 0) {
            imagetype = dev_parseimagedata(imageptr, dl_len, TRF_DOWNLOAD_WEB);
        }
        else {
            if (strcmp(param->file_type, "3 Vendor Configuration File") == 0) {
                LOG(m_handler, DEBUG, "The file type is 3 vendor configuration file, will call parse image data function\n");
                imagetype = dev_parseimagedata(imageptr, dl_len, TRF_DOWNLOAD_SETTINGS);
            }
            else {
                LOG(m_handler, ERROR, "The invalid file type : %s\n", param->file_type);
                free(imageptr);
                dlclose(handle);
                return INVALID_ARGUMENT;
            }
        }
    }

    if (imagetype == NO_FORMAT) {
        LOG(m_handler, DEBUG, "Invalid file\n");
        free(imageptr);
        dlclose(handle);
        return DOWNLOAD_FAILURE;
    }
    dlclose(handle);

    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: trf_download
 * Description: call native funtion to download
 * Parameter: TR_download *down: the pointer point to the TR_download structure; time_t *start_time: the start time; time_t *comp_time: the complete time 
 * Return value: 0:SUCCESS  other:FAIL
 *********************************************************************
*/ 
int trf_download(TR_download *down, time_t *start_time, time_t *comp_time)
{
    int res;
    *start_time = dev_get_cur_time();
    //TODO: Add native function here/
    res = dev_download_file(down);
    LOG(m_handler, DEBUG, "The download return value is %d\n", res);
    //sleep(5);
    *comp_time = dev_get_cur_time();
	
    return res;
}
