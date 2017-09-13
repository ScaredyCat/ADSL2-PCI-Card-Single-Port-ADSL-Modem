/*
** $Id: vboxgetty.c,v 1.9 2000/11/30 15:35:20 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>
#include <sys/vfs.h>
#include <signal.h>

#include "init.h"
#include "vboxgetty.h"
#include "log.h"
#include "modem.h"
#include "rcvbox.h"
#include "script.h"
#include "libvbox.h"

/** Prototypes ***********************************************************/

static void version(void);
static void version_vboxputty(void);
static void usage(void);
static void usage_vboxputty(void);
static void main_program(void);
static int  main_vboxputty(char*, int, int, int, int);
static int  answer_call(void);
static int  trigger_call(char *, int, int);
static int	check_spool_space(unsigned long);

/** Variables ************************************************************/

char *vbasename;

static int modemstate;

/** Structures ***********************************************************/

static struct option arguments[] =
{
	{ "version" , no_argument      , NULL, 'v' },
	{ "help"    , no_argument      , NULL, 'h' },
	{ "file"    , required_argument, NULL, 'f' },
	{ "device"  , required_argument, NULL, 'd' },
	{ "language", required_argument, NULL, 'l' },
	{ NULL      , 0                , NULL, 0   }
};

static struct option arguments_vboxputty[] =
{
        { "version" , no_argument      , NULL, 'v' },
	{ "help"    , no_argument      , NULL, 'h' },
	{ "file"    , required_argument, NULL, 'f' },
	{ "device"  , required_argument, NULL, 'd' },
	{ "call"    , required_argument, NULL, 'c' },
	{ "ring"    , required_argument, NULL, 's' },
	{ "wait"    , required_argument, NULL, 'w' },
	{ "try"     , required_argument, NULL, 't' },
	{ "redial"  , required_argument, NULL, 'r' },
	{ NULL      , 0                , NULL, 0 }
};

/*************************************************************************/
/** The magic main...																	**/
/*************************************************************************/

int main(int argc, char **argv)
{
	char *usevrc   = NULL;
	char *device   = NULL;
	char *language = NULL;
	char *call     = NULL;
	int  ring      = 40;
	int  wait      = 2;
	int  try       = 3;
	int  redial    = 30;
	int	opts;

	if (!(vbasename = rindex(argv[0], '/')))
	{
		vbasename = argv[0];
	}
	else vbasename++;

	usevrc = GETTYRC;
	device = "";

	if (strcmp(vbasename, "vboxputty") == 0) {
	  while ((opts = getopt_long(argc, argv, "vhf:d:c:s:w:t:r:", arguments_vboxputty, (int *)0)) != EOF) {
	    switch (opts)
	      {
	      case 'f':
		usevrc = optarg;
		break;
	      case 'd':
		device = optarg;
		break;
	      case 'c':
		call = optarg;
		break;
	      case 's':
		ring = atoi(optarg);
		break;
	      case 'w':
		wait = atoi(optarg);
		break;
	      case 't':
		try  = atoi(optarg);
		break;
	      case 'r':
		redial = atoi(optarg);
		break;
	      case 'v':
		version_vboxputty();
		break;
	      case 'h':
	      default:
		usage_vboxputty();
		break;
	      }
	  }
	}
	else {
	while ((opts = getopt_long(argc, argv, "vhf:d:", arguments, (int *)0)) != EOF)
	{
		switch (opts)
		{
			case 'f':
				usevrc = optarg;
				break;
				
			case 'd':
				device = optarg;
				break;

			case 'l':
				language = optarg;
				break;

			case 'v':
				version();
				break;
				
			case 'h':
			default:
				usage();
				break;
		}
	}

	if (language) setenv("LANG", language, 1);

	}

	if (getuid() != 0)
	{
		log(L_STDERR, "%s: must be run by root!\n", vbasename);

		exit(5);
	}

	if (access(device, W_OK|R_OK|F_OK) != 0)
	{
		log(L_STDERR, "%s: device \"%s\" is not accessable.\n", vbasename, device);

		exit(5);
	}

	if (access(usevrc, R_OK|F_OK) != 0)
	{
		log(L_STDERR, "%s: Setup \"%s\" doesn't exist.\n", vbasename, usevrc);

		exit(5);
	}

	if (!init_program(device, usevrc)) exit(5);

	signal(SIGHUP	, exit_program);
	signal(SIGINT	, exit_program);
	signal(SIGTERM	, exit_program);

	if (strcmp(vbasename, "vboxputty") == 0) {
	  int rc;
	  rc =main_vboxputty(call, ring, wait, try, redial);
	  exit_program_code(rc);
	}
	main_program();
	exit_program(SIGTERM);
	return 0; /*NOTREACHED*/
}

