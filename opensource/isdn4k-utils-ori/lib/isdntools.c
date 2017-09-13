/* $Id: isdntools.c,v 1.28 2001/08/18 11:59:01 paul Exp $
 *
 * ISDN accounting for isdn4linux. (Utilities)
 *
 * Copyright 1995, 1997 by Stefan Luethje (luethje@sl-gw.lake.de)
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
 * $Log: isdntools.c,v $
 * Revision 1.28  2001/08/18 11:59:01  paul
 * Added missing endpwent() call that meant /etc/passwd was being kept open;
 * reorganized num_match() a bit; made the arrays in expand_number() one byte
 * bigger to prevent possible off-by-one buffer overflow.
 *
 * Revision 1.27  2000/09/05 08:05:03  paul
 * Now isdnlog doesn't use any more ISDN_XX defines to determine the way it works.
 * It now uses the value of "COUNTRYCODE = 999" to determine the country, and sets
 * a variable mycountrynum to that value. That is then used in the code to set the
 * way isdnlog works.
 * It works for me, please check it! No configure.in / doc changes yet until
 * it has been checked to work.
 * So finally a version of isdnlog that can be compiled and distributed
 * internationally.
 *
 * Revision 1.26  1999/08/20 19:43:46  akool
 * removed avon-, vorwahl- and areacodes-support
 *
 * Revision 1.25  1999/06/11 15:46:54  akool
 * not required references to libndbm removed
 *
 * Revision 1.24  1998/12/16 20:57:30  akool
 *  - first try to add the 1999 tarif of the German Telekom
 *  - fix the areacode 2.0 support
 *
 * Revision 1.23  1998/10/13 21:53:26  luethje
 * isdnrep and lib: bugfixes
 *
 * Revision 1.22  1998/09/26 18:30:30  akool
 *  - quick and dirty Call-History in "-m" Mode (press "h" for more info) added
 *    - eat's one more socket, Stefan: sockets[3] now is STDIN, FIRST_DESCR=4 !!
 *  - Support for tesion)) Baden-Wuerttemberg Tarif
 *  - more Providers
 *  - Patches from Wilfried Teiken <wteiken@terminus.cl-ki.uni-osnabrueck.de>
 *    - better zone-info support in "tools/isdnconf.c"
 *    - buffer-overrun in "isdntools.c" fixed
 *  - big Austrian Patch from Michael Reinelt <reinelt@eunet.at>
 *    - added $(DESTDIR) in any "Makefile.in"
 *    - new Configure-Switches "ISDN_AT" and "ISDN_DE"
 *      - splitted "takt.c" and "tools.c" into
 *          "takt_at.c" / "takt_de.c" ...
 *          "tools_at.c" / "takt_de.c" ...
 *    - new feature
 *        CALLFILE = /var/log/caller.log
 *        CALLFMT  = %b %e %T %N7 %N3 %N4 %N5 %N6
 *      in "isdn.conf"
 *  - ATTENTION:
 *      1. "isdnrep" dies with an seg-fault, if not HTML-Mode (Stefan?)
 *      2. "isdnlog/Makefile.in" now has hardcoded "ISDN_DE" in "DEFS"
 *      	should be fixed soon
 *
 * Revision 1.21  1998/06/07 21:03:26  akool
 * Renamed old to new zone-names (CityCall, RegioCall, GermanCall, GlobalCall)
 *
 * Revision 1.20  1998/05/11 19:43:49  luethje
 * Some changes for "vorwahlen.dat"
 *
 * Revision 1.19  1998/05/10 22:12:01  luethje
 * Added support for VORWAHLEN2.EXE
 *
 * Revision 1.18  1998/04/28 08:34:36  paul
 * Fixed compiler warnings from egcs.
 *
 * Revision 1.17  1998/03/08 12:13:49  luethje
 * Patches by Paul Slootman
 *
 * Revision 1.16  1997/06/22 22:57:08  luethje
 * bugfixes
 *
 * Revision 1.15  1997/06/15 23:50:34  luethje
 * some bugfixes
 *
 * Revision 1.14  1997/05/19 23:37:05  luethje
 * bugfix for isdnconf
 *
 * Revision 1.13  1997/05/19 22:58:28  luethje
 * - bugfix: it is possible to install isdnlog now
 * - improved performance for read files for vbox files and mgetty files.
 * - it is possible to decide via config if you want to use avon or
 *   areacode.
 *
 * Revision 1.12  1997/05/09 23:31:06  luethje
 * isdnlog: new switch -O
 * isdnrep: new format %S
 * bugfix in handle_runfiles()
 *
 * Revision 1.11  1997/04/15 00:20:17  luethje
 * replace variables: some bugfixes, README comleted
 *
 * Revision 1.10  1997/04/08 21:57:04  luethje
 * Create the file isdn.conf
 * some bug fixes for pid and lock file
 * make the prefix of the code in `isdn.conf' variable
 *
 * Revision 1.9  1997/04/08 00:02:24  luethje
 * Bugfix: isdnlog is running again ;-)
 * isdnlog creates now a file like /var/lock/LCK..isdnctrl0
 * README completed
 * Added some values (countrycode, areacode, lock dir and lock file) to
 * the global menu
 *
 * Revision 1.8  1997/04/03 22:39:13  luethje
 * bug fixes: environ variables are working again, no seg. 11 :-)
 * improved performance for reading the config files.
 *
 * Revision 1.7  1997/03/20 00:19:27  luethje
 * inserted the line #include <errno.h> in avmb1/avmcapictrl.c and imon/imon.c,
 * some bugfixes, new structure in isdnlog/isdnrep/isdnrep.c.
 *
 * Revision 1.6  1997/03/19 00:08:43  luethje
 * README and function expand_number() completed.
 *
 * Revision 1.5  1997/03/18 23:01:50  luethje
 * Function Compare_Sections() completed.
 *
 * Revision 1.4  1997/03/07 23:34:49  luethje
 * README.conffile completed, paranoid_check() used by read_conffiles(),
 * policy.h will be removed by "make distclean".
 *
 * Revision 1.3  1997/03/06 20:36:34  luethje
 * Problem in create_runfie() fixed. New function paranoia_check() implemented.
 *
 * Revision 1.2  1997/03/03 22:05:39  luethje
 * merging of the current version and my tree
 *
 * Revision 2.6.26  1997/01/19  22:23:43  akool
 * Weitere well-known number's hinzugefuegt
 *
 * Revision 2.6.24  1997/01/15  19:13:43  akool
 * neue AreaCode Lib 0.99 integriert
 *
 * Revision 2.6.20  1997/01/05  20:06:43  akool
 * atom() erkennt nun "non isdnlog" "/tmp/isdnctrl0" Output's
 *
 * Revision 2.6.19  1997/01/05  19:39:43  akool
 * AREACODE Support added
 *
 * Revision 2.40    1996/06/16  10:06:43  akool
 * double2byte(), time2str() added
 *
 * Revision 2.3.26  1996/05/05  12:09:16  akool
 * known.interface added
 *
 * Revision 2.3.15  1996/04/22  21:10:16  akool
 *
 * Revision 2.3.4  1996/04/05  11:12:16  sl
 * confdir()
 *
 * Revision 2.2.5  1996/03/25  19:41:16  akool
 * 1TR6 causes implemented
 *
 * Revision 2.23  1996/03/14  20:29:16  akool
 * Neue Routine i2a()
 *
 * Revision 2.17  1996/02/25  19:14:16  akool
 * Soft-Error in atom() abgefangen
 *
 * Revision 2.06  1996/02/07  18:49:16  akool
 * AVON-Handling implementiert
 *
 * Revision 2.01  1996/01/20  12:11:16  akool
 * Um Einlesen der neuen isdnlog.conf Felder erweitert
 * discardconfig() implementiert
 *
 * Revision 2.00  1996/01/10  20:11:16  akool
 *
 */

