/*****************************************************************************/
/* The information contained in this file is confidential and proprietary to */
/* ADMtek Incorporated. No part of this file may be reproduced or distributed*/
/* , in any form or by any means for any purpose, without the express written*/
/* permission of ADMtek Incorporated.                                        */
/*                                                                           */
/*(c) COPYRIGHT 2001,2002 ADMtek Incorporated, ALL RIGHTS RESERVED.          */
/*    http://www.admtek.com.tw                                               */
/*                                                                           */
/*****************************************************************************/
/*  File Name: WpaTkipMic.c                                                      */
/*  File Description:                                                        */
/*                                                                           */
/*  History:                                                                 */
/*      Date        Author      Version     Description                      */
/*      ----------  ---------   -------     --------------                   */
/*                                                                           */
/*****************************************************************************/

// IEEE Std 802.11i/D3.0, November 2002
// F.2.2 Example code
//
// A Michael object implements the computation of the Michael MIC.
//
// Conceptually, the object stores the message to be authenticated.
// At construction the message is empty. 
// The append() method appends bytes to the message.
// The getMic() method computes the MIC over the message and returns the result.
// As a side-effect it also resets the stored message 
// to the empty message so that the object can be re-used 
// for another MIC computation.

#ifndef BYTE
    typedef unsigned char BYTE;
#endif
#ifndef WORD
    typedef unsigned short WORD;
#endif
#ifndef DWORD
    typedef unsigned long DWORD;
#endif

typedef struct _Michael_
{
    DWORD   K0, K1;     //Key.
    DWORD   L, R;       //Current state.
    DWORD   M;          //Message accumulator(single word).
    int     nBytesInM;  //Bytes in M.
}Michael, *pMichael;

//Rotation functions on 32 bit values.
#define ROL32(A, n)             (((DWORD)(A) << (n)) | (((DWORD)(A) >> (32-(n))) & (((DWORD)1 << (n)) - 1)))
#define ROR32(A, n)             ROL32((A), 32-(n))

#define SNAP_Header  "\xAA\xAA\x03\x00\x00\x00"

/******************************************************************************/
/* Convert from BYTE[] to DWORD in a portable way                             */
/******************************************************************************/
DWORD getDWORD(BYTE *p)
{
	return (((DWORD)(*p)) | 
	        (((DWORD)(*(p+1))) << 8) |
            (((DWORD)(*(p+2))) << 16) | 
            (((DWORD)(*(p+3))) << 24));
}

/******************************************************************************/
/* Convert from DWORD to Byte[] in a portable way                             */
/******************************************************************************/
void putDWORD(BYTE *p, DWORD val)
{
    int i;

    for(i = 0; i < 4; i++)
    {
        *p++ = (BYTE)(val & 0xff);
        val >>= 8;
    }
}

/******************************************************************************/
/* Reset the state to the empty message                                       */
/******************************************************************************/
void resetMIC(pMichael pMIC)
{
    pMIC->L = pMIC->K0;
    pMIC->R = pMIC->K1;
    pMIC->nBytesInM = 0;
    pMIC->M = 0;

    return;
}

/******************************************************************************/
/* Set the key to a new value                                                 */
/******************************************************************************/
void setKey(BYTE *key, pMichael pMIC)
{
    pMIC->K0 = getDWORD(key);
    pMIC->K1 = getDWORD(key+4);

    //Reset the message.
    resetMIC(pMIC);

    return;
}

/******************************************************************************/
/* Add a single byte to the internal message                                  */
/******************************************************************************/
void inline appendByte(BYTE b, pMichael pMIC)   //Updated by ardong. 20030623. (Add "inline")
{
    //Append the byte to our word-sized buffer.
    pMIC->M |= (DWORD)b << ((pMIC->nBytesInM) << 3);
    pMIC->nBytesInM++;

    //Process the word if it is full.
    if (pMIC->nBytesInM >= 4)
    {
        pMIC->L ^= pMIC->M;
        pMIC->R ^= ROL32(pMIC->L, 17);
        pMIC->L += pMIC->R;
        pMIC->R ^= ((pMIC->L & 0xFF00FF00) >> 8) | ((pMIC->L & 0x00FF00FF) << 8);
        pMIC->L += pMIC->R;
        pMIC->R ^= ROL32(pMIC->L, 3);
        pMIC->L += pMIC->R;
        pMIC->R ^= ROR32(pMIC->L, 2);
        pMIC->L += pMIC->R;

        //Clear the buffer.
        pMIC->M = 0;
        pMIC->nBytesInM = 0;
    }

    return;
}

