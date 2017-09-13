/*
** $Id: modem.c,v 1.7 1998/11/10 18:36:28 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termio.h>
#include <signal.h>

#include "log.h"
#include "modem.h"
#include "libvboxmodem.h"
#include "stringutils.h"

/** Variables ************************************************************/

static unsigned char lastmodemresult[VBOXMODEM_BUFFER_SIZE + 1];

static int timeoutstatus = 0;

/** Structures ***********************************************************/

struct modemsetup modemsetup =
{
	4,															  /* Echo timeout (sec)	*/
	4,														  /* Command timeout (sec)	*/
	6,															  /* Ring timeout (sec)	*/
	1800,														 /* Alive timeout (sec)	*/
	400															  /* Toggle DTR (ms)	*/
};

/** Prototypes ***********************************************************/

static void	 modem_timeout_function(int);
static int	 modem_write(struct vboxmodem *, unsigned char *);
static int	 modem_get_echo(struct vboxmodem *, unsigned char *);
static int	 modem_get_rawsequence(struct vboxmodem *, unsigned char *, int);
static int	 modem_check_result(unsigned char *, unsigned char *);

/*************************************************************************/
/** modem_set_timeout():	Setzt den Timeout für die Modemfunctionen.	**/
/*************************************************************************/
/** => timeout					Timeout in Sekunden. Bei Angabe von 0 wird	**/
/**								ein bestehender Timeout gelöscht.				**/
/*************************************************************************/

void modem_set_timeout(int timeout)
{
	if (timeout != 0)
	{
		timeoutstatus = 0;

		signal(SIGALRM, modem_timeout_function);

#ifdef HAVE_SIGINTERRUPT 
		siginterrupt(SIGALRM, 1);
#endif

		alarm(timeout);
	}
	else
	{
		signal(SIGALRM, SIG_IGN);
		alarm(0);
	}
}

/*************************************************************************/
/** modem_get_timeout():	Gibt zurück ob ein Timeout aufgetreten ist.	**/
/*************************************************************************/
/** <=							1 wenn ein Timeout aufgetreten ist oder 0 	**/
/**								wenn nicht.												**/
/*************************************************************************/

int modem_get_timeout(void)
{
	return(timeoutstatus);
}

/*************************************************************************/
/** modem_get_sequence():	Liest einen Text vom Modem.						**/
/*************************************************************************/
/** => seq						Text der gelesen werden soll.						**/
/**																							**/
/** <=							0 wenn der Text gelesen werden konnte oder	**/
/**								-1 wenn nicht.											**/
/*************************************************************************/

int modem_get_sequence(struct vboxmodem *vbm, unsigned char *seq)
{
	return(modem_get_rawsequence(vbm, seq, 0));
}

/*************************************************************************/
/** modem_flush():	Leert die Modem-Eingabe/Ausgabe.							**/
/*************************************************************************/
/** => vbm				Zeiger auf die Modem-Struktur.							**/
/** => timeout			Timeout in Sekunden.											**/
/*************************************************************************/

void modem_flush(struct vboxmodem *vbm, int timeout)
{
	TIO	porttio;
	TIO	savetio;
	long	gotjunk = 0;
	char	onebyte = 0;

	log_line(LOG_D, "Flushing modem%s...\n", (timeout ? " (with timeout)" : ""));

	if (vboxmodem_get_termio(vbm, &porttio) == 0)
	{
		savetio = porttio;

		porttio.c_lflag		&= ~ICANON;
		porttio.c_cc[VMIN]	 = 0;
		porttio.c_cc[VTIME]	 = timeout;

		if (vboxmodem_set_termio(vbm, &porttio) == 0)
		{
			while (vboxmodem_raw_read(vbm, &onebyte, 1) == 1)
			{
				if (gotjunk++ < 20)
				{
					log_line(LOG_D, "Junk: ");
					log_char(LOG_D, onebyte);
					log_text(LOG_D, "\n");
				}
			}

			if (gotjunk > 20)
			{
				log_line(LOG_D, "Flush has junked %d byte(s)...\n", gotjunk);
			}

			vboxmodem_set_termio(vbm, &savetio);
		}
	}

	tcflush(vbm->fd, TCIOFLUSH);
	tcflush(vbm->fd, TCIOFLUSH);
}

/*************************************************************************/
/** modem_hangup():	Wechselt die DTR Leitung um das Modem aufzulegen.	**/
/*************************************************************************/
/** => vbm				Zeiger auf die Modem-Struktur.							**/
/**																							**/
/** <=					0 wenn das Modem aufgelegt werden konnte oder -1	**/
/**						wenn nicht.														**/
/*************************************************************************/

int modem_hangup(struct vboxmodem *vbm)
{
	TIO porttio;
	TIO savetio;

	log_line(LOG_D, "Hangup modem (drop dtr %d ms)...\n", modemsetup.toggle_dtr_time);

	modem_flush(vbm, 1);

	if (vboxmodem_get_termio(vbm, &porttio) == -1) return(-1);

	savetio = porttio;
	
	cfsetospeed(&porttio, B0);
	cfsetispeed(&porttio, B0);

	vboxmodem_set_termio(vbm, &porttio);

	usleep(modemsetup.toggle_dtr_time * 1000);
	      
	return(vboxmodem_set_termio(vbm, &savetio));
}

