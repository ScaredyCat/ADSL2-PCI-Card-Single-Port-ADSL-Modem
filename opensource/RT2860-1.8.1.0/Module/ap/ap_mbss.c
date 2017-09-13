/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    ap_mbss.c

    Abstract:
    Support multi-BSS function.

    Note:
    1. Call RT28xx_MBSS_Init() in init function and
       call RT28xx_MBSS_Remove() in close function

    2. MAC of different BSS is initialized in APStartUp()

    3. BSS index (0 ~ 15) of different rx packet is got in
       APHandleRxDoneInterrupt() by using FromWhichBSSID = pEntry->apidx;
       Or FromWhichBSSID = BSS0;

    4. BSS index (0 ~ 15) of different tx packet is assigned in
       MBSS_VirtualIF_PacketSend() by using RTMP_SET_PACKET_NET_DEVICE_MBSSID()
    5. BSS index (0 ~ 15) of different BSS is got in APHardTransmit() by using
       RTMP_GET_PACKET_IF()

    6. BSS index (0 ~ 15) of IOCTL command is put in pAd->OS_Cookie->ioctl_if

    7. Beacon of different BSS is enabled in APMakeAllBssBeacon() by writing 1
       to the register MAC_BSSID_DW1

    8. The number of MBSS can be 1, 2, 4, or 8

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Sample Lin  01-02-2007      created
*/

#define MODULE_MBSS
#include "rt_config.h"

#ifdef MBSS_SUPPORT

/* extern function prototype */
#if 0
INT rt28xx_ioctl(
	IN	PNET_DEV			net_dev, 
	IN	OUT	struct ifreq	*rq, 
	IN	INT					cmd);
#endif

/* local function prototype */
static INT MBSS_VirtualIF_Open(
	IN	PNET_DEV			dev_p);
static INT MBSS_VirtualIF_Close(
	IN	PNET_DEV			dev_p);
static INT MBSS_VirtualIF_PacketSend(
	IN PNDIS_PACKET			skb_p,
	IN PNET_DEV				dev_p);
static INT MBSS_VirtualIF_Ioctl(
	IN PNET_DEV				dev_p, 
	IN OUT struct ifreq 	*rq_p, 
	IN INT cmd);
#if 0 /* yet implement */
static struct net_device_stats *MBSS_VirtualIF_EtherStats_Get(
    IN PNET_DEV				dev_p);
#endif





