/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * rfc822.h
 *
 * $Id: rfc822.h,v 1.14 2005/05/04 02:48:30 tyhuang Exp $
 */

#ifndef RFC822_H
#define RFC822_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <adt/dx_str.h>
/*
 * message header(msgHeader) = a list of header fields 
 */

typedef struct msgBodyObj	*MsgBody;    
typedef struct msgHdrElmObj	*MsgHdrElm;
typedef struct msgHeaderObj	*MsgHdr;

struct msgHdrElmObj {
	char*			content;
	struct msgHdrElmObj*	next;
};

struct msgBodyObj {
	int		length;
	unsigned char*	content;
};

struct msgHeaderObj {
	char*			name;
	int			size;
	MsgHdrElm		first;
	MsgHdrElm		last; 
	struct msgHeaderObj*	next;
	/* internal type of sip header*/
	int	hdrtype;
};

MsgHdr		msgHdrNew(void);
MsgHdr		msgHdrDup(MsgHdr hdr);
void		msgHdrFree(MsgHdr);
MsgHdrElm	msgHdrElmNew(void);
void		msgHdrElmFree(MsgHdrElm);
/*int		msgHdrAsStr(MsgHdr,char* buf, int, int*);*/
int		msgHdrAsStr(MsgHdr,DxStr buf);
/* 
 * [in]First parameter: memoey block
 * [out]Second parameter: length of headers
 * [out]Third parameter: structure of headers
 */
int rfc822_parse(unsigned char *, int *, MsgHdr);
int rfc822_parseline(unsigned char*, int, MsgHdr);

/*
 * [in]1: memory block
 * [in]2: structure of header
 * [out]3: structure of body
 */
int rfc822_extractbody(unsigned char*, MsgHdr, MsgBody);

#ifdef  __cplusplus
}
#endif

#endif /* RFC822_H */