/*************************************************************************/
/** modem_command():	Sendet ein Kommando zum Modem und wartet auf eine	**/
/**						Rückantwort.													**/
/*************************************************************************/
/** => vbm				Zeiger auf die Modem-Struktur.							**/
/** => command			Kommando das gesendet werden soll.						**/
/** => result			Rückantwort auf die gewartet werden soll. Mehrere	**/
/**						Rückantworten können mit '|' getrennt angegeben		**/
/**						werden.															**/
/**																							**/
/** <=					-1 bei einem Fehler, 0 wenn keine der Rückantwort-	**/
/**						en gefunden wurde oder die Nummer der Rückantwort.	**/
/*************************************************************************/

int modem_command(struct vboxmodem *vbm, unsigned char *command, unsigned char *result)
{
	unsigned char line[VBOXMODEM_BUFFER_SIZE + 1];
	int           back;

	lastmodemresult[0] = '\0';

	if ((command) && (*command))
	{
		modem_flush(vbm, 0);

		log_line(LOG_D, "Sending \"%s\"...\n", command);

		if (strcmp(command, "\r") != 0)
		{
			if ((modem_write(vbm, command) == -1) || (modem_write(vbm, "\r") == -1))
			{
				log_line(LOG_E, "Can't send modem command.\n");

				modem_flush(vbm, 1);

				return(-1);
			}

			if (modem_get_echo(vbm, command) == -1)
			{
				log_line(LOG_E, "Can't read modem command echo.\n");

				modem_flush(vbm, 1);

				return(-1);
			}
		}
		else
		{
			if (vboxmodem_raw_write(vbm, command, strlen(command)) == -1)
			{
				log_line(LOG_E, "Can't send modem command.\n");

				modem_flush(vbm, 1);
				
				return(-1);
			}
		}
	}

	if ((result) && (*result))
	{
		if (modem_read(vbm, line, modemsetup.commandtimeout) == -1)
		{
			if ((command) && (*command))
			{
				log_line(LOG_E, "Can't read modem command result.\n");
			}

			modem_flush(vbm, 1);
			
			return(-1);
		}

		strcpy(lastmodemresult, line);

		if (strcmp(result, "?") == 0) return(0);

		if ((back = modem_check_result(line, result)) < 1)
		{
			log_line(LOG_E, "Modem returns unneeded command \"");
			log_code(LOG_E, line);
			log_text(LOG_E, "\".\n");

			modem_flush(vbm, 1);
			
			return(-1);
		}
		else return(back);
	}

	return(0);
}

/*************************************************************************/
/** modem_read():		Liest einen terminierten String vom Modem.			**/
/*************************************************************************/
/** => vbm				Zeiger auf die Modem-Struktur.							**/
/** => line				Speicherbereich in den der String gelesen werden 	**/
/**						soll.																**/
/** => readtimeout	Timeout in Sekunden.											**/
/**																							**/
/** <=					0 wenn der String gelesen werden konnte oder -1		**/
/**						wenn nicht.														**/
/*************************************************************************/

int modem_read(struct vboxmodem *vbm, unsigned char *line, int readtimeout)
{
	unsigned char	c;
	int	         r;
	int	         timeout;
	int	         linelen;
	int	         havetxt;

	log_line(LOG_D, "Reading modem answer (%ds timeout)...\n", readtimeout);

	linelen = 0;
	havetxt = 0;

	modem_set_timeout(readtimeout);

	while (((r = vboxmodem_raw_read(vbm, &c, 1)) == 1) && (linelen < (VBOXMODEM_BUFFER_SIZE - 1)))
	{
		if (c >= 32) havetxt = 1;

		if (havetxt)
		{
			if (c == '\n') break;
			
			if ((c != '\r') && (c != '\n'))
			{
				*line++ = c;
				
				linelen++;
			}
		}

		if (modem_get_timeout()) break;
	}

	timeout = modem_get_timeout();
	
	modem_set_timeout(0);

	*line = 0;

	if ((r != 1) || (timeout) || (linelen >= (VBOXMODEM_BUFFER_SIZE - 1)))
	{
		log_line(LOG_W, "Can't read from modem [%d]%s.\n", r, (timeout ? " (timeout)" : ""));

		return(-1);
	}

	return(0);
}

/*************************************************************************/
/** modem_wait():	Wartet auf einen eingehenden Anruf.							**/
/*************************************************************************/
/** => vbm			Zeiger auf die Modem-Struktur.								**/
/**																							**/
/** <=				0 wenn ein eingehender Anruf anliegt oder -1 bei		**/
/**					Timeout/Fehler.													**/
/*************************************************************************/

