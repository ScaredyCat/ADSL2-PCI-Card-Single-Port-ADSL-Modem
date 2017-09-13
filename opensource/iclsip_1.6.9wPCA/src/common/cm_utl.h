/*
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * cm_utl.h
 *
 * $Id: cm_utl.h,v 1.16 2006/11/21 01:21:13 tyhuang Exp $
 */

#ifndef CM_UTL_H
#define CM_UTL_H

#include <stdio.h>
#include <stdlib.h>

#define MAXFIELD 100

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef MIN
#undef MIN
#endif
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))

#ifdef MAX
#undef MAX
#endif
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))

#define ISASCII(c)	(((c) & ~0x7f) == 0)
#define TOASCII(c)	((c) & 0x7f)

/* return string rep of integer i
 * string is in static memory, so it must be copied before another is
 * converted
 */
char*	i2a(int x, char* buf, int buflen);

#ifndef a2i
#define a2i(x)	((x)?atoi(x):0)
#endif

#ifndef a2ui
#define a2ui(x)	((x)?strtoul(x,NULL,10):0)
#endif

char*	trimWS(char * s);

/* Notice: quote() will not allocate new memory space to store the quoted string.
           User should make sure that memory space of $s is big enough to insert
           additional two bytes. */
char*	quote(char* s);

char*	unquote(char* s);

/* remove '<' '>' */
char* unanglequote(char* s);

/* return true if pattern appears in s */
int	strCon(const char* s, const char* pattern);

/* case-insensitive version of strCon() */
int	strICon(const char* s, const char* pattern);

/* string case-insensitive compare */
int	strICmp(const char* s1, const char* s2);

/* string case-insensitive n charactors compare */
int	strICmpN(const char* s1, const char* s2, int n);

/* strDup, use malloc() to duplicate string s.
   User should use free() to deallocate memory. */
char*	strDup(const char* s);

/* attach string src to string dst */
char*	strAtt(char*dst, const char*src);

/* Copy characters from src to dst, return number copied, dst[n]
 * until the first not in good
 */
int	strCpySpn(char* dst, const char* src, const char* good, int n);

/* Copy characters from src to dst, return number copied, dst[n]
 * until the first in bad
 */
int	strCpyCSpn(char* dst, const char* src, const char* bad, int n);

char *strChr(unsigned char*s,int c);

void* Calloc(size_t num, size_t size);

#ifdef  __cplusplus
}
#endif

#endif /* CM_UTL_H */

