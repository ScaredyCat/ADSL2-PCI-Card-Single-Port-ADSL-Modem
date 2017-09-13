/*
** $Id: script.c,v 1.11 2000/11/30 15:35:20 paul Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <tcl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "script.h"
#include "log.h"
#include "voice.h"
#include "modem.h"
#include "libvbox.h"

/** Tcl functions *********************************************************/

int vbox_breaklist(ClientData, Tcl_Interp *, int, char *[]);
int vbox_init_touchtones(ClientData, Tcl_Interp *, int, char *[]);
int vbox_put_message(ClientData, Tcl_Interp *, int, char *[]);
int vbox_get_message(ClientData, Tcl_Interp *, int, char *[]);
int vbox_wait(ClientData, Tcl_Interp *, int, char *[]);
int vbox_pause(ClientData, Tcl_Interp *, int, char *[]);
int vbox_get_nr_new_messages(ClientData, Tcl_Interp *, int, char *[]);
int vbox_get_nr_all_messages(ClientData, Tcl_Interp *, int, char *[]);
int vbox_message_info(ClientData, Tcl_Interp *, int, char *[]);

static struct vbox_tcl_function vbox_tcl_functions[] =
{
	{ "vbox_breaklist"          , vbox_breaklist           },
	{ "vbox_put_message"        , vbox_put_message         },
	{ "vbox_play_message"       , vbox_put_message         },
	{ "vbox_get_message"	       , vbox_get_message         },
	{ "vbox_record_message"     , vbox_get_message         },
	{ "vbox_init_touchtones"    , vbox_init_touchtones     },
	{ "vbox_wait"               , vbox_wait                },
	{ "vbox_pause"              , vbox_pause               },
	{ "vbox_get_nr_new_messages", vbox_get_nr_new_messages },
	{ "vbox_get_nr_all_messages", vbox_get_nr_all_messages },
	{ "vbox_message_info"       , vbox_message_info        },
	{ NULL                      , NULL                     }
};

/** Prototypes ************************************************************/

static int init_tcl_functions(Tcl_Interp *);
static int init_tcl_variables(Tcl_Interp *);
static int make_tcl_variable(Tcl_Interp *, char *, char *);

/** Defines **************************************************************/

#define MYMKVAR(V, A) make_tcl_variable(interp, V, A)

/*************************************************************************/
/** script_run():	Starts a external tcl script.									**/
/*************************************************************************/