/******************************************************************************/
/* Append bytes to the message to be MICed                                    */
/******************************************************************************/
void inline appendMIC(BYTE *src, int nBytes, pMichael pMIC) //Updated by ardong. 20030623. (Add "inline")
{
    while(nBytes > 0)
    {
        appendByte(*src++, pMIC);
        nBytes--;
    }

    return;
}

/******************************************************************************/
/* Get the MIC result. Destination should accept 8bytes of result             */
/* This also resets the message to empty                                      */
/******************************************************************************/
void inline getMIC(BYTE *dst, pMichael pMIC)    //Updated by ardong. 20030623. (Add "inline")
{
    BYTE Padding1 = 0x5a;
    BYTE Padding2 = 0x00;

    //Append the minimum padding.
    appendByte(Padding1, pMIC);
    appendByte(Padding2, pMIC);
    appendByte(Padding2, pMIC);
    appendByte(Padding2, pMIC);
    appendByte(Padding2, pMIC);

    //Padding until the length is a multiple of 4.
    while(pMIC->nBytesInM != 0)
    {
        appendByte(Padding2, pMIC);
    }

    //The appendByte function has already computed the result.
    putDWORD(dst, pMIC->L);
    putDWORD(dst + 4, pMIC->R);

    //Reset to the empty message.
    resetMIC(pMIC);
}

/******************************************************************************/
/* perform Message Integrity Check                                            */
/******************************************************************************/
void Mic_append_snap(BYTE *DA, BYTE *SA, BYTE *Payload, int Length, BYTE *MICKey, BYTE *pMICPadding)
{
	Michael MIC;
	pMichael pMIC = &MIC;
	static const BYTE priority[4] = { 0, 0, 0, 0};

    //Initial MIC.
	pMIC->M  = 0;
      
	//Set MIC key.
	setKey(MICKey, pMIC);
	
	//Append DA and SA.
	appendMIC(DA, 6, pMIC);
	appendMIC(SA, 6, pMIC);
	
	//Append one octet priority field (0x0) and 3 octets reserved (0).
	appendMIC((BYTE *)priority, 4, pMIC);
	
	//Append SNAP header.
	appendMIC((BYTE *)SNAP_Header, 6, pMIC);
		
	//Append MSDU data.
	appendMIC(Payload, Length, pMIC);
	
	//Get MIC padding.
    getMIC(pMICPadding, pMIC);
}

/******************************************************************************/
/* perform Message Integrity Check                                            */
/******************************************************************************/
void Mic(BYTE *DA, BYTE *SA, BYTE *Payload, int Length, BYTE *MICKey, BYTE *pMICPadding)
{
	Michael MIC;
	pMichael pMIC = &MIC;
	static const BYTE priority[4] = { 0, 0, 0, 0};

    //Initial MIC.
	pMIC->M  = 0;
      
	//Set MIC key.
	setKey(MICKey, pMIC);
	
	//Append DA and SA.
	appendMIC(DA, 6, pMIC);
	appendMIC(SA, 6, pMIC);
	
	//Append one octet priority field (0x0) and 3 octets reserved (0).
	appendMIC((BYTE *)priority, 4, pMIC);
	
	//Append MSDU data.
	appendMIC(Payload, Length, pMIC);
	
	//Get MIC padding.
    getMIC(pMICPadding, pMIC);
}

//Added by ardong. 20030623. Testing Michael function.
#define INLINE  __inline

//Rotate left 17.
__inline unsigned long rol17(unsigned long w)
{
    register unsigned long t, q;
	
	t = w << 17;
	q = (w >> 15);
	return(t|q);
}

//Rotate left 3.
__inline unsigned long rol3(unsigned long w) 			
{
    register unsigned long t, q;
	
	t = w << 3;
	q = (w >> 29);
	return(t|q);
}

//Rotate right 2.
__inline unsigned long ror2(unsigned long w) 			
{
    register unsigned long t, q;
	
	t = (w >> 2);
	q = w << 30;
	return(t|q);
}

#define	MBLOCK(L, R)						            \
	R = R ^ rol17(L);					                \
	L += R;							                    \
	R ^= ((L & 0xff00ff00)>>8)|((L & 0x00ff00ff) << 8);	\
	L += R;							                    \
	R ^= rol3(L);						                \
	L += R;							                    \
	R ^= ror2(L);						                \
	L += R;

