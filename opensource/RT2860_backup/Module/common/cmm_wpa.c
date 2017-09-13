/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	wpa.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Jan	Lee		03-07-22		Initial
	Paul Lin	03-11-28		Modify for supplicant
*/
#include "rt_config.h"

UCHAR		OUI_WPA_NONE_AKM[4]		= {0x00, 0x50, 0xF2, 0x00};
UCHAR       OUI_WPA_WEP40[4]        = {0x00, 0x50, 0xF2, 0x01};
UCHAR       OUI_WPA_TKIP[4]     = {0x00, 0x50, 0xF2, 0x02};
UCHAR       OUI_WPA_CCMP[4]     = {0x00, 0x50, 0xF2, 0x04};
UCHAR       OUI_WPA2_WEP40[4]   = {0x00, 0x0F, 0xAC, 0x01};
UCHAR       OUI_WPA2_TKIP[4]        = {0x00, 0x0F, 0xAC, 0x02};
UCHAR       OUI_WPA2_CCMP[4]        = {0x00, 0x0F, 0xAC, 0x04};

/*
	========================================================================

	Routine Description:
		PRF function 

	Arguments:

	Return Value:

	Note:
		802.1i	Annex F.9

	========================================================================
*/
VOID	PRF(
	IN	UCHAR	*key,
	IN	INT		key_len,
	IN	UCHAR	*prefix,
	IN	INT		prefix_len,
	IN	UCHAR	*data,
	IN	INT		data_len,
	OUT	UCHAR	*output,
	IN	INT		len)
{
	INT		i;
    UCHAR   *input;
	INT		currentindex = 0;
	INT		total_len;

	os_alloc_mem(NULL, (PUCHAR *)&input, 1024);
	
    if (input == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("!!!PRF: no memory!!!\n"));
        return;
    } /* End of if */
	
	NdisMoveMemory(input, prefix, prefix_len);
	input[prefix_len] =	0;
	NdisMoveMemory(&input[prefix_len + 1], data, data_len);
	total_len =	prefix_len + 1 + data_len;
	input[total_len] = 0;
	total_len++;
	for	(i = 0;	i <	(len + 19) / 20; i++)
	{
		HMAC_SHA1(input, total_len,	key, key_len, &output[currentindex]);
		currentindex +=	20;
		input[total_len - 1]++;
	}	
    os_free_mem(NULL, input);
}

/*
	========================================================================
	
	Routine Description:
		Count TPTK from PMK

	Arguments:
		
	Return Value:
		Output		Store the TPTK

	Note:
		
	========================================================================
*/
VOID WpaCountPTK(
	IN	PRTMP_ADAPTER	pAd, 
	IN	UCHAR	*PMK,
	IN	UCHAR	*ANonce,
	IN	UCHAR	*AA,
	IN	UCHAR	*SNonce,
	IN	UCHAR	*SA,
	OUT	UCHAR	*output,
	IN	UINT	len)
{	
	UCHAR	concatenation[76];
	UINT	CurrPos = 0;
	UCHAR	temp[32];
	UCHAR	Prefix[] = {'P', 'a', 'i', 'r', 'w', 'i', 's', 'e', ' ', 'k', 'e', 'y', ' ', 
						'e', 'x', 'p', 'a', 'n', 's', 'i', 'o', 'n'};

#ifdef LEAP_SUPPORT
	UCHAR	CCKMPrefix[] = {'F', 'a', 's', 't', '-', 'R', 'o', 'a', 'm', ' ', 'G', 'e', 'n', 
						'e', 'r', 'a', 't', 'e', ' ', 'B', 'a', 's', 'e', ' ', 'K', 'e', 'y'};
#endif //LEAP_SUPPORT //

	NdisZeroMemory(temp, sizeof(temp));
	NdisZeroMemory(concatenation, 76);

#ifdef LEAP_SUPPORT 
	if (LEAP_CCKM_ON(pAd))
	{
		NdisMoveMemory(concatenation, AA, 6);
		CurrPos += 6;

		NdisMoveMemory(&concatenation[CurrPos],	SA, 6);
		CurrPos += 6;

		NdisMoveMemory(&concatenation[CurrPos],	SNonce, 32);
		CurrPos += 32;

		NdisMoveMemory(&concatenation[CurrPos],	ANonce, 32);
		CurrPos += 32;
		
		PRF(PMK, 16, CCKMPrefix, 27, concatenation, 76, output, len);
	}
	else
#endif // LEAP_SUPPORT // 		
	{
		// Get smaller address
		if (RTMPCompareMemory(SA, AA, 6) == 1)
			NdisMoveMemory(concatenation, AA, 6);
		else
			NdisMoveMemory(concatenation, SA, 6);
		CurrPos += 6;

		// Get larger address
		if (RTMPCompareMemory(SA, AA, 6) == 1)
			NdisMoveMemory(&concatenation[CurrPos], SA, 6);
		else
			NdisMoveMemory(&concatenation[CurrPos], AA, 6);
		
		// store the larger mac address for backward compatible of 
		// ralink proprietary STA-key issue		
		NdisMoveMemory(temp, &concatenation[CurrPos], MAC_ADDR_LEN);		
		CurrPos += 6;

		// Get smaller Nonce
		if (RTMPCompareMemory(ANonce, SNonce, 32) == 0)
			NdisMoveMemory(&concatenation[CurrPos], temp, 32);	// patch for ralink proprietary STA-key issue
		else if (RTMPCompareMemory(ANonce, SNonce, 32) == 1)
			NdisMoveMemory(&concatenation[CurrPos], SNonce, 32);
		else
			NdisMoveMemory(&concatenation[CurrPos], ANonce, 32);
		CurrPos += 32;

		// Get larger Nonce
		if (RTMPCompareMemory(ANonce, SNonce, 32) == 0)
			NdisMoveMemory(&concatenation[CurrPos], temp, 32);	// patch for ralink proprietary STA-key issue
		else if (RTMPCompareMemory(ANonce, SNonce, 32) == 1)
			NdisMoveMemory(&concatenation[CurrPos], ANonce, 32);
		else
			NdisMoveMemory(&concatenation[CurrPos], SNonce, 32);
		CurrPos += 32;

		hex_dump("concatenation=", concatenation, 76);

#ifdef LEAP_SUPPORT 
		if ((pAd->StaCfg.LeapAuthMode == CISCO_AuthModeLEAP) && (pAd->StaCfg.WepStatus == Ndis802_11Encryption2Enabled))
			PRF(PMK, 16, Prefix, 22, concatenation, 76, output, len);
		else
#endif // LEAP_SUPPORT //			
			PRF(PMK, LEN_MASTER_KEY, Prefix, 22, concatenation, 76, output, len);
	}
}

