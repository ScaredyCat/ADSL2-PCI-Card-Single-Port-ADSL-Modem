#include	"rt_config.h"


/*
	========================================================================
	
	Routine Description:
		Add WPA key process.
		In Adhoc WPANONE, bPairwise = 0;  KeyIdx = 0;

	Arguments:
		pAd 					Pointer to our adapter
		pBuf							Pointer to the where the key stored

	Return Value:
		NDIS_SUCCESS					Add key successfully

	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
#if 0 // remove by AlbertY
NDIS_STATUS RTMPWPAAddKeyProc(
	IN	PRTMP_ADAPTER	pAd,
	IN	PVOID			pBuf)
{
	PNDIS_802_11_KEY	pKey;
	ULONG				KeyIdx;
//	NDIS_STATUS 		Status;
//	ULONG 	offset;	// unused variable, snowpin 2006.07.13
	
	PUCHAR		pTxMic, pRxMic;
	BOOLEAN 	bTxKey; 		// Set the key as transmit key
	BOOLEAN 	bPairwise;		// Indicate the key is pairwise key
	BOOLEAN 	bKeyRSC;		// indicate the receive  SC set by KeyRSC value.
								// Otherwise, it will set by the NIC.
	BOOLEAN 	bAuthenticator; // indicate key is set by authenticator.
	UCHAR		apidx = BSS0;
	
	pKey = (PNDIS_802_11_KEY) pBuf;
	KeyIdx = pKey->KeyIndex & 0xff;
	// Bit 31 of Add-key, Tx Key
	bTxKey		   = (pKey->KeyIndex & 0x80000000) ? TRUE : FALSE;
	// Bit 30 of Add-key PairwiseKey
	bPairwise	   = (pKey->KeyIndex & 0x40000000) ? TRUE : FALSE;
	// Bit 29 of Add-key KeyRSC
	bKeyRSC 	   = (pKey->KeyIndex & 0x20000000) ? TRUE : FALSE;
	// Bit 28 of Add-key Authenticator
	bAuthenticator = (pKey->KeyIndex & 0x10000000) ? TRUE : FALSE;

	DBGPRINT(RT_DEBUG_TRACE,("RTMPWPAAddKeyProc==>pKey->KeyIndex = %x. bPairwise= %d\n", pKey->KeyIndex, bPairwise));
	// 1. Check Group / Pairwise Key
	if (bPairwise)	// Pairwise Key
	{
		// 1. KeyIdx must be 0, otherwise, return NDIS_STATUS_INVALID_DATA
		if (KeyIdx != 0)
			return(NDIS_STATUS_INVALID_DATA);
		
		// 2. Check bTx, it must be true, otherwise, return NDIS_STATUS_INVALID_DATA
		if (bTxKey == FALSE)
			return(NDIS_STATUS_INVALID_DATA);

		// 3. If BSSID is all 0xff, return NDIS_STATUS_INVALID_DATA
		if (MAC_ADDR_EQUAL(pKey->BSSID, BROADCAST_ADDR))
			return(NDIS_STATUS_INVALID_DATA);

		// 3.1 Check Pairwise key length for TKIP key. For AES, it's always 128 bits
		//if ((pAdapter->PortCfg.WepStatus == Ndis802_11Encryption2Enabled) && (pKey->KeyLength != LEN_TKIP_KEY))
		if ((pAd->StaCfg.PairCipher == Ndis802_11Encryption2Enabled) && (pKey->KeyLength != LEN_TKIP_KEY))
			return(NDIS_STATUS_INVALID_DATA);

		pAd->SharedKey[apidx][KeyIdx].Type = PAIRWISE_KEY;

		if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2)
		{
			// Send media specific event to start PMKID caching
			RTMPIndicateWPA2Status(pAd);
		}
	}
	else
	{
		// 1. Check BSSID, if not current BSSID or Bcast, return NDIS_STATUS_INVALID_DATA
		if ((! MAC_ADDR_EQUAL(pKey->BSSID, BROADCAST_ADDR)) &&
			(! MAC_ADDR_EQUAL(pKey->BSSID, pAd->ApCfg.MBSSID[apidx].Bssid)))
			return(NDIS_STATUS_INVALID_DATA);

		// 2. Check Key index for supported Group Key
		if (KeyIdx >= GROUP_KEY_NUM)
			return(NDIS_STATUS_INVALID_DATA);
		
		// 3. Set as default Tx Key if bTxKey is TRUE
		if (bTxKey == TRUE)
			pAd->ApCfg.MBSSID[apidx].DefaultKeyId = (UCHAR) KeyIdx;

		pAd->SharedKey[apidx][KeyIdx].Type = GROUP_KEY;
	}
			
	// 4. Select RxMic / TxMic based on Supp / Authenticator
	if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPANone)
	{
		// for WPA-None Tx, Rx MIC is the same
		pTxMic = (PUCHAR) (&pKey->KeyMaterial) + 16;
		pRxMic = pTxMic;
	}
	else if (bAuthenticator == TRUE)
	{
		pTxMic = (PUCHAR) (&pKey->KeyMaterial) + 16;
		pRxMic = (PUCHAR) (&pKey->KeyMaterial) + 24;
	}
	else
	{
		pRxMic = (PUCHAR) (&pKey->KeyMaterial) + 16;
		pTxMic = (PUCHAR) (&pKey->KeyMaterial) + 24;
	}
		
	// 6. Check RxTsc
	if (bKeyRSC == TRUE)
	{
		NdisMoveMemory(pAd->SharedKey[apidx][KeyIdx].RxTsc, &pKey->KeyRSC, 6);			
		NdisMoveMemory(pAd->MacTab.Content[BSSID_WCID].PairwiseKey.RxTsc, &pKey->KeyRSC, 6);			
	}
	else
	{
		NdisZeroMemory(pAd->SharedKey[apidx][KeyIdx].RxTsc, 6);		
	}
		
	// 7. Copy information into Pairwise Key structure.
	// pKey->KeyLength will include TxMic and RxMic, therefore, we use 16 bytes hardcoded.
	pAd->SharedKey[apidx][KeyIdx].KeyLen = (UCHAR) pKey->KeyLength;		
	pAd->MacTab.Content[BSSID_WCID].PairwiseKey.KeyLen = (UCHAR)pKey->KeyLength;
	NdisMoveMemory(pAd->SharedKey[BSS0][KeyIdx].Key, &pKey->KeyMaterial, 16);
	NdisMoveMemory(pAd->MacTab.Content[BSSID_WCID].PairwiseKey.Key, &pKey->KeyMaterial, 16);
	if (pKey->KeyLength == LEN_TKIP_KEY)
	{
		// Only Key lenth equal to TKIP key have these
		NdisMoveMemory(pAd->SharedKey[apidx][KeyIdx].RxMic, pRxMic, 8);
		NdisMoveMemory(pAd->SharedKey[apidx][KeyIdx].TxMic, pTxMic, 8);
		NdisMoveMemory(pAd->MacTab.Content[BSSID_WCID].PairwiseKey.RxMic, pRxMic, 8);
		NdisMoveMemory(pAd->MacTab.Content[BSSID_WCID].PairwiseKey.TxMic, pTxMic, 8);
	}

	COPY_MAC_ADDR(pAd->SharedKey[BSS0][KeyIdx].BssId, pKey->BSSID);

	// Init TxTsc to one based on WiFi WPA specs
	pAd->SharedKey[apidx][KeyIdx].TxTsc[0] = 1;
	pAd->SharedKey[apidx][KeyIdx].TxTsc[1] = 0;
	pAd->SharedKey[apidx][KeyIdx].TxTsc[2] = 0;
	pAd->SharedKey[apidx][KeyIdx].TxTsc[3] = 0;
	pAd->SharedKey[apidx][KeyIdx].TxTsc[4] = 0;
	pAd->SharedKey[apidx][KeyIdx].TxTsc[5] = 0;
	// 4. Init TxTsc to one based on WiFi WPA specs
	pAd->MacTab.Content[BSSID_WCID].PairwiseKey.TxTsc[0] = 1;
	pAd->MacTab.Content[BSSID_WCID].PairwiseKey.TxTsc[1] = 0;
	pAd->MacTab.Content[BSSID_WCID].PairwiseKey.TxTsc[2] = 0;
	pAd->MacTab.Content[BSSID_WCID].PairwiseKey.TxTsc[3] = 0;
	pAd->MacTab.Content[BSSID_WCID].PairwiseKey.TxTsc[4] = 0;
	pAd->MacTab.Content[BSSID_WCID].PairwiseKey.TxTsc[5] = 0;

	if (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption3Enabled)
	{	
		pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_AES;
		pAd->MacTab.Content[BSSID_WCID].PairwiseKey.CipherAlg = CIPHER_AES;
	}
	else if (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption2Enabled)
	{	
		pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_TKIP;
		pAd->MacTab.Content[BSSID_WCID].PairwiseKey.CipherAlg = CIPHER_TKIP;
	}
	else if (pAd->ApCfg.MBSSID[apidx].WepStatus == Ndis802_11Encryption1Enabled)
	{
		if (pAd->SharedKey[apidx][KeyIdx].KeyLen == 5)
		{
			pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_WEP64;
			pAd->MacTab.Content[BSSID_WCID].PairwiseKey.CipherAlg = CIPHER_WEP64;
		}
		else if (pAd->SharedKey[apidx][KeyIdx].KeyLen == 13)
		{
			pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_WEP128;
			pAd->MacTab.Content[BSSID_WCID].PairwiseKey.CipherAlg = CIPHER_WEP128;
		}
		else
		{
			pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_NONE;
			pAd->MacTab.Content[BSSID_WCID].PairwiseKey.CipherAlg = CIPHER_NONE;
		}
	}
	else
	{
		pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_NONE;
		pAd->MacTab.Content[BSSID_WCID].PairwiseKey.CipherAlg = CIPHER_NONE;
	}

	if ((pAd->OpMode == OPMODE_STA))  // Pairwise Key. Add BSSID to WCTable
	{
		pAd->MacTab.Content[BSSID_WCID].PairwiseKey.CipherAlg = pAd->SharedKey[BSS0][KeyIdx].CipherAlg;
		pAd->MacTab.Content[BSSID_WCID].PairwiseKey.KeyLen = pAd->SharedKey[BSS0][KeyIdx].KeyLen;
	}

	if ((pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2) || 
		(pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPA2PSK))
	{
		//
		// On WPA2, Update Group Key Cipher.
		//
		if (!bPairwise)
		{
			if (pAd->StaCfg.GroupCipher == Ndis802_11Encryption3Enabled)
				pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_AES;
			else if (pAd->StaCfg.GroupCipher == Ndis802_11Encryption2Enabled)
				pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_TKIP;
		}
	}

	DBGPRINT(RT_DEBUG_TRACE, ("pAd->SharedKey[%d][%d].CipherAlg = %d\n", apidx, KeyIdx, pAd->SharedKey[apidx][KeyIdx].CipherAlg));

#if 0
	DBGPRINT_RAW(RT_DEBUG_TRACE, ("%s Key #%d", CipherName[pAd->SharedKey[apidx][KeyIdx].CipherAlg],KeyIdx));
	for (i = 0; i < 16; i++)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("%02x:", pAd->SharedKey[apidx][KeyIdx].Key[i]));
	}
	DBGPRINT_RAW(RT_DEBUG_TRACE, ("\n	  Rx MIC Key = "));
	for (i = 0; i < 8; i++)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("%02x:", pAd->SharedKey[apidx][KeyIdx].RxMic[i]));
	}
	DBGPRINT_RAW(RT_DEBUG_TRACE, ("\n	  Tx MIC Key = "));
	for (i = 0; i < 8; i++)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("%02x:", pAd->SharedKey[apidx][KeyIdx].TxMic[i]));
	}
	DBGPRINT_RAW(RT_DEBUG_TRACE, ("\n	  RxTSC = "));
	for (i = 0; i < 6; i++)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("%02x:", pAd->SharedKey[apidx][KeyIdx].RxTsc[i]));
	}
#endif	  
	DBGPRINT_RAW(RT_DEBUG_TRACE, ("\n pKey-> BSSID:%02x:%02x:%02x:%02x:%02x:%02x \n",
		pKey->BSSID[0],pKey->BSSID[1],pKey->BSSID[2],pKey->BSSID[3],pKey->BSSID[4],pKey->BSSID[5]));

	if ((bTxKey) && (pAd->OpMode == OPMODE_STA))  // Pairwise Key. Add BSSID to WCTable
		RTMPAddBSSIDCipher(pAd, BSSID_WCID, pKey, pAd->SharedKey[BSS0][KeyIdx].CipherAlg);
	

	// No matter pairwise key or what leyidx is, always has a copy at on-chip SharedKeytable.
	AsicAddSharedKeyEntry(pAd, 
						  apidx, 
						  (UCHAR)KeyIdx, 
						  pAd->SharedKey[apidx][KeyIdx].CipherAlg,
						  pAd->SharedKey[apidx][KeyIdx].Key, 
						  pAd->SharedKey[apidx][KeyIdx].TxMic,
						  pAd->SharedKey[apidx][KeyIdx].RxMic);

	// The WCID key specified in used at Tx. For STA, always use pairwise key.

	// ad-hoc mode need to specify WAP Group key with WCID index=BSS0Mcast_WCID. Let's always set this key here.
/*	if (bPairwise == FALSE)
	{
		offset = MAC_IVEIV_TABLE_BASE + (BSS0Mcast_WCID * HW_IVEIV_ENTRY_SIZE);
		NdisZeroMemory(IVEIV, 8);
		// 1. IV/EIV
		// Specify key index to find shared key.
		if ((pAd->SharedKey[BSS0][KeyIdx].CipherAlg==CIPHER_TKIP) ||
			(pAd->SharedKey[BSS0][KeyIdx].CipherAlg==CIPHER_AES))
		IVEIV[3] = 0x20;		// Eiv bit on. keyid always 0 for pairwise key
		IVEIV[3] |= (KeyIdx<< 6);	// groupkey index is not 0
		for (i=0; i<8; i++)
		{
			RTMP_IO_WRITE8(pAd, offset+i, IVEIV[i]);
		}

		// 2. WCID Attribute UDF:3, BSSIdx:3, Alg:3, Keytable:use share key, BSSIdx is 0
		WCIDAttri = (pAd->SharedKey[BSS0][KeyIdx].CipherAlg<<1)|PAIRWISEKEYTABLE;
		offset = MAC_WCID_ATTRIBUTE_BASE + (BSS0Mcast_WCID* HW_WCID_ATTRI_SIZE);
		RTMP_IO_WRITE32(pAd, offset, WCIDAttri);
		
	}

*/

   if (pAd->SharedKey[apidx][KeyIdx].Type == GROUP_KEY)
	{
		// 802.1x port control
		pAd->StaCfg.PortSecured = WPA_802_1X_PORT_SECURED;
		DBGPRINT(RT_DEBUG_TRACE,("!!WPA_802_1X_PORT_SECURED!!\n"));

	}

	return (NDIS_STATUS_SUCCESS);
}
#endif 

