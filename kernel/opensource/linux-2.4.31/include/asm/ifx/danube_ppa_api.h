#ifndef __DANUBE_PPA_API_H__20060913_1556__
#define __DANUBE_PPA_API_H__20060913_1556__



/******************************************************************************
**
** FILE NAME    : danube_ppa_api.h
** PROJECT      : Twinpass-E
** MODULES      : PPA (Routing Acceleration)
**
** DATE         : 13 SEP 2006
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA (Routing Acceleration) Protocol Stack Hook API Header File
** COPYRIGHT    :   Copyright (c) 2006
**          Infineon Technologies AG
**          Am Campeon 1-12, 85579 Neubiberg, Germany
**
**   Any use of this software is subject to the conclusion of a respective
**   License agreement. Without such a License agreement no rights to the
**   software are granted
**
** HISTORY
** $Date        $Author         $Comment
** 13 SEP 2006  Xu Liang        Initiate Version
** 26 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/



#include <asm/danube/danube_ppa_stack_al.h>
#include <asm/danube/danube_ppa_ppe_hal.h>
#include <asm/danube/danube_ppa_ppe_d3_hal.h>


/*
 * ####################################
 *              Definition
 * ####################################
 */

#define USE_NEW_MC_API                          1

#define MAX_MC_IFS_NUM                          8

/*
 *  ioctl Command
 */
#define PPA_IOC_MAGIC                           ((uint32_t)'p')
#define PPA_CMD_INIT                            _IOW(PPA_IOC_MAGIC, 0, PPA_CMD_INIT_INFO)
#define PPA_CMD_EXIT                            _IO(PPA_IOC_MAGIC, 1)
#define PPA_CMD_ENABLE                          _IOW(PPA_IOC_MAGIC, 2, PPA_CMD_ENABLE_INFO)
#define PPA_CMD_GET_STATUS                      _IOR(PPA_IOC_MAGIC, 3, PPA_CMD_GET_STATUS_INFO)
#define PPA_CMD_SET_IF_MAC                      _IOW(PPA_IOC_MAGIC, 4, PPA_CMD_IF_MAC_INFO)
#define PPA_CMD_GET_IF_MAC                      _IOWR(PPA_IOC_MAGIC, 5, PPA_CMD_IF_MAC_INFO)
#define PPA_IOC_MAXNR                           6

/*
 *  flags
 */
#define PPA_F_BEFORE_NAT_TRANSFORM              0x00000001
#define PPA_F_SESSION_ORG_DIR                   0x00000010
#define PPA_F_SESSION_REPLY_DIR                 0x00000020
#define PPA_F_SESSION_BIDIRECTIONAL             (PPA_F_SESSION_ORG_DIR | PPA_F_SESSION_REPLY_DIR)
#define PPA_F_BRIDGED_SESSION                   0x00000100
#define PPA_F_SESSION_NEW_DSCP                  0x00001000
#define PPA_F_SESSION_VLAN                      0x00002000
#define PPA_F_MTU                               0x00004000
#define PPA_F_BRIDGE_LOCAL                      0x00010000
#define PPA_F_STATIC_ENTRY                      0x20000000
#define PPA_F_DROP_PACKET                       0x40000000
#define PPA_F_BRIDGE_ACCEL_MODE                 0x80000000

/*
 *  interface flags
 */
#define PPA_F_IF_ALLOW_BROADCAST_MULTICAST      0x00000001

/*
 *  ifx_ppa_session_add return value
 */
#define IFX_PPA_SESSION_NOT_ADDED               -1
#define IFX_PPA_SESSION_ADDED                   0
#define IFX_PPA_SESSION_EXISTS                  1

/*
 *  ifx_ppa_inactivity_status return value
 */
//#define IFX_PPA_SESSION_NOT_ADDED               -1
#define IFX_PPA_HIT                             0
#define IFX_PPA_TIMEOUT                         1



/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*
 *  API structures
 */

typedef struct {
    PPA_IFNAME *ifname;
    uint32_t    if_flags;
} PPA_IFINFO;

typedef struct ppa_verify_checks {
    uint32_t    f_ip_verify             :1; //  Enable/Disable IP verification checks
    uint32_t    f_tcp_udp_verify        :1; //  Enable/Disable TCP/UDP verification checks
    uint32_t    f_drop_on_no_hit        :1; //  Drop unicast packets on no hit, forward to MIPS CPU otherwise (default)
    uint32_t    f_mc_drop_on_no_hit     :1; //  Drop multicast on no hit, forward to MIPS CPU otherwise (default)
} PPA_VERIFY_CHECKS;