/*
	========================================================================
	
	Routine Description:
		Misc function to Generate random number

	Arguments:
		
	Return Value:

	Note:
		802.1i  Annex F.9
		
	========================================================================
*/
VOID	GenRandom(
	IN	PRTMP_ADAPTER	pAd, 
	OUT	UCHAR			*random,
	IN	UCHAR			apidx)
{	
	INT		i, curr;
	UCHAR	local[80], KeyCounter[32];
	UCHAR	result[80];
	ULONG	CurrentTime;
	UCHAR	prefix[] = {'I', 'n', 'i', 't', ' ', 'C', 'o', 'u', 'n', 't', 'e', 'r'};

	NdisZeroMemory(result, 80);
	NdisZeroMemory(local, 80);
	NdisZeroMemory(KeyCounter, 32);	

#ifdef CONFIG_AP_SUPPORT 
#ifdef APCLI_SUPPORT
	if (apidx >= MIN_NET_DEVICE_FOR_APCLI)
	{
		COPY_MAC_ADDR(local, pAd->CurrentAddress);
	}
	else
#endif // APCLI_SUPPORT //
	{
		NdisMoveMemory(KeyCounter, pAd->ApCfg.Key_Counter, 32);
		COPY_MAC_ADDR(local, pAd->ApCfg.MBSSID[apidx].Bssid);
	}
#endif // CONFIG_AP_SUPPORT //
	
	
	for	(i = 0;	i <	32;	i++)
	{		
		curr =	MAC_ADDR_LEN;
		NdisGetSystemUpTime(&CurrentTime);

#ifdef CONFIG_AP_SUPPORT 
#ifdef APCLI_SUPPORT
		if (apidx >= MIN_NET_DEVICE_FOR_APCLI)
		{
			COPY_MAC_ADDR(local, pAd->CurrentAddress);
		}
		else
#endif // APCLI_SUPPORT //
		{
			COPY_MAC_ADDR(local, pAd->ApCfg.MBSSID[apidx].Bssid);
		}	
#endif // CONFIG_AP_SUPPORT //


		curr +=	MAC_ADDR_LEN;
		NdisMoveMemory(&local[curr],  &CurrentTime,	sizeof(CurrentTime));
		curr +=	sizeof(CurrentTime);
		NdisMoveMemory(&local[curr],  result, 32);
		curr +=	32;
		NdisMoveMemory(&local[curr],  &i,  2);		
		curr +=	2;
		PRF(KeyCounter, 32, prefix,12, local, curr, result, 32); 
	}

#ifdef CONFIG_AP_SUPPORT
    for (i = 32; i > 0; i--)
    {   
#ifdef APCLI_SUPPORT
		if (apidx >= MIN_NET_DEVICE_FOR_APCLI)
			break;
#endif // APCLI_SUPPORT //
	
        if (pAd->ApCfg.Key_Counter[i-1] == 0xff)
        {
            pAd->ApCfg.Key_Counter[i-1] = 0;
        }
        else
        {
            pAd->ApCfg.Key_Counter[i-1]++;
            break;
        }
    }
#endif // CONFIG_AP_SUPPORT //
	
	NdisMoveMemory(random, result,	32);	
}


VOID RTMPMakeRSNIE(
    IN  PRTMP_ADAPTER   pAd,
    IN  UINT            AuthMode,
    IN  UINT            WepStatus,
	IN	UCHAR			apidx)
{
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT	
	UINT		apcliIfidx = 0;
	BOOLEAN		bApClientRSNIE = FALSE;

	if (apidx >= MIN_NET_DEVICE_FOR_APCLI)
	{
		bApClientRSNIE = TRUE;
		apcliIfidx = apidx - MIN_NET_DEVICE_FOR_APCLI;
	}

	if (bApClientRSNIE)
	{		
		// Only support WPAPSK or WPA2PSK for AP-Client mode    
	    if ((AuthMode != Ndis802_11AuthModeWPAPSK) && (AuthMode != Ndis802_11AuthModeWPA2PSK))
    		return;
	
		pAd->ApCfg.ApCliTab[apcliIfidx].RSNIE_Len = 0;
		NdisZeroMemory(pAd->ApCfg.ApCliTab[apcliIfidx].RSN_IE, MAX_LEN_OF_RSNIE);
	}
	else
#endif // APCLI_SUPPORT //
	{
		if (WepStatus == Ndis802_11Encryption4Enabled)
    	    pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus = Ndis802_11Encryption2Enabled;
    	else
    	    pAd->ApCfg.MBSSID[apidx].GroupKeyWepStatus = WepStatus;

		if ((AuthMode != Ndis802_11AuthModeWPA) && (AuthMode != Ndis802_11AuthModeWPAPSK)
    	    && (AuthMode != Ndis802_11AuthModeWPA2) && (AuthMode != Ndis802_11AuthModeWPA2PSK)
    	    && (AuthMode != Ndis802_11AuthModeWPA1WPA2) && (AuthMode != Ndis802_11AuthModeWPA1PSKWPA2PSK))
    	    return;

    	pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0] = 0;
    	pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1] = 0;
    	NdisZeroMemory(pAd->ApCfg.MBSSID[apidx].RSN_IE[0], MAX_LEN_OF_RSNIE);
    	NdisZeroMemory(pAd->ApCfg.MBSSID[apidx].RSN_IE[1], MAX_LEN_OF_RSNIE);
	}	
