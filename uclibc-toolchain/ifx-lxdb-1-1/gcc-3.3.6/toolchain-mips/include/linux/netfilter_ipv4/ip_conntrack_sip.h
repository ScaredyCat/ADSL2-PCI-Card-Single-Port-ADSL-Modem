/* ============================================================================
 * Copyright (C) 2003[- 2004]  Infineon Technologies AG.
 *
 * All rights reserved.
 * ============================================================================
 *
 * ============================================================================
 *
 * This document contains proprietary information belonging to Infineon 
 * Technologies AG. Passing on and copying of this document, and communication
 * of its contents is not permitted without prior written authorisation.
 * 
 * ============================================================================
 */


/* ===========================================================================
 *
 * File Name: ip_conntrack_sip.h
 * Author : Atanu Mondal
 * Date: 
 *
 * ===========================================================================
 *
 * Project: <project/component name>
 * Block: <block/module name>
 *
 * ===========================================================================
 * Contents:  This file contains the data structures and definitions used 
 * 	      by the core iptables and the sip alg modules. 
 * ===========================================================================
 * References: <List of design documents covering this file.>
 */




#ifndef __IP_CONNTRACK_SIP_H__
#define __IP_CONNTRACK_SIP_H__

#ifdef __KERNEL__

#include <linux/netfilter_ipv4/lockhelp.h>

DECLARE_LOCK_EXTERN(ip_sip_lock);

#endif /* __KERNEL__ */

#define SIP_PORT	5060	
#define MAX_NO_TOKEN 20		
#define CALL_ID_LENGTH 100
#define MAX_PORT_SIZE 10
#define MAX_METHOD_SIZE 16
#define MAX_EXPECTATION 100
#define MAX_IP_ADDRESS_SIZE 16


#if 1
enum BeginOrEnd
{
	BEGIN,
	END
};

enum SipControlProtocol
{
	TCP=1,
	UDP,
	TCPUDP
};

enum WhatToChange
{
	SRC_IP_ADDR,
	DST_IP_ADDR
};

enum SeqNumberDecision
{
	FIRST_TIME_ENTRY,
	FIRST_CHANGE_DONE,
	RETRANSMISSION
};

enum TokenType{
	CALLID=0,		//0
	CONTACT,		//1
	FROM,			//2
	CONTENT_LENGTH,		//3
	CONTENT_TYPE,		//4
	SUBJECT,		//5
	TO,			//6
	VIA,			//7
	RECORD_ROUTE,		//8
	ROUTE,			//9
	REQUEST_URI,		//10
	ENDOFTOKEN		
};

typedef struct{
	enum TokenType token;
	char* tokenlist[MAX_NO_TOKEN];
}messagetokentype;

#endif
enum call_origination{ INSIDE_INITIATION, OUTSIDE_INITIATION};

/* This structure is per expected connection */
struct ip_ct_sip_expect {
	char call_id[100];
	enum call_origination originator; /* You need to store infor abt who has
						     who has initiated this call */
	char flag;			/* To figure out whether this expectation has been
					   mapped by NAT or not */
	u_int16_t port;			/* Port of the sip helper/RTCP/RTP channel */

	enum ip_conntrack_dir dir;	/* Direction of the original connection */
	unsigned int offset;		/* offset of the address in the payload */


	/* Below data will be required for
	   taking decision in changing the
	   child conntrack
	*/
	u_int32_t exp_old_dst_addr;
//	u_int32_t exp_dst_addr;
//	u_int16_t exp_dst_port;
	enum call_origination expected_hit_direction;
//	enum WhatToChange change_type;
//	u_int32_t int_cli_addr;
//	u_int32_t ext_cli_addr;
	uint len;
	
};

/* This structure exists only once per master */
struct ip_ct_sip_master {
	int is_sip;				/* is sip connection..probably not needed */
#ifdef CONFIG_IP_NF_NAT_NEEDED
	enum ip_conntrack_dir dir;		/* Direction of the original connection */
	enum SeqNumberDecision SeqNumberingFlag;/* To calculate or not to calculate seq number in TCP */
	u_int32_t seq[IP_CT_DIR_MAX];		/* Exceptional packet mangling for signal addressess... */
	unsigned int offset[IP_CT_DIR_MAX];	/* ...and the offset of the addresses in the payload */
#endif
};

#if 0 
messagetokentype messagetoken[]=
{
	{CALLID,{"Call-ID:","i:","****"}},			//0
	{CONTACT,{"Contact:", "m:", "****"}},			//1
	{FROM,{"From:", "f:","****"}},				//2
	{CONTENT_LENGTH,{"Content-Length:", "l:","****"}},	//3
	{CONTENT_TYPE,{"Content-Type:","c:","****"}},		//4
	{SUBJECT,{"Subject:","s:","****"}},			//5
	{TO,{"To:","t:","****"}},				//6
	{VIA,{"Via:","v:","****"}},				//7
	{RECORD_ROUTE,{"Record-Route:","****"}},		//8
	{ROUTE,{"Route:","****"}},				//9
	{REQUEST_URI,{"INVITE","ACK","OPTIONS","BYE","CANCEL","REGISTER","****"}},
	{ENDOFTOKEN,{"****"}}
};
#endif
#endif
