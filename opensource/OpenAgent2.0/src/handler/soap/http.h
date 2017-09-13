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


#ifndef HTTP_H_
#define HTTP_H_

#include "../../res/global_res.h"
#include "../../auth/auth.h"
#include "soap.h"


/*
 * Define http header value
 */
#define HTTP_METHOD     "POST"
#define HTTP_VERSION    "HTTP/1.1"
#define USER_AGENT      "TR069_AGENT/1.0"
#define CONTENT_LENGTH  "      "
#define CONNECTION      "keep-alive"
#define CONN_TYPE       "text/xml; charset=utf-8"
#define SOAP_ACTION     ""


/*
 * Define global constants ......
 */
#define STR_NC_VALUE_LEN       8
#define STR_CONTENT_LENGTH_LEN 6
#define HTTP_HEAD_LINE_LEN     512
#define SUPPORT_MAX_COOKIE_NUM 20
#define STR_COOKIE_INFO_LEN    512
#define STR_COOKIE_NAME_LEN    64
#define STR_COOKIE_VALUE_LEN   64
#define STR_COOKIE_DOMAIN_LEN  128
#define STR_COOKIE_PATH_LEN    128
#define STR_COOKIE_PORT_LEN    128 

#define ACS_USERNAME_LEN       256
#define ACS_PASSWORD_LEN       256

/*
 * Define return values .......
 */
#define HTTP_GEN_SUCCESS 0
#define HTTP_GEN_FAILED  -1

/*
 * Define Global enum ......
 */
typedef enum {
    NO_AUTH,
    BASIC,
    DIGEST
}AUTH_STATUS;

typedef enum {
    STATUS_OK,
    STATUS_NEED_AUTH,
    STATUS_NEED_REDIRECTD,
    STATUS_NO_CONTENT,
    CONTENT_NOT_SUPPORT,
    PROTOCOL_NOT_SUPPORT,
    UNKNOW_STATUS_CODE,
    FORMAT_ERROR,
    PARSER_ERROR
}HTTP_PARSE_RESULT;

/*
 * Define Global structer
 */
typedef struct {
    char name[STR_COOKIE_NAME_LEN];
    char value[STR_COOKIE_VALUE_LEN];
    char path[STR_COOKIE_PATH_LEN];
    char domain[STR_COOKIE_DOMAIN_LEN];
    char port[STR_COOKIE_PORT_LEN];
}TR_session_cookie;

/*
 * Define global variable ....
 */
AUTH_STATUS auth_status;
unsigned short nonce_count;
unsigned short have_cookie_flag;
unsigned short cookie_discard_flag;
unsigned short cookie_version;
TR_session_cookie session_cookie_array[SUPPORT_MAX_COOKIE_NUM];
TR_digest_auth digest_auth;


/*
 * Declear funtion ..........
 */
extern int get_line(char *s, int size, char *stream);
extern int save_to_sendbuf(TRF_node *xmlroot, char *sendbuf);
extern HTTP_PARSE_RESULT parse_http_header(char *recv_buf);

#endif /* HTTP_H_ */

