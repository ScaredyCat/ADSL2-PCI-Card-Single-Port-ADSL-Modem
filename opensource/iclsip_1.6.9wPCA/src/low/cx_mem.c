/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cx_mem.c
 *
 * $Id: cx_mem.c,v 1.6 2005/03/07 05:05:03 tyhuang Exp $
 */

#include "cx_mem.h"

/*
 * 8 bytes extra for memory header
 * The 1st 4-byte is used internally to point to next bucket block
 * The 2nd 4-byte is used to record the memory size
 */
#define MEM_HDR_SIZE	8
#define MEM_SIZE_POS	4

/* Define bucket size that needed (better be multiple of BKT_QUANTUM_SIZE */
/* WARNING: bucket size can not exceed (CMM_MAX_MAP_ENT * BKT_QUANTUM_SIZE) */
#define BUF_SIZE_1          32                    /* 1 */
#define BUF_SIZE_2          64                    /* 2 */
#define BUF_SIZE_3         128                    /* 3 */
#define BUF_SIZE_4         256                    /* 4 */
#define BUF_SIZE_5         512                    /* 5 */
#define BUF_SIZE_6        1024                    /* 6 */
#define BUF_SIZE_7        2816                    /* 7 */

/* region information structure -- passed to SRegRegion() */
typedef struct sRegInfo
{
    void         *regCb;  /* region control block pointer */
    char         *start;  /* start address of region */
    U32          size;    /* size of region */
} SRegInfo;

/* Memory bucket structure */
typedef struct cmMmBkt                /* Bucket Structure */
{
    char      *nextBlk;  /* Pointer to the next memory block */
    U32       size;     /* Size of the block */
    U32       numBlks;  /* Total number of blocks in the bucket */
    U32       numAlloc; /* Number of blocks allocated */
    U32	      maxAlloc; /* Maxinum number of blocks has been allocated */
    U32	      numReq;   /* Number of the allocation request */
    CxMutex   bktLock;  /* Lock to protect the bucket pool */
} CmMmBkt;

/* Size-To-Bucket map table structure */ 
typedef struct cmMmMapBkt
{
    U16   bktIdx;      /* The index to the memory bucket */
    U16   numReq;      /* Number of the allocation request */
    U16   numFailure;  /* Number of allocation failure form the bucket */
} CmMmMapBkt;

/* Heap entry structure linked in the heap control block */ 
typedef struct   cmHEntry      CmHEntry;
struct cmHEntry
{
    CmHEntry  *next;             /* Pointer to the next entry block */
    U32       size;             /* size of the heap entry block */
};

#define CMM_MINBUFSIZE   (PTRALIGN(sizeof(CmHEntry)))

/* Heap control block */
typedef struct cmMmHeapCb
{
    CmHEntry *nextHeapBlk; /* Next heap block entry */
    U32      avlSize;      /* Total available memory */
    U32      minSize;      /* Minimum size that can be allocated */
    CxMutex  heapLock;     /* Lock to protect the heap pool */
    U32      numReq;       /* Number of allocation request */
    U32      numFailure;   /* Number of allocation failure */
    U32      lastReqSize;  /* The memory size of the last allocation request */
} CmMmHeapCb;

/* Memory region control block */ 
typedef struct cmMmRegCb
{
    SRegInfo   regInfo;   /* Region information block */
    char      *pAddr;     /* Physical address of the memory block.
			     Valid if CMM_REG_PHY_VALID bit is set */

    U32        bktSize;   /* Size of the memory used for the bucket pool */
    U32        bktQnPwr;  /* Quantum size of the bucket pool */
    U32        bktMaxBlkSize; /* Maximum size of block in the bucket pool */
    U32        numBkts;       /* Number of buckets in the Bucket Pool */

    CmMmMapBkt mapTbl[CMM_MAX_MAP_ENT]; /* size-to-bucket map table */
    CmMmBkt    bktTbl[CMM_MAX_BKT_ENT]; /* Pointer to the memory bkt tbl */

    BOOL       heapFlag;  /* Set to true if the heap pool is configured */
    U32        heapSize;  /* Size of the heap pool */
    CmMmHeapCb heapCb;    /* Heap pool control block */
} CmMmRegCb;

