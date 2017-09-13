/* ============================================================================
 * Copyright (C) 2003[- 2004] – Infineon Technologies AG.
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
 * File Name: ifx_ip_nat_sip.c
 * Author 	: Atanu Mondal
 * Date		: 
 *
 * ===========================================================================
 *
 * Project: <project/component name>
 * Block: <block/module name>
 *
 * ===========================================================================
 * Contents: <Short but complete description of what is in the file.>
 *
 * <e.g.:
 * This file contains all functions for ….
 * 
 * ===========================================================================
 * References: <List of design documents covering this file.>
 */

/* Revisions
 * 710061: Change hooknum processing logic. IN2OUT (Postrouting) OUT2IN (Prerouting)
 */
/*
 * ===========================================================================
 *                           <INCLUDE FILES>
 * ===========================================================================
 */


#include <linux/module.h>
#include <net/tcp.h>
#include <net/udp.h>

#include <linux/netfilter_ipv4/lockhelp.h>
#if 0
#include <linux/netfilter_ipv4/ip_nat.h>
#include <linux/netfilter_ipv4/ip_nat_protocol.h>
#endif

DECLARE_RWLOCK_EXTERN(ip_sip_rwlock);

#define ASSERT_READ_LOCK MUST_BE_READ_LOCKED(&ip_sip_rwlock);
#define ASSERT_WRITE_LOCK MUST_BE_WRITE_LOCKED(&ip_sip_rwlock);

#include <linux/netfilter_ipv4/ip_nat_helper.h>
#include <linux/netfilter_ipv4/listhelp.h>

#include <linux/netfilter_ipv4/ifx_sip_common.h>
#include <linux/version.h>

/*
 * ===========================================================================
 *                           <DEFINITIONS & DECLARATIONS>
 * ===========================================================================
 */

MODULE_LICENSE("GPL");

DECLARE_LOCK_EXTERN(ip_sip_lock);
DECLARE_RWLOCK_EXTERN(ip_nat_lock);

DECLARE_RWLOCK(ip_sip_rwlock);
DECLARE_RWLOCK(ip_sip_user_reg_rwlock);
DECLARE_RWLOCK(ip_sip_temp_call_rwlock);
static DECLARE_LOCK(sip_nat_seqofs_lock);

struct module *ip_nat_sip = THIS_MODULE;

struct list_head  sip_call_data;
struct list_head sip_helper_registration_list;/* stores the registered port and protocols from user space*/
struct list_head sip_user_registration_list;/* stores the user registration data from Register messages*/
struct list_head sip_temp_call_data_list;/* stores the temp call data from invite messages; these call are not registered */

int synchronise_flag;

/* initialize the list */
LIST_HEAD(sip_call_data);
LIST_HEAD(sip_helper_registration_list);
LIST_HEAD(sip_user_registration_list);
LIST_HEAD(sip_temp_call_data_list);

DECLARE_WAIT_QUEUE_HEAD(wait_queue_sipdaemon);

//data structure to keep the active call related data
struct sip_call_data
{
	struct list_head list;
	int timer_set_flag; /*This flag is raised once the timer is set for the data */
	struct timer_list timeout;
	char original_call_id[CALL_ID_LENGTH]; /* FIXME : Use defines */
	char changed_call_id[CALL_ID_LENGTH];
	u_int32_t	localip; /* Ip address of the local machine */
	u_int32_t	algip; /* Ip address of the alg machine */
	int localport; /* Local port */
	int algport; /* Changed alg port */
	int key_algport; /* algport as a key to the sip_call_data */
	char from_to_field_port[8]; /* Port in the From/To field */
	int audio_rtp_original_port;
	int audio_rtp_changed_port;
	int image_original_port;
	int image_changed_port;
	int reg_msg_contact_port;
	enum call_origination call_dir;
	enum ip_conntrack_dir CALL_IP_CT_DIR_ORIGINAL;
	enum ip_conntrack_dir CALL_IP_CT_DIR_REPLY;
};

struct sip_helper_registration_data
{
	struct list_head list;
	int src_port;
	int dst_port;
	enum SipControlProtocol proto;
	struct ip_nat_helper* sip_nat;
	enum oper_type oper;
};

enum TypeOfChange{
	ADD_RULE=1,
	DELETE_RULE
};

//data structure to keep the registration message related data
struct sip_user_registration_data
{
	struct list_head list;
	int change_flag;//required for identifing which data needs to sent to user
	enum TypeOfChange typeofchange;
	int timer_set_flag;
	struct timer_list timeout;
	int timer_value;/* value of the last set timer*/
	char original_ip_address[16];
	char changed_ip_address[16];
#if 0
	int original_port;
	int changed_port;
#else
	int original_contact_port;
	int changed_contact_port; /* use this for adding/deleting DNAT rule */
#endif
	enum SipControlProtocol proto;
	char user_name[128];
	char user[128];
};

//data structure to keep the registration message related data
struct sip_temp_call_data
{
	struct list_head list;
	int change_flag;//required for identifing which data needs to sent to user
	enum TypeOfChange typeofchange;
	char original_ip_address[16];
	char changed_ip_address[16];
	int original_port;
	int changed_port;
	enum SipControlProtocol proto;
};
#if 0
#define DEBUGP	printk
#else
#define DEBUGP(format,args...)
#endif

#if 0
#define IDEBUG	printk
#else
#define IDEBUG(format,args...)
#endif

#if 0
#define SDEBUG	printk
#else
#define SDEBUG(format,args...)
#endif

#if 0 // chandrav -- for testing purpose
#define IFX_ALG_QOS_DBG  printk
#else
#define IFX_ALG_QOS_DBG(format, args...)
#endif

#ifdef __DEBUG_SIP_ALG__
#define PDEBUG	printk
#else
#define PDEBUG(format, args...)
#endif

#define SIP_ASSERT(tmp)	\
	if(!tmp)	\
return 1;	\


#define BUF_LEN 80
#define SUCCESS 0

#define RAISE_FLAG 1
#define DOWN_FLAG  0

static int Device_Open = 0;

/* is s2<=s1<=s3 ? */
__inline int sip_between(__u32 seq1, __u32 seq2, __u32 seq3)
{
	return seq3 - seq2 >= seq1 - seq2;
}

__inline int sip_before(__u32 seq1, __u32 seq2)
{
	return (__s32)(seq1-seq2) < 0;
}

extern char* sip_token_parser(char* data, enum TokenType token, char* endpointer, enum BeginOrEnd resultType);

extern void sip_get_method(char *szData, char *szMethod);

extern int calculate_skbuff_data(struct sk_buff **pskb,
		char **data,	
		char **endData, 
		enum SipControlProtocol* proto);
extern char* sip_strstr(const char*,const char*, const char*, enum BeginOrEnd);

extern int sip_help(const struct iphdr *iph, size_t len, struct ip_conntrack *ct,enum ip_conntrack_info ctinfo);

extern unsigned int sip_nat_help(struct ip_conntrack *ct,struct ip_conntrack_expect *exp,struct ip_nat_info *info,enum ip_conntrack_info ctinfo,	unsigned int hooknum,struct sk_buff **pskb);

extern unsigned int sip_nat_expected(struct sk_buff **pskb,\
		unsigned int hooknum,\
		struct ip_conntrack *ct,\
		struct ip_nat_info *info);


/* Extern definition */

extern int ct_sip_get_info(const char *dptr, size_t dlen, unsigned int *matchoff, unsigned int *matchlen, struct sip_header_nfo *hnfo); 
extern struct sip_header_nfo ct_sip_hdrs[];
extern int parse_ipaddr(const char *cp,	const char **endp, uint32_t *ipaddr, const char *limit);
extern enum SIP_MESSAGES get_sip_method(const char *dptr);

extern int sip_device_conn_port_deregister(struct sip_params* sipparam, enum oper_type oper);
extern int sip_device_conn_port_register(struct sip_params* sipparam, enum oper_type oper);
#define IFX_BUFFER_TIME			30

/* REG_PORT_OFFSET should lie outside the masquerade range in -j MASQ rule.
 * This guarantees that DNAT port and MASQ port don't overlap --> Separate
 * namespaces */
#if 0
#define REG_PORT_OFFSET 		0
#else
#define REG_PORT_OFFSET			40000

#endif

#define BASE_AUDIO_RTP_PORT 	5100
#define MAX_AUDIO_RTP_PORTS		100

#define BASE_IMAGE_PORT			7000
#define	MAX_IMAGE_PORTS			100

#define IFX_OFFSET_MARGIN		8

//#define IFX_SYMMTRICAL_RTP

int free_audio_rtp_port_offset = 0;
int free_image_port_offset = 0;

/* Nirav end */

/*
 * ===========================================================================
 *                           <LOCAL TYPES>
 * ===========================================================================
 */


enum sip_message_type
{
	INVITE = 1,
	BYE = 2,
	REQUEST = 3,
	RESPONSE = 4,
	TYPE_NOT_FOUND = 5
};

/*
 * ===========================================================================
 *                           <LOCAL FUNCTION PROTOTYPES>
 * ===========================================================================
 */

/* Local function prototypes to be declared here */

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

int calculate_skbuff_data(struct sk_buff **pskb,
		char **data,	
		char **endData, 
		enum SipControlProtocol* proto)
{
	struct iphdr *iph = (*pskb)->nh.iph;
	struct udphdr *udph;
	struct tcphdr *tcph;

	unsigned int udplen, tcplen;
	unsigned int szDatalen;
	//char *pendptr = NULL;
	//char **ppendptr = &pendptr;


	if(iph->protocol == 0x06)
	{
		*proto = TCP;
		tcph = (void*)iph+iph->ihl*4;
		*data = (void*)tcph + tcph->doff*4;
		tcplen = (*pskb)->len - iph->ihl*4;
		szDatalen = tcplen - tcph->doff*4;
		*endData = (char*)(*data) + (szDatalen -1);
	}
	else if(iph->protocol == 0x11)
	{
		*proto = UDP;
		udph = (void*)iph+iph->ihl*4;
		*data = (char*)udph + sizeof(struct udphdr);
		udplen = (*pskb)->len - iph->ihl*4;
		szDatalen = udplen - sizeof(struct udphdr);
		*endData = (char*)(*data) + (szDatalen -1);
	}

	return 1;
}

#if 0
char* sip_token_parser(char* data, 
		enum TokenType token, 
		char* endpointer, 
		enum BeginOrEnd resultType)
{
	int i = 0;
	char* result=NULL;
	while(strcmp(messagetoken[token].tokenlist[i],"****")!=0)
	{
		if((result = sip_strstr(data, messagetoken[token].tokenlist[i], endpointer, resultType))!= NULL)
		{
			return result;	
		}
		else
		{
			i++;
		}
	}

	return result;
}
#endif
char * sip_strstr(const char * s1,
		const char * s2, 
		const char *endpointer,
		enum BeginOrEnd resultType)
{
	int  l2;
	l2 = strlen(s2);
	if (!l2)
	{
		return NULL;
	}
	endpointer -= (l2-1);
	while (s1 <= endpointer) 
	{
		if (!memcmp(s1,s2,l2))
		{
			if(resultType == END)
			{
				return (char*)(s1+l2);
			}
			else
			{
				return (char *) s1;
			}
		}
		s1++;
	}
	return NULL;
}

/*
 * ===========================================================================
 *                           <LOCAL FUNCTIONS>
 * ===========================================================================
 */


/* This function is called whenever a process attempts 
 * to open the device file */
static int device_open(struct inode *inode, 
		struct file *file)
{
	PDEBUG("device_open(%p)\n", file);

	/* We don't want to talk to two processes at the 
	 * same time */
	//if (Device_Open)
	//	return -EBUSY;

	/* If this was a process, we would have had to be 
	 * more careful here, because one process might have 
	 * checked Device_Open right before the other one 
	 * tried to increment it. However, we're in the 
	 * kernel, so we're protected against context switches.
	 *
	 * This is NOT the right attitude to take, because we
	 * might be running on an SMP box, but we'll deal with
	 * SMP in a later chapter.
	 */ 

	Device_Open++;

	/* Initialize the message */
	//	Message_Ptr = Message;

	MOD_INC_USE_COUNT;

	return SUCCESS;
}

/* This function is called when a process closes the 
 * device file. It doesn't have a return value because 
 * it cannot fail. Regardless of what else happens, you 
 * should always be able to close a device (in 2.0, a 2.2
 * device file could be impossible to close).
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static int device_release(struct inode *inode, 
		struct file *file)
#else
static void device_release(struct inode *inode, 
		struct file *file)
#endif
{
	PDEBUG("device_release(%p,%p)\n", inode, file);

	/* We're now ready for our next caller */
	Device_Open --;

	MOD_DEC_USE_COUNT;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	return 0;
#endif
}



static int  get_registration_data(const struct sip_helper_registration_data* i, int src_port, int dst_port, enum SipControlProtocol proto, enum oper_type oper)
{
	if((i->src_port == src_port) && (i->dst_port == dst_port) && (i->proto == proto) && (i->oper == oper))
		return 1;
	else
		return 0;
}


static int device_port_deregister(struct sip_params* sipparam, enum oper_type oper)
{
	struct sip_helper_registration_data* sip_helper_registration_data;

	sip_helper_registration_data = LIST_FIND( &sip_helper_registration_list, get_registration_data,
			struct sip_helper_registration_data*, sipparam->sip_src_port, sipparam->sip_dst_port, sipparam->proto, oper);
	if(sip_helper_registration_data==NULL) {
		IDEBUG("sip_nat: No registration for src_port=%d, dst_port=%d and proto=%d, oper=%s exists\n",sipparam->sip_src_port,sipparam->sip_dst_port,sipparam->proto,(oper == STATIC_OPER ? "STATIC_OPER" : "DYNAMIC_OPER"));
		return -1;
	}

	/* Start deregistering */
	IDEBUG("sip_nat: De-Registering sip_nat_help for protocol %d and src_port=%d, dst_port=%d, oper=%s\n",sipparam->proto,sipparam->sip_src_port, sipparam->sip_dst_port,(oper == STATIC_OPER ? "STATIC_OPER" : "DYNAMIC_OPER"));
	ip_nat_helper_unregister(sip_helper_registration_data->sip_nat);
	PDEBUG("sip: Nat helper unregistered\n");
	kfree(sip_helper_registration_data->sip_nat);

	PDEBUG("sip: removing registration data from list\n");
	list_del(&sip_helper_registration_data->list);

	PDEBUG("sip: freeing up registration data\n");
	kfree(sip_helper_registration_data);

	return 0;
}

static int device_port_register(struct sip_params* sipparam, enum oper_type oper)
{
	int ret;
	struct ip_nat_helper* sip_nat = NULL;
	struct sip_helper_registration_data* sip_helper_registration_data = NULL;

	sip_helper_registration_data = LIST_FIND( &sip_helper_registration_list, get_registration_data, struct sip_helper_registration_data*, sipparam->sip_src_port, sipparam->sip_dst_port, sipparam->proto, STATIC_OPER);
	if(sip_helper_registration_data==NULL) {
		sip_helper_registration_data = LIST_FIND( &sip_helper_registration_list, get_registration_data, struct sip_helper_registration_data*, sipparam->sip_src_port, sipparam->sip_dst_port, sipparam->proto, DYNAMIC_OPER);
	}

	if (sip_helper_registration_data != NULL) {
		IDEBUG("sip_nat: Registration for src_port=%d, dst_port=%d and proto=%d alredy exists\n",sipparam->sip_src_port,sipparam->sip_dst_port,sipparam->proto);
		return -1;
	}

	/* Need to check whether port is already existing*/
	sip_helper_registration_data = kmalloc(sizeof(struct sip_helper_registration_data), GFP_ATOMIC);
	if(sip_helper_registration_data == NULL) {
		PDEBUG("sip: Memory not available\n");
		return -1;
	}

	sip_nat = kmalloc(sizeof(struct ip_nat_helper), GFP_ATOMIC);
	if(sip_nat==NULL) {
		PDEBUG("sip: Memory not available\n");
		kfree(sip_helper_registration_data);
	//	kfree(sip_conntrack);
		return -1;
	}

	memset(sip_nat,0,sizeof(struct ip_nat_helper));
	memset(sip_helper_registration_data,0,sizeof(struct sip_helper_registration_data));

	/* Init list head of the sip_helper_registration_data*/
	INIT_LIST_HEAD(&sip_helper_registration_data->list);

	memset(&(sip_nat->mask),0x00,sizeof(sip_nat->mask));
	memset(&(sip_nat->tuple),0x00,sizeof(sip_nat->tuple));
#if 0
	if(sipparam->proto == UDP) {
		sip_nat->tuple.dst.protonum = IPPROTO_UDP;
		if (sipparam->sip_src_port > 0) {
			sip_nat->tuple.dst.u.udp.port = htons(sipparam->sip_src_port);
			sip_nat->mask.dst.u.udp.port = 0xFFFF;
			IDEBUG("sip_nat: Registering sip_nat for UDP SRC_PORT=%d\n",sipparam->sip_src_port);
		} else if (sipparam->sip_dst_port > 0) {
			sip_nat->tuple.src.u.udp.port = htons(sipparam->sip_dst_port);
			sip_nat->mask.src.u.udp.port = 0xFFFF;
			IDEBUG("sip_nat: Registering sip_nat for UDP DST_PORT=%d\n",sipparam->sip_dst_port);
	}
	} else if(sipparam->proto ==TCP) {
		sip_nat->tuple.dst.protonum = IPPROTO_TCP;
		if (sipparam->sip_src_port > 0) {
			sip_nat->tuple.dst.u.tcp.port = htons(sipparam->sip_src_port);
			sip_nat->mask.dst.u.tcp.port = 0xFFFF;
			IDEBUG("sip_nat: Registering sip_nat for TCP SRC_PORT=%d\n",sipparam->sip_src_port);
		} else if (sipparam->sip_dst_port > 0) {
			sip_nat->tuple.src.u.tcp.port = htons(sipparam->sip_dst_port);
			sip_nat->mask.src.u.tcp.port = 0xFFFF;
			IDEBUG("sip_nat: Registering sip_nat for TCP DST_PORT=%d\n",sipparam->sip_dst_port);
	}
	}
#else
	/* Sumedh - sip_nat's tuple is reversed, set only the src as configured from known destination */
	if(sipparam->proto == UDP) {
		sip_nat->tuple.dst.protonum = IPPROTO_UDP;
		sip_nat->tuple.src.u.udp.port = htons(sipparam->sip_dst_port);
		sip_nat->mask.src.u.udp.port = 0xFFFF;
		IDEBUG("sip_nat: Registering sip_nat for UDP DST_PORT=%d\n",sipparam->sip_dst_port);
	} else if(sipparam->proto ==TCP) {
		sip_nat->tuple.dst.protonum = IPPROTO_TCP;
		sip_nat->tuple.src.u.tcp.port = htons(sipparam->sip_dst_port);
		sip_nat->mask.src.u.tcp.port = 0xFFFF;
		IDEBUG("sip_nat: Registering sip_nat for TCP DST_PORT=%d\n",sipparam->sip_dst_port);
	}

#endif
#if 0
	sip_nat->mask.dst.protonum = 0xFFFF;
#else
	/* To handle TCP and UDP both the protocol */
	sip_nat->mask.dst.protonum = ~(IPPROTO_UDP | IPPROTO_TCP);
#endif

	sip_nat->help = sip_nat_help;
	sip_nat->me = THIS_MODULE;
	sip_nat->flags = IP_NAT_HELPER_F_ALWAYS;
	sip_nat->expect = sip_nat_expected;

	ret = ip_nat_helper_register(sip_nat);
	if(ret != 0) {
		IDEBUG("sip_nat: failed to register sip_nat helper for protocol %d and src_port=%d, dst_port=%d\n",sipparam->proto,sipparam->sip_src_port,sipparam->sip_dst_port);
		kfree(sip_nat);
		kfree(sip_helper_registration_data);
		return ret;
	}
	PDEBUG("sip: sip_nat helper registered\n");
	IDEBUG("sip_nat: Registering sip_nat_help for protocol %d and src_port=%d, dst_port=%d\n",sipparam->proto,sipparam->sip_src_port,sipparam->sip_dst_port);

	/* store the addresses in the sip_helper_registration_list*/
	sip_helper_registration_data->src_port = sipparam->sip_src_port;
	sip_helper_registration_data->dst_port = sipparam->sip_dst_port;
	sip_helper_registration_data->proto = sipparam->proto;
	sip_helper_registration_data->sip_nat = sip_nat;
	sip_helper_registration_data->oper = oper;

	PDEBUG("sip: Adding Sip_registration_data\n");
	list_prepend(&sip_helper_registration_list, &sip_helper_registration_data->list);
	return 0;
}

