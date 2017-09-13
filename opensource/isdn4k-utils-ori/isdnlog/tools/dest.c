/*
 * Destination handling
 *
 * Copyright 1999 by Leopold Toetsch <lt@toetsch.at>
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
 * Changes:
 *
 * 1.00 08.10.1999 lt Initial Version
 * 1.01 26.07.2000 lt Added cdb support
 */

/* #define DEBUG */

#define _DEST_C_

#ifdef STANDALONE
# include <stdlib.h>
# include <stddef.h>
# include <stdio.h>
# ifdef __GLIBC__
#  define __USE_GNU  /* for declaration of basename() */
# endif
# include <string.h>
# include <ctype.h>
# include <stdarg.h>
# include <time.h>
# include <unistd.h>
# include <errno.h>

# if !defined(__GLIBC__) && !defined(basename)
extern const char *basename(const char *name);
# endif

#else
# include "isdnlog.h"
# include "tools.h"
#endif

#include "dest.h"
#include "zone/config.h"
#include "zone/common.h"

#ifndef LENGTH
#define LENGTH 256
#endif
#ifndef UNKNOWN
#define UNKNOWN -1
#endif
#ifdef DESTTEST
char   *Strncpy(char *dest, const char *src, int len);
char   *Strncat(char *dest, const char *src, int len);
#endif

#ifndef min
#define min(x,y) (x)<(y)?(x):(y)
#endif

static char version[] = "1.01";
static _DB db;		/* our dest.db */
static int init_ok=0;

typedef struct {
  char number[TN_MAX_NUM_LEN];
  TELNUM num;
  int lru;
} num_cache_t;

#define CACHE_SIZE 10
static num_cache_t num_cache[CACHE_SIZE];

static void add_cache(char *number, TELNUM *num) {
  int i, mlru,m;
  mlru=9999;
  m=0;
  for (i=0; i<CACHE_SIZE; i++) {
    if(!*num_cache[i].number) {
      Strncpy(num_cache[i].number, number, TN_MAX_NUM_LEN);
      memcpy(&num_cache[i].num, num, sizeof(TELNUM));
      break;
    }
    if(num_cache[i].lru<mlru) {
      m=i;
      mlru=num_cache[i].lru;
    }
  }
  if(i==CACHE_SIZE) {
    Strncpy(num_cache[m].number, number, TN_MAX_NUM_LEN);
    memcpy(&num_cache[m].num, num, sizeof(TELNUM));
    num_cache[m].lru=1;
  }
}

static int get_cache(char *number, TELNUM *num) {
  int i;
  for (i=0; i<CACHE_SIZE; i++) {
    if (!num_cache[i].number)
      break;
    num_cache[i].lru--;
    if(strcmp(num_cache[i].number, number) == 0) {
      num_cache[i].lru++;
      memcpy((char*)num+offsetof(TELNUM,scountry),
        (char*)&num_cache[i].num+offsetof(TELNUM,scountry),
	sizeof(TELNUM)-offsetof(TELNUM,scountry));
      return true;
    }
  }
  return false;
}

static void warning(char *fmt,...)
{
  va_list ap;
  char    msg[BUFSIZ];

  va_start(ap, fmt);
  vsnprintf(msg, BUFSIZ, fmt, ap);
  va_end(ap);
#ifdef STANDALONE
  fprintf(stderr, "WARNING: %s\n", msg);
#else
  print_msg(PRT_NORMAL, "WARNING: %s\n", msg);
#endif
}

void    exitDest(void)
{
  CLOSE(db);
  init_ok=0;
}

