/* based on: $XConsortium: xload.c,v 1.37 94/04/17 20:43:44 converse Exp $ */
/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*
 * xisdnload - display ISDN load average in a window
 *
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <linux/isdn.h>

#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/StripChart.h>
#include <X11/Xmu/SysUtil.h>

#ifdef REGEX_NUMBER
#include <regex.h>
#endif

#include "xisdnload.bit"

char *ProgramName;

static void quit();

/*
 * Definition of the Application resources structure.
 */

typedef struct _XISDNLoadResources {
  Boolean show_label;
  char *online_color;
  char *active_color;
  char *trying_color;
  char *activate;
  char *deactivate;
#ifdef REGEX_NUMBER
  char *number;
#endif
} XISDNLoadResources;

/*
 * Command line options table.  Only resources are entered here...there is a
 * pass over the remaining options after XtParseCommand is let loose.
 */

static XrmOptionDescRec options[] = {
 {"-scale",		"*load.minScale",	XrmoptionSepArg,	NULL},
 {"-update",		"*load.update",		XrmoptionSepArg,	NULL},
 {"-hl",		"*load.highlight",	XrmoptionSepArg,	NULL},
 {"-highlight",		"*load.highlight",	XrmoptionSepArg,	NULL},
 {"-label",		"*label.label",		XrmoptionSepArg,	NULL},
 {"-nolabel",		"*showLabel",	        XrmoptionNoArg,       "False"},
 {"-jumpscroll",	"*load.jumpScroll",	XrmoptionSepArg,	NULL},
 {"-online",		"*onlineColor",         XrmoptionSepArg,	NULL},
 {"-trying",		"*tryingColor",         XrmoptionSepArg,	NULL},
 {"-active",		"*activeColor",         XrmoptionSepArg,	NULL},
 {"-activate",		"*activate",		XrmoptionSepArg,	NULL},
 {"-deactivate",	"*deactivate",		XrmoptionSepArg,	NULL},
#ifdef REGEX_NUMBER
 {"-number",		"*number",		XrmoptionSepArg,	NULL},
#endif
};

/*
 * The structure containing the resource information for the
 * Xisdnload application resources.
 */

#define Offset(field) (XtOffsetOf(XISDNLoadResources, field))

static XtResource my_resources[] = {
  {"showLabel", XtCBoolean, XtRBoolean, sizeof(Boolean),
     Offset(show_label), XtRImmediate, (XtPointer) TRUE},
  {"onlineColor", "OnlineColor", XtRString, sizeof(char *),
     Offset(online_color), XtRString, NULL},
  {"activeColor", "ActiveColor", XtRString, sizeof(char *),
     Offset(active_color), XtRString, NULL},
  {"tryingColor", "TryingColor", XtRString, sizeof(char *),
     Offset(trying_color), XtRString, NULL},
  {"activate", "Activate", XtRString, sizeof(char *),
     Offset(activate), XtRString, NULL},
  {"deactivate", "Deactivate", XtRString, sizeof(char *),
     Offset(deactivate), XtRString, NULL},
#ifdef REGEX_NUMBER
  {"number", "Number", XtRString, sizeof(char *),
     Offset(number), XtRString, NULL},
#endif
};

#undef Offset

static XISDNLoadResources resources;

static XtActionsRec xisdnload_actions[] = {
    { "quit",	quit },
};
static Atom wm_delete_window;

typedef struct {
  unsigned long ibytes;
  unsigned long obytes;
} Siobytes;

static Siobytes iobytes[ISDN_MAX_CHANNELS];
static Pixel onlinecolor, activecolor, tryingcolor, bgcolor;
static long last[ISDN_MAX_CHANNELS];
static int usageflags[ISDN_MAX_CHANNELS];
static int flags[ISDN_MAX_DRIVERS];
static char phone[ISDN_MAX_CHANNELS][20];
static int fd_isdninfo;
static Widget label_wid;
static char label_format[80];
static int online_now, online_last;
static int trying_now, trying_last;
static struct timeval tv_start, tv_last;
static long bytes_last, bytes_total, bytes_now;
static int secs_running;
static char num[100], history[160];
#ifdef REGEX_NUMBER
static regex_t preg;
#endif

/*
 * Exit with message describing command line format.
 */

