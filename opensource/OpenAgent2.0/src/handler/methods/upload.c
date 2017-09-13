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
 * Include necessary files .....
 */

#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dlfcn.h>
#include "upload.h"
#include "../../auth/basic/base64.h"
#include "../../auth/auth.h"
#include "../soap/soap.h"
#include "../../tools/logger.h"
#include "../../comm/comm.h"

int dev_upload(char *filetype, char *up_url, char *username, char *passwd);

 /*
 ****************************************************************************
 * Function:    dev_upload_func
 * Dsecription: call device function get upload file value
 * Parameter:   char* buf             - buf to storage uplaod file value
 *              char dev_func_name[]  - device function name in this array
 * Return:      0                     - success
 *              -1                    - fail
 ****************************************************************************
 */

static int dev_upload_func(char* buf, char dev_func_name[])
{
    int res = -1;
    void *handle;
    int (*func)(char*, int);
    char *error;
    
    handle = dlopen (dev_lib_path, RTLD_LAZY);
    if (!handle) {
        LOG(m_handler, ERROR, "%s\n", dlerror());
        dlclose(handle);
        return res;
    }

    func = dlsym(handle, dev_func_name);
    if ((error = dlerror()) != NULL) {
        LOG(m_handler, ERROR,  "%s\n", error);
        dlclose(handle);
        return res;
    }
    //call vendor function
    res = (*func)(buf, UP_FILE_LENGTH);
    if (res != 0 ) {
        LOG(m_handler, ERROR, "call vendor upload function fail!\n");
        return res;
    }
    LOG(m_handler, DEBUG, "call vendor device func success!\n");
    dlclose(handle);
    return res ;
}

 /*
 *************************************************************************
 * Function:    get_ul_param get 
 * Description: get the parameter of the upload reques
 * Parameter:
 *     TRF_node *method       - pointer to the node of the method
 *     TR_upload *ul upload   - parameter structure which you want save param to it
 * Return:  SUCCESS           - success
 *          FAIL              -fail
 **************************************************************************
 */

static int get_ul_param(TRF_node *method, TR_upload *ul)
{
    method = method->child;
    
    while(method != NULL) {
        if (!strcmp(method->value.element.name, "CommandKey")) {
            if (method->child != NULL) {
                strncpy(ul->command_key, method->child->value.opaque, 32);
            } else {
                LOG(m_handler, DEBUG, "the CommandKey is empty!\n");
                strcpy(ul->command_key, "\0");
            }
            method = method->next;
            continue;
        }
        
        if (!strcmp(method->value.element.name, "FileType")) {
            if (method->child != NULL) {
                strncpy(ul->file_type, method->child->value.opaque,64);
            } else {
                LOG(m_handler, ERROR, "element child is NULL, fail!\n");
                return FAIL;
            }
            method = method->next;
            continue;
        }
        if (!strcmp(method->value.element.name, "URL")) {
            if (method->child != NULL) { 
                strncpy(ul->url, method->child->value.opaque,256);
            } else {
                LOG(m_handler, ERROR, "element child is NULL, fail!\n");
                return FAIL;
            }
            method = method->next;
            continue;
        }
        if (!strcmp(method->value.element.name, "Username")) {
            if (method->child != NULL) {
                strncpy(ul->username, method->child->value.opaque, 256);
            } else {
                LOG(m_handler, ERROR, "the username is empty!\n");
                strcpy(ul->username, "\0");
            }
            method = method->next;
            continue;
        }
        if (!strcmp(method->value.element.name, "Password")) {
            if (method->child != NULL) {
                strncpy(ul->password, method->child->value.opaque, 256);
            } else {
                LOG(m_handler, DEBUG, "the password is empty!\n");
                strcpy(ul->password, "\0");
            }
            method = method->next;
            continue;
        }
        if (!strcmp(method->value.element.name, "DelaySeconds")) {
            if (method->child != NULL) {
                ul->delay_seconds = atoi(method->child->value.opaque);
            } else {
                LOG(m_handler, ERROR, "element child is NULL, fail!\n");
                return FAIL;
            }
            method = method->next;
            continue;
        }
    }
    return SUCCESS;
}

 /*
 ********************************************************************
 * Function: gen_ul_resp_argu
 * Description: generate upload respond argument
 * Parameter:
 *     TR_upload_resp *ulr pointer the upload response argument
 *     TRF_node *method pointer to the node of the method
 * Return:
 *     success return SUCCESS , else return FAIL
 *********************************************************************
 */

