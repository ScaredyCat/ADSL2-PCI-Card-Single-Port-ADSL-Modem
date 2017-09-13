/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_rsp.h
 *
 * $Id: sip_rsp.h,v 1.25 2005/06/09 06:47:46 tyhuang Exp $
 */

#ifndef SIP_RSP_H
#define SIP_RSP_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include <common/cm_utl.h>
#include "sip_cm.h"
#include "sip_bdy.h"
#include "sip_hdr.h"
#include "sip_url.h"
#include "rfc822.h"

typedef struct sipRspObj*	SipRsp;

				/* Create a new sip response message */
				/* Return value is a sip response message object pointer */
CCLAPI SipRsp			sipRspNew(void);

				/* Free a sip response message */
CCLAPI void			sipRspFree(IN SipRsp);
				
				/* Duplicate a sip response message */ 
CCLAPI SipRsp			sipRspDup(SipRsp _source);

				/* Input a response status line for sip response message */
CCLAPI RCODE			sipRspSetStatusLine(IN SipRsp,IN SipRspStatusLine*);

				/* Retrieve response status line from response message */
				/* Return a sip response status line pointer to a status line structure */
CCLAPI SipRspStatusLine*	sipRspGetStatusLine(IN SipRsp); 

				/* Retrieve response header from response message */
				/* return a pointer to response header structure, must cast to specific type */
CCLAPI void*			sipRspGetHdr(IN SipRsp,IN SipHdrType);

				/* Generic form to add a response header */
				/* the sipHdrType must match to void * data type */
CCLAPI RCODE			sipRspAddHdr(IN SipRsp,IN SipHdrType,IN void *);

				/* Generic form to add a response header from character string */
CCLAPI RCODE			sipRspAddHdrFromTxt(IN SipRsp,IN SipHdrType,IN char*);


				/* Retrieve response message body*/
				/* return a pointer to response message body*/
CCLAPI SipBody*			sipRspGetBody(IN SipRsp);

				/* Add the message body into sip response message */
CCLAPI RCODE			sipRspSetBody(IN SipRsp,IN SipBody*);

				/* Create a new response message from a message buffer */
				/* If success, get a new sip response message object pointer */
				/* else (failed), get a NULL pointer */
CCLAPI SipRsp			sipRspNewFromTxt(IN char*);

				/* Transfer a sip response message into character string */
				/* return a pointer to point a buffer */ 
CCLAPI char*			sipRspPrint(IN SipRsp);

				/* to set unknown header buf in response */
CCLAPI RCODE			sipRspAddUnknowHdr(IN SipRsp ,IN const char*);

				/* remove topmost via header in response ,request from ¤O¤¤*/
CCLAPI RCODE			sipRspDelViaHdrTop(IN SipRsp _this);
CCLAPI RCODE			sipRspDelAllViaHdr(IN SipRsp _this);

				/* add record-route */
CCLAPI RCODE			sipRspAddRecRouteHdr(IN SipRsp ,int , char*);

/* Remove SIP header from SipReq,added by tyhuang 2005/6/9 */
CCLAPI RCODE			sipRspDelHdr(IN SipRsp,IN SipHdrType);

/* get original header list */
CCLAPI MsgHdr			sipRspGetHdrLst(IN SipRsp);
/***********************************************************************
*
*	WWW Authenticate  ( Digest and Basic )
*
***********************************************************************/
typedef struct {
	char	realm_[128];
} SipBasicWWWAuthn;

/* flags in 'SipDigestWWWAuthn' */
#define SIP_DWAFLAG_NONE	0x0
#define SIP_DWAFLAG_DOMAIN	0x1
#define SIP_DWAFLAG_NONCE	0x2
#define SIP_DWAFLAG_OPAQUE	0x4
#define SIP_DWAFLAG_STALE	0x8
#define SIP_DWAFLAG_ALGORITHM	0x10
#define SIP_DWAFLAG_QOP		0x20

typedef struct {
	unsigned int	flags_;
	char		realm_[128];
	char		domain_[256];		/* must set flag */
	char		nonce_[256];		/* must set flag */
	char		opaque_[256];		/* must set flag */
	BOOL		stale_;			/* must set flag */
	char		algorithm_[64];		/* must set flag */
	char		qop_[64];		/* must set flag */
} SipDigestWWWAuthn;

typedef struct {
	SipChallengeType		iType;
	union {
		SipBasicWWWAuthn	basic_;
		SipDigestWWWAuthn	digest_;
	} auth_;
} SipAuthnStruct;

/* Get/Add Authenticate header for WWW-Authenticate and Proxy-Authenticate header*/
CCLAPI RCODE	sipRspAddWWWAuthn(IN SipRsp, IN SipAuthnStruct);
CCLAPI RCODE	sipRspGetWWWAuthn(IN SipRsp, IN OUT SipAuthnStruct*);
CCLAPI RCODE	sipRspAddPxyAuthn(IN SipRsp, IN SipAuthnStruct);
CCLAPI RCODE	sipRspGetPxyAuthn(IN SipRsp, IN OUT SipAuthnStruct*);

#ifdef  __cplusplus
}
#endif

#endif /* SIP_RSP_H */
