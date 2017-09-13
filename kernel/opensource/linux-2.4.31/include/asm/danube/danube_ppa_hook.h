#ifndef __DANUBE_PPA_HOOK_HAL_H__20061109_1613__
#define __DANUBE_PPA_HOOK_HAL_H__20061109_1613__



/******************************************************************************
**
** FILE NAME    : danube_ppa_hook.h
** PROJECT      : Twinpass-E
** MODULES     	: PPA (Routing Acceleration)
**
** DATE         : 9 NOV 2006
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA (Routing Acceleration) Protocol Stack Hook Pointers
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
 *             Declaration
 * ####################################
 */

#if 0
  typedef int32_t (*ppa_hook_session_add_cb)(PPA_BUF *, PPA_SESSION *, uint32_t);
  typedef int32_t (*ppa_hook_session_del_cb)(PPA_SESSION *, uint32_t);
  typedef int32_t (*ppa_hook_inactivity_status_cb)(PPA_U_SESSION *);
  typedef int32_t (*ppa_hook_set_inactivity_cb)(PPA_U_SESSION*, int32_t);
  typedef int32_t (*ppa_hook_mc_session_add_cb)(PPA_BUF *, PPA_MC_SESSION *, uint32_t);
  typedef int32_t (*ppa_hook_mc_session_del_cb)(uint32_t, uint32_t);

  extern ppa_hook_session_add_cb ppa_hook_session_add_fn;
  extern ppa_hook_session_del_cb ppa_hook_session_del_fn;
  extern ppa_hook_inactivity_status_cb ppa_hook_inactivity_status_fn;
  extern ppa_hook_set_inactivity_cb ppa_hook_set_inactivity_fn;
//  extern ppa_hook_mc_session_add_cb ppa_hook_mc_session_add_fn;
//  extern ppa_hook_mc_session_del_cb ppa_hook_mc_session_del_fn;
#endif

#ifdef __KERNEL__
  extern int32_t (*ppa_hook_init_fn)(PPA_INIT_INFO *, uint32_t);
  extern void (*ppa_hook_exit_fn)(void);

  extern int32_t (*ppa_hook_enable_fn)(uint32_t, uint32_t, uint32_t);
  extern int32_t (*ppa_hook_get_status_fn)(uint32_t *, uint32_t *);

  extern int32_t (*ppa_hook_session_add_fn)(PPA_BUF *, PPA_SESSION *, uint32_t);
  extern int32_t (*ppa_hook_session_del_fn)(PPA_SESSION *, uint32_t);
  extern int32_t (*ppa_hook_session_modify_fn)(PPA_SESSION *, PPA_SESSION_EXTRA *, uint32_t);
  extern int32_t (*ppa_hook_session_get_fn)(PPA_SESSION ***, PPA_SESSION_EXTRA **, int32_t *, uint32_t);

 #if !defined(USE_NEW_MC_API) || !USE_NEW_MC_API
  extern int32_t (*ppa_hook_mc_session_add_fn)(PPA_BUF *, PPA_MC_SESSION *, uint32_t);
  extern int32_t (*ppa_hook_mc_session_del_fn)(uint32_t, uint32_t);
 #else
  extern int32_t (*ppa_hook_mc_group_update_fn)(PPA_MC_GROUP *, uint32_t);
  extern int32_t (*ppa_hook_mc_group_get_fn)(IPADDR, PPA_MC_GROUP *, uint32_t);
 #endif

  extern int32_t (*ppa_hook_inactivity_status_fn)(PPA_U_SESSION *);
  extern int32_t (*ppa_hook_set_inactivity_fn)(PPA_U_SESSION*, int32_t);

  extern int32_t (*ppa_hook_get_if_stats_fn)(PPA_IFNAME *, PPA_IF_STATS *, uint32_t);
  extern int32_t (*ppa_hook_get_accel_stats_fn)(PPA_ACCEL_STATS *, PPA_ACCEL_STATS *, uint32_t);

  extern int32_t (*ppa_hook_set_if_mac_address_fn)(PPA_IFNAME *, uint8_t *);
  extern int32_t (*ppa_hook_get_if_mac_address_fn)(PPA_IFNAME *, uint8_t *);

  extern int32_t (*ppa_hook_bridge_entry_add_fn)(uint8_t *, PPA_NETIF *, uint32_t);
  extern int32_t (*ppa_hook_bridge_entry_delete_fn)(uint8_t *, uint32_t);
  extern int32_t (*ppa_hook_set_bridge_entry_timeout_fn)(uint8_t *, uint32_t);
  extern int32_t (*ppa_hook_bridge_entry_inactivity_status_fn)(uint8_t *);
  extern int32_t (*ppa_hook_bridge_entry_hit_time_fn)(uint8_t *, uint32_t *);
#endif



#endif  //  __DANUBE_PPA_HOOK_HAL_H__20061109_1613__
