

#include "rt_config.h"
#include "ap_autoChSel.h"


//static ChannelInfo_s ChannelInfo;
//static BSSINFO BssInfoTab;

static UCHAR ZeroSsid[32] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

#if 0	// defined but not used
static ULONG AutoChSelectBssSearch(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pBssid,
	IN UCHAR Channel);
#endif

static ULONG AutoChBssSearchWithSSID(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR Bssid,
	IN PUCHAR pSsid,
	IN UCHAR SsidLen,
	IN UCHAR Channel);

static VOID AutoChBssEntrySet(
	OUT BSSENTRY *pBss, 
	IN PUCHAR pBssid, 
	IN CHAR Ssid[], 
	IN UCHAR SsidLen, 
	IN UCHAR Channel,
	IN CHAR Rssi);

static void UpdateChannelInfo(
	IN PRTMP_ADAPTER pAd,
	IN int ch,
	IN UINT32 FalseCCA);

static int SelectClearChannel(
	IN PRTMP_ADAPTER pAd);


/*
** Functions.
*/
#if 0 // defined but not used
static ULONG AutoChSelectBssSearch(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pBssid,
	IN UCHAR Channel)
{
	UCHAR i;
	PBSSINFO pBssInfoTab = pAd->pBssInfoTab;

	if(pBssInfoTab == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("pAd->pBssInfoTab equal NULL.\n"));
		return (ULONG)BSS_NOT_FOUND;
	}

	for (i = 0; i < pAd->pBssInfoTab->BssNr; i++) 
	{
		//
		// Some AP that support A/B/G mode that may used the same BSSID on 11A and 11B/G.
		// We should distinguish this case.
		//
		if ((((pBssInfoTab->BssEntry[i].Channel <= 14) && (Channel <= 14)) ||
			 ((pBssInfoTab->BssEntry[i].Channel > 14) && (Channel > 14))) &&
			MAC_ADDR_EQUAL(pBssInfoTab->BssEntry[i].Bssid, pBssid))
		{
			return i;
		}
	}
	return (ULONG)BSS_NOT_FOUND;
}
#endif

static ULONG AutoChBssSearchWithSSID(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR Bssid,
	IN PUCHAR pSsid,
	IN UCHAR SsidLen,
	IN UCHAR Channel)
{
	UCHAR i;
	PBSSINFO pBssInfoTab = pAd->pBssInfoTab;

	if(pBssInfoTab == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("pAd->pBssInfoTab equal NULL.\n"));
		return (ULONG)BSS_NOT_FOUND;
	}

	for (i = 0; i < pBssInfoTab->BssNr; i++) 
	{
		if ((((pBssInfoTab->BssEntry[i].Channel <= 14) && (Channel <= 14)) ||
			((pBssInfoTab->BssEntry[i].Channel > 14) && (Channel > 14))) &&
			MAC_ADDR_EQUAL(&(pBssInfoTab->BssEntry[i].Bssid), Bssid) &&
			(SSID_EQUAL(pSsid, SsidLen, pBssInfoTab->BssEntry[i].Ssid, pBssInfoTab->BssEntry[i].SsidLen) ||
			(NdisEqualMemory(pSsid, ZeroSsid, SsidLen)) || 
			(NdisEqualMemory(pBssInfoTab->BssEntry[i].Ssid, ZeroSsid, pBssInfoTab->BssEntry[i].SsidLen))))
		{ 
			return i;
		}
	}
	return (ULONG)BSS_NOT_FOUND;
}

