/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_mem.h
 *
 * $Id: cx_mem.h,v 1.2 2005/01/19 09:08:02 tyhuang Exp $
 */
#ifndef CX_MEM_H
#define CX_MEM_H

#include <stdio.h>
#include <stdlib.h>
#include <common/cm_def.h>
#include "cx_mutex.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MAXFIELD           100
#define CMM_MAX_BKT_ENT    10
#define SS_LOCK_MUTEX      1

/* Note: (CMM_MAX_MAP_ENT * BKT_QUANTUM_SIZE) is the max bucket size */
#define CMM_MAX_MAP_ENT    401
#define BKT_QUANTUM_SIZE   16

#define PTRSTRIDE	   4

#define MAX_BUF_SIZE      BUF_SIZE_7
#define MAX_BKTS             7


#define OBJ_NAME_LEN       16 /* String length for object name */

#ifdef CCL_CX_SPEC /* specific  memory requirement  */

#define HEAP_SIZE	    (1024 * 256)
#define BUF_NMB_BKT_1       (128 * 8)        /* 1 */
#define BUF_NMB_BKT_2       (128 * 8)        /* 2 */
#define BUF_NMB_BKT_3       (64  * 1)        /* 3 */
#define BUF_NMB_BKT_4       (64  * 1)        /* 4 */
#define BUF_NMB_BKT_5       (16  * 1)        /* 5 */
#define BUF_NMB_BKT_6       (16  * 1)        /* 6 */
#define BUF_NMB_BKT_7       (16  * 3)        /* 7 */

#else /* default the memory requirement */

#define HEAP_SIZE	    (1024 * 2048)
#define BUF_NMB_BKT_1       (0)        /* 1 */
#define BUF_NMB_BKT_2       (0)        /* 2 */
#define BUF_NMB_BKT_3       (0)        /* 3 */
#define BUF_NMB_BKT_4       (0)        /* 4 */
#define BUF_NMB_BKT_5       (0)        /* 5 */
#define BUF_NMB_BKT_6       (0)        /* 6 */
#define BUF_NMB_BKT_7       (0)        /* 7 */

#endif

#define PTRALIGN(s) ( (U32)(s) % PTRSTRIDE ? \
                      ((U32)(s) + (PTRSTRIDE - ((U32)(s) % PTRSTRIDE))) : \
                      (U32)(s) )
#define CMM_DATALIGN(s, msz)  (((U32)(s) % msz ) ? \
                    ((U32)(s) + ((msz - (U32)(s) % msz))): (U32)(s)) 

typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int	U32;
typedef short		S16;

/* cx mem initial function */
RCODE cxMemInit(void);

/* cx mem cleanup function */
void cxMemClean(void);

/* cx mem alloc/free function */
void cxMemFree(void *p);
void  *cxMemCalloc(int n, int size);
void  *cxMemMalloc(int size);
void  *cxMemRealloc(void *p, int size);

/* additional function : will display memory status */
void cxMemDisplayInfo();
void cxMemDisplayBktInfo();
int cxMemInfoString(char *infoStr[], int maxLine);

#ifdef  __cplusplus
}
#endif

#endif /* QWY_MEM_H */