static int gen_ul_resp_argu(TR_upload_resp *ulr, TRF_node *method)
{
    TRF_node *status, *st, *ct;
    char timebuf[20];
    
    //generate status node
    status = mxmlNewElement(method, "Status");
    if (!status) {
        return FAIL;
    }
    if (mxmlNewInteger(status, ulr->status) == NULL) {
        return FAIL;
    }

    //genarate starttime node 
    st = mxmlNewElement(method, "StartTime");
    if (!st) {
        return FAIL;
    }
    
    if (ulr->start_time != 0) {
        format_time(ulr->start_time, timebuf);
        if (mxmlNewText(st, 0, timebuf) == NULL) {
            return FAIL;
        }
    }

    //genarate completetime node
    ct = mxmlNewElement(method, "CompleteTime");
    if (!ct) {
        return FAIL;
    }
    
    if (ulr->complete_time != 0) {
        format_time(ulr->start_time, timebuf);
        if (mxmlNewText(ct, 0, timebuf) == NULL) {
            return FAIL;
        }
    }
    return SUCCESS;
}


 /*
 *****************************************************************
 * Function:    gen_ul_resp() 
 * Description: generate upload respond
 * Parameter:
 *     TR_upload_resp *ulr    - pointer the upload response argument
 *     TRF_node *xmlroot      - root node of xml
 * Return value:
 *     success return SUCCESS, else return FAIL
 ****************************************************************
 */ 

static int gen_ul_resp(TR_upload_resp *ulr)
{
    char name[] = "UploadResponse";
    TRF_node *method;
    int res;

    //generate envelope
    method = gen_soap_frame(name);
    if (method == NULL) {
        return FAIL;
    }
    
    res = gen_ul_resp_argu(ulr, method);
    if (res != 0) {
        return FAIL;
    }
    return SUCCESS;
}

 /*
 ***************************************************************
 * Function:    process_ul()
 * Description: process upload
 * Parameter:
 *     TRF_node *method      - pointer to the node of the method
 * Return value:
 *     success return SUCCESS, else return FAIL
 ***************************************************************
 */

int process_ul(TRF_node *method)
{
    int res;
    TR_upload *up;
    TR_upload_resp ulr;
    
    up = (TR_upload *)malloc(sizeof(TR_upload));
    if (up == NULL) {
        LOG(m_handler, ERROR, "Unable allocate memory for upload\n");
        if (gen_method_fault(xmlroot, "Upload", INTERNAL_ERROR) == NULL) {
            return FAIL;
        }
        return SUCCESS;
    }
    
    //Init upload struct
    memset(up, 0, sizeof(TR_upload));
    LOG(m_handler, DEBUG, "Get Upload param\n");

    get_ul_param(method, up);
    
    LOG(m_handler, DEBUG, "up.command_key : %s\nup.delay_seconds: %d\nGet Upload param finished\nup.file_type : %s\nup.password : %s\nup.url : %s\nup.username: %s\n", up->command_key,up->delay_seconds,up->file_type,up->password,up->url,up->username);
 
    //generate upload response(status is 1)
    ulr.status = 1;
    ulr.start_time = 0;
    ulr.complete_time = 0;
    
    res = gen_ul_resp(&ulr);
    if (res != 0) {
        LOG(m_handler, ERROR, "Generate upload response failure\n");
        return FAIL;
    }
    
    //Add trf_upload to task list
    res = add_task_list("trf_upload", up);
    if (res != 0) {
        free(up);
        return FAIL;
    }
    return SUCCESS;
}

 /*
 *******************************************************
 * Function:    trf_upload()
 * Description: tranfer upload
 * Parameter:   TR_upload *up
 *              time_t *starttime
 *              time_t *completetime
 * Return:
 *     success return 0, else return fault code
 ******************************************************
 */

