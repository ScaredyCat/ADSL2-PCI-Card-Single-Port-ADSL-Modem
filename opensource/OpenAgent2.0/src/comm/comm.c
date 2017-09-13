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
#include "comm.h"


/************************************************************************
*
*	private macrodefinition define
*
************************************************************************/
#ifndef SSL_PORT
    #define SSL_PORT 443
#endif

#ifndef SSL_DEPTH
    #define SSL_DEPTH 1
#endif

#ifndef TIME_OUT
    #define TIME_OUT 30
#endif


/***********************************************************************
*
*	private var define
*
***********************************************************************/
//normal socket descriptor
int sockfd;

//socket connect mode flag
int conn_mode;
int conn_mode_flag;
int no_port_flag;

//pthread attribute
pthread_attr_t thread_attr;

#ifdef USE_SSL
    SSL * ssl;
    SSL_CTX * ctx;
#endif

//last thread exit, post this semaphore
sem_t sem_goodbye_pthread;

//thread exit flag
int stop_flag;

//debug code
sem_t sem_recv_GO;
sem_t sem_send_GO;
//debug code

//whether only recv thread create successful flag
int recv_thread_flag;

/***********************************************************************
*
*	private function declare
*
***********************************************************************/
void * func_send();
void * func_recv();
	
void recv_abort_signal(int);

int init_comm();
#ifdef USE_SSL
int ssl_auth(char * _ip, int _port, int *p_sockfd, SSL **p_ssl, SSL_CTX **p_ctx, char *ca_path, char *client_path, char *key_path, int *p_flag);
#endif
int set_abort_signal_cb();
int start_thread();
long my_pow(int value, int n);
long get_random_time(long range_min, long range_max);

#ifdef USE_SSL
    /*
     *********************************************************************
     * Function name: password_cb
     * Description: password callback function
     * Parameter: char *buf: the buffer for password; size: the buffer size; rwflag: read or write; userdata: user's data
     * Return value: the length of the password
     *********************************************************************
    */
    static int password_cb(char *buf, int size, int rwflag, void *userdata)
    {
    	const char *pass = (const char *)userdata;
    	int pass_len = strlen(pass);
                                                                                                 
    	if (pass_len > size)
            return FAIL;
                                                                                                 
    	strncpy(buf, pass, pass_len);
                                                                                                 
    	return pass_len;
    }

    /*
     *********************************************************************
     * Function name: verify_cb
     * Description: verify cert callback function
     * Parameter: int ok: the cert verify is OK or not; X509_STORE_CTX *stor: the pointer point to the context
     * Return value: 0 or 1
     *********************************************************************
    */
    static int verify_cb(int ok, X509_STORE_CTX *store)
    {
        char data[256];
        if(!ok)
        {       
            LOG(m_comm, DEBUG, "in verify callback\n");

            SSL_INIT_ERR = 1;
            X509 *cert = X509_STORE_CTX_get_current_cert(store);
            int depth = X509_STORE_CTX_get_error_depth(store);
            int err = X509_STORE_CTX_get_error(store);

            LOG(m_comm, DEBUG, "Error with certificate at depth: %i\n", depth);

            X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
            LOG(m_comm, DEBUG, " issuer = %s\n", data);

            X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
            LOG(m_comm, DEBUG, " subject = %s\n", data);

            LOG(m_comm, DEBUG, " err %i:%s\n", err, X509_verify_cert_error_string(err));
        }

        return ok;
    }
                                                                                                 
#endif

int init_comm_modules_res();
int destroy_comm_modules_res();
int send_inform();

/*
 *********************************************************************
 * Function name: start_comm
 * Description: init the comm module and start the sending and receiving thread
 * Parameter: none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int start_comm()
{
    int res;
    int port;

    sem_wait(&SEM_CONNECTED);

    res = init_comm();
    if(res == SUCCESS)
    {
        port = atoi(http_port);
        
        #ifdef USE_SSL
            res = connect_auth(acs_ip, port, &sockfd, (void **)(&ssl), (void **)(&ctx), ca_cert_path, client_cert_path, client_key_path, &conn_mode_flag, &conn_mode, &no_port_flag);
        #else
            res = connect_auth(acs_ip, port, &sockfd, NULL, NULL, NULL, NULL, NULL, &conn_mode_flag, &conn_mode, &no_port_flag);
        #endif
    
        if(res == SUCCESS)
        {
            res = set_abort_signal_cb();
    
            if(res == SUCCESS)
            {
                res = start_thread();
            }
        }
    }

    if(res != SUCCESS)
    {
        post_retry();

        if(!recv_thread_flag)
        {
            destroy_comm_modules_res();
            sem_post(&SEM_CONNECTED);
        }
    }
    
    return res;
}

/*
 *********************************************************************
 * Function name: func_send
 * Description: the executed function of sending thread
 * Parameter: none
 * Return value: none
 *********************************************************************
*/
void * func_send()
{	
    char *p_data = NULL;
    Qbuf_node *_qn = NULL;
    int res;

    recv_thread_flag = 0;
    sem_post(&sem_recv_GO);

    while(stop_flag != 1)
    {
        LOG(m_comm, DEBUG, "wait SEM_SEND.\n");
        sem_wait(&SEM_SEND);

        _qn = send_qbuf_list.head(&send_qbuf_list);
        if( _qn == NULL )
        {
            sleep(1);
            continue;
        }
        LOG(m_comm, DEBUG, "get qbuf from send_qbuf_head ok.\n");
        
        p_data = _qn->get_data_ptr(_qn);
        if(strlen(p_data) == 0)
        {
            stop_flag = 1;
        }
        else
        {
            #ifdef USE_SSL
                res = sock_send(&sockfd, (void **)(&ssl), p_data, TIME_OUT, conn_mode);
            #else
                res = sock_send(&sockfd, NULL, p_data, TIME_OUT, conn_mode);
            #endif
            if(res != SUCCESS)
            {
                stop_flag = 1;
                LOG(m_comm, DEBUG, "send data error.\n");
            }
            LOG(m_comm, DEBUG, "send data successful.\n");
        }
       
        empty_qbuf_list.append(&empty_qbuf_list, _qn);
        LOG(m_comm, DEBUG, "release qbuf to empty_qbuf_head ok.\n");
    }

    sem_wait(&sem_goodbye_pthread);
    sem_wait(&SEM_HANDLER_ABORT);

    destroy_comm_modules_res();
    sem_post(&SEM_CONNECTED);
    LOG(m_comm, DEBUG, "comm stop.\n")

    pthread_exit(NULL);
}

