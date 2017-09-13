/* $Id: isdnconf.c,v 1.34 2002/07/04 10:29:18 paul Exp $
 *
 * ISDN accounting for isdn4linux. (Utilities)
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
 * $Log: isdnconf.c,v $
 * Revision 1.34  2002/07/04 10:29:18  paul
 * "DEM" -> "EUR"
 *
 * Revision 1.33  2000/09/01 06:30:20  paul
 * Now install country-de.dat as country.dat; renaming the source can be done by
 * someone who is more experienced with CVS.
 * Default value for country.dat changed in isdnconf.c; also default zonefile
 * etc. changed from .gdbm to .cdb
 *
 * Revision 1.32  2000/02/20 19:03:08  akool
 * isdnlog-4.12
 *  - ABC_LCR enhanced
 *  - country-de.dat more aliases
 *  - new rates
 *  - isdnlog/Makefile.in ... defined NATION
 *  - isdnlog/isdnlog/processor.c ... msn patch for NL
 *  - isdnlog/tools/isdnconf.c ... default config
 *
 * Revision 1.31  1999/12/31 13:57:19  akool
 * isdnlog-4.00 (Millenium-Edition)
 *  - Oracle support added by Jan Bolt (Jan.Bolt@t-online.de)
 *  - resolved *any* warnings against rate-de.dat
 *  - Many new rates
 *  - CREDITS file added
 *
 * Revision 1.30  1999/10/25 18:30:03  akool
 * isdnlog-3.57
 *   WARNING: Experimental version!
 *   	   Please use isdnlog-3.56 for production systems!
 *
 * Revision 1.29  1999/06/15 20:05:08  akool
 * isdnlog Version 3.33
 *   - big step in using the new zone files
 *   - *This*is*not*a*production*ready*isdnlog*!!
 *   - Maybe the last release before the I4L meeting in Nuernberg
 *
 * Revision 1.28  1999/05/13 11:39:47  akool
 * isdnlog Version 3.28
 *
 *  - "-u" Option corrected
 *  - "ausland.dat" removed
 *  - "countries-de.dat" fully integrated
 *      you should add the entry
 *      "COUNTRYFILE = /usr/lib/isdn/countries-de.dat"
 *      into section "[ISDNLOG]" of your config file!
 *  - rate-de.dat V:1.02-Germany [13-May-1999 12:26:24]
 *  - countries-de.dat V:1.02-Germany [13-May-1999 12:26:26]
 *
 * Revision 1.27  1999/05/04 19:33:37  akool
 * isdnlog Version 3.24
 *
 *  - fully removed "sondernummern.c"
 *  - removed "gcc -Wall" warnings in ASN.1 Parser
 *  - many new entries for "rate-de.dat"
 *  - better "isdnconf" utility
 *
 * Revision 1.26  1999/04/15 19:14:58  akool
 * isdnlog Version 3.15
 *
 * - reenable the least-cost-router functions of "isdnconf"
 *   try "isdnconf -c <areacode>" or even "isdnconf -c ."
 * - README: "rate-xx.dat" documented
 * - small fixes in processor.c and rate.c
 * - "rate-de.dat" optimized
 * - splitted countries.dat into countries-de.dat and countries-us.dat
 *
 * Revision 1.25  1999/04/10 16:36:36  akool
 * isdnlog Version 3.13
 *
 * WARNING: This is pre-ALPHA-dont-ever-use-Code!
 * 	 "tarif.dat" (aka "rate-xx.dat"): the next generation!
 *
 * You have to do the following to test this version:
 *   cp /usr/src/isdn4k-utils/isdnlog/holiday-de.dat /etc/isdn
 *   cp /usr/src/isdn4k-utils/isdnlog/rate-de.dat /usr/lib/isdn
 *   cp /usr/src/isdn4k-utils/isdnlog/samples/rate.conf.de /etc/isdn/rate.conf
 *
 * After that, add the following entries to your "/etc/isdn/isdn.conf" or
 * "/etc/isdn/callerid.conf" file:
 *
 * [ISDNLOG]
 * SPECIALNUMBERS = /usr/lib/isdn/sonderrufnummern.dat
 * HOLIDAYS       = /usr/lib/isdn/holiday-de.dat
 * RATEFILE       = /usr/lib/isdn/rate-de.dat
 * RATECONF       = /etc/isdn/rate.conf
 *
 * Please replace any "de" with your country code ("at", "ch", "nl")
 *
 * Good luck (Andreas Kool and Michael Reinelt)
 *
 * Revision 1.24  1999/03/24 19:38:57  akool
 * - isdnlog Version 3.10
 * - moved "sondernnummern.c" from isdnlog/ to tools/
 * - "holiday.c" and "rate.c" integrated
 * - NetCologne rates from Oliver Flimm <flimm@ph-cip.uni-koeln.de>
 * - corrected UUnet and T-Online rates
 *
 * Revision 1.23  1999/03/15 21:28:44  akool
 * - isdnlog Version 3.06
 * - README: explain some terms about LCR, corrected "-c" Option of "isdnconf"
 * - isdnconf: added a small LCR-feature - simply try "isdnconf -c 069"
 * - isdnlog: dont change CHARGEINT, if rate is't known!
 * - sonderrufnummern 1.02 [15-Mar-99] :: added WorldCom
 * - tarif.dat 1.09 [15-Mar-99] :: added WorldCom
 * - isdnlog now correctly handles the new "Ortstarif-Zugang" of UUnet
 *
 * Revision 1.22  1999/02/28 19:33:39  akool
 * Fixed a typo in isdnconf.c from Andreas Jaeger <aj@arthur.rhein-neckar.de>
 * CHARGEMAX fix from Oliver Lauer <Oliver.Lauer@coburg.baynet.de>
 * isdnrep fix from reinhard.karcher@dpk.berlin.fido.de (Reinhard Karcher)
 * "takt_at.c" fixes from Ulrich Leodolter <u.leodolter@xpoint.at>
 * sondernummern.c from Mario Joussen <mario.joussen@post.rwth-aachen.de>
 * Reenable usage of the ZONE entry from Schlottmann-Goedde@t-online.de
 * Fixed a typo in callerid.conf.5
 *
 * Revision 1.21  1999/01/24 19:02:45  akool
 *  - second version of the new chargeint database
 *  - isdnrep reanimated
 *
 * Revision 1.20  1998/11/24 20:53:03  akool
 *  - changed my email-adress
 *  - new Option "-R" to supply the preselected provider (-R24 -> Telepassport)
 *  - made Provider-Prefix 6 digits long
 *  - full support for internal S0-bus implemented (-A, -i Options)
 *  - isdnlog now ignores unknown frames
 *  - added 36 allocated, but up to now unused "Auskunft" Numbers
 *  - added _all_ 122 Providers
 *  - Patch from Jochen Erwied <mack@Joker.E.Ruhr.DE> for Quante-TK-Anlagen
 *    (first dialed digit comes with SETUP-Frame)
 *
 * Revision 1.19  1998/09/26 18:30:08  akool
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
 * Revision 1.18  1998/06/14 15:34:35  akool
 * AVM B1 support (Layer 3)
 * Telekom's new currency DEM 0,121 supported
 * Disable holiday rates #ifdef ISDN_NL
 * memory leak in "isdnrep" repaired
 *
 * Revision 1.17  1998/05/20 12:34:38  paul
 * More paranoid about freeing pointers.
 *
 * Revision 1.16  1998/05/19 15:55:57  paul
 * Moved config stuff for City Weekend from isdnlog.c to tools/isdnconf.c, so
 * that isdnrep also understands a "cityweekend=y" line in isdn.conf.
 *
 * Revision 1.15  1998/03/08 11:43:13  luethje
 * I4L-Meeting Wuerzburg final Edition, golden code - Service Pack number One
 *
 * Revision 1.14  1998/03/01 20:36:22  keil
 * bugfixes from Florian La Roche
 *
 * Revision 1.13  1997/06/24 23:35:33  luethje
 * isdnctrl can use a config file
 *
 * Revision 1.12  1997/06/22 23:03:34  luethje
 * In subsection FLAGS it will be checked if the section name FLAG is korrect
 * isdnlog recognize calls abroad
 * bugfix for program starts
 *
 * Revision 1.11  1997/05/25 19:41:13  luethje
 * isdnlog:  close all files and open again after kill -HUP
 * isdnrep:  support vbox version 2.0
 * isdnconf: changes by Roderich Schupp <roderich@syntec.m.EUnet.de>
 * conffile: ignore spaces at the end of a line
 *
 * Revision 1.10  1997/05/15 22:21:45  luethje
 * New feature: isdnrep can transmit via HTTP fax files and vbox files.
 *
 * Revision 1.9  1997/05/10 22:41:17  luethje
 * bug in format string fixed
 *
 * Revision 1.8  1997/05/04 20:20:01  luethje
 * README completed
 * isdnrep finished
 * interval-bug fixed
 *
 * Revision 1.7  1997/04/20 22:52:32  luethje
 * isdnrep has new features:
 *   -variable format string
 *   -can create html output (option -w1 or ln -s isdnrep isdnrep.cgi)
 *    idea and design from Dirk Staneker (dirk.staneker@student.uni-tuebingen.de)
 * bugfix of processor.c from akool
 *
 * Revision 1.6  1997/04/17 20:09:57  luethje
 * patch of Ingo Schneider
 *
 */