static int get_data_with_flag_raised(const struct sip_user_registration_data* i)
{
	if(i->change_flag == RAISE_FLAG)
		return 1;
	else
		return 0;
}

static int get_temp_call_data_with_flag_raised(const struct sip_temp_call_data* i)
{
	if(i->change_flag == RAISE_FLAG)
		return 1;
	else
		return 0;
}

/* This function is called whenever a process tries to 
 * do an ioctl on our device file. We get two extra 
 * parameters (additional to the inode and file 
 * structures, which all device functions get): the number
 * of the ioctl called and the parameter given to the 
 * ioctl function.
 *
 * If the ioctl is write or read/write (meaning output 
 * is returned to the calling process), the ioctl call 
 * returns the output of this function.
 */
int device_ioctl(
		struct inode *inode,
		struct file *file,
		unsigned int ioctl_num,/* The number of the ioctl */
		unsigned long ioctl_param) /* The parameter to it */
{
	//int i;
	//char *temp;
	struct sip_params *sipparam;
	struct sip_dnat_param *dnat_param;
	struct sip_user_registration_data* sip_reg_data=NULL;
	struct sip_temp_call_data* sip_temp_call=NULL;
	//char ch;
	PDEBUG("Received ioctl call\n");
	/* Switch according to the ioctl called */
	switch (ioctl_num) {
		case IOCTL_SET_MSG:
			break;

		case IOCTL_DEREGISTER_PORT:
			sipparam=(struct sip_params*)ioctl_param;
			device_port_deregister(sipparam,STATIC_OPER);
			break;

		case IOCTL_REGISTER_PORT:
			PDEBUG("Received IOCTL_REGISTER_PORT\n");
			sipparam = (struct sip_params*)ioctl_param;
			device_port_register(sipparam,STATIC_OPER);
			break;

		case IOCTL_SIP_ALG_DAEMON:
				//check if anything can be given to the userspace
			WRITE_LOCK(&ip_sip_user_reg_rwlock);					
			sip_reg_data = LIST_FIND(&sip_user_registration_list, get_data_with_flag_raised, struct sip_user_registration_data*);
			if(sip_reg_data != NULL) {
				IDEBUG("sip_nat: device_ioctl - sip_reg_data data found\n");
				dnat_param = (struct sip_dnat_param*)ioctl_param;
				//Sanitise the memory chunk
				memset(dnat_param, 0, sizeof(struct sip_dnat_param));
					
				//Set the valid data flag
				dnat_param->validdataflag = RAISE_FLAG;
				dnat_param->original_port = sip_reg_data->original_contact_port;
				dnat_param->changed_port = sip_reg_data->changed_contact_port;
				//sanitise the char fields
				memset(dnat_param->original_ip_address, '\0', sizeof(dnat_param->original_ip_address));
				memset(dnat_param->changed_ip_address, '\0', sizeof(dnat_param->changed_ip_address));
				//store the ip addresses
				memcpy(	dnat_param->original_ip_address, sip_reg_data->original_ip_address, sizeof(sip_reg_data->original_ip_address));
				memcpy(	dnat_param->changed_ip_address, sip_reg_data->changed_ip_address, sizeof(sip_reg_data->changed_ip_address));
				dnat_param->proto = sip_reg_data->proto;
					
				//tell the user space what to do
				dnat_param->typeofchange = sip_reg_data->typeofchange;

				//data is copied.. down the flag
				sip_reg_data->change_flag = DOWN_FLAG;

				//If the typeofchange is for deletion, and no timer is
				//pending then delete the data structure now.
				if(sip_reg_data->typeofchange == DELETE_RULE) {
					if(!timer_pending(&sip_reg_data->timeout)) {
						list_del(&sip_reg_data->list);
						kfree(sip_reg_data);
				}
				}

				WRITE_UNLOCK(&ip_sip_user_reg_rwlock);	
				return 0;
			} 
			WRITE_UNLOCK(&ip_sip_user_reg_rwlock);	

			WRITE_LOCK(&ip_sip_temp_call_rwlock);					

			sip_temp_call = LIST_FIND(&sip_temp_call_data_list, get_temp_call_data_with_flag_raised, struct sip_temp_call_data*);
			if (sip_temp_call != NULL) {
				IDEBUG("sip_nat: device_ioctl - sip_temp_call data found\n");
					dnat_param = (struct sip_dnat_param*)ioctl_param;
					//Sanitise the memory chunk
					memset(dnat_param, 0, sizeof(struct sip_dnat_param));
					
					//Set the valid data flag
					dnat_param->validdataflag = RAISE_FLAG;
				dnat_param->original_port = sip_temp_call->original_port;
				dnat_param->changed_port = sip_temp_call->changed_port;
					//sanitise the char fields
				memset(dnat_param->original_ip_address, '\0', sizeof(dnat_param->original_ip_address));
				memset(dnat_param->changed_ip_address, '\0', sizeof(dnat_param->changed_ip_address));
					//store the ip addresses
				memcpy(	dnat_param->original_ip_address, sip_temp_call->original_ip_address, sizeof(sip_temp_call->original_ip_address));
				memcpy(	dnat_param->changed_ip_address, sip_temp_call->changed_ip_address, sizeof(sip_temp_call->changed_ip_address));
				dnat_param->proto = sip_temp_call->proto;

					//tell the user space what to do
				dnat_param->typeofchange = sip_temp_call->typeofchange;

					//data is copied.. down the flag
				sip_temp_call->change_flag = DOWN_FLAG;
					
					//If the typeofchange is for deletion, and no timer is
					//pending then delete the data structure now.
				if(sip_temp_call->typeofchange == DELETE_RULE) {
					list_del(&sip_temp_call->list);
					kfree(sip_temp_call);
					}
				WRITE_UNLOCK(&ip_sip_temp_call_rwlock);	
					return 0;
				}
			WRITE_UNLOCK(&ip_sip_temp_call_rwlock);	

			//if synchronise_flag is UP, make it down and
			//wake up the process before going to sleep
			PDEBUG("Entering sleep\n");	
			interruptible_sleep_on_timeout(&wait_queue_sipdaemon,10*HZ);

			PDEBUG("Waking up\n");
					return 0;
				break;

		case IOCTL_SIP_TEST_CASE:
			PDEBUG("Test case.. waking up the sleep process\n");
			wake_up_interruptible(&wait_queue_sipdaemon);
			break;
	}
	return SUCCESS;
}

/* This structure will hold the functions to be called 
 * when a process does something to the device we 
 * created. Since a pointer to this structure is kept in 
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions. */
struct file_operations Fops = {
ioctl: device_ioctl,   /* ioctl */
	   open: device_open,
	   release: device_release  /* a.k.a. close */
};



/******************************************************************
 *  Function Name	: 	sip_resize_packet
 *  Description      :  	Resizes the skbuff according to the newlen
 *  					    only if the newlen > oldlen + tailroom
 *  Input Values    	:  	struct sk_buff **skb,
 *  						u_int32_t newlen
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes			:   expands skbuff if required 
 *********************************************************************/
int sip_resize_packet(struct sk_buff **skb,
		u_int32_t newlen)
{
	struct sk_buff *newskb;

	if(newlen > (*skb)->len + skb_tailroom(*skb)) {

		newskb = skb_copy_expand(*skb, skb_headroom(*skb), 
				newlen - (*skb)->len,
				GFP_ATOMIC);
		if (!newskb) {
			printk("ip_nat_resize_packet: failed\n");
			return 0;
		} else {
			kfree_skb(*skb);
			*skb = newskb;
		}
	}

	return 1;
}

static inline int sip_nat_resize_packet(struct sk_buff **skb,
		struct ip_conntrack *ct, 
		enum ip_conntrack_info ctinfo,
		int new_size)
{
	struct iphdr *iph;
	struct tcphdr *tcph;
	void *data;
	int dir;
	struct ip_nat_seq *this_way, *other_way;

	DEBUGP("ip_nat_resize_packet: old_size = %u, new_size = %u\n",
			(*skb)->len, new_size);

	iph = (*skb)->nh.iph;
	tcph = (void *)iph + iph->ihl*4;
	data = (void *)tcph + tcph->doff*4;

	dir = CTINFO2DIR(ctinfo);

	this_way = &ct->nat.info.seq[dir];
	other_way = &ct->nat.info.seq[!dir];

	if (new_size > (*skb)->len + skb_tailroom(*skb)) {
		struct sk_buff *newskb;
		newskb = skb_copy_expand(*skb, skb_headroom(*skb),
				new_size - (*skb)->len,
				GFP_ATOMIC);

		if (!newskb) {
			printk("ip_nat_resize_packet: oom\n");
			return 0;
		} else {
			kfree_skb(*skb);
			*skb = newskb;
		}
	}

	iph = (*skb)->nh.iph;
	tcph = (void *)iph + iph->ihl*4;
	data = (void *)tcph + tcph->doff*4;

	DEBUGP("ip_nat_resize_packet: Seq_offset before: ");
	//	DUMP_OFFSET(this_way);

	LOCK_BH(&sip_nat_seqofs_lock);

	/* SYN adjust. If it's uninitialized, of this is after last 
	 * correction, record it: we don't handle more than one 
	 * adjustment in the window, but do deal with common case of a 
	 * retransmit */

	/* Need to pass flag to figure out whether its 1st, intermidiate
	   or last adjustment
	 */
#if 0
	if(ct->help.ct_sip_info.SeqNumberingFlag == FIRST_TIME_ENTRY)
	{
		this_way->correction_pos = ntohl(tcph->seq);
		this_way->offset_before = this_way->offset_after;
		this_way->offset_after = (int32_t)
			this_way->offset_before + new_size - (*skb)->len;
		ct->help.ct_sip_info.SeqNumberingFlag = FIRST_CHANGE_DONE;
		DEBUGP("ip_nat_resize_packet: 1st time Correcting offset\n");

	}
	else if(ct->help.ct_sip_info.SeqNumberingFlag == FIRST_CHANGE_DONE)
	{
		this_way->offset_after = (int32_t)
			this_way->offset_after + new_size - (*skb)->len;
		DEBUGP("ip_nat_resize_packet: Recorrecting offset\n");

	}
	else if(ct->help.ct_sip_info.SeqNumberingFlag == RETRANSMISSION)
	{
		DEBUGP("ip_nat_resize_packet: Retransmission no change in  offset\n");
	}
#endif
	if (this_way->offset_before == this_way->offset_after
	    || sip_before(this_way->correction_pos, ntohl(tcph->seq))) {
		this_way->correction_pos = ntohl(tcph->seq);
		this_way->offset_before = this_way->offset_after;
		this_way->offset_after = (int32_t)
			this_way->offset_before + new_size - (*skb)->len;

		PDEBUG("ip_nat_resize_packet: Correcting offset\n");
		PDEBUG("First time seq adjustment\n");
		PDEBUG("correction_pos = [%u]\n",this_way->correction_pos);
		PDEBUG("this_way->offset_before = [%d]\n",this_way->offset_before);
		PDEBUG("this_way->offset_after = [%d]\n", this_way->offset_after);
	}
	else if(this_way->correction_pos == ntohl(tcph->seq))
	{
		//for 2nd mangling onwards
		//only adjust the offset after value
		this_way->offset_after = (int32_t)
				this_way->offset_after + new_size - (*skb)->len;
		
		PDEBUG("Offset_after adjustment\n");
		PDEBUG("this_way->offset_after = [%d]\n", this_way->offset_after);
	}

	UNLOCK_BH(&sip_nat_seqofs_lock);
#if 0
	DEBUGP("ip_nat_resize_packet: Seq_offset after: ");
	DUMP_OFFSET(this_way);
#endif
	return 1;
}



int sip_mangle_tcp_data(struct sk_buff **skb,
		struct ip_conntrack *ct,
		enum ip_conntrack_info ctinfo,
		unsigned int match_offset,
		unsigned int match_len,
		char *rep_buffer,
		unsigned int rep_len)
{
	struct iphdr *iph = (*skb)->nh.iph;
	struct tcphdr *tcph;
	unsigned char *data;
	u_int32_t tcplen, newlen, newtcplen;

	tcplen = (*skb)->len - iph->ihl*4;
	newtcplen = tcplen - match_len + rep_len;
	newlen = iph->ihl*4 + newtcplen;

	if (newlen > 65535) {
		if (net_ratelimit())
			printk("ip_nat_mangle_tcp_packet: nat'ed packet "
					"exceeds maximum packet size\n");
		return 0;
	}

	if ((*skb)->len != newlen) {
		if (!sip_nat_resize_packet(skb, ct, ctinfo, newlen)) {
			printk("resize_packet failed!!\n");
			return 0;
		}
	}

	/* Alexey says: if a hook changes _data_ ... it can break
	   original packet sitting in tcp queue and this is fatal */
	if (skb_cloned(*skb)) {
		struct sk_buff *nskb = skb_copy(*skb, GFP_ATOMIC);
		if (!nskb) {
			if (net_ratelimit())
				printk("Out of memory cloning TCP packet\n");
			return 0;
		}
		/* Rest of kernel will get very unhappy if we pass it
		   a suddenly-orphaned skbuff */
		if ((*skb)->sk)
			skb_set_owner_w(nskb, (*skb)->sk);
		kfree_skb(*skb);
		*skb = nskb;
	}

	/* skb may be copied !! */
	iph = (*skb)->nh.iph;
	tcph = (void *)iph + iph->ihl*4;
	data = (void *)tcph + tcph->doff*4;

	/* move post-replacement */
	memmove(data + match_offset + rep_len,
			data + match_offset + match_len,
			(*skb)->tail - (data + match_offset + match_len));

	/* insert data from buffer */
	memcpy(data + match_offset, rep_buffer, rep_len);

	/* update skb info */
	if (newlen > (*skb)->len) {
		DEBUGP("ip_nat_mangle_tcp_packet: Extending packet by "
				"%u to %u bytes\n", newlen - (*skb)->len, newlen);
		skb_put(*skb, newlen - (*skb)->len);
	} else {
		DEBUGP("ip_nat_mangle_tcp_packet: Shrinking packet from "
				"%u to %u bytes\n", (*skb)->len, newlen);
		skb_trim(*skb, newlen);
	}

	/* fix checksum information */

	iph->tot_len = htons(newlen);
	(*skb)->csum = csum_partial((char *)tcph + tcph->doff*4,
				    newtcplen - tcph->doff*4, 0);

	tcph->check = 0;
	tcph->check = tcp_v4_check(tcph, newtcplen, iph->saddr, iph->daddr,
			csum_partial((char *)tcph, tcph->doff*4,
				(*skb)->csum));
	ip_send_check(iph);

	return 1;

}




/******************************************************************
 *  Function Name	:  	sip_mangle_udp_data
 *  Description      :  	Changes the data portion of the udp packet
 *  						with rep_buffer content.
 *  Input Values    	:  	struct sk_buff **skb,
 *  						struct ip_conntrack *ct,
 *  						enum ip_conntrack_info ctinfo,
 *  						unsigned int match_offset,
 *  						unsigned int match_len,
 *  						char *rep_buffer,
 *  						unsigned int rep_len
 *  Output Values 	:  
 *  Return Valu   	:	int
 *  Notes			: 
 *********************************************************************/
