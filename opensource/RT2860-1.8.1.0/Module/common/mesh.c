/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	mesh.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Fonchi		2007-06-25      For mesh (802.11s) support.
*/
#include "rt_config.h"


extern	const struct iw_handler_def rt73_iw_handler_def;

extern UCHAR OUI_WPA_NONE_AKM[4];
extern UCHAR OUI_WPA_WEP40[4];
extern UCHAR OUI_WPA_TKIP[4];
extern UCHAR OUI_WPA_CCMP[4];
extern UCHAR OUI_WPA2_WEP40[4];
extern UCHAR OUI_WPA2_TKIP[4];
extern UCHAR OUI_WPA2_CCMP[4];
extern UCHAR OUI_MSA_8021X[4];		// Not yet final - IEEE 802.11s-D1.06
extern UCHAR OUI_MSA_PSK[4];		// Not yet final - IEEE 802.11s-D1.06

typedef VOID (*MESH_ACT_FRAME_HANDLER_FUNC)(IN PRTMP_ADAPTER pAd, IN RX_BLK *pRxBlk);

typedef struct _MESH_ACTION_HANDLER
{
	UINT8 Category;
	UINT8 ActionCode;
	MESH_ACT_FRAME_HANDLER_FUNC pHandle;
} MESH_ACTION_HANDLER, *PMESH_ACTION_HANDLER;

static MESH_ACTION_HANDLER MeshActHandler[] = 
{
	// Peer Link Management.
	{CATEGORY_MESH_PEER_LINK, ACT_CODE_PEER_LINK_OPEN, (MESH_ACT_FRAME_HANDLER_FUNC)MeshPeerLinkOpenProcess},
	{CATEGORY_MESH_PEER_LINK, ACT_CODE_PEER_LINK_CONFIRM, (MESH_ACT_FRAME_HANDLER_FUNC)MeshPeerLinkConfirmProcess},
	{CATEGORY_MESH_PEER_LINK, ACT_CODE_PEER_LINK_CLOSE, (MESH_ACT_FRAME_HANDLER_FUNC)MeshPeerLinkCloseProcess},

	// HWMP.
	{CATEGORY_MESH_PATH_SELECTION, ACT_CODE_PATH_REQUEST, (MESH_ACT_FRAME_HANDLER_FUNC)MeshPreqRcvProcess},
	{CATEGORY_MESH_PATH_SELECTION, ACT_CODE_PATH_REPLY, (MESH_ACT_FRAME_HANDLER_FUNC)MeshPrepRcvProcess},
	{CATEGORY_MESH_PATH_SELECTION, ACT_CODE_PATH_ERROR, (MESH_ACT_FRAME_HANDLER_FUNC)MeshPerrRcvProcess},
	{CATEGORY_MESH_PATH_SELECTION, ACT_CODE_MULTIPATH_NOTICE,
															(MESH_ACT_FRAME_HANDLER_FUNC)MeshMultipathNoticeRcvProcess},

	{CATEGORY_MESH_RES_COORDINATION, RESOURCE_CHANNEL_SWITCH_ANNOUNCEMENT,
															(MESH_ACT_FRAME_HANDLER_FUNC)MeshChSwAnnounceProcess}
};
#define MESH_ACT_HANDLER_TAB_SIZE (sizeof(MeshActHandler) / sizeof(MESH_ACTION_HANDLER))


/* extern function prototype */
extern INT RTMPGetKeyParameter(
    IN  PCHAR   key,
    OUT PCHAR   dest,   
    IN  INT     destsize,
    IN  PCHAR   buffer);

/* local function prototype */
static INT Mesh_VirtualIF_Open(
	IN	PNET_DEV	dev_p);

static INT Mesh_VirtualIF_Close(
	IN	PNET_DEV	dev_p);

static INT Mesh_VirtualIF_PacketSend(
	IN PNDIS_PACKET		skb_p, 
	IN PNET_DEV			dev_p);

static INT Mesh_VirtualIF_Ioctl(
	IN PNET_DEV				dev_p,
	IN OUT struct ifreq 	*rq_p,
	IN INT cmd);

static VOID MeshCfgInit(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR		 pHostName);

static VOID MakeMeshRSNIE(
    IN  PRTMP_ADAPTER   pAd,
    IN  UINT            AuthMode,
    IN  UINT            WepStatus);

#if (WIRELESS_EXT >= 12)
struct iw_statistics *Mesh_VirtualIF_get_wireless_stats(
	IN  struct net_device *net_dev);
#endif


INT Set_MeshId_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT   success = FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	if(strlen(arg) <= MAX_MESH_ID_LEN)
	{
		NdisZeroMemory(pAd->MeshTab.MeshId, MAX_MESH_ID_LEN);
		NdisMoveMemory(pAd->MeshTab.MeshId, arg, strlen(arg));
		pAd->MeshTab.MeshIdLen = (UCHAR)strlen(arg);
		success = TRUE;

		DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) Set_MeshId_Proc::(Len=%d,MeshId=%s)\n",
			pAd->MeshTab.MeshIdLen, pAd->MeshTab.MeshId));
	
		MeshDown(pAd, TRUE);
		MeshUp(pAd);		
	}
	else
		success = FALSE;

	return success;
}

INT Set_MeshHostName_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT   success = FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	if(strlen(arg) <= MAX_HOST_NAME_LEN)
	{
		NdisZeroMemory(pAd->MeshTab.HostName, MAX_HOST_NAME_LEN);
		NdisMoveMemory(pAd->MeshTab.HostName, arg, strlen(arg));
		success = TRUE;

		DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) Set_MeshHostName_Proc::(HostName=%s)\n",
			pAd->MeshTab.HostName));
	}
	else
		success = FALSE;

	return success;
}

INT Set_MeshAutoLink_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	UINT Enable;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	Enable = simple_strtol(arg, 0, 16);

	pAd->MeshTab.MeshAutoLink = (Enable > 0) ? TRUE : FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("%s::(enable = %d)\n", __FUNCTION__, pAd->MeshTab.MeshAutoLink));
	
	return TRUE;
}

INT Set_MeshAddLink_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT i;
	CHAR *value;
	UCHAR PeerMac[ETH_LENGTH_OF_ADDRESS];	
	ULONG LinkIdx;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	if(strlen(arg) == 17)  //Mac address acceptable format 01:02:03:04:05:06 length 17
	{
		for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":"), i++) 
		{
			if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
				return FALSE;  //Invalid

			AtoH(value, &PeerMac[i], 1);
		}

		if(i != 6)
			return FALSE;  //Invalid
	}

	LinkIdx = GetMeshLinkId(pAd, PeerMac);
	if (LinkIdx == BSS_NOT_FOUND)
	{
		LinkIdx = MeshLinkAlloc(pAd, PeerMac, MESH_LINK_STATIC);
		if (LinkIdx == BSS_NOT_FOUND)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s() All Mesh-Links been occupied.\n", __FUNCTION__));
			return FALSE;
		}
	}

	if (!VALID_MESH_LINK_ID(LinkIdx))
		return FALSE;

	MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, MESH_LINK_MNG_ACTOPN, 0, NULL, LinkIdx);

	DBGPRINT(RT_DEBUG_TRACE, ("%s::(LinkIdx = %ld)\n", __FUNCTION__, LinkIdx));
	
	return TRUE;
}

INT Set_MeshDelLink_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT i;
	CHAR *value;
	UCHAR PeerMac[ETH_LENGTH_OF_ADDRESS];	
	ULONG LinkIdx;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	if(strlen(arg) == 17)  //Mac address acceptable format 01:02:03:04:05:06 length 17
	{
		for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":"), i++) 
		{
			if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
				return FALSE;  //Invalid

			AtoH(value, &PeerMac[i], 1);
		}

		if(i != 6)
			return FALSE;  //Invalid
	}

	LinkIdx = GetMeshLinkId(pAd, PeerMac);
	if (!VALID_MESH_LINK_ID(LinkIdx))
		return FALSE;

	pAd->MeshTab.MeshLink[LinkIdx].Entry.LinkType = MESH_LINK_DYNAMIC;
	MlmeEnqueueEx(pAd, MESH_LINK_MNG_STATE_MACHINE, MESH_LINK_MNG_CNCL, 0, NULL, LinkIdx);

	DBGPRINT(RT_DEBUG_TRACE, ("%s::(LinkIdx = %ld)\n", __FUNCTION__, LinkIdx));
	
	return TRUE;
}

INT Set_MeshMaxTxRate_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	UINT Rate;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	Rate = simple_strtol(arg, 0, 10);

	if (Rate <= 12)
		pAd->MeshTab.MeshMaxTxRate = Rate;
	else 
		DBGPRINT(RT_DEBUG_ERROR, ("%s::Wrong Tx Rate setting(%d), (0 ~ 12))\n", __FUNCTION__, Rate));

	DBGPRINT(RT_DEBUG_TRACE, ("%s::(Max Tx Rate = %ld)\n", __FUNCTION__, pAd->MeshTab.MeshMaxTxRate));
	
	return TRUE;
}

INT Set_MeshRouteAdd_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT	success = TRUE;
#if 0
	PUCHAR thisChar = NULL;
	UCHAR DestAddr[MAC_ADDR_LEN];
	UCHAR MeshDestAddr[MAC_ADDR_LEN];
	UCHAR NextHop[MAC_ADDR_LEN];
	UCHAR Addr[MAC_ADDR_LEN];
	ULONG Metric = 0;
	UCHAR i;
	PUCHAR value;
	UCHAR Count = 0;
	PMESH_ROUTING_ENTRY pEntry = NULL;
	PMAC_TABLE_ENTRY pMacEntry = NULL;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	while ((thisChar = strsep((char **)&arg, "-")) != NULL)
	{
		if (Count == 0)
		{
			// Get Dest Addr	
			if(strlen(thisChar) == 17)
			{
				for (i=0, value = rstrtok(thisChar,":"); value; value = rstrtok(NULL,":"))
				{
					if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
						return FALSE;  //Invalid

					AtoH(value, &Addr[i++], 2);
				}

				if(i != 6)
					return FALSE;  //Invalid
			}
			else
			{
				return FALSE;  //Invalid
			}

			COPY_MAC_ADDR(DestAddr, Addr);
		}
		else if (Count == 1)
		{
			// Get Mesh DA
			if(strlen(thisChar) == 17)
			{
				for (i=0, value = rstrtok(thisChar,":"); value; value = rstrtok(NULL,":"))
				{
					if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
						return FALSE;  //Invalid

					AtoH(value, &Addr[i++], 2);
				}

				if(i != 6)
					return FALSE;  //Invalid
			}
			else
			{
				return FALSE;	
			}

			COPY_MAC_ADDR(MeshDestAddr, Addr);
		}
		else if (Count == 2)
		{
			// Get Next Hop
			if(strlen(thisChar) == 17)
			{
				for (i=0, value = rstrtok(thisChar,":"); value; value = rstrtok(NULL,":"))
				{
					if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
						return FALSE;  //Invalid

					AtoH(value, &Addr[i++], 2);
				}

				if(i != 6)
					return FALSE;  //Invalid
			}
			else
			{
				return FALSE;	
			}

			COPY_MAC_ADDR(NextHop, Addr);
		}
		else if (Count == 3)
		{
			// Get Metric
			if ((thisChar != NULL) && (strlen(thisChar) > 0))
				Metric = simple_strtol(thisChar, 0, 10);
			else
				return FALSE;
		}
		else
		{
			return FALSE;
		}

		Count++;
	}

	if (MeshEntryTableLookUp(pAd, DestAddr))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("The Dest Mesh Point aleady on Route Table!!!\n"));
		return FALSE;
	}
	else
	{
		pMacEntry = MeshTableLookup(pAd, NextHop, TRUE);

		if (pMacEntry == NULL)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("Next Hop have not on Mac Table !!!\n"));
			return FALSE;
		}

		pEntry = MeshRoutingTableLookup(pAd, MeshDestAddr);
		if (!(pEntry && MAC_ADDR_EQUAL(pEntry->NextHop, NextHop)))
		{
			pEntry = MeshRoutingTableInsert(pAd, DestAddr, MeshDestAddr, 0, NextHop, pMacEntry->MatchMeshTabIdx, Metric);
			if (!pEntry)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("Add Dest Mesh Point Route fail !!!\n"));
				return FALSE;
			}
		}

		if (!MeshEntryTableInsert(pAd, DestAddr, pEntry->Idx))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("Add Dest Mesh Point Route fail !!!\n"));
			return FALSE;
		}

		if (MeshTableLookup(pAd, DestAddr, FALSE) == NULL)
		{
			MeshEntryTableInsert(pAd, MeshDestAddr, pEntry->Idx);
		}
	}
#endif
	return success;
}

INT Set_MeshRouteDelete_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT	success = TRUE;
#if 0
	PUCHAR value;
	UCHAR DestAddr[MAC_ADDR_LEN];
	UCHAR i;
	PMESH_ENTRY pMeshEntry = NULL;
	PMESH_ROUTING_ENTRY pRouteEntry = NULL;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	if(strlen(arg) == 17)
	{
		for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":"))
		{
			if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
				return FALSE;  //Invalid

			AtoH(value, &DestAddr[i++], 2);
		}

		if(i != 6)
			return FALSE;  //Invalid
	}
	else
	{
		return FALSE;  //Invalid
	}

	pMeshEntry = MeshEntryTableLookUp(pAd, DestAddr);
	pRouteEntry = MeshRoutingTableLookup(pAd, DestAddr);

	if (pMeshEntry && pRouteEntry)
		MeshRoutingTableDelete(pAd, DestAddr);
	else
	MeshEntryTableDelete(pAd, DestAddr);
#endif
	return success;
}

INT Set_MeshRouteUpdate_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT	success = TRUE;
#if 0
	PUCHAR thisChar = NULL;
	UCHAR DestAddr[MAC_ADDR_LEN];
	UCHAR MeshDestAddr[MAC_ADDR_LEN];
	UCHAR NextHop[MAC_ADDR_LEN];
	UCHAR Addr[MAC_ADDR_LEN];
	ULONG Metric = 0;
	UCHAR i;
	PUCHAR value;
	UCHAR Count = 0;
	PMESH_ENTRY pMeshEntry = NULL;
	PMESH_ROUTING_ENTRY pRouteEntry = NULL;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	while ((thisChar = strsep((char **)&arg, "-")) != NULL)
	{
		if (Count == 0)
		{
			// Get Dest Addr
			if(strlen(thisChar) == 17)
			{
				for (i=0, value = rstrtok(thisChar,":"); value; value = rstrtok(NULL,":"))
				{
					if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
						return FALSE;  //Invalid

					AtoH(value, &Addr[i++], 2);
				}

				if(i != 6)
					return FALSE;  //Invalid
			}
			else
			{
				return FALSE;  //Invalid
			}

			COPY_MAC_ADDR(DestAddr, Addr);
		}
		else if (Count == 1)
		{
			// Get Mesh DA
			if(strlen(thisChar) == 17)
			{
				for (i=0, value = rstrtok(thisChar,":"); value; value = rstrtok(NULL,":"))
				{
					if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
						return FALSE;  //Invalid

					AtoH(value, &Addr[i++], 2);
				}

				if(i != 6)
					return FALSE;  //Invalid
			}
			else
			{
				return FALSE;	
			}

			COPY_MAC_ADDR(MeshDestAddr, Addr);
		}
		else if (Count == 2)
		{
			// Get Next Hop
			if(strlen(thisChar) == 17)
			{
				for (i=0, value = rstrtok(thisChar,":"); value; value = rstrtok(NULL,":"))
				{
					if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
						return FALSE;  //Invalid

					AtoH(value, &Addr[i++], 2);
				}

				if(i != 6)
					return FALSE;  //Invalid
			}
			else
			{
				return FALSE;	
			}

			COPY_MAC_ADDR(NextHop, Addr);
		}
		else if (Count == 3)
		{
			// Get Metric
			if ((thisChar != NULL) && (strlen(thisChar) > 0))
				Metric = simple_strtol(thisChar, 0, 10);
			else
				return FALSE;
		}
		else
		{
			return FALSE;
		}

		Count++;
	}

	pMeshEntry = MeshEntryTableLookUp(pAd, DestAddr);
	pRouteEntry = MeshRoutingTableLookup(pAd, MeshDestAddr);
	
	if (pMeshEntry)
	{
		pRouteEntry = &pAd->MeshTab.MeshRouteTab.Content[pMeshEntry->Idx];
		
		COPY_MAC_ADDR(pRouteEntry->MeshDestAddr, MeshDestAddr);
		COPY_MAC_ADDR(pRouteEntry->NextHop, NextHop);
		pRouteEntry->PathMetric = Metric;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("The can not find the Mesh Route !!!\n"));
		return FALSE;
	}