/****************************************************************************/

#define  _ISDNCONF_C_

/****************************************************************************/

#include "tools.h"

/****************************************************************************/

static char ***Environment = NULL;
static char *OlabelPtr = NULL;
static char *IlabelPtr = NULL;

/****************************************************************************/

static char *NextItem(char *Line, int komma);
static char*NextOption(char** Options, char* Delim);
static void Append(char **Target, char*Source);
static int SetFlags(KNOWN *FlagPtr, char *flags);
static int IsVariable(char *string);
static int Set_Globals(section *SPtr);
static info_args** Set_Flags(section *SPtr, int *Flags);
static int Get_Events(char* Flags);
static int Set_Numbers(section *SPtr, char *Section, int msn);
static int Set_known_Size(int msn);
static section* writeentry(section *SPtr, int Index);
static int readconfigfile(char *_myname);
static int _readconfig(char *_myname);
static int readoldconfig(char *myname);
static section* writeinfoargs(section *SPtr, info_args *infoarg);
static void appendflag(char*str1, char*str2);
static char *writeflags(int flag);
static section* writevariables(section *SPtr);
static section* writeglobal(section *SPtr);
static int SetEnv(char ****EnvPtr, char *name, char *value);
static int GetNextEnv(char ***EnvPtr, char **name, char **value);
static int ClearEnv(char ****EnvPtr);
static int Set_ILabel(char *value);
static int Set_OLabel(char *value);
static char *Get_FmtStr(char *Ptr, char *name);

/****************************************************************************/

static char *NextItem(char *Line, int komma)
{
  register char *p;
  register char *Ptr = Line;


  while (*Line && (*Line != ' ') && (*Line != '\t'))
    Line++;

  p = Line;

  while ((*Line == ' ') || (*Line == '\t'))
    Line++;

  *p = 0;

       p = Line;

       while((Ptr = Check_Quote(Ptr, komma ? "\\$@;#" : S_QUOTES, QUOTE_DELETE)) != NULL);

  return(Line);
} /* NextItem */

/******************************************************************************/

static char*NextOption(char** Options, char* Delim)
{
	char *RetCode = NULL;
	char *Ptr = *Options;
	int Len;

	*Delim = '\0';

	if (Ptr == NULL)
		return NULL;

	if ((Ptr = Check_Quote(Ptr,";,",QUOTE_DELETE)) == NULL)
		Ptr = "";

	if (*Ptr == ',' || *Ptr == ';' || *Ptr == '\0')
	{
		*Delim = *Ptr;

		if (Ptr != *Options)
		{
			Len = Ptr-(*Options);
			RetCode = (char*) calloc(Len+1, sizeof(char));
			strncpy(RetCode,*Options,Len);
		}

		if (*Ptr == ',')
		{
			*Options = Ptr+1;
		}
		else
		{
			while(!isalpha(*Ptr) && *Ptr != '\0')
				Ptr++;

			if (*Ptr == '\0')
			  *Options = NULL;
			else
			  *Options = Ptr;
		}
	}

	return RetCode;
}

/******************************************************************************/

static void Append(char **Target, char*Source)
{
	if (*Target == NULL)
	{
		*Target = Source;
		return;
	}

	if (Source == NULL)
		return;

	*Target = (char*) realloc(*Target, sizeof(char)*(strlen(*Target)+strlen(Source)+2));
	strcat(*Target,",");
	strcat(*Target,Source);
}

/******************************************************************************/

static int SetFlags(KNOWN *FlagPtr, char *flags)
{
	auto char Delim = '\0';
	auto char *Ptr;
  auto int  NumArgs = 0;
	auto int  flag = 0;

	while (*flags == ' ' || *flags == '\t')
		flags++;

  FlagPtr->infoargs = NULL;
  FlagPtr->flags = 0;

  while (flags && *flags) {
  	flag = 0;

    while(isalpha(*flags) || isspace(*flags))
    {
      switch (toupper(*flags++)) {
        case 'I' : flag |= RING_INCOMING; break;
        case 'O' : flag |= RING_OUTGOING; break;
        case 'R' : flag |= RING_RING    ; break;
        case 'C' : flag |= RING_CONNECT ; break;
        case 'B' : flag |= RING_BUSY    ; break;
        case 'A' : flag |= RING_AOCD    ; break;
        case 'E' : flag |= RING_ERROR   ; break;
        case 'H' : flag |= RING_HANGUP  ; break;
        case 'K' : flag |= RING_KILL    ; break;
        case 'L' : flag |= RING_LOOP    ; break;
        case 'S' : flag |= RING_SPEAK   ; break;
        case 'P' : flag |= RING_PROVIDER; break;
        case 'U' : flag |= RING_UNIQUE  ; break;
        case ' ' : break;
        case '\t': break;

        default : _print_msg( "%s: WARNING: Unknown flag ``%c'' in file \"%s\" line %d, ignored\n", Myname, *(flags-1), OLDCONFFILE, ln);
                  break;
      } /* switch */
    }

    if (flag != 0)
    {
    	if(*flags == '=')
    	{
    	  flags++;
    	  FlagPtr->flags |= flag;
    	  FlagPtr->infoargs = (info_args**) realloc(FlagPtr->infoargs, sizeof(info_args*) * (NumArgs+2));
    	  FlagPtr->infoargs[NumArgs] = (info_args*) calloc(1, sizeof(info_args));

   		  FlagPtr->infoargs[NumArgs]->flag    = flag;
   		  FlagPtr->infoargs[NumArgs]->infoarg = NextOption(&flags,&Delim);

   		  if (Delim == ',')
   		  {
                         if ((Ptr = NextOption(&flags,&Delim)) != NULL)
                         {
                           FlagPtr->infoargs[NumArgs]->interval= atoi(Ptr);
                           free(Ptr);
                         }
   	 		}

    		  if (Delim == ',')
     		    do
     		    	Append(&(FlagPtr->infoargs[NumArgs]->time),
     		    	       NextOption(&flags,&Delim));
     		    while(Delim == ',');

        if (Delim != ';')
        {
          _print_msg( "%s: WARNING: Syntax-Error in file \"%s\" line %d, ignored (\";\" expected, but found \"%c\")\n", Myname, OLDCONFFILE, ln, Delim);
          flags = NULL;
        }

    	  FlagPtr->infoargs[++NumArgs] = NULL;
    	}
    	else
    	{
        _print_msg( "%s: WARNING: Syntax-Error in file \"%s\" line %d, ignored (\"=\" expected, but found \"%c\")\n", Myname, OLDCONFFILE, ln, *flags);
        flags = NULL;
      }
    }
  } /* while */

  return 0;
}