/* --------------------------------- Public -------------------------------- */
/*
========================================================================
Routine Description:
    Init Multi-BSS function.

Arguments:
    ad_p            points to our adapter
    main_dev_p      points to the main BSS network interface

Return Value:
    None

Note:
	1. Only create and initialize virtual network interfaces.
	2. No main network interface here.
	3. If you down ra0 and modify the BssNum of RT2860AP.dat/RT2870AP.dat, it will
	   not work! You must rmmod rt2860ap.ko and lsmod rt2860ap.ko again.
========================================================================
*/
VOID RT28xx_MBSS_Init(
	IN PRTMP_ADAPTER 		ad_p,
	IN PNET_DEV				main_dev_p)
{
#define MBSS_MAX_DEV_NUM    32
	PNET_DEV cur_dev_p;
	PNET_DEV new_dev_p;
    VIRTUAL_ADAPTER *mbss_ad_p;
	CHAR slot_name[IFNAMSIZ];
	INT bss_index, max_bss_num;
	INT index = 0;


	/* sanity check to avoid redundant virtual interfaces are created */
	if (ad_p->flg_mbss_init != FALSE)
		return;
	/* End of if */
	ad_p->flg_mbss_init = TRUE;

	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));


    /* init */
    max_bss_num = ad_p->ApCfg.BssidNum;
	if (max_bss_num > MAX_MBSSID_NUM)
		max_bss_num = MAX_MBSSID_NUM;
	/* End of if */
    DBGPRINT(RT_DEBUG_INFO, ("mbss> num = %d\n", max_bss_num));

	/* first bss_index must not be 0 (BSS0), must be 1 (BSS1) */
	for(bss_index=FIRST_MBSSID; bss_index<MAX_MBSSID_NUM; bss_index++)
		ad_p->ApCfg.MBSSID[bss_index].MSSIDDev = NULL;
	/* End of for */


    /* create virtual network interface */
	for(bss_index=FIRST_MBSSID; bss_index<max_bss_num; bss_index++)
	{
        /* allocate a new network device */
#if LINUX_VERSION_CODE <= 0x20402 // Red Hat 7.1
		new_dev_p = alloc_netdev(sizeof(VIRTUAL_ADAPTER), "eth%d", ether_setup);
#else
		new_dev_p = alloc_etherdev(sizeof(VIRTUAL_ADAPTER));
#endif // LINUX_VERSION_CODE //

        if (new_dev_p == NULL)
        {
			/* allocation fail, exit */
            ad_p->ApCfg.BssidNum = bss_index; /* re-assign new MBSS number */
			DBGPRINT(RT_DEBUG_ERROR,
                     ("Allocate network device fail (MBSS)...\n"));
            break;
        } /* End of if */


        /* find an available interface name, max 32 ra interfaces */
		for(index=0; index<MBSS_MAX_DEV_NUM; index++)
		{
#ifdef MULTIPLE_CARD_SUPPORT
			if (ad_p->MC_RowID >= 0)
	            sprintf(slot_name, "ra%02d_%d", ad_p->MC_RowID, index);
			else
#endif // MULTIPLE_CARD_SUPPORT //
			sprintf(slot_name, "ra%d", index);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			cur_dev_p = dev_get_by_name(slot_name);
#else
			for(cur_dev_p=dev_base; cur_dev_p!=NULL; cur_dev_p=cur_dev_p->next)
			{
				if (strncmp(cur_dev_p->name, slot_name, 6) == 0)
					break;
				/* End of if */
			} /* End of for */
#endif // LINUX_VERSION_CODE //

			if (cur_dev_p == NULL)
                break; /* fine, the RA name is not used */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			else
            {
                /* every time dev_get_by_name is called, and it has returned a
                   valid PNET_DEV *, dev_put should be called
                   afterwards, because otherwise the machine hangs when the
                   device is unregistered (since dev->refcnt > 1). */
				dev_put(cur_dev_p);
            } /* End of if */
#endif // LINUX_VERSION_CODE //
		} /* End of for */

        /* assign interface name to the new network interface */
		if (index < MBSS_MAX_DEV_NUM)
		{
#ifdef MULTIPLE_CARD_SUPPORT
			if (ad_p->MC_RowID >= 0)
			{
	            sprintf(new_dev_p->name, "ra%02d_%d", ad_p->MC_RowID, index);
	            DBGPRINT(RT_DEBUG_TRACE, ("Register MBSSID IF (ra%02d-%d)\n", ad_p->MC_RowID, index));
			}
			else
#endif // MULTIPLE_CARD_SUPPORT //
			{
            sprintf(new_dev_p->name, "ra%d", index);
            DBGPRINT(RT_DEBUG_TRACE, ("Register MBSSID IF (ra%d)\n", index));
		}
		}
		else
		{
			/* error! no any available ra name can be used! */
            ad_p->ApCfg.BssidNum = bss_index; /* re-assign new MBSS number */
			DBGPRINT(RT_DEBUG_ERROR,
                     ("Has %d RA interfaces (MBSS)...\n", MBSS_MAX_DEV_NUM));
            kfree(new_dev_p);;
            break;
		} /* End of if */


        /* init the new network interface */
		ether_setup(new_dev_p);

		mbss_ad_p = new_dev_p->priv; /* sizeof(priv) = sizeof(VIRTUAL_ADAPTER) */
		mbss_ad_p->VirtualDev = new_dev_p;  /* 4B */
		mbss_ad_p->RtmpDev    = main_dev_p; /* 4B */

		/* init MAC address of virtual network interface */
		NdisMoveMemory(&new_dev_p->dev_addr,
                       ad_p->ApCfg.MBSSID[bss_index].Bssid,
                       MAC_ADDR_LEN);

		/* init operation functions */
		new_dev_p->open             = MBSS_VirtualIF_Open;
		new_dev_p->stop             = MBSS_VirtualIF_Close;
		new_dev_p->hard_start_xmit  = (HARD_START_XMIT_FUNC)MBSS_VirtualIF_PacketSend;
		new_dev_p->do_ioctl         = MBSS_VirtualIF_Ioctl;
        /* if you dont implement get_stats, dont assign your function with empty
		   body; or kernel will panic */
//        new_dev_p->get_stats        = MBSS_VirtualIF_EtherStats_Get;
		new_dev_p->priv_flags		= INT_MBSSID; /* we are virtual interface */

		/* register this device to OS */
		register_netdevice(new_dev_p);

		/* backup our virtual network interface */
        ad_p->ApCfg.MBSSID[bss_index].MSSIDDev = new_dev_p;
	} /* End of for */

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
}


