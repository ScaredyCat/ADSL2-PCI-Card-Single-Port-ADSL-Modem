#ifndef RADACC_H
#define RADACC_H

#include "radDef.h"
#include "radVar.h"
#include "radDS.h"
#include "radMD5.h"

extern INT16 RadAccounting (RAD_CLI_ACC_INPUT *pRadInputAcc, RAD_SRV_INFO *pRadSrvAcc,
					 		void (*CallBackfPtr)(UINT16 , UINT8 *));

void FillAttributesOfAcc (RAD_CLI_ACC_INPUT * pRadInputAccTmp, UINT8 *PktAcc, UINT16 *PktLen);

void MakeFillAuthenticator (UINT8 *PktAcc, RAD_SRV_INFO *pRadSrvAcc);

#endif

