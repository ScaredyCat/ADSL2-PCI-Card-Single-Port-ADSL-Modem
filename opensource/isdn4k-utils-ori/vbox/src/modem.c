/*
** $Id: modem.c,v 1.13 1998/06/10 14:31:08 michael Exp $
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

#include <termio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "modem.h"
#include "log.h"
#include "voice.h"
#include "libvbox.h"

/** Defines **************************************************************/

#define USE_NEW_MODEM_READER	  /* Define to use new modem read command	*/
#undef  DBG_NEW_MODEM_READER	/* Define to debug new modem read command	*/
#undef  DBG_OLD_MODEM_READER	/* Define to debug old modem read command	*/

/** Variables ************************************************************/

static int   timeoutstatus		= FALSE;
static int   nocarrier			= FALSE;
static int   nocarrierpos		= 0;
static char *nocarriertxt		= "NO CARRIER";

static char	 modem_store_result[MODEM_BUFFER_LEN + 1];

#ifdef USE_NEW_MODEM_READER

static int	 modem_input_pos	= 0;
static int	 modem_input_len	= 0;

static char	 modem_input[MODEM_INPUT_LEN + 1];

#endif 

/** Prototypes ***********************************************************/

static void		modem_set_speed(TIO *);
static void		modem_set_sane_mode(TIO *, int);
static void		modem_set_raw_mode(TIO *);
static int		modem_write(char *);
static int		modem_read(char *, int);
static void		modem_timeout_function(int);
static void		modem_check_nocarrier(char);
static int		modem_check_result(char *, char *);
static void		modem_set_flowcontrol(TIO *);
static int		modem_get_echo(char *);
static int		modem_get_rawsequence(char *, int);

/*************************************************************************/
/** modem_open_port(): Opens the modem port.									   **/
/*************************************************************************/

int modem_open_port(void)
{
	log(L_DEBUG, "Opening modem port \"%s\"...\n", setup.modem.device);

	if (setup.modem.fd == -1)
	{
		if ((setup.modem.fd = open(setup.modem.device, O_RDWR|O_NDELAY)) == -1)
		{
			log(L_FATAL, "Can't open modem port \"%s\".\n", setup.modem.device);

			returnerror();
		}

		if (fcntl(setup.modem.fd, F_SETFL, O_RDWR) == -1)
		{
			log(L_FATAL, "Can't call fcntl() to setup modem port.\n");
			
			returnerror();
		}
	}
	else log(L_WARN, "Found open modem port (%d).\n", setup.modem.fd);

	returnok();
}

/*************************************************************************/
/** modem_set_speed(): Sets the modemport speed to 57600.					**/
/*************************************************************************/

static void modem_set_speed(TIO *modemtio)
{
	log(L_JUNK, "Setting modem speed to 57600...\n");

	cfsetospeed(modemtio, B57600);
	cfsetispeed(modemtio, B57600);
}

/*************************************************************************/
/** modem_set_termio():	Sets the modem terminal IO.							**/
/*************************************************************************/

int modem_set_termio(TIO *modemtio)
{
	if (tcsetattr(setup.modem.fd, TCSANOW, modemtio) >= 0) returnok();

	returnerror();
}

/*************************************************************************/
/** modem_get_termio():	Gets the modem terminal IO.							**/
/*************************************************************************/

int modem_get_termio(TIO *modemtio)
{
	if (tcgetattr(setup.modem.fd, modemtio) >= 0) returnok();
	
	returnerror();
}

/*************************************************************************/
/** modem_set_flowcontrol(): Sets modem flowcontrol to hardware hand-	**/
/**								  shake.												   **/
/*************************************************************************/

static void modem_set_flowcontrol(TIO *modemtio)
{
	log(L_JUNK, "Setting modem flow control (HARD)...\n");

	modemtio->c_cflag &= ~(CRTSCTS);
	modemtio->c_iflag &= ~(IXON|IXOFF|IXANY);
	modemtio->c_cflag |=  (CRTSCTS);
}
  
/*************************************************************************/
/** modem_hangup(): Toggles the data terminal ready line to hangup the	**/
/**					  modem.															   **/
/*************************************************************************/