/* Bucket configuration structure. */
typedef struct cmMmBktCfg
{
    U32  size;     /* Size of the memory block */
    U32  numBlks;  /* Number of the block in the bucket */
} CmMmBktCfg;

/* Memory Region configuration structure. */ 
typedef struct cmMmRegCfg
{
    U32   size;       /* Size of the memory */
    char  *vAddr;     /* Start address of the memory */
    U8     lType;     /* Lock Type to be used */

    char  *pAddr;     /* Physical address of the memory block: Valid 
			 if CMM_REG_PHY_VALID bit of chFlag is set */
    U32    bktQnSize; /* Quatum size of the memory block */
    U16    numBkts;   /* Number of buckets in the Bucket Pool */

    CmMmBktCfg  bktCfg[CMM_MAX_BKT_ENT];  /* Bucket configuration structure */
} CmMmRegCfg;


S16 cmFree(CmMmRegCb *regCb, char *ptr, U32 size);
RCODE cmAlloc(CmMmRegCb *regCb, U32 *size, U8 **ptr);
RCODE cmHeapFree(CmMmHeapCb *heapCb, char *ptr, U32 size);
RCODE cmHeapAlloc(CmMmHeapCb *heapCb, U8 **ptr, U32 *size);
S16 cmMmRegInit(CmMmRegCb *regCb, CmMmRegCfg *cfg);
void  cmMmHeapInit(char *memAddr, CmMmHeapCb *heapCb, U32 size);
void cmMmBktInit(char **memAddr,
		 CmMmRegCb *regCb,
		 CmMmRegCfg *cfg,
		 U16 bktIdx,
		 U16 *lstMapIdx);

CmMmRegCfg mtCMMRegCfg;
CmMmRegCb *cxMemCb=NULL;

/*
 * cxMemInit()
 *
 * Desc:  This function initializes specific information
 *        in the region/pool tables and configures the common
 *        memory manager for use.
 *
 * Ret:   RC_OK    - successful, 
 *        RC_ERROR - unsuccessful.
 */

RCODE cxMemInit(void)
{
    int memSize;

	if (NULL!=cxMemCb) {
		/* has initialized before */
		return RC_OK;
	}
    /* allocate space for the region control block */
    cxMemCb = (CmMmRegCb *)calloc(1, sizeof(CmMmRegCb));
    if (cxMemCb == NULL) {
		return RC_ERROR;
    }

    memSize = HEAP_SIZE +\
	(BUF_NMB_BKT_1 * BUF_SIZE_1) +\
	(BUF_NMB_BKT_2 * BUF_SIZE_2) +\
	(BUF_NMB_BKT_3 * BUF_SIZE_3) +\
	(BUF_NMB_BKT_4 * BUF_SIZE_4) +\
	(BUF_NMB_BKT_5 * BUF_SIZE_5) +\
	(BUF_NMB_BKT_6 * BUF_SIZE_6) +\
	(BUF_NMB_BKT_7 * BUF_SIZE_7);

    /* allocate space for the region */
    mtCMMRegCfg.vAddr = (char *)calloc(memSize, sizeof(char));
    if (mtCMMRegCfg.vAddr == NULL) {
       free(cxMemCb);
       return RC_ERROR;
    }

    /* set up the CMM configuration structure */
    mtCMMRegCfg.size = memSize;
    mtCMMRegCfg.lType = SS_LOCK_MUTEX;
    mtCMMRegCfg.bktQnSize = 16;
    mtCMMRegCfg.numBkts = MAX_BKTS;

    mtCMMRegCfg.bktCfg[0].size = BUF_SIZE_1;
    mtCMMRegCfg.bktCfg[0].numBlks = BUF_NMB_BKT_1;

    mtCMMRegCfg.bktCfg[1].size = BUF_SIZE_2;
    mtCMMRegCfg.bktCfg[1].numBlks = BUF_NMB_BKT_2;

    mtCMMRegCfg.bktCfg[2].size = BUF_SIZE_3;
    mtCMMRegCfg.bktCfg[2].numBlks = BUF_NMB_BKT_3;

    mtCMMRegCfg.bktCfg[3].size = BUF_SIZE_4;
    mtCMMRegCfg.bktCfg[3].numBlks = BUF_NMB_BKT_4;

    mtCMMRegCfg.bktCfg[4].size = BUF_SIZE_5;
    mtCMMRegCfg.bktCfg[4].numBlks = BUF_NMB_BKT_5;

    mtCMMRegCfg.bktCfg[5].size = BUF_SIZE_6;
    mtCMMRegCfg.bktCfg[5].numBlks = BUF_NMB_BKT_6;

    mtCMMRegCfg.bktCfg[6].size = BUF_SIZE_7;
    mtCMMRegCfg.bktCfg[6].numBlks = BUF_NMB_BKT_7;

    /* initialize the CMM */
    if (cmMmRegInit(cxMemCb, &mtCMMRegCfg) != RC_OK) {
		free(mtCMMRegCfg.vAddr);
		free(cxMemCb);
		return RC_ERROR;
    }

    return RC_OK;
}

