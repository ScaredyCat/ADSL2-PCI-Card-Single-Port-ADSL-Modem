#ifndef _IPT_SET_H
#define _IPT_SET_H

#include <linux/netfilter_ipv4/ip_set.h>

struct ipt_set_info {
	int16_t id;
	u_int8_t set_level, ip_level;
	u_int32_t flags[IP_SET_LEVELS];
};

/* match info */
struct ipt_set_info_match {
	struct ipt_set_info match;
};

struct ipt_set_info_target {
	struct ipt_set_info add_set;
	struct ipt_set_info del_set;
};

#endif /*_IPT_SET_H*/
