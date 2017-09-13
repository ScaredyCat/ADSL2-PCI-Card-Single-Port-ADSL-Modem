/* $Id: country.c,v 1.8 2000/02/22 20:04:11 akool Exp $
 *
 * Länderdatenbank
 *
 * Copyright 1995 .. 2000 by Andreas Kool (akool@isdn4linux.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: country.c,v $
 * Revision 1.8  2000/02/22 20:04:11  akool
 * isdnlog-4.13
 *  - isdnlog/tools/rate-at.c ... chg. 1003
 *  - isdnlog/tools/country.c ... no dupl. area warning
 *  - isdnlog/rate-at.dat ... chg. 1003
 *  - isdnlog/tools/dest/pp_rate ... added 'q'
 *  - isdnlog/country-de.dat ... splitted _INM*
 *
 *  - isdnlog/tools/rate.c ... getSpecial, vbn2prefix fixed, include
 *  - isdnlog/tools/dest/pp_rate ... include
 *  - isdnlog/tools/rate-files.man ... include
 *
 *  - new rates, Services (S:, N:) reenabled
 *
 * Revision 1.7  1999/12/31 13:57:19  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *  - resolved *any* warnings against rate-de.dat
 *  - Many new rates
 *  - CREDITS file added
 *
 * Revision 1.6  1999/09/26 10:55:20  akool
 * isdnlog-3.55
 *   - Patch from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 *     added hup3 to option file
 *   - changed country-de.dat to ISO 3166 Countrycode / Airportcode
 *
 * Revision 1.5  1999/06/22 19:41:03  akool
 * zone-1.1 fixes
 *
 * Revision 1.4  1999/06/16 19:12:53  akool
 * isdnlog Version 3.34
 *   fixed some memory faults
 *
 * Revision 1.3  1999/06/15 20:04:58  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.2  1999/06/03 18:51:11  akool
 * isdnlog Version 3.30
 *  - rate-de.dat V:1.02-Germany [03-Jun-1999 19:49:22]
 *  - small fixes
 *
 * Revision 1.1  1999/05/27 18:19:57  akool
 * first release of the new country decoding module
 *
 */

/*
 * Schnittstelle zur Länderdatenbank:
 *
 * void exitCountry(void)
 *   deinitialisiert die Länderdatenbank
 *
 * void initCountry(char *path, char **msg)
 *   initialisiert die Länderdatenbank
 *
 * int getCountry (char *name, COUTRY **country)
 *   sucht das Land oder die Vorwahl *name und
 *   stellt den Eintrag in **country zur Verfügung.
 *   Rückgabewert ist der phonetische Abstand
 *   (0 = exakte Übereinsatimmung)
 *
 * int getCountrycode (char *number, char **name)
 *   sucht die passende Auslandsvorwahl zu *number
 *   liefert den Namen des Landes in *name
 *   Rückgabewert ist die Länge der Vorwahl
 *
 */

#define _COUNTRY_C_

#ifdef STANDALONE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#ifndef __GLIBC__
extern const char *basename (const char *name);
#endif
#else
#include "isdnlog.h"
#include "tools.h"
#endif
#include "country.h"

#define LENGTH 10240  /* max length of lines in data file */
#ifdef STANDALONE
#define UNKNOWN -1
#endif


static COUNTRY *Country = NULL;
static int      nCountry = 0;
static int      line=0;
static int 	verbose=0;

static void warning (char *file, char *fmt, ...)
{
  va_list ap;
  char msg[BUFSIZ];

  va_start (ap, fmt);
  vsnprintf (msg, BUFSIZ, fmt, ap);
  va_end (ap);
#ifdef STANDALONE
  if(verbose)
    fprintf(stderr, "WARNING: %s line %3d: %s\n", basename(file), line, msg);
#else
  print_msg(PRT_NORMAL, "WARNING: %s line %3d: %s\n", basename(file), line, msg);
#endif
}

static char *strip (char *s)
{
  char *p;

  while (isblank(*s)) s++;
  for (p=s; *p; p++)
    if (*p=='#' || *p=='\n') {
      *p='\0';
      break;
    }
  for (p--; p>s && isblank(*p); p--)
    *p='\0';
  return s;
}

static char* str2list (char **s)
{
  static char buffer[BUFSIZ];
  char *p=buffer;

  while (**s) {
    if (**s==',')
      break;
    else
      *p++=*(*s)++;
  }
  *p = '\0';
  return buffer;
}