int     initDest(char *path, char **msg)
{
  static char message[LENGTH];
  char    vinfo[] = "vErSiO";
  datum   key, value;

  if (msg)
    *(*msg = message) = '\0';
  if (init_ok == 1)
    return 0;
  else if(init_ok == -1)
    return -1;
  if (!path || !*path) {
    if (msg)
      snprintf(message, LENGTH,
      "Dest V%s: Error: no destination database specified!", version);
    return init_ok = -1;
  }
  if ((db = OPEN(path, READ)) == 0) {
    if (msg)
      snprintf(message, LENGTH,
	       "Dest V%s: Error: open '%s': '%s'",
	       version, path, GET_ERR);
    return -1;
  }
  /* read info */
  key.dptr = vinfo;
  key.dsize = 7;
  value = FETCH(db, key);
  if (value.dptr == 0) {
    if (msg)
      snprintf(message, LENGTH,
	       "Dest V%s: Error: File '%s': no Vinfo",
	       version, path);
    exitDest();
    return init_ok = -1;
  }
  if (msg)
    snprintf(message, LENGTH,
    "Dest V%s: File '%s' opened fine - %s", version, path, value.dptr);

  if (*dbv == 'G')
    free(value.dptr);
  init_ok = 1;
  return 0;
}

static void append(char *dest, char *what)
{
  if (!what || !*what)
    return;
  if (strcmp(dest, what)) {
    if(*dest)
      Strncat(dest, "/", TN_MAX_SAREA_LEN);
    Strncat(dest, what, TN_MAX_SAREA_LEN);
  }
}

static bool isKey(const char *p)
{
  bool    key;

  if ( (key = !isdigit(*p)) )
    for (; key && *p; p++)
      if(!isupper(*p) && *p != '_' && !isdigit(*p)) { /* e.g. _DEMD1 */
        key = false;
        break;
      }
  return key;
}