void usage()
{
    fprintf (stderr, "usage:  %s [-options ...]\n\n", ProgramName);
    fprintf (stderr, "where options include:\n");
    fprintf (stderr,
      "    -display dpy            X server on which to display\n");
    fprintf (stderr,
      "    -geometry geom          size and location of window\n");
    fprintf (stderr,
      "    -fn font                font to use in label\n");
    fprintf (stderr,
      "    -scale number           minimum number of scale lines\n");
    fprintf (stderr,
      "    -update seconds         interval between updates\n");
    fprintf (stderr,
      "    -label string           annotation text\n");
    fprintf (stderr,
      "    -bg color               background color\n");
    fprintf (stderr,
      "    -fg color               graph color\n");
    fprintf (stderr,
      "    -hl color               scale and text color\n");
    fprintf (stderr,
      "    -online color           background color when online\n");
    fprintf (stderr,
      "    -active color           background color when active for demand dialing\n");
    fprintf (stderr,
      "    -nolabel                removes the label from above the chart.\n");
    fprintf (stderr,
      "    -jumpscroll value       number of pixels to scroll on overflow\n");
    fprintf (stderr,
      "    -activate string        exec this to activate demand dialing\n");
    fprintf (stderr,
      "    -deactivate string      exec this to deactivate demand dialing\n");
#ifdef REGEX_NUMBER
    fprintf (stderr,
      "    -number string          regexp to match against number to watch\n");
#endif
    fprintf (stderr, "\n");
    exit(1);
}



int get_active()
{
  static char buf[8192];
  int l;
  int fd_route;
  int res = 0;

  fd_route = open("/proc/net/route", O_RDONLY | O_NDELAY);
  if (fd_route < 0) {
    perror("/proc/net/route");
    exit(1);
  }
  if ((l = read(fd_route, buf, sizeof(buf))) > 0) {
    buf[l] = 0;
    if (strstr(buf, "isdn") || strstr(buf, "ippp"))
      res = 1;
  }
  close(fd_route);

  return res;
}



void
InitLoadPoint()
{
  int i;

  fd_isdninfo = open("/dev/isdninfo", O_RDONLY | O_NDELAY);
  if (fd_isdninfo < 0) {
    perror("can't open /dev/isdninfo");
    exit(1);
  }

  /*
   * Read the current iobytes values and count the bytes as the count at
   * "last time"; starting xisdnload when there has been a connection for some
   * time otherwise leads to a huge initial spike, and the rest is scaled doen
   * to effectively zero until the spike scrolls off. This should prevent that.
   */
  if (ioctl(fd_isdninfo, IIOCGETCPS, &iobytes))
    perror("Can't get bytes transferred:");

  bytes_last = 0;
  for (i = 0; i < ISDN_MAX_CHANNELS; i++) {
    bytes_last += iobytes[i].ibytes + iobytes[i].obytes;
    iobytes[i].ibytes = 0;
    iobytes[i].obytes = 0;
    strcpy(phone[i], "???");
    last[i] = 0;
    usageflags[i] = 0;
  }
  online_last = 0;
  online_now = -1;
  trying_last = -1;
  trying_now = -1;
  gettimeofday(&tv_last, NULL);
  tv_last.tv_sec--; /* avoid devision by zero */
  tv_start = tv_last;
  bytes_total = 0;
  bytes_now = 0;
  secs_running = 0;
  strcpy(num, "");
}



