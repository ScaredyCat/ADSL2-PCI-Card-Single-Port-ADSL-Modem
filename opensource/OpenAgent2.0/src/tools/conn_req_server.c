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
 *********************************************************************
 *
 *	conn_req_server.c - Connection Request Server in Agent
 *
 *	$Author: jasonw $
 *
 *	history: $Date: 2007-06-08 02:22:16 $, initial version by simonl
 *
 **********************************************************************
 */

/*
 * Include files .....
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include "../res/global_res.h"
#include "../CLI/CLI.h"
#include "../auth/digest/digcalc.h"
#include "../handler/soap/http.h"
#include "agent_conf.h"

/*
 * macrodefinition ......
 */

#ifndef BACKLOG
    #define BACKLOG 5
#endif

#ifndef BUFFER_SIZE
    #define BUFFER_SIZE 1024
#endif

#ifndef SHORT_SIZE
    #define SHORT_SIZE 256
#endif

/*
 * Global var define .....
 */

char *s_realm = "DigAuth";
char s_nonce[64] = "";
char *s_algorithm = "MD5";
char *s_qop = "auth";
char s_date[32];
char *s_domain = "/";
char *s_display_text = "Connection Request OK";
int  F_auth = 1;


/*
 * Function declare
 */
static void *conn_req_ser();
static int str_process(int sockfd);
static int verify_conn_req(int sockfd);
static int parse_first_line(char *p_line);
static int parse_authorization_line(char *p_line, char *buf);
static void generate_auth_pack(char *buf);
static void generate_nonce();
static void get_date();
static int call_sys_cmd(char *cmd);

/*
 ***********************************************************************
 * Function: init_CRS
 * 
 *
 ***********************************************************************
 */
int init_CRS()
{
    int res = -1;
    pthread_attr_t thread_attr;
    pthread_t p_CRS;
    
    //init thread attr
    if (pthread_attr_init(&thread_attr) == 0) {
        LOG(m_handler, DEBUG, "pthread_attr_init success.\n");

        //set thread attr
        if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) == 0) {
            LOG(m_handler, DEBUG, "pthread_attr_setdetachstate success.\n");

            //Creat thread
            if (pthread_create(&p_CRS, &thread_attr, conn_req_ser, NULL) == 0) {
                LOG(m_handler, DEBUG, "Init CRS successful\n");
                res = SUCCESS;
            } else {
                switch(errno) {
                    case EAGAIN:
                        LOG(m_handler, ERROR, "pthread_create(p_CRS) failed, show cause: ? too much thread numbers.\n");
                        break;
                    case EINVAL:
                        LOG(m_handler, ERROR, "pthread_create(p_CRS) failed, show cause: ? thread id illegality.\n");
                        break;
                    default :
                        LOG(m_handler, ERROR, "pthread_create(p_CRS) failed, show cause: ? unknown.\n");
                        break;
                }
            }
        } else {
            LOG(m_handler, ERROR, "pthread_attr_setdetachstate Failed.\n");
        }
    } else {
        LOG(m_handler, ERROR, "pthread_attr_init failed .\n");
    }

    return res; 
}

/*
 ************************************************************************
 * Function: conn_req_server
 * Description: connected 
 * Parameter: 
 * Return Values: 
 *     
 ************************************************************************
 */
