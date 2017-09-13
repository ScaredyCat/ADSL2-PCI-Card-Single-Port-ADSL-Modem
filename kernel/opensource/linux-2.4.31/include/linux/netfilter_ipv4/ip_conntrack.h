#ifndef _IP_CONNTRACK_H
#define _IP_CONNTRACK_H
/* Connection state tracking for netfilter.  This is separated from,
   but required by, the NAT layer; it can also be used by an iptables
   extension. */

#include <linux/config.h>
#include <linux/netfilter_ipv4/ip_conntrack_tuple.h>
#include <linux/bitops.h>
#include <asm/atomic.h>

#ifdef CONFIG_IFX_ALG_QOS // chandrav
#define IFX_ALG_APP_ICMP        0x01000000
#define IFX_ALG_APP_FTP         0x02000000
#define IFX_ALG_APP_PPTP        0x03000000
#define IFX_ALG_APP_SIP         0x04000000
#define IFX_ALG_APP_CUSEEME     0x05000000
#define IFX_ALG_APP_H323        0x06000000
#define IFX_ALG_APP_RTSP        0x07000000
#define IFX_ALG_APP_IPSEC       0x08000000


#define IFX_ALG_PROTO_CTRL      0x00
#define IFX_ALG_PROTO_RTP       0x00010000
#define IFX_ALG_PROTO_RTCP      0x00020000
#define IFX_ALG_PROTO_DATA      0x00030000
#endif  //CONFIG_IFX_ALG_QOS

enum ip_conntrack_info
{
	/* Part of an established connection (either direction). */
	IP_CT_ESTABLISHED,

	/* Like NEW, but related to an existing connection, or ICMP error
	   (in either direction). */
	IP_CT_RELATED,

	/* Started a new connection to track (only
           IP_CT_DIR_ORIGINAL); may be a retransmission. */
	IP_CT_NEW,

	/* >= this indicates reply direction */
	IP_CT_IS_REPLY,

	/* Number of distinct IP_CT types (no NEW in reply dirn). */
	IP_CT_NUMBER = IP_CT_IS_REPLY * 2 - 1
};

/* Bitset representing status of connection. */
enum ip_conntrack_status {
	/* It's an expected connection: bit 0 set.  This bit never changed */
	IPS_EXPECTED_BIT = 0,
	IPS_EXPECTED = (1 << IPS_EXPECTED_BIT),

	/* We've seen packets both ways: bit 1 set.  Can be set, not unset. */
	IPS_SEEN_REPLY_BIT = 1,
	IPS_SEEN_REPLY = (1 << IPS_SEEN_REPLY_BIT),

	/* Conntrack should never be early-expired. */
	IPS_ASSURED_BIT = 2,
	IPS_ASSURED = (1 << IPS_ASSURED_BIT),

	/* Connection is confirmed: originating packet has left box */
	IPS_CONFIRMED_BIT = 3,
	IPS_CONFIRMED = (1 << IPS_CONFIRMED_BIT),
};

#include <linux/netfilter_ipv4/ip_conntrack_tcp.h>
#include <linux/netfilter_ipv4/ip_conntrack_icmp.h>
#ifdef CONFIG_IP_NF_CT_PROTO_GRE
#include <linux/netfilter_ipv4/ip_conntrack_proto_gre.h>
#endif


/* per conntrack: protocol private data */
union ip_conntrack_proto {
	/* insert conntrack proto private data here */
	struct ip_ct_tcp tcp;
	struct ip_ct_icmp icmp;
#ifdef CONFIG_IP_NF_CT_PROTO_GRE
        struct ip_ct_gre gre;
#endif
};

union ip_conntrack_expect_proto {
	/* insert expect proto private data here */
#ifdef CONFIG_IP_NF_CT_PROTO_GRE
        struct ip_ct_gre_expect gre;
#endif
};

/* Add protocol helper include file here */
#include <linux/netfilter_ipv4/ip_conntrack_amanda.h>

#include <linux/netfilter_ipv4/ip_conntrack_ftp.h>
#include <linux/netfilter_ipv4/ip_conntrack_irc.h>
#ifdef CONFIG_IP_NF_TALK
#include <linux/netfilter_ipv4/ip_conntrack_talk.h>
#endif
#ifdef CONFIG_IP_NF_PPTP
#include <linux/netfilter_ipv4/ip_conntrack_pptp.h>
#endif
#ifdef CONFIG_IP_NF_MMS
#include <linux/netfilter_ipv4/ip_conntrack_mms.h>
#endif
#ifdef CONFIG_IP_NF_H323
#include <linux/netfilter_ipv4/ip_conntrack_h323.h>
#endif
#ifdef CONFIG_IP_NF_RTSP
#include <linux/netfilter_ipv4/ip_conntrack_rtsp.h>
#endif


