/* $Id: isdnrep.c,v 1.95 2001/03/21 10:24:01 paul Exp $
 *
 * ISDN accounting for isdn4linux. (Report-module)
 *
 * Copyright 1995 .. 2000 by Andreas Kool (akool@isdn4linux.de)
 *                     and Stefan Luethje (luethje@sl-gw.lake.de)
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
 * functions htoi() and url_unescape():
 * Copyright 1994, Steven Grimm <koreth@hyperion.com>.
 *
 *
 * $Log: isdnrep.c,v $
 * Revision 1.95  2001/03/21 10:24:01  paul
 * Previous patch for correctly deleting entries messed up the printing
 * of a specific date (or range), now hopefully fixed. Please test!
 * Also replaced usage of tmpnam() with more secure mkstemp().
 * Reindented some lines, tabstop of 2 is really strange.
 *
 * Revision 1.94  2001/02/08 14:04:53  paul
 * Fix bug where deleting up to 31/12/99 caused ALL entries to be deleted;
 * now the entries being deleted are output (as usual with isdnrep), and
 * the rest are preserved in the log.
 * Also cleaned up error messages to be a bit more consistent.
 *
 * Revision 1.93  2000/08/17 21:34:44  akool
 * isdnlog-4.40
 *  - README: explain possibility to open the "outfile=" in Append-Mode with "+"
 *  - Fixed 2 typos in isdnlog/tools/zone/de - many thanks to
 *      Tobias Becker <tobias@talypso.de>
 *  - detect interface (via IIOCNETGPN) _before_ setting CHARGEINT/HUPTIMEOUT
 *  - isdnlog/isdnlog/processor.c ... fixed wrong init of IIOCNETGPNavailable
 *  - isdnlog/isdnrep/isdnrep.c ... new option -S summary
 *  - isdnlog/isdnrep/rep_main.c
 *  - isdnlog/isdnrep/isdnrep.1.in
 *  - isdnlog/tools/NEWS
 *  - isdnlog/tools/cdb/debian ... (NEW dir) copyright and such from orig
 *  - new "rate-de.dat" from sourceforge (hi and welcome: Who is "roro"?)
 *
 * Revision 1.92  2000/06/22 16:08:24  keil
 * parameter in (...) are allways converted into int
 * newer gcc give an error using char here
 *
 * Revision 1.91  2000/05/27 14:55:30  akool
 * isdnlog-4.25
 *  - isdnlog/isdnrep/isdnrep.c ... bugfix for wrong providers and duration
 *                                  by Hans Klein on d.a.c.i
 *
 *  - isdnlog/tools/rate-at.c ... 1046 Neu, 1002 ab 1.6., 1024
 *  - isdnlog/rate-at.dat ... 1046 Neu, 1002 ab 1.6., 1024
 *  - new rates 01078:3U and 01024:Super_24
 *
 * Revision 1.90  2000/03/06 07:03:20  akool
 * isdnlog-4.15
 *   - isdnlog/tools/tools.h ... moved one_call, sum_calls to isdnrep.h
 *     ==> DO A 'make clean' PLEASE
 *   - isdnlog/tools/telnum.c ... fixed a small typo
 *   - isdnlog/isdnrep/rep_main.c ... incl. dest.h
 *   - isdnlog/isdnrep/isdnrep.c ... fixed %l, %L
 *   - isdnlog/isdnrep/isdnrep.h ... struct one_call, sum_calls are now here
 *
 *   Support for Norway added. Many thanks to Tore Ferner <torfer@pvv.org>
 *     - isdnlog/rate-no.dat  ... NEW
 *     - isdnlog/holiday-no.dat  ... NEW
 *     - isdnlog/samples/isdn.conf.no ... NEW
 *     - isdnlog/samples/rate.conf.no ... NEW
 *
 * Revision 1.89  2000/02/28 19:53:55  akool
 * isdnlog-4.14
 *   - Patch from Roland Rosenfeld <roland@spinnaker.de> fix for isdnrep
 *   - isdnlog/tools/rate.c ... epnum
 *   - isdnlog/tools/rate-at.c ... new rates
 *   - isdnlog/rate-at.dat
 *   - isdnlog/tools/rate-files.man ... %.3f
 *   - doc/Configure.help ... unknown cc
 *   - isdnlog/configure.in ... unknown cc
 *   - isdnlog/.Config.in ... unknown cc
 *   - isdnlog/Makefile.in ... unknown cc
 *   - isdnlog/tools/dest/Makefile.in ... LANG => DEST_LANG
 *   - isdnlog/samples/rate.conf.pl ... NEW
 *   - isdnlog/samples/isdn.conf.pl ... NEW
 *   - isdnlog/rate-pl.dat ... NEW
 *   - isdnlog/tools/isdnrate.c ... fixed -P pid_dir, restarts on HUP now
 *   - isdnlog/tools/isdnrate.man ... SIGHUP documented
 *
 * Revision 1.88  2000/02/07 20:32:41  akool
 * isdnlog-4.09
 *   - NEW: 01078:3U and 010050:Drillisch foreign countries
 *   - isdnlog/isdnrep/isdnrep.c ... moved hist, provider ok again
 *   - isdnlog/isdnrep/CHANGES.isdnrep ... NEW (old changes)
 *   - isdnlog/isdnlog/isdnlog.8.in ... addded signals
 *   - isdnlog/README ... upd. core (SIGSEGV), files
 *
 * Revision 1.87  2000/01/16 12:36:58  akool
 * isdnlog-4.03
 *  - Patch from Gerrit Pape <pape@innominate.de>
 *    fixes html-output if "-t" option of isdnrep is omitted
 *  - Patch from Roland Rosenfeld <roland@spinnaker.de>
 *    fixes "%p" in ILABEL and OLABEL
 *
 * Revision 1.86  1999/12/31 13:57:18  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *  - resolved *any* warnings against rate-de.dat
 *  - Many new rates
 *  - CREDITS file added
 *
 *  for older changes please look at CHANGES
 */


#define _REP_FUNC_C_

#include <sys/param.h>
#include <dirent.h>
#include <search.h>
#include <linux/limits.h>
#include <string.h>

#include "dest.h"
#include "isdnrep.h"
#include "../../vbox/src/libvbox.h"
#include "libisdn.h"

#define END_TIME    1

#define SET_TIME    1
#define GET_TIME    2
#define GET_DATE    4
#define GET_DATE2   8
#define GET_YEAR   16

/*****************************************************************************/

#define GET_OUT     0
#define GET_IN      1

#define GET_BYTES   0
#define GET_BPS     2

/*****************************************************************************/

#define C_BEGIN_FMT '%'

#define FMT_FMT 1
#define FMT_STR 2

/*****************************************************************************/

#define C_VBOX  'v'
#define C_FAX   'f'

#define F_VBOX  1
#define F_FAX   2

/*****************************************************************************/

#define F_BEGIN 1
#define F_END   2

/*****************************************************************************/

#define DEF_FMT "  %X %D %15.15H %T %-15.15F %7u %U %I %O"
#define WWW_FMT "%X %D %17.17H %T %-17.17F %-20.20l SI: %S %9u %U %I %O"

/*****************************************************************************/

#define STR_FAX "Fax: "

/*****************************************************************************/

#define H_ENV_VAR "QUERY_STRING"

/*****************************************************************************/

#define F_1ST_LINE       1
#define F_BODY_HEADER    2
#define F_BODY_HEADERL   4
#define F_BODY_LINE      8
#define F_BODY_BOTTOM1  16
#define F_BODY_BOTTOM2  32
#define F_COUNT_ONLY    64
#define F_TEXT_LINE    128

/*****************************************************************************/

#define H_BG_COLOR     "#FFFFFF"
#define H_TABLE_COLOR1 "#CCCCFF"
#define H_TABLE_COLOR2 "#FFCCCC"
#define H_TABLE_COLOR3 "#CCFFCC"
#define H_TABLE_COLOR4 "#FFFFCC"
#define H_TABLE_COLOR5 "#CCFFFF"

#define H_FORM_ON      "<FORM METHOD=\"put\" ACTION=\"%s\">"
#define H_FORM_OFF     "</FORM>"

#define H_1ST_LINE     "<CENTER><FONT size=+1><B>%s</B></FONT><P>\n"
#define H_TEXT_LINE    "<CENTER><B>%s</B><P>\n"
#define H_BODY_LINE    "<TR>%s</TR>\n"
#define H_BODY_HEADER1 "<TABLE width=%g%% bgcolor=%s border=0 cellspacing=0 cellpadding=0>\n"
#define H_BODY_HEADER2 "<COL width=%d*>\n"
//#define H_BODY_HEADER2 "<COLGROUP span=1 width=%d*>\n"
#define H_BODY_HEADER3 "<TH colspan=%d>%s</TH>\n"
#define H_BODY_BOTTOM1 "<TD align=left colspan=%d>%s</TD>%s\n"
#define H_BODY_BOTTOM2 "</TABLE><P>\n"

#define H_LINE "<TR><TD colspan=%d><HR size=%d noshade width=100%%></TD></TR>\n"

#define H_LEFT         "<TD align=left><TT>%s</TT></TD>"
#define H_CENTER       "<TD align=center><TT>%s</TT></TD>"
#define H_RIGHT        "<TD align=right><TT>%s</TT></TD>"
#define H_LINK         "<A HREF=\"%s?-M+%c%d%s\">%s</A>"
#define H_LINK_DAY     "<A HREF=\"%s?%s\">%s</A>&nbsp;"
#define H_FORM_DAY     "%s<input name=\"%s\" maxlength=%d value=\"\" size=%d><INPUT TYPE=\"submit\" NAME=\"submit\" VALUE=\"go\">"

#define H_EMPTY        "&nbsp;"

#define H_APP_ZYXEL2 "application/x-zyxel2"
#define H_APP_ZYXEL3 "application/x-zyxel3"
#define H_APP_ZYXEL4 "application/x-zyxel4"
#define H_APP_ULAW   "application/x-ulaw"
#define H_APP_TEXT   "text/plain"
#define H_APP_FAX3   "application/x-faxg3"

/*****************************************************************************/

#define	UNKNOWNZONE  MAXZONES

/*****************************************************************************/

typedef struct {
	int   type;
	char *string;
	char  s_type;
	char *range;
} prt_fmt;

typedef struct {
	int    version;
	int    compression;
	int    used;
	time_t time;
	char   type;
	char  *name;
} file_list;

/*****************************************************************************/

static time_t get_month(char *String, int TimeStatus);
static time_t get_time(char *String, int TimeStatus);
static int show_msn(one_call *cur_call);
static int add_sum_calls(sum_calls *s1, sum_calls *s2);
static int print_sum_calls(sum_calls *s, int computed);
static int add_one_call(sum_calls *s1, one_call *s2, double units);
static int clear_sum(sum_calls *s1);
static char *print_currency(double money, int computed);
static void strich(int type);
static int n_match(char *Pattern, char* Number, char* version);
static int set_caller_infos(one_call *cur_call, char *string, time_t from);
static int set_alias(one_call *cur_call, int *nx, char *myname);
static int print_header(int lday);
static int print_entries(one_call *cur_call, double unit, int *nx, char *myname);
static int print_bottom(double unit, char *start, char *stop);
static char *get_time_value(time_t t, int *day, int flag);
static char **string_to_array(char *string);
static prt_fmt** get_format(const char *format);
static char *set_byte_string(int flag, double Bytes);
static int print_line(int status, one_call *cur_call, int computed, char *overlap);
static int print_line2(int status, const char *, ...);
static int print_line3(const char *fmt, ...);
static int append_string(char **string, prt_fmt *fmt_ptr, char* value);
static char *html_conv(char *string);
static int get_format_size(void);
static int set_col_size(void);
static char *overlap_string(char *s1, char *s2);
static char* fill_spaces(char *string);
static void free_format(prt_fmt** ptr);
static int html_bottom(char *progname, char *start, char *stop);
static int html_header(void);
static char *print_diff_date(char *start, char *stop);
static int get_file_list(void);
static int set_dir_entries(char *directory, int (*set_fct)(const char *, const char *));
static int set_vbox_entry(const char *path, const char *file);
static int set_mgetty_entry(const char *path, const char *file);
static int set_element_list(file_list* elem);
static int Compare_files(const void *e1, const void *e2);
static file_list *get_file_to_call(time_t filetime, char type);
static char *get_links(time_t filetime, char type);
static char *append_fax(char **string, char *file, char type, int version);
static int time_in_interval(time_t t1, time_t t2, char type);
static char *nam2html(char *file);
static char *get_a_day(time_t t, int d_diff, int m_diff, int flag);
static char *get_time_string(time_t begin, time_t end, int d_diff, int m_diff);
static char *get_default_html_params(void);
static char *create_vbox_file(char *file, int *compression);
static int htoi(char *s);
static char **get_http_args(char *str, int *index);
static char *url_unescape(char *str);
static int app_fmt_string(char *target, int targetlen, char *fmt, int condition, char *value);
static int find_format_length(char *string);

/*****************************************************************************/