#endif
	return success;
}

INT Set_MeshPortalEnable_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT	success = TRUE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	return success;
}

INT Set_MeshMultiCastAgeOut_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	UINT AgeTime;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	AgeTime = simple_strtol(arg, 0, 10);

	if ((AgeTime <= 65535) && (AgeTime >= 1))
		pAd->MeshTab.MeshMultiCastAgeOut = AgeTime;
	else 
		DBGPRINT(RT_DEBUG_ERROR, ("%s::Wrong MeshMultiCastAgeOut setting(%d), (1 ~ 65535))\n", __FUNCTION__, AgeTime));

	DBGPRINT(RT_DEBUG_TRACE, ("%s::(MeshMultiCastAgeOut = %ld)\n", __FUNCTION__, pAd->MeshTab.MeshMultiCastAgeOut));

	pAd->MeshTab.MeshMultiCastAgeOut = (AgeTime * 1000);

	return TRUE;
}

INT Set_MeshAuthMode_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	UCHAR	i;
	INT		success = TRUE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;
	
	if ((strncmp(arg, "WPANONE", 7) == 0) || (strncmp(arg, "wpanone", 7) == 0))
		pAd->MeshTab.AuthMode = Ndis802_11AuthModeWPANone;		
	else
		pAd->MeshTab.AuthMode = Ndis802_11AuthModeOpen;

	// Set all mesh link as Port_Not_Secure
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (pAd->MacTab.Content[i].ValidAsMesh)
		{
			pAd->MacTab.Content[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}
		
    MakeMeshRSNIE(pAd, pAd->MeshTab.AuthMode, pAd->MeshTab.WepStatus);
	
	if(pAd->MeshTab.AuthMode >= Ndis802_11AuthModeWPA)
	{	
		if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPANone)
			pAd->MeshTab.DefaultKeyId = 0;
		else
			pAd->MeshTab.DefaultKeyId = 1;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) Set_MeshAuthMode_Proc::(MeshAuthMode(%d)=%s)\n",
			pAd->MeshTab.AuthMode, GetAuthMode(pAd->MeshTab.AuthMode)));
	
	return success;
}

INT Set_MeshEncrypType_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT	success = TRUE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	if ((strncmp(arg, "WEP", 3) == 0) || (strncmp(arg, "wep", 3) == 0))
    {
		if (pAd->MeshTab.AuthMode < Ndis802_11AuthModeWPA)
			pAd->MeshTab.WepStatus = Ndis802_11WEPEnabled;				  
	}
	else if ((strncmp(arg, "TKIP", 4) == 0) || (strncmp(arg, "tkip", 4) == 0))
	{
		if (pAd->MeshTab.AuthMode >= Ndis802_11AuthModeWPA)
			pAd->MeshTab.WepStatus = Ndis802_11Encryption2Enabled;                       
    }
	else if ((strncmp(arg, "AES", 3) == 0) || (strncmp(arg, "aes", 3) == 0))
	{
		if (pAd->MeshTab.AuthMode >= Ndis802_11AuthModeWPA)
			pAd->MeshTab.WepStatus = Ndis802_11Encryption3Enabled;                            
	}    
	else
	{
		pAd->MeshTab.WepStatus = Ndis802_11WEPDisabled;                 
	}

	if(pAd->MeshTab.WepStatus >= Ndis802_11Encryption2Enabled)
	{		
		if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPANone)
			pAd->MeshTab.DefaultKeyId = 0;
		else
			pAd->MeshTab.DefaultKeyId = 1;
	}
								
	MakeMeshRSNIE(pAd, pAd->MeshTab.AuthMode, pAd->MeshTab.WepStatus);

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) Set_MeshEncrypType_Proc::(MeshEncrypType(%d)=%s)\n",
			pAd->MeshTab.WepStatus, GetEncryptType(pAd->MeshTab.WepStatus)));

	return success;
}

INT Set_MeshDefaultkey_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	ULONG		KeyIdx;
	INT			success = TRUE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	KeyIdx = simple_strtol(arg, 0, 10);
	if((KeyIdx >= 1 ) && (KeyIdx <= 4))
		pAd->MeshTab.DefaultKeyId = (UCHAR) (KeyIdx - 1);
	else
		pAd->MeshTab.DefaultKeyId = 0;	// Default value

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) Set_MeshDefaultkey_Proc::(MeshDefaultkey=%d)\n",
										pAd->MeshTab.DefaultKeyId));

	return success;
}

INT Set_MeshWEPKEY_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	UCHAR		i;
	UCHAR		KeyLen;
	UCHAR		CipherAlg = CIPHER_NONE;
	INT			success = TRUE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: //wep 40 Ascii type
		case 13: //wep 104 Ascii type
			pAd->MeshTab.SharedKey.KeyLen = KeyLen;
			NdisMoveMemory(pAd->MeshTab.SharedKey.Key, arg, KeyLen);
			if (KeyLen == 5)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;

			NdisMoveMemory(pAd->MeshTab.DesiredWepKey, arg, KeyLen);
			pAd->MeshTab.DesiredWepKeyLen= KeyLen;
			
			DBGPRINT(RT_DEBUG_TRACE, ("IF(mesh0) Set_MeshWEPKRY_Proc::(WepKey=%s ,type=%s, Alg=%s)\n", arg, "Ascii", CipherName[CipherAlg]));		
			break;
		case 10: //wep 40 Hex type
		case 26: //wep 104 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pAd->MeshTab.SharedKey.KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->MeshTab.SharedKey.Key, KeyLen/2);
			if (KeyLen == 10)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;

			NdisMoveMemory(pAd->MeshTab.DesiredWepKey, arg, KeyLen);
			pAd->MeshTab.DesiredWepKeyLen = KeyLen;
			
			DBGPRINT(RT_DEBUG_TRACE, ("IF(mesh0) Set_MeshWEPKRY_Proc::(WepKey=%s, type=%s, Alg=%s)\n", arg, "Hex", CipherName[CipherAlg]));		
			break;				
		default: //Invalid argument 
			pAd->MeshTab.SharedKey.KeyLen = 0;
			pAd->MeshTab.DesiredWepKeyLen = KeyLen;
			DBGPRINT(RT_DEBUG_ERROR, ("IF(mesh0) Set_MeshWEPKRY_Proc::Invalid argument (=%s)\n", arg));		
			return FALSE;
	}

	pAd->MeshTab.SharedKey.CipherAlg = CipherAlg;
    

	return success;
}


INT Set_MeshWPAKEY_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	UCHAR	keyMaterial[40];
	INT		success = TRUE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;
		
	if ((strlen(arg) < 8) || (strlen(arg) > 64))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("IF(mesh0) Set failed!!(PassPhrasKey=%s), the key string required 8 ~ 64 characters \n", arg));
		return FALSE;
	}

	NdisMoveMemory(pAd->MeshTab.WPAPassPhraseKey, arg, strlen(arg));
	pAd->MeshTab.WPAPassPhraseKeyLen = strlen(arg);

	if (strlen(arg) == 64)
	{
	    AtoH(arg, pAd->MeshTab.PMK, 32);
	}
	else
	{
	    PasswordHash((char *)arg, pAd->MeshTab.MeshId, pAd->MeshTab.MeshIdLen, keyMaterial);
	    NdisMoveMemory(pAd->MeshTab.PMK, keyMaterial, 32);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("IF(mesh0) Set_MeshWPAKEY_Proc::PassPhrasKey (=%s)\n", arg));		
	
	return success;
}

INT Set_MeshRouteInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT i;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	PMESH_ROUTING_TABLE	pRouteTab = pAd->MeshTab.pMeshRouteTab;

	if (pRouteTab == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Mesh Route Table doesn't exist.\n", __FUNCTION__));
		return FALSE;
	}

	if(!IS_MESH_IF(pObj))
		return FALSE;

	printk("\n\n");

	printk("\n%-19s%-6s%-19s%-16s%-12s%-9s\n",
		"MESH DA", "DSN", "NEXTHOP", "NEXTHOPLINKID", "METRICS", "ROUTE_IDX");
	
	for (i=0; i<MAX_ROUTE_TAB_SIZE; i++)
	{
		PMESH_ROUTING_ENTRY pEntry = &pRouteTab->Content[i];
		if (pEntry->Valid)
		{
			printk("%02X:%02X:%02X:%02X:%02X:%02X  ",
				pEntry->MeshDA[0], pEntry->MeshDA[1], pEntry->MeshDA[2],
				pEntry->MeshDA[3], pEntry->MeshDA[4], pEntry->MeshDA[5]);
			printk("%-6d", pEntry->Dsn);
			printk("%02X:%02X:%02X:%02X:%02X:%02X  ",
				pEntry->NextHop[0], pEntry->NextHop[1], pEntry->NextHop[2],
				pEntry->NextHop[3], pEntry->NextHop[4], pEntry->NextHop[5]);
			printk("%-16d", (int)pEntry->NextHopLinkID);
			printk("%-12d", pEntry->PathMetric);
			printk("%-9d\n", (int)pEntry->Idx);
		}
	} 

	return TRUE;
}

INT Set_MeshEntryInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	printk("\n%-19s%-10s\n",
		"DESTMAC", "ROUTE_IDX");
	
	MeshEntryTableGet(pAd);
	return TRUE;
}

INT Set_MeshInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT i;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	printk("HostName = %s, Len=%d\n", pAd->MeshTab.HostName, strlen(pAd->MeshTab.HostName));
	printk("Mesh Id = %s, Len=%d\n", pAd->MeshTab.MeshId, pAd->MeshTab.MeshIdLen);
	printk("Mesh AutoLink = %s\n", pAd->MeshTab.MeshAutoLink == TRUE ? "Enable" : "Disable");
	printk("Channel Precedence (CPI) = %d\n", pAd->MeshTab.CPI);
	printk("mesh ctrl current state =%d\n", pAd->MeshTab.CtrlCurrentState);

	for (i = 0; i < MAX_MESH_LINKS; i++)
	{
		printk("mesh link (%d) current state =%d,", i, pAd->MeshTab.MeshLink[i].CurrentState);
		printk(" Valid =%d,", pAd->MeshTab.MeshLink[i].Entry.Valid);
		printk(" MatchWcid =%d,", pAd->MeshTab.MeshLink[i].Entry.MacTabMatchWCID);
		printk(" LocalId =%x,", pAd->MeshTab.MeshLink[i].Entry.LocalLinkId);
		printk(" PeerId =%x,", pAd->MeshTab.MeshLink[i].Entry.PeerLinkId);
		printk(" PeerMacAddr =%02x:%02x:%02x:%02x:%02x:%02x\n",
			pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr[0], pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr[1],
			pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr[2], pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr[3],
			pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr[4], pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr[5]);
	}
	printk("\n\n");

	printk("\n%-19s%-4s%-4s%-4s%-7s%-7s%-7s%-10s%-6s%-6s%-6s%-6s\n",
		"MAC", "IDX", "AID", "PSM", "RSSI0", "RSSI1", "RSSI2", "PhMd", "BW", "MCS", "SGI", "STBC");
	
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if (pEntry->ValidAsMesh)
		{
			printk("%02X:%02X:%02X:%02X:%02X:%02X  ",
				pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
			printk("%-4d", (int)pEntry->MatchWDSTabIdx);
			printk("%-4d", (int)pEntry->Aid);
			printk("%-4d", (int)pEntry->PsMode);
			printk("%-7d", pEntry->RssiSample.AvgRssi0);
			printk("%-7d", pEntry->RssiSample.AvgRssi1);
			printk("%-7d", pEntry->RssiSample.AvgRssi2);
			printk("%-10s", GetPhyMode(pEntry->HTPhyMode.field.MODE));
			printk("%-6s", GetBW(pEntry->HTPhyMode.field.BW));
			printk("%-6d", pEntry->HTPhyMode.field.MCS);
			printk("%-6d", pEntry->HTPhyMode.field.ShortGI);
			printk("%-6d\n", pEntry->HTPhyMode.field.STBC);
		}
	} 

	return TRUE;
}

INT Set_NeighborInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	int i;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	if (pAd->MeshTab.pMeshNeighborTab == NULL)
	{
		printk("Mesh Neighbor Tab not ready.\n");
		return TRUE;
	}
		

	if (pAd->MeshTab.pMeshNeighborTab->NeighborNr == 0)
	{
		printk("Mesh Neighbor Tab empty.\n");
		return TRUE;
	}

	printk("Neighbor MP Num = %d\n", pAd->MeshTab.pMeshNeighborTab->NeighborNr);
#if 0
	for (i = 0; i < MAX_NEIGHBOR_MP; i++ )
	{
		if (pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].Valid != TRUE)
			continue;

		printk("%d, HostName=%s,", i, pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].HostName);
		printk(" Rssi=%d,", pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].RealRssi);
		printk(" Ch=%d,", pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].Channel);
		printk(" CPI=%d,", pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].CPI);
		printk(" State=%d,", pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].State);
		printk(" LinkId=%d,", pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].MeshLinkIdx);
		printk(" MeshId=%s, len=%d", pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].MeshId,
			pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].MeshIdLen);
		printk(" Mac=%02x:%02x:%02x:%02x:%02x:%02x,",
			pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].PeerMac[0], pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].PeerMac[1],
			pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].PeerMac[2], pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].PeerMac[3],
			pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].PeerMac[4], pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].PeerMac[5]);

		if (pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].MeshEncrypType == ENCRYPT_OPEN_WEP)
			printk(" Encryption=OPEN-WEP\n");
		else if (pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].MeshEncrypType == ENCRYPT_WPANONE_TKIP) 
			printk(" Encryption=WPANONE-TKIP\n");
		else if (pAd->MeshTab.pMeshNeighborTab->NeighborMP[i].MeshEncrypType == ENCRYPT_WPANONE_AES) 
			printk(" Encryption=WPANONE-AES\n");
		else
			printk(" Encryption=OPEN-NONE\n");
		
	}
