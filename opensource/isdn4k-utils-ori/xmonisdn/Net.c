#include <X11/IntrinsicP.h>		/* for toolkit stuff */
#include <X11/StringDefs.h>		/* for useful atom names */
#include <X11/cursorfont.h>		/* for cursor constants */
#include <X11/Xosdefs.h>		/* for X_NOT_POSIX def */
#include <pwd.h>			/* for getting username */
#include <sys/stat.h>			/* for stat() ** needs types.h ***/
#include <unistd.h>
#include <stdio.h>			/* for printing error messages */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <linux/isdn.h>

#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
# include <sys/wait.h>
#else
#define _POSIX_SOURCE
# include <sys/wait.h>
#undef _POSIX_SOURCE
#endif
# define waitCode(w)	WEXITSTATUS(w)
# define waitSig(w)	WIFSIGNALED(w)
typedef int		waitType;
# define INTWAITTYPE
#else /* ! X_NOT_POSIX */
#ifdef SYSV
# define waitCode(w)	(((w) >> 8) & 0x7f)
# define waitSig(w)	((w) & 0xff)
typedef int		waitType;
# define INTWAITTYPE
#else
# include	<sys/wait.h>
# define waitCode(w)	((w).w_T.w_Retcode)
# define waitSig(w)	((w).w_T.w_Termsig)
typedef union wait	waitType;
#endif /* SYSV else */
#endif /* ! X_NOT_POSIX else */

#include <X11/bitmaps/netactive>
#include <X11/bitmaps/netinactive>
#include <X11/bitmaps/netwaiting>
#include <X11/bitmaps/netactiveout>
#include <X11/bitmaps/netstart>
#include <X11/bitmaps/netstop>

#include <X11/Xaw/XawInit.h>
#include "NetP.h"
#include <X11/Xmu/Drawing.h>
#include <X11/extensions/shape.h>

/* Only allow calling scripts when setuid root if the -r option is given */
extern int allow_setuid;
/*
 * The default user interface is to have the netstat turn itself off whenever
 * the user presses a button in it.  Expert users might want to make this 
 * happen on EnterWindow.  It might be nice to provide support for some sort of
 * exit callback so that you can do things like press q to quit.
 */

static char defaultTranslations[] = 
  "<ButtonPress>Button2:  netup()\n\
   <ButtonPress>Button3:  netdown()";

static void Check(), NetUp(), NetDown();

static XtActionsRec actionsList[] = { 
    { "check",	Check },
    { "netup",	NetUp },
    { "netdown",NetDown },
};


/* Initialization of defaults */

#define offset(field) XtOffsetOf(NetstatRec, netstat.field)
#define goffset(field) XtOffsetOf(WidgetRec, core.field)

static Dimension defDim = 48;
static Pixmap nopix = None;

static XtResource resources[] = {
    { XtNwidth, XtCWidth, XtRDimension, sizeof (Dimension), 
	goffset (width), XtRDimension, (XtPointer)&defDim },
    { XtNheight, XtCHeight, XtRDimension, sizeof (Dimension),
	goffset (height), XtRDimension, (XtPointer)&defDim },
    { XtNupdate, XtCInterval, XtRInt, sizeof (int),
	offset (update), XtRString, "5" },
    { XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
	offset (foreground_pixel), XtRString, XtDefaultForeground },
    { XtNfile, XtCFile, XtRString, sizeof (String),
	offset (filename), XtRString, NULL },
    { XtNvolume, XtCVolume, XtRInt, sizeof(int),
	offset (volume), XtRString, "33"},
    { XtNactivePixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
	offset (active.bitmap), XtRString, "netactive" },
    { XtNactivePixmapMask, XtCPixmapMask, XtRBitmap, sizeof(Pixmap),
	offset (active.mask), XtRBitmap, (XtPointer) &nopix },
    { XtNactiveoutPixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
	offset (activeout.bitmap), XtRString, "netactiveout" },
    { XtNactiveoutPixmapMask, XtCPixmapMask, XtRBitmap, sizeof(Pixmap),
	offset (activeout.mask), XtRBitmap, (XtPointer) &nopix },
    { XtNwaitingPixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
	offset (waiting.bitmap), XtRString, "netwaiting" },
    { XtNwaitingPixmapMask, XtCPixmapMask, XtRBitmap, sizeof(Pixmap),
	offset (waiting.mask), XtRBitmap, (XtPointer) &nopix },
    { XtNinactivePixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
	offset (inactive.bitmap), XtRString, "netinactive" },
    { XtNinactivePixmapMask, XtCPixmapMask, XtRBitmap, sizeof(Pixmap),
	offset (inactive.mask), XtRBitmap, (XtPointer) &nopix },
    { XtNstartPixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
	offset (start.bitmap), XtRString, "netstart" },
    { XtNstartPixmapMask, XtCPixmapMask, XtRBitmap, sizeof(Pixmap),
	offset (start.mask), XtRBitmap, (XtPointer) &nopix },
    { XtNstopPixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
	offset (stop.bitmap), XtRString, "netstop" },
    { XtNstopPixmapMask, XtCPixmapMask, XtRBitmap, sizeof(Pixmap),
	offset (stop.mask), XtRBitmap, (XtPointer) &nopix },
    { XtNflip, XtCFlip, XtRBoolean, sizeof(Boolean),
	offset (flipit), XtRString, "true" },
    { XtNshapeWindow, XtCShapeWindow, XtRBoolean, sizeof(Boolean),
        offset (shapeit), XtRString, "false" },
};