static void *conn_req_ser()
{
    int	listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_in	cliaddr, servaddr;
    char cmd[128]; 
    //init_conf_path();
    //init_logger();

    //call device function to get request server port
    /*res = call_dev_func(get_req_port_func, GET_OPT, conn_req_port, locate);
    if (res != GET_VAL_SUCCESS) {
        LOG(m_tools, ERROR, "get port failed\n");
        pthread_exit(NULL);
    }*/
    agent_conf a_conf;
    if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
        pthread_exit(NULL);
    }
    //set firewall
    sprintf(cmd, "iptables -I INPUT 1 -j ACCEPT -i eth1 -p tcp --dport %d",
                   a_conf.conn_req_ser_port);
    if (call_sys_cmd(cmd) != 0) {
        pthread_exit(NULL);
    }

    /*sprintf(cmd, "iptables -I FORWARD 1 -j ACCEPT -i eth1 -p tcp --dport %s",
                   conn_req_port);
    if (TR_sys_cmd(cmd) == -1) {
        return -1;
    }*/
    
    //create socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        LOG(m_tools, ERROR, "Create listen socket fd error.\n");
        pthread_exit(NULL);
    }
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(a_conf.conn_req_ser_port);

    /*
    if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) == -1) {
        LOG(m_tools, ERROR, "Bind listen socket error.\n");
        pthread_exit(NULL);
    }*/

    //This loop for bind error
    for (;;) {
        if ((bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) == -1) {
            LOG(m_tools, ERROR, "Bind listen socket error.\n");
            //pthread_exit(NULL);
            sleep(3);
            continue;
        }
        break;
    }

    if((listen(listenfd, BACKLOG)) == -1) {
        LOG(m_tools, ERROR, "listening on socket error.\n");
        pthread_exit(NULL);
    }

    clilen = sizeof(cliaddr);

    for ( ; ; ) {
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            if (errno == EINTR) {
                continue;		/* back to for() */
            } else {
                LOG(m_tools, ERROR, "accept error.\n");
                pthread_exit(NULL);
            }
        }

        if ((str_process(connfd)) != 0)	{ /* process the request */
            LOG(m_tools, ERROR, "child invoke str_process() failed.\n");
        } else {
            LOG(m_tools, DEBUG, "child invoke str_process() success.\n");
        }
        
        close(connfd);
    }

    return NULL;
}

/*
 **********************************************************************
 *
 *
 *
 **********************************************************************
 */
static int call_sys_cmd(char *cmd)
{
    int ret = -1;
    void *handle;
    int (*dev_func)(char *);
    const char *error;

    handle = dlopen(dev_lib_path, RTLD_LAZY);
    if (!handle) {
        LOG(m_tools, ERROR, "dlopen dll failed\n");
        return ret;
    }

    dev_func = dlsym(handle, dev_sys_cmd);
    if ((error = dlerror()) != NULL) {
        LOG(m_tools, ERROR, "ERROR : %s\n", error);
        dlclose(handle);
        return ret;
    }

    ret = dev_func(cmd);

    if (dlclose(handle) != 0) {
        LOG(m_handler, ERROR, "Close handle failed\n");
        ret = -1;
    }

    return ret;
}

/*
 ***********************************************************************
 * Function: str_process()
 * Description:
 * Parameter:
 * Return Values:
 * 
 ************************************************************************
 */
static int str_process(int sockfd)
{
    int	res;
    
    res = verify_conn_req(sockfd);
    if (res == -1) {
        LOG(m_tools, ERROR, "Invoke verify_conn_req() error.\n");
        return -1;
    } else if (res == 0) {
        LOG(m_tools, DEBUG, "Invoke verify_conn_req() success.\n");
	res = cli_conn_req();
        if (res != 0) {
            LOG(m_tools, ERROR, "Invoke cli_conn_req() failed.\n");
            return -1;
        } else {
            LOG(m_tools, DEBUG, "Invoke cli_conn_req() success.\n");
        }
    } else {
        LOG(m_tools, DEBUG, "Invoke verify_conn_req() success.\n");
    }

    return 0;
}

/*
 ***********************************************************************
 * Function: verify_conn_req()
 * Description: Authenticate the ACS's identity
 * Parameter:
 * Return Values:
 * 
 ***********************************************************************
 */