static int      invertnumbers = 0;
static int      unknowns = 0;
static UNKNOWNS unknown[MAXUNKNOWN];
static int      zones[MAXZONES + 1];
static int      zones_usage[MAXZONES + 1];
static char *   zones_names[MAXZONES + 1];
static double   zones_dm[MAXZONES + 1];
static double   zones_dur[MAXZONES + 1];
static char**   ShowMSN = NULL;
static int*     colsize = NULL;
static double   h_percent = 100.0;
static char*    h_table_color = H_TABLE_COLOR1;
static file_list **file_root = NULL;
static int      file_root_size = 0;
static int      file_root_member = 0;
static char    *_myname;
static time_t   _begintime;
static int      read_path = 0;

/*****************************************************************************/

static char msg1[]    = "isdnrep: can't open %s (%s)\n";
static char wrongdate2[]   = "isdnrep: wrong date for delete: %s\n";
static char nomemory[]   = "isdnrep: out of memory!\n";

static char htmlconv[][2][10] = {
	{">", "&gt;"},
	{"<", "&lt;"},
	{" ", H_EMPTY},
	{"" , ""},
};

/*****************************************************************************/

static int Tarif96 = 0;
static int Tarif962 = 0;

static sum_calls day_sum;
static sum_calls day_com_sum;
static sum_calls all_sum;
static sum_calls all_com_sum;

/*****************************************************************************/

static double *msn_sum;
static int    *usage_sum;
static double *dur_sum;

static int    usage_provider[MAXPROVIDER];
static int    provider_failed[MAXPROVIDER];
static double duration_provider[MAXPROVIDER];
static double pay_provider[MAXPROVIDER];
static char   unknownzones[4096];

#undef MAXPROVIDER
#define MAXPROVIDER getNProvider()


/*****************************************************************************/

void info(int chan, int reason, int state, char *msg)
{
  /* DUMMY - dont needed here! */
} /* info */

/*****************************************************************************/

int send_html_request(char *myname, char *option)
{
	char file[PATH_MAX];
	char commandline[PATH_MAX];
	char *filetype = NULL;
	char *command  = NULL;
	char *vboxfile = NULL;
	int  compression;


	if (*option == C_VBOX)
	{
		sprintf(file,"%s%c%s",vboxpath,C_SLASH,option+2);

		if (option[1] == '1')
		{
			if (vboxcommand1)
				command = vboxcommand1;
			else
				filetype = H_APP_ZYXEL4;
		}
		else
		if (option[1] == '2')
		{
			if (vboxcommand2)
				command = vboxcommand2;
			else
			{
				vboxfile = strcpy(file,create_vbox_file(file,&compression));

				switch(compression)
				{
					case 2 : filetype = H_APP_ZYXEL2;
					         break;
					case 3 : filetype = H_APP_ZYXEL3;
					         break;
					case 4 : filetype = H_APP_ZYXEL4;
					         break;
					case 6 : filetype = H_APP_ULAW;
					         break;
					default: printf( "Content-Type: %s\n\n",H_APP_TEXT);
				           printf( "%s: unsupported compression type of vbox file :`%d'\n",myname,compression);
                                           return(UNKNOWN);
					         break;
				}
			}
		}
		else
		{
			printf( "Content-Type: %s\n\n",H_APP_TEXT);
			printf( "%s: unsupported version of vbox `%c'\n",myname,option[0]);
                        return(UNKNOWN);
		}
	}
	else
	if (*option == C_FAX)
	{
		sprintf(file,"%s%c%s",mgettypath,C_SLASH,option+2);

		if (option[1] == '3')
		{
			if (mgettycommand)
				command = mgettycommand;
			else
				filetype = H_APP_FAX3;
		}
		else
		{
			printf( "Content-Type: %s\n\n",H_APP_TEXT);
			printf( "%s:unsupported version of fax `%c%c'\n",myname,option[0],option[1]);
                        return(UNKNOWN);
		}
	}
	else
	{
		printf( "Content-Type: %s\n\n",H_APP_TEXT);
		printf( "%s:invalid option string `%c%c'\n",myname,option[0],option[1]);
                return(UNKNOWN);
	}

	if (command == NULL)
	{
		char precmd[SHORT_STRING_SIZE];

		sprintf(precmd, "echo \"Content-Type: %s\"",filetype);
		system(precmd);
		sprintf(precmd, "echo");
		system(precmd);
	//	printf( "Content-Type: %s\n\n",filetype);
	}

	sprintf(commandline,"%s %s",command?command:"cat",file);
	system(commandline);

	if (vboxfile)
	{
		if (unlink(vboxfile))
		{
			print_msg(PRT_ERR, "isdnrep: can't delete file `%s': %s!\n",file, strerror(errno));
                        return(UNKNOWN);
		}
	}

	return 0;
}

/*****************************************************************************/

int read_logfile(char *myname)
{
  auto	   double     einheit = 0.23;
  auto	   int	      nx[2];
  auto	   time_t     now, from = 0;
  auto 	   struct tm *tm;
  auto     int        lday = UNKNOWN;
  auto	   char	      start[20] = "", stop[20];
  auto     FILE      *fi, *ftmp = NULL;
  auto     char       string[BUFSIZ], s[BUFSIZ];
  one_call            cur_call;


  initHoliday(holifile, NULL);
  initDest(destfile, NULL);
  initRate(rateconf, ratefile, zonefile, NULL);

  interns0 = 3; /* Fixme: */

  msn_sum = calloc(mymsns + 1, sizeof(double));
  usage_sum = calloc(mymsns + 1, sizeof(int));
  dur_sum = calloc(mymsns + 1, sizeof(double));
  *unknownzones = 0;

	_myname = myname;
	_begintime = begintime;

	if (html & H_PRINT_HEADER)
		html_header();

	if (lineformat == NULL)
	{
		if (html)
			lineformat = WWW_FMT;
		else
			lineformat = DEF_FMT;
	}

	if (get_format(lineformat) == NULL)
                return(UNKNOWN);

	/* following two lines must be after get_format()! */
	if (html)
		get_file_list();

  clear_sum(&day_sum);
  clear_sum(&day_com_sum);
  clear_sum(&all_sum);
  clear_sum(&all_com_sum);

	if (knowns == 0 || strcmp(known[knowns-1]->who,S_UNKNOWN))
	{
		if ((known = (KNOWN**) realloc(known, sizeof(KNOWN *) * (knowns+1))) == NULL)
    {
      print_msg(PRT_ERR, nomemory);
      return(UNKNOWN);
    }

		if ((known[knowns] = (KNOWN*) calloc(1,sizeof(KNOWN))) == NULL)
    {
      print_msg(PRT_ERR, nomemory);
      return(UNKNOWN);
    }

    known[knowns]->who = S_UNKNOWN;
    known[knowns++]->num = "0000";
	}

  if (delentries)
  {
    if(begintime)
    {
      print_msg(PRT_ERR, wrongdate2, timestring);
      return(UNKNOWN);
    }
    else
    {
      if ((ftmp = tmpfile()) == NULL)
      {
        print_msg(PRT_ERR, msg1, "tmpfile", strerror(errno));
        return(UNKNOWN);
      }
    }
  }

  if (!timearea) { /* from darf nicht gesetzt werden, wenn alle Telefonate angezeigt werden sollen */
		/* get time of start of today (midnight) */
    time(&now);
    	/* aktuelle Zeit wird gesetzt */
    tm = localtime(&now);
    	/* Zeit von 1970 nach Struktur */
    tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
    from = mktime(tm);
    	/* Struktur nach Zeit von 1970 */
    	/* from hat den aktuellen Tag 0.00 Uhr */
  } /* if */

  if ((fi = fopen(logfile, "r")) == (FILE *)NULL){
  	print_msg(PRT_ERR, msg1, logfile, strerror(errno));
        return(UNKNOWN);
  }

  memset(zones_usage, 0, sizeof(zones_usage));

  while (fgets(s, BUFSIZ, fi)) {
    strcpy(string,s);

		if (*s == '#')
			continue;

		/* set_caller_infos() returns UNKNOWN also if outside time range */
		/* but if we are deleting, then the line must be copied to tempfile */
		/* so no direct "continue;" in that case */

		if (set_caller_infos(&cur_call,s,from) == UNKNOWN) {
		  if (!delentries)
			  continue;
			if (cur_call.t >= endtime) {
				/* past end: stop delete, start copying */
				fputs(string,ftmp);
				continue;
			}
		}

		if (!print_failed && cur_call.duration == 0)
			continue;

		if (!begintime)
			begintime = cur_call.t;

		if (phonenumberonly)
			if (!show_msn(&cur_call))
				continue;

		if (incomingonly && cur_call.dir == DIALOUT)
			continue;

		if (outgoingonly && cur_call.dir == DIALIN)
			continue;

		get_time_value(cur_call.t,&day,SET_TIME);

		/* Andreas, warum ist diese Abfrage hier drin, warum soll das nicht moeglich
		   sein? */
		if (cur_call.duration > 172800.0) /* laenger als 2 Tage? Das KANN nicht sein! */
			cur_call.duration = 0;

		set_alias(&cur_call,nx,myname);

		if (day != lday) {
			if (header)
				print_header(lday);

			lday = day;

			if (!*start)
			{
				sprintf(start, "%s %s", get_time_value(0,NULL,GET_DATE),
				                        get_time_value(0,NULL,GET_YEAR));
				sprintf(stop, "%s %s", get_time_value(0,NULL,GET_DATE),
				                       get_time_value(0,NULL,GET_YEAR));
			}
			else
				sprintf(stop, "%s %s", get_time_value(0,NULL,GET_DATE),
				                       get_time_value(0,NULL,GET_YEAR));
		} /* if */

		if (!cur_call.currency_factor)
		                einheit = 1.0;
			else
				einheit = cur_call.currency_factor;

		print_entries(&cur_call,einheit,nx,myname);

	} /* while */

	fclose(fi);

	if (delentries) /* Erzeugt neue verkuerzte Datei */
    lday = UNKNOWN;

  if (lday == UNKNOWN && html)
	{
		if (begintime == 0)
			begintime = time(NULL);

		get_time_value(begintime,&lday,SET_TIME);
		sprintf(start, "%s %s", get_time_value(0,NULL,GET_DATE),
		                        get_time_value(0,NULL,GET_YEAR));

		if (endtime) {
		get_time_value(endtime,&lday,SET_TIME);
		sprintf(stop, "%s %s", get_time_value(0,NULL,GET_DATE),
		                       get_time_value(0,NULL,GET_YEAR));
		}
		else {
		  strcpy(stop, start);
		}

		print_line2(F_1ST_LINE,"I S D N  Connection Report");
		print_line2(F_TEXT_LINE,"");
		print_line2(F_TEXT_LINE,"no calls from %s to %s", start, stop);
		print_line2(F_TEXT_LINE,"");
                lday = UNKNOWN;
	}

  if (lday != UNKNOWN && header)
		print_bottom(einheit, start, stop);

	if (delentries) /* Erzeugt neue verkuerzte Datei */
	{
		rewind(ftmp);

		if ((fi = fopen(logfile, "w")) == (FILE *)NULL)
		{
			print_msg(PRT_ERR, msg1, logfile, strerror(errno));
      return(UNKNOWN);
		}

		while (fgets(s, BUFSIZ, ftmp))
			fputs(s,fi);

		fclose(fi);
		fclose(ftmp);
	}

	if (html & H_PRINT_HEADER)
		html_bottom(myname,start,stop);

	return 0;
} /* read_logfile */

/*****************************************************************************/