/****************************************************************************/


#define  PUBLIC /**/
#define  _ISDNTOOLS_C_
#define  _GNU_SOURCE

/****************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "libisdn.h"

/****************************************************************************/

#define GERMAN_CODE 49

/****************************************************************************/

typedef struct {
	char code[15];
	long int pointer;
} s_areacode;

/****************************************************************************/

static int (*print_msg)(const char *, ...) = printf;
#if 0 /* DELETE_ME AK:18-Aug-99 */
#ifdef LIBAREA
static char *_get_areacode(char *code, int *Len, int flag);
#else
static char *_get_avon(char *code, int *Len, int flag);
#endif
#endif
static int create_runfile(const char *file, const char *format);
#if 0 /* DELETE_ME AK:18-Aug-99 */
static long int area_read_value(FILE *fp, int size);
static int area_read_file(void);
static int area_get_index(char *code);
#endif

/****************************************************************************/

#if 0 /* DELETE_ME AK:18-Aug-99 */
static char areacodes[][2][30] = {
	{"+49130", "Toll-free"},
	{"+491802", "Service 180-2 (0,12/Anruf)"},
	{"+491803", "Service 180-3 (DM 0,24/Anruf)"},
	{"+491805", "Service 180-5 (DM 0,48/Anruf)"},
	{"+491901", "Service 190 (DM 1,20/Minute)"},
	{"+491902", "Service 190 (DM 1,20/Minute)"},
	{"+491903", "Service 190 (DM 1,20/Minute)"},
	{"+491904", "Service 190 (DM 0,80/Minute)"},
	{"+491905", "Service 190 (DM 1,20/Minute)"},
	{"+491906", "Service 190 (DM 0,80/Minute)"},
	{"+491907", "Service 190 (DM 2,40/Minute)"},
	{"+491908", "Service 190 (DM 3,60/Minute)"},
	{"+491909", "Service 190 (DM 2,40/Minute)"},
	{"+49161", "Mobilfunknetz C"},
	{"+49171", "Mobilfunknetz D1"},
	{"+49172", "Mobilfunknetz D2"},
	{"+49177", "Mobilfunknetz E-Plus"},
	{"+491188", "Auskunft Inland"},
	{"+491910", "T-Online"},
	{"", ""},
};

