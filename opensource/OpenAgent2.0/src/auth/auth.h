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

/**********************************************************************
*
*	auth.h - HTTP Authentication Module
*
*	$Author: joinsonj $
*
*	history: $Date: 2007-06-08 02:22:46 $, initial version by simonl
*
***********************************************************************/

#ifndef AUTH_H_
#define AUTH_H_

/***********************************************************************
*
*	include file
*
***********************************************************************/

#include "../res/global_res.h"
//#include "../device/dev_param.h"
#include "basic/base64.h"
#include "digest/digcalc.h"

/************************************************************************
*
*	structure define
*
************************************************************************/

typedef struct digest_auth 
{
    char pszNonce[200];                  /* Nonce value specified by ACS */ 
    char pszCNonce[200];                /* Nonce value specified by Agent */ 
    char pszUser[76];                    /* Username of Agent device */
    char pszRealm[100];                  /* The string to be displayed to users */
    char pszPass[48];                    /* Password of Agent device */
    char pszAlg[40];                      /* Algorithm, default is md5 */
    char szNonceCount[9];        /* The hexadecimal count of the number of requests
                                                    that the client has sent with the nonce value in this request */
    char pszMethod[20];                /* HTTP request method name */
    char pszQop[12];                     /* quality of protection. "auth" or "auth-int" or empty */
    char pszURI[200];                      /* The URI from Request-URI of the Request-Line */
}TR_digest_auth;


/************************************************************************
*
*	macrodefinition
*
************************************************************************/



/************************************************************************
*
*	global var define
*
************************************************************************/



/***********************************************************************
*
*	function declare
*
***********************************************************************/

extern int get_basic_auth_info(char *auth_head, char *username, char *password);

extern int get_digest_auth_info(TR_digest_auth *digest_auth, char *auth_head);

#endif /* AUTH_H_ */

