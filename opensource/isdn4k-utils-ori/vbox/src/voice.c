/*
** $Id: voice.c,v 1.14 2002/01/31 20:12:16 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include "config.h"

#if TIME_WITH_SYS_TIME
#   include <sys/time.h>
#   include <time.h>
#else
#   if HAVE_SYS_TIME_H
#      include <sys/time.h>
#   else
#      include <time.h>
#   endif
#endif

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <limits.h>
#include <errno.h>

#include "voice.h"
#include "init.h"
#include "script.h"
#include "modem.h"
#include "log.h"
#include "perms.h"
#include "rcvbox.h"
#include "libvbox.h"

/** Variables ************************************************************/

static int	voicestatus		= VOICE_ACTION_OK;
static int	sequencestatus	= ST_NO_INPUT;

char touchtones[TOUCHTONE_BUFFER_LEN + 1];

/** Prototypes ***********************************************************/

static int		voice_set_compression(short);
static void		voice_set_header(vaheader_t *);
static void		voice_handle_touchtone(char);
static void		voice_close_or_unlink(int, char *);
static void		voice_handle_touchtone_dle(int);

/*************************************************************************/
/** voice_init_section(): Set the voice setup defaults.						**/
/*************************************************************************/

void voice_init_section(void)
{
	xstrncpy(setup.voice.standardmsg		, setup.spool					, VOICE_MAX_MESSAGE );
	xstrncat(setup.voice.standardmsg		, "/messages/standard.msg"	, VOICE_MAX_MESSAGE );
	xstrncpy(setup.voice.beepmsg			, setup.spool					, VOICE_MAX_MESSAGE );
	xstrncat(setup.voice.beepmsg			, "/messages/beep.msg"		, VOICE_MAX_MESSAGE );
	xstrncpy(setup.voice.timeoutmsg		, setup.spool					, VOICE_MAX_MESSAGE );
	xstrncat(setup.voice.timeoutmsg		, "/messages/timeout.msg"	, VOICE_MAX_MESSAGE );
	xstrncpy(setup.voice.tclscriptname	, setup.spool					, VOICE_MAX_SCRIPT  );
	xstrncat(setup.voice.tclscriptname	, "/standard.tcl"				, VOICE_MAX_SCRIPT  );
	xstrncpy(setup.voice.checknewpath	, setup.spool					, VOICE_MAX_CHECKNEW);
	xstrncat(setup.voice.checknewpath	, "/incoming"					, VOICE_MAX_CHECKNEW);
	xstrncpy(setup.voice.callerid			, "*** Unknown ***"			, VOICE_MAX_CALLERID);
	xstrncpy(setup.voice.phone				, "*** Unknown ***"			, VOICE_MAX_PHONE	  );
	xstrncpy(setup.voice.name				, "*** Unknown ***"			, VOICE_MAX_NAME	  );
	xstrncpy(setup.voice.section			, "STANDARD"					, VOICE_MAX_SECTION );

	setup.voice.rings			= -1;
	setup.voice.ringsonnew	= -1;
	setup.voice.doanswer		= TRUE;
	setup.voice.dorecord		= TRUE;
	setup.voice.dobeep		= TRUE;
	setup.voice.domessage	= TRUE;
	setup.voice.dotimeout	= TRUE;
	setup.voice.recordtime	= TRUE;
}

/*************************************************************************/
/** voice_user_section(): Loads a user section.								   **/
/*************************************************************************/

void voice_user_section(char *id)
{
	vboxrc_find_user_from_id(id);
	vboxrc_find_user_section(setup.voice.section);
}

/*************************************************************************/
/** voice_set_compression():	Sets voice compression mode.					**/
/*************************************************************************/

static int voice_set_compression(short c)
{
	char command[64];

	if ((c >= 2) && (c <= 6) && (c != 5))
	{
		printstring(command, "AT+VSM=%d+VLS=2", c);

		log(L_DEBUG, "Setting voice compression \"%s\"...\n", compressions[c]);

		return(modem_command(command, "OK|VCON"));
	}

	log(L_FATAL, "Unknown compression %d - can't set.\n", c);
	
	returnerror();
}

/*************************************************************************/
/** voice_close_or_unlink(): Close and/or unlink a file.                **/
/*************************************************************************/