static int print_bottom(double unit, char *start, char *stop)
{
  auto     char       string[BUFSIZ], sx[BUFSIZ];
  register int	      i, j, k;
  register char      *p = NULL;
  sum_calls           tmp_sum;
  auto     double     s = 0.0, s2 = 0.0;
  auto	   int	      s1 = 0;


	if (timearea && summary < 2) {
		strich(1);
		print_sum_calls(&day_sum,0);

		if (day_com_sum.eh)
		{
			print_sum_calls(&day_com_sum,1);

			clear_sum(&tmp_sum);
			add_sum_calls(&tmp_sum,&day_sum);
			add_sum_calls(&tmp_sum,&day_com_sum);
			strich(1);
			print_sum_calls(&tmp_sum,0);
		}
		else
			printf("\n");
	} /* if */

	add_sum_calls(&all_sum,&day_sum);
	add_sum_calls(&all_com_sum,&day_com_sum);

	strich(2);
	print_sum_calls(&all_sum,0);

	if (bill)
          return(0);

	if (all_com_sum.eh)
	{
		print_sum_calls(&all_com_sum,1);

		clear_sum(&tmp_sum);
		add_sum_calls(&tmp_sum,&all_sum);
		add_sum_calls(&tmp_sum,&all_com_sum);
		strich(2);
		print_sum_calls(&tmp_sum,0);
	}
	else
		printf("\n");

	print_line2(F_BODY_BOTTOM2,"");

	get_format("%-14.14s %4d call(s) %10.10s  %12s %-12s %-12s");

	for (j = 0; summary < 2 && j < 2; j++)
	{
		if ((j == DIALOUT && !incomingonly) || (!outgoingonly && j == DIALIN))
		{
			sprintf(string, "%s Summary for %s", (j == DIALOUT) ? "Outgoing calls (calling:)" : "Incoming calls (called by:)",
			              print_diff_date(start,stop));

			h_percent = 80.0;
			h_table_color = H_TABLE_COLOR2;
			print_line2(F_BODY_HEADER,"");
			print_line2(F_BODY_HEADERL,"%s",string);
			strich(1);

			for (i = 0 ; i < knowns; i++) {
				if (known[i]->usage[j]) {
					print_line3(NULL,
					          /*!numbers?*/known[i]->who/*:known[i]->num*/,
					          known[i]->usage[j],
					          double2clock(known[i]->dur[j]),
                                                  j==DIALOUT?print_currency(known[i]->pay,0):
                                                             fill_spaces(print_currency(known[i]->pay,0)),
					          set_byte_string(GET_IN|GET_BYTES,known[i]->ibytes[j]),
					          set_byte_string(GET_OUT|GET_BYTES,known[i]->obytes[j]));
				} /* if */
			} /* for */

			print_line2(F_BODY_BOTTOM2,"");
		}
	}

	if (!incomingonly)
	{
		h_percent = 60.0;
		h_table_color = H_TABLE_COLOR3;
		get_format("%-21.21s %4d call(s) %10.10s  %12s");
		print_line2(F_BODY_HEADER,"");
		/* Fixme: zones are provider-specific
		   we are summing up zones for all provides here */
		print_line2(F_BODY_HEADERL,"Outgoing calls ordered by Zone");
		strich(1);

                for (i = 0; i < MAXZONES /* + 1 */; i++)
                  if (zones_usage[i]) {
                    auto     char  s[BUFSIZ];

                    sprintf(s, "Zone %3d:%s", i, zones_names[i]);

                    print_line3(NULL, s, zones_usage[i],
                      double2clock(zones_dur[i]),
                      print_currency(zones_dm[i], 0));
			} /* if */

#if DEBUG
                if (zones_usage[UNKNOWNZONE])
                  printf( "(%s)\n", unknownzones);
#endif

		print_line2(F_BODY_BOTTOM2,"");

		h_percent = 60.0;
		h_table_color = H_TABLE_COLOR4;
                get_format("%-8.8s %05s %-25.25s %4d call(s) %10.10s  %12s %s");
		print_line2(F_BODY_HEADER,"");
		print_line2(F_BODY_HEADERL,"Outgoing calls ordered by Provider");
		strich(1);

		for (i = 0; i < MAXPROVIDER; i++) {
                  prefix2provider(i, string);
		  if (usage_provider[i]) {
                    if (duration_provider[i])
                      sprintf(sx, "%5.1f%% avail.",
                        100.0 * (usage_provider[i] - provider_failed[i]) / usage_provider[i]);
                    else
                      *sx = 0;
    		   p = getProvider(i);
    		   if (!p || p[strlen(p) - 1] == '?') /* UNKNOWN Provider */
                      p = "UNKNOWN";

		    print_line3(NULL, "Provider", string, p,
		      usage_provider[i],
		      double2clock(duration_provider[i]),
                      print_currency(pay_provider[i], 0), sx);
       } /* if */

    } /* for */

		print_line2(F_BODY_BOTTOM2,"");

		h_percent = 60.0;
		h_table_color = H_TABLE_COLOR5;
		get_format("%-30.30s %4d call(s) %10.10s  %12s");
		print_line2(F_BODY_HEADER,"");
		print_line2(F_BODY_HEADERL,"Outgoing calls ordered by MSN");
		strich(1);

		for (k = 0; k <= mymsns; k++) {
			if (msn_sum[k]) {

				print_line3(NULL, ((k == mymsns) ? S_UNKNOWN : known[k]->who),
				  usage_sum[k],
				  double2clock(dur_sum[k]),
				  print_currency(msn_sum[k], 0));

				s += msn_sum[k];
				s1 += usage_sum[k];
				s2 += dur_sum[k];
			} /* if */
 		} /* for */
	}

#if 0
  if (s) {
		strich(2);
		print_line3(NULL, "TOTAL", s1, double2clock(s2), print_currency(s, 0));
  } /* if */
#endif

	print_line2(F_BODY_BOTTOM2,"");

	if (seeunknowns && unknowns) {
		printf("\n\nUnknown caller(s)\n");
		strich(3);

		for (i = 0; i < unknowns; i++) {
#if 0
			printf("%s %-14s ", unknown[i].called ? "called by" : "  calling", unknown[i].num);
#else
                  if ((unknown[i].cause != 1) &&  /* Unallocated (unassigned) number */
                      (unknown[i].cause != 3) &&  /* No route to destination */
                      (unknown[i].cause != 28)) { /* Invalid number format (address incomplete) */

                       printf("%s ", unknown[i].called ? "Called by" : "  Calling");

                       printf("??? %s\n\t\t\t ", unknown[i].num);
                  } /* if */
#endif
			for (k = 0; k < unknown[i].connects; k++) {
				strcpy(string, ctime(&unknown[i].connect[k]));

				if ((p = strchr(string, '\n')))
					*p = 0;

				*(string + 19) = 0;

				if (k && (k + 1) % 2) {
					printf("\n\t\t\t ");
				} /* if */

				printf("%s%s", k & 1 ? ", " : "", string + 4);
			} /* for */

			printf("\n");
		} /* for */
	} /* if */

	return 0;
}

/*****************************************************************************/

static int print_line3(const char *fmt, ...)
{
	char *string = NULL;
	char  tmpstr[BUFSIZ*3];
	auto  va_list ap;
	prt_fmt** fmtstring;


	if (fmt == NULL)
		fmtstring = get_format(NULL);
	else
		fmtstring = get_format(fmt);

	if (fmtstring == NULL)
                return(UNKNOWN);

	va_start(ap, fmt);

	while(*fmtstring != NULL)
	{
		if ((*fmtstring)->type == FMT_FMT)
		{
			switch((*fmtstring)->s_type)
			{
				case 's' : append_string(&string,*fmtstring,va_arg(ap,char*));
				           break;
				case 'c' : sprintf(tmpstr,"%c",va_arg(ap,int));
				           append_string(&string,*fmtstring,tmpstr);
				           break;
				case 'd' : sprintf(tmpstr,"%d",va_arg(ap,int));
				           append_string(&string,*fmtstring,tmpstr);
				           break;
				case 'f' : sprintf(tmpstr,"%f",va_arg(ap,double));
				           append_string(&string,*fmtstring,tmpstr);
				           break;
                                default  : print_msg(PRT_ERR, "isdnrep: internal Error: unknown format `%c'!\n",(*fmtstring)->s_type);
				           break;
			}
		}
		else
		if ((*fmtstring)->type == FMT_STR)
		{
			append_string(&string,NULL,(*fmtstring)->string);
		}
		else
			print_msg(PRT_ERR, "isdnrep: internal Error: unknown format type `%d'!\n",(*fmtstring)->type);

		fmtstring++;
	}

	va_end(ap);

	print_line2(F_BODY_LINE,"%s",string);
	free(string);
	return 0;
}

/*****************************************************************************/

static int print_line2(int status, const char *fmt, ...)
{
	char string[BUFSIZ*3];
	auto va_list ap;


	va_start(ap, fmt);
	vsnprintf(string, BUFSIZ*3, fmt, ap);
	va_end(ap);

	if (!html)
		status = 0;

	switch (status)
	{
		case F_COUNT_ONLY  : break;
		case F_1ST_LINE    : printf(H_1ST_LINE,string);
		                     break;
		case F_TEXT_LINE    : printf(H_TEXT_LINE,string);
		                     break;
		case F_BODY_BOTTOM1:
		case F_BODY_LINE   : printf(H_BODY_LINE,string);
		                     break;
		case F_BODY_HEADER : printf(H_BODY_HEADER1,h_percent,h_table_color);
		                     set_col_size();
		                     break;
		case F_BODY_HEADERL: printf(H_BODY_HEADER3,get_format_size(),string);
		                     break;
		case F_BODY_BOTTOM2: printf(H_BODY_BOTTOM2);
		                     break;
		default            : printf("%s\n",string);
		                     break;
	}

	return 0;
}

/*****************************************************************************/