/******************************************************************************/

static int IsVariable(char *string)
{
  char s1[SHORT_STRING_SIZE] = "",
       s2[LONG_STRING_SIZE] = "";

  if (sscanf(string,"%[a-zA-Z0-9] = %[^\n]",s1,s2) == 2)
  {
    if (SetEnv(&Environment,s1,s2) == 0)
      return 1;

    _print_msg( "%s: WARNING: Error in file \"%s\" line %d: Can't set variable (%s)!\n", Myname, OLDCONFFILE, ln, strerror(errno));
  }

  return 0;
}

/****************************************************************************/

int writeconfig(char *_myname)
{
	int i;
  auto char     s[BUFSIZ];
  auto section *SPtr = NULL;


	if ((SPtr = writevariables(SPtr)) == NULL)
	{
		free_section(SPtr);
		return -1;
	}

	if ((SPtr = writeglobal(SPtr)) == NULL)
	{
		free_section(SPtr);
		return -1;
	}

  sprintf(s, "%s%c%s", confdir(), C_SLASH, CONFFILE);
	write_file(SPtr,s,_myname,VERSION);

	free_section(SPtr);
	SPtr = NULL;

	for (i = 0; i < knowns; i++)
	{
		if ((SPtr = writeentry(SPtr,i)) == NULL)
		{
			free_section(SPtr);
			return -1;
		}
	}

  sprintf(s, "%s%c%s", confdir(), C_SLASH, CALLERIDFILE);
	write_file(SPtr,s,_myname,VERSION);

	free_section(SPtr);

	return 0;
}

/****************************************************************************/

static void appendflag(char*str1, char*str2)
{
	char delim[2];

	delim[0] = C_FLAG_DELIM;
	delim[1] = '\0';

	if (str1[0] != '\0')
		strcat(str1,delim);

	strcat(str1,str2);
}

/****************************************************************************/

static char *writeflags(int flag)
{
	static char string[BUFSIZ];

	string[0] = '\0';

	if (flag & RING_INCOMING)
		appendflag(string,"I");
	if (flag & RING_OUTGOING)
		appendflag(string,"O");
	if (flag & RING_RING)
		appendflag(string,"R");
	if (flag & RING_CONNECT)
		appendflag(string,"C");
	if (flag & RING_BUSY)
		appendflag(string,"B");
	if (flag & RING_AOCD)
		appendflag(string,"A");
	if (flag & RING_ERROR)
		appendflag(string,"E");
	if (flag & RING_HANGUP)
		appendflag(string,"H");
	if (flag & RING_KILL)
		appendflag(string,"K");
	if (flag & RING_LOOP)
		appendflag(string,"L");
	if (flag & RING_SPEAK)
		appendflag(string,"S");
	if (flag & RING_PROVIDER)
		appendflag(string,"P");
	if (flag & RING_UNIQUE)
		appendflag(string,"U");

	return string;
}

/****************************************************************************/

static section* writeinfoargs(section *SPtr, info_args *infoarg)
{
	section *Ptr;
	auto char     s[BUFSIZ];
	auto char     s1[BUFSIZ];

	strcpy(s, CONF_SEC_FLAG);
	if ((Ptr = Set_Section(&SPtr,s,C_OVERWRITE | C_WARN | C_NOT_UNIQUE)) == NULL)
	{
		_print_msg("Can't set section `%s'!\n",CONF_SEC_FLAG);
		free_section(SPtr);
		return NULL;
	}

	sprintf(s1,"%d",infoarg->flag);
	strcpy(s, CONF_ENT_FLAGS);
	if (Set_Entry(Ptr,NULL,s,writeflags(infoarg->flag),C_OVERWRITE | C_WARN) == NULL)
	{
		_print_msg("Can't set entry `%s'!\n",CONF_ENT_FLAGS);
		free_section(SPtr);
		return NULL;
	}

	strcpy(s, CONF_ENT_PROG);
	if (Set_Entry(Ptr,NULL,s,infoarg->infoarg,C_OVERWRITE | C_WARN) == NULL)
	{
		_print_msg("Can't set entry `%s'!\n",CONF_ENT_PROG);
		free_section(SPtr);
		return NULL;
	}

	if (infoarg->user != NULL)
	{
		strcpy(s, CONF_ENT_USER);
		if (Set_Entry(Ptr,NULL,s,infoarg->user,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_USER);
			free_section(SPtr);
			return NULL;
		}
	}

	if (infoarg->group != NULL)
	{
		strcpy(s, CONF_ENT_GROUP);
		if (Set_Entry(Ptr,NULL,s,infoarg->group,C_OVERWRITE | C_WARN) == NULL)
		{
		_print_msg("Can't set entry `%s'!\n",CONF_ENT_GROUP);
			free_section(SPtr);
			return NULL;
		}
	}

	if (infoarg->interval != 0)
	{
		sprintf(s1,"%d",infoarg->interval);
		strcpy(s, CONF_ENT_INTVAL);
		if (Set_Entry(Ptr,NULL,s,s1,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_INTVAL);
			free_section(SPtr);
			return NULL;
		}
	}

	if (infoarg->time != NULL)
	{
		strcpy(s, CONF_ENT_TIME);
		if (Set_Entry(Ptr,NULL,s,infoarg->time,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_TIME);
			free_section(SPtr);
			return NULL;
		}
	}

	return SPtr;
}

/****************************************************************************/

static section* writevariables(section *SPtr)
{
	char *name;
	char *value;
	auto char     s[BUFSIZ];
	section *Ptr;


	strcpy(s, CONF_SEC_VAR);
	if ((Ptr = Set_Section(&SPtr,s,C_OVERWRITE | C_WARN)) == NULL)
	{
		_print_msg("Can't set section `%s'!\n",CONF_SEC_VAR);
		free_section(SPtr);
		return NULL;
	}

	if (GetNextEnv(Environment,&name,&value) == 0)
		do
		{
			if (Set_Entry(Ptr,NULL,name,value,C_OVERWRITE | C_WARN) == NULL)
			{
				_print_msg("Can't set entry `%s'!\n",name);
				free_section(SPtr);
				return NULL;
			}
		}
		while(GetNextEnv(NULL,&name,&value) == 0);

	return SPtr;
}

/****************************************************************************/

