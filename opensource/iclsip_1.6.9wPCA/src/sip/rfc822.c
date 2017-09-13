/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * rfc822.c
 *
 * $Id: rfc822.c,v 1.41 2006/03/31 07:05:48 tyhuang Exp $
 *
 * Assumption:
 *   1. A message will not across two memory blocks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "rfc822.h"
#include <common/cm_utl.h>
#include <common/cm_trace.h>
#include "sip_cm.h"

MsgHdr msgHdrNew(void)
{
	MsgHdr _this;

	_this = malloc(sizeof(*_this));
	_this->size = 0;
	_this->name = NULL;
	_this->first = NULL;
	_this->last = NULL;
	_this->next = NULL;

	/* -1 for hdrtype */
	_this->hdrtype = -1;

	return _this;
} /* msgHeaderNew */


MsgHdr msgHdrDup(MsgHdr hdr)
{
	MsgHdr _this,tmphdr;
	MsgHdrElm  tmpelm,nextelm=NULL;

	if (!hdr) return NULL;

	_this = calloc(1,sizeof(*_this));
	tmphdr=_this;
	while (hdr && tmphdr) {
		tmphdr->size = hdr->size;
		tmphdr->name = strDup(hdr->name);
		if (NULL!=(tmpelm=hdr->first))
			nextelm=msgHdrElmNew();
		tmphdr->first=nextelm;
		while (tmpelm && nextelm) {
			nextelm->content=strDup(tmpelm->content);
			tmpelm=tmpelm->next;
			if (tmpelm){ 
				nextelm->next=msgHdrElmNew();
				nextelm=nextelm->next;
			}
		}
		tmphdr->last = nextelm;
		hdr=hdr->next;
		if (hdr) {
			tmphdr->next=calloc(1,sizeof(*_this));
			tmphdr=tmphdr->next;
		}
	}
	return _this;
} /* msgHeaderDup */

/* Free this linked list */
void msgHdrFree(MsgHdr hdr)
{
	MsgHdr tmp = hdr, next;
	while(1){ 
		/* keep next for loop */
		next = tmp->next;

		/* free HdrElm linked list */
		msgHdrElmFree(tmp->first);

		if(tmp->name != NULL)
			free(tmp->name);
		free(tmp);
		if (next == NULL)
			break;
		else
			tmp = next;
	}/*end while loop*/
}/* msgHeaderFree */

MsgHdrElm	msgHdrElmNew(void)
{
	MsgHdrElm _this;

	_this = calloc(1,sizeof *_this);

	return _this;
} /* msgHdrElmNew */

void msgHdrElmFree(MsgHdrElm hdrelm)
{
	MsgHdrElm tmp = hdrelm, next;
  
	while(1){
		if (tmp == NULL)
			break;
		else{
			next = tmp->next;
			if(tmp->content){
				free(tmp->content);
				tmp->content=NULL;
			}
			free(tmp);
			tmp = next;
		}
	}/*end while loop*/
} /* msgHdrElmFree */

/*
* isws()
* Returns 1 if character is WS (linear white space, tab or space), 0
* otherwise.
*/
int isws(unsigned char ch)
{
	return ((ch=='\t')||(ch ==' ')||(ch=='\r')||(ch=='\n'));
} /* isws */


