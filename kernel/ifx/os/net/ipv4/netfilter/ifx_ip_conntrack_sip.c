/* ============================================================================
 * Copyright (C) 2003[- 2004] ?Infineon Technologies AG.
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

#include <linux/config.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <linux/ctype.h>
#include <net/tcp.h>
#include <net/udp.h>

#include <linux/netfilter_ipv4/lockhelp.h>
#include <linux/netfilter_ipv4/ip_conntrack_helper.h>

#include <linux/netfilter_ipv4/ip_conntrack_sip.h>
#ifdef CONFIG_IFX_ALG_QOS  //chandrav
#include <linux/netfilter_ipv4/ip_conntrack_core.h>
#endif /* CONFIG_IFX_ALG_QOS */

DECLARE_LOCK(ip_sip_lock);
DECLARE_RWLOCK(ip_sip_conn_rwlock);
DECLARE_RWLOCK_EXTERN(ip_conntrack_lock);

#define ASSERT_READ_LOCK MUST_BE_READ_LOCKED(&ip_sip_conn_rwlock);
#define ASSERT_WRITE_LOCK MUST_BE_WRITE_LOCKED(&ip_sip_conn_rwlock);

#include <linux/netfilter_ipv4/listhelp.h>
#include <linux/netfilter_ipv4/ifx_sip_common.h>

/* Nirav */
int ip_ct_expect_remove_old(struct ip_conntrack_expect *expect);

static int Device_Open = 0;

struct list_head sip_conn_registration_list;/* stores registered port and protocols*/

/*initialize the list*/
LIST_HEAD(sip_conn_registration_list);

struct sip_conn_registration_data
{
		struct list_head list;
		int src_port;
		int dst_port;
		enum SipControlProtocol proto;
		struct ip_conntrack_helper* sip_conntrack;
		enum oper_type oper;
};

#if 0
#define IDEBUG	printk
#else
#define IDEBUG(format,args...)
#endif

#if 0
#define PDEBUG printk
#define DEBUGP	printk
#else
#define PDEBUG(format, args...)
#define DEBUGP(format, args...)
#endif /* __DEBUG_SIP_ALG__ */

#if 0 // chandrav -- for testing purpose
#define IFX_ALG_QOS_DBG  printk
#else
#define IFX_ALG_QOS_DBG(format, args...)
#endif

static int digits_len(const char *dptr, const char *limit, int *shift);
static int string_len(const char *dptr, const char *limit, int *shift);
static int ipaddr_len(const char *dptr, const char *limit, int *shift);
static int skp_port_len(const char *dptr, const char *limit, int *shift);
static int skpspecial_user_name(const char *dptr, const char *limit, int *shift);
static int skpspecial_port_len(const char *dptr, const char *limit, int *shift);
static int epaddr_len(const char *dptr, const char *limit, int *shift);
static int skp_digits_len(const char *dptr, const char *limit, int *shift);
static int skp_epaddr_len(const char *dptr, const char *limit, int *shift);