/*************************************************************************/
/** version():	Displays the package version.										**/
/*************************************************************************/

static void version(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "%s version %s (%s)\n", vbasename, VERSION, VERDATE);
	fprintf(stderr, "\n");
	
	exit(1);
}
static void version_vboxputty(void)
{
	fprintf(stderr, "\n");
       fprintf(stderr, "vboxgetty version %s (%s)\n", VERSION, VERDATE);
	fprintf(stderr, "%s version 0.1.0 (13.1.2000)  <pape@innominate.de>\n", vbasename);
	fprintf(stderr, "\n");
	
	exit(1);
}

/*************************************************************************/
/** usage(): Displays usage message.												**/
/*************************************************************************/

static void usage(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s OPTION [ OPTION ] [ ... ]\n", vbasename);
	fprintf(stderr, "\n");
	fprintf(stderr, "-f, --file FILE    Overwrites \"%s\".\n", GETTYRC);
	fprintf(stderr, "-d, --device TTY   Use device TTY for modem operations [required].\n");
	fprintf(stderr, "-h, --help         Displays this short help.\n");
	fprintf(stderr, "-v, --version      Displays the package version.\n");
	fprintf(stderr, "\n");
	
	exit(1);
}
static void usage_vboxputty(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s OPTION [ OPTION ] [ ... ]\n", vbasename);
	fprintf(stderr, "\n");
	fprintf(stderr, "-f, --file FILE    Overwrites \"%s\".\n", GETTYRC);
	fprintf(stderr, "-d, --device TTY   Use device TTY for modem operations [required].\n");
	fprintf(stderr, "-c, --call NUMBER  Number to dial is NUMBER.\n");
	fprintf(stderr, "-s, --ring SEC     Ring SEC seconds, then hangup.\n");
        fprintf(stderr, "-w, --wait SEC     sleep SEC seconds after connect.\n");
	fprintf(stderr, "-t, --try NUM      try NUM times to reach NUMBER.\n");
	fprintf(stderr, "-r, --redial SEC   Wait SEC seconds after connection-failure.\n");
	fprintf(stderr, "-h, --help         Displays this short help.\n");
	fprintf(stderr, "-v, --version      Displays the package version.\n");
	fprintf(stderr, "\n");
	
	exit(1);
}

/*************************************************************************/
/** main_program(): Mainloop.														   **/
/*************************************************************************/

static void main_program(void)
{
	int modeminits;

	modemstate = MODEM_STATE_INITIALIZE;
	modeminits = 0;

	while (modemstate != MODEM_STATE_EXIT)
	{
		switch (modemstate)
		{
			case MODEM_STATE_INITIALIZE:

				if (!modem_initialize())
				{
					if (setup.modem.badinitsexit > 0)
					{
						modeminits++;
						
						if (modeminits >= setup.modem.badinitsexit)
						{
							modemstate = MODEM_STATE_EXIT;
							modeminits = 0;

							log(L_FATAL, "Exit program while bad init limit are reached.\n");
						}
						else log(L_WARN, "Bad initialization - Program will exist on %d trys!\n", (setup.modem.badinitsexit - modeminits));
					}
				}
				else
				{
					modemstate = MODEM_STATE_WAITING;
					modeminits = 0;
				}

				break;

			case MODEM_STATE_WAITING:
			
				modem_flush(0);
				
				if (modem_wait())
				{
					modemstate = MODEM_STATE_RING;
					modeminits = 0;
				}
				else modemstate = MODEM_STATE_CHECK;
				
				break;

			case MODEM_STATE_CHECK:
			
				log(L_DEBUG, "Checking if modem is still alive...\n");

				if (!ctrl_ishere(setup.spool, CTRL_NAME_STOP))
				{
					if (modem_command("AT", "OK") >= 1)
					{
						modemstate = MODEM_STATE_WAITING;
						modeminits = 0;
					}
					else modemstate = MODEM_STATE_INITIALIZE;
				}
				else
				{
					log(L_INFO, "Control file \"%s\" exists - program will quit...\n", CTRL_NAME_STOP);

					modemstate = MODEM_STATE_EXIT;
				}
				
				break;

			case MODEM_STATE_RING:

				modemstate = MODEM_STATE_INITIALIZE;

				if (check_spool_space(setup.freespace))
				{
					modem_set_nocarrier_state(FALSE);

					if (modem_count_rings(vboxrc_get_rings_to_wait()))
					{
						if (!modem_get_nocarrier_state())
						{
							if (!answer_call()) modemstate = MODEM_STATE_EXIT;

							modem_hangup();
						}
					}
				}

				break;
		}
	}
}