__inline static unsigned long getwb(unsigned char *cp)
{
    register unsigned long t;

	t = 0;
	t = *cp++;
	t |= (*cp++) << 8;
	t |= (*cp++) << 16;
	t |= (*cp++) << 24;
	return(t);
}

__inline void putwb(unsigned long w, unsigned char *cp)
{
	//MS compiler forces use of 0xff.
	*cp++ = (unsigned char)w;
	*cp++ = (unsigned char)(w >> 8);
	*cp++ = (unsigned char)(w >> 16);
	*cp++ = (unsigned char)(w >> 24);
	return;
}

void Mic_Simple(BYTE *PktHdr, BYTE *Payload, int Len, BYTE *MICKey, BYTE *pMICPadding) 
{
    register unsigned long M;
    unsigned long L, R;
//    int Len = Length;
    int PktHdrLen = 12;
    register unsigned char *sp, *cp;
    unsigned char Priority[] = {0x00, 0x00, 0x00, 0x00};

	L = getwb(MICKey);          //L = *LL.
	R = getwb(MICKey + 4);      //R = *RR.

	sp = Payload;
	sp[Len++] = 0x5A;			//Message padding.
	sp[Len++] = 0;				//4 required.
	sp[Len++] = 0;
	sp[Len++] = 0;
	sp[Len++] = 0;

    //4 bytes aligned.
	while(Len & 0x03)
	{
		sp[Len++] = 0;
	}

	//Mic the header if present.
	sp = PktHdr;
	while (PktHdrLen > 0)
	{
		M = getwb(sp);
		sp += 4;
		PktHdrLen -= 4;
		L ^= M;
		MBLOCK(L, R);
	}

    //Priority. 4 bytes.
    M = getwb(Priority);
    L ^= M;
    MBLOCK(L, R);

    //Mic data payload.
	sp = Payload;                     
	while (Len > 0)
	{
		M = getwb(sp);
		sp += 4;
		Len -= 4;
		L ^= M;
		MBLOCK(L, R);
	}

    cp = pMICPadding;
	putwb(L, (BYTE *)cp); 
	cp = pMICPadding + 4;
	putwb(R, (BYTE *)cp); 
}

void Mic_SNAP_Simple(BYTE *PktHdr, BYTE *Payload, int Length, BYTE *MICKey, BYTE *pMICPadding) 
{
    register unsigned long M;
    unsigned long L, R;
    int Len = Length;
    int PktHdrLen = 12;
    register unsigned char *sp, *cp;
    unsigned char Priority[] = {0x00, 0x00, 0x00, 0x00};
    unsigned char Temp[8] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00,0x00, 0x00};

	L = getwb(MICKey);          //L = *LL.
	R = getwb(MICKey + 4);      //R = *RR.

	//Mic the header if present.
	sp = PktHdr;
	while (PktHdrLen > 0)
	{
		M = getwb(sp);
		sp += 4;
		PktHdrLen -= 4;
		L ^= M;
		MBLOCK(L, R);
	}

    //Priority. 4 bytes.
    M = getwb(Priority);
    L ^= M;
    MBLOCK(L, R);

	sp = Payload;
	sp[Len++] = 0x5A;			//Message padding.
	sp[Len++] = 0;				//4 required.
	sp[Len++] = 0;
	sp[Len++] = 0;
	sp[Len++] = 0;
	sp[Len++] = 0;
	sp[Len++] = 0;
	sp[Len++] = 0;

    Len -= ((Len + 6) % 4);

//    memcpy(Temp, SNAP_Header, 6);
    memcpy(Temp + 6, Payload, 2);

    //SNAP_Header. (First 4 bytes)
    M = getwb(Temp);
    L ^= M;
    MBLOCK(L, R);

    M = getwb(Temp + 4);
    L ^= M;
    MBLOCK(L, R);

    //The remain part of Payload.
    sp = Payload + 2;
    Len -= 2;
	while (Len > 0)
	{
		M = getwb(sp);
		sp += 4;
		Len -= 4;
		L ^= M;
		MBLOCK(L, R);
	}

//    cp = pMICPadding;
//	putwb(L, (BYTE *)cp); 
//	cp = pMICPadding + 4;
//	putwb(R, (BYTE *)cp);

//Added by ardong. 20030623. Padding MIC value here.
	cp = Payload + Length;
	putwb(L, (unsigned char *)cp); 
	cp = Payload + Length + 4;
	putwb(R, (unsigned char *)cp); 
}