BOOLEAN RTMPCheckStrPrintAble(
    IN  CHAR *pInPutStr, 
    IN  UCHAR strLen)
{
    UCHAR i=0;
    
    for (i=0; i<strLen; i++)
    {
        if ((pInPutStr[i] < 0x21) ||
            (pInPutStr[i] > 0x7E))
            return FALSE;
    }
    
    return TRUE;
}

/*
	========================================================================
	
	Routine Description:
		Remove WPA Key process

	Arguments:
		pAd 					Pointer to our adapter
		pBuf							Pointer to the where the key stored

	Return Value:
		NDIS_SUCCESS					Add key successfully

	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/



#if 0 // No supply currently for STA
/*
	========================================================================
	
	Routine Description:
		Construct and indicate WPA2 Media Specific Status

	Arguments:
		pAdapter	Pointer to our adapter

	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	Note: only for STATION
		
	========================================================================
*/
VOID	RTMPIndicateWPA2Status(
	IN	PRTMP_ADAPTER	pAd)
{
	UCHAR 	apidx=0;

	struct
	{
		NDIS_802_11_STATUS_TYPE 			Status;
		NDIS_802_11_PMKID_CANDIDATE_LIST	List;
	}	Candidate;

	Candidate.Status = Ndis802_11StatusType_PMKID_CandidateList;
	Candidate.List.Version = 1;
	// This might need to be fixed to accomadate with current saved PKMIDs
	Candidate.List.NumCandidates = 1;
	NdisMoveMemory(&Candidate.List.CandidateList[0].BSSID, pAd->ApCfg.MBSSID[apidx].Bssid, 6);
	Candidate.List.CandidateList[0].Flags = 0;
	NdisMIndicateStatus(pAd->AdapterHandle, NDIS_STATUS_MEDIA_SPECIFIC_INDICATION, &Candidate, sizeof(Candidate));	
	DBGPRINT(RT_DEBUG_TRACE,("RTMPIndicateWPA2Status\n"));
}
#endif

#if 0
VOID	RTMPAddWcidCipher(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_802_11_KEY	pKey,
	IN	UCHAR	CipherAlg,
	IN    UCHAR  Wcid)
{
	PUCHAR		pTxMic, pRxMic;
	BOOLEAN 	bKeyRSC, bAuthenticator;		// indicate the receive  SC set by KeyRSC value.
	UCHAR	i;
	ULONG		WCIDAttri;
	ULONG 	offset;
	UCHAR	KeyIdx, IVEIV[8];
	ULONG		Value;
	
	DBGPRINT(RT_DEBUG_TRACE, ("RTMPAddWcidCipher to Mactable[%d]==> \n", Wcid));
	
	// Bit 29 of Add-key KeyRSC
	bKeyRSC 	   = (pKey->KeyIndex & 0x20000000) ? TRUE : FALSE;
	// Bit 28 of Add-key Authenticator
	bAuthenticator = (pKey->KeyIndex & 0x10000000) ? TRUE : FALSE;
	KeyIdx = (UCHAR)pKey->KeyIndex&0xff;
	
	pAd->MacTab.Content[Wcid].PairwiseKey.KeyLen = (UCHAR)pKey->KeyLength;

	if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPANone)
	{
		// for WPA-None Tx, Rx MIC is the same
		pTxMic = (PUCHAR) (&pKey->KeyMaterial) + 16;
		pRxMic = pTxMic;
	}
	else if (bAuthenticator == TRUE)
	{
		pTxMic = (PUCHAR) (&pKey->KeyMaterial) + 16;
		pRxMic = (PUCHAR) (&pKey->KeyMaterial) + 24;
	}
	else
	{
		pRxMic = (PUCHAR) (&pKey->KeyMaterial) + 16;
		pTxMic = (PUCHAR) (&pKey->KeyMaterial) + 24;
	}
	
	// 2. Record Security Key.
	NdisMoveMemory(pAd->MacTab.Content[Wcid].PairwiseKey.Key, &pKey->KeyMaterial, 16);
	if (pKey->KeyLength == LEN_TKIP_KEY)
	{
		// Only Key lenth equal to TKIP key have these
		NdisMoveMemory(pAd->MacTab.Content[Wcid].PairwiseKey.RxMic, pRxMic, 8);
		NdisMoveMemory(pAd->MacTab.Content[Wcid].PairwiseKey.TxMic, pTxMic, 8);
	}
	// 3. Check RxTsc. And used to init to ASIC IV. 
	if (bKeyRSC == TRUE)
		NdisMoveMemory(pAd->MacTab.Content[Wcid].PairwiseKey.RxTsc, &pKey->KeyRSC, 6);		   
	else
		NdisZeroMemory(pAd->MacTab.Content[Wcid].PairwiseKey.RxTsc, 6);	   

	// 4. Init TxTsc to one based on WiFi WPA specs
	pAd->MacTab.Content[Wcid].PairwiseKey.TxTsc[0] = 1;
	pAd->MacTab.Content[Wcid].PairwiseKey.TxTsc[1] = 0;
	pAd->MacTab.Content[Wcid].PairwiseKey.TxTsc[2] = 0;
	pAd->MacTab.Content[Wcid].PairwiseKey.TxTsc[3] = 0;
	pAd->MacTab.Content[Wcid].PairwiseKey.TxTsc[4] = 0;
	pAd->MacTab.Content[Wcid].PairwiseKey.TxTsc[5] = 0;
	

	offset = PAIRWISE_KEY_TABLE_BASE + (Wcid * HW_KEY_ENTRY_SIZE);
	for (i=0; i<pKey->KeyLength; i++)
	{
		RTMP_IO_WRITE8(pAd, offset+i, pKey->KeyMaterial[i]);
	}
	
	offset = PAIRWISE_KEY_TABLE_BASE + (Wcid * HW_KEY_ENTRY_SIZE) + 0x10;
	for (i=0; i<8; i++)
	{
		RTMP_IO_WRITE8(pAd, offset+i, *(pTxMic+i));
	}
	offset = PAIRWISE_KEY_TABLE_BASE + (Wcid * HW_KEY_ENTRY_SIZE) + 0x18;
	for (i=0; i<8; i++)
	{
		RTMP_IO_WRITE8(pAd, offset+i, *(pRxMic+i));
	}

	offset = MAC_IVEIV_TABLE_BASE + (Wcid * HW_IVEIV_ENTRY_SIZE);
	NdisZeroMemory(IVEIV, 8);
	// IV/EIV
	if ((CipherAlg == CIPHER_TKIP) || (CipherAlg == CIPHER_TKIP_NO_MIC) || (CipherAlg == CIPHER_AES))
	{
		IVEIV[3] = 0x20;	// Eiv bit on. keyid always 0 for pairwise key
	}
	// default key idx needs to set.   in TKIP/AES KeyIdx = 0 , WEP KeyIdx is default tx key.  
	{
		IVEIV[3] |= (KeyIdx<< 6);	
	}
	for (i=0; i<8; i++)
	{
		RTMP_IO_WRITE8(pAd, offset+i, IVEIV[i]);
	}

	// WCID Attribute UDF:3, BSSIdx:3, Alg:3,  BSSIdx is 0
	//always use pairwise key table. And softap always use BSSIdx=0 
	WCIDAttri = (CipherAlg<<1)|PAIRWISEKEYTABLE;
	offset = MAC_WCID_ATTRIBUTE_BASE + (Wcid* HW_WCID_ATTRI_SIZE);
	RTMP_IO_WRITE32(pAd, offset, WCIDAttri);
	RTMP_IO_READ32(pAd, offset, &Value);
	DBGPRINT(RT_DEBUG_TRACE, ("Wcid[%d] WCIDAttri : offset = %x, WCIDAttri = %x\n", Wcid, offset, WCIDAttri));
	
	DBGPRINT(RT_DEBUG_TRACE,("AddWcid[%d]: Alg=%s, KeyLength = %d\n",  Wcid, CipherName[CipherAlg], pKey->KeyLength));
	DBGPRINT(RT_DEBUG_TRACE,("	pKey->KeyIndex = %x\n", pKey->KeyIndex));
	DBGPRINT(RT_DEBUG_TRACE,("	Key =\n"));
	for (i=0; i<pKey->KeyLength; i++)
		DBGPRINT_RAW(RT_DEBUG_TRACE,(" %x:", pKey->KeyMaterial[i]));
	DBGPRINT(RT_DEBUG_TRACE,("	 \n"));
	DBGPRINT(RT_DEBUG_TRACE,("	TxMIC  = %02x:%02x:%02x:%02x:...\n", pTxMic[0],pTxMic[1],pTxMic[2],pTxMic[3]));
	DBGPRINT(RT_DEBUG_TRACE,("RxMic = %02x:%02x:%02x:%02x:...\n", pRxMic[0],pRxMic[1],pRxMic[2],pRxMic[3]));

}