/*
 *********************************************************************
 * Function name: func_recv
 * Description: the executed function of receiving thread
 * Parameter: none
 * Return value: none
 *********************************************************************
*/
void * func_recv()
{
    int res;
    char *p_data = NULL;
    Qbuf_node *_qn = NULL;

    sem_wait(&sem_recv_GO);
    
    while(stop_flag != 1)
    {
        _qn = empty_qbuf_list.head(&empty_qbuf_list);
        if( _qn == NULL )
        {
            LOG(m_comm, ERROR, "get qbuf from empty_qbuf_head failure.\n");
            sleep(1);
            continue;
        }
        LOG(m_comm, DEBUG, "get qbuf from empty_qbuf_head ok.\n");

        p_data = _qn->get_data_ptr(_qn);
        memset(p_data, 0, MAX_DATA_LEN - 1);

        #ifdef USE_SSL
            res = sock_recv(&sockfd, (void**)(&ssl), p_data, MAX_DATA_LEN - 1, TIME_OUT * 2, conn_mode);
        #else
            res = sock_recv(&sockfd, NULL, p_data, MAX_DATA_LEN - 1, TIME_OUT * 2, conn_mode);
        #endif
        if(res < 0)
        {
            stop_flag = 1;
        }

        recv_qbuf_list.append(&recv_qbuf_list, _qn);
        LOG(m_comm, DEBUG, "insert qbuf to recv_qbuf_head ok.\n");

        sem_post(&SEM_RECV);
        LOG(m_comm, DEBUG, "post SEM_RECV.\n");
    }
	
    sem_post(&SEM_RECV);
    sem_post(&sem_goodbye_pthread);

    if(recv_thread_flag)
    {
        destroy_comm_modules_res();
        sem_post(&SEM_CONNECTED);
    }

    pthread_exit(NULL);
}

/*
 *********************************************************************
 * Function name: sock_send
 * Description: send data
 * Parameter: int p_sockfd: the pointer point to the socket; void **p_ssl: the pointer point to  the pointer point to the ssl; char _pdb: the pointer point to the sending data; int time_out: the timeout time; int mode: the connect mode
 * Return value: 0: SUCCESS; -1, -2, -3, -4: FAIL
 *********************************************************************
*/
int sock_send(int *p_sockfd, void **p_ssl, char * _pdb, int time_out, int mode)
{
    int send_ret_value, result;
    fd_set sendfds;
    struct timeval timeout;

    FD_ZERO(&sendfds);
    FD_SET(*p_sockfd, &sendfds);
    timeout.tv_sec = time_out;
    timeout.tv_usec = 0;

    if(select(*p_sockfd + 1, NULL, &sendfds, NULL, &timeout) > 0)
    {
        if(FD_ISSET(*p_sockfd, &sendfds))
        {
            if(mode == 1)
            {
                #ifdef USE_SSL
                send_ret_value = SSL_write(*((SSL **)p_ssl), _pdb, strlen(_pdb));
 
                if( send_ret_value < strlen(_pdb))
                {
                    result = COMM_ERROR_RW;
                }
                else
                {
                    result = SUCCESS;
                }
                #else
                result = COMM_ERROR_LIB;
                #endif
                 
            }
            else
            {
                send_ret_value = send(*p_sockfd, _pdb, strlen(_pdb), 0);
 
                if( send_ret_value < strlen(_pdb))
                {
                    result = COMM_ERROR_RW;
                }
                else
                {
                    result = SUCCESS;
                }
            }

        }
        else
        {
            LOG(m_comm, DEBUG, "FD_ISSET error\n");
            result = COMM_ERROR_FDSET; 
        }
    }
    else 
    {
        LOG(m_comm, DEBUG, "send timeout\n");
        result = COMM_ERROR_TIMEOUT;
    }

    return result;
}

/*
 *********************************************************************
 * Function name:  sock_recv
 * Description: receive data
 * Parameter: int p_sockfd: the pointer point to the socket; void **p_ssl: the pointer point to  the pointer point to the ssl; char _pdb: the pointer point to the receiving data; int time_out: the timeout time; int mode: the connect mode
 * Return value: 0: SUCCESS; -1, -2, -3, -4: FAIL
 *********************************************************************
*/
int sock_recv(int *p_sockfd, void **p_ssl, char * _pdb, int length, int time_out, int mode)
{
    int recv_ret_value, result;
    fd_set recvfds;
    struct timeval timeout;

    FD_ZERO(&recvfds);
    FD_SET(*p_sockfd, &recvfds);
    timeout.tv_sec = time_out;
    timeout.tv_usec = 0;
                                                                         
    if(select(*p_sockfd + 1, &recvfds, NULL, NULL, &timeout) > 0)
    {
        if(FD_ISSET(*p_sockfd, &recvfds))
        {
            if(mode == 1)
            {
                #ifdef USE_SSL
                recv_ret_value = SSL_read(*((SSL **)p_ssl), _pdb, length);

                if( recv_ret_value < 0 )
                {
                    result = COMM_ERROR_RW;
                }
                else
                {
                    result = recv_ret_value;
                    if(recv_ret_value > length)
                    {
                        _pdb[0] = '\0';
                        result = COMM_ERROR_RW;
                    }
                    else
                    {
                        _pdb[recv_ret_value] = '\0';
                    }
                }
                #else
                result = COMM_ERROR_LIB;
                #endif
            }
            else
            {
                recv_ret_value = recv(*p_sockfd, _pdb, length, 0);
 
                if( recv_ret_value < 0 )
                {
                    result = COMM_ERROR_RW;
                }
                else
                {
                    result = recv_ret_value;
                    if(recv_ret_value > length)
                    {
                        _pdb[0] = '\0';
                        result = COMM_ERROR_RW;
                    }
                    else
                    {
                        _pdb[recv_ret_value] = '\0';
                    }
                }
            }
        }
        else 
        {
            LOG(m_comm, DEBUG, "FD_ISSET error\n");
            result = COMM_ERROR_FDSET;
        }
    }
    else 
    {
        LOG(m_comm, DEBUG, "recv timeout\n");
        result = COMM_ERROR_TIMEOUT;
    }

    return result;
}