int sip_mangle_udp_data(struct sk_buff **skb,
		struct ip_conntrack *ct,
		enum ip_conntrack_info ctinfo,
		unsigned int match_offset,
		unsigned int match_len,
		char *rep_buffer,
		unsigned int rep_len)
{
	struct iphdr *iph = (*skb)->nh.iph;
	struct udphdr *udph = (void *)iph + iph->ihl * 4;
	unsigned char *data = (char *)udph + sizeof(struct udphdr);
	unsigned int udplen = (*skb)->len - iph->ihl * 4;

	u_int32_t newlen, newudplen;

	newudplen = udplen - match_len + rep_len;
	newlen = iph->ihl*4 + newudplen;

	if (newlen > 65535)
	{
		if (net_ratelimit())
			PDEBUG("sip_mangle_udp_data: nat'ed packet "
					"exceeds maximum packet size\n");
		return 0;
	}         

	if ((*skb)->len != newlen)
	{
		if (!sip_resize_packet(skb, newlen))
		{
			PDEBUG("resize_packet failed!!\n");
			return 0;
		}
	}

	/* skb may be copied !! */
	iph = (*skb)->nh.iph;
	udph = (void *)iph + iph->ihl*4;
	data = (void *)udph + sizeof(struct udphdr);

	/* move post-replacement */
	memmove(data + match_offset + rep_len,
			data + match_offset + match_len,
			(*skb)->tail - (data + match_offset + match_len));

	/* insert data from buffer */

	PDEBUG("sip_nat: Mangling data ......\n");
	memcpy(data + match_offset, rep_buffer, rep_len);

	/* update skb info */
	if (newlen > (*skb)->len)
	{
		PDEBUG("sip_mangle_udp_packet: Extending packet by "
				"%u to %u bytes\n", 
				newlen - (*skb)->len, newlen);
		skb_put(*skb, newlen - (*skb)->len);
	}
	else
	{
		PDEBUG("ip_nat_mangle_udp_packet: Shrinking packet from "
				"%u to %u bytes\n", (*skb)->len, newlen);
		skb_trim(*skb, newlen);
	}

	iph->tot_len = htons(newlen);	
	udph->len = htons(newudplen);

	udph->check	= 0;
	ip_send_check(iph);

	return 1;
}




/******************************************************************
 *  Function Name	:  	sip_get_content_length
 *  Description      :  	Checks if there is a message content other 
 *  						than the header in the Sip message.
 *  Input Values    	:  	struct sk_buff **pskb
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes			: 
 *********************************************************************/
static int sip_get_content_length(struct sk_buff **pskb)
{

	char* data;
	char* endData;
	enum SipControlProtocol proto;
	int iRet = 0;
	int matchoff = 0, matchlen = 0;

	calculate_skbuff_data(pskb,&data,&endData,&proto);

	if (ct_sip_get_info(data, (int)(endData - data), &matchoff, &matchlen, &ct_sip_hdrs[POS_CONTENT]) > 0) {
		iRet = simple_strtoul(data + matchoff, NULL, 10);
	}
	return iRet;
}

/******************************************************************
 *  Function Name	:  	field_token_handler
 *  Description 	:  	Mangles the Sip message according to the
 *  				field_token passed to this functions and 
 *  				the ipaddress is changed by the rep_buffer
 *  				content.
 *  Input Values    	:  	struct ip_conntrack *ct,
 *  				struct sk_buff **pskb,
 *  				enum ip_conntrack_info ctinfo,
 *  				enum sip_message_fields field_token,
 *  				char* rep_buffer,
 *  				int   rep_len
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes		: 
 *********************************************************************/
static int field_token_handler(struct ip_conntrack *ct,
		struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		int pos_hdr,
		char *rep_buffer,
		char *tobe_rep_buffer,
		int *ptoken_offset)
{
	char *data, *endData;
	int matchoff = 0, matchlen = 0;
	enum SipControlProtocol proto;
	int iRet = -EINVAL;

	calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);
	if (ptoken_offset != NULL) {
		data += *ptoken_offset - (*ptoken_offset > IFX_OFFSET_MARGIN ? IFX_OFFSET_MARGIN : *ptoken_offset);
	}

	if (ct_sip_get_info(data, endData - data, &matchoff, &matchlen, &ct_sip_hdrs[pos_hdr]) > 0) {
		if (matchlen == -1)
			matchlen = 0;
		if (tobe_rep_buffer == NULL || (tobe_rep_buffer != NULL && (strncmp(data + matchoff, tobe_rep_buffer, matchlen) == 0))) {
			if(ptoken_offset)
				matchoff += *ptoken_offset - (*ptoken_offset > IFX_OFFSET_MARGIN ? IFX_OFFSET_MARGIN : *ptoken_offset);
			if (proto == UDP) {
				iRet = sip_mangle_udp_data(pskb, ct, ctinfo, 
						matchoff, matchlen, rep_buffer,
						strlen(rep_buffer));
			} else if (proto == TCP) {
				iRet = sip_mangle_tcp_data(pskb, ct, ctinfo, 
						matchoff, matchlen, rep_buffer,
						strlen(rep_buffer));
	}
			//IDEBUG("sip_nat: field_token_handler - tobe_rep_buffer [%s] is replaced with rep_buffer [%s]\n",tobe_rep_buffer,rep_buffer);
			if (ptoken_offset != NULL)
				*ptoken_offset = matchoff + ct_sip_lnlen(data + matchoff, endData);
		} else {
			//IDEBUG("sip_nat: field_token_handler - tobe_rep_buffer [%s] does not match \n[<<%s>>]\n",tobe_rep_buffer,data + matchoff);
		}
	} else {
		/* The header OR ipaddr in that header does not exist
		 * So simply ignore it
		 */
	}
	return iRet;
}

/* Get the connection ip from the c: in the SDP */
static unsigned int get_connection_ip(struct sk_buff **pskb)
{
	char *data, *endData;
	int matchoff = 0, matchlen = 0;
	enum SipControlProtocol proto;
	char localip[16];
	int ip = 0, i = 3;

	calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);
	if (ct_sip_get_info(data, endData - data, &matchoff, &matchlen, &ct_sip_hdrs[POS_CONECTION]) > 0) {
		char *temp_beg = NULL;
		char *temp_end = NULL;
		memset(localip, 0x00, sizeof(localip));
		memcpy(localip, data + matchoff, matchlen);
		temp_beg = localip;

		while((temp_end = strchr(temp_beg,'.')) != NULL) {
			*temp_end = '\0';
			temp_end++;
			ip |= (simple_strtoul(temp_beg, NULL, 10) << i * 8);
			temp_beg = temp_end;
			i--;
		}
		if (temp_beg)
			ip |= simple_strtoul(temp_beg, NULL, 10);
	}
	return ip;
}

static int ifx_conntrack_change_expect(struct ip_conntrack_expect *expect,
		struct ip_conntrack_tuple *pnewtuple, u_int16_t *pport)
{
	int ret = 0;
	for (; *pport != 0; *pport += 2) {
		pnewtuple->dst.u.udp.port = htons(*pport);
		/* store the old ip. In case changing of the
		   expectation is a success.. we need to store
		   this for mangling purpose
		   */

		if (ip_conntrack_change_expect(expect, pnewtuple) == 0) {
			IDEBUG("sip_nat: Changing expectation with new tuple.......port: %d....\n",ntohs(*pport));

			ret=1;/* raise the flag */
			break;
		}
	}
	return ret;
}

/******************************************************************
 *  Function Name	:  	sip_data_fixup
 *  Description      :  	This function takes care of the port binding
 *  						and changing the expectations according to 
 *  						the bindings and changing the SDP data in 
 *  						accordance with the binded port.
 *  Input Values    	:  	struct ip_ct_sip_expect *info,
 *  						struct ip_conntrack *ct,
 *  						struct sk_buff **pskb,
 *  						enum ip_conntrack_info ctinfo,
 *  						struct ip_conntrack_expect *expect
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes			: 	One of the most crucial function in the whole
 *  						module. Please handle carefully.
 *  				** This function is NOT called when the call
 *  				is from 1 LAN ph to another **
 *
 *********************************************************************/
static int sip_data_fixup( struct ip_conntrack *ct,
		struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		struct ip_conntrack_expect *expect,
		struct sip_call_data* call_data) 
{
	u_int16_t port = 0;
	struct ip_conntrack_tuple newtuple;
	char port_buffer[8];
	char orig_port_buffer[8];
	char rep_buffer[16];
	char tobe_rep_buffer[16];
	u_int32_t localip;
	int token_offset = 0;

	sprintf(rep_buffer,"%u.%u.%u.%u",NIPQUAD(call_data->algip));
	sprintf(tobe_rep_buffer,"%u.%u.%u.%u",NIPQUAD(call_data->localip));
	MUST_BE_LOCKED(&ip_sip_lock);
	if (expect) {
		struct ip_ct_sip_expect *info = &(expect->help.exp_sip_info);

	memset(&newtuple,0,sizeof(struct ip_conntrack_tuple));
	/* Change address inside packet to match way we're mapping
	 *        this connection. */
		newtuple.dst.ip = call_data->algip;

	//RTP is always on UDP. So expectation protocols is always UDP
	newtuple.dst.protonum = IPPROTO_UDP;
	newtuple.src.u.udp.port = expect->tuple.src.u.udp.port;
	
	/*
	 * Only mangle when the port in the Sip message matches 
	 * with the port of the info othewise do not touch..
	 * It cannot be put inside the info->flag=0 because a 
	 * message can be repeated and in that case it need to 
	 * mangled but not binded again.
	 */
		if(info->flag == 0) {
			SDEBUG("First time.... entering for binding\n");
			port = info->orig_port;	

			switch (info->media_type) {
				case AUDIO_RTCP:
					SDEBUG("sip_nat: This is AUDIO_RTCP IMPLICIT/EXPLICIT expectation for port=%d\n",info->orig_port);
					port = call_data->audio_rtp_changed_port + 1;
					if (ifx_conntrack_change_expect(expect, &newtuple, &port) == 1) {
						info->flag = 1;
						if (info->exp_type == EXPLICIT) {
							/* modify the rtcp media port */
							token_offset = info->exp_offset;
							sprintf(port_buffer, "%u", port);
							sprintf(orig_port_buffer, "%u", info->orig_port);
							field_token_handler(ct, pskb, ctinfo, POS_MEDIA_AUDIO_RTCP, port_buffer, orig_port_buffer, &token_offset);
			}
			}
				break;

				case AUDIO_RTP:
					SDEBUG("sip_nat: This is EXPLICIT audio expectation for port=%d\n",info->orig_port);
					if (call_data->audio_rtp_original_port == port) {
						port = call_data->audio_rtp_changed_port;
					} else {
						port = BASE_AUDIO_RTP_PORT + free_audio_rtp_port_offset;
						free_audio_rtp_port_offset = (free_audio_rtp_port_offset + 2) % MAX_AUDIO_RTP_PORTS;
						call_data->audio_rtp_original_port = info->orig_port;
			}
					if (ifx_conntrack_change_expect(expect, &newtuple, &port) == 1) {
						info->flag = 1;
						call_data->audio_rtp_changed_port = port;				
						/* modify the media port */
						token_offset = info->exp_offset;
						sprintf(port_buffer, "%u", call_data->audio_rtp_changed_port);
						sprintf(orig_port_buffer, "%u", call_data->audio_rtp_original_port);
						field_token_handler(ct, pskb, ctinfo, POS_MEDIA_AUDIO_RTP, port_buffer, orig_port_buffer, &token_offset);
			}
					break;

				case IMAGE_DATA:
					SDEBUG("sip_nat: This is EXPLICIT image expectation for port=%d\n",info->orig_port);
					if (call_data->image_original_port == port) {
						port = call_data->image_changed_port;
					} else {
						port = BASE_IMAGE_PORT + free_image_port_offset;
						free_image_port_offset = (free_image_port_offset + 2) % MAX_IMAGE_PORTS;
						call_data->image_original_port = info->orig_port;
			}
					if (ifx_conntrack_change_expect(expect, &newtuple, &port) == 1) {
						info->flag = 1;
						call_data->image_changed_port = port;				
						/* modify the media port */
						token_offset = info->exp_offset;
						sprintf(port_buffer, "%u", call_data->image_changed_port);
						sprintf(orig_port_buffer, "%u", call_data->image_original_port);
						field_token_handler(ct, pskb, ctinfo, POS_MEDIA_IMAGE, port_buffer, orig_port_buffer, &token_offset);
			}
					break;

				case VIDEO_RTP:
					break;

				case VIDEO_RTCP:
					break;
		}
		}
		if (port == 0) {
			SDEBUG("sip_data_fixup: no free port found!\n");
			return 0;
	}
		SDEBUG("sip_nat: Changed new expectation tuple : %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u\n",NIPQUAD(expect->tuple.src.ip),ntohl(expect->tuple.src.u.udp.port),NIPQUAD(expect->tuple.dst.ip),ntohl(expect->tuple.dst.u.udp.port));

		/* Currently, it is assumed that all the connection ip address are the same */
		localip = get_connection_ip(pskb);
		if (localip > 0){
			info->localip = localip;
	}
	}

	if ((*pskb)->sip_nat_applied == 0) {
		token_offset = 0;
		/* modify the connection ipaddr. */
		while(field_token_handler(ct, pskb, ctinfo, POS_CONECTION, rep_buffer, tobe_rep_buffer, &token_offset) != -EINVAL);

		/* modify the owner ipaddr */
		field_token_handler(ct, pskb, ctinfo, POS_OWNER, rep_buffer, tobe_rep_buffer, NULL);
	}

		return 0;
}

/******************************************************************
 *  Function Name	:  	get_call_data
 *  Description  	:  	Compares call-id of the message with the 
 *  				stored call-id in the sip_call_data and
 *  				returns a 1 for a match and 0 for a fail.
 *  Input Values    	:  	const struct sip_call_data* i, 
 *  					char* call_id,
 *  					int datalen
 *  Output Values 	:  	
 *  Return Value   	:	int
 *  Notes		:	** Apart from the Call-ID, use the information
 *  				of the local-ip of LAN phone for identifying if the call is same as
 * 				existing or new. If LanPh1 --> LanPh2 (through ext. proxy), then we will
 * 				see INVITE from proxy --> LAN ph2 again at ALG. This time, the conntrack's
 *				local ip(can be src or ORG/REPLY dir) should not match the Lan ph1 -->
 *  				Proxy call. Will work iff only 1 conntrack exists per call**
 *********************************************************************/
static int get_call_data(const struct sip_call_data* i, char* call_id,
		int datalen, u_int32_t ct_org_src_ip, u_int32_t ct_reply_src_ip) 
{
	/* The list search and compare function will be here */
	if(strncmp(i->original_call_id, call_id,datalen)== 0) {	
		if (i->localip == ct_org_src_ip || i->localip == ct_reply_src_ip)
			return 1;
	} else if(strncmp(i->changed_call_id, call_id,datalen)==0) {
		if (i->localip == ct_org_src_ip || i->localip == ct_reply_src_ip)
			return 1;
	}
	return 0;
}



/******************************************************************
 *  Function Name	:  	sip_nat_expected
 *  Description      	:  	This function is called when one of the 
 *  				expectation has been hit and the child
 *  				connectrack is beeing built for the 1st time.
 *  				This function is called only once.. and the 
 *  				rest of the packet follows the same mangling.
 *  Input Values    	: 	struct sk_buff **pskb,
 *  				unsigned int hooknum,
 *  				struct ip_conntrack *ct,
 *  				struct ip_nat_info *info 
 *  Output Values 	:  
 *  Return Value   	:	unsigned int
 *  Notes		: 
 *********************************************************************/
unsigned int sip_nat_expected(struct sk_buff **pskb,
		unsigned int hooknum,
		struct ip_conntrack *ct,
		struct ip_nat_info *info)
{
	struct ip_nat_multi_range mr;
	u_int32_t newdstip = 0, newsrcip = 0, newip = 0;
	struct ip_conntrack *master = master_ct(ct);
	struct ip_ct_sip_expect *exp_sip_info;
	u_int16_t newport = 0, newsrcport = 0, newdstport = 0;
	struct sip_call_data *call_data = NULL;

	IDEBUG("Entering sip_nat_expected\n");

	IP_NF_ASSERT(info);
	IP_NF_ASSERT(master);
	
	IP_NF_ASSERT(!(info->initialized & (1<<HOOK2MANIP(hooknum))));

	if ((master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip == master->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip) &&
			(master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip == master->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip)) {
		IDEBUG("sip_nat_expected : original and reply direction tuples are unchanged. No mangaling is required for this packet\n");
		/* It's done. */
		info->initialized |= (1 << HOOK2MANIP(hooknum));
		return NF_ACCEPT;
	}

	exp_sip_info = &ct->master->help.exp_sip_info;

	READ_LOCK(&ip_sip_rwlock);
	//call_data = LIST_FIND( &sip_call_data, get_call_data, struct sip_call_data*, exp_sip_info->call_id, strlen(exp_sip_info->call_id), exp_sip_info->key_algport);
	call_data = LIST_FIND( &sip_call_data, get_call_data, struct sip_call_data*, exp_sip_info->call_id, strlen(exp_sip_info->call_id), exp_sip_info->localip,0);
	READ_UNLOCK(&ip_sip_rwlock);

	if (call_data == NULL) {
		/* Cant do anything */
		//printk("(%s): call_data is NULL, exp->localip=%u.%u.%u.%u\n",__FUNCTION__, NIPQUAD(exp_sip_info->localip));
		info->initialized |= (1 << HOOK2MANIP(hooknum));
		return NF_ACCEPT;
	}
	//printk("(%s): call_data NOT NULL, exp->localip=%u.%u.%u.%u\n",__FUNCTION__, NIPQUAD(exp_sip_info->localip));