/*
 * cxMemClean()
 *
 * Desc:  This function remove specific information
 *        in the region/pool tables and delete the common
 *        memory manager for use.
 *
 * Ret:   RC_OK    - successful, 
 *        RC_ERROR - unsuccessful.
 */

void cxMemClean(void)
{
    U16  bktIdx; 

	if (NULL==cxMemCb) return;
    /*if (cxMemCb->bktSize) {*/
	/* Bucket pool is configured */
	if (cxMemCb->numBkts>0) {
		/* Free the initialized locks of the buckets */
		for ( bktIdx = cxMemCb->numBkts; bktIdx > 0;) {
			cxMutexFree(cxMemCb->bktTbl[--bktIdx].bktLock);
		}
    }

    if (cxMemCb->heapFlag) {
		/* Destroy the bucket lock */
		cxMutexFree(cxMemCb->heapCb.heapLock);
		cxMemCb->heapFlag=FALSE;
    }

	if (NULL!=mtCMMRegCfg.vAddr) {
		free(mtCMMRegCfg.vAddr);
		mtCMMRegCfg.vAddr=NULL;
	}
	if (NULL!=cxMemCb) {
		free(cxMemCb);
		cxMemCb=NULL;
	}

    return;
}

/*
 * cmMmBktInit()
 *
 * Desc:  Initialize the bucket and the map table. This function is called by the cmMmRegInit. 
 *
 * Ret:   void
 */

void cmMmBktInit(char **memAddr,
		 CmMmRegCb *regCb,
		 CmMmRegCfg *cfg,
		 U16 bktIdx,
		 U16 *lstMapIdx)
{
    U32   cnt;
    U16   idx;
    U32   numBlks;
    U32   size;
    char **next;

    size = cfg->bktCfg[bktIdx].size; 
    numBlks = cfg->bktCfg[bktIdx].numBlks;

    /* Reset the next pointer */
    regCb->bktTbl[bktIdx].nextBlk = NULL;

    /* Initialize the link list of the memory block */
    next = &(regCb->bktTbl[bktIdx].nextBlk);
    for (cnt = 0; cnt < numBlks; cnt++) {
	*next     = *memAddr;
	next      = (char **)(*memAddr);
	*memAddr  = (*memAddr) + size;
    }
    *next = NULL;

    /* Initialize the Map entry */
    idx = size / cfg->bktQnSize;

    /* 
     * Check if the size is multiple of quantum size. If not we need to
     * initialize one more map table entry.
     */
    if(size % cfg->bktQnSize) {
	idx++;
    }

    while ( *lstMapIdx < idx) {
	regCb->mapTbl[*lstMapIdx].bktIdx = bktIdx;
	regCb->mapTbl[*lstMapIdx].numReq     = 0;
	regCb->mapTbl[*lstMapIdx].numFailure = 0;

	(*lstMapIdx)++;
    }

    /* Initialize the bucket structure */
    regCb->bktTbl[bktIdx].size     = size;
    regCb->bktTbl[bktIdx].numBlks  = numBlks;
    regCb->bktTbl[bktIdx].numAlloc = 0;
    regCb->bktTbl[bktIdx].maxAlloc = 0;
    regCb->bktTbl[bktIdx].numReq = 0;

    /* Update the total bucket size */
    regCb->bktSize += (size * numBlks); 

    return;
} /* end of cmMmBktInit */

