#ifndef _XawNetstatP_h
#define _XawNetstatP_h

#include "Net.h"
#include <X11/Xaw/SimpleP.h>

#ifndef NETUP_COMMAND
#define NETUP_COMMAND "/sbin/isdnnet start &"
#endif
#ifndef NETDOWN_COMMAND
#define NETDOWN_COMMAND "/sbin/isdnnet stop &"
#endif


typedef struct {			/* new fields for netstat widget */
    /* resources */
    int update;				/* 1/100 seconds between updates */
    Pixel foreground_pixel;		/* color index of normal state fg */
    String filename;			/* filename to watch */
    Boolean flipit;			/* do flip of full pixmap */
    int volume;				/* bell volume */
    /* local state */
    GC gc;				/* normal GC to use */
    long last_size;			/* size in bytes of netstatname */
    XtIntervalId interval_id;		/* time between checks */
    Boolean state;			/* state of ISDN connection */
    struct _mbimage {
	Pixmap bitmap, mask;		/* depth 1, describing shape */
	Pixmap pixmap;			/* full depth pixmap */
	int width, height;		/* geometry of pixmaps */
    } active, waiting,  inactive, activeout, start, stop;
    Boolean shapeit;			/* do shape extension */
    struct {
	Pixmap mask;
	int x, y;
    } shape_cache;			/* last set of info */
} NetstatPart;

typedef struct _NetstatRec {		/* full instance record */
    CorePart core;
    SimplePart simple;
    NetstatPart netstat;
} NetstatRec;


typedef struct {			/* new fields for netstat class */
    int dummy;				/* stupid C compiler */
} NetstatClassPart;

typedef struct _NetstatClassRec {	/* full class record declaration */
    CoreClassPart core_class;
    SimpleClassPart simple_class;
    NetstatClassPart netstat_class;
} NetstatClassRec;

extern NetstatClassRec netstatClassRec;	 /* class pointer */

#endif /* _XawNetstatP_h */