#endif // CONFIG_AP_SUPPORT //



    // For WPA1, RSN_IE=221
    if ((AuthMode == Ndis802_11AuthModeWPA) || (AuthMode == Ndis802_11AuthModeWPAPSK) || (AuthMode == Ndis802_11AuthModeWPANone)
        || (AuthMode == Ndis802_11AuthModeWPA1WPA2) || (AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
    {
        RSNIE               *pRsnie;
        RSNIE_AUTH          *pRsnie_auth;
        RSN_CAPABILITIES    *pRSN_Cap;
        UCHAR               Rsnie_size = 0;

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT	
		if (bApClientRSNIE)
			pRsnie = (RSNIE*)pAd->ApCfg.ApCliTab[apcliIfidx].RSN_IE;
		else
#endif // APCLI_SUPPORT //
        pRsnie = (RSNIE*)pAd->ApCfg.MBSSID[apidx].RSN_IE[0];
#endif // CONFIG_AP_SUPPORT //


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
#ifdef CONFIG_AP_SUPPORT				
#ifdef APCLI_SUPPORT
				if (bApClientRSNIE)
				{
					if (pAd->ApCfg.ApCliTab[apcliIfidx].bMixCipher)
						NdisMoveMemory(pRsnie->mcast, OUI_WPA_TKIP, 4);
					else
						NdisMoveMemory(pRsnie->mcast, OUI_WPA_CCMP, 4);
				}
				else
#endif // APCLI_SUPPORT //
                	NdisMoveMemory(pRsnie->mcast, OUI_WPA_CCMP, 4);
#endif // CONFIG_AP_SUPPORT //
                pRsnie->ucount = 1;
                NdisMoveMemory(pRsnie->ucast[0].oui, OUI_WPA_CCMP, 4);
                Rsnie_size = sizeof(RSNIE);
                break;

#ifdef CONFIG_AP_SUPPORT
            case Ndis802_11Encryption4Enabled:
                NdisMoveMemory(pRsnie->mcast, OUI_WPA_TKIP, 4);
                pRsnie->ucount = 2;
                NdisMoveMemory(pRsnie->ucast[0].oui, OUI_WPA_TKIP, 4);
                NdisMoveMemory(pRsnie->ucast[0].oui + 4, OUI_WPA_CCMP, 4);
                Rsnie_size = sizeof(RSNIE) + 4;
                break;
#endif // CONFIG_AP_SUPPORT //					
        }

        pRsnie_auth = (RSNIE_AUTH*)((PUCHAR)pRsnie + Rsnie_size);

        switch (AuthMode)
        {
            case Ndis802_11AuthModeWPA:
            case Ndis802_11AuthModeWPA1WPA2:
                pRsnie_auth->acount = 1;
                NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA_WEP40, 4);
                break;

            case Ndis802_11AuthModeWPAPSK:
            case Ndis802_11AuthModeWPA1PSKWPA2PSK:
                pRsnie_auth->acount = 1;
                NdisMoveMemory(pRsnie_auth->auth[0].oui, OUI_WPA_TKIP, 4);
                break;

        }

        pRSN_Cap = (RSN_CAPABILITIES*)((PUCHAR)pRsnie_auth + sizeof(RSNIE_AUTH));
		
	    pRsnie->version = cpu2le16(pRsnie->version);
	    pRsnie->ucount = cpu2le16(pRsnie->ucount);
	    pRsnie_auth->acount = cpu2le16(pRsnie_auth->acount);
		pRSN_Cap->word = cpu2le16(pRSN_Cap->word);

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
		if (bApClientRSNIE)
			pAd->ApCfg.ApCliTab[apcliIfidx].RSNIE_Len = Rsnie_size + sizeof(RSNIE_AUTH) + sizeof(RSN_CAPABILITIES);
		else
#endif // APCLI_SUPPORT //
        	pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0] = Rsnie_size + sizeof(RSNIE_AUTH) + sizeof(RSN_CAPABILITIES);