int script_run(char *script)
{
	Tcl_Interp	*interpreter;
	int		result;
	int         rcdelete;

	log(L_DEBUG, "Initializing tcl script \"%s\"...\n", script);

	breaklist_init();

	result = FALSE;

	if ((interpreter = Tcl_CreateInterp()))
	{
		if (Tcl_Init(interpreter) == TCL_OK)
		{
			if (init_tcl_functions(interpreter))
			{
				if (init_tcl_variables(interpreter))
				{
					log(L_DEBUG, "Answering call...\n");

					if (modem_command("ATA", "VCON|CONNECT") > 0)
				{
						log(L_INFO, "Running tcl script \"%s\"...\n", script);

					if (Tcl_EvalFile(interpreter, script) != TCL_OK)
					{
						log(L_ERROR, "In \"%s\": %s (line %d).\n", script, interpreter->result, interpreter->errorLine);
					}
					else
					{
						log(L_DEBUG, "Back from tcl script...\n");

						result = TRUE;
					}
				}
					else log(L_FATAL, "Can't answer call!\n");
				}
				else log(L_ERROR, "In \"%s\": %s (line %d).\n", script, interpreter->result, interpreter->errorLine);
			}
			else log(L_FATAL, "Can't create all new tcl commands.\n");

			if ((rcdelete = Tcl_InterpDeleted(interpreter)) == 0)
			{
				log(L_DEBUG, "Freeing tcl interpreter...\n");

				Tcl_DeleteInterp(interpreter);
			}
			else
			{
				log(L_ERROR, "Can't free tcl interpreter - Tcl_InterpDeleted() returns %d!\n", rcdelete);

				result = FALSE;
			}
		}
		else log(L_FATAL, "Can't initialize tcl interpreter.\n");
	}
	else log(L_FATAL, "Can't create tcl interpreter.\n");

	breaklist_exit();

	if (!result)
	{
		log(L_ERROR, "General tcl problem - setting flag to quit program...\n");
	}

	return(result);
}
int script_run_call(char *script, char *call, int wait) {
  /* returns 99 on temporary failure. */

  Tcl_Interp *interpreter;
  int		   result;
  int         rcdelete;

  log(L_DEBUG, "Initializing tcl script \"%s\"...\n", script);
  breaklist_init();
  result = 1;
  if ((interpreter = Tcl_CreateInterp())) {
    if (Tcl_Init(interpreter) == TCL_OK) {
      if (init_tcl_functions(interpreter)) {
	if (init_tcl_variables(interpreter)) {
	  char cmnd[PATH_MAX +1 ];
	  int mcrc;
	  xstrncpy(cmnd, "ATD", PATH_MAX);
	  xstrncat(cmnd, call, PATH_MAX);
	  log(L_DEBUG, "vboxputty: Triggering call...\n");
	  if ((mcrc =modem_command(cmnd, "VCON|CONNECT|BUSY")) > 0) {
	    if (mcrc == 3) {
	      return(99);
	    }
	    sleep(wait);
	    log(L_INFO, "vboxputty: Running tcl script \"%s\"...\n", script);
	    if (Tcl_EvalFile(interpreter, script) != TCL_OK) {
	      log(L_ERROR, "In \"%s\": %s (line %d).\n", script, interpreter->result, interpreter->errorLine);
	    }
	    else {
	      log(L_DEBUG, "vboxputty: Back from tcl script...\n");
	      result = 0;
	    }
	  }
	  else {
	    log(L_FATAL, "vboxputty: Can't trigger call: no VCON|CONNECT.\n");
	    result = 99;
	  }
	}
	else log(L_ERROR, "In \"%s\": %s (line %d).\n", script, interpreter->result, interpreter->errorLine);
      }
      else log(L_FATAL, "Can't create all new tcl commands.\n");
      if ((rcdelete = Tcl_InterpDeleted(interpreter)) == 0) {
	log(L_DEBUG, "Freeing tcl interpreter...\n");
	Tcl_DeleteInterp(interpreter);
      }
      else {
	log(L_ERROR, "Can't free tcl interpreter - Tcl_InterpDeleted() returns %d!\n", rcdelete);
	result = 1;
      }
    }
    else log(L_FATAL, "Can't initialize tcl interpreter.\n");
  }
  else log(L_FATAL, "Can't create tcl interpreter.\n");
  
  breaklist_exit();
  
  if (result != 0) {
    log(L_ERROR, "General tcl problem - setting flag to quit program...\n");
  }

  return(result);
}

/**************************************************************************/
/** init_tcl_functions(): Inits all new tcl functions.                   **/
/**************************************************************************/

static int init_tcl_functions(Tcl_Interp *interp)
{
	int i;

	i = 0;

	while (vbox_tcl_functions[i].name)
	{
		if (!Tcl_CreateCommand(interp, vbox_tcl_functions[i].name, vbox_tcl_functions[i].proc, NULL, NULL))
		{
			log(L_FATAL, "Can't create tcl function '%s'.\n", vbox_tcl_functions[i].name);

			returnerror();
		}

		i++;
	}

	returnok();
}

/**************************************************************************/
/** init_tcl_variables(): Inits all new tcl variables.                   **/
/**************************************************************************/

