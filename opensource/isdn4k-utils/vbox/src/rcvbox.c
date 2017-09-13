/*
** $Id: rcvbox.c,v 1.6 1998/04/28 08:34:44 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fnmatch.h>

#include "log.h"
#include "rcvbox.h"
#include "streamio.h"
#include "libvbox.h"

/** Prototypes ***********************************************************/

static int		vboxrc_goto_section(char *);
static int		vboxrc_parse_time(char *);
static int		vboxrc_parse_days(char *);
static void		vboxrc_return_timestr(time_t, char *);
static time_t	vboxrc_return_time(time_t, char *, int);

/** Variables ************************************************************/

static char *weekdaynames[] =
{
	"SO" , "MO" , "DI" , "MI" , "DO" , "FR" , "SA" ,
	"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",

	NULL
};

/*************************************************************************/
/** vboxrc_return_time():	Converts a time string to seconds since the	**/
/**								1st januar 1970.										**/
/*************************************************************************/

static time_t vboxrc_return_time(time_t timenow, char *timestr, int mode)
{
	struct tm *locala;
	struct tm  localb;

	char	timestring[5 + 1];
	char *hourstr;
	char *minsstr;
	int	hourint;
	int	minsint;
	int	secsint;

	xstrncpy(timestring, timestr, 5);

	hourstr = timestring;
	minsstr = index(hourstr, ':');

	if (!minsstr)
	{
		if (mode == 0) minsstr = "00";
		if (mode != 0) minsstr = "59";
	}
	else *minsstr++ = '\0';

	if (mode == 0) secsint =  0;
	else           secsint = 59;

	hourint = atoi(hourstr);
	minsint = atoi(minsstr);

	if ((hourint < 0) || (hourint > 23)) return(0);
	if ((minsint < 0) || (minsint > 59)) return(0);

	if (!(locala = localtime(&timenow))) return(0);

	memcpy(&localb, locala, sizeof(struct tm));

	localb.tm_sec	= secsint;
	localb.tm_min	= minsint;
	localb.tm_hour	= hourint;

	return(mktime(&localb));
}

/*************************************************************************/
/** vboxrc_return_timestr():	Converts a unix time into a string like	**/
/**									HH:MM:SS.											**/
/*************************************************************************/

static void vboxrc_return_timestr(time_t timeint, char *timestr)
{
	struct tm *tm;

	if ((tm = localtime(&timeint)))
	{
		if (strftime(timestr, 20, "%H:%M:%S", tm) == 8) return;
	}

	xstrncpy(timestr, "??:??:??", 8);
}

/*************************************************************************/
/** vboxrc_parse_time():	Checks if one or more timestring match the	**/
/**								current time.											**/
/*************************************************************************/

static int vboxrc_parse_time(char *timestr)
{
	char		timestring[VBOXRC_MAX_RCLINE + 1];
	char		timevaluebeg[8 + 1];
	char		timevalueend[8 + 1];
	char		timevaluenow[8 + 1];
	char	  *timeptr;
	char	  *timenxt;
	char	  *timebeg;
	char	  *timeend;
	time_t	timenow;
	time_t	timesecsbeg;
	time_t	timesecsend;

	log(L_DEBUG, "Parsing time(s) \"%s\"...\n", timestr);

	xstrncpy(timestring, timestr, VBOXRC_MAX_RCLINE);

	timeptr = timestring;
	timenow = time(NULL);

	vboxrc_return_timestr(timenow, timevaluenow);

	if (strcmp(timestring, "*") == 0)
	{
		log(L_DEBUG, "Range **:**:** - **:**:** (%s): match.\n", timevaluenow);
		
		returnok();
	}
	
	if ((strcmp(timestring, "!") == 0) || (strcmp(timestring, "-") == 0))
	{
		log(L_DEBUG, "Range --:--:-- - --:--:-- (%s): don't match.\n", timevaluenow);
		
		returnerror();
	}
	
	while (timeptr)
	{
		if ((timenxt = index(timeptr, ','))) *timenxt++ = '\0';

		timebeg = timeptr;
		timeend = index(timebeg, '-');

		if (timeend)
			*timeend++ = '\0';
		else
			timeend = timebeg;

		if (!timeend) timeend = timebeg;

		timesecsbeg = vboxrc_return_time(timenow, timebeg, 0);
		timesecsend = vboxrc_return_time(timenow, timeend, 1);

		if ((timesecsbeg > 0) && (timesecsend > 0))
		{
			if ((timesecsend >= timesecsbeg))
			{
				vboxrc_return_timestr(timesecsbeg, timevaluebeg);
				vboxrc_return_timestr(timesecsend, timevalueend);
				vboxrc_return_timestr(timenow    , timevaluenow);
			
				log_line(L_DEBUG, "Range %s - %s (%s): ", timevaluebeg, timevalueend, timevaluenow);

				if ((timenow >= timesecsbeg) && (timenow <= timesecsend))
				{
					log_text(L_DEBUG, "match.\n");
				
					returnok();
				}
				else log_text(L_DEBUG, "don't match.\n");
			}
			else log(L_WARN, "Bad time; start greater than end (%s-%s) - ignored...\n", timebeg, timeend);
		}
		else log(L_WARN, "Bad time; can't convert timestring (%s-%s) - ignored...\n", timebeg, timeend);

		timeptr = timenxt;
	}

	log(L_JUNK, "String contains no matching time!\n");

	returnerror();
}