#endif // CONFIG_AP_SUPPORT //


    }

    // For WPA2, RSN_IE=48, if WPA1WPA2/WPAPSKWPA2PSK mix mode, we store RSN_IE in RSN_IE[1] else RSNIE[0]
    if ((AuthMode == Ndis802_11AuthModeWPA2) || (AuthMode == Ndis802_11AuthModeWPA2PSK)
        || (AuthMode == Ndis802_11AuthModeWPA1WPA2) || (AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
    {
        RSNIE2              *pRsnie2;
        RSNIE_AUTH          *pRsnie_auth2;
        RSN_CAPABILITIES    *pRSN_Cap;
        UCHAR               Rsnie_size = 0;

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT	
		if (bApClientRSNIE)
			pRsnie2 = (RSNIE2*)pAd->ApCfg.ApCliTab[apcliIfidx].RSN_IE;
		else
#endif // APCLI_SUPPORT //	
		{	
        	if ((AuthMode == Ndis802_11AuthModeWPA1WPA2) || (AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
        	    pRsnie2 = (RSNIE2*)pAd->ApCfg.MBSSID[apidx].RSN_IE[1];
        	else
        	    pRsnie2 = (RSNIE2*)pAd->ApCfg.MBSSID[apidx].RSN_IE[0];
		}	
#endif // CONFIG_AP_SUPPORT //


        pRsnie2->version = 1;

        switch (WepStatus)
        {
            case Ndis802_11Encryption2Enabled:
                NdisMoveMemory(pRsnie2->mcast, OUI_WPA2_TKIP, 4);
                pRsnie2->ucount = 1;
                NdisMoveMemory(pRsnie2->ucast[0].oui, OUI_WPA2_TKIP, 4);
                Rsnie_size = sizeof(RSNIE2);
                break;

            case Ndis802_11Encryption3Enabled:
#ifdef CONFIG_AP_SUPPORT				
#ifdef APCLI_SUPPORT
				if (bApClientRSNIE)
				{
					if (pAd->ApCfg.ApCliTab[apcliIfidx].bMixCipher)
						NdisMoveMemory(pRsnie2->mcast, OUI_WPA2_TKIP, 4);
					else
						NdisMoveMemory(pRsnie2->mcast, OUI_WPA2_CCMP, 4);
				}
				else
#endif // APCLI_SUPPORT //
                	NdisMoveMemory(pRsnie2->mcast, OUI_WPA2_CCMP, 4);
#endif // CONFIG_AP_SUPPORT //	
                pRsnie2->ucount = 1;
                NdisMoveMemory(pRsnie2->ucast[0].oui, OUI_WPA2_CCMP, 4);
                Rsnie_size = sizeof(RSNIE2);
                break;

#ifdef CONFIG_AP_SUPPORT
            case Ndis802_11Encryption4Enabled:
                NdisMoveMemory(pRsnie2->mcast, OUI_WPA2_TKIP, 4);
                pRsnie2->ucount = 2;
                NdisMoveMemory(pRsnie2->ucast[0].oui, OUI_WPA2_TKIP, 4);
                NdisMoveMemory(pRsnie2->ucast[0].oui + 4, OUI_WPA2_CCMP, 4);
                Rsnie_size = sizeof(RSNIE2) + 4;
                break;
#endif // CONFIG_AP_SUPPORT //				
        }

        pRsnie_auth2 = (RSNIE_AUTH*)((PUCHAR)pRsnie2 + Rsnie_size);

        switch (AuthMode)
        {
            case Ndis802_11AuthModeWPA2:
            case Ndis802_11AuthModeWPA1WPA2:
                pRsnie_auth2->acount = 1;
                NdisMoveMemory(pRsnie_auth2->auth[0].oui, OUI_WPA2_WEP40, 4);
                break;

            case Ndis802_11AuthModeWPA2PSK:
            case Ndis802_11AuthModeWPA1PSKWPA2PSK:
                pRsnie_auth2->acount = 1;
                NdisMoveMemory(pRsnie_auth2->auth[0].oui, OUI_WPA2_TKIP, 4);
                break;
        }

        pRSN_Cap = (RSN_CAPABILITIES*)((PUCHAR)pRsnie_auth2 + sizeof(RSNIE_AUTH));
#ifdef CONFIG_AP_SUPPORT		
#ifdef APCLI_SUPPORT
		if (!bApClientRSNIE)
#endif // APCLI_SUPPORT //			
        	pRSN_Cap->field.PreAuth = (pAd->ApCfg.MBSSID[apidx].PreAuth == TRUE) ? 1 : 0;
#endif // CONFIG_AP_SUPPORT //

	    pRsnie2->version = cpu2le16(pRsnie2->version);
	    pRsnie2->ucount = cpu2le16(pRsnie2->ucount);
	    pRsnie_auth2->acount = cpu2le16(pRsnie_auth2->acount);
		pRSN_Cap->word = cpu2le16(pRSN_Cap->word);

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
		if (bApClientRSNIE)
			pAd->ApCfg.ApCliTab[apcliIfidx].RSNIE_Len = Rsnie_size + sizeof(RSNIE_AUTH) + sizeof(RSN_CAPABILITIES);
		else
#endif // APCLI_SUPPORT //
		{
        	if ((AuthMode == Ndis802_11AuthModeWPA1WPA2) || (AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK))
        	    pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1] = Rsnie_size + sizeof(RSNIE_AUTH) + sizeof(RSN_CAPABILITIES);
        	else
        	    pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0] = Rsnie_size + sizeof(RSNIE_AUTH) + sizeof(RSN_CAPABILITIES);
		}
#endif // CONFIG_AP_SUPPORT //

    }

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	if (bApClientRSNIE)
	{
		hex_dump("RTMPMakeRSNIE for ApClient", pAd->ApCfg.ApCliTab[apcliIfidx].RSN_IE, pAd->ApCfg.ApCliTab[apcliIfidx].RSNIE_Len);
		DBGPRINT(RT_DEBUG_TRACE,("RTMPMakeRSNIE in APCLI mode: RSNIE_Len = %d\n", pAd->ApCfg.ApCliTab[apcliIfidx].RSNIE_Len));
	}
	else
#endif // APCLI_SUPPORT //
    	DBGPRINT(RT_DEBUG_TRACE,("RTMPMakeRSNIE in AP mode: RSNIE_Len[0] = %d, RSNIE_Len[1] = %d\n", pAd->ApCfg.MBSSID[apidx].RSNIE_Len[0], pAd->ApCfg.MBSSID[apidx].RSNIE_Len[1]));
#endif // CONFIG_AP_SUPPORT //


}


/*
    ==========================================================================
    Description:
       
    Return:
         TRUE if this is EAP frame
         FALSE otherwise
    ==========================================================================
*/
BOOLEAN RTMPCheckWPAframe(
    IN PRTMP_ADAPTER    pAd,
    IN PUCHAR           pData,
    IN ULONG            DataByteCount)
{
	ULONG	Body_len;

    if(DataByteCount < (LENGTH_802_1_H + LENGTH_EAPOL_H))
        return FALSE;

    DBGPRINT(RT_DEBUG_INFO, ("RTMPCheckWPAframe ===> \n"));
    
	// Skip LLC header	
    if (NdisEqualMemory(SNAP_802_1H, pData, 6) ||
        // Cisco 1200 AP may send packet with SNAP_BRIDGE_TUNNEL
        NdisEqualMemory(SNAP_BRIDGE_TUNNEL, pData, 6)) 
    {
        pData += 6;
    }
    if (NdisEqualMemory(EAPOL, pData, 2)) 
    {
        pData += 2;         
    }
    else    
        return FALSE;

    switch (*(pData+1))     
    {   
        case EAPPacket:
			Body_len = (*(pData+2)<<8) | (*(pData+3));
#ifdef CONFIG_AP_SUPPORT 			
#ifdef IDS_SUPPORT
			if((*(pData+4)) == EAP_CODE_REQUEST)
				pAd->ApCfg.RcvdEapReqCount ++;
#endif // IDS_SUPPORT //			
#endif // CONFIG_AP_SUPPORT //	
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAP-Packet frame, TYPE = 0, Length = %ld\n", Body_len));
            break;
        case EAPOLStart:
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAPOL-Start frame, TYPE = 1 \n"));
            break;
        case EAPOLLogoff:
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAPOLLogoff frame, TYPE = 2 \n"));
            break;
        case EAPOLKey:
			Body_len = (*(pData+2)<<8) | (*(pData+3));
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAPOL-Key frame, TYPE = 3, Length = %ld\n", Body_len));
            break;
        case EAPOLASFAlert:
            DBGPRINT(RT_DEBUG_TRACE, ("Receive EAPOLASFAlert frame, TYPE = 4 \n"));
            break;
        default:
            return FALSE;
    
    }   
    return TRUE;
}


/*
    ==========================================================================
    Description:
        ENCRYPT AES GTK before sending in EAPOL frame.
        AES GTK length = 128 bit,  so fix blocks for aes-key-wrap as 2 in this function.
        This function references to RFC 3394 for aes key wrap algorithm.
    Return:
    ==========================================================================
*/  
VOID AES_GTK_KEY_WRAP( 
    IN UCHAR    *key,
    IN UCHAR    *plaintext,
    IN UCHAR    p_len,
    OUT UCHAR   *ciphertext)
{
    UCHAR       A[8], BIN[16], BOUT[16];
    UCHAR       R[512];
    INT         num_blocks = p_len/8;   // unit:64bits
    INT         i, j;
    aes_context aesctx;
    UCHAR       xor;

    rtmp_aes_set_key(&aesctx, key, 128);

    // Init IA
    for (i = 0; i < 8; i++)
        A[i] = 0xa6;

    //Input plaintext
    for (i = 0; i < num_blocks; i++)
    {
        for (j = 0 ; j < 8; j++)
            R[8 * (i + 1) + j] = plaintext[8 * i + j];
    }

    // Key Mix
    for (j = 0; j < 6; j++)
    {
        for(i = 1; i <= num_blocks; i++)
        {
            //phase 1
            NdisMoveMemory(BIN, A, 8);
            NdisMoveMemory(&BIN[8], &R[8 * i], 8);
            rtmp_aes_encrypt(&aesctx, BIN, BOUT);

            NdisMoveMemory(A, &BOUT[0], 8);
            xor = num_blocks * j + i;
            A[7] = BOUT[7] ^ xor;
            NdisMoveMemory(&R[8 * i], &BOUT[8], 8);
        }
    }

    // Output ciphertext
    NdisMoveMemory(ciphertext, A, 8);

    for (i = 1; i <= num_blocks; i++)
    {
        for (j = 0 ; j < 8; j++)
            ciphertext[8 * i + j] = R[8 * i + j];
    }
}


/*
	========================================================================
	
	Routine Description:
		Misc function to decrypt AES body
	
	Arguments:
			
	Return Value:
	
	Note:
		This function references to	RFC	3394 for aes key unwrap algorithm.
			
	========================================================================
*/
VOID	AES_GTK_KEY_UNWRAP( 
	IN	UCHAR	*key,
	OUT	UCHAR	*plaintext,
	IN	UCHAR    c_len,
	IN	UCHAR	*ciphertext)
	
{
	UCHAR       A[8], BIN[16], BOUT[16];
	UCHAR       xor;
	INT         i, j;
	aes_context aesctx;
	UCHAR       *R;
	INT         num_blocks = c_len/8;	// unit:64bits

	
	os_alloc_mem(NULL, (PUCHAR *)&R, 512);

	if (R == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, ("!!!AES_GTK_KEY_UNWRAP: no memory!!!\n"));
        return;
    } /* End of if */

	// Initialize
	NdisMoveMemory(A, ciphertext, 8);
	//Input plaintext
	for(i = 0; i < (c_len-8); i++)
	{
		R[ i] = ciphertext[i + 8];
	}

	rtmp_aes_set_key(&aesctx, key, 128);

	for(j = 5; j >= 0; j--)
	{
		for(i = (num_blocks-1); i > 0; i--)
		{
			xor = (num_blocks -1 )* j + i;
			NdisMoveMemory(BIN, A, 8);
			BIN[7] = A[7] ^ xor;
			NdisMoveMemory(&BIN[8], &R[(i-1)*8], 8);
			rtmp_aes_decrypt(&aesctx, BIN, BOUT);
			NdisMoveMemory(A, &BOUT[0], 8);
			NdisMoveMemory(&R[(i-1)*8], &BOUT[8], 8);
		}
	}

	// OUTPUT
	for(i = 0; i < c_len; i++)
	{
		plaintext[i] = R[i];
	}

	DBGPRINT_RAW(RT_DEBUG_INFO, ("plaintext = \n"));
	for(i = 0; i < (num_blocks *8); i++)
	{
		DBGPRINT_RAW(RT_DEBUG_INFO, ("%2x ", plaintext[i]));
		if(i%16 == 15)
			DBGPRINT_RAW(RT_DEBUG_INFO, ("\n "));
	}
	DBGPRINT_RAW(RT_DEBUG_INFO, ("\n  \n"));	

	os_free_mem(NULL, R);
}


