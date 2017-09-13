/*
** $Id: voice.c,v 1.11 1998/11/10 18:36:39 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifdef HAVE_CONFIG_H
#	include "../config.h"
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "voice.h"
#include "log.h"
#include "modem.h"
#include "stringutils.h"
#include "vboxrc.h"
#include "vboxgetty.h"
#include "tclscript.h"
#include "control.h"
#include "audio.h"
#include "breaklist.h"

/** Variables ************************************************************/

static struct vboxuser *voicevboxuser;
static struct vboxcall *voicevboxcall;

static int voicedesc = -1;
static int voicevbox =  0;
static int voicestat =  VBOXVOICE_STAT_OK;
static int audiodesc = -1;

static unsigned char voicename_ulaw[PATH_MAX + 1];
static unsigned char voicename_vbox[PATH_MAX + 1];
static unsigned char voicename_call[PATH_MAX + 1];

unsigned char voice_touchtone_sequence[VBOXVOICE_SEQUENCE + 1];

/** Prototypes ***********************************************************/

static void voice_stop_vtxrtx(void);
static void voice_create_vboxcall(void);
static void voice_remove_vboxcall(void);
static void voice_mkdir(unsigned char *);
static int  voice_check_touchtone(int);

/************************************************************************* 
 ** voice_init():	Beantwortet den Anruf und startet das Tcl-Skript.		**
 *************************************************************************
 ** => vboxuser	Zeiger auf die vboxuser-Struktur								**
 ** => vboxcall	Zeiger auf die vboxcall-Struktur								**
 *************************************************************************/

int voice_init(struct vboxuser *vboxuser, struct vboxcall *vboxcall)
{
	unsigned char	tempconnect[12];
	unsigned char	msgsavetime[32];
	time_t	      currenttime;
	char	        *stop;
	int		      rc;

	struct vbox_tcl_variable vars[] = 
	{
		{ "vbxv_savetime"		, msgsavetime				},
		{ "vbxv_callerid"		, vboxuser->incomingid	},
		{ "vbxv_callername"	, vboxcall->name 			},
		{ "vbxv_localphone"	, vboxuser->localphone	},
		{ "vbxv_username"		, vboxuser->name			},
		{ "vbxv_userhome"		, vboxuser->home			},
		{ "vbxv_usedscript"	, vboxcall->script		},
		{ "vbxv_saveulaw"		, voicename_ulaw 			},
		{ "vbxv_savevbox"		, voicename_vbox			},
		{ NULL					, NULL						}
	};

	printstring(msgsavetime, "%d", vboxcall->savetime);

		/* Die beiden übergebenen Strukturen global machen, damit alle	*/
		/* Voice Funktionen sie benutzen können.								*/

	voicevboxuser = vboxuser;
	voicevboxcall = vboxcall;

		/* Die Namen der beiden Dateien (*.ulaw und *.vbox) erzeugen.	*/
		/* Eine Datei enthält die Audiodaten, die andere die Inform-	*/
		/* ationen über den Benutzer.												*/

	currenttime = time(NULL);

	printstring(voicename_ulaw, "%s/new/%11.11lu-%8.8lu.ulaw", vboxuser->home, (unsigned long)currenttime, (unsigned long)getpid());
	printstring(voicename_vbox, "%s/new/%11.11lu-%8.8lu.vbox", vboxuser->home, (unsigned long)currenttime, (unsigned long)getpid());

		/* Den Namen der Call Datei erzeugen, in dem später Daten zum	*/
		/* aktuellen Anruf gesichert werden.									*/

	printstring(voicename_call, "%s/vboxcall-%s", vboxuser->home, savettydname);

		/* Userspool Verzeichnisstruktur erzeugen. Wenn eines der	*/
		/* Verzeichnisse bereits existiert, bleibt es unverändert.	*/

	voice_mkdir(NULL );
	voice_mkdir("new");
	voice_mkdir("msg");
	voice_mkdir("tcl");

		/* Variablen für Tcl erzeugen, Kompression setzen, Full Duplex	*/
		/* Audio Modus starten und das Skript aufrufen.						*/

	log_line(LOG_A, "Answering call...\n");

	voicevbox =  0;
	voicedesc = -1;
	audiodesc = -1;

	*voice_touchtone_sequence = '\0';

	if (scr_init_variables(vars) == 0)
	{
		if (modem_command(&vboxmodem, "ATA", "VCON") > 0)
		{
			log_line(LOG_D, "Setting voice compression to \"ulaw\"...\n");

			if (modem_command(&vboxmodem, "AT+VSM=6+VLS=2", "OK") > 0)
			{
				log_line(LOG_D, "Starting full duplex audio mode...\n");

				if (modem_command(&vboxmodem, "AT+VTX+VRX", "CONNECT") > 0)
				{
					voice_hear(0);
					voice_save(0);

						/* Bevor das Skript gestartet wird, werden die	*/
						/* ersten 11 Byte vom Modem eingelesen. Damit	*/
						/* wird ein Fehler in i4l umgangen, der die		*/
						/* Connectmessage bei Full Duplex 2x in den Mo-	*/
						/* dembuffer schreibt.									*/

					modem_set_timeout(2);
					vboxmodem_raw_read(&vboxmodem, tempconnect, 11);
					modem_set_timeout(0);

					voice_create_vboxcall();

					rc = scr_execute(vboxcall->script, vboxuser);

					voice_remove_vboxcall();

					voice_hear(0);
					voice_save(0);

					voice_stop_vtxrtx();

					if ((stop = ctrl_exists(vboxuser->home, "suspend", savettydname)))
					{
						ctrl_remove(vboxuser->home, "suspend", savettydname);

						if (!vboxmodem.nocarrier)
						{
							log_line(LOG_A, "Suspending call to number %s...\n", stop);

							if (modem_command(&vboxmodem, "AT+S1", "OK") > 0) {
								log_line(LOG_D, "Call suspended to number %s.\n", stop);
							} else {
								log_line(LOG_E, "Can't suspend call to number %s.\n", stop);
							}
						}
					}

					return(rc);
				}
			}
		}
	}

	log_line(LOG_E, "Unable to answer call!\n");

	return(-1);
}