int modem_hangup(void)
{
	TIO porttio;
	TIO savetio;

	log(L_DEBUG, "Hangup modem (drop dtr %d ms)...\n", setup.modem.toggle_dtr_time);

	modem_flush(1);

	if (!modem_get_termio(&porttio)) returnerror();

	savetio = porttio;
	
	cfsetospeed(&porttio, B0);
	cfsetispeed(&porttio, B0);

	modem_set_termio(&porttio);
	xpause(setup.modem.toggle_dtr_time);
	      
	return(modem_set_termio(&savetio));
}

/*************************************************************************/
/** modem_set_sane_mode():	Sets modem terminal IO to sane mode.			**/
/*************************************************************************/

static void	modem_set_sane_mode(TIO *modemtio, int local)
{
	modemtio->c_iflag  =  (BRKINT|IGNPAR|IXON|IXANY);
	modemtio->c_oflag  =  (OPOST|TAB3);
	modemtio->c_cflag &= ~(CSIZE|CSTOPB|PARENB|PARODD|CLOCAL);
	modemtio->c_cflag |=  (CS8|CREAD|HUPCL|(local ? CLOCAL : 0));
	modemtio->c_lflag  =  (ECHOK|ECHOE|ECHO|ISIG|ICANON);
}

/*************************************************************************/
/** modem_set_raw_mode(): Sets modem terminal IO to raw mode.				**/
/*************************************************************************/

static void modem_set_raw_mode(TIO *modemtio)
{
	modemtio->c_iflag		 &= (IXON|IXOFF|IXANY);
	modemtio->c_oflag		  = 0;
	modemtio->c_lflag		  = 0;
	modemtio->c_cc[VMIN]   = 1;
	modemtio->c_cc[VTIME]  = 0;
}

/*************************************************************************/
/** modem_initialize():	Initialize the modem.									**/
/*************************************************************************/

int modem_initialize(void)
{
	TIO porttio;

	log(L_INFO, "Initializing modem port (voice mode; %d ms)...\n", setup.modem.initpause);

	xpause(setup.modem.initpause);

	if (!modem_hangup()) log(L_WARN, "Can't hangup modem!\n");

	if (!modem_get_termio(&porttio))
	{
		log(L_ERROR, "Can't get modem terminal IO settings (not initialized).\n");
		
		returnerror();
	}

	modem_set_sane_mode(&porttio, TRUE);
	modem_set_speed(&porttio);
	modem_set_raw_mode(&porttio);
	modem_set_flowcontrol(&porttio);

	if (!modem_set_termio(&porttio))
	{
		log(L_ERROR, "Can't set modem terminal IO settings (not initialized).\n");
		
		returnerror();
	}

	if (modem_command(setup.modem.init, "OK") <= 0) returnerror();

	if (modem_command(setup.modem.interninita, "OK|VCON") <= 0) returnerror();
	if (modem_command(setup.modem.interninitb, "OK|VCON") <= 0) returnerror();

	returnok();
}

/*************************************************************************/
/** modem_close_port():	Close modem port.											**/
/*************************************************************************/

void modem_close_port(void)
{
	if (!modem_hangup()) log(L_WARN, "Can't hangup modem!\n");

	log(L_DEBUG, "Closing modem port (%d)...\n", setup.modem.fd);

	close(setup.modem.fd);

	setup.modem.fd = -1;
}

/*************************************************************************/
/** modem_get_last_result(): Returns the last modem result.				   **/
/*************************************************************************/

char *modem_get_last_result(void)
{
	return(modem_store_result);
}

/*************************************************************************/
/** modem_command():	Sends a command to the modem and waits for one or	**/
/**						more results.													**/
/*************************************************************************/

