/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* Copyright Telcordia Technologies 1999 */
/*
 * dx_hash.h
 *
 * $Id: dx_hash.h,v 1.10 2004/11/23 09:28:56 tyhuang Exp $
 */

#ifndef DX_HASH_H
#define DX_HASH_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_trace.h>

typedef struct dxHashObj	*DxHash;
typedef struct entryObj		*Entry;

typedef enum {
	DX_HASH_POINTER, 
	DX_HASH_CSTRING, 
	DX_HASH_INT
} DxHashType;

/* create new hashtable
 * initSize = number of entries in table (may grow)
 * keysUpper: bool should keys be mapped to upper case
 * strings: adtHashType   should val be copied (cstrings and ints)
 *                 by adtHashAdd
 * keys are always strings and always copied
 */
DxHash	dxHashNew(int initSize, int keysUpper, DxHashType strings);

/* destroy hashtable, call freeVal on each val */
void	dxHashFree(DxHash, void (*freeVal)(void*));

/* Duplicate the hash */
DxHash  dxHashDup(DxHash, void*(*elementDupCB)(void*));

/* add pair key/val to hash, return old value */
void*	dxHashAdd(DxHash,const char* key, void* val);

/* del key from hash, return old value */
void*	dxHashDel(DxHash,const char* key);

/* return val for key in hash.  NULL if not found. */
void*	dxHashItem(DxHash,const char* key);

/* iterator over keys in hash.  NextKeys returns NULL when no keys left. */
void	StartKeys(DxHash);
char*	NextKeys(DxHash);

void    dxHashLock(DxHash);
void    dxHashUnlock(DxHash);

int     dxHashSize(DxHash);

/*========================================================================
// set this DxHash's trace flag ON or OFF*/
void	dxHashSetTraceFlag(DxHash, TraceFlag);

/* get a list from table after hashing */
Entry	dxHashItemLst(DxHash, const char* key);
/* add item ,allow use the same key */
RCODE	dxHashAddEx(DxHash _this,const char* key, void* val);
/* del item by key and value */
RCODE dxHashDelEx(DxHash _this,const char* key,void* val);

char*	dxEntryGetKey(Entry);
void*	dxEntryGetValue(Entry);
Entry	dxEntryGetNext(Entry);

#ifdef  __cplusplus
}
#endif

#endif /* DX_HASH_H */