int modem_wait(struct vboxmodem *vbm)
{
	struct timeval  timeout;
	struct timeval *usetimeout;
	fd_set	       fd;
	int		       back;

	log_line(LOG_A, "Waiting...\n");
	            
	FD_ZERO(&fd);
	FD_SET(vbm->fd, &fd);
      
	if (modemsetup.alivetimeout > 0)
	{
		timeout.tv_sec    = modemsetup.alivetimeout;
		timeout.tv_usec   = modemsetup.alivetimeout * 1000;

		usetimeout = &timeout;
	}
	else usetimeout = NULL;

	back = select(FD_SETSIZE, &fd, NULL, NULL, usetimeout);
   
	if (back <= 0)
	{
		if (back < 0)
		{
			log_line(LOG_E, "Select returns with error (%d)...\n", back);
		}
		else log_line(LOG_D, "Select returns with timeout...\n");

		return(-1);
	}

	return(0);
}

/*************************************************************************/
/** modem_set_nocarrier():	Setzt den NO CARRIER Status.						**/
/*************************************************************************/
/** => vbm						Zeiger auf die Modem-Struktur.					**/
/** => carrier					1 wenn ein Carrier anliegt oder 0.				**/
/*************************************************************************/

void modem_set_nocarrier(struct vboxmodem *vbm, int carrier)
{
	vbm->nocarrier = carrier;
}

/*************************************************************************/
/** modem_get_nocarrier():	Gibt den NO CARRIER Status zurück.				**/
/*************************************************************************/
/** <=							1 wenn ein Carrier anliegt oder 0.				**/
/*************************************************************************/

int modem_get_nocarrier(struct vboxmodem *vbm)
{
	return(vbm->nocarrier);
}

/*************************************************************************/
/** modem_timeout_function():	Funktion die bei einem Modem-Timeout ge-	**/
/**									startet wird.										**/
/*************************************************************************/
/** => s								Signalnummer.										**/
/*************************************************************************/

static void modem_timeout_function(int s)
{
	alarm(0);
	signal(SIGALRM, SIG_IGN);

	log_line(LOG_D, "*** Timeout [%d] ***\n", s);

	timeoutstatus = 1;
}

/*************************************************************************/
/** modem_write():	Sendet einen 0-terminierten String zum Modem.		**/
/*************************************************************************/
/** => vbm				Zeiger auf die Modem-Struktur.							**/
/** => s					String der gesendet werden soll.							**/
/**																							**/
/** <=					0 wenn der String gesendet werden konnte oder -1	**/
/**						bei einem Fehler.												**/
/*************************************************************************/

static int modem_write(struct vboxmodem *vbm, unsigned char *s)
{
	if (vboxmodem_raw_write(vbm, s, strlen(s)) == strlen(s)) return(0);

	return(-1);
}

/*************************************************************************/
/** modem_get_echo():	Liest das Echo eines Modem-Kommandos.				**/
/*************************************************************************/
/** => vbm					Zeiger auf die Modem-Struktur.						**/
/** => echo					Modem-Echo das erwartet wird.							**/
/**																							**/
/** <=						0 wenn das Modem-Echo gelesen werden konnte		**/
/**							oder -1 bei einem Fehler.								**/
/*************************************************************************/

static int modem_get_echo(struct vboxmodem *vbm, unsigned char *echo)
{
	return(modem_get_rawsequence(vbm, echo, 1));
}

/*************************************************************************/
/** modem_get_rawsequence():															**/
/*************************************************************************/

static int modem_get_rawsequence(struct vboxmodem *vbm, unsigned char *line, int echo)
{
	unsigned char	c;
	int	         i;
	int	         timeout;
	
	timeout = (echo ? modemsetup.echotimeout : modemsetup.commandtimeout);

	log_line(LOG_D, "Reading modem %s (%ds timeout)...\n", (echo ? "echo" : "sequence"), timeout);

	modem_set_timeout(timeout);

	for (i = 0; i < strlen(line); i++)
	{
		if ((vboxmodem_raw_read(vbm, &c, 1) != 1) || (modem_get_timeout()))
		{
			modem_set_timeout(0);

			return(-1);
		}

		if (line[i] != c)
		{
			modem_set_timeout(0);

			return(-1);
		}
	}

	if (echo)
	{
		if ((vboxmodem_raw_read(vbm, &c, 1) != 1) || (modem_get_timeout()))
		{
			modem_set_timeout(0);

			return(-1);
		}
	}

	modem_set_timeout(0);

	if (echo)
	{
		if (c != '\r') return(-1);
	}
	
	return(0);
}

/*************************************************************************/
/** modem_check_result():																**/
/*************************************************************************/

static int modem_check_result(unsigned char *have, unsigned char *need)
{
	unsigned char	line[VBOXMODEM_BUFFER_SIZE + 1];
	unsigned char *word;
	unsigned char *more;
	int	         nr;

	log_line(LOG_D, "Waiting for \"");
	log_code(LOG_D, need);
	log_text(LOG_D, "\"... ");

	xstrncpy(line, need, VBOXMODEM_BUFFER_SIZE);

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
				log_text(LOG_D, "Got \"");
				log_code(LOG_D, word);
				log_text(LOG_D, "\" (%d).\n", nr);
			}
			else log_text(LOG_D, "Got it.\n");
			
			return(nr);
		}
	
		word = strtok(NULL, "|");
	}

	log_text(LOG_D, "Oops!\n");

	return(0);
}
