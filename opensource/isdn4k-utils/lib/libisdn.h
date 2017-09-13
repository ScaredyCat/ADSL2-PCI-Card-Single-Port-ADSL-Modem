
/* $Id: libisdn.h,v 1.14 2002/01/31 20:03:59 paul Exp $
 *
 * ISDN accounting for isdn4linux.
 *
 * Copyright 1997 by Stefan Luethje (luethje@sl-gw.lake.de)
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
 * $Log: libisdn.h,v $
 * Revision 1.14  2002/01/31 20:03:59  paul
 * Add "#define CCODE_OTHER 9999" for defining behaviour in
 * non-specified countries.
 *
 * Revision 1.13  2000/09/05 08:05:03  paul
 * Now isdnlog doesn't use any more ISDN_XX defines to determine the way it works.
 * It now uses the value of "COUNTRYCODE = 999" to determine the country, and sets
 * a variable mycountrynum to that value. That is then used in the code to set the
 * way isdnlog works.
 * It works for me, please check it! No configure.in / doc changes yet until
 * it has been checked to work.
 * So finally a version of isdnlog that can be compiled and distributed
 * internationally.
 *
 * Revision 1.12  1999/10/26 18:17:17  akool
 * isdnlog-3.58
 *   - big cleanup ( > 1.3 Mb removed!)
 *   - v0.02 of destination support - better, but not perfect
 *     (does't work with gcc-2.7.2.3 yet - use egcs!)
 *
 * Revision 1.11  1999/08/20 19:43:48  akool
 * removed avon-, vorwahl- and areacodes-support
 *
 * Revision 1.10  1998/10/13 21:53:33  luethje
 * isdnrep and lib: bugfixes
 *
 * Revision 1.9  1998/05/11 19:43:51  luethje
 * Some changes for "vorwahlen.dat"
 *
 * Revision 1.8  1998/05/10 22:12:04  luethje
 * Added support for VORWAHLEN2.EXE
 *
 * Revision 1.7  1997/05/19 22:58:29  luethje
 * - bugfix: it is possible to install isdnlog now
 * - improved performance for read files for vbox files and mgetty files.
 * - it is possible to decide via config if you want to use avon or
 *   areacode.
 *
 * Revision 1.6  1997/04/08 21:57:07  luethje
 * Create the file isdn.conf
 * some bug fixes for pid and lock file
 * make the prefix of the code in `isdn.conf' variable
 *
 * Revision 1.5  1997/04/08 00:02:25  luethje
 * Bugfix: isdnlog is running again ;-)
 * isdnlog creates now a file like /var/lock/LCK..isdnctrl0
 * README completed
 * Added some values (countrycode, areacode, lock dir and lock file) to
 * the global menu
 *
 * Revision 1.4  1997/03/20 00:28:02  luethje
 * Inserted lines into the files for the revision tool.
 *
 */

#ifndef _LIB_H_
#define _LIB_H_

#include <linux/limits.h>

#include "policy.h"
#include "conffile.h"

#if 0 /* DELETE_ME AK:18-Aug-99 */
#include "areacode/areacode.h"
#include "avon/createDB.h"
#endif

/****************************************************************************/

extern char *basename __P((__const char *__name));

/****************************************************************************/

#ifndef NUMBER_SIZE
#	define NUMBER_SIZE 40
#endif

#ifndef CONFDIR_VAR
#	define CONFDIR_VAR "ISDN_CONF_PATH"
#endif

#define C_SLASH '/'
/* #define C_SLASH '\\' */

#ifndef TMPDIR
# define TMPDIR P_tmpdir
#endif

#ifndef I4LCONFDIR
# define I4LCONFDIR "/etc/isdn"
#endif

#ifndef RUNDIR
# define RUNDIR "/var/run"
#endif

#ifndef LOCKDIR
# define LOCKDIR "/var/lock"
#endif

#ifndef LOCKFILE
# define LOCKFILE "LCK.."
#endif

#ifndef CONFFILE
# define CONFFILE "isdn.conf"
#endif

#ifndef CALLERIDFILE
# define CALLERIDFILE "callerid.conf"
#endif

#ifndef USERCONFFILE
#	define USERCONFFILE "~/.isdn"
#endif

#ifndef S_COUNTRY_PREFIX
# define S_COUNTRY_PREFIX    "+" /* "00" */
#endif

#ifndef S_AREA_PREFIX
# define S_AREA_PREFIX  "0"
#endif

#if 0 /* DELETE_ME AK:18-Aug-99 */
#ifndef S_AREA_DIFF_FILE
# define S_AREA_DIFF_FILE  "vorwahlen.dat"
#endif