static int print_line(int status, one_call *cur_call, int computed, char *overlap)
{
	char *string = NULL;
	char  help[32];
	prt_fmt **fmtstring = get_format(NULL);
	int dir;
	int i = 0;
	int free_col;
        int last_free_col = UNKNOWN;


	if (colsize == NULL || status == F_COUNT_ONLY)
	{
		free(colsize);

		if ((colsize = (int*) calloc(get_format_size()+1,sizeof(int))) == NULL)
		{
			print_msg(PRT_ERR, nomemory);
                        return(UNKNOWN);
		}
	}

	while (*fmtstring != NULL)
	{
		free_col = 0;

		if ((*fmtstring)->type == FMT_FMT)
		{
			switch((*fmtstring)->s_type)
			{
				/* time: */
				case 'X': if (status == F_BODY_LINE)
				            colsize[i] = append_string(&string,*fmtstring, get_time_value(0,NULL,GET_TIME));
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				            	colsize[i] = append_string(&string,*fmtstring, fill_spaces(get_time_value(0,NULL,GET_TIME)));
				          }
				          break;
				/* date (normal format with year): */
				case 'x': if (status == F_BODY_LINE)
				            colsize[i] = append_string(&string,*fmtstring, get_time_value(0,NULL,GET_DATE2));
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				            	colsize[i] = append_string(&string,*fmtstring, fill_spaces(get_time_value(0,NULL,GET_DATE2)));
				          }
				          break;
				/* date (without year): */
				case 'y': if (status == F_BODY_LINE)
				            colsize[i] = append_string(&string,*fmtstring, get_time_value(0,NULL,GET_DATE));
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				            	colsize[i] = append_string(&string,*fmtstring, fill_spaces(get_time_value(0,NULL,GET_DATE)));
				          }
				          break;
				/* year: */
				case 'Y': if (status == F_BODY_LINE)
				            colsize[i] = append_string(&string,*fmtstring, get_time_value(0,NULL,GET_YEAR));
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				            	colsize[i] = append_string(&string,*fmtstring, fill_spaces(get_time_value(0,NULL,GET_YEAR)));
				          }
				          break;
				/* duration: */
				case 'D': if (status == F_BODY_LINE)
				            colsize[i] = append_string(&string,*fmtstring, double2clock(cur_call->duration));
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				            	colsize[i] = append_string(&string,*fmtstring, fill_spaces(double2clock(cur_call->duration)));
				          }
				          break;
				/* Home (if possible the name): */
				/* Benoetigt Range! */
				case 'H': if (status == F_BODY_LINE)
				          {
				          	dir = cur_call->dir?CALLED:CALLING;
				            if (!numbers)
				          	{
				          		colsize[i] = append_string(&string,*fmtstring,
				           	             cur_call->who[dir][0]?cur_call->who[dir]:cur_call->num[dir]);
				          		break;
				          	}
				          }
				/* Home (number): */
				/* Benoetigt Range! */
				case 'h': if (status == F_BODY_LINE)
				            colsize[i] = append_string(&string,*fmtstring,
				                        cur_call->num[cur_call->dir?CALLED:CALLING]);
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				          		colsize[i] = append_string(&string,*fmtstring, "");
				          }
				          break;
				/* The other (if possible the name): */
				/* Benoetigt Range! */
			 	case 'F': if (status == F_BODY_LINE)
				          {
				            dir = cur_call->dir?CALLING:CALLED;
				          	if (!numbers)
				          	{
				          		colsize[i] = append_string(&string,*fmtstring,
				           	             cur_call->who[dir][0]?cur_call->who[dir]:cur_call->num[dir]);
				          		break;
				          	}
				          }
				/* The other (number): */
				/* Benoetigt Range! */
				case 'f': if (status == F_BODY_LINE)
				            colsize[i] = append_string(&string,*fmtstring,
				                        cur_call->num[cur_call->dir?CALLING:CALLED]);
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				          		colsize[i] = append_string(&string,*fmtstring, "");
				          }
				          break;
				/* The home location: */
				/* Benoetigt Range! */
				case 'L': if (status == F_BODY_LINE)
				          {
				            dir = cur_call->dir?CALLED:CALLING;
		          		colsize[i] = append_string(&string,*fmtstring, cur_call->sarea[dir]);
				          }
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				          		colsize[i] = append_string(&string,*fmtstring, "");
				          }
				          break;
				/* The remote location: */
				/* Benoetigt Range! */
				case 'l': if (status == F_BODY_LINE)
				          {
				            dir = cur_call->dir?CALLING:CALLED;
			          		colsize[i] = append_string(&string,*fmtstring,  cur_call->sarea[dir]);
				          }
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				          		colsize[i] = append_string(&string,*fmtstring, "");
				          }
				          break;
				/* The "To"-sign (from home to other) (-> or <-): */
				case 'T': if (status == F_BODY_LINE)
				            colsize[i] = append_string(&string,*fmtstring, cur_call->dir?"<-":"->");
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				          		colsize[i] = append_string(&string,*fmtstring, "  ");
				          }
				          break;
				/* The "To"-sign (from other to home) (-> or <-): */
				case 't': if (status == F_BODY_LINE)
				            colsize[i] = append_string(&string,*fmtstring, cur_call->dir?"->":"<-");
				          else
				          {
				          	free_col = 1;
				          	if (!html || status == F_COUNT_ONLY)
				          		colsize[i] = append_string(&string,*fmtstring, "  ");
				          }
				          break;
				/* The units (if exists): */
				/* Benoetigt Range! */
				case 'u': if (cur_call->eh > 0)
				          {
				          	sprintf(help,"%d EH",cur_call->eh);
				          	colsize[i] = append_string(&string,*fmtstring, help);
				          }
				          else
				          	colsize[i] = append_string(&string,*fmtstring, "");
				          break;
				/* The money or/and a message: */
				case 'U': if (cur_call->duration || cur_call->eh > 0 || cur_call->pay > 0)
				          {
				          	if (cur_call->dir)
				          		colsize[i] = append_string(&string,NULL,"            ");
				          	else
                                                        colsize[i] = append_string(&string,*fmtstring,print_currency(cur_call->pay,computed));
				          }
				          else
				          if ((status == F_BODY_LINE) &&
                                              (cur_call->cause != UNKNOWN) &&
				              (cur_call->cause != 0x10) &&  /* Normal call clearing */
                                              (cur_call->cause != 0x1f))    /* Normal, unspecified */
				          	colsize[i] = append_string(&string,*fmtstring,qmsg(TYPE_CAUSE, VERSION_EDSS1, cur_call->cause));
				          else
				          	colsize[i] = append_string(&string,NULL,"            ");
				          break;
				/* In-Bytes: */
				/* Benoetigt Range! */
				case 'I': colsize[i] = append_string(&string,*fmtstring,set_byte_string(GET_IN|GET_BYTES,(double)cur_call->ibytes));
				          break;
				/* Out-Bytes: */
				/* Benoetigt Range! */
				case 'O': colsize[i] = append_string(&string,*fmtstring,set_byte_string(GET_OUT|GET_BYTES,(double)cur_call->obytes));
				          break;
				/* In-Bytes per second: */
				/* Benoetigt Range! */
				case 'P': colsize[i] = append_string(&string,*fmtstring,set_byte_string(GET_IN|GET_BPS,cur_call->duration?cur_call->ibytes/(double)cur_call->duration:0.0));
				          break;
				/* Out-Bytes per second: */
				/* Benoetigt Range! */
				case 'p': colsize[i] = append_string(&string,*fmtstring,set_byte_string(GET_OUT|GET_BPS,cur_call->duration?cur_call->obytes/(double)cur_call->duration:0.0));
				          break;
				/* SI: */
				case 'S': if (status == F_BODY_LINE)
				          	colsize[i] = append_string(&string,*fmtstring,int2str(cur_call->si,2));
				          else
				          	colsize[i] = append_string(&string,*fmtstring,"  ");
				          break;

			 	case 'j': if (status == F_BODY_LINE)
				          {
				          	if (!numbers)
				          	{
                                                        register char *p;

                                                        p = (cur_call->provider >= 0) ? getProvider(cur_call->provider) : "";

                                                        if (cur_call->dir == DIALIN)
                                                          p = "";

                                                        colsize[i] = append_string(&string,*fmtstring, p);
				          		break;
				          	}
				          }
                                          break;

				/* Link for answering machine! */
				case 'C': if (html)
				          {
				          	if (status == F_BODY_LINE)
				          		colsize[i] = append_string(&string,*fmtstring,get_links(cur_call->t,C_VBOX));
				          	else
				          		colsize[i] = append_string(&string,*fmtstring,"     ");
				          }
				          else
				          	print_msg(PRT_ERR, "isdnrep: unknown format %%C!\n");
				          break;
				/* Link for fax! */
				case 'G': if (html)
				          {
				          	if (status == F_BODY_LINE)
				          		colsize[i] = append_string(&string,*fmtstring,get_links(cur_call->t,C_FAX));
				          	else
				          		colsize[i] = append_string(&string,*fmtstring,"        ");
				          }
				          else
				          	print_msg(PRT_ERR, "isdnrep: unknown format %%G!\n");
				          break;
				/* there are dummy entries */
				case 'c':
				case 'd':
				case 's': if (status != F_BODY_LINE)
				          	free_col = 1;

				          colsize[i] = append_string(&string,*fmtstring, " ");
				          break;
                                default : print_msg(PRT_ERR, "isdnrep: internal Error: unknown format `%c'!\n",(*fmtstring)->s_type);
				          break;
			}
		}
		else
		if ((*fmtstring)->type == FMT_STR)
		{
			if (!html || status == F_COUNT_ONLY || status == F_BODY_LINE || last_free_col != i-1)
				colsize[i] = append_string(&string,NULL,(*fmtstring)->string);
			else
				free_col = 1;
		}
		else
			print_msg(PRT_ERR, "isdnrep: internal Error: unknown format type `%d'!\n",(*fmtstring)->type);

		if (html && status == F_BODY_BOTTOM1 && free_col && last_free_col == i-1)
			last_free_col = i;

		fmtstring++;
		i++;
	}

        if (last_free_col != UNKNOWN)
	{
		char *help2 = NULL;


		if ((help2 = (char*) calloc(strlen(H_BODY_BOTTOM1)+(string?strlen(string):0)+strlen(overlap)+1,sizeof(char))) == NULL)
		{
			print_msg(PRT_ERR, nomemory);
                        return(UNKNOWN);
		}

		sprintf(help2,H_BODY_BOTTOM1,last_free_col+1,overlap,string?string:"");

		free(string);
		string = help2;
		overlap = NULL;
	}

        colsize[i] = UNKNOWN;

	if (status == F_COUNT_ONLY)
		return strlen(string);
	else
		print_line2(status,"%s",overlap_string(string,overlap));

	free(string);

	return 0;
}

/*****************************************************************************/

static void bprint(one_call *call)
{
  register char  *p = call->num[CALLED];
  auto	   char	  target[BUFSIZ], s[BUFSIZ];
  auto	   TELNUM number;

  if (call->duration) {
    if (!memcmp(call->num[CALLED], mycountry, strlen(mycountry))) { /* eigenes Land */
      p += strlen(mycountry);
      sprintf(target, "0%s", p);
    }
    else
      sprintf(target, "%s", p);

    printf( "%s %s %-16s  ",
      get_time_value(0,NULL, GET_TIME),
      double2clock(call->duration),
      target);

    if (call->duration) {
      printf( "%s %-15s",
      print_currency(call->pay * 100.0 / 116.0, 0), getProvider(call->provider));

      strcpy(s, call->num[CALLED]);

      number.nprovider=call->provider;
      normalizeNumber(s, &number, TN_NO_PROVIDER);
      printf( "%s\n", formatNumber("%A", &number));
    }
    else
      printf( "%*s** %s\n", 30, "", qmsg(TYPE_CAUSE, VERSION_EDSS1, call->cause));
  } /* if */
} /* bprint */

/*****************************************************************************/

static int append_string(char **string, prt_fmt *fmt_ptr, char* value)
{
	char  tmpstr[BUFSIZ*3];
	char  tmpfmt2[20];
	char *tmpfmt;
	char *htmlfmt;
	int   condition = html && (*value == ' ' || fmt_ptr == NULL || (fmt_ptr->s_type!='C' && fmt_ptr->s_type!='G'));


	if (fmt_ptr != NULL)
		sprintf(tmpfmt2,"%%%ss",fmt_ptr->range);
	else
		strcpy(tmpfmt2,"%s");

	if (html)
	{
		switch (fmt_ptr==NULL?'\0':fmt_ptr->range[0])
		{
			case '-' : htmlfmt = H_LEFT;
			           break;
			case '\0': htmlfmt = H_CENTER;
			           break;
			default  : htmlfmt = H_RIGHT;
			           break;
		}

		if (!strncmp(STR_FAX,value,strlen(STR_FAX)))
			htmlfmt = H_LEFT;

		if ((tmpfmt = (char*) alloca(sizeof(char)*(strlen(htmlfmt)+strlen(tmpfmt2)+1))) == NULL)
		{
			print_msg(PRT_ERR, nomemory);
                        return(UNKNOWN);
		}

		sprintf(tmpfmt,htmlfmt,tmpfmt2);
	}
	else
		tmpfmt = tmpfmt2;

	app_fmt_string(tmpstr,BUFSIZ*3-1,tmpfmt,condition,value);

	if (*string == NULL)
		*string = (char*) calloc(strlen(tmpstr)+1,sizeof(char));
	else
		*string = (char*) realloc(*string,sizeof(char)*(strlen(*string)+strlen(tmpstr)+1));

	if (*string == NULL)
	{
		print_msg(PRT_ERR, nomemory);
                return(UNKNOWN);
	}

	strcat(*string,tmpstr);

	return strlen(tmpstr);
}

/*****************************************************************************/

static char *html_conv(char *string)
{
	static char  RetCode[BUFSIZ];
	char *ptr = RetCode;
	int   i;
	int empty = 1;

	if (string == NULL)
		return NULL;

	do
	{
		for(i = 0; htmlconv[i][0][0] != '\0'; i++)
		{
			if (htmlconv[i][0][0] == *string)
			{
				strcpy(ptr,htmlconv[i][1]);
				while (*ptr != '\0') ptr++;
				break;
			}
		}

		if (htmlconv[i][0][0] == '\0')
			*ptr++ = *string;

		if (*string != '\0' && !isspace(*string))
			empty = 0;
	}
	while(*string++ != '\0');

	if (empty)
		return H_EMPTY;

	return RetCode;
}

/*****************************************************************************/

static char *set_byte_string(int flag, double Bytes)
{
	static char string[4][20];
	static int num = 0;
	int  factor = 1;
	char prefix = ' ';


	num = (num+1)%4;

	if (!Bytes)
	{
		if (flag & GET_BPS)
			strcpy(string[num],"              ");
		else
			strcpy(string[num],"            ");
	}
	else
	{
		if (Bytes >= 9999999999.0)
		{
			factor = 1073741824;
			prefix = 'G';
		}
		else
		if (Bytes >= 9999999)
		{
			factor = 1048576;
			prefix = 'M';
		}
		else
		if (Bytes >= 9999)
		{
			factor = 1024;
			prefix = 'k';
		}

		sprintf(string[num],"%c=%s %cB%s",flag&GET_IN?'I':'O',double2str(Bytes/factor,7,2,0),prefix,flag&GET_BPS?"/s":"");
	}

	return string[num];
}

/*****************************************************************************/

static int set_col_size(void)
{
	one_call *tmp_call;
	int size = 0;
	int i = 0;

	if ((tmp_call = (one_call*) calloc(1,sizeof(one_call))) == NULL)
	{
		print_msg(PRT_ERR, nomemory);
                return(UNKNOWN);
	}

	print_line(F_COUNT_ONLY,tmp_call,0,NULL);

        while(colsize[i] != UNKNOWN)
		if (html)
			printf(H_BODY_HEADER2,colsize[i++]);
		else
			size += colsize[i++];

	free(tmp_call);
	return size;
}

/*****************************************************************************/

static int get_format_size(void)
{
	int i = 0;
	prt_fmt** fmt = get_format(NULL);

	if (fmt == NULL)
		return 0;

	while(*fmt++ != NULL) i++;

	return i;
}

/*****************************************************************************/

static char *overlap_string(char *s1, char *s2)
{
	int i = 0;

	if (s1 == NULL || s2 == NULL)
		return s1;

	while(s1[i] != '\0' && s2[i] != '\0')
	{
		if (isspace(s1[i]))
			s1[i] = s2[i];

		i++;
	}

	return s1;
}

/*****************************************************************************/

static char* fill_spaces(char *string)
{
	static char RetCode[256];
	char *Ptr = RetCode;

	while (*string != '\0')
	{
		*Ptr++ = ' ';
		string++;
	}

	*Ptr = '\0';

	return RetCode;
}

/*****************************************************************************/

static void free_format(prt_fmt** ptr)
{
	prt_fmt** ptr2 = ptr;


	if (ptr == NULL)
		return;

	while(*ptr != NULL)
	{
		free((*ptr)->string);
		free((*ptr)->range);
		free((*ptr++));
	}

	free(ptr2);
	free(colsize);
	colsize = NULL;
}

/*****************************************************************************/