#endif /* 0 */
/*
	========================================================================
	
	Routine Description:
	 	As STA's BSSID is a WC too, it uses shared key table.
	 	This function write correct unicast TX key to ASIC WCID.
 	 	And we still make a copy in our MacTab.Content[BSSID_WCID].PairwiseKey.
		Caller guarantee TKIP/AES always has keyidx = 0. (pairwise key)
		Caller guarantee WEP calls this function when set Txkey,  default key index=0~3.
		
	Arguments:
		pAd 					Pointer to our adapter
		pKey							Pointer to the where the key stored

	Return Value:
		NDIS_SUCCESS					Add key successfully

	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
#if 0	// removed by AlbertY
VOID	RTMPAddBSSIDCipher(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR	Aid,
	IN	PNDIS_802_11_KEY	pKey,
	IN  UCHAR   CipherAlg)
{
	PUCHAR		pTxMic, pRxMic;
	BOOLEAN 	bKeyRSC, bAuthenticator;		// indicate the receive  SC set by KeyRSC value.
	UCHAR	    i;
	ULONG		WCIDAttri;
	ULONG 	offset;
	UCHAR	KeyIdx, IVEIV[8];
	ULONG		Value;
	UCHAR		apidx;
	
	DBGPRINT(RT_DEBUG_TRACE, ("RTMPAddBSSIDCipher==> Aid = %d\n",Aid));
	
	// Bit 29 of Add-key KeyRSC
	bKeyRSC 	   = (pKey->KeyIndex & 0x20000000) ? TRUE : FALSE;
	// Bit 28 of Add-key Authenticator
	bAuthenticator = (pKey->KeyIndex & 0x10000000) ? TRUE : FALSE;
	KeyIdx = (UCHAR)pKey->KeyIndex&0xff;
	
	if (KeyIdx > 4)
		return;
   	
	apidx = pAd->MacTab.Content[Aid].apidx;


	if (CipherAlg == CIPHER_TKIP)
	{	if (pAd->ApCfg.MBSSID[apidx].AuthMode == Ndis802_11AuthModeWPANone)
	{	
			// for WPA-None Tx, Rx MIC is the same
			pTxMic = (PUCHAR) (&pKey->KeyMaterial) + 16;
			pRxMic = pTxMic;
	}
		else if (bAuthenticator == TRUE)
	{	
			pTxMic = (PUCHAR) (&pKey->KeyMaterial) + 16;
			pRxMic = (PUCHAR) (&pKey->KeyMaterial) + 24;
	}
		else
	{
			pRxMic = (PUCHAR) (&pKey->KeyMaterial) + 16;
			pTxMic = (PUCHAR) (&pKey->KeyMaterial) + 24;
		}
		
		offset = PAIRWISE_KEY_TABLE_BASE + (Aid * HW_KEY_ENTRY_SIZE) + 0x10;
		for (i=0; i<8; )
		{
			Value = *(pTxMic+i);
			Value += (*(pTxMic+i+1)<<8);
			Value += (*(pTxMic+i+2)<<16);
			Value += (*(pTxMic+i+3)<<24);
			RTMP_IO_WRITE32(pAd, offset+i, Value);
			i+=4;
		}
		offset = PAIRWISE_KEY_TABLE_BASE + (Aid * HW_KEY_ENTRY_SIZE) + 0x18;
		for (i=0; i<8; )
		{
			Value = *(pRxMic+i);
			Value += (*(pRxMic+i+1)<<8);
			Value += (*(pRxMic+i+2)<<16);
			Value += (*(pRxMic+i+3)<<24);
			RTMP_IO_WRITE32(pAd, offset+i, Value);
			i+=4;
	}
		// Only Key lenth equal to TKIP key have these
		NdisMoveMemory(pAd->MacTab.Content[Aid].PairwiseKey.RxMic, pRxMic, 8);
		NdisMoveMemory(pAd->MacTab.Content[Aid].PairwiseKey.TxMic, pTxMic, 8);
		
		DBGPRINT(RT_DEBUG_TRACE,("	TxMIC = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", pTxMic[0],pTxMic[1],pTxMic[2],pTxMic[3], pTxMic[4],pTxMic[5],pTxMic[6],pTxMic[7]));
		DBGPRINT(RT_DEBUG_TRACE,("	RxMic = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", pRxMic[0],pRxMic[1],pRxMic[2],pRxMic[3], pRxMic[4],pRxMic[5],pRxMic[6],pRxMic[7]));

	}
	// 2. Record Security Key.
	pAd->MacTab.Content[BSSID_WCID].PairwiseKey.KeyLen= (UCHAR)pKey->KeyLength;
	NdisMoveMemory(pAd->MacTab.Content[Aid].PairwiseKey.Key, &pKey->KeyMaterial, pKey->KeyLength);
	pAd->MacTab.Content[Aid].PairwiseKey.CipherAlg = CipherAlg;
	// 3. Check RxTsc. And used to init to ASIC IV. 
	if (bKeyRSC == TRUE)
		NdisMoveMemory(pAd->MacTab.Content[Aid].PairwiseKey.RxTsc, &pKey->KeyRSC, 6);		   
	else
		NdisZeroMemory(pAd->MacTab.Content[Aid].PairwiseKey.RxTsc, 6);	   

	// 4. Init TxTsc to one based on WiFi WPA specs
	pAd->MacTab.Content[Aid].PairwiseKey.TxTsc[0] = 1;
	pAd->MacTab.Content[Aid].PairwiseKey.TxTsc[1] = 0;
	pAd->MacTab.Content[Aid].PairwiseKey.TxTsc[2] = 0;
	pAd->MacTab.Content[Aid].PairwiseKey.TxTsc[3] = 0;
	pAd->MacTab.Content[Aid].PairwiseKey.TxTsc[4] = 0;
	pAd->MacTab.Content[Aid].PairwiseKey.TxTsc[5] = 0;
	
	offset = PAIRWISE_KEY_TABLE_BASE + (Aid * HW_KEY_ENTRY_SIZE);
	for (i=0; i<pKey->KeyLength; i++)
	{
		RTMP_IO_WRITE8(pAd, offset+i, pKey->KeyMaterial[i]);
	}
	
	offset = SHARED_KEY_TABLE_BASE + (KeyIdx * HW_KEY_ENTRY_SIZE);	
	for(i = 0;i<pKey->KeyLength; i++)
	{
		RTMP_IO_WRITE8(pAd, offset+i, pKey->KeyMaterial[i]);
	}
	
	offset = MAC_IVEIV_TABLE_BASE + (Aid * HW_IVEIV_ENTRY_SIZE);
	NdisZeroMemory(IVEIV, 8);
	Value = 0;

	// IV/EIV
	if ((CipherAlg == CIPHER_TKIP) || (CipherAlg == CIPHER_TKIP_NO_MIC) || (CipherAlg == CIPHER_AES))
	{
		IVEIV[3] = 0x20;	// Eiv bit on. keyid always 0 for pairwise key
		Value = 0x20000000;
	}
	else // default key idx needs to set.   in TKIP/AES KeyIdx = 0 , WEP KeyIdx is default tx key.  
	{
		IVEIV[3] |= (KeyIdx<< 6);	
		Value += (KeyIdx<< 30);
	}
	RTMP_IO_WRITE32(pAd, offset, Value);
	Value = 0;
	RTMP_IO_WRITE32(pAd, (offset+4), Value);

	// WCID Attribute UDF:3, BSSIdx:3, Alg:3, Keytable:1=PAIRWISE KEY, BSSIdx is 0
	if ((CipherAlg == CIPHER_TKIP) || (CipherAlg == CIPHER_TKIP_NO_MIC) || (CipherAlg == CIPHER_AES))
		WCIDAttri = (CipherAlg<<1)|SHAREDKEYTABLE;
	else
		WCIDAttri = (CipherAlg<<1)|SHAREDKEYTABLE;
	
	offset = MAC_WCID_ATTRIBUTE_BASE + (Aid* HW_WCID_ATTRI_SIZE);
	RTMP_IO_WRITE32(pAd, offset, WCIDAttri);
	RTMP_IO_READ32(pAd, offset, &Value);
	DBGPRINT(RT_DEBUG_TRACE, ("BSSID_WCID : offset = %x, WCIDAttri = %x\n", offset, WCIDAttri));
	
	DBGPRINT(RT_DEBUG_TRACE,("AddBSSIDasWCIDEntry: Alg=%s, KeyLength = %d\n", CipherName[CipherAlg], pKey->KeyLength));
	DBGPRINT(RT_DEBUG_TRACE,("Key [idx=%x] [KeyLen = %d]\n", pKey->KeyIndex, pKey->KeyLength));
	for (i=0; i<pKey->KeyLength; i++)
		DBGPRINT_RAW(RT_DEBUG_TRACE,(" %x:", pKey->KeyMaterial[i]));
	DBGPRINT(RT_DEBUG_TRACE,("	 \n"));
}
#endif
/*
	========================================================================
	Routine Description:
		Change NIC PHY mode. Re-association may be necessary. possible settings
		include - PHY_11B, PHY_11BG_MIXED, PHY_11A, and PHY_11ABG_MIXED 

	Arguments:
		pAd - Pointer to our adapter
		phymode  - 

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
VOID	RTMPSetPhyMode(
	IN	PRTMP_ADAPTER	pAd,
	IN	ULONG phymode)
{
	INT i;

	// the selected phymode must be supported by the RF IC encoded in E2PROM

	// if no change, do nothing
	/* bug fix
	if (pAd->CommonCfg.PhyMode == phymode)
		return;
    */
	pAd->CommonCfg.PhyMode = (UCHAR)phymode;

	DBGPRINT(RT_DEBUG_TRACE,("RTMPSetPhyMode(=%d)HTPhyMode.MODE=%d, channel=%d \n", pAd->CommonCfg.PhyMode, pAd->CommonCfg.HTPhyMode.field.MODE, pAd->CommonCfg.Channel));
	
	BuildChannelList(pAd);

	// sanity check user setting
	for (i = 0; i < pAd->ChannelListNum; i++)
	{
		if (pAd->CommonCfg.Channel == pAd->ChannelList[i].Channel)
			break;
	}

	if (i == pAd->ChannelListNum)
	{
		pAd->CommonCfg.Channel = FirstChannel(pAd);
		DBGPRINT(RT_DEBUG_ERROR, ("RTMPSetPhyMode: channel is out of range, use first channel=%d \n", pAd->CommonCfg.Channel));
	}
	
	NdisZeroMemory(pAd->CommonCfg.SupRate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(pAd->CommonCfg.ExtRate, MAX_LEN_OF_SUPPORTED_RATES);
	NdisZeroMemory(pAd->CommonCfg.DesireRate, MAX_LEN_OF_SUPPORTED_RATES);
	switch (phymode) {
		case PHY_11B:
			pAd->CommonCfg.SupRate[0]  = 0x82;	  // 1 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[1]  = 0x84;	  // 2 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[2]  = 0x8B;	  // 5.5 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[3]  = 0x96;	  // 11 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRateLen  = 4;
			pAd->CommonCfg.ExtRateLen  = 0;
			pAd->CommonCfg.DesireRate[0]  = 2;	   // 1 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[1]  = 4;	   // 2 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[2]  = 11;    // 5.5 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[3]  = 22;    // 11 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.HTPhyMode.field.MODE = MODE_CCK; // This MODE is only FYI. not use
			break;

		case PHY_11G:
		case PHY_11N:
		case PHY_11BG_MIXED:
		case PHY_11ABG_MIXED:
		case PHY_11ABGN_MIXED:
		case PHY_11BGN_MIXED:
		case PHY_11GN_MIXED:
			pAd->CommonCfg.SupRate[0]  = 0x82;	  // 1 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[1]  = 0x84;	  // 2 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[2]  = 0x8B;	  // 5.5 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[3]  = 0x96;	  // 11 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[4]  = 0x12;	  // 9 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.SupRate[5]  = 0x24;	  // 18 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.SupRate[6]  = 0x48;	  // 36 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.SupRate[7]  = 0x6c;	  // 54 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.SupRateLen  = 8;
			pAd->CommonCfg.ExtRate[0]  = 0x0C;	  // 6 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.ExtRate[1]  = 0x18;	  // 12 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.ExtRate[2]  = 0x30;	  // 24 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.ExtRate[3]  = 0x60;	  // 48 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.ExtRateLen  = 4;
			pAd->CommonCfg.DesireRate[0]  = 2;	   // 1 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[1]  = 4;	   // 2 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[2]  = 11;    // 5.5 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[3]  = 22;    // 11 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[4]  = 12;    // 6 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[5]  = 18;    // 9 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[6]  = 24;    // 12 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[7]  = 36;    // 18 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[8]  = 48;    // 24 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[9]  = 72;    // 36 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[10] = 96;    // 48 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[11] = 108;   // 54 mbps, in units of 0.5 Mbps
			break;

		case PHY_11A:
		case PHY_11AN_MIXED:
		case PHY_11AGN_MIXED:
			pAd->CommonCfg.SupRate[0]  = 0x8C;	  // 6 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[1]  = 0x12;	  // 9 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.SupRate[2]  = 0x98;	  // 12 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[3]  = 0x24;	  // 18 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.SupRate[4]  = 0xb0;	  // 24 mbps, in units of 0.5 Mbps, basic rate
			pAd->CommonCfg.SupRate[5]  = 0x48;	  // 36 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.SupRate[6]  = 0x60;	  // 48 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.SupRate[7]  = 0x6c;	  // 54 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.SupRateLen  = 8;
			pAd->CommonCfg.ExtRateLen  = 0;
			pAd->CommonCfg.DesireRate[0]  = 12;    // 6 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[1]  = 18;    // 9 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[2]  = 24;    // 12 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[3]  = 36;    // 18 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[4]  = 48;    // 24 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[5]  = 72;    // 36 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[6]  = 96;    // 48 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.DesireRate[7]  = 108;   // 54 mbps, in units of 0.5 Mbps
			pAd->CommonCfg.HTPhyMode.field.MODE = MODE_OFDM; // This MODE is only FYI. not use
			break;

		default:
			break;
	}


	MlmeUpdateTxRates(pAd, FALSE);
	MlmeUpdateHtTxRates(pAd, 0xff);
	//AsicSetSlotTime(pAd, TRUE); //FALSE);

	pAd->CommonCfg.BandState = UNKNOWN_BAND;	
//	MakeIbssBeacon(pAd);	// supported rates may change
}