/*
 * Fun:   cmMmHeapInit
 *
 * Desc:  Initialize the heap pool. This function is called by the cmMmRegInit.
 *
 * Ret:   void
 */

void  cmMmHeapInit(char *memAddr, CmMmHeapCb *heapCb, U32 size)
{
    /* Initialize the heap control block */
    heapCb->avlSize    = size;
    heapCb->minSize    = CMM_MINBUFSIZE;
    heapCb->nextHeapBlk = (CmHEntry *)memAddr;
    heapCb->nextHeapBlk->next = NULL;
    heapCb->nextHeapBlk->size = size;

    heapCb->numReq      = 0;
    heapCb->numFailure  = 0;
    heapCb->lastReqSize = 0;

    return;
} /* end of cmMmHeapInit */


/*
 * cmMmRegInit()
 *
 * Desc:  Configure the memory region for allocation. The function 
 *        registers the memory region with System Service by calling
 *        SRegRegion.
 *
 *        The memory owner calls this function to initialize the memory 
 *        manager with the information of the memory region. Before 
 *        calling this function, the memory owner should allocate memory 
 *        for the memory region. The memory owner should also provide the 
 *        memory for the control block needed by the memory manager. The 
 *        memory owner should allocate the memory for the region control 
 *        block as cachable memory. This may increase the average 
 *        throughput in allocation and deallocation as the region control
 *        block is mostly accessed by the CMM.
 *
 * Ret:   RC_OK     - successful, 
 *        RC_ERROR  - unsuccessful.
 */

