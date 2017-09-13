/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_url.h
 *
 * $Id: sip_url.h,v 1.15 2005/01/12 06:35:26 tyhuang Exp $
 */

#ifndef SIP_URL_H
#define SIP_URL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sip_cm.h"

#define SIPURL_TOKEN		"sip:"
#define SIPSURL_TOKEN		"sips:"
#define SIPURL_AT		"@"


#define SIPURLPARAM_NONE	0x0
#define	SIPURLPARAM_VALUE	0x1

/* modified by mac during SipiT 10 ( Apr 23, 2002 ) */
#define MAX_PARAM_LENGTH	(256)
#define MAX_HEADER_LENGTH	(256)

typedef struct {
	unsigned short		flag_;
	char			pname_[MAX_PARAM_LENGTH];
	char			pvalue_[MAX_PARAM_LENGTH];	/* must set flag */
} SipURLParam;

typedef struct {
	char			hname_[MAX_HEADER_LENGTH];
	char			hvalue_[MAX_HEADER_LENGTH];
} SipURLHdr;

/* modified by mac during SipiT 10 ( Apr 23, 2002 ) */
void InitializeURLParam ( SipURLParam * pParam );
void InitializeURLHdr ( SipURLHdr * pHeader );


typedef struct sipURLObj*	SipURL;

			/* Create a new URL object */
			/* if success, return a SipURL object pointer */
CCLAPI SipURL		sipURLNew(void);
CCLAPI void		sipURLFree(IN SipURL);	/* free a URL object */
CCLAPI SipURL		sipURLDup(IN SipURL);	/* Duplicate a sip URL, return a new sip URL object */
CCLAPI SipURL		sipURLNewFromTxt(IN const char* uristr); /* Create a new SIP URL from a character string buffer */
CCLAPI char*		sipURLToStr(IN SipURL);	/* Transfer a SIP URL into character string */

CCLAPI RCODE		sipURLSetScheme(IN SipURL, IN const char* scheme);
CCLAPI char*		sipURLGetScheme(IN SipURL);
			/* modify URL user parameter */
CCLAPI RCODE		sipURLSetUser(IN SipURL, IN const char* user);
CCLAPI char*		sipURLGetUser(IN SipURL);
CCLAPI RCODE		sipURLDelUser(IN SipURL);

			/* modify URL password parameter */
CCLAPI RCODE		sipURLSetPasswd(IN SipURL, IN const char* passwd);
CCLAPI char*		sipURLGetPasswd(IN SipURL);
CCLAPI RCODE		sipURLDelPasswd(IN SipURL);

			/* modify URL host parameter */
CCLAPI RCODE		sipURLSetHost(IN SipURL,IN const char* host);
CCLAPI char*		sipURLGetHost(IN SipURL);
CCLAPI RCODE		sipURLDelHost(IN SipURL);

			/* modify URL port parameter */
CCLAPI RCODE		sipURLSetPort(IN SipURL, IN unsigned int port);
CCLAPI unsigned int	sipURLGetPort(IN SipURL);
CCLAPI RCODE		sipURLDelPort(IN SipURL);

			/* modify URL general parameters */
CCLAPI RCODE		sipURLAddParamAt(IN SipURL,IN SipURLParam parm, IN int position);
CCLAPI SipURLParam*	sipURLGetParamAt(IN SipURL,IN int position);
CCLAPI RCODE		sipURLDelParamAt(IN SipURL,IN int position);
CCLAPI int		sipURLGetParamSize(IN SipURL);

			/* modify URL header */
CCLAPI RCODE		sipURLAddHdrAt(IN SipURL, IN SipURLHdr head,IN int position);
CCLAPI SipURLHdr*	sipURLGetHdrAt(IN SipURL, IN int position);
CCLAPI RCODE		sipURLDelHdrAt(IN SipURL, IN int position);
CCLAPI int		sipURLGetHdrSize(IN SipURL);

#ifdef  __cplusplus
}
#endif

#endif /* SIP_URL_H */
