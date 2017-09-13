#ifndef __DANUBE_PPA_PPE_HAL_H__20060914_1733__
#define __DANUBE_PPA_PPE_HAL_H__20060914_1733__



/******************************************************************************
**
** FILE NAME    : danube_ppa_ppe_hal.h
** PROJECT      : Twinpass-E
** MODULES     	: PPA (Routing Acceleration)
**
** DATE         : 12 SEP 2006
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA (Routing Acceleration) Firmware (D2) Operation API Header
**                File
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
** 14 SEP 2006  Xu Liang        Initiate Version
** 26 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/



/*
 * ####################################
 *              Definition
 * ####################################
 */

#define ENABLE_MODULE_D2                            0

#undef IFX_PPA_IF_NOT_FOUND
#undef IFX_PPA_IF_TYPE_LAN
#undef IFX_PPA_IF_TYPE_WAN
#undef IFX_PPA_IF_TYPE_MIX
#define IFX_PPA_IF_NOT_FOUND                        0
#define IFX_PPA_IF_TYPE_LAN                         1
#define IFX_PPA_IF_TYPE_WAN                         2
#define IFX_PPA_IF_TYPE_MIX                         3

#undef IFX_PPA_ACC_MODE_NONE
#undef IFX_PPA_ACC_MODE_ROUTING
#define IFX_PPA_ACC_MODE_NONE                       0
#define IFX_PPA_ACC_MODE_ROUTING                    2

#define IFX_PPA_SET_ROUTE_CFG_ENTRY_NUM             0x01
#define IFX_PPA_SET_ROUTE_CFG_MC_ENTRY_NUM          0x02
#define IFX_PPA_SET_ROUTE_CFG_IP_VERIFY             0x04
#define IFX_PPA_SET_ROUTE_CFG_TCPUDP_VERIFY         0x08
#define IFX_PPA_SET_ROUTE_CFG_TCPUDP_ERR_DROP       0x10
#define IFX_PPA_SET_ROUTE_CFG_DROP_ON_NOT_HIT       0x20
#define IFX_PPA_SET_ROUTE_CFG_MC_DROP_ON_NOT_HIT    0x40

#define IFX_PPA_ROUTE_TYPE_NULL                     0
#define IFX_PPA_ROUTE_TYPE_IPV4                     1
#define IFX_PPA_ROUTE_TYPE_NAT                      2
#define IFX_PPA_ROUTE_TYPE_NAPT                     3

#undef IFX_PPA_DEST_LIST_ETH0
#undef IFX_PPA_DEST_LIST_ETH1
#undef IFX_PPA_DEST_LIST_CPU0
#undef IFX_PPA_DEST_LIST_EXT_INT1
#undef IFX_PPA_DEST_LIST_EXT_INT2
#undef IFX_PPA_DEST_LIST_EXT_INT3
#undef IFX_PPA_DEST_LIST_EXT_INT4
#undef IFX_PPA_DEST_LIST_EXT_INT5
#define IFX_PPA_DEST_LIST_ETH0                      0x01
#define IFX_PPA_DEST_LIST_ETH1                      0x02
#define IFX_PPA_DEST_LIST_CPU0                      0x04
#define IFX_PPA_DEST_LIST_EXT_INT1                  0x08
#define IFX_PPA_DEST_LIST_EXT_INT2                  0x10
#define IFX_PPA_DEST_LIST_EXT_INT3                  0x20
#define IFX_PPA_DEST_LIST_EXT_INT4                  0x40
#define IFX_PPA_DEST_LIST_EXT_INT5                  0x80

#define IFX_PPA_PPPOE_MODE_TRANSPARENT              0
#define IFX_PPA_PPPOE_MODE_TERMINATION              1

#define IFX_PPA_UPDATE_ROUTING_ENTRY_ROUTE_TYPE     0x0001
#define IFX_PPA_UPDATE_ROUTING_ENTRY_NEW_IP         0x0002
#define IFX_PPA_UPDATE_ROUTING_ENTRY_NEW_PORT       0x0004
#define IFX_PPA_UPDATE_ROUTING_ENTRY_NEW_MAC        0x0008
#define IFX_PPA_UPDATE_ROUTING_ENTRY_MTU_IX         0x0010
#define IFX_PPA_UPDATE_ROUTING_ENTRY_NEW_DSCP_EN    0x0020
#define IFX_PPA_UPDATE_ROUTING_ENTRY_NEW_DSCP       0x0040
#define IFX_PPA_UPDATE_ROUTING_ENTRY_VLAN_INS_EN    0x0080
#define IFX_PPA_UPDATE_ROUTING_ENTRY_VLAN_IX        0x0100
#define IFX_PPA_UPDATE_ROUTING_ENTRY_VLAN_RM_EN     0x0200
#define IFX_PPA_UPDATE_ROUTING_ENTRY_PPPOE_MODE     0x0400
#define IFX_PPA_UPDATE_ROUTING_ENTRY_PPPOE_IX       0x0800
#define IFX_PPA_UPDATE_ROUTING_ENTRY_DEST_LIST      0x1000
#define IFX_PPA_UPDATE_ROUTING_ENTRY_DEST_CHID      0x2000