/*
	========================================================================
	Routine Description:
		Caller ensures we has 802.11n support.
		Calls at setting HT from AP/STASetinformation

	Arguments:
		pAd - Pointer to our adapter
		phymode  - 

	========================================================================
*/
VOID	RTMPSetHT(
	IN	PRTMP_ADAPTER	pAd,
	IN	OID_SET_HT_PHYMODE *pHTPhyMode)
{

	//ULONG	*pmcs;
	ULONG	Value;
	UCHAR	BBPValue = 0;
	UCHAR	BBP3Value = 0;
	UCHAR	TxStream = pAd->CommonCfg.TxStream;
	UCHAR	RxStream = pAd->CommonCfg.RxStream;
#ifdef CONFIG_AP_SUPPORT
	UCHAR	apidx;
#endif // CONFIG_AP_SUPPORT //

	// Don't zero supportedHyPhy structure.
	RTMPZeroMemory(&pAd->CommonCfg.HtCapability, sizeof(pAd->CommonCfg.HtCapability));
	RTMPZeroMemory(&pAd->CommonCfg.AddHTInfo, sizeof(pAd->CommonCfg.AddHTInfo));
	RTMPZeroMemory(&pAd->CommonCfg.NewExtChanOffset, sizeof(pAd->CommonCfg.NewExtChanOffset));
	RTMPZeroMemory(&pAd->CommonCfg.DesiredHtPhy, sizeof(pAd->CommonCfg.DesiredHtPhy));

   	if (pAd->CommonCfg.bRdg)
	{
		pAd->CommonCfg.HtCapability.ExtHtCapInfo.PlusHTC = 1;
		pAd->CommonCfg.HtCapability.ExtHtCapInfo.RDGSupport = 1;
	}
	else
	{
		pAd->CommonCfg.HtCapability.ExtHtCapInfo.PlusHTC = 0;
		pAd->CommonCfg.HtCapability.ExtHtCapInfo.RDGSupport = 0;
	}

#if 0
	// pAd->CommonCfg.BASize is read from Registry.
	if (pAd->CommonCfg.BACapability.field.RxBAWinLimit & 0xc0)  //0x40 = 64
	{
	pAd->CommonCfg.HtCapability.HtCapParm.MaxRAmpduFactor = 3;
	pAd->CommonCfg.DesiredHtPhy.MaxRAmpduFactor = 3;
		//  Our BaSixe in TxWI definition is 6 bit. So Max setting is 63.
	}
	else if (pAd->CommonCfg.BACapability.field.RxBAWinLimit & 0x20)  // 0x20 = 32
	{
		pAd->CommonCfg.HtCapability.HtCapParm.MaxRAmpduFactor = 2;
		pAd->CommonCfg.DesiredHtPhy.MaxRAmpduFactor = 2;
	}
	else if (pAd->CommonCfg.BACapability.field.RxBAWinLimit & 0x10) // 0x10 = 16
	{
		pAd->CommonCfg.HtCapability.HtCapParm.MaxRAmpduFactor = 1;
		pAd->CommonCfg.DesiredHtPhy.MaxRAmpduFactor = 1;
	}
	else
	{
		pAd->CommonCfg.HtCapability.HtCapParm.MaxRAmpduFactor = 0;
		pAd->CommonCfg.DesiredHtPhy.MaxRAmpduFactor = 0;
	}
#else
	pAd->CommonCfg.HtCapability.HtCapParm.MaxRAmpduFactor = 3;
	pAd->CommonCfg.DesiredHtPhy.MaxRAmpduFactor = 3;
#endif



	DBGPRINT(RT_DEBUG_TRACE, ("RTMPSetHT -(RxBAWinLimit= %d), (MaxRAmpduFactor=%d)...\n", pAd->CommonCfg.BACapability.field.RxBAWinLimit, pAd->CommonCfg.HtCapability.HtCapParm.MaxRAmpduFactor));
	pAd->CommonCfg.DesiredHtPhy.bHtEnable = TRUE;
	// Mimo power save, A-MSDU size, 
	pAd->CommonCfg.DesiredHtPhy.AmsduEnable = (USHORT)pAd->CommonCfg.BACapability.field.AmsduEnable;
	pAd->CommonCfg.DesiredHtPhy.AmsduSize = (UCHAR)pAd->CommonCfg.BACapability.field.AmsduSize;
	pAd->CommonCfg.DesiredHtPhy.MimoPs = (UCHAR)pAd->CommonCfg.BACapability.field.MMPSmode;
	pAd->CommonCfg.DesiredHtPhy.MpduDensity = (UCHAR)pAd->CommonCfg.BACapability.field.MpduDensity;

	pAd->CommonCfg.HtCapability.HtCapInfo.AMsduSize = (USHORT)pAd->CommonCfg.BACapability.field.AmsduSize;
	pAd->CommonCfg.HtCapability.HtCapInfo.MimoPs = (USHORT)pAd->CommonCfg.BACapability.field.MMPSmode;
	pAd->CommonCfg.HtCapability.HtCapParm.MpduDensity = (UCHAR)pAd->CommonCfg.BACapability.field.MpduDensity;
	
	DBGPRINT(RT_DEBUG_TRACE, ("RTMPSetHT-----> AMsduSize = %d (MimoPs = %d) (RxBAWinLimit = %d)  \n", pAd->CommonCfg.DesiredHtPhy.AmsduSize, pAd->CommonCfg.HtCapability.HtCapInfo.MimoPs, pAd->CommonCfg.BACapability.field.RxBAWinLimit));
	DBGPRINT(RT_DEBUG_TRACE, (" RTMPSetHT<---BW = %d, STBC = %d, SHORTGI = %d,>MCS = %d,ExtOffset = %d  \n", pHTPhyMode->BW,pHTPhyMode->STBC,pHTPhyMode->SHORTGI,pHTPhyMode->MCS,pHTPhyMode->ExtOffset));
	if(pHTPhyMode->HtMode == HTMODE_GF)
	{
		pAd->CommonCfg.HtCapability.HtCapInfo.GF = 1;
		pAd->CommonCfg.DesiredHtPhy.GF = 1;
	}
	else
		pAd->CommonCfg.DesiredHtPhy.GF = 0;
	
	// Support in 40-MHz
	//pAd->CommonCfg.DesiredHtPhy.MCSSet[4]= 0x1;
	 
	// Set desired rate according to GUI setting,  SUpported 
	switch (TxStream /*pHTPhyMode->TransmitNo*/)
	{
		case 1:
			if (pHTPhyMode->MCS == MCS_AUTO)
			{
				pAd->CommonCfg.DesiredHtPhy.MCSSet[0]= 0xff;
				pAd->CommonCfg.DesiredHtPhy.MCSSet[1]= 0x00;
			}
			else
			{
				pAd->CommonCfg.DesiredHtPhy.MCSSet[0]= 1<<pHTPhyMode->MCS;
				pAd->CommonCfg.DesiredHtPhy.MCSSet[1]= 0x00;
			}
			//pAd->CommonCfg.HtCapability.MCSSet[0] =  0xff;
			//pAd->CommonCfg.HtCapability.MCSSet[1] =  0x00;
			break;

		case 2:
			if (pHTPhyMode->MCS == MCS_AUTO)
			{
				pAd->CommonCfg.DesiredHtPhy.MCSSet[0]= 0xff;
				pAd->CommonCfg.DesiredHtPhy.MCSSet[1]= 0xff;
			}
			else
			{
				ULONG mode;
				//mcs = 1<<(pHTPhyMode->MCS);
				mode = pHTPhyMode->MCS / 8;
				if (mode < 2)
					pAd->CommonCfg.DesiredHtPhy.MCSSet[mode] = (1 << (pHTPhyMode->MCS - mode * 8));
//				pmcs = (ULONG*)&(pAd->CommonCfg.DesiredHtPhy.MCSSet[0]);
//		*pmcs= 1<<(pHTPhyMode->MCS);
			}
			//pAd->CommonCfg.HtCapability.MCSSet[0] =  0xff;
			//pAd->CommonCfg.HtCapability.MCSSet[1] =  0xff;
			break;
	}

	// Set Rx MCSSet
	switch (RxStream)
	{
		case 1:			
			pAd->CommonCfg.HtCapability.MCSSet[0] =  0xff;
			pAd->CommonCfg.HtCapability.MCSSet[1] =  0x00;
			break;

		case 2:
		case 3:				
			pAd->CommonCfg.HtCapability.MCSSet[0] =  0xff;
			pAd->CommonCfg.HtCapability.MCSSet[1] =  0xff;
			break;
	}


	if (pAd->CommonCfg.bForty_Mhz_Intolerant && (pAd->CommonCfg.Channel <= 14) && (pHTPhyMode->BW == BW_40) )
	{
		pHTPhyMode->BW = BW_20;
		pAd->CommonCfg.HtCapability.HtCapInfo.Forty_Mhz_Intolerant = 1;
	}

	if(pHTPhyMode->BW == BW_40)
	{
        pAd->CommonCfg.HtCapability.MCSSet[4] = 0x1; // MCS 32
#ifdef CONFIG_AP_SUPPORT        
        pAd->CommonCfg.DesiredHtPhy.MCSSet[4] = 0x1;
#endif // CONFIG_AP_SUPPORT //
		pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth = 1;
        if (pAd->CommonCfg.Channel <= 14) {
           pAd->CommonCfg.HtCapability.HtCapInfo.CCKmodein40 = 1;
        }
		pAd->CommonCfg.DesiredHtPhy.ChannelWidth = 1;
		pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth = 1;
		pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset = (pHTPhyMode->ExtOffset == EXTCHA_BELOW)? (EXTCHA_BELOW): EXTCHA_ABOVE;
		// Set Regsiter for extension channel position.
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &BBP3Value);
		if ((pHTPhyMode->ExtOffset == EXTCHA_BELOW))
		{
			Value |= 0x1;
			BBP3Value |= (0x20);
			RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);
		}
		else if ((pHTPhyMode->ExtOffset == EXTCHA_ABOVE))
		{
			Value &= 0xfe;
			BBP3Value &= (~0x20);
			RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);
		} 
		// Turn on BBP 40MHz mode now only as AP . 
		// Sta can turn on BBP 40MHz after connection with 40MHz AP. Sta only broadcast 40MHz capability before connection.
		if ((pAd->OpMode == OPMODE_AP) || ADHOC_ON(pAd))
		{
			RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R4, &BBPValue);
			BBPValue &= (~0x18);
			BBPValue |= 0x10;
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R4, BBPValue);

			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, BBP3Value);
			pAd->CommonCfg.BBPCurrentBW = BW_40;
		}
	}
	else
	{
		pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth = 0;
		pAd->CommonCfg.DesiredHtPhy.ChannelWidth = 0;
		pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth = 0;
		pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset = EXTCHA_NONE;
		pAd->CommonCfg.CentralChannel = pAd->CommonCfg.Channel;
		// Turn on BBP 20MHz mode by request here.
		{
			RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R4, &BBPValue);
			BBPValue &= (~0x18);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R4, BBPValue);
			pAd->CommonCfg.BBPCurrentBW = BW_20;
		}
	}
		
	if(pHTPhyMode->STBC == STBC_USE)
	{
		pAd->CommonCfg.HtCapability.HtCapInfo.TxSTBC = 1;
		pAd->CommonCfg.DesiredHtPhy.TxSTBC = 1;
		pAd->CommonCfg.HtCapability.HtCapInfo.RxSTBC = 1;
		pAd->CommonCfg.DesiredHtPhy.RxSTBC = 1;
	}
	else
	{
		pAd->CommonCfg.DesiredHtPhy.TxSTBC = 0;
		pAd->CommonCfg.DesiredHtPhy.RxSTBC = 0;
	}

	if(pHTPhyMode->SHORTGI == GI_400)
	{
		pAd->CommonCfg.HtCapability.HtCapInfo.ShortGIfor20 = 1;
		pAd->CommonCfg.HtCapability.HtCapInfo.ShortGIfor40 = 1;
		pAd->CommonCfg.DesiredHtPhy.ShortGIfor20 = 1;
		pAd->CommonCfg.DesiredHtPhy.ShortGIfor40 = 1;
	}
	else
	{
		pAd->CommonCfg.HtCapability.HtCapInfo.ShortGIfor20 = 0;
		pAd->CommonCfg.HtCapability.HtCapInfo.ShortGIfor40 = 0;
		pAd->CommonCfg.DesiredHtPhy.ShortGIfor20 = 0;
		pAd->CommonCfg.DesiredHtPhy.ShortGIfor40 = 0;
	}
	
	// We support link adaptation for unsolicit MCS feedback, set to 2.
	pAd->CommonCfg.HtCapability.ExtHtCapInfo.MCSFeedback = MCSFBK_NONE; //MCSFBK_UNSOLICIT;
	pAd->CommonCfg.AddHTInfo.ControlChan = pAd->CommonCfg.Channel;
	// 1, the extension channel above the control channel. 
	
	// Only for updating Registry transmit setting. However, real transmit setting is from OID setting or via negociation.
	pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = pHTPhyMode->ExtOffset;
	pAd->CommonCfg.RegTransmitSetting.field.BW = pHTPhyMode->BW;
	pAd->CommonCfg.RegTransmitSetting.field.ShortGI = pHTPhyMode->SHORTGI;
	pAd->CommonCfg.RegTransmitSetting.field.STBC = pHTPhyMode->STBC;
	pAd->CommonCfg.RegTransmitSetting.field.HTMODE = pHTPhyMode->HtMode;
	pAd->CommonCfg.RegTransmitSetting.field.MCS = pHTPhyMode->MCS;
	