int modem_command(char *command, char *result)
{
	char	line[MODEM_BUFFER_LEN + 1];
	char	commandsuffix[2];
	int	back;

	*modem_store_result = '\0';

	commandsuffix[0] = MODEM_COMMAND_SUFFIX;
	commandsuffix[1] = 0;

	if ((command) && (*command))
	{
		modem_flush(0);

		log_line(L_DEBUG, "Sending \"");
		log_code(L_DEBUG, command);
		log_text(L_DEBUG, "\"...\n");

		if (strcmp(command, commandsuffix) != 0)
		{
			if ((!modem_write(command)) || (!modem_write(commandsuffix)))
			{
				log(L_ERROR, "Can't send modem command.\n");

				modem_flush(1);

				returnerror();
			}

			if (!modem_get_echo(command))
			{
				log(L_ERROR, "Can't read modem command echo.\n");

				modem_flush(1);

				returnerror();
			}
		}
		else
		{
			if (!modem_write(command))
			{
				log(L_ERROR, "Can't send modem command.\n");

				modem_flush(1);
				
				returnerror();
			}
		}
	}

	if ((result) && (*result))
	{
		if (!modem_read(line, setup.modem.timeout_cmd))
		{
			if ((command) && (*command))
			{
				log(L_ERROR, "Can't read modem command result.\n");
			}

			modem_flush(1);
			
			returnerror();
		}

		strcpy(modem_store_result, line);

		if (strcmp(result, "?") == 0) returnok();

		if ((back = modem_check_result(line, result)) < 1)
		{
			log_line(L_ERROR, "Modem returns unneeded command \"");
			log_code(L_ERROR, line);
			log_text(L_ERROR, "\".\n");

			modem_flush(1);
			
			returnerror();
		}
		else return(back);
	}

	returnok();
}

/*************************************************************************/
/** modem_get_s_register(): Returns a s-register value.					   **/
/*************************************************************************/

char *modem_get_s_register(int r)
{
	char temp[4];
	char command[10];

	if ((r >= MODEM_MIN_S_REGISTER) && (r <= MODEM_MAX_S_REGISTER))
	{
		sprintf(command, "ATS%d?", r);	

		if (modem_command(command, "?") == 1)
		{
			log(L_DEBUG, "Got s-register value \"%s\".\n", modem_get_last_result());

			modem_raw_read(temp, 4);

			return(modem_get_last_result());
		}
	}

	return("ERROR");
}

/*************************************************************************/
/** modem_check_result(): Checks for a string in the modem result.		**/
/*************************************************************************/

static int modem_check_result(char *have, char *need)
{
	char	line[MODEM_BUFFER_LEN + 1];
	char *word;
	char *more;
	int	nr;

	log_line(L_DEBUG, "Waiting for \"");
	log_code(L_DEBUG, need);
	log_text(L_DEBUG, "\"... ");

	xstrncpy(line, need, MODEM_BUFFER_LEN);

	more	= strchr(line, '|');
	word	= strtok(line, "|");
	nr		= 0;

	while (word)
	{
		nr++;

		if (strncmp(have, word, strlen(word)) == 0)
		{
			if (more)
			{
				log_text(L_DEBUG, "Got \"");
				log_code(L_DEBUG, word);
				log_text(L_DEBUG, "\" (%d).\n", nr);
			}
			else log_text(L_DEBUG, "Got it.\n");
			
			return(nr);
		}
	
		word = strtok(NULL, "|");
	}

	log_text(L_DEBUG, "Oops!\n");

	return(0);
}

/*************************************************************************/
/** modem_write(): Sends a null terminated string to the modem.			**/
/*************************************************************************/

static int modem_write(char *s)
{
	if (modem_raw_write(s, strlen(s)) == strlen(s)) returnok();

	returnerror();
}

/*************************************************************************/
/** modem_raw_write(): Sends a string to the modem.							**/
/*************************************************************************/

size_t modem_raw_write(char *string, int len)
{
	return(write(setup.modem.fd, string, len));
}

/*************************************************************************/
/** modem_get_echo(): Reads modem echo.											**/
/*************************************************************************/

static int modem_get_echo(char *echo)
{
	return(modem_get_rawsequence(echo, TRUE));
}

/*************************************************************************/
/** modem_get_sequence(): Reads a specified sequence from the modem.	   **/
/*************************************************************************/

int modem_get_sequence(char *seq)
{
	return(modem_get_rawsequence(seq, FALSE));
}

