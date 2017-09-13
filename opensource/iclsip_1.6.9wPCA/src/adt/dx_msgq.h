/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_msgq.h
 *
 * $Id: dx_msgq.h,v 1.8 2004/09/23 04:41:27 tyhuang Exp $
 */

#ifndef DX_MSGQ_H
#define DX_MSGQ_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_trace.h>
#include "dx_vec.h"

typedef struct dxMsgQObj*	DxMsgQ;

DxMsgQ       dxMsgQNew(int initSize);
void         dxMsgQFree(DxMsgQ);
int          dxMsgQGetLen(DxMsgQ);
int          dxMsgQSendMsg(DxMsgQ, char* msg, unsigned int len);
unsigned int dxMsgQGetMsg(DxMsgQ, char* msg, unsigned int len,long timeout);

/*========================================================================
// set this DxMsgQ's trace flag ON or OFF*/
void	dxMsgQSetTraceFlag(DxMsgQ, TraceFlag);

#ifdef  __cplusplus
}
#endif

#endif /* DX_MSGQ_H */