/* per expectation: application helper private data */
union ip_conntrack_expect_help {
	/* insert conntrack helper private data (expect) here */
#ifdef CONFIG_IP_NF_TALK
        struct ip_ct_talk_expect exp_talk_info;
#endif
#ifdef CONFIG_IP_NF_PPTP
        struct ip_ct_pptp_expect exp_pptp_info;
#endif
#ifdef CONFIG_IP_NF_MMS
        struct ip_ct_mms_expect exp_mms_info;
#endif
#ifdef CONFIG_IP_NF_H323
        struct ip_ct_h225_expect exp_h225_info;
#endif
#ifdef CONFIG_IP_NF_RTSP
        struct ip_ct_rtsp_expect exp_rtsp_info;
#endif
	struct ip_ct_amanda_expect exp_amanda_info;
	struct ip_ct_ftp_expect exp_ftp_info;
	struct ip_ct_irc_expect exp_irc_info;

#ifdef CONFIG_IP_NF_NAT_NEEDED
	union {
		/* insert nat helper private data (expect) here */
	} nat;
#endif
};

/* per conntrack: application helper private data */
union ip_conntrack_help {
	/* insert conntrack helper private data (master) here */
#ifdef CONFIG_IP_NF_TALK
        struct ip_ct_talk_master ct_talk_info;
#endif
#ifdef CONFIG_IP_NF_PPTP
        struct ip_ct_pptp_master ct_pptp_info;
#endif
#ifdef CONFIG_IP_NF_MMS
        struct ip_ct_mms_master ct_mms_info;
#endif
#ifdef CONFIG_IP_NF_H323
        struct ip_ct_h225_master ct_h225_info;
#endif
#ifdef CONFIG_IP_NF_RTSP
        struct ip_ct_rtsp_master ct_rtsp_info;
#endif
	struct ip_ct_ftp_master ct_ftp_info;
	struct ip_ct_irc_master ct_irc_info;
};

#ifdef CONFIG_IP_NF_NAT_NEEDED
#include <linux/netfilter_ipv4/ip_nat.h>
#ifdef CONFIG_IP_NF_NAT_PPTP
#include <linux/netfilter_ipv4/ip_nat_pptp.h>
#endif


/* per conntrack: nat application helper private data */
union ip_conntrack_nat_help {
	/* insert nat helper private data here */
#ifdef CONFIG_IP_NF_NAT_PPTP
        struct ip_nat_pptp nat_pptp_info;
#endif
};
#endif

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/skbuff.h>

#ifdef CONFIG_NETFILTER_DEBUG
#define IP_NF_ASSERT(x)							\
do {									\
	if (!(x))							\
		/* Wooah!  I'm tripping my conntrack in a frenzy of	\
		   netplay... */					\
		printk("NF_IP_ASSERT: %s:%i(%s)\n",			\
		       __FILE__, __LINE__, __FUNCTION__);		\
} while(0)
#else
#define IP_NF_ASSERT(x)
#endif

struct ip_conntrack_expect
{
	/* Internal linked list (global expectation list) */
	struct list_head list;

	/* reference count */
	atomic_t use;

	/* expectation list for this master */
	struct list_head expected_list;

	/* The conntrack of the master connection */
	struct ip_conntrack *expectant;

	/* The conntrack of the sibling connection, set after
	 * expectation arrived */
	struct ip_conntrack *sibling;

	/* Tuple saved for conntrack */
	struct ip_conntrack_tuple ct_tuple;

	/* Timer function; deletes the expectation. */
	struct timer_list timeout;

	/* Data filled out by the conntrack helpers follow: */

	/* We expect this tuple, with the following mask */
	struct ip_conntrack_tuple tuple, mask;

	/* Function to call after setup and insertion */
	int (*expectfn)(struct ip_conntrack *new);

	/* At which sequence number did this expectation occur */
	u_int32_t seq;
  
	union ip_conntrack_expect_proto proto;

	union ip_conntrack_expect_help help;
};

#ifdef CONFIG_IP_NF_TURBONAT
#include <net/dst.h>
struct route_info
{
        int init;
        int maniptype;
        __u32 original_saddr;
        __u32 saddr;
        __u32 daddr;
        struct ip_conntrack_manip *manip;
        struct dst_entry dst;
#ifdef CONFIG_NETFILTER
        /* tos/dscp marking */
        __u8  tos;
        /* Can be used for communication between hooks. */
        unsigned long   nfmark;
#endif /*CONFIG_NETFILTER*/
};

#ifdef CONFIG_ADM8668_HW_NAT
#define LINK_TCP        1
#define LINK_UDP        0

#define DEAD_TIMER      0x1

struct hwnat_info
{
        u8 smac[8];
        u8 dmac[8];
        u32 saddr;
        u32 daddr;
        u32 nataddr;
        u16 sport;
        u16 dport;
        u16 natport;
        u16 linktype;
        u32 dir;
        u32 index;
        u32 flag;
};
#endif  /* CONFIG_ADM8668_HW_NAT */
#endif