struct sip_header_nfo ct_sip_hdrs[] = {
	{ 	/* From header */
		"From:",	sizeof("From:") - 1,
		"\r\nf:",	sizeof("\r\nf:") - 1, /* rfc3261 "\r\n" */
		"@", 	sizeof("@") - 1,
		ipaddr_len
	},
	{ 	/* From port header */
		"From:",	sizeof("From:") - 1,
		"\r\nf:",	sizeof("\r\nf:") - 1, /* rfc3261 "\r\n" */
		"@", 	sizeof("@") - 1,
		skpspecial_port_len
	},
	{ 	/* From user name */
		"From:",	sizeof("From:") - 1,
		"\r\nf:",	sizeof("\r\nf:") - 1, /* rfc3261 "\r\n" */
		"sip:", 	sizeof("sip:") - 1,
		skpspecial_user_name
	},
	{ 	/* From user name */
		"From:",	sizeof("From:") - 1,
		"\r\nf:",	sizeof("\r\nf:") - 1, /* rfc3261 "\r\n" */
		"sips:", 	sizeof("sips:") - 1,
		skpspecial_user_name
	},
	{ 	/* To header */
		"To:",		sizeof("To:") - 1,
		"\r\nt:",	sizeof("\r\nt:") - 1, /* rfc3261 "\r\n" */
		"@", 	sizeof("@") - 1,
		ipaddr_len
	},
	{ 	/* To port header */
		"To:",		sizeof("To:") - 1,
		"\r\nt:",	sizeof("\r\nt:") - 1, /* rfc3261 "\r\n" */
		"@", 	sizeof("@") - 1,
		skpspecial_port_len
	},
	{ 	/* Via header for ip addr */
		"Via:",		sizeof("Via:") - 1,
		"\r\nv:",	sizeof("\r\nv:") - 1, /* rfc3261 "\r\n" */
		"UDP ", 	sizeof("UDP ") - 1,
		ipaddr_len
	},
	{ 	/* Via header for ip addr */
		"Via:",		sizeof("Via:") - 1,
		"\r\nv:",	sizeof("\r\nv:") - 1, /* rfc3261 "\r\n" */
		"TCP ", 	sizeof("TCP ") - 1,
		ipaddr_len
	},
	{ 	/* Via header for port */
		"Via:",		sizeof("Via:") - 1,
		"\r\nv:",	sizeof("\r\nv:") - 1, /* rfc3261 "\r\n" */
		"UDP ", 	sizeof("UDP ") - 1,
		skp_port_len
	},
	{ 	/* Via header for port */
		"Via:",		sizeof("Via:") - 1,
		"\r\nv:",	sizeof("\r\nv:") - 1, /* rfc3261 "\r\n" */
		"TCP ", 	sizeof("TCP ") - 1,
		skp_port_len
	},
	{	/* Proxy-Authorization header */
		"Proxy-Authorization:",	sizeof("Proxy-Authorization:") - 1,
		"Proxy-Authorization:",	sizeof("Proxy-Authorization:") - 1,
		"@",					sizeof("@") - 1,
		ipaddr_len
	},
	{ 	/* CSeq header */
		"CSeq:", sizeof("CSeq:") - 1,
		"CSeq:",	sizeof("CSeq:") - 1,
		":",		sizeof(":") - 1, 
		skp_digits_len
	},
	{ 	/* Call-ID header */
		"Call-ID:", sizeof("Call-ID:") - 1,
		"\r\ni:",	sizeof("\r\ni:") - 1,
		":",		sizeof(":") - 1, 
		string_len
	},
	{ 	/* Call-ID for ip addr */
		"Call-ID:", sizeof("Call-ID:") - 1,
		"\r\ni:",	sizeof("\r\ni:") - 1,
		"@",		sizeof("@") - 1, 
		ipaddr_len
	},
	{ 	/* Contact header for ip */
		"Contact:",	sizeof("Contact:") - 1,
		"\r\nm:",	sizeof("\r\nm:") - 1,
		"@",		sizeof("@") - 1,
		ipaddr_len
	},
	{ 	/* Contact header for port */
		"Contact:",	sizeof("Contact:") - 1,
		"\r\nm:",	sizeof("\r\nm:") - 1,
		"@",		sizeof("@") - 1,
		skp_port_len
	},
	{ 	/* Contact header for port */
		"Contact:",	sizeof("Contact:") - 1,
		"\r\nm:",	sizeof("\r\nm:") - 1,
		"@",		sizeof("@") - 1,
		skpspecial_port_len
	},
	{ 	/* Contact header for ip and port */
		"Contact:",	sizeof("Contact:") - 1,
		"\r\nm:",	sizeof("\r\nm:") - 1,
		"@",		sizeof("@") - 1,
		epaddr_len
	},
	{ 	/* Contact header for transport */
		"Contact:",	sizeof("Contact:") - 1,
		"\r\nm:",	sizeof("\r\nm:") - 1,
		"transport=",	sizeof("transport=") - 1,
		string_len
	},
	{ 	/* Contact header for expires */
		"Contact:",	sizeof("Contact:") - 1,
		"\r\nm:",	sizeof("\r\nm:") - 1,
		"expires=",	sizeof("expires=") - 1,
		skp_digits_len
	},
	{ 	/* Expires header */
		"Expires:",	sizeof("Expires:") - 1,
		"Expires:",	sizeof("Expires:") - 1,
		":",		sizeof(":") - 1,
		skp_digits_len
	},
	{ 	/* Refer header */
		"Refer-To:", sizeof("Refer-To:") - 1,
		"\r\nr:",	sizeof("\r\nr:") - 1,
		"%40",		sizeof("%40") - 1, 
		ipaddr_len
	},
	{ 	/* Referred-By header */
		"Referred-By:", sizeof("Referred-By:") - 1,
		"\r\nb:",	sizeof("\r\nb:") - 1,
		"@",		sizeof("@") - 1, 
		ipaddr_len
	},
	{ 	/* Content length header */
		"Content-Length:", sizeof("Content-Length:") - 1,
		"\r\nl:",	sizeof("\r\nl:") - 1,
		":",		sizeof(":") - 1, 
		skp_digits_len
	},
	{	/* SDP audio RTP info */
		"\nm=audio",sizeof("\nm=audio") - 1,	
		"\rm=audio",sizeof("\rm=audio") - 1,
		" ",		sizeof(" ") - 1,
		digits_len
	},
	{	/* SDP audio RTCP info */
		"\na=rtcp",	sizeof("\na=rtcp") - 1,	
		"\ra=rtcp",	sizeof("\ra=rtcp") - 1,
		":",		sizeof(":") - 1,
		digits_len
	},
	{	/* SDP image info */
		"\nm=image",sizeof("\nm=image") - 1,	
		"\rm=image",sizeof("\rm=image") - 1,
		" ",		sizeof(" ") - 1,
		digits_len
	},
	{ 	/* SDP owner address*/	
		"\no=",		sizeof("\no=") - 1, 
		"\ro=",		sizeof("\ro=") - 1,
		"IN IP4 ",	sizeof("IN IP4 ") - 1,
		epaddr_len
	},
	{ 	/* SDP connection info */
		"\nc=",		sizeof("\nc=") - 1, 
		"\rc=",		sizeof("\rc=") - 1,
		"IN IP4 ",	sizeof("IN IP4 ") - 1,
		epaddr_len
	},
	{ 	/* Requests headers ip */
		"sip:",		sizeof("sip:") - 1,
		"sip:",		sizeof("sip:") - 1, /* yes, i know.. ;) */
		"@", 		sizeof("@") - 1, 
		ipaddr_len
	},
	{ 	/* Requests headers port */
		"sip:",		sizeof("sip:") - 1,
		"sip:",		sizeof("sip:") - 1, /* yes, i know.. ;) */
		"@", 		sizeof("@") - 1, 
		skp_port_len
	},
	{ 	/* SDP version header */
		"\nv=",		sizeof("\nv=") - 1,
		"\rv=",		sizeof("\rv=") - 1,
		"=", 		sizeof("=") - 1, 
		digits_len
	},
	{ 	/* To user name */
		"To:",		sizeof("To:") - 1,
		"\r\nt:",	sizeof("\r\nt:") - 1, /* rfc3261 "\r\n" */
		"sip:", 	sizeof("sip:") - 1,
		skpspecial_user_name
	},
	{ 	/* To user name */
		"To:",		sizeof("To:") - 1,
		"\r\nt:",	sizeof("\r\nt:") - 1, /* rfc3261 "\r\n" */
		"sips:", 	sizeof("sips:") - 1,
		skpspecial_user_name
	},
	{ 	/* Record-Route */
		"Record-Route:",sizeof("Record-Route:") - 1,
		"\r\nrr:",	sizeof("\r\nt:") - 1, /* rfc3261 "\r\n" */
		"sip:", 	sizeof("sip:") - 1,
		skpspecial_user_name
	},
	{ 	/* Record-Route */
		"Record-Route:",sizeof("Record-Route:") - 1,
		"\r\nrr:",	sizeof("\r\nt:") - 1, /* rfc3261 "\r\n" */
		"sips:", 	sizeof("sips:") - 1,
		skpspecial_user_name
	}
};