/*
* RFC822_ParseLine()
* Extracts header from line for processing. Create a linked list of headers.
* 'php' : list of headers
* Return 0 if ok, < 0 if syntax error.
*/
int rfc822_parseline(unsigned char *line, int linelength,MsgHdr php)
{
	char *s,*p;             /* string pointer */
	/*msgHeader h_list = php;*/
	MsgHdrElm cur_elm; 
	/*unsigned char *buf=NULL,*freeptr=NULL;*/
	/*char buf[SIP_MAX_HDRLENGTH+1]={'\0'};*/
	char *buf;
	int tokenlen = 0;
	/*int hasLWSPchar = 0;*/

	if (!line ||(linelength<1)) return 0;

	/* Mark end of line; if \r\n, remove last \r. */
	if (linelength > 0 && line[linelength-1] == '\r') 
		linelength--;
	line[linelength] = '\0';

	/* get field name */
	/*
	for(s=line;s<line+linelength;s++){
		if (*s==':'){
			php->name = malloc(tokenlen+1);
			php->name[tokenlen] = '\0';
			strncpy(php->name, (const char*)buf, tokenlen);
			tokenlen = 0;
			s++;
			hasLWSPchar = 0;
			break;
		}
		else if (!(isws(*s))){
			buf[tokenlen] = *s;
			tokenlen++;
			hasLWSPchar = 0;
		}
		else if (!hasLWSPchar) {
			buf[tokenlen] = ' ';
			tokenlen++;
			hasLWSPchar = 1;
		}
	}*//*end for loop*/

	/*
	buf=strDup(line);
	if(!buf)
		return 0;
	else
		freeptr=buf;
	*/
	/*sprintf(buf,"%s",line);*/
	if(strlen((const char*)line)>SIP_MAX_HDRLENGTH)
		return 0;
	else{
		/*sprintf(buf,"%s",line)*/
		/*strncpy(buf,line,linelength+1);*/
		buf=(char*)line;
	
	}
	s=strchr((const char*)buf,':');
	if(!s){
		/*
		if(freeptr)
			free(freeptr);
		*/
		return 0;
	}else
		*s='\0';

	php->name = strDup((const char*)trimWS(buf));

	if (php->name == NULL){
		/*
		if(freeptr)
			free(freeptr);
		*/
		return 0; /*every line must have a field name*/
	}
	
	s++;
	p=s;
	while((p=strchr(p,'\t')))
		*p++=' ';
	
	p=s;
	while((p=strchr(p,'\r')))
		*p++=' ';

	p=s;
	while((p=strchr(p,'\n')))
		*p++=' ';
	
	/* extract field body element*/
	/*for(;s<=line+linelength;s++){*/
	while(1){
		/* skip double quote
		if (*s=='"') {
			buf[tokenlen] = *s;
			tokenlen++;
			hasLWSPchar = 0;
			s++;
			for(;s<=line+linelength;s++) {
				buf[tokenlen] = *s;
				tokenlen++;
				hasLWSPchar = 0;
				if( *s!='"' ) 
					continue;
				else
					break;
			}
			s++;
		}*/

		/* get , otherwise  \0 */
		p=strchr(s,',');
		if (!p) 
			p=strchr(s,'\0');
		linelength=p-s+1;
		
		/*if (*s==',' || *s=='\0'){*/
		if (*p==',' || *p=='\0'){
			tokenlen=linelength-1;
			cur_elm = msgHdrElmNew();
			cur_elm->content = calloc(linelength,sizeof(char));
			/*
			cur_elm->content[tokenlen] = '\0';
			cur_elm->next = NULL;
			*/
			/*strncpy((char*)cur_elm->content, (const char*)buf, tokenlen);*/
			strncpy((char*)cur_elm->content,(const char*)s, tokenlen);
			s=p;
			tokenlen = 0;
			if (php->size == 0){
				php->first = cur_elm;
				php->last = cur_elm;
				php->size++;
			}
			else{
				php->last->next = cur_elm;
				php->last = cur_elm;
				php->size++;
			}
			if(*s=='\0')
				break;
			s++;
		}
		/*
		else if (!(isws(*s))){
			buf[tokenlen] = *s;
			tokenlen++;
			hasLWSPchar = 0;
		}
		else if (!hasLWSPchar) {
			buf[tokenlen] = ' ';
			tokenlen++;
			hasLWSPchar = 1;
		}
		
		buf[tokenlen] = *s;
		tokenlen++;*/
	} /*end for(;s<=line+linelength;s++) */
	/*
	if(freeptr)
			free(freeptr);
	*/
	return 1;
} /* RFC822_ParseLine() */