#undef offset
#undef goffset

static void GetNetinfoFile(), CloseDown();
static void check_netstat(), redraw_netstat(), beep();
static void Initialize(), Realize(), Destroy(), Redisplay();
static Boolean SetValues();
static int  parse_info();

NetstatClassRec netstatClassRec = {
    { /* core fields */
    /* superclass		*/	(WidgetClass) &simpleClassRec,
    /* class_name		*/	"Netstat",
    /* widget_size		*/	sizeof(NetstatRec),
    /* class_initialize		*/	XawInitializeWidgetSet,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	Realize,
    /* actions			*/	actionsList,
    /* num_actions		*/	XtNumber(actionsList),
    /* resources		*/	resources,
    /* resource_count		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	Destroy,
    /* resize			*/	NULL,
    /* expose			*/	Redisplay,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	defaultTranslations,
    /* query_geometry		*/	XtInheritQueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
    },
    { /* simple fields */
    /* change_sensitive         */	XtInheritChangeSensitive
    },
    { /* netstat fields */
    /* ignore                   */	0
    }
};

WidgetClass netstatWidgetClass = (WidgetClass) &netstatClassRec;

static FILE *isdninfo;
static int updownwait = 0;

/*
 * widget initialization
 */

static GC get_netstat_gc (w)
    NetstatWidget w;
{
    XtGCMask valuemask;
    XGCValues xgcv;

    valuemask = GCForeground | GCBackground | GCFunction | GCGraphicsExposures;
    xgcv.foreground = w->netstat.foreground_pixel;
    xgcv.background = w->core.background_pixel;
    xgcv.function = GXcopy;
    xgcv.graphics_exposures = False;	/* this is Bool, not Boolean */
    return (XtGetGC ((Widget) w, valuemask, &xgcv));
}


/* ARGSUSED */
static void Initialize (request, new, args, num_args)
    Widget request, new;
    ArgList args;
    Cardinal *num_args;
{
    NetstatWidget w = (NetstatWidget) new;
    int shape_event_base, shape_error_base;

    if (w->core.width <= 0) w->core.width = 1;
    if (w->core.height <= 0) w->core.height = 1;

    if (w->netstat.shapeit && !XShapeQueryExtension (XtDisplay (w),
						     &shape_event_base,
						     &shape_error_base))
    w->netstat.shapeit = False;
    w->netstat.shape_cache.mask = None;
    w->netstat.gc = get_netstat_gc (w);
    w->netstat.interval_id = (XtIntervalId) 0;
    w->netstat.active.pixmap = None;
    w->netstat.inactive.pixmap = None;
    w->netstat.waiting.pixmap = None;
    w->netstat.activeout.pixmap = None;
    w->netstat.start.pixmap = None;
    w->netstat.stop.pixmap = None;
    w->netstat.state = 0;
    if (!w->netstat.filename) GetNetinfoFile (w);
    return;
}