EXPORT_SYMBOL(ct_sip_hdrs);

struct sip_message_t sip_messages [] =
{
	{ "INVITE", 	MSG_INVITE },
	{ "REGISTER",	MSG_REGISTER },
	{ "ACK",		MSG_ACK },
	{ "BYE",		MSG_BYE },
	{ "CANCEL",		MSG_CANCEL },
	{ "OPTIONS",	MSG_OPTIONS },
	{ "NOTIFY",		MSG_NOTIFY },
	{ "REFER",		MSG_REFER },
	{ "PRACK",		MSG_PRACK },
	{ "SIP/2.0 200",MSG_SIP_2_0_200 },
	{ "SIP/2.0 2", 	MSG_SIP_2_0_2xx },
	{ "SIP/2.0 3", 	MSG_SIP_2_0_3xx },
	{ "SIP/2.0 401",MSG_SIP_2_0_401 },
	{ "SIP/2.0 407",MSG_SIP_2_0_407 },
	{ "SIP/2.0 4", 	MSG_SIP_2_0_4xx },
	{ "SIP/2.0 5", 	MSG_SIP_2_0_5xx },
	{ "SIP/2.0 6", 	MSG_SIP_2_0_6xx },
	{ "SIP/2.0", 	MSG_SIP_2_0 }
};

enum SIP_MESSAGES get_sip_method(const char *dptr)
{
	int i;

	for (i = 0; i < sizeof(sip_messages) / (sizeof(struct sip_message_t)); i++) {
		if (memcmp(dptr,sip_messages[i].str_msg,strlen(sip_messages[i].str_msg)) == 0)
			return sip_messages[i].msg;
	}
	return MSG_UNKNOWN;
}

static int string_len(const char *dptr, const char *limit, int *shift) {
	int len = 0;

	for (; dptr <= limit && *dptr == ' '; dptr++)
		(*shift)++;
	while (dptr <= limit && *dptr != ' ' && *dptr != '\r' && *dptr != '\n') {
		dptr++;
		len++;
	}
	return len;
}

static int digits_len(const char *dptr, const char *limit, int *shift)
{
	int len = 0;	
	while (dptr <= limit && isdigit(*dptr)) {
		dptr++;
		len++;
	}
	return len;
} 

/* get digits lenght, skiping blank spaces. */
static int skp_digits_len(const char *dptr, const char *limit, int *shift)
{
	for (; dptr <= limit && *dptr == ' '; dptr++)
		(*shift)++;
		
	return digits_len(dptr, limit, shift);
}

/* Simple ipaddr parser.. */
int parse_ipaddr(const char *cp,	const char **endp, 
			uint32_t *ipaddr, const char *limit)
{
	unsigned long int val;
	int i, digit = 0;
	
	for (i = 0, *ipaddr = 0; cp <= limit && i < 4; i++) {
		digit = 0;
		if (!isdigit(*cp))
			break;
		
		val = simple_strtoul(cp, (char **)&cp, 10);
		if (val > 0xFF)
			return -1;
	
		((uint8_t *)ipaddr)[i] = val;	
		digit = 1;
	
		if (*cp != '.')
			break;
		cp++;
	}
	if (!digit)
		return -1;
	
	if (endp)
		*endp = cp;

	return 0;
}

/* skip ip address. returns it lenght. */
static int ipaddr_len(const char *dptr, const char *limit, int *shift)
{
	const char *aux = dptr;
	uint32_t ip;
	
	if (parse_ipaddr(dptr, &dptr, &ip, limit) < 0) {
		DEBUGP("ip: %s parse failed.!\n", dptr);
		return 0;
	}
	return dptr - aux;
}