CHAR *GetEapolMsgType(CHAR msg)
{
    if(msg == EAPOL_PAIR_MSG_1)
        return "Pairwise Message 1";
    else if(msg == EAPOL_PAIR_MSG_2)
        return "Pairwise Message 2";
	else if(msg == EAPOL_PAIR_MSG_3)
        return "Pairwise Message 3";
	else if(msg == EAPOL_PAIR_MSG_4)
        return "Pairwise Message 4";
	else if(msg == EAPOL_GROUP_MSG_1)
        return "Group Message 1";
	else if(msg == EAPOL_GROUP_MSG_2)
        return "Group Message 2";
    else
    	return "Invalid Message";
}


/*
    ========================================================================
    
    Routine Description:
    Check Sanity RSN IE of EAPoL message

    Arguments:
        
    Return Value:

		
    ========================================================================
*/
BOOLEAN RTMPCheckRSNIE(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR          pData,
	IN  UCHAR           DataLen,
	IN  MAC_TABLE_ENTRY *pEntry,
	OUT	UCHAR			*Offset)
{
	PUCHAR              pVIE;
	UCHAR               len;
	PEID_STRUCT         pEid;
	BOOLEAN				result = FALSE;
		
	pVIE = pData;
	len	 = DataLen;
	*Offset = 0;

	while (len > sizeof(RSNIE2))
	{
		pEid = (PEID_STRUCT) pVIE;	
		// WPA RSN IE
		if ((pEid->Eid == IE_WPA) && (NdisEqualMemory(pEid->Octet, WPA_OUI, 4)))
		{			
			if ((pEntry->AuthMode == Ndis802_11AuthModeWPA || pEntry->AuthMode == Ndis802_11AuthModeWPAPSK) &&
				(NdisEqualMemory(pVIE, pEntry->RSN_IE, pEntry->RSNIE_Len)) &&
				(pEntry->RSNIE_Len == (pEid->Len + 2)))
			{				
					DBGPRINT(RT_DEBUG_INFO, ("RTMPCheckRSNIE ==> WPA/WPAPSK RSN IE matched, Length(%d) \n", (pEid->Len + 2)));
					result = TRUE;				
			}		
			
			*Offset += (pEid->Len + 2);			
		}
		// WPA2 RSN IE
		else if ((pEid->Eid == IE_RSN) && (NdisEqualMemory(pEid->Octet + 2, RSN_OUI, 3)))
		{
			if ((pEntry->AuthMode == Ndis802_11AuthModeWPA2 || pEntry->AuthMode == Ndis802_11AuthModeWPA2PSK) &&
				(NdisEqualMemory(pVIE, pEntry->RSN_IE, pEntry->RSNIE_Len)) &&
				(pEntry->RSNIE_Len == (pEid->Len + 2)))
			{				
					DBGPRINT(RT_DEBUG_INFO, ("RTMPCheckRSNIE ==> WPA2/WPA2PSK RSN IE matched, Length(%d) \n", (pEid->Len + 2)));
					result = TRUE;				
			}			

			*Offset += (pEid->Len + 2);
		}		
		else
		{			
			break;
		}

		pVIE += (pEid->Len + 2);
		len  -= (pEid->Len + 2);
	}
	
	DBGPRINT(RT_DEBUG_INFO, ("RTMPCheckRSNIE ==> skip_offset(%d) \n", *Offset));
		
	return result;
	
}