S16 cmMmRegInit(CmMmRegCb *regCb, CmMmRegCfg *cfg)
{
    char *memAddr;
    U16   bktIdx;
    U16   lstMapIdx;
    U32   lstQnSize;
    U32   bktBlkSize;

    /* error check on parameters */
    if ((regCb == NULL) || (cfg == NULL)) {
	return RC_ERROR;
    }

    /* Error check on the configuration fields */
    if ((!cfg->size) || (cfg->vAddr == NULL) || 
        (cfg->numBkts > CMM_MAX_BKT_ENT)) {
	return RC_ERROR;
    }

    /* Error check whether the size of the mapping table is sufficient */
    if ((cfg->numBkts) && 
	((cfg->bktCfg[cfg->numBkts - 1].size) / cfg->bktQnSize) > CMM_MAX_MAP_ENT) {
	return RC_ERROR;
    }

    /* Check if the quantum size is power of 2 */
    if ((cfg->numBkts) &&
	((cfg->bktQnSize - 1) & (cfg->bktQnSize))) {
	return RC_ERROR;
    }

    /* 
     * Check if the size of the memory region is enough and also whether two
     * consecutive buckets falls within same quanta. (This is to make sure
     * user does not define bucket size too close to each other.)
     */
    lstQnSize      = cfg->bktQnSize;
    regCb->bktSize = 0;

    for ( bktIdx = 0; bktIdx < cfg->numBkts; bktIdx++) {
	if ((bktBlkSize = cfg->bktCfg[bktIdx].size) < lstQnSize) {
	    /* 
	     * Two consecutive buckets are not separated by quantum size.
	     */
	    return RC_ERROR;
	}
	regCb->bktSize += (cfg->bktCfg[bktIdx].size * 
			   cfg->bktCfg[bktIdx].numBlks);

	if (regCb->bktSize > cfg->size) {
	    /* Size of the memory region is less than the required size */
	    return RC_ERROR;
	}

	lstQnSize = ((bktBlkSize / cfg->bktQnSize) + 1) * cfg->bktQnSize;
    }

    /* Initialize the region control block */
    regCb->regInfo.regCb = regCb;
    regCb->regInfo.start = cfg->vAddr;
    regCb->regInfo.size  = cfg->size;

    /* Initial address of the memory region block */
    memAddr    = cfg->vAddr;

    /* Initialize the fields related to the bucket pool */
    regCb->bktMaxBlkSize = 0;
    regCb->bktSize       = 0; 

    if (cfg->numBkts) {
	/* Last bucket has the maximum size */
	regCb->bktMaxBlkSize = cfg->bktCfg[cfg->numBkts - 1].size;
   
	/* Get the power of the bktQnSize */
	regCb->bktQnPwr = 0; 
	while( !((cfg->bktQnSize >> regCb->bktQnPwr) & 0x01)) {
	    regCb->bktQnPwr++;
	}
    
	/* Initialize the bktIndex of the map entries to 0xFF */
	for ( lstMapIdx = 0; lstMapIdx < CMM_MAX_MAP_ENT; lstMapIdx++) {
	    regCb->mapTbl[lstMapIdx].bktIdx = 0xFF;
	}

	lstMapIdx = 0;
	for ( bktIdx = 0; bktIdx < cfg->numBkts; bktIdx++) {
	    /* Allocate the lock for the bucket pool */
	    /*regCb->bktTbl[bktIdx].bktLock = SMutexNew(CX_MUTEX_INTERTHREAD,0);*/
		regCb->bktTbl[bktIdx].bktLock = cxMutexNew(CX_MUTEX_INTERTHREAD,0);
	    if (regCb->bktTbl[bktIdx].bktLock == NULL) {
		/* Free the initialized lock for the earlier buckets. */
		for ( ;bktIdx > 0;) {
		    cxMutexFree(regCb->bktTbl[--bktIdx].bktLock);
		}

		return RC_ERROR;
	    }

	    cmMmBktInit( &memAddr, regCb, cfg, bktIdx, &lstMapIdx); 
	}

	/* Used while freeing the bktLock in cmMmRegDeInit */
	regCb->numBkts = cfg->numBkts;
    }

    /* 
     * Initialize the heap pool if size of the memory region is more
     * than the size of the bucket pool 
     */
    regCb->heapSize = 0;
    regCb->heapFlag = FALSE;

    /* Align the memory address */
    memAddr = (char *)(PTRALIGN(memAddr));

    regCb->heapSize = cfg->vAddr + cfg->size - memAddr;  

    /* 
     * Round the heap size so that the heap size is multiple 
     * of CMM_MINBUFSIZE 
     */
    regCb->heapSize -= (regCb->heapSize %  CMM_MINBUFSIZE);

    if (regCb->heapSize) {
	/* Allocate the lock for the heap pool */
	/*regCb->heapCb.heapLock = SMutexNew(CX_MUTEX_INTERTHREAD,0); */
		regCb->heapCb.heapLock = cxMutexNew(CX_MUTEX_INTERTHREAD,0);
		if (regCb->heapCb.heapLock == NULL) {
			/* Free the initialized locks of the buckets */
			for (bktIdx = cfg->numBkts; bktIdx > 0; bktIdx--) {
			cxMutexFree(regCb->bktTbl[bktIdx-1].bktLock);
		}
	    return RC_ERROR;
	}
        
	regCb->heapFlag = TRUE;
	cmMmHeapInit(memAddr, &(regCb->heapCb), regCb->heapSize); 
    }

    return RC_OK;
} /* end of cmMmRegInit*/

/*
 * Fun:   cmHeapAlloc
 *
 * Desc:  Allocates the memory block from the heap pool. 
 *        This function is called by the cmAlloc. cmAlloc calls this
 *        function when there is no memory block available in the bucket 
 *        and the  heap pool is configured.
 *
 * Ret:   RC_OK    - successful
 *        RC_ERROR - unsuccessful.
 */

