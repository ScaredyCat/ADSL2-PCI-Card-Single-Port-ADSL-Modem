#ifndef __DANUBE_PPA_STACK_AL_H__20060915_2352__
#define __DANUBE_PPA_STACK_AL_H__20060915_2352__



/******************************************************************************
**
** FILE NAME    : danube_ppa_stack_al.h
** PROJECT      : Twinpass-E
** MODULES     	: PPA (Routing Acceleration)
**
** DATE         : 15 SEP 2006
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA (Routing Acceleration) Protocol Stack (Linux) Adaptor
**                Header File
** COPYRIGHT    : 	Copyright (c) 2006
**			Infineon Technologies AG
**			Am Campeon 1-12, 85579 Neubiberg, Germany
**
**   Any use of this software is subject to the conclusion of a respective
**   License agreement. Without such a License agreement no rights to the
**   software are granted
**
** HISTORY
** $Date        $Author         $Comment
** 15 SEP 2006  Xu Liang        Initiate Version
** 26 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/



#ifdef __KERNEL__
  #include <linux/if_pppox.h>
  #include <linux/netfilter_ipv4/ip_conntrack.h>
#endif

/*
 * ####################################
 *              Definition
 * ####################################
 */

#define RET_E_OK		                        0
#define RET_E_ERR		                        -1
#define RET_E_NOT_AVAIL		                    -1
#define RET_E_NOT_POSSIBLE	                    -2

#define PPA_ETH_ALEN                            ETH_ALEN
#define PPA_IF_NAME_SIZE                        IFNAMSIZ

/*
 *  definition for application layer
 */
#ifndef __KERNEL__
  #define ETH_ALEN                                6
  #define IFNAMSIZ                                16
#endif



/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*
 *  data type for application layer
 */
#ifndef __KERNEL__
  typedef unsigned long uint32_t;
  typedef unsigned short uint16_t;
  typedef unsigned char uint8_t;
//  typedef long int32_t;
//  typedef int int8_t;
#endif

/*
 *  data type for API
 */

typedef char PPA_IFNAME;

typedef uint32_t IPADDR;

#ifdef __KERNEL__
  typedef struct sk_buff PPA_BUF;

  typedef struct ip_conntrack PPA_SESSION;

  typedef struct net_device PPA_NETIF;

  typedef struct semaphore PPA_LOCK;

  typedef kmem_cache_t PPA_MEM_CACHE;

  typedef struct timer_list PPA_TIMER;
#endif



/*
 * ####################################
 *           Inline Functions
 * ####################################
 */



/*
 * ####################################
 *             Declaration
 * ####################################
 */

#ifdef __KERNEL__
  PPA_SESSION *ppa_get_session(PPA_BUF *);

  uint8_t ppa_get_pkt_ip_proto(PPA_BUF *);
  uint32_t ppa_get_pkt_src_ip(PPA_BUF *);
  uint32_t ppa_get_pkt_dst_ip(PPA_BUF *);
  void *ppa_get_pkt_transport_hdr(PPA_BUF *);
  uint16_t ppa_get_pkt_src_port(PPA_BUF *);
  uint16_t ppa_get_pkt_dst_port(PPA_BUF *);
  void ppa_get_pkt_rx_src_mac_addr(PPA_BUF *, uint8_t[ETH_ALEN]);
  void ppa_get_pkt_rx_dst_mac_addr(PPA_BUF *, uint8_t[ETH_ALEN]);
  PPA_NETIF *ppa_get_pkt_src_if(PPA_BUF *);
  PPA_NETIF *ppa_get_pkt_dst_if(PPA_BUF *);

  #ifdef CONFIG_PPPOE
    extern int32_t pppoe_get_pppoe_addr(struct net_device *, struct pppoe_addr *);
    extern sid_t ppa_get_pkt_pppoe_session_id(struct sk_buff *);
  #endif
  uint32_t ppa_check_is_ppp_netif(PPA_NETIF *);
  uint32_t ppa_check_is_pppoe_netif(PPA_NETIF *);
  int32_t ppa_pppoe_get_eth_netif(PPA_NETIF *, PPA_IFNAME[IFNAMSIZ]);
  int32_t ppa_pppoe_get_dst_mac(PPA_NETIF * netif, uint8_t[ETH_ALEN]);
  int32_t ppa_get_dst_mac(PPA_BUF *, PPA_SESSION *, uint8_t[ETH_ALEN]);

  PPA_NETIF *ppa_get_netif(PPA_IFNAME *);
  void ppa_get_netif_hwaddr(PPA_NETIF *, uint8_t[ETH_ALEN]);
  PPA_IFNAME *ppa_get_netif_name(PPA_NETIF *);
  uint32_t ppa_is_netif_equal(PPA_NETIF *, PPA_NETIF *);
  uint32_t ppa_is_netif_name(PPA_NETIF *, PPA_IFNAME *);
  uint32_t ppa_is_netif_name_prefix(PPA_NETIF *, PPA_IFNAME *, int);

  int32_t ppa_if_is_vlan_if(PPA_NETIF *, PPA_IFNAME *);
  int32_t ppa_vlan_get_physical_if(PPA_IFNAME *, PPA_IFNAME[]);

  int32_t ppa_get_bridge_member_ifs(PPA_IFNAME *, int *, PPA_IFNAME **);

  uint32_t ppa_is_session_equal(PPA_SESSION *, PPA_SESSION *);
  uint32_t ppa_get_session_helper(PPA_SESSION *);
  #ifdef CONFIG_IP_NF_NAT_NEEDED
    uint32_t ppa_get_nat_helper(PPA_SESSION *);
  #endif
  uint32_t ppa_check_is_special_session(PPA_BUF *, PPA_SESSION *);

  int32_t ppa_is_pkt_fragment(PPA_BUF *);
  int32_t ppa_is_pkt_host_output(PPA_BUF *);
  int32_t ppa_is_pkt_broadcast(PPA_BUF *);
  int32_t ppa_is_pkt_multicast(PPA_BUF *);
  int32_t ppa_is_pkt_loopback(PPA_BUF *);
  int32_t ppa_is_pkt_local(PPA_BUF *);
  int32_t ppa_is_pkt_routing(PPA_BUF *);
  int32_t ppa_is_pkt_mc_routing(PPA_BUF *);

  int32_t ppa_lock_init(PPA_LOCK *);
  void ppa_lock_get(PPA_LOCK *);
  void ppa_lock_release(PPA_LOCK *);

  void *ppa_malloc(uint32_t);
  int32_t ppa_free(void *);

  int32_t ppa_mem_cache_create(const char *, uint32_t, PPA_MEM_CACHE **);
  int32_t ppa_mem_cache_destroy(PPA_MEM_CACHE *);
  void *ppa_mem_cache_alloc(PPA_MEM_CACHE *);
  void ppa_mem_cache_free(void *, PPA_MEM_CACHE *);

  void ppa_memcpy(void *, const void *, uint32_t);
  void ppa_memset(void *, uint32_t, uint32_t);

  int32_t ppa_timer_init(PPA_TIMER *, void (*)(unsigned long));
  int32_t ppa_timer_add(PPA_TIMER *, uint32_t);
  void ppa_timer_del(PPA_TIMER *);
  uint32_t ppa_get_time_in_sec(void);
#endif  //  __KERNEL__



#endif  //  __DANUBE_PPA_STACK_AL_H__20060915_2352__