/*************************************************************************
 ** voice_wait():	Liest eine angegebene Zeit lang Audiodaten vom Modem.	**
 *************************************************************************
 ** => timeout		Timeout in Sekunden												**
 *************************************************************************
 ** <=				 0 wenn der Timeout eingetreten ist.						**
 **					 1 wenn eine gültige Touchtonesequenz gefunden wurde.	**
 **					 2 wenn der Anruf suspended werden soll.					**
 **					-1 bei einem Fehler oder Remote hangup						**
 *************************************************************************/

int voice_wait(int timeout)
{
	unsigned char  modem_line_i[1];
	unsigned char  modem_line_o[VBOXVOICE_BUFSIZE + 1];
	int	         total_byte_i;
	int	         total_byte_o;
	int            modem_byte_i;
	int            modem_byte_o;
	int            last_was_dle;
	int            result;
	char          *stop;

	total_byte_i = 0;
	total_byte_o = 0;
	last_was_dle = 0;
	voicestat	 = VBOXVOICE_STAT_OK;

	log(LOG_D, "Reading voice datas (%ds timeout)...\n", timeout);

	modem_set_timeout(timeout);
	
	while (voicestat == VBOXVOICE_STAT_OK)
	{
		modem_byte_i = 0;
		modem_byte_o = 0;
		result       = 0;

		while ((modem_byte_o < (VBOXVOICE_BUFSIZE - 2)) && (voicestat == VBOXVOICE_STAT_OK))
		{
			if ((result = vboxmodem_raw_read(&vboxmodem, modem_line_i, 1)) == 1)
			{
				modem_byte_i++;

				if (last_was_dle)
				{
					switch (*modem_line_i)
					{
						case DLE:
							modem_line_o[modem_byte_o++] = DLE;
							break;

						case ETX:
							voicestat |= VBOXVOICE_STAT_HANGUP;
							break;
							
						default:
							if (voice_check_touchtone(*modem_line_i) == 0) voicestat |= VBOXVOICE_STAT_TOUCHTONE;
							break;
					}
					
					last_was_dle = 0;
				}
				else
				{
					if (*modem_line_i == DLE) last_was_dle = 1;

					modem_line_o[modem_byte_o++] = *modem_line_i;
				}
			}
			else break;
		}

		total_byte_o += modem_byte_o;
		total_byte_i += modem_byte_i;

		if (modem_byte_o > 0)
		{
			if (voicedesc != -1) write(voicedesc, modem_line_o, modem_byte_o);
			if (audiodesc != -1) write(audiodesc, modem_line_o, modem_byte_o);

				/* Einmal in der Sekunde die Logmessage ausgeben und die	*/
				/* Controls checken.                 							*/

			if ((total_byte_o >= VBOXVOICE_SAMPLERATE) || (voicestat != VBOXVOICE_STAT_OK))
			{
				log_line(LOG_D, "Wait: incoming %04d; outgoing %04d...\n", total_byte_i, total_byte_o);

				total_byte_i = 0;
				total_byte_o = 0;

				if ((stop = ctrl_exists(voicevboxuser->home, "suspend", savettydname)))
				{
					log(LOG_D, "Control \"vboxctrl-suspend-%s\" detected: %s.\n", savettydname, stop);

					voicestat |= VBOXVOICE_STAT_SUSPEND;
				}

				if ((stop = ctrl_exists(voicevboxuser->home, "audio", savettydname)))
				{
					log(LOG_D, "Control \"vboxctrl-audio-%s\" detected: %s.\n", savettydname, stop);

					if (strcasecmp(stop,  "stop") == 0) voice_hear(0);
					if (strcasecmp(stop, "start") == 0) voice_hear(1);
				}
			}
		}

		if ((result != 1) || (modem_get_timeout()))
		{
			if (!modem_get_timeout())
			{
				log_line(LOG_W, "Can't read voice data (%s).\n", strerror(errno));

				voicestat |= VBOXVOICE_STAT_TIMEOUT;
			}
			else voicestat |= VBOXVOICE_STAT_TIMEOUT;
		}
	}	

	modem_set_timeout(0);

	result = 0;

	if (voicestat & VBOXVOICE_STAT_TOUCHTONE)
	{
		log_line(LOG_D, "Full touchtone sequence found!\n");

		result = 1;
	}

	if (voicestat & VBOXVOICE_STAT_SUSPEND) result = 2;

	if ((voicestat & VBOXVOICE_STAT_HANGUP) || (vboxmodem.nocarrier))
	{
		log_line(LOG_D, "*** Remote hangup ***\n");

		voice_save(0);
		voice_hear(0);		

		modem_command(&vboxmodem, "", "NO CARRIER");

		result = -1;
	}
	
	return(result);
}