RCODE cmHeapAlloc(CmMmHeapCb *heapCb, U8 **ptr, U32 *size)
{
    CmHEntry  *prvHBlk;    /* Previous heap block */
    CmHEntry  *curHBlk;    /* Current heap block */ 
    U32        tmpSize;

    heapCb->numReq++;
    heapCb->lastReqSize = *size;

    /* Roundup the requested size */
	*size = CMM_DATALIGN(*size, (heapCb->minSize));
   
    /* Check if the available total size is adequate. */
    if ((*size) > heapCb->avlSize) {
		printf("No available memory! \n");
		getchar();
	return RC_ERROR;
    }

    /* Acquire the heap lock */
    if (cxMutexLock(heapCb->heapLock) != 0){
		printf("MutexLock error! \n");
		return RC_ERROR;
	}
    /* 
     * Search through the heap block list in the heap pool of size 
     * greater than or equal to the requested size.
     */ 
    prvHBlk = (CmHEntry *)&(heapCb->nextHeapBlk);
    for ( curHBlk = prvHBlk->next ; curHBlk; curHBlk = curHBlk->next) {
	/*
	 * Since the size of the block is always multiple of CMM_MINBUFSIZE 
	 * and the requested size is rounded to the size multiple of
	 * CMM_MINBUFSIZE, the difference between the size of the heap block
	 * and the size to allocate will be either zero or multiple of
	 * CMM_MINBUFSIZE. 
	 */
		if ((*size) <= curHBlk->size) {
			if ((tmpSize = (curHBlk->size - (*size)))) {
			/* Heap block of bigger size */
			*ptr = (char *)curHBlk + tmpSize;             
			curHBlk->size = tmpSize;
			} 
			else {
			/* Heap block is same size of the requested size */
			*ptr = (char *)curHBlk;
			prvHBlk->next = curHBlk->next;
			}
			heapCb->avlSize -= (*size); 
			/* Release the lock */
			cxMutexUnlock(heapCb->heapLock);
			return RC_OK;
		}
		prvHBlk = curHBlk;
    }
    /* Release the lock */
    cxMutexUnlock(heapCb->heapLock);
    heapCb->numFailure++;

    return RC_ERROR;
} /* end of cmHeapAlloc */

/*
 * cmHeapFree()
 *
 * Desc:  Return the memory block from the heap pool. 
 *
 *        This function returns the memory block to the heap  pool. This 
 *        function is called by cmFree. The function does not check the 
 *        validity of the memory block. The caller must be sure that the 
 *        block was previously allocated and belongs to the heap pool. The 
 *        function maintain the sorting order of the memory block on the
 *        starting address of the block. This function also do compaction 
 *        if the neighbouring blocks are already in the heap. 
 *
 *       Ret:   RC_OK    - successful
 *              RC_ERROR - unsuccessful.
 */

RCODE cmHeapFree(CmMmHeapCb *heapCb, char *ptr, U32 size)
{
    CmHEntry  *p;    
    CmHEntry  *curHBlk;    /* Current heap block */ 


    /* Roundup the requested size */
    size = CMM_DATALIGN(size, (heapCb->minSize));
    /* increase the avlSize */
    heapCb->avlSize += size;

    p = (CmHEntry *)ptr; 

    /* Acquire the heap lock */
    if (cxMutexLock(heapCb->heapLock) != 0)
	return RC_ERROR;

    for ( curHBlk = heapCb->nextHeapBlk; curHBlk; curHBlk = curHBlk->next) {
	/* 
	 * The block will be inserted to maintain the sorted order on the
	 * starting address of the block.
	 */
		if (p > curHBlk) {
			if (!(curHBlk->next) || (p < (curHBlk->next))) {
			/* Heap block should be inserted here */

			/* 
			 * Check if the block to be returned can be merged with the
			 * current block.
			 */
			if (((char *)curHBlk + curHBlk->size) == (char *)p) {
				/* Merge the block */
				size = (curHBlk->size += size);
				p = curHBlk;
			}
			else {
				/* insert the block */
				p->next = curHBlk->next;
				p->size = size; 
				curHBlk->next = p;
			}

			/* Try to merge with the next block in the chain */
			if (((char *)p + size) == (char *)(p->next)) {
				/* p->next can not be NULL */
				p->size += p->next->size; 
				p->next  = p->next->next;
			}

			/* Release the lock */
			cxMutexUnlock(heapCb->heapLock);

			return RC_OK;
 		}
	}
	else{
		if(heapCb->nextHeapBlk != NULL){		
		/* Heap block is not empty. Insert the block in the begining. */
			p->size = size;
			p->next = heapCb->nextHeapBlk ;
			heapCb->nextHeapBlk  = p;
			
			if (((char *)p + size) == (char *)(p->next) && p->next ) {
				/* p->next can not be NULL */
				p->size += p->next->size; 
				p->next  = p->next->next;
			}
			cxMutexUnlock(heapCb->heapLock);
			return RC_OK;
		}
	} 
    }

    if (heapCb->nextHeapBlk == NULL) {
	/* Heap block is empty. Insert the block in the head. */
	heapCb->nextHeapBlk = p;
	p->next = NULL;
	p->size = size;

	/* Release the heap lock */
	cxMutexUnlock(heapCb->heapLock);

	return RC_OK;
    }

    /* Release the lock */
    cxMutexUnlock(heapCb->heapLock);

    return RC_ERROR;
} /* end of cmHeapFree */


