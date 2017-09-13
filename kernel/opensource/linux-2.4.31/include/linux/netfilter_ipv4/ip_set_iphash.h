#ifndef __IP_SET_IPHASH_H
#define __IP_SET_IPHASH_H

#include <linux/netfilter_ipv4/ip_set.h>

#define SETTYPE_NAME "iphash"
#define MAX_RANGE 0x0000FFFF

struct ip_set_iphash {
	struct ip_set_private **childsets;	/* child sets */

	/* Type speficic members: */
	uint32_t initval;		/* initval for jhash_1word */
	ip_set_ip_t hashsize;		/* hash size */
	ip_set_ip_t netmask;		/* netmask */
	ip_set_ip_t *members;		/* the iphash proper */
};

struct ip_set_req_iphash_create {
	uint32_t initval;
	ip_set_ip_t hashsize;
	ip_set_ip_t netmask;
};

struct ip_set_req_iphash {
	ip_set_ip_t ip;
	u_int32_t flags;
};

#endif	/* __IP_SET_IPHASH_H */