static int init_tcl_variables(Tcl_Interp *interp)
{
	char savename[32];
	char savetime[32];

	printstring(savename, "%14.14lu-%8.8lu", (unsigned long)time(NULL), (unsigned long)getpid());
	printstring(savetime, "%d", setup.voice.recordtime);

	if (!MYMKVAR("vbox_var_bindir"   , BINDIR                 )) returnerror();
	if (!MYMKVAR("vbox_var_savename"	, savename               )) returnerror();
	if (!MYMKVAR("vbox_var_rectime"	, savetime               )) returnerror();
	if (!MYMKVAR("vbox_var_spooldir"	, setup.spool            )) returnerror();
	if (!MYMKVAR("vbox_var_checknew", setup.voice.checknewpath)) returnerror();
	if (!MYMKVAR("vbox_msg_standard"	, setup.voice.standardmsg)) returnerror();
	if (!MYMKVAR("vbox_msg_beep"		, setup.voice.beepmsg    )) returnerror();
	if (!MYMKVAR("vbox_msg_timeout"	, setup.voice.timeoutmsg )) returnerror();
	if (!MYMKVAR("vbox_caller_id"		, setup.voice.callerid   )) returnerror();
	if (!MYMKVAR("vbox_caller_phone"	, setup.voice.phone      )) returnerror();
	if (!MYMKVAR("vbox_caller_name"	, setup.voice.name       )) returnerror();
	if (!MYMKVAR("vbox_user_name"		, setup.users.name       )) returnerror();
	if (!MYMKVAR("vbox_user_home"		, setup.users.home       )) returnerror();

	if (!MYMKVAR("vbox_flag_standard", setup.voice.domessage ? "TRUE" : "FALSE")) returnerror();
	if (!MYMKVAR("vbox_flag_beep"    , setup.voice.dobeep    ? "TRUE" : "FALSE")) returnerror();
	if (!MYMKVAR("vbox_flag_timeout" , setup.voice.dotimeout ? "TRUE" : "FALSE")) returnerror();
	if (!MYMKVAR("vbox_flag_record"	, setup.voice.dorecord  ? "TRUE" : "FALSE")) returnerror();

	returnok();
}

/**************************************************************************/
/** make_tcl_variable(): Creates a new tcl variable.                     **/
/**************************************************************************/

static int make_tcl_variable(Tcl_Interp *interp, char *var, char *arg)
{
	if (Tcl_VarEval(interp, "set ", var, " \"", arg, "\"", NULL) != TCL_OK)
	{
		log(L_FATAL, "Can't initzalize tcl variable '%s'.\n", var);

		returnerror();
	}

	returnok();
}

/*************************************************************************/
/** script_check_interpreter(): Checks tcl interpreter versions.			**/
/*************************************************************************/

int script_check_interpreter(void)
{
	log(L_INFO, "Tcl interpreter version %s...\n", TCL_VERSION);

	returnok();
}

/*************************************************************************/
/** vbox_message_info(): Returns message header fields.                 **/
/*************************************************************************/

int vbox_message_info(ClientData cd, Tcl_Interp *ip, int argc, char *argv[])
{
	vaheader_t  header;
	int         fd;
	char       *name;
	char       *need;
	int         field;

	strcpy(ip->result, "");

	if (argc == 3)
	{
		name = argv[1];
		need = argv[2];

		if ((fd = open(name, O_RDONLY)) != -1)
		{
			if (header_get(fd, &header))
			{
				field = (int)xstrtol(need, 0);

				switch (field)
				{
					case 1:
						printstring(ip->result, "%ld", (long)ntohl(header.time));
						break;

					case 2:
						printstring(ip->result, "%d", (int)ntohl(header.compression));
						break;

					case 3:
						printstring(ip->result, "%s", header.callerid);
						break;

					case 4:
						printstring(ip->result, "%s", header.name);
						break;

					case 5:
						printstring(ip->result, "%s", header.phone);
						break;

					case 6:
						printstring(ip->result, "%s", header.location);
						break;

					default:
						log(L_ERROR, "[vbox_message_info] unknown field number %d.\n", field);
						break;
				}
			}
			else log(L_ERROR, "[vbox_message_info] can't get header from '%s'.\n", name);

			close(fd);
		}
		else log(L_ERROR, "[vbox_message_info] can't open '%s'.\n", name);
	}
	else log(L_ERROR, "[vbox_message_info] usage: vbox_message_info <message> <fieldnr>\n");

	return(TCL_OK);
}