/* skip port. returns it lenght. */
static int skp_port_len(const char *dptr, const char *limit, int *shift)
{
	for (; dptr <= limit && *dptr != ':'; dptr++)
		(*shift)++;	
	
	if (*dptr == ':') {
		dptr++;
		(*shift)++;
		return digits_len(dptr, limit, shift);
	}	
	return  0;
}
/* special skip user name. returns it lenght. */
static int skpspecial_user_name(const char *dptr, const char *limit, int *shift)
{
	int length = 0;
	while(*dptr != ':' && *dptr != '>' && *dptr != ';' && *dptr != '\r' && *dptr != '\n' && dptr < limit){
		dptr++;
		length++;
	}
	
	if ( length > 0) {
		return length;
	}	
	return  -1; /* Special return value to signify user name does not exist */
}
/* special skip port. returns it lenght. */
static int skpspecial_port_len(const char *dptr, const char *limit, int *shift)
{
	while(*dptr != ':' && *dptr != '>' && *dptr != ';' && *dptr != '\r' && *dptr != '\n' && dptr < limit){
		dptr++;
		(*shift)++;
	}
	
	if (*dptr == ':') {
		return (digits_len((dptr + 1), limit, shift) + 1);
	}	
	return  -1; /* Special return value to signify port does not exist */
}
/* skip ip address. returns it lenght. */
static int epaddr_len(const char *dptr, const char *limit, int *shift)
{
	const char *aux = dptr;
	uint32_t ip;
	
	if (parse_ipaddr(dptr, &dptr, &ip, limit) < 0) {
		DEBUGP("ip: %s parse failed.!\n", dptr);
		return 0;
	}

	/* Port number */
	if (*dptr == ':') {
		dptr++;
		dptr += digits_len(dptr, limit, shift);
	}
	return dptr - aux;
}

/* get address lenght, skiping user info. */
static int skp_epaddr_len(const char *dptr, const char *limit, int *shift)
{
	for (; dptr <= limit && *dptr != '@'; dptr++)
		(*shift)++;	
	
	if (*dptr == '@') {
		dptr++;
		(*shift)++;
		return epaddr_len(dptr, limit, shift);
	}	
	return  0;
}

int strncasecmp(const char *s1,const char *s2,int len)
{
	int i;

	for(i = 0; i < len; i++) {
		if (toupper(s1[i]) != toupper(s2[i]))
			return (toupper(s1[i]) - toupper(s2[i]));
	}	
	return 0;
}

/* Returns 0 if not found, -1 error parsing. */
int ct_sip_get_info(const char *dptr, size_t dlen, 
		unsigned int *matchoff, 
		unsigned int *matchlen,
		struct sip_header_nfo *hnfo)
{
	const char *limit, *aux, *k = dptr;
	int shift = 0;
	
	limit = dptr + (dlen - hnfo->lnlen);

	while (dptr <= limit) {
		if ((strncasecmp(dptr, hnfo->lname, hnfo->lnlen) != 0) &&
			(strncasecmp(dptr, hnfo->sname, hnfo->snlen) != 0)) {
			dptr++;
			continue;
		}
		aux = ct_sip_search(hnfo->ln_str, dptr, hnfo->ln_strlen, 
						ct_sip_lnlen(dptr, limit));
		if (!aux) {
			DEBUGP("'%s' not found in '%s'.\n", hnfo->ln_str, hnfo->lname);
			return -1;
		}
		aux += hnfo->ln_strlen;
		
		*matchlen = hnfo->match_len(aux, limit, &shift);
		if (!*matchlen)
			return -1;

		*matchoff = (aux - k) + shift; 
		
		DEBUGP("%s match succeeded! - len: %u\n", hnfo->lname, *matchlen);
		return 1;
	}
	DEBUGP("%s header not found.\n", hnfo->lname);
	return 0;
}

#ifdef CONFIG_IFX_ALG_QOS  // chandrav
/*
 * The expect function for SIP ALG
 * This will mark the ALG application family and
 * ALG protocol family for the SIP data/control traffic.
 */
static int sip_expectfn(struct ip_conntrack *ct)
{
	WRITE_LOCK(&ip_conntrack_lock);
	ct->ifx_alg_qos_mark = IFX_ALG_APP_SIP | IFX_ALG_PROTO_RTP; 
	IFX_ALG_QOS_DBG("SIP ALG: Marked the Child conntracker!!! \n");	
	WRITE_UNLOCK(&ip_conntrack_lock);
	return NF_ACCEPT; /* unused */
}
#endif /* CONFIG_IFX_ALG_QOS */