static section* writeglobal(section *SPtr)
{
	auto char     s[BUFSIZ];
	auto char     s1[BUFSIZ];
	int      IIndex = 0;
	section *Ptr;
	section *SubPtr = NULL;


	strcpy(s, CONF_SEC_GLOBAL);
	if ((Ptr = Set_Section(&SPtr,s,C_OVERWRITE | C_WARN)) == NULL)
	{
		_print_msg("Can't set section `%s'!\n",CONF_SEC_GLOBAL);
		free_section(SPtr);
		return NULL;
	}

	if (mycountry != NULL && mycountry[0] != '\0')
	{
		strcpy(s, CONF_ENT_COUNTRY);
		if (Set_Entry(Ptr,NULL,s,mycountry,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_COUNTRY);
			free_section(SPtr);
			return NULL;
		}
	}

	if (myarea != NULL && myarea[0] != '\0')
	{
		strcpy(s, CONF_ENT_AREA);
		if (Set_Entry(Ptr,NULL,s,myarea,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_AREA);
			free_section(SPtr);
			return NULL;
		}
	}

	strcpy(s, CONF_SEC_ISDNLOG);
	if ((Ptr = Set_Section(&SPtr,s,C_OVERWRITE | C_WARN)) == NULL)
	{
		_print_msg("Can't set section `%s'!\n",CONF_SEC_ISDNLOG);
		free_section(SPtr);
		return NULL;
	}

	if (reloadcmd != NULL)
	{
		strcpy(s, CONF_ENT_RELOAD);
		if (Set_Entry(Ptr,NULL,s,reloadcmd,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_RELOAD);
			free_section(SPtr);
			return NULL;
		}
	}

	if (stopcmd != NULL)
	{
		strcpy(s, CONF_ENT_STOP);
		if (Set_Entry(Ptr,NULL,s,stopcmd,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_STOP);
			free_section(SPtr);
			return NULL;
		}
	}

	if (rebootcmd != NULL)
	{
		strcpy(s, CONF_ENT_REBOOT);
		if (Set_Entry(Ptr,NULL,s,rebootcmd,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_REBOOT);
			free_section(SPtr);
			return NULL;
		}
	}

#if 0 /* Fixme: remove */
	if (CityWeekend != 0)
	{
		strcpy(s, CONF_ENT_CW);
		if (Set_Entry(Ptr,NULL,s,"yes",C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_CW);
			free_section(SPtr);
			return NULL;
		}
	}
#endif
	if (chargemax != 0)
	{
		strcpy(s, CONF_ENT_CHARGE);
		sprintf(s1, "%.2f",chargemax);
		if (Set_Entry(Ptr,NULL,s,s1,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_CHARGE);
			free_section(SPtr);
			return NULL;
		}
	}

	if (connectmax != 0)
	{
		strcpy(s, CONF_ENT_CONNECT);
		sprintf(s1, "%.10g,%d",connectmax,connectmaxmode);
		if (Set_Entry(Ptr,NULL,s,s1,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_CONNECT);
			free_section(SPtr);
			return NULL;
		}
	}

	if (bytemax != 0)
	{
		strcpy(s, CONF_ENT_BYTE);
		sprintf(s1, "%.10g,%d",bytemax,bytemaxmode);
		if (Set_Entry(Ptr,NULL,s,s1,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_BYTE);
			free_section(SPtr);
			return NULL;
		}
	}

	if (currency != NULL && currency_factor != 0)
	{
		strcpy(s, CONF_ENT_CURR);
		sprintf(s1, "%.3f,%s",currency_factor,currency);
		if (Set_Entry(Ptr,NULL,s,s1,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_CURR);
			free_section(SPtr);
			return NULL;
		}
	}

	if (IlabelPtr != NULL)
	{
		strcpy(s, CONF_ENT_ILABEL);
		if (Set_Entry(Ptr,NULL,s,IlabelPtr,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_ILABEL);
			free_section(SPtr);
			return NULL;
		}
	}

	if (OlabelPtr != NULL)
	{
		strcpy(s, CONF_ENT_OLABEL);
		if (Set_Entry(Ptr,NULL,s,OlabelPtr,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_OLABEL);
			free_section(SPtr);
			return NULL;
		}
	}

	IIndex = 0;

	if (start_procs.infoargs != NULL && start_procs.infoargs[0] != NULL)
	{
		while (start_procs.infoargs[IIndex] != NULL)
		{
			if ((SubPtr = writeinfoargs(SubPtr,start_procs.infoargs[IIndex])) == NULL)
			{
				_print_msg("Can't set entry `%s'!\n",CONF_SEC_START);
				free_section(SPtr);
				return NULL;
			}

			IIndex++;
		}

		strcpy(s, CONF_SEC_START);

		if (Set_SubSection(Ptr,s,SubPtr,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_SEC_START);
			free_section(SPtr);
			return NULL;
		}
	}

	return SPtr;
}

/****************************************************************************/

static section* writeentry(section *SPtr, int Index)
{
	section *Ptr;
	int IIndex = 0;
	section *SubPtr = NULL;
	auto char     s[BUFSIZ];
	auto char     s1[BUFSIZ];


	if (Index < mymsns)
		strcpy(s,CONF_SEC_MSN);
	else
		strcpy(s,CONF_SEC_NUM);

	if ((Ptr = Set_Section(&SPtr,s,C_OVERWRITE | C_WARN | C_NOT_UNIQUE)) == NULL)
	{
		_print_msg("Can't set section `%s'!\n",CONF_SEC_NUM);
		free_section(SPtr);
		return NULL;
	}

	strcpy(s, CONF_ENT_NUM);
	if (Set_Entry(Ptr,NULL,s,known[Index]->num,C_OVERWRITE | C_WARN) == NULL)
	{
		_print_msg("Can't set entry `%s'!\n",CONF_ENT_NUM);
		free_section(SPtr);
		return NULL;
	}

	if (known[Index]->si != 0)
	{
		strcpy(s, CONF_ENT_SI);
		sprintf(s1,"%d",known[Index]->si);
		if (Set_Entry(Ptr,NULL,s,s1,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_SI);
			free_section(SPtr);
			return NULL;
		}
	}

	strcpy(s, CONF_ENT_ALIAS);
	if (Set_Entry(Ptr,NULL,s,known[Index]->who,C_OVERWRITE | C_WARN) == NULL)
	{
		_print_msg("Can't set entry `%s'!\n",CONF_ENT_ALIAS);
		free_section(SPtr);
		return NULL;
	}

	if (strcmp(known[Index]->interface,"-") != 0 && known[Index]->interface[0] != '\0')
	{
		strcpy(s, CONF_ENT_INTFAC);
		if (Set_Entry(Ptr,NULL,s,known[Index]->interface,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_ENT_INTFAC);
			free_section(SPtr);
			return NULL;
		}
	}

	strcpy(s, CONF_ENT_ZONE);
	sprintf(s1,"%d",known[Index]->zone);
	if (Set_Entry(Ptr,NULL,s,s1,C_OVERWRITE | C_WARN) == NULL)
	{
		_print_msg("Can't set entry `%s'!\n",CONF_ENT_ZONE);
		free_section(SPtr);
		return NULL;
	}

	IIndex = 0;

	if (known[Index]->infoargs != NULL && known[Index]->infoargs[0] != NULL)
	{
		while (known[Index]->infoargs[IIndex] != NULL)
		{
			if ((SubPtr = writeinfoargs(SubPtr,known[Index]->infoargs[IIndex])) == NULL)
			{
				_print_msg("Can't set entry `%s'!\n",CONF_SEC_START);
				free_section(SPtr);
				return NULL;
			}

			IIndex++;
		}

		strcpy(s, CONF_SEC_START);

		if (Set_SubSection(Ptr,s,SubPtr,C_OVERWRITE | C_WARN) == NULL)
		{
			_print_msg("Can't set entry `%s'!\n",CONF_SEC_START);
			free_section(SPtr);
			return NULL;
		}
	}

	return SPtr;
}

/****************************************************************************/

void setDefaults()
{
  if (currency == NULL)
    currency="EUR";

  if (currency_factor == 0.0)
    currency_factor = 0.062;

  currency_mode = AOC_UNITS;
} /* setDefaults */

/****************************************************************************/