	IDEBUG("sip_nat_expected : Current packet %s. ct->dst.ip=%u.%u.%u.%u:%d, ct->src.ip=%u.%u.%u.%u:%d while call_data->algip=%u.%u.%u.%u and call_data->localip=%u.%u.%u.%u\n",(exp_sip_info->expectations_hit_direction == IN_2_OUT ? "IN_2_OUT" : "OUT_2_IN"),NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip),ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port,NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip),ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port,NIPQUAD(call_data->algip),NIPQUAD(call_data->localip));
	LOCK_BH(&ip_sip_lock);
	switch (exp_sip_info->expectations_hit_direction ){
		case IN_2_OUT:
			newsrcip = call_data->algip;
		newdstip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip;
#ifdef IFX_SYMMTRICAL_RTP
			/* For symmetric RTP connection - sourceRTP = destRTP */
			newsrcport = call_data->audio_rtp_changed_port + (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port % 2);
#else
			newsrcport = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port;
#endif
			newdstport = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port;
			break;

		case OUT_2_IN:
#if 0
			newdstip = call_data->localip;
#else
			newdstip = exp_sip_info->localip;
#endif
		newsrcip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
#if 0
			newdstport = call_data->audio_rtp_original_port + (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port % 2);
#else
			newdstport = exp_sip_info->orig_port;
#endif
			newsrcport = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port;
			break;
	}

	UNLOCK_BH(&ip_sip_lock);

	if(HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC) {
		IDEBUG("Manipulating src to %u.%u.%u.%u:%d\n",NIPQUAD(newsrcip),newsrcport);
		newip = newsrcip;
		newport = newsrcport;
	} else {
		IDEBUG("Manipulating dst to %u.%u.%u.%u:%d\n",NIPQUAD(newdstip),newdstport);
		newip = newdstip;
		newport = newdstport;
	}

	mr.rangesize = 1;
	mr.range[0].flags = IP_NAT_RANGE_MAP_IPS;// | IP_NAT_RANGE_PROTO_SPECIFIED;
	if ((exp_sip_info->expectations_hit_direction == OUT_2_IN && HOOK2MANIP(hooknum) == IP_NAT_MANIP_DST)
#ifdef IFX_SYMMTRICAL_RTP
			|| (exp_sip_info->expectations_hit_direction == IN_2_OUT && HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC)
#endif
	   ){
		mr.range[0].flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
	}
	mr.range[0].min_ip = mr.range[0].max_ip = newip;
	mr.range[0].min.udp.port = mr.range[0].max.udp.port = htons(newport);
	IDEBUG("sip_nat_expected : mr.flags=%d while newip=%u.%u.%u.%u:%d\n",mr.range[0].flags,NIPQUAD(newip),newport);

	return ip_nat_setup_info(ct, &mr, hooknum);
}

/* Sumedh - Used to search for a user at local ip or changed ip. 
 * e.g. abc@localip.com or abc@algip.com will both be tried for match
 * used for identifying a local call
 */
static int user_ip_search(const struct sip_user_registration_data *i, char *user, char *ip)
{
	SDEBUG("(%s): Finding %s@%s\n", __FUNCTION__, user, ip);
	if( user == NULL || ip == NULL)
	{	
		SDEBUG("(%s): user & ip NULL\n", __FUNCTION__);
		return 0;
	}
	SDEBUG("(%s): i->user = %s, i->orgip= %s\n", __FUNCTION__, i->user, i->original_ip_address);
	if( strncmp(i->user, user, strlen(i->user))==0 &&
		( strncmp(i->original_ip_address, ip, strlen(i->original_ip_address))==0 ||
		  strncmp(i->changed_ip_address, ip, strlen(i->changed_ip_address))==0
		 )
	  )
	{
		SDEBUG("(%s): Found match!\n", __FUNCTION__);
		return 1;
	}
	else
	{
		SDEBUG("(%s): No Match\n", __FUNCTION__);
	return 0;
	}	
}

#if 0  //Not used
/* Sumedh - given a ip and port, try to find info in registration list, 
 * match succeeds when ip==changed_ip and port==changed_contact port 
 */
static find_changed_ip_port(const struct sip_user_registration_data *i, char *ip, int port)
{
	if (ip == NULL || port == 0)
		return 0;
	if (strncmp(i->changed_ip_address, ip, strlen(i->changed_ip_address))==0) {
		if (i->changed_contact_port == port) {
			IDEBUG("(%s) : Matched", __FUNCTION__);
			return 1;
		}
	}
	return 0;
}
#endif

/* Sumedh - i->user_name is for e.g. 'abc@proxyip.com' 
 * find match in registration list (used for identifying a local call
 */
static int is_user_registered(const struct sip_user_registration_data *i, char *user_name)
{
	IDEBUG("!! (%s) Comparing i->name:%s with name:%s\n", __FUNCTION__, i->user_name, user_name);
	if( user_name == NULL)
	{
		IDEBUG("(%s) Return 0..\n", __FUNCTION__);
		return 0;
	}
	if( strncmp(i->user_name, user_name, strlen(i->user_name))==0)
	{
		IDEBUG("(%s) Return 1..\n", __FUNCTION__);
		return 1;
	}
	else
	{
		IDEBUG("(%s) Return 0..no match\n", __FUNCTION__);
		return 0;
	}
}

static int get_user_registration_data(const struct sip_user_registration_data* i, char* ip_address, int port, char *user_name, int flag)
{
	/* Cant match if everything is NULL!!! */
	if (ip_address == NULL && user_name == NULL && port == 0)
		return 0;	
	/*The list search and compare function*/
	if(ip_address == NULL || strncmp(i->original_ip_address, ip_address, strlen(i->original_ip_address))==0) {
		if(port == 0 || i->original_contact_port == port) 
			if(user_name == NULL || strncmp(user_name,i->user_name,strlen(user_name)) == 0)
				if (flag == i->change_flag)
					return 1;
	} else if(ip_address == NULL || strncmp(i->changed_ip_address, ip_address, strlen(i->changed_ip_address))==0) {
		if(port == 0 || i->changed_contact_port == port)
			if(user_name == NULL || strncmp(user_name,i->user_name,strlen(user_name)) == 0)
				if (flag == i->change_flag)
		return 1;
	}
	return 0;
}

static int get_temp_call_data(const struct sip_temp_call_data* i, char* ip_address, int port)
{
	/*The list search and compare function*/
	if(strncmp(i->original_ip_address, ip_address, strlen(i->original_ip_address))==0) {
		if(i->original_port == port) 
						return 1;
	} else if(strcmp(i->changed_ip_address, ip_address)==0) {
		if(i->changed_port == port)
						return 1;
				}
	return 0;
}		

/* Sumedh - extract source port from skb and return numeric form */
static int get_src_port_num(struct sk_buff *pskb, int* port )
{
	struct iphdr *iph = (pskb)->nh.iph;
	struct udphdr *udph;
	struct tcphdr *tcph;
	if(iph->protocol == 0x06) {
		tcph = (void*)iph+iph->ihl*4;
		*port = ntohs(tcph->source);
	} else if(iph->protocol == 0x11) {
		udph = (void*)iph+iph->ihl*4;
		*port = ntohs(udph->source);
	} else
		return -1;
	return 0;
}
/* Sumedh - extract source port from skb and return string form */
static int get_src_port(struct sk_buff *pskb, char* port, int* portlen )
{
	int portnum;

	if (get_src_port_num(pskb, &portnum) != 0)
	   return -1;	
	sprintf(port,"%d",portnum);
	*portlen = strlen(port);
	return 1;
}
/* Sumedh - extract destination port from skb and return string form */
static int get_dest_port(struct sk_buff **pskb, char* port, int* portlen )
{
	struct iphdr *iph = (*pskb)->nh.iph;
	struct udphdr *udph;
	struct tcphdr *tcph;
	if(iph->protocol == 0x06) {
		tcph = (void*)iph+iph->ihl*4;
		sprintf(port,"%d",ntohs(tcph->dest));
	} else if(iph->protocol == 0x11) {
		udph = (void*)iph+iph->ihl*4;
		sprintf(port,"%d",ntohs(udph->dest));
	}
	*portlen = strlen(port);
	return 1;
}
/******************************************************************
 *  Function Name	:  	in_req_initiated_inside_handler
 *  Description      	:  	This takes care of the necessary mangling
 *  				of a Request message coming in for a call
 *  				initiated from inside.
 *  Input Values    	: 	struct ip_conntrack *ct,
 *  				struct sk_buff **pskb,
 *  				enum ip_conntrack_info ctinfo 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes		: 
 *********************************************************************/
static int in_req_initiated_inside_handler(struct ip_conntrack *ct,
					   struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		char *localip_buf,
		char *localport_buf,
		char *algip_buf,
		char *algport_buf,
		char *from_to_field_port)
{
	char temp[20];
	char shdr_via_ip[16];
	int shdr_via_port=0;
	struct sip_user_registration_data *tmp_reg_data;

	sprintf(temp,":%s",algport_buf);
	
	SDEBUG(" In (%s) !\n", __FUNCTION__);
	/* To		: Replace the ALG address with local address */
	field_token_handler(ct, pskb, ctinfo, POS_TO, localip_buf, algip_buf, NULL);

	/* To Port	: Replace the ALG port with local port */
#if 0
	field_token_handler(ct, pskb, ctinfo, POS_TO_PORT, from_to_field_port, temp, NULL);
#endif
	
	/* Call-ID	: Replace the ALG address with local address */
	field_token_handler(ct, pskb, ctinfo, POS_CALLID_IP, localip_buf, algip_buf, NULL);

	/* Request-URI	: Replace the ALG address with local address*/
	field_token_handler(ct, pskb, ctinfo, POS_REQ_HEADER_IP , localip_buf, algip_buf, NULL);

	/* Request-URI	: Replace the ALG port with local port*/
	field_token_handler(ct, pskb, ctinfo, POS_REQ_HEADER_PORT , localport_buf, algport_buf, NULL);

	/* Proxy-Authorization : Replace the ALG address with local address */
	field_token_handler(ct, pskb, ctinfo, POS_PROXY_AUTH, localip_buf, algip_buf, NULL);

	return 1;
	
}

/******************************************************************
 *  Function Name	:  	in_res_initiated_inside_handler
 *  Description      	:  	This takes care of the necessary mangling
 *  				of a Response message coming in for a call
 *  				initiated from inside.
 *  Input Values    	: 	struct ip_conntrack *ct,
 *  				struct sk_buff **pskb,
 *  				enum ip_conntrack_info ctinfo 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes		: 
 *********************************************************************/
static int in_res_initiated_inside_handler(struct ip_conntrack *ct,
					   struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		char *localip_buf,
		char *localport_buf,
		char *algip_buf,
		char *algport_buf,
		char *from_to_field_port)
{
	char temp[20];
	char viaport_buf[8];
	char org_viaport_buf[8];
	sprintf(temp,":%s",algport_buf);
	SDEBUG(" In (%s) !\n", __FUNCTION__);
	/* From		: Replace the ALG address with local address */
	field_token_handler(ct, pskb, ctinfo, POS_FROM, localip_buf, algip_buf, NULL);

	/* From Port: Replace the ALG port with local port */
#if 0
	field_token_handler(ct, pskb, ctinfo, POS_FROM_PORT, from_to_field_port, temp, NULL);
#endif
	
	/* Call-ID	: Replace the ALG address with local address */
	field_token_handler(ct, pskb, ctinfo, POS_CALLID_IP, localip_buf, algip_buf, NULL);

	/* Via		: Replace the ALG address with local address *
	 * 		  New Via ip & port will be in ct's ORG dir SRC tuple
	 */
	if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_IP, localip_buf, algip_buf, NULL);
#if 0
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, localport_buf, algport_buf, NULL);
#else
		sprintf(viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.tcp.port));
		sprintf(org_viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.tcp.port));
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, org_viaport_buf, viaport_buf, NULL);
#endif
	} else {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_IP, localip_buf, algip_buf, NULL);
#if 0
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, localport_buf, algport_buf, NULL);
#else
		sprintf(viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port));
		sprintf(org_viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port));
		IDEBUG("in_res_initiated_inside_handler: VIA UDP %s->%s",org_viaport_buf, viaport_buf);
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, org_viaport_buf, viaport_buf, NULL);
#endif
	}

	return 1;
}
/******************************************************************
 *  Function Name	:  	out_req_initiated_inside_handler
 *  Description      	:  	This takes care of the necessary mangling
 *  				of a Request message going out for a call
 *  				initiated from inside.
 *  Input Values    	: 	struct ip_conntrack *ct,
 *  					struct sk_buff **pskb,
 *  					enum ip_conntrack_info ctinfo 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes		:	** If not a local call, then change Call-id & FROM ** 
 *********************************************************************/
static int out_req_initiated_inside_handler(struct ip_conntrack *ct,
					    struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		char *localip_buf,
		char *localport_buf,
		char *algip_buf,
		char *algport_buf,
		char *from_to_field_port,
		int local_call)
{
	char temp[20];
	char viaport_buf[8];
	int  viaport_len=0;
	sprintf(temp,":%s",algport_buf);
	SDEBUG(" In (%s) !\n", __FUNCTION__);
	
#if 1
	/* From		: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_FROM, algip_buf, localip_buf, NULL);
#else
	if(!local_call)
		field_token_handler(ct, pskb, ctinfo, POS_FROM, algip_buf, localip_buf, NULL);
#endif
	
#if 1
	/* Call-ID	: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_CALLID_IP, algip_buf, localip_buf, NULL);
#else
	if(!local_call)
		field_token_handler(ct, pskb, ctinfo, POS_CALLID_IP, algip_buf, localip_buf, NULL);
#endif

	/* Via		: Replace the local address with ALG address 
	 * 		  Local port with port from transport header *
	 */
	
	get_src_port(*pskb, viaport_buf, &viaport_len); /* get via port from transport hdr */
	if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
	
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_IP, algip_buf, localip_buf, NULL);
#if 0
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, algport_buf, localport_buf, NULL);
#else
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, viaport_buf, NULL /*localport_buf*/, NULL);
#endif
	} else {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_IP, algip_buf, localip_buf, NULL);
#if 0
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, algport_buf, localport_buf, NULL);
#else
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, viaport_buf, NULL /*localport_buf*/, NULL);
#endif
	}
	
	/* Contact	: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_IP, algip_buf, localip_buf, NULL);

	/* Contact	: Replace the local port with ALG port */
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_PORT, algport_buf, localport_buf, NULL);

	/* Referred-By:	Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_REFERRED_BY, algip_buf, localip_buf, NULL);
	
	/* Replaces:	Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_REFER_TO, algip_buf, localip_buf, NULL);

	return 1;
}



/******************************************************************
 *  Function Name	:  	out_res_initiated_inside_handler
 *  Description      	:  	This takes care of the necessary mangling
 *  				of a Response message going out for a call
 *  				initiated from inside.
 *  Input Values    	: 	struct ip_conntrack *ct,
 *  				struct sk_buff **pskb,
 *  				enum ip_conntrack_info ctinfo 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes		: 
 *********************************************************************/
static int out_res_initiated_inside_handler(struct ip_conntrack *ct,
					    struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		char *localip_buf,
		char *localport_buf,
		char *algip_buf,
		char *algport_buf,
		char *from_to_field_port)
{
	char temp[20];
	SDEBUG(" In (%s) !\n", __FUNCTION__);
	sprintf(temp,":%s",algport_buf);
	/* To		: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_TO, algip_buf, localip_buf, NULL);

	/* To Port	: Replace the local port with ALG port */
#if 0
	field_token_handler(ct, pskb, ctinfo, POS_TO_PORT, temp, from_to_field_port, NULL);
#endif

	/* Call-ID	: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_CALLID_IP, algip_buf, localip_buf, NULL);
	
	/* Contact	: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_IP, algip_buf, localip_buf, NULL);

	/* Contact	: Replace the local port with ALG port */
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_PORT, algport_buf, localport_buf, NULL);

	return 1;
}



/******************************************************************
 *  Function Name	:  	in_req_initiated_outside_handler
 *  Description      	:  	This takes care of the necessary mangling
 *  				of a Request message coming in for a call
 *  				initiated from outside.
 *  Input Values    	: 	struct ip_conntrack *ct,
 *  				struct sk_buff **pskb,
 *  				enum ip_conntrack_info ctinfo 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes		: 
 *********************************************************************/
static int in_req_initiated_outside_handler(struct ip_conntrack *ct,
					    struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		char *localip_buf,
		char *localport_buf,
		char *algip_buf,
		char *algport_buf,
		char *from_to_field_port,
		int local_call,
		int record_route,
		char from_localip[16],
		char from_localport[8])
{
	char temp[20];
	char from[128],from_user[128], other_lanip_buff[16];
	
	memset(from, 0x00, sizeof(from));
	memset(from_user, 0x00, sizeof(from_user));
	memset(other_lanip_buff, 0x00, sizeof(other_lanip_buff));

	SDEBUG(" In (%s) !\n", __FUNCTION__);
	sprintf(temp,":%s",localport_buf);
	/* To		: Replace the ALG address with local address */
	field_token_handler(ct, pskb, ctinfo, POS_TO, localip_buf, algip_buf, NULL);

	/* To Port	: Replace the ALG port with local port */
#if 0
	field_token_handler(ct, pskb, ctinfo, POS_TO_PORT, temp, from_to_field_port, NULL);
#endif
	
	SDEBUG("!!! in (%s): local_call=%d, record_route=%d\n",__FUNCTION__, local_call, record_route);
	if(local_call==1 && record_route==0)
	{
		SDEBUG("!!! local=[%s] alg[%s]\n",from_localip,algip_buf);
		field_token_handler(ct, pskb, ctinfo, POS_FROM, from_localip, algip_buf, NULL);
		field_token_handler(ct, pskb, ctinfo, POS_CALLID_IP, from_localip, algip_buf, NULL);
		field_token_handler(ct, pskb, ctinfo, POS_CONTACT_IP, from_localip, algip_buf, NULL);
		field_token_handler(ct, pskb, ctinfo, POS_CONTACT_PORT, from_localport, NULL, NULL);
	}
	else
		SDEBUG("!!! not replacing !!!!!!!\n");

		
	get_user_name(pskb, from);
	extract_user(from, from_user);
	if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
		if(record_route==1 && local_call==1)
		{
			int retval=0, off=0;
			char * endData;
			unsigned char *data;
			enum SipControlProtocol proto;
			struct sip_user_registration_data *tmp_reg_data=NULL;

			calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);
			do{	
				SDEBUG("(%s): data=%x\n",__FUNCTION__, data);
				retval = get_tcp_via_ip(pskb, other_lanip_buff, data, endData, proto, &off);
				SDEBUG("(%s): via udp ip=%s ret = %d\n",__FUNCTION__, other_lanip_buff,retval);
				tmp_reg_data = LIST_FIND(&sip_user_registration_list, user_ip_search, struct sip_user_registration_data*, from_user, other_lanip_buff);	
				if(tmp_reg_data == NULL)
					tmp_reg_data = LIST_FIND(&sip_user_registration_list, is_user_registered, struct sip_user_registration_data*, from);	

				if(tmp_reg_data!=NULL)
				{
					field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_IP, algip_buf, other_lanip_buff, &off);
					SDEBUG("(%s): Replacing %s -> %s\n",__FUNCTION__,algip_buf,other_lanip_buff);
				}
				data= data + off;	
			}while(retval>=0);
		}

	}
	else {
		/* If Record Route==1 and if call is Local, then find the VIA of caller and change to ALG IP:contact port */
		if(record_route==1 && local_call==1)
		{
			int retval=0, off=0;
			char * endData;
			unsigned char *data;
			enum SipControlProtocol proto;
			struct sip_user_registration_data *tmp_reg_data=NULL;

			calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);
			do{	
				SDEBUG("(%s): data=%x\n",__FUNCTION__, data);
				retval = get_udp_via_ip(pskb, other_lanip_buff, data, endData, proto, &off);
				SDEBUG("(%s): via udp ip=%s ret = %d\n",__FUNCTION__, other_lanip_buff,retval);
				tmp_reg_data = LIST_FIND(&sip_user_registration_list, user_ip_search, struct sip_user_registration_data*, from_user, other_lanip_buff);	
				if(tmp_reg_data == NULL)
					tmp_reg_data = LIST_FIND(&sip_user_registration_list, is_user_registered, struct sip_user_registration_data*, from);	

				if(tmp_reg_data!=NULL)
				{
					field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_IP, algip_buf, other_lanip_buff, &off);
					SDEBUG("(%s): Replacing %s -> %s\n",__FUNCTION__,algip_buf,other_lanip_buff);
				}
				data= data + off;	
			}while(retval>=0);
		}
	}

	/* Request-URI	: Replace the ALG address with local address*/
	field_token_handler(ct, pskb, ctinfo, POS_REQ_HEADER_IP , localip_buf, algip_buf, NULL);

	/* Request-URI	: Replace the ALG port with local port*/
	field_token_handler(ct, pskb, ctinfo, POS_REQ_HEADER_PORT , localport_buf, algport_buf, NULL);

	/* Proxy-Authorization : Replace the ALG address with local address */
	field_token_handler(ct, pskb, ctinfo, POS_PROXY_AUTH, localip_buf, algip_buf, NULL);

	return 1;
}