#define IFX_PPA_UPDATE_WAN_MC_ENTRY_VLAN_INS_EN     0x0001
#define IFX_PPA_UPDATE_WAN_MC_ENTRY_VLAN_IX         0x0002
#define IFX_PPA_UPDATE_WAN_MC_ENTRY_VLAN_RM_EN      0x0004
#define IFX_PPA_UPDATE_WAN_MC_ENTRY_SRC_MAC_EN      0x0008
#define IFX_PPA_UPDATE_WAN_MC_ENTRY_SRC_MAC_IX      0x0010
#define IFX_PPA_UPDATE_WAN_MC_ENTRY_DEST_LIST       0x0020
#define IFX_PPA_UPDATE_WAN_MC_ENTRY_DEST_CHID       0x0040

#define IFX_PPA_ADD_MAC_ENTRY_PPPOE                 0x01
#define IFX_PPA_ADD_MAC_ENTRY_LAN                   0x02
#define IFX_PPA_ADD_MAC_ENTRY_WAN                   0x00

#undef IFX_PPA_SET_FAST_MODE_CPU1
#undef IFX_PPA_SET_FAST_MODE_ETH1
#define IFX_PPA_SET_FAST_MODE_CPU1                  0x01
#define IFX_PPA_SET_FAST_MODE_ETH1                  0x02

#undef IFX_PPA_SET_FAST_MODE_CPU1_DIRECT
#undef IFX_PPA_SET_FAST_MODE_CPU1_INDIRECT
#undef IFX_PPA_SET_FAST_MODE_ETH1_DIRECT
#undef IFX_PPA_SET_FAST_MODE_ETH1_INDIRECT
#define IFX_PPA_SET_FAST_MODE_CPU1_DIRECT           IFX_PPA_SET_FAST_MODE_CPU1
#define IFX_PPA_SET_FAST_MODE_CPU1_INDIRECT         0
#define IFX_PPA_SET_FAST_MODE_ETH1_DIRECT           IFX_PPA_SET_FAST_MODE_ETH1
#define IFX_PPA_SET_FAST_MODE_ETH1_INDIRECT         0



/*
 * ####################################
 *              Data Type
 * ####################################
 */



/*
 * ####################################
 *             Declaration
 * ####################################
 */

#if !defined(ENABLE_MODULE_D2) || !ENABLE_MODULE_D2
  extern void ifx_ppa_ppe_hal_init(void);
  extern void ifx_ppa_ppe_hal_exit(void);
#endif

