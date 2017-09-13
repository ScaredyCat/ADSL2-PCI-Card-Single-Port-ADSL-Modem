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


/*
	========================================================================
	
	Routine Description:
		Classify WPA EAP message type

	Arguments:
		EAPType		Value of EAP message type
		MsgType		Internal Message definition for MLME state machine
		
	Return Value:
		TRUE		Found appropriate message type
		FALSE		No appropriate message type

	IRQL = DISPATCH_LEVEL
	
	Note:
		All these constants are defined in wpa.h
		For supplicant, there is only EAPOL Key message avaliable
		
	========================================================================
*/
BOOLEAN	WpaMsgTypeSubst(
	IN	UCHAR	EAPType,
	OUT	ULONG	*MsgType)	
{
	switch (EAPType)
	{
		case EAPPacket:
			*MsgType = MT2_EAPPacket;
			break;
		case EAPOLStart:
			*MsgType = MT2_EAPOLStart;
			break;
		case EAPOLLogoff:
			*MsgType = MT2_EAPOLLogoff;
			break;
		case EAPOLKey:
			*MsgType = MT2_EAPOLKey;
			break;
		case EAPOLASFAlert:
			*MsgType = MT2_EAPOLASFAlert;
			break;
		default:
			DBGPRINT(RT_DEBUG_INFO, ("WpaMsgTypeSubst : return FALSE; \n"));	   
			return FALSE;		
	}	
	return TRUE;
}


/*
	========================================================================

	Routine Description:
		SHA1 function 

	Arguments:

	Return Value:

	Note:

	========================================================================
*/
VOID	HMAC_SHA1(
	IN	UCHAR	*text,
	IN	UINT	text_len,
	IN	UCHAR	*key,
	IN	UINT	key_len,
	IN	UCHAR	*digest)
{
	SHA_CTX	context;
	UCHAR	k_ipad[65]; /* inner padding - key XORd with ipad	*/
	UCHAR	k_opad[65]; /* outer padding - key XORd with opad	*/
	INT		i;

	// if key is longer	than 64	bytes reset	it to key=SHA1(key)	
	if (key_len	> 64) 
	{
		SHA_CTX		 tctx;
		SHAInit(&tctx);
		SHAUpdate(&tctx, key, key_len);
		SHAFinal(&tctx,	key);
		key_len	= 20;
	}
	NdisZeroMemory(k_ipad, sizeof(k_ipad));
	NdisZeroMemory(k_opad, sizeof(k_opad));
	NdisMoveMemory(k_ipad, key,	key_len);
	NdisMoveMemory(k_opad, key,	key_len);

	// XOR key with	ipad and opad values  
	for	(i = 0;	i <	64;	i++) 
	{	
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	// perform inner SHA1 
	SHAInit(&context); 						/* init context for 1st pass */
	SHAUpdate(&context,	k_ipad,	64);		/*	start with inner pad */
	SHAUpdate(&context,	text, text_len);	/*	then text of datagram */
	SHAFinal(&context, digest);				/* finish up 1st pass */

	//perform outer	SHA1  
	SHAInit(&context);					/* init context for 2nd pass */
	SHAUpdate(&context,	k_opad,	64);	/*	start with outer pad */
	SHAUpdate(&context,	digest,	20);	/*	then results of	1st	hash */
	SHAFinal(&context, digest);			/* finish up 2nd pass */
}

VOID WPAMake8023Hdr(
    IN PRTMP_ADAPTER    pAd, 
    IN PCHAR            pDAddr, 
    IN OUT PCHAR        pHdr)
{    
     // Addr1: DA, Addr2: BSSID, Addr3: SA
    NdisMoveMemory(pHdr, pDAddr, MAC_ADDR_LEN);
    NdisMoveMemory(&pHdr[MAC_ADDR_LEN], pAd->CurrentAddress, MAC_ADDR_LEN);
    pHdr[2*MAC_ADDR_LEN] = 0x88;
    pHdr[2*MAC_ADDR_LEN+1] = 0x8e;
}

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
//	UCHAR	input[1024];
    UCHAR   *input;
	INT		currentindex = 0;
	INT		total_len;
	
    input = kmalloc(1024, GFP_ATOMIC);
    if (input == NULL)
        return;
    /* End of if */

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
    kfree(input);
}