static int set_expected_media(struct ip_conntrack *ct,
			enum ip_conntrack_info ctinfo, 
			const struct iphdr *iph,
			uint32_t ipaddr, uint16_t port,
			enum MEDIA_TYPE	media_type,
			enum EXPECTATION_TYPE exp_type,
			const char *dptr,
			int exp_offset)
{
	struct ip_conntrack_expect expect, *exp = &expect;
	struct ip_ct_sip_expect *exp_sip_info = &exp->help.exp_sip_info;
	struct udphdr *udph = (struct udphdr *)((u_int32_t *)iph + iph->ihl);
	int ret = 0;
	
	memset(exp,0,sizeof(struct ip_conntrack_expect));	

	exp_sip_info->flag=0;
	exp_sip_info->orig_port = port;
	exp_sip_info->media_type = media_type;
	exp_sip_info->exp_type = exp_type;
	exp_sip_info->exp_offset = exp_offset;
	
	exp->tuple = ((struct ip_conntrack_tuple)
		{ { 0 , { 0 } },
		  { ipaddr, { .udp = { htons(port) } }, IPPROTO_UDP }});
		  
	exp->mask = ((struct ip_conntrack_tuple) 
		{ { 0x00, { 0 } },
		  { 0xFFFFFFFF, { .udp = { 0xFFFF } }, 0xFF }});

#ifdef CONFIG_IFX_ALG_QOS  // chandrav
	/*
	 * Set the Expect Function for SIP traffic
	 */
	exp->expectfn = sip_expectfn ;
#else
	exp->expectfn = NULL;
#endif  /* CONFIG_IFX_ALG_QOS */

	/* Nirav - Set the seq no for TCP. For UDP, set the length of the packet */
	if (iph->protocol == IPPROTO_TCP) {
		struct tcphdr *tcph = (void *)iph + iph->ihl * 4;
		exp->seq = ntohl(tcph->seq);
	} else {
		exp->seq = ntohl(udph->len);
	}

#if 0
	exp->tuple.dst.u.udp.port++;
	ret = ip_conntrack_expect_related(ct,exp);
	if (ret == -EEXIST) {
		ip_ct_expect_remove_old(exp);
		ret = ip_conntrack_expect_related(ct,exp);
		if ( ret != 0) {
			IDEBUG("sip_conntrack : Error %d - Could not register expectation for dst.ip=%u.%u.%u.%u:%d and dst.protonum=%d\n",ret,NIPQUAD(exp->tuple.dst.ip),exp->tuple.dst.u.udp.port,IPPROTO_UDP);
			//return NF_ACCEPT;
		} 
	}
	IDEBUG("sip_conntrack : Successfully registered expectation for dst.ip=%u.%u.%u.%u:%d and dst.protonum=%d\n",NIPQUAD(exp->tuple.dst.ip),exp->tuple.dst.u.udp.port,IPPROTO_UDP);
#endif
	/* Expect the connection */
	ret = ip_conntrack_expect_related(ct,exp);
	if (ret == -EEXIST) {
		ip_ct_expect_remove_old(exp);
		ret = ip_conntrack_expect_related(ct,exp);
		if ( ret != 0) {
			IDEBUG("sip_conntrack : Error %d - Could not register expectation for dst.ip=%u.%u.%u.%u:%d and dst.protonum=%d\n",ret,NIPQUAD(exp->tuple.dst.ip),exp->tuple.dst.u.udp.port,IPPROTO_UDP);
			return NF_ACCEPT;
		} 
	}
	IDEBUG("sip_conntrack : Successfully registered %s %s expectation for dst.ip=%u.%u.%u.%u:%d and dst.protonum=%d\n",(exp_sip_info->media_type == AUDIO_RTP ? "AUDIO_RTP" : "AUDIO_RTCP"), (exp_sip_info->exp_type == EXPLICIT ? "EXPLICIT" : "IMPLICIT"), NIPQUAD(exp->tuple.dst.ip),exp->tuple.dst.u.udp.port,IPPROTO_UDP);
	return NF_ACCEPT;
}