/************************************************************************* 
 ** voice_play():	Spielt eine Nachricht ab und liest dabei Voicedaten	**
 **					vom Modem.															**
 *************************************************************************
 ** name				Name der Nachricht die gespielt werden soll.				**
 *************************************************************************
 ** <=				 0 wenn der Timeout eingetreten ist.						**
 **					 1 wenn eine gültige Touchtonesequenz gefunden wurde.	**
 **					 2 wenn der Anruf suspended werden soll.					**
 **					-1 bei einem Fehler oder Remote hangup						**
 *************************************************************************/

int voice_play(unsigned char *name)
{
	unsigned char  modem_line_i[1];
	unsigned char  modem_line_o[VBOXVOICE_BUFSIZE + 1];
	int				total_byte_i;
	int				total_byte_o;
	int            modem_byte_i;
	int            modem_byte_o;
	int            last_was_dle;
	int            result;
	int	         desc;
	char          *stop;
	int	         i;

	if ((!name) || (!*name))
	{
		log(LOG_W, "No message to play selected (ignored).\n");

		return(0);
	}

	if ((stop = rindex(name, '/'))) name = ++stop;

	log(LOG_D, "Playing \"%s\"...\n", name);

	printstring(temppathname, "%s/msg/%s", voicevboxuser->home, name);

   errno = 0;
	desc  = open(temppathname, O_RDONLY);

	if (desc == -1)
	{
		printstring(temppathname, "%s/msg/%s", PKGDATADIR, name);

      errno = 0;
		desc  = open(temppathname, O_RDONLY);
	}

	if (desc == -1)
	{
		log(LOG_W, "Can't open \"%s\" (%s).\n", name, strerror(errno));

		return(0);
	}

	last_was_dle	= 0;
	total_byte_i	= 0;
	total_byte_o	= 0;
	voicestat		= VBOXVOICE_STAT_OK;

	while (voicestat == VBOXVOICE_STAT_OK)
	{
		modem_byte_i = 0;
		modem_byte_o = 0;
		result       = 0;

		modem_set_timeout(5);

		while ((modem_byte_o < (VBOXVOICE_BUFSIZE - 2)) && (voicestat == VBOXVOICE_STAT_OK))
		{
			if ((i = read(desc, modem_line_i, 1)) != 1)
			{
				if (i != 0) log(LOG_W, "Can't read \"%s\" [%d] (%s).\n", name, i, strerror(errno));

				voicestat |= VBOXVOICE_STAT_DONE;
			}
			else vboxmodem_raw_write(&vboxmodem, modem_line_i, 1);

			if ((result = vboxmodem_raw_read(&vboxmodem, modem_line_i, 1)) == 1)
			{
				modem_byte_i++;

				if (last_was_dle)
				{
					switch (*modem_line_i)
					{
						case DLE:
							modem_line_o[modem_byte_o++] = DLE;
							break;

						case ETX:
							voicestat |= VBOXVOICE_STAT_HANGUP;
							break;
							
						default:
							if (voice_check_touchtone(*modem_line_i) == 0) voicestat |= VBOXVOICE_STAT_TOUCHTONE;
							break;
					}
					
					last_was_dle = 0;
				}
				else
				{
					if (*modem_line_i == DLE) last_was_dle = 1;

					modem_line_o[modem_byte_o++] = *modem_line_i;
				}
			}
			else break;
		}

		modem_set_timeout(0);

		total_byte_o += modem_byte_o;
		total_byte_i += modem_byte_i;

		if (modem_byte_o > 0)
		{
			if (voicedesc != -1) write(voicedesc, modem_line_o, modem_byte_o);
			if (audiodesc != -1) write(audiodesc, modem_line_o, modem_byte_o);

				/* Einmal in der Sekunde die Logmessage ausgeben und die	*/
				/* Controls checken.                 							*/

			if ((total_byte_o >= VBOXVOICE_SAMPLERATE) || (voicestat != VBOXVOICE_STAT_OK))
			{
				log_line(LOG_D, "Play: incoming %04d; outgoing %04d...\n", total_byte_i, total_byte_o);

				total_byte_i = 0;
				total_byte_o = 0;

				if ((stop = ctrl_exists(voicevboxuser->home, "suspend", savettydname)))
				{
					log(LOG_D, "Control \"vboxctrl-suspend-%s\" detected: %s.\n", savettydname, stop);

					voicestat |= VBOXVOICE_STAT_SUSPEND;
				}

				if ((stop = ctrl_exists(voicevboxuser->home, "audio", savettydname)))
				{
					log(LOG_D, "Control \"vboxctrl-audio-%s\" detected: %s.\n", savettydname, stop);

					if (strcasecmp(stop,  "stop") == 0) voice_hear(0);
					if (strcasecmp(stop, "start") == 0) voice_hear(1);
				}
			}
		}

		if ((result != 1) || (modem_get_timeout()))
		{
			if (!modem_get_timeout())
			{
				log_line(LOG_W, "Can't read voice data (%s).\n", strerror(errno));

				voicestat |= VBOXVOICE_STAT_TIMEOUT;
			}
			else voicestat |= VBOXVOICE_STAT_TIMEOUT;
		}
	}	

	modem_set_timeout(0);

	if (desc != -1) close(desc);

	result = 0;

	if (voicestat & VBOXVOICE_STAT_TOUCHTONE)
	{
		log_line(LOG_D, "Full touchtone sequence found!\n");

		result = 1;
	}

	if (voicestat & VBOXVOICE_STAT_SUSPEND) result = 2;

	if ((voicestat & VBOXVOICE_STAT_HANGUP) || (vboxmodem.nocarrier))
	{
		log_line(LOG_D, "*** Remote hangup ***\n");

		voice_save(0);
		voice_hear(0);		

		modem_command(&vboxmodem, "", "NO CARRIER");

		result = -1;
	}
	
	return(result);
}