#else
	printk("\n%-4s%-19s%-6s%-4s%-4s%-6s%-8s%-6s%-8s%-14s%-16s%-6s\n",
		"IDX", "MAC", "MRSI", "CH", "BW", "CHOF", "CPI", "STATE", "LINKID", "ECRP", "MESHID", "HSTN");
	
	for (i=0; i<MAX_NEIGHBOR_MP; i++)
	{
		PMESH_NEIGHBOR_ENTRY pEntry = &pAd->MeshTab.pMeshNeighborTab->NeighborMP[i];
		if (pEntry->Valid)
		{
			printk("%-4d", i);
			printk("%02X:%02X:%02X:%02X:%02X:%02X  ",
				pEntry->PeerMac[0], pEntry->PeerMac[1], pEntry->PeerMac[2],
				pEntry->PeerMac[3], pEntry->PeerMac[4], pEntry->PeerMac[5]);
			printk("%-6d", (int)pEntry->RealRssi);
			printk("%-4d", (int)pEntry->Channel);
			printk("%-4d", (int)pEntry->ChBW);
			printk("%-6d", (int)pEntry->ExtChOffset);
			printk("%-8d", (int)pEntry->CPI);
			printk("%-6d", (int)pEntry->State);
			printk("%-8x", (int)pEntry->MeshLinkIdx);
			if (pEntry->MeshEncrypType == ENCRYPT_OPEN_WEP)
				printk("%-14s", "OPEN-WEP");
			else if (pEntry->MeshEncrypType == ENCRYPT_WPANONE_TKIP) 
				printk("%-14s", "WPANONE-TKIP");
			else if (pEntry->MeshEncrypType == ENCRYPT_WPANONE_AES) 
				printk("%-14s", "WPANONE-AES");
			else
				printk("%-14s", "OPEN-NONE");
			printk("%-16s", pEntry->MeshId);
			printk("%s\n", pEntry->HostName);
		}
	} 
#endif

	return TRUE;
}

INT Set_MultipathInfo_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT i;
	LONG HashId;
	PMESH_MULTIPATH_ENTRY pEntry;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	for (i = 0; i < MAX_MESH_LINKS; i++)
	{
		if (!PeerLinkValidCheck(pAd, i))
			continue;
		printk("Link(%d) ", i);
		for (HashId = 0; HashId < MULTIPATH_HASH_TAB_SIZE; HashId++)
		{
			pEntry = (PMESH_MULTIPATH_ENTRY)(pAd->MeshTab.MeshLink[i].Entry.MultiPathHash[HashId].pHead);
			if (pEntry == NULL)
				continue;

			printk(" HashId(%ld):", HashId);
			while (pEntry)
			{
				printk("SA=%02x:%02x:%02x:%02x:%02x:%02x ",
					pEntry->MeshSA[0], pEntry->MeshSA[1], pEntry->MeshSA[2],
					pEntry->MeshSA[3], pEntry->MeshSA[4], pEntry->MeshSA[5]);
				pEntry = pEntry->pNext;
			}
		}
		printk("\n");
	}

	return TRUE;
}

INT Set_MultiCastAgeOut_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	printk("Multi Cast Age Timeout = %ld sec\n", (pAd->MeshTab.MeshMultiCastAgeOut) / 1000);
	
	return TRUE;
}

INT Set_MeshOnly_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	UINT Enable;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	Enable = simple_strtol(arg, 0, 16);

	pAd->MeshTab.MeshOnly= (Enable > 0) ? TRUE : FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("%s::(enable = %d)\n", __FUNCTION__, pAd->MeshTab.MeshOnly));
	
	return TRUE;
}

INT Set_PktSig_Display_Proc(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR arg)
{
	INT i;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	PMESH_BMPKTSIG_TAB pTab = pAd->MeshTab.pBMPktSigTab;

	if(!IS_MESH_IF(pObj))
		return FALSE;

	printk("\n%-4s%-19s%-4s\n", "IDX", "MAC", "SEQ");
	for (i=0; i<MAX_BMPKTSIG_TAB_SIZE; i++)
	{
		PMESH_BMPKTSIG_ENTRY pEntry = &(pTab->Content[i]);
		if (pTab->Content[i].Valid == FALSE)
			continue;

		printk("%-4d", i);
		printk("%02x:%02x:%02x:%02x:%02x:%02x  ",
				pEntry->MeshSA[0], pEntry->MeshSA[1], pEntry->MeshSA[2],
				pEntry->MeshSA[3], pEntry->MeshSA[4], pEntry->MeshSA[5]);
		printk("%-8x:%-9x%-9x%-9x%-9x",
			pEntry->MeshSeqBased, pEntry->Offset[0], pEntry->Offset[1],
			pEntry->Offset[2], pEntry->Offset[3]);
		printk("\n");
	}

	return TRUE;
}

/* --------------------------------- Public -------------------------------- */
/*
========================================================================
Routine Description:
    Init Mesh function.

Arguments:
    ad_p            points to our adapter
    main_dev_p      points to the main BSS network interface

Return Value:
    None

Note:
	1. Only create and initialize virtual network interfaces.
	2. No main network interface here.
========================================================================
*/
VOID RTMP_Mesh_Init(
	IN PRTMP_ADAPTER 		pAd,
	IN PNET_DEV				main_dev_p,
	IN PUCHAR				pHostName)
{
#define MESH_MAX_DEV_NUM	32
	PNET_DEV	cur_dev_p;
	PNET_DEV	new_dev_p;
	VIRTUAL_ADAPTER *mesh_ad_p;
	CHAR slot_name[IFNAMSIZ];
	INT index = 0;

	/* sanity check to avoid redundant virtual interfaces are created */
	if (pAd->flg_mesh_init != FALSE)
		return;

	pAd->flg_mesh_init = TRUE;

	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	/* init */
	pAd->MeshTab.dev = NULL;
 
	/* create virtual network interface */
	do {
		/* allocate a new network device */
#if LINUX_VERSION_CODE <= 0x20402 // Red Hat 7.1
		new_dev_p = alloc_netdev(sizeof(VIRTUAL_ADAPTER), "eth%d", ether_setup);
#else   
		new_dev_p = alloc_etherdev(sizeof(VIRTUAL_ADAPTER));
#endif // LINUX_VERSION_CODE //
        
		if (new_dev_p == NULL)
		{
			/* allocation fail, exit */
			DBGPRINT(RT_DEBUG_ERROR,
						("Allocate network device fail (APCLI)...\n"));
			break;
		}

  
        /* find a available interface name, max 32 ra interfaces */
		for(index = 0; index < MESH_MAX_DEV_NUM; index++)
		{
#ifdef MULTIPLE_CARD_SUPPORT
			if (pAd->MC_RowID >= 0)
				sprintf(slot_name, "mesh%02d-%d", pAd->MC_RowID, index);
			else
#endif // MULTIPLE_CARD_SUPPORT //
				sprintf(slot_name, "mesh%d", index);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			cur_dev_p = dev_get_by_name(slot_name);
#else
			for(cur_dev_p=dev_base; cur_dev_p!=NULL; cur_dev_p=cur_dev_p->next)
			{
				if (strncmp(cur_dev_p->name, slot_name, 6) == 0)
					break;
			}
#endif // LINUX_VERSION_CODE //

			if (cur_dev_p == NULL)
				break; /* fine, the RA name is not used */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			else
			{
				/* every time dev_get_by_name is called, and it has returned a
					valid struct net_device*, dev_put should be called
					afterwards, because otherwise the machine hangs when the
					device is unregistered (since dev->refcnt > 1). */
				dev_put(cur_dev_p);
			}
#endif // LINUX_VERSION_CODE //
		} /* End of for */

		/* assign interface name to the new network interface */
		if (index < MESH_MAX_DEV_NUM)
		{
			sprintf(new_dev_p->name, "mesh%d", index);
			DBGPRINT(RT_DEBUG_TRACE, ("Register MESH IF (mesh%d)\n", index));
		}
		else
		{
			/* error! no any available ra name can be used! */
			DBGPRINT(RT_DEBUG_ERROR,
						("Has %d mesh interfaces (MESH)...\n", MESH_MAX_DEV_NUM));
			kfree(new_dev_p);
			break;
		}

 		/* init the new network interface */
		ether_setup(new_dev_p);

		mesh_ad_p = new_dev_p->priv; /* sizeof(priv) = sizeof(VIRTUAL_ADAPTER) */
		mesh_ad_p->VirtualDev = new_dev_p;  /* 4 Bytes */
		mesh_ad_p->RtmpDev    = main_dev_p; /* 4 Bytes */

		/* init MAC address of virtual network interface */
		COPY_MAC_ADDR(pAd->MeshTab.CurrentAddress, pAd->CurrentAddress);
#ifdef CONFIG_AP_SUPPORT
		pAd->MeshTab.CurrentAddress[ETH_LENGTH_OF_ADDRESS - 1] =
			(pAd->MeshTab.CurrentAddress[ETH_LENGTH_OF_ADDRESS - 1] + pAd->ApCfg.BssidNum) & 0xFF;
#endif // CONFIG_AP_SUPPORT //
		NdisMoveMemory(&new_dev_p->dev_addr,
						pAd->MeshTab.CurrentAddress,
						MAC_ADDR_LEN);

		/* init operation functions */
		new_dev_p->open				= Mesh_VirtualIF_Open;
		new_dev_p->stop				= Mesh_VirtualIF_Close;
		new_dev_p->hard_start_xmit	= (HARD_START_XMIT_FUNC)Mesh_VirtualIF_PacketSend;
		
		new_dev_p->do_ioctl			= Mesh_VirtualIF_Ioctl;
		/* if you dont implement get_stats, dont assign your function with empty
			body; or kernel will panic */
		//new_dev_p->get_stats		= Mesh_VirtualIF_EtherStats_Get;
		new_dev_p->priv_flags		= INT_MESH; /* we are virtual interface */

		/* register this device to OS */
		register_netdevice(new_dev_p);

		/* backup our virtual network interface */
        pAd->MeshTab.dev = new_dev_p;
	} while(FALSE);

	// Initialize Mesh configuration
	MeshCfgInit(pAd, pHostName);

	// initialize Mesh Tables and allocate spin locks
	NdisAllocateSpinLock(&pAd->MeshTabLock);

	NeighborTableInit(pAd);
	BMPktSigTabInit(pAd);
	MultipathPoolInit(pAd);

	MeshRoutingTable_Init(pAd);
	MeshEntryTable_Init(pAd);
	MeshProxyEntryTable_Init(pAd);

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
} /* End of RTMP_Mesh_Init */


/*
========================================================================
Routine Description:
    Close Mesh network interface.

Arguments:
    ad_p            points to our adapter

Return Value:
    None

Note:
========================================================================
*/
VOID RTMP_Mesh_Close(
	IN PRTMP_ADAPTER ad_p)
{
	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	// close virtual interface.
	if (ad_p->MeshTab.dev)
		dev_close(ad_p->MeshTab.dev);

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
} /* End of RTMP_Mesh_Close */


/*
========================================================================
Routine Description:
    Remove Mesh network interface.

Arguments:
    ad_p            points to our adapter

Return Value:
    None

Note:
========================================================================
*/
VOID RTMP_Mesh_Remove(
	IN PRTMP_ADAPTER ad_p)
{
	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	// remove virtual interface.
	if (ad_p->MeshTab.dev)
	{
		unregister_netdev(ad_p->MeshTab.dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		free_netdev(ad_p->MeshTab.dev);
#else
		kfree(ad_p->MeshTab.dev);
#endif // LINUX_VERSION_CODE //
	}

	NeighborTableDestroy(ad_p);
	BMPktSigTabExit(ad_p);

	MeshRoutingTable_Exit(ad_p);
	MeshEntryTable_Exit(ad_p);
	MeshProxyEntryTable_Exit(ad_p);

	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
} /* End of RTMP_Mesh_Remove */


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
static INT Mesh_VirtualIF_Open(
	IN PNET_DEV		dev_p)
{
	VIRTUAL_ADAPTER *virtual_pAd = dev_p->priv;
	RTMP_ADAPTER *pAd;
	wait_queue_head_t wait;

	init_waitqueue_head(&wait);

	/* sanity check */
	ASSERT(virtual_pAd);
	ASSERT(virtual_pAd->RtmpDev);
	pAd = virtual_pAd->RtmpDev->priv;
	ASSERT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> %s\n", __FUNCTION__, dev_p->name));
	ASSERT(virtual_pAd->VirtualDev);

	if (ADHOC_ON(pAd))
		return -1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	if (!try_module_get(THIS_MODULE))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: cannot reserve module\n", __FUNCTION__));
		return -1;
	}
	#else
		MOD_INC_USE_COUNT;
#endif	

	// Initialize RF register to default value
	MeshChannelInit(pAd);

	netif_start_queue(virtual_pAd->VirtualDev);

	// Statup Mesh Protocol Stack.
	MeshUp(pAd);


	DBGPRINT(RT_DEBUG_TRACE, ("%s: <=== %s\n", __FUNCTION__, dev_p->name));

	return 0;
}


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
static INT Mesh_VirtualIF_Close(
	IN	PNET_DEV	dev_p)
{
	VIRTUAL_ADAPTER	*virtual_ad_p = dev_p->priv;
	PRTMP_ADAPTER pAd;

	/* sanity check */
	ASSERT(virtual_ad_p);
	ASSERT(virtual_ad_p->RtmpDev);
	pAd = virtual_ad_p->RtmpDev->priv;
	ASSERT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> %s\n", __FUNCTION__, dev_p->name));
	ASSERT(virtual_ad_p->VirtualDev);

	// stop mesh.
	netif_stop_queue(virtual_ad_p->VirtualDev);
	MeshDown(pAd, TRUE);


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	module_put(THIS_MODULE);
#else
	MOD_DEC_USE_COUNT;
#endif

	return 0;
} 


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
static INT Mesh_VirtualIF_PacketSend(
	IN PNDIS_PACKET 	skb_p, 
	IN PNET_DEV			dev_p)
{
	VIRTUAL_ADAPTER *virtual_pAd = dev_p->priv;
	PRTMP_ADAPTER pAd;
	PMESH_STRUCT pMesh;

	DBGPRINT(RT_DEBUG_INFO, ("%s --->\n", __FUNCTION__));

	/* sanity check */
	ASSERT(virtual_pAd);
	ASSERT(virtual_pAd->RtmpDev);
	pAd = virtual_pAd->RtmpDev->priv;
	ASSERT(pAd);

#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
	{
		RELEASE_NDIS_PACKET(pAd, skb_p, NDIS_STATUS_FAILURE);
		return 0;
	}
#endif // RALINK_ATE //
	if ((RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)) ||
		(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))          ||
		(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)))
	{
		/* wlan is scanning/disabled/reset */
		RELEASE_NDIS_PACKET(pAd, skb_p, NDIS_STATUS_FAILURE);
		return 0;
	}

	if (!(virtual_pAd->VirtualDev->flags & IFF_UP))
	{
		/* the interface is down */
		RELEASE_NDIS_PACKET(pAd, skb_p, NDIS_STATUS_FAILURE);
		return 0;
	}

	pMesh = (PMESH_STRUCT)&pAd->MeshTab;
	do
	{
		if (MeshValid(pMesh) != TRUE)
			break;
		/* find the device in our Mesh list */
		if (pMesh->dev == dev_p)
		{
			/* ya! find it */
			pAd->RalinkCounters.PendingNdisPacketCount ++;
			RTMP_SET_PACKET_SOURCE(skb_p, PKTSRC_NDIS);
			RTMP_SET_PACKET_MOREDATA(skb_p, FALSE);
			RTMP_SET_PACKET_NET_DEVICE_MESH(skb_p, 0);
			RTPKT_TO_OSPKT(skb_p)->dev = virtual_pAd->RtmpDev;

			if (!(*(RTPKT_TO_OSPKT(skb_p)->data) & 0x01))
			{
				DBGPRINT(RT_DEBUG_INFO,
							("%s(Mesh) - unicast packet to "
							"(Mesh0)\n", __FUNCTION__));
			}
 
			/* transmit the packet */
			return rt28xx_packet_xmit(RTPKT_TO_OSPKT(skb_p));
		}
    } while(FALSE);


	/* can not find the BSS so discard the packet */
	DBGPRINT(RT_DEBUG_INFO,
				("%s - needn't to send or net_device not "
					"exist.\n", __FUNCTION__));
	RELEASE_NDIS_PACKET(pAd, skb_p, NDIS_STATUS_FAILURE);
	DBGPRINT(RT_DEBUG_INFO, ("%s <---\n", __FUNCTION__));
	return 0;
} /* End of Mesh_VirtualIF_PacketSend */


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
static INT Mesh_VirtualIF_Ioctl(
	IN PNET_DEV				dev_p, 
	IN OUT struct ifreq 	*rq_p, 
	IN INT 					cmd)
{
	if (dev_p->priv_flags & INT_MESH)
		return rt28xx_ioctl(dev_p, rq_p, cmd);
	else
		return -1;
} /* End of Mesh_VirtualIF_Ioctl */

