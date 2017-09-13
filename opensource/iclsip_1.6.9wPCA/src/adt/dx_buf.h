/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_buf.h
 *
 * $Id: dx_buf.h,v 1.9 2004/09/23 04:41:27 tyhuang Exp $
 */

#ifndef DX_BUF_H
#define DX_BUF_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include <common/cm_trace.h>

typedef struct dxBufferObj* 	DxBuffer;

DxBuffer	dxBufferNew(int size);
void		dxBufferFree(DxBuffer);
DxBuffer	dxBufferDup(DxBuffer);
void		dxBufferClear(DxBuffer);
int		dxBufferRead(DxBuffer, char* buff, int len);
int		dxBufferWrite(DxBuffer, char* buff, int len);
int		dxBufferGetCurrLen(DxBuffer);
int		dxBufferGetMaxLen(DxBuffer);

/*========================================================================
// set this DxBuffer's trace flag ON or OFF*/
void		dxBufferSetTraceFlag(DxBuffer, TraceFlag);

#ifdef  __cplusplus
}
#endif


#endif /* DX_BUF_H */
