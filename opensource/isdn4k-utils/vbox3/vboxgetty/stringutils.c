/*
** $Id: stringutils.c,v 1.6 1998/11/10 18:36:32 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "stringutils.h"

/************************************************************************* 
 **
 *************************************************************************/

unsigned char *xstrtoupper(unsigned char *text)
{
	int i;

	if (text)
	{
		for (i = 0; i < strlen(text); i++) text[i] = toupper(text[i]);
	}

	return(text);
}

/*************************************************************************/
/** xstrncpy():																			**/
/*************************************************************************/

void xstrncpy(unsigned char *dest, unsigned char *source, int max)
{
	strncpy(dest, source, max);
   
	dest[max] = '\0';
}

/************************************************************************* 
 **
 *************************************************************************/

void xstrncat(unsigned char *dest, unsigned char *source, int max)
{
   if ((max - strlen(dest)) > 0) strncat(dest, source, max - strlen(dest));

   dest[max] = '\0';
}






/*************************************************************************/
/** xstrtol():																				**/
/*************************************************************************/
      
long xstrtol(unsigned char *string, long number)
{
	long  back;
	char *stop;

	if (string)
	{
		back = strtol(string, &stop, 10);
		
		if (*stop == '\0') return(back);
	}

	return(number);
}

/*************************************************************************/
/** xstrtoo():																				**/
/*************************************************************************/

long xstrtoo(unsigned char *string, long number)
{
	long  back;
	char *stop;

	if (string)
	{
		back = strtol(string, &stop, 8);
		
		if (*stop == '\0') return(back);
	}

	return(number);
}