/*
 *********************************************************************
 * Function name: recv_abort_signal
 * Description: the callback function to handle the SIG_ABORT signal
 * Parameter: int _sig: the signal value
 * Return value: none
 *********************************************************************
*/
void recv_abort_signal(int _sig)
{
    stop_flag = 1;
}

/*
 *********************************************************************
 * Function name: init_socket_ssl_mode
 * Description: int and connect to ssl port with sslv2
 * Parameter: char *_ip: the ip address of server; int _port: the port of server; int *p_sockfd: the pointer point to the socket; SSL **p_ssl: the pointer point to the pointer point to the ssl; SSL_CTX **p_ctx: the pointer point to the pointer point to the ssl context; char *ca_path: the ca certificate path; char *client_path: the client certificate path; char *key_path: the key path
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
#ifdef USE_SSL
    int init_socket_ssl_mode(char * _ip, int _port, int *p_sockfd, SSL **p_ssl, SSL_CTX **p_ctx, char *ca_path, char *client_path, char *key_path)
    {
        int res;
        SSL_INIT_ERR = 0;

        struct timeval timeout;
        fd_set fds;
        int errlen;
        int error;
        unsigned long fc;

        struct sockaddr_in dest;

        bzero(&dest, sizeof(&dest));

        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = inet_addr(_ip);
        dest.sin_port = htons(_port);
        LOG(m_comm, DEBUG, "set SSL port %d and ip %s.\n", _port, _ip);
		
        fc = 1;
        if(ioctl(*p_sockfd, FIONBIO, &fc) == -1)
        {
            LOG(m_comm, ERROR, "ioctl error.\n");
            return FAIL;
        }

        if(connect(*p_sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == -1)
        {
            errlen = sizeof(int);
            FD_ZERO(&fds);
            FD_SET(*p_sockfd, &fds);
            timeout.tv_sec = TIME_OUT;
            timeout.tv_usec = 0;
                                                                                                 
            if((res = select(*p_sockfd + 1, NULL, &fds, NULL, &timeout)) > 0)
            {
                getsockopt(*p_sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&errlen);
                if(error == 0)
                {
                    LOG(m_comm, DEBUG, "In select()\n");
                }
                else
                {
                    LOG(m_comm, ERROR, "error\n");
                    return FAIL;
                }
            }
            else
            {
                LOG(m_comm, ERROR, "Connect Timeout\n");
                //continue;
                return COMM_SSL_ERROR;
            }
        }
        else
        {
            LOG(m_comm, DEBUG, "TCP connected\n");
        }
		
        fc = 0;    
        if(ioctl(*p_sockfd, FIONBIO, &fc) == -1)
        {
            LOG(m_comm, ERROR, "ioctl error\n");
            return FAIL;
        }
		
        SSL_load_error_strings();
        if(SSL_library_init() != 1)
        {
            LOG(m_comm, ERROR, "SSL lib init failure.\n");
            SSL_INIT_ERR = 1;
            return FAIL;
        }
        LOG(m_comm, DEBUG, "SSL lib init ok.\n");
			
        if((*p_ctx = SSL_CTX_new(SSLv23_method())) == NULL )
        {
            LOG(m_comm, ERROR, "create SSL context failure.\n");
            SSL_INIT_ERR = 1;
            return FAIL;
        }
        LOG(m_comm, DEBUG, "create SSL context ok.\n");

        SSL_CTX_set_verify(*p_ctx, SSL_VERIFY_PEER, verify_cb);
        SSL_CTX_set_verify_depth(*p_ctx, SSL_DEPTH);
        SSL_CTX_set_options(*p_ctx, SSL_OP_NO_SSLv2);
        LOG(m_comm, DEBUG, "set verify mode to SSL_VERIFY_PEER mode.\n");
		
        if(SSL_CTX_load_verify_locations(*p_ctx, ca_path, NULL) != 1)
        {
            LOG(m_comm, ERROR, "loding CA root file failure.\n");
            //close(*p_sockfd);
            SSL_INIT_ERR = 1;
            return FAIL;
        }
     
        SSL_CTX_set_default_passwd_cb_userdata(*p_ctx, (void *)PASSWD);
        SSL_CTX_set_default_passwd_cb(*p_ctx, password_cb);

        LOG(m_comm, DEBUG, "loding CA root file ok.\n");
			
        if(SSL_CTX_use_certificate_file(*p_ctx, client_path, SSL_FILETYPE_PEM) != 1)
        {
            LOG(m_comm, ERROR, "loding client cert file failure.\n");
            //return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "loding client cert file ok.\n");
        }	

        if(SSL_CTX_use_PrivateKey_file(*p_ctx, key_path, SSL_FILETYPE_PEM) != 1)
        {
            LOG(m_comm, ERROR, "loding client key file failure.\n");
            //return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "loding client key file ok.\n");
        }	

        if(SSL_CTX_check_private_key(*p_ctx) != 1)
        {
            LOG(m_comm, ERROR, "check private key and cert file failure.\n");
            //return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "check private key and cert file ok.\n");
        }		

        if((*p_ssl = SSL_new(*p_ctx)) == NULL)
        {
            LOG(m_comm, ERROR, "create SSL object failure.\n");
            SSL_INIT_ERR = 1;
            return FAIL;
        }
        LOG(m_comm, DEBUG, "create SSL object ok.\n");
			
        if(SSL_set_fd(*p_ssl, *p_sockfd) != 1)
        {
            LOG(m_comm, ERROR, "bind SSL socket failure.\n");
            SSL_INIT_ERR = 1;
            return FAIL;
        }
        LOG(m_comm, DEBUG, "bind SSL socket ok.\n");
			
        if(SSL_connect(*p_ssl) != 1)
        {
            LOG(m_comm, ERROR, "SSL socket connect failure.\n");			
            return FAIL;
        }
        LOG(m_comm, DEBUG, "SSL socket connect ok.\n");
			
        return SUCCESS;
    }
#endif

/*
 *********************************************************************
 * Function name: init_socket_sslv3_mode
 * Description: int and connect to ssl port with sslv3
 * Parameter: char *_ip: the ip address of server; int _port: the port of server; int *p_sockfd: the pointer point to the socket; SSL **p_ssl: the pointer point to the pointer point to the ssl; SSL_CTX **p_ctx: the pointer point to the pointer point to the ssl context; char *ca_path: the ca certificate path; char *client_path: the client certificate path; char *key_path: the key path
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
#ifdef USE_SSL
    int init_socket_sslv3_mode(char * _ip, int _port, int *p_sockfd, SSL **p_ssl, SSL_CTX **p_ctx, char *ca_path, char *client_path, char *key_path)
    {
        int res;

        struct timeval timeout;
        fd_set fds;
        int errlen;
        int error;
        unsigned long fc;
                                                                                                     
        struct sockaddr_in dest;
                                                                                                 
        bzero(&dest, sizeof(&dest));
                                                                                                
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = inet_addr(_ip);
        dest.sin_port = htons(_port);
        LOG(m_comm, DEBUG, "set SSL port %d and ip %s.\n", _port, _ip);
                                                                                                 
        fc = 1;
        if(ioctl(*p_sockfd, FIONBIO, &fc) == -1)
        {
            LOG(m_comm, ERROR, "ioctl error.\n");
            return FAIL;
        }

        if(connect(*p_sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == -1)
        {
            errlen = sizeof(int);
            FD_ZERO(&fds);
            FD_SET(*p_sockfd, &fds);
            timeout.tv_sec = TIME_OUT;
            timeout.tv_usec = 0;
                                                                                                 
            if((res = select(*p_sockfd + 1, NULL, &fds, NULL, &timeout)) > 0)
            {
                getsockopt(*p_sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&errlen);
                if(error == 0)
                {
                    LOG(m_comm, DEBUG, "In select()\n");
                }
                else
                {
                    LOG(m_comm, ERROR, "error\n");
                    return FAIL;
                }
            }
            else
            {
                LOG(m_comm, ERROR, "Connect Timeout\n");
                return FAIL;
            }
        }
        else
        {
            LOG(m_comm, DEBUG, "TCP connected\n");
        }
		
        fc = 0;    
        if(ioctl(*p_sockfd, FIONBIO, &fc) == -1)
        {
            LOG(m_comm, ERROR, "ioctl error\n");
            return FAIL;
        }
                                                                                                 
        SSL_load_error_strings();
        if(SSL_library_init() != 1)
        {
            LOG(m_comm, ERROR, "SSL lib init failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "SSL lib init ok.\n");
                                                                                                 
        if((*p_ctx = SSL_CTX_new(SSLv3_method())) == NULL)
        {
            LOG(m_comm, ERROR, "create SSL context failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "create SSL context ok.\n");
                                                                                                    
        SSL_CTX_set_verify(*p_ctx, SSL_VERIFY_PEER, verify_cb);
        SSL_CTX_set_verify_depth(*p_ctx, SSL_DEPTH);
    
        LOG(m_comm, DEBUG, "set verify mode to SSL_VERIFY_PEER mode.\n");
                                                                                                 
        if(SSL_CTX_load_verify_locations(*p_ctx, ca_path, NULL) != 1)
        {
            LOG(m_comm, ERROR, "loding CA root file failure.\n");
            return FAIL;
        }
                                                                                                
        SSL_CTX_set_default_passwd_cb_userdata(*p_ctx, (void *)PASSWD);
        SSL_CTX_set_default_passwd_cb(*p_ctx, password_cb);
                                                                                                     
        LOG(m_comm, DEBUG, "loding CA root file ok.\n");
                                                                                               
        if(SSL_CTX_use_certificate_file(*p_ctx, client_path, SSL_FILETYPE_PEM) != 1)
        {
            LOG(m_comm, ERROR, "loding client cert file failure.\n");
            //return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "loding client cert file ok.\n");
        }                                                                                     
                                                                                               
        if(SSL_CTX_use_PrivateKey_file(*p_ctx, key_path, SSL_FILETYPE_PEM) != 1)
        {
            LOG(m_comm, ERROR, "loding client key file failure.\n");
            //return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "loding client key file ok.\n");
        }                                                                                    

        if(SSL_CTX_check_private_key(*p_ctx) != 1)
        {
            LOG(m_comm, ERROR, "check private key and cert file failure.\n");
            //return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "check private key and cert file ok.\n");
        }                                                                                     

        if((*p_ssl = SSL_new(*p_ctx)) == NULL)
        {
            LOG(m_comm, ERROR, "create SSL object failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "create SSL object ok.\n");
                                                                                                
        if(SSL_set_fd(*p_ssl, *p_sockfd) != 1)
        {
            LOG(m_comm, ERROR, "bind SSL socket failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "bind SSL socket ok.\n");
                                                                                                 
        if(SSL_connect(*p_ssl) != 1)
        {
            LOG(m_comm, ERROR, "SSL socket connect failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "SSL socket connect ok.\n");
                                                                                                 
        return SUCCESS;
    }
#endif

/*
 *********************************************************************
 * Function name: init_socket_tlsv1_mode
 * Description: int and connect to ssl port with tlsv1
 * Parameter: char *_ip: the ip address of server; int _port: the port of server; int *p_sockfd: the pointer point to the socket; SSL **p_ssl: the pointer point to the pointer point to the ssl; SSL_CTX **p_ctx: the pointer point to the pointer point to the ssl context; char *ca_path: the ca certificate path; char *client_path: the client certificate path; char *key_path: the key path
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
#ifdef USE_SSL
    int init_socket_tlsv1_mode(char * _ip, int _port, int *p_sockfd, SSL **p_ssl, SSL_CTX **p_ctx, char *ca_path, char *client_path, char *key_path)
    {
        int res;

        struct timeval timeout;
        fd_set fds;
        int errlen;
        int error;
        unsigned long fc;
                                                                                                     
        struct sockaddr_in dest;
                                                                                                 
        bzero(&dest, sizeof(&dest));
                                                                                                 
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = inet_addr(_ip);
        dest.sin_port = htons(_port);
        LOG(m_comm, DEBUG, "set SSL port %d and ip %s.\n", _port, _ip);
                                                                                                 
        fc = 1;
        if(ioctl(*p_sockfd, FIONBIO, &fc) == -1)
        {
            LOG(m_comm, ERROR, "ioctl error.\n");
            return FAIL;
        }

        if(connect(*p_sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == -1)
        {
            errlen = sizeof(int);
            FD_ZERO(&fds);
            FD_SET(*p_sockfd, &fds);
            timeout.tv_sec = TIME_OUT;
            timeout.tv_usec = 0;
                                                                                                 
            if((res = select(*p_sockfd + 1, NULL, &fds, NULL, &timeout)) > 0)
            {
                getsockopt(*p_sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&errlen);
                if(error == 0)
                {
                    LOG(m_comm, DEBUG, "In select()\n");
                }
                else
                {
                    LOG(m_comm, ERROR, "error\n");
                    return FAIL;
                }
            }
            else
            {
                LOG(m_comm, ERROR, "Connect Timeout\n");
                return FAIL;
            }
        }
        else
        {
            LOG(m_comm, DEBUG, "TCP connected\n");
        }
		
        fc = 0;    
        if(ioctl(*p_sockfd, FIONBIO, &fc) == -1)
        {
            LOG(m_comm, ERROR, "ioctl error\n");
            return FAIL;
        }
                                                                                                 
        SSL_load_error_strings();
        if(SSL_library_init() != 1)
        {
            LOG(m_comm, ERROR, "SSL lib init failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "SSL lib init ok.\n");
                                                                                                 
        if((*p_ctx = SSL_CTX_new(TLSv1_method())) == NULL)
        {
            LOG(m_comm, ERROR, "create SSL context failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "create SSL context ok.\n");
                                                                                                     
        SSL_CTX_set_verify(*p_ctx, SSL_VERIFY_PEER, verify_cb);
        SSL_CTX_set_verify_depth(*p_ctx, SSL_DEPTH);
                                                                                                 
        LOG(m_comm, DEBUG, "set verify mode to SSL_VERIFY_PEER mode.\n");
                                                                                                 
        if(SSL_CTX_load_verify_locations(*p_ctx, ca_path, NULL) != 1)
        {
            LOG(m_comm, ERROR, "loding CA root file failure.\n");
            return FAIL;
        }
                                                                                                 
        SSL_CTX_set_default_passwd_cb_userdata(*p_ctx, (void *)PASSWD);
        SSL_CTX_set_default_passwd_cb(*p_ctx, password_cb);
                                                                                                 
        LOG(m_comm, DEBUG, "loding CA root file ok.\n");
                                                                                                 
        if(SSL_CTX_use_certificate_file(*p_ctx, client_path, SSL_FILETYPE_PEM) != 1)
        {
            LOG(m_comm, ERROR, "loding client cert file failure.\n");
            //return FAIL;
        }        
        else
        {
            LOG(m_comm, DEBUG, "loding client cert file ok.\n");
        }                                                                                         
                                                                                                 
        if(SSL_CTX_use_PrivateKey_file(*p_ctx, key_path, SSL_FILETYPE_PEM) != 1)
        {
            LOG(m_comm, ERROR, "loding client key file failure.\n");
            //return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "loding client key file ok.\n");
        }                                                                                         

        if(SSL_CTX_check_private_key(*p_ctx) != 1)
        {
            LOG(m_comm, ERROR, "check private key and cert file failure.\n");
            //return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "check private key and cert file ok.\n");
        }                                                                                         

        if((*p_ssl = SSL_new(*p_ctx)) == NULL)
        {
            LOG(m_comm, ERROR, "create SSL object failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "create SSL object ok.\n");
                                                                                                 
        if(SSL_set_fd(*p_ssl, *p_sockfd) != 1)
        {
            LOG(m_comm, ERROR, "bind SSL socket failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "bind SSL socket ok.\n");
                                                                                                
        if(SSL_connect(*p_ssl) != 1)
        {
            LOG(m_comm, ERROR, "SSL socket connect failure.\n");
            return FAIL;
        }
        LOG(m_comm, DEBUG, "SSL socket connect ok.\n");                                                                                                 
        return SUCCESS;
    }
#endif

/*
 *********************************************************************
 * Function name: init_socket_http_mode
 * Description: connect to the http port
 * Parameter: char _ip: the ip address of server; int _port: the port of server; int *p_sockfd: the pointer point to the socket
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int init_socket_http_mode(char * _ip, int _port, int *p_sockfd)
{
    int res;
   
    struct timeval timeout;
    fd_set fds;
    int errlen;
    int error;
    unsigned long fc;

    struct sockaddr_in dest;

    bzero(&dest, sizeof(&dest));

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(_ip);
    LOG(m_device, DEBUG, "The http_port: %d\n", _port);
    dest.sin_port = htons(_port);
    LOG(m_comm, DEBUG, "set HTTP port and ip.\n");
 	
    fc = 1;
    if(ioctl(*p_sockfd, FIONBIO, &fc) == -1)
    {
        LOG(m_comm, ERROR, "ioctl error.\n");
        return FAIL;
    }

    if(connect(*p_sockfd, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == -1)
    {
        errlen = sizeof(int);
        FD_ZERO(&fds);
        FD_SET(*p_sockfd, &fds);
        timeout.tv_sec = TIME_OUT;
        timeout.tv_usec = 0;
                                                                                                 
        if((res = select(*p_sockfd + 1, NULL, &fds, NULL, &timeout)) > 0)
        {
            getsockopt(*p_sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&errlen);
            if(error == 0)
            {
                LOG(m_comm, DEBUG, "In select()\n");
            }
            else
            {
                LOG(m_comm, ERROR, "error\n");
                return FAIL;
            }
        }
        else
        {
            LOG(m_comm, ERROR, "Connect Timeout\n");
            return FAIL;
        }
    }
    else
    {
        LOG(m_comm, DEBUG, "TCP connected\n");
    }
		
    fc = 0;    
    if(ioctl(*p_sockfd, FIONBIO, &fc) == -1)
    {
        LOG(m_comm, ERROR, "ioctl error\n");
        return FAIL;
    }

    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: init_comm_modules_res
 * Description: init the socket, semaphore, global variable and so on
 * Parameter: none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int init_comm_modules_res()
{
    sockfd = 0;
    conn_mode = 0;
    conn_mode_flag = 0;
    stop_flag = 0;
    recv_thread_flag = 0;

    sem_init(&sem_goodbye_pthread, 0, 0);
    //debug code
    sem_init(&sem_send_GO, 0, 0);
    sem_init(&sem_recv_GO, 0, 0);
    //debug code

    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: destroy_comm_modules_res
 * Description: destroy the comm module resource
 * Parameter: none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int destroy_comm_modules_res()
{
    if(conn_mode != 0)
    {
        #ifdef USE_SSL
        destroy_ssl_res(&ssl, &ctx);
        #endif
    }
	
    close(sockfd);

    //debug code
    sem_destroy(&sem_send_GO);
    sem_destroy(&sem_recv_GO);
    //debug code
	
    sem_destroy(&sem_goodbye_pthread);
    sem_destroy(&SEM_HANDLER_ABORT);

    return SUCCESS;
}

#ifdef USE_SSL
    /*
     *********************************************************************
     * Function name: destroy_ssl_res
     * Description: destroy the ssl and context
     * Parameter: SSL **p_ssl :the pointer point to the pointer point to the ssl; SSL_CTX **p_ctx :the pointer point to the pointer point to the ssl context
     * Return value: none
     *********************************************************************
    */
    void destroy_ssl_res(SSL **p_ssl, SSL_CTX **p_ctx)
    {
        //close both ssl pipe
        SSL_set_shutdown(*p_ssl, 2);
 
        //close ssl connect
        SSL_shutdown(*p_ssl);
 
        //free ssl res
        SSL_free(*p_ssl);
 
        SSL_CTX_free(*p_ctx);
   }