/******************************************************************
*  Function Name	:  	sip_help
*  Description      	:  	This is the helper function which gets
*  				called when a connection matches the
*  				registered tupple.
*  Input Values    	:	const struct iphdr *iph, 
*  					size_t len,
*  					struct ip_conntrack *ct,
*  					enum ip_conntrack_info ctinfo 
*  						
*  Output Values 	:  
*  Return Value   	:
*  Notes		:	Builds the expection by looking at the media
*  				info carried by the SIP message
*********************************************************************/
int sip_help(const struct iphdr *iph, size_t len,	struct ip_conntrack *ct, enum ip_conntrack_info ctinfo)
{
	unsigned int dataoff, datalen;
	const char *dptr;
	int ret = NF_ACCEPT;
	int matchoff, matchlen;
	uint32_t ipaddr;
	uint16_t port;
	enum SIP_MESSAGES sip_msg;

	/* Until there's been traffic both ways, don't look in packets. */
	if ( iph->protocol == IPPROTO_TCP 
			&& ctinfo != IP_CT_ESTABLISHED
			&& ctinfo != IP_CT_ESTABLISHED+IP_CT_IS_REPLY) {
		return NF_ACCEPT;
	}

	if (iph->protocol == IPPROTO_TCP) {
		struct tcphdr *tcph = (void *)iph + iph->ihl * 4;
		dataoff = tcph->doff * 4;
		dptr = (const char *)tcph + tcph->doff * 4;
	} else if (iph->protocol == IPPROTO_UDP) {
		dataoff = iph->ihl * 4 + sizeof(struct udphdr);
		dptr = (const char *)iph + dataoff;
	} else {
		/* Unknown protocol */
		return NF_ACCEPT;
	}

	/* No Data ? */ 
	if (dataoff >= len) {
		DEBUGP("skb->len = %u\n", len);
		return NF_ACCEPT;
	}

	WRITE_LOCK(&ip_conntrack_lock);
	ct->ifx_alg_qos_mark = IFX_ALG_APP_SIP; 
	IFX_ALG_QOS_DBG("SIP ALG: Marked the Master conntracker!!! \n");	
	WRITE_UNLOCK(&ip_conntrack_lock);
        
	LOCK_BH(&ip_sip_lock);
	
	datalen = len - dataoff;
	if (datalen < (sizeof("SIP/2.0 200") - 1))
		goto out;
	
	sip_msg = get_sip_method(dptr);
	/* RTP info only in some SDP pkts */
	if (sip_msg != MSG_INVITE && sip_msg != MSG_SIP_2_0_200) {
		goto out;
	}

	/* Check if this is the reponse to the INVITE message */
	if (sip_msg == MSG_SIP_2_0_200) {
		if (ct_sip_get_info(dptr, datalen, &matchoff, &matchlen, &ct_sip_hdrs[POS_CSEQ]) > 0) {
			if (get_sip_method(dptr + matchoff + matchlen + 1) != MSG_INVITE)
				goto out;
		}
	}
	
	//IDEBUG("sip_conntrack : Message found\n[%s]\n",dptr);
	/* Get ip and port address from SDP packet. */
	if (ct_sip_get_info(dptr, datalen, &matchoff, &matchlen, 
	    &ct_sip_hdrs[POS_CONECTION]) > 0) {

		/* We'll drop only if there are parse problems. */
		if (parse_ipaddr(dptr + matchoff, NULL, &ipaddr, 
		    dptr + datalen) < 0) {
			ret = NF_DROP;
			goto out;
		}

		/* This is INVITE or SIP/2.0 200 packet with hold */
		if (ipaddr == 0) {
			PDEBUG("Hold packet encountered\n");
			goto out;
		}
		
		if (ct_sip_get_info(dptr, datalen, &matchoff, &matchlen, 
		    &ct_sip_hdrs[POS_MEDIA_AUDIO_RTP]) > 0) {
			int rtcp_matchoff = 0, rtcp_port = 0;

			port = simple_strtoul(dptr + matchoff, NULL, 10);
			if (port < 1024 && port !=0 ) {
				ret = NF_DROP;
				goto out;
			}
			if (ct_sip_get_info(dptr + matchoff, datalen - matchoff, &rtcp_matchoff, &matchlen, 
				&ct_sip_hdrs[POS_MEDIA_AUDIO_RTCP]) > 0) {
				rtcp_port = simple_strtoul(dptr + matchoff + rtcp_matchoff, NULL, 10);
				if (rtcp_port < 1024 && port !=0 ) {
					ret = NF_DROP;
					goto out;
				}
				/* Expect the EXPLICIT RTCP connection */
				ret = set_expected_media(ct, ctinfo, iph, ipaddr, rtcp_port, AUDIO_RTCP, EXPLICIT, dptr, matchoff);
			} else {
				/* Expect the IMPLICIT RTCP connection */
				ret = set_expected_media(ct, ctinfo, iph, ipaddr, (port + 1), AUDIO_RTCP, IMPLICIT, dptr, 0);
			}
			/* Expect the RTP connection */
			//ret = set_expected_media(ct, ctinfo, iph, ipaddr, port, IMAGE_DATA, EXPLICIT, dptr, matchoff - ct_sip_lnbegin(dptr + matchoff, dptr));
			ret = set_expected_media(ct, ctinfo, iph, ipaddr, port, AUDIO_RTP, EXPLICIT, dptr, matchoff - ct_sip_lnbegin(dptr + matchoff, dptr));
		}
		/* Nirav Starent start */
		if (ct_sip_get_info(dptr, datalen, &matchoff, &matchlen, 
		    &ct_sip_hdrs[POS_MEDIA_IMAGE]) > 0) {
			port = simple_strtoul(dptr + matchoff, NULL, 10);
			if (port < 1024 && port !=0 ) {
				ret = NF_DROP;
				goto out;
			}
			/* Expect the image RTP connection */
			ret = set_expected_media(ct, ctinfo, iph, ipaddr, port, IMAGE_RTP, EXPLICIT, dptr, matchoff - ct_sip_lnbegin(dptr + matchoff, dptr));

		}
		/* Nirav Starent end */
	}
out:	
	UNLOCK_BH(&ip_sip_lock);
	return ret;
}

static int device_conn_open(struct inode * inode, struct file *file)
{
	PDEBUG("device_open(%p)\n", file);
	printk("device_open(%p)\n", file);

	if(Device_Open)
		return -EBUSY;

	Device_Open++;
	MOD_INC_USE_COUNT;

	return 0;
}

static int device_conn_release(struct inode *inode, struct file *file)
{
	PDEBUG("device release(%p, %p)\n",inode, file);

	Device_Open--;
	MOD_DEC_USE_COUNT;

	return 0;
}

static int get_conn_registration_data(const struct sip_conn_registration_data* i, int src_port, int dst_port, enum SipControlProtocol proto, enum oper_type oper)
{
	if((i->src_port == src_port) && (i->dst_port == dst_port) && (i->proto == proto) && (i->oper == oper)) {
		PDEBUG("src port=%d dst_port=%d matched %d\n",src_port, dst_port);
		PDEBUG("protocol matched %d\n",proto);
		return 1;
	}
	else 
		return 0;
}