/*
========================================================================
Routine Description:
    Close Multi-BSS network interface.

Arguments:
    ad_p            points to our adapter

Return Value:
    None

Note:
    FIRST_MBSSID = 1
    Main BSS is not closed here.
========================================================================
*/
VOID RT28xx_MBSS_Close(
	IN PRTMP_ADAPTER 	ad_p)
{
	UINT bss_index;


	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	for(bss_index=FIRST_MBSSID; bss_index<MAX_MBSSID_NUM; bss_index++)
	{
		if (ad_p->ApCfg.MBSSID[bss_index].MSSIDDev)
			dev_close(ad_p->ApCfg.MBSSID[bss_index].MSSIDDev);
	}

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
}


/*
========================================================================
Routine Description:
    Remove Multi-BSS network interface.

Arguments:
    ad_p            points to our adapter

Return Value:
    None

Note:
    FIRST_MBSSID = 1
    Main BSS is not removed here.
========================================================================
*/
VOID RT28xx_MBSS_Remove(
	IN PRTMP_ADAPTER 	ad_p)
{
	UINT bss_index;


	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	for(bss_index=FIRST_MBSSID; bss_index<MAX_MBSSID_NUM; bss_index++)
	{
		if (ad_p->ApCfg.MBSSID[bss_index].MSSIDDev)
        {
            unregister_netdev(ad_p->ApCfg.MBSSID[bss_index].MSSIDDev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
            free_netdev(ad_p->ApCfg.MBSSID[bss_index].MSSIDDev);
#else
		    kfree(ad_p->ApCfg.MBSSID[bss_index].MSSIDDev);
#endif // LINUX_VERSION_CODE //

			// Clear it as NULL to prevent latter access error.
			ad_p->ApCfg.MBSSID[bss_index].MSSIDDev = NULL;
		}
	}

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
} /* End of RT28xx_MBSS_Remove */



/* --------------------------------- Private -------------------------------- */
/*
========================================================================
Routine Description:
    Open a virtual network interface.

Arguments:
    dev_p           which WLAN network interface

Return Value:
    0: open successfully
    otherwise: open fail

Note:
========================================================================
*/
static INT MBSS_VirtualIF_Open(
	IN	PNET_DEV	dev_p)
{
    VIRTUAL_ADAPTER *virtual_ad_p = dev_p->priv;


	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> MBSSVirtualIF_open\n", dev_p->name));
	ASSERT(virtual_ad_p->VirtualDev);
    netif_start_queue(virtual_ad_p->VirtualDev);
    return 0;
} /* End of MBSS_VirtualIF_Open */


/*
========================================================================
Routine Description:
    Close a virtual network interface.

Arguments:
    dev_p           which WLAN network interface

Return Value:
    0: close successfully
    otherwise: close fail

Note:
========================================================================
*/
static INT MBSS_VirtualIF_Close(
	IN	PNET_DEV	dev_p)
{
	VIRTUAL_ADAPTER	*virtual_ad_p = dev_p->priv;


	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> MBSSVirtualIF_close\n", dev_p->name));
	ASSERT(virtual_ad_p->VirtualDev);
	netif_stop_queue(virtual_ad_p->VirtualDev);	
	return 0;
} /* End of MBSS_VirtualIF_Close */


/*
========================================================================
Routine Description:
    Send a packet to WLAN.

Arguments:
    skb_p           points to our adapter
    dev_p           which WLAN network interface

Return Value:
    0: transmit successfully
    otherwise: transmit fail

Note:
========================================================================
*/
static INT MBSS_VirtualIF_PacketSend(
	IN PNDIS_PACKET			skb_p, 
	IN PNET_DEV				dev_p)
{
    VIRTUAL_ADAPTER  *virtual_ad_p = dev_p->priv;
    RTMP_ADAPTER     *ad_p;
    MULTISSID_STRUCT *mbss_p;
    PNDIS_PACKET     pkt_p = (PNDIS_PACKET)skb_p;
    INT              bss_index;


	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

    /* sanity check */
	ASSERT(virtual_ad_p);
	ASSERT(virtual_ad_p->RtmpDev);
	ad_p = virtual_ad_p->RtmpDev->priv;
	ASSERT(ad_p);

#ifdef RALINK_ATE
    if (ad_p->ate.Mode != ATE_STOP)
    {
        RELEASE_NDIS_PACKET(ad_p, pkt_p, NDIS_STATUS_FAILURE);
        return 0;
    } /* End of if */
#endif // RALINK_ATE //

	if ((RTMP_TEST_FLAG(ad_p, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)) ||
		(RTMP_TEST_FLAG(ad_p, fRTMP_ADAPTER_RADIO_OFF))          ||
		(RTMP_TEST_FLAG(ad_p, fRTMP_ADAPTER_RESET_IN_PROGRESS)))
	{
		/* wlan is scanning/disabled/reset */
		RELEASE_NDIS_PACKET(ad_p, pkt_p, NDIS_STATUS_FAILURE);
		return 0;
	} /* End of if */

	if (!(virtual_ad_p->RtmpDev->flags & IFF_UP))
	{
		/* the interface is down */
		RELEASE_NDIS_PACKET(ad_p, pkt_p, NDIS_STATUS_FAILURE);
		return 0;
	} /* End of if */


    /* 0 is main BSS, dont handle it here */
    /* FIRST_MBSSID = 1 */
    mbss_p = ad_p->ApCfg.MBSSID;

    for(bss_index=FIRST_MBSSID; bss_index<ad_p->ApCfg.BssidNum; bss_index++)
    {
        /* find the device in our MBSS list */
        if (mbss_p[bss_index].MSSIDDev == dev_p)
        {
            /* ya! find it */
            ad_p->RalinkCounters.PendingNdisPacketCount ++;
            RTMP_SET_PACKET_SOURCE(skb_p, PKTSRC_NDIS);
            RTMP_SET_PACKET_MOREDATA(skb_p, FALSE);
            RTMP_SET_PACKET_NET_DEVICE_MBSSID(skb_p, bss_index);
            RTPKT_TO_OSPKT(skb_p)->dev = virtual_ad_p->RtmpDev;

            if (!(*(RTPKT_TO_OSPKT(skb_p)->data) & 0x01))
            {
                DBGPRINT(RT_DEBUG_INFO,
                         ("VirtualIFSendPackets(MBSSID) - unicast packet to "
                          "(ra%d)\n", bss_index));
            } /* End of if */

            /* transmit the packet */
            return rt28xx_packet_xmit(skb_p);
        } /* End of if */
    } /* End of for */


    /* can not find the BSS so discard the packet */
    DBGPRINT(RT_DEBUG_INFO,
             ("MBSSVirtualIFSendPackets - needn't to send or net_device not "
              "exist.\n"));
	RELEASE_NDIS_PACKET(ad_p, pkt_p, NDIS_STATUS_FAILURE);
	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
    return 0;
} /* End of MBSS_VirtualIF_PacketSend */


/*
========================================================================
Routine Description:
    IOCTL to WLAN.

Arguments:
    dev_p           which WLAN network interface
    rq_p            command information
    cmd             command ID

Return Value:
    0: IOCTL successfully
    otherwise: IOCTL fail

Note:
    SIOCETHTOOL     8946    New drivers use this ETHTOOL interface to
                            report link failure activity.
========================================================================
*/
static INT MBSS_VirtualIF_Ioctl(
	IN PNET_DEV				dev_p, 
	IN OUT struct ifreq 	*rq_p, 
	IN INT 					cmd)
{
	VIRTUAL_ADAPTER *virtual_ad_p;
	RTMP_ADAPTER *ad_p;


	/* sanity check */
    virtual_ad_p = dev_p->priv;
    ASSERT(virtual_ad_p);
	ASSERT(virtual_ad_p->RtmpDev);
    ad_p = virtual_ad_p->RtmpDev->priv;
    ASSERT(ad_p);

	if (!ad_p)
		return -EINVAL;
	
	if (!RTMP_TEST_FLAG(ad_p, fRTMP_ADAPTER_INTERRUPT_IN_USE))
		return -ENETDOWN;
	/* End of if */

	/* do real IOCTL */
	return rt28xx_ioctl(dev_p, rq_p, cmd);
} /* End of MBSS_VirtualIF_Ioctl */


#if 0
/*
========================================================================
Routine Description:
    IOCTL to WLAN.

Arguments:
    dev_p           which WLAN network interface
    rq_p            command information
    cmd             command ID

Return Value:
    0: IOCTL successfully
    otherwise: IOCTL fail

Note:
    1. not implement
    2. can not return NULL; Or kernel will crush.
========================================================================
*/
static struct net_device_stats *MBSS_VirtualIF_EtherStats_Get(
    IN  PNET_DEV	dev_p)
{
//	VIRTUAL_ADAPTER *virtual_ad_p = dev_p->priv;
//	RTMP_ADAPTER *ad_p = virtual_ad_p->RtmpDev->priv;


    DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));
    return NULL;
} /* End of MBSS_VirtualIF_EtherStats_Get */
#endif

#endif // MBSS_SUPPORT //

/* End of ap_mbss.c */