static char countrycodes[][2][30] = {
	{"+30", "Greece"},
	{"+31", "Netherlands"},
	{"+32", "Belgium"},
	{"+33", "France"},
	{"+34", "Spain"},
	{"+39", "Italy"},
	{"+41", "Switzerland"},
	{"+43", "Austria"},
	{"+44", "Great Britain"},
	{"+45", "Denmark"},
	{"+46", "Sweden"},
	{"+47", "Norway"},
	{"+49", "Germany"},
	{"+352", "Luxemburg"},
	{"+1", "United States"},
	{"", ""},
};

static char *avonlib = NULL;
static char *codelib = NULL;
static s_areacode *codes = NULL;
static int codes_number = 0;
#endif

/****************************************************************************/

void set_print_fct_for_lib(int (*new_print_msg)(const char *, ...))
{
	print_msg = new_print_msg;
	set_print_fct_for_conffile(new_print_msg);
	set_print_fct_for_libtools(new_print_msg);
#if 0 /* DELETE_ME AK:18-Aug-99 */
	set_print_fct_for_avon(new_print_msg);
#endif
}

/****************************************************************************/

int num_match(char* Pattern, char *number)
{
	int RetCode = -1;
	char **Ptr;
	char **Array;

	if (!strcmp(Pattern, number)) /* match */
		return 0;

	if (!strchr(Pattern,C_NUM_DELIM))
    return match(expand_number(Pattern), number, 0);

	Ptr = Array = String_to_Array(Pattern,C_NUM_DELIM);

	while (*Ptr != NULL && RetCode != 0)
	{
		RetCode = match(expand_number(*Ptr), number, 0);
		Ptr++;
	}

	del_Array(Array);

	return RetCode;
}

/****************************************************************************/

char *expand_number(char *s)
{
	int all_allowed = 0;
	char *Ptr;
	int   Index = 0;
	char Help[NUMBER_SIZE+1];
	static char Num[NUMBER_SIZE+1];


	Help[0] = '\0';
	Ptr = s;

	if (Ptr == NULL || Ptr[0] == '\0')
		return "";

	while(isblank(*Ptr))
		Ptr++;

	if (*Ptr  == '+')
	{
		Strncpy(Help,countryprefix,NUMBER_SIZE);
		Ptr++;
	}

	Index = strlen(Help);

	while(*Ptr != '\0')
	{
		if (*Ptr == ',' || Index >= NUMBER_SIZE)
			break;

		if (isdigit(*Ptr) || *Ptr == '?' || *Ptr == '*'|| 
		    *Ptr == '[' ||  *Ptr == ']' || all_allowed   )
		{
			if (*Ptr == '[')
				all_allowed  = 1;

			if (*Ptr == ']')
				all_allowed  = 0;

			Help[Index++] = *Ptr;
		}

		Ptr++;
	}

	Help[Index] = '\0';

	if (Help[0] == '\0')
		return s;

	if (Help[0] == '*' || !strncmp(Help,countryprefix,strlen(countryprefix)))
	{
		strcpy(Num,Help);
	}
	else
	if (!strncmp(Help,areaprefix,strlen(areaprefix)))
	{
		strcpy(Num,mycountry);
		strcat(Num,Help+strlen(areaprefix));
	}
	else
	{
		strcpy(Num,mycountry);
		strcat(Num,myarea/*+strlen(areaprefix)*/);
		strcat(Num,Help);
	}

	return Num;
}

/****************************************************************************/

char *expand_file(char *s)
{
	char *Ptr;
	uid_t id = -1;
	char  Help[PATH_MAX];
	static char file[PATH_MAX];
	struct passwd *password;


	Help[0] = '\0';
	file[0] = '\0';

	if (s == NULL)
		return NULL;

	if (s[0] == '~')
	{
		if (s[1] == C_SLASH)
		{
			/* Ghandi, vielleicht kommt hier auch getuid() hin */
			id = geteuid();
		}
		else
		{
			strcpy(Help,s+1);
			if ((Ptr = strchr(Help,C_SLASH)) != NULL)
				*Ptr = '\0';
			else
				return NULL;
		}

		setpwent();
		while((password = getpwent()) != NULL &&
		      strcmp(password->pw_name,Help)  &&
		      password->pw_uid != id)
        ;
    endpwent();

		if (password == NULL)
			return NULL;

		strcpy(file,password->pw_dir);
		strcat(file,strchr(s,C_SLASH));
	}

	return (file[0] == '\0'?s:file);
}