/*
    ========================================================================
    
    Routine Description:
    Parse KEYDATA field.  KEYDATA[] May contain 2 RSN IE and optionally GTK.  
    GTK  is encaptulated in KDE format at  p.83 802.11i D10

    Arguments:
        
    Return Value:

    Note:
        802.11i D10  
        
    ========================================================================
*/
BOOLEAN RTMPParseEapolKeyData(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR          pKeyData,
	IN  UCHAR           KeyDataLen,
	IN	UCHAR			GroupKeyIndex,
	IN	UCHAR			MsgType,
	IN	BOOLEAN			bWPA2,
	IN  MAC_TABLE_ENTRY *pEntry)
{
    PKDE_ENCAP          pKDE = NULL;
    PUCHAR              pMyKeyData = pKeyData;
    UCHAR               KeyDataLength = KeyDataLen;
    UCHAR               GTKLEN = 0;
	UCHAR				DefaultIdx = 0;
	UCHAR				skip_offset;		
	    
	// Verify The RSN IE contained in pairewise_msg_2 && pairewise_msg_3 and skip it
	if (MsgType == EAPOL_PAIR_MSG_2 || MsgType == EAPOL_PAIR_MSG_3)
    {
		// Check RSN IE whether it is WPA2/WPA2PSK   		
		if (!RTMPCheckRSNIE(pAd, pKeyData, KeyDataLen, pEntry, &skip_offset))
		{
			// send wireless event - for RSN IE different
			if (pAd->CommonCfg.bWirelessEvent)
				RTMPSendWirelessEvent(pAd, IW_RSNIE_DIFF_EVENT_FLAG, pEntry->Addr, pEntry->apidx, 0); 

        	DBGPRINT(RT_DEBUG_ERROR, ("RSN_IE Different in msg %d of 4-way handshake!\n", MsgType));			
			hex_dump("Receive RSN_IE ", pKeyData, KeyDataLen);
			hex_dump("Desired RSN_IE ", pEntry->RSN_IE, pEntry->RSNIE_Len);	
					
			return FALSE;			
    	}
    	else
		{
			if (bWPA2 && MsgType == EAPOL_PAIR_MSG_3)
			{
				// skip RSN IE
				pMyKeyData += skip_offset;
				KeyDataLength -= skip_offset;
				DBGPRINT(RT_DEBUG_TRACE, ("RTMPParseEapolKeyData ==> WPA2/WPA2PSK RSN IE matched in Msg 3, Length(%d) \n", skip_offset));
			}
			else
				return TRUE;			
		}
	}

	DBGPRINT(RT_DEBUG_TRACE,("RTMPParseEapolKeyData ==> KeyDataLength %d without RSN_IE \n", KeyDataLength));

	// Parse EKD format in pairwise_msg_3_WPA2 && group_msg_1_WPA2
	if (bWPA2 && (MsgType == EAPOL_PAIR_MSG_3 || MsgType == EAPOL_GROUP_MSG_1))
	{				
		if (KeyDataLength >= 8)	// KDE format exclude GTK length
    	{
        	pKDE = (PKDE_ENCAP) pMyKeyData;			
	        DBGPRINT(RT_DEBUG_INFO, ("pKDE->Type %x \n", pKDE->Type));
    	    DBGPRINT(RT_DEBUG_INFO, ("pKDE->Len 0x%x \n", pKDE->Len));
        	DBGPRINT(RT_DEBUG_INFO, ("pKDE->OUI %x %x %x \n", pKDE->OUI[0],pKDE->OUI[1],pKDE->OUI[2]));
	    	DBGPRINT(RT_DEBUG_INFO, ("pKDE->DataType %x \n", pKDE->DataType));
			
			DefaultIdx = pKDE->GTKEncap.Kid;

			// Sanity check - KED length
			if (KeyDataLength < (pKDE->Len + 2))
    		{
        		DBGPRINT(RT_DEBUG_ERROR, ("ERROR: The len from KDE is too short \n"));
        		return FALSE;			
    		}

			// Get GTK length - refer to IEEE 802.11i-2004 p.82
			GTKLEN = pKDE->Len -6;
			if (GTKLEN < LEN_AES_KEY)
			{
				DBGPRINT(RT_DEBUG_ERROR, ("ERROR: GTK Key length is too short (%d) \n", GTKLEN));
        		return FALSE;
			}
			
    	}
		else
    	{
			DBGPRINT(RT_DEBUG_ERROR, ("ERROR: KDE format length is too short \n"));
	        return FALSE;
    	}

		DBGPRINT(RT_DEBUG_TRACE, ("GTK in KDE format ,DefaultKeyID=%d, KeyLen=%d \n", DefaultIdx, GTKLEN));
		// skip it
		pMyKeyData += 8;
		KeyDataLength -= 8;
		
	}
	else if (!bWPA2 && MsgType == EAPOL_GROUP_MSG_1)
	{
		DefaultIdx = GroupKeyIndex;
		DBGPRINT(RT_DEBUG_TRACE, ("GTK DefaultKeyID=%d \n", DefaultIdx));
	}
		
	// Sanity check - shared key index must be 1 ~ 3
	if (DefaultIdx < 1 || DefaultIdx > 3)
    {
     	DBGPRINT(RT_DEBUG_ERROR, ("ERROR: GTK Key index(%d) is invalid in %s %s \n", DefaultIdx, ((bWPA2) ? "WPA2" : "WPA"), GetEapolMsgType(MsgType)));
        return FALSE;
    } 

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT		
	// Set Group key material, TxMic and RxMic for AP-Client
	if (!APCliUpdateSharedKeyTable(pAd, pMyKeyData, KeyDataLength, DefaultIdx, pEntry))
	{		
		return FALSE;
	}

#endif // APCLI_SUPPORT //
#endif // CONFIG_AP_SUPPORT //


	return TRUE;
 
}