static void voice_close_or_unlink(int fd, char *name)
{
	if (fd != -1) close(fd);
	
	if (name) unlink(name);
	if (name) unlink(name);
}

/*************************************************************************
 ** voice_put_message(): Plays voice message.									**
 *************************************************************************/

int voice_put_message(char *message)
{
	vaheader_t header;
	long int   compression;
	char	     line_i[MODEM_BUFFER_LEN + 1];
	char	     line_o[MODEM_BUFFER_LEN + MODEM_BUFFER_LEN + 1];
	int	     fd;
	int	     i;
	int	     byte_i;
	int	     byte_o;
	int	     written;
	int	     havedle;
	time_t     timebeg;
	time_t     timeend;
	int	     bytetotal;
	int	     secstotal;

	log(L_INFO, "Playing \"%s\"...\n", message);

	if ((fd = open(message, O_RDONLY)) == -1)
	{
		log(L_ERROR, "Can't open \"%s\".\n", message);

		return(VOICE_ACTION_ERROR);
	}

	if (!header_get(fd, &header))
	{
		log(L_ERROR, "Can't read vbox audio header from message.\n");

		voice_close_or_unlink(fd, NULL);

		return(VOICE_ACTION_ERROR);
	}

	compression = ntohl(header.compression);

	if (!voice_set_compression(compression))
	{
		log(L_ERROR, "Can't set voice audio compression or line mode.\n");
		
		voice_close_or_unlink(fd, NULL);

		return(VOICE_ACTION_ERROR);
	}

	if (modem_get_nocarrier_state())
	{
		voice_close_or_unlink(fd, NULL);

		return(VOICE_ACTION_REMOTEHANGUP);
	}

	if (modem_command("AT+VTX", "CONNECT") == 0)
	{
		log(L_ERROR, "Can't start voice play mode.\n");
		
		voice_close_or_unlink(fd, NULL);

		return(VOICE_ACTION_ERROR);
	}

	voicestatus		= VOICE_ACTION_OK;
	sequencestatus	= ST_NO_INPUT;
	havedle			= FALSE;
	bytetotal		= 0;

	timebeg = time(NULL);

	while (voicestatus == VOICE_ACTION_OK)
	{
		if ((byte_i = read(fd, line_i, MODEM_BUFFER_LEN)) <= 0)
		{
			if (byte_i == 0)
                            log(L_DEBUG, "End of audio data\n");
                        else
                            log(L_DEBUG, "End of audio data with error (%s)\n", strerror(errno));

			break;
		}

		byte_o = 0;

		for (i = 0; i < byte_i; i++)
		{
			line_o[byte_o] = line_i[i];

			if (line_o[byte_o++] == DLE) line_o[byte_o++] = DLE;
		}

		bytetotal += byte_o;

		log(L_JUNK, "Play: <DATA %d incoming; %d outgoing>\n", byte_i, byte_o);

		if (!modem_get_nocarrier_state())
		{
			written	= 0;
			errno		= 0;

			while (written != byte_o)
			{
				written += modem_raw_write(&line_o[written], (byte_o - written));

				if (errno != 0) break;
			}

			if ((written != byte_o) || (errno != 0))
			{
				log(L_ERROR, "Could only write %d of %d bytes (%s).\n", written, byte_o, strerror(errno));

				voicestatus = VOICE_ACTION_ERROR;
			}
		}
		else voicestatus = VOICE_ACTION_REMOTEHANGUP;

		while ((modem_check_input()) && (voicestatus == VOICE_ACTION_OK))
		{
			log(L_JUNK, "Have input...\n");

			if (modem_raw_read(line_i, 1) == 1)
			{
				if (havedle)
				{
					switch (*line_i)
					{
						case ETX:
						case 'b':
						case 'c':
						case 'e':
						case 'd':
						case 'q':
						case 's':
							log_line(L_DEBUG, "Found sequence \"<DLE>");
							log_char(L_DEBUG, *line_i);
							log_text(L_DEBUG, "\" (ignored)...\n");
							break;

						case DC4:
							log(L_DEBUG, "Found sequence \"<DLE><DC4>\" (remote hangup)...\n");
							voicestatus = VOICE_ACTION_REMOTEHANGUP;
							break;
							
						default:
							voice_handle_touchtone_dle(*line_i);
							break;
					}

					havedle = FALSE;
				}
				else
				{
					if (*line_i != DLE)
					{
						log_line(L_DEBUG, "Got unneeded character \"");
						log_char(L_DEBUG, *line_i);
						log_text(L_DEBUG, "\" (need a \"<DLE>\").\n");
					}
					else havedle = TRUE;
				}

				if (voicestatus == VOICE_ACTION_OK)
				{
					if ((index(touchtones, '#')) && (index(touchtones, '*')))
					{
						log(L_DEBUG, "Touchtone sequence \"%s\" found.\n", touchtones);

						if (breaklist_search(touchtones))
						{
							log(L_INFO, "Sequence \"%s\" found in breaklist...\n", touchtones);

							voicestatus = VOICE_ACTION_TOUCHTONES;
						}
						else
						{
							log(L_DEBUG, "Sequence \"%s\" not in breaklist (ignored)...\n", touchtones);

							*touchtones = '\0';
						}
					}
				}
			}
			else log(L_ERROR, "Can't read input from modem.\n");
		}
		if (ctrl_ishere(setup.spool, CTRL_NAME_SUSPEND)) {   
			log(L_INFO, "Control file \"%s\" exists - suspending call...\n", CTRL_NAME_SUSPEND);
			if (!ctrl_remove(setup.spool, CTRL_NAME_SUSPEND)) {
				log(L_WARN, "Can't remove control file \"%s\"!\n", CTRL_NAME_SUSPEND);
			}
			log(L_JUNK, "Sending \"<DLE><ETX>\"...\n");
			printstring(line_o, "%c%c", DLE, ETX);
			modem_raw_write(line_o, strlen(line_o));
			if (modem_command("", "VCON")>0) {
#ifdef VBOX_SUSPEND_VALUE
				printstring(line_o, "AT+S%d", VBOX_SUSPEND_VALUE);
#else
				printstring(line_o, "AT+S");
#endif
				if (modem_command(line_o, "OK") <= 0) {
					log(L_WARN, "Can't suspend call\n");
				} else {
					log(L_INFO, "Call suspended\n");
					voicestatus = VOICE_ACTION_REMOTEHANGUP;
				}
			}
		}
	}

	timeend = time(NULL);

	if (timeend >= timebeg)
	{
		secstotal = (timeend - timebeg);
		bytetotal = get_message_ptime(compression, bytetotal);

		log(L_JUNK, "Function play %d secs (kernel needs %d secs)...\n", secstotal, bytetotal);

		if (secstotal < bytetotal)
		{
			log(L_JUNK, "Waiting %d secs to complete playing...\n", (bytetotal - secstotal));

			xpause((bytetotal - secstotal) * 1000);
		}
	}
	else log(L_WARN, "Oops - can't calculate time to wait!\n");

	voice_close_or_unlink(fd, NULL);

	if ((voicestatus == VOICE_ACTION_REMOTEHANGUP) || (modem_get_nocarrier_state()))
	{
			/*
			 * Remote hangup: We have got the sequence <DLE><DC4> in the
			 * modem stream...
			 */

		modem_command("", "NO CARRIER");
	}
	else
	{
			/*
			 * Local hangup: Send <DLE><ETX> to the modem and wait for the
			 * result VCON...
			 */

		log(L_JUNK, "Sending \"<DLE><ETX>\"...\n");
		printstring(line_o, "%c%c", DLE, ETX);
		modem_raw_write(line_o, strlen(line_o));
		modem_command("", "VCON");
	}

	if (modem_get_nocarrier_state()) voicestatus = VOICE_ACTION_REMOTEHANGUP;

	return(voicestatus);
}