/*************************************************************************/
/** vbox_breaklist(): DTMF sequence support.									   **/
/*************************************************************************/

int vbox_breaklist(ClientData cd, Tcl_Interp *ip, int argc, char *argv[])
{
	int   i;
	char *line;

	if (argc == 2)
	{
		if (strcasecmp(argv[1], "list") == 0)
		{
			for (i = 0; i < VBOX_MAX_BREAKLIST; i++)
			{
				if ((line = breaklist_nr(i)))
				{
					log(L_DEBUG, "[vbox_breaklist] %s\n", line);
				}
			}

			return(TCL_OK);
		}
	}

	if (argc >= 3)
	{
		if (strcasecmp(argv[1], "add") == 0)
		{
			for (i = 2; i < argc; i++)
			{
				if (!breaklist_add(argv[i]))
				{
					log(L_ERROR, "[vbox_breaklist] can't add \"%s\".\n", argv[i]);
				}
			}

			return(TCL_OK);
		}

		if (strcasecmp(argv[1], "rem") == 0)
		{
			for (i = 2; i < argc; i++)
			{
				if (strcasecmp(argv[i], "all") == 0)
				{
					breaklist_exit();
				}
				else breaklist_rem(argv[i]);
			}

			return(TCL_OK);
		}
	}

	log(L_ERROR, "[vbox_breaklist] usage: vbox_breaklist <rem|add> <sequence> [sequence] [...]\n");

	return(TCL_OK);
}

/*************************************************************************/
/** vbox_put_message():	Plays a voice message.									**/
/*************************************************************************/

int vbox_put_message(ClientData cd, Tcl_Interp *ip, int argc, char *argv[])
{
	int result;

	if (argc != 2)
	{
		log(L_ERROR, "[vbox_put_message] usage: vbox_put_message <messagename>\n");

		printstring(ip->result, "ERROR");
	}
	else
	{
		result = voice_put_message(argv[1]);

		switch (result)
		{
			case VOICE_ACTION_ERROR:
				printstring(ip->result, "ERROR");
				break;

			case VOICE_ACTION_LOCALHANGUP:
			case VOICE_ACTION_REMOTEHANGUP:
				printstring(ip->result, "HANGUP");
				break;

			case VOICE_ACTION_TOUCHTONES:
				printstring(ip->result, touchtones);
				touchtones[0] = '\0';
				break;

			default:
				printstring(ip->result, "OK");
				break;
		}

		log(L_DEBUG, "[vbox_put_message] result \"%s\".\n", ip->result);
	}

	return(TCL_OK);
}

/*************************************************************************/
/** vbox_get_message():	Records a voice message.								**/
/*************************************************************************/

int vbox_get_message(ClientData cd, Tcl_Interp *ip, int argc, char *argv[])
{
	int result;

	if (argc != 3)
	{
		log(L_ERROR, "[vbox_get_message] usage: vbox_get_message <messagename> <recordtime>\n");

		printstring(ip->result, "ERROR");
	}
	else
	{
		result = voice_get_message(argv[1], argv[2], TRUE);

		switch (result)
		{
			case VOICE_ACTION_ERROR:
				printstring(ip->result, "ERROR");
				break;

			case VOICE_ACTION_TIMEOUT:
				printstring(ip->result, "TIMEOUT");
				break;

			case VOICE_ACTION_LOCALHANGUP:
			case VOICE_ACTION_REMOTEHANGUP:
				printstring(ip->result, "HANGUP");
				break;

			case VOICE_ACTION_TOUCHTONES:
				printstring(ip->result, touchtones);
				touchtones[0] = '\0';
				break;

			default:
				printstring(ip->result, "OK");
				break;
		}

		log(L_DEBUG, "[vbox_get_message] result \"%s\".\n", ip->result);
	}

	return(TCL_OK);
}