/************************************************************************* 
 ** voice_save():	Schaltet das mitspeichern der eingehenden Voicedaten	**
 **					ein oder aus.														**
 *************************************************************************
 ** => save			> 0 um das mitspeichern einzuschalten; alle anderen	**
 **					Werte schaltes es aus.											**
 *************************************************************************/

int voice_save(int save)
{
	FILE *vbox;

	if (save > 0)
	{
		if (voicedesc == -1)
		{
			log(LOG_D, "Starting voice recording...\n");
			log(LOG_D, "Opening \"%s\"...\n", voicename_ulaw);

			errno = 0;

			if ((voicedesc = open(voicename_ulaw, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) == -1)
			{
				log(LOG_E, "Can't open/append \"%s\" (%s).\n", voicename_ulaw, strerror(errno));

				return(-1);
			}
		}

		if (!voicevbox)
		{
				/* Wenn die *.vbox Datei noch nicht existiert wird sie	*/
				/* jetzt erzeugt. Die Datei enthält die Informationen		*/
				/* wer wann wie die Nachricht gesprochen hat.				*/

			log(LOG_D, "Creating \"%s\"...\n", voicename_vbox);

			errno = 0;

			if ((vbox = fopen(voicename_vbox, "w")))
			{
				fprintf(vbox, "Name: %s\n" , voicevboxcall->name      );
				fprintf(vbox, "ID  : %s\n" , voicevboxuser->incomingid);
				fprintf(vbox, "Time: %ld\n", time(NULL)               );

				fclose(vbox);

				voicevbox = 1;
			}
			else log(LOG_E, "Can't create \"%s\" (%s).\n", voicename_vbox, strerror(errno));
		}
	}
	else
	{
		if (voicedesc != -1)
		{
			log(LOG_D, "Stopping voice recording...\n");
			log(LOG_D, "Closing \"%s\"...\n", voicename_ulaw);

			close(voicedesc);
		}
		
		voicedesc = -1;
	}

	return(0);
}

/************************************************************************* 
 ** voice_hear():	Schaltet das mithören der Voicedaten über /dev/audio	**
 **					ein oder aus.														**
 *************************************************************************
 ** => mode			> 0 um das Mithören einzuschalten; alle anderen Werte	**
 **					schalten das Mithören aus.										**
 **																							**
 ** Die Funktion gibt 0 zurück wenn die Aktion ausgeführt werden konnte	**
 ** oder -1 wenn nicht.																	**
 *************************************************************************/

int voice_hear(int mode)
{
	if (mode > 0)
	{
		if (audiodesc == -1)
		{
			log(LOG_D, "Starting audio playback...\n");

			if ((audiodesc = audio_open_dev("/dev/audio")) == -1) return(-1);
		}
	}
	else
	{
		if (audiodesc != -1)
		{
			log(LOG_D, "Stopping audio playback...\n");

			audio_close_dev(audiodesc);
		}
		
		audiodesc = -1;
	}

	return(0);
}

/************************************************************************* 
 ** FIXME
 *************************************************************************/

static void voice_stop_vtxrtx(void)
{
	unsigned char line[4];
	int           have;

	if (!vboxmodem.nocarrier)
	{
			/* DLE/DC4 an den Modememluator schicken um den Record-Modus */
			/* zu stoppen. Der Emulator sollte mit DLE/ETX antworten.	 */

		log_line(LOG_D, "Sending \"<DLE><DC4>\" to stop record mode...\n");

		printstring(line, "%c%c", DLE, DC4);

		vboxmodem_raw_write(&vboxmodem, line, 2);

		have = 0;

		modem_set_timeout(modemsetup.commandtimeout);

		while (vboxmodem_raw_read(&vboxmodem, line, 1) == 1)
		{
			log_char(LOG_D, *line);

			if (*line != DLE)
			{
				if ((*line == ETX) && (have))
				{
					have++;

					break;
				}
				else have = 0;
			}
			else have = 1;
		}

		modem_set_timeout(0);

		if (have == 2) log_line(LOG_D, "Found <DLE><ETX>!\n");

		log_line(LOG_D, "Sending \"<DLE><ETX>\" to stop playback mode...\n");

		printstring(line, "%c%c", DLE, ETX);

		vboxmodem_raw_write(&vboxmodem, line, 2);

		have = 0;

		modem_set_timeout(modemsetup.commandtimeout);

		while (vboxmodem_raw_read(&vboxmodem, line, 1) == 1)
		{
			log_char(LOG_D, *line);

			if (*line != DLE)
			{
				if ((*line == DC4) && (have))
				{
					have++;

					break;
				}
				else have = 0;
			}
			else have = 1;
		}

		modem_set_timeout(0);

		if (have == 2) log_line(LOG_D, "Found <DLE><DC4>!\n");
	}
}

/************************************************************************* 
 ** voice_create_vboxcall():	Erzeugt eine Call Datei mit Informationen	**
 **									über den aktuellen Anruf.						**
 *************************************************************************/

static void voice_create_vboxcall(void)
{
	FILE *call;

	log(LOG_D, "Creating \"%s\"...\n", voicename_call);

	if ((call = fopen(voicename_call, "w")))
	{
			/* CallerID, Name, tty-Device und die aktuelle PID in der	*/
			/* Datei speichern.														*/

		fprintf(call, "%s:%s:%s:%d\n", voicevboxuser->incomingid, voicevboxuser->name, savettydname, getpid());

		fclose(call);
	}
	else log(LOG_E, "Can't create \"%s\" (%s)!\n", voicename_call, strerror(errno));
}

/************************************************************************* 
 ** voice_remove_vboxcall():	Löscht die Call Datei.							**
 *************************************************************************/

static void voice_remove_vboxcall(void)
{
	log(LOG_D, "Removing \"%s\"...\n", voicename_call);

	if (remove(voicename_call) != 0)
	{
		if (errno != ENOENT) log(LOG_E, "Can't remove \"%s\" (%s)!\n", voicename_call, strerror(errno));
	}
}

/************************************************************************* 
 ** voice_mkdir():	Erzeugt die Verzeichnisse im Userspool.				**
 *************************************************************************/

static void voice_mkdir(unsigned char *name)
{
	if (name)
		printstring(temppathname, "%s/%s", voicevboxuser->home, name);
	else
		printstring(temppathname, "%s", voicevboxuser->home);

	log(LOG_D, "Creating directory \"%s\"...\n", temppathname);

	errno = 0;

	if (mkdir(temppathname, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH) == -1)
	{
		if (errno != EEXIST)
		{
			log(LOG_E, "Can't create \"%s\" (%s).\n", temppathname, strerror(errno));
		}
	}
}

/************************************************************************* 
 ** voice_check_touchtone():	Prüft ob es sich bei einem Zeichen um		**
 **									einen Touchtone handelt.						**
 *************************************************************************
 ** => c								Zeichen												**
 **																							**
 ** Rückgabe ist 0 wenn es ein Touchtone war oder -1 wenn nicht.			**
 *************************************************************************/

static int voice_check_touchtone(int c)
{
	unsigned char  tone[2];
	unsigned char *stop;
	int				i;

	switch (c)
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
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case '#':
		case '*':
		{
			if (c != '*')
			{
				tone[0] = c;
				tone[1] = 0;

				if (*voice_touchtone_sequence == '*')
				{
						/* Wenn das erste Zeichen in der Sequenz ein '*'	*/
						/* ist (Sequenzstart) wird der neue Touchtone am	*/
						/* Ende angehängt - es sei denn die Sequenz ent-	*/
						/* hält ein '#' (Sequenzende).							*/

					if (!(stop = rindex(voice_touchtone_sequence, '#')))
					{
						if (strlen(voice_touchtone_sequence) >= VBOXVOICE_SEQUENCE)
						{
								/* Die Sequenz ist bereits voll; der neue	*/
								/* Touchtone wird nicht angehängt!			*/

							log(LOG_E, "Internal touchtone sequence is full (touchtone ignored).\n");
						}
						else strcat(voice_touchtone_sequence, tone);
					}
					else strcpy(voice_touchtone_sequence, tone);
				}
				else
				{
						/* Erstes Zeichen in der Sequenz ist kein '*', der	*/
						/* Touchtone ersetzt die alte Sequenz.					*/

					strcpy(voice_touchtone_sequence, tone);
				}
			}
			else strcpy(voice_touchtone_sequence, "*");

			log(LOG_I, "Touchtone \"%c\" entered (%s).\n", c, voice_touchtone_sequence);

				/* Prüfen ob die eingegebene Sequenz in der Breakliste	*/
				/* vorkommt.															*/

			for (i = 0; i < VBOXBREAK_MAX_ENTRIES; i++)
			{
				if (breaklist[i])
				{
					if (strcmp(breaklist[i], voice_touchtone_sequence) == 0) return(0);
				}
			}
		}
		break;

		case 'q':	/* Quiet					*/
		case 's':	/* Silence				*/
		case 'c':	/* Fax calling tone	*/
		case 'b':	/* Busy tone			*/
			break;

		default:
         log_line(LOG_W, "Illegal \"<DLE>\" shielded code \"");
         log_char(LOG_W, c);
         log_text(LOG_W, "\" (ignored)...\n");
			break;
	}

	return(-1);
}
