#include <unistd.h>
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "Net.h"
#include <X11/Xaw/Cardinals.h>

extern void exit();
static void quit();

/* Only allow calling scripts when setuid root if the -r option is given */
int allow_setuid = 0;

char *ProgramName;

static XrmOptionDescRec options[] = {
{ "-update", "*netstat.update", XrmoptionSepArg, (caddr_t) NULL },
{ "-file",   "*netstat.file", XrmoptionSepArg, (caddr_t) NULL },
{ "-shape",  "*netstat.shapeWindow", XrmoptionNoArg, (caddr_t) "on" },
{ "-volume", "*netstat.volume", XrmoptionSepArg, (caddr_t) NULL },
};

static XtActionsRec xmonisdn_actions[] = {
    { "quit", quit },
};
static Atom wm_delete_window;

static void Usage ()
{
    static char *help_message[] = {
"where options include:",
"    -display host:dpy              X server to contact",
"    -geometry geom                 size of Netstat",
"    -file file                     isdninfo-file to watch",
"    -update 1/10 of a second      how often to check",
"    -bg color                      background color",
"    -fg color                      foreground color",
"    -rv                            reverse video",
"    -volume percentage             how loud to ring the bell",
"    -shape                         shape the window",
NULL};
    char **cpp;

    fprintf (stderr, "usage:  %s [-options ...]\n", ProgramName);
    for (cpp = help_message; *cpp; cpp++) {
	fprintf (stderr, "%s\n", *cpp);
    }
    fprintf (stderr, "\n");
    exit (1);
}


int main (argc, argv)
    int argc;
    char **argv;
{
    XtAppContext xtcontext;
    Widget toplevel, w;

    ProgramName = argv[0];

    /* check for first arg `-r'. Must be first! */
    if (argc > 1 && !strcmp(argv[1], "-r")) {
	int i;
	allow_setuid = 1;
	argc--;
	for (i = 1; i < argc; i++)
	        argv[i] = argv[i+1];
	argv[i] = 0;
    }
    else {
	if (geteuid() == 0 && getuid() != 0) {
	    fprintf(stderr,
		    "`%s' setuid root not allowed! Read the manpage to find out why.\n",
		    ProgramName);
	    exit(1);
	}
    }

    toplevel = XtAppInitialize(&xtcontext, "xmonisdn", options, XtNumber (options),
			       &argc, argv, NULL, NULL, 0);
    if (argc != 1) Usage ();

    /*
     * This is a hack so that f.delete will do something useful in this
     * single-window application.
     */
    XtAppAddActions (xtcontext, xmonisdn_actions, XtNumber(xmonisdn_actions));
    XtOverrideTranslations(toplevel,
		   XtParseTranslationTable ("<Message>WM_PROTOCOLS: quit()"));

    w = XtCreateManagedWidget ("netstat", netstatWidgetClass, toplevel,
			       NULL, 0);
    XtRealizeWidget (toplevel);
    wm_delete_window = XInternAtom (XtDisplay(toplevel), "WM_DELETE_WINDOW",
                                    False);
    (void) XSetWMProtocols (XtDisplay(toplevel), XtWindow(toplevel),
                            &wm_delete_window, 1);
    XtAppMainLoop (xtcontext);

    return 0;
}

static void quit (w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    if (event->type == ClientMessage &&
        event->xclient.data.l[0] != wm_delete_window) {
        XBell (XtDisplay(w), 0);
        return;
    }
    XCloseDisplay (XtDisplay(w));
    exit (0);
}