static VOID MeshCfgInit(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR		 pHostName)
{
	INT	i;

	// default configuration of Mesh.
	pAd->MeshTab.OpMode = MESH_MP;

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		pAd->MeshTab.OpMode |= MESH_AP;
#endif // CONFIG_AP_SUPPORT //

	pAd->MeshTab.PathProtocolId = MESH_HWMP;
	pAd->MeshTab.PathMetricId = MESH_AIRTIME;
	pAd->MeshTab.ContgesionCtrlId = NULL_PROTOCOL;
	pAd->MeshTab.TTL = MESH_TTL;
	pAd->MeshTab.MeshMaxTxRate = 0;
	pAd->MeshTab.MeshMultiCastAgeOut = MULTIPATH_AGEOUT;
	pAd->MeshTab.UCGEnable = FALSE;
	if (pAd->MeshTab.MeshIdLen == 0)
	{
		pAd->MeshTab.MeshIdLen = strlen(DEFAULT_MESH_ID);
		NdisMoveMemory(pAd->MeshTab.MeshId, DEFAULT_MESH_ID, pAd->MeshTab.MeshIdLen);
	}

	// initialize state
	pAd->MeshTab.EasyMeshSecurity = TRUE;	// Default is TRUE for CMPC
	pAd->MeshTab.bInitialMsaDone = FALSE;
	pAd->MeshTab.bKeyholderDone  = FALSE;
	pAd->MeshTab.bConnectedToMKD = FALSE;
	pAd->MeshTab.MeshOnly = FALSE;

	pAd->MeshTab.bAutoTxRateSwitch = TRUE;
	pAd->MeshTab.DesiredTransmitSetting.field.MCS = MCS_AUTO;

	for (i = 0; i < MAX_MESH_LINKS; i++)
		NdisZeroMemory(&pAd->MeshTab.MeshLink[i].Entry, sizeof(MESH_LINK_ENTRY));

	if (strlen(pHostName) > 0)
	{
		if (strlen(pHostName) < MAX_HOST_NAME_LEN)
			strcpy(pAd->MeshTab.HostName, pHostName);
		else
			strncpy(pAd->MeshTab.HostName, pHostName, MAX_HOST_NAME_LEN-1);
	}
	else
		strcpy(pAd->MeshTab.HostName, DEFAULT_MESH_HOST_NAME);

}


VOID MeshUp(
	IN PRTMP_ADAPTER pAd)
{
	BOOLEAN TxPreamble;

    DBGPRINT(RT_DEBUG_TRACE, ("%s: ===> \n", __FUNCTION__));
	if ( RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
		 RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
		 //||!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
		return;

	pAd->MeshTab.MeshChannel = pAd->CommonCfg.Channel;

	// Make regular Beacon frame
	MeshMakeBeacon(pAd, MESH_BEACON_IDX(pAd));

	// Check if the security is supported
	if (pAd->MeshTab.EasyMeshSecurity)
	{
		// 1. This mode can support OPEN, WEP, WPANONE and Mesh-PSK.
		// 2. These setting doesn't need MKD and MA. An MP can connect
		// 	  to the other MP directly.
		// 3. This is does't be defined in IEEE 802.11s standard.
		if (pAd->MeshTab.AuthMode != Ndis802_11AuthModeOpen && 
			pAd->MeshTab.AuthMode != Ndis802_11AuthModeWPANone)
		{
			pAd->MeshTab.AuthMode = Ndis802_11AuthModeOpen;
			pAd->MeshTab.WepStatus = Ndis802_11WEPDisabled;
		}			
		pAd->MeshTab.OpMode &= ~(MESH_MKD);
		
		DBGPRINT(RT_DEBUG_TRACE, ("MeshUp: the Easy MSA is enabled. \n"));
	}
	else
	{
		// 1. It refer to the definition of IEEE 802.11s
		// 2. It ONLY supports Mesh-PSK and Mesh-802.1X authentication.
		// 3. An MP can't connect to the other MP directly.
		//	  It MUST bulid its security link with an MA or MKD.
		if (pAd->MeshTab.AuthMode != Ndis802_11AuthModeWPA2 && 
			pAd->MeshTab.AuthMode != Ndis802_11AuthModeWPA2PSK)
		{
			pAd->MeshTab.AuthMode = Ndis802_11AuthModeWPA2;
			pAd->MeshTab.WepStatus = Ndis802_11Encryption3Enabled;
		}

		DBGPRINT(RT_DEBUG_TRACE, ("MeshUp: the Easy MSA is disabled. \n"));
	}

	// If the MP implements the MKD function, set value into the MKDD-ID field.
	// Otherwise, zero the MKDD-ID value.
	if (pAd->MeshTab.OpMode & MESH_MKD)
	{
		NdisMoveMemory(pAd->MeshTab.LocalMSCIE.MKDDID, pAd->MeshTab.CurrentAddress, MAC_ADDR_LEN);
		pAd->MeshTab.bInitialMsaDone = TRUE;
		pAd->MeshTab.bKeyholderDone  = TRUE;
		pAd->MeshTab.bConnectedToMKD = TRUE;
	}
	else
	{
		NdisZeroMemory(&pAd->MeshTab.LocalMSCIE, sizeof(MESH_SECURITY_CAPABILITY_IE));
		pAd->MeshTab.bInitialMsaDone = FALSE;
		pAd->MeshTab.bKeyholderDone  = FALSE;
		pAd->MeshTab.bConnectedToMKD = FALSE;
	}


	// Init PMKID
	pAd->MeshTab.PMKID_Len = 0;
	NdisZeroMemory(pAd->MeshTab.PMKID, LEN_PMKID);

	TxPreamble = (pAd->CommonCfg.TxPreamble == Rt802_11PreambleLong ? 0 : 1);

	pAd->MeshTab.CapabilityInfo =
			CAP_GENERATE(0, 0, (pAd->MeshTab.WepStatus != Ndis802_11EncryptionDisabled), TxPreamble, pAd->CommonCfg.bUseShortSlotTime, 0);

	if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPANone)
	{
		pAd->MeshTab.DefaultKeyId = 0;	// always be zero
		
        NdisZeroMemory(&pAd->MeshTab.SharedKey, sizeof(CIPHER_KEY));  
		pAd->MeshTab.SharedKey.KeyLen = LEN_TKIP_EK;

		NdisMoveMemory(pAd->MeshTab.SharedKey.Key, pAd->MeshTab.PMK, LEN_TKIP_EK);
            
        if (pAd->MeshTab.WepStatus == Ndis802_11Encryption2Enabled)
        {
    		NdisMoveMemory(pAd->MeshTab.SharedKey.RxMic, &pAd->MeshTab.PMK[16], LEN_TKIP_RXMICK);
    		NdisMoveMemory(pAd->MeshTab.SharedKey.TxMic, &pAd->MeshTab.PMK[16], LEN_TKIP_TXMICK);
        }

		// Decide its ChiperAlg
		if (pAd->MeshTab.WepStatus == Ndis802_11Encryption2Enabled)
			pAd->MeshTab.SharedKey.CipherAlg = CIPHER_TKIP;
		else if (pAd->MeshTab.WepStatus == Ndis802_11Encryption3Enabled)
			pAd->MeshTab.SharedKey.CipherAlg = CIPHER_AES;
		else
        {         
            DBGPRINT(RT_DEBUG_WARN, ("Unknow Cipher (=%d), set Cipher to AES\n", pAd->MeshTab.WepStatus));
			pAd->MeshTab.SharedKey.CipherAlg = CIPHER_AES;
        } 
	}

	DBGPRINT(RT_DEBUG_TRACE, ("!!! %s - AuthMode(%d)=%s, WepStatus(%d)=%s !!!\n", 
									__FUNCTION__,
									pAd->MeshTab.AuthMode, GetAuthMode(pAd->MeshTab.AuthMode),
									pAd->MeshTab.WepStatus, GetEncryptType(pAd->MeshTab.WepStatus)));

	MlmeEnqueueEx(pAd, MESH_CTRL_STATE_MACHINE, MESH_CTRL_JOIN, 0, NULL, 0);
	DBGPRINT(RT_DEBUG_TRACE, ("%s: <=== \n", __FUNCTION__));
}

VOID MeshDown(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN WaitFlag)
{
	ULONG WaitCnt;

	// clear PMKID
	pAd->MeshTab.PMKID_Len = 0;
	NdisZeroMemory(pAd->MeshTab.PMKID, LEN_PMKID);

	// clear these flag
	pAd->MeshTab.bInitialMsaDone = FALSE;	
	pAd->MeshTab.bKeyholderDone  = FALSE;
	pAd->MeshTab.bConnectedToMKD = FALSE;

	MlmeEnqueueEx(pAd, MESH_CTRL_STATE_MACHINE, MESH_CTRL_DISCONNECT, 0, NULL, 0);
	MeshMlmeHandler(pAd);

	WaitCnt = 0;
	do
	{
		INT idx;
		BOOLEAN WaitMeshClose;
		wait_queue_head_t wait;

		if (WaitFlag == FALSE)
			break;

		WaitMeshClose = FALSE;
		init_waitqueue_head(&wait);

		for (idx = 0; idx < MAX_MESH_LINKS; idx++)
		{
			if (PeerLinkMngRuning(pAd, idx) || PeerLinkValidCheck(pAd, idx))
				WaitMeshClose = TRUE;
		}

		if(WaitMeshClose == TRUE)
			wait_event_interruptible_timeout(wait,0, 10 * OS_HZ/1000);
		else
			break;
	} while (WaitCnt++ < 1000);

	/* when the ra interface is down, do not send its beacon frame */
	MeshCleanBeaconFrame(pAd, MESH_BEACON_IDX(pAd));

}

VOID MeshHalt(
	IN PRTMP_ADAPTER pAd)
{
	int idx;
	BOOLEAN 	  Cancelled;

	RTMPCancelTimer(&pAd->MeshTab.PldTimer, &Cancelled);
	RTMPCancelTimer(&pAd->MeshTab.McsTimer, &Cancelled);
	for (idx = 0; idx < MAX_MESH_LINKS; idx++)
	{
		RTMPCancelTimer(&pAd->MeshTab.MeshLink[idx].TOR, &Cancelled);
		RTMPCancelTimer(&pAd->MeshTab.MeshLink[idx].TOC, &Cancelled);
		RTMPCancelTimer(&pAd->MeshTab.MeshLink[idx].TOH, &Cancelled);
	}
}

BOOLEAN MeshAcceptPeerLink(
	IN PRTMP_ADAPTER pAd)
{
	return (pAd->MeshTab.LinkSize < MAX_MESH_LINKS) ? 1 : 0;
}

/*
================================================================
Description : because Mesh and CLI share the same WCID table in ASIC. 
Mesh entry also insert to pAd->MacTab.content[].  Such is marked as ValidAsMesh = TRUE.
Also fills the pairwise key.
Because front MAX_AID_BA entries have direct mapping to BAEntry, which is only used as CLI. So we insert Mesh
from index MAX_AID_BA.
================================================================
*/
MAC_TABLE_ENTRY *MacTableInsertMeshEntry(
	IN  PRTMP_ADAPTER   pAd, 
	IN  PUCHAR pAddr,
	IN  UINT MeshLinkIdx)
{
	PMAC_TABLE_ENTRY pEntry = NULL;

	// if FULL, return
	if (pAd->MacTab.Size >= MAX_LEN_OF_MAC_TABLE)
		return NULL;

	do
	{
		if((pEntry = MeshTableLookup(pAd, pAddr, TRUE)) != NULL)
			break;

		// allocate one MAC entry
		pEntry = MacTableInsertEntry(pAd, pAddr, MeshLinkIdx + MIN_NET_DEVICE_FOR_MESH, TRUE);
		if (pEntry)
		{
			pAd->MeshTab.MeshLink[MeshLinkIdx].Entry.MacTabMatchWCID = pEntry->Aid;
			pEntry->MatchMeshTabIdx = MeshLinkIdx;

			//AsicAddKeyEntry(pAd, pEntry->Aid, BSS0, 0, &pAd->WdsTab.Wpa_key, TRUE, TRUE);

			DBGPRINT(RT_DEBUG_TRACE, ("MacTableInsertMeshEntry - allocate entry #%d, Total= %d\n",pEntry->Aid, pAd->MacTab.Size));
			break;
		}
	} while(FALSE);

	return pEntry;
}


/*
	==========================================================================
	Description:
		Delete all Mesh Entry in pAd->MacTab
	==========================================================================
 */
BOOLEAN MacTableDeleteMeshEntry(
	IN PRTMP_ADAPTER pAd,
	IN USHORT wcid,
	IN PUCHAR pAddr)
{
	if (!VALID_WCID(wcid))
		return FALSE;

	MacTableDeleteEntry(pAd, wcid, pAddr);
	MeshCreatePerrAction(pAd, pAddr);

	return TRUE;
}

MAC_TABLE_ENTRY *MeshTableLookup(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pAddr,
	IN BOOLEAN bResetIdelCount)
{
	ULONG HashIdx;
	MAC_TABLE_ENTRY *pEntry = NULL;

	RTMP_SEM_LOCK(&pAd->MacTabLock);
	HashIdx = MAC_ADDR_HASH_INDEX(pAddr);
	pEntry = pAd->MacTab.Hash[HashIdx];

	while (pEntry)
	{
		if ((pEntry->ValidAsMesh == TRUE)
			&& MAC_ADDR_EQUAL(pEntry->Addr, pAddr))
		{
			if(bResetIdelCount)
				pEntry->NoDataIdleCount = 0;
			break;
		}
		else
			pEntry = pEntry->pNext;
	}

	RTMP_SEM_UNLOCK(&pAd->MacTabLock);
	return pEntry;
}

MAC_TABLE_ENTRY *MeshTableLookupByWcid(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR wcid,
	IN PUCHAR pAddr,
	IN BOOLEAN bResetIdelCount)
{
	ULONG MeshIndex;
	PMAC_TABLE_ENTRY pCurEntry = NULL;
	PMAC_TABLE_ENTRY pEntry = NULL;

	if (!VALID_WCID(wcid))
		return NULL;

	RTMP_SEM_LOCK(&pAd->MeshTabLock);
	RTMP_SEM_LOCK(&pAd->MacTabLock);

	do
	{
		pCurEntry = &pAd->MacTab.Content[wcid];

		MeshIndex = 0xff;
		if ((pCurEntry) && (pCurEntry->ValidAsMesh == TRUE))
		{
			MeshIndex = pCurEntry->MatchMeshTabIdx;
		}

		if (MeshIndex == 0xff)
			break;

		if (pAd->MeshTab.MeshLink[MeshIndex].Entry.Valid != TRUE)
			break;

		if (MAC_ADDR_EQUAL(pCurEntry->Addr, pAddr))
		{
			if(bResetIdelCount)
				pCurEntry->NoDataIdleCount = 0;
			pEntry = pCurEntry;
			break;
		}
	} while(FALSE);

	RTMP_SEM_UNLOCK(&pAd->MacTabLock);
	RTMP_SEM_UNLOCK(&pAd->MeshTabLock);

	return pEntry;
}