/******************************************************************
 *  Function Name	:  	in_res_initiated_outside_handler
 *  Description      	:  	This takes care of the necessary mangling
 *  				of a Response message coming in for a call
 *  				initiated from outside.
 *  Input Values    	: 	struct ip_conntrack *ct,
 *  				struct sk_buff **pskb,
 *  				enum ip_conntrack_info ctinfo 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes		: 
 *********************************************************************/
static int in_res_initiated_outside_handler(struct ip_conntrack *ct,
					    struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		char *localip_buf,
		char *localport_buf,
		char *algip_buf,
		char *algport_buf,
		char *from_to_field_port,
		int  swap_flag)
{
	char temp[20];
	char viaport_buf[8];
	char org_viaport_buf[8];

	sprintf(temp,":%s",localport_buf);
	SDEBUG(" In (%s) !\n", __FUNCTION__);
	/* FIXME: Also change FROM */
	/* From		: Replace the ALG address with local address */
	field_token_handler(ct, pskb, ctinfo, POS_FROM, localip_buf, algip_buf, NULL);

	/* From Port: Replace the ALG port with local port */
#if 0
	field_token_handler(ct, pskb, ctinfo, POS_FROM_PORT, temp, from_to_field_port, NULL);
#endif

	/* Via		: Replace the ALG address with local address *
	 * 		  New via will be ct's reply direction SRC
	 */
	if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_IP, localip_buf, algip_buf, NULL);
#if 0
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, localport_buf, algport_buf, NULL);
#else
		if (!swap_flag) {
			sprintf(viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.tcp.port));
			sprintf(org_viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.tcp.port));
		} else {
			sprintf(viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.tcp.port));
			sprintf(org_viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.tcp.port));
		}
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, org_viaport_buf, viaport_buf, NULL);
#endif
	} else {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_IP, localip_buf, algip_buf, NULL);
#if 0
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, localport_buf, algport_buf, NULL);
#else
		if (!swap_flag) {
			sprintf(viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port));
			sprintf(org_viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.udp.port));
		} else {
			sprintf(viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port));
			sprintf(org_viaport_buf, "%d", ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port));
		}
		IDEBUG("%s: VIA %s -> %s\n",__FUNCTION__,viaport_buf,org_viaport_buf,NULL);
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, org_viaport_buf, viaport_buf, NULL);
#endif
	}

	return 1;
}
/******************************************************************
 *  Function Name	:  	out_req_initiated_outside_handler
 *  Description      	:  	This takes care of the necessary mangling
 *  				of a Request message going out for a call
 *  				initiated from outside.
 *  Input Values    	: 	struct ip_conntrack *ct,
 *  				struct sk_buff **pskb,
 *  				enum ip_conntrack_info ctinfo 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes		: 
 *********************************************************************/
static int out_req_initiated_outside_handler(struct ip_conntrack *ct,
					     struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		char *localip_buf,
		char *localport_buf,
		char *algip_buf,
		char *algport_buf,
		char *from_to_field_port)
{
	char temp[20];
	char viaport_buf[8];
	int viaport_len=0;
	
	sprintf(temp,":%s",localport_buf);
	SDEBUG(" In (%s) !\n", __FUNCTION__);
	/* From		: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_FROM, algip_buf, localip_buf, NULL);
	
	/* From Port: Replace the local port with alg port */
#if 0
	field_token_handler(ct, pskb, ctinfo, POS_FROM_PORT, from_to_field_port, temp, NULL);
#endif

	/* Via		: Replace the local address with ALG address 
	 * 		Local port with port from transport header */

	get_src_port(*pskb, viaport_buf, &viaport_len); /* get via port from transport hdr */
	IDEBUG("(%s) VIA port --> %s (src port)\n", __FUNCTION__, viaport_buf);
	if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_IP, algip_buf, localip_buf, NULL);
#if 0
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, algport_buf, localport_buf, NULL);
#else
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, viaport_buf, NULL /*localport_buf*/, NULL);
#endif
	} else {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_IP, algip_buf, localip_buf, NULL);
#if 0
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, algport_buf, localport_buf, NULL);
#else
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, viaport_buf, NULL /*localport_buf*/, NULL);
#endif
	}
	
	/* Contact	: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_IP, algip_buf, localip_buf, NULL);

	/* Contact	: Replace the local port with ALG port */
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_PORT, algport_buf, localport_buf, NULL);

	/* Referred-By:	Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_REFERRED_BY, algip_buf, localip_buf, NULL);

	/* Replaces:	Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_REFER_TO, algip_buf, localip_buf, NULL);

	return 1;
}
/******************************************************************
 *  Function Name	:  	out_res_initiated_outside_handler
 *  Description    	:  	This takes care of the necessary mangling
 *  				of a Response message going out for a call
 *  				initiated from outside.
 *  Input Values    	: 	struct ip_conntrack *ct,
 *  				struct sk_buff **pskb,
 *  				enum ip_conntrack_info ctinfo 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes		: 
 *********************************************************************/
static int out_res_initiated_outside_handler(struct ip_conntrack *ct,
					     struct sk_buff **pskb,
		enum ip_conntrack_info ctinfo,
		char *localip_buf,
		char *localport_buf,
		char *algip_buf,
		char *algport_buf,
		char *from_to_field_port,
		int record_route,
		int local_call)
{
	char temp[20];
	sprintf(temp,":%s",localport_buf);
	SDEBUG(" In (%s) !\n", __FUNCTION__);
	/* To	: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_TO, algip_buf, localip_buf, NULL);

	/* To Port	: Replace the local port with ALG port */
#if 0
	field_token_handler(ct, pskb, ctinfo, POS_TO_PORT, from_to_field_port, temp, NULL);
#endif

#if 0
	/* Contact	: Replace the local address with ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_IP, algip_buf, localip_buf, NULL);

	/* Contact	: Replace the local port with ALG port */
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_PORT, algport_buf, localport_buf, NULL);
#else
	/* Contact	: Replace with ALG port/ip IFF 
	 * 			record route is not set or the call is not local
	 * 			in this case, the 200 OK for INVITE will be sent without contact hdr change,
	 * 			This ensures that the ACK can be rcvd locally from Ph1-Ph2
	 */
	if (!(record_route==0 && local_call==1))
	{
		field_token_handler(ct, pskb, ctinfo, POS_CONTACT_IP, algip_buf, localip_buf, NULL);	
		field_token_handler(ct, pskb, ctinfo, POS_CONTACT_PORT, algport_buf, localport_buf, NULL);
	}
#endif
	return 1;
}


static int get_expire_timer(struct sk_buff **pskb, int* expire_timer)
{
	unsigned char* data;
	char* endData;
	enum SipControlProtocol proto;
	int matchoff = 0, matchlen = 0;

	calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);

	if (ct_sip_get_info(data, endData - (char *)data, &matchoff, &matchlen, &ct_sip_hdrs[POS_CONTACT_EXPIRES]) > 0) {
		*expire_timer = simple_strtoul(data + matchoff, NULL, 10);
		//IDEBUG("sip_nat: Expires timer found %d\n",*expire_timer);
		return 1;
	} else if (ct_sip_get_info(data, endData - (char *)data, &matchoff, &matchlen, &ct_sip_hdrs[POS_EXPIRES]) > 0) {
		*expire_timer = simple_strtoul(data + matchoff, NULL, 10);
		//IDEBUG("sip_nat: Expires timer found %d\n",*expire_timer);
		return 1;
	}
	return 0;
}

static int get_port(struct sk_buff **pskb, int pos_hdr , int *port)
{
	char *data;
	char* endData;
	enum SipControlProtocol proto;
	int matchoff = 0, matchlen = 0;

	calculate_skbuff_data(pskb, &data, &endData, &proto);
	if (ct_sip_get_info(data, (int)(endData - data), &matchoff, &matchlen, &ct_sip_hdrs[pos_hdr]) > 0) {
		*port = simple_strtoul(data + matchoff, NULL, 10);
	} else {
		return -1;
	}
	return 0;
}

static void sip_reg_data_timed_out(unsigned long sip_reg_data)
{
	struct sip_user_registration_data *user_reg_data= (void*)sip_reg_data;
	struct sip_params sipparam;

	SDEBUG("sip_nat: Registration of ip addr %s is timed out. jiffies=%u\n",user_reg_data->original_ip_address, jiffies/HZ);
	WRITE_LOCK(&ip_sip_user_reg_rwlock);
	user_reg_data->change_flag = RAISE_FLAG;
	user_reg_data->typeofchange = DELETE_RULE;
	WRITE_UNLOCK(&ip_sip_user_reg_rwlock);

	/*Nirav - Delete the correposnding nat_helper for the destination port */
	sipparam.sip_src_port = 0;
	sipparam.sip_dst_port = user_reg_data->changed_contact_port;
	sipparam.proto = user_reg_data->proto;
	device_port_deregister(&sipparam, DYNAMIC_OPER);
	/* Also delete the conntrack_helper for the same port */
	sip_device_conn_port_deregister(&sipparam, DYNAMIC_OPER);

	//wake up the user process to change the rules in the iptables FIXME
	//synchronise_flag = RAISE_FLAG;
	wake_up_interruptible(&wait_queue_sipdaemon);
#if 0
	while(synchronise_flag!= DOWN_FLAG)
	{
		PDEBUG("Waiting for going ahead\n");
	}
	//delete the data strucure from the list
	WRITE_LOCK(&ip_sip_user_reg_rwlock);
	list_del(&user_reg_data->list);
	WRITE_UNLOCK(&ip_sip_user_reg_rwlock);
	IP_NF_ASSERT(!timer_pending(&user_reg_data->timer));

	kfree(user_reg_data);
		
#endif
}
static int get_udp_via_ip(struct sk_buff **pskb, char *via_ip, unsigned char *data, char *endData, enum SipControlProtocol proto, int *matchoff)
{
	//unsigned char* data;
	//char* endData;
	//enum SipControlProtocol proto;
	int matchlen = 0;

	if (ct_sip_get_info(data, endData - (char *)data, matchoff, &matchlen, &ct_sip_hdrs[POS_VIA_UDP_IP]) > 0) {
		memcpy(via_ip,data + *matchoff, matchlen );
		IDEBUG("sip_nat: UDP VIA IP field found %s\n",via_ip);
			return 1;
		}
		else
			return -1;
	return 0;	
}
#if 0	// Not used
static int get_udp_via_port(struct sk_buff **pskb, int *via_port)
{
	unsigned char* data;
	char* endData;
	enum SipControlProtocol proto;
	int matchoff = 0, matchlen = 0;
		
	calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);

	if (ct_sip_get_info(data, endData - (char *)data, &matchoff, &matchlen, &ct_sip_hdrs[POS_VIA_UDP_PORT]) > 0) {
		*via_port = simple_strtoul(data + matchoff, NULL, 10);
		IDEBUG("sip_nat: UDP VIA IP field found %d\n",via_port);
		return 1;
	}
	else
		return -1;
	return 0;	
}
#endif
static int get_tcp_via_ip(struct sk_buff **pskb, char *via_ip, unsigned char *data, char *endData, enum SipControlProtocol proto, int *matchoff)
{
	//unsigned char* data;
	//char* endData;
	//enum SipControlProtocol proto;
	int matchlen = 0;

	//calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);

	if (ct_sip_get_info(data, endData - (char *)data, matchoff, &matchlen, &ct_sip_hdrs[POS_VIA_TCP_IP]) > 0) {
		memcpy(via_ip,data + *matchoff, matchlen );
		IDEBUG("sip_nat: TCP VIA IP field found %s\n",via_ip);
		return 1;
	}
	else
		return -1;
	return 0;	
}
#if 0
static int get_tcp_via_port(struct sk_buff **pskb, int *via_port)
{
	unsigned char* data;
	char* endData;
	enum SipControlProtocol proto;
	int matchoff = 0, matchlen = 0;

	calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);

	if (ct_sip_get_info(data, endData - (char *)data, &matchoff, &matchlen, &ct_sip_hdrs[POS_VIA_TCP_PORT]) > 0) {
		*via_port = simple_strtoul(data + matchoff, NULL, 10);
		IDEBUG("sip_nat: TCP VIA IP field found %d\n",via_port);
		return 1;
	}
	else
		return -1;
	return 0;	
}
#endif