/*
 * action procedures
 */

/*
 * paranoia_check - enhanced security.
 *
 * refuse execution, if cmd is NOT owned by
 * root or writeable by group or world.
 *
 */

/* ARGSUSED */
static int paranoia_check(cmd)
    char *cmd;
{
#ifdef PARANOIA_CHECK
    struct stat stbuf;

    /* only check if setuid root */
    if (geteuid() != 0 || getuid() == 0)
	return 1;

    /* don't allow any scripts if setuid root and not called with -r option */
    if (!allow_setuid)
	return 0;

    if (stat(cmd, &stbuf))
      /* stat failed, stay on the safe side */
	return 0;
    if (stbuf.st_uid != 0)
      /* owner is not root */
	return 0;
    if (stbuf.st_mode & (S_IWGRP | S_IWOTH))
      /* writable by group or world */
	return 0;
#endif
    return 1;
}

/* ARGSUSED */
static void NetUp (gw, event, params, nparams)
    Widget gw;
    XEvent *event;
    String *params;
    Cardinal *nparams;
{
    NetstatWidget w = (NetstatWidget) gw;

    
    if ((w->netstat.state <= 1) && (!updownwait)) {
      w->netstat.state = 5;
      updownwait = 150 / w->netstat.update;
      if (updownwait < 2)
	  updownwait = 2;
      redraw_netstat(w);
      if (paranoia_check(NETUP_COMMAND))
      	  system(NETUP_COMMAND);
    }
    return;
}



/* ARGSUSED */
static void NetDown (gw, event, params, nparams)
    Widget gw;
    XEvent *event;
    String *params;
    Cardinal *nparams;
{
    NetstatWidget w = (NetstatWidget) gw;

    if ((w->netstat.state > 1) && (w->netstat.state < 5) && (!updownwait)) {
      w->netstat.state = 6;
      redraw_netstat(w);
      updownwait = 150 / w->netstat.update;
      if (updownwait < 2)
	  updownwait = 2;
      if (paranoia_check(NETDOWN_COMMAND))
          system(NETDOWN_COMMAND);
    }
    return;
}



/* ARGSUSED */
static void Check (gw, event, params, nparams)
    Widget gw;
    XEvent *event;
    String *params;
    Cardinal *nparams;
{
    NetstatWidget w = (NetstatWidget) gw;

    if (!updownwait) {
      w->netstat.state = 0;
      fclose(isdninfo);
      check_netstat (w);
    }

    return;
}


/* ARGSUSED */
static void clock_tic (client_data, id)
    XtPointer client_data;
    XtIntervalId *id;
{
    NetstatWidget w = (NetstatWidget) client_data;

    check_netstat (w);

    /*
     * reset the timer
     */

    w->netstat.interval_id =
	XtAppAddTimeOut (XtWidgetToApplicationContext((Widget) w),
			 w->netstat.update * 100, clock_tic, client_data);

    return;
}

static Pixmap make_pixmap (dpy, w, bitmap, depth, flip, widthp, heightp)
    Display *dpy;
    NetstatWidget w;
    Pixmap bitmap;
    Boolean flip;
    int depth;
    int *widthp, *heightp;
{
    Window root;
    int x, y;
    unsigned int width, height, bw, dep;
    unsigned long fore, back;

    if (!XGetGeometry (dpy, bitmap, &root, &x, &y, &width, &height, &bw, &dep))
      return None;

    *widthp = (int) width;
    *heightp = (int) height;
    if (flip) {
	fore = w->core.background_pixel;
	back = w->netstat.foreground_pixel;
    } else {
	fore = w->netstat.foreground_pixel;
	back = w->core.background_pixel;
    }
    return XmuCreatePixmapFromBitmap (dpy, w->core.window, bitmap, 
				      width, height, depth, fore, back);
}

