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
 * File Name: (From version control system)
 * Author : Atanu Mondal
 * Date: 
 *
 * ===========================================================================
 *
 * Project: <project/component name>
 * Block: <block/module name>
 *
 * ===========================================================================
 * Contents: This file contains the common definitions required by the
 * 	     conntrack and the nat helper files.
 * 
 * ===========================================================================
 * References: <List of design documents covering this file.>
 */



#ifndef __IFX_SIP_COMMON_H__
#define __IFX_SIP_COMMON_H__

#include <linux/netfilter_ipv4/ip_conntrack_sip.h>
#include <linux/ioctl.h>

#if 1
struct sip_params
{
	int sip_src_port;
	int sip_dst_port;
	enum SipControlProtocol proto;
};

struct sip_dnat_param
{
	int original_port;
	int changed_port;
	char original_ip_address[16];
	char changed_ip_address[16];
	enum SipControlProtocol proto;
	int typeofchange;
	int validdataflag;
};
#endif

enum PORT_TYPE { SRC_PORT = 1, DST_PORT };

enum oper_type {STATIC_OPER = 0, DYNAMIC_OPER = 1};

/* The major device number. We can't rely on dynamic 
 * registration any more, because ioctls need to know 
 * it. */
#define MAJOR_NUM 				233
#define SIP_CONN_MAJOR_NUM 			234


/* Set the message of the device driver */
#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, char *)
/* _IOR means that we're creating an ioctl command 
 * number for passing information from a user process
 * to the kernel module. 
 *
 * The first arguments, MAJOR_NUM, is the major device 
 * number we're using.
 *
 * The second argument is the number of the command 
 * (there could be several with different meanings).
 *
 * The third argument is the type we want to get from 
 * the process to the kernel.
 */

/* Register the port and protocol*/
#define IOCTL_REGISTER_PORT _IOWR(MAJOR_NUM, 2, struct sip_params*)
#define IOCTL_REGISTER_CONN_PORT _IOWR(SIP_CONN_MAJOR_NUM, 2, struct sip_params*)
 /* The IOCTL is used for both input and output. It 
  * receives from the user a number, n, and returns 
  * Message[n]. */

/* Deregister the port and the protocol */
#define IOCTL_DEREGISTER_PORT _IOWR(MAJOR_NUM, 3, struct sip_params*)
#define IOCTL_DEREGISTER_CONN_PORT _IOWR(SIP_CONN_MAJOR_NUM,3, struct sip_params*)



/* Read and block for knowing the port registration */
#define IOCTL_SIP_ALG_DAEMON _IOWR(MAJOR_NUM, 4, struct sip_dnat_param*)
//#define IOCTL_SIP_ALG_CONN_DAEMON _IOWR(SIP_CONN_MAJOR_NUM, 4, struct sip_params*)


/*This is only for testing. Can be removed later*/
#define IOCTL_SIP_TEST_CASE _IOWR(MAJOR_NUM, 5, struct sip_params*)

 /* This IOCTL is used for output, to get the message 
  * of the device driver. However, we still need the 
  * buffer to place the message in to be input, 
  * as it is allocated by the process.
  */



/* The name of the device file */
#define DEVICE_FILE_NAME "sip_dev"
#define DEVICE_CONN_FILE_NAME "sip_conn_dev"
#define DEVICE_CONN_NAME "sip_conn_dev"
#if 0
messagetokentype messagetoken[]=
{
	{CALLID,{"Call-ID:","i:","Call-Id:","call-id", "CALL-ID","****"}},			//0
	{CONTACT,{"Contact:","m:","CONTACT:","contact:", "****"}},			//1
	{FROM,{"From:", "f:","FROM:","from:","****"}},				//2
	{CONTENT_LENGTH,{"Content-Length:", "l:","CONTENT-LENGTH:", "content-length:","****"}},	//3
	{CONTENT_TYPE,{"Content-Type:","c:","CONTENT-TYPE:","content-type:","****"}},		//4
	{SUBJECT,{"Subject:","s:","SUBJECT:", "subject:","****"}},			//5
	{TO,{"To:","t:","TO:","to:","****"}},				//6
	{VIA,{"Via:","v:","VIA:", "via:","****"}},				//7
	{RECORD_ROUTE,{"Record-Route:","RECORD-ROUTE:","record-route:","****"}},		//8
	{ROUTE,{"Route:","ROUTE:","route:","****"}},				//9
	{REQUEST_URI,{"INVITE","ACK","OPTIONS","BYE","CANCEL","Invite","invite","Ack","ack","Options","options","Bye","bye","Cancel","cancel","****"}}, //10
	{REGISTER,{"REGISTER","Register","register","****"}},				//11
	{EXPIRE_TIMER, {"Expires:","EXPIRES:","expires:","****"}},     		//12
	{ENDOFTOKEN,{"****"}}
};

