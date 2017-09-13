#ifndef __DANUBE_ETH2_H__2005_08_04__11_23__
#define __DANUBE_ETH2_H__2005_08_04__11_23__


/******************************************************************************
**
** FILE NAME    : danube_eth2.h
** PROJECT      : Danube
** MODULES     	: Second ETH Interface (MII1)
**
** DATE         : 4 AUG 2005
** AUTHOR       : Xu Liang
** DESCRIPTION  : Second ETH Interface (MII1) Driver Header File
** COPYRIGHT    : 	Copyright (c) 2006
**			Infineon Technologies AG
**			Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
**  4 AUG 2005  Xu Liang        Initiate Version
** 23 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/


/*
 * ####################################
 *              Definition
 * ####################################
 */

/*
 *  ioctl Command
 */
#define SET_ETH_SPEED_AUTO              SIOCDEVPRIVATE
#define SET_ETH_SPEED_10                SIOCDEVPRIVATE + 1
#define SET_ETH_SPEED_100               SIOCDEVPRIVATE + 2
#define SET_ETH_DUPLEX_AUTO             SIOCDEVPRIVATE + 3
#define SET_ETH_DUPLEX_HALF             SIOCDEVPRIVATE + 4
#define SET_ETH_DUPLEX_FULL             SIOCDEVPRIVATE + 5
#define SET_ETH_REG                     SIOCDEVPRIVATE + 6
#define VLAN_TOOLS                      SIOCDEVPRIVATE + 7
#define MAC_TABLE_TOOLS                 SIOCDEVPRIVATE + 8
#define SET_VLAN_COS                    SIOCDEVPRIVATE + 9
#define SET_DSCP_COS                    SIOCDEVPRIVATE + 10
#define ENABLE_VLAN_CLASSIFICATION      SIOCDEVPRIVATE + 11
#define DISABLE_VLAN_CLASSIFICATION     SIOCDEVPRIVATE + 12
#define VLAN_CLASS_FIRST                SIOCDEVPRIVATE + 13
#define VLAN_CLASS_SECOND               SIOCDEVPRIVATE + 14
#define ENABLE_DSCP_CLASSIFICATION      SIOCDEVPRIVATE + 15
#define DISABLE_DSCP_CLASSIFICATION     SIOCDEVPRIVATE + 16
#define PASS_UNICAST_PACKETS            SIOCDEVPRIVATE + 17
#define FILTER_UNICAST_PACKETS          SIOCDEVPRIVATE + 18
#define KEEP_BROADCAST_PACKETS          SIOCDEVPRIVATE + 19
#define DROP_BROADCAST_PACKETS          SIOCDEVPRIVATE + 20
#define KEEP_MULTICAST_PACKETS          SIOCDEVPRIVATE + 21
#define DROP_MULTICAST_PACKETS          SIOCDEVPRIVATE + 22


/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*
 *  Data Type Used to Call ioctl
 */
struct vlan_cos_req {
    int     pri;
    int     cos_value;
};

struct dscp_cos_req {
    int     dscp;
    int     cos_value;
};


/*
 * ####################################
 *             Declaration
 * ####################################
 */

#if defined(__KERNEL__)
#endif  //  defined(__KERNEL__)


#endif  //  __DANUBE_ETH2_H__2005_08_04__11_23__
