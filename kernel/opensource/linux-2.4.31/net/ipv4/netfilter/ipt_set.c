/* Kernel module to match an IP address set. */

#include <linux/module.h>
#include <linux/ip.h>
#include <linux/skbuff.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_set.h>
#include <linux/netfilter_ipv4/ipt_set.h>

static inline int
match_set(const struct ipt_set_info *info,
	  const struct sk_buff *skb,
	  int inv)
{
	if (ip_set_testip_kernel(ip_set_list[info->id], 
				 skb,
				 info->flags,
				 info->set_level,
				 info->ip_level))
		inv = !inv;
	return inv;
}

static int
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const void *matchinfo,
      int offset,
      const void *hdr,
      u_int16_t datalen,
      int *hotdrop)
{
	const struct ipt_set_info_match *info = matchinfo;
	
	if (info->match.id < 0)
		return 0;
		
	return match_set(&info->match,
			 skb,
			 info->match.flags[0] & IPSET_MATCH_INV);
}

static int
checkentry(const char *tablename,
	   const struct ipt_ip *ip,
	   void *matchinfo,
	   unsigned int matchsize,
	   unsigned int hook_mask)
{
	struct ipt_set_info_match *info = matchinfo;

	if (matchsize != IPT_ALIGN(sizeof(struct ipt_set_info_match))) {
		ip_set_printk("invalid matchsize %d", matchsize);
		return 0;
	}

	if (info->match.id < 0)
		return 1;
		
	if (!ip_set_get_byid(info->match.id)) {
		ip_set_printk("cannot verify setid %i to match",
			      info->match.id);
		return 0;	/* error */
	}
	DP("checkentry OK");

	return 1;
}

static void destroy(void *matchinfo, unsigned int matchsize)
{
	struct ipt_set_info_match *info = matchinfo;

	if (matchsize != IPT_ALIGN(sizeof(struct ipt_set_info_match))) {
		ip_set_printk("invalid matchsize %d", matchsize);
		return;
	}

	if (info->match.id >= 0)
		ip_set_put(ip_set_list[info->match.id]);
}

static struct ipt_match set_match = {
	.name		= "set",
	.match		= &match,
	.checkentry	= &checkentry,
	.destroy	= &destroy,
	.me		= THIS_MODULE
};

static int __init init(void)
{
	return ipt_register_match(&set_match);
}

static void __exit fini(void)
{
	ipt_unregister_match(&set_match);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
