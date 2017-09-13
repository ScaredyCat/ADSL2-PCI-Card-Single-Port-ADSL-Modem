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
/*  File Name: 1xvars.h                                                     */ 
/*  File Description: This file contains the extern declarations             */
/*                    for all the data structures that are present           */
/*                    in the system.                                         */
/*                                                                           */ 
/*  History:                                                                 */ 
/*      Date        Author      Version     Description                      */ 
/*      ----------  ---------   -------     --------------                   */
/*      02/21/2002  Joy Lin     V1.0        Initial Version                  */ 
/*                                                                           */ 
/*****************************************************************************/
 

#ifndef __1XVARS_H__
#define __1XVARS_H__

/**************** Global Variable Decleration ***********************/

extern UINT8 ReAuthEnabled;
extern UINT16 ReAuthPeriod;

extern UINT8 MulticastKey[16];
extern UINT8 EapSessionKey[64];
extern UINT8 GTK[40];
extern UINT8 *GroupTK1Key, *GroupTxMICKey;

#endif /* __1XVARS_H__ */