/*************************************************************************/
/** modem_get_rawsequence():	Reads a raw sequence from modem. This is**/
/**				a subroutine for modem_get_sequence() &	**/
/**				modem_get_echo().			**/
/*************************************************************************/

static int modem_get_rawsequence(char *line, int echo)
{
	char	c;
	int	i;
	int	timeout;
	
	timeout = (echo ? setup.modem.timeout_echo : setup.modem.timeout_cmd);

	log(L_JUNK, "Reading modem %s (%d secs timeout)...\n", (echo ? "echo" : "sequence"), timeout);

	modem_set_timeout(timeout);

	for (i = 0; i < strlen(line); i++)
	{
		if ((modem_raw_read(&c, 1) != 1) || (modem_get_timeout()))
		{
			modem_set_timeout(0);

			returnerror();
		}

		if (line[i] != c)
		{
			modem_set_timeout(0);

			returnerror();
		}
	}

	if (echo)
	{
		if ((modem_raw_read(&c, 1) != 1) || (modem_get_timeout()))
		{
			modem_set_timeout(0);

			returnerror();
		}
	}

	modem_set_timeout(0);

	if (echo)
	{
		if (c != MODEM_COMMAND_SUFFIX) returnerror();
	}
	
	returnok();
}

/*************************************************************************/
/** modem_wait_sequence():	Reads a sequence from modem and breaks	**/
/**				on timeout or if the sequence was	**/
/**				matched.				**/
/*************************************************************************/

int modem_wait_sequence(char *line)
{
	char	c;
	int	i=0;
	int	timeout;
	
	timeout = setup.modem.timeout_cmd;
	log_line(L_DEBUG, "Waiting (%d secs timeout) for sequence \"", timeout);
	log_code(L_DEBUG, line);
	log_text(L_DEBUG, "\"... ");
	modem_set_timeout(timeout);
	while (1)
	{
		if ((modem_raw_read(&c, 1) != 1) || (modem_get_timeout()))
		{
			modem_set_timeout(0);
			log_text(L_DEBUG, " NOT FOUND\n");
			returnerror();
		}

		if (line[i] == c)
		{
			i++;
			if (i==strlen(line)) {
				modem_set_timeout(0);
				log_text(L_DEBUG, " GOT IT\n");
				returnok();
			}
		} else {
			i=0;
			if (line[i] == c)
				i++;
		}
	}
}

/*************************************************************************/
/** modem_read():	Reads a terminated string from the modem.					**/
/*************************************************************************/

static int modem_read(char *line, int readtimeout)
{
	char	c;
	int	r;
	int	linelen = 0;
	int	havetxt = FALSE;
	int	timeout;

	log(L_JUNK, "Reading modem input (%d secs timeout)...\n", readtimeout);

	modem_set_timeout(readtimeout);

	while (((r = modem_raw_read(&c, 1)) == 1) && (linelen < (MODEM_BUFFER_LEN - 1)))
	{
		if (c >= 32) havetxt = TRUE;

		if (havetxt)
		{
			if (c == '\n') break;
			
			if ((c != '\r') && (c != '\n'))
			{
				*line++ = c;
				
				linelen++;
			}
		}
	}

	timeout = modem_get_timeout();
	
	modem_set_timeout(0);

	*line = 0;

	if ((r != 1) || (timeout) || (linelen >= (MODEM_BUFFER_LEN - 1)) )
	{
		log(L_JUNK, "Can't read from modem [%d]%s.\n", r, (timeout ? " (timeout)" : ""));

		returnerror();
	}

	returnok();
}

/*************************************************************************/
/** modem_raw_read(): Reads a raw string from modem.						   **/
/*************************************************************************/