struct ip_conntrack_helper;

struct ip_conntrack
{
	/* Usage count in here is 1 for hash table/destruct timer, 1 per skb,
           plus 1 for any connection(s) we are `master' for */
	struct nf_conntrack ct_general;

	/* These are my tuples; original and reply */
	struct ip_conntrack_tuple_hash tuplehash[IP_CT_DIR_MAX];

	/* Have we seen traffic both ways yet? (bitset) */
	unsigned long status;

	/* Timer function; drops refcnt when it goes off. */
	struct timer_list timeout;

	/* If we're expecting another related connection, this will be
           in expected linked list */
	struct list_head sibling_list;
	
	/* Current number of expected connections */
	unsigned int expecting;

	/* If we were expected by an expectation, this will be it */
	struct ip_conntrack_expect *master;

	/* Helper, if any. */
	struct ip_conntrack_helper *helper;

	/* Our various nf_ct_info structs specify *what* relation this
           packet has to the conntrack */
	struct nf_ct_info infos[IP_CT_NUMBER];

	/* Storage reserved for other modules: */

	union ip_conntrack_proto proto;

	union ip_conntrack_help help;

#ifdef CONFIG_IP_NF_NAT_NEEDED
	struct {
		struct ip_nat_info info;
		union ip_conntrack_nat_help help;
#if defined(CONFIG_IP_NF_TARGET_MASQUERADE) || \
	defined(CONFIG_IP_NF_TARGET_MASQUERADE_MODULE)
		int masq_index;
#endif
	} nat;
#endif /* CONFIG_IP_NF_NAT_NEEDED */

#ifdef CONFIG_IFX_ALG_QOS
        int ifx_alg_qos_mark;
        int rtcp_expect_registered;
#endif

#ifdef CONFIG_IP_NF_TURBONAT
        /* For Turbo NAT -- Joew,2003/9/19*/
        struct route_info in_route;
        struct route_info out_route;
        __u32 tmp_saddr;
        int tcp_ack;
#ifdef CONFIG_ADM8668_HW_NAT
        int hw_nat_enable;
        unsigned long state_timeout;
        /* for debug */
        struct hwnat_info hwnat;
#endif
#endif

};

#ifdef CONFIG_IFX_ALG_QOS
#if 0  // chandrav
#define IFX_ALG_QOS_DBG printk
#else
#define IFX_ALG_QOS_DBG(format, args...)
#endif
#endif

/* get master conntrack via master expectation */
#define master_ct(conntr) (conntr->master ? conntr->master->expectant : NULL)

/* Alter reply tuple (maybe alter helper).  If it's already taken,
   return 0 and don't do alteration. */
extern int
ip_conntrack_alter_reply(struct ip_conntrack *conntrack,
			 const struct ip_conntrack_tuple *newreply);

/* Is this tuple taken? (ignoring any belonging to the given
   conntrack). */
extern int
ip_conntrack_tuple_taken(const struct ip_conntrack_tuple *tuple,
			 const struct ip_conntrack *ignored_conntrack);

/* Return conntrack_info and tuple hash for given skb. */
extern struct ip_conntrack *
ip_conntrack_get(struct sk_buff *skb, enum ip_conntrack_info *ctinfo);

/* decrement reference count on a conntrack */
extern inline void ip_conntrack_put(struct ip_conntrack *ct);

/* find unconfirmed expectation based on tuple */
struct ip_conntrack_expect *
ip_conntrack_expect_find_get(const struct ip_conntrack_tuple *tuple);

/* decrement reference count on an expectation */
void ip_conntrack_expect_put(struct ip_conntrack_expect *exp);

extern int invert_tuplepr(struct ip_conntrack_tuple *inverse,
			  const struct ip_conntrack_tuple *orig);

/* Refresh conntrack for this many jiffies */
extern void ip_ct_refresh(struct ip_conntrack *ct,
			  unsigned long extra_jiffies);

/* These are for NAT.  Icky. */
/* Call me when a conntrack is destroyed. */
extern void (*ip_conntrack_destroyed)(struct ip_conntrack *conntrack);

/* Returns new sk_buff, or NULL */
struct sk_buff *
ip_ct_gather_frags(struct sk_buff *skb, u_int32_t user);

/* Iterate over all conntracks: if iter returns true, it's deleted. */
extern void
ip_ct_iterate_cleanup(int (*iter)(struct ip_conntrack *i, void *data),
                      void *data);

/* It's confirmed if it is, or has been in the hash table. */
static inline int is_confirmed(struct ip_conntrack *ct)
{
	return test_bit(IPS_CONFIRMED_BIT, &ct->status);
}

extern unsigned int ip_conntrack_htable_size;
#endif /* __KERNEL__ */
#endif /* _IP_CONNTRACK_H */