/****************************************************************************/

char *confdir(void)
{
  static char *confdirvar = NULL;

	if (confdirvar == NULL && (confdirvar = getenv(CONFDIR_VAR)) == NULL)
    confdirvar = I4LCONFDIR;

  return(confdirvar);
} /* confdir */

/****************************************************************************/

int handle_runfiles(const char *_progname, char **_devices, int flag)
{
	static char   progname[SHORT_STRING_SIZE] = "";
  static char **devices = NULL;
  auto   char **mydevices = NULL;
  auto   char   string[PATH_MAX];
  auto   char   string2[SHORT_STRING_SIZE];
  auto   char  *Ptr = NULL;
  auto   int    RetCode = -1;
	auto   FILE  *fp;


	if (progname[0] == '\0' || devices == NULL)
	{
		if (_progname == NULL || _devices == NULL)
			return -1;

		Ptr = strrchr(progname,C_SLASH);
		strcpy(progname,Ptr?Ptr+1:_progname);

		while (*_devices != NULL)
		{
			Ptr = strrchr(*_devices,C_SLASH);
			append_element(&devices,Ptr?Ptr+1:*_devices);
			_devices++;
		}
	}

	if (flag == START_PROG)
	{
		sprintf(string,"%s%c%s.%s.pid",RUNDIR,C_SLASH,progname,devices[0]);

		if ((RetCode = create_runfile(string,"%d\n")) != 0)
		{
			if (RetCode > 0)
			{
				print_msg("Another %s is running with pid %d!\n", progname, RetCode);
				print_msg("If not delete the file `%s' and try it again!\n", string);
			}

			return RetCode;
		}

		mydevices = devices;

		while (*mydevices != NULL)
		{
			sprintf(string,"%s%c%s%s",LOCKDIR,C_SLASH,LOCKFILE,*mydevices);

			if ((RetCode = create_runfile(string,"%10d\n")) != 0)
			{
				if (RetCode > 0)
					print_msg("Another process (pid=%d) is running on device %s!\n", RetCode, *mydevices);

				return RetCode;
			}

			mydevices++;
		}

		RetCode = 0;
	}

	if (flag == STOP_PROG)
	{
		sprintf(string,"%s%c%s.%s.pid",RUNDIR,C_SLASH,progname,devices[0]);

		if ((fp = fopen(string, "r")) != NULL)
		{
			if (fgets(string2,SHORT_STRING_SIZE,fp) != NULL)
			{
				if (atoi(string2) == (int)getpid())
				{
					if (unlink(string))
						print_msg("Can not remove file %s (%s)!\n", string, strerror(errno));
					else
						print_msg("File %s removed!\n", string, strerror(errno));
				}
				else
					print_msg("This is not my lock file `%s': Has PID %d!\n", string, atoi(string2));
			}

			fclose(fp);
		}

		while (*devices != NULL)
		{
			sprintf(string,"%s%c%s%s",LOCKDIR,C_SLASH,LOCKFILE,*devices);

			if ((fp = fopen(string, "r")) != NULL)
			{
				if (fgets(string2,SHORT_STRING_SIZE,fp) != NULL)
				{
					if (atoi(string2) == (int)getpid())
					{
						if (unlink(string))
							print_msg("Can not remove file %s (%s)!\n", string, strerror(errno));
						else
							print_msg("File %s removed!\n", string, strerror(errno));
					}
					else
						print_msg("This is not my lock file `%s': Has PID %d!\n", string, atoi(string2));
				}

				fclose(fp);
			}

			devices++;
		}

		RetCode = 0;
	}

	return RetCode;
}

/****************************************************************************/

static int create_runfile(const char *file, const char *format)
{
	auto char  string[SHORT_STRING_SIZE];
	auto int   RetCode = -1;
	auto int   fd      = -1;
	auto FILE *fp;

	if (file == NULL)
		return -1;

	if ((fd = open(file, O_WRONLY|O_CREAT|O_EXCL|O_TRUNC, 0644)) >= 0)
	{
		sprintf(string, format, (int)getpid());

		if (write(fd, string, strlen(string)) != strlen(string) )
		{
			print_msg("Can not write to PID file `%s'!\n", file);
  		RetCode = -1;
		}
  	else
  		RetCode = 0;

		close(fd);
	}
	else
	{
		if ((fp = fopen(file, "r")) == NULL)
			return -1;

		if (fgets(string,SHORT_STRING_SIZE,fp) != NULL)
			RetCode = atoi(string);
		else
			/* Datei ist leer. */
			RetCode = -1;

		if ( RetCode == -1 || (int)getpid() == RetCode                           ||
		    ((int) getpid() != RetCode && kill(RetCode,0) != 0 && errno == ESRCH)  )
		    /* Wenn der alte Prozess nicht mehr existiert! */
		{

			fclose(fp);
			if (unlink(file))
				return -1;

			return create_runfile(file,format);
		}
		
		fclose(fp);
	}

	return RetCode;
} /* create_runfile */


