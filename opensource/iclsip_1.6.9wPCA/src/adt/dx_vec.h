/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * dx_vec.h
 *
 * $Id: dx_vec.h,v 1.10 2004/09/23 04:41:27 tyhuang Exp $
 */
#include <common/cm_def.h>
#include <common/cm_trace.h>

#ifndef VECTOR_H
#define VECTOR_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
	DX_VECTOR_POINTER = 0,
	DX_VECTOR_INTEGER,
	DX_VECTOR_STRING
} DxVectorType;

typedef struct dxVectorObj*	DxVector;

DxVector        dxVectorNew(int initialCapacity, int capacityIncrement, DxVectorType);
void		dxVectorFree(DxVector, void(*elementFreeCB)(void*));
DxVector  	dxVectorDup(DxVector, void*(*elementDupCB)(void*));
int		dxVectorGetSize(DxVector);
DxVectorType	dxVectorGetType(DxVector);
int             dxVectorGetCapacity(DxVector);

/*  Fun: dxVectorTrimToSize
 *
 *  Desc: Trim capability to current size.  
 *
 */
void dxVectorTrimToSize(DxVector);

/*  Fun: dxVectorEnsureCapacity
 *
 *  Desc: Increases the capacity of this vector, if necessary, to ensure that 
 *        it can hold at least the number of components specified by the 
 *        $minCapacity argument. 
 *
 *  Ret: RC_OK, if success
 *       RC_ERROR, if error
 */
RCODE dxVectorEnsureCapacity(DxVector, int minCapacity);

/*  Fun: dxVectorAddElement
 *
 *  Desc: Appends the specified element to the end of this Vector. 
 *
 *  Ret: RC_OK, if success
 *       RC_ERROR, if error
 */
RCODE dxVectorAddElement(DxVector, void* elemt);

/*  Fun: dxVectorAddElementAt
 *
 *  Desc: Inserts the specified element at the specified position in this Vector. 
 *
 *  Ret: RC_OK, if success
 *       RC_ERROR, if error
 */
RCODE dxVectorAddElementAt(DxVector, void* elemt, int position);

/*  Fun: dxVectorGetLastElement
 *
 *  Desc: Returns the last component of the vector. 
 *
 *  Ret: Last element, if success
 *       NULL, if error
 */
void* dxVectorGetLastElement(DxVector);

/*  Fun: dxVectorGetIndexOf
 *
 *  Desc: Searches for the first occurence of the given argument, beginning 
 *        the search at index, and testing for equality as follows, 
 *        DX_VECTOR_POINTER, compare two pointers directly.
 *        DX_VECTOR_INTEGER, compare the integer value.
 *        DX_VECTOR_STRING, use strcmp() function.
 *
 *  Ret: index, if success
 *       -1, if error
 */
int dxVectorGetIndexOf(DxVector, void* elemt, int position);

/*  Fun: dxVectorGetLastIndexOf
 *
 *  Desc: Searches backwards for the specified object, starting from the 
 *        specified index, and returns an index to it.
 *
 *  Ret: index, if success
 *       -1, if error
 */
int dxVectorGetLastIndexOf(DxVector, void* elemt, int position);

/*  Fun: dxVectorGetElementAt
 *
 *  Desc: Returns the component at the specified index.
 *
 *  Ret: Element, if success
 *       NULL, if error
 */
void* dxVectorGetElementAt(DxVector, int position);

/*  Fun: dxVectorRemoveElement
 *
 *  Desc: Removes the first occurrence of $elemt from this vector.
 *        If vector type is DX_VECTOR_POINTER, user is responsible
 *        to free the memory pointed by $elemt.
 *        On the case of DX_VECTOR_INTEGER and DX_VECTOR_STRING,
 *        this function call free the memory directly. 
 *
 *  Ret: RC_OK, if success
 *       RC_ERROR, if error
 */
RCODE dxVectorRemoveElement(DxVector, void* elemt);

/*  Fun: dxVectorRemoveElementAt
 *
 *  Desc: Remove element at a specific position of vecter. 
 *
 *  Ret: Return element value, user should free the memory space 
 *       returned. ( call free() function )
 */
void* dxVectorRemoveElementAt(DxVector, int position);

/*  Fun: dxVectorSetTraceFlag
 *
 *  Desc: Remove element at a specific position of vecter. 
 *
 *  Ret: Return element value, user should free the memory space 
 *       returned. ( call free() function )
 */
void dxVectorSetTraceFlag(DxVector, TraceFlag traceflag);

#ifdef  __cplusplus
}
#endif

#endif /* DX_VEC_H */