/*************************************************************************/
/** vboxrc_parse_days():	Checks if one or more daystrings match the	**/
/**								current day.											**/
/*************************************************************************/

static int vboxrc_parse_days(char *strdays)
{
	struct tm  *timelocal;
	char		  *beg;
	char		  *nxt;
	char			days[VBOXRC_MAX_RCLINE + 1];
	int			i;
	int			d;
	time_t		currenttime;

	xstrncpy(days, strdays, VBOXRC_MAX_RCLINE);

	log(L_DEBUG, "Parsing day(s) \"%s\"...\n", days);

	if (strcmp(days, "*") == 0)
	{
		log(L_DEBUG, "Range *: match.\n");
	
		returnok();
	}

	if ((strcmp(days, "-") == 0) || (strcmp(days, "!") == 0))
	{
		log(L_DEBUG, "Range -: don't match.\n");

		returnerror();
	}

	currenttime = time(NULL);

	if (!(timelocal = localtime(&currenttime)))
	{
		log(L_ERROR, "Can't get local time (don't match)...\n");
		
		returnerror();
	}

	for (i = 0; i < strlen(days); i++)
	{
		if ((!isalpha(days[i])) && (days[i] != ','))
		{
			log(L_ERROR, "Error in day string \"%s\" in line #%ld (don't match).\n", days, setup.vboxrc->line);

			returnerror();
		}
	}

	beg = days;

	while (beg)
	{
		if ((nxt = index(beg, ','))) *nxt++ = 0;

		d = 0;
		i = 0;

		while (weekdaynames[i])
		{
			if (strcasecmp(weekdaynames[i], beg) == 0)
			{
				if (d == timelocal->tm_wday)
				{
					log(L_DEBUG, "Range %s: match.\n", beg);

					returnok();
				}
				else log(L_DEBUG, "Range %s: don't match.\n", beg);
			}

			d++;
			i++;
			
			if (d >= 7) d = 0;
		}

		beg = nxt;
	}

	log(L_JUNK, "String contains no matching day!\n");

	returnerror();
}

/*************************************************************************/
/** vboxrc_goto_section():	Jump to a specified section.						**/
/*************************************************************************/

static int vboxrc_goto_section(char *section)
{
	char line[VBOXRC_MAX_RCLINE + 1];

	log(L_JUNK, "Jumping to section \"%s\"...\n", section);

	setup.vboxrc = streamio_reopen(setup.vboxrc);
   
	while (streamio_gets(line, VBOXRC_MAX_RCLINE, setup.vboxrc))
	{
		if (strcasecmp(line, section) == 0) returnok();
	}
                  
	returnerror();
}

/*************************************************************************/
/** vboxrc_get_rings_to_wait():	Returns the number of rings to answer	**/
/**										call.												**/
/*************************************************************************/

int vboxrc_get_rings_to_wait(void)
{
	char	line[VBOXRC_MAX_RCLINE + 1];
	char *time;
	char *ring;
	char *days;
	char *stop;
	int	need;
               
	log(L_DEBUG, "Getting number of rings to wait before answer call...\n");

	need = VBOXRC_DEF_RINGS;

	if (vboxrc_goto_section("[RINGS]"))
	{
		while (streamio_gets(line, VBOXRC_MAX_RCLINE, setup.vboxrc))
		{
			if ((*line == '[') || (*line == ']')) break;

			time = strtok(line, "\t ");
			days = strtok(NULL, "\t ");
			ring = strtok(NULL, "\t ");

			if ((!time) || (!ring) || (!days))
			{
				log(L_ERROR, "Error in vboxrc line #%ld (ignored).\n", setup.vboxrc->line);

				continue;
			}
                                                                  
			if ((vboxrc_parse_time(time)) && (vboxrc_parse_days(days)))
			{
				need = (int)strtoul(ring, &stop, 10);

				if (*stop != 0)
				{
					log(L_ERROR, "Bad number of rings in line #%ld (ignored).\n", setup.vboxrc->line);

					need = VBOXRC_DEF_RINGS;
				}
				else
				{
					log(L_DEBUG, "Call will be answered after %ld rings...\n", need);

					return(need);
				}
			}
		}
	}
	else log(L_WARN, "Unable to locate section \"[RINGS]\" (useing defaults)...\n");

	log(L_WARN, "Call will be answered after %ld rings (default)...\n", need);

	return(need);
}

