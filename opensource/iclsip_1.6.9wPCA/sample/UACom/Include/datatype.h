/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 *
 * 
 * datatype.h
 *
 * $Id: datatype.h,v 1.1 2004/12/23 12:06:52 ljchuang Exp $
 */

/* We use standard I/O for debugging.  */
#ifndef CPIM_DATATYPE_H
#define CPIM_DATATYPE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <adt/dx_lst.h>
#include <adt/dx_hash.h>

typedef enum {
	CPIM_PRESENCE,
	CPIM_TUPLE,
	CPIM_STATUS,
	CPIM_CONTACT,
	CPIM_NOTE,
	CPIM_BASIC,
	CPIM_STATUSEXT,
	CPIM_TIMESTAMP,
	CPIM_ELEMENT,
	CPIM_NULL
} TagType;

struct _CPIMpres {
	char* nsname;
	char* entity;
	DxLst tupleLst;
	DxLst noteLst;
	DxLst extLst;
	DxHash nsHash;
};

struct _CPIMstatus {
	short Basic;
	DxLst extLst;
};

struct _CPIMstatusExt {
	char* tag;
	char* val;
};

struct _CPIMcontact {
	float priority;
	char* URI;
};

struct _CPIMtuple {
	char* id;
	char* timestamp;
	struct _CPIMstatus* status;
	struct _CPIMcontact* contact;
	DxLst noteLst;
	DxLst extLst;
};

struct _CPIMelement {
	char* nsname;
	char* tag;
	char* val;
	DxHash attrHash;
	DxLst childLst;
};

struct _stackdata {
	TagType type;
	void* data;
};


#ifdef  __cplusplus
}
#endif

#endif