typedef struct {
#if 0
    uint32_t    f_lan_ip_verify         :1; //  Enable/Disable IP verification checks
    uint32_t    f_lan_tcp_udp_verify    :1; //  Enable/Disable TCP/UDP verification checks
    uint32_t    f_lan_drop_on_no_hit    :1; //  Drop unicast packets on no hit, forward to MIPS CPU otherwise (default)
    uint32_t    f_lan_mc_drop_on_no_hit :1; //  Drop multicast on no hit, forward to MIPS CPU otherwise (default)
    uint32_t    f_wan_ip_verify         :1; //  Enable/Disable IP verification checks
    uint32_t    f_wan_tcp_udp_verify    :1; //  Enable/Disable TCP/UDP verification checks
    uint32_t    f_wan_drop_on_no_hit    :1; //  Drop unicast packets on no hit, forward to MIPS CPU otherwise (default)
    uint32_t    f_wan_mc_drop_on_no_hit :1; //  Drop multicast on no hit, forward to MIPS CPU otherwise (default)
#else
    PPA_VERIFY_CHECKS   lan_rx_checks;      // LAN Ingress checks
    PPA_VERIFY_CHECKS   wan_rx_checks;      // WAN Ingress checks
#endif
    uint32_t    num_lanifs;                 //  Number of LAN side interfaces
//    PPA_IFNAME **p_lanifs;                  //  Array of LAN side interfaces' names
    PPA_IFINFO *p_lanifs;
    uint32_t    num_wanifs;                 //  Number of WAN side interfaces
//    PPA_IFNAME **p_wanifs;                  //  Array of WAN side interfaces' names
    PPA_IFINFO *p_wanifs;
    uint32_t    max_lan_source_entries;     //  Number of session entries with LAN source
    uint32_t    max_wan_source_entries;     //  Number of session entries with WAN source
    uint32_t    max_mc_entries;             //  Number of multicast sessions
    uint32_t    max_bridging_entries;       //  Number of bridging entries

    uint32_t    add_requires_min_hits;      // Minimum number of calls to ppa_add before session would be added in h/w
} PPA_INIT_INFO;

typedef struct {
    uint32_t    new_dscp            :6;
    uint32_t    dscp_remark         :1;
    uint32_t    vlan_insert         :1;
    uint32_t    vlan_remove         :1;
    uint32_t    reserved1           :7;
    uint32_t    vlan_prio           :3;
    uint32_t    vlan_cfi            :1;
    uint32_t    vlan_id             :12;
    uint16_t    mtu;
    uint32_t    session_flags;
} PPA_SESSION_EXTRA;

#if !defined(USE_NEW_MC_API) || !USE_NEW_MC_API
  typedef struct {
    uint32_t    ip_mc_group;            //  IP multicast group address
    int8_t      num_ifs;                //  Number of output interfaces
//    PPA_IFNAME **array_mem_ifs;         //  Array of output interfaces' names
    PPA_IFINFO *array_mem_ifs;
    uint8_t     replace_src_mac;        //  Whether to replace source MAC address
    uint8_t     src_mac[PPA_ETH_ALEN];  //  Source MAC address to replace
  } PPA_MC_SESSION;
#else
  typedef struct {
    PPA_IFNAME *ifname;
    uint8_t     ttl;
  } IF_TTL_ENTRY;

  typedef struct {
    IPADDR          ip_mc_group;
    int8_t          num_ifs;
    IF_TTL_ENTRY    array_mem_ifs[MAX_MC_IFS_NUM];
    uint8_t         if_mask;
  } PPA_MC_GROUP;
#endif

typedef void PPA_U_SESSION;

typedef struct {
    uint32_t    tx_pkts;
    uint32_t    rx_pkts;
    uint32_t    tx_discard_pkts;
    uint32_t    tx_error_pkts;
    uint32_t    rx_discard_pkts;
    uint32_t    rx_error_pkts;
    uint32_t    tx_bytes;
    uint32_t    rx_bytes;
} PPA_IF_STATS;

typedef struct {
    uint32_t    fast_tcp_pkt_tx;
    uint32_t    fast_udp_pkt_tx;
    uint32_t    reserved1;
    uint32_t    fast_udp_mcast_pkt_tx;
    uint32_t    fast_tcp_pkt_drop;
    uint32_t    fast_udp_pkt_drop;
    uint32_t    reserved2;
    uint32_t    reserved3;
    uint32_t    rx_pkt_error;
} PPA_ACCEL_STATS;

#if 0
typedef struct {
    uint32_t    filter_type;
    uint32_t    ip_src,      ip_dst;
    uint32_t    ip_src_mask, ip_dst_mask;
    uint8_t     src_mac[PPA_ETH_ALEN], dst_mac[PPA_ETH_ALEN];
    uint32_t    spi;
    uint32_t    l4_protocol;
} PPA_FILTER;