static VOID AutoChBssEntrySet(
	OUT BSSENTRY *pBss, 
	IN PUCHAR pBssid, 
	IN CHAR Ssid[], 
	IN UCHAR SsidLen, 
	IN UCHAR Channel,
	IN CHAR Rssi)
{
	COPY_MAC_ADDR(pBss->Bssid, pBssid);
	if (SsidLen > 0)
	{
		// For hidden SSID AP, it might send beacon with SSID len equal to 0
		// Or send beacon /probe response with SSID len matching real SSID length,
		// but SSID is all zero. such as "00-00-00-00" with length 4.
		// We have to prevent this case overwrite correct table
		if (NdisEqualMemory(Ssid, ZeroSsid, SsidLen) == 0)
		{
			NdisMoveMemory(pBss->Ssid, Ssid, SsidLen);
			pBss->SsidLen = SsidLen;
		}
	}

	pBss->Channel = Channel;
	pBss->CentralChannel = Channel;
	pBss->Rssi = Rssi;

	return;
}

static void AutoChBssTableReset(
	IN PRTMP_ADAPTER pAd)
{
	if (pAd->pBssInfoTab)
		NdisZeroMemory(pAd->pBssInfoTab, sizeof(BSSINFO));
	else
		DBGPRINT(RT_DEBUG_ERROR, ("pAd->pBssInfoTab equal NULL.\n"));

	return;
}

static void ChannelInfoReset(
	IN PRTMP_ADAPTER pAd)
{
	if (pAd->pChannelInfo)
		NdisZeroMemory(pAd->pChannelInfo, sizeof(CHANNELINFO));
	else
		DBGPRINT(RT_DEBUG_ERROR, ("pAd->pChannelInfo equal NULL.\n"));

	return;
}

static void UpdateChannelInfo(
	IN PRTMP_ADAPTER pAd,
	IN int ch,
	IN UINT32 FalseCCA)
{
	if(pAd->pChannelInfo != NULL)
		pAd->pChannelInfo->FalseCCA[ch] = FalseCCA;
	else
		DBGPRINT(RT_DEBUG_ERROR, ("pAd->pChannelInfo equal NULL.\n"));

	return;
}