/*****************************************************************************/
/*
 * set_country_behaviour() - different countries have small differences in
 * ISDN implementations. Use the COUNTRYCODE setting to select the behaviour
 */

static void set_country_behaviour(char *mycountry)
{
	/* amazing, strtol will also accept "+0049" */
	mycountrynum = strtol(mycountry, (char **)0, 10);
/*
 * There's no real point in testing for a known country code.
 * Setting an unknown countrycode to 49 (DE) is probably the worst
 * thing you can do if you're not actually in Germany. You might
 * give a warning, but do we really want that?  So simply accept
 * the given value  - Paul Slootman 20010818
 */
#if 0
	switch (mycountrynum) {
		case CCODE_NL:
		case CCODE_CH:
		case CCODE_AT:
		case CCODE_DE:
		case CCODE_LU:
		/* any more special cases ? */
			/* these only need to have mycountrynum set correctly */
			break;
		default:
			mycountrynum = 49; /* use Germany as default for now */
	}
#endif /* 0 */
}


/****************************************************************************/
/*
 * Sets the country codes that are used for the lib. This function must
 * be called by each program!
 */

#define _MAX_VARS 8

int Set_Codes(section* Section)
{
	static char *ptr[_MAX_VARS] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	int i;
	int RetCode = 0;
	entry *Entry;
	char *ptr2;
	char s[SHORT_STRING_SIZE];
	section *SPtr;

	for (i=0; i < _MAX_VARS; i++)
		if (ptr[i] != NULL)
		{
			free(ptr[i]);
			ptr[i] = NULL;
		}

	if ((SPtr = Get_Section(Section,CONF_SEC_GLOBAL)) == NULL)
		return -1;
#if 0 /* DELETE_ME AK:18-Aug-99 */
	if ((Entry = Get_Entry(SPtr->entries,CONF_ENT_AREALIB)) != NULL &&
	    Entry->value != NULL                                             )
		ptr[0] = acFileName = strdup(Entry->value);

	if ((Entry = Get_Entry(SPtr->entries,CONF_ENT_AVONLIB)) != NULL &&
	    Entry->value != NULL                                             )
		ptr[1] = avonlib = strdup(Entry->value);
	else
	{
		sprintf(s, "%s%c%s", confdir(), C_SLASH, AVON);
		ptr[1] = avonlib = strdup(s);
	}
#endif
	if ((Entry = Get_Entry(SPtr->entries,CONF_ENT_COUNTRY_PREFIX)) != NULL &&
	    Entry->value != NULL                                             )
		ptr[2] = countryprefix = strdup(Entry->value);

	if ((Entry = Get_Entry(SPtr->entries,CONF_ENT_AREA_PREFIX)) != NULL &&
	    Entry->value != NULL                                             )
		ptr[3] = areaprefix = strdup(Entry->value);
#if 0 /* DELETE_ME AK:18-Aug-99 */
	if ((Entry = Get_Entry(SPtr->entries,CONF_ENT_CODELIB)) != NULL &&
	    Entry->value != NULL                                             )
		ptr[4] = codelib = strdup(Entry->value);
#endif
	if ((Entry = Get_Entry(SPtr->entries,CONF_ENT_AREA)) != NULL &&
	    Entry->value != NULL                                          )
	{
		ptr2 = Entry->value;

		if (strncmp(Entry->value,areaprefix,strlen(areaprefix)))
			ptr[5] = myarea = strdup(ptr2);
		else
			ptr[5] = myarea = strdup(ptr2+strlen(areaprefix));

		if (ptr[5] != NULL)
			RetCode++;
		else
			print_msg("Error: Variable `%s' are not set!\n",CONF_ENT_AREA);
	}

	if ((Entry = Get_Entry(SPtr->entries,CONF_ENT_COUNTRY)) != NULL &&
	    Entry->value != NULL                                             )
	{
		ptr2 = Entry->value;

		if (strncmp(Entry->value,countryprefix,strlen(countryprefix)))
		{
			sprintf(s,"%s%s",countryprefix,
			                 Entry->value[0]=='+'?(Entry->value)+1:Entry->value);
			ptr2 = s;
		}
			
		if ((ptr[6] = mycountry = strdup(ptr2)) != NULL) {
			RetCode++;
			set_country_behaviour(mycountry);
		}
		else
			print_msg("Error: Variable `%s' are not set!\n",CONF_ENT_COUNTRY);
	}

#if 0 /* DELETE_ME AK:18-Aug-99 */
	if ((Entry = Get_Entry(SPtr->entries,CONF_ENT_AREADIFF)) != NULL &&
	    Entry->value != NULL                                             )
		ptr[7] = areadifffile = strdup(Entry->value);
	else
	{
		if ((areadifffile = (char*) calloc(strlen(confdir())+strlen(S_AREA_DIFF_FILE)+2,sizeof(char))) == NULL)
			print_msg("Can not allocate memory!\n");
		else
		{
			sprintf(areadifffile,"%s%c%s",confdir(),C_SLASH,S_AREA_DIFF_FILE);

			if (access(areadifffile,R_OK))
			{
				free(areadifffile);
				areadifffile = NULL;
			}
			else
				ptr[7] = areadifffile;
		}
	}
#endif
	SPtr = Section;

	while ((SPtr = Get_Section(SPtr,CONF_SEC_VAR)) != NULL)
	{
		Entry = SPtr->entries;

		while(Entry != NULL)
		{
			if (setenv(Entry->name, Entry->value, 1) != 0)
				print_msg("Warning: Can not set environment variable `%s=%s'!\n", Entry->name, Entry->value);

			Entry = Entry->next;
		}

		SPtr = SPtr->next;
	}

	return (RetCode==2?0:-1);
}

