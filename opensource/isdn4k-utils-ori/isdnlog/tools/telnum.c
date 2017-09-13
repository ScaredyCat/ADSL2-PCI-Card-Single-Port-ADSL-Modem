/* telnum.c
 * (c) 1999 by Leopold Toetsch <lt@toetsch.at>
 *
 * telefon number utils
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
 *
 * Interface
 *
 * void initTelNum(void)
 * ---------------------
 * init the package, call this once on startup
 *
 * int normalizeNumber(char *number, TELNUM *num, int flag)
 * --------------------------------------------------------
 *      number is "[provider] [country] [area] [msn]"
 *      country may be name of country
 *      spaces or '_' are allowd to separate parts of number
 *      fills struct TELNUM *num with parts of number
 *      for flag look at telnum.h
 *      ret 0 .. ok
 *      ret UNKNOWN .. not found
 *
 *      Input number e.g.
 *      10055 0049 30 1234567
 *      100550049301234567
 *      1033 Deutschland 30 1234567
 *      +49 30 1234567
 *      030 1234567
 *      1234567
 *
 * char * formatNumber(char* format, TELNUM* num)
 * ----------------------------------------------
 *      takes a format string
 *      %Np .. Provider
 *      %Nc .. country +49
 *      %NC .. countryname
 *      %Nt .. tld = 2 char isdcode for country
 *      %Na .. area 30 or 030 if no country/Provider
 *      %NA .. areaname
 *      %Nm .. msn
 *      %f .. full +49 30 12356 (Deutschland, Berlin)
 *      %F .. full +49 30/12345, Berlin
 *      %s .. short +49 30 123456
 *      %l .. long +49 30 12356 - Berlin (DE)
 *      %n .. number 004930123456
 *
 *  N is number of chars to skip in format if part is not present
 *  e.g. "%1c %1a %m"
 *      +4930123 => "+49 30 123"
 *      123 => "123" not "  123"
 *
 *
 * char* prefix2provider(int prefix, char* provider)
 * --------------------------------------------------------------
 *  returns formatted provider for prefix
 *
 * int provider2prefix(char* provider, int*prefix)
 * ------------------------------------------------------------
 *      ret len of provider
 *
 * void clearNum(TELNUM *num)
 * --------------------------
 *      call this if you have a number on stack or want to reuse it
 *      normalizeNumber calls this for you
 *
 * void initNum(TELNUM *num)
 *      inits a number with myarea, mycountry
 *      you may set the area yourself prior to calling this
 *
 * TELNUM * getMynum( void )
 *      returns a pointer to defaultnum (myara, mycountry ...)
 *      this number should not be modified
 *
 */

#include "dest.h"

#define DEFAULT (UNKNOWN-1)

/* #define DEBUG 1 */

static TELNUM defnum;

TELNUM *getMynum(void)
{
  return &defnum;
}

static void error(char *fmt,...)
{
  va_list ap;
  char    msg[BUFSIZ];

  va_start(ap, fmt);
  vsnprintf(msg, BUFSIZ, fmt, ap);
  va_end(ap);
#ifdef STANDALONE
  fprintf(stderr, "%s\n", msg);
#else
  print_msg(PRT_ERR, "%s\n", msg);
#endif
}


static void _init(void);

void    initTelNum(void)
{
  if (!vbn)
    vbn = "";
  if (!vbnlen || !*vbnlen) {
    error("VBNLEN not defined.\n\tPlease read isdnlog/README\n");
    exit(1);
  }
  _init();
}				/* pre_init */

static int split_vbn(char **p, TELNUM * num)
{
  int     l;

  if ((l = provider2prefix(*p, &num->nprovider))) {
    Strncpy(num->provider, *p, l + 1);
    *p += l;
    return l;
  }
  return 0;
}

static void clearArea(TELNUM * num, int a)
{
  strcpy(num->area, a == DEFAULT ? defnum.area : "");
  strcpy(num->sarea, a == DEFAULT ? defnum.sarea : "?");
  num->narea = a == DEFAULT ? defnum.narea : a;
}

static inline void clearCountry(TELNUM * num, int c)
{
  *num->scountry = '\0';
  *num->country = '\0';
  num->ncountry = c;
}