/*************************************************************************/
/** vbox_wait(): Waits for DTMF input.											   **/
/*************************************************************************/

int vbox_wait(ClientData cd, Tcl_Interp *ip, int argc, char *argv[])
{
	int result;

	if (argc != 2)
	{
		log(L_ERROR, "[vbox_wait] usage: vbox_wait <seconds>\n");

		printstring(ip->result, "ERROR");
	}
	else
	{
		result = voice_get_message("", argv[1], FALSE);

		switch (result)
		{
			case VOICE_ACTION_ERROR:
				printstring(ip->result, "ERROR");
				break;

			case VOICE_ACTION_TIMEOUT:
				printstring(ip->result, "TIMEOUT");
				break;

			case VOICE_ACTION_LOCALHANGUP:
			case VOICE_ACTION_REMOTEHANGUP:
				printstring(ip->result, "HANGUP");
				break;

			case VOICE_ACTION_TOUCHTONES:
				printstring(ip->result, touchtones);
				touchtones[0] = '\0';
				break;

			default:
				printstring(ip->result, "OK");
				break;
		}

		log(L_DEBUG, "[vbox_wait] result \"%s\".\n", ip->result);
	}

	return(TCL_OK);
}

/*************************************************************************/
/** vbox_pause():	Sleeps some milliseconds.										**/
/*************************************************************************/

int vbox_pause(ClientData cd, Tcl_Interp *ip, int argc, char *argv[])
{
	long p;

	if (argc != 2)
	{
		log(L_ERROR, "[vbox_pause] usage: vbox_pause <ms>\n");

		printstring(ip->result, "ERROR");
	}
	else
	{
		p = xstrtol(argv[1], 800);

		log(L_JUNK, "[vbox_pause] waiting %lu ms...\n", p);

		xpause(p);

		printstring(ip->result, "OK");
	}

	return(TCL_OK);
}

/*************************************************************************/
/** vbox_init_touchtones(): Initialize touchtone sequence.				   **/
/*************************************************************************/

int vbox_init_touchtones(ClientData cd, Tcl_Interp *ip, int argc, char *argv[])
{
	*touchtones = '\0';

	printstring(ip->result, "OK");

	return(TCL_OK);
}

/*************************************************************************/
/** vbox_get_nr_new_messages(): Returns the number of new messages.     **/
/*************************************************************************/

int vbox_get_nr_new_messages(ClientData cd, Tcl_Interp *ip, int argc, char *argv[])
{
	int n;

	if (argc != 2)
	{
		log(L_ERROR, "[vbox_get_nr_new_messages] usage: vbox_get_nr_new_messages <path>\n");

		printstring(ip->result, "0");
	}
	else
	{
		log(L_JUNK, "[vbox_get_nr_new_messages] counting new messages in \"%s\"...\n", argv[1]);

		n = get_nr_messages(argv[1], TRUE);

		log(L_DEBUG, "[vbox_get_nr_new_messages] result \"%d\".\n", n);

		printstring(ip->result, "%d", n);
	}

	return(TCL_OK);
}

/*************************************************************************/
/** vbox_get_nr_all_messages(): Returns the number of messages.         **/
/*************************************************************************/

int vbox_get_nr_all_messages(ClientData cd, Tcl_Interp *ip, int argc, char *argv[])
{
	int n;

	if (argc != 2)
	{
		log(L_ERROR, "[vbox_get_nr_all_messages] usage: vbox_get_nr_all_messages <path>\n");

		printstring(ip->result, "0");
	}
	else
	{
		log(L_JUNK, "[vbox_get_nr_all_messages] counting all messages in \"%s\"...\n", argv[1]);

		n = get_nr_messages(argv[1], FALSE);

		log(L_DEBUG, "[vbox_get_nr_all_messages] result \"%d\".\n", n);

		printstring(ip->result, "%d", n);
	}

	return(TCL_OK);
}