/* Parse SIP Message header, not including requestline & status line */
int rfc822_parse(unsigned char *head, int *headerlength, MsgHdr php)
{
	unsigned char *lineend, *linestart = head;
	int linelength = 0;     /* length of line to be processed */
	int lf = 0;             /* number of LFs in a row */
	int retval = 1;
	MsgHdr tmp = php;


	*headerlength=0;
	for (lineend = head; (*lineend && retval) > 0; lineend++){
	
		lineend=(unsigned char*)strChr(lineend,'\n');
		if( !lineend ) {
			TCRPrint(TRACE_LEVEL_WARNING,"line break is not found!");
			break;
		}
		if (*lineend == '\n'){
			if (lineend == head)
				return -1; /*Leading LF*/ /*should discard this, and continue Oct.29*/
			else {
				/*if (*(lineend-1) == '\r') *//*CRLF appear together */
				/* line break is \n only ,\r is not necessary indeed */
					lf++; /* Find the first CRLR */
			}
		}/*end if(*lineend == '\n') */

		linelength=lineend-linestart;
		switch (lf){
			case 1:
				switch (*(lineend+1)){ 
					case ' ': /* folding line*/
					case '\t': /* folding line, Horizontal Tab */
						linelength++;
						lf = 0;
						break;
					case '\r': /* CR,Carriage return, 0x0d */
					case '\n': /* LF,New line, 0x0a */
						linelength++;
						break;
					default:
						if (tmp->name!=NULL) { /*tmp->name == NUL means the first msgHeader */
							tmp->next = msgHdrNew();
							tmp = tmp->next;
						}
						retval = rfc822_parseline(linestart, linelength, tmp);
						linelength = 0;
						linestart = lineend+1;
						lf = 0;
				}
				break;
			case 2: 
				*headerlength = (int)((char *)(lineend - head)) + 1; /*include head and tail*/
				if (tmp->name!=NULL){
					tmp->next = msgHdrNew();
					tmp = tmp->next;
				}
				retval = rfc822_parseline(linestart, linelength, tmp);
				break;
			default: /* Just a regular character; count it.*/
				linelength++;
		} /*end siwthch(lf)*/
		/* After 2 LFs, we've reached the end of header.*/
		if (lf == 2)
			break;
	} /*end for lineend */

	/*  if (lf < 2) */
		/*    return 0;*/

	return 1;
} /* RFC822_Parse */

int msgHdrAsStr(MsgHdr hdr,DxStr buf)
{
	DxStr tmpbuf;
	int idx;
	MsgHdrElm ptrHdrElm;

	tmpbuf=dxStrNew();
	dxStrCat(tmpbuf,(const char*)hdr->name);
	dxStrCat(tmpbuf,":");
	ptrHdrElm=hdr->first;
	for(idx=0;idx<hdr->size;idx++){
		if(ptrHdrElm != NULL)
			dxStrCat(tmpbuf,(const char*)ptrHdrElm->content);
		if(idx != (hdr->size-1))
			dxStrCat(tmpbuf,",");
		if(ptrHdrElm->next != NULL)
			ptrHdrElm=ptrHdrElm->next;
		else
			break;
	}
	dxStrCat(tmpbuf,"\r\n");
	dxStrCat(buf,(const char*)dxStrAsCStr(tmpbuf));
	dxStrFree(tmpbuf);
	return 1;
}


int rfc822_extractbody(unsigned char *header, MsgHdr php, MsgBody pby)
{
	MsgHdrElm hdrelm;
	int len=0;
	/* find the length of body */
	while (1){
		if ( !strICmp((const char*)php->name,"Content-Length")||!strICmp((const char*)php->name,"l") ) {
			hdrelm = php->first;
			pby->length = atoi((const char*)hdrelm->content);
			break;
		}
		else
			php = php->next;
		/* can't find Content-Length */
		if (php == NULL)
			return 0;
	}/*end while */
	if ((pby->length > 0)&& header){
		/*pby->content = (unsigned char *)calloc((pby->length)+1,1);*/
		len=strlen((const char*)header);
		
		if(len<pby->length){
			/*
			strncpy((char*)pby->content,(const char*) header, len);
			pby->content[len] = '\0';
			*/
			pby->length=len;
			TCRPrint(1, "*** [rfc822_extractbody] content-length does not match message body!\n");
		}else
			len=pby->length;

		pby->content = (unsigned char *)calloc(len+1,1);
		strncpy((char*)pby->content,(const char*) header, len);		
		/*pby->content[pby->length] = '\0';*/
	}
	else
		pby->content = NULL;
	return 1;
} /* RFC822_ExtractBody */