/*
 * cmAlloc()
 *
 * Ret:   RC_OK     - successful
 *        RC_ERROR  - unsuccessful.
 *
 * Desc:  The function allocates a memory block of size at least equal to 
 *        the requested size. The size parameter will be updated with the 
 *        actual size of the memory block allocated for the request. The 
 *        CMM tries to allocate the memory block form the bucket pool. If
 *        there is no memory in the bucket the CMM allocates the memory 
 *        block form the heap pool. This function is always called by the
 *        System Service module.
 *    
 *        The caller of the function should try to use the out value of 
 *        the size while returning the memory block to the region. However 
 *        the current design of the memory manager does not enforce to pass
 *        the actual size of the memory block.  (Due to the SGetSBuf 
 *        semantics the layer will not able to pass the correct size of the
 *        memory block while calling SPutSBuf).
 */

RCODE cmAlloc(CmMmRegCb *regCb, U32 *size, U8 **ptr)
{
   
    /*
     * Error check on parameters.
     * Add more checks if this routine is open to public usage.
     */
    if (!(*size)) {
	return RC_ERROR;
    }


    /* Memory not available in the bucket pool */
    if (regCb->heapFlag &&  (*size < regCb->heapSize)) {
	/* 
	 * The heap memory block is available. Allocate the memory block from
	 * heap pool.
	 */
	return cmHeapAlloc(&(regCb->heapCb), ptr, size);
    }

    return RC_ERROR;    /* No memory available */

} /* end of cmAlloc */

/*
 * cmFree()
 *
 * Ret:   RC_OK     - successful
 *        RC_ERROR  - unsuccessful.
 *
 * Desc:  The user calls this function to return the previously allocated 
 *        memory block to the memory region. The memory manager does not 
 *        check the validity of the state of the memory block(like whether 
 *        it was allocated earlier). The caller must be sure that, the 
 *        address specified in the parameter 'ptr' is valid and was 
 *        allocated previously from same region.
 */

S16 cmFree(CmMmRegCb *regCb, char *ptr, U32 size)
{
  
    /* error check on parameters */
    if ((regCb == NULL) || (!size) || (ptr == NULL)) {
	return RC_ERROR;
    }

    /* Check if the memory block is from the memory region */
    if (ptr >= (regCb->regInfo.start + regCb->regInfo.size) ) {
	return RC_ERROR;
    }

    
    /* The memory block was allocated from the heap pool */ 
    return cmHeapFree (&(regCb->heapCb), ptr, size);
} /* end of cmFree */


/******************************************************************
 *  same as malloc()
 **************************************************************** */
void *cxMemMalloc(int size)
{
    U8 *p;
    int retSize;
    RCODE ret;

    if ( size <= 0 ) {
	return NULL;
    }

    retSize = size + MEM_HDR_SIZE; /* Actual size to allocate */
    ret = cmAlloc(cxMemCb, &retSize, &p);

    if (ret == RC_OK) {
	/* Record the memory size */
	*((U32 *)(p+MEM_HDR_SIZE-MEM_SIZE_POS)) = size + MEM_HDR_SIZE;

	return (p + MEM_HDR_SIZE);
    }
    else
	return NULL;
}

/******************************************************************
 *  same as calloc()
 **************************************************************** */
void *cxMemCalloc(int n, int size)
{
    U32 retSize;
    U8 *p;
    RCODE ret;

    size = n * size;
    if ( size <= 0 ) {
	return NULL;
    }

    retSize = size + MEM_HDR_SIZE; /* Actual size to allocate */
    ret = cmAlloc(cxMemCb, &retSize, &p);

    if (ret == RC_OK) {
	/* Record the memory size */
	*((U32 *)(p+MEM_HDR_SIZE-MEM_SIZE_POS)) = size + MEM_HDR_SIZE;

	memset( (p + MEM_HDR_SIZE), '\0', size);
	return (p + MEM_HDR_SIZE);
    }
    else
	return NULL;
}