int readconfig(char *_myname)
{
  register int cc;


  if (!(cc = readconfigfile(_myname)))
    setDefaults();

  return(cc);
} /* readconfig */

/****************************************************************************/

static int readconfigfile(char *_myname)
{
  auto char     file1[BUFSIZ];
  auto char     file2[BUFSIZ];
  struct stat buf1, buf2;
  int ret1, ret2;


  if (use_new_config) {
  sprintf(file1, "%s%c%s", confdir(), C_SLASH, CONFFILE);
  ret1 = stat(file1,&buf1);

 	sprintf(file2, "%s%c%s", confdir(), C_SLASH, OLDCONFFILE);
  ret2 = stat(file2,&buf2);

	if (ret1 == 0)
	{
		if (access(file1,R_OK) == 0)
		{
			if (ret2 == 0 && access(file2,R_OK) == 0)
			{
				if (buf1.st_mtime >= buf2.st_mtime)
				{
					return _readconfig(_myname);
				}
				else
				{
					if (readoldconfig(_myname) == 0)
						return writeconfig(_myname);
					else
					{
						_print_msg("Can't read file %s!\n",file2);
						return -1;
					}
				}
			}
			else
			{
				return _readconfig(_myname);
			}
		}
		else
		{
			_print_msg("Can't read file %s!\n",file1);
			return -1;
		}
	}
	else
	{
		if (ret2 == 0 && access(file2,R_OK) == 0)
		{
			if (readoldconfig(_myname) == 0)
				return writeconfig(_myname);
			else
			{
				_print_msg("Can't read file %s!\n",file2);
				return -1;
			}
		}
		else
		{
			_print_msg("Can't read file %s!\n",file2);
			return -1;
		}
	}

	return 0;
  }
  else
    return(readoldconfig(_myname));
}

/****************************************************************************/

section *read_isdnconf(section **_conf_dat)
{
	read_conffiles(_conf_dat,NULL);

	return *_conf_dat;
}

/****************************************************************************/

static int _readconfig(char *_myname)
{
  auto section *SPtr;
  auto int      i;
  int  cur_msn = 0;


  Myname         = _myname;
  mymsns         = 0;
  mycountry      = "";
  myarea         = "";
  currency       = NULL;
#if 0 /* Fixme: remove */
  CityWeekend    = 0;
#endif
  chargemax      = 0.0;
  connectmax     = 0;
  connectmaxmode = 0;
  bytemax        = 0;
  bytemaxmode    = 0;
  knowns         = retnum = 0;
  known          = NULL;
  reloadcmd      = RELOADCMD;
  stopcmd        = STOPCMD;
  rebootcmd      = REBOOTCMD;
  logfile        = LOGFILE;
  callfile       = NULL;
  callfmt        = NULL;
  holifile       = CONFIG_DATADIR"/holiday-"NATION".dat";
  countryfile	 = CONFIG_DATADIR"/country.dat";
  rateconf       = CONFIG_I4LCONFDIR"/rate.conf";
  ratefile       = CONFIG_DATADIR"/rate-"NATION".dat";
  zonefile	 = CONFIG_DATADIR"/zone-"NATION"-%s.cdb";
  destfile	 = CONFIG_DATADIR"/dest.cdb";
  lcdfile        = NULL;
  start_procs.infoargs = NULL;
  start_procs.flags    = 0;
  conf_dat       = NULL;
  vboxpath      = NULL;
  vboxcommand1  = NULL;
  vboxcommand2  = NULL;
  mgettypath    = NULL;
  mgettycommand = NULL;


	ClearEnv(&Environment);

	if ((SPtr = read_isdnconf(&conf_dat)) == NULL)
		return -1;

	if (Replace_Variables(conf_dat) != 0)
		return -1;

	Set_Globals(conf_dat);

	while (SPtr != NULL)
	{
		if (!strcmp(SPtr->name,CONF_SEC_MSN))
			Set_Numbers(SPtr,SPtr->name,cur_msn++);
		else
		if (!strcmp(SPtr->name,CONF_SEC_NUM))
			Set_Numbers(SPtr,SPtr->name,NO_MSN);

		SPtr = SPtr->next;
	}

	for (i = 0; i < mymsns; i++)
		if (known[i]->num == NULL)
		{
   		_print_msg("Error: MSN number %d is not set!\n",i+1);
   		return -1;
   	}

	return 0;
}

/****************************************************************************/

static char *Get_FmtStr(char *Ptr, char *name)
{
	static char *RetCode = NULL;
	char *ptr2;

	if (*Ptr != '\"')
		return Ptr;

	free(RetCode);
	ptr2 = RetCode = strdup(++Ptr);

	while(*Ptr != '\"')
	{
		if (*Ptr == '\0')
		{
   		_print_msg("Warning: Missing character `\"' at the end of entry `%s'!\n",name);
   		break;
   	}

		if (*Ptr == '\\' && Ptr[1] == '\"')
			Ptr++;

		*ptr2++ = *Ptr++;
	}

	*ptr2 = '\0';

	if (*Ptr == '\"')
		Ptr++;

	while(isspace(*Ptr)) Ptr++;

	if (*Ptr != '\0')
   		_print_msg("Warning: Invalid token at the end of entry `%s'!\n",name);

  return RetCode;
}

/****************************************************************************/

static int Set_Globals(section *SPtr)
{
	auto int      cnt = 0;
	auto char  ***sPtr = lineformats;
	auto section *Ptr;
  auto entry   *CEPtr;

	if (sPtr != NULL)
	{
		while (sPtr != NULL)
		{
			if (sPtr[0])
				free(sPtr[0]);
			if (sPtr[1])
				free(sPtr[1]);
			free(sPtr);

			sPtr++;
		}

		free(lineformats);
	}

	if ((Ptr = Get_Section(SPtr,CONF_SEC_ISDNLOG)) != NULL)
	{
		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_ILABEL)) != NULL)
			Set_ILabel(Get_FmtStr(CEPtr->value,CEPtr->name));
		else
			Set_ILabel(NULL);

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_OLABEL)) != NULL)
			Set_OLabel(Get_FmtStr(CEPtr->value,CEPtr->name));
		else
			Set_OLabel(NULL);

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_START)) != NULL)
			start_procs.infoargs = Set_Flags(CEPtr->subsection,&(start_procs.flags));

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_RELOAD)) != NULL)
			reloadcmd = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_STOP)) != NULL)
			stopcmd = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_REBOOT)) != NULL)
			rebootcmd = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_LOGFILE)) != NULL)
			logfile = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_CALLFILE)) != NULL)
			callfile = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_CALLFMT)) != NULL)
			callfmt = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_HOLIFILE)) != NULL)
			holifile = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_COUNTRYFILE)) != NULL)
			countryfile = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_ZONEFILE)) != NULL)
			zonefile = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_RATECONF)) != NULL)
			rateconf = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_RATEFILE)) != NULL)
			ratefile = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_DESTFILE)) != NULL)
			destfile = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_PRESELECT)) != NULL)
			preselect = atoi(CEPtr->value);

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_VBN)) != NULL)
			vbn = CEPtr->value;
		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_VBNLEN)) != NULL)
			vbnlen = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_LCDFILE)) != NULL)
			lcdfile = CEPtr->value;

#if 0 /* Fixme: remove */
		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_CW)) != NULL)
			CityWeekend = toupper(*(CEPtr->value)) == 'Y'?1:0;