/* Sumedh - extract TO field from SIP hdr */
static int get_to_user_name(struct sk_buff **pskb, char *user_name)
{
	unsigned char* data;
	char* endData;
	enum SipControlProtocol proto;
	int matchoff = 0, matchlen = 0;

	calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);

	if (ct_sip_get_info(data, endData - (char *)data, &matchoff, &matchlen, &ct_sip_hdrs[POS_TO_SIP]) > 0) {
		memcpy(user_name,data + matchoff, matchlen );
		IDEBUG("sip_nat: TO field found %s\n",user_name);
		return 1;
	} else if (ct_sip_get_info(data, endData - (char *)data, &matchoff, &matchlen, &ct_sip_hdrs[POS_TO_SIPS]) > 0) {
		memcpy(user_name,data + matchoff, matchlen );
		IDEBUG("sip_nat: TO field found %s\n",user_name);
		return 1;
	}
	return 0;
}
/* Sumedh - extract FROM field from SIP hdr */
static int get_user_name(struct sk_buff **pskb, char *user_name)
{
	unsigned char* data;
	char* endData;
	enum SipControlProtocol proto;
	int matchoff = 0, matchlen = 0;

	calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);

	if (ct_sip_get_info(data, endData - (char *)data, &matchoff, &matchlen, &ct_sip_hdrs[POS_FROM_SIP]) > 0) {
		memcpy(user_name,data + matchoff, matchlen );
		IDEBUG("sip_nat: From field found %s\n",user_name);
		return 1;
	} else if (ct_sip_get_info(data, endData - (char *)data, &matchoff, &matchlen, &ct_sip_hdrs[POS_FROM_SIPS]) > 0) {
		memcpy(user_name,data + matchoff, matchlen );
		IDEBUG("sip_nat: From field found %s\n",user_name);
		return 1;
	}
	return 0;
}
/* Sumedh - Given name in format abc@IP format, return user as 'abc' */
static int extract_user(char *fullname, char *name)
{
	int i=0;
	for(;fullname[i]!='@';i++)
	{
		name[i]=fullname[i];
		//printk("i=%d name[i]=%c\n",i, name[i]);
	}
	name[i]=0;
}
/* Sumedh - Given name in format abc@IP format, return ip as 'IP' */
static int extract_ip(char *fullname, char *name)
{
	int i=0, j=0;
	for(;fullname[i]!='@';i++);
	i++;//skip '@'
	for(;i<strlen(fullname);i++)
	{
		name[j++]=fullname[i];
		//printk("i=%d j=%d name[j]=%c\n",i, j-1,name[j-1]);
	}
	name[j]=0;
}
static int register_message_handler(struct ip_conntrack *ct,
						struct sk_buff **pskb,
						enum ip_conntrack_info ctinfo,
		enum SipControlProtocol proto,
		char *local_address,
		char *alg_address,
		int *changed_via_port)
{
	int orig_port = 0;
	int changed_contact_port = 0;
		int expire_timer=0;
	struct sip_params sipparam;
	char localport_buf[8];
	char algport_buf[8];
	char port_buffer[8];
	int port_len=0;
	int via_port=0;

	struct sip_user_registration_data* sip_reg_data=NULL;

		/*Internal port should be replaced with the ALG port, which is
	  src port of the transport header */
	get_src_port_num(*pskb, &via_port);
	IDEBUG("sip_nat: (register_message_handler) Via Port to be replaced by [%d] \n", via_port);

		/* Find out the expired timer on the Register message. */
	if(get_expire_timer(pskb, &expire_timer) < 0) {
				//expire timer is not present
		return 0;
		}
		
	/* Get the port number */
	get_port(pskb, POS_CONTACT_PORT, &orig_port);

	/* * If port is NULL, use the defaukt SIP_PORT */
	if (orig_port == 0)
		orig_port = SIP_PORT;

	WRITE_LOCK(&ip_sip_user_reg_rwlock);

	/* Find out if there is an entry into the registry list for this local ip and port number.*/
	sip_reg_data = LIST_FIND(&sip_user_registration_list, get_user_registration_data, struct sip_user_registration_data*, local_address, orig_port, NULL, 0);
		//In case it does not
	if(sip_reg_data == NULL) {
		IDEBUG("sip_nat: Registration message - New data found with expire_timer = %d\n",expire_timer);
		if (expire_timer > 0) {
#if 0
			changed_contact_port = orig_port + REG_PORT_OFFSET;
#else
			/* Make some mapping so that contact ports are assigned a new range */
			changed_contact_port = orig_port % 1024  + REG_PORT_OFFSET;
#endif
			/* Get a new unique port no for this new entry */
#if 1 
			do {
				sip_reg_data = LIST_FIND(&sip_user_registration_list, get_user_registration_data, struct sip_user_registration_data*, alg_address, changed_contact_port, NULL, 0);
				if (sip_reg_data != NULL) 
					changed_contact_port += 2;
			}while(sip_reg_data != NULL);
#endif
			IDEBUG("sip_nat: Got a unique ipaddr=%s and port=%d and contact port=%d\n",alg_address,via_port,changed_contact_port);

			sip_reg_data = (struct sip_user_registration_data*)kmalloc(sizeof(struct sip_user_registration_data), GFP_ATOMIC);
			if(!sip_reg_data) {
				/* No memory*/
				PDEBUG("OOPs ... No memory\n");
				WRITE_UNLOCK(&ip_sip_user_reg_rwlock);
				return -1;
			}

			//sanitise the memory chunk
			memset(sip_reg_data, 0, sizeof(*sip_reg_data));
			INIT_LIST_HEAD(&sip_reg_data->list);

			//store the identification parameters
			sip_reg_data->proto = proto;
			memcpy(sip_reg_data->original_ip_address, local_address, strlen(local_address));
			sip_reg_data->original_contact_port = orig_port;

			//store the data
			memcpy(sip_reg_data->changed_ip_address, alg_address, strlen(alg_address));
#if 0
			sip_reg_data->changed_port = changed_port;
#else
			sip_reg_data->changed_contact_port = changed_contact_port;
#endif

			//Store the user name
			get_user_name(pskb,sip_reg_data->user_name);
			extract_user(sip_reg_data->user_name,sip_reg_data->user);

			//set the timer
			init_timer(&sip_reg_data->timeout);
			sip_reg_data->timeout.data = (unsigned long)sip_reg_data;
			sip_reg_data->timeout.function = sip_reg_data_timed_out;

			//registration data will live IFX_BUFFER_TIME sec
			//This is temporary time. The actual will be updated from its reponse
			sip_reg_data->timeout.expires = jiffies+ (IFX_BUFFER_TIME)*HZ;	
			sip_reg_data->timer_value = expire_timer;

			add_timer(&sip_reg_data->timeout);
			sip_reg_data->timer_set_flag = 1;
			/* Add to the global list */
			list_prepend(&sip_user_registration_list, &sip_reg_data->list);

			sip_reg_data->change_flag = RAISE_FLAG;
			sip_reg_data->typeofchange = ADD_RULE;

			/* Now add the nat_helper for the destination port */
			sipparam.sip_src_port = 0;
#if 0
			sipparam.sip_dst_port = sip_reg_data->changed_port;
#else
			sipparam.sip_dst_port = sip_reg_data->changed_contact_port;
#endif

			sipparam.proto = sip_reg_data->proto;
			device_port_register(&sipparam, DYNAMIC_OPER);
			/* Also add the conntrack_helper for the same port */
			sip_device_conn_port_register(&sipparam, DYNAMIC_OPER);

			wake_up_interruptible(&wait_queue_sipdaemon);	
			/*
			   Create a registration data entry in an exclusive registry list
			   Registration data will be identified by the Call-ID and CSeq.
			   The original port and changed port, both will be stored.
			   Create a timer according to the (Expiry timer+IFX_BUFFER_TIMEsec) in the register message.
			   Enter the timer in the timer list.

			   The timer handler will wake up the sleeping queue of the user daemon
			   and send a message to deregister the registered port(i.e remove the iptables rule
			   for DNAT).
			 */
			sprintf(localport_buf,"%d",sip_reg_data->original_contact_port);
			sprintf(algport_buf,"%d",sip_reg_data->changed_contact_port);

#if 0
					{
				/* Change the packets source port so as to reflect the newly added SNAT rule */
				struct ip_nat_info *info = &ct->nat.info;
				struct ip_conntrack_tuple new_tuple,orig_tp;

				new_tuple.src.ip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
				new_tuple.src.u.udp.port = sip_reg_data->changed_port;
				info->manips[info->num_manips] =
					((struct ip_nat_info_manip)
					 { IP_CT_DIR_ORIGINAL, NF_IP_POST_ROUTING,
					 IP_NAT_MANIP_SRC, new_tuple.src });

				orig_tp.src.ip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
				orig_tp.src.u.udp.port = sip_reg_data->original_port;
				/* In the reverse direction, a destination manip. */
				info->manips[info->num_manips] =
					((struct ip_nat_info_manip)
					 { IP_CT_DIR_REPLY, NF_IP_PRE_ROUTING,
					 IP_NAT_MANIP_DST, orig_tp.src });

				ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port = sip_reg_data->changed_port;
#if 1
				find_nat_proto((*pskb)->nh.iph->protocol)->manip_pkt((*pskb)->nh.iph, (*pskb)->len, &(info->manips[info->num_manips].manip), IP_NAT_MANIP_SRC);
#endif

					}
#endif

		} else {
			/* expire_timer == 0
			 * This may be deregistration message - for which we have not seen registration message
			 * Change VIA to match network header but otherwise ignore remaining details */
			IDEBUG("sip_nat: Registration message - This is De-Registration message. Will be handled in response\n");
				}
	} else {
		sprintf(localport_buf,"%d",sip_reg_data->original_contact_port);
		sprintf(algport_buf,"%d", sip_reg_data->changed_contact_port);

		/* sip_reg_data != NULL */
		if (expire_timer == 0) {
			IDEBUG("sip_nat: Registration message - This is De-Registration message. Will be handled in response\n");
		} else {
					// Restart the timer of registration data.
			IDEBUG("sip_nat: Registration message - Renewal of existing entry will be handled in reponse\n");
		}
	}
	WRITE_UNLOCK(&ip_sip_user_reg_rwlock);

	/* Via	:Replace the local address with the ALG address */
	if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_IP, alg_address, NULL, NULL);
	} else {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_IP, alg_address, NULL, NULL);
					}

	/* Contact :Replace the local address with the ALG address */
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_IP, alg_address, NULL, NULL);

	if(sip_reg_data == NULL)
		IDEBUG("sip_nat_help: As sip_reg_data is NULL, not replacing VIA and Contact ports\n");
	
	/* Contact :Replace the local port with the ALG port */
	if (sip_reg_data != NULL) {
		char contactport_buf[8];
		sprintf(contactport_buf,"%d",changed_contact_port);
		IDEBUG("sip_nat: Replacing in Contact header, local port [%s] with Contact port [%d]\n",localport_buf,changed_contact_port);
		if(field_token_handler(ct, pskb, ctinfo, POS_CONTACT_PORT, contactport_buf, localport_buf, NULL) < 0) {
			char temp_contactport_buf[8];
			sprintf(temp_contactport_buf,":%s",contactport_buf);
			field_token_handler(ct, pskb, ctinfo, POS_CONTACT_PORT_SPC, temp_contactport_buf, NULL, NULL);
					}
					}

	/* Via	:Replace the local port with the ALG port */
	sprintf(port_buffer, "%d", via_port);
	if (sip_reg_data != NULL) {
		IDEBUG("sip_nat: Replacing in Via header, local port [%s] with VIA port [%s]\n",localport_buf,port_buffer);
		if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
			field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, port_buffer, localport_buf, NULL);
		} else {
			field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, port_buffer, localport_buf, NULL);
				}
		*changed_via_port = via_port;
			}
	else {
		/* Sumedh - Always assign what we see in conntrack */
		IDEBUG("sip_nat: Reg Data is NULL, yet Replacing in Via header, local port [%s] with VIA port [%s]\n",localport_buf,port_buffer);
		if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
			field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, port_buffer, NULL, NULL);
		} else {
			field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, port_buffer, NULL, NULL);
		}
		*changed_via_port = via_port;
		}

		return 1;
}

static int register_message_reply_handler(struct ip_conntrack *ct,
						struct sk_buff **pskb,
						enum ip_conntrack_info ctinfo,
		enum SipControlProtocol proto,
		char *local_address,
		int local_port,
		char *alg_address)
{
	int expire_timer=0;
	struct sip_user_registration_data* sip_reg_data=NULL;
	int port = 0;
	char localport_buf[8];
	char algport_buf[8];
	struct sip_params sipparam;
	char user_name[128];
	int reg_data_found = 0;
	char port_buffer[8];
	int port_len=0;
	int via_port=0;

	memset(user_name,0x00,sizeof(user_name));
	get_user_name(pskb, user_name);
	
#if 1
	/* Get the port number */
	if (get_port(pskb, POS_CONTACT_PORT, &port) >= 0) 
#else
	get_dest_port(pskb, port_buffer, &port_len);
	PDEBUG("sip_nat: (register_message_reply_handler) Dest Port is [%s]\n", port_buffer);
	via_port = simple_strtoul(port_buffer, NULL, 10);
	if (via_port >=0)
#endif
	{
	/* Check the registration data for this message. In case there is an entry
	   allow or else drop.*/
		WRITE_LOCK(&ip_sip_user_reg_rwlock); 
#if 1
		sip_reg_data = LIST_FIND(&sip_user_registration_list, get_user_registration_data,
				struct sip_user_registration_data*, alg_address, port, user_name, 0);
#else
		sip_reg_data = LIST_FIND(&sip_user_registration_list, get_user_registration_data,
				struct sip_user_registration_data*, alg_address, via_port, user_name, 0);
#endif

		if(sip_reg_data != NULL) {
			/* Find out the expired timer on the Register message. */
			if(get_expire_timer(pskb, &expire_timer) > 0) {
			PDEBUG("Changing the timer value\n");
				SDEBUG("sip_nat: register_message_reply_handler changing expire_timer=%d for orig_ip=%s and orig_port=%d\n",expire_timer,sip_reg_data->original_ip_address,sip_reg_data->original_contact_port);
				sip_reg_data->timer_value = expire_timer;
			// Restart the timer of registration data.
				mod_timer(&sip_reg_data->timeout,  jiffies + (sip_reg_data->timer_value + IFX_BUFFER_TIME) * HZ);
		}
			sprintf(localport_buf,"%d",sip_reg_data->original_contact_port);
			sprintf(algport_buf,"%d",sip_reg_data->changed_contact_port);
			reg_data_found = 1;
		}
		WRITE_UNLOCK(&ip_sip_user_reg_rwlock);
	} else {
		/*
		if (get_port(pskb, POS_VIA_UDP_PORT, &port) < 0) 
			get_port(pskb, POS_VIA_TCP_PORT, &port);
			*/
		/* Check the registration data for this message. In case there is an entry
		   allow or else drop.*/
		WRITE_LOCK(&ip_sip_user_reg_rwlock); 
#if 0
		sip_reg_data = LIST_FIND(&sip_user_registration_list, get_user_registration_data,
				struct sip_user_registration_data*, alg_address, port, user_name, 0);
#else
		do {
			sip_reg_data = LIST_FIND(&sip_user_registration_list, get_user_registration_data,
					struct sip_user_registration_data*, (local_port > 0 ? local_address : NULL), local_port, user_name, 0);
#endif
			if(sip_reg_data != NULL) {
				if(!(get_expire_timer(pskb, &expire_timer) > 0 && expire_timer>0)) {

					IDEBUG("sip_nat: register_message_reply_handler - This is Deregistration message\n");
					//Raise the flag for changes and wake up the 
					//daemon. delete the timer from the data before that
					if(sip_reg_data->timer_set_flag == 1) {
						IDEBUG("sip_nat: Timer exist and hence deleting it\n");
						del_timer(&sip_reg_data->timeout);
						IP_NF_ASSERT(!timer_pending(&sip_reg_data->timeout));
						sip_reg_data->timer_set_flag = 0;
	}
					sip_reg_data->change_flag = RAISE_FLAG;
					sip_reg_data->typeofchange = DELETE_RULE;
					/*Nirav - Delete the correposnding nat_helper for the destination port */
					sipparam.sip_src_port = 0;
#if 0
					sipparam.sip_dst_port = sip_reg_data->changed_port;
#else
					sipparam.sip_dst_port = sip_reg_data->changed_contact_port;
#endif
					sipparam.proto = sip_reg_data->proto;
					device_port_deregister(&sipparam, DYNAMIC_OPER);
					/* Also delete the conntrack_helper for the same port */
					sip_device_conn_port_deregister(&sipparam, DYNAMIC_OPER);
	   
					wake_up_interruptible(&wait_queue_sipdaemon);	

					sprintf(localport_buf,"%d",sip_reg_data->original_contact_port);
					sprintf(algport_buf,"%d",sip_reg_data->changed_contact_port);
					reg_data_found = 1;
				}
			}
		} while(sip_reg_data);
		WRITE_UNLOCK(&ip_sip_user_reg_rwlock);
	}

	/* Via : Replace the ALg address with the internal address*/
	if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_IP, local_address, NULL, NULL);
	} else {
		field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_IP, local_address, NULL, NULL);
	}

	/*Contact	:Replace the ALG address with the internal address*/
	field_token_handler(ct, pskb, ctinfo, POS_CONTACT_IP, local_address, NULL, NULL);

	/*Contact	:Replace the ALG port with the internal port */
	if (reg_data_found)
		field_token_handler(ct, pskb, ctinfo, POS_CONTACT_PORT, localport_buf, algport_buf, NULL);

	/* Via	:Replace the local port with the ALG port */
#if 0 /* [ Could be a DE- REGISTER without any registration contact */
	if (reg_data_found)
#endif
	{   
		get_dest_port(pskb, port_buffer, &port_len);
		PDEBUG("sip_nat: (register_message_reply_handler) Dest Port is [%s]\n", port_buffer);
		if ((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
			field_token_handler(ct, pskb, ctinfo, POS_VIA_TCP_PORT, port_buffer, NULL, NULL);
		} else {
			field_token_handler(ct, pskb, ctinfo, POS_VIA_UDP_PORT, port_buffer, NULL, NULL);
		}
	}
	return 1;
}

/******************************************************************
 *  Function Name	:  	sipdata_timed_out
 *  Description      	:  	Timer interrupt handler. This function 
 *  				deletes the sip_data with which the timer
 *  				was associated.
 *  Input Values    	: 	unsigned long call_data 
 *  Output Values 	:  
 *  Return Value   	:	void
 *  Notes		: 
 *********************************************************************/
static void sipdata_timed_out(unsigned long call_data)
{
	struct sip_call_data *sip_data = (void*)call_data;
	WRITE_LOCK(&ip_sip_rwlock);
	list_del(&sip_data->list);
	WRITE_UNLOCK(&ip_sip_rwlock);
	IP_NF_ASSERT(!timer_pending(&sip_data->timeout));
	IDEBUG("sip_nat: Freeing up sip_data - Timer expired for Call-Id [%s]. Now jiffies=%u\n",sip_data->original_call_id, jiffies/HZ);
	kfree(sip_data);
}

/******************************************************************
 *  Function Name	:  	start_timer
 *  Description      :  	allocates a timer for the sip-data and
 *  						starts the timer.	
 *  Input Values    	: 	struct sip_call_data* call_data 
 *  Output Values 	:  
 *  Return Value   	:	int
 *  Notes			: 
 *********************************************************************/
static int start_timer(struct sip_call_data* call_data)
{
	if(call_data->timeout.data == (unsigned long)call_data) {
		/* timer has already been initialized */
		IDEBUG("Timer already been initialized\n");
		return 1;
	}
	init_timer(&call_data->timeout);
	call_data->timeout.data = (unsigned long)call_data;
	call_data->timeout.function = sipdata_timed_out;
	call_data->timeout.expires = jiffies + IFX_BUFFER_TIME * HZ; /* IFX_BUFFER_TIME sec */
	SDEBUG("sip_nat: Adding timer for the Call-Id [%s] expiry=%u\n",call_data->original_call_id, call_data->timeout.expires);
	add_timer(&call_data->timeout);
	call_data->timer_set_flag = 1;
	return 1;
}
	
static int sip_fixup_content_length(struct sk_buff** pskb,
				struct ip_conntrack *ct,
				enum ip_conntrack_info ctinfo)
{
	char* data;
	char* endData;
	enum SipControlProtocol proto;
	char rep_buffer[10];
	int rep_len = 0, match_len = 0, offset = 0;
	int datalen = 0;
	char *tmp = NULL;

	calculate_skbuff_data(pskb, (char**)&data, (char**)&endData, &proto);

	if (ct_sip_get_info(data, (int)(endData - data), &offset, &match_len, &ct_sip_hdrs[POS_CONTENT]) < 0) 
		return 1;
	if (simple_strtoul(data + offset, NULL, 10) == 0)
		return 1;

	tmp = (char*)sip_strstr(data, "\r\n\r\n",endData,END);
	if (tmp == NULL)
		return 1;

	datalen = (endData - tmp + 1);
	memset(rep_buffer,0x00,sizeof(rep_buffer));
	sprintf(rep_buffer, "%d",datalen);
	rep_len = strlen(rep_buffer);

	if(proto == UDP) {
		sip_mangle_udp_data(pskb, ct, ctinfo, 
				offset, match_len, rep_buffer,
				rep_len);
	} else if(proto == TCP) {
		PDEBUG("Calling TCP mangle\n");
		sip_mangle_tcp_data(pskb, ct, ctinfo, 
				offset, match_len, rep_buffer,
				rep_len);
	}

	return 1;
}

