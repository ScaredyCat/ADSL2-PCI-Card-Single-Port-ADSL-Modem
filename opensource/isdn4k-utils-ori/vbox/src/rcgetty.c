/*
** $Id: rcgetty.c,v 1.10 2002/01/31 20:09:21 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>

#include "rcgetty.h"
#include "streamio.h"
#include "log.h"
#include "init.h"
#include "libvbox.h"

/** Prototypes ***********************************************************/

static char *parse_str(char *, char *, void *, int, int);
static char *parse_uid(char *, char *, void *, int, int);
static char *parse_gid(char *, char *, void *, int, int);
static char *parse_int(char *, char *, void *, int, int);
static char *parse_cmp(char *, char *, void *, int, int);
static char *parse_msk(char *, char *, void *, int, int);
static char *parse_log(char *, char *, void *, int, int);

static int parse_line(streamio_t *, char *, int, char *, int);

/** Structures ***********************************************************/

struct rctable
{
	char	*name;
	void	*ptr;
	int	 min;
	int	 max;
	char	*(*parsefunction)(char *, char *, void *, int, int);
};   

static long useloglevel = L_DEFAULT;

static struct rctable rct[] =
{
	{ "modeminit"		,  setup.modem.init				, 0, MODEM_MAX_INITSTRING	, parse_str },
	{ "interninita"	,  setup.modem.interninita		, 0, MODEM_MAX_INITSTRING	, parse_str },
	{ "interninitb"	,  setup.modem.interninitb		, 0, MODEM_MAX_INITSTRING	, parse_str },
	{ "user"				, &setup.users.uid				, 0, 0							, parse_uid },
	{ "group"			, &setup.users.gid				, 0, 0							, parse_gid },
	{ "umask"			, &setup.users.umask				, 0, 0							, parse_msk },
	{ "dropdtrtime"	, &setup.modem.toggle_dtr_time, 0, 60000						, parse_int },
	{ "initpause"		, &setup.modem.initpause		, 0, 60000						, parse_int },
	{ "badinitsexit"	, &setup.modem.badinitsexit	, 0, 60000						, parse_int },
	{ "ringtimeout"	, &setup.modem.timeout_ring	, 0, 60000						, parse_int },
	{ "echotimeout"	, &setup.modem.timeout_echo	, 0, 60000						, parse_int },
	{ "commandtimeout", &setup.modem.timeout_cmd		, 0, 60000						, parse_int },
	{ "alivetimeout"	, &setup.modem.timeout_alive	, 0, 60000						, parse_int },
	{ "compression"	, &setup.modem.compression		, 2, 6							, parse_cmp },
	{ "vboxconfig"		,  setup.vboxrcname				, 0, SETUP_MAX_VBOXRC		, parse_str },
	{ "spooldir"		,  setup.spool						, 0, SETUP_MAX_SPOOLNAME	, parse_str },
	{ "freespace"		, &setup.freespace				, 0, 2000000000				, parse_int },
	{ "debuglevel"    , &useloglevel                , 0, 60000                 , parse_log },

	{ NULL, NULL, 0, 0, NULL}
};

/*************************************************************************/
/** getty_get_settings(): Reads and parse vboxgetty's settings.			**/
/*************************************************************************/

