/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_req.h
 *
 * $Id: sip_req.h,v 1.25 2005/07/22 13:40:12 tyhuang Exp $
 */

#ifndef SIP_REQ_H
#define SIP_REQ_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sip_hdr.h"
#include "sip_cm.h"
#include "sip_bdy.h"
#include "rfc822.h"

typedef struct sipReqObj*	SipReq;

			/* Create a new sip request message */
			/* Return value is a sip request message object pointer */
CCLAPI SipReq		sipReqNew(void);		

			/* Free a sip request message */
CCLAPI void		sipReqFree(IN SipReq);		

			/* Duplicate a sip request message */
CCLAPI SipReq		sipReqDup(SipReq _source);

			/* Input a request line for sip request message */
CCLAPI RCODE		sipReqSetReqLine(IN SipReq,IN SipReqLine*);

			/* Retrieve request line from request message */
			/* Return a sip request line pointer to a reqline structure */
CCLAPI SipReqLine*	sipReqGetReqLine(IN SipReq);
			
			/* transfer Method Type To Name */
CCLAPI const char*	sipMethodTypeToName(SipMethodType);

			/* Generic form to add a request header */
CCLAPI RCODE		sipReqAddHdr(IN SipReq,IN SipHdrType,IN void*);

			/* Generic form to add a request header from character string */
CCLAPI RCODE		sipReqAddHdrFromTxt(IN SipReq,IN SipHdrType,IN char*);

			/* Retrieve request header from request message */
			/* return a pointer to request header structure, must cast to specific type */
CCLAPI void*		sipReqGetHdr(IN SipReq,IN SipHdrType);

			/* Retrieve request message body*/
			/* return a pointer to request message body*/
CCLAPI SipBody*		sipReqGetBody(IN SipReq);

			/* Add the message body into sip request message */
CCLAPI RCODE		sipReqSetBody(IN SipReq,IN SipBody*);

			/* Create a new request message from a message buffer */
			/* If success, get a new sip request message object pointer */
			/* else (failed), get a NULL pointer */
CCLAPI SipReq		sipReqNewFromTxt(IN char*);

			/* Transfer a sip request message into character string */
			/* return a pointer to point a buffer */ 
CCLAPI char*		sipReqPrint(IN SipReq);

			/* to set unknwon header buf in request */
CCLAPI RCODE		sipReqAddUnknowHdr(IN SipReq,IN const char*);

			/* Add/delete  Via header for request message */
CCLAPI RCODE		sipReqAddViaHdrTop(IN SipReq,char*);
CCLAPI RCODE		sipReqDelViaHdrTop(IN SipReq);

			/* Add/delete Route header for request message */
			/* pos = 0 : top; pos = 1:Last */
CCLAPI RCODE		sipReqAddRouteHdr(IN SipReq,int pos, char*);
CCLAPI RCODE		sipReqDelRouteHdrTop(IN SipReq);

			/* set new value for max-forward of request message */
CCLAPI RCODE		sipReqSetMaxForward(IN SipReq,IN unsigned int);

/* Remove SIP header from SipReq,added by tyhuang 2005/6/9 */
CCLAPI RCODE		sipReqDelHdr(IN SipReq,IN SipHdrType);

/* get original header list */
CCLAPI MsgHdr		sipReqGetHdrLst(IN SipReq);

/***********************************************************************
*	Authorization  ( Digest and Basic )
***********************************************************************/

typedef struct {
	char			username[64];
	char			passwd[128];
} SipBasicAuthz;

/* flags in 'SipDigestAuthz' */
#define SIP_DAFLAG_NONE		0x0 
#define SIP_DAFLAG_ALGORITHM	0x1
#define SIP_DAFLAG_CNONCE	0x2
#define SIP_DAFLAG_OPAQUE	0x4
#define SIP_DAFLAG_MESSAGE_QOP	0x8
#define SIP_DAFLAG_NONCE_COUNT	0x10

typedef struct {
	unsigned int		flag_;
	char			username_[64];		
	char			realm_[128];
	char			nonce_[256];
	char			digest_uri_[256];
	char			algorithm_[64];		/* must set flag */
	char			cnonce_[256];		/* must set flag */
	char			opaque_[256];		/* must set flag */
	char			message_qop_[64];	/* must set flag */
	char			nonce_count_[256];	/* must set flag */
	char			response_[33];		/* Generate from the previous parameters */
} SipDigestAuthz;

typedef struct {
	SipChallengeType	iType;
	union {
		SipBasicAuthz	basic_;    /* BASIC authenticate method, obsoleted at new version*/
		SipDigestAuthz	digest_;
	} auth_;
} SipAuthzStruct;

/* Get/Add Authorization header for Authorization and Proxy-Authorization header*/
CCLAPI RCODE		sipReqAddAuthz(IN SipReq, IN SipAuthzStruct);
CCLAPI RCODE		sipReqGetAuthz(IN SipReq, IN OUT SipAuthzStruct*);
CCLAPI RCODE		sipReqAddPxyAuthz(IN SipReq, IN SipAuthzStruct);
CCLAPI RCODE		sipReqGetPxyAuthz(IN SipReq, IN OUT SipAuthzStruct*);

/* method = request method */
/* entity_body = optional, 32-Byte char array. It is only needed when (auth->message_qop_ == "auth-init") */
/* Generate Response-key contained in SipDigestAuthz data structure */
CCLAPI RCODE		sipDigestAuthzGenRspKey(IN OUT SipDigestAuthz* auth,IN char* password,IN char* method,IN char* entity_body);

#ifdef  __cplusplus
}
#endif

#endif /* SIP_REQ_H */