/******************************************************************
 *  same as realloc()
 **************************************************************** */
void *cxMemRealloc(void *p, int size)
{
    U8 *retP;
    int orgSize; /* Original size */

    if ( size <= 0 ) {
		return NULL;
    }

    if (p == NULL) {  /* The same as SMalloc() */
		retP = cxMemMalloc(size);
		return retP;
    }

    if (size == 0) {  /* The same as to free memory */
		cxMemFree(p);
		return NULL;
    }

    retP = cxMemMalloc(size);
    if (retP) {
	orgSize = *(U32 *)((U8 *)p - MEM_SIZE_POS) - MEM_HDR_SIZE;
	(size > orgSize) ? memcpy(retP, p, orgSize) : memcpy(p, retP, size);
    }
    cxMemFree(p);

    return (retP);
}

/******************************************************************
 *  same as free() 
 **************************************************************** */
void cxMemFree(void *p)
{
    U32 size;
    U8 *mp;
    RCODE ret;

    if(!p)
	return;

    mp = (U8 *)p - MEM_HDR_SIZE; /* Shift to original position */

    size = *((U32 *)((U8 *)p - MEM_SIZE_POS));

    if (size > 0) {
	*((U32 *)((U8 *)p - MEM_SIZE_POS)) = 0;

	ret = cmFree(cxMemCb, mp, size);
	
    }

    return;
}

/*
 * This routine compile the memory information to a char string array for
 * GUI application to display.
 *
 * infoStr : char pointer array to hold the information
 * maxLine : The size of the pointer array
 *
 * Return : The number of lines been filled with string
 */
#define LINES 20

int cxMemInfoString(char *infoStr[], int maxLine)
{
	int i = 0, lineNum = 0;
	static int f_strInitialized = 0;
	static char *str[LINES];
    CmMmHeapCb *heapCbP = &(cxMemCb->heapCb);

	sprintf(str[lineNum++], "\nHeap Size %d, Available %d\n",
		cxMemCb->heapSize, heapCbP->avlSize);
    sprintf(str[lineNum++], "Heap Request %d, Failure %d, Last Req %d bytes\n\n",
	   heapCbP->numReq, heapCbP->numFailure, heapCbP->lastReqSize);

	sprintf(str[lineNum++], "Total preallocated mem %d bytes.\n\n", cxMemCb->regInfo.size);

	while(i < lineNum) {
		infoStr[i] = str[i];
		i++;
		if (i == maxLine)
			break;
	}

    return i;
}

void cxMemDisplayInfo()
{
    CmMmHeapCb *heapCbP = &(cxMemCb->heapCb);

/*  
    for(bktInd = 0; bktInd < qwyMemCb->numBkts; bktInd++) {
		bktCbP = &(qwyMemCb->bktTbl[bktInd]);

	printf("Bkt[%d], size=%4d, Total %5d, ",
	       bktInd, bktCbP->size, bktCbP->numBlks);
	printf("Alloc %5d, Max Alloc %5d, Request %d\n",
	       bktCbP->numAlloc, bktCbP->maxAlloc, bktCbP->numReq);
    }
*/

    printf("\nHeap Size %d, Available %d\n",
	   cxMemCb->heapSize, heapCbP->avlSize);
    printf("Heap Request %d, Failure %ld, Success %d times\n\n",
	   heapCbP->numReq, heapCbP->numFailure, heapCbP->numReq - heapCbP->numFailure );
	

    printf("Total preallocated mem %d bytes.\n\n", cxMemCb->regInfo.size);
    return;
}

void cxMemDisplayBktInfo()
{
    int idx, size;

    idx = 0;
    while (cxMemCb->mapTbl[idx].bktIdx != 0xFF) {
	size = (idx << cxMemCb->bktQnPwr);
	if (cxMemCb->mapTbl[idx].numReq > 0)
	    printf("Map %3d, bktIndex %d, size %5d to %5d, req %d, fail %d\n",
		   idx,
		   cxMemCb->mapTbl[idx].bktIdx,
		   size,
		   size+(0x01 << cxMemCb->bktQnPwr),
		   cxMemCb->mapTbl[idx].numReq,
		   cxMemCb->mapTbl[idx].numFailure);

	idx++;
    }

    return;
}