int sip_device_conn_port_deregister(struct sip_params* sipparam, enum oper_type oper)
{
	struct sip_conn_registration_data* sip_conn_registration_data = NULL;

	sip_conn_registration_data = LIST_FIND(&sip_conn_registration_list, get_conn_registration_data,	struct sip_conn_registration_data*, sipparam->sip_src_port, sipparam->sip_dst_port, sipparam->proto, oper);

	if(sip_conn_registration_data == NULL) {
		IDEBUG("sip_conntrack: No registration for src_port=%d, dst_port=%d and proto=%d, oper=%s exists\n",sipparam->sip_src_port,sipparam->sip_dst_port,sipparam->proto,(oper == STATIC_OPER ? "STATIC_OPER" : "DYNAMIC_OPER"));
		return -1;
	}

	/* Start deregistaring considering none is dependent on this*/
	ip_conntrack_helper_unregister(sip_conn_registration_data->sip_conntrack);
	IDEBUG("sip_conntrack: Conntrack helper unregistered for the protocol=%d and src_port=%d, dst_port=%d, oper=%s\n",sipparam->proto,sipparam->sip_src_port,sipparam->sip_dst_port,(oper == STATIC_OPER ? "STATIC_OPER" : "DYNAMIC_OPER"));

	PDEBUG("device_port_deregister: Removing registration data from list\n");
	list_del(&sip_conn_registration_data->list);

	PDEBUG("device_port_deregister: freeing up registration data %p\n",sip_conn_registration_data);
	kfree(sip_conn_registration_data);

	return 0;
}

int sip_device_conn_port_register(struct sip_params* sipparam, enum oper_type oper)
{
	int ret;
	struct ip_conntrack_helper* sip_conntrack = NULL;
	struct sip_conn_registration_data* sip_conn_registration_data = NULL;

	/*Need to check whether the port is already existing*/
	sip_conn_registration_data = LIST_FIND(&sip_conn_registration_list, get_conn_registration_data,	struct sip_conn_registration_data*, sipparam->sip_src_port, sipparam->sip_dst_port, sipparam->proto, STATIC_OPER);

	if(sip_conn_registration_data == NULL) {
		sip_conn_registration_data = LIST_FIND(&sip_conn_registration_list, get_conn_registration_data,	struct sip_conn_registration_data*, sipparam->sip_src_port, sipparam->sip_dst_port, sipparam->proto, DYNAMIC_OPER);
	}
	
	if(sip_conn_registration_data != NULL) {
		IDEBUG("sip_conntrack: Registration for src_port=%d, dst_port=%d and proto=%d already exists\n",sipparam->sip_src_port,sipparam->sip_dst_port,sipparam->proto);
		return -1;
	}

	sip_conn_registration_data = kmalloc(sizeof(struct sip_conn_registration_data), GFP_ATOMIC);

	if(sip_conn_registration_data == NULL) {
		PDEBUG("device_port_register: Memory not available\n");
		return -1;
	}

	sip_conntrack = kmalloc(sizeof(struct ip_conntrack_helper),GFP_ATOMIC);
	if(sip_conntrack == NULL) {
		PDEBUG("device_port_register: Memory not available\n");
		kfree(sip_conn_registration_data);
		return -1;
	}

	memset(sip_conntrack, 0, sizeof(struct ip_conntrack_helper));
	memset(sip_conn_registration_data, 0, sizeof(struct sip_conn_registration_data));

	INIT_LIST_HEAD(&sip_conn_registration_data->list);

	sip_conntrack->list.next =NULL;
	sip_conntrack->list.prev =NULL;
	sip_conntrack->name = "SIP";
	sip_conntrack->flags = IP_CT_HELPER_F_REUSE_EXPECT;
	sip_conntrack->me = THIS_MODULE;
	sip_conntrack->max_expected = MAX_EXPECTATION;
	sip_conntrack->timeout = 200;

	memset(&(sip_conntrack->mask),0x00,sizeof(sip_conntrack->mask));
	memset(&(sip_conntrack->tuple),0x00,sizeof(sip_conntrack->tuple));

#if 0
	if(sipparam->proto == TCP) {
		sip_conntrack->tuple.dst.protonum = IPPROTO_TCP;
		if (sipparam->sip_src_port > 0) {
			sip_conntrack->tuple.dst.u.tcp.port = htons(sipparam->sip_src_port);	
			sip_conntrack->mask.dst.u.tcp.port = 0xFFFF;
			IDEBUG("sip_conntrack: Registering sip_conntrack for TCP SRC_PORT=%d\n",sipparam->sip_src_port);
		} else if (sipparam->sip_dst_port > 0) {
			sip_conntrack->tuple.src.u.tcp.port = htons(sipparam->sip_dst_port);	
			sip_conntrack->mask.src.u.tcp.port = 0xFFFF;
			IDEBUG("sip_conntrack: Registering sip_conntrack for TCP DST_PORT=%d\n",sipparam->sip_dst_port);
		}
	} else if(sipparam->proto == UDP) {
		sip_conntrack->tuple.dst.protonum = IPPROTO_UDP;
		if (sipparam->sip_src_port > 0) {
			sip_conntrack->tuple.dst.u.udp.port = htons(sipparam->sip_src_port);	
			sip_conntrack->mask.dst.u.udp.port = 0xFFFF;
			IDEBUG("sip_conntrack: Registering sip_conntrack for UDP SRC_PORT=%d\n",sipparam->sip_src_port);
		} else if (sipparam->sip_dst_port > 0) {
			sip_conntrack->tuple.src.u.udp.port = htons(sipparam->sip_dst_port);	
			sip_conntrack->mask.src.u.udp.port = 0xFFFF;
			IDEBUG("sip_conntrack: Registering sip_conntrack for UDP DST_PORT=%d\n",sipparam->sip_dst_port);
		}
	}
#else
	if(sipparam->proto == UDP) {
		sip_conntrack->tuple.dst.protonum = IPPROTO_UDP;
		sip_conntrack->tuple.src.u.udp.port = htons(sipparam->sip_dst_port);
		sip_conntrack->mask.src.u.udp.port = 0xFFFF;
		IDEBUG("sip_conntrack: Registering sip_conntrack for UDP DST_PORT=%d\n",sipparam->sip_dst_port);
	} else if(sipparam->proto ==TCP) {
		sip_conntrack->tuple.dst.protonum = IPPROTO_TCP;
		sip_conntrack->tuple.src.u.tcp.port = htons(sipparam->sip_dst_port);
		sip_conntrack->mask.src.u.tcp.port = 0xFFFF;
		IDEBUG("sip_conntrack: Registering sip_conntrack for TCP DST_PORT=%d\n",sipparam->sip_dst_port);
	}

#endif
#if 0
	sip_conntrack->mask.dst.protonum = 0xFFFF;
#else
	/* To handle TCP and UDP both the protocol */
	sip_conntrack->mask.dst.protonum = ~(IPPROTO_UDP | IPPROTO_TCP);
#endif

	sip_conntrack->help = sip_help;

	ret = ip_conntrack_helper_register(sip_conntrack);
	if(ret) {
		IDEBUG("sip_conntrack: Failed to register conntrack_helper for the protocol=%d and src_port=%d, dst_port=%d\n",sipparam->proto,sipparam->sip_src_port,sipparam->sip_dst_port);
		kfree(sip_conntrack);
		kfree(sip_conn_registration_data);
		return ret;
	}

	PDEBUG("sip_conntrack: conntrack helper registered\n");
	/* store the addresses in the sip_registration_list*/
	sip_conn_registration_data->src_port = sipparam->sip_src_port;
	sip_conn_registration_data->dst_port = sipparam->sip_dst_port;
	sip_conn_registration_data->proto = sipparam->proto;
	sip_conn_registration_data->sip_conntrack = sip_conntrack;
	sip_conn_registration_data->oper = oper;
	IDEBUG("sip_conntrack: Conntrack helper registered for the protocol=%d and src_port=%d, dst_port=%d\n",sipparam->proto,sipparam->sip_src_port,sipparam->sip_dst_port);

	PDEBUG("sip: Adding Sip_registration_data %p\n",sip_conn_registration_data);
	list_prepend(&sip_conn_registration_list, &sip_conn_registration_data->list);
	return 0;
}

