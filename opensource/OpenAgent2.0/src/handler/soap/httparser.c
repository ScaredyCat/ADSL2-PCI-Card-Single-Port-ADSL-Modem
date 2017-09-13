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
 * Include file ......
 */
#include "http.h"
#include "../methods/methods.h"
#include "../../device/TRF_param.h"
#include "../../tools/agent_conf.h"
/*
 * Decleation function ........
 */
static HTTP_PARSE_RESULT parse_auth_info(char *auth_buf, TR_digest_auth *pdigest_auth);
//static inline int get_line(char *s, int size, char *stream);
static HTTP_PARSE_RESULT parse_cookie(char *cookie_buf);

/*
 *******************************************************************
 * Function: parse_http_header
 * Description: parse http head get the head information
 * Parameter: 
 *        char *recv_buf point to the string of recv buffer
 * Return Value:
 *        return parse status
 *******************************************************************
 */
 
HTTP_PARSE_RESULT parse_http_header(char *recv_buf)
{
    char line[HTTP_HEAD_LINE_LEN], protocol[6], status[4], title[10];
    int status_code, cl;
    HTTP_PARSE_RESULT ret = PARSER_ERROR;

    //init variable
    memset(line, 0, sizeof(line));
    memset(protocol, 0, sizeof(protocol));
    memset(status, 0, sizeof(status));
    memset(title, 0, sizeof(title));

    
    //parse the first line for http response
    if (get_line(line, sizeof(line), recv_buf) == 0) {
        return ret;
    }

    if (sscanf(line, "%s %s %s", protocol, status, title) != 3) {
        return ret;
    }
    
    //get http response status code
    status_code = atoi(status);
    
    //change the pointer
    recv_buf = strchr(recv_buf, '\n');
    if (recv_buf == NULL) {
        return ret;
    }
    recv_buf++;
    
    //parse the rest of header
    switch (status_code) {
        case 200:
            ret = STATUS_OK;
            while (get_line(line, sizeof(line), recv_buf) != 0) {
                if (strncasecmp(line, "Content-Type:", 13) == 0) {
                    if (strstr(line, "text/xml") == NULL) {
                        ret = CONTENT_NOT_SUPPORT;
                        break;
                    }
                } else if (strncasecmp(line, "Content-Length:", 15) == 0) {
                    cl = atoi(line + 15);
                    if (cl == 0) {
                        ret = STATUS_NO_CONTENT;
                        break;
                    }
                } else if (strncasecmp(line, "Set-Cookie2:", 12) == 0) {
                    have_cookie_flag = 1;
                    if (parse_cookie(line + 12) != 0) {
                        ret = FORMAT_ERROR;
                        break;
                    }
                }
                recv_buf = strchr(recv_buf, '\n');
                if (recv_buf == NULL) {
                    ret = PARSER_ERROR;
                    break;
                }
                recv_buf++;
            }
            break;
        case 204:
           ret = STATUS_NO_CONTENT;
           break;
        case 401:
            while (get_line(line, sizeof(line), recv_buf) != 0) {
                if (strncasecmp(line, "WWW-Authenticate:", 17) == 0) {
                    //get auth infomation
                    memset(&digest_auth, 0, sizeof(digest_auth));
                    ret = parse_auth_info(line + 17, &digest_auth);
                    break;
                }
                recv_buf = strchr(recv_buf, '\n');
                if (recv_buf == NULL) {
                    ret = PARSER_ERROR;
                    break;
                }
                recv_buf++;
            }
            break;
        case 301:
            //TODO
            break;
        case 302:
            //TODO
            break;
        case 307:
            while (get_line(line, sizeof(line), recv_buf) != 0) {
                if (strncasecmp(line, "Location:", 9) == 0) {
                    //get location infomation
                    //TODO
                    ret = STATUS_NEED_REDIRECTD;
                    break;
                }
                recv_buf = strchr(recv_buf, '\n');
                if (recv_buf == NULL) {
                    ret = PARSER_ERROR;
                    break;
                }
                recv_buf++;
            }
            break;
        default:
            ret = UNKNOW_STATUS_CODE;
            break;
    }
    
    return ret;
}