int trf_upload(TR_upload *up, time_t *starttime, time_t *completetime)
{
    int res;
    
    *starttime = dev_get_cur_time();

    res = dev_upload(up->file_type, up->url, up->username, up->password);

    *completetime = dev_get_cur_time();
	
    return res;
}
                                                                                
 /*
 ******************************************************************************
 * Function:    get_digest_val()
 * Description: get digest value
 * Parameter:   char *dg
 *              TR_digest_auth *digest_au
 * Return:
 *     success return SUCCESS, else return FAIL
 ******************************************************************************/                                                                                
static int get_digest_val(char *dg, TR_digest_auth *digest_au)
{
    char *pc1, *pc2;
    char name[50];
    char val[500];
                                                                                
    pc2 = dg;
    pc1 = strstr(dg, "=");
                                                                                
    //This for loop get pszRealm,pszNonce,pszAlg,pszQop values
    while (*pc2 != '\n' && *pc2 != '\r') {
        memset(name, 0, sizeof(name));
        memset(val, 0, sizeof(val));
                                                                                
        //get parameter name
        while (*pc2 == ' ') {
            pc2++;
        }
        while (*(pc1 - 1)  == ' ') {
            pc1--;
       }
        strncpy(name, pc2, pc1 - pc2);
        LOG(m_handler, DEBUG, "digest parameter : %s\n", name);
                                                                                
        //get parameter value
        pc2 = strstr(pc1, "=");
        pc2++;
                                                                                
        while (*pc2 == ' ') {
            pc2++;
        }
        if (*pc2 == '"') {
            pc2++;
            pc1 = strstr(pc2, "\"");
            strncpy(val, pc2, pc1-pc2);
            pc1 = pc1 + 2;
        }
        else {
            pc1 = strstr(pc2, ",");
            strncpy(val, pc2, pc1-pc2);
            pc1 = pc1 + 1;
        }
                                                                                
        //Move pointer
        pc2 = pc1;
        while (*pc2 == ' ') {
            pc2++;
        }
        pc1 = strstr(pc2, "=");
                                                                                
        if (!strcmp(name, "realm")) {
            strcpy(digest_au->pszRealm, val);
            continue;
        }
        if (!strcmp(name, "nonce")) {
            strcpy(digest_au->pszNonce, val);
            continue;
        }
        if (!strcmp(name, "algorithm")) {
            strcpy(digest_au->pszAlg, val);
            continue;
        }
        if (!strcmp(name, "qop")) {
            strcpy(digest_au->pszQop, val);
            continue;
        }
                                                                                
    }
    strcpy(digest_au->pszCNonce, "58dde74d9b47ca4f8e410cb170dd47dc");
    strcpy(digest_au->szNonceCount, "00000001");
    strcpy(digest_au->pszMethod, "POST");
    return SUCCESS;
}
                                                                                
 /*
 *****************************************************************************
 * function base64en: encode string in base64
 * arguments:
 *      str     the string which will be encode in base64
 *      num     the length of string
 * return value:
 *      the enconded string in base64
 ****************************************************************************
 */

char * base64en(char *str)
{ 
    int num = 0, i = 0;
    char *base_str = NULL;
    
    num = strlen(str);
    i = num;
    if (num%3 !=0 ) {
        num = ((num/3) + 2)*4;
    } else
         num = (num/3) * 4;                                                        
    base_str = (char *)malloc(num*sizeof(char));
    if (base_str == NULL) {
        LOG(m_handler, ERROR, "Unable allocate memory for base64!\n");
        return NULL;
    }
    
    memset(base_str, 0, sizeof(base_str));
    Base64Enc((unsigned char *)base_str, (const unsigned char *)str, i);
                                                                                
    return base_str;
}

                                                                                
 /*
 *****************************************************************************
 * function dev_upload: upload a file to some URL
 * arguments:
 *      new_session     flag of whether to bulid a new session to upload
 *      filetyp         ACS's request file type
 *      up_url          upload's URL
 *      username        ACS's offered URL's user name
 *      passwd          user name's passwd
 * return value
 *      SUCCESS              error return
 *      FAIL                 upload success
 ****************************************************************************
 */