static prt_fmt** get_format(const char *format)
{
	static prt_fmt **RetCode = NULL;
	prt_fmt *fmt = NULL;
	char    *Ptr = NULL;
	char    *string = NULL;
	char    *start = NULL;
	char    *End = NULL;
	char     Range[20] = "";
	int      num;
	char     Type;


	if (format == NULL)
		return RetCode;

	free_format(RetCode);
	RetCode = NULL;

	if ((End    = (char*) alloca(sizeof(char)*(strlen(format)+1))) == NULL ||
	    (string = (char*) alloca(sizeof(char)*(strlen(format)+1))) == NULL   )
	{
		print_msg(PRT_ERR, nomemory);
		return NULL;
	}

	Ptr = start = strcpy(string,format);

	do
	{
		if (*Ptr == C_BEGIN_FMT)
		{
			if (Ptr[1] != C_BEGIN_FMT)
			{
				if (Ptr != start)
				{
					if ((fmt = (prt_fmt*) calloc(1,sizeof(prt_fmt))) == NULL)
					{
						print_msg(PRT_ERR, nomemory);
						delete_element(&RetCode,0);
						return NULL;
					}

					*Ptr = '\0';
					fmt->string= strdup(start);
					fmt->type  = FMT_STR;
					append_element(&RetCode,fmt);
				}

				*Range = *End = '\0';
				if ((num = sscanf(Ptr+1,"%[^a-zA-Z]%c%[^\n]",Range,&Type,End)) > 1 ||
				    (num = sscanf(Ptr+1,"%c%[^\n]",&Type,End))                 > 0   )
				{
					if (!isalpha(Type))
    				print_msg(PRT_ERR, "isdnrep: warning: invalid token in format type `%c'!\n",Type);

					if ((fmt = (prt_fmt*) calloc(1,sizeof(prt_fmt))) == NULL)
					{
						print_msg(PRT_ERR, nomemory);
						delete_element(&RetCode,0);
						return NULL;
					}

					switch (Type)
					{
						case 'C': read_path |= F_VBOX;
						          break;
						case 'G': read_path |= F_FAX;
						          break;
						default : break;
					}

					fmt->s_type= Type;
					fmt->range = strdup(Range);
					fmt->type  = FMT_FMT;

					append_element(&RetCode,fmt);
					*Range = '\0';

					if (*End != '\0')
						Ptr = start = strcpy(string,End);
					else
						Ptr = start = "";
				}
				else
				{
    			print_msg(PRT_ERR, "isdnrep: error: invalid token in format string `%s'!\n",format);
    			return NULL;
    		}
			}
			else
			{
				memmove(Ptr,Ptr+1,strlen(Ptr));
				Ptr++;
			}
		}
		else
			Ptr++;
	}
	while(*Ptr != '\0');

	if (Ptr != start)
	{
		if ((fmt = (prt_fmt*) calloc(1,sizeof(prt_fmt))) == NULL)
		{
			print_msg(PRT_ERR, nomemory);
			delete_element(&RetCode,0);
			return NULL;
		}

		fmt->string= strdup(start);
		fmt->type  = FMT_STR;
		append_element(&RetCode,fmt);
	}

	return RetCode;
}

/*****************************************************************************/

static int print_entries(one_call *cur_call, double unit, int *nx, char *myname)
{
  register int i, zone, computed = 0;


  if (1 /* cur_call->dir == DIALOUT */) {

    zone = (cur_call->zone >= 0) ? cur_call->zone : UNKNOWNZONE;

		  zones[zone] += cur_call->eh;
    zones_dm[zone] += cur_call->pay;

		  if (cur_call->duration > 0)
		    zones_dur[zone] += cur_call->duration;

    zones_usage[zone] += 1;

    if (zone == UNKNOWNZONE) {
      if (*unknownzones)
        strcat(unknownzones, ", ");

      strcat(unknownzones, cur_call->num[1]);
                } /* if */

    add_one_call(computed ? &day_com_sum : &day_sum, cur_call, unit);

		if (cur_call->dir) {
      if (nx[CALLING] == UNKNOWN) {
	known[knowns - 1]->usage[DIALIN]++;
	known[knowns - 1]->ibytes[DIALIN] += cur_call->ibytes;
	known[knowns - 1]->obytes[DIALIN] += cur_call->obytes;

				if (cur_call->duration > 0)
					known[knowns-1]->dur[DIALIN] += cur_call->duration;
			}
      else {
				known[nx[CALLING]]->usage[cur_call->dir]++;
				known[nx[CALLING]]->dur[cur_call->dir] += cur_call->duration;
				known[nx[CALLING]]->ibytes[DIALIN] += cur_call->ibytes;
				known[nx[CALLING]]->obytes[DIALIN] += cur_call->obytes;
      } /* else */
		}
		else {
      usage_provider[cur_call->provider]++;

      if ((cur_call->cause == 0x22) || /* No circuit/channel available */
          (cur_call->cause == 0x2a))   /* Switching equipment congestion */
        provider_failed[cur_call->provider]++;

      duration_provider[cur_call->provider] += cur_call->duration;
      pay_provider[cur_call->provider] += cur_call->pay;

      if (nx[CALLED] == UNKNOWN) {
				known[knowns-1]->eh += cur_call->eh;
      	known[knowns-1]->pay += cur_call->pay;
				known[knowns-1]->ibytes[DIALOUT] += cur_call->ibytes;
				known[knowns-1]->obytes[DIALOUT] += cur_call->obytes;
				known[knowns-1]->usage[DIALOUT]++;

				if (cur_call->duration > 0)
					known[knowns-1]->dur[DIALOUT] += cur_call->duration;
			}
			else {
				known[nx[CALLED]]->eh += cur_call->eh;
      	known[nx[CALLED]]->pay += cur_call->pay;
				known[nx[CALLED]]->usage[cur_call->dir]++;
				known[nx[CALLED]]->dur[cur_call->dir] += cur_call->duration;
				known[nx[CALLED]]->ibytes[DIALOUT] += cur_call->ibytes;
				known[nx[CALLED]]->obytes[DIALOUT] += cur_call->obytes;
			} /* if */
		} /* else */
	}
  else
    add_one_call(computed ? &day_com_sum : &day_sum, cur_call, unit);

      	 	      		if (cur_call->dir == DIALOUT) {
    int first_found = UNKNOWN;

                                  for (i = 0; i < mymsns; i++) {
                                    if (!n_match(known[i]->num, cur_call->num[0], cur_call->version)) {
                                      /* Ermitteln der ersten passenden MSN (lt. README) */
        if (first_found == UNKNOWN)
                                        first_found = i;
          /* exakte Uebereinstimmung inkl. SI */
                                      if (known[i]->si == cur_call->si) {
          msn_sum[i] += cur_call->pay;
                                      usage_sum[i]++;
                                      dur_sum[i] += cur_call->duration;
                                      break;
                                    } /* if */
                                    } /* if */
                                  } /* for */

                                  if (i == mymsns) {
                                    /* keine exakte Uebereinstimmung, aber ohne SI */
      if (first_found != UNKNOWN)
                                      i = first_found;
      msn_sum[i] += cur_call->pay;
                                    usage_sum[i]++;
                                    dur_sum[i] += cur_call->duration;
                                  } /* if */

      	 	      		} /* if */

  if (bill) {
    if (cur_call->dir == DIALOUT)
    bprint(cur_call);
  }
  else if(!summary)
    print_line(F_BODY_LINE,cur_call,computed,NULL);

  return(0);
} /* print_entries */

/*****************************************************************************/

static int print_header(int lday)
{
  sum_calls tmp_sum;
	time_t now;


        if (lday == UNKNOWN) {
		time(&now);

                if (bill)
                  print_line2(F_1ST_LINE, "Ihre Verbindungen im einzelnen  -  %s", ctime(&now));
                else
		  print_line2(F_1ST_LINE,"I S D N  Connection Report  -  %s", ctime(&now));
	}
	else
	{
	    if (summary >= 2)
		return 0;
		strich(1);
		print_sum_calls(&day_sum,0);

		if (day_com_sum.eh)
		{
			print_sum_calls(&day_com_sum,1);

			clear_sum(&tmp_sum);
			add_sum_calls(&tmp_sum,&day_sum);
			add_sum_calls(&tmp_sum,&day_com_sum);
			strich(1);
			print_sum_calls(&tmp_sum,0);
		}
		else
			printf("\n\n");

		add_sum_calls(&all_sum,&day_sum);
		clear_sum(&day_sum);

		add_sum_calls(&all_com_sum,&day_com_sum);
		clear_sum(&day_com_sum);

		print_line2(F_BODY_BOTTOM2,"");
	} /* if */

	h_percent = 100.0;
	h_table_color = H_TABLE_COLOR1;
	print_line2(F_BODY_HEADER,"");
	print_line2(F_BODY_HEADERL,"%s %s", get_time_value(0,NULL,GET_DATE),
	                                       get_time_value(0,NULL,GET_YEAR));

	return 0;
}

/*****************************************************************************/

static char *get_time_value(time_t t, int *day, int flag)
{
	static char time_string[4][18] = {"","",""};
        static int  oldday = UNKNOWN;
        static int  oldyear = UNKNOWN;
	struct tm *tm;


	if (flag & SET_TIME)
	{
  	tm = localtime(&t);

		/*sprintf(time_string[0],"%d:%d:%d",tm->tm_hour,tm->tm_min,tm->tm_sec);*/
		strftime(time_string[0],17,"%X",tm);

		if (oldday != tm->tm_yday || oldyear != tm->tm_year)
		{
	 		*day = tm->tm_mday;

			Tarif96 = tm->tm_year > 95;

		/* Ab Juli '96 gibt's noch mal eine Senkung in den Zonen 3 und 4
		   genaue Preise sind jedoch noch nicht bekannt. FIX-ME */

			Tarif962 = (Tarif96 && (tm->tm_mon > 5)) || (tm->tm_year > 96);

			strftime(time_string[1],17,"%a %b %d",tm);
			strftime(time_string[2],17,"%x",tm);
			oldday  = tm->tm_yday;

			if (oldyear != tm->tm_year)
			{
				sprintf(time_string[3],"%d",1900+tm->tm_year);
				oldyear = tm->tm_year;
			}
		}
	}

	if (flag & GET_TIME)
		return time_string[0];
	else
	if (flag & GET_DATE)
		return time_string[1];
	else
	if (flag & GET_DATE2)
		return time_string[2];
	else
	if (flag & GET_YEAR)
		return time_string[3];

	return NULL;

}

/*****************************************************************************/

static int set_alias(one_call *cur_call, int *nx, char *myname)
{
	auto int n, cc, i, j;
	auto int hit;


	for (n = CALLING; n <= CALLED; n++) {
                nx[n] = UNKNOWN;
		hit = 0;

		if (!*(cur_call->num[n])) {
//			if (!numbers)
			{
				cur_call->num[n][0] = C_UNKNOWN;
				cur_call->num[n][1] = '\0';
			}
			/* Wenn keine Nummer, dann Name "UNKNOWN" */

			hit++;
		} else {
			/* In der folg. Schleife werden Nummern durch Namen ersetzt */
			cc = 0;

			for (j = 0; ((j < 2) && !cc); j++) {
				for (i = 0; i < knowns; i++) {

					if (cur_call->version[0] != '\0')
					{
						if (!strcmp(cur_call->version,LOG_VERSION_2) ||
						    !strcmp(cur_call->version,LOG_VERSION_3) ||
                                                    !strcmp(cur_call->version,LOG_VERSION_4) ||
						    !strcmp(cur_call->version,LOG_VERSION) )
							cc = ((known[i]->si == cur_call->si) || j) &&
							     !n_match(known[i]->num, cur_call->num[n], cur_call->version);
					}
					else
					{
						if (*cur_call->num[n] != '0') {

							/* Alte Syntax der "isdn.log" : Ohne vorlaufene "0" */
							cc = ((known[i]->si == cur_call->si) || j) &&
							     !match(known[i]->num+1, cur_call->num[n], 0);

							if (!cc) {
								/* Ganz alte Syntax der "isdn.log" : Ohne Vorwahl */
								cc = ((known[i]->si == cur_call->si) || j) &&
							     !n_match(known[i]->num, cur_call->num[n], LOG_VERSION_1);
							} /* if */
						}
					}

					if (cc) {

						strncpy(cur_call->who[n], known[i]->who,NUMSIZE);

 						nx[n] = i;
						hit++;
						break;
					} /* if */
				} /* for */
			} /* for */

			/* In der naechsten Schleife werden die unbekannten Nummern
			   registriert */
			if (!hit && seeunknowns) {
				for (i = 0; i < unknowns; i++)
					if (!strcmp(unknown[i].num, cur_call->num[n])) {
					hit++;
					break;
				} /* if */

				strcpy(unknown[i].num, cur_call->num[n]);
                                strcpy(unknown[i].mynum, cur_call->num[1 - n]);
                                unknown[i].si1 = cur_call->si1;
				unknown[i].called = !n;
				unknown[i].connect[unknown[i].connects] = cur_call->t;
                                unknown[i].cause = cur_call->cause;

				/* ACHTUNG: MAXCONNECTS und MAXUNKNOWN sollten unbedingt groesser sein ! */
				if (unknown[i].connects + 1 < MAXCONNECTS)
					unknown[i].connects++;
				else
					print_msg(PRT_ERR, "%s: WARNING: too many unknown connections from %s\n", myname, unknown[i].num);

				if (!hit) {
					if (unknowns < MAXUNKNOWN)
						unknowns++;
					else
						print_msg(PRT_ERR, "%s: WARNING: too many unknown numbers\n", myname);
				} /* if */
			} /* if */
		} /* else */
	} /* for */

	return 0;
}