static int verify_conn_req(int sockfd)
{
    int byte_read, byte_write, res;
    char readbuf[BUFFER_SIZE];
    char line[HTTP_HEAD_LINE_LEN];
    char *p_readbuf = NULL;
    char writebuf[BUFFER_SIZE];
    
    //init variable value
    memset(readbuf, 0, sizeof(readbuf));
    memset(writebuf, 0, sizeof(writebuf));
    memset(line, 0, sizeof(line));
    
    byte_read = read(sockfd, readbuf, BUFFER_SIZE);
    if (byte_read <= 0) {
        LOG(m_tools, ERROR, "Read error from %d.\n", sockfd);
        return -1;
    }

    //parse the first line, check whether is a connection request
    if (get_line(line, sizeof(line), readbuf) == 0) {
        return -1;
    }
    res = parse_first_line(line);
    if (res != 0) {
        return -1;
    }
   
    //LOG(m_tools, DEBUG, "recv data: %s\n", p_readbuf);
    p_readbuf = strchr(readbuf, '\n');
    p_readbuf++;
    
    LOG(m_tools, DEBUG, "recv data: %s\n", p_readbuf);
    while (get_line(line, sizeof(line), p_readbuf) != 0) {
        if (strncasecmp(line, "Authorization:", 14) == 0) {
            LOG(m_tools, DEBUG, "Receive an Authorization header line, parse it\n");
            //parse auth infoemation
            res = parse_authorization_line(line + 14, writebuf);
            if (res == 1) {
                break;
            } else if (res == -1) {
                return -1;
            }
            //write data to buf
            sprintf(writebuf, "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: Apache/2.0.40 (Red Hat Linux)\r\nContent-Length: %d\r\nConnection: close\r\nContent-Type: text/plain; charset=ISO-8859-1\r\n\r\n%s", s_date, strlen(s_display_text), s_display_text);
            //write data to socket
            byte_write = write(sockfd, writebuf, strlen(writebuf));
            if (byte_write != strlen(writebuf)) {
                LOG(m_tools, ERROR, "Write error to socket %d.\n", sockfd);
                return -1;
            }
            return 0;
        }
        p_readbuf = strchr(p_readbuf, '\n');
        p_readbuf++;
    }
    
    //send 401 unauthorized package
    LOG(m_tools, DEBUG, "No Auth info or Auth info error, send 401 package\n");
    generate_auth_pack(writebuf);

    byte_write = write(sockfd, writebuf, strlen(writebuf));
    if (byte_write != strlen(writebuf)) {
        LOG(m_tools, ERROR, "Write error to socket %d.\n", sockfd);
        return -1;
    }
    
    return 1;
}

/*
 ***********************************************************************
 * Function: parse_first_line()
 * Description: parse first line and check whether is a
 *                          connection request
 * Parameter:
 * Return Values:
 * 
 ***********************************************************************
 */
static int parse_first_line(char *p_line)
{
    char protocol[6], method[10], path[64], path_cmp[64], *p;
    
    
    //init variable value
    memset(protocol, 0, sizeof(protocol));
    memset(method, 0, sizeof(method));
    memset(path, 0, sizeof(path));
    memset(path, 0, sizeof(path_cmp));
    
    LOG(m_tools, DEBUG, "%s\n", p_line);
    
    /*if (sscanf(p_line, "%s %s %s", method, path, protocol) != 3) {
        return -1;
    }*/
    
    //LOG(m_tools, DEBUG, "method : %s path : %s protocol : %s\n", method, path, protocol);
    while (*p_line == ' ')
        p_line++;
    p = strchr(p_line, ' ');
    strncpy(method, p_line, p - p_line);
    p_line = p++;
    while (*p_line == ' ')
        p_line++;
    
    p = strchr(p_line, ' ');
    strncpy(path, p_line, p - p_line);
    
    if (strcmp(method, "GET") != 0) {
        LOG(m_tools, ERROR, "NOT GET request.\n");
        return -1;
    }

    LOG(m_tools, DEBUG, "%s\n", path);

    //sprintf(path_cmp, "/%ld", gethostid());
    sprintf(path_cmp, "/%d", 0);
    
    LOG(m_tools, DEBUG, "%s\n", path_cmp);

    if (strcmp(path, path_cmp)) {
        return -1;
    }

    return 0;
}

/*
 ***********************************************************************
 * Function: parse_authorization_line 
 * Description: parse authorization header line and 
 *              determine whether authenticate success
 * Parameter:  p_line: point to the string of the authorization line
 *             buf: Filled with the package sent out
 * return value:
 *       0:   authenticate success
 *       1:   authenticate fail
 *       -1: error
 ***********************************************************************
 */
