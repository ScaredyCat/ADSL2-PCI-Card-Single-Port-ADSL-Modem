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
*	history: $Date: 2007-06-08 02:22:42 $, initial version by simonl
*
***********************************************************************/

/***********************************************************************
*
*	include file
*
***********************************************************************/

#include "auth.h"

/************************************************************************
*
*	global var define
*
************************************************************************/

/***********************************************************************
*
*	function prototype
*
***********************************************************************/

/***********************************************************************
*
*	get_basic_auth_info - Get the string of basic authorization header field which will be put in HTTP head.
*	@param:
*		auth_head is the string of basic authorization header field which will be put in HTTP head.
*	@return value
*		0 means success
*            -1 means failed
*
***********************************************************************/
int get_basic_auth_info(char *auth_head, char *username, char *password)
{
    char input[128];
    char output[128];
    int length;

    sprintf(input, "%s:%s", username, password);

    /*Encode "username:password" using base64*/
    length = Base64Enc((unsigned char *)output, (const unsigned char *)input, strlen(input));

    LOG(m_auth, DEBUG, "The encoded string is %s length of %d\n", output, length);

    sprintf(auth_head, "Authorization: Basic %s", output);

    return 0;
}

/***********************************************************************
*
*	get_digest_auth_info - Get the string of digest authorization header field which will be put in HTTP head.
*	@param:
*            digest_auth is the point to a TR_digest_auth structure which stores the information need to generate the digest response value.
*            auth_head is the string of digest authorization header field which will be put in HTTP head.
*	@return value
*		0 means success
*            -1 means failed
*
***********************************************************************/
int get_digest_auth_info(TR_digest_auth *digest_auth, char *auth_head)
{
    HASHHEX HA1;
    HASHHEX HA2 = "";
    HASHHEX Response;

    /*calculate the digest response*/
    DigestCalcHA1(digest_auth->pszAlg, digest_auth->pszUser, digest_auth->pszRealm, digest_auth->pszPass, digest_auth->pszNonce, digest_auth->pszCNonce, HA1);
    DigestCalcResponse(HA1, digest_auth->pszNonce, digest_auth->szNonceCount, digest_auth->pszCNonce, digest_auth->pszQop, digest_auth->pszMethod, digest_auth->pszURI, HA2, Response);

    LOG(m_auth, DEBUG, "The digest response string is %s\n", Response);

    sprintf(auth_head, "Authorization: Digest username=\"%s\", realm=\"%s\", qop=\"%s\", algorithm=\"%s\", uri=\"%s\", nonce=\"%s\", nc=%s, cnonce=\"%s\", response=\"%s\"", digest_auth->pszUser, digest_auth->pszRealm, digest_auth->pszQop, digest_auth->pszAlg, digest_auth->pszURI, digest_auth->pszNonce, digest_auth->szNonceCount, digest_auth->pszCNonce, Response);

    return 0;
}