int     getDest(char *onumber, TELNUM * num)
{
  bool    first = true;
  size_t  len;
  datum   key, value, nvalue;
  static char unknown[] = "Unknown";
  char   *p, *q, *city = 0, *s, *name;
  int     arealen, countrylen, prefixlen;
  char   *number = strdup(onumber);
  char    dummy[100]; /* V2.7.2.3 kills stack */
  char    tld[4];
  char    dummy2[100]; /* V2.7.2.3 kills stack */

#ifdef DEBUG
  printf("getD. %s\n", number);
#endif
  *dummy = *dummy2 = '\0'; /* for keeping gcc happy */
  *tld='\0';
  if (get_cache(number, num)) {
#ifdef DEBUG
    printf("getD (cache). %s %s\n", number, formatNumber("%f",num));
#endif
    free(number);
    return 0;
  }
  len = strlen(number);
  if (len==2 && isalpha(*number) && isupper(*number))
    Strncpy(tld,number,3);

  if (isdigit(*number)) {
    warning("getDest called with local number '%s'", number);
    add_cache(number, num);
    return UNKNOWN;
  }
  countrylen = arealen = prefixlen = 0;
  num->ncountry = 0;
  num->narea = 0;
  *num->area = '\0';
  *num->sarea = '\0';
  *num->scountry = '\0';
  *num->country = '\0';
  *num->msn = '\0';
  *num->keys = '\0';
  if (len > 1 && !isdigit(number[1]) && !isKey(number))
    city = strdup(number);
again:
  key.dptr = number;
  key.dsize = len;
  value = FETCH(db, key);
again2:
  if (value.dptr != 0) {
/* we have a value:
 * it could be
 *   :RKEY ... pointer to a KEY
 *   :city ... pointer to a city
 *   name;code ... top level entry i.e country
 *   name;codes[;:KEY]  ... region
 *   [#len];code;:KEY ... city
 */
    while (value.dptr && *value.dptr == ':') {
      /* check for city, i.e. lowercase chars */
      if (!isKey(value.dptr + 1)) {
	city = strdup(value.dptr + 1);
#ifdef DEBUG
	printf("C. %s\n", city);
#endif
      }
      else {
	append(num->keys, value.dptr + 1);
        Strncpy(tld,value.dptr+1,3);
      }
      key.dptr = value.dptr + 1;
      key.dsize = value.dsize - 2;	/* w/o : and \x0 */
      nvalue = FETCH(db, key);
      if (*dbv == 'G')
	free(value.dptr);
      value = nvalue;
    }
    /* do we have something valid */
    if (value.dptr == 0 && first) {
      strcpy(num->scountry, unknown);
      strcpy(num->sarea, unknown);
      Strncpy(num->msn, number, TN_MAX_MSN_LEN);
      free(number);
      if (city)
	free(city);
      return 0;
    }
    /* now we must have a name or city */
    first = false;
    s = value.dptr;
    p = strsep(&s, ";");
    /* name or #len or empty */
    name = 0;
#ifdef DEBUG
    printf("1. %s\n", p);
#endif
    if (p && *p) {
      if (*p == '#')
	prefixlen = atoi(p + 1);
      else
	name = strdup(p);
    }

    p = strsep(&s, ";");
    /* codes or empty */
#ifdef DEBUG
    printf("2. %s\n", p);
#endif
    if (p && *p) {
      q = strtok(p, ",");	/* we could have multiple codes */
      if(*number!='+') {
        if (arealen == 0) {	/* first one must be city */
	  arealen = strlen(q);
	  Strncpy(num->area, q, TN_MAX_AREA_LEN);
        }
      }
      else {
        if (arealen == 0) {	/* we take the orig number */
	  arealen = strlen(number);
	  Strncpy(num->area, number, TN_MAX_AREA_LEN);
        }
      }
      if (strstr(num->area, q))	/* only if new number has same prefix */
        countrylen = strlen(q);	/* last one must be country */
    }

    p = strsep(&s, ";");
    /* :KEY or empty */
#ifdef DEBUG
    printf("3. %s\n", p);
#endif
    /* we should be at toplevel i.e country */
    if (!p) {

      append(num->scountry, name);
      if (countrylen && (arealen || prefixlen)) {
	append(num->sarea, city);
	Strncpy(num->country, num->area, countrylen+1);
	num->ncountry = atoi(num->country+1);
	strcpy(num->tld,tld);
	p = num->area + countrylen;
	arealen -= countrylen;
	if(prefixlen)
	  arealen=prefixlen;
	Strncpy(num->area, p, 1 + arealen);
	num->narea = atoi(num->area);
	if (*onumber == '+' && strlen(onumber) > arealen + countrylen)
	  Strncpy(num->msn, onumber + arealen + countrylen, TN_MAX_MSN_LEN);
	add_cache(onumber, num);
      }
    }
    else if (p && *p == ':') {
      /* do we have a  code */
      append(num->sarea, name);
      append(num->keys, p + 1);
      Strncpy(tld,p+1,3);
      key.dptr = p + 1;
      key.dsize = strlen(p + 1);
      nvalue = FETCH(db, key);
      if (*dbv == 'G')
	free(value.dptr);
      value = nvalue;
      goto again2;
    }
    if (*dbv == 'G')
      free(value.dptr);
    free(number);
    if (city)
      free(city);
    return 0;
  }				/* if value */
  else if (first && len && *number == '+') {	/* try shorter nums */
    number[--len] = '\0';
    goto again;			/* I like it */
  }
  if (number)
    free(number);
  if (city)
    free(city);
  return UNKNOWN;
}

#ifdef STANDALONE
char   *Strncpy(char *dest, const char *src, int len)
{
  int     l = strlen(src);

  if (l > len - 1)
    l = len - 1;
  strncpy(dest, src, l);
  dest[l] = '\0';
  return dest;
}
char   *Strncat(char *dest, const char *src, int len)
{
  int     destlen = strlen(dest);

  return Strncpy(dest + destlen, src, len - destlen);
}

#endif

#ifdef DESTTEST

int     main(int argc, char *argv[])
{
  char   *msg;
  TELNUM  num;
  int     i = 1, res;

  if (initDest("./dest" RDBEXT, &msg)) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "%s\n", msg);
  if (argc == 1) {
    fprintf(stderr, "Usage:\n\t%s number|name ...\n", basename(argv[0]));
    exit(EXIT_FAILURE);
  }
  memset(&num, 0, sizeof(num));
  while (--argc) {
    res = getDest(argv[i++], &num);
    printf("%s %s(%d)=%s %s(%s) %s - %s\n",
	   res == 0 ? "Ok." : "Err", num.country, num.ncountry, num.scountry,
	   num.sarea, num.area,
	   num.msn, num.keys);
  }
  exitDest();
  return (EXIT_SUCCESS);
}
#endif
