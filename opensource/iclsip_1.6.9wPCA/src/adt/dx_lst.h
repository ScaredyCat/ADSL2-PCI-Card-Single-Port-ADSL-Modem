/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_lst.h
 *
 * $Id: dx_lst.h,v 1.8 2006/04/17 03:45:48 tyhuang Exp $
 */
#include <common/cm_def.h>
#include <common/cm_trace.h>

#ifndef DX_LST_H
#define DX_LST_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
	DX_LST_POINTER = 0,
	DX_LST_INTEGER,
	DX_LST_STRING
} DxLstType;

typedef struct dxLstObj* DxLst;

DxLst     dxLstNew(DxLstType);
void      dxLstFree(DxLst, void(*elementFreeCB)(void*));
DxLst     dxLstDup(DxLst, void*(*elementDupCB)(void*));
int       dxLstGetSize(DxLst);
DxLstType dxLstGetType(DxLst);

RCODE     dxLstPutHead(DxLst,void* elm);
RCODE     dxLstPutTail(DxLst,void* elm);
void*     dxLstGetHead(DxLst);
void*     dxLstGetTail(DxLst);

int       dxLstInsert(DxLst,void* elm,int pos);
void*     dxLstGetAt(DxLst,int pos);

void*     dxLstPeek(DxLst,int pos);
void*     dxLstPeekTail(DxLst);

/* 
 * before call dxLstPeekByIter(),you must call dxLstResetIter() 
 * otherwise you will get nothing from list;
 */
void*	  dxLstPeekByIter(DxLst);
RCODE	  dxLstResetIter(DxLst);

#ifdef  __cplusplus
}
#endif

#endif /* DX_LST_H */