/*
	========================================================================
	
	Routine Description:
		Construct EAPoL message for WPA handshaking 
		Its format is below,
		
		+--------------------+
		| Protocol Version	 |  1 octet
		+--------------------+
		| Protocol Type		 |	1 octet	
		+--------------------+
		| Body Length		 |  2 octets
		+--------------------+
		| Descriptor Type	 |	1 octet
		+--------------------+
		| Key Information    |	2 octets
		+--------------------+
		| Key Length	     |  1 octet
		+--------------------+
		| Key Repaly Counter |	8 octets
		+--------------------+
		| Key Nonce		     |  32 octets
		+--------------------+
		| Key IV			 |  16 octets
		+--------------------+
		| Key RSC			 |  8 octets
		+--------------------+
		| Key ID or Reserved |	8 octets
		+--------------------+
		| Key MIC			 |	8 octets
		+--------------------+
		| Key Data Length	 |	2 octets
		+--------------------+
		| Key Data			 |	n octets
		+--------------------+
		

	Arguments:
		pAd			Pointer	to our adapter
				
	Return Value:
		None
		
	Note:
		
	========================================================================
*/
VOID	ConstructEapolMsg(
	IN 	PRTMP_ADAPTER    	pAd,
    IN 	UCHAR				AuthMode,
    IN 	UCHAR				WepStatus,
    IN 	UCHAR				GroupKeyWepStatus,
    IN 	UCHAR				MsgType,  
    IN	UCHAR				DefaultKeyIdx,
    IN 	UCHAR				*ReplayCounter,
	IN 	UCHAR				*KeyNonce,
	IN	UCHAR				*TxRSC,
	IN	UCHAR				*PTK,
	IN	UCHAR				*GTK,
	IN	UCHAR				*RSNIE,
	IN	UCHAR				RSNIE_Len,
    OUT PEAPOL_PACKET       pMsg)
{
	BOOLEAN	bWPA2 = FALSE;

	// Choose WPA2 or not
	if ((AuthMode == Ndis802_11AuthModeWPA2) || (AuthMode == Ndis802_11AuthModeWPA2PSK))
		bWPA2 = TRUE;
		
    // Init Packet and Fill header    
    pMsg->ProVer = EAPOL_VER;
    pMsg->ProType = EAPOLKey;

	// Default 95 bytes, the EAPoL-Key descriptor exclude Key-data field
	pMsg->Body_Len[1] = LEN_EAPOL_KEY_MSG;  

	// Fill in EAPoL descriptor
	if (bWPA2)
		pMsg->KeyDesc.Type = WPA2_KEY_DESC;
	else
		pMsg->KeyDesc.Type = WPA1_KEY_DESC;
			
	// Fill in Key information, refer to IEEE Std 802.11i-2004 page 78 
	// When either the pairwise or the group cipher is AES, the DESC_TYPE_AES(2) shall be used.
	pMsg->KeyDesc.KeyInfo.KeyDescVer = 
        	(((WepStatus == Ndis802_11Encryption3Enabled) || (GroupKeyWepStatus == Ndis802_11Encryption3Enabled)) ? (DESC_TYPE_AES) : (DESC_TYPE_TKIP));

	// Specify Key Type as Group(0) or Pairwise(1)
	if (MsgType >= EAPOL_GROUP_MSG_1)
		pMsg->KeyDesc.KeyInfo.KeyType = GROUPKEY;
	else
		pMsg->KeyDesc.KeyInfo.KeyType = PAIRWISEKEY;

	// Specify Key Index, only group_msg1_WPA1
	if (!bWPA2 && (MsgType >= EAPOL_GROUP_MSG_1))
		pMsg->KeyDesc.KeyInfo.KeyIndex = DefaultKeyIdx;
	
	if (MsgType == EAPOL_PAIR_MSG_3)
		pMsg->KeyDesc.KeyInfo.Install = 1;
	
	if ((MsgType == EAPOL_PAIR_MSG_1) || (MsgType == EAPOL_PAIR_MSG_3) || (MsgType == EAPOL_GROUP_MSG_1))
		pMsg->KeyDesc.KeyInfo.KeyAck = 1;

	if (MsgType != EAPOL_PAIR_MSG_1)	
		pMsg->KeyDesc.KeyInfo.KeyMic = 1;
 
	if ((bWPA2 && (MsgType >= EAPOL_PAIR_MSG_3)) || (!bWPA2 && (MsgType >= EAPOL_GROUP_MSG_1)))
    {                        
       	pMsg->KeyDesc.KeyInfo.Secure = 1;                   
    }

	if (bWPA2 && ((MsgType == EAPOL_PAIR_MSG_3) || (MsgType == EAPOL_GROUP_MSG_1)))
    {                               	
        pMsg->KeyDesc.KeyInfo.EKD_DL = 1;            
    }

	// key Information element has done. 
	*(USHORT *)(&pMsg->KeyDesc.KeyInfo) = cpu2le16(*(USHORT *)(&pMsg->KeyDesc.KeyInfo));

	// Fill in Key Length
#if 0
	if (bWPA2)
	{
		// In WPA2 mode, the field indicates the length of pairwise key cipher, 
		// so only pairwise_msg_1 and pairwise_msg_3 need to fill. 
		if ((MsgType == EAPOL_PAIR_MSG_1) || (MsgType == EAPOL_PAIR_MSG_3))
			pMsg->KeyDesc.KeyLength[1] = ((WepStatus == Ndis802_11Encryption2Enabled) ? LEN_TKIP_KEY : LEN_AES_KEY);
	}
	else if (!bWPA2)
#endif
	{
		if (MsgType >= EAPOL_GROUP_MSG_1)
		{
			// the length of group key cipher
			pMsg->KeyDesc.KeyLength[1] = ((GroupKeyWepStatus == Ndis802_11Encryption2Enabled) ? TKIP_GTK_LENGTH : LEN_AES_KEY);
		}
		else
		{
			// the length of pairwise key cipher
			pMsg->KeyDesc.KeyLength[1] = ((WepStatus == Ndis802_11Encryption2Enabled) ? LEN_TKIP_KEY : LEN_AES_KEY);			
		}				
	}			
	
 	// Fill in replay counter        		
    NdisMoveMemory(pMsg->KeyDesc.ReplayCounter, ReplayCounter, LEN_KEY_DESC_REPLAY);

	// Fill Key Nonce field		  
	// ANonce : pairwise_msg1 & pairwise_msg3
	// SNonce : pairwise_msg2
	// GNonce : group_msg1_wpa1	
	if ((MsgType <= EAPOL_PAIR_MSG_3) || ((!bWPA2 && (MsgType == EAPOL_GROUP_MSG_1))))
    	NdisMoveMemory(pMsg->KeyDesc.KeyNonce, KeyNonce, LEN_KEY_DESC_NONCE);

	// Fill key IV - WPA2 as 0, WPA1 as random
	if (!bWPA2 && (MsgType == EAPOL_GROUP_MSG_1))
	{		
		// Suggest IV be random number plus some number,
		NdisMoveMemory(pMsg->KeyDesc.KeyIv, &KeyNonce[16], LEN_KEY_DESC_IV);		
        pMsg->KeyDesc.KeyIv[15] += 2;		
	}
	
    // Fill Key RSC field        
    // It contains the RSC for the GTK being installed.
	if ((MsgType == EAPOL_PAIR_MSG_3 && bWPA2) || (MsgType == EAPOL_GROUP_MSG_1))
	{		
        NdisMoveMemory(pMsg->KeyDesc.KeyRsc, TxRSC, 6);
	}

	// Clear Key MIC field for MIC calculation later   
    NdisZeroMemory(pMsg->KeyDesc.KeyMic, LEN_KEY_DESC_MIC);
	
	ConstructEapolKeyData(pAd, 
						  AuthMode,
						  WepStatus,
						  GroupKeyWepStatus, 
						  MsgType, 
						  DefaultKeyIdx, 
						  bWPA2, 
						  PTK,
						  GTK,
						  RSNIE,
						  RSNIE_Len,
						  pMsg);
 
	// Calculate MIC and fill in KeyMic Field except Pairwise Msg 1.
	if (MsgType != EAPOL_PAIR_MSG_1)
	{
		CalculateMIC(pAd, WepStatus, PTK, pMsg);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("===> ConstructEapolMsg for %s %s\n", ((bWPA2) ? "WPA2" : "WPA"), GetEapolMsgType(MsgType)));
	DBGPRINT(RT_DEBUG_TRACE, ("	     Body length = %d \n", pMsg->Body_Len[1]));
	DBGPRINT(RT_DEBUG_TRACE, ("	     Key length  = %d \n", pMsg->KeyDesc.KeyLength[1]));


}