static void Realize (gw, valuemaskp, attr)
    Widget gw;
    XtValueMask *valuemaskp;
    XSetWindowAttributes *attr;
{
    NetstatWidget w = (NetstatWidget) gw;
    register Display *dpy = XtDisplay (w);
    int depth = w->core.depth;

    *valuemaskp |= (CWBitGravity | CWCursor);
    attr->bit_gravity = ForgetGravity;
    attr->cursor = XCreateFontCursor (dpy, XC_top_left_arrow);

    (*netstatWidgetClass->core_class.superclass->core_class.realize)
	(gw, valuemaskp, attr);

    /*
     * build up the pixmaps that we'll put into the image
     */
    if (w->netstat.active.bitmap == None) {
	w->netstat.active.bitmap = 
	  XCreateBitmapFromData (dpy, w->core.window, (char *) netactive_bits,
				 netactive_width, netactive_height);
    }
    if (w->netstat.inactive.bitmap == None) {
	w->netstat.inactive.bitmap =
	  XCreateBitmapFromData (dpy, w->core.window, (char *) netinactive_bits,
				 netinactive_width, netinactive_height);
    }
    if (w->netstat.waiting.bitmap == None) {
	w->netstat.waiting.bitmap =
	  XCreateBitmapFromData (dpy, w->core.window, (char *) netwaiting_bits,
				 netwaiting_width, netwaiting_height);
    }
    if (w->netstat.activeout.bitmap == None) {
	w->netstat.activeout.bitmap =
	  XCreateBitmapFromData (dpy, w->core.window, (char *) netactiveout_bits,
				 netactiveout_width, netactiveout_height);
    }

    if (w->netstat.start.bitmap == None) {
	w->netstat.start.bitmap =
	  XCreateBitmapFromData (dpy, w->core.window, (char *) netstart_bits,
				 netstart_width, netstart_height);
    }

    if (w->netstat.stop.bitmap == None) {
	w->netstat.stop.bitmap =
	  XCreateBitmapFromData (dpy, w->core.window, (char *) netstop_bits,
				 netstop_width, netstop_height);
    }

    w->netstat.inactive.pixmap = make_pixmap (dpy, w, w->netstat.inactive.bitmap,
					   depth, False,
					   &w->netstat.inactive.width,
					   &w->netstat.inactive.height);
    w->netstat.active.pixmap = make_pixmap (dpy, w, w->netstat.active.bitmap,
					  depth, False,
					  &w->netstat.active.width,
					  &w->netstat.active.height);
			 
    w->netstat.waiting.pixmap = make_pixmap (dpy, w, w->netstat.waiting.bitmap,
					  depth, False,
					  &w->netstat.waiting.width,
					  &w->netstat.waiting.height);

    w->netstat.activeout.pixmap = make_pixmap (dpy, w, w->netstat.activeout.bitmap,
					  depth, w->netstat.flipit,
					  &w->netstat.activeout.width,
					  &w->netstat.activeout.height);
			 
    w->netstat.start.pixmap = make_pixmap (dpy, w, w->netstat.start.bitmap,
					  depth, False,
					  &w->netstat.start.width,
					  &w->netstat.start.height);

    w->netstat.stop.pixmap = make_pixmap (dpy, w, w->netstat.stop.bitmap,
					  depth, False,
					  &w->netstat.stop.width,
					  &w->netstat.stop.height);

    if (w->netstat.inactive.mask == None || w->netstat.active.mask == None ||
	w->netstat.waiting.mask == None || w->netstat.activeout.mask == None ||
	w->netstat.start.mask == None || w->netstat.stop.mask == None)
      w->netstat.shapeit = False;

    w->netstat.interval_id = 
	XtAppAddTimeOut (XtWidgetToApplicationContext((Widget) w),
			 w->netstat.update * 100, clock_tic, (XtPointer) w);

    w->netstat.shape_cache.mask = None;

    check_netstat (w);

    return;
}