#endif
		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_CHARGE)) != NULL)
			chargemax = strtod(CEPtr->value,NULL);

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_CONNECT)) != NULL)
	  {
	    if (sscanf(CEPtr->value,"%lg,%d", &connectmax, &connectmaxmode) != 2)
	      _print_msg("%s: WARNING: Syntax error in `%s' in Line %d, ignored\n", Myname, CONF_ENT_CONNECT, ln);
	  }

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_BYTE)) != NULL)
	  {
	    if (sscanf(CEPtr->value,"%lg,%d", &bytemax, &bytemaxmode) != 2)
	      _print_msg("%s: WARNING: Syntax error in `%s' in Line %d, ignored\n", Myname, CONF_ENT_BYTE, ln);
	  }

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_CURR)) != NULL)
	  {
	    currency_factor = atof(CEPtr->value);

	    if ((currency = strchr(CEPtr->value, ',')) == NULL)
	      _print_msg("%s: WARNING: Syntax error in `%s' in Line %d, ignored\n", Myname, CONF_ENT_CURR, ln);
	    else
	    	currency++;
	  }

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_VBOXPATH)) != NULL)
			vboxpath = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_VBOXCMD1)) != NULL)
			vboxcommand1 = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_VBOXCMD2)) != NULL)
			vboxcommand2 = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_MGTYPATH)) != NULL)
			mgettypath = CEPtr->value;

		if ((CEPtr = Get_Entry(Ptr->entries,CONF_ENT_MGTYCMD)) != NULL)
			mgettycommand = CEPtr->value;

		CEPtr = Ptr->entries;
		cnt = 0;

	  while (CEPtr != NULL)
	  {
	  	if (!strncmp(CEPtr->name,CONF_ENT_REPFMT,strlen(CONF_ENT_REPFMT)))
	  	{
	  		if ((lineformats = (char ***) realloc(lineformats,sizeof(char**)*(cnt+2))) == NULL)
	  		{
					_print_msg("%s: ERROR: Can't allocate memory!\n", Myname);
					return 0;
				}

	  		if ((lineformats[cnt] = (char **) calloc(2,sizeof(char*))) == NULL)
	  		{
					_print_msg("%s: ERROR: Can't allocate memory!\n", Myname);
					return 0;
				}

	  		if ((lineformats[cnt][0] = strdup(CEPtr->name+strlen(CONF_ENT_REPFMT))) == NULL)
	  		{
					_print_msg("%s: ERROR: Can't allocate memory!\n", Myname);
					return 0;
				}

	  		if ((lineformats[cnt][1] = strdup(Get_FmtStr(CEPtr->value,CEPtr->name))) == NULL)
	  		{
					_print_msg("%s: ERROR: Can't allocate memory!\n", Myname);
					return 0;
				}

				lineformats[++cnt] = NULL;
	  	}

	  	CEPtr = CEPtr->next;
	  }
	}
	else
		_print_msg("%s: WARNING: There is no section `%s'!\n", Myname, CONF_SEC_ISDNLOG);

	if (mycountry == NULL && mycountry[0] == '\0') {
		_print_msg("%s: WARNING: Variable `%s' is not set!\n", Myname, CONF_ENT_COUNTRY);
		mycountry = "";
	}

	if (myarea == NULL && myarea[0] == '\0')
	{
		_print_msg("%s: WARNING: Variable `%s' is not set!\n", Myname, CONF_ENT_AREA);
		myarea = "";
	}
#if 0
  if (chargemax == 0)
  {
  	_print_msg("%s: WARNING: Variable `%s' is not set, \nperforming no action when chargemax-overflow\n", Myname, CONF_ENT_CHARGE);
  }
#endif
	return 0;
}

/****************************************************************************/

static info_args** Set_Flags(section *SPtr, int *Flags)
{
	static int NumArgs;
	info_args** RetCode = NULL;
	entry* EPtr;
	int Flag;


	if (SPtr == NULL)
	{
		NumArgs = 0;
		return NULL;
	}

	RetCode = Set_Flags(SPtr->next,Flags);

	if (strcmp(SPtr->name,CONF_SEC_FLAG))
		return RetCode;

	RetCode = realloc(RetCode, sizeof(info_args*) * (NumArgs+2));
	RetCode[NumArgs] = (info_args*) calloc(1, sizeof(info_args));

	EPtr = SPtr->entries;

	while (EPtr != NULL)
	{
		if (!strcmp(EPtr->name,CONF_ENT_FLAGS))
		{
  		if ((Flag = Get_Events(EPtr->value)) < 0)
  			_print_msg("Error: Invalid value of variable `%s'!\n",EPtr->name);
  		else
  		{
			  RetCode[NumArgs]->flag = Flag;
			  *Flags |= Flag;
			}
		}
		else
		if (!strcmp(EPtr->name,CONF_ENT_PROG))
		{
 		  RetCode[NumArgs]->infoarg = EPtr->value;
		}
		else
		if (!strcmp(EPtr->name,CONF_ENT_USER))
		{
 		  RetCode[NumArgs]->user = EPtr->value;
		}
		else
		if (!strcmp(EPtr->name,CONF_ENT_GROUP))
		{
 		  RetCode[NumArgs]->group = EPtr->value;
		}
		else
		if (!strcmp(EPtr->name,CONF_ENT_INTVAL))
		{
			if (EPtr->value != NULL)
				RetCode[NumArgs]->interval = atoi(EPtr->value);
		}
		else
		if (!strcmp(EPtr->name,CONF_ENT_TIME))
		{
 		  RetCode[NumArgs]->time = EPtr->value;
		}
/*
		else
   		_print_msg("Error: Invalid variable `%s'!\n",EPtr->name);
*/

		EPtr = EPtr->next;
	}


  RetCode[++NumArgs] = NULL;

  return RetCode;
}

/****************************************************************************/

static int Get_Events(char* Flags)
{
	int flag = 0, Index = 0;


	while (*Flags != '\0')
	{
		while (isspace(*Flags) || *Flags == C_FLAG_DELIM) Flags++;

		if (*Flags != '\0')
		{
			switch (toupper(*Flags))
			{
					case 'I' : flag |= RING_INCOMING; break;
					case 'O' : flag |= RING_OUTGOING; break;
					case 'R' : flag |= RING_RING    ; break;
					case 'C' : flag |= RING_CONNECT ; break;
					case 'B' : flag |= RING_BUSY    ; break;
					case 'A' : flag |= RING_AOCD    ; break;
					case 'E' : flag |= RING_ERROR   ; break;
					case 'H' : flag |= RING_HANGUP  ; break;
					case 'K' : flag |= RING_KILL    ; break;
					case 'L' : flag |= RING_LOOP    ; break;
					case 'S' : flag |= RING_SPEAK   ; break;
					case 'P' : flag |= RING_PROVIDER; break;
					case 'U' : flag |= RING_UNIQUE  ; break;

					default : _print_msg( "%s: WARNING: Unknown flag `%c' in file \"%s\" line %d, ignored\n", Myname, *Flags, CONFFILE, ln);
					          break;
			} /* switch */

			Flags++;
		}

		Index++;
	}

	return flag;
}

/****************************************************************************/

