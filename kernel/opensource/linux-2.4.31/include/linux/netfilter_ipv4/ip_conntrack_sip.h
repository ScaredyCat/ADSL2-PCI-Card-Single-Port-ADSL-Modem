/* ============================================================================
 * Copyright (C) 2003[- 2004] ? Infineon Technologies AG.
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
	UDP
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
	REGISTER,		//11
	EXPIRE_TIMER,		//12
	ENDOFTOKEN		
};

typedef struct{
	enum TokenType token;
	char* tokenlist[MAX_NO_TOKEN];
}messagetokentype;

#endif
enum call_origination{ INSIDE_INITIATION = 0 , OUTSIDE_INITIATION = 1};
enum MESSAGE_DIR { IN_2_OUT = 0, OUT_2_IN = 1 };
enum MEDIA_TYPE { AUDIO_RTP = 1, AUDIO_RTCP = 2, IMAGE_DATA = 3, VIDEO_RTP = 4, VIDEO_RTCP = 5, IMAGE_RTP = 6 };
enum EXPECTATION_TYPE { IMPLICIT = 0, EXPLICIT = 1 };

/* This structure is per expected connection */
struct ip_ct_sip_expect {
	char call_id[CALL_ID_LENGTH];
	int key_algport;
	char flag;			/* To figure out whether this expectation has been
					   mapped by NAT or not */
	u_int16_t orig_port;			/* Port of the sip helper/RTCP/RTP channel */
	u_int16_t changed_port;			/* Changed Port by NAT */
	u_int32_t localip;				/* Local ip address */
	int exp_offset;					/* Offset where this expectation is generated from */
	enum MEDIA_TYPE	media_type;		/* The media type of this expectation */
	enum EXPECTATION_TYPE exp_type;
	enum MESSAGE_DIR expectations_hit_direction;
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

#endif