/****************************************************************************/

#if 0 /* DELETE_ME AK:18-Aug-99 */
char *get_areacode(char *code, int *Len, int flag)
{
	auto     char  *Ptr;
	auto     int    i = 0;
	register int    prefix = strlen(countryprefix);


	if (Len != NULL)
		*Len = -1;

	if (code == NULL || code[0] =='\0')
		return NULL;

	if (!(flag & C_NO_EXPAND))
	{
		char *ptr = expand_number(code);

		if ((code = alloca(strlen(ptr)+1)) == NULL)
			print_msg("Can not allocate memory!\n");

		strcpy(code,ptr);
	}

	while (areacodes[i][0][0] != '\0')
	{
		if (!memcmp(areacodes[i][0]+1,code+prefix,strlen(areacodes[i][0]+1)))
		{
			if (Len != NULL)
				*Len = strlen(areacodes[i][0]) - 1 /* das "+" */ + prefix;

			return areacodes[i][1];
		}

		i++;
	}

#if 0 /* DELETE_ME AK:18-Aug-99 */
#ifdef LIBAREA
	if (codelib != NULL && !strcasecmp(codelib,"AREACODE"))
		Ptr = _get_areacode(code,Len,flag);
#else
	if (codelib != NULL && !strcasecmp(codelib,"AVON"))
		Ptr = _get_avon(code,Len,flag);
#endif
	else
#ifdef LIBAREA
		Ptr = _get_areacode(code,Len,flag);
#else
		Ptr = _get_avon(code,Len,flag);
#endif
#endif

	if (Ptr != NULL)
		return Ptr;

	i=0;

	while (countrycodes[i][0][0] != '\0')
	{
		if (!memcmp(countrycodes[i][0]+1,code+prefix,strlen(countrycodes[i][0]+1)))
		{
			if (Len != NULL)
				*Len = strlen(countrycodes[i][0]) - 1 /* das "+" */ + prefix;

			return countrycodes[i][1];
		}

		i++;
	}

	if (!(flag & C_NO_WARN))
		print_msg("Unknown areacode `%s'!\n", code);

	return NULL;
}

/****************************************************************************/
#endif

#if 0 /* DELETE_ME AK:18-Aug-99 */
#ifndef LIBAREA
static char *_get_avon(char *code, int *Len, int flag)
{
	static int opened = 0;
	static char s[BUFSIZ];
	int prefix = strlen(countryprefix);
	int ll=0;
	int l;


	l = strlen(code) - prefix;

	s[0] = '\0';

	if (Len != NULL)
		*Len = -1;

	if (!opened)
	{
		if (avonlib == NULL && !(flag & C_NO_ERROR))
				print_msg("No path for AVON library defined!\n");

		if (!access(avonlib, R_OK))
		{
			createDB(avonlib,0);
			openDB(avonlib,O_WRONLY);
		}

		opened = 1;

		if (dbm == NULL && !(flag & C_NO_ERROR))
			print_msg("!!! Problem with AVON database! - disabling AVON support!\n");
	}

	if (dbm != NULL && l > 3)
	{
		key.dptr = code+prefix;

		do
		{
			ll++;
			key.dsize = l--;
			data = dbm_fetch(dbm, key);

			if (data.dptr != NULL)
			{
				if (Len != NULL)
					*Len = ll;

				strcpy(s,data.dptr);
			}
		}
		while(l > 1);
	}

	return (s[0]?s:NULL);
}
#endif
#endif

/****************************************************************************/