/*************************************************************************/
/** voice_get_message(): Record a message.										**/
/*************************************************************************/
/** If save is not true the recorded data are not written.					**/
/*************************************************************************/

int voice_get_message(char *name, char *timestr, int save)
{
	vaheader_t	header;
	char			line_i[MODEM_BUFFER_LEN + 1];
	char			line_o[MODEM_BUFFER_LEN + 1];
	int			byte_i;
	int			byte_o;
	int			result;
	int			havedle;
	int			savetimeout;
	int			fd;

	savetimeout = xstrtol(timestr, 90);

	if (save)
		log(L_INFO, "Recording \"%s\" (%d secs)...\n", name, savetimeout);
	else
		log(L_INFO, "Waiting %d secs for input...\n", savetimeout);

	if (!voice_set_compression(setup.modem.compression))
	{
		log(L_ERROR, "Can't set voice audio compressen.\n");

		return(VOICE_ACTION_ERROR);
	}

	if (save)
	{
		if ((fd = open(name, O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR|S_IWGRP|S_IWOTH)) == -1)
	   {
	   	log(L_ERROR, "Can't create \"%s\".\n", name);

			return(VOICE_ACTION_ERROR);
		}

		truncate(name, 0);

		voice_set_header(&header);

		if (!header_put(fd, &header))
		{
	      log(L_ERROR, "Can't write vbox audio header.\n");
      
			voice_close_or_unlink(fd, name);

			return(VOICE_ACTION_ERROR);
		}
	}
	else fd = -1;

	if (modem_get_nocarrier_state())
	{
		if (save) voice_close_or_unlink(fd, name);

		return(VOICE_ACTION_LOCALHANGUP);
	}

	if (modem_command("AT+VRX", "CONNECT") == 0)
	{
		log(L_ERROR, "Can't start record mode.\n");

		if (save) voice_close_or_unlink(fd, name);

		return(VOICE_ACTION_ERROR);
	}

	sequencestatus	= ST_NO_INPUT;
	voicestatus		= VOICE_ACTION_OK;
	havedle			= FALSE;

	modem_set_timeout(savetimeout);

	while (voicestatus == VOICE_ACTION_OK)
	{
		byte_o = 0;
		byte_i = 0;
		result = 0;

		while ((byte_o < MODEM_BUFFER_LEN) && (voicestatus == VOICE_ACTION_OK))
		{
			if ((result = modem_raw_read(line_i, 1)) == 1)
			{
				byte_i++;

				if (havedle)
				{
					switch (*line_i)
					{
						case DLE:
							line_o[byte_o++] = DLE;
							break;

						case ETX:
							log(L_DEBUG, "Found sequence \"<DLE><ETX>\" (remote hangup)...\n");
							voicestatus = VOICE_ACTION_REMOTEHANGUP;
							break;

						default:
							voice_handle_touchtone_dle(*line_i);
							break;
					}

					havedle = FALSE;
				}
				else
				{
					if (*line_i != DLE)
					{
						line_o[byte_o++] = *line_i;
					}
					else havedle = TRUE;
				}
			}
			else break;
		}

		if (byte_o > 0)
		{
			if (save)
			{
				log(L_JUNK, "Record: <DATA %d incoming; %d outgoing>\n", byte_i, byte_o);
                         
				write(fd, line_o, byte_o);
			}
			else log(L_JUNK, "Wait: <DATA %d incoming>\n", byte_i);
		}

		if ((result != 1) || (modem_get_timeout()))
		{
			if (!modem_get_timeout())
			{
				log(L_ERROR, "Can't read incoming data (%s).\n", strerror(errno));
				
				voicestatus = VOICE_ACTION_ERROR;
			}
			else voicestatus = VOICE_ACTION_TIMEOUT;
		}

		if ((result == 1) && (ctrl_ishere(setup.spool, CTRL_NAME_SUSPEND))) {   
			log(L_INFO, "Control file \"%s\" exists - suspending call...\n", CTRL_NAME_SUSPEND);
			if (!ctrl_remove(setup.spool, CTRL_NAME_SUSPEND)) {
				log(L_WARN, "Can't remove control file \"%s\"!\n", CTRL_NAME_SUSPEND);
			}
			log(L_JUNK, "Sending \"<DLE><DC4>\"...\n");
			printstring(line_o, "%c%c", DLE, DC4);
			printstring(line_i, "%c%c", DLE, ETX);
			modem_raw_write(line_o, strlen(line_o));
			modem_wait_sequence(line_i);
			if (modem_command("", "VCON")>0) {
#ifdef VBOX_SUSPEND_VALUE
				printstring(line_o, "AT+S%d", VBOX_SUSPEND_VALUE);
#else
				printstring(line_o, "AT+S");
#endif
				if (modem_command(line_o, "OK") <= 0) {
					log(L_WARN, "Can't suspend call\n");
				} else {
					log(L_INFO, "Call suspended\n");
					voicestatus = VOICE_ACTION_REMOTEHANGUP;
				}
			}
		}

		if ((voicestatus == VOICE_ACTION_OK) || (voicestatus == VOICE_ACTION_TIMEOUT))
		{
			if ((index(touchtones, '#')) && (index(touchtones, '*')))
			{
				log(L_DEBUG, "Touchtone sequence \"%s\" found.\n", touchtones);

				if (breaklist_search(touchtones))
				{
					log(L_INFO, "Sequence \"%s\" found in breaklist...\n", touchtones);

					voicestatus = VOICE_ACTION_TOUCHTONES;
				}
				else
				{
					log(L_DEBUG, "Sequence \"%s\" not in breaklist (ignored)...\n", touchtones);

					*touchtones = '\0';
				}
			}
		}
	}

	modem_set_timeout(0);

	if (save)
	{
		voice_close_or_unlink(fd, NULL);

		permissions_set(name, setup.users.uid, setup.users.gid, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH, setup.users.umask);
	}

	if ((voicestatus == VOICE_ACTION_REMOTEHANGUP) || (modem_get_nocarrier_state()))
	{
			/*
			 * Remote hangup: Modem should response with the sequence
			 * NO CARRIER.
			 */

		modem_command("", "NO CARRIER");
	}
	else
	{
			/*
			 * Local hangup: send <DLE><DC4> to the modem and read the
			 * response <DLE><ETX> and VCON.
			 */

		printstring(line_o, "%c%c", DLE, DC4);
		printstring(line_i, "%c%c", DLE, ETX);

		log(L_JUNK, "Sending \"<DLE><DC4>\"...\n");

		modem_raw_write(line_o, strlen(line_o));

		modem_get_sequence(line_i);

		modem_command("", "VCON");
	}

	if (modem_get_nocarrier_state()) voicestatus = VOICE_ACTION_REMOTEHANGUP;

	return(voicestatus);
}