static int Set_Numbers(section *SPtr, char *Section, int msn)
{
  auto entry *CEPtr;
  auto char  *num = 0, *who = 0;
  auto int    Index;


	if ((CEPtr = Get_Entry(SPtr->entries,CONF_ENT_NUM)) != NULL)
		num = CEPtr->value;
	else
	{
		_print_msg("%s: ERROR: There is no variable `%s' in section `%s'!\n", Myname, CONF_ENT_NUM, Section);
		num = S_UNKNOWN;
	}

	if ((CEPtr = Get_Entry(SPtr->entries,CONF_ENT_ALIAS)) != NULL)
		who = CEPtr->value;
	else
	{
		_print_msg("%s: ERROR: There is no variable `%s' in section `%s'!\n", Myname, CONF_ENT_ALIAS, Section);
		who = S_UNKNOWN;
	}

	if (*num && *who) {

		if ((Index = Set_known_Size(msn)) >= 0) {

			known[Index]->num       = num;
			known[Index]->who       = who;
			known[Index]->day     = -1;
			known[Index]->charge  = 0.0;
			known[Index]->rcharge = 0.0;
			known[Index]->scharge = 0.0;

			known[Index]->month   = -1;
			known[Index]->online  = 0.0;
			known[Index]->sonline = 0.0;

			known[Index]->bytes   = 0.0;
			known[Index]->sbytes  = 0.0;

			if ((CEPtr = Get_Entry(SPtr->entries,CONF_ENT_ZONE)) != NULL)
				known[Index]->zone = atoi(CEPtr->value);
#if 0 /* FIXME - or REMOVEME */
			else {
			  if (msn < 0) {
                            if ((known[Index]->zone = area_diff(NULL, num)) < 1) {
                              if (is_sondernummer(num, DTAG)) /* Fixme: DTAG is specific to Germany */
			        known[Index]->zone = SONDERNUMMER;
                              else {
			        _print_msg("%s: WARNING: There is no variable `%s' for number `%s' -- assuming GermanCall!\n", Myname, CONF_ENT_ZONE, num);
			      	known[Index]->zone = GERMANCALL;
                              } /* else */
                            } /* if */
				}
				else
			    known[Index]->zone = CITYCALL; /* sich selbst anrufen kostet CityCall */
			}
#endif
			if ((CEPtr = Get_Entry(SPtr->entries,CONF_ENT_INTFAC)) != NULL)
				known[Index]->interface = CEPtr->value;
			else
				known[Index]->interface = "-";

			if ((CEPtr = Get_Entry(SPtr->entries,CONF_ENT_SI)) != NULL)
				known[Index]->si = strtol(CEPtr->value, NIL, 0);
			else
				known[Index]->si = 0;

			if ((CEPtr = Get_Entry(SPtr->entries,CONF_ENT_START)) != NULL)
				known[Index]->infoargs = Set_Flags(CEPtr->subsection,&(known[Index]->flags));
			else
				known[Index]->infoargs = NULL;
		}
		else
		{
			_print_msg("%s: ERROR: Can't allocate memory!\n", Myname);
			return -1;
		}
	}

	return 0;
}

/****************************************************************************/

static int Set_known_Size(int msn)
{
	int Index;
	KNOWN **Ptr;

	if (knowns == 0)
		mymsns = 0;

	if (msn < 0)
	{
		if ((known = (KNOWN **) realloc(known, sizeof(KNOWN *) * (knowns + 1))) == NULL)
			return -1;

		knowns++;
	}
	else
	if (mymsns <= msn)
	{
		int NewKnowns = msn+knowns-mymsns+1;

		if ((Ptr = (KNOWN **) calloc(NewKnowns, sizeof(KNOWN *))) == NULL)
			return -1;

		if (known != NULL)
		{
			memcpy(Ptr,known,sizeof(KNOWN*) * (mymsns));
			memcpy(&(Ptr[msn+1]),&(known[mymsns]),sizeof(KNOWN*) * (knowns-mymsns));
			free(known);
		}

		known = Ptr;
		knowns= NewKnowns;
		mymsns = msn+1;
	}

	if (msn < 0)
		Index = knowns - 1;
	else
		Index = msn;

  if ((known[Index] = (KNOWN *) calloc(1,sizeof(KNOWN))) == NULL)
  	return -1;

	return Index;
}

/****************************************************************************/

static int readoldconfig(char *myname)
{
  register char *p;
  register int	 i;
  auto     int   start_ln = 0;
  auto     FILE *f;
  auto     char  s[BUFSIZ];
  auto 	   char *num, *who, *zone, *flags, *interface;
  auto     char *Ptr;

	ln = 0;

  mymsns     = 3;
  mycountry     = "";
  myarea   = "";
  currency   = NULL;
  chargemax  = 0.0;
  connectmax = 0.0;
  bytemax    = 0.0;
  connectmaxmode = 0;
  bytemaxmode = 0;
  knowns     = retnum = 0;
  known      = (KNOWN **)NULL;
  start_procs.infoargs = NULL;
  start_procs.flags    = 0;


  sprintf(s, "%s%c%s", confdir(), C_SLASH, OLDCONFFILE);

  if ((f = fopen(s, "r")) != (FILE *)NULL) {

    while (FGets(s, BUFSIZ, f, &ln) != NULL) {
      if ((*s != '\0') && (*s != '\n')) {

        if ((p = strchr(s, '\n'))) {
          *p = 0;

          while (*--p == ' ')
            *p = 0;
        } /* if */

        if (!IsVariable(s))
        {
          num       = s;
          who	    = NextItem(num, 1);
          zone      = NextItem(who, 0);
          interface = NextItem(zone, 0);
          flags     = NextItem(interface, 0);

          if (*num && *who) {
            known = realloc(known, sizeof(KNOWN *) * (knowns + 1));

            if ((known[knowns] = (KNOWN *)malloc(sizeof(KNOWN)))) {

              if ((p = strchr(num, ','))) { /* MSN,SI */
      	        known[knowns]->si      = atoi(p + 1);
		*p = 0;
              }
              else
      	        known[knowns]->si      = 0;

      	      known[knowns]->num       = strdup(num);
      	      known[knowns]->who       = strdup(who);
      	      known[knowns]->zone      = atoi(zone);
	      known[knowns]->interface = strdup(interface);
              known[knowns]->flags     = 0;

              SetFlags(known[knowns], flags);

  	      known[knowns]->dur[CALLING] = known[knowns]->dur[CALLED] = 0.0;
  	      known[knowns]->eh = 0;
  	      known[knowns]->usage[DIALOUT] = known[knowns]->usage[DIALIN] = 0;

      	      known[knowns]->day     = -1;
      	      known[knowns]->charge  = 0.0;
      	      known[knowns]->rcharge = 0.0;
      	      known[knowns]->scharge = 0.0;

      	      known[knowns]->month   = -1;
              known[knowns]->online  = 0.0;
              known[knowns]->sonline = 0.0;

              known[knowns]->bytes   = 0.0;
	      known[knowns]->sbytes  = 0.0;

              knowns++;
            }
            else {
              _print_msg("%s: WARNING: Out of memory in Line %d\n", myname, ln);
              break;
            } /* else */
          }
          else {
  	        _print_msg("%s: WARNING: Syntax error in Line %d, ignored\n", myname, ln);
            /* break; */
          } /* else */
        } /* else */
        else
          if (start_ln == 0 && getenv(VAR_START) != NULL)
            start_ln = ln;

      } /* if */

    } /* while */

    fclose(f);

    if ((Ptr = getenv(VAR_MYMSNS)) == NULL) {
      _print_msg("%s: WARNING: Variable `%s' is not set, now is 3!\n", myname, VAR_MYMSNS);
      mymsns = 3;
    }
    else
      mymsns = atoi(Ptr);

    if ((myarea = getenv(VAR_MYAREA)) == NULL)
      _print_msg("%s: WARNING: Variable `%s' is not set!\n", myname, VAR_MYAREA);

    if ((mycountry = getenv(VAR_MYCOUNTRY)) == NULL)
      _print_msg("%s: WARNING: Variable `%s' is not set!\n", myname, VAR_MYCOUNTRY);

    if ((Ptr = getenv(VAR_CURRENCY)) != NULL) {
      currency_factor = atof(Ptr);

      if ((currency = strchr(Ptr, ',')) == NULL)
	_print_msg("%s: WARNING: Syntax error in `CURRENCY' in Line %d, ignored\n", myname, ln);
      else
        currency++;
    }

    if ((Ptr = getenv(VAR_CHARGEMAX)) == NULL)
      _print_msg("%s: WARNING: Variable `%s' is not set, \nperforming no action when chargemax-overflow\n", myname, VAR_CHARGEMAX);
    else
      chargemax = strtod(Ptr, NULL);

    if ((Ptr = getenv(VAR_CONNECTMAX)) == NULL)
      _print_msg("%s: WARNING: Variable `%s' is not set, \nperforming no action when connectmax-overflow\n", myname, VAR_CONNECTMAX);
    else {
      if (sscanf(Ptr, "%lg,%d", &connectmax, &connectmaxmode) != 2)
	_print_msg("%s: WARNING: Syntax error in `%s' in Line %d, ignored\n", Myname, VAR_CONNECTMAX, ln);
    } /* else */

    if ((Ptr = getenv(VAR_BYTEMAX)) == NULL)
      _print_msg("%s: WARNING: Variable `%s' is not set, \nperforming no action when connectmax-overflow\n", myname, VAR_BYTEMAX);
    else {
      if (sscanf(Ptr, "%lg,%d", &bytemax, &bytemaxmode) != 2)
	_print_msg("%s: WARNING: Syntax error in `%s' in Line %d, ignored\n", Myname, VAR_BYTEMAX, ln);
    } /* else */

    if ((Ptr = getenv(VAR_START)) != NULL) {
      ln = start_ln;
      SetFlags(&start_procs, Ptr);
    }

    Set_ILabel(getenv(VAR_ILABEL));
    Set_OLabel(getenv(VAR_OLABEL));

/* Wenn eine unbekannte Varible kommt, stuerzt isdnlog einfach ab  ;-) !!!!*/
    if ((mycountry != NULL && mycountry[0] == '\0') &&
        (myarea != NULL && myarea[0] == '\0')         )
    {
      for (i = 0; i < mymsns; i++) {
        if (known != NULL && known[i]->num != NULL) {
          sprintf(s, "%s%s%s", mycountry, myarea, known[i]->num);
          free(known[i]->num);
          known[i]->num  = strdup(s);
        }
        else
        {
        	_print_msg("%s: ERROR: There are only %d MSN's, expected %d!\n",myname, i, mymsns);
          return -1;
        }
      } /* for */
    } /* if */
  }
  else
  {
    _print_msg("%s: WARNING: Can't open ``%s''\n", myname, s);
    return -1;
  }

  return 0;
} /* readconfig */

