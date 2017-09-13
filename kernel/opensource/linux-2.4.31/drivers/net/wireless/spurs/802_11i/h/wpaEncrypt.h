/*******************************************************************************************/
/* The information contained in this file is confidential and proprietary to               */
/* ADMtek Incorporated..  No part of this file may be reproduced or distributed,           */
/* in any form or by any means for any purpose, without the express written                */
/* permission of ADMtek Inorporated..                                                      */
/*                                                                                         */
/* (c) COPYRIGHT 2002 ADMtek Inorporated., ALL RIGHTS RESERVED.                            */
/*                                                                                         */
/*                                                                                         */
/* Module Description:                                                                     */
/*                                                                                         */
/*                                                                                         */
/*******************************************************************************************/
#ifndef _WPATKIP_H_
#define _WPATKIP_H_

#include "ifx_wdrv_types.h"

//Macros for extraction/creation of BYTE/WORD values.
#define RotR1(v16)      ((((v16) >> 1) & 0x7FFF) ^ (((v16) & 1) <<15))
#define Lo8(v16)        ((BYTE)((v16) & 0x00FF))
#define Hi8(v16)        ((BYTE)(((v16) >> 8 ) & 0x00FF))
#define Lo16(v32)       ((WORD)((v32) & 0xFFFF))
#define Hi16(v32)       ((WORD)(((v32) >> 16 ) & 0xFFFF))
#define Mk16(hi, lo)    ((lo) ^ (((WORD)(hi)) << 8))

//Select the Nth 16-bit word of the temporal key byte array TK[].
#define TK16(N)         Mk16(TK[2*(N)+1], TK[2*(N)])

//Fixed algorithm "parameters".
#define PHASE1_LOOP_CNT		8   //This needs to be "big enough".
#define TA_SIZE             6   //48-bit transmitter address.
#define TK_SIZE             16  //128-bit temporal key.
#define P1K_SIZE            10  //80-bit Phase1 key.
#define RC4_KEY_SIZE        16  //128-bit RC4KEY (104 bits unknown).

//Configuration settings.
//#define DO_SANITY_CHECK     0   //Validate properties of S-box?



void HMAC_SHA1(unsigned char *text, int text_len,
               unsigned char *key, int key_len,
               unsigned char *digest);

void PRF(unsigned char *key, int key_len,
		 unsigned char *prefix, int prefix_len,
		 unsigned char *data, int data_len,
		 unsigned char *output, int len);

void NIST_AES_Key_Wrap(unsigned char *key, unsigned char *plaintext, unsigned char *ciphertext);

#endif //_WPATKIP_H_