MAC_TABLE_ENTRY *FindMeshEntry(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR 			Wcid,
	IN PUCHAR			pAddr)
{
	MAC_TABLE_ENTRY *pEntry;

	// lookup the match wds entry for the incoming packet.
	pEntry = MeshTableLookupByWcid(pAd, Wcid, pAddr, TRUE);

	return pEntry;
}

VOID MlmeHandleRxMeshFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk)
{
	PHEADER_802_11					pHeader = pRxBlk->pHeader;

	DBGPRINT(RT_DEBUG_INFO, ("-----> MlmeHandleRxMeshFrame\n"));

	if(!MESH_ON(pAd))
		return;

	//
	// handle Mesh
	//
	if (pHeader->FC.SubType == SUBTYPE_MULTIHOP)
		CmmRxMeshMgmtFrameHandle(pAd, pRxBlk);

	DBGPRINT(RT_DEBUG_INFO, ("<----- MlmeHandleRxMeshFrame\n"));
}

VOID CmmRxMeshMgmtFrameHandle(
	IN	PRTMP_ADAPTER	pAd,
	IN	RX_BLK			*pRxBlk)
{
	int i;
	PHEADER_802_11					pHeader = pRxBlk->pHeader;
//	PNDIS_PACKET					pRxPacket = pRxBlk->pRxPacket;
	UCHAR							Category;
	UCHAR							ActionField;
	UCHAR							MeshHdrLen;

	DBGPRINT(RT_DEBUG_INFO, ("-----> CmmRxMeshMgmtFrameHandle\n"));

#ifdef WDS_SUPPORT
	if (WdsTableLookup(pAd, pHeader->Addr2, FALSE) != NULL)
	{
		DBGPRINT(RT_DEBUG_INFO, ("%s() Peer WDS entry.\n", __FUNCTION__));
		return;
	}
#endif // WDS_SUPPORT //


	pRxBlk->pData = (UCHAR *)pHeader;
	pRxBlk->pData += LENGTH_802_11;
	pRxBlk->DataSize -= LENGTH_802_11;

	MeshHdrLen = GetMeshHederLen(pRxBlk->pData);
	// get Category
	NdisMoveMemory(&Category, pRxBlk->pData + MeshHdrLen, 1);
	// get ActionField
	NdisMoveMemory(&ActionField, pRxBlk->pData + MeshHdrLen + 1, 1);

	for (i = 0;  i < MESH_ACT_HANDLER_TAB_SIZE; i++)
	{
		if ((Category == MeshActHandler[i].Category)
			&& (ActionField == MeshActHandler[i].ActionCode)
			&& (MeshActHandler[i].pHandle))
		{
			(*(MeshActHandler[i].pHandle))(pAd, pRxBlk);
			break;
		}
	}

	DBGPRINT(RT_DEBUG_INFO, ("<----- CmmRxMeshMgmtFrameHandle\n"));
}

LONG
PathRouteIDSearch(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR	pAddr)
{
	MESH_ENTRY *pEntry = NULL;
	UCHAR	DestAddr[MAC_ADDR_LEN];

	DBGPRINT(RT_DEBUG_INFO, ("-----> PathRouteIDSearch\n"));

	if (*pAddr & 0x01) // B/Mcast packet.
		return BMCAST_ROUTE_ID;

	COPY_MAC_ADDR(DestAddr, pAddr);
	pEntry = MeshEntryTableLookUp(pAd, DestAddr);

	if (pEntry)
	{
		if (pEntry->PathReqTimerRunning)
		{
			DBGPRINT(RT_DEBUG_INFO, ("%s ---> PathReqTimerRunning is TRUE\n", __FUNCTION__));
			return -1;
		}
		else
			return (ULONG)pEntry->Idx;
	}

	DBGPRINT(RT_DEBUG_INFO, ("<----- PathRouteIDSearch\n"));

	return -1;
}

PUCHAR
PathRouteAddrSearch(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR	RouteIdx)
{
	MESH_ROUTING_ENTRY *pEntry = NULL;
	PMESH_ROUTING_TABLE	pRouteTab = pAd->MeshTab.pMeshRouteTab;

	if (pRouteTab == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Mesh Route Table doesn't exist.\n", __FUNCTION__));
		return NULL;
	}

	if (RouteIdx >= MAX_ROUTE_TAB_SIZE)
		return NULL;

	if (pRouteTab->Content[RouteIdx].Valid == TRUE)
	{
		pEntry = &pRouteTab->Content[RouteIdx];
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Can't find the Route Index = (%d)\n", (int)RouteIdx));
		return NULL;
	}

	return pEntry->MeshDA;
}

UCHAR
PathMeshLinkIDSearch(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR	RouteIdx)
{
	MESH_ROUTING_ENTRY *pEntry = NULL;
	PMESH_ROUTING_TABLE	pRouteTab = pAd->MeshTab.pMeshRouteTab;

	if (pRouteTab == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: Mesh Route Table doesn't exist.\n", __FUNCTION__));
		return 0;
	}

	if (RouteIdx >= MAX_ROUTE_TAB_SIZE)
		return 0;

	if (pRouteTab->Content[RouteIdx].Valid == TRUE)
	{
		pEntry = &pRouteTab->Content[RouteIdx];
		return pEntry->NextHopLinkID;
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Can't find the Route Index = (%d)\n", (int)RouteIdx));
	}

	return 0;
}

UINT GetMeshHederLen(
	IN PUCHAR pSrcBufVA)
{
	UINT MeshHdrLen = 0;

	switch(GetMeshFlagAE(pSrcBufVA))
	{
		case 0:
			MeshHdrLen = 5;
			break;
		case 1:
			MeshHdrLen = 11;
			break;
		case 2:
			MeshHdrLen = 17;
			break;
		case 3:
			MeshHdrLen = 23;
			break;
		default:
			DBGPRINT(RT_DEBUG_ERROR, ("%s: Undown Mesh AE type=%x\n", __FUNCTION__, GetMeshFlagAE(pSrcBufVA)));
			break;
	}

	return MeshHdrLen; 
}

UINT8 GetMeshFlag(
	IN PUCHAR pSrcBufVA)
{
	PMESH_HEADER pMeshHead;

	pMeshHead = (PMESH_HEADER)(pSrcBufVA);

	return pMeshHead->MeshFlag; 
}

UINT8 GetMeshFlagAE(
	IN PUCHAR pSrcBufVA)
{
	PMESH_HEADER pMeshHead;

	pMeshHead = (PMESH_HEADER)(pSrcBufVA);

	return (UINT8)((PMESH_FLAG)&pMeshHead->MeshFlag)->field.AE;
}

UINT8 GetMeshTTL(
	IN PUCHAR pSrcBufVA)
{
	PMESH_HEADER pMeshHead;

	pMeshHead = (PMESH_HEADER)(pSrcBufVA);

	return pMeshHead->MeshTTL; 
}

UINT32 GetMeshSeq(
	IN PUCHAR pSrcBufVA)
{
	PMESH_HEADER pMeshHead;

	pMeshHead = (PMESH_HEADER)(pSrcBufVA);

	return (UINT32)((pMeshHead->MeshSeq[2] << 16) + (pMeshHead->MeshSeq[1] << 8) + (pMeshHead->MeshSeq[0]));
}

PUCHAR GetMeshAddr4(
	IN PUCHAR pSrcBufVA)
{
	PMESH_HEADER pMeshHead;

	pMeshHead = (PMESH_HEADER)(pSrcBufVA);
	if (((PMESH_FLAG)&pMeshHead->MeshFlag)->field.AE == 1
		|| ((PMESH_FLAG)&pMeshHead->MeshFlag)->field.AE == 3)
		return (PUCHAR)(pSrcBufVA + 5);
	else
		return NULL; 
}

PUCHAR GetMeshAddr5(
	IN PUCHAR pSrcBufVA)
{
	PMESH_HEADER pMeshHead;

	pMeshHead = (PMESH_HEADER)(pSrcBufVA);
	if (((PMESH_FLAG)&pMeshHead->MeshFlag)->field.AE == 2)
		return (PUCHAR)(pSrcBufVA + 5);
	else if (((PMESH_FLAG)&pMeshHead->MeshFlag)->field.AE == 3)
		return (PUCHAR)(pSrcBufVA + 5 + MAC_ADDR_LEN);
	else
		return NULL; 
}

PUCHAR GetMeshAddr6(
	IN PUCHAR pSrcBufVA)
{
	PMESH_HEADER pMeshHead;

	pMeshHead = (PMESH_HEADER)(pSrcBufVA);
	if (((PMESH_FLAG)&pMeshHead->MeshFlag)->field.AE == 2)
		return (PUCHAR)(pSrcBufVA + 5 + MAC_ADDR_LEN);
	else if (((PMESH_FLAG)&pMeshHead->MeshFlag)->field.AE == 3)
		return (PUCHAR)(pSrcBufVA + 5 + MAC_ADDR_LEN + MAC_ADDR_LEN);
	else
		return NULL; 
}

ULONG GetMeshLinkId(
	IN PRTMP_ADAPTER pAd,
	IN PCHAR PeerMacAddr)
{
	ULONG i;

	for (i = 0; i < MAX_MESH_LINKS; i++)
	{
		if (MAC_ADDR_EQUAL(PeerMacAddr, pAd->MeshTab.MeshLink[i].Entry.PeerMacAddr))
			break;
	}

	if (i == MAX_MESH_LINKS)
		return BSS_NOT_FOUND;

	return (ULONG)i;
}

VOID MeshDataPktProcess(
	IN PRTMP_ADAPTER pAd,
	IN PNDIS_PACKET pPacket,
	IN USHORT MeshLinkIdx,
	OUT PNDIS_PACKET *pMeshForwardPacket,
	OUT BOOLEAN *pbDirectForward,
	OUT BOOLEAN *pbAnnounce)

{
	PUCHAR pHeader802_3 = GET_OS_PKT_DATAPTR(pPacket);
	PUCHAR pMeshHdr = pHeader802_3 + LENGTH_802_3;
	UINT MeshHdrLen = GetMeshHederLen(pMeshHdr);
	UINT8 MeshTTL = GetMeshTTL(pMeshHdr);
	UINT8 MeshFlagAE = GetMeshFlagAE(pMeshHdr);

	do
	{
		*pbAnnounce = FALSE;
		*pbDirectForward = FALSE;

		if (!MESH_ON(pAd))
			break;

		if (*pHeader802_3 & 0x01)
		{
			if (--MeshTTL > 0)
				*pbDirectForward = TRUE;
			else
				*pbDirectForward = FALSE;
			*pbAnnounce = TRUE;
		}
		else
		{
			if (MAC_ADDR_EQUAL(pHeader802_3, pAd->MeshTab.CurrentAddress)
				|| MeshProxyEntryTableLookUp(pAd, pHeader802_3) != NULL)
			{
				*pbAnnounce = TRUE;
				*pbDirectForward = FALSE;
			}
			else
			{
				if (--MeshTTL > 0)
					*pbDirectForward = TRUE;
				else
					*pbDirectForward = FALSE;
				*pbAnnounce = FALSE;
			}
		}

		if (*pbDirectForward == TRUE)
		{
			PUCHAR pFwdPktHeader = NULL;
			if (*pbAnnounce == TRUE)
			{
				*pMeshForwardPacket = (PNDIS_PACKET)skb_copy(RTPKT_TO_OSPKT(pPacket), GFP_ATOMIC);
				if (*pMeshForwardPacket == NULL)
				{
					ASSERT(*pMeshForwardPacket);
					*pbAnnounce = FALSE;
					*pbDirectForward = FALSE;
					break;
				}
				pFwdPktHeader = GET_OS_PKT_DATAPTR(*pMeshForwardPacket);
			}
			else
				pFwdPktHeader = pHeader802_3;
			
			// override eth type/len field.
			// ApHardTransmit will check the field to decide that to add LLC or not.
			// Thus override eth type/lens field here let ApHardTransmit added LLC for the packet.
			NdisMoveMemory(pFwdPktHeader + 12, pFwdPktHeader + LENGTH_802_3 + MeshHdrLen + MAC_ADDR_LEN, 2);
		}

		if (*pbAnnounce == TRUE)
		{
			PUCHAR pSrcBuf;	
			UINT Offset;

			pSrcBuf = pMeshHdr + MeshHdrLen + MAC_ADDR_LEN; // skip Mesh header and LLC header.
			if (MeshFlagAE == 2)
			{	// the lenght of hdr shall be 16 bytes here.
				COPY_MAC_ADDR(pSrcBuf -= MAC_ADDR_LEN, GetMeshAddr6(pMeshHdr));		// copy Addr6 of mesh hdr to 802.3 header.
				COPY_MAC_ADDR(pSrcBuf -= MAC_ADDR_LEN, GetMeshAddr5(pMeshHdr)); 	// copy Addr5 of mesh hdr to 802.3 header.
			}
			else if(MeshFlagAE == 1)
			{
				// Mesh Data frame never AE=1.
				// drop the frame.
				DBGPRINT(RT_DEBUG_ERROR, ("%s: Receive Mesh-Data frame carry AE=1. Drop the frame.\n", __FUNCTION__));
				*pbAnnounce = FALSE;
				*pbDirectForward = FALSE;
				if (*pMeshForwardPacket != NULL)
					RELEASE_NDIS_PACKET(pAd, *pMeshForwardPacket, NDIS_STATUS_FAILURE);
				*pMeshForwardPacket = NULL;
				break;
			}
			else
			{
				COPY_MAC_ADDR(pSrcBuf -= MAC_ADDR_LEN, pHeader802_3 + MAC_ADDR_LEN);	// copy SA to 802.3 header.
				COPY_MAC_ADDR(pSrcBuf -= MAC_ADDR_LEN, pHeader802_3); 					// copy DA to 802.3 header.
			}
			Offset = pSrcBuf - pHeader802_3;
			GET_OS_PKT_DATAPTR(pPacket) = pSrcBuf;
			GET_OS_PKT_LEN(pPacket) -= Offset;
		}
	} while (FALSE);

	return;
}

ULONG InsertPktMeshHeader(
	IN PRTMP_ADAPTER pAd,
	IN TX_BLK *pTxBlk, 
	IN PUCHAR *pHeaderBufPtr)
{
	ULONG TempLen = 0;
	MESH_FLAG MeshFlag;
	UINT16 MeshTTL;
	UINT32 MeshSeq;
	PUCHAR pMeshAddr5 = NULL;
	PUCHAR pMeshAddr6 = NULL;

	PerpareMeshHeader(pAd, pTxBlk, &MeshFlag, &MeshTTL, &MeshSeq, &pMeshAddr5, &pMeshAddr6);
	InsertMeshHeader(pAd, *pHeaderBufPtr, &TempLen, MeshFlag.word, MeshTTL, MeshSeq,
						NULL, pMeshAddr5, pMeshAddr6);

	*pHeaderBufPtr += TempLen;
	pTxBlk->MpduHeaderLen += TempLen;

	return TempLen;
}

UINT32 RandomMeshCPI(
	IN PRTMP_ADAPTER pAd)
{
	return (UINT32)((RandomByte(pAd) << 8) + (RandomByte(pAd)));
}

UINT16 RandomLinkId(
	IN PRTMP_ADAPTER pAd)
{
	return (UINT16)((RandomByte(pAd) << 8) + (RandomByte(pAd)));
}