/******************************************************************
 *  Function Name	:  	sip_nat_help
 *  Description      	:  	This is the basic nat helper function which
 *  				gets called when master conntrack has been
 *  				hit. This function will be called for all 
 *  				the SIP control messages.
 *  Input Values    	:  	struct ip_conntrack *ct,
 *  				struct ip_conntrack_expect *exp,
 *  				struct ip_nat_info *info,
 *  				enum ip_conntrack_info ctinfo,
 *  				unsigned int hooknum,
 *  				struct sk_buff **pskb
 *  Output Values 	:  
 *  Return Value   	:	unsigned int
 *  Notes		: 
 *********************************************************************/
unsigned int sip_nat_help(struct ip_conntrack *ct,
		struct ip_conntrack_expect *exp,
		struct ip_nat_info *info,
		enum ip_conntrack_info ctinfo,
		unsigned int hooknum,
		struct sk_buff **pskb)
{
	unsigned char* data;
	char* endData;
	enum SipControlProtocol proto;
	int dir, iRet;

	enum ip_nat_manip_type manip_type;
	char call_id[CALL_ID_LENGTH];
	int datalen;
	enum SIP_MESSAGES message_type;
	enum SIP_MESSAGES req_msg_of_res = 0;
	enum MESSAGE_DIR cur_message = 0;
	char localip_buf[16];
	char algip_buf[16];
	char localport_buf[8];
	char algport_buf[8];
	char from_localip[16];
	char from_localport[8];
	int temp_algport = 0;
	u_int32_t ct_org_src_ip=0, ct_reply_src_ip=0;
	int record_route=0, local_call=0;
		
	struct sip_call_data *call_data = NULL, *new_call_data = NULL;
	//struct tcphdr* tcph;
	int matchoff = 0, matchlen = 0;

	int oldudplen = 0;
	int swap_flag = 0; /* This flag indicates that orginal ct for this SIP
			      Call is reverse the current ct for this pkt */

	int tmp_sport=0; char tmp_dport_s[8]; int tmp_dport_l=0;

	/* Until there's been traffic both ways, don't look in packets. */
	if ( (*pskb)->nh.iph->protocol == IPPROTO_TCP 
			&& ctinfo != IP_CT_ESTABLISHED
			&& ctinfo != IP_CT_ESTABLISHED+IP_CT_IS_REPLY) {
		return NF_ACCEPT;
	}


	memset(from_localip,0x00,sizeof(from_localip));
	memset(from_localport,0x00,sizeof(from_localport));
	/* Only mangle things once: original direction in POST_ROUTING
	   and reply direction on PRE_ROUTING. */
	dir = CTINFO2DIR(ctinfo);
	
	get_src_port_num(*pskb,&tmp_sport);
	get_dest_port(pskb, tmp_dport_s, &tmp_dport_l);
	IDEBUG("sip_nat_help:: Pkt-> src_ip=%u.%u.%u.%u, src_port = %d, dst_ip=%u.%u.%u.%u,dst_port = %s\n",
			NIPQUAD(ntohl((*pskb)->nh.iph->saddr)), tmp_sport,
			NIPQUAD(ntohl((*pskb)->nh.iph->daddr)), tmp_dport_s);
	IDEBUG("sip_nat_help:: Conntrack-> src_ip=%u.%u.%u.%u, src_port = %d, dst_ip=%u.%u.%u.%u,dst_port = %d\n",
			NIPQUAD(ntohl(ct->tuplehash[dir].tuple.src.ip)), ntohl(ct->tuplehash[dir].tuple.src.u.udp.port),
			NIPQUAD(ntohl(ct->tuplehash[dir].tuple.dst.ip)), ntohl(ct->tuplehash[dir].tuple.src.u.udp.port));

	if (!((hooknum == NF_IP_POST_ROUTING && dir == IP_CT_DIR_ORIGINAL)
				|| (hooknum == NF_IP_PRE_ROUTING && dir == IP_CT_DIR_REPLY))) {
		IDEBUG("sip_nat: *Not touching dir %s at hook %s\n",
			dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
				hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
			: hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
			: hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "???");
		//return NF_ACCEPT;//Sumedh 710061
		/* Instead of this, a check will be done later in code
		 * We will mangle IN->OUT pkts in POSTROUTING, and OUT->IN in PREROUTING hook
		 */
	} else {
		IDEBUG("sip_nat: Processing the packet in dir %s from %s hook\n",
				dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
				hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
				: hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
				: hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "???");
		if (exp) {
			IDEBUG("sip_nat: expectation tuple : %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u\n",NIPQUAD(exp->tuple.src.ip),ntohl(exp->tuple.src.u.udp.port),NIPQUAD(exp->tuple.dst.ip),ntohl(exp->tuple.dst.u.udp.port));
			IDEBUG("sip_nat: expectation mask  : %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u\n",NIPQUAD(exp->mask.src.ip),ntohl(exp->mask.src.u.udp.port),NIPQUAD(exp->mask.dst.ip),ntohl(exp->mask.dst.u.udp.port));
	}
	}

	if ((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip == ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip) &&
			(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip == ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip)) {
		IDEBUG("sip_nat: original and reply direction tuples are unchanged. No mangaling is required for this packet\n");
		return NF_ACCEPT;
	}

	if((*pskb)->nh.iph->protocol == IPPROTO_UDP) {
		PDEBUG("Transport is UDP\n");
		struct iphdr *iph = (*pskb)->nh.iph;
		const struct udphdr *udph = (const struct udphdr *)((u_int32_t *)iph + 5);
		oldudplen = ntohl(udph->len);
	} else if((*pskb)->nh.iph->protocol == IPPROTO_TCP) {
		PDEBUG("Transport is TCP\n");
	} else {
		IDEBUG("sip_nat: Protocol not understood..dropping\n");
			return NF_DROP;
		}