static void Destroy (gw)
    Widget gw;
{
    NetstatWidget w = (NetstatWidget) gw;
    Display *dpy = XtDisplay (gw);

    XtFree (w->netstat.filename);
    if (w->netstat.interval_id) XtRemoveTimeOut (w->netstat.interval_id);
    XtReleaseGC(gw, w->netstat.gc);
#define freepix(p) if (p) XFreePixmap (dpy, p)
    freepix (w->netstat.active.bitmap);	
    freepix (w->netstat.active.mask);	
    freepix (w->netstat.active.pixmap);
    freepix (w->netstat.inactive.bitmap);
    freepix (w->netstat.inactive.mask);		
    freepix (w->netstat.inactive.pixmap);
    freepix (w->netstat.waiting.bitmap);
    freepix (w->netstat.waiting.mask);		
    freepix (w->netstat.waiting.pixmap);
    freepix (w->netstat.activeout.bitmap);
    freepix (w->netstat.activeout.mask);		
    freepix (w->netstat.activeout.pixmap);
    freepix (w->netstat.start.bitmap);
    freepix (w->netstat.start.mask);		
    freepix (w->netstat.start.pixmap);
    freepix (w->netstat.stop.bitmap);
    freepix (w->netstat.stop.mask);		
    freepix (w->netstat.stop.pixmap);
    freepix (w->netstat.shape_cache.mask);
#undef freepix
    return;
}


static void Redisplay (gw, event, region)
    Widget gw;
    XEvent *event;
    Region region;
{
    NetstatWidget w = (NetstatWidget) gw;

    check_netstat (w);
    redraw_netstat(w);
}


static void check_netstat (w)
    NetstatWidget w;
{
  int newstate = 0;
  struct timeval timeout;
  fd_set fdset;

  if (w->netstat.state >= 5) { /* network start or stop */
    if (updownwait <= 0) {
      fclose(isdninfo);
      w->netstat.state = 0;
      updownwait = 0;
    } else updownwait--;
  }

  if (w->netstat.state == 0) { /* first invocation */
    if (!w->netstat.filename) GetNetinfoFile();
      if (!(isdninfo = fopen(w->netstat.filename, "r"))) {
	fprintf(stderr, "xmonisdn: Can't open %s\n",w->netstat.filename);
	CloseDown(w,-1);
      }
  }
  FD_ZERO(&fdset);
  FD_SET(fileno(isdninfo),&fdset);
  timeout.tv_sec =  0;
  timeout.tv_usec = 200;
  switch (select(32,&fdset,(fd_set *)0,(fd_set *)0,&timeout)) {
  case 0: /* nothing changed */
    if (w->netstat.state == 0) {
      fprintf(stderr, "xmonisdn: No info from isdninfo-file after initial poll\n");
      CloseDown(w,-1);
    }
    return;    
  case 1: /* read new info */
    newstate = parse_info();
    break;
  default:
    fprintf(stderr,"xmonisdn: Error in select-statement\n");
    CloseDown(w,-1);
  }

  if (newstate == w->netstat.state) return;
  else {
    if (((w->netstat.state == 4) || (newstate == 4)) &&
	(w->netstat.state < 5) && (newstate < 5)) beep(w);
    w->netstat.state = newstate;
    updownwait = 0;
  }

  redraw_netstat(w);
}

static int parse_info()
{
  char infoline[4096];
  char temp[128];
  char *infoptr;
  register int i;
  int num;
  int newstate; 
  
  newstate = 1;
  fgets(infoline, 4095, isdninfo); /* idmap */
  fgets(infoline, 4095, isdninfo); /* chmap */
  infoptr = infoline + 7;
  for (i=0; i<ISDN_MAX_CHANNELS; i++) {
    sscanf(infoptr,"%d",&num);
    if (num != -1) newstate = 2;
    sscanf(infoptr,"%s",temp);
    infoptr = infoptr + strlen(temp) + 1;
  }
  fgets(infoline, 4095, isdninfo); /* drmap */
  fgets(infoline, 4095, isdninfo); /* usage */
  if (newstate >= 2) {
    infoptr = infoline + 7;
    for (i=0; i<ISDN_MAX_CHANNELS; i++) {
      sscanf(infoptr,"%d",&num);
      if ((num&ISDN_USAGE_MASK) == ISDN_USAGE_NET) {
	if (num&ISDN_USAGE_OUTGOING)
	   newstate = 4;
	else
	   if (newstate < 3)
	      newstate = 3;
      }
      sscanf(infoptr,"%s",temp);
      infoptr = infoptr + strlen(temp) + 1;
    }
  }
  fgets(infoline, 4095, isdninfo); /* flags */
  fgets(infoline, 4095, isdninfo); /* phone */

  return newstate;
}
  