UINT8 RandomChSwWaitTime(
	IN PRTMP_ADAPTER pAd)
{
	UINT8 ChSwCnt = RandomByte(pAd);

	ChSwCnt = (ChSwCnt >= 50) ? ChSwCnt : (UINT8)(50 + RandomByte(pAd));
	return ChSwCnt;
}

void rtmp_read_mesh_from_file(
	IN PRTMP_ADAPTER pAd,
	char *tmpbuf,
	char *buffer)
{
	UCHAR		keyMaterial[40];
	ULONG		KeyIdx;
	ULONG		KeyLen;

	//MeshId
	if(RTMPGetKeyParameter("MeshId", tmpbuf, 255, buffer))
	{
		//MeshId acceptable strlen must be less than 32 and bigger than 0.
		if((strlen(tmpbuf) < 0) || (strlen(tmpbuf) > 32))
			pAd->MeshTab.MeshIdLen = 0;
		else
			pAd->MeshTab.MeshIdLen = strlen(tmpbuf);

		if(pAd->MeshTab.MeshIdLen > 0)
		{
			NdisMoveMemory(&pAd->MeshTab.MeshId, tmpbuf, pAd->MeshTab.MeshIdLen);
		}
		else
		{
			NdisZeroMemory(&pAd->MeshTab.MeshId, MAX_MESH_ID_LEN);
		}
		DBGPRINT(RT_DEBUG_TRACE, ("MeshIdLen=%d, MeshId=%s\n", pAd->MeshTab.MeshIdLen, pAd->MeshTab.MeshId));
	}

	//MeshAutoLink
	if (RTMPGetKeyParameter("MeshAutoLink", tmpbuf, 255, buffer))
	{
		LONG Enable;
		Enable = simple_strtol(tmpbuf, 0, 10);
		pAd->MeshTab.MeshAutoLink = (Enable > 0) ? TRUE : FALSE;
		DBGPRINT(RT_DEBUG_TRACE, ("%s::(MeshAutoLink=%d)\n", __FUNCTION__, pAd->MeshTab.MeshAutoLink));
	}
	else 
		pAd->MeshTab.MeshAutoLink = TRUE;

	//MeshAuthMode
	if (RTMPGetKeyParameter("MeshAuthMode", tmpbuf, 255, buffer))
	{										
		if ((strncmp(tmpbuf, "WPANONE", 7) == 0) || (strncmp(tmpbuf, "wpanone", 7) == 0))
			pAd->MeshTab.AuthMode = Ndis802_11AuthModeWPANone;				
		else
			pAd->MeshTab.AuthMode = Ndis802_11AuthModeOpen;
			
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) MeshAuthMode(%d)=%s \n", pAd->MeshTab.AuthMode, GetAuthMode(pAd->MeshTab.AuthMode)));
		MakeMeshRSNIE(pAd, pAd->MeshTab.AuthMode, pAd->MeshTab.WepStatus);		

		if(pAd->MeshTab.AuthMode >= Ndis802_11AuthModeWPA)
		{	
			if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPANone)
				pAd->MeshTab.DefaultKeyId = 0;
			else
				pAd->MeshTab.DefaultKeyId = 1;
		}

	}

	//MeshEncrypType
	if (RTMPGetKeyParameter("MeshEncrypType", tmpbuf, 255, buffer))
	{										
		if ((strncmp(tmpbuf, "WEP", 3) == 0) || (strncmp(tmpbuf, "wep", 3) == 0))
        {
			if (pAd->MeshTab.AuthMode < Ndis802_11AuthModeWPA)
				pAd->MeshTab.WepStatus = Ndis802_11WEPEnabled;				  
		}
		else if ((strncmp(tmpbuf, "TKIP", 4) == 0) || (strncmp(tmpbuf, "tkip", 4) == 0))
		{
			if (pAd->MeshTab.AuthMode >= Ndis802_11AuthModeWPA)
				pAd->MeshTab.WepStatus = Ndis802_11Encryption2Enabled;                       
        }
		else if ((strncmp(tmpbuf, "AES", 3) == 0) || (strncmp(tmpbuf, "aes", 3) == 0))
		{
			if (pAd->MeshTab.AuthMode >= Ndis802_11AuthModeWPA)
				pAd->MeshTab.WepStatus = Ndis802_11Encryption3Enabled;                            
		}    
		else
		{
			pAd->MeshTab.WepStatus = Ndis802_11WEPDisabled;                 
		}
							
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) MeshEncrypType(%d)=%s \n", pAd->MeshTab.WepStatus, GetEncryptType(pAd->MeshTab.WepStatus)));
		MakeMeshRSNIE(pAd, pAd->MeshTab.AuthMode, pAd->MeshTab.WepStatus);
		
	}
	
	//MeshWPAKEY
	if (RTMPGetKeyParameter("MeshWPAKEY", tmpbuf, 255, buffer))
	{
		// The WPA KEY must be 8~64 characters
		if((strlen(tmpbuf) >= 8) && (strlen(tmpbuf) <= 64))
		{
			NdisMoveMemory(pAd->MeshTab.WPAPassPhraseKey, tmpbuf, strlen(tmpbuf));
			pAd->MeshTab.WPAPassPhraseKeyLen = strlen(tmpbuf);
						
			if (strlen(tmpbuf) == 64)
			{// Hex mode
				AtoH(tmpbuf, pAd->MeshTab.PMK, 32);
			}
			else
			{// ASCII mode
				PasswordHash((char *)tmpbuf, pAd->MeshTab.MeshId, pAd->MeshTab.MeshIdLen, keyMaterial);
				NdisMoveMemory(pAd->MeshTab.PMK, keyMaterial, 32);
			}	
			DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) MeshWPAKEY=%s, its length=%d\n", pAd->MeshTab.WPAPassPhraseKey, pAd->MeshTab.WPAPassPhraseKeyLen));
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("I/F(mesh0) set MeshWPAKEY fail, key string required 8 ~ 64 characters!!!\n"));
		}
																				
	}

	//MeshDefaultkey
	if (RTMPGetKeyParameter("MeshDefaultkey", tmpbuf, 255, buffer))
	{								
		KeyIdx = simple_strtol(tmpbuf, 0, 10);
		if((KeyIdx >= 1 ) && (KeyIdx <= 4))
			pAd->MeshTab.DefaultKeyId = (UCHAR) (KeyIdx - 1);
		else
			pAd->MeshTab.DefaultKeyId = 0;

		DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) DefaultKeyID(0~3)=%d\n", pAd->MeshTab.DefaultKeyId));		
	}

	//MeshWEPKEY	
	if (RTMPGetKeyParameter("MeshWEPKEY", tmpbuf, 255, buffer))
	{		    		                
		KeyLen = strlen(tmpbuf);
			
		// Hex type
		if((KeyLen == 10) || (KeyLen == 26))
		{
			pAd->MeshTab.SharedKey.KeyLen = KeyLen / 2;
			AtoH(tmpbuf, pAd->MeshTab.SharedKey.Key, KeyLen / 2);
			if (KeyLen == 10)
				pAd->MeshTab.SharedKey.CipherAlg = CIPHER_WEP64;
			else
				pAd->MeshTab.SharedKey.CipherAlg = CIPHER_WEP128;

			NdisMoveMemory(pAd->MeshTab.DesiredWepKey, tmpbuf, KeyLen);
			pAd->MeshTab.DesiredWepKeyLen = KeyLen;
							
			DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) MeshWEPKEY=%s, it's HEX type and %s\n", tmpbuf, (KeyLen == 10) ? "wep64":"wep128"));
		}
		// ASCII type
		else if ((KeyLen == 5) || (KeyLen == 13))													
		{
			pAd->MeshTab.SharedKey.KeyLen = KeyLen;
			NdisMoveMemory(pAd->MeshTab.SharedKey.Key, tmpbuf, KeyLen);
			if (KeyLen == 5)
				pAd->MeshTab.SharedKey.CipherAlg = CIPHER_WEP64;
			else
				pAd->MeshTab.SharedKey.CipherAlg = CIPHER_WEP128;

			NdisMoveMemory(pAd->MeshTab.DesiredWepKey, tmpbuf, KeyLen);
			pAd->MeshTab.DesiredWepKeyLen = KeyLen;
									
			DBGPRINT(RT_DEBUG_TRACE, ("I/F(mesh0) MeshWEPKEY=%s, it's ASCII type and %s\n", tmpbuf, (KeyLen == 5) ? "wep64":"wep128"));
		}
		//Invalid key length
		else
		{ 
			pAd->MeshTab.DesiredWepKeyLen = 0;
			DBGPRINT(RT_DEBUG_ERROR, ("I/F(mesh0) MeshWEPKEY is Invalid key length(%d)!\n", (UCHAR)KeyLen));
		}
	}	

	return;
}

static VOID MakeMeshRSNIE(
    IN  PRTMP_ADAPTER   pAd,
    IN  UINT            AuthMode,
    IN  UINT            WepStatus)
{
		
	// Only support WPANONE, WPA2PSK or WPA2 for Mesh Link    
    if ((AuthMode != Ndis802_11AuthModeWPANone)/* && 
		(AuthMode != Ndis802_11AuthModeWPA2PSK) && 
		(AuthMode != Ndis802_11AuthModeWPA2)*/)
		return;

	// ONLY support TKIP or AES, not mix mode 
	if ((WepStatus != Ndis802_11Encryption2Enabled) && (WepStatus != Ndis802_11Encryption3Enabled))
		return;
	
	pAd->MeshTab.RSNIE_Len = 0;
	NdisZeroMemory(pAd->MeshTab.RSN_IE, MAX_LEN_OF_RSNIE);
	
    // For WPA1, RSN_IE=221
    if (AuthMode == Ndis802_11AuthModeWPANone)
    {
        RSNIE               *pRsnie;
        RSNIE_AUTH          *pRsnie_auth;
        RSN_CAPABILITIES    *pRSN_Cap;
        UCHAR               Rsnie_size = 0;
		
		pRsnie = (RSNIE*)pAd->MeshTab.RSN_IE;
		
        NdisMoveMemory(pRsnie->oui, OUI_WPA_WEP40, 4);
        pRsnie->version = 1;

        switch (WepStatus)
        {
            case Ndis802_11Encryption2Enabled:
                NdisMoveMemory(pRsnie->mcast, OUI_WPA_TKIP, 4);
                pRsnie->ucount = 1;
                NdisMoveMemory(pRsnie->ucast[0].oui, OUI_WPA_TKIP, 4);
                Rsnie_size = sizeof(RSNIE);
                break;

            case Ndis802_11Encryption3Enabled:		
				NdisMoveMemory(pRsnie->mcast, OUI_WPA_CCMP, 4);								
                pRsnie->ucount = 1;
                NdisMoveMemory(pRsnie->ucast[0].oui, OUI_WPA_CCMP, 4);
                Rsnie_size = sizeof(RSNIE);
                break;						
        }

        pRsnie_auth = (RSNIE_AUTH*)((PUCHAR)pRsnie + Rsnie_size);

        switch (AuthMode)
        {
            case Ndis802_11AuthModeWPA:            
                pRsnie_auth->acount = 1;
                NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA_WEP40, 4);
                break;

            case Ndis802_11AuthModeWPAPSK:            
                pRsnie_auth->acount = 1;
                NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA_TKIP, 4);
                break;

			case Ndis802_11AuthModeWPANone:
                pRsnie_auth->acount = 1;
                NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA_NONE_AKM, 4);
                break;
        }

        pRSN_Cap = (RSN_CAPABILITIES*)((PUCHAR)pRsnie_auth + sizeof(RSNIE_AUTH));
		
	    pRsnie->version = cpu2le16(pRsnie->version);
	    pRsnie->ucount = cpu2le16(pRsnie->ucount);
	    pRsnie_auth->acount = cpu2le16(pRsnie_auth->acount);
		pRSN_Cap->word = cpu2le16(pRSN_Cap->word);

		pAd->MeshTab.RSNIE_Len = Rsnie_size + sizeof(RSNIE_AUTH) + sizeof(RSN_CAPABILITIES);

    }

    // Todo WPA2 RSNIE 
    	
	DBGPRINT(RT_DEBUG_TRACE,("==> MakeMeshRSNIE is completed, the length is %d \n", pAd->MeshTab.RSNIE_Len));
	

}


VOID LocalMsaIeInit(
	IN PRTMP_ADAPTER pAd,
	IN INT			 idx)
{
	PMSA_HANDSHAKE_IE	pLocalMsaIe;

	// clear the local MP's MSAIE field
	NdisZeroMemory(pAd->MeshTab.MeshLink[idx].Entry.LocalMsaIe, MESH_MAX_MSAIE_LEN);	
	pAd->MeshTab.MeshLink[idx].Entry.LocalMsaIeLen = 0;

	pLocalMsaIe = (PMSA_HANDSHAKE_IE)pAd->MeshTab.MeshLink[idx].Entry.LocalMsaIe;

	// Requests Authentication subfield of the Handshake Control field shall be set to 1 if the local MP
	// requests Initial MSA Authentication during this MSA authentication mechanism. This subfield
	// shall be set to zero if the PMKID list field of the RSNIE contains one or more entries.
	pLocalMsaIe->MeshHSControl.word = 0;	
	if (pAd->MeshTab.bInitialMsaDone == FALSE || pAd->MeshTab.PMKID_Len == 0)
		pLocalMsaIe->MeshHSControl.field.RequestAuth = 1;	
	
	// Selected AKM and Pairwise-Cipher Suite
	// If the local MP is the Selector MP, the field shall contain the local MP's selection 	
	if (pAd->MeshTab.MeshLink[idx].Entry.bValidLocalMpAsSelector)
	{
		UCHAR	AuthMode = pAd->MeshTab.AuthMode;
		UCHAR	EncrypType = pAd->MeshTab.WepStatus;
	
		switch (AuthMode)
        {
           	case Ndis802_11AuthModeWPA2:            	
               	NdisMoveMemory(pLocalMsaIe->SelectedAKM, OUI_MSA_8021X, OUI_SUITE_LEN);
            break;

          	case Ndis802_11AuthModeWPA2PSK:
           		NdisMoveMemory(pLocalMsaIe->SelectedAKM, OUI_MSA_PSK, OUI_SUITE_LEN);
            break;

			//default:
			case Ndis802_11AuthModeWPANone:                
	            NdisMoveMemory(pLocalMsaIe->SelectedAKM, OUI_WPA_NONE_AKM, OUI_SUITE_LEN);
            break;
        }	
			
		switch (EncrypType)
        {
           	case Ndis802_11Encryption2Enabled:  
				if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPANone)
	               	NdisMoveMemory(pLocalMsaIe->SelectedPairwiseCipher, OUI_WPA_TKIP, OUI_SUITE_LEN);
				else
					NdisMoveMemory(pLocalMsaIe->SelectedPairwiseCipher, OUI_WPA2_TKIP, OUI_SUITE_LEN);
			break;

          	case Ndis802_11Encryption3Enabled:
           		if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPANone)
	               	NdisMoveMemory(pLocalMsaIe->SelectedPairwiseCipher, OUI_WPA_CCMP, OUI_SUITE_LEN);
				else
					NdisMoveMemory(pLocalMsaIe->SelectedPairwiseCipher, OUI_WPA2_CCMP, OUI_SUITE_LEN);
            break;			
        }			
	}

	pAd->MeshTab.MeshLink[idx].Entry.LocalMsaIeLen = sizeof(MSA_HANDSHAKE_IE);

}