/*
 ************************************************************************
 * Function: get_line
 * Description: get a line from a string stream
 * Parameter: 
 *     char *s - used to store string line
 *     int size - the max length of sting which will be copy
 *     char stream - the source string
 * Return Value:
 *     the length of string s
 ************************************************************************
 */

int get_line(char *s, int size, char *stream)
{
    int i = 0;
    char *p;

    p = s;
    for(;;) {
        if (size == 0) {
            *p = '\0';
            break;
        }
        size--;
        if (stream[i] == '\r' || stream[i] == '\n') {
            *p = '\0';
            break;
        }
        *(p++) = stream[i];
        i++;
    }

    return strlen(s);
}

/*
 ***********************************************************************
 * Function:  parse_auth_info()
 * Description: get authcation information from http response
 * Return Value:
 *     return PARSE RESULT
 ***********************************************************************
 */
static HTTP_PARSE_RESULT parse_auth_info(char *auth_buf, TR_digest_auth *pdigest_auth)
{
    char *p;
    HTTP_PARSE_RESULT ret = PARSER_ERROR;
    //int res = 0, locate[4];
    
    //delete space
    while (*auth_buf == ' ') {
        auth_buf++;
    }
    
    //get auth type
    if (memcmp(auth_buf, "Digest", 6) == 0) {
        auth_status = DIGEST;
        auth_buf += 6;
    } else if (memcmp(auth_buf, "Basic", 5) == 0) {
        auth_status = BASIC;
        ret = STATUS_NEED_AUTH;
        return ret;
    } else {
        return ret;
    }
    
    //get auth info from http response(realm/qop/nonce/algorithm)
    while (*auth_buf != '\0') {
        
        //delete space
        if (*auth_buf == ' ') {
            auth_buf++;
            continue;
        }
        
        p = strchr(auth_buf, ',');
        if (p == NULL) {
            p = strchr(auth_buf, '\0');
        }
        if (memcmp(auth_buf, "realm=\"", 7) == 0) {
            auth_buf += 7;
            strncpy(pdigest_auth->pszRealm, auth_buf, p - 1 - auth_buf);
        } else if (memcmp(auth_buf, "qop=\"", 5) == 0) {
            auth_buf += 5;
            strncpy(pdigest_auth->pszQop, auth_buf, p - 1 - auth_buf);
        } else if (memcmp(auth_buf, "nonce=\"", 7) == 0) {
            auth_buf += 7;
            strncpy(pdigest_auth->pszNonce, auth_buf, p - 1 - auth_buf );
        } else if (memcmp(auth_buf, "algorithm=\"", 11) == 0) {
            auth_buf += 11;
            strncpy(pdigest_auth->pszAlg, auth_buf, p - 1 - auth_buf);
        }
        
        if (*p == '\0') {
            break;
        }
        auth_buf = ++p;
        
    }
    //get pszCNonce value
    strcpy(pdigest_auth->pszCNonce, "58dde74d9b47ca4f8e410cb170dd47dc");
    
    //get user name
    /*res = call_dev_func(acsuser_dev_func, GET_OPT, pdigest_auth->pszUser, locate);
    if (res != GET_VAL_SUCCESS) {
        return ret;
    }
    LOG(m_handler, DEBUG, "username : %s\n", pdigest_auth->pszUser);
    //get password value
    res = call_dev_func(acspwd_dev_func, GET_OPT, pdigest_auth->pszPass, locate);
    if (res != GET_VAL_SUCCESS) {
        return ret;
    }
    LOG(m_handler, DEBUG, "Password : %s\n", pdigest_auth->pszPass);
    */
    /*agent_conf a_conf;
    if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
        return ret;
    }
    strcpy(pdigest_auth->pszUser, a_conf.acs_username);
    strcpy(pdigest_auth->pszPass, a_conf.acs_password);
    */
    
    //get szNonceCount value
    strcpy(pdigest_auth->szNonceCount, "00000001");
    
    //get pszMethod value
    strcpy(pdigest_auth->pszMethod, "POST");
    
    //get pszURI value
    strcpy(pdigest_auth->pszURI, acs_path);

    ret = STATUS_NEED_AUTH;
    
    return ret;
}

