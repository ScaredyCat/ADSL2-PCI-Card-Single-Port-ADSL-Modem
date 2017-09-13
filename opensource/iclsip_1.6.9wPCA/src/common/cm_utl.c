/*
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cm_utl.c
 *
 * $Id: cm_utl.c,v 1.37 2006/11/21 01:24:00 tyhuang Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "cm_utl.h"

/* return string rep of integer i
 * string is in static memory, so it must be copied before another is
 * converted
 */
char* i2a(int x, char* buf, int buflen)
{
	char temp[12];

	if(!buf||buflen<0)
		return NULL;

	sprintf(temp,"%d",x);
	if( strlen(temp)<(unsigned int)buflen )
		strcpy(buf,temp);
	else
		return NULL;

	return buf;
}

/* copy string s, mapping to uppercase */
char* copyUp(const char* src, char* dst, int dstlen)
{
	/*char* copy;*/
	char* c;

	if( src==NULL || dst==NULL || dstlen<=(int)strlen(src) )
		return NULL;

	/*copy = malloc(strlen(s)+1);*/
	c = dst;

	while( *src != 0 ) {
		(*c)=toupper(*src);
		c++;
		src++;
	}
	(*c) = 0;

	return dst;
}

/* return true if pattern appears in s */
/* ??? check and see if strstr is reasonably efficient
 * probably will be replaced by simple data structure
 */
int strCon(const char* s, const char* pattern)
{
	return s!=NULL && strstr(s,pattern) != NULL;
}

int strICon(const char* s, const char* pattern)
{
	char	*aa,*bb;
	int	ret;

	if( !s||!pattern ) return 0;

	aa = copyUp(s,strDup(s),strlen(s)+1);
	if ( !aa )
		return 0;

	bb = copyUp(pattern,strDup(pattern),strlen(pattern)+1);
	if ( !bb ) {
		free(aa);
		return 0;
	}

	ret = ( strstr(aa,bb)!=NULL );
	free(aa);	free(bb);

	return ret;
}

/* remove whitespace from beginning and end of s, moving characters forward
 * returns s
 */
char* trimWS(char*s)
{
	char* pre;
	char* t;
	int len;

	if( s==NULL ) return s;

	pre = s;
	while( isspace((int)(*pre)) ) pre++;	/* NB !isspace('\0') */

	len=strlen(pre);
	if( pre>s )  /*strcpy(s,pre);*/
		strncpy(s,(const char*)pre,len+1);

	t = s+len;				/* start t at EOS */
	while( s<t && isspace((int)*(t-1)) )	/* if preceding is space, trim */
		t--;

	*t='\0';
	return s;
}

char* quote(char* s)
{
	char	*quoted;

	if( !s )		return NULL;

	quoted = (char*)malloc(strlen(s)+3);
	if( !quoted )
		return NULL;
	quoted[0] = 0;
	strcat(quoted,"\"");
	strcat(quoted,s);
	strcat(quoted,"\"");
	strcpy(s,quoted);
	free( quoted );

	return s;
}

char* unquote(char* s)
{
	if( !s )		return NULL;

	s = trimWS(s);
	if ( *s=='\"' && s[strlen(s)-1]=='\"' ) {
		s[strlen(s)-1] = '\0';
		strcpy( s, s+1 );
	}

	return s;
}

char* unanglequote(char* s)
{
	if( !s )		return NULL;

	s = trimWS(s);
	if ( *s=='<' && s[strlen(s)-1]=='>' ) {
		s[strlen(s)-1] = '\0';
		strcpy( s, s+1 );
	}

	return s;
}

int strICmp(const char* s1, const char* s2)
{

#ifdef _WIN32
#elif defined(UNIX)
#else
	char*	aa;
	char*	bb;
	int	result;
#endif
	
	if( !s1 || !s2 )
		return -1;

#ifdef _WIN32
	return _stricmp(s1,s2);
#elif defined _WIN32_WCE
	return _stricmp(s1,s2);
#elif defined(UNIX)
	return strcasecmp(s1,s2);
#else
	aa = copyUp((char*)s1,strDup(s1),strlen(s1)+1);
	if ( !aa )
		return -1;

	bb = copyUp((char*)s2,strDup(s2),strlen(s2)+1);
	if ( !bb ) {
		free(aa);
		return -1;
	}
	result = strcmp(aa,bb);

	free(aa);
	free(bb);
	return result;
#endif
}

int strICmpN(const char* s1, const char* s2, int n)
{
	char*	aa;
	char*	bb;
	int	result;

	if( !s1 || !s2 )
		return -1;

	aa = copyUp((char*)s1,strDup(s1),strlen(s1)+1);
	if ( !aa )
		return -1;

	bb = copyUp((char*)s2,strDup(s2),strlen(s2)+1);
	if ( !bb ) {
		free(aa);
		return -1;
	}
	result = strncmp(aa,bb,n);

	free(aa);
	free(bb);

	return result;
}

char* strDup(const char* s)
{

#ifdef _WIN32
#elif defined(UNIX)
#else
	char* d=NULL;
#endif

	if( !s )
		return NULL;

#ifdef _WIN32
	return _strdup(s);
#elif defined _WIN32_WCE
	return _strdup(s);
#elif defined(UNIX)
	return strdup(s);
#else

	d = (char*)malloc(strlen(s)+1);
	if( !d )
		return NULL;

	return strcpy(d,s);
#endif

}

char* strAtt(char*dst, const char*src)
{
	char* buff = NULL;

	if (!dst && !src)
		return NULL;
	else if (!dst)
		return (char*)src;
	else if (!src)
		return dst;

	buff = (char*)malloc((strlen(dst)+strlen(src)+1) * sizeof(char));
	if( !buff )
		return NULL;

	sprintf(buff, "%s%s", dst, src);
	buff[(strlen(dst)+strlen(src))] = '\0';

	return buff;
}

/* Copy characters from src to dst, return number copied, dst[n]
 * until the first not in good
 */
int strCpySpn(char* dst, const char* src, const char* good, int n)
{
	int len = 0;

	if (!dst || !src || !good)
		return -1;

	len = strspn(src, good);

	if (len >= n) len = n-1;
	strncpy(dst, src, len);
	dst[len] = '\0';

	return len;
}

/* Copy characters from src to dst, return number copied, dst[n]
 * until the first in bad
 */
int strCpyCSpn(char* dst, const char* src, const char* bad, int n)
{
	int len = 0;

	if (!dst || !src || !bad)
		return -1;

	len = strcspn(src, bad);

	if (len >= n) len = n-1;
	strncpy(dst, src, len);
	dst[len] = '\0';

	return len;
}

char *strChr(unsigned char*s,int c)
{
	if(s)
		return strchr((const char*)s,c);
	else
		return NULL;
}

void* Calloc(size_t num, size_t size)
{
#ifndef _WIN32_WCE
	return calloc(num,size);
#else
	size_t totalsize=num*size;
	void *mem=malloc(totalsize);
	if(NULL==mem)
		return NULL;
	else{
		memset(mem,0,totalsize);
		return mem;
	}
#endif
}