/* 
    ==========================================================================
    Description:
        It shall be queried about mesh security information through IOCTL     
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlQueryMeshSecurityInfo(
		IN PRTMP_ADAPTER pAd, 
		IN struct iwreq *wrq)
{
	UCHAR	key_len = 0;
	MESH_SECURITY_INFO	meshInfo;

	DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlQueryMeshSecurityInfo==>\n"));
	
	NdisZeroMemory((PUCHAR)&meshInfo, sizeof(MESH_SECURITY_INFO));
																																	
	if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeOpen && pAd->MeshTab.WepStatus == Ndis802_11Encryption1Enabled)
		meshInfo.EncrypType = ENCRYPT_OPEN_WEP;		// 1 - 	OPEN-WEP	
	else if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPANone && pAd->MeshTab.WepStatus == Ndis802_11Encryption2Enabled)
		meshInfo.EncrypType = ENCRYPT_WPANONE_TKIP;	// 2 -	WPANONE-TKIP
	else if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPANone && pAd->MeshTab.WepStatus == Ndis802_11Encryption3Enabled)
		meshInfo.EncrypType = ENCRYPT_WPANONE_AES;	// 3 -	WPANONE-AES
	else
		meshInfo.EncrypType = ENCRYPT_OPEN_NONE;	// 0 - 	OPEN-NONE

	meshInfo.KeyIndex = pAd->MeshTab.DefaultKeyId + 1;
				
	if (meshInfo.EncrypType == ENCRYPT_OPEN_WEP)
	{		
		key_len = pAd->MeshTab.DesiredWepKeyLen;		
		if (key_len > 0)
		{					
			meshInfo.KeyLength = key_len;			
			NdisMoveMemory(meshInfo.KeyMaterial, pAd->MeshTab.DesiredWepKey, key_len);
		}
	}
	else if (meshInfo.EncrypType == ENCRYPT_WPANONE_TKIP || meshInfo.EncrypType == ENCRYPT_WPANONE_AES)
	{
		key_len = pAd->MeshTab.WPAPassPhraseKeyLen;
		if (key_len > 0)
		{								
			meshInfo.KeyLength = key_len;
			NdisMoveMemory(meshInfo.KeyMaterial, pAd->MeshTab.WPAPassPhraseKey, key_len);
		}
	}
		
	wrq->u.data.length = sizeof(MESH_SECURITY_INFO);
	if (copy_to_user(wrq->u.data.pointer, &meshInfo, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

}


/* 
    ==========================================================================
    Description:
        It shall be set mesh security through IOCTL        
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
INT RTMPIoctlSetMeshSecurityInfo(
		IN PRTMP_ADAPTER pAd, 
		IN struct iwreq *wrq)
{	
	INT 	Status = NDIS_STATUS_SUCCESS;
	MESH_SECURITY_INFO	meshInfo;

	DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlSetMeshSecurityInfo==>\n"));

	if (wrq->u.data.length != sizeof(MESH_SECURITY_INFO))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: the length is too large \n", __FUNCTION__));
		return -EINVAL;
	}

	if (copy_from_user(&meshInfo, wrq->u.data.pointer, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("%s: copy_from_user() fail\n", __FUNCTION__));
		return -EFAULT;
	}

	// Default security mode is OPEN-NONE
	pAd->MeshTab.AuthMode = Ndis802_11AuthModeOpen;
	pAd->MeshTab.WepStatus = Ndis802_11EncryptionDisabled;
	NdisZeroMemory((PUCHAR)&pAd->MeshTab.SharedKey, sizeof(CIPHER_KEY));

	// Set default key index
	if((meshInfo.KeyIndex >= 1) && (meshInfo.KeyIndex <= 4))
		pAd->MeshTab.DefaultKeyId = meshInfo.KeyIndex - 1;
	else
		pAd->MeshTab.DefaultKeyId = 0;
		
	// OPEN-WEP
	if (meshInfo.EncrypType == ENCRYPT_OPEN_WEP)					
	{
		if (!Set_MeshWEPKEY_Proc(pAd, meshInfo.KeyMaterial))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("%s: Mesh WEP key isn't valid \n", __FUNCTION__));
			return -EFAULT;
		}

		pAd->MeshTab.AuthMode = Ndis802_11AuthModeOpen;
		pAd->MeshTab.WepStatus = Ndis802_11Encryption1Enabled;							
	}
	else if (meshInfo.EncrypType == ENCRYPT_WPANONE_TKIP || meshInfo.EncrypType == ENCRYPT_WPANONE_AES) 
	{
		if (!Set_MeshWPAKEY_Proc(pAd, meshInfo.KeyMaterial))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("%s: Mesh WPA key isn't valid \n", __FUNCTION__));
			return -EFAULT;
		}

		pAd->MeshTab.AuthMode = Ndis802_11AuthModeWPANone;
		if (meshInfo.EncrypType == ENCRYPT_WPANONE_TKIP)
			pAd->MeshTab.WepStatus = Ndis802_11Encryption2Enabled;
		else
			pAd->MeshTab.WepStatus = Ndis802_11Encryption3Enabled;

		MakeMeshRSNIE(pAd, pAd->MeshTab.AuthMode, pAd->MeshTab.WepStatus);
	}
			
	return Status;

}

UCHAR MeshCheckPeerMpCipher(
		IN USHORT 		 CapabilityInfo, 
		IN PUCHAR 		 pVIE,
		IN UCHAR		 LenVIE)
{
	UCHAR	EncrypType = ENCRYPT_OPEN_NONE;
	PUCHAR 	pTmp;	
	
	if (CAP_IS_PRIVACY_ON(CapabilityInfo))
	{
		if (LenVIE > sizeof(RSNIE))
		{
			pTmp = pVIE;

			// skip IE and length - 2 bytes 
			pTmp += 2;
			
			// skip OUI(4),Vesrion(2),Multicast cipher(4) 4+2+4==10
			pTmp += 10;				//point to number of unicast

			// skip number of unicast(2)
			pTmp += sizeof(USHORT);	//pointer to unicast cipher

			if(RTMPEqualMemory(OUI_WPA_TKIP, pTmp, 4))
				EncrypType = ENCRYPT_WPANONE_TKIP;
			else if (RTMPEqualMemory(OUI_WPA_CCMP, pTmp, 4))
				EncrypType = ENCRYPT_WPANONE_AES;			
		}
		else
		{
			EncrypType = ENCRYPT_OPEN_WEP;	
		}
														
	}

	return EncrypType;
}

BOOLEAN MeshAllowToSendPacket(
	IN RTMP_ADAPTER *pAd,
	IN PNDIS_PACKET pPacket,
	OUT UCHAR		*pWcid)
{
	BOOLEAN	allowed = FALSE;
	PMESH_PROXY_ENTRY	pMeshProxyEntry = NULL;
	PUCHAR pSrcBufVA = GET_OS_PKT_DATAPTR(pPacket);

	if (MESH_ON(pAd))
	{
		LONG RouteId = PathRouteIDSearch(pAd, pSrcBufVA);

		if (!MAC_ADDR_EQUAL(pAd->MeshTab.CurrentAddress, pSrcBufVA + MAC_ADDR_LEN))
		{
			pMeshProxyEntry = MeshProxyEntryTableLookUp(pAd, pSrcBufVA + MAC_ADDR_LEN);

			if (!pMeshProxyEntry)
			{
				pMeshProxyEntry = MeshProxyEntryTableInsert(pAd, pAd->MeshTab.CurrentAddress, pSrcBufVA + MAC_ADDR_LEN);
			}
		}

		if (RouteId == BMCAST_ROUTE_ID)
		{
			MeshClonePacket(pAd, pPacket, MESH_PROXY, 0);
		}
		else if (RouteId >= 0)
		{
			*pWcid = pAd->MeshTab.MeshLink[PathMeshLinkIDSearch(pAd, RouteId)].Entry.MacTabMatchWCID;
			RTMP_SET_MESH_ROUTE_ID(pPacket, (UINT8)RouteId);

			// MESH_PROXY indicate the packet come from os layer to mesh0 virtual interface.
			RTMP_SET_MESH_SOURCE(pPacket, MESH_PROXY);
			allowed = TRUE;
		}
		else
		{
			// entity is not exist.
			// start path discovery.
			if (MAC_ADDR_EQUAL(pSrcBufVA + MAC_ADDR_LEN, pAd->MeshTab.CurrentAddress))
				MeshCreatePreqAction(pAd, NULL, pSrcBufVA);
			else
				MeshCreatePreqAction(pAd, pSrcBufVA + MAC_ADDR_LEN, pSrcBufVA);
		}
	}
 
	return allowed;
}

VOID PerpareMeshHeader(
	IN RTMP_ADAPTER *pAd,
	IN TX_BLK *pTxBlk,
	OUT PMESH_FLAG pMeshFlag,
	OUT UINT16 *pMeshTTL,
	OUT UINT32 *pMeshSeq,
	OUT PUCHAR *ppMeshAddr5,
	OUT PUCHAR *ppMeshAddr6)
{
	UINT MeshHdrLen = 0;
	PNDIS_PACKET pPacket = pTxBlk->pPacket;
	PUCHAR pSrcBufVA = GET_OS_PKT_DATAPTR(pPacket);
	PHEADER_802_11 pHeader_802_11 = (PHEADER_802_11)&pTxBlk->HeaderBuf[TXINFO_SIZE + TXWI_SIZE];

	if (RTMP_GET_MESH_SOURCE(pPacket) == MESH_PROXY)
	{
		pMeshFlag->word = 0;
		*pMeshTTL = pAd->MeshTab.TTL;
		*pMeshSeq = INC_MESH_SEQ(pAd->MeshTab.MeshSeq);
		if (( (*pSrcBufVA & 0x01) // B/Mcast packet.
				|| MAC_ADDR_EQUAL(pSrcBufVA, pHeader_802_11->Addr3)) // or DA is a MP.
			&& MAC_ADDR_EQUAL(pSrcBufVA + MAC_ADDR_LEN, pAd->MeshTab.CurrentAddress))
		{	
			// the packet if come from current MP to another MP.
			// Since DA equal to MesH DA, and SA equal to current MP's MAC addr.
			pMeshFlag->field.AE = 0;
			*ppMeshAddr5 = NULL;
			*ppMeshAddr6 = NULL;
		}
		else
		{
			pMeshFlag->field.AE = 2;
			*ppMeshAddr5 = pSrcBufVA;
			*ppMeshAddr6 = pSrcBufVA + MAC_ADDR_LEN;
		}
	}
	else
	{	// must be MESH_FORWARD here.
		// shift pSrcBufVA pointer to Mesh header.
		pSrcBufVA += LENGTH_802_3;
		MeshHdrLen = GetMeshHederLen(pSrcBufVA);
		pMeshFlag->word = GetMeshFlag(pSrcBufVA);
		*pMeshTTL = GetMeshTTL(pSrcBufVA) - 1;
		*pMeshSeq = GetMeshSeq(pSrcBufVA);
		*ppMeshAddr5 = GetMeshAddr5(pSrcBufVA);
		*ppMeshAddr6 = GetMeshAddr6(pSrcBufVA);

		// skip Mesh header and LLC Header (8 Bytes).
		pTxBlk->pSrcBufData = pTxBlk->pSrcBufData + MeshHdrLen + 8;
		pTxBlk->SrcBufLen  -= (MeshHdrLen + 8);
	}

	return;
}

BOOLEAN MeshChCheck(
	IN RTMP_ADAPTER *pAd,
	IN PMESH_NEIGHBOR_ENTRY pNeighborEntry)
{
	BOOLEAN result = FALSE;

	if (pNeighborEntry->ChBW == BW_40)
		result = ((pNeighborEntry->Channel == pAd->CommonCfg.Channel)
					&& (pNeighborEntry->ChBW == pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth)
					&& (pNeighborEntry->ExtChOffset == pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset)) ? TRUE : FALSE;
	else
		result = (pNeighborEntry->Channel == pAd->CommonCfg.Channel) ? TRUE : FALSE;

	return result;
}

/*
	==========================================================================
	Description:
		Pre-build a BEACON frame in the shared memory
	==========================================================================
*/
VOID MeshMakeBeacon(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			idx)
{
	UCHAR DsLen = 1, SsidLen;
	UCHAR RSNIe=IE_WPA;
	HEADER_802_11 BcnHdr;
	LARGE_INTEGER FakeTimestamp;
	ULONG FrameLen = 0;
	PTXWI_STRUC pTxWI = &pAd->BeaconTxWI;
	PUCHAR pBeaconFrame = pAd->MeshTab.BeaconBuf;
	UCHAR *ptr;
	UINT i;
	UINT32 longValue;
			
	HTTRANSMIT_SETTING BeaconTransmit;   // MGMT frame PHY rate setting when operatin at Ht rate.

	// ignore SSID for MP. Refer to IEEE 802.11s-D1.06
	SsidLen = 0;

	DBGPRINT(RT_DEBUG_TRACE, ("MeshMakeBeacon - %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(pAd->MeshTab.CurrentAddress)));
	MgtMacHeaderInit(pAd, &BcnHdr, SUBTYPE_BEACON, 0, BROADCAST_ADDR, pAd->MeshTab.CurrentAddress);

	if (pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPANone)		
		RSNIe = IE_WPA;
	else if ((pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPA2) || 
			(pAd->MeshTab.AuthMode == Ndis802_11AuthModeWPA2PSK))
		RSNIe = IE_WPA2;
	
	// for update framelen to TxWI later.
	MakeOutgoingFrame(pBeaconFrame,                  &FrameLen,
					sizeof(HEADER_802_11),           &BcnHdr, 
					TIMESTAMP_LEN,                   &FakeTimestamp,
					2,                               &pAd->CommonCfg.BeaconPeriod,
					2,                               &pAd->MeshTab.CapabilityInfo,
					1,                               &SsidIe, 
					1,                               &SsidLen,
					1,                               &SupRateIe, 
					1,                               &pAd->CommonCfg.SupRateLen,
					pAd->CommonCfg.SupRateLen,       pAd->CommonCfg.SupRate, 
					1,                               &DsIe, 
					1,                               &DsLen, 
					1,                               &pAd->MeshTab.MeshChannel,
					END_OF_ARGS);

	if (pAd->CommonCfg.ExtRateLen)
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pBeaconFrame+FrameLen,         &TmpLen,
						1,                               &ExtRateIe, 
						1,                               &pAd->CommonCfg.ExtRateLen,
						pAd->CommonCfg.ExtRateLen,           pAd->CommonCfg.ExtRate, 
						END_OF_ARGS);
		FrameLen += TmpLen;
	}

	// Append RSN_IE when  WPA OR WPAPSK, 		
	if (pAd->MeshTab.AuthMode >= Ndis802_11AuthModeWPA)
	{
		ULONG TmpLen;
		MakeOutgoingFrame(pBeaconFrame+FrameLen,        &TmpLen,
						  1,                            &RSNIe,
						  1,                            &pAd->MeshTab.RSNIE_Len,
						  pAd->MeshTab.RSNIE_Len,      	pAd->MeshTab.RSN_IE,
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}
        
#if 1
	// AP Channel Report
	{
		UCHAR APChannelReportIe = IE_AP_CHANNEL_REPORT;
		ULONG	TmpLen;

		// 802.11n D2.0 Annex J
		// USA
		// regulatory class 32, channel set 1~7
		// regulatory class 33, channel set 5-11

		UCHAR rclass32[]={32, 1, 2, 3, 4, 5, 6, 7};
        UCHAR rclass33[]={33, 5, 6, 7, 8, 9, 10, 11};
		UCHAR rclasslen = 8; //sizeof(rclass32);

		if (pAd->CommonCfg.PhyMode == PHY_11BGN_MIXED)
		{
			MakeOutgoingFrame(pBeaconFrame+FrameLen,&TmpLen,
							  1,                    &APChannelReportIe,
							  1,                    &rclasslen,
							  rclasslen,            rclass32,
   							  1,                    &APChannelReportIe,
							  1,                    &rclasslen,
							  rclasslen,            rclass33,
							  END_OF_ARGS);
			FrameLen += TmpLen;		
		}	
	}
#endif
    
	BeaconTransmit.word = 0;
	RTMPWriteTxWI(pAd, pTxWI, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, 0, BSS0Mcast_WCID, 
		FrameLen, PID_MGMT, 0, 0,IFS_HTTXOP, FALSE, &BeaconTransmit);
	//
	// step 6. move BEACON TXD and frame content to on-chip memory
	//
	ptr = (PUCHAR)&pAd->BeaconTxWI;
#ifdef BIG_ENDIAN
    RTMPWIEndianChange(ptr, TYPE_TXWI);
#endif
	for (i=0; i<TXWI_SIZE; i+=4)  // 24-byte TXINFO field
	{
		longValue =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[idx] + i, *ptr);
		ptr +=4;
	}

	// update BEACON frame content. start right after the 24-byte TXINFO field
	ptr = pAd->MeshTab.BeaconBuf;
#ifdef BIG_ENDIAN
    RTMPFrameEndianChange(pAd, ptr, DIR_WRITE, FALSE);
#endif
	for (i= 0; i< FrameLen; i++)
	{
		RTMP_IO_WRITE8(pAd, pAd->BeaconOffset[idx] + TXWI_SIZE + i, *ptr); 
		ptr ++;
	}
	pAd->MeshTab.TimIELocationInBeacon = (UCHAR)FrameLen; 
	pAd->MeshTab.CapabilityInfoLocationInBeacon = sizeof(HEADER_802_11) + TIMESTAMP_LEN + 2;


}