/*
 *********************************************************************************
 * Function name: parse_cookie()
 * Description: Parse http cookie "Set-Cookie2"
 * Parameter: 
 *     char *cookie_buf - pointer to cookie infomation
 * Return value: 
 *     return parser status
 *********************************************************************************
 */
static HTTP_PARSE_RESULT parse_cookie(char *cookie_buf)
{
    char *p;
    TR_session_cookie cookie;
    HTTP_PARSE_RESULT ret = FORMAT_ERROR;
    char version[4];
    int i;
    
    //init valiable
    memset(&cookie, 0, sizeof(TR_session_cookie));
     
    //delete space
    //p = cookie_buf;
    while (*cookie_buf == ' ') {
        cookie_buf++;
    }

    //cookie_buf = p;
    //get cookie name and value
    p = strchr(cookie_buf, '=');
    if (p == NULL) {
        return ret;
    }
    strncpy(cookie.name, cookie_buf, p - cookie_buf);
    cookie_buf = ++p;
    p = strchr(cookie_buf, ';');
    if (p == NULL) {
        return ret;
    } else {
        strncpy(cookie.value, cookie_buf, p - cookie_buf);
        cookie_buf = p;
    }
    
    //get cookie av path / domain / port /discard / Max-Age / version /secure
    while (*cookie_buf != '\0') {
        cookie_buf++;
        if (*cookie_buf == ' ') {
            cookie_buf++;
            continue;
        }
        p = strchr(cookie_buf, ';');
        if (p == NULL) {
            p = strchr(cookie_buf, '\0');
        }
        if (memcmp(cookie_buf, "Version=", 8) == 0) {
            cookie_buf += 8;
            strncpy(version, cookie_buf, p - cookie_buf);
            cookie_version = atoi(version);
        } else if (memcmp(cookie_buf, "Domain=", 7) == 0) {
            cookie_buf += 7;
            strncpy(cookie.domain, cookie_buf, p - cookie_buf);
        } else if (memcmp(cookie_buf, "Path=", 5) == 0) {
            cookie_buf += 5;
            strncpy(cookie.path, cookie_buf, p - cookie_buf);
        } else if (memcmp(cookie_buf, "Port=", 5) == 0) {
            cookie_buf += 5;
            strncpy(cookie.port, cookie_buf, p - cookie_buf);
        } else if (memcmp(cookie_buf, "Discard", 7) == 0) {
            cookie_discard_flag = 1;
        }
        cookie_buf = p;

    }
    
    //compare the name domain path , store the info to cache
    for (i = 0; i < SUPPORT_MAX_COOKIE_NUM; i++) {
        if (!strcmp(session_cookie_array[i].name, cookie.name)
            && !strcmp(session_cookie_array[i].domain, cookie.domain)
            && !strcasecmp(session_cookie_array[i].path, cookie.path)) {
            //then new cookie supersedes the old
            break;
        } else if (!strcmp(session_cookie_array[i].name, "")) {
            break;
        }
    }
    //then store the information
    if (i < SUPPORT_MAX_COOKIE_NUM) {
        strcpy(session_cookie_array[i].name, cookie.name);
        strcpy(session_cookie_array[i].value, cookie.value);
        strcpy(session_cookie_array[i].domain, cookie.domain);
        strcpy(session_cookie_array[i].path, cookie.path);
        strcpy(session_cookie_array[i].port, cookie.port);
        ret = STATUS_OK;
    } else {
        //TODO
        LOG(m_handler, DEBUG, "MAX COOKIE NUM is overflow\n");
    }
    
    return ret;
}

/*
 ***********************************************************
 * Function: check_termination
 * Description: 
 * Return Value:
 ***********************************************************
 */
 
int check_termination()
{
    //add code
    return 1;
}