static void GetNetinfoFile (w)
    NetstatWidget w;
{
    char *netinfo_file;
    struct stat stbuf;
    
    netinfo_file="/dev/isdn/isdninfo";  
    if (stat(netinfo_file, &stbuf)<0)
      netinfo_file="/dev/isdninfo";  
    
    w->netstat.filename = (String) XtMalloc (strlen (netinfo_file) + 1);
    strcpy (w->netstat.filename, netinfo_file);
    return;
}

static void CloseDown (w, status)
    NetstatWidget w;
    int status;
{
    Display *dpy = XtDisplay (w);

    XtDestroyWidget ((Widget)w);
    XCloseDisplay (dpy);
    exit (status);
}


/* ARGSUSED */
static Boolean SetValues (gcurrent, grequest, gnew, args, num_args)
    Widget gcurrent, grequest, gnew;
    ArgList args;
    Cardinal *num_args;
{
    NetstatWidget current = (NetstatWidget) gcurrent;
    NetstatWidget new = (NetstatWidget) gnew;
    Boolean redisplay = FALSE;

    if (current->netstat.update != new->netstat.update) {
	if (current->netstat.interval_id) 
	  XtRemoveTimeOut (current->netstat.interval_id);
	new->netstat.interval_id =
	    XtAppAddTimeOut (XtWidgetToApplicationContext(gnew),
			     new->netstat.update * 100, clock_tic,
			     (XtPointer) gnew);
    }

    if (current->netstat.foreground_pixel != new->netstat.foreground_pixel ||
	current->core.background_pixel != new->core.background_pixel) {
	XtReleaseGC (gcurrent, current->netstat.gc);
	new->netstat.gc = get_netstat_gc (new);
	redisplay = TRUE;
    }

    return (redisplay);
}


/*
 * drawing code
 */

static void redraw_netstat (w)
    NetstatWidget w;
{
    register Display *dpy = XtDisplay (w);
    register Window win = XtWindow (w);
    register int x, y;
    GC gc = w->netstat.gc;
    Pixel back = w->core.background_pixel;
    struct _mbimage *im = 0;

    /* center the picture in the window */

    switch (w->netstat.state) {
    case 0:
    case 1:
      im = &w->netstat.inactive;
      break;
    case 2:
      im = &w->netstat.waiting;
      break;
    case 3:
      im = &w->netstat.active;
      break;
    case 4:
      im = &w->netstat.activeout;
      if (w->netstat.flipit) back = w->netstat.foreground_pixel; 
      break;
    case 5:
      im = &w->netstat.start;
      break;
    case 6:
      im = &w->netstat.stop;
      break;
    default:
      fprintf(stderr,"xmonisdn: Error in redrawing -- wrong netstat.state:%d\n", w->netstat.state);
      CloseDown(w, -1);
    }

    x = (((int)w->core.width) - im->width) / 2;
    y = (((int)w->core.height) - im->height) / 2;

    XSetWindowBackground (dpy, win, back);
    XClearWindow (dpy, win);
    XCopyArea (dpy, im->pixmap, win, gc, 0, 0, im->width, im->height, x, y);

    /*
     * XXX - temporary hack; walk up widget tree to find top most parent (which
     * will be a shell) and mash it to have our shape.  This will be replaced
     * by a special shell widget.
     */
    if (w->netstat.shapeit) {
	Widget parent;

	for (parent = (Widget) w; XtParent(parent);
	     parent = XtParent(parent)) {
	    x += parent->core.x + parent->core.border_width;
	    y += parent->core.y + parent->core.border_width;
	}

	if (im->mask != w->netstat.shape_cache.mask ||
	    x != w->netstat.shape_cache.x || y != w->netstat.shape_cache.y) {
	    XShapeCombineMask (XtDisplay(parent), XtWindow(parent),
			       ShapeBounding, x, y, im->mask, ShapeSet);
	    w->netstat.shape_cache.mask = im->mask;
	    w->netstat.shape_cache.x = x;
	    w->netstat.shape_cache.y = y;
	}
    }

    return;
}


static void beep (w)
    NetstatWidget w;
{
  if (w->netstat.volume > 0)
    XBell (XtDisplay (w), w->netstat.volume);
  return;
}