#ifdef CONFIG_AP_SUPPORT	
	for (apidx=0; apidx<pAd->ApCfg.BssidNum; apidx++)
	{	
		pAd->ApCfg.MBSSID[apidx].bWmmCapable = TRUE;
	}
#endif // CONFIG_AP_SUPPORT //

	{
		// EDCA parameters used for AP's own transmission
		if (pAd->CommonCfg.APEdcaParm.bValid == FALSE)
		{
			pAd->CommonCfg.APEdcaParm.bValid = TRUE;
			pAd->CommonCfg.APEdcaParm.Aifsn[0] = 3;
			pAd->CommonCfg.APEdcaParm.Aifsn[1] = 7;
			pAd->CommonCfg.APEdcaParm.Aifsn[2] = 1;
			pAd->CommonCfg.APEdcaParm.Aifsn[3] = 1;
	
			pAd->CommonCfg.APEdcaParm.Cwmin[0] = 4;
			pAd->CommonCfg.APEdcaParm.Cwmin[1] = 4;
			pAd->CommonCfg.APEdcaParm.Cwmin[2] = 3;
			pAd->CommonCfg.APEdcaParm.Cwmin[3] = 2;

			pAd->CommonCfg.APEdcaParm.Cwmax[0] = 6;
			pAd->CommonCfg.APEdcaParm.Cwmax[1] = 10;
			pAd->CommonCfg.APEdcaParm.Cwmax[2] = 4;
			pAd->CommonCfg.APEdcaParm.Cwmax[3] = 3;

			pAd->CommonCfg.APEdcaParm.Txop[0]  = 0;
			pAd->CommonCfg.APEdcaParm.Txop[1]  = 0;
			pAd->CommonCfg.APEdcaParm.Txop[2]  = 94;	//96;
			pAd->CommonCfg.APEdcaParm.Txop[3]  = 47;	//48;
		}
		AsicSetEdcaParm(pAd, &pAd->CommonCfg.APEdcaParm);
	}
	MlmeUpdateHtTxRates(pAd, 0xff);

	if (pHTPhyMode->MCS != MCS_AUTO)
	{
		pAd->CommonCfg.bAutoTxRateSwitch = FALSE;
		
		pAd->CommonCfg.HTPhyMode.field.STBC		= pHTPhyMode->STBC;
		pAd->CommonCfg.HTPhyMode.field.ShortGI	= pHTPhyMode->SHORTGI;
		pAd->CommonCfg.HTPhyMode.field.MCS		= pHTPhyMode->MCS;
		pAd->CommonCfg.HTPhyMode.field.MODE		= pHTPhyMode->HtMode;
		pAd->CommonCfg.HTPhyMode.field.BW		= pHTPhyMode->BW;

	}
	else
	{
		pAd->CommonCfg.bAutoTxRateSwitch = TRUE;
	}
}
/*
	========================================================================
	Routine Description:
		Update HT IE from our capability.
		
	Arguments:
		Send all HT IE in beacon/probe rsp/assoc rsp/action frame.
		
	
	========================================================================
*/
VOID	RTMPUpdateHTIE(
	IN	RT_HT_CAPABILITY	*pRtHt,
	OUT		HT_CAPABILITY_IE *pHtCapability,
	OUT		ADD_HT_INFO_IE		*pAddHtInfo)
{
	RTMPZeroMemory(pHtCapability, sizeof(HT_CAPABILITY_IE));
	RTMPZeroMemory(pAddHtInfo, sizeof(ADD_HT_INFO_IE));
	
		pHtCapability->HtCapInfo.ChannelWidth = pRtHt->ChannelWidth;
		pHtCapability->HtCapInfo.MimoPs = pRtHt->MimoPs;
		pHtCapability->HtCapInfo.GF = pRtHt->GF;
		pHtCapability->HtCapInfo.ShortGIfor20 = pRtHt->ShortGIfor20;
		pHtCapability->HtCapInfo.ShortGIfor40 = pRtHt->ShortGIfor40;
		pHtCapability->HtCapInfo.TxSTBC = pRtHt->TxSTBC;
		pHtCapability->HtCapInfo.RxSTBC = pRtHt->RxSTBC;
		pHtCapability->HtCapInfo.AMsduSize = pRtHt->AmsduSize;
		pHtCapability->HtCapParm.MaxRAmpduFactor = pRtHt->MaxRAmpduFactor;
		pHtCapability->HtCapParm.MpduDensity = pRtHt->MpduDensity;

		pAddHtInfo->AddHtInfo.ExtChanOffset = pRtHt->ExtChanOffset ;
		pAddHtInfo->AddHtInfo.RecomWidth = pRtHt->RecomWidth;
		pAddHtInfo->AddHtInfo2.OperaionMode = pRtHt->OperaionMode;
		pAddHtInfo->AddHtInfo2.NonGfPresent = pRtHt->NonGfPresent;
		RTMPMoveMemory(pAddHtInfo->MCSSet, pRtHt->MCSSet, 4); // rt2860 only support MCS max=32, no need to copy all 16 uchar.
	
        DBGPRINT(RT_DEBUG_TRACE,("RTMPUpdateHTIE <== \n"));
}

/*
	========================================================================
	Description:
		Add Client security information into ASIC WCID table and IVEIV table.
    Return:
	========================================================================
*/
VOID	RTMPAddWcidAttributeEntry(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			BssIdx,
	IN 	UCHAR		 	KeyIdx,
	IN 	UCHAR		 	CipherAlg,
	IN 	MAC_TABLE_ENTRY *pEntry)
{
	ULONG		WCIDAttri;
	USHORT		offset;
	UCHAR		IVEIV = 0;
	USHORT		Wcid;
#ifdef CONFIG_AP_SUPPORT
	BOOLEAN		IEEE8021X = FALSE;
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	if (BssIdx >= MIN_NET_DEVICE_FOR_APCLI)
	{
		if (pEntry)		
			BssIdx -= MIN_NET_DEVICE_FOR_APCLI;		
		else
		{
			DBGPRINT(RT_DEBUG_WARN, ("RTMPAddWcidAttributeEntry: AP-Client link doesn't need to set Group WCID Attribute. \n"));	
			return;
		}
	}	
	else 
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
	if (BssIdx >= MIN_NET_DEVICE_FOR_WDS)
	{
		if (pEntry)		
			BssIdx = BSS0;		
		else
		{
			DBGPRINT(RT_DEBUG_WARN, ("RTMPAddWcidAttributeEntry: WDS link doesn't need to set Group WCID Attribute. \n"));	
			return;
		}
	}	
	else
#endif // WDS_SUPPORT //
	{
		if (BssIdx >= pAd->ApCfg.BssidNum)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("RTMPAddWcidAttributeEntry: The BSS-index(%d) is out of range for MBSSID link. \n", BssIdx));	
			return;
		}
	}

	// choose wcid number
	if (pEntry)
		Wcid = pEntry->Aid;
	else
		GET_GroupKey_WCID(Wcid, BssIdx);		

	if (BssIdx < pAd->ApCfg.BssidNum)
		IEEE8021X = pAd->ApCfg.MBSSID[BssIdx].IEEE8021X;
	
#endif // CONFIG_AP_SUPPORT //


	// Update WCID attribute table
	offset = MAC_WCID_ATTRIBUTE_BASE + (Wcid * HW_WCID_ATTRI_SIZE);
#ifdef CONFIG_AP_SUPPORT	
	// 1.	Wds-links always use Pair-wise key table. 	
	// 2. 	When the CipherAlg is TKIP, AES or the dynamic WEP is enabled, 
	// 		it needs to set key into Pair-wise Key Table.
	// 3.	The pair-wise key security mode is set NONE, it means as no security.
	if (pEntry && pEntry->ValidAsWDS)
		WCIDAttri = (BssIdx<<4) | (CipherAlg<<1) | PAIRWISEKEYTABLE;
	else if ((pEntry) && ((CipherAlg == CIPHER_TKIP) || (CipherAlg == CIPHER_TKIP_NO_MIC) || (CipherAlg == CIPHER_AES) || (CipherAlg == CIPHER_NONE) || (IEEE8021X == TRUE)))
		WCIDAttri = (BssIdx<<4) | (CipherAlg<<1) | PAIRWISEKEYTABLE;
	else
		WCIDAttri = (BssIdx<<4) | (CipherAlg<<1) | SHAREDKEYTABLE;