#endif

#define POS_FROM				0
#define POS_FROM_PORT			1
#define POS_FROM_SIP			2
#define POS_FROM_SIPS			3
#define POS_TO					4
#define POS_TO_PORT				5
#define POS_VIA_UDP_IP			6
#define POS_VIA_TCP_IP			7
#define POS_VIA_UDP_PORT		8
#define POS_VIA_TCP_PORT		9
#define POS_PROXY_AUTH			10
#define POS_CSEQ				11
#define POS_CALLID				12
#define POS_CALLID_IP			13
#define POS_CONTACT_IP			14
#define POS_CONTACT_PORT		15
#define POS_CONTACT_PORT_SPC	16
#define POS_CONTACT_IP_PORT		17
#define POS_CONTACT_TRANS		18
#define POS_CONTACT_EXPIRES		19
#define POS_EXPIRES				20
#define POS_REFER_TO			21
#define POS_REFERRED_BY			22
#define POS_CONTENT				23
#define POS_MEDIA_AUDIO_RTP		24
#define POS_MEDIA_AUDIO_RTCP	25
#define POS_MEDIA_IMAGE			26
#define POS_OWNER				27
#define POS_CONECTION			28
#define POS_REQ_HEADER_IP		29
#define POS_REQ_HEADER_PORT		30
#define POS_SDP_HEADER			31
#define POS_TO_SIP			32
#define POS_TO_SIPS			33
#define POS_RR				34
#define POS_RRS				35

struct sip_header_nfo {
	const char *lname;
	size_t lnlen;
	const char *sname;
	size_t snlen;
	const char *ln_str;
	size_t ln_strlen;
	int (*match_len)(const char *, const char *, int *);

};

enum SIP_MESSAGES {
	MSG_INVITE = 1,
	MSG_REGISTER,
	MSG_ACK,
	MSG_BYE,
	MSG_CANCEL,
	MSG_OPTIONS,
	MSG_REFER,
	MSG_PRACK,
	MSG_NOTIFY,
	MSG_REQUEST,
	MSG_RESPONSE,
	MSG_SIP_2_0_200,
	MSG_SIP_2_0_2xx,
	MSG_SIP_2_0_3xx,
	MSG_SIP_2_0_401,
	MSG_SIP_2_0_407,
	MSG_SIP_2_0_4xx,
	MSG_SIP_2_0_5xx,
	MSG_SIP_2_0_6xx,
	MSG_SIP_2_0,
	MSG_UNKNOWN
};

struct sip_message_t {
	const char *str_msg;
	enum SIP_MESSAGES msg;
};

extern int strncasecmp(const char *s1,const char *s2,int len);
/* Linear string search, case in-sensitive. */
static __inline__ 
const char *ct_sip_search(const char *needle, const char *haystack, 
			size_t needle_len, size_t haystack_len) 
{
	const char *limit = haystack + (haystack_len - needle_len);

	while (haystack <= limit) {
		//if (memcmp(haystack, needle, needle_len) == 0)
		if (strncasecmp(haystack, needle, needle_len) == 0)
			return haystack;
		haystack++;
	}
	return NULL;
}

/* get line lenght until first CR or LF seen which is not followed by ' ' or '\t'. */
static __inline__ int ct_sip_lnlen(const char *line, const char *limit)
{
        const char *k = line;

        while ((line <= limit) && (*line == '\r' || *line == '\n'))
                line++;

        while (line <= limit) {
                //if (*line == '\r' || *line == '\n')
                if (*line++ == '\n') 
					if (*line != ' ' && *line != '\t')
                        break;
        }
        return line - k;
}

/* get line length until last CR or LF seen which is not followed by ' ' or '\t'. */
static __inline__ int ct_sip_lnbegin(const char *line, const char *limit)
{
        const char *k = line;

        while (line >= limit) {
                if (*line == '\r' || *line == '\n')
					break;
				line--;
        }
        return k - line;
}


#endif /* __IFX_SIP_COMMON_H__ */