static char *xlat (char *s)
{
  static char buffer[BUFSIZ];
  char *p = buffer;

  do {
    *p=tolower(*s);
    switch (*p) {
    case 'Ä':
    case 'ä':
      *p++='a';
      *p  ='e';
      break;
    case 'Ö':
    case 'ö':
      *p++='o';
      *p  ='e';
      break;
    case 'Ü':
    case 'ü':
      *p++='u';
      *p  ='e';
      break;
    case 'ß':
      *p++='s';
      *p  ='s';
      break;
    }
    if (isalnum(*p))
      p++;
  } while (*s++);

  return buffer;
}

static int min3(int x, int y, int z)
{
  if (x < y)
    y = x;
  if (y < z)
    z = y;
  return(z);
}

#define WMAX 64
#define P     1
#define Q     1
#define R     1

static int wld(char *needle, char *haystack) /* weighted Levenshtein distance */
{
  int i, j;
  int l1 = strlen(needle);
  int l2 = strlen(haystack);
  int dw[WMAX+1][WMAX+1];

  dw[0][0]=0;

  for (j=1; j<=WMAX; j++)
    dw[0][j]=dw[0][j-1]+Q;

  for (i=1; i<=WMAX; i++)
    dw[i][0]=dw[i-1][0]+R;

  for (i=1; i<=l1; i++)
    for(j=1; j<=l2; j++)
      dw[i][j]=min3(dw[i-1][j-1]+((needle[i-1]==haystack[j-1])?0:P),dw[i][j-1]+Q,dw[i-1][j]+R);

  return(dw[l1][l2]);
}

static COUNTRY *findCountry (char *name)
{
  char *xname;
  int   i, j;

  if (*name=='+') {
    for (i=0; i<nCountry; i++) {
      for (j=0; j<Country[i].nCode; j++) {
	if (strcmp (name, Country[i].Code[j])==0)
	  return &Country[i];
      }
    }
  } else {
    xname=xlat(name);
    for (i=0; i<nCountry; i++) {
      if (strcmp (name, Country[i].Name)==0)
	return &Country[i];
      for (j=0; j<Country[i].nAlias; j++) {
	if (strcmp (xname, Country[i].Alias[j])==0)
	  return &Country[i];
      }
    }
  }
  return NULL;
}

void exitCountry(void)
{
  int i, j;

  for (i=0; i<nCountry; i++) {
    if (Country[i].Name) free (Country[i].Name);
    for (j=0; j<Country[i].nAlias; j++)
      if (Country[i].Alias[j]) free (Country[i].Alias[j]);
    for (j=0; j<Country[i].nCode; j++)
      if (Country[i].Code[j]) free (Country[i].Code[j]);
  }
  if (Country) free (Country);
  Country=NULL;
  nCountry=0;
}

