/*
** $Id: tclscript.c,v 1.11 1998/11/10 18:36:33 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <tcl.h>
#include <string.h>
#include <unistd.h>

#include "vboxgetty.h"
#include "log.h"
#include "modem.h"
#include "tclscript.h"
#include "stringutils.h"
#include "breaklist.h"
#include "voice.h"

/** Variables ************************************************************/

static Tcl_Interp *interpreter = NULL;

/** Tcl Prototypes *******************************************************/

int vbox_block(VBOX_TCLFUNC_PROTO);
int vbox_log(VBOX_TCLFUNC_PROTO);
int vbox_modem_command(VBOX_TCLFUNC_PROTO);
int vbox_voice(VBOX_TCLFUNC_PROTO);
int vbox_breaklist(VBOX_TCLFUNC_PROTO);

/** Structures ***********************************************************/

static struct vbox_tcl_function vbox_tcl_functions[] =
{
	{ "exit"						, vbox_block			},
	{ "vbox_log"				, vbox_log				},
	{ "vbox_modem_command"	, vbox_modem_command	},
	{ "vbox_voice"				, vbox_voice			},
	{ "vbox_breaklist"		, vbox_breaklist		},
	{ NULL						, NULL					}
};

/** Prototypes ***********************************************************/

static int scr_init_functions(void);





int scr_create_interpreter(void)
{
	log_line(LOG_D, "Initializing tcl interpreter...\n");

	if (TCL_MAJOR_VERSION >= 8)
	{
		if ((interpreter = Tcl_CreateInterp()))
		{
			if (scr_init_functions() == 0) return(0);
		}
	}

	return(-1);
}

void scr_remove_interpreter(void)
{
	int rc;

	if (interpreter)
	{
		log_line(LOG_D, "Removing tcl interpreter...\n");

		if ((rc = Tcl_InterpDeleted(interpreter)) == 0)
		{
			Tcl_DeleteInterp(interpreter);
		}
		else log_line(LOG_W, "Tcl interpreter can't be removed (returns %d).\n", rc);
	}
}

/*************************************************************************/
/** scr_execute():	Executes a tcl script.										**/
/*************************************************************************/
/** => name				Name of the script to execute								**/
/** => user				User name (if set, script is first searched in		**/
/**						users spooldir)												**/
/** <=					0 on success or -1 on error								**/
/*************************************************************************/

int scr_execute(unsigned char *name, struct vboxuser *user)
{
	int canrun = 0;

	if (user)
	{
		printstring(temppathname, "%s/tcl/%s", user->home, name);

		if (access(temppathname, F_OK|R_OK) == 0) canrun = 1;
	}

	if (!canrun)
	{
		printstring(temppathname, "%s/tcl/%s", PKGDATADIR, name);

		if (access(temppathname, F_OK|R_OK) == 0) canrun = 1;
	}

	if (canrun)
	{
		log_line(LOG_D, "Running \"%s\"...\n", temppathname);

		if (Tcl_EvalFile(interpreter, temppathname) == TCL_OK)
		{
			log_line(LOG_D, "Tcl script returns without errors.\n");
			
			return(0);
		}
		else log_line(LOG_E, "%s: %s [line %d]!\n", name, Tcl_GetStringResult(interpreter), interpreter->errorLine);
	}
	else log_line(LOG_E, "Tcl script \"%s\" not found.\n", name);

	return(-1);
}

/*************************************************************************/
/** scr_tcl_version():	Returns current tcl version number.					**/
/*************************************************************************/
/** <=						Tcl version string										**/
/*************************************************************************/

unsigned char *scr_tcl_version(void)
{
	return(TCL_VERSION);
}

/*************************************************************************/
/** scr_init_functions():	Adds the vbox functions to the interpreter.	**/
/*************************************************************************/
/** <=							0 on success or -1 on error						**/
/*************************************************************************/

