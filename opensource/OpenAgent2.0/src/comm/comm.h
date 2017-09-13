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

#ifndef COMM_H_
    #define COMM_H_
	
    /***********************************************************************
    *
    *	include file
    *
    ***********************************************************************/
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/timeb.h>
    #include <sys/ioctl.h>
    #include <errno.h>
    #include <resolv.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include "../res/global_res.h"
    #include "../res/qbuf.h"
    #include "../handler/handler.h"
    #include "../handler/soap/soap.h"
    #include "../handler/methods/inform.h"
    /************************************************************************
    *
    *	publish macrodefinition define
    *
    ************************************************************************/
    //#define USE_SSL 1

    //added by keelson
    #ifdef USE_SSL
       	#define PASSWD "passwd" // the password of client.key
    #endif
    #define COMM_SSL_ERROR -2
    #define COMM_ERROR_RW -1
    #define COMM_ERROR_LIB -2
    #define COMM_ERROR_FDSET -3
    #define COMM_ERROR_TIMEOUT -4
                                                                                                 
                                                                                                 
	
    /************************************************************************
    *
    *	publish var define
    *
    ************************************************************************/
	
	
    /***********************************************************************
    *
    *	publish function declare
    *
    ***********************************************************************/
    int start_comm();
    int post_retry();
    int dev_get_acs_ip_path(char *url, char *acs_name, char *acs_ip, char *acs_port, char *acs_path, int *flag, int name_flag, int *no_port_flag);
    int init_socket_http_mode(char * _ip, int _port, int *p_sockfd);
    int sock_send(int *p_sockfd, void **p_ssl, char * _pdb, int time_out, int mode);
    int sock_recv(int *p_sockfd, void **p_ssl, char * _pdb, int length, int time_out, int mode);
    int connect_auth(char * _ip, int _port, int *p_sockfd, void **p_ssl, void **p_ctx, char *ca_path, char *client_path, char *key_path, int *p_conn_mode_flag, int *p_conn_mode, int *p_no_port_flag);
    //void monitor_network_status();

#ifdef USE_SSL
    #include <openssl/crypto.h>
    #include <openssl/ssl.h>
    #include <openssl/ssl23.h>
    #include <openssl/ssl2.h>

    int SSL_INIT_ERR;

    int init_socket_ssl_mode(char * _ip, int _port, int *p_socket, SSL **p_ssl, SSL_CTX **p_ctx, char *ca_path, char *client_path, char *key_path);
    int init_socket_sslv3_mode(char * _ip, int _port, int *p_socket, SSL **p_ssl, SSL_CTX **p_ctx, char *ca_path, char *client_path, char *key_path);
    int init_socket_tlsv1_mode(char * _ip, int _port, int *p_socket, SSL **p_ssl, SSL_CTX **p_ctx, char *ca_path, char *client_path, char *key_path);
    void destroy_ssl_res(SSL **p_ssl, SSL_CTX **p_ctx);
#endif
#endif
