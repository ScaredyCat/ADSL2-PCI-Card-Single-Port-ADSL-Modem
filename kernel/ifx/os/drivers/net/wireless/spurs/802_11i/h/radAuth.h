#ifndef RADAUTH_H
#define RADAUTH_H

#include "radDef.h"
#include "radVar.h"
#include "radDS.h"
#include "radMD5.h"

extern INT16 RadAuthentication (RAD_CLI_AUTH_INPUT *pRadInputAuth, RAD_SRV_INFO *pRadSrvAuth,
								void (*CallBackfPtr)(UINT16, UINT8 *));

INT16 CheckInputOfAuth (RAD_CLI_AUTH_INPUT *pRadInputAuthTmp);

extern void HidePwdOfAuth (UINT8 *UserPwd, UINT8 *ReqAuth, UINT8 *HiddenPwd,
						   UINT16 *LenOfPwd, UINT8 *pSecret);

void MakeStrOfAuth (UINT8 *InStr, UINT8 *OutStr, UINT16 *StrLenTmp, UINT8 * pSecret);

void FillAttributesOfAuth (RAD_CLI_AUTH_INPUT *pRadInputAuthTmp, UINT8 Pwd[], UINT16 PwdLen,
						   UINT16 *PktLen, UINT8 RadPkt[], UINT8 * pSecret);

#endif

