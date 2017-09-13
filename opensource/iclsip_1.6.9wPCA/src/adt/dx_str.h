/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* Copyright Telcordia Technologies 1999 */
/*
 * dx_str.h
 *
 * $Id: dx_str.h,v 1.10 2004/09/23 04:41:27 tyhuang Exp $
 */

#ifndef DX_STR_H
#define DX_STR_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>

typedef struct dxStrObj		*DxStr;

CCLAPI DxStr	dxStrNew(void);			/* allocate a new String */
CCLAPI int	dxStrLen(DxStr);		/* length of string */
CCLAPI RCODE	dxStrClr(DxStr);	        /* set String to "" */
CCLAPI RCODE	dxStrFree(DxStr);		/* De-allocate String */
CCLAPI RCODE	dxStrCat(DxStr, const char* s);		/* Append s to DxStr */
CCLAPI RCODE	dxStrCatN(DxStr, const char* s, int n);	/* Append n character from s to DxStr */
CCLAPI char*	dxStrAsCStr(DxStr);		/* return char* version of String */
/* WARNING: dxStrAsCStr does NOT return a copy.  That is the callers responsibility */

#ifdef  __cplusplus
}
#endif

#define MINALLOC    256

#endif /* DX_STR_H */