/*************************************************************************/
/** vboxrc_find_user_from_id():	Finds a user with a specified caller	**/
/**										number.											**/
/*************************************************************************/

void vboxrc_find_user_from_id(char *id)
{
	char	line[VBOXRC_MAX_RCLINE + 1];
	char *phone;
	char *table;
	char *owner;
	char *dummy;

	log(L_DEBUG, "Searching user with caller number \"%s\"...\n", id);

	if (vboxrc_goto_section("[CALLERIDS]"))
	{
		while (streamio_gets(line, VBOXRC_MAX_RCLINE, setup.vboxrc))
		{
			if ((*line == '[') || (*line == ']')) break;

			dummy = line;
			owner = NULL;
			table = NULL;
			phone = NULL;
                                                
			while (isspace(*dummy)) dummy++;

			if ((*dummy) && (dummy)) phone = strsep(&dummy, "\t ");

			while (isspace(*dummy)) dummy++;

			if ((*dummy) && (phone)) table = strsep(&dummy, "\t ");

			while (isspace(*dummy)) dummy++;

			if ((*dummy) && (table)) owner = dummy;

			if ((!owner) || (!phone) || (!table))
			{
				log(L_ERROR, "Error in vboxrc line #%ld (ignored).\n", setup.vboxrc->line);

				continue;
			}

			if (fnmatch(phone, id, FNM_NOESCAPE | FNM_PERIOD) == 0)
			{
				if ((*table == '*') || (*table == '-'))
				{
					if (*table == '*') xstrncpy(setup.voice.section, owner     , VOICE_MAX_SECTION);
					if (*table == '-') xstrncpy(setup.voice.section, "STANDARD", VOICE_MAX_SECTION);
				}
				else xstrncpy(setup.voice.section, table, VOICE_MAX_SECTION);

				xstrncpy(setup.voice.name, owner, VOICE_MAX_NAME);

				log(L_DEBUG, "Caller number match user \"%s\"...\n", setup.voice.name);
				log(L_DEBUG, "Section \"[%s]\" will be used...\n", setup.voice.section);

				return;
			}
		}
	}
	else log(L_WARN, "Unable to locate section \"[CALLERIDS]\" (useing defaults)...\n");

	log(L_WARN, "Section \"[%s]\" will be used (default)...\n", setup.voice.section);
}

/*************************************************************************/
/** vboxrc_find_user_section():	Finds a specified user section.			**/
/*************************************************************************/