#endif // CONFIG_AP_SUPPORT //

		
	RTMP_IO_WRITE32(pAd, offset, WCIDAttri);

		
	// Update IV/EIV table
	offset = MAC_IVEIV_TABLE_BASE + (Wcid * HW_IVEIV_ENTRY_SIZE);

	// WPA mode
	if ((CipherAlg == CIPHER_TKIP) || (CipherAlg == CIPHER_TKIP_NO_MIC) || (CipherAlg == CIPHER_AES))
	{	
		// Eiv bit on. keyid always is 0 for pairwise key 			
		IVEIV = (KeyIdx <<6) | 0x20;	
	}	 
	else
	{
		// WEP KeyIdx is default tx key. 
		IVEIV = (KeyIdx << 6);	
	}
	
	// For key index and ext IV bit, so only need to update the position(offset+3).
	RTMP_IO_WRITE8(pAd, offset+3, IVEIV); 
	
	
	DBGPRINT(RT_DEBUG_TRACE,("RTMPAddWcidAttributeEntry: WCID #%d, KeyIndex #%d, Alg=%s\n",Wcid, KeyIdx, CipherName[CipherAlg]));
	DBGPRINT(RT_DEBUG_TRACE,("	WCIDAttri = %lx \n",  WCIDAttri));	

}

/* 
    ==========================================================================
    Description:
        Parse encryption type
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
    ==========================================================================
*/
CHAR *GetEncryptType(CHAR enc)
{
    if(enc == Ndis802_11WEPDisabled)
        return "NONE";
    if(enc == Ndis802_11WEPEnabled)
    	return "WEP";
    if(enc == Ndis802_11Encryption2Enabled)
    	return "TKIP";
    if(enc == Ndis802_11Encryption3Enabled)
    	return "AES";
	if(enc == Ndis802_11Encryption4Enabled)
    	return "TKIPAES";
    else
    	return "UNKNOW";
}

CHAR *GetAuthMode(CHAR auth)
{
    if(auth == Ndis802_11AuthModeOpen)
    	return "OPEN";
    if(auth == Ndis802_11AuthModeShared)
    	return "SHARED";
	if(auth == Ndis802_11AuthModeAutoSwitch)
    	return "AUTOWEP";
    if(auth == Ndis802_11AuthModeWPA)
    	return "WPA";
    if(auth == Ndis802_11AuthModeWPAPSK)
    	return "WPAPSK";
    if(auth == Ndis802_11AuthModeWPANone)
    	return "WPANONE";
    if(auth == Ndis802_11AuthModeWPA2)
    	return "WPA2";
    if(auth == Ndis802_11AuthModeWPA2PSK)
    	return "WPA2PSK";
	if(auth == Ndis802_11AuthModeWPA1WPA2)
    	return "WPA1WPA2";
	if(auth == Ndis802_11AuthModeWPA1PSKWPA2PSK)
    	return "WPA1PSKWPA2PSK";
    else
    	return "UNKNOW";
}		

#if 1 //#ifndef UCOS
/* 
    ==========================================================================
    Description:
        Get site survey results
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage:
        		1.) UI needs to wait 4 seconds after issue a site survey command
        		2.) iwpriv ra0 get_site_survey
        		3.) UI needs to prepare at least 4096bytes to get the results
    ==========================================================================
*/
#define	LINE_LEN	(4+28+20+8+10+9+7+3)	// Channel+SSID+Bssid+WepStatus+AuthMode+Signal+WiressMode+NetworkType
VOID RTMPIoctlGetSiteSurvey(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq)
{
	CHAR		*msg;
	INT 		i=0;	 
	INT 		Status=0;
	CHAR		Ssid[MAX_LEN_OF_SSID +1];
    INT         Rssi = 0;
    UINT        Rssi_Quality = 0;
    NDIS_802_11_NETWORK_TYPE    wireless_mode;

	os_alloc_mem(NULL, (PUCHAR *)&msg, sizeof(CHAR)*((MAX_LEN_OF_BSS_TABLE)*LINE_LEN));

	if (msg == NULL)
    {   
        DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlGetSiteSurvey - msg memory alloc fail.\n"));
		return;
    }
    
	memset(msg, 0 ,(MAX_LEN_OF_BSS_TABLE)*LINE_LEN );
	memset(Ssid, 0 ,(MAX_LEN_OF_SSID +1));
	sprintf(msg,"%s","\n");
	sprintf(msg+strlen(msg),"%-4s%-28s%-20s%-8s%-10s%-9s%-7s%-3s\n",
	    "Ch", "SSID", "BSSID", "Enc", "Auth", "Siganl(%)", "W-Mode", " NT");
	
	for(i=0;i<MAX_LEN_OF_BSS_TABLE;i++)
	{
		if( pAdapter->ScanTab.BssEntry[i].Channel==0)
			break;

		if((strlen(msg)+LINE_LEN ) >= IW_SCAN_MAX_DATA)
			break;
        
		//Channel
		sprintf(msg+strlen(msg),"%-4d", pAdapter->ScanTab.BssEntry[i].Channel);
		//SSID
		memcpy(Ssid, pAdapter->ScanTab.BssEntry[i].Ssid, pAdapter->ScanTab.BssEntry[i].SsidLen);
		Ssid[pAdapter->ScanTab.BssEntry[i].SsidLen] = '\0';
		sprintf(msg+strlen(msg),"%-28s", Ssid);      
		//BSSID
		sprintf(msg+strlen(msg),"%02x:%02x:%02x:%02x:%02x:%02x   ", 
			pAdapter->ScanTab.BssEntry[i].Bssid[0], 
			pAdapter->ScanTab.BssEntry[i].Bssid[1],
			pAdapter->ScanTab.BssEntry[i].Bssid[2], 
			pAdapter->ScanTab.BssEntry[i].Bssid[3], 
			pAdapter->ScanTab.BssEntry[i].Bssid[4], 
			pAdapter->ScanTab.BssEntry[i].Bssid[5]);
		//Encryption Type
		sprintf(msg+strlen(msg),"%-8s",GetEncryptType(pAdapter->ScanTab.BssEntry[i].WepStatus));
		//Authentication Mode
		sprintf(msg+strlen(msg),"%-10s",GetAuthMode(pAdapter->ScanTab.BssEntry[i].AuthMode));
        // Rssi
        Rssi = (INT)(pAdapter->ScanTab.BssEntry[i].Rssi - pAdapter->BbpRssiToDbmDelta);
        if (Rssi >= -50)
            Rssi_Quality = 100;
        else if (Rssi >= -80)    // between -50 ~ -80dbm
            Rssi_Quality = (UINT)(24 + ((Rssi + 80) * 26)/10);
        else if (Rssi >= -90)   // between -80 ~ -90dbm
            Rssi_Quality = (UINT)(((Rssi + 90) * 26)/10);
        else    // < -84 dbm
            Rssi_Quality = 0;
        sprintf(msg+strlen(msg),"%-9d", Rssi_Quality);
        // Wireless Mode
        wireless_mode = NetworkTypeInUseSanity(&pAdapter->ScanTab.BssEntry[i]);
        if (wireless_mode == Ndis802_11FH ||
            wireless_mode == Ndis802_11DS)
            sprintf(msg+strlen(msg),"%-7s", "11b");
        else if (wireless_mode == Ndis802_11OFDM5)
            sprintf(msg+strlen(msg),"%-7s", "11a");
        else if (wireless_mode == Ndis802_11OFDM5_N)
            sprintf(msg+strlen(msg),"%-7s", "11a/n");
        else if (wireless_mode == Ndis802_11OFDM24)
            sprintf(msg+strlen(msg),"%-7s", "11b/g");
        else if (wireless_mode == Ndis802_11OFDM24_N)
            sprintf(msg+strlen(msg),"%-7s", "11b/g/n");
        else
            sprintf(msg+strlen(msg),"%-7s", "unknow");
		//Network Type		
		if (pAdapter->ScanTab.BssEntry[i].BssType == BSS_ADHOC)
			sprintf(msg+strlen(msg),"%-3s\n", " Ad");
		else
			sprintf(msg+strlen(msg),"%-3s\n", " In");
	}

	wrq->u.data.length = strlen(msg);
	Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);

    DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlGetSiteSurvey - wrq->u.data.length = %d\n", wrq->u.data.length));
	os_free_mem(NULL, (PUCHAR)msg);
}


#define	MAC_LINE_LEN	(14+4+4+10+10+10+6+6)	// Addr+aid+psm+datatime+rxbyte+txbyte+current tx rate+last tx rate
VOID RTMPIoctlGetMacTable(
	IN PRTMP_ADAPTER pAd, 
	IN struct iwreq *wrq)
{
	INT i;
	RT_802_11_MAC_TABLE MacTab;
	char *msg;

	MacTab.Num = 0;
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (pAd->MacTab.Content[i].ValidAsCLI && (pAd->MacTab.Content[i].Sst == SST_ASSOC))
		{
			COPY_MAC_ADDR(MacTab.Entry[MacTab.Num].Addr, &pAd->MacTab.Content[i].Addr);
			MacTab.Entry[MacTab.Num].Aid = (UCHAR)pAd->MacTab.Content[i].Aid;
			MacTab.Entry[MacTab.Num].Psm = pAd->MacTab.Content[i].PsMode;
			MacTab.Entry[MacTab.Num].MimoPs = pAd->MacTab.Content[i].MmpsMode;
#if 0 // ToDo
			MacTab.Entry[MacTab.Num].HSCounter.LastDataPacketTime = pAd->MacTab.Content[i].HSCounter.LastDataPacketTime;
			MacTab.Entry[MacTab.Num].HSCounter.TotalRxByteCount = pAd->MacTab.Content[i].HSCounter.TotalRxByteCount;
			MacTab.Entry[MacTab.Num].HSCounter.TotalTxByteCount = pAd->MacTab.Content[i].HSCounter.TotalTxByteCount;
#endif
			MacTab.Entry[MacTab.Num].TxRate.field.MCS = pAd->MacTab.Content[i].HTPhyMode.field.MCS;
			MacTab.Entry[MacTab.Num].TxRate.field.BW = pAd->MacTab.Content[i].HTPhyMode.field.BW;
			MacTab.Entry[MacTab.Num].TxRate.field.ShortGI = pAd->MacTab.Content[i].HTPhyMode.field.ShortGI;
			MacTab.Entry[MacTab.Num].TxRate.field.STBC = pAd->MacTab.Content[i].HTPhyMode.field.STBC;
			MacTab.Entry[MacTab.Num].TxRate.field.rsv = pAd->MacTab.Content[i].HTPhyMode.field.rsv;
			MacTab.Entry[MacTab.Num].TxRate.field.MODE = pAd->MacTab.Content[i].HTPhyMode.field.MODE;
			MacTab.Entry[MacTab.Num].TxRate.word = pAd->MacTab.Content[i].HTPhyMode.word;
			MacTab.Num += 1;
		}
	}
	wrq->u.data.length = sizeof(RT_802_11_MAC_TABLE);
	if (copy_to_user(wrq->u.data.pointer, &MacTab, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

	msg = (CHAR *) kmalloc(sizeof(CHAR)*(MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN), MEM_ALLOC_FLAG);
	memset(msg, 0 ,MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN );
	sprintf(msg,"%s","\n");
	sprintf(msg+strlen(msg),"%-14s%-4s%-4s%-10s%-10s%-10s%-6s%-6s\n",
		"MAC", "AID", "PSM", "LDT", "RxB", "TxB","CTxR", "LTxR");
	
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if (pEntry->ValidAsCLI && (pEntry->Sst == SST_ASSOC))
		{
			if((strlen(msg)+MAC_LINE_LEN ) >= (MAX_LEN_OF_MAC_TABLE*MAC_LINE_LEN) )
				break;	
			sprintf(msg+strlen(msg),"%02x%02x%02x%02x%02x%02x  ",
				pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->Aid);
			sprintf(msg+strlen(msg),"%-4d", (int)pEntry->PsMode);
			sprintf(msg+strlen(msg),"%-10d",0/*pAd->MacTab.Content[i].HSCounter.LastDataPacketTime*/); // ToDo
			sprintf(msg+strlen(msg),"%-10d",0/*pAd->MacTab.Content[i].HSCounter.TotalRxByteCount*/); // ToDo
			sprintf(msg+strlen(msg),"%-10d",0/*pAd->MacTab.Content[i].HSCounter.TotalTxByteCount*/); // ToDo
			sprintf(msg+strlen(msg),"%-6d",RateIdToMbps[pAd->MacTab.Content[i].CurrTxRate]);
			sprintf(msg+strlen(msg),"%-6d\n",0/*RateIdToMbps[pAd->MacTab.Content[i].LastTxRate]*/); // ToDo
		}
	} 
	// for compatible with old API just do the printk to console
	//wrq->u.data.length = strlen(msg);
	//if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));
	}

	kfree(msg);
}
#endif // UCOS //