	/* Collect all the data relating to the packet below */
	calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);
	datalen = ((char*)endData - (char*)data) +1 ;

	if(datalen < 16) {
		//If there is no data .. what to mangle
		IDEBUG("sip_nat : No data.. Returning\n");
		return NF_ACCEPT;
		}

	READ_LOCK(&ip_nat_lock);
	manip_type = HOOK2MANIP(hooknum);	
	READ_UNLOCK(&ip_nat_lock);

	LOCK_BH(&ip_sip_lock);

	/* Collect the Method */
	message_type = get_sip_method(data);
	if(message_type == MSG_UNKNOWN) {
		IDEBUG("sip_nat: Error ....Message Type not found\n");
			/* XXX: Should we do NF_DROP here */
			UNLOCK_BH(&ip_sip_lock);
			return NF_ACCEPT;
		}

	/* Collect the call id */
	if (ct_sip_get_info(data, (int)(endData - (char *)data), &matchoff, &matchlen, &ct_sip_hdrs[POS_CALLID]) > 0) {
		memset(call_id, 0x00, sizeof(call_id));
		memcpy(call_id, data + matchoff, matchlen);
		datalen = matchlen;
		//IDEBUG("sip_nat : Call ID found [%s]\n",call_id);
		}

	/* Get ALG's dst port */
	if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip == ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip) {
		if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) 
			temp_algport = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port;
		else
			temp_algport = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.tcp.port;
	} else {
		if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) 
			temp_algport = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port;
		else
			temp_algport = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.tcp.port;
	}
	/* Pass this CTs ORG src ip and REPLY src ip. 1 of them will match calldata's local ip if existing */
	ct_org_src_ip = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip);
	ct_reply_src_ip = ntohl(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip);

	READ_LOCK(&ip_sip_rwlock);
	//call_data = LIST_FIND( &sip_call_data, get_call_data, struct sip_call_data*, call_id, strlen(call_id), (message_type == MSG_REGISTER ? 0 : temp_algport, ));
	call_data = LIST_FIND( &sip_call_data, get_call_data, struct sip_call_data*, call_id, strlen(call_id),ct_org_src_ip, ct_reply_src_ip);

	READ_UNLOCK(&ip_sip_rwlock);

	//IDEBUG("sip_nat: Message\n[%s]\n",data);
	if (call_data == NULL) {
		int temp_port = 0;
		char temp_ip[16];
		struct sip_user_registration_data* sip_reg_data=NULL;

							/* Add a new entry for the new connection */
		new_call_data = (struct sip_call_data*) kmalloc(sizeof(struct sip_call_data), GFP_ATOMIC);
		if(!new_call_data) {
									/* No memory ..drop the packet */
									PDEBUG(" OOPs ... No memory\n");
									UNLOCK_BH(&ip_sip_lock);
									return NF_DROP;
							}

							/* sanitise the memory chunk */
		memset(new_call_data, 0, sizeof(struct sip_call_data));

		INIT_LIST_HEAD(&new_call_data->list);
		memcpy(new_call_data->original_call_id, call_id, strlen(call_id) + 1);

		/* Set the key_algport */
		new_call_data->key_algport = temp_algport;

		IDEBUG("sip_nat: New call_id [%s] with algport %d found.\n",new_call_data->original_call_id, new_call_data->key_algport);
		/* Depending upon the src and dst match, conclude the call initiation direction */	
		if ((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip == ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip) &&
				(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip != ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip)) {
			new_call_data->call_dir = INSIDE_INITIATION;
			IDEBUG("sip_nat: This is new INSIDE_INITIATION\n");
		} else if ((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip == ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip) &&
				(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip != ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip)) {
			new_call_data->call_dir = OUTSIDE_INITIATION;
			IDEBUG("sip_nat: This is new OUTSIDE_INITIATION\n");
					}

		/* If the call has initiated in the reply direction then reverse the actual initiataion direction */	
		if (dir == IP_CT_DIR_REPLY){
			new_call_data->call_dir = 1 - new_call_data->call_dir;
			IDEBUG("sip_nat: As the conntrack is existing the call_dir is changed to %s\n",(new_call_data->call_dir == INSIDE_INITIATION ? "INSIDE_INITIATION" : "OUTSIDE_INITIATION"));
		}

		/* Store the local and alg ip address */
		if ((new_call_data->call_dir == INSIDE_INITIATION && dir == IP_CT_DIR_ORIGINAL)
				|| (new_call_data->call_dir == OUTSIDE_INITIATION && dir == IP_CT_DIR_REPLY)) {
			new_call_data->localip = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip);
			new_call_data->algip = ntohl(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip);
		} else if ((new_call_data->call_dir == INSIDE_INITIATION && dir == IP_CT_DIR_REPLY)
				|| (new_call_data->call_dir == OUTSIDE_INITIATION && dir == IP_CT_DIR_ORIGINAL)) {
			new_call_data->localip = ntohl(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip);
			new_call_data->algip = ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip);
			}

		/* Get the local port and find out the corresponding alg port registered for it */
		if (new_call_data->call_dir == INSIDE_INITIATION){
			/* Here, we will get local ip and local port */
			get_port(pskb, POS_CONTACT_PORT, &temp_port);
			sprintf(temp_ip,"%u.%u.%u.%u",NIPQUAD(new_call_data->localip));
			IDEBUG("\nCHECK REGISTERRATION temp_ip=%s, temp_port=%d...local\n",temp_ip, temp_port);
		} else {
			/* Here, we will get alg ip and alg port */
			get_port(pskb, POS_REQ_HEADER_PORT, &temp_port);
			sprintf(temp_ip,"%u.%u.%u.%u",NIPQUAD(new_call_data->algip));
			IDEBUG("\nCHECK REGISTERRATION temp_ip=%s, temp_port=%d...alg\n",temp_ip, temp_port);
							}

		/* Find out if there is an entry into the registry list for ip and port number.*/
		sip_reg_data = LIST_FIND(&sip_user_registration_list, get_user_registration_data, struct sip_user_registration_data*, temp_ip, temp_port, NULL, 0);
		if (sip_reg_data != NULL) {
			new_call_data->localport = sip_reg_data->original_contact_port;
			new_call_data->algport = sip_reg_data->changed_contact_port;

#if 0
			if (new_call_data->call_dir == INSIDE_INITIATION) {
				if (ct_sip_get_info(data, (int)(endData - (char *)data), &matchoff, &matchlen, &ct_sip_hdrs[POS_FROM_PORT]) > 0) {
					if (matchlen > 0)
						memcpy(new_call_data->from_to_field_port, data + matchoff, matchlen);
				}
			} else {
				if (ct_sip_get_info(data, (int)(endData - (char *)data), &matchoff, &matchlen, &ct_sip_hdrs[POS_TO_PORT]) > 0) {
					if (matchlen > 0)
						memcpy(new_call_data->from_to_field_port, data + matchoff, matchlen);
				}
			}
#endif
		} else {
			/* This may itself be the register message. Or this phone has not registered itself */
			if (new_call_data->call_dir == INSIDE_INITIATION && message_type == MSG_INVITE) {
				struct sip_temp_call_data *sip_temp_call = NULL;
				char local_address[16];
				int orig_port;

				sprintf(local_address,"%u.%u.%u.%u",NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip));
				orig_port = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port);

				WRITE_LOCK(&ip_sip_temp_call_rwlock);					
				/* Find out if there is an entry into the temp list for this From and port number.*/
				sip_temp_call = LIST_FIND(&sip_temp_call_data_list, get_temp_call_data, struct sip_temp_call_data*, local_address, orig_port);
				if (sip_temp_call == NULL) {
					sip_temp_call = (struct sip_temp_call_data*)kmalloc(sizeof(struct sip_temp_call_data), GFP_ATOMIC);
					//sanitise the memory chunk
					memset(sip_temp_call, 0, sizeof(struct sip_temp_call_data));
					INIT_LIST_HEAD(&sip_temp_call->list);

					sip_temp_call->change_flag = RAISE_FLAG;
					sip_temp_call->typeofchange = ADD_RULE;
					memcpy(sip_temp_call->original_ip_address,local_address,strlen(local_address));
					sprintf(sip_temp_call->changed_ip_address,"%u.%u.%u.%u",NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip));
					if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) {
						sip_temp_call->original_port = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port);
						sip_temp_call->changed_port = ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port);
						sip_temp_call->proto = UDP;
					} else {
						sip_temp_call->original_port = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.tcp.port);
						sip_temp_call->changed_port = ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.tcp.port);
						sip_temp_call->proto = TCP;
					}
					IDEBUG("sip_nat: INVITE message found from localip=%s:%d and algip=%s:%d without previous registration message\n",sip_temp_call->original_ip_address,sip_temp_call->original_port,sip_temp_call->changed_ip_address,sip_temp_call->changed_port);
					list_prepend(&sip_temp_call_data_list, &sip_temp_call->list);
					wake_up_interruptible(&wait_queue_sipdaemon);	
				}
				WRITE_UNLOCK(&ip_sip_temp_call_rwlock);					
			}

			if (new_call_data->call_dir == INSIDE_INITIATION) {
				if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) {
					new_call_data->localport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port);
					new_call_data->algport = ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port);
				} else {
					new_call_data->localport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.tcp.port);
					new_call_data->algport = ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.tcp.port);
							}
			} else {
				if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == IPPROTO_UDP) {
					new_call_data->localport = ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.udp.port);
					new_call_data->algport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port);
				} else {
					new_call_data->localport = ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.tcp.port);
					new_call_data->algport = ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.tcp.port);
				}
			}
		}
		IDEBUG("sip_nat: The localip=%u.%u.%u.%u:%d and algip=%u.%u.%u.%u:%d\n",NIPQUAD(new_call_data->localip),new_call_data->localport,NIPQUAD(new_call_data->algip),new_call_data->algport);

							/* Add to the global list */
							WRITE_LOCK(&ip_sip_rwlock);
		PDEBUG("sip_call_data adding : %p\n", new);
		list_prepend(&sip_call_data, &new_call_data->list);
							WRITE_UNLOCK(&ip_sip_rwlock);

		call_data = new_call_data;
	} else {
		IDEBUG("sip_nat: Existing call_id [%s] with algport=%d found with direction %s\n",call_data->original_call_id, call_data->key_algport, (call_data->call_dir == INSIDE_INITIATION ? "INSIDE_INITIATION" : "OUTSIDE_INITIATION"));
	} /* End of initialization for the new call */
					
	/* Find out the call's actual IP_CT_DIR_ORIGINAL and IP_CT_DIR_REPLY */
	if ((call_data->call_dir == INSIDE_INITIATION && ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip) == call_data->localip)
			|| (call_data->call_dir == OUTSIDE_INITIATION && ntohl(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip) == call_data->localip)) {
		call_data->CALL_IP_CT_DIR_ORIGINAL = IP_CT_DIR_ORIGINAL;
		call_data->CALL_IP_CT_DIR_REPLY = IP_CT_DIR_REPLY;
		IDEBUG("sip_nat: call_dir_orig = ct_dir_orig : ct.orig.src.ip=%u.%u.%u.%u and ct.reply.src.ip=%u.%u.%u.%u\n",NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip),NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip));
	} else if ((call_data->call_dir == INSIDE_INITIATION && ntohl(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip) == call_data->localip)
			|| (call_data->call_dir == OUTSIDE_INITIATION && ntohl(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip) == call_data->localip)) {
		call_data->CALL_IP_CT_DIR_ORIGINAL = IP_CT_DIR_REPLY;
		call_data->CALL_IP_CT_DIR_REPLY = IP_CT_DIR_ORIGINAL;
		IDEBUG("sip_nat: call_dir_orig = ct_dir_reply : ct.orig.src.ip=%u.%u.%u.%u and ct.reply.src.ip=%u.%u.%u.%u\n",NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip),NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip));
				}
					
	/* Find out the current message's direction */
	switch ((call_data->call_dir << 1) | dir) {
		case ((INSIDE_INITIATION << 1) | IP_CT_DIR_ORIGINAL):
		case ((OUTSIDE_INITIATION << 1) | IP_CT_DIR_REPLY):
			cur_message = IN_2_OUT;
			break;
					
		case ((OUTSIDE_INITIATION << 1) | IP_CT_DIR_ORIGINAL):
		case ((INSIDE_INITIATION << 1) | IP_CT_DIR_REPLY):
			cur_message = OUT_2_IN;
			break;
				}
	/* Reverse the direction if the call's IP_CT_DIR_ORIGINAL is not same as IP_CT_DIR_ORIGINAL */
	if (call_data->CALL_IP_CT_DIR_ORIGINAL == IP_CT_DIR_REPLY)
				{
		swap_flag = 1; /* Orig CT reverse dir to Curr CT */
		cur_message = 1 - cur_message;
		IDEBUG("(%s): call_data->ORG == REPLY, NEW cur_message = %s\n", 
				__FUNCTION__, cur_message==IN_2_OUT?"IN2OUT":"OUT2IN");
			}
	IDEBUG("sip_nat: Current message is %s\n",(cur_message == IN_2_OUT ? "IN_2_OUT" : "OUT_2_IN"));

	/* Sumedh -- 71006 */
	/* 
	 * We will mangle IN->OUT pkts in POSTROUTING, and OUT->IN in PREROUTING hook
				 */ 
	if (! ((cur_message==IN_2_OUT && hooknum==NF_IP_POST_ROUTING) ||
			(cur_message==OUT_2_IN && hooknum==NF_IP_PRE_ROUTING)))
					{
		IDEBUG("%s: Message dir %s in hook %s not touched\n", 
			__FUNCTION__,cur_message==IN_2_OUT?"IN->OUT":"OUT->IN",
		     	hooknum==NF_IP_PRE_ROUTING?"PREROUTING":
				hooknum==NF_IP_POST_ROUTING?"POSTROUTING":
					hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "???");
		UNLOCK_BH(&ip_sip_lock);
		return NF_ACCEPT;
					}
					else
					{
		IDEBUG("%s: Processing packet in dir %s in hook %s\n",
			__FUNCTION__, cur_message==IN_2_OUT?"IN->OUT":"OUT->IN",
		     	hooknum==NF_IP_PRE_ROUTING?"PREROUTING":
				hooknum==NF_IP_POST_ROUTING?"POSTROUTING":"???");
				}
	IDEBUG("sip_nat: Message\n[%s]\n",data);
	/* Sumedh --710061 end      */

	sprintf(localip_buf,"%u.%u.%u.%u",NIPQUAD(call_data->localip));
	sprintf(algip_buf,"%u.%u.%u.%u",NIPQUAD(call_data->algip));
	sprintf(localport_buf,"%d",call_data->localport);
	sprintf(algport_buf,"%d",call_data->algport);

	iRet = sip_get_content_length(pskb);
	if(iRet > 0) {
		//if (cur_message == IN_2_OUT ) {
		if (1) {
			/* Sumedh - Ensure that the call is not local(lan->lan) */
			char to_username[128], to_user[128], to_ip[16];
			char from_username[128], from_user[128], from_ip[16];
			struct sip_user_registration_data *tmp_reg_data, *tmp_reg_data2;

			memset(to_username,0x00, sizeof(to_username));
			memset(to_user,0x00, sizeof(to_user));
			memset(to_ip,0x00, sizeof(to_ip));
			memset(from_username,0x00, sizeof(from_username));
			memset(from_user,0x00, sizeof(from_user));
			memset(from_ip,0x00, sizeof(from_ip));

			get_to_user_name(pskb, to_username);
			SDEBUG("get_to_user_name : %s\n",to_username);
			/* during registration, we store abc@proxyip. Find it it matches with TO */
			tmp_reg_data = LIST_FIND(&sip_user_registration_list, is_user_registered, struct sip_user_registration_data*, to_username);
			if(tmp_reg_data == NULL)
				{
				extract_user(to_username, to_user);
				extract_ip(to_username, to_ip);
				SDEBUG("%s not local, trying %s@%s\n",to_username, to_user, to_ip);
				/* If not, it may still match abc@localip1 */
				tmp_reg_data = LIST_FIND(&sip_user_registration_list, user_ip_search, struct sip_user_registration_data*, to_user, to_ip);	
				}
			
			get_user_name(pskb, from_username);
			SDEBUG("get_from_user_name : %s\n",from_username);
			/* during registration, we store abc@proxyip. Find it it matches with FROM */
			tmp_reg_data2 = LIST_FIND(&sip_user_registration_list, is_user_registered, struct sip_user_registration_data*, from_username);
			if(tmp_reg_data2 == NULL)
				{
				SDEBUG("tmp_reg_data2 is NULL\n");
				extract_user(from_username, from_user);
				extract_ip(from_username, from_ip);
				SDEBUG("%s not local, trying %s@%s\n",from_username, from_user, from_ip);
				/* If not, it may still match abc@localip2 */
				tmp_reg_data2 = LIST_FIND(&sip_user_registration_list, user_ip_search, struct sip_user_registration_data*, from_user, from_ip);	
				}
		
			/* If both are NOT locally found, then go ahead with SDP mangling.
			 * If both are locally found, then it is a LAN->LAN call
			 */ 	
			if(!(tmp_reg_data != NULL && tmp_reg_data2 !=NULL))
				{
				SDEBUG("!!! NOT local  \n");
				if(cur_message == IN_2_OUT)
				{	
					sip_data_fixup(ct, pskb, ctinfo, exp, call_data);
					//if data is mangled fix the content length
					sip_fixup_content_length(pskb, ct, ctinfo);
				}
			}
			else
				{
				strncpy(from_localip,tmp_reg_data2->original_ip_address, strlen(tmp_reg_data2->original_ip_address));
				sprintf(from_localport,"%d",tmp_reg_data2->original_contact_port);
				SDEBUG("!!!! from_localip = [%s]\n",from_localip);
				local_call=1;
				SDEBUG("!!!local CALL \n");
				SDEBUG("Since the call is destined to another LAN phone, not mangling data!!\n");
				}
				}
		//else if(cur_message == OUT_2_IN) 
		if(cur_message == OUT_2_IN) {
			IDEBUG("Checking whether to mangle SDP in OUT->IN pkt");
		}
		if (exp) {
			exp->help.exp_sip_info.expectations_hit_direction = 1 - cur_message;
			strncpy(exp->help.exp_sip_info.call_id,call_data->original_call_id,strlen(call_data->original_call_id));
			exp->help.exp_sip_info.key_algport = call_data->key_algport;
			/* Store the localip in expectation also */
			exp->help.exp_sip_info.localip = call_data->localip;
				}
			}

	if ((*pskb)->sip_nat_applied == 0) {
		if (ct_sip_get_info(data, (int)(endData - (char *)data), &matchoff, &matchlen, &ct_sip_hdrs[POS_CSEQ]) > 0) {
			req_msg_of_res = get_sip_method(data + matchoff + matchlen + 1);
			//IDEBUG("sip_nat: CSeq method found %d\n",req_msg_of_res);
		}

		if (message_type == MSG_REGISTER) {
			enum SipControlProtocol reg_proto = 1;
			int changed_via_port = 0;
			IDEBUG("sip_nat: Calling register_message_handler\n");
			if (ct_sip_get_info(data, (int)(endData - (char *)data), &matchoff, &matchlen, &ct_sip_hdrs[POS_CONTACT_TRANS]) > 0) {
				if (strncmp((char *)data + matchoff, "TCP", sizeof("TCP") - 1) == 0)
					reg_proto = TCP;
		else
					reg_proto = UDP;
				IDEBUG("sip_nat: Contact transport found as %s\n",(reg_proto == TCP ? "TCP" : "UDP"));
			} else {
				reg_proto = UDP;
				IDEBUG("sip_nat: Contact transport could not be found\n");
		}
			if (ct_sip_get_info(data, (int)(endData - (char *)data), &matchoff, &matchlen, &ct_sip_hdrs[POS_CONTACT_PORT]) > 0) {
				call_data->reg_msg_contact_port = simple_strtoul(data + matchoff, NULL, 10);
			} else {
				call_data->reg_msg_contact_port = 0;
	}
			IDEBUG("sip_nat : For the DE/REGISTER message contact port = %d\n",call_data->reg_msg_contact_port);

			register_message_handler(ct,pskb,ctinfo,reg_proto, localip_buf, algip_buf, &changed_via_port);
			call_data->key_algport = changed_via_port;
		} else if ((message_type == MSG_SIP_2_0_200 && req_msg_of_res == MSG_REGISTER) 
				|| (message_type == MSG_SIP_2_0_401 && req_msg_of_res == MSG_REGISTER)) {
			IDEBUG("sip_nat: Calling register_message_reply_handler\n");
			register_message_reply_handler(ct,pskb,ctinfo,proto, localip_buf, call_data->reg_msg_contact_port, algip_buf);	
		} else {
			enum SIP_MESSAGES msg_type;

			if (message_type <= MSG_REQUEST)
				msg_type = MSG_REQUEST;
			else
				msg_type = MSG_RESPONSE;

			//IDEBUG("sip_nat: localip_buf = %s\n",localip_buf);
			//IDEBUG("sip_nat: algip_buf = %s\n",algip_buf);
			//
			if (ct_sip_get_info(data, (int)(endData - (char *)data), &matchoff, &matchlen, &ct_sip_hdrs[POS_RR]) > 0) {
				IDEBUG("sip_nat : Record-Route Found\n");
				record_route =1;
			}
			else if (ct_sip_get_info(data, (int)(endData - (char *)data), &matchoff, &matchlen, &ct_sip_hdrs[POS_RRS]) > 0) {
				IDEBUG("sip_nat : Record-Route over sips: Found\n");
				record_route =1;
			}	


			switch ((msg_type << 2) | (call_data->call_dir << 1) | cur_message) {
				case ((MSG_REQUEST << 2) | (INSIDE_INITIATION << 1) | IN_2_OUT):
					out_req_initiated_inside_handler(ct, pskb, ctinfo, localip_buf, localport_buf, algip_buf, algport_buf, call_data->from_to_field_port, local_call);
					break;	

				case ((MSG_REQUEST << 2) | (INSIDE_INITIATION << 1) | OUT_2_IN):
					in_req_initiated_inside_handler(ct, pskb, ctinfo, localip_buf, localport_buf, algip_buf, algport_buf, call_data->from_to_field_port);
					break;	

				case ((MSG_REQUEST << 2) | (OUTSIDE_INITIATION << 1) | IN_2_OUT):
					out_req_initiated_outside_handler(ct, pskb, ctinfo, localip_buf, localport_buf, algip_buf, algport_buf, call_data->from_to_field_port);
					break;	

				case ((MSG_REQUEST << 2) | (OUTSIDE_INITIATION << 1) | OUT_2_IN):
					in_req_initiated_outside_handler(ct, pskb, ctinfo, localip_buf, localport_buf, algip_buf, algport_buf, call_data->from_to_field_port, local_call, record_route, from_localip, from_localport);
					break;	

				case ((MSG_RESPONSE << 2) | (INSIDE_INITIATION << 1) | IN_2_OUT):
					out_res_initiated_inside_handler(ct, pskb, ctinfo, localip_buf, localport_buf, algip_buf, algport_buf, call_data->from_to_field_port);
					break;	

				case ((MSG_RESPONSE << 2) | (INSIDE_INITIATION << 1) | OUT_2_IN):
					in_res_initiated_inside_handler(ct, pskb, ctinfo, localip_buf, localport_buf, algip_buf, algport_buf, call_data->from_to_field_port);
					break;	

				case ((MSG_RESPONSE << 2) | (OUTSIDE_INITIATION << 1) | IN_2_OUT):
					out_res_initiated_outside_handler(ct, pskb, ctinfo, localip_buf, localport_buf, algip_buf, algport_buf, call_data->from_to_field_port, record_route, local_call);
					break;	

				case ((MSG_RESPONSE << 2) | (OUTSIDE_INITIATION << 1) | OUT_2_IN):
					in_res_initiated_outside_handler(ct, pskb, ctinfo, localip_buf, localport_buf, algip_buf, algport_buf, call_data->from_to_field_port, swap_flag);
					break;	
		}
		}

		if ((message_type == MSG_SIP_2_0_200 && req_msg_of_res == MSG_BYE) ||
				(message_type == MSG_SIP_2_0_200 && req_msg_of_res == MSG_REGISTER) || 
				message_type == MSG_SIP_2_0_3xx || 
				message_type == MSG_SIP_2_0_4xx || 
				message_type == MSG_SIP_2_0_5xx || 
				message_type == MSG_SIP_2_0_6xx) {
			/* Handle dialogue completion specific things over here*/
			struct sip_temp_call_data *sip_temp_call = NULL;

			WRITE_LOCK(&ip_sip_temp_call_rwlock);					
			/* Find out if there is an entry into the temp list for this From and port number.*/
			sip_temp_call = LIST_FIND(&sip_temp_call_data_list, get_temp_call_data, struct sip_temp_call_data*, localip_buf, call_data->localport);
			if (sip_temp_call != NULL) {
				sip_temp_call->change_flag = RAISE_FLAG;
				sip_temp_call->typeofchange = DELETE_RULE;
				wake_up_interruptible(&wait_queue_sipdaemon);	
		}
			WRITE_UNLOCK(&ip_sip_temp_call_rwlock);					
			start_timer(call_data);
	}

		calculate_skbuff_data(pskb, (char**)&data, &endData, &proto);
		if (new_call_data) {
			if (ct_sip_get_info(data, (int)(endData - (char *)data), &matchoff, &matchlen, &ct_sip_hdrs[POS_CALLID]) > 0) {
				memset(call_id, 0x00, sizeof(call_id));
				memcpy(call_id, data + matchoff, matchlen);
				IDEBUG("sip_nat : For original_call_id [%s] changed_call_id [%s]\n",new_call_data->original_call_id,call_id);
			}
			memcpy(new_call_data->changed_call_id,call_id, strlen(call_id) + 1);
			}
		(*pskb)->sip_nat_applied = 1;
		}

	iRet = sip_get_content_length(pskb);
	if(iRet > 0 && cur_message == IN_2_OUT) {
		struct ip_conntrack_expect *other_exp = NULL;
		struct list_head *cur_item;
		list_for_each(cur_item, &ct->sibling_list) { 
			PDEBUG("do_bindings: Inside the loop\n");
			other_exp = list_entry(cur_item, struct ip_conntrack_expect, 
					expected_list);

			/* if this expectation is already established, skip */
			if (proto == UDP && other_exp->seq == oldudplen) {
				struct iphdr *iph = (*pskb)->nh.iph;
				const struct udphdr *udph = (const struct udphdr *)((u_int32_t *)iph + 5);

				other_exp->seq = ntohs(udph->len);
		}
		}
	}

	IDEBUG("sip_nat: Changed message\n[%s]\n",data);

	//sip_fixup_content_length(pskb);
sip_nat_out:
	UNLOCK_BH(&ip_sip_lock);
	return NF_ACCEPT;
}

/******************************************************************
 *  Function Name	:   init
 *  Description      :   module loading function. It loads the helper
 *  						module with the nat core in kernel.
 *  Input Values    	:  
 *  Output Values 	:  
 *  Return Value   	:
 *  Notes			: 
 *********************************************************************/
static int __init init(void)
{
	int ret=0;

	PDEBUG("Registering Sip_device\n");
	ret = register_chrdev(MAJOR_NUM, DEVICE_FILE_NAME, &Fops);
	
	  /* Negative values signify an error */
	if (ret < 0) {
		printk ("%s failed with %d\n", "Sorry, registering the character device ", ret);
	}
	return ret;
}

/******************************************************************
 *  Function Name	:  	fini
 *  Description      :  	module unloading function
 *  Input Values    	:	void 
 *  Output Values 	:  
 *  Return Value   	:	void
 *  Notes			: 
 *********************************************************************/
static void __exit fini(void)
{
	struct list_head  *cur_item;
	struct list_head  *temp_item;
	struct sip_call_data* cur;
	struct sip_user_registration_data* cur1;
	struct sip_temp_call_data* cur2;
	struct sip_helper_registration_data* cur_registration;
	int ret;
	/* free up all sip_related data */
	WRITE_LOCK(&ip_sip_rwlock);	
	list_for_each_safe(cur_item, temp_item, &sip_call_data) {

		cur = list_entry(cur_item, struct sip_call_data, list);
		PDEBUG(" sip_call_data deleting :  %p\n",cur);

		if(cur->timer_set_flag == 1) {
			del_timer(&cur->timeout);
			PDEBUG("timer deleted for sip_data \n");
			list_del(&cur->list);
		}
			list_del(&cur->list);
			kfree(cur);
		}
	WRITE_UNLOCK(&ip_sip_rwlock);

	//release the user process
	PDEBUG("Waking up 6\n");
	wake_up_interruptible(&wait_queue_sipdaemon);	
	WRITE_LOCK(&ip_sip_user_reg_rwlock);	
	list_for_each_safe(cur_item, temp_item, &sip_user_registration_list) {

		cur1 = list_entry(cur_item, struct sip_user_registration_data, list);
		PDEBUG(" sip_user_registration_data deleting :  %p\n",cur);

		if(cur1->timer_set_flag == 1) {
			del_timer(&cur1->timeout);
			PDEBUG("timer deleted for sip_data \n");
		}
			list_del(&cur1->list);
			kfree(cur1);
		}
	WRITE_UNLOCK(&ip_sip_user_reg_rwlock);

	WRITE_LOCK(&ip_sip_temp_call_rwlock);	
	list_for_each_safe(cur_item, temp_item, &sip_temp_call_data_list) {

		cur2 = list_entry(cur_item, struct sip_temp_call_data, list);
		PDEBUG(" sip_temp_call_data deleting :  %p\n",cur);

		list_del(&cur2->list);
		kfree(cur2);
	}
	WRITE_UNLOCK(&ip_sip_temp_call_rwlock);


	list_for_each_safe(cur_item, temp_item, &sip_helper_registration_list) {
		cur_registration = list_entry(cur_item, struct sip_helper_registration_data, list);
		PDEBUG("sip: deleting registration data\n");
		//		PDEBUG("sip: conntrack helper unregistered\n");
		//		ip_conntrack_helper_unregister(cur_registration->sip_conntrack);
		PDEBUG("sip: nat helper unregistered\n");
		ip_nat_helper_unregister(cur_registration->sip_nat);
		//		kfree(cur_registration->sip_conntrack);
		kfree(cur_registration->sip_nat);
		list_del(&cur_registration->list);
		kfree(cur_registration);
		
	}

	ret = unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);
	/* If there's an error, report it */ 
	if (ret < 0)
		printk("Error in module_unregister_chrdev: %d\n", ret);
}

module_init(init);
module_exit(fini);


/* ===========================================================================
 * Revision History:
 *
 * $Log$
 * ===========================================================================
 */