typedef union {
    struct {
        uint32_t    udp_remove;
        uint32_t    process_xxx;
    } ipsec_action;
    struct {
        uint32_t    rate;
        uint32_t    rate_method;
        uint32_t    drop;
    } firwall_action;
} PPA_ACTION;
#endif

/*
 *  ioctl command structures
 */

typedef struct {
    PPA_IFNAME  ifname[PPA_IF_NAME_SIZE];
    uint32_t    if_flags;
} PPA_CMD_IFINFO;

typedef struct {
    PPA_INIT_INFO   init_info;  //  p_lanifs, p_wanifs is index of first PPA_CMD_IFINFO item
    uint32_t        flags;
    PPA_CMD_IFINFO  ifinfo[20];
} PPA_CMD_INIT_INFO;

typedef struct {
    uint32_t        lan_rx_ppa_enable;
    uint32_t        wan_rx_ppa_enable;
    uint32_t        flags;
} PPA_CMD_ENABLE_INFO;

typedef struct {
    uint32_t        lan_rx_ppa_enable;
    uint32_t        wan_rx_ppa_enable;
} PPA_CMD_GET_STATUS_INFO;

typedef struct {
    PPA_IFNAME  ifname[PPA_IF_NAME_SIZE];
    uint8_t     mac[PPA_ETH_ALEN];
} PPA_CMD_IF_MAC_INFO;



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
  int32_t ifx_ppa_init(PPA_INIT_INFO *, uint32_t);
  void ifx_ppa_exit(void);

  int32_t ifx_ppa_enable(uint32_t, uint32_t, uint32_t);
  int32_t ifx_ppa_get_status(uint32_t *, uint32_t *);

  int32_t ifx_ppa_session_add(PPA_BUF *, PPA_SESSION *, uint32_t);
  int32_t ifx_ppa_session_delete(PPA_SESSION *, uint32_t flags);
  int32_t ifx_ppa_session_modify(PPA_SESSION *, PPA_SESSION_EXTRA *, uint32_t);
  int32_t ifx_ppa_session_get(PPA_SESSION ***, PPA_SESSION_EXTRA **, int32_t *, uint32_t);

 #if !defined(USE_NEW_MC_API) || !USE_NEW_MC_API
  int32_t ifx_ppa_mc_session_add(PPA_BUF *, PPA_MC_SESSION *, uint32_t);
  int32_t ifx_ppa_mc_session_delete(uint32_t, uint32_t);
  int32_t ifx_ppa_mc_session_modify(uint32_t, PPA_MC_SESSION *, PPA_SESSION_EXTRA *, uint32_t);
  int32_t ifx_ppa_all_mc_session_get(PPA_MC_SESSION ***, PPA_SESSION_EXTRA **, int32_t *, uint32_t);
  int32_t ifx_ppa_mc_session_get(uint32_t, PPA_MC_SESSION **, PPA_SESSION_EXTRA *, uint32_t);
 #else
  int32_t ifx_ppa_mc_group_update(PPA_MC_GROUP *, uint32_t);
  int32_t ifx_ppa_mc_group_get(IPADDR, PPA_MC_GROUP *, uint32_t);
 #endif

  int32_t ifx_ppa_set_session_inactivity(PPA_U_SESSION *, int32_t);
  int32_t ifx_ppa_inactivity_status(PPA_U_SESSION *);

  int32_t ifx_ppa_get_if_stats(PPA_IFNAME *, PPA_IF_STATS *, uint32_t);
  int32_t ifx_ppa_get_accel_stats(PPA_ACCEL_STATS *, PPA_ACCEL_STATS *, uint32_t);

  #if 0
    int32_t ifx_ppa_filter_add(PPA_FILTER *, PPA_ACTION *, uint32_t);
  #endif

  int32_t ifx_ppa_set_if_mac_address(PPA_IFNAME *, uint8_t *);
  int32_t ifx_ppa_get_if_mac_address(PPA_IFNAME *, uint8_t *);

  int32_t ifx_ppa_bridge_entry_add(uint8_t *, PPA_NETIF *, uint32_t);
  int32_t ifx_ppa_bridge_entry_delete(uint8_t *, uint32_t);
  int32_t ifx_ppa_set_bridge_entry_timeout(uint8_t *, uint32_t);
  int32_t ifx_ppa_bridge_entry_inactivity_status(uint8_t *);
  int32_t ifx_ppa_bridge_entry_hit_time(uint8_t *, uint32_t *);
#endif

#include <asm/danube/danube_ppa_hook.h>



#endif  //  __DANUBE_PPA_API_H__20060913_1556__