int modem_raw_read(char *line, int len)
{
#ifdef USE_NEW_MODEM_READER

	int use = 0;
	int i;

#ifdef DBG_NEW_MODEM_READER
	log(L_JUNK, "[READ] Function request %d byte(s) (now pos %d; len %d).\n", len, modem_input_pos, modem_input_len);
#endif

	if (len > MODEM_INPUT_LEN)
	{
		log(L_FATAL, "Internal modem buffer overflow (size %d; request %d)!\n", MODEM_INPUT_LEN, len);
		
		return(-1);
	}

	if (modem_input_len >= len)
	{
		memcpy(line, &modem_input[modem_input_pos], len);

		modem_input_len -= len;
		modem_input_pos += len;

#ifdef DBG_NEW_MODEM_READER
		log(L_JUNK, "[READ] Return all %d bytes (now pos %d; len %d).\n", len, modem_input_pos, modem_input_len);
#endif

		return(len);
	}

	if (modem_input_len > 0)
	{
		memcpy(line, &modem_input[modem_input_pos], modem_input_len);

#ifdef DBG_NEW_MODEM_READER
		log(L_JUNK, "[READ] Store %d of %d bytes (now pos 0; len 0).\n", modem_input_len, len);
#endif
	}
#ifdef DBG_NEW_MODEM_READER
	else log(L_JUNK, "[READ] Store nothing (pos 0; len 0).\n");
#endif
	
	len -= modem_input_len;
	use += modem_input_len;
	
	modem_input_pos = 0;
	modem_input_len = 0;

	if ((modem_input_len = read(setup.modem.fd, modem_input, MODEM_INPUT_LEN)) < 0)
	{
		modem_input_pos = 0;
		modem_input_len = 0;

#ifdef DBG_NEW_MODEM_READER
		log(L_JUNK, "[READ] Return only %d bytes (now pos %d; len %d).\n", use, modem_input_pos, modem_input_len);
#endif

		return(use);
	}

#ifdef DBG_NEW_MODEM_READER
	log(L_JUNK, "[READ] Read %d bytes (now pos %d; len %d).\n", modem_input_len, modem_input_pos, modem_input_len);
#endif

	for (i = 0; i < modem_input_len; i++)
	{
#ifdef DBG_NEW_MODEM_READER
		log_line(L_JUNK, "[READ] ");
		log_char(L_JUNK, modem_input[i]);
		log_text(L_JUNK, "\n");
#endif

		modem_check_nocarrier(modem_input[i]);
	}

	if (modem_input_len < len) len = modem_input_len;

	memcpy(&line[use], &modem_input[modem_input_pos], len);

	modem_input_len -= len;
	modem_input_pos += len;

#ifdef DBG_NEW_MODEM_READER
	log(L_JUNK, "[READ] Return %d bytes (now pos %d; len %d.\n", use + len, modem_input_pos, modem_input_len);
#endif

	return(use + len);

#else

	int r;
	int i;

	if ((r = read(setup.modem.fd, line, len)) > 0)
	{
		for (i = 0; i < r; i++)
		{
#ifdef DBG_OLD_MODEM_READER
			log_line(L_JUNK, "[OLDREAD] ");
			log_char(L_JUNK, line[i]);
			log_text(L_JUNK, "\n");
#endif

			modem_check_nocarrier(line[i]);
		}
	}

	return(r);

#endif
}

/*************************************************************************/
/** modem_timeout_function():															**/
/*************************************************************************/

static void modem_timeout_function(int s)
{
	alarm(0);
	signal(SIGALRM, SIG_IGN);

	log(L_JUNK, "Modem timeout function called...\n");

	timeoutstatus = TRUE;
}

/*************************************************************************/
/** modem_set_timeout(): Sets the timeout for the modem functions.		**/
/*************************************************************************/

void modem_set_timeout(int timeout)
{
	if (timeout != 0)
	{
		timeoutstatus = FALSE;

		signal(SIGALRM, modem_timeout_function);
		siginterrupt(SIGALRM, 1);
		alarm(timeout);
	}
	else
	{
		signal(SIGALRM, SIG_IGN);
		alarm(0);
	}
}

/*************************************************************************/
/** modem_get_timeout(): Returns the timeout status.						   **/
/*************************************************************************/

int modem_get_timeout(void)
{
	return(timeoutstatus);
}

/*************************************************************************/
/** modem_check_nocarrier():	Watchs the modem input for NO CARRIER.		**/
/*************************************************************************/