#if 0 /* DELETE_ME AK:18-Aug-99 */
static char *_get_areacode(char *code, int *Len, int flag)
{
	auto     int    cc = 0;
	auto     char  *err;
	static   acInfo ac;
	static   int    warned = 0;
	int prefix = strlen(countryprefix);


        if (warned)
          return(NULL);

	if ((cc = GetAreaCodeInfo(&ac, code + prefix)) == acOk) {
	  if (ac.AreaCodeLen > 0) {
			if (Len != NULL)
				*Len = ac.AreaCodeLen + prefix;

			return ac.Info;
		}
	}
	else {
		switch (cc) {
			case acFileError    : err = "Cannot open/read file";
			                      break;
			case acInvalidFile  : err = "The file exists but is no area code data file";
			                      break;
			case acWrongVersion : err = "Wrong version of data file";
			                      break;
            case acInvalidInput : err = "Input string is not a number or empty";
                             	  break;
			default             : err = "Unknown AreaCode error";
			                      break;
    } /* switch */

	  if (!(flag & C_NO_ERROR)) {
	    print_msg("!!! Problem with AreaCode: %s", err);

	    if (cc != acInvalidInput) {
	      print_msg(" - disabling AreaCode support!\n");
		warned = 1;
	}
            else
	      print_msg("\n");
          } /* if */
	} /* else */

	return NULL;
}
#endif

/****************************************************************************/

int read_conffiles(section **Section, char *groupfile)
{
	static section *conf_dat = NULL;
	static int read_again = 0;
	auto char    s[6][BUFSIZ];
	auto char **vars  = NULL;
	auto char **files = NULL;
	auto int    fileflag[6];
	auto int    i = 0;
	auto int      RetCode = -1;

	*Section = NULL;

	if (!read_again)
	{
	  sprintf(s[0], "%s%c%s", confdir(), C_SLASH, CONFFILE);
	  sprintf(s[1], "%s%c%s", confdir(), C_SLASH, CALLERIDFILE);

		if (paranoia_check(s[0]))
			return -1;

		if (paranoia_check(s[1]))
			return -1;

	  append_element(&files,s[0]);
	  fileflag[i++] = MERGE_FILE;
	  append_element(&files,s[1]);
	  fileflag[i++] = APPEND_FILE;

	  if (groupfile != NULL)
	  {
		  strcpy(s[2],groupfile);
			append_element(&files,s[2]);
	  	fileflag[i++] = MERGE_FILE;
		}

	  strcpy(s[3],expand_file(USERCONFFILE));
		append_element(&files,s[3]);
	  fileflag[i++] = MERGE_FILE;
	}

	sprintf(s[4],"%s|%s/%s|!%s",CONF_SEC_MSN,CONF_SEC_NUM,CONF_ENT_NUM,CONF_ENT_SI);
	append_element(&vars,s[4]);

/*
	sprintf(s[5],"%s|%s/%s",CONF_SEC_MSN,CONF_SEC_NUM,CONF_ENT_ALIAS);
	append_element(&vars,s[5]);
*/

	if ((RetCode = read_files(&conf_dat, files, fileflag, vars, C_OVERWRITE|C_NOT_UNIQUE|C_NO_WARN_FILE)) > 0)
	{
		*Section = conf_dat;

		if (Set_Codes(conf_dat) != 0)
			return -1;
	}
	else
		*Section = conf_dat;

	if (!read_again)
		delete_element(&files,0);

	delete_element(&vars,0);

	read_again = 1;
	return RetCode;
}

/****************************************************************************/

int paranoia_check(char *cmd)
{
	struct stat stbuf;


	if (getuid() == 0)
	{
		if (stat(cmd, &stbuf))
		{
			if (errno == ENOENT)
				return 0;

			print_msg("stat() failed for file `%s', stay on the safe side!\n", cmd);
			return -1;
		}

		if (stbuf.st_uid != 0)
		{
			print_msg("Owner of file `%s' is not root!\n", cmd);
			return -1;
		}

		if ((stbuf.st_gid != 0 && (stbuf.st_mode & S_IWGRP)) ||
		    (stbuf.st_mode & S_IWOTH)                          )
		{
			print_msg("File `%s' is writable by group or world!\n", cmd);
			return -1;
		}
	}

	return 0;
}

/****************************************************************************/

#if 0 /* DELETE_ME AK:18-Aug-99 */
static long int area_read_value(FILE *fp, int size)
{
	long value = 0;
	static int endian = -1;

	if (size != 2 && size != 4 && size != 1)
	{
		print_msg("Can not read lenght %d, only 1, 2, 4\n");
		return -1;
	}

	if (endian == -1)
	{
		if (htons(0x0101) == 0x0101)
			endian = 0;
		else
			endian = 1;
	}

	if (fread(&value,size,1,fp) != 1)
	{
		print_msg("Can not read from file `%s': Too less data!\n", areadifffile);
		return -1;
	}

	if (endian)
	{
		if (size == 2)
			value = ntohs(value);
		else
		if (size == 4)
			value = ntohl(value);
	}

	return value;
}

/****************************************************************************/