static int SelectClearChannel(
	IN PRTMP_ADAPTER pAd)
{
	#define CCA_THRESHOLD (100)

	PBSSINFO pBssInfoTab = pAd->pBssInfoTab;
	PCHANNELINFO pChannelInfo = pAd->pChannelInfo;
	int i;
	int ch, channel_no;
	BSSENTRY *pBss;
	UINT32 min_dirty, min_falsecca;
	int candidate_ch;

	if(pBssInfoTab == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("pAd->pBssInfoTab equal NULL.\n"));
		return (FirstChannel(pAd));
	}

	if(pChannelInfo == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("pAd->pChannelInfo equal NULL.\n"));
		return (FirstChannel(pAd));
	}

	for (i = 0; i < pBssInfoTab->BssNr; i++) {
		pBss = &(pBssInfoTab->BssEntry[i]);
		channel_no = pBss->Channel;
		if (pChannelInfo->max_rssi[channel_no] < pBss->Rssi) {
			pChannelInfo->max_rssi[channel_no] = pBss->Rssi;
		}

		if (pBss->Rssi >= RSSI_TO_DBM_OFFSET-50) {
			/* high signal >= -50 dbm */
			pChannelInfo->dirtyness[channel_no] += 50;
		} else if (pBss->Rssi <= RSSI_TO_DBM_OFFSET-80) {
			/* low signal <= -80 dbm */
			pChannelInfo->dirtyness[channel_no] += 30;
		} else {
			/* mid signal -50 ~ -80 dbm */
			pChannelInfo->dirtyness[channel_no] += 40;
		}

		if (channel_no > 1) pChannelInfo->dirtyness[channel_no-1] += 16;
		if (channel_no > 2) pChannelInfo->dirtyness[channel_no-2] += 12;
		if (channel_no > 3) pChannelInfo->dirtyness[channel_no-3] += 8;
		if (channel_no > 4) pChannelInfo->dirtyness[channel_no-4] += 4;
		if (channel_no < 14) pChannelInfo->dirtyness[channel_no+1] += 16;
		if (channel_no < 13) pChannelInfo->dirtyness[channel_no+2] += 12;
		if (channel_no < 12) pChannelInfo->dirtyness[channel_no+3] += 8;
		if (channel_no < 11) pChannelInfo->dirtyness[channel_no+4] += 4;

		pChannelInfo->total_rssi[channel_no] += pBss->Rssi;

		DBGPRINT(RT_DEBUG_TRACE,(" ch%d bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
			channel_no, pBss->Bssid[0], pBss->Bssid[1], pBss->Bssid[2], pBss->Bssid[3], pBss->Bssid[4], pBss->Bssid[5]));
	}
			
	DBGPRINT(RT_DEBUG_TRACE, ("=====================================================\n"));
	for (i = pAd->ChannelListNum; i >= 1; i--) {
		DBGPRINT(RT_DEBUG_TRACE, ("Channel %d : total RSSI = %ld, max RSSI = %ld, Dirty = %ld, False CCA = %u\n",
					i, pChannelInfo->total_rssi[i],
					pChannelInfo->max_rssi[i], pChannelInfo->dirtyness[i],
					pChannelInfo->FalseCCA[i]));
	}
	DBGPRINT(RT_DEBUG_TRACE, ("=====================================================\n"));

	min_dirty = min_falsecca = 0xFFFFFFFF;
	candidate_ch = 0;

	/* 
	 * Rule 1. Pick up a good channel that False_CCA =< CCA_THRESHOLD 
	 *		   by dirtyness and total_rssi
	 */
	for (ch = FirstChannel(pAd); ch != 0; ch = NextChannel(pAd, ch)) {
		if (pChannelInfo->FalseCCA[ch]<=CCA_THRESHOLD) {
			if (min_dirty > pChannelInfo->dirtyness[ch] + pChannelInfo->total_rssi[ch]) {
				min_dirty = pChannelInfo->dirtyness[ch] + pChannelInfo->total_rssi[ch];
				candidate_ch = ch;
			}				
		}
	}

	if (candidate_ch) {
		DBGPRINT(RT_DEBUG_TRACE, ("Rule 1 ==> Select Channel %d\n", candidate_ch));
		return candidate_ch;
	}

	/*
	 * Rule 2. Pick up a good channel that False_CCA > CCA_THRESHOLD 
	 *		   by FalseCCA, dirtyness and total_rssi
	 */
	for (ch = FirstChannel(pAd); ch != 0; ch = NextChannel(pAd, ch)) {
		if (pChannelInfo->FalseCCA[ch] > CCA_THRESHOLD) {
			if (min_falsecca > pChannelInfo->FalseCCA[ch] + pChannelInfo->dirtyness[ch] + pChannelInfo->total_rssi[ch]) {
				min_falsecca = pChannelInfo->FalseCCA[ch] + pChannelInfo->dirtyness[ch] + pChannelInfo->total_rssi[ch];
				candidate_ch = ch;
			}
		}
	}

	if (candidate_ch) {
		DBGPRINT(RT_DEBUG_TRACE, ("Rule 2 ==> Select Channel %d\n", candidate_ch));
		return	candidate_ch;
	}
	
	return (FirstChannel(pAd));
}

ULONG AutoChBssInsertEntry(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR pBssid,
	IN CHAR Ssid[],
	IN UCHAR SsidLen, 
	IN UCHAR ChannelNo,
	IN CHAR Rssi)
{
	ULONG	Idx;
	PBSSINFO pBssInfoTab = pAd->pBssInfoTab;

	if(pBssInfoTab == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("pAd->pBssInfoTab equal NULL.\n"));
		return BSS_NOT_FOUND;
	}

	Idx = AutoChBssSearchWithSSID(pAd, pBssid, Ssid, SsidLen, ChannelNo);
	if (Idx == BSS_NOT_FOUND) 
	{
		if (pBssInfoTab->BssNr >= MAX_LEN_OF_BSS_TABLE)
			return BSS_NOT_FOUND;
		Idx = pBssInfoTab->BssNr;
		AutoChBssEntrySet(&pBssInfoTab->BssEntry[Idx], pBssid, Ssid, SsidLen, ChannelNo, Rssi);
		pBssInfoTab->BssNr++;
	} 
	else
	{
		AutoChBssEntrySet(&pBssInfoTab->BssEntry[Idx], pBssid, Ssid, SsidLen, ChannelNo, Rssi);
	}

	return Idx;
}