static void repair(one_call *cur_call)
{
  RATE Rate;
  TELNUM srcnum, destnum;

  if (*cur_call->num[CALLING]) {
    normalizeNumber(cur_call->num[CALLING],&srcnum,TN_ALL);
    strcpy(cur_call->sarea[CALLING], srcnum.sarea);
  }
  else
    *cur_call->sarea[CALLING] = 0;
  if (*cur_call->num[CALLED]) {
    destnum.nprovider = cur_call->provider;
    Strncpy(destnum.provider,getProvider(cur_call->provider), TN_MAX_PROVIDER_LEN);
    normalizeNumber(cur_call->num[CALLED], &destnum, TN_NO_PROVIDER);
    strcpy(cur_call->sarea[CALLED], destnum.sarea);
  }
  else
    *cur_call->sarea[CALLED] = 0;

  if ((cur_call->dir == DIALOUT) &&
      (cur_call->duration > 0) &&
      *cur_call->num[CALLED]
     ) {


    call[0].connect = cur_call->t;
    call[0].disconnect = cur_call->t + cur_call->duration;
    call[0].intern[CALLED] = strlen(cur_call->num[CALLED]) < interns0;
    call[0].provider = cur_call->provider;
    call[0].aoce = cur_call->eh;
    call[0].dialin = 0;
    strcpy(call[0].num[CALLED], cur_call->num[CALLED]);
    strcpy(call[0].onum[CALLED], cur_call->num[CALLED]);

    call[0].sondernummer[CALLED] = destnum.ncountry==0;

    clearRate(&Rate);
    Rate.src[0] = srcnum.country;
    Rate.src[1] = srcnum.area;
    Rate.src[2] = "";
    Rate.dst[0] = destnum.country;
    Rate.dst[1] = destnum.area;
    Rate.dst[2] = destnum.msn;
    Rate.start = cur_call->t;
    Rate.now = call[0].disconnect;
    Rate.prefix = cur_call->provider;
    if (!getRate(&Rate,0)) {
      if(strcmp(cur_call->version, LOG_VERSION))
         cur_call->pay = Rate.Charge; /* Fixme: is that ok, propably rates have changed */
      cur_call->zone = Rate._zone;
      zones_names[Rate._zone] = Rate.Zone ? strdup(Rate.Zone) : strdup("??");
    }
  }
} /* repair */

/*****************************************************************************/

static int set_caller_infos(one_call *cur_call, char *string, time_t from)
{
	int      rc;
  register int    i = 0, adapt = 0;
  auto     char **array;
  auto     double dur1 = 0.0, dur2 = 0.0;


	array = string_to_array(string);

	if (array[5] == NULL)
    return(UNKNOWN);

	cur_call->t = atol(array[5]);

	rc = 0;
	if (delentries && cur_call->t < endtime)
		rc = UNKNOWN; /* return this, but first process the record */
	else if (timearea && (cur_call->t < begintime || cur_call->t > endtime))
    return(UNKNOWN);
	else
		if (cur_call->t < from)
      return(UNKNOWN);

	cur_call->eh = 0;
  cur_call->dir = UNKNOWN;
  cur_call->cause = UNKNOWN;
	cur_call->ibytes = cur_call->obytes = 0L;
  cur_call->pay    = 0.0;
	cur_call->version[0] = '\0';
	cur_call->si = cur_call->si1 = 0;
	cur_call->dir = DIALOUT;
	cur_call->who[0][0] = '\0';
	cur_call->who[1][0] = '\0';
  cur_call->provider = UNDEFINED;
  cur_call->zone = UNDEFINED;

	for (i = 1; array[i] != NULL; i++)
	{
		switch (i)
		{
			case  0 : cur_call->t = atom(array[i]);
			          break;
			case  1 : strcpy(cur_call->num[0], Kill_Blanks(array[i]));
			          break;
			case  2 : strcpy(cur_call->num[1], Kill_Blanks(array[i]));
								/* Korrektur der falschen Eintraege aus den ersten Januar-Tagen 1998 */
								if (!memcmp(cur_call->num[1], "+491019", 7)) {
									cur_call->provider = 19;
									memmove(cur_call->num[1] + 3, cur_call->num[1] + 8, strlen(cur_call->num[1]) - 7);
									adapt++;
								}
								else if (!memcmp(cur_call->num[1], "+491033", 7)) {
									cur_call->provider = 33;
									memmove(cur_call->num[1] + 3, cur_call->num[1] + 8, strlen(cur_call->num[1]) - 7);
									adapt++;
								}
								else if (!memcmp(cur_call->num[1], "+491070", 7)) {
									cur_call->provider = 70;
									memmove(cur_call->num[1] + 3, cur_call->num[1] + 8, strlen(cur_call->num[1]) - 7);
									adapt++;
								} /* else */
								if (adapt)
									strcpy(cur_call->version, LOG_VERSION_4);

								break;
			case  3 : dur1 = cur_call->duration = strtod(array[i],NULL);
								if (dur1 < 0)  /* wrong entry on some incoming voice calls */
									dur1 = cur_call->duration = 0;
			          break;
			case  4 : dur2 = cur_call->duration = strtod(array[i],NULL)/HZ;
			          break;
			case  5 : /*cur_call->t = atol(array[i]);*/
			          break;
			case  6 : cur_call->eh = atoi(array[i]);
			          break;
			case  7 : cur_call->dir = (*array[i] == 'I') ? DIALIN : DIALOUT;
			          break;
			case  8 : cur_call->cause = atoi(array[i]);
			          break;
			case  9 : cur_call->ibytes = atol(array[i]);
			          break;
			case  10: cur_call->obytes = atol(array[i]);
			          break;
			case  11: if (!adapt)
			            strcpy(cur_call->version,array[i]);
			          break;
			case  12: cur_call->si = atoi(array[i]);
			          break;
			case  13: cur_call->si1 = atoi(array[i]);
			          break;
			case  14: cur_call->currency_factor = atof(array[i]);
			          break;
			case  15: strncpy(cur_call->currency, array[i], 3);
			          break;
      case  16: cur_call->pay = atof(array[i]);
								/* Korrektur der falschen Eintr„ge vor dem 16-Jan-99 */
								if (cur_call->pay == -1.0)
									cur_call->pay = 0.0;
			          break;
			case  17: if (!adapt) {
			      	    cur_call->provider = atoi(array[i]);
									/* Korrektur der falschen Eintrage bis zum 16-Jan-99 */
									if (cur_call->provider <= UNKNOWN || cur_call->provider >= MAXPROVIDER)
										cur_call->provider = preselect;
									/* -lt- provider-# may change during time */
									cur_call->provider = pnum2prefix(cur_call->provider,cur_call->t);
								} /* if */
								break;

			/* Seit 21-Jan-99 steht die Zone im Logfile */
			case  18: cur_call->zone = atoi(array[i]);
			      	  break;

			default : print_msg(PRT_ERR, "isdnrep: unknown element found `%s'!\n",array[i]);
			          break;
		}
	}

	if (i < 3)
		return(UNKNOWN);

  /* alle Eintraege vor dem 21-Jan-99 ergaenzen:
       - zone fehlte
       - pay war falsch
  */

  {
#if DEBUG
    auto char yy[BUFSIZ];
#endif

    if (abs((int)dur1 - (int)dur2) > 1) {
#if DEBUG
      sprintf(yy, "\nWARNING: Wrong duration! dur1=%f, dur2=%f, durdiff=%f\n",
        dur1, dur2, dur1 - dur2);
      printf( yy);
#endif
      cur_call->duration = dur1;
    }

  }

  repair(cur_call);

  return(rc);
}


/*****************************************************************************/

static char **string_to_array(char *string)
{
	static char *array[30];
	auto   char *ptr;
	auto   int   i = 0;

	memset(array, 0, sizeof(array));
	if ((ptr = strrchr(string,C_DELIM)) != NULL)
		*ptr = '\0';

	array[0] = string;

	while((string = strchr(string,C_DELIM)) != NULL)
	{
		*string++ = '\0';
		array[++i] = string;
	}

	array[++i] = NULL;
	return array;
}

/*****************************************************************************/

int get_term (char *String, time_t *Begin, time_t *End,int delentries)
{
  time_t now;
  time_t  Date[2];
  int Cnt;
  char DateStr[2][256] = {"",""};


  time(&now);

  if (String[0] == '-')
    strcpy(DateStr[1],String+1);
  else
  if (String[strlen(String)-1] == '-')
  {
    strcpy(DateStr[0],String);
    DateStr[0][strlen(String)-1] ='\0';
  }
  else
  if (strchr(String,'-'))
  {
    if (sscanf(String,"%[/.0-9]-%[/.0-9]",DateStr[0],DateStr[1]) != 2)
      return 0;
  }
  else
  {
    strcpy(DateStr[0],String);
    strcpy(DateStr[1],String);
  }

  for (Cnt = 0; Cnt < 2; Cnt++)
  {
    if (strchr(DateStr[Cnt],'/'))
      Date[Cnt] = get_month(DateStr[Cnt],delentries?0:Cnt);
    else
      Date[Cnt] = get_time(DateStr[Cnt],delentries?0:Cnt);
  }

  *Begin = DateStr[0][0] == '\0' ? 0 : Date[0];
  *End = DateStr[1][0] == '\0' ? now : Date[1];

  return 1;
}

/*****************************************************************************/

static time_t get_month(char *String, int TimeStatus)
{
  time_t now;
  int Cnt = 0;
  struct tm *TimeStruct;
  int Args[3];
  int ArgCnt;


  time(&now);
  TimeStruct = localtime(&now);
  TimeStruct->tm_sec = 0;
  TimeStruct->tm_min = 0;
  TimeStruct->tm_hour= 0;
  TimeStruct->tm_mday= 1;
  TimeStruct->tm_isdst= UNKNOWN;

  ArgCnt = sscanf(String,"%d/%d/%d",&(Args[0]),&(Args[1]),&(Args[2]));

  switch (ArgCnt)
  {
    case 3:
      TimeStruct->tm_mday = Args[0];
      Cnt++;
    case 2:
      /* if (Args[Cnt+1] > 99) */
      if (Args[Cnt+1] >= 1900)
        TimeStruct->tm_year = ((Args[Cnt+1] / 100) - 19) * 100 + (Args[Cnt+1]%100);
      else
        if (Args[Cnt+1] < 70)
          TimeStruct->tm_year = Args[Cnt+1] + 100;
        else
          TimeStruct->tm_year = Args[Cnt+1];
    case 1:
      TimeStruct->tm_mon = Args[Cnt];
      break;
    default:
      return 0;
  }

  if (TimeStatus == END_TIME)
  {
    if (ArgCnt == 3)	/* Wenn Tag angegeben ist */
    {
      TimeStruct->tm_mday++;
      TimeStruct->tm_mon--;
    }
  }
  else
    TimeStruct->tm_mon--;

  return mktime(TimeStruct);
}

/*****************************************************************************/

static time_t get_time(char *String, int TimeStatus)
{
  time_t now;
  int Len = 0;
  int Year;
  char *Ptr;
  struct tm *TimeStruct;


  time(&now);
  TimeStruct = localtime(&now);
  TimeStruct->tm_sec = 0;
  TimeStruct->tm_min = 0;
  TimeStruct->tm_hour= 0;
  TimeStruct->tm_isdst= UNKNOWN;

  switch (strlen(String))
  {
    case 0:
            return 0;
    case 1:
    case 2:
            TimeStruct->tm_mday = atoi(String);
            break;
    default:
            Len = strlen(String);

      if ((Ptr = strchr(String,'.')) != NULL)
      {
        TimeStruct->tm_sec = atoi(Ptr+1);
        Len -= strlen(Ptr);
      }

      if (Len % 2)
        return 0;

      if (sscanf(String,"%2d%2d%2d%2d%d",
          &(TimeStruct->tm_mon),
          &(TimeStruct->tm_mday),
          &(TimeStruct->tm_hour),
          &(TimeStruct->tm_min),
          &Year		) 		> 4) {
        /* if (Year > 99) */
	if (Year >= 1900)
          TimeStruct->tm_year = ((Year / 100) - 19) * 100 + (Year%100);
        else {
	  if (Year < 70)
	    TimeStruct->tm_year = Year + 100;
	  else
	    TimeStruct->tm_year = Year;
        }
      }
      TimeStruct->tm_mon--;
      break;
  }

  if (TimeStatus == END_TIME) {
    if (TimeStruct->tm_sec == 0 &&
        TimeStruct->tm_min == 0 &&
        TimeStruct->tm_hour== 0   )
      TimeStruct->tm_mday++;
    else {
      if (TimeStruct->tm_sec == 0 &&
        TimeStruct->tm_min == 0   )
        TimeStruct->tm_hour++;
      else {
        if (TimeStruct->tm_sec == 0   )
          TimeStruct->tm_min++;
        else
          TimeStruct->tm_sec++;
      }
    }
  }
  return mktime(TimeStruct);
}