int initCountry(char *path, char **msg)
{
  FILE    *stream;
  COUNTRY *c;
  int      Aliases=0, Codes=0, Index=-1;
  char    *s, *n, *x;
  char     buffer[LENGTH];
  char     version[LENGTH]="<unknown>";
  static char message[LENGTH];

  if (msg)
    *(*msg=message)='\0';

  exitCountry();

  if (!path || !*path) {
    if (msg) snprintf (message, LENGTH, "Warning: no country database specified!");
    return 0;
  }

  if ((stream=fopen(path,"r"))==NULL) {
    if (msg) snprintf (message, LENGTH, "Error: could not load countries from %s: %s",
		       path, strerror(errno));
    return -1;
  }

  line=0;
  while ((s=fgets(buffer,LENGTH,stream))!=NULL) {
    line++;
    if (*(s=strip(s))=='\0')
      continue;
    if (s[1]!=':') {
      warning (path, "expected ':', got '%s'!", s+1);;
      continue;
    }
    switch (*s) {
    case 'N': /* N:Name */
      s+=2; while (isblank(*s)) s++;
      if ((c=findCountry(s))!=NULL) {
	warning (path, "'%s' is an alias for '%s'", s, c->Name);
      }
      Index++;
      nCountry++;
      Country=realloc(Country, nCountry*sizeof(COUNTRY));
      Country[Index].Name=strdup(s);
      Country[Index].nCode=0;
      Country[Index].Code=NULL;
      Country[Index].nAlias=1; /* translated name ist 1st alias */
      Country[Index].Alias=malloc(sizeof(char*));
      Country[Index].Alias[0]=strdup(xlat(s));
      break;

    case 'E': /* E:English */
    case 'A': /* A:Alias[,Alias...] */
      if (Index<0) {
	warning (path, "Unexpected tag '%c'", *s);
	break;
      }
      s+=2;
      while(1) {
	if (*(n=strip(str2list(&s)))) {
	  x=xlat(n);
	  if ((c=findCountry(x))!=NULL) {
	    warning (path, "Ignoring duplicate entry '%s'", n);
	  } else {
	    Country[Index].Alias=realloc (Country[Index].Alias, (Country[Index].nAlias+1)*sizeof(char*));
	    Country[Index].Alias[Country[Index].nAlias]=strdup(x);
	    Country[Index].nAlias++;
	    Aliases++;
	  }
	} else {
	  warning (path, "Ignoring empty alias");
	}
	if (*s==',') {
	  s++;
	  continue;
	}
	break;
      }
      break;

    case 'C': /* C: Code[,Code...] */
      if (Index<0) {
	warning (path, "Unexpected tag '%c'", *s);
	break;
      }
      s+=2;
      while(1) {
	if (*(n=strip(str2list(&s)))) {
	  if (*n!='+') {
	    warning (path, "Code must start with '+'", n);
	  } else if ((c=findCountry(n))!=NULL) {
	    warning (path, "Ignoring duplicate entry '%s'", n);
	  } else {
	    Country[Index].Code=realloc (Country[Index].Code, (Country[Index].nCode+1)*sizeof(char*));
	    Country[Index].Code[Country[Index].nCode]=strdup(n);
	    Country[Index].nCode++;
	    Codes++;
	  }
	} else {
	  warning (path, "Ignoring empty code");
	}
	if (*s==',') {
	  s++;
	  continue;
	}
	break;
      }
      break;

    case 'V': /* V:xxx Version der Datenbank */
      s+=2; while(isblank(*s)) s++;
      strcpy(version, s);
      break;

    case 'R':
    case 'T':
      break;

    default:
      warning(path, "Unknown tag '%c'", *s);
    }
  }
  fclose(stream);

  if (msg) snprintf (message, LENGTH, "Country Version %s loaded [%d countries, %d aliases, %d codes from %s]",
		     version, nCountry, Aliases, Codes, path);

  return nCountry;
}

int getCountry (char *name, COUNTRY **country)
{
  char *xname;
  int   d, i, j, m;

  *country=NULL;
  if (*name=='+') {
    for (i=0; i<nCountry; i++) {
      for (j=0; j<Country[i].nCode; j++) {
	if (strcmp (name, Country[i].Code[j])==0) {
	  *country=&Country[i];
	  return 0;
	}
      }
    }
    return UNKNOWN;
  }

  xname=xlat(name);

  for (i=0; i<nCountry; i++) {
    for (j=0; j<Country[i].nAlias; j++) {
      if (strcmp(xname, Country[i].Alias[j])==0) {
	*country=&Country[i];
	return 0;
      }
    }
  }

  m=666; /* the number of the beast */
  for (i=0; i<nCountry; i++) {
    for (j=0; j<Country[i].nAlias; j++) {
      if ((d=wld(xname, Country[i].Alias[j]))<m) {
	m=d;
	*country=&Country[i];
      }
    }
  }
  return (m==666 ? UNKNOWN : m);
}

int getCountrycode(char *number, char **name)
{
  int i, j, l, m;

  if (name)
    *name="";

  m=UNKNOWN;
  for (i=0; i<nCountry; i++) {
    for (j=0; j<Country[i].nCode; j++) {
      l=strlen(Country[i].Code[j]);
      if (l>m && strncmp (number, Country[i].Code[j], l)==0) {
	m=l;
	if (name) *name=Country[i].Name;
      }
    }
  }
  return m;
}


#ifdef COUNTRYTEST
void main (int argc, char *argv[])
{
  COUNTRY *country;
  char    *msg;
  int      d, i;

  initCountry ("/usr/lib/isdn/country-de.dat", &msg);
//  fprintf (stderr, "%s\n", msg);

  for (i=1; i<argc; i++) {
#if 1
    d=getCountry(argv[i], &country);
    if (country==NULL)
      printf ("<%s> unknown country!\n", argv[i]);
    else
      printf ("<%s>=<%s> d=%d\n", argv[i], country->Name, d);
#else
    d=getCountrycode (argv[i], &msg);
    printf ("<%s>=<%s> d=%d\n", argv[i], msg, d);
#endif
  }
}
#endif