#endif

/*
 *********************************************************************
 * Function name: my_pow
 * Description: get the result of value*vaule*...(n times)
 * Parameter: value and n are the value*vaule*...(n times)
 * Return value: the result of value*vaule*...(n times)
 *********************************************************************
*/
long my_pow(int value, int n)
{
    int i;
    long result = 1;
 
    for(i = 0; i < n; i++)
        result *= value;
 
    return result;
}

/*
 *********************************************************************
 * Function name: get_random_time
 * Description: get the random value between range_min and range_max
 * Parameter: long range_min: the min value of the range; long range_max: the max value of the range
 * Return value: the random value between range_min and range_max
 *********************************************************************
*/
long get_random_time(long range_min, long range_max)
{
    unsigned int random_seed;
    static unsigned int i = 1;
    struct timeb time;
    long result;
 
    i++;
 
    ftime(&time);
    random_seed = ((((unsigned int)time.time&0xFFFF)+ (unsigned int)time.millitm) ^ (unsigned int)time.millitm) + i;
    srand((unsigned int)random_seed);
 
    result = range_min + (long)(((double)(rand()) / (double)RAND_MAX) * ((double)(range_max - range_min + 1)));
    return result;
}

/*
 *********************************************************************
 * Function name: post_retry
 * Description: add the retry event and update the related values and post the SEM_INFORM semaphore
 * Parameter: none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int post_retry()
{
    int temp_try_time = get_max_try_time();

    inform_retry++;

    if(inform_retry > temp_try_time)
    {
        inform_retry = 0;
        del_event("M RETRY CONNECT");
        return FAIL;
    }

    retry_interval = get_random_time(my_pow(2, inform_retry - 1) * 5, my_pow(2, inform_retry - 1) * 10);
    add_event("M RETRY CONNECT", "");
    sem_post(&SEM_INFORM);

    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: init_comm
 * Description: init socket, semaphore, buffer queue, mutex, global variable and so on
 * Parameter: none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int init_comm()
{
    int res;
    agent_conf temp_agent_conf;

    init_comm_modules_res();

    reset_sys_sem();
	
    res = empty_qbuf_list.reset_lock(&empty_qbuf_list);
    if(res != SUCCESS)
    {
        LOG(m_comm, ERROR, "empty qbuf list lock reset failed\n");
        return FAIL;
    }
    res = send_qbuf_list.reset_lock(&send_qbuf_list);
    if(res != SUCCESS)
    {
        LOG(m_comm, ERROR, "send qbuf list lock reset failed\n");
        return FAIL;
    }
    res = recv_qbuf_list.reset_lock(&recv_qbuf_list);
    if(res != SUCCESS)
    {
        LOG(m_comm, ERROR, "recv qbuf list lock reset failed\n");
        return FAIL;
    }

    while(send_qbuf_list.qbuf_nodes_num(&send_qbuf_list))
    {
        res = send_qbuf_list.move_node(&send_qbuf_list, &empty_qbuf_list);
        if(res != SUCCESS)
        {
            LOG(m_comm, ERROR, "qbuf reset failed\n");
            return FAIL;
        }
    }

    while(recv_qbuf_list.qbuf_nodes_num(&recv_qbuf_list))
    {
        res = recv_qbuf_list.move_node(&recv_qbuf_list, &empty_qbuf_list);
        if(res != SUCCESS)
        {
            LOG(m_comm, ERROR, "qbuf reset failed\n");
            return FAIL;
        }
    }

    LOG(m_comm, DEBUG, "buf_queue_reset() ok.\n");
	
    //debug qbuf memory
    empty_qbuf_list.display(&empty_qbuf_list);
    send_qbuf_list.display(&send_qbuf_list);
    recv_qbuf_list.display(&recv_qbuf_list);

    get_agent_conf(&temp_agent_conf);
    memset(http_port, 0, sizeof(http_port));
    if( dev_get_acs_ip_path(temp_agent_conf.acs_url, NULL, acs_ip, http_port, acs_path, &conn_mode_flag, 0, &no_port_flag) != 0 )
    {
	LOG(m_comm, ERROR, "dev_get_acs_ip_path() failure.\n");
	return FAIL;
    }
    LOG(m_comm, DEBUG, "dev_get_acs_ip_path() ok. conn_mode_flag is %d.\n", conn_mode_flag);
        
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
	LOG(m_comm, ERROR, "socket() failure.\n");
  	return FAIL;
    }
    LOG(m_comm, DEBUG, "socket() ok.\n");

    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: ssl_auth
 * Description: try connect to ACS by SSL and do the authentication
 * Parameter: char *_ip: the ip address of server; int _port: the port of server; int *p_sockfd: the pointer point to the socket; SSL **p_ssl: the pointer point to the pointer point to the ssl; SSL_CTX **p_ctx: the pointer point to the pointer point to the ssl context; char *ca_path: the ca certificate path; char *client_path: the client certificate path; char *key_path: the key path; int *p_flag: the pointer point to the connect mode
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
#ifdef USE_SSL
int ssl_auth(char * _ip, int _port, int *p_sockfd, SSL **p_ssl, SSL_CTX **p_ctx, char *ca_path, char *client_path, char *key_path, int *p_flag)
{
    int res;

    if((res = init_socket_ssl_mode(_ip, _port, p_sockfd, p_ssl, p_ctx, ca_path, client_path, key_path)) != SUCCESS)
    {
        if(res == COMM_SSL_ERROR)
        {
            close(*p_sockfd);
            LOG(m_comm, ERROR, "TCP connect in ssl port failed\n");
                
            if((*p_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                LOG(m_comm, ERROR, "socket() failure.\n");
                return FAIL;
            }

            *p_flag = 0;
			    
            return FAIL;
        }
        else if(SSL_INIT_ERR == 1)
        {
            //reset scoket
            SSL_free(*p_ssl);
            SSL_CTX_free(*p_ctx);
            close(*p_sockfd);
                               
            if((*p_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                LOG(m_comm, ERROR, "socket() failure.\n");
                return FAIL;
            }                                                                 
            LOG(m_comm, ERROR, "init SSL failure.\n");
            *p_flag = 0;
            return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "init ssl first failed, start to connect with SSLv3\n");

            //free ssl res
            SSL_free(*p_ssl);
            SSL_CTX_free(*p_ctx);
            close(*p_sockfd);

            if((*p_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                LOG(m_comm, ERROR, "socket() failure.\n");
                return FAIL;
            }

            if(init_socket_sslv3_mode(_ip, _port, p_sockfd, p_ssl, p_ctx, ca_path, client_path, key_path) != SUCCESS)
            {
                LOG(m_comm, DEBUG, "init ssl second failed\n");
                if(SSL_INIT_ERR == 1)
                {
                    //reset scoket
                    close(*p_sockfd);
                    *p_sockfd = socket(AF_INET, SOCK_STREAM, 0);
                                                                                                 
                    LOG(m_comm, ERROR, "init SSL failure.\n");
                    *p_flag = 0;
                    return FAIL;
                }
                else
                {
                    LOG(m_comm, DEBUG, "start to connect with TLSv1\n");
        	  
                    //free ssl res
                    SSL_free(*p_ssl);
                    SSL_CTX_free(*p_ctx);
                    close(*p_sockfd);

                    if((*p_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        LOG(m_comm, ERROR, "socket() failure.\n");
                        return FAIL;
                    }

                    if(init_socket_tlsv1_mode(_ip, _port, p_sockfd, p_ssl, p_ctx, ca_path, client_path, key_path) != SUCCESS)
                    {
                        //reset scoket
                        close(*p_sockfd);
                        *p_sockfd = socket(AF_INET, SOCK_STREAM, 0);
			
                        LOG(m_comm, ERROR, "init SSL failure.\n");
                        *p_flag = 0;
                        return FAIL;
                    }
                    else
                    {
                        LOG(m_comm, DEBUG, "init SSL ok.\n");
                       	*p_flag = 1;
                        return SUCCESS;
                    }
                }
            }
            else
            {
                LOG(m_comm, DEBUG, "init SSL ok.\n");
                *p_flag = 1;
                return SUCCESS;
            }
        }
    }
    else 
    {
        LOG(m_comm, DEBUG, "init SSL ok.\n");
        *p_flag = 1;
        return SUCCESS;
    }

    return SUCCESS;
}
#endif

/*
 *********************************************************************
 * Function name: connect_auth
 * Description: try connect to ACS by SSL or HTTP
 * Parameter: char *_ip: the ip address of server; int _port: the port of server; int *p_sockfd: the pointer point to the socket; SSL **p_ssl: the pointer point to the pointer point to the ssl; SSL_CTX **p_ctx: the pointer point to the pointer point to the ssl context; char *ca_path: the ca certificate path; char *client_path: the client certificate path; char *key_path: the key path; int *p_conn_mode_flag: the pointer point to the connect mode; int *p_conn_mode: the pointer point to the connect mode; int *p_no_port_flag: the pointer point to the no port flag
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int connect_auth(char * _ip, int _port, int *p_sockfd, void **p_ssl, void **p_ctx, char *ca_path, char *client_path, char *key_path, int *p_conn_mode_flag, int *p_conn_mode, int *p_no_port_flag)
{
    int res = FAIL;

    if(*p_conn_mode_flag != 0 && *p_conn_mode_flag != 1 && *p_conn_mode_flag != -1)
    {
        LOG(m_comm, ERROR, "conn flag error.\n");
        return FAIL;
    }

    if(*p_conn_mode_flag == -1 || *p_conn_mode_flag == 1)
    {
        #ifdef USE_SSL
        res = ssl_auth(_ip, _port, p_sockfd, (SSL **)p_ssl, (SSL_CTX **)p_ctx, ca_path, client_path, key_path, p_conn_mode);
        if(res != SUCCESS)
        {
            LOG(m_comm, ERROR, "ssl connect failure.\n");
            if(*p_conn_mode_flag == 1)
                return FAIL;
        }
        #else
        if(*p_conn_mode_flag == 1)
            return FAIL;

        *p_conn_mode = 0;
        #endif
    }
    else
        *p_conn_mode = 0;

    if(*p_conn_mode != 0 && *p_conn_mode != 1)
    {
        LOG(m_comm, ERROR, "conn mode error.\n");
        return FAIL;
    }

    if( *p_conn_mode == 0 )
    {
        if(*p_conn_mode_flag == -1 && *p_no_port_flag == 1)
            _port = 80;

        if((res = init_socket_http_mode(_ip, _port, p_sockfd)) != SUCCESS )
        {
            //destroy_comm_modules_res();

            LOG(m_comm, ERROR, "init HTTP failure.\n");

            return FAIL;
        }
        else
        {
            LOG(m_comm, DEBUG, "init HTTP ok.\n");
        }
    }

    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: set_abort_signal_cb
 * Description: set the callback function of the SIG_ABORT signal
 * Parameter: none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int set_abort_signal_cb()
{
    struct sigaction act;

    act.sa_handler = recv_abort_signal;
    
    if(sigemptyset(&act.sa_mask) != 0 )
    {
        LOG(m_comm, ERROR, "sigemptyset() failure.\n");
        return FAIL;
    }
    
    act.sa_flags = 0;
    if(sigaction(SIG_ABORT, &act, 0) != 0 )
    {
        LOG(m_comm, ERROR, "sigaction() failure.\n");
        return FAIL;
    }
    LOG(m_comm, DEBUG, "sigaction() ok.\n");
    
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: start_thread
 * Description: init and create the sending and receiving thread
 * Parameter: none
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int start_thread()
{
    if( pthread_attr_init(&thread_attr) != 0 )
    {
        LOG(m_comm, ERROR, "pthread_attr_init failure.\n");
        return FAIL;
    }
    LOG(m_comm, DEBUG, "pthread_attr_init ok.\n");
	
    if( pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0 )
    {
        LOG(m_comm, ERROR, "pthread_attr_setdetachstate failure.\n");
        return FAIL;
    }
    LOG(m_comm, DEBUG, "pthread_attr_setdetachstate ok.\n");

    if( pthread_create(&p_recv, &thread_attr, func_recv, NULL) != 0 )
    {
        switch(errno)
        {
            case EAGAIN:
                LOG(m_comm, ERROR, "pthread_create(p_recv) failure, show cause: ? too much thread numbers.\n");
                break;

            case EINVAL:
                LOG(m_comm, ERROR, "pthread_create(p_recv) failure, show cause: ? thread id illegality.\n");
                break;

            default:
                LOG(m_comm, ERROR, "pthread_create(p_recv) failure, show cause: ? unknown.\n");
                break;
        }

        return FAIL;
    }
    LOG(m_comm, DEBUG, "pthread_create(p_recv) ok.\n");
    recv_thread_flag = 1;
       	
    if( pthread_create(&p_send, &thread_attr, func_send, NULL) != 0 )
    {
         switch(errno)
         {
            case EAGAIN:
                LOG(m_comm, ERROR, "pthread_create(p_send) failure, show cause: ? too much thread numbers.\n");
                break;

            case EINVAL:
                LOG(m_comm, ERROR, "pthread_create(p_send) failure, show cause: ? thread id illegality.\n");
                break;

            default:
                LOG(m_comm, ERROR, "pthread_create(p_send) failure, show cause: ? unknown.\n");
                break;
        }
		
        stop_flag = 1;
        sem_post(&sem_recv_GO);

        return FAIL;
    }
    LOG(m_comm, DEBUG, "pthread_create(p_send) ok. comm start.\n");
	
    return SUCCESS;
}

/*
 *********************************************************************
 * Function name: dev_get_acs_ip_path
 * Description: get the acs_name, acs_ip, acs_port, connect mode flag and no_port_flag if need
 * Parameter: char *url: the url; char *acs_name: the name of acs; char *acs_ip: the ip address of acs; char *acs_port: the port of acs; char *acs_path: the path of the acs url; int *flag: the pointer point to the connect mode flag; int name_flag:  the flag of whether need acs name; int *no_port_flag: the pointer point to the flag of whether exist port in url
 * Return value: 0:SUCCESS  -1:FAIL
 *********************************************************************
*/
int dev_get_acs_ip_path(char *url, char *acs_name, char *acs_ip, char *acs_port, char *acs_path, int *flag, int name_flag, int *no_port_flag)
{
    char *p;
    char *ptr;
    char **acs_ip1;
    char str[32];
    char host_buf[128];
    char buf[128];
    struct hostent *acs_server;

    strcpy(buf, url);

    *no_port_flag = 0;

    if (strcmp(buf, "") == 0)
    {
        LOG(m_device, ERROR, "The ACS URL is empty\n");
        return FAIL;
    }
    //Begin to get acs ip and path

    p = strstr(buf, "http://");
    if(p == NULL)
    {
        LOG(m_device, DEBUG, "There is no \"http://\" in given url.\n");
        
        p = strstr(buf, "https://");

        if(p == NULL)
        {
            LOG(m_device, DEBUG, "There is no \"https://\" in given url.\n");
            *flag = -1;
        }
        else
        {
            p = p + 8;
            strcpy(buf, p);
            *flag = 1;
        }
    }
    else
    {
        p = p + 7;
        strcpy(buf, p);
        *flag = 0;
    }
    p = strstr(buf, "/");
    if(p == NULL)
    {
        ptr = strstr(buf, ":");
        if(ptr != NULL){
            strncpy(host_buf, buf, (ptr-buf));
            host_buf[ptr-buf] = '\0';
            strcpy(buf, host_buf);
            strcpy(acs_port, ptr+1);
            
        }
        else
        {
            *no_port_flag = 1;

            if(*flag == 0)
                strcpy(acs_port, "80");
            else if(*flag == 1 || *flag == -1)
                strcpy(acs_port, "443");
        }
        
        if(name_flag)
            strcpy(acs_name, buf);

        acs_server = gethostbyname(buf);
        if(acs_server == NULL)
        {
            LOG(m_device, ERROR, "gethostbyname unsuccessfully.\n");
            return FAIL;
        }
    
        acs_ip1 = acs_server->h_addr_list;  
   
        strcpy(acs_ip, (char *)inet_ntop(acs_server->h_addrtype, *acs_ip1, str, sizeof(str)));
    }
    else
    {
        ptr = strstr(buf, ":");
        if(ptr != NULL)
        {
            strncpy(host_buf, buf, (ptr-buf));
            host_buf[ptr-buf] = '\0';
            strncpy(acs_port, ptr+1, p-ptr-1);
            acs_port[p-ptr-1] = '\0';
            
        }
        else
        {
            *no_port_flag = 1;

            strncpy(host_buf, buf, (p-buf));
            host_buf[p-buf] = '\0'; 
            if(*flag == 0)
                strcpy(acs_port, "80");
            else if(*flag == 1 || *flag == -1)
                strcpy(acs_port, "443");
        }

        if(name_flag)
            strcpy(acs_name, host_buf);

        acs_server = gethostbyname(host_buf);
        if(acs_server == NULL)
        {
            LOG(m_device, ERROR, "gethostname unsuccessfully.\n");
            return FAIL;
        }
        
        acs_ip1 = acs_server->h_addr_list;
        
        strcpy(acs_ip, (char *)inet_ntop(acs_server->h_addrtype, *acs_ip1, str,
sizeof(str)));
    }

    if(name_flag)
        LOG(m_device, DEBUG, "The acs name is %s\n", acs_name);
  
    LOG(m_device, DEBUG, "The acs ip is %s\n", acs_ip);
    LOG(m_device, DEBUG, "The acs port is %s\n", acs_port);

    //get acs path
    ptr = strstr(buf, "/");
    if(ptr == NULL)
    {
        strcpy(acs_path, "/");
    }
    else
    {
        strcpy(acs_path, ptr);
    }
    LOG(m_device, DEBUG, "The acs path is %s\n", acs_path);

    return SUCCESS;
}