static int main_vboxputty (char *call, int ring, int wait, int try, int redial) {
  int modeminits = 0;
  int rc =FALSE;
  int i;

  if (!modem_initialize()) {
    if (setup.modem.badinitsexit > 0) {
      modeminits++;
      if (modeminits >= setup.modem.badinitsexit) {
	modeminits = 0;
	
	log(L_FATAL, "vboxputty: Exit program while bad init limit are reached.\n");
	exit_program_code(1);
      }
      else {
	log(L_WARN, "vboxputty: Bad initialization - Program will exist on %d trys!\n", (setup.modem.badinitsexit - modeminits));
      }
    }
  }

  for (i =0; i < try; i++) {  
    modem_flush(0);
    if ((rc =trigger_call(call, ring, wait)) != 0) {
      log(L_ERROR, "vboxputty: Cannot trigger call (No %d/%d): %d\n",
	  i +1, try, rc);
      sleep(redial);
    }
    else {
      break;
    }
  }
  return(rc);
}

/*************************************************************************/
/** answer_call(): Answers the call and starts the tcl script.			   **/
/*************************************************************************/

static int answer_call(void)
{
	char run[PATH_MAX + 1];

	if (!index(setup.voice.tclscriptname, '/'))
	{
		xstrncpy(run, setup.spool, PATH_MAX);
		xstrncat(run, "/", PATH_MAX);
		xstrncat(run, setup.voice.tclscriptname, PATH_MAX);
	}
	else xstrncpy(run, setup.voice.tclscriptname, PATH_MAX);

	return(script_run(run));
}
static int trigger_call(char *call, int timeout, int wait) {
  char run[PATH_MAX + 1];
  
  if (!index(setup.voice.tclscriptname, '/')) {
    xstrncpy(run, setup.spool, PATH_MAX);
    xstrncat(run, "/", PATH_MAX);
    xstrncat(run, setup.voice.tclscriptname, PATH_MAX);
  }
  else {
    xstrncpy(run, setup.voice.tclscriptname, PATH_MAX);
  }
  if (!modem_command("ATS18=1", "OK")) {
    log(L_ERROR, "vboxputty: Setting of S18 failed.\n");
  }
  else {
    setup.modem.timeout_cmd = timeout;
    return(script_run_call(run, call, wait));
  }
  return(FALSE);
}

/*************************************************************************/
/** check_spool_space(): Checks space on spoolpartition.                **/
/*************************************************************************/

static int check_spool_space(unsigned long need)
{
	struct statfs stat;
	unsigned long have;

	log(L_DEBUG, "Checking free space on \"%s\"...\n", setup.spool);

	if (need <= 0)
	{
		log(L_WARN, "Free disc space check disabled!\n");

		returnok();
	}

	if (statfs(setup.spool, &stat) == 0)
	{
		have = (stat.f_bfree * stat.f_bsize);

		log_line(L_JUNK, "%ld bytes available; %ld bytes needed... ", have, need);

		if (have >= need)
		{
			log_text(L_JUNK, "enough.\n");

			returnok();
		}

		log_text(L_JUNK, "not enough!\n");
	}
	else log(L_ERROR, "Can't get statistic about disc space!");

	returnerror();
}

/*************************************************************************/
/** block_all_signals(): Blocks all signals.									   **/
/*************************************************************************/

void block_all_signals(void)
{
	int i;

	log(L_DEBUG, "Blocking all signals (0-%d)...\n", NSIG);
	
	for (i = 0; i < NSIG; i++) signal(i, SIG_IGN);
}