/*****************************************************************************/

int set_msnlist(char *String)
{
  int Cnt;
  int Value = 1;
  char *Ptr = String;


  if (ShowMSN)
    return 0;

  while((Ptr = strchr(Ptr,',')) != NULL)
  {
    Ptr++;
    Value++;
  }

  ShowMSN = (char**) calloc(Value+1,sizeof(char*));
  for(Cnt = 0; Cnt < Value; Cnt++)
    ShowMSN[Cnt] = (char*) calloc(NUMSIZE,sizeof(char));

  if (!ShowMSN)
  {
    print_msg(PRT_ERR, nomemory);
    exit(1);
  }

  if (*String == 'n')
  {
    ++String;
    invertnumbers++;
  }

  Cnt = 0;

  while(String)
  {

    if (*String == 'm')
    {
      Value = atoi(++String);

      if (Value > 0 && Value <= mymsns)
      {
        strcpy(ShowMSN[Cnt],known[Value-1]->num);
        Cnt++;
      }
      else
      {
        print_msg(PRT_ERR, "isdnrep: invalid MSN %d!\n", Cnt);
      }
    }
    else
      sscanf(String,"%[^,],",ShowMSN[Cnt++]);

    String = strchr(String,',');
    if (String) String++;
  }

  return 0;
}

/*****************************************************************************/

static int show_msn(one_call *cur_call)
{
  int Cnt;

  for(Cnt = 0; ShowMSN[Cnt] != NULL; Cnt++)
    if (!num_match(ShowMSN[Cnt], cur_call->num[0]) ||
        !num_match(ShowMSN[Cnt]   , cur_call->num[1]))
      return !invertnumbers;
    else
    if (!strcmp(ShowMSN[Cnt],"0"))
      return !invertnumbers;

  return invertnumbers;
}

/*****************************************************************************/

static char *print_currency(double money, int computed)
{
  static char RetCode[256];


	sprintf(RetCode,"%s %s%c", double2str(money, 8, 4, 0), currency, computed ? '*' : ' ');
  return RetCode;
}

/*****************************************************************************/

static int print_sum_calls(sum_calls *s, int computed)
{
  static char String[256];
  one_call *tmp_call;
  int RetCode;


  if (bill)
    printf( "%*s%s\n", 36, "", print_currency(s->pay, 0));
  else {
	if ((tmp_call = (one_call*) calloc(1,sizeof(one_call))) == NULL)
	{
		print_msg(PRT_ERR, nomemory);
                return(UNKNOWN);
	}

	tmp_call->eh = s->eh;
        tmp_call->pay    = s->pay;
	tmp_call->obytes = s->obytes;
	tmp_call->ibytes = s->ibytes;

  sprintf(String,"%3d IN=%s, %3d OUT=%s, %3d failed",
    s->in,
    double2clock(s->din),
    s->out,
    double2clock(s->dout),
    s->err);

  RetCode = print_line(F_BODY_BOTTOM1,tmp_call,computed,String);
	free(tmp_call);

  } /* else */

  return RetCode;
}

/*****************************************************************************/

static int add_one_call(sum_calls *s1, one_call *s2, double units)
{
  if (s2->dir == DIALIN)
  {
    s1->in++;
    s1->din += s2->duration > 0?s2->duration:0;
  }
  else
  {
    s1->out++;
    s1->dout   += s2->duration > 0?s2->duration:0;
    s1->eh     += s2->eh;
    s1->pay    += s2->pay;
  }

  s1->ibytes += s2->ibytes;
  s1->obytes += s2->obytes;
  return 0;
}

/*****************************************************************************/

static int clear_sum(sum_calls *s1)
{
  s1->in     = 0;
  s1->out    = 0;
  s1->eh     = 0;
  s1->err    = 0;
  s1->din    = 0;
  s1->dout   = 0;
  s1->pay    = 0;
  s1->ibytes = 0L;
  s1->obytes = 0L;

  return 0;
}

/*****************************************************************************/

static int add_sum_calls(sum_calls *s1, sum_calls *s2)
{
  s1->in     += s2->in;
  s1->out    += s2->out;
  s1->eh     += s2->eh;
  s1->err    += s2->err;
  s1->din    += s2->din;
  s1->dout   += s2->dout;
  s1->pay    += s2->pay;
  s1->ibytes += s2->ibytes;
  s1->obytes += s2->obytes;
  return 0;
}

/*****************************************************************************/

static void strich(int type)
{
	if (html)
	{
		switch (type) {
			case 3 :
			case 1 : printf(H_LINE,get_format_size(),1);
			         break;
			case 2 : printf(H_LINE,get_format_size(),3);
			         break;
                        default: print_msg(PRT_ERR, "isdnrep: internal error: invalid line flag!\n");
			         break;
		} /* switch */
  }
  else
	{
		char *string;
		int size = set_col_size();

		if ((string = (char*) calloc(size+1,sizeof(char))) == NULL)
    {
      print_msg(PRT_ERR, nomemory);
      return;
    }

		while (size>0)
			string[--size] = type==2?'=':'-';

		printf("%s\n",string);

		free(string);
  }
} /* strich */

/*****************************************************************************/

static int n_match(char *Pattern, char* Number, char* version)
{
        int RetCode = UNKNOWN;
	char s[SHORT_STRING_SIZE];

        if (!strcmp(version,LOG_VERSION_3) || !strcmp(version,LOG_VERSION_4) || !strcmp(version,LOG_VERSION))
	{
		RetCode = num_match(Pattern,Number);
	}
	else
	if (!strcmp(version,LOG_VERSION_2))
	{
		strcpy(s,expand_number(Number));
		RetCode = num_match(Pattern,s);
	}
	else
	if (!strcmp(version,LOG_VERSION_1))
	{
		if ((RetCode = match(Pattern, Number,0)) != 0            &&
		    !strncmp(Pattern,areaprefix,strlen(areaprefix))  )
		{
			sprintf(s,"*%s%s",myarea/*+strlen(areaprefix)*/,Pattern);
			RetCode = match(s,Number,0);
		}
	}
	else
		print_msg(PRT_ERR, "isdnrep: unknown version of logfile entries!\n");

	return RetCode;
}

/*****************************************************************************/

static int html_header(void)
{
	printf("Content-Type: text/html\n\n");
	printf("<HTML>\n");
	printf("<BODY bgcolor=%s>\n",H_BG_COLOR);

	return 0;
}

/*****************************************************************************/

static int html_bottom(char *_progname, char *start, char *stop)
{
	int value;
	int value2;
	char *progname = strdup(_progname);
	char *ptr      = strrchr(progname,'.');


	if (ptr)
		*ptr = '\0';

/*
	printf(H_FORM_ON,_myname);
	printf(H_FORM_DAY,"Date:","-t",40,10);
	printf(H_FORM_OFF);
*/

	if (!incomingonly)
	{
		value = incomingonly;
		value2 = outgoingonly;
		incomingonly++;
		outgoingonly = 0;
		if ((ptr = get_time_string(_begintime,endtime,0,0)) != NULL)
			printf(H_LINK_DAY,_myname,ptr,"incoming only");
		incomingonly = value;
		outgoingonly = value2;
	}

	if (!outgoingonly)
	{
		value = incomingonly;
		value2 = outgoingonly;
		incomingonly = 0;
		outgoingonly++;
		if ((ptr = get_time_string(_begintime,endtime,0,0)) != NULL)
			printf(H_LINK_DAY,_myname,ptr,"outgoing only");
		incomingonly = value;
		outgoingonly = value2;
	}

	if (outgoingonly || incomingonly)
	{
		value = incomingonly;
		value2 = outgoingonly;
		incomingonly = outgoingonly = 0;
		if ((ptr = get_time_string(_begintime,endtime,0,0)) != NULL)
			printf(H_LINK_DAY,_myname,ptr,"all calls");
		incomingonly = value;
		outgoingonly = value2;
	}

	value = print_failed;
	print_failed = !print_failed;
	if ((ptr = get_time_string(_begintime,endtime,0,0)) != NULL)
		printf(H_LINK_DAY,_myname,ptr,value?"print_failed off":"print_failed on");
	print_failed = value;

	if ((ptr = get_time_string(_begintime,endtime,0,-1)) != NULL)
		printf(H_LINK_DAY,_myname,ptr,"previous month");

	if ((ptr = get_time_string(_begintime,endtime,-1,0)) != NULL)
		printf(H_LINK_DAY,_myname,ptr,"previous day");

	if ((ptr = get_time_string(_begintime,endtime,1,0)) != NULL)
		printf(H_LINK_DAY,_myname,ptr,"next day");

	if ((ptr = get_time_string(_begintime,endtime,0,1)) != NULL)
		printf(H_LINK_DAY,_myname,ptr,"next month");

	printf("\n<BR><H6>%s %s  %s</H6>\n",progname,VERSION,__DATE__);
	printf("\n</BODY>\n");
	printf("<HEAD><TITLE>%s %s\n",progname,print_diff_date(start,stop));
	printf("</TITLE>\n");
	printf("</HTML>\n");

	free(progname);
	return 0;
}

/*****************************************************************************/

static char *print_diff_date(char *start, char *stop)
{
	static char RetCode[64];

	if (strcmp(start,stop))
		sprintf(RetCode,"%s .. %s",start,stop);
	else
		sprintf(RetCode,"%s",start);

	return RetCode;
}

/*****************************************************************************/

static int get_file_list(void)
{
	if (read_path & F_VBOX)
		set_dir_entries(vboxpath,set_vbox_entry);

	if (read_path & F_FAX)
		set_dir_entries(mgettypath,set_mgetty_entry);

	qsort(file_root,file_root_member,sizeof(file_list*),Compare_files);
	return 0;
}

/*****************************************************************************/

static int set_dir_entries(char *directory, int (*set_fct)(const char *, const char *))
{
	struct dirent *eptr;
	DIR *dptr;

	if (directory != NULL)
	{
		if ((dptr = opendir(directory)) != NULL)
		{
			while ((eptr = readdir(dptr)) != NULL)
			{
				if (eptr->d_name[0] != '.')
				{
					if (set_fct(directory,eptr->d_name))
                                                return(UNKNOWN);
				}
			}

			closedir(dptr);
		}
		else
		{
			print_msg(PRT_ERR, "isdnrep: can't open directory `%s': %s!\n", directory, strerror(errno));
                        return(UNKNOWN);
		}
	}

	return 0;
}

/*****************************************************************************/

static int set_vbox_entry(const char *path, const char *file)
{
	struct tm tm;
	file_list *lptr = NULL;
	char string[PATH_MAX];
	int cnt;
	FILE *fp;
	vaheader_t ptr;



	sprintf(string,"%s%c%s",path,C_SLASH,file);

	if ((fp = fopen(string,"r")) == NULL)
	{
		print_msg(PRT_ERR, msg1, string, strerror(errno));
                return(UNKNOWN);
	}

	fread(&ptr,sizeof(vaheader_t),1,fp);
	fclose(fp);

	if (strncmp(ptr.magic,"VBOX",4))
	{
		/* Version 0.x and 1.x of vbox! */

		if ((cnt = sscanf(file,"%2d%2d%2d%2d%2d%2d",
	 		            &(tm.tm_year),
	 		            &(tm.tm_mon),
	 		            &(tm.tm_mday),
	 		            &(tm.tm_hour),
	 		            &(tm.tm_min),
	 		            &(tm.tm_sec))) != 6)
		{
			print_msg(PRT_ERR, "isdnrep: invalid file name `%s'!\n",file);
                        return(UNKNOWN);
		}

		if ((lptr = (file_list*) calloc(1,sizeof(file_list))) == NULL)
		{
			print_msg(PRT_ERR, nomemory);
                        return(UNKNOWN);
		}

	 	if (tm.tm_mday > 31)
	 	{
	 		int help = tm.tm_mday;
	 		tm.tm_mday = tm.tm_year;
	 		tm.tm_year = help;
	 	}

		tm.tm_mon--;
                tm.tm_isdst = UNKNOWN;

		lptr->name = strdup(file);
	 	lptr->time = mktime(&tm);
		lptr->type = C_VBOX;
		lptr->used = 0;
		lptr->version = 1;
		lptr->compression = 0;
	}
	else
	{
		/* Version 2.x of vbox! */

		if ((lptr = (file_list*) calloc(1,sizeof(file_list))) == NULL)
		{
			print_msg(PRT_ERR, nomemory);
                        return(UNKNOWN);
		}

		lptr->name = strdup(file);
	 	lptr->time = ntohl(ptr.time);
		lptr->type = C_VBOX;
		lptr->used = 0;
		lptr->version = 2;
		lptr->compression = ntohl(ptr.compression);
	}
/*
	else
	{
		print_msg(PRT_ERR, "Version %d of vbox is not implemented yet!\n",vboxversion);
		print_msg(PRT_ERR, "Invalid version %d of vbox!\n",vboxversion);
                return(UNKNOWN);
	}
*/

	return set_element_list(lptr);
}