static int scr_init_functions(void)
{
	int i = 0;
	
	while (vbox_tcl_functions[i].name)
	{
		if (!Tcl_CreateObjCommand(interpreter, vbox_tcl_functions[i].name, vbox_tcl_functions[i].proc, NULL, NULL))
		{
			log_line(LOG_E, "Can't add new tcl command \"%s\".\n", vbox_tcl_functions[i].name);

			return(-1);
		}
		else log_line(LOG_D, "New tcl command \"%s\" added.\n", vbox_tcl_functions[i].name);

		i++;
	}

	return(0);
}

/*************************************************************************/
/** scr_init_variables():	Initialize global tcl variables.					**/
/*************************************************************************/
/** => vars						Pointer to a filled variable structure			**/
/** <=							0 on success or -1 on error						**/
/*************************************************************************/

int scr_init_variables(struct vbox_tcl_variable *vars)
{
	int i = 0;
	
	while (vars[i].name)
	{
		if (Tcl_VarEval(interpreter, "set ", vars[i].name, " \"", vars[i].args, "\"", NULL) != TCL_OK)
		{
			log_line(LOG_E, "Can't set tcl variable \"%s\".\n", vars[i].name);

			return(-1);
		}
		else log_line(LOG_D, "Tcl variable \"%s\" set to \"%s\".\n", vars[i].name, vars[i].args);
	
		i++;
	}

	return(0);
}

















int vbox_block(VBOX_TCLFUNC)
{
	log_line(LOG_W, "Tcl command \"%s\" is blocked!\n", Tcl_GetStringFromObj(objv[0], NULL));

	return(TCL_OK);
}







int vbox_log(VBOX_TCLFUNC)
{
	unsigned char *levelv;
	int	         levelc;

	if (objc == 3)
	{
		levelv = Tcl_GetStringFromObj(objv[1], NULL);
		levelc = LOG_E;
		
		switch (levelv[0])
		{
			case 'W':
				levelc = LOG_W;
				break;

			case 'I':
				levelc = LOG_I;
				break;

			case 'A':
				levelc = LOG_A;
				break;

			case 'D':
				levelc = LOG_D;
				break;
		}

		log_line(levelc, "%s\n", Tcl_GetStringFromObj(objv[2], NULL));
	}
	else log_line(LOG_E, "Bad vars %d\n", objc);

	return(TCL_OK);
}

int vbox_modem_command(VBOX_TCLFUNC)
{
	int i;

	if (objc == 3)
	{
		i = modem_command(&vboxmodem, Tcl_GetStringFromObj(objv[1], NULL), Tcl_GetStringFromObj(objv[2], NULL));

		printstring(temppathname, "%d", i);
		
		Tcl_SetResult(intp, temppathname, NULL);
	}
	else log_line(LOG_E, "Bad vars %d\n", objc);

	return(TCL_OK);
}

/*************************************************************************/
/** vbox_voice():	Tcl Kommando "vbox_voice". Die Funktion gibt immer		**/
/**					TCL_OK zurück. Das Ergebnis der jeweiligen Funktion	**/
/**					wird als Rückgabe des Tcl Kommandos geliefert.			**/
/*************************************************************************/