static void modem_check_nocarrier(char c)
{
	if (c == nocarriertxt[nocarrierpos])
	{
		nocarrierpos++;

		if (nocarrierpos >= strlen(nocarriertxt))
		{
			log(L_JUNK, "*** NO CARRIER ***\n");

			nocarrier		= TRUE;
			nocarrierpos	= 0;
		}
	}
	else
	{
		nocarrierpos = 0;
		
		if (c == nocarriertxt[0]) nocarrierpos++;
	}
}

/*************************************************************************/
/** modem_flush(): Flushs modem input/output.									**/
/*************************************************************************/

void modem_flush(int timeout)
{
	TIO	porttio;
	TIO	savetio;
	long	gotjunk		= 0;
	char	onebyte		= 0;

	log(L_DEBUG, "Flushing modem%s...\n", (timeout ? " (timeout)" : ""));

	if (modem_get_termio(&porttio))
	{
		savetio = porttio;

		porttio.c_lflag		&= ~ICANON;
		porttio.c_cc[VMIN]	 = 0;
		porttio.c_cc[VTIME]	 = timeout;

		if (modem_set_termio(&porttio))
		{
			while (modem_raw_read(&onebyte, 1) == 1)
			{
				if (gotjunk++ < 20)
				{
					log_line(L_JUNK, "Junk: ");
					log_char(L_JUNK, onebyte);
					log_text(L_JUNK, "\n");
				}
			}

			if (gotjunk > 20)
			{
				log(L_DEBUG, "Flush has junked %d bytes...\n", gotjunk);
			}

			modem_set_termio(&savetio);
		}
	}

	tcflush(setup.modem.fd, TCIOFLUSH);
	tcflush(setup.modem.fd, TCIOFLUSH);
}

/*************************************************************************/
/** modem_wait():	Waits for modem activity.										**/
/*************************************************************************/

int modem_wait(void)
{
	struct timeval  timeout;
	struct timeval *usetimeout;

	fd_set	fd;
	int		back;

	log(L_INFO, "Waiting...\n");

	FD_ZERO(&fd);
	FD_SET(setup.modem.fd, &fd);
        	
	if (setup.modem.timeout_alive > 0)
	{
		timeout.tv_sec		= setup.modem.timeout_alive;
		timeout.tv_usec	= setup.modem.timeout_alive * 1000;

		usetimeout = &timeout;
	}
	else usetimeout = NULL;

	back = select(FD_SETSIZE, &fd, NULL, NULL, usetimeout);
	
	if (back <= 0)
	{
		if (back < 0)
		{
			log(L_ERROR, "Select returns with error (%d)...\n", back);
		}
		else log(L_JUNK, "Select returns with timeout...\n");

		returnerror();
	}

	log(L_INFO, "Wakeup!\n");

	returnok();
}

/*************************************************************************/
/** modem_count_rings(): Counts incoming rings.                         **/
/*************************************************************************/