void AutoChBssTableInit(
	IN PRTMP_ADAPTER pAd)
{
	pAd->pBssInfoTab = kmalloc(sizeof(BSSINFO), GFP_ATOMIC);
	if (pAd->pBssInfoTab)
		NdisZeroMemory(pAd->pBssInfoTab, sizeof(BSSINFO));
	else
		DBGPRINT(RT_DEBUG_ERROR, ("%s Fail to alloc memory for pAd->pBssInfoTab", __FUNCTION__));

	return;
}

void ChannelInfoInit(
	IN PRTMP_ADAPTER pAd)
{
	pAd->pChannelInfo = kmalloc(sizeof(CHANNELINFO), GFP_ATOMIC);
	if (pAd->pChannelInfo)
		NdisZeroMemory(pAd->pChannelInfo, sizeof(CHANNELINFO));
	else
		DBGPRINT(RT_DEBUG_ERROR, ("%s Fail to alloc memory for pAd->pChannelInfo", __FUNCTION__));


	return;
}

void AutoChBssTableDestroy(
	IN PRTMP_ADAPTER pAd)
{
	if (pAd->pBssInfoTab)
		kfree(pAd->pBssInfoTab);
	pAd->pBssInfoTab = NULL;

	return;
}

void ChannelInfoDestroy(
	IN PRTMP_ADAPTER pAd)
{
	if (pAd->pChannelInfo)
		kfree(pAd->pChannelInfo);
	pAd->pChannelInfo = NULL;

	return;
}

UCHAR New_ApAutoSelectChannel(
	IN PRTMP_ADAPTER pAd)
{
	INT i;
	UCHAR ch;
	UINT32 FalseCca = 0;
	BOOLEAN bFindIt = FALSE;

	if (pAd->CommonCfg.Channel > 14)
	{
		if (pAd->CommonCfg.bIEEE80211H)
		{
			while(TRUE)
			{
				ch = pAd->ChannelList[RandomByte(pAd)%pAd->ChannelListNum].Channel;

				if (ch == 0)
					ch = FirstChannel(pAd);
				if (!RadarChannelCheck(pAd, ch))
					continue;

				for (i=0; i<pAd->ChannelListNum; i++)
				{
					if (pAd->ChannelList[i].Channel == ch)
					{
						if (pAd->ChannelList[i].RemainingTimeForUse == 0)
							bFindIt = TRUE;
						
						break;
					}
				}
				
				if (bFindIt == TRUE)
					break;
			};
		}
		else
		{
			ch = pAd->ChannelList[RandomByte(pAd)%pAd->ChannelListNum].Channel;
			if (ch == 0)
				ch = FirstChannel(pAd);
		}
		DBGPRINT(RT_DEBUG_TRACE,("1.APAutoSelectChannel pick up ch#%d\n",ch));
	}
	else
	{
		/* reset bss table */
		AutoChBssTableReset(pAd);

		/* clear Channel Info */
		ChannelInfoReset(pAd);

		for (ch = pAd->ChannelListNum; ch >= 1; ch--)
		{
			//AsicSwitchChannel(pAd, ch);
			APSwitchChannel(pAd, ch);
			pAd->ApCfg.AutoChannel_Channel = ch;

			RTMPusecDelay(500000); // 0.5 sec at each channel

			RTMP_IO_READ32(pAd, RX_STA_CNT1, &FalseCca);
			FalseCca &= 0x0000ffff;
			UpdateChannelInfo(pAd, ch, FalseCca);        
		}
		ch = SelectClearChannel(pAd);

		DBGPRINT(RT_DEBUG_TRACE, ("ApAutoSelectChannel pick up ch#%d\n",ch));
	}

	return ch;
}