int vbox_voice(VBOX_TCLFUNC)
{
	unsigned char *cmd;
	unsigned char *arg;
	int	rc;
	int	i;

	rc = 0;

	if (objc >= 3)
	{
		cmd = Tcl_GetStringFromObj(objv[1], NULL);
		arg = Tcl_GetStringFromObj(objv[2], NULL);

		if ((cmd) && (arg))
		{
			switch (*cmd)
			{
				case 'W':
				case 'w':
				{
						/* Zeitspanne warten und dabei auch eingehende	*/
						/* Daten vom Modem bearbeiten.						*/

					switch (voice_wait(xstrtol(arg, 60)))
					{
						case 0:
							Tcl_SetResult(intp, "OK", NULL);
							break;

						case 1:
							Tcl_SetResult(intp, voice_touchtone_sequence, NULL);
							break;

						case 2:
							Tcl_SetResult(intp, "SUSPEND", NULL);
							break;

						default:
							Tcl_SetResult(intp, "HANGUP", NULL);
							break;
					}
				}
				break;

				case 'P':
				case 'p':
				{
						/* Nachricht(en) abspielen und dabei auch eingeh-	*/
						/* ende Daten vom Modem bearbeiten.						*/

 					rc = 0;
					i  = 2;

					while (i < objc)
					{
						arg = Tcl_GetStringFromObj(objv[i], NULL);

						if ((rc = voice_play(arg)) != 0) break;

						i++;
					}

					switch (rc)
					{
						case 0:
							Tcl_SetResult(intp, "OK", NULL);
							break;

						case 1:
							Tcl_SetResult(intp, voice_touchtone_sequence, NULL);
							break;

						case 2:
							Tcl_SetResult(intp, "SUSPEND", NULL);
							break;

						default:
							Tcl_SetResult(intp, "HANGUP", NULL);
							break;
					}
				}
				break;

				case 'R':
				case 'r':
				{
						/* Speicherung der Audiodaten starten/stoppen	*/
						/* (record). In diesem Fall wird ERROR bei ei-	*/
						/* nem Fehler oder OK zurückgegeben.				*/

					if (strcasecmp(arg,  "stop") == 0) rc = voice_save(0);
					if (strcasecmp(arg, "start") == 0) rc = voice_save(1);

					switch (rc)
					{
						case 0:
							Tcl_SetResult(intp, "OK", NULL);
							break;

						default:
							Tcl_SetResult(intp, "ERROR", NULL);
							break;
					}
				}
				break;

				case 'A':
				case 'a':
				{
						/* Eingehende Audiodaten zum mithören an ein	*/
						/* anderes Device schicken.						*/

					if (strcasecmp(arg,  "stop") == 0) rc = voice_hear(0);
					if (strcasecmp(arg, "start") == 0) rc = voice_hear(1);

					switch (rc)
					{
						case 0:
							Tcl_SetResult(intp, "OK", NULL);
							break;

						default:
							Tcl_SetResult(intp, "ERROR", NULL);
							break;
					}
				}
				break;

				default:
					Tcl_SetResult(intp, "ERROR", NULL);
					break;
			}
		}
	}

	return(TCL_OK);
}
















int vbox_breaklist(VBOX_TCLFUNC)
{
	unsigned char *cmd;
	unsigned char *arg;
	int	rc;
	char *rs;

	if (objc == 2)
	{
		if ((cmd = Tcl_GetStringFromObj(objv[1], NULL)))
		{
			switch (*cmd)
			{
				case 'c':
				case 'C':
					breaklist_clear();
					break;

				case 'd':
					breaklist_dump();
					break;

				default:
					log(LOG_W, "Usage: vbox_breaklist <clear|dump>", cmd);
					break;
			}

			Tcl_SetResult(intp, "OK", NULL);
		}

		return(TCL_OK);
	}

	if (objc >= 3)
	{
		cmd = Tcl_GetStringFromObj(objv[1], NULL);
		arg = Tcl_GetStringFromObj(objv[2], NULL);

		if ((cmd) && (arg))
		{
			switch (*cmd)
			{
				case 'A':
				case 'a':
				{
					rs = breaklist_add(arg);

					Tcl_SetResult(intp, (rs ? "OK" : "ERROR"), NULL);
				}
				break;

				case 'R':
				case 'r':
				{
					rc = breaklist_del(arg);

					Tcl_SetResult(intp, (rc == 0 ? "OK" : "ERROR"), NULL);
				}
				break;

				default:
				{
					log(LOG_W, "Usage: vbox_breaklist <add|remove> <sequence>\n");

					Tcl_SetResult(intp, "ERROR", NULL);
				}
				break;
			}
		}
	}

	return(TCL_OK);
}