void
GetLoadPoint( w, closure, call_data )
Widget	w;
XtPointer closure;
XtPointer call_data;	/* pointer to (double) return value */
{
  double *loadavg = (double *)call_data;
  double cps = 0.0;
  int idx;
  int get_iobytes;
  char buf[4096];
  char s[120];
  char f[80];
  time_t t;
  struct timeval tv_now;
  struct tm *tm;
  char now[20];
  long bytes_delta;
  int secs_delta;
  Arg args[1];

  gettimeofday(&tv_now, NULL);
  secs_delta = (tv_now.tv_sec + tv_now.tv_usec / 1000000) -
    (tv_last.tv_sec + tv_last.tv_usec / 1000000);
  tv_last = tv_now;

  if (read(fd_isdninfo, buf, sizeof(buf)) > 0) {
    sscanf(strstr(buf, "usage:"),
         "usage: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
         &usageflags[0], &usageflags[1], &usageflags[2], &usageflags[3],
         &usageflags[4], &usageflags[5], &usageflags[6], &usageflags[7],
         &usageflags[8], &usageflags[9], &usageflags[10], &usageflags[11],
	   &usageflags[12], &usageflags[13], &usageflags[14], &usageflags[15]);
    sscanf(strstr(buf, "flags:"),
         "flags: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
         &flags[0], &flags[1], &flags[2], &flags[3],
         &flags[4], &flags[5], &flags[6], &flags[7],
         &flags[8], &flags[8], &flags[10], &flags[11],
         &flags[12], &flags[13], &flags[14], &flags[15]);
    sscanf(strstr(buf, "phone:"),
         "phone: %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
         phone[0], phone[1], phone[2], phone[3],
         phone[4], phone[5], phone[6], phone[7],
         phone[8], phone[8], phone[10], phone[11],
         phone[12], phone[13], phone[14], phone[15]);

  }
  get_iobytes = 1;
  strcpy(num, "");
  for (online_now = 0, trying_now = 0, bytes_now = 0, idx = 0; idx < 16; idx++) {
    if (usageflags[idx]) {
#ifdef REGEX_NUMBER
      if (!regexec(&preg, phone[idx], 0, NULL, 0)) {  
#endif
        if (flags[idx/2]) {
	  online_now++;
	  if (get_iobytes) {
            if (ioctl(fd_isdninfo, IIOCGETCPS, &iobytes))
              perror("Can't get bytes transferred:");
	    get_iobytes = 0;
	  }
	  bytes_now += iobytes[idx].ibytes + iobytes[idx].obytes;
	  if (!strlen(num)) {
            strcpy(num, phone[idx]);
	  } else {
	    strcat(num, " ");
	    strcat(num, phone[idx]);
          }
        } else {
          trying_now++;
	  if (!strlen(num)) {
	    strcpy(num, "(");
	    strcat(num, phone[idx]);
	    strcat(num, ")");
	  } else {
	    strcat(num, " (");
	    strcat(num, phone[idx]);
	    strcat(num, ")");
          }
        }
#ifdef REGEX_NUMBER
      }
#endif
    }
  }

  if (!online_now)
    online_now = - get_active();

  bytes_delta = bytes_now - bytes_last;
  bytes_last = bytes_now;

  if (online_now >= 1) {
    secs_running = (tv_now.tv_sec + tv_now.tv_usec / 1000000) -
      (tv_start.tv_sec + tv_start.tv_usec / 1000000);
    cps = (double)bytes_delta / (double)secs_delta;
    if (cps < 0.0) cps = 0.0;
    /* if (cps > 8000.0) cps = 8000.0; */
    bytes_total = bytes_now;
    if (online_last < 1) {
      t = time(NULL);
      tm = localtime(&t);
      sprintf(now, "%.2d:%.2d:%.2d", tm->tm_hour, tm->tm_min, tm->tm_sec);
      strcpy(history, now);
      strcat(history, "-");
      tv_start = tv_now;
      XtSetArg(args[0], XtNbackground, trying_now ? tryingcolor : onlinecolor);
      XtSetValues(w, args, 1);
    }
    if (resources.show_label) {
      t = time(NULL);
      tm = localtime(&t);
      sprintf(now, "%.2d:%.2d:%.2d", tm->tm_hour, tm->tm_min, tm->tm_sec);
      sprintf(f, "%s%%s", label_format);
      sprintf(s, f, num,
	      secs_running / 60, secs_running % 60, cps,
	      bytes_total / 1024, history, now);
      XtSetArg (args[0], XtNlabel, s);
      XtSetValues (label_wid, args, ONE);
    }
  } else if ((online_now != online_last) || (trying_now != trying_last)) {
    if (online_last >= 1) {
      if (resources.show_label) {
	if (secs_running > 0) {
	  t = time(NULL);
	  tm = localtime(&t);
	  sprintf(now, "%.2d:%.2d:%.2d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	  strcat(history, now);
	  strcat(history, " ");
	  sprintf(s, label_format, "offline",
		  secs_running / 60, secs_running % 60,
		  (double)bytes_total / (double)secs_running,
		  bytes_total / 1024, history);
	} else {
	  sprintf(s, "uninitialized %s", history);
	}
	XtSetArg (args[0], XtNlabel, s);
	XtSetValues (label_wid, args, ONE);
      }
    }
    if (online_now == 0) {
      XtSetArg(args[0], XtNbackground, trying_now ? tryingcolor : bgcolor);
      XtSetValues(w, args, 1);
    } else {
      XtSetArg(args[0], XtNbackground, trying_now ? tryingcolor : activecolor);
      XtSetValues(w, args, 1);
    }
  }

  online_last = online_now;
  trying_last = trying_now;
  *loadavg = cps / 1000.0; /* unit: 1000Bytes/sec */

}



void ToggleActive(Widget w, XtPointer p, XEvent *e, Boolean *c)
{
  if (e->type == ButtonPress) {
    if (get_active()) {
      system(resources.deactivate);
    } else {
      system(resources.activate);
    }
  }
}



int main(argc, argv)
    int argc;
    char **argv;
{
    XtAppContext app_con;
    Widget toplevel, load, pane, load_parent;
    Arg args[1];
    Pixmap icon_pixmap = None;
    char *label;
    XrmValue namein, pixelout;
    time_t t;
    struct tm *tm;
    char now[20];
    int a;

    t = time(NULL);
    tm = localtime(&t);
    sprintf(now, "%.2d:%.2d:%.2d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    sprintf(history, "(%s) ", now);

    ProgramName = argv[0];

    /* For security reasons, we reset our uid/gid after doing the necessary
       system initialization and before calling any X routines. */
    InitLoadPoint();
    setgid(getgid());		/* reset gid first while still (maybe) root */
    setuid(getuid());

    toplevel = XtAppInitialize(&app_con, "XISDNLoad", options,
			       XtNumber(options),
			       &argc, argv, NULL, NULL, (Cardinal) 0);
    if (argc != 1) usage();

    XtGetApplicationResources( toplevel, (XtPointer) &resources,
			      my_resources, XtNumber(my_resources),
			      NULL, (Cardinal) 0);

#ifdef REGEX_NUMBER
    if (resources.number) {
      a = regcomp(&preg, resources.number, REG_EXTENDED);
    } else {
      a = regcomp(&preg, "", REG_EXTENDED);
    }
    if (a) {
      fprintf(stderr, "illegal number regexp `%s'.\n", resources.number);
      exit(1);
    }
#endif
    
    /*
     * This is a hack so that f.delete will do something useful in this
     * single-window application.
     */
    XtAppAddActions (app_con, xisdnload_actions, XtNumber(xisdnload_actions));
    XtOverrideTranslations(toplevel,
		   XtParseTranslationTable ("<Message>WM_PROTOCOLS: quit()"));

    XtSetArg (args[0], XtNiconPixmap, &icon_pixmap);
    XtGetValues(toplevel, args, ONE);
    if (icon_pixmap == None) {
      XtSetArg(args[0], XtNiconPixmap,
	       XCreateBitmapFromData(XtDisplay(toplevel),
				     XtScreen(toplevel)->root,
				     (char *)xisdnload_bits,
				     xisdnload_width, xisdnload_height));
      XtSetValues (toplevel, args, ONE);
    }

    if (resources.show_label) {
      pane = XtCreateManagedWidget ("paned", panedWidgetClass,
				    toplevel, NULL, ZERO);

      label_wid = XtCreateManagedWidget ("label", labelWidgetClass,
					 pane, NULL, ZERO);

      XtSetArg (args[0], XtNlabel, &label);
      XtGetValues(label_wid, args, ONE);

      strcpy(label_format, label);

      sprintf(now, "uninitialized %s", history);
      XtSetArg (args[0], XtNlabel, now);
      XtSetValues (label_wid, args, ONE);

      load_parent = pane;
    }
    else
      load_parent = toplevel;

    load = XtCreateManagedWidget ("load", stripChartWidgetClass,
				  load_parent, NULL, ZERO);

    XtSetArg (args[0], XtNbackground, &bgcolor);
    XtGetValues(toplevel, args, ONE);

    if (resources.online_color) {
      namein.addr = resources.online_color;
      namein.size = strlen(resources.online_color) + 1;
      XtConvert(load, XtRString, &namein, XtRPixel, &pixelout);
      if (!pixelout.addr) {
	fprintf(stderr, "could not convert online color `%s'.\n",
		resources.online_color);
        exit(1);
      }
      onlinecolor = *(Pixel*)(pixelout.addr);
    } else {
      onlinecolor = bgcolor;
    }

    if (resources.trying_color) {
      namein.addr = resources.trying_color;
      namein.size = strlen(resources.trying_color) + 1;
      XtConvert(load, XtRString, &namein, XtRPixel, &pixelout);
      if (!pixelout.addr) {
	fprintf(stderr, "could not convert trying color `%s'.\n",
		resources.online_color);
        exit(1);
      }
      tryingcolor = *(Pixel*)(pixelout.addr);
    } else {
      tryingcolor = bgcolor;
    }

    if (resources.active_color) {
      namein.addr = resources.active_color;
      namein.size = strlen(resources.active_color) + 1;
      XtConvert(load, XtRString, &namein, XtRPixel, &pixelout);
      if (!pixelout.addr) {
	fprintf(stderr, "could not convert active color `%s'.\n",
		resources.online_color);
        exit(1);
      }
      activecolor = *(Pixel*)(pixelout.addr);
    } else {
      activecolor = bgcolor;
    }

    XtAddCallback(load, XtNgetValue, GetLoadPoint, NULL);

    XtAddEventHandler(toplevel, ButtonPressMask, ButtonPressMask,
		      ToggleActive, NULL);
    
    XtRealizeWidget (toplevel);

    wm_delete_window = XInternAtom (XtDisplay(toplevel), "WM_DELETE_WINDOW",
				    False);
    (void) XSetWMProtocols (XtDisplay(toplevel), XtWindow(toplevel),
			    &wm_delete_window, 1);

    XtAppMainLoop(app_con);

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
    XtDestroyApplicationContext(XtWidgetToApplicationContext(w));
    exit (0);
}