int     normalizeNumber(char *target, TELNUM * num, int flag)
{
  char   *origp = strdup(target);
  char   *p = origp;
  int     res = 0;
  char   *q;

  if ((flag & TN_NOCLEAR) == 0)
  clearNum(num);
#if DEBUG
  print_msg(PRT_V, "NN %s (Prov %d)=> ", target, num->nprovider);
#endif
  if (flag & TN_PROVIDER) {
    if (!split_vbn(&p, num)) {
      num->nprovider = pnum2prefix(preselect,0);
    }
    Strncpy(num->provider, getProvider(num->nprovider), TN_MAX_PROVIDER_LEN);
  }
  if (flag & TN_COUNTRY) {
    /* subst '00' => '+' */
    if (p[0] == '0' && p[1] == '0')
      *++p = '+';
    if (getSpecial(p)) {	/* sondernummer */
      goto is_sonder;
    }
    if (!isdigit(*p)) {
      res = getDest(p, num);
      /* isdnrate is coming with +4319430 but this may be a
         sondernummer */
      if (atoi(mycountry + 1) == num->ncountry && (*num->area || *num->msn)) {
	q = malloc(strlen(num->area) + strlen(num->msn) + 1);
	strcpy(q, num->area);
	strcat(q, num->msn);
	if (getSpecial(q)) {	/* sondernummer */
	  clearCountry(num, 0);
	  *num->sarea = '\0';
	  Strncpy(num->area, q, TN_MAX_AREA_LEN);
	  num->narea = atoi(num->area);
	  *num->msn = '\0';
	}
	free(q);
      }
    }
    else {
      if (getSpecial(p)) {	/* sondernummer */
      is_sonder:
	clearCountry(num, 0);
	*num->sarea = '\0';
	Strncpy(num->area, p, TN_MAX_AREA_LEN);
	num->narea = atoi(num->area);
	*num->msn = '\0';
      }
      else {
	clearArea(num, DEFAULT);
	if (*p == '0') {	/* must be distant call in country */
	  q = malloc(strlen(mycountry) + strlen(p));
	  strcpy(q, mycountry);
	  strcat(q, p + 1);
	  free(origp);
	  origp = p = q;
	  if (getSpecial(p)) {	/* sondernummer */
	    goto is_sonder;
	  }
	  res = getDest(p, num);
	}
	else
	  Strncpy(num->msn, p, TN_MAX_MSN_LEN);
      }
    }
  }
  free(origp);
#if DEBUG
  print_msg(PRT_V, "(%d) %s\n", res, formatNumber("%l Prov %p", num));
#endif
  return (res);
}


int     provider2prefix(char *p, int *prefix)
{
  int     l1, l2 = 0, len, _prefix;
  char   *q;
  char   *vbns = strdup(vbn);

  q = strtok(vbns, ":");
  while (q) {
    l1 = strlen(q);
    if (!memcmp(p, q, l1)) {
/*        Strncpy(num->vbn, q, TN_MAX_VBN_LEN); */
#if DEBUG
      print_msg(PRT_V, "VBN \"%s\"\n", q);
#endif
      if ((_prefix = vbn2prefix(p, &len)) != UNKNOWN) {
	*prefix = _prefix;
	l2 = len;
      }
      break;
    }
    q = strtok(0, ":");
  }
  free(vbns);
  /* now check if resulting num matches any exceptions */
  getPrsel(p+l2, prefix, 0, 0); /* check for exceptions */
  return l2;
}

void    initNum(TELNUM * num)
{
  char   *s;

  if (!*num->area)
    Strncpy(num->area, myarea, TN_MAX_AREA_LEN);
  num->ncountry = defnum.ncountry;
  strcpy(num->scountry, defnum.scountry);
  strcpy(num->country, defnum.country);
  num->narea = atoi(num->area);	/* 1.01 */
  s = malloc(strlen(mycountry) + strlen(num->area) + 1);
  strcpy(s, mycountry);
  strcat(s, num->area);
  getDest(s, num);
  free(s);
  strcpy(num->vbn, defnum.vbn);
}

static void _init(void)
{
  char   *s;

  //clearNum(&defnum);
  Strncpy(defnum.area, myarea, TN_MAX_AREA_LEN);
  s = malloc(strlen(mycountry) + strlen(myarea) + 1);
  strcpy(s, mycountry);
  strcat(s, myarea);
  getDest(s, &defnum);
  Strncpy(defnum.vbn, vbn, TN_MAX_VBN_LEN);
  defnum.nprovider=pnum2prefix(preselect,0);
  Strncpy(defnum.provider, getProvider(defnum.nprovider), TN_MAX_PROVIDER_LEN);
}

void    clearNum(TELNUM * num)
{
  memcpy(num, &defnum,	sizeof(TELNUM));
}

