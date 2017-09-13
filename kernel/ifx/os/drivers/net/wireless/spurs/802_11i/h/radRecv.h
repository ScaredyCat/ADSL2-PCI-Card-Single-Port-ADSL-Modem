#ifndef RADRECV_H
#define RADRECV_H

#include "radDef.h"
#include "radVar.h"
#include "radDS.h"
#include "radMD5.h"

extern void CheckPacketForAuth(void);

extern void CheckPacketForAcc (void);

void ReleaseMemory (UINT8 ReqUserTableId);

int ExecuteAction (UINT8 *pRadRecvPkt, UINT8 SrvIndex);

#endif