#ifdef __KERNEL__
  void get_firmware_id(uint32_t *p_family,
                       uint32_t *p_type,
                       uint32_t *p_if,
                       uint32_t *p_mode,
                       uint32_t *p_major,
                       uint32_t *p_minor);

  void get_max_route_entries(uint32_t *p_entry,
                             uint32_t *p_mc_entry);

  void set_etx1_dmach_on(uint32_t ch_on,
                         uint32_t ch_mask);

  void set_vlan_id(uint32_t f_is_lan,
                   uint32_t vlan_id);

  void set_if_type(uint32_t if_type,  //  1: LAN, 2: WAN, 3:MIX
                   uint32_t if_no);

  uint32_t get_if_type(uint32_t if_no);

  void set_route_cfg(uint32_t f_is_lan,
                     uint32_t entry_num,
                     uint32_t mc_entry_num,
                     uint32_t f_ip_verify,
                     uint32_t f_tcpudp_verify,
                     uint32_t f_tcpudp_err_drop,
                     uint32_t f_drop_on_no_hit,
                     uint32_t f_mc_drop_on_no_hit,
                     uint32_t flags); //  bit 0: entry_num is valid
                                      //  bit 1: mc_entry_num is valid
                                      //  bit 2: f_ip_verify is valid
                                      //  bit 3: f_tcpudp_verify is valid
                                      //  bit 4: f_tcpudp_err_drop is valid
                                      //  bit 5: f_drop_on_no_hit is valid
                                      //  bit 6: f_mc_drop_on_no_hit is valid

  void set_if_wfq(uint32_t if_wfq,
                  uint32_t if_no);

  void set_dplus_wfq(uint32_t wfq);

  void get_acc_mode(uint32_t f_is_lan,
                    uint32_t *p_acc_mode);

  void set_acc_mode(uint32_t f_is_lan,
                    uint32_t acc_mode);   //  0: none, 2: routing

  int32_t add_routing_entry(uint32_t f_is_lan,
                            uint32_t src_ip,
                            uint32_t src_port,
                            uint32_t dst_ip,
                            uint32_t dst_port,
                            uint32_t route_type,
                            uint32_t new_ip,
                            uint32_t new_port,
                            uint8_t  new_mac[6],
                            uint32_t mtu_ix,
                            uint32_t f_new_dscp_enable,
                            uint32_t new_dscp,
                            uint32_t f_vlan_ins_enable,
                            uint32_t vlan_ix,
                            uint32_t f_vlan_rm_enable,
                            uint32_t pppoe_mode,
                            uint32_t pppoe_ix,
                            uint32_t dest_list,
                            uint32_t dest_chid,
                            uint32_t *p_entry);

  void del_routing_entry(uint32_t entry);

  int32_t update_routing_entry(uint32_t entry,
                               uint32_t route_type,
                               uint32_t new_ip,
                               uint32_t new_port,
                               uint8_t  new_mac[6],
                               uint32_t mtu_ix,
                               uint32_t f_new_dscp_enable,
                               uint32_t new_dscp,
                               uint32_t f_vlan_ins_enable,
                               uint32_t vlan_ix,
                               uint32_t f_vlan_rm_enable,
                               uint32_t pppoe_mode,
                               uint32_t pppoe_ix,
                               uint32_t dest_list,
                               uint32_t dest_chid,
                               uint32_t flags);

  int32_t add_wan_mc_entry(uint32_t dest_ip_compare,
                           uint32_t f_vlan_ins_enable,
                           uint32_t vlan_ix,
                           uint32_t f_vlan_rm_enable,
                           uint32_t f_src_mac_enable,
                           uint32_t src_mac_ix,
                           uint32_t pppoe_mode,
                           uint32_t dest_list,
                           uint32_t dest_chid,
                           uint32_t *p_entry);

  void del_wan_mc_entry(uint32_t entry);

  int32_t update_wan_mc_entry(uint32_t entry,
                              uint32_t f_vlan_ins_enable,
                              uint32_t vlan_ix,
                              uint32_t f_vlan_rm_enable,
                              uint32_t f_src_mac_enable,
                              uint32_t src_mac_ix,
                              uint32_t pppoe_mode,
                              uint32_t dest_list,
                              uint32_t dest_chid,
                              uint32_t flags);

  int32_t get_dest_ip_from_wan_mc_entry(uint32_t entry,
                                        uint32_t *p_ip);

  int32_t add_vlan_entry(uint32_t vci,
                         uint32_t *p_entry);

  void del_vlan_entry(uint32_t entry);

  int32_t get_vlan_entry(uint32_t entry,
                         uint32_t *p_vci);

  #if 0

    int32_t add_mac_entry(uint8_t mac[6],
                          uint32_t session_id,
                          uint32_t *p_entry,
                          uint32_t flags);

    void del_mac_entry(uint32_t entry,
                       uint32_t flags);

    int32_t get_mac_entry(uint32_t entry,
                          uint8_t lan_mac[6],
                          uint8_t wan_mac[6],
                          uint32_t *p_session_id);

  #else

    int32_t add_pppoe_entry(uint32_t session_id,
                            uint32_t *p_entry);

    void del_pppoe_entry(uint32_t entry);

    int32_t get_pppoe_entry(uint32_t entry,
                            uint32_t *p_session_id);

  #endif

  int32_t add_mtu_entry(uint32_t mtu_size,
                        uint32_t *p_entry);

  void del_mtu_entry(uint32_t entry);

  int32_t get_mtu_entry(uint32_t entry,
                        uint32_t *p_mtu_size);

  void get_lan_rx_mib(uint32_t *p_fast_tcp_pkts,
                      uint32_t *p_fast_udp_pkts,
                      uint32_t *p_fast_drop_tcp_pkts,
                      uint32_t *p_fast_drop_udp_pkts,
                      uint32_t *p_drop_pkts);

  void get_wan_rx_mib(uint32_t *p_fast_uc_tcp_pkts,
                      uint32_t *p_fast_uc_udp_pkts,
                      uint32_t *p_fast_mc_tcp_pkts,
                      uint32_t *p_fast_mc_udp_pkts,
                      uint32_t *p_fast_drop_tcp_pkts,
                      uint32_t *p_fast_drop_udp_pkts,
                      uint32_t *p_drop_pkts);

  uint32_t test_and_clear_hit_stat(uint32_t entry);

  uint32_t test_and_clear_mc_hit_stat(uint32_t entry);

  void set_fast_mode(uint32_t mode,
                     uint32_t flags);

  void set_mac_table(uint8_t lan_mac[6],
                     uint8_t wan_mac[6]);

  void get_mac_table(uint8_t lan_mac[6],
                     uint8_t wan_mac[6]);
#endif  //  __KERNEL__



#endif  //  __DANUBE_PPA_PPE_HAL_H__20060914_1733__