static int parse_authorization_line(char *p_line, char *buf)
{
    char *p;
    char name[100], value[100];
    typedef struct {
        char c_username[100];
        char c_realm[100];
        char c_qop[100];
        char c_algorithm[100];
        char c_uri[100];
        char c_nonce[100];
        char c_nc[100];
        char c_cnonce[100];
        char c_response[100];
    }digest_auth;
    
    digest_auth auth;
    
    char conn_req_username[256];
    char conn_req_passwd[256];
    HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX s_response;
    HASHHEX s_rspauth;

    //check pointer
    if ((!p_line) || (!buf)) {
        LOG(m_tools, ERROR, "The pointer parameter(s) is(are) NULL.\n");
        return -1;
    }
    //init variable
    memset(&auth, 0, sizeof(auth));
    memset(conn_req_username, 0, sizeof(conn_req_username));
    memset(conn_req_passwd, 0, sizeof(conn_req_passwd));
    
    //Judge whether it is a digest authorization
    while (*p_line == ' ') {
        p_line++;
    }
    if (memcmp(p_line, "Digest", 6) != 0) {
        LOG(m_tools, ERROR, "Not a digest authorization.\n");
        return -1;
    }
    
    p_line += 6;
    p = strchr(p_line, '=');
    
    
    //get auth info from http response(realm/qop/nonce/algorithm)
    while (*p_line != '\0') {
        //init name value
        memset(name, 0, sizeof(name));
        memset(value, 0, sizeof(value));
        //delete space
        while (*p_line == ' ') {
            p_line++;
        }
        while (*(p - 1) == ' ') {
            p--;
        }
        //get name
        strncpy(name, p_line, p - p_line);
        LOG(m_handler, DEBUG, "name : %s\n", name);
        //get value
        p_line = strchr(p, '=');
        p_line++;
        
        while (*p_line == ' ') {
            p_line++;
        }
        if (*p_line == '"') {
            p_line++;
            p = strchr(p_line, '"');
            strncpy(value, p_line, p - p_line);
            p += 2;
        } else {
            p = strchr(p_line, ',');
            strncpy(value, p_line, p - p_line);
            p++;
        }
        LOG(m_handler, DEBUG, "value : %s\n", value);
        p_line = p;
        while (*p_line == ' ') {
            p_line++;
        }
        p = strchr(p_line, '=');
        
        if (!strcmp(name, "username")) {
            strcpy(auth.c_username, value);
        } else if (!strcmp(name, "realm")) {
            strcpy(auth.c_realm, value);
        } else if (!strcmp(name, "qop")) {
            strcpy(auth.c_qop, value);
        } else if (!strcmp(name, "algorithm")) {
            strcpy(auth.c_algorithm, value);
        } else if (!strcmp(name, "uri")) {
            strcpy(auth.c_uri, value);
        } else if (!strcmp(name, "nonce")) {
            strcpy(auth.c_nonce, value);
        } else if (!strcmp(name, "nc")) {
            strcpy(auth.c_nc, value);
        } else if (!strcmp(name, "cnonce")) {
            strcpy(auth.c_cnonce, value);
        } else if (!strcmp(name, "response")) {
            strcpy(auth.c_response, value);
        } else {
            LOG(m_tools, DEBUG, "unrecognized: %s=%s\n", name, value);
        }
        
        /*p = strchr(p_line, ',');
        if (p == NULL) {
            p = strchr(p_line, '\0');
        }
        if (memcmp(p_line, "username=\"", 10) == 0) {
            p_line += 10;
            strncpy(auth.c_username, p_line, p - 1 - p_line);
        } else if (memcmp(p_line, "realm=\"", 7) == 0) {
            p_line += 7;
            strncpy(auth.c_realm, p_line, p - 1 - p_line);
        } else if (memcmp(p_line, "qop=\"", 5) == 0) {
            p_line += 5;
            strncpy(auth.c_qop, p_line, p - 1 - p_line);
        } else if (memcmp(p_line, "nonce=\"", 7) == 0) {
            p_line += 7;
            strncpy(auth.c_nonce, p_line, p - 1 - p_line );
        } else if (memcmp(p_line, "algorithm=\"", 11) == 0) {
            p_line += 11;
            strncpy(auth.c_algorithm, p_line, p - 1 - p_line);
        } else if (memcmp(p_line, "uri=\"", 5) == 0) {
            p_line += 5;
            strncpy(auth.c_uri, p_line, p -1 - p_line);
        } else if (memcmp(p_line, "nc=\"", 4) == 0) {
            p_line += 4;
            strncpy(auth.c_nc, p_line, p - 1 - p_line);
        } else if (memcmp(p_line, "cnonce=\"", 8) == 0) {
            p_line += 8;
            strncpy(auth.c_cnonce, p_line, p - 1 - p_line);
        } else if (memcmp(p_line, "response=\"", 10) == 0) {
            p_line += 10;
            strncpy(auth.c_response, p_line, p - 1 - p_line);
        } else {
            LOG(m_tools, DEBUG, "Have unknow info\n");
        }

        if (*p == '\0') {
            break;
        }
        p_line = ++p;*/

    }
    //get username and password value from device
    /*res = call_dev_func(requser_dev_func, GET_OPT, conn_req_username, locate);
    if (res != GET_VAL_SUCCESS) {
        LOG(m_tools, ERROR, "get username failed\n");
        return -1;
    }
    res = call_dev_func(reqpwd_dev_func, GET_OPT, conn_req_passwd, locate);
    if (res != GET_VAL_SUCCESS) {
        LOG(m_tools, ERROR, "get password failed\n");
        return -1;
    }*/
    agent_conf a_conf;
    if (get_agent_conf(&a_conf) == AGENT_CONF_FAILED) {
        return -1;
    }
    strcpy(conn_req_username, a_conf.conn_req_ser_username);
    strcpy(conn_req_passwd, a_conf.conn_req_ser_password);
    
    //Calculate the digest response
    DigestCalcHA1(auth.c_algorithm, auth.c_username, auth.c_realm, \
                  conn_req_passwd, \
                  auth.c_nonce, auth.c_cnonce, HA1);
    DigestCalcResponse(HA1, auth.c_nonce, auth.c_nc, auth.c_cnonce, \
                       auth.c_qop, "GET", \
                       auth.c_uri, HA2, s_response);
    DigestCalcResponse(HA1, auth.c_nonce, auth.c_nc, auth.c_cnonce, \
                       auth.c_qop, "", \
                       auth.c_uri, HA2, s_rspauth);

    //Judge whether the authentication is successful
    if (strcmp(s_realm, auth.c_realm) ||
       strcmp(s_nonce, auth.c_nonce) ||
       strcmp(s_algorithm, auth.c_algorithm) ||
       strcmp(s_qop, auth.c_qop) ||
       strcmp(conn_req_username, auth.c_username) ||
       strcmp(s_response, auth.c_response)) {
        
        LOG(m_tools, DEBUG, "Digest authenticate failed\n");

        //generate_auth_pack(buf);
        return 1;
        
    } else {
        LOG(m_tools, DEBUG, "Digest authenticate success, send 200 OK package.\n");

        //Get current datetime
        get_date();

        sprintf(buf, "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: Apache/2.0.40 (Red Hat Linux)\r\nAuthentication-Info: rspauth=\"%s\", cnonce=\"%s\", nc=%s, qop=%s\r\nContent-Length: %d\r\nConnection: close\r\nContent-Type: text/plain; charset=ISO-8859-1\r\n\r\n%s", s_date, s_rspauth, auth.c_cnonce, auth.c_nc, auth.c_qop, strlen(s_display_text), s_display_text);
        return 0;
    }
}

