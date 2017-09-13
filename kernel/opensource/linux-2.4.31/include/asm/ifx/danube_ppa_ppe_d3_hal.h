#ifndef __DANUBE_PPA_PPE_D3_HAL_H__20061109_1512__
#define __DANUBE_PPA_PPE_D3_HAL_H__20061109_1512__



/******************************************************************************
**
** FILE NAME    : danube_ppa_ppe_d3_hal.h
** PROJECT      : Twinpass-E
** MODULES     	: PPA (3-port bridging Acceleration)
**
** DATE         : 9 NOV 2006
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA (3-port bridging Acceleration) Firmware (D3) Operation API
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
**  9 NOV 2006  Xu Liang        Initiate Version
*******************************************************************************/



/*
 * ####################################
 *              Definition
 * ####################################
 */

#define ENABLE_MODULE_D3                            0

#undef IFX_PPA_IF_NOT_FOUND
#undef IFX_PPA_IF_TYPE_LAN
#undef IFX_PPA_IF_TYPE_WAN
#undef IFX_PPA_IF_TYPE_MIX
#define IFX_PPA_IF_NOT_FOUND                        0
#define IFX_PPA_IF_TYPE_LAN                         1
#define IFX_PPA_IF_TYPE_WAN                         2
#define IFX_PPA_IF_TYPE_MIX                         3

#undef IFX_PPA_ACC_MODE_NONE
#undef IFX_PPA_ACC_MODE_BRIDGING
#define IFX_PPA_ACC_MODE_NONE                       0
#define IFX_PPA_ACC_MODE_BRIDGING                   2

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

#define IFX_PPA_PORT_ETH0                           0x00
#define IFX_PPA_PORT_ETH1                           0x01
#define IFX_PPA_PORT_ANY                            0x02


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

#if !defined(ENABLE_MODULE_D3) || !ENABLE_MODULE_D3
  extern void ifx_ppa_ppe_d3_hal_init(void);
  extern void ifx_ppa_ppe_d3_hal_exit(void);
#endif

#ifdef __KERNEL__
  void get_bridging_firmware_id(uint32_t *p_family,
                                uint32_t *p_type,
                                uint32_t *p_if,
                                uint32_t *p_mode,
                                uint32_t *p_major,
                                uint32_t *p_minor);

  void get_max_bridging_entries(uint32_t *p_entry);

  void set_bridging_etx1_dmach_on(uint32_t ch_on,
                                  uint32_t ch_mask);

  void set_bridging_fast_mode(uint32_t mode,
                              uint32_t flags);

  void set_bridging_if_wfq(uint32_t if_wfq,
                           uint32_t if_no);

  void set_fastpath_wfq(uint32_t wfq);

  void get_bridging_acc_mode(uint32_t f_is_eth0,
                           uint32_t *p_acc_mode);

  void set_bridging_acc_mode(uint32_t f_is_eth0,
                           uint32_t acc_mode);

  void get_bridging_entry_num(uint32_t *p_entry_num);

  void set_bridging_entry_num(uint32_t entry_num);

  void get_bridging_mac_change_drop(uint32_t f_is_eth0,
                                    uint32_t *pf_drop_enable);

  void set_bridging_mac_change_drop(uint32_t f_is_eth0,
                                    uint32_t f_drop_enable);

  void get_bridging_broadcast_dest_list(uint32_t f_is_eth0,
                                        uint32_t *p_dest_list);

  void set_bridging_broadcast_dest_list(uint32_t f_is_eth0,
                                        uint32_t dest_list);

  void get_bridging_unknown_mc_dest_list(uint32_t f_is_eth0,
                                         uint32_t *p_dest_list);

  void set_bridging_unknown_mc_dest_list(uint32_t f_is_eth0,
                                         uint32_t dest_list);

  void get_bridging_unknown_uc_dest_list(uint32_t f_is_eth0,
                                         uint32_t *p_dest_list);

  void set_bridging_unknown_uc_dest_list(uint32_t f_is_eth0,
                                         uint32_t dest_list);

  void get_bridging_eth_mib(uint32_t f_is_eth0,
                            uint32_t *p_rx_fast_pkts,
                            uint32_t *p_rx_fast_bytes,
                            uint32_t *p_rx_cpu_pkts,
                            uint32_t *p_rx_cpu_bytes,
                            uint32_t *p_rx_drop_pkts,
                            uint32_t *p_rx_drop_bytes,
                            uint32_t *p_tx_fast_pkts,
                            uint32_t *p_tx_fast_bytes);

  uint32_t test_and_clear_bridging_hit_stat(uint32_t entry);

  int32_t add_bridging_entry(uint32_t port,
                             uint8_t  mac[6],
                             uint32_t dest_list,
                             uint32_t dest_chid,
                             uint32_t *p_entry);

  void del_bridging_entry(uint32_t entry);
#endif



#endif  //  __DANUBE_PPA_PPE_D3_HAL_H__20061109_1512__