/*
	==========================================================================
	Description:
		Update the BEACON frame in the shared memory. Because TIM IE is variable
		length. other IEs after TIM has to shift and total frame length may change
		for each BEACON period.
	Output:
		pAd->ApCfg.MBSSID[apidx].CapabilityInfo
		pAd->ApCfg.ErpIeContent
	==========================================================================
*/
VOID MeshUpdateBeaconFrame(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			idx) 
{
	//PTXWI_STRUC pTxWI = &pAd->BeaconTxWI;
	PUCHAR pBeaconFrame = pAd->MeshTab.BeaconBuf;
	UCHAR *ptr;
	ULONG FrameLen = pAd->MeshTab.TimIELocationInBeacon;
	ULONG UpdatePos = pAd->MeshTab.TimIELocationInBeacon;
	ULONG CapInfoPos = pAd->MeshTab.CapabilityInfoLocationInBeacon;
	//UCHAR RSNIe=IE_WPA, RSNIe2=IE_WPA2;
	//UCHAR ID_1B, TimFirst, TimLast, *pTim;	
	UINT i;
	HTTRANSMIT_SETTING BeaconTransmit;   // MGMT frame PHY rate setting when operatin at Ht rate.

	// The beacon of mesh isn't be initialized
	if (FrameLen == 0)
		return;

	//
	// step 1 - update BEACON's Capability
	//
	ptr = pBeaconFrame + pAd->MeshTab.CapabilityInfoLocationInBeacon;
	*ptr = (UCHAR)(pAd->MeshTab.CapabilityInfo & 0x00ff);
	*(ptr+1) = (UCHAR)((pAd->MeshTab.CapabilityInfo & 0xff00) >> 8);

	//
	// step 5. Update HT. Since some fields might change in the same BSS.
	//
	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		ULONG TmpLen;
		UCHAR HtLen, HtLen1;
		//UCHAR i;

#ifdef BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
		ADD_HT_INFO_IE	addHTInfoTmp;
		USHORT	b2lTmp, b2lTmp2;
#endif

		// add HT Capability IE 
		HtLen = sizeof(pAd->CommonCfg.HtCapability);
		HtLen1 = sizeof(pAd->CommonCfg.AddHTInfo);
#ifndef BIG_ENDIAN
		MakeOutgoingFrame(pBeaconFrame+FrameLen,         &TmpLen,
								  1,                                &HtCapIe,
								  1,                                &HtLen,
								 HtLen,          &pAd->CommonCfg.HtCapability, 
								  1,                                &AddHtInfoIe,
								  1,                                &HtLen1,
								 HtLen1,          &pAd->CommonCfg.AddHTInfo, 
						  END_OF_ARGS);
#else
		NdisMoveMemory(&HtCapabilityTmp, &pAd->CommonCfg.HtCapability, HtLen);
		*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
		*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));

		NdisMoveMemory(&addHTInfoTmp, &pAd->CommonCfg.AddHTInfo, HtLen1);
		*(USHORT *)(&addHTInfoTmp.AddHtInfo2) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo2));
		*(USHORT *)(&addHTInfoTmp.AddHtInfo3) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo3));

		MakeOutgoingFrame(pBeaconFrame+FrameLen,         &TmpLen,
								  1,                                &HtCapIe,
								  1,                                &HtLen,
								 HtLen,                   &HtCapabilityTmp, 
								  1,                                &AddHtInfoIe,
								  1,                                &HtLen1,
								 HtLen1,                   &addHTInfoTmp, 
						  END_OF_ARGS);
#endif
		FrameLen += TmpLen;

	}

#if 0	// remove it temporarily
	// add WMM IE here
	if (pAd->ApCfg.MBSSID[apidx].bWmmCapable)
	{
		ULONG TmpLen;
		UCHAR i;
		UCHAR WmeParmIe[26] = {IE_VENDOR_SPECIFIC, 24, 0x00, 0x50, 0xf2, 0x02, 0x01, 0x01, 0, 0}; 
		WmeParmIe[8] = pAd->ApCfg.BssEdcaParm.EdcaUpdateCount & 0x0f;
#ifdef UAPSD_AP_SUPPORT
        UAPSD_MR_IE_FILL(WmeParmIe[8], pAd);
#endif // UAPSD_AP_SUPPORT //
		for (i=QID_AC_BE; i<=QID_AC_VO; i++)
		{
			WmeParmIe[10+ (i*4)] = (i << 5)                                         +     // b5-6 is ACI
								   ((UCHAR)pAd->ApCfg.BssEdcaParm.bACM[i] << 4)     +     // b4 is ACM
								   (pAd->ApCfg.BssEdcaParm.Aifsn[i] & 0x0f);              // b0-3 is AIFSN
			WmeParmIe[11+ (i*4)] = (pAd->ApCfg.BssEdcaParm.Cwmax[i] << 4)           +     // b5-8 is CWMAX
								   (pAd->ApCfg.BssEdcaParm.Cwmin[i] & 0x0f);              // b0-3 is CWMIN
			WmeParmIe[12+ (i*4)] = (UCHAR)(pAd->ApCfg.BssEdcaParm.Txop[i] & 0xff);        // low byte of TXOP
			WmeParmIe[13+ (i*4)] = (UCHAR)(pAd->ApCfg.BssEdcaParm.Txop[i] >> 8);          // high byte of TXOP
		}

		MakeOutgoingFrame(pBeaconFrame+FrameLen,         &TmpLen,
						  26,                            WmeParmIe,
						  END_OF_ARGS);
		FrameLen += TmpLen;
	}
#endif

	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		ULONG TmpLen;
		UCHAR HtLen, HtLen1;
		//UCHAR i;
#ifdef BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
		ADD_HT_INFO_IE	addHTInfoTmp;
		USHORT	b2lTmp, b2lTmp2;
#endif
		// add HT Capability IE 
		HtLen = sizeof(pAd->CommonCfg.HtCapability);
		HtLen1 = sizeof(pAd->CommonCfg.AddHTInfo);

		if (pAd->bBroadComHT == TRUE)
		{
			UCHAR epigram_ie_len;
			UCHAR BROADCOM_HTC[4] = {0x0, 0x90, 0x4c, 0x33};
			UCHAR BROADCOM_AHTINFO[4] = {0x0, 0x90, 0x4c, 0x34};


			epigram_ie_len = HtLen + 4;
#ifndef BIG_ENDIAN
			MakeOutgoingFrame(pBeaconFrame + FrameLen,      &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_HTC[0],
						  HtLen,          					&pAd->CommonCfg.HtCapability, 
						  END_OF_ARGS);
#else
			NdisMoveMemory(&HtCapabilityTmp, &pAd->CommonCfg.HtCapability, HtLen);
			*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
			*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));

			MakeOutgoingFrame(pBeaconFrame + FrameLen,       &TmpLen,
						1,                               &WpaIe,
						1,                               &epigram_ie_len,
						4,                               &BROADCOM_HTC[0], 
						HtLen,                           &HtCapabilityTmp,
						END_OF_ARGS);
#endif

			FrameLen += TmpLen;

			epigram_ie_len = HtLen1 + 4;
#ifndef BIG_ENDIAN
			MakeOutgoingFrame(pBeaconFrame + FrameLen,        &TmpLen,
						  1,                                &WpaIe,
						  1,                                &epigram_ie_len,
						  4,                                &BROADCOM_AHTINFO[0],
						  HtLen1, 							&pAd->CommonCfg.AddHTInfo, 
						  END_OF_ARGS);
#else
			NdisMoveMemory(&addHTInfoTmp, &pAd->CommonCfg.AddHTInfo, HtLen1);
			*(USHORT *)(&addHTInfoTmp.AddHtInfo2) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo2));
			*(USHORT *)(&addHTInfoTmp.AddHtInfo3) = SWAP16(*(USHORT *)(&addHTInfoTmp.AddHtInfo3));

			MakeOutgoingFrame(pBeaconFrame + FrameLen,         &TmpLen,
							1,                             &WpaIe,
							1,                             &epigram_ie_len,
							4,                             &BROADCOM_AHTINFO[0],
							HtLen1,                        &addHTInfoTmp,
							END_OF_ARGS);
#endif
			FrameLen += TmpLen;
		}

	}

	// P802.11n_D1.10 
	// 7.3.2.27 Extended Capabilities IE
	// HT Information Exchange Support
	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		ULONG TmpLen;
		UCHAR ExtCapIe[3] = {IE_EXT_CAPABILITY, 1, 0x01};
		MakeOutgoingFrame(pBeaconFrame+FrameLen, &TmpLen,
							3,                   ExtCapIe,
							END_OF_ARGS);
		FrameLen += TmpLen;

	}


   	// add Ralink-specific IE here - Byte0.b0=1 for aggregation, Byte0.b1=1 for piggy-back
	{
		ULONG TmpLen;
		UCHAR RalinkSpecificIe[9] = {IE_VENDOR_SPECIFIC, 7, 0x00, 0x0c, 0x43, 0x00, 0x00, 0x00, 0x00};

		if (pAd->CommonCfg.bAggregationCapable)
			RalinkSpecificIe[5] |= 0x1;
		if (pAd->CommonCfg.bPiggyBackCapable)
			RalinkSpecificIe[5] |= 0x2;
		if (pAd->CommonCfg.bRdg)
			RalinkSpecificIe[5] |= 0x4;

		MakeOutgoingFrame(pBeaconFrame+FrameLen, &TmpLen,
							9,                   RalinkSpecificIe,
							END_OF_ARGS);
		FrameLen += TmpLen;
	}
	
	// Insert MeshIDIE and MeshConfigurationIE in Beacon frame
	//if (MeshValid(&pAd->MeshTab))
	{
		pAd->MeshTab.MeshCapability.field.AcceptPeerLinks = MeshAcceptPeerLink(pAd);
		InsertMeshIdIE(pAd, pBeaconFrame+FrameLen, &FrameLen);
		InsertMeshConfigurationIE(pAd, pBeaconFrame+FrameLen, &FrameLen, FALSE);
	}
	
	// Insert MSCIE
	InsertMSCIE(pAd, pBeaconFrame+FrameLen, &FrameLen);
	
	//
	// step 6. Since FrameLen may change, update TXWI.
	//
	// Update in real buffer
	// Update sw copy.	
	if (pAd->CommonCfg.Channel <= 14)
		BeaconTransmit.word = 0;
	else
		BeaconTransmit.word = 0x4000;
	
	RTMPWriteTxWI(pAd, &pAd->BeaconTxWI, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, 0, 0xff, 
		FrameLen, PID_MGMT, QID_MGMT, 0, IFS_HTTXOP, FALSE, &BeaconTransmit);
	//
	// step 6. move BEACON TXWI and frame content to on-chip memory
	//	
	if (((pAd->MeshTab.dev == NULL) 
			|| !(pAd->MeshTab.dev->flags & IFF_UP))
		)
	{
		/* when the ra interface is down, do not send its beacon frame */
		/* clear all zero */
		for(i=0; i<TXWI_SIZE; i+=4)
			RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[idx] + i, 0);
	}
	else	
	{
		ptr = (PUCHAR)&pAd->BeaconTxWI;
#ifdef BIG_ENDIAN
		RTMPWIEndianChange(ptr, TYPE_TXWI);
#endif
		for (i=0; i<TXWI_SIZE; i++)  // 24-byte TXINFO field
		{
			RTMP_IO_WRITE8(pAd, pAd->BeaconOffset[idx] + i, *ptr);
			ptr ++;
		}

		// Update CapabilityInfo in Beacon
		ptr = &pAd->MeshTab.BeaconBuf[CapInfoPos];
		for (i = CapInfoPos; i < (CapInfoPos+2); i++)
		{
			RTMP_IO_WRITE8(pAd, pAd->BeaconOffset[idx] + TXWI_SIZE + i, *ptr); 
			ptr ++;
		}
	
		ptr = &pAd->MeshTab.BeaconBuf[UpdatePos];
		if (FrameLen > UpdatePos)
		{
			for (i= UpdatePos; i< (FrameLen); i++)
			{
				RTMP_IO_WRITE8(pAd, pAd->BeaconOffset[idx] + TXWI_SIZE + i, *ptr); 
				ptr ++;
			}
		}
	} /* End of if */
}

VOID MeshCleanBeaconFrame(
	IN PRTMP_ADAPTER	pAd,
	IN UCHAR			idx) 
{
	INT i;

	/* when the ra interface is down, do not send its beacon frame */
	/* clear all zero */
	for(i=0; i<TXWI_SIZE; i+=4)
		RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[idx] + i, 0);
}

/*
	==========================================================================
	Description:
	Note: 
		BEACON frame in shared memory should be built ok before this routine
		can be called. Otherwise, a garbage frame maybe transmitted out every
		Beacon period.
 
	==========================================================================
 */
VOID AsicEnableMESHSync(
	IN PRTMP_ADAPTER pAd)
{
	BCN_TIME_CFG_STRUC csr;

	RTMP_IO_READ32(pAd, BCN_TIME_CFG, &csr.word);

	//
	// For Wi-Fi faily generated beacons between participating stations. 
	// Set TBTT phase adaptive adjustment step to 8us (default 16us)
	// don't change settings 2006-5- by Jerry
	// RTMP_IO_WRITE32(pAd, TBTT_SYNC_CFG, 0x00001010);
	
	// start sending BEACON
	csr.field.BeaconInterval = pAd->CommonCfg.BeaconPeriod << 4; // ASIC register in units of 1/16 TU
	csr.field.bTsfTicking = 1;
	csr.field.TsfSyncMode = 3; // sync TSF in IBSS mode
	csr.field.bTBTTEnable = 1;
	csr.field.bBeaconGen = 1;
	RTMP_IO_WRITE32(pAd, BCN_TIME_CFG, csr.word);
}


