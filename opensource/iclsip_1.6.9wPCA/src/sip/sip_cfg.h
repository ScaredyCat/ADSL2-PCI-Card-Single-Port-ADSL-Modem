/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_cfg.h
 *
 * $Id: sip_cfg.h,v 1.22 2005/04/21 09:58:14 tyhuang Exp $
 */

#ifndef SIP_CFG_H
#define SIP_CFG_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "sip_cm.h"
#include "sip_tx.h"
#include <common/cm_def.h>


typedef struct {
	UINT16		tcp_port;	/* TCP/UDP listen port number */
	LogFlag		log_flag;
	TraceLevel	trace_level;	
	char		fname[64];	/* log file name */
	char		raddr[128];	/* log server IP address */
	UINT16		rport;		/* log server port */
	char		laddr[128];	/* local IP address */
	int		limithdrset;	/* flag for limiting set of header fields */ 
	UINT16		tls_port;	/* TLS listem port number ;0 is off */
} SipConfigData;

			/* initialize protocol stack(transport type TCP or UDP, 
						     TCP callback function, 
						     UDP callback function, 
						     configuration data) */
CCLAPI RCODE		sipLibInit(IN SipTransType, 
				   IN SipTCPEvtCB, 
				   IN SipUDPEvtCB, 
				   IN SipConfigData);

#ifdef CCL_TLS
CCLAPI RCODE		sipLibInitWithTLS(IN SipTransType, 
				   IN SipTCPEvtCB, 
				   IN SipUDPEvtCB,
				   IN SipTLSEvtCB,
				   IN int tlsport,
				   IN SipConfigData);
#endif
			/* shutdown SIP protocol stack */
CCLAPI RCODE		sipLibClean(void); 

			/* config protocol stack */
			/* This API not implement now */
CCLAPI RCODE		sipConfig(IN SipConfigData*); 

			/*Generate Call-ID header*/
                        /*host: input host name, ip: input ip address */
			/* return the char pointer to Call-ID string .it is not thread-safe */
CCLAPI unsigned char*	sipCallIDGen(IN char* host, IN char* ip); 

CCLAPI RCODE CallIDGenerator(IN char* host,IN char* ipaddr,OUT char* buffer,IN OUT int* length);

/* memory function */
CCLAPI void* sipMMalloc(int size);	
CCLAPI void* sipMCalloc(int n, int size);
CCLAPI void* sipMRalloc(void *p, int size);									
CCLAPI void sipMFree(void *p);

#ifdef  __cplusplus
}
#endif

#endif /* SIP_CFG_H */