/****************************************************************************/

void discardconfig(void)
{
  register int i,j;

	free_section(conf_dat);
	ClearEnv(&Environment);

  for (i = 0; i < knowns; i++) {
  /* Unnoetig mit neuer readconfig
    free(known[i]->num);
    free(known[i]->who);
    free(known[i]->interface);
  */

  	for (j = 0; known[i]->infoargs != NULL && known[i]->infoargs[j] != NULL; j++) {
  	/* Unnoetig mit neuer readconfig
    	free(known[i]->infoargs[j]->time);
    	free(known[i]->infoargs[j]->infoarg);
    */
    	free(known[i]->infoargs[j]);
  	}

		free(known[i]->infoargs);
    free(known[i]);
  } /* for */

  free(known);

  currency = NULL;
  mycountry   = "";
  myarea = "";

  if (start_procs.infoargs != NULL)
  {
  	int j;
    for (j = 0; start_procs.infoargs != NULL && start_procs.infoargs[j] != NULL; j++) {
    /* Unnoetig mit neuer readconfig
      free(start_procs.infoargs[j]->time);
      free(start_procs.infoargs[j]->infoarg);
     */
      free(start_procs.infoargs[j]);
    }
	  free(start_procs.infoargs);
	}

} /* discardconfig */

/****************************************************************************/

static int SetEnv(char ****EnvPtr, char *name, char *value)
{
	int elem = 2;
	char ***CurEnvPtr = NULL;


	if (setenv(name,value,1) != 0)
		return -1;

	if (!strcmp(name,CONF_ENT_AREA)    || !strcmp(name,CONF_ENT_CURR)   ||
	    !strcmp(name,CONF_ENT_START)   || !strcmp(name,CONF_ENT_ILABEL) ||
	    !strcmp(name,CONF_ENT_OLABEL)  || !strcmp(name,VAR_MYMSNS)      ||
	    !strcmp(name,CONF_ENT_COUNTRY) || !strcmp(name,CONF_ENT_CHARGE) ||
	    !strcmp(name,CONF_ENT_RELOAD)  || !strcmp(name,CONF_ENT_STOP)   ||
	    !strcmp(name,CONF_ENT_REBOOT)  || !strcmp(name,CONF_ENT_CONNECT)||
	    !strcmp(name,CONF_ENT_AREA)    || !strcmp(name,CONF_ENT_CONNECT)||
	    !strcmp(name,CONF_ENT_BYTE)                                       )
		return 0;

	CurEnvPtr = *EnvPtr;

	if (CurEnvPtr != NULL)
		while(*CurEnvPtr != NULL)
		{
			elem++;
			CurEnvPtr++;
		}

	CurEnvPtr = NULL;

	if ((*EnvPtr = (char***) realloc(*EnvPtr,sizeof(char**)*elem)) == NULL)
		return -1;

	(*EnvPtr)[elem-1] = NULL;

	if (((*EnvPtr)[elem-2] = (char**) calloc(2,sizeof(char*))) == NULL)
		return -1;

	if (((*EnvPtr)[elem-2][0] = strdup(name)) == NULL)
		return -1;

	if (((*EnvPtr)[elem-2][1] = strdup(value)) == NULL)
		return -1;

	return 0;
}

/****************************************************************************/

static int GetNextEnv(char ***EnvPtr, char **name, char **value)
{
	static char ***CurEnvPtr = NULL;


	if (EnvPtr != NULL)
		CurEnvPtr = EnvPtr;
	else
	{
		if (CurEnvPtr == NULL || CurEnvPtr[0] == NULL)
			return -1;
		else
			CurEnvPtr++;
	}


	if (CurEnvPtr[0] == NULL || CurEnvPtr[0][0] == NULL || CurEnvPtr[0][1] == NULL)
		return -1;

	*name  = CurEnvPtr[0][0];
	*value = CurEnvPtr[0][1];

	return 0;
}

/****************************************************************************/

static int ClearEnv(char ****EnvPtr)
{
	char ***CurEnvPtr = NULL;
	CurEnvPtr = *EnvPtr;

	if (CurEnvPtr == NULL)
		return 0;

	while(CurEnvPtr[0] != NULL)
	{
		free(CurEnvPtr[0][0]);
		free(CurEnvPtr[0][1]);
		free(CurEnvPtr[0]);
		CurEnvPtr++;
	}

	free(*EnvPtr);
	*EnvPtr = CurEnvPtr = NULL;

	return 0;
}

/****************************************************************************/

static int Set_ILabel(char *value)
{
		if ((IlabelPtr = value) == NULL)
			IlabelPtr = "%b %e %T %ICall to tei %t from %N2 on %n2";

		sprintf(ilabel, "%%s%s  %%s%%s", IlabelPtr);

    return 0;
}

/****************************************************************************/

static int Set_OLabel(char *value)
{
		if ((OlabelPtr = value) == NULL)
			OlabelPtr = "%b %e %T %Itei %t calling %N2 with %n2";

		sprintf(olabel, "%%s%s  %%s%%s", OlabelPtr);

    return 0;
}

/****************************************************************************/
