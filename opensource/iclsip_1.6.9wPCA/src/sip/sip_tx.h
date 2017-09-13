/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sip_tx.h
 *
 * $Id: sip_tx.h,v 1.29 2005/07/21 06:54:59 tyhuang Exp $
 */

#ifndef SIP_TX_H
#define SIP_TX_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sip_req.h"
#include "sip_rsp.h"
#include "sip_hdr.h"

typedef struct sipTCPObj*	SipTCP;
typedef struct sipUDPObj*	SipUDP;
typedef struct sipTLSObj*	SipTLS;

/* define transport type, using TCP/UDP */
typedef enum {
	SIP_TRANS_UNKN			=0,
	SIP_TRANS_UDP			=0x01,
	SIP_TRANS_TCP_CLIENT		=0x02,
	SIP_TRANS_TCP_SERVER		=0x04,
	SIP_TRANS_TLS_CLIENT	=0x08,
	SIP_TRANS_TLS_SERVER	=0x16,
	SIP_TRANS_END			=0x32
	/*SIP_TRANS_END			=0x08,*/
} SipTransType;

typedef enum {
	SIP_TCPEVT_UNKN			=0,
	SIP_TCPEVT_CLIENT_CONN		=0x01,
	SIP_TCPEVT_CLIENT_CLOSE		=0x02,
	SIP_TCPEVT_SERVER_CLOSE		=0x04,
	SIP_TCPEVT_DATA			=0x08,
	SIP_TCPEVT_END			=0x10,
	SIP_TCPEVT_CONNECTED		=0x20
} SipTCPEvtType ;

typedef enum {
	SIP_TLSEVT_UNKN			=0,
	SIP_TLSEVT_CLIENT_CONN		=0x01,
	SIP_TLSEVT_CLIENT_CLOSE		=0x02,
	SIP_TLSEVT_SERVER_CLOSE		=0x04,
	SIP_TLSEVT_DATA			=0x08,
	SIP_TLSEVT_END			=0x10,
	SIP_TLSEVT_CONNECTED		=0x20
} SipTLSEvtType ;

typedef enum {
	SIP_UDPEVT_UNKN			=0,
	SIP_UDPEVT_DATA			=0x01
} SipUDPEvtType;

typedef void (*SipTLSEvtCB)(SipTLS tls, SipTLSEvtType event, SipMsgType msgtype, void* msg);
typedef void (*SipTCPEvtCB)(SipTCP tcp, SipTCPEvtType event, SipMsgType msgtype, void* msg);
typedef void (*SipUDPEvtCB)(SipUDP udp, SipUDPEvtType event, SipMsgType msgtype, void* msg);
typedef void (*sipTimerCB)(int e,void* data);

			/* Create a new TCP object */
CCLAPI SipTCP		sipTCPNew(const char* laddr, UINT16 lport);
CCLAPI SipTCP		sipTCPSrvNew(const char* raddr, UINT16 rport);

			/* Establish a TCP connection */
			/* input a tcp object pointer and IP address/port for remote side */
CCLAPI RCODE		sipTCPConnect(IN SipTCP,IN const char* raddr,IN UINT16 rport);

			/* free a TCP object */
CCLAPI void		sipTCPFree(IN SipTCP);

			/* free a TCP connect, connect from client side */
CCLAPI void		sipTCPServerFree(IN SipTCP);

			/* Send out a buffer of character string using a exist TCP connect*/
			/* return value = how many bytes had been sent */
CCLAPI int		sipTCPSendTxt(IN SipTCP, IN char* msgtext);

			/* Send out a sip request message using a exist TCP connection */
			/* return value = how many bytes had been sent */
CCLAPI int		sipTCPSendReq(IN SipTCP, IN SipReq);

			/* Send out a sip response message using a exist TCP connection */
			/* return value = how many bytes had been sent */
CCLAPI int		sipTCPSendRsp(IN SipTCP, IN SipRsp);

			/* Get remote side address from a TCP object */
			/* return value: remote side IP address */
CCLAPI const char*	sipTCPGetRaddr(IN SipTCP);

			/* Get remote side port number from a TCP object */
			/* return value: remote side port number */