int getty_get_settings(char *rcname)
{
	streamio_t	*rc;
	char			 parsedevice[MODEM_MAX_TTYNAME + 1];
	char			 cmd[MODEM_MAX_RCCMD + 1];
	char			 arg[MODEM_MAX_RCARG + 1];
	char			*msg;
	int			 result;
	int			 i;

	log(L_DEBUG, "Parsing settings in \"%s\" for port \"%s\"...\n", rcname, setup.modem.device);

	xstrncpy(setup.modem.interninita	, "AT+FCLASS=8"	  , MODEM_MAX_INITSTRING);
	xstrncpy(setup.modem.interninitb	, "ATS13.2=1S13.4=1", MODEM_MAX_INITSTRING);
	xstrncpy(setup.modem.init			, "ATZ"				  , MODEM_MAX_INITSTRING);

	setup.modem.toggle_dtr_time	= MODEM_TOGGLETIME;
	setup.modem.timeout_ring		= MODEM_RINGTIMEOUT;
	setup.modem.timeout_echo		= MODEM_ECHOTIMEOUT;
	setup.modem.timeout_cmd			= MODEM_CMDTIMEOUT;
	setup.modem.timeout_alive		= MODEM_TIMEOUT;
	setup.modem.fd						= -1;
	setup.modem.badinitsexit		= 0;
	setup.modem.compression			= 4;
	setup.modem.initpause			= 1500;

	setup.users.home[0]	= '\0';
	setup.users.name[0]	= '\0';
	setup.users.uid		= 0;
	setup.users.gid		= 0;
	setup.users.umask		= S_IRWXG|S_IRWXO;

		/*
		 * Parse the configuration and put all correct values into the
		 * global structure.
		 */

	if ((rc = streamio_open(rcname)))
	{
		xstrncpy(parsedevice, "-global-", MODEM_MAX_TTYNAME);

		while ((result = parse_line(rc, cmd, MODEM_MAX_RCCMD, arg, MODEM_MAX_RCARG)) == 1)
		{
			if (strcasecmp(cmd, "port") == 0)
			{
				xstrncpy(parsedevice, arg, MODEM_MAX_TTYNAME);

				log(L_DEBUG, "Found command '%s = %s'...\n", cmd, arg);

				continue;
			}

			if ((strcmp(parsedevice, "-global-") == 0) || (strcmp(parsedevice, setup.modem.device) == 0))
			{
				i = 0;
				
				while (rct[i].name)
				{
					if (strcasecmp(rct[i].name, cmd) == 0)
					{
						if (!(msg = rct[i].parsefunction(cmd, arg, rct[i].ptr, rct[i].min, rct[i].max)))
						{
							log(L_DEBUG, "Found command '%s = %s' (%s)...\n", cmd, arg, parsedevice);
						}
						else
						{
							log(L_ERROR, "Bad value \"%s\" for command \"%s\" ignored (%s)...\n", arg, cmd, parsedevice);
							log(L_ERROR, "Parser returns \"%s\" (min %d; max %d).\n", msg, rct[i].min, rct[i].max);
						}

						break;
					}

					i++;
				}

				if (!rct[i].name)
				{
					log(L_WARN, "Unknown command \"%s\" ignored (%s)...\n", cmd, parsedevice);
				}
			}
		}

		streamio_close(rc);

		if (!result)
		{
			if (!*cmd) log(L_ERROR, "Error in \"%s\": Missing command.\n", rcname);
			if (!*arg) log(L_ERROR, "Error in \"%s\": Missing argument (%s).\n", rcname, cmd);

			returnerror();
		}
	}
	else
	{
		log(L_ERROR, "Can't open \"%s\".\n", rcname);
		
		returnerror();
	}

        if (setup.modem.timeout_ring < MODEM_RINGTIMEOUT) {
            log(L_WARN, "ringtimeout is set to %d; at least %d is recommended.\n", setup.modem.timeout_ring, MODEM_RINGTIMEOUT);
        }

	returnok();
}

/*************************************************************************/
/** parse_line():	Splits a line into command and argument.					**/
/*************************************************************************/

static int parse_line(streamio_t *rc, char *cmd, int maxcmd, char *arg, int maxarg)
{
	char	temp[MODEM_MAX_RCLINE + 1];
	char *line;
	char *stop;

	*cmd = '\0';
	*arg = '\0';

	while (streamio_gets(temp, MODEM_MAX_RCLINE, rc))
	{
		line = temp;

		while (isspace(*line)) line++;

		stop = line;
		
		while ((!isspace(*stop)) && (*stop != '=') && (*stop)) stop++;

		*stop++ = '\0';

		xstrncpy(cmd, line, maxcmd);

		line = stop;

		while ((isspace(*line)) || (*line == '=')) line++;

		xstrncpy(arg, line, maxarg);

		if ((*cmd) && (*arg)) returnok();

		returnerror();
	}

	return(-1);
}