INT	Set_BASetup_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    UCHAR mac[6], tid;
	char *token, sepValue[] = ":", DASH = '-';
	INT i;
    MAC_TABLE_ENTRY *pEntry;

/*
	The BASetup inupt string format should be xx:xx:xx:xx:xx:xx-d, 
		=>The six 2 digit hex-decimal number previous are the Mac address, 
		=>The seventh decimal number is the tid value.
*/
	//printk("\n%s\n", arg);
	
	if(strlen(arg) < 19)  //Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.
		return FALSE;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token)>1))
	{
		tid = simple_strtol((token+1), 0, 10);
		if (tid > 15)
			return FALSE;
		
		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++)
		{
			if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (PUCHAR)(&mac[i]), 1);
		}
		if(i != 6)
			return FALSE;

		printk("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x\n", mac[0], mac[1],
				mac[2], mac[3], mac[4], mac[5], tid);

	    pEntry = MacTableLookup(pAd, mac);

    	if (pEntry) {
        	printk("\nSetup BA Session: Tid = %d\n", tid);
	        BAOriSessionSetUp(pAd, pEntry, tid, 0, 100, TRUE);
    	}

		return TRUE;
	}

	return FALSE;

}

INT	Set_BADecline_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG bBADecline;

	bBADecline = simple_strtol(arg, 0, 10);

	if (bBADecline == 0)
	{
		pAd->CommonCfg.bBADecline = FALSE;
	}
	else if (bBADecline == 1)
	{
		pAd->CommonCfg.bBADecline = TRUE;
	}
	else 
	{
		return FALSE; //Invalid argument
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_BADecline_Proc::(BADecline=%d)\n", pAd->CommonCfg.bBADecline));

	return TRUE;
}

INT	Set_BAOriTearDown_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    UCHAR mac[6], tid;
	char *token, sepValue[] = ":", DASH = '-';
	INT i;
    MAC_TABLE_ENTRY *pEntry;

    //printk("\n%s\n", arg);
/*
	The BAOriTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d, 
		=>The six 2 digit hex-decimal number previous are the Mac address, 
		=>The seventh decimal number is the tid value.
*/
    if(strlen(arg) < 19)  //Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.
		return FALSE;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token)>1))
	{
		tid = simple_strtol((token+1), 0, 10);
		if (tid > 15)
			return FALSE;
		
		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++)
		{
			if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (PUCHAR)(&mac[i]), 1);
		}
		if(i != 6)
			return FALSE;

	    printk("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x", mac[0], mac[1],
	           mac[2], mac[3], mac[4], mac[5], tid);

	    pEntry = MacTableLookup(pAd, mac);

	    if (pEntry) {
	        printk("\nTear down Ori BA Session: Tid = %d\n", tid);
        BAOriSessionTearDown(pAd, pEntry->Aid, tid, FALSE, TRUE);
	    }

		return TRUE;
	}

	return FALSE;

}

INT	Set_BARecTearDown_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    UCHAR mac[6], tid;
	char *token, sepValue[] = ":", DASH = '-';
	INT i;
    MAC_TABLE_ENTRY *pEntry;

    //printk("\n%s\n", arg);
/*
	The BARecTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d, 
		=>The six 2 digit hex-decimal number previous are the Mac address, 
		=>The seventh decimal number is the tid value.
*/
    if(strlen(arg) < 19)  //Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and tid value in decimal format.
		return FALSE;

	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token)>1))
	{
		tid = simple_strtol((token+1), 0, 10);
		if (tid > 15)
			return FALSE;
		
		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++)
		{
			if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (PUCHAR)(&mac[i]), 1);
		}
		if(i != 6)
			return FALSE;

		printk("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x", mac[0], mac[1],
		       mac[2], mac[3], mac[4], mac[5], tid);

		pEntry = MacTableLookup(pAd, mac);

		if (pEntry) {
		    printk("\nTear down Rec BA Session: Tid = %d\n", tid);
		    BARecSessionTearDown(pAd, pEntry->Aid, tid, FALSE);
		}

		return TRUE;
	}

	return FALSE;

}

INT	Set_HtBw_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG HtBw;

	HtBw = simple_strtol(arg, 0, 10);
	if (HtBw == BW_40)
		pAd->CommonCfg.RegTransmitSetting.field.BW  = BW_40;
	else if (HtBw == BW_20)
		pAd->CommonCfg.RegTransmitSetting.field.BW  = BW_20;
	else
		return FALSE;  //Invalid argument 

	SetCommonHT(pAd);
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtBw_Proc::(HtBw=%d)\n", pAd->CommonCfg.RegTransmitSetting.field.BW));

	return TRUE;
}

INT	Set_HtMcs_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG HtMcs;

	HtMcs = simple_strtol(arg, 0, 10);
		
	if ( HtMcs <= 15 || HtMcs == 32)			
		pAd->CommonCfg.RegTransmitSetting.field.MCS = HtMcs;
	//else if ( HtMcs >15 && HtMcs < MCS_AUTO)
	//	return FALSE;  //Invalid argument 
	else// if ( HtMcs >= MCS_AUTO )
		pAd->CommonCfg.RegTransmitSetting.field.MCS = MCS_AUTO;	


	SetCommonHT(pAd);
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMcs_Proc::(HtMcs=%d)\n",pAd->CommonCfg.RegTransmitSetting.field.MCS));

	return TRUE;
}

INT	Set_HtGi_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG HtGi;

	HtGi = simple_strtol(arg, 0, 10);
		
	if ( HtGi == GI_400)			
		pAd->CommonCfg.RegTransmitSetting.field.ShortGI = GI_400;
	else if ( HtGi == GI_800 )
		pAd->CommonCfg.RegTransmitSetting.field.ShortGI = GI_800;
	else 
		return FALSE; //Invalid argument 	

	SetCommonHT(pAd);
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtGi_Proc::(ShortGI=%d)\n",pAd->CommonCfg.RegTransmitSetting.field.ShortGI));

	return TRUE;
}


INT	Set_HtTxBASize_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	UCHAR Size;

	Size = simple_strtol(arg, 0, 10);
		
	if (Size <=0 || Size >=64)
	{
		Size = 8;
	}
	pAd->CommonCfg.TxBASize = Size-1;
	DBGPRINT(RT_DEBUG_ERROR, ("Set_HtTxBASize ::(TxBASize= %d)\n", Size));

	return TRUE;
}


INT	Set_HtOpMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{

	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);

	if (Value == HTMODE_GF)
		pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_GF;
	else if ( Value == HTMODE_MM )
		pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_MM;
	else 
		return FALSE; //Invalid argument 	

	SetCommonHT(pAd);
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtOpMode_Proc::(HtOpMode=%d)\n",pAd->CommonCfg.RegTransmitSetting.field.HTMODE));

	return TRUE;

}	

INT	Set_HtStbc_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{

	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);
	
	if (Value == STBC_USE)
		pAd->CommonCfg.RegTransmitSetting.field.STBC = STBC_USE;
	else if ( Value == STBC_NONE )
		pAd->CommonCfg.RegTransmitSetting.field.STBC = STBC_NONE;
	else 
		return FALSE; //Invalid argument 	

	SetCommonHT(pAd);
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_Stbc_Proc::(HtStbc=%d)\n",pAd->CommonCfg.RegTransmitSetting.field.STBC));

	return TRUE;											
}

INT	Set_HtHtc_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{

	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->HTCEnable = FALSE;
	else if ( Value ==1 )
        pAd->HTCEnable = TRUE;
	else 
		return FALSE; //Invalid argument 	
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtHtc_Proc::(HtHtc=%d)\n",pAd->HTCEnable));

	return TRUE;		
}
			
INT	Set_HtExtcha_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{

	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);
	
	if (Value == 0)			
		pAd->CommonCfg.RegTransmitSetting.field.EXTCHA  = EXTCHA_BELOW;
	else if ( Value ==1 )
        pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = EXTCHA_ABOVE;
	else 
		return FALSE; //Invalid argument 	
	
	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtExtcha_Proc::(HtExtcha=%d)\n",pAd->CommonCfg.RegTransmitSetting.field.EXTCHA));

	return TRUE;			
}

INT	Set_HtMpduDensity_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);
	
	if (Value <=7 && Value >= 0)
		pAd->CommonCfg.BACapability.field.MpduDensity = Value;
	else
		pAd->CommonCfg.BACapability.field.MpduDensity = 4;

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMpduDensity_Proc::(HtMpduDensity=%d)\n",pAd->CommonCfg.BACapability.field.MpduDensity));

	return TRUE;																																	
}

INT	Set_HtBaWinSize_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);

	// for intel IOT 
	Value = 64;

	if (Value >=1 && Value <= 64)
	{
		pAd->CommonCfg.REGBACapability.field.RxBAWinLimit = Value;
		pAd->CommonCfg.BACapability.field.RxBAWinLimit = Value;
	}
	else
	{
        pAd->CommonCfg.REGBACapability.field.RxBAWinLimit = 64;
		pAd->CommonCfg.BACapability.field.RxBAWinLimit = 64;
	}
	
	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtBaWinSize_Proc::(HtBaWinSize=%d)\n",pAd->CommonCfg.BACapability.field.RxBAWinLimit));

	return TRUE;																																	
}		

INT	Set_HtRdg_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);
	
	if (Value == 0)			
		pAd->CommonCfg.bRdg = FALSE;
	else if ( Value ==1 )
	{
		pAd->HTCEnable = TRUE;
        pAd->CommonCfg.bRdg = TRUE;
	}
	else 
		return FALSE; //Invalid argument
	
	SetCommonHT(pAd);	
		
	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtRdg_Proc::(HtRdg=%d)\n",pAd->CommonCfg.bRdg));

	return TRUE;																																	
}		

INT	Set_HtLinkAdapt_Proc(																																																																																																																																																																																																																																																																																																																			
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->bLinkAdapt = FALSE;
	else if ( Value ==1 )
	{
			pAd->HTCEnable = TRUE;
			pAd->bLinkAdapt = TRUE;
	}
	else
		return FALSE; //Invalid argument
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtLinkAdapt_Proc::(HtLinkAdapt=%d)\n",pAd->bLinkAdapt));

	return TRUE;																																	
}		

INT	Set_HtAmsdu_Proc(																																																																																																																																																																																																																																																																																																																			
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->CommonCfg.BACapability.field.AmsduEnable = FALSE;
	else if ( Value == 1 )
        pAd->CommonCfg.BACapability.field.AmsduEnable = TRUE;
	else
		return FALSE; //Invalid argument
	
	SetCommonHT(pAd);	
		
	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtAmsdu_Proc::(HtAmsdu=%d)\n",pAd->CommonCfg.BACapability.field.AmsduEnable));

	return TRUE;																																	
}			

INT	Set_HtAutoBa_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->CommonCfg.BACapability.field.AutoBA = FALSE;
    else if (Value == 1)	
		pAd->CommonCfg.BACapability.field.AutoBA = TRUE;
	else
		return FALSE; //Invalid argument
	
    pAd->CommonCfg.REGBACapability.field.AutoBA = pAd->CommonCfg.BACapability.field.AutoBA;
	SetCommonHT(pAd);	
		
	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtAutoBa_Proc::(HtAutoBa=%d)\n",pAd->CommonCfg.BACapability.field.AutoBA));

	return TRUE;				
		
}		
																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																						
INT	Set_HtProtect_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->CommonCfg.bHTProtect = FALSE;
    else if (Value == 1)	
		pAd->CommonCfg.bHTProtect = TRUE;
	else
		return FALSE; //Invalid argument

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtProtect_Proc::(HtProtect=%d)\n",pAd->CommonCfg.bHTProtect));

	return TRUE;
}