int device_conn_ioctl(
		struct inode *inode,
		struct file *file,
		unsigned int ioctl_num,/* The number of the ioctl */
		unsigned long ioctl_param) /* The parameter to it */
{
	struct sip_params *sipparam;
	int ret;

	PDEBUG("Received ioctl call\n");
	/* Switch according to the ioctl called */
	switch (ioctl_num) {
		case IOCTL_DEREGISTER_CONN_PORT:
			sipparam=(struct sip_params*)ioctl_param;
			ret = sip_device_conn_port_deregister(sipparam, STATIC_OPER);
			break;

		case IOCTL_REGISTER_CONN_PORT:
			PDEBUG("Received IOCTL_REGISTER_PORT\n");
			sipparam = (struct sip_params*)ioctl_param;

			ret= sip_device_conn_port_register(sipparam, STATIC_OPER);
			break;

		default:
			printk("ERROR: ioctl num not matched\n");
			return -1;
	}

	return ret;
}

struct file_operations Fconn_ops = {
ioctl: device_conn_ioctl,   /* ioctl */
	   open: device_conn_open,
	   release: device_conn_release  /* a.k.a. close */
};

/******************************************************************
 *  Function Name	:  	fini
 *  Description      :  	This function is called before the module
 *  						is unloaded from the kernel.
 *  Input Values    	:	void
 *  Output Values 	:  
 *  Return Value   	:	void
 *  Notes			:	
 *********************************************************************/
static void __exit fini(void)
{
	struct list_head  *cur_item;
	struct list_head  *temp_item;
	struct sip_conn_registration_data* cur_registration;
	int ret;

	list_for_each_safe(cur_item, temp_item, &sip_conn_registration_list) {
		cur_registration = list_entry(cur_item, struct sip_conn_registration_data, list);
		printk("sip: deleting registration data\n");
		printk("sip: conntrack helper unregistered\n");
		ip_conntrack_helper_unregister(cur_registration->sip_conntrack);
		kfree(cur_registration->sip_conntrack);
		list_del(&cur_registration->list);
		kfree(cur_registration);
	}

	ret = unregister_chrdev(SIP_CONN_MAJOR_NUM, DEVICE_CONN_NAME);
	/* If there's an error, report it */ 
	if (ret < 0)
		printk("Error in module_unregister_chrdev: %d\n", ret);

}

/******************************************************************
 *  Function Name	:  	init
 *  Description      :  	This function is called when the module is
 *  						loaded in the kernel.
 *  Input Values    	:	void 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes			:	It registers the sip conntrack helper 
 *  						function with the conntrack core function
 *********************************************************************/
static int __init init(void)
{
	int ret=0;
	printk("\n\n(%s)::init: Registering Sip Conntrack device\n", __FUNCTION__);
	ret = register_chrdev(SIP_CONN_MAJOR_NUM, DEVICE_CONN_NAME, &Fconn_ops);
	if(ret<0) {
		printk("Sorry, registering the char device with return value %d\n", ret);
	}
	return ret;
}


module_init(init);
module_exit(fini);

MODULE_LICENSE("GPL");

/* ===========================================================================
 * Revision History:
 *
 * 15/06/2004 Initial version. Released for code review.
 * ===========================================================================
 */