/*************************************************************************/
/** parse_cmp(): Converts string into compression mode.						**/
/*************************************************************************/

static char *parse_cmp(char *cmd, char *arg, void *ptr, int min, int max)
{
	int cmp = 0;

	if (strcasecmp(arg, "adpcm-2") == 0) cmp = 2;
	if (strcasecmp(arg, "adpcm-3") == 0) cmp = 3;
	if (strcasecmp(arg, "adpcm-4") == 0) cmp = 4;
	if (strcasecmp(arg, "alaw"   ) == 0) cmp = 5;
	if (strcasecmp(arg, "ulaw"   ) == 0) cmp = 6;

	if ((cmp < min) || (cmp > max)) return("unknown compression");

	if (cmp == 5) return("alaw not longer supported");

	(*(int *)ptr) = cmp;

	return(NULL);
}

/*************************************************************************/
/** parse_int(): Converts string into integer number.						   **/
/*************************************************************************/

static char *parse_int(char *cmd, char *arg, void *ptr, int min, int max)
{
	char	*stop;
	int	 nr;

	nr = (int)strtol(arg, &stop, 10);
	
	if ((nr >= min) && (nr <= max) && (*stop == '\0'))
	{
		(*(int *)ptr) = nr;
		
		return(NULL);
	}

	if (nr < min) return("value too small");
	if (nr > max) return("value too big");

	return("can't convert value to integer");
}

/*************************************************************************/
/** parse_gid(): Converts string into groupid.									**/
/*************************************************************************/

static char *parse_gid(char *cmd, char *arg, void *ptr, int min, int max)
{
	struct group *group;
	
	if ((group = getgrnam(arg)))
	{
		(*(gid_t *)ptr) = group->gr_gid;
		
		return(NULL);
	}

	return("unknown groupname");
}

/*************************************************************************/
/** parse_uid(): Converts string into userid.									**/
/*************************************************************************/

static char *parse_uid(char *cmd, char *arg, void *ptr, int min, int max)
{
	struct passwd *passwd;
	
	if ((passwd = getpwnam(arg)))
	{
		(*(uid_t *)ptr) = passwd->pw_uid;
		
		return(NULL);
	}
	
	return("unknown username");
}

/*************************************************************************/
/** parse_str(): Converts string to string (that's magic :-).				**/
/*************************************************************************/

static char *parse_str(char *cmd, char *arg, void *ptr, int min, int max)
{
	xstrncpy((char *)ptr, arg, max);

	return(NULL);
}

/*************************************************************************/
/** parse_msk(): Converts string into umask (octal).							**/
/*************************************************************************/

static char *parse_msk(char *cmd, char *arg, void *ptr, int min, int max)
{
	mode_t	mask;
	char	  *stop;

	mask = (mode_t)strtol(arg, &stop, 8);

	if (*stop == '\0')
	{
		(*(mode_t *)ptr) = mask;
		
		return(NULL);
	}

	return("unknown umask");
}

/*************************************************************************/
/** parse_log(): Converts the loglevels.           						   **/
/*************************************************************************/

static char *parse_log(char *cmd, char *arg, void *ptr, int min, int max)
{
	int i;

	useloglevel = L_STDERR;

	for (i = 0; i < strlen(arg); i++)
	{
		switch (arg[i])
		{
			case 'F':
			case 'f':
				useloglevel |= L_FATAL;
				break;

			case 'E':
			case 'e':
				useloglevel |= L_ERROR;
				break;

			case 'W':
			case 'w':
				useloglevel |= L_WARN;
				break;

			case 'I':
			case 'i':
				useloglevel |= L_INFO;
				break;

			case 'D':
			case 'd':
				useloglevel |= L_DEBUG;
				break;

			case 'J':
			case 'j':
				useloglevel |= L_JUNK;
				break;
		}
	}

	log_debuglevel(useloglevel);

	return(NULL);
}