INT	Set_SendPSMPAction_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    UCHAR mac[6], mode;
	char *token, sepValue[] = ":", DASH = '-';
	INT i;
    MAC_TABLE_ENTRY *pEntry;

    //printk("\n%s\n", arg);
/*
	The BARecTearDown inupt string format should be xx:xx:xx:xx:xx:xx-d, 
		=>The six 2 digit hex-decimal number previous are the Mac address, 
		=>The seventh decimal number is the mode value.
*/
    if(strlen(arg) < 19)  //Mac address acceptable format 01:02:03:04:05:06 length 17 plus the "-" and mode value in decimal format.
		return FALSE;

   	token = strchr(arg, DASH);
	if ((token != NULL) && (strlen(token)>1))
	{
		mode = simple_strtol((token+1), 0, 10);
		if (mode > MMPS_ENABLE)
			return FALSE;
		
		*token = '\0';
		for (i = 0, token = rstrtok(arg, &sepValue[0]); token; token = rstrtok(NULL, &sepValue[0]), i++)
		{
			if((strlen(token) != 2) || (!isxdigit(*token)) || (!isxdigit(*(token+1))))
				return FALSE;
			AtoH(token, (PUCHAR)(&mac[i]), 1);
		}
		if(i != 6)
			return FALSE;

		printk("\n%02x:%02x:%02x:%02x:%02x:%02x-%02x", mac[0], mac[1],
		       mac[2], mac[3], mac[4], mac[5], mode);

		pEntry = MacTableLookup(pAd, mac);

		if (pEntry) {
		    printk("\nSendPSMPAction MIPS mode = %d\n", mode);
		    SendPSMPAction(pAd, pEntry->Aid, mode);
		}

		return TRUE;
	}

	return FALSE;


}

INT	Set_HtMIMOPSmode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;
	
	Value = simple_strtol(arg, 0, 10);
	
	if (Value <=3 && Value >= 0)
		pAd->CommonCfg.BACapability.field.MMPSmode = Value;
	else
		pAd->CommonCfg.BACapability.field.MMPSmode = 3;

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMIMOPSmode_Proc::(MIMOPS mode=%d)\n",pAd->CommonCfg.BACapability.field.MMPSmode));

	return TRUE;																																	
}

#ifdef CONFIG_AP_SUPPORT
/* 
    ==========================================================================
    Description:
        Set Tx Stream number
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_HtTxStream_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG 	Value;	
		
	Value = simple_strtol(arg, 0, 10);

	if ((Value <= 2) && (Value >= 1) && (Value <= pAd->Antenna.field.TxPath))
		pAd->CommonCfg.TxStream = Value;
	else
		pAd->CommonCfg.TxStream = pAd->Antenna.field.TxPath;

	SetCommonHT(pAd);

	APStop(pAd);
	APStartUp(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtTxStream_Proc::(Tx Stream=%d)\n",pAd->CommonCfg.TxStream));
		
	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set Rx Stream number
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_HtRxStream_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG 	Value;	
		
	Value = simple_strtol(arg, 0, 10);

	if ((Value <= 3) && (Value >= 1) && (Value <= pAd->Antenna.field.RxPath))
		pAd->CommonCfg.RxStream = Value;
	else
		pAd->CommonCfg.RxStream = pAd->Antenna.field.RxPath;

	SetCommonHT(pAd);

	APStop(pAd);
	APStartUp(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtRxStream_Proc::(Rx Stream=%d)\n",pAd->CommonCfg.RxStream));
		
	return TRUE;
}
#endif // CONFIG_AP_SUPPORT //

INT	Set_ForceShortGI_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->WIFItestbed.bShortGI = FALSE;
    else if (Value == 1)	
			pAd->WIFItestbed.bShortGI = TRUE;
	else
		return FALSE; //Invalid argument

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_ForceShortGI_Proc::(ForceShortGI=%d)\n", pAd->WIFItestbed.bShortGI));

	return TRUE;
}



INT	Set_ForceGF_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->WIFItestbed.bGreenField = FALSE;
    else if (Value == 1)	
			pAd->WIFItestbed.bGreenField = TRUE;
	else
		return FALSE; //Invalid argument

	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_ForceGF_Proc::(ForceGF=%d)\n", pAd->WIFItestbed.bGreenField));

	return TRUE;
}

INT	Set_HtMimoPs_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Value;

	Value = simple_strtol(arg, 0, 10);
	if (Value == 0)
		pAd->CommonCfg.bMIMOPSEnable = FALSE;
    else if (Value == 1)	
		pAd->CommonCfg.bMIMOPSEnable = TRUE;
	else
		return FALSE; //Invalid argument

	DBGPRINT(RT_DEBUG_TRACE, ("Set_HtMimoPs_Proc::(HtMimoPs=%d)\n",pAd->CommonCfg.bMIMOPSEnable));

	return TRUE;
}

#ifdef ETH_CONVERT_SUPPORT
INT Set_EthConvertMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{

	// Dongle mode: it means use our default MAC address to connect to AP, and
	//				support multiple internal PCs connect to Internet via this default MAC
	// Clone mode : it means use one specific MAC address to connect to remote AP, and
	//				just the node who owns the MAC address can connect to Internet.
	// Hybrid mode: it means use some specific MAC address to connecto to remote AP, and
	//				support mulitple internal PCs connect to Internet via this specified MAC address.
	if (rtstrcasecmp(arg, "dongle") == TRUE)
	{
		pAd->EthConvert.ECMode = ETH_CONVERT_MODE_DONGLE;
		NdisMoveMemory(&pAd->EthConvert.EthCloneMac[0], &pAd->PermanentAddress[0], MAC_ADDR_LEN);
		pAd->EthConvert.CloneMacVaild = TRUE;
	}
	else if (rtstrcasecmp(arg, "clone") == TRUE)
	{
		pAd->EthConvert.ECMode = ETH_CONVERT_MODE_CLONE;
		pAd->EthConvert.CloneMacVaild = FALSE;
	}
	else if (rtstrcasecmp(arg, "hybrid") == TRUE)
	{
		pAd->EthConvert.ECMode = ETH_CONVERT_MODE_HYBRID;
		pAd->EthConvert.CloneMacVaild = FALSE;
	}
	else
	{
		pAd->EthConvert.ECMode = ETH_CONVERT_MODE_DISABLE;
		pAd->EthConvert.CloneMacVaild = FALSE;
	}
	pAd->EthConvert.macAutoLearn = FALSE;
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_EthConvertMode_Proc(): EthConvertMode=%d!\n", pAd->EthConvert.ECMode));
	
	return TRUE;
}


INT Set_EthCloneMac_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	extern UCHAR ZERO_MAC_ADDR[MAC_ADDR_LEN];
	extern UCHAR BROADCAST_ADDR[MAC_ADDR_LEN];
	//
	// If the input is the zero mac address, it means use our default(from EEPROM) MAC address as out-going 
	//     MAC address.
	// If the input is the broadcast MAC address, it means use the source MAC of first packet forwarded by
	//     our device as the out-going MAC address.
	// If the input is any other specific valid MAC address, use it as the out-going MAC address.
	//
	pAd->EthConvert.macAutoLearn = FALSE;
	if (strlen(arg) == 0)
	{	
		NdisZeroMemory(&pAd->EthConvert.EthCloneMac[0], MAC_ADDR_LEN);
		goto done;
	}
	
	if (rtstrmactohex(arg, &pAd->EthConvert.EthCloneMac[0]) == FALSE)
		goto fail;

done:
	DBGPRINT(RT_DEBUG_TRACE, ("Set_EthCloneMac_Proc(): CloneMac = %02x:%02x:%02x:%02x:%02x:%02x\n",	
	    pAd->EthConvert.EthCloneMac[0], pAd->EthConvert.EthCloneMac[1], pAd->EthConvert.EthCloneMac[2], 
	    pAd->EthConvert.EthCloneMac[3], pAd->EthConvert.EthCloneMac[4], pAd->EthConvert.EthCloneMac[5]));
	
	if (NdisEqualMemory(&pAd->EthConvert.EthCloneMac[0], &ZERO_MAC_ADDR[0], MAC_ADDR_LEN))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Use our default Mac address for cloned MAC!\n"));
		NdisMoveMemory(&pAd->EthConvert.EthCloneMac[0], &pAd->PermanentAddress[0], MAC_ADDR_LEN);
		pAd->EthConvert.CloneMacVaild = TRUE;
	}
	else if (NdisEqualMemory(&pAd->EthConvert.EthCloneMac[0], &BROADCAST_ADDR[0], MAC_ADDR_LEN))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Use first frowarded Packet's source Mac for cloned MAC!\n"));
		NdisMoveMemory(&pAd->EthConvert.EthCloneMac[0], &pAd->PermanentAddress[0], MAC_ADDR_LEN);
		pAd->EthConvert.CloneMacVaild = FALSE;
		pAd->EthConvert.macAutoLearn = TRUE;
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Use user assigned spcific Mac address for cloned MAC!\n"));
		pAd->EthConvert.CloneMacVaild = TRUE;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_EthCloneMac_Proc(): After ajust, CloneMac = %02x:%02x:%02x:%02x:%02x:%02x\n",	
	    pAd->EthConvert.EthCloneMac[0], pAd->EthConvert.EthCloneMac[1], pAd->EthConvert.EthCloneMac[2], 
	    pAd->EthConvert.EthCloneMac[3], pAd->EthConvert.EthCloneMac[4], pAd->EthConvert.EthCloneMac[5]));
	
	return TRUE;

fail:
	DBGPRINT(RT_DEBUG_ERROR, ("Set_EthCloneMac_Proc: wrong Mac Address format or length!\n"));
	NdisMoveMemory(&pAd->EthConvert.EthCloneMac[0], &pAd->CurrentAddress[0], MAC_ADDR_LEN);
	
	return FALSE;
	
}
#endif // ETH_CONVERT_SUPPORT //


//#endif // UCOS //

INT	SetCommonHT(
	IN	PRTMP_ADAPTER	pAd)
{
    OID_SET_HT_PHYMODE		SetHT;
		
	SetHT.PhyMode = pAd->CommonCfg.PhyMode;
	SetHT.TransmitNo = ((UCHAR)pAd->Antenna.field.TxPath);
	SetHT.HtMode = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.HTMODE;
	SetHT.ExtOffset = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.EXTCHA;
	SetHT.MCS = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.MCS;
	SetHT.BW = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.BW;
	SetHT.STBC = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.STBC;
	SetHT.SHORTGI = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.ShortGI;		

	if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED))
	{
		// Check the MCS validity in 1S
		if ((SetHT.TransmitNo == 1) && ((SetHT.MCS >= MCS_8) && (SetHT.MCS <= MCS_15)))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("SetCommonHT: MCS(%d) is invalid in 1S, reset it as MCS_7\n", SetHT.MCS));
			pAd->CommonCfg.RegTransmitSetting.field.MCS = MCS_7;
			SetHT.MCS = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.MCS;
		}

		if ((SetHT.BW == BW_20) && (SetHT.MCS == MCS_32))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("SetCommonHT: MCS_32 is only supported in 40-MHz, reset it as MCS_0\n"));
			pAd->CommonCfg.RegTransmitSetting.field.MCS = MCS_0;
			SetHT.MCS = (UCHAR)pAd->CommonCfg.RegTransmitSetting.field.MCS;
		}

		RTMPSetHT(pAd, &SetHT);
	}
	
	return TRUE;
}

INT	Set_FixedTxMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    if (strcmp(arg, "OFDM") == 0)
	{
		pAd->CommonCfg.FixedTxMode = FIXED_TXMODE_OFDM;
	}	
    else if (strcmp(arg, "CCK") == 0)
    {
        pAd->CommonCfg.FixedTxMode = FIXED_TXMODE_CCK;
    }
	else 
	{
		pAd->CommonCfg.FixedTxMode  = FIXED_TXMODE_NONE;
		DBGPRINT(RT_DEBUG_WARN, ("Invalid FixedTxMode (%s), ignored, Fix Tx Mode = NONE.\n", arg));
	}
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_FixedTxMode_Proc::(FixedTxMode=%d)\n", pAd->CommonCfg.FixedTxMode));

	return TRUE;
}