/*
	========================================================================
	
	Routine Description:
		Construct the Key Data field of EAPoL message 

	Arguments:
		pAd			Pointer	to our adapter
		Elem		Message body
		
	Return Value:
		None
		
	Note:
		
	========================================================================
*/
VOID	ConstructEapolKeyData(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			AuthMode,
	IN	UCHAR			WepStatus,
	IN	UCHAR			GroupKeyWepStatus,
	IN 	UCHAR			MsgType,
	IN	UCHAR			DefaultKeyIdx,
	IN	BOOLEAN			bWPA2Capable,
	IN	UCHAR			*PTK,
	IN	UCHAR			*GTK,
	IN	UCHAR			*RSNIE,
	IN	UCHAR			RSNIE_LEN,
	OUT PEAPOL_PACKET   pMsg)
{
	UCHAR		*mpool, *Key_Data, *Rc4GTK;  
	UCHAR       ekey[(LEN_KEY_DESC_IV+LEN_EAP_EK)];   
	UCHAR		data_offset;


	if (MsgType == EAPOL_PAIR_MSG_1 || MsgType == EAPOL_PAIR_MSG_4 || MsgType == EAPOL_GROUP_MSG_2)
		return;
 
	// allocate memory pool
	os_alloc_mem(pAd, (PUCHAR *)&mpool, 1500);

    if (mpool == NULL)
		return;
        
	/* Rc4GTK Len = 512 */
	Rc4GTK = (UCHAR *) ROUND_UP(mpool, 4);
	/* Key_Data Len = 512 */
	Key_Data = (UCHAR *) ROUND_UP(Rc4GTK + 512, 4);

	NdisZeroMemory(Key_Data, 512);
	pMsg->KeyDesc.KeyDataLen[1] = 0;
	data_offset = 0;
	
	// Encapsulate RSNIE in pairwise_msg2 & pairwise_msg3		
	if (RSNIE_LEN && ((MsgType == EAPOL_PAIR_MSG_2) || (MsgType == EAPOL_PAIR_MSG_3)))
	{
		if (bWPA2Capable)			
			Key_Data[data_offset + 0] = IE_WPA2;		
		else						 				
			Key_Data[data_offset + 0] = IE_WPA;            			
		
        Key_Data[data_offset + 1] = RSNIE_LEN;
		NdisMoveMemory(&Key_Data[data_offset + 2], RSNIE, RSNIE_LEN);
		data_offset += (2 + RSNIE_LEN);
	}

	// Encapsulate KDE format in pairwise_msg3_WPA2 & group_msg1_WPA2
	if (bWPA2Capable && ((MsgType == EAPOL_PAIR_MSG_3) || (MsgType == EAPOL_GROUP_MSG_1)))
	{
		// Key Data Encapsulation (KDE) format - 802.11i-2004  Figure-43w and Table-20h
        Key_Data[data_offset + 0] = 0xDD;

		if (GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
		{
			Key_Data[data_offset + 1] = 0x16;// 4+2+16(OUI+DataType+DataField)			
		}
		else
		{
			Key_Data[data_offset + 1] = 0x26;// 4+2+32(OUI+DataType+DataField)			
		}
		
        Key_Data[data_offset + 2] = 0x00;
        Key_Data[data_offset + 3] = 0x0F;
        Key_Data[data_offset + 4] = 0xAC;
        Key_Data[data_offset + 5] = 0x01;

		// GTK KDE format - 802.11i-2004  Figure-43x
        Key_Data[data_offset + 6] = (DefaultKeyIdx & 0x03);
        Key_Data[data_offset + 7] = 0x00;	// Reserved Byte
				
		data_offset += 8;
	}


	// Encapsulate GTK and encrypt the key-data field with KEK.
	// Only for pairwise_msg3_WPA2 and group_msg1
	if ((MsgType == EAPOL_PAIR_MSG_3 && bWPA2Capable) || (MsgType == EAPOL_GROUP_MSG_1))
	{		
		// Fill in GTK 
		if (GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
		{			
			NdisMoveMemory(&Key_Data[data_offset], GTK, LEN_AES_KEY);
			data_offset += LEN_AES_KEY;
		}
		else
		{			
			NdisMoveMemory(&Key_Data[data_offset], GTK, TKIP_GTK_LENGTH);
			data_offset += TKIP_GTK_LENGTH;
		}

		// Still dont know why, but if not append will occur "GTK not include in MSG3"
		// Patch for compatibility between zero config and funk
		if (MsgType == EAPOL_PAIR_MSG_3 && bWPA2Capable)
		{
			if (GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
			{
				Key_Data[data_offset + 0] = 0xDD;
				Key_Data[data_offset + 1] = 0;
				data_offset += 2;
			}
			else
			{
				Key_Data[data_offset + 0] = 0xDD;
				Key_Data[data_offset + 1] = 0;
				Key_Data[data_offset + 2] = 0;
				Key_Data[data_offset + 3] = 0;
				Key_Data[data_offset + 4] = 0;
				Key_Data[data_offset + 5] = 0;
				data_offset += 6;
			}
		}

		// Encrypt the data material in key data field
		if (WepStatus == Ndis802_11Encryption3Enabled)
		{
			AES_GTK_KEY_WRAP(&PTK[16], Key_Data, data_offset, Rc4GTK);
            // AES wrap function will grow 8 bytes in length
            data_offset += 8;            				
		}
		else
		{
			// PREPARE Encrypted  "Key DATA" field.  (Encrypt GTK with RC4, usinf PTK[16]->[31] as Key, IV-field as IV)
			// put TxTsc in Key RSC field
			pAd->PrivateInfo.FCSCRC32 = PPPINITFCS32;   //Init crc32.

			// ekey is the contanetion of IV-field, and PTK[16]->PTK[31]
			NdisMoveMemory(ekey, pMsg->KeyDesc.KeyIv, LEN_KEY_DESC_IV);
			NdisMoveMemory(&ekey[LEN_KEY_DESC_IV], &PTK[16], LEN_EAP_EK);
			ARCFOUR_INIT(&pAd->PrivateInfo.WEPCONTEXT, ekey, sizeof(ekey));  //INIT SBOX, KEYLEN+3(IV)
			pAd->PrivateInfo.FCSCRC32 = RTMP_CALC_FCS32(pAd->PrivateInfo.FCSCRC32, Key_Data, data_offset);
			WPAARCFOUR_ENCRYPT(&pAd->PrivateInfo.WEPCONTEXT, Rc4GTK, Key_Data, data_offset);  
		}

		NdisMoveMemory(pMsg->KeyDesc.KeyData, Rc4GTK, data_offset);
	}
	else
	{
		NdisMoveMemory(pMsg->KeyDesc.KeyData, Key_Data, data_offset);
	}

	// set key data length field and total length
	pMsg->KeyDesc.KeyDataLen[1] = data_offset;		
    pMsg->Body_Len[1] += data_offset;

	os_free_mem(pAd, mpool);

}


VOID	CalculateMIC(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			PeerWepStatus,
	IN	UCHAR			*PTK,
	OUT PEAPOL_PACKET   pMsg)
{
    UCHAR   *OutBuffer;
	ULONG	FrameLen = 0;
	UCHAR	mic[LEN_KEY_DESC_MIC];
	UCHAR	digest[80];

	// allocate memory for MIC calculation
	os_alloc_mem(pAd, (PUCHAR *)&OutBuffer, 512);

    if (OutBuffer == NULL)
    {
		DBGPRINT(RT_DEBUG_ERROR, ("!!!CalculateMIC: no memory!!!\n"));
		return;
    }
		
	// make a frame for calculating MIC.
    MakeOutgoingFrame(OutBuffer,            	&FrameLen,
                      pMsg->Body_Len[1] + 4,  	pMsg,
                      END_OF_ARGS);

	NdisZeroMemory(mic, sizeof(mic));
			
	// Calculate MIC
    if (PeerWepStatus == Ndis802_11Encryption3Enabled)
 	{
		HMAC_SHA1(OutBuffer,  FrameLen, PTK, LEN_EAP_MICK, digest);
		NdisMoveMemory(mic, digest, LEN_KEY_DESC_MIC);
	}
	else
	{
		hmac_md5(PTK,  LEN_EAP_MICK, OutBuffer, FrameLen, mic);
	}

	NdisMoveMemory(pMsg->KeyDesc.KeyMic, mic, LEN_KEY_DESC_MIC);

	os_free_mem(pAd, OutBuffer);
}