const char* area_diff_string(char* number1, char* number2)
{
	switch(area_diff(number1,number2))
	{
		case AREA_LOCAL :	return "CityCall";   break;
		case AREA_R50   :	return "RegioCall";  break;
		case AREA_FAR   :	return "GermanCall"; break;
		case AREA_ABROAD:	return "GlobalCall"; break;
		default         :	break;
	}

	return "";
}

/****************************************************************************/

int area_diff(char* _code, char *_diffcode)
{
	FILE *fp = NULL;
	char code[NUMBER_SIZE];
	char diffcode[NUMBER_SIZE];
	char value[15];
	int index;
	int number;
	int i = 0;


	if (codes == NULL)
		if (area_read_file() == -1)
			return AREA_ERROR;

	if (_code == NULL)
	{
		Strncpy(code,mycountry,NUMBER_SIZE);
		Strncat(code,myarea,NUMBER_SIZE);
	}
	else
		Strncpy(code,expand_number(_code),NUMBER_SIZE);

	if (strncmp(mycountry,code,strlen(mycountry)))
		return AREA_UNKNOWN;

	if (_diffcode == NULL)
		return AREA_ERROR;
	else
		Strncpy(diffcode,expand_number(_diffcode),NUMBER_SIZE);

	if ((index = area_get_index(code)) == -1)
		return AREA_ERROR;

	if ((fp = fopen(areadifffile,"r")) == NULL)
	{
		print_msg("Can not open file `%s': %s\n", areadifffile, strerror(errno));
		return -1;
	}

	fseek(fp,codes[index].pointer,SEEK_SET);

	number = area_read_value(fp,2);

	i = 0;
	while(i++<number)
	{
		sprintf(value,"%s%d%ld",countryprefix,GERMAN_CODE,(area_read_value(fp,2)+32768)%65536);
		if (!strncmp(value,diffcode,strlen(value)))
		{
			fclose(fp);
			return AREA_LOCAL;
		}
	}

	number = area_read_value(fp,2);
	
	i = 0;
	while(i++<number)
	{
		sprintf(value,"%s%d%ld",countryprefix,GERMAN_CODE,(area_read_value(fp,2)+32768)%65536);
		if (!strncmp(value,diffcode,strlen(value)))
		{
			fclose(fp);
			return AREA_R50;
		}
	}

	fclose(fp);

	if (!strncmp(mycountry,diffcode,strlen(mycountry)))
	{
		i = 0;

		do
		{
			if (areacodes[i][0][0] != '\0')
			{
				strcpy(value, expand_number((char*) areacodes[i][0]));

				if (!strncmp(diffcode,value,strlen(value)))
					return AREA_UNKNOWN;
			}
		}
		while (areacodes[++i][0][0] != '\0');

		return AREA_FAR;
	}
	else
		return AREA_ABROAD;

	return AREA_UNKNOWN;
}

/****************************************************************************/

static int area_get_index(char *code)
{
	int index = -1;

	if (codes == NULL)
		return -1;

	while(++index < codes_number)
		if (!strcmp(codes[index].code,code))
			break;

	if (index == codes_number)
	{
		print_msg("Can not find area code `%s'!\n", code);
		index = -1;
	}

	return index;
}

/****************************************************************************/

static int area_read_file(void)
{
	FILE *fp = NULL;
	int i = 0;


	if (areadifffile == NULL)
	{
//		print_msg("There is no file name for vorwahl database!\n");
		return -1;
	}

	if ((fp = fopen(areadifffile,"r")) == NULL)
	{
		print_msg("Can not open file `%s': %s\n", areadifffile, strerror(errno));
		return -1;
	}

	if ((codes_number = area_read_value(fp,2)) < 0)
	{
		print_msg("Number of areacodes is wrong: %d\n", codes_number);
		return -1;
	}

	if (codes != NULL)
		free(codes);

	if ((codes = (s_areacode*) calloc(codes_number,sizeof(s_areacode))) == NULL)
	{
		print_msg("%s\n", strerror(errno));
		return -1;
	}

	if (fseek(fp,3,SEEK_SET) != 0)
	{
		print_msg("Can not seek file `%s' to position %d: %s\n", areadifffile, 4*codes_number+3, strerror(errno));
		return -1;
	}

	while (i<codes_number)
		codes[i++].pointer = area_read_value(fp,4) -1;

	if (fseek(fp,4*codes_number+3,SEEK_SET) != 0)
	{
		print_msg("Can not seek file `%s' to position %d: %s\n", areadifffile, 4*codes_number+3, strerror(errno));
		return -1;
	}

	i = 0;
	while (i<codes_number)
		sprintf(codes[i++].code,"%s%d%ld",countryprefix,GERMAN_CODE,(area_read_value(fp,2)+32768)%65536);

	fclose(fp);
	return 0;
}

/****************************************************************************/
#endif