void vboxrc_find_user_section(char *section)
{
	char	line[VBOXRC_MAX_RCLINE + 1];
	char *time;
	char *days;
	char *text;
	char *strg;
	char *stop;
	char *args;
	int	correct;

	sprintf(line, "[%s]", section);

	log(L_DEBUG, "Parsing settings from section \"%s\"...\n", line);

	correct = FALSE;

	if (vboxrc_goto_section(line))
	{
		while (streamio_gets(line, VBOXRC_MAX_RCLINE, setup.vboxrc))
		{
			if ((*line == '[') || (*line == ']')) break;

			time = strtok(line, "\t ");
			days = strtok(NULL, "\t ");
			text = strtok(NULL, "\t ");
			strg = strtok(NULL, "\t ");

			if ((!time) || (!text) || (!strg) || (!days))
			{
				log(L_ERROR, "Error in vboxrc line #%ld (ignored).\n", setup.vboxrc->line);

				continue;
			}

			if ((vboxrc_parse_time(time)) && (vboxrc_parse_days(days)))
			{
				setup.voice.recordtime = (int)strtoul(strg, &stop, 10);

				if ((*stop != 0) || (setup.voice.recordtime < 0))
				{
					log(L_ERROR, "Bad record time in line #%ld (using defaults).\n", setup.vboxrc->line);

					setup.voice.recordtime = 60;
				}

				if (!index(text, '/'))
				{
					xstrncpy(setup.voice.standardmsg, setup.spool , VOICE_MAX_MESSAGE);
					xstrncat(setup.voice.standardmsg, "/messages/", VOICE_MAX_MESSAGE);
					xstrncat(setup.voice.standardmsg, text        , VOICE_MAX_MESSAGE);
				}
				else xstrncpy(setup.voice.standardmsg, text, VOICE_MAX_MESSAGE);

				while ((text = strtok(NULL, "\t ")))
				{
					log(L_JUNK, "Found Flag \"%s\"...\n", text);

					if (strcasecmp(text, "NOANSWER"    ) == 0) setup.voice.doanswer	= FALSE;
					if (strcasecmp(text, "NORECORD"    ) == 0) setup.voice.dorecord	= FALSE;
					if (strcasecmp(text, "NOTIMEOUTMSG") == 0) setup.voice.dotimeout	= FALSE;
					if (strcasecmp(text, "NOBEEPMSG"   ) == 0) setup.voice.dobeep		= FALSE;
					if (strcasecmp(text, "NOSTDMSG"    ) == 0) setup.voice.domessage	= FALSE;

					if (strncasecmp(text, "RINGS=", 6) == 0)
					{
						if ((args = index(text, '=')))
						{
							args++;
							
							setup.voice.rings = strtol(args, &stop, 10);

							if ((*stop != 0) || (setup.voice.rings < 0))
							{
								log(L_ERROR, "Bad flag RINGS in line #%ld (ignored).\n", setup.vboxrc->line);

								setup.voice.rings = -1;
							}
						}
					}

					if (strncasecmp(text, "TOLLRINGS=", 10) == 0)
					{
						if ((args = index(text, '=')))
						{
							args++;
							
							setup.voice.ringsonnew = strtol(args, &stop, 10);

							if ((*stop != 0) || (setup.voice.ringsonnew < 0))
							{
								log(L_ERROR, "Bad flag TOLLRINGS in line #%ld (ignored).\n", setup.vboxrc->line);

								setup.voice.ringsonnew = -1;
							}
						}
					}

					if (strncasecmp(text, "SCRIPT=", 7) == 0)
					{
						if ((args = index(text, '=')))
						{
							args++;

							if (!index(args, '/'))
							{
								xstrncpy(setup.voice.tclscriptname , setup.spool, VOICE_MAX_SCRIPT);
								xstrncat(setup.voice.tclscriptname , "/"        , VOICE_MAX_SCRIPT);
								xstrncat(setup.voice.tclscriptname , args       , VOICE_MAX_SCRIPT);
							}
							else xstrncpy(setup.voice.tclscriptname, args, VOICE_MAX_SCRIPT);
						}
					}

					if (strncasecmp(text, "TOLLCHECK=", 10) == 0)
					{
						if ((args = index(text, '=')))
						{
							args++;

							xstrncpy(setup.voice.checknewpath, args, VOICE_MAX_CHECKNEW);
						}
					}
				}

				correct = TRUE;

				break;
			}
		}
	}
	else log(L_WARN, "Unable to locate section \"%s\" (useing defaults)...\n", line);

	if (!correct)
	{
		log(L_WARN, "No (or not all) settings found (using defaults)...\n");
	}

	log(L_JUNK, "Settings: Message \"%s\".\n", setup.voice.standardmsg);
	log(L_JUNK, "Settings: Beep \"%s\".\n", setup.voice.beepmsg);
	log(L_JUNK, "Settings: Timeout \"%s\".\n", setup.voice.timeoutmsg);
	log(L_JUNK, "Settings: Script \"%s\".\n", setup.voice.tclscriptname);
	
	if (setup.voice.rings > 0)
	{
		log(L_JUNK, "Settings: Rings changed to %d.\n", setup.voice.rings);
	}
	else log(L_JUNK, "Settings: Rings not changed.\n");

	if (setup.voice.ringsonnew > 0)
	{
		log(L_JUNK, "Settings: Rings changed to %d if new message exist.\n", setup.voice.ringsonnew);
		log(L_JUNK, "Settings: Check for new messages in \"%s\".\n", setup.voice.checknewpath);
	}

	log(L_JUNK, "Settings: %d secs record time.\n", setup.voice.recordtime);
	log(L_JUNK, "Settings: %s.\n", setup.voice.doanswer ? "Answer call" : "Don't answer call");
	log(L_JUNK, "Settings: %s.\n", setup.voice.dorecord ? "Record a message" : "Don't record a message");
	log(L_JUNK, "Settings: %s.\n", setup.voice.domessage ? "Play standard message" : "Don't play standard message");
	log(L_JUNK, "Settings: %s.\n", setup.voice.dobeep ? "Play beep message" : "Don't play beep message");
	log(L_JUNK, "Settings: %s.\n", setup.voice.dotimeout ? "Play timeout message" : "Don't play timeout message");
}