/*****************************************************************************/

static int set_mgetty_entry(const char *path, const char *file)
{
	file_list *lptr = NULL;
	char string[PATH_MAX];

	sprintf(string,"%s%c%s",path,C_SLASH,file);

	if (access(string,R_OK))
	{
		print_msg(PRT_ERR, msg1, string, strerror(errno));
                return(UNKNOWN);
	}

	if ((lptr = (file_list*) calloc(1,sizeof(file_list))) == NULL)
	{
		print_msg(PRT_ERR, nomemory);
                return(UNKNOWN);
	}

	lptr->name = strdup(file);

	strcpy(string,"0x3");
	strncpy(string+3,file+2,7);
	lptr->time = strtol(string,NIL,0);

	lptr->type = C_FAX;
	lptr->used = 0;
	lptr->version = 3;
	lptr->compression = 0;

	return set_element_list(lptr);
}

/*****************************************************************************/

static int set_element_list(file_list* elem)
{
	if (file_root_size == file_root_member)
	{
		file_root_size += 10;
		if ((file_root = (file_list**) realloc(file_root,sizeof(file_list*)*file_root_size)) == NULL)
		{
			print_msg(PRT_ERR, nomemory);
                        return(UNKNOWN);
		}
	}

	file_root[file_root_member++] = elem;

	return 0;
}

/*****************************************************************************/

static int Compare_files(const void *e1, const void *e2)
{
	if ((*(file_list**) e1)->time > (*(file_list**) e2)->time)
		return 1;
	else
	if ((*(file_list**) e1)->time < (*(file_list**) e2)->time)
                return(UNKNOWN);

	return 0;
}

/*****************************************************************************/

static file_list *get_file_to_call(time_t filetime, char type)
{
	static file_list **ptr = NULL;
	static int         cnt;
	static time_t      _filetime;
	int interval;

	if (filetime == 0)
	{
		if (ptr != NULL)
		{
			ptr++;
			cnt--;
		}
	}
	else
	{
		ptr = file_root;
		cnt = file_root_member;
		_filetime = filetime;
	}

	while (cnt > 0 && (interval = time_in_interval((*ptr)->time,_filetime,type)) < 1)
	{
		if (interval == 0 && (*ptr)->used == 0 && (*ptr)->type == type)
		{
			(*ptr)->used = 1;
			return (*ptr);
		}

		ptr++;
		cnt--;
	}

	return NULL;
}

/*****************************************************************************/

static int time_in_interval(time_t t1, time_t t2, char type)
{
	int begin;
	int end;

	switch (type)
	{
		case C_VBOX : begin = 0;
		              end   = 10;
		              break;
		case C_FAX  : begin = -2;
		              end   = 1;
		              break;
		default     : begin = 0;
		              end   = 0;
		              break;
	}

	if ((int) t1 < ((int) t2) + begin)
                return(-1);

	if ((int) t1 > ((int) t2) + end)
                return(1);

	return 0;
}

/*****************************************************************************/

static char *get_links(time_t filetime, char type)
{
	static char *string[5] = {NULL,NULL,NULL,NULL,NULL};
        static int   cnt = UNKNOWN;
	file_list   *ptr;


	cnt = (cnt+1) % 5;
	free(string[cnt]);
	string[cnt] = NULL;

	if (type == C_VBOX)
	{
		if ((ptr = get_file_to_call(filetime,type)) != NULL)
		{
			if ((string[cnt] = (char*) calloc(SHORT_STRING_SIZE,sizeof(char))) == NULL)
			{
				print_msg(PRT_ERR, nomemory);
				return NULL;
			}

			sprintf(string[cnt],H_LINK,_myname,type,ptr->version,nam2html(ptr->name),"phone");
		}
	}
	else
	if (type == C_FAX)
	{
		while((ptr = get_file_to_call(filetime,type)) != NULL)
		{
			append_fax(&(string[cnt]),ptr->name,type,ptr->version);
			filetime = 0;
		}
	}
	else
	{
		print_msg(PRT_ERR, "isdnrep: internal error: invalid type %d of file!\n", (int)type);
		return NULL;
	}

	return string[cnt]?string[cnt]:"        ";
}

/*****************************************************************************/

static char *append_fax(char **string, char *file, char type, int version)
{
	static int cnt = 0;
	char help[BUFSIZ];
	char help2[20];

	if (*string == NULL)
		cnt = 1;
	else
		cnt++;

	sprintf(help2,"&lt;%d&gt;",cnt);
	sprintf(help,H_LINK,_myname,type,version,nam2html(file),help2);

	if (*string == NULL)
		*string = strdup(STR_FAX);

	if ((*string = (char*) realloc(*string,sizeof(char)*(strlen(*string)+strlen(help)+2))) == NULL)
	{
		print_msg(PRT_ERR, nomemory);
		return NULL;
	}

	strcat(*string,help);
	strcat(*string," ");

	return *string;
}

/*****************************************************************************/

static char *nam2html(char *file)
{
	static char RetCode[SHORT_STRING_SIZE];
	char *ptr = RetCode;

	while (*file != '\0')
	{
		switch (*file)
		{
			case '+': strcpy(ptr,"%2b");
			          ptr += 3;
			          file++;
			          break;
			default : *ptr++ = *file++;
			          break;
		}
	}

	*ptr = '\0';

	return RetCode;
}

/*****************************************************************************/

static char *get_a_day(time_t t, int d_diff, int m_diff, int flag)
{
	static char string[16];
	struct tm *tm;
	time_t cur_time;
	char   flag2 = 0;


	if (t == 0)
	{
		if (flag & F_END)
			return NULL;

		time(&t);
		flag2 = 1;
	}

	if (flag & F_END)
		t++;

	tm = localtime(&t);

	tm->tm_mday += d_diff;
	tm->tm_mon  += m_diff;
        tm->tm_isdst = UNKNOWN;

	if (flag2 == 1)
	{
		tm->tm_hour  = 0;
		tm->tm_min   = 0;
		tm->tm_sec   = 0;
	}

	t = mktime(tm);

	if (flag & F_END)
		t-=2;

	time(&cur_time);

	if (cur_time < t)
	{
		if (!(flag & F_END))
			return NULL;
	/*
		if (flag & F_END)
			t = cur_time;
		else
			return NULL;
	*/
	}

	tm = localtime(&t);
	sprintf(string,"%02d%02d%02d%02d%04d.%02d",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_year+1900,tm->tm_sec);

	return string;
}

/*****************************************************************************/

static char *get_time_string(time_t begin, time_t end, int d_diff, int m_diff)
{
	static char string[256];
	char *ptr = NULL;


	strcpy(string,get_default_html_params());

	if ((ptr = get_a_day(begin,d_diff,m_diff,F_BEGIN)) != NULL)
	{
		strcat(string,"+-t");
		strcat(string,ptr);

		if ((ptr = get_a_day(end,d_diff,m_diff,F_END)) != NULL)
		{
			strcat(string,"-");
			strcat(string,ptr);
		}

		return string;
	}

	return NULL;
}

/*****************************************************************************/

static char *get_default_html_params(void)
{
	static char string[50];

	sprintf(string,"-w%d%s%s%s",
	                        html-1,
	/*                         ^^---sehr gefaehrlich, da eine UND-Verknuepfung!!! */
	                        print_failed?"+-E":"",
	                        outgoingonly?"+-o":"",
	                        incomingonly?"+-i":"");
	return string;
}

/*****************************************************************************/

static char *create_vbox_file(char *file, int *compression)
{
	int fdin, fdout, len;
	char string[BUFSIZ];
	char *fileout = NULL;
	vaheader_t header;

	if ((fdin = open(file,O_RDONLY)) == -1)
	{
		print_msg(PRT_ERR, msg1, file, strerror(errno));
		return NULL;
	}

	if (read(fdin,&header,sizeof(vaheader_t)) == sizeof(vaheader_t))
	{
		if (compression != NULL)
			*compression = ntohl(header.compression);

		fileout = strdup("/tmp/isdnrepXXXXXX");
		if( (fdout = mkstemp(fileout)) < 0 )
		{
			print_msg(PRT_ERR, msg1, fileout, strerror(errno));
			close(fdin);
			free(fileout);
			return NULL;
		}

		while((len = read(fdin,string,BUFSIZ)) > 0)
		{
			if (write(fdout,string,len) != len)
			{
				print_msg(PRT_ERR, "isdnrep: can't write to file `%s': %s!\n", fileout, strerror(errno));
				close(fdout);
				close(fdin);
				unlink(fileout);
				free(fileout);
				return NULL;
			}
		}

		close(fdout);

		if (len < 0)
		{
			print_msg(PRT_ERR, "isdnrep: can't read from file `%s': %s!\n", file, strerror(errno));
			close(fdin);
			unlink(fileout);
			free(fileout);
			return NULL;
		}
	}

	close(fdin);
	return fileout;
}

/*****************************************************************************/

static int htoi(char *s)
{
	int	value;
	char c;

	c = s[0];
	if (isupper(c))
		c = tolower(c);
	value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

	c = s[1];
	if (isupper(c))
		c = tolower(c);
	value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

	return (value);
}

/*****************************************************************************/

static char *url_unescape(char *str)
{
	char *RetCode = str;
	char *dest = str;

	while (str[0])
	{
		if (str[0] == '+')
			dest[0] = ' ';
		else if (str[0] == '%' && isxdigit(str[1]) && isxdigit(str[2]))
		{
			dest[0] = (unsigned char) htoi(str + 1);
			str += 2;
		}
		else
			dest[0] = str[0];

		str++;
		dest++;
	}

	dest[0] = '\0';

	return RetCode;
}

/*****************************************************************************/

int new_args(int *nargc, char ***nargv)
{
	int index = *nargc;
	int index2 = 1;
	char **h_args;
	char *h_env;

	if ((h_env = getenv(H_ENV_VAR)) == NULL)
		return 1;

	if ((h_args = get_http_args(h_env,&index)) == NULL)
		return 2;

	while (index2 < *nargc)
		h_args[index++] = (*nargv)[index2++];

	h_args[0] = (*nargv)[0];
	h_args[index] = NULL;

	*nargc = index;
	*nargv = h_args;

unlink("/tmp/iii");
{
FILE *fp = fopen("/tmp/iii","w");
for(index2=0;index2 < index;index2++)
{
fputs(h_args[index2],fp);
fputs("*\n",fp);
}
fclose(fp);
}
	return 0;
}

/*****************************************************************************/

static char **get_http_args(char *str, int *index)
{
	char **RetCode = NULL;

	if (index == NULL)
		return NULL;

	if (*index < 0)
		*index = 0;

	str = url_unescape(strdup(str));

	if (str == NULL)
		return NULL;

	if (str == '\0')
	{
		free(str);
		return NULL;
	}

	if ((RetCode = (char**) calloc(20+(*index),sizeof(char*))) == NULL)
	{
		print_msg(PRT_ERR, nomemory);
		return NULL;
	}

	*index = 1;
	RetCode[0] = NULL;

	RetCode[(*index)++] = str;

	while(*str != '\0')
	{
		if (*str == ' ' || *str == '=')
		{
			*str++ = '\0';

			if (*str != ' ' && *str != '=' &&  *str != '\0')
				RetCode[(*index)++] = str;
		}
		else
			str++;
	}

	RetCode[*index] = NULL;

	return RetCode;
}

/*****************************************************************************/

static int find_format_length(char *string)
{
	int len;
	char type;
	char* dest;

	if (string == NULL)
                return(UNKNOWN);

	while (*string != '\0')
	{
		if (*string++ == '%')
		{
			if (*string == '%')
			{
				string++;
				continue;
			}

			while(index("0123456789-",*string)) string++;

			if (*string == '\0')
                                return(UNKNOWN);

			if (*string == '.')
			{
				if (sscanf(++string,"%d%c",&len,&type) == 2 && type == 's' && len >= 0)
				{
					if ((dest = index(--string,'s')) == NULL)
                                                return(UNKNOWN);

					memmove(string,dest,strlen(dest)+1);

					return len;
				}

				if (*string == 's')
					return 0;

                                return(UNKNOWN);
			}
			else if (*string == 's')
                                return(UNKNOWN);

		}
	}

        return(UNKNOWN);
}

/*****************************************************************************/

static int app_fmt_string(char *target, int targetlen, char *fmt, int condition, char *value)
{
	char tmpfmt[BUFSIZ];
	char tmpvalue[BUFSIZ];
	int len;

	strcpy(tmpfmt,fmt);
	strcpy(tmpvalue,value);
	len = find_format_length(tmpfmt);

        if (len != UNKNOWN && len > 0 && strlen(tmpvalue) > len)
		tmpvalue[len] = '\0';

	value = condition?html_conv(tmpvalue):tmpvalue;

	return snprintf(target,targetlen,tmpfmt,value);
}

/* vim:set ts=2: */
/*****************************************************************************/