#ifndef AVON
# define AVON  "avon"
#endif
#endif

/****************************************************************************/

#define CONF_SEC_GLOBAL         "GLOBAL"
#define CONF_ENT_COUNTRY_PREFIX "COUNTRYPREFIX"
#define CONF_ENT_AREA_PREFIX    "AREAPREFIX"
#define CONF_ENT_COUNTRY        "COUNTRYCODE"
#define CONF_ENT_AREA           "AREACODE"
#if 0 /* Fixme: tools.h defines these - should they be global ??? */
#define CONF_ENT_VBN            "VBN"
#define CONF_ENT_VBNLEN         "VBNLEN"
#define CONF_ENT_PRESELECT      "PRESELECTED"
#endif

#if 0 /* DELETE_ME AK:18-Aug-99 */
#define CONF_ENT_AREALIB        "AREALIB"
#define CONF_ENT_AVONLIB        "AVON"
#define CONF_ENT_CODELIB        "CODELIB"
#define CONF_ENT_AREADIFF       "AREADIFF"
#endif

#define CONF_SEC_VAR    "VARIABLES"

#define CONF_SEC_NUM    "NUMBER"
#define CONF_SEC_MSN    "MSN"
#define CONF_ENT_NUM    "NUMBER"
#define CONF_ENT_ALIAS  "ALIAS"
#define CONF_ENT_ZONE   "ZONE"
#define CONF_ENT_INTFAC "INTERFACE"
#define CONF_ENT_SI     "SI"
#define CONF_ENT_START  "START"

/****************************************************************************/

#define C_NUM_DELIM  ','

/****************************************************************************/

#define C_NO_ERROR   1
#define C_NO_WARN    2
#define C_NO_EXPAND  4

/****************************************************************************/

#define START_PROG 1
#define STOP_PROG  2

/****************************************************************************/

/* some country codes to change behaviour of isdnlog in different countries */
#define	CCODE_NL	 31
#define	CCODE_CH	 41
#define	CCODE_AT	 43
#define	CCODE_DE	 49
#define	CCODE_LU	352
#define	CCODE_OTHER    9999

/****************************************************************************/

#if 0 /* DELETE_ME AK:18-Aug-99 */
#define AREA_ERROR   -1
#define AREA_UNKNOWN  0
#define AREA_LOCAL    1
#define AREA_R50      2
#define AREA_FAR      3
#define AREA_ABROAD   4
#endif

/****************************************************************************/

#ifdef _ISDNTOOLS_C_
#define _EXTERN
#define SET_NULL           = ""
#define SET_NULL2          = NULL
#define SET_AREA_PREFIX    = S_AREA_PREFIX
#define SET_COUNTRY_PREFIX = S_COUNTRY_PREFIX
#else
#define _EXTERN extern
#define SET_NULL
#define SET_NULL2
#define SET_AREA_PREFIX
#define SET_COUNTRY_PREFIX
#endif

_EXTERN char    *mycountry     SET_NULL;
_EXTERN int	mycountrynum;
_EXTERN char    *myarea        SET_NULL;
_EXTERN char    *areaprefix    SET_AREA_PREFIX;
_EXTERN char    *countryprefix SET_COUNTRY_PREFIX;
#if 0 /* DELETE_ME AK:18-Aug-99 */
_EXTERN char    *areadifffile  SET_NULL2;
#endif

_EXTERN void set_print_fct_for_lib(int (*new_print_msg)(const char *, ...));
_EXTERN int num_match(char *Pattern, char *number);
_EXTERN char *expand_number(char *s);
_EXTERN char *expand_file(char *s);
_EXTERN char *confdir(void);
_EXTERN int handle_runfiles(const char *_progname, char **_devices, int flag);
_EXTERN int Set_Codes(section* Section);
#if 0 /* DELETE_ME AK:18-Aug-99 */
_EXTERN char *get_areacode(char *code, int *Len, int flag);
#endif
_EXTERN int read_conffiles(section **Section, char *groupfile);
_EXTERN int paranoia_check(char *cmd);
#if 0 /* DELETE_ME AK:18-Aug-99 */
_EXTERN int area_diff(char* _code, char *_diffcode);
_EXTERN const char* area_diff_string(char* number1, char* number2);
#endif

#undef SET_NULL
#undef SET_NULL2
#undef SET_AREA_PREFIX
#undef SET_COUNTRY_PREFIX
#undef _EXTERN

/****************************************************************************/

#endif