/*     %Np .. Provider
 *     %Nc .. country +49
 *     %NC .. countryname
 *     %Na .. area 89 or 089 if no country/Provider
 *     %NA .. areaname
 *     %Nm .. msn
 *     %f .. full +49 89 12356 (Deutschland, Berlin)
 *     %s .. short +48 89 123456
 *     %n .. number 004889123456
 *     N is number of chars to skip in format if part is not present
 *     e.g. "%1c %1a %m"
 *     +4930123 => "+49 30 123"
 *     123 => "123" not "  123"
 */
#define SKIP if(skip>0)	\
			  while(skip-- && *p && p[1]) \
				p++

char   *formatNumber(char *format, TELNUM * num)
{
  char   *s = retstr[++retnum];
  char   *p, *q, *r;
  int     first = 1;
  int     skip;

  retnum %= MAXRET;
  *s = '\0';
  for (p = format, q = s; *p; p++) {
    if (*p == '%') {
      skip = 0;
    again:
      switch (*++p) {
      case 'p':
	if (num->nprovider > 0) {
	  q = stpcpy(q, num->provider);
	  first = 0;
	}
	else
	  SKIP;
	break;
      case 'c':
	if (num->ncountry > 0 && num->country) {
	  q = stpcpy(q, num->country);
	  first = 0;
	}
	else
	  SKIP;
	break;
      case 'C':
	if (num->ncountry > 0)
	  q = stpcpy(q, num->scountry);
	else
	  SKIP;
	break;
      case 't':		/* tld */
	if (*num->tld)
	  q = stpcpy(q, num->tld);
	else
	  SKIP;
	break;
      case 'a':
	if (num->narea > 0) {
/*                      if(first) *q++ = '0'; / sondernummber - no country */
	  q = stpcpy(q, num->area);
	}
	else
	  SKIP;
	break;
      case 'A':
	if (*num->sarea)
	  q = stpcpy(q, num->sarea);
	else
	  SKIP;
	break;
      case 'm':
	if (*num->msn)
	  q = stpcpy(q, num->msn);
	else
	  SKIP;
	break;
      case 'f':
	q = stpcpy(q, formatNumber("%1c %1a %1m (%2C, %A)", num));
	break;

      case 'F':
	if (num->ncountry == defnum.ncountry)
	  q = stpcpy(q, formatNumber("%1c %1a/%1m, %A", num));
	else
	  q = stpcpy(q, formatNumber("%1c %1a/%1m, %2C, %A", num));
	break;

      case 's':
	q = stpcpy(q, formatNumber("%1c %1a %m", num));
	break;
      case 'l':
	q = stpcpy(q, formatNumber("%1c %1a %1m - %1A (%t)", num));
	break;
      case 'n':
	if (num->ncountry > 0) {
	  q = stpcpy(r = ++q, formatNumber("%c%a%m", num));
	  r[0] = r[1] = '0';
	}
	else
	  q = stpcpy(q, formatNumber("%a%m", num));
	break;
      default:
	if (isdigit(*p)) {
	  skip = strtol(p, &p, 10);
	  p--;
	  goto again;
	}
      }
    }
    else
      *q++ = *p;
  }
  *q = '\0';
  return s;
}

#ifdef TEST_TELNUM
int     verbose = 0;
static char *myshortname;

static void init()
{
  auto char *version, **message;

  if (readconfig(myshortname) < 0)
    exit(1);

  if (verbose)
    message = &version;
  else
    message = NULL;

  initHoliday(holifile, message);

  if (verbose && *version)
    print_msg(PRT_V, "%s\n", version);
  initDest("/usr/lib/isdn/dest.gdbm", message);		/* Fixme: */
  if (verbose && *version)
    print_msg(PRT_V, "%s\n", version);

  initRate(rateconf, ratefile, zonefile, message);

  if (verbose && *version)
    print_msg(PRT_V, "%s\n", version);

  initTelNum();
}				/* init */

static void deinit(void)
{
  exitRate();
  exitDest();
  exitHoliday();
}

int     main(int argc, char *argv[], char *envp[])
{
  register int i;
  TELNUM  num;

  myname = argv[0];
  myshortname = basename(myname);

  if (argc > 1) {
    init();
    i = argc - 1;
    argc++;
    while (i--) {
      normalizeNumber(argv[argc], &num);
      printf("%s => %s\n", argv[argc++], formatNumber("%l", &num));
    }
  }
  return EXIT_SUCCESS;
}
#endif