int modem_count_rings(int needrings)
{
	char	line[MODEM_BUFFER_LEN + 1];
	char *sreg;
	int	haverings;
	int	havesregs;
	int	havesetup;
	int	n;

	voice_init_section();
	
	haverings	= 0;
	havesregs	= FALSE;
	havesetup	= FALSE;

	while (modem_read(line, setup.modem.timeout_ring))
	{
		if (ctrl_ishere(setup.spool, CTRL_NAME_STOP))
		{
			log(L_INFO, "Control file \"%s\" exists - killing me now...\n", CTRL_NAME_STOP);
		
			kill(getpid(), SIGTERM);
		}

		if (havesetup)
		{
			if (ctrl_ishere(setup.spool, CTRL_NAME_REJECT))
			{
				log(L_INFO, "Control file \"%s\" exists - rejecting call...\n", CTRL_NAME_REJECT);

				if (!ctrl_remove(setup.spool, CTRL_NAME_REJECT))
				{
					log(L_WARN, "Can't remove control file \"%s\"!\n", CTRL_NAME_REJECT);
				}
		
				returnerror();
			} 

			if (ctrl_ishere(setup.spool, CTRL_NAME_ANSWERNOW))
			{
				log(L_INFO, "Control file \"%s\" exists - answering now...\n", CTRL_NAME_ANSWERNOW);

				if (!ctrl_remove(setup.spool, CTRL_NAME_ANSWERNOW))
				{
					log(L_WARN, "Can't remove control file \"%s\"!\n", CTRL_NAME_ANSWERNOW);
				}
		
				returnok();
			}

			if (ctrl_ishere(setup.spool, CTRL_NAME_ANSWERALL))
			{
				log(L_INFO, "Control file \"%s\" exists - answering now...\n", CTRL_NAME_ANSWERALL);
		
				returnok();
			}

			if (!setup.voice.doanswer)
			{
				log(L_INFO, "Call will not be answered - disabled in \"vboxrc\"...\n");
				
				returnerror();
			}

			if (needrings <= 0)
			{
				log(L_INFO, "Call will not be answered - no rings are set...\n");

				returnerror();
			}
		}

		if ((strncmp(line, "CALLER NUMBER: ", 15) == 0) && (!havesetup))
		{
			if ((sreg = modem_get_s_register(20)))
			{
				if (strcmp(sreg, "1") != 0)
				{
					log(L_INFO, "Incoming call is not voice - hanging up...\n");
				
					returnerror();
				}
			}
			else log(L_WARN, "Can't get incoming call type - I think it's voice...\n");

			voice_init_section();

			xstrncpy(setup.voice.callerid, &line[15], VOICE_MAX_CALLERID);

			voice_user_section(setup.voice.callerid);

			log(L_INFO, "[%2d/%2d] CALLER NUMBER: %s (%s)...\n", haverings, needrings, setup.voice.callerid, setup.voice.name);

			if ((setup.voice.rings >= 0) && (setup.voice.rings != needrings))
			{
				log(L_DEBUG, "New number of rings from \"vboxrc\" are %d...\n", setup.voice.rings);

				needrings = setup.voice.rings;
			}

			if ((setup.voice.ringsonnew >= 0) && (setup.voice.ringsonnew != needrings))
			{
				log(L_DEBUG, "Checking for new messages in \"%s\"...\n", setup.voice.checknewpath);

				if ((n = get_nr_messages(setup.voice.checknewpath, TRUE)) > 0)
				{
 					log(L_DEBUG, "Found %d new messages; new number of rings are %d...\n", n, setup.voice.ringsonnew);

					needrings = setup.voice.ringsonnew;
				}
			}

			havesetup = TRUE;

			continue;
		}

		if ((haverings >= needrings) && (havesetup)) returnok();

		if (strcmp(line, "RING") == 0)
		{
			haverings++;

			log(L_INFO, "[%2d/%2d] RING...\n", haverings, needrings);
		}
		else
		{
			log_line(L_JUNK, "Got junk line \"");
			log_code(L_JUNK, line);
			log_text(L_JUNK, "\"...\n");
		}

		if ((haverings >= needrings) && (havesetup)) returnok();
	}

	returnerror();
}

/*************************************************************************/
/** modem_set_nocarrier_state():	Sets NO CARRIER state.						**/
/*************************************************************************/

void modem_set_nocarrier_state(int state)
{
	nocarrier = state;
}

/*************************************************************************/
/** modem_get_nocarrier_state():	Returns NO CARRIER state.					**/
/*************************************************************************/

int modem_get_nocarrier_state(void)
{
	return(nocarrier);
}

/*************************************************************************/
/** modem_check_input(): Checks for modem input.								**/
/*************************************************************************/

int modem_check_input(void)
{
	struct timeval timeout;

	fd_set	fd;
	int		result;

#ifdef USE_NEW_MODEM_READER
	if (modem_input_len > 0) returnok();
#endif
        
	FD_ZERO(&fd);
	FD_SET(setup.modem.fd, &fd);

	timeout.tv_sec		= 0;
	timeout.tv_usec	= 0;

	result = select(FD_SETSIZE , &fd, NULL, NULL, &timeout);
                
	if (result > 0) returnok();

	returnerror();
}