/*
 ***********************************************************************
 * Function: generate_auth_pack()
 * Description: Generate 401 Unauthorized package
 * Parameter:
 * Return Values:
 * 
 ***********************************************************************
 */
static void generate_auth_pack(char *buf)
{   
    //Generate new nonce
    generate_nonce();

    //Get current datetime
    get_date();

    sprintf(buf, "HTTP/1.1 401 Authorization Required\r\nDate: %s\r\nServer: Apache/2.0.40 (Red Hat Linux)\r\nWWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\", algorithm=%s, domain=\"%s\", qop=\"%s\"\r\nVary: accept-language\r\nAccept-Ranges: bytes\r\nContent-Length: 0\r\nConnection: close\r\nContent-Type: text/html; charset=ISO-8859-1\r\n\r\n", s_date, s_realm, s_nonce, s_algorithm, s_domain, s_qop);

    return;
}

/*
 ***********************************************************************
 * Function: generate_nonce
 * Description: Generate new nonce and write to s_nonce
 * Parameter:
 * Return Values:
 ***********************************************************************
 */
static void generate_nonce()
{
    static char *base64_encoding =
       "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i, j;
    
    memset(s_nonce, '\0', sizeof(s_nonce));
    srand((int)time(0));
    for (i = 0;i < (sizeof(s_nonce) - 1);i++) {
        j = (int)(64.0 * rand()/(RAND_MAX + 1.0));
        s_nonce[i] = base64_encoding[j];
    }
    
    return;
}

/*
 ***********************************************************************
 * Function: get_date
 * Description: Get current datetime and write to s_date
 * Parameter:
 * Return value:
 * 
 ***********************************************************************
 */
static void get_date()
{
    time_t timeval;

    //Get current time
    memset(s_date, '\0', sizeof(s_date));
    time(&timeval);
    strcpy(s_date, ctime(&timeval));
    s_date[strlen(s_date) - 1] = '\0';

    return;
}