/*CCLAPI UINT16		sipTCPGetRport(IN SipTCP);*/
CCLAPI RCODE	sipTCPGetRport(SipTCP _this,UINT16 *port);

CCLAPI const char*	sipTCPGetLaddr(SipTCP _this);

#ifdef CCL_TLS
			/* Create a new TLS object */
CCLAPI SipTLS		sipTLSNew(const char* laddr, UINT16 lport);
CCLAPI SipTLS		sipTLSSrvNew(const char* raddr, UINT16 rport);

			/* Establish a TLS connection */
			/* input a tcp object pointer and IP address/port for remote side */
CCLAPI RCODE		sipTLSConnect(IN SipTLS,IN const char* raddr,IN UINT16 rport);

			/* free a TLS object */
CCLAPI void		sipTLSFree(IN SipTLS);

			/* free a TLS connect, connect from client side */
CCLAPI void		sipTLSServerFree(IN SipTLS);

			/* Send out a buffer of character string using a exist TLS connect*/
			/* return value = how many bytes had been sent */
CCLAPI int		sipTLSSendTxt(IN SipTLS, IN char* msgtext);

			/* Send out a sip request message using a exist TLS connection */
			/* return value = how many bytes had been sent */
CCLAPI int		sipTLSSendReq(IN SipTLS, IN SipReq);

			/* Send out a sip response message using a exist TLS connection */
			/* return value = how many bytes had been sent */
CCLAPI int		sipTLSSendRsp(IN SipTLS, IN SipRsp);

			/* Get remote side address from a TLS object */
			/* return value: remote side IP address */
CCLAPI const char*	sipTLSGetRaddr(IN SipTLS);

			/* Get remote side port number from a TLS object */
			/* return value: remote side port number */
CCLAPI RCODE	sipTLSGetRport(SipTLS _this,UINT16 *port);
#endif
			/* Create a new UDP object */
			/* if failed, return NULL */
CCLAPI SipUDP		sipUDPNew(IN const char* laddr, IN UINT16 lport);

			/* free a UDP object */
CCLAPI void		sipUDPFree(IN SipUDP);

			/* Get remote side IP address from a UDP object */
			/* return a pointer to a charter string, if failed return NULL */ 
CCLAPI const char*	sipUDPGetRaddr(IN SipUDP);

			/* Get remote side port number from a UDP object */
			/* return a port number, if failed : return 0 */ 
CCLAPI RCODE sipUDPGetRport(SipUDP _this,UINT16 *port);

			/* Send out a buffer of character string*/
			/* return value = how many bytes had been sent */
CCLAPI int		sipUDPSendTxt(IN SipUDP , IN char* msgtext, IN char* raddr, IN UINT16 rport);

			/* Send out a sip request message using a exist UDP */
			/* return value = how many bytes had been sent */
CCLAPI int		sipUDPSendReq(IN SipUDP,IN SipReq,IN const char* raddr,IN UINT16 rport);

			/* Send out a sip response message using a exist UDP */
			/* return value = how many bytes had been sent */
CCLAPI int		sipUDPSendRsp(IN SipUDP,IN SipRsp,IN const char* raddr,IN UINT16 rport);

CCLAPI unsigned short	sipUDPGetRefCount(SipUDP _this);

CCLAPI unsigned short	sipUDPIncRefCount(SipUDP _this);

CCLAPI unsigned short	sipUDPDecRefCount(SipUDP _this);

			/* Retrieve event from event queue */
			/* return how many event retrieve, if failed, return -1 */
CCLAPI int		sipEvtDispatch(IN int timeout/*msec; -1 for blocking*/);

			/* Set a timer, when time up, it will callback to function cb with parameter data */
			/* return timer ID, if failed return -1 */
CCLAPI int		sipTimerSet(IN int delay/*msec*/,IN sipTimerCB cb,IN void* data);

			/* delete timer, return from sipTimerSet() */
CCLAPI void*		sipTimerDel(IN int e);

#ifdef  __cplusplus
}
#endif

#endif /* SIP_TX_H */