/*************************************************************************/
/** voice_set_header():	Fills the vbox audio header.							**/
/*************************************************************************/

static void voice_set_header(vaheader_t *header)
{
	memset(header, 0, sizeof(vaheader_t));

	xstrncpy(header->magic	 , VAH_MAGIC				, VAH_MAX_MAGIC	);
	xstrncpy(header->name	 , setup.voice.name		, VAH_MAX_NAME		);
	xstrncpy(header->callerid, setup.voice.callerid	, VAH_MAX_CALLERID);
	xstrncpy(header->phone	 , setup.voice.phone		, VAH_MAX_PHONE	);
	xstrncpy(header->location, "*** Unknown ***"		, VAH_MAX_LOCATION);

	header->time			= htonl(time(NULL));
	header->compression	= htonl(setup.modem.compression);
}

/*************************************************************************/
/** voice_handle_touchtone_dle():	Checks a byte for a touchtone DLE	**/
/**											sequence.									**/
/*************************************************************************/

static void voice_handle_touchtone_dle(int byte)
{
	switch (byte)
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '*':
		case '#':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
			voice_handle_touchtone(byte);
			break;

		default:
			log_line(L_ERROR, "Illeagal \"<DLE>\" shielded code \"");
			log_char(L_ERROR, byte);
			log_text(L_ERROR, "\" (ignored)...\n");
			break;
	}
}

/**************************************************************************/
/** voice_handle_touchtone():                                            **/
/**************************************************************************/

static void voice_handle_touchtone(char Tone)
{
	char Temp[2];

	log(L_DEBUG, "Found touchtone \"%c\"...\n", Tone);

	if (Tone == '*')
	{
		printstring(touchtones, "*");
		
		return;
	}

	if (*touchtones != '*')
	{
		if ((Tone != '#') && (Tone != '*'))
		{
			printstring(touchtones, "*%c#", Tone);
		}
		else
		{
			if (Tone == '#') printstring(touchtones, "*#");
		}

		return;
	}

	if (index(touchtones, '#'))
	{
		if ((Tone != '#') && (Tone != '*'))
		{
			printstring(touchtones, "*%c#", Tone);
		}
		else
		{
			if (Tone == '#') printstring(touchtones, "*#");
		}

		return;
	}

	if (strlen(touchtones) < TOUCHTONE_BUFFER_LEN)
	{
		Temp[0] = Tone;
		Temp[1] = 0;
		
		strcat(touchtones, Temp);
	}
}