int dev_upload(char *filetype, char *up_url, char *username, char *passwd)
{
    int res = 0, base64_mark = -1;
    int sockfd, no_auth_mark = 1, nrecv = 0;
    char recvbuf[UP_MAX_DATA_SIZE];
    char port[256];
                                                                                  
    char file_content[UP_FILE_LENGTH], *base64_file = NULL, *basic64_cert = NULL;
    char content_length[8];
                                                                                
    char http_pkg[UP_FILE_LENGTH];
    char post_url[256];
    char file_ser_name[256], file_ser_ip[256];
    char input[128];
    char http_digest[2048];
    char *sp, *dg;
    TR_digest_auth *digest_au;

    int ssl_conn_flag;
    int ssl_conn_mode;
    int dl_no_port_flag;
    #ifdef USE_SSL
    LOG(m_handler, DEBUG, "SSL is call!\n");
    SSL *dl_ssl;
    SSL_CTX *dl_ctx;
    #endif
                                                                              
    char filetype_1[] = "1 Vendor Configuration File";
    char filetype_2[] = "2 Vendor Log File";
    
    memset(file_content, 0, sizeof(file_content));
                                                                            
    //judge wether need to authornication
    if(username[0] == '\0') {
        no_auth_mark = 0;
    }

    //get file name according to  filetype
    if (strcmp(filetype, filetype_1) == 0) {
        //get vendor config file
        res = dev_upload_func(file_content, get_upfig); 
        LOG(m_handler, DEBUG, "res :%d\n", res);
        if (res != 0 ) {
            LOG(m_handler, ERROR, "Get device config file fail!\n");
            return INTERNAL_ERROR;
        }
        LOG(m_handler, DEBUG, "Get device config file success!\n")
         #ifdef  BASE64
             base64_file = base64en(file_content);
             if (base64_file == NULL) {
                 return INTERNAL_ERROR;
             }
             base64_mark = 1;
             LOG(m_handler, DEBUG, "Upload Base64 encode!\n");
         #endif
    } else  if (strcmp(filetype, filetype_2) == 0) {
          LOG(m_handler, DEBUG, "FileType is %s\n", filetype_2);
          //get log file
          res = dev_upload_func(file_content, get_uplog);
          if (res != 0 ) {
              LOG(m_handler, ERROR, "Get device syslog fail!\n");
              return INTERNAL_ERROR;
          }
          LOG(m_handler, DEBUG, "Get device syslog success!\n")
          #ifdef  BASE64
              base64_file = base64en(file_content);      //free (base64_file)
              if (base64_file == NULL) {
                 return INTERNAL_ERROR;
              }
              base64_mark = 1;
	      LOG(m_handler, DEBUG, "Upload Base64 encode!\n");
          #endif                                                                      
    } else {
          LOG(m_handler, ERROR, "FileType is unvalid\n");
          return INVALID_ARGUMENT;
    }
                                                                                
    if(base64_mark ==1) {
	sprintf(content_length, "%d", strlen(base64_file));
    }
    else {
	sprintf(content_length, "%d", strlen(file_content));
    }

    res = dev_get_acs_ip_path(up_url, file_ser_name, file_ser_ip, port, post_url, &ssl_conn_flag, 1, &dl_no_port_flag);
    if (res != 0) {
         LOG (m_handler, ERROR, "Get file server ip and name unsuccessfully\n");
         return INTERNAL_ERROR;
    }
    LOG(m_handler, DEBUG, "The ssl flag is %d\n", ssl_conn_flag);
     
    //get http basic authentication header
    if (no_auth_mark) {
        sprintf(input, "%s:%s", username, passwd);
        LOG(m_handler, DEBUG, "Username password :%s\n", input);
        basic64_cert = base64en(input);     
        if (basic64_cert == NULL) {
            LOG(m_handler, ERROR, "encode basic64_cert into base64 fail!\n");
            return INTERNAL_ERROR;
        }
    }
                                                         
    //generate http package
    strcpy(http_pkg, "POST ");
    strcat(http_pkg, post_url);
    strcat(http_pkg, " HTTP/1.1\r\nHost: ");
    strcat(http_pkg, file_ser_name);
    strcat(http_pkg, "\r\nUser-Agent: Mozilla/5.0\r\n");
    if (base64_mark == 1)
        strcat(http_pkg, "Content-Transfer-Encoding: base64\r\n");
    strcat(http_pkg, "Connection:keep-alive\r\n");
    strcat(http_pkg, "Content-Type: text/plain\r\n");
    strcat(http_pkg, "Content-Length:");
    strcat(http_pkg, content_length);
    strcat(http_pkg, "\r\n");
                                                                                
    if (no_auth_mark == 1) {
        strcat(http_pkg, "Authorization: Basic ");
        strcat(http_pkg, basic64_cert);
        strcat(http_pkg, "\r\n");
    }
                                                                                
    strcat(http_pkg, "\r\n");
    if (base64_mark ==1)
        strcat(http_pkg, base64_file);
    else
        strcat(http_pkg, file_content);
                                                                                
    strcat(http_pkg, "\r\n");
    LOG(m_handler, DEBUG, "send:\n%s\n", http_pkg);                                                                            
    //build a new session
    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) {
        LOG(m_handler, ERROR, "Socket error\n");
        return INTERNAL_ERROR;
    }
     
                                                                                
    #ifdef USE_SSL
        res = connect_auth(file_ser_ip, atoi(port), &sockfd, (void **)(&dl_ssl), (void **)(&dl_ctx), ca_cert_path, client_cert_path, client_key_path, &ssl_conn_flag, &ssl_conn_mode, &dl_no_port_flag);
    #else
        res = connect_auth(file_ser_ip, atoi(port), &sockfd, NULL, NULL, NULL, NULL, NULL, &ssl_conn_flag, &ssl_conn_mode
, &dl_no_port_flag); 
    #endif
    
    LOG(m_handler, DEBUG, "Upload flag is %d, conn_mode is %d\n", ssl_conn_flag, ssl_conn_mode);
 
    if (res != 0) {
        LOG (m_handler, ERROR, "Upload connect auth Unsuccessfully!\n");
        if (ssl_conn_mode != 0) {
            #ifdef USE_SSL
                destroy_ssl_res(&dl_ssl, &dl_ctx);
            #endif
        }
        return INTERNAL_ERROR;
    }
    LOG (m_handler, ERROR, "Upload connect auth successfully!\n");                                                                            
    //Send
    #ifdef USE_SSL
        res = sock_send(&sockfd, (void **)(&dl_ssl), http_pkg, UP_TIME_OUT, ssl_conn_mode);
    #else
        res = sock_send(&sockfd, NULL, http_pkg, UP_TIME_OUT, ssl_conn_mode);
    #endif

    free(basic64_cert);
    #ifdef BASE64
        free(base64_file);
    #endif

    if (res != 0) {
        LOG (m_handler, ERROR, "Send unsuccessfully!\n");
        if (ssl_conn_mode != 0) {
            #ifdef USE_SSL
                destroy_ssl_res(&dl_ssl, &dl_ctx);
            #endif
        }
        close (sockfd);
        return INTERNAL_ERROR;
    }
    LOG (m_handler, DEBUG, "Send success!\n");
        
    //Recevie
    #ifdef USE_SSL
        nrecv = sock_recv(&sockfd, (void**)(&dl_ssl), recvbuf, UP_MAX_DATA_SIZE, UP_TIME_OUT , ssl_conn_mode);
    #else
      nrecv = sock_recv(&sockfd, NULL, recvbuf, UP_MAX_DATA_SIZE, UP_TIME_OUT, ssl_conn_mode);
    #endif

    switch(nrecv) {
        case SUCCESS:
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            break;

        case COMM_ERROR_RW:
            LOG (m_handler, ERROR, "Recv data from file server unsuccessfully!\n");
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            close (sockfd);
            return UPLOAD_FAILURE;

        case COMM_ERROR_LIB:
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            close (sockfd);
            return INTERNAL_ERROR;

        case COMM_ERROR_FDSET:
            LOG (m_handler, ERROR, "FD_ISSET error!\n");
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            close (sockfd);
            return INTERNAL_ERROR;

        case COMM_ERROR_TIMEOUT:
            LOG (m_handler, DEBUG, "Receive error (maybe timeout)!\n");
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            close (sockfd);
            return UPLOAD_FAILURE;
    }

    LOG(m_handler, DEBUG, "receive data\n%s\n", recvbuf);
    if (strstr(recvbuf, "401 Unauthorized") != NULL) {
        if (ssl_conn_mode != 0) {
            #ifdef USE_SSL
                destroy_ssl_res(&dl_ssl, &dl_ctx);
            #endif
        }
        close(sockfd);
    } else {
          if (strstr(recvbuf, "200 OK") == NULL) {
              LOG(m_handler, ERROR, "Upload file unsuccessfully\n.");
              if (ssl_conn_mode != 0) {
                  #ifdef USE_SSL
                      destroy_ssl_res(&dl_ssl, &dl_ctx);
                  #endif
              }
              close (sockfd);
              return UPLOAD_FAILURE;
          }
          LOG(m_handler, DEBUG, "Upload success and http code is 200 OK!");
          if (ssl_conn_mode != 0) {
              #ifdef USE_SSL
                  destroy_ssl_res(&dl_ssl, &dl_ctx);
              #endif
          }
          close(sockfd);
          return SUCCESS;
     }
  
    //if receive 401 , try upload with http digest authornication                                                            
    digest_au = (TR_digest_auth *)malloc(sizeof(TR_digest_auth));
    if (digest_au == NULL) {
        LOG(m_handler, DEBUG, "malloc memory failed\n");
        return INTERNAL_ERROR;
    }
          
    memset(digest_au, 0, sizeof(TR_digest_auth));                                                                         
    //get digest authentication
    sp = strstr(recvbuf, "WWW-Authenticate");
    if (sp == NULL) {
        LOG(m_handler, ERROR, "WWW-Authenticate: can't find WWW-Authenticate\n");
        return INTERNAL_ERROR;
    }
    sp = strtok(sp, " ");
    sp = strtok(NULL, " ");
                                                                                
    dg = strtok(NULL, "");
    dg = strtok(dg, "\r\n");
                                                                                
    if (strcmp(sp, "Digest") != 0) {
        LOG(m_handler, DEBUG, "auth failed\n");
        return FILE_TRAN_AUTH_FAIL;
    }
                                                                                
    //get digest authcation value
    res = get_digest_val(dg, digest_au);
    if (res != 0) {
        LOG(m_handler, ERROR, "Get digest parameter values failed\n");
        return INTERNAL_ERROR;
    }
    strcpy(digest_au->pszUser, username);
    strcpy(digest_au->pszPass, passwd);
    strcpy(digest_au->pszURI, post_url);
                                                                                
    get_digest_auth_info(digest_au, http_digest);
    LOG(m_handler, DEBUG, "calculate the digest response success!\n");

    //generate http package
    strcpy(http_pkg, "POST ");
    strcat(http_pkg, post_url);
    strcat(http_pkg, " HTTP/1.1\r\nHost: ");
    strcat(http_pkg, file_ser_name);
    strcat(http_pkg, "\r\nUser-Agent: gSOAP/2.7\r\n");
    if (base64_mark == 1)
        strcat(http_pkg, "Content-Transfer-Encoding: base64\r\n");
    strcat(http_pkg, "Connection:keep-alive\r\n");
    strcat(http_pkg, "Content-Type: text/plain; charset=utf-8\r\n");
    strcat(http_pkg, "Content-Length: ");
    strcat(http_pkg, content_length);
    strcat(http_pkg, "\r\n");
    strcat(http_pkg, http_digest);
    strcat(http_pkg, "\r\n\r\n");
                                                                                
    if (base64_mark == 1)
        strcat(http_pkg, base64_file);
    else
        strcat(http_pkg, file_content);
                                                                                
    //first authentication failed, try http digest authentication
    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) {
        LOG(m_handler, DEBUG,"Socket error\n");
        return INTERNAL_ERROR;
    }
         
    #ifdef USE_SSL
        res = connect_auth(file_ser_ip, atoi(port), &sockfd, (void **)(&dl_ssl), (void **)(&dl_ctx), ca_cert_path, client_cert_path, client_key_path, &ssl_conn_flag, &ssl_conn_mode, &dl_no_port_flag);
    #else
        res = connect_auth(file_ser_ip, atoi(port), &sockfd, NULL, NULL, NULL, NULL, NULL, &ssl_conn_flag, &ssl_conn_mode
, &dl_no_port_flag);
    #endif

    LOG(m_handler, DEBUG, "Upload flag is %d, conn_mode is %d\n", ssl_conn_flag, ssl_conn_mode);

    if (res != 0) {
        LOG (m_handler, ERROR, "Upload connect auth Unsuccessfully!\n");
        if (ssl_conn_mode != 0) {
            #ifdef USE_SSL
                destroy_ssl_res(&dl_ssl, &dl_ctx);
            #endif
        }
        return INTERNAL_ERROR;
    }
    LOG (m_handler, ERROR, "Upload connect auth successfully!\n");                                                          
    
    LOG(m_handler, DEBUG, "send data\n%s\n",http_pkg);
    //Send
    #ifdef USE_SSL
        res = sock_send(&sockfd, (void **)(&dl_ssl), http_pkg, UP_TIME_OUT, ssl_conn_mode);
    #else
        res = sock_send(&sockfd, NULL, http_pkg, UP_TIME_OUT, ssl_conn_mode);
    #endif
    
    #ifdef BASE64
        free(base64_file);
    #endif

    if (res != 0) {
        LOG (m_handler, ERROR, "Send unsuccessfully!\n");
        if (ssl_conn_mode != 0) {
            #ifdef USE_SSL
                destroy_ssl_res(&dl_ssl, &dl_ctx);
            #endif
        }
        close (sockfd);
        return INTERNAL_ERROR;
    }
    LOG (m_handler, DEBUG, "Send success!\n");
    //Recevie
    #ifdef USE_SSL
        nrecv = sock_recv(&sockfd, (void**)(&dl_ssl), recvbuf, UP_MAX_DATA_SIZE, UP_TIME_OUT , ssl_conn_mode);
    #else
        nrecv = sock_recv(&sockfd, NULL, recvbuf, UP_MAX_DATA_SIZE, UP_TIME_OUT, ssl_conn_mode);
    #endif
    switch(nrecv) {
        case SUCCESS:
            break;

        case COMM_ERROR_RW:
            LOG (m_handler, ERROR, "Recv data from file server unsuccessfully!\n");
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            close (sockfd);
            return UPLOAD_FAILURE;

        case COMM_ERROR_LIB:
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            close (sockfd);
            return INTERNAL_ERROR;

        case COMM_ERROR_FDSET:
            LOG (m_handler, ERROR, "FD_ISSET error!\n");
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            close (sockfd);
            return INTERNAL_ERROR;

        case COMM_ERROR_TIMEOUT:
            LOG (m_handler, DEBUG, "Receive error (maybe timeout)!\n");
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            close (sockfd);
            return UPLOAD_FAILURE;
    }

    LOG(m_handler, DEBUG, "receive data\n%s\n", recvbuf);
    if (strstr(recvbuf, "200 OK") == NULL) {
        if (strstr(recvbuf, "401 Unauthorized") != NULL) {
            LOG(m_handler, ERROR, "Digest authentication unsccessfully!\n");
            if (ssl_conn_mode != 0) {
                #ifdef USE_SSL
                    destroy_ssl_res(&dl_ssl, &dl_ctx);
                #endif
            }
            close(sockfd);
            return FILE_TRAN_AUTH_FAIL;
        } else {
              LOG(m_handler, ERROR, "Upload file unsuccessfully!\n");
              if (ssl_conn_mode != 0) {
                  #ifdef USE_SSL
                      destroy_ssl_res(&dl_ssl, &dl_ctx);
                  #endif
              }
              close(sockfd);
              return UPLOAD_FAILURE;
        }
    } else {
          LOG(m_handler, DEBUG, "Upload success and http code is 200 OK!\n");
          close(sockfd);
    }

    if (ssl_conn_mode != 0) {
        #ifdef USE_SSL
            destroy_ssl_res(&dl_ssl, &dl_ctx);
        #endif
    }
    return SUCCESS;
}



