#ifndef _XawNetstat_h
#define _XawNetstat_h

/*
 * Netstat widget; looks a lot like the clock widget, don't it...
 */

/* resource names used by Netstat widget that aren't defined in StringDefs.h */

#ifndef _XtStringDefs_h_
#define XtNupdate "update"
#endif

#define XtNvolume "volume"
#define XtNactivePixmap "activePixmap"
#define XtNactivePixmapMask "activePixmapMask"
#define XtNactiveoutPixmap "activeoutPixmap"
#define XtNactiveoutPixmapMask "activeoutPixmapMask"
#define XtNwaitingPixmap "waitingPixmap"
#define XtNwaitingPixmapMask "waitingPixmapMask"
#define XtNinactivePixmap "inactivePixmap"
#define XtNinactivePixmapMask "inactivePixmapMask"
#define XtNstartPixmap "startPixmap"
#define XtNstartPixmapMask "startPixmapMask"
#define XtNstopPixmap "stopPixmap"
#define XtNstopPixmapMask "stopPixmapMask"
#define XtNflip "flip"
#define XtNshapeWindow "shapeWindow"

#define XtCVolume "Volume"
#define XtCPixmapMask "PixmapMask"
#define XtCFlip "Flip"
#define XtCShapeWindow "ShapeWindow"


/* structures */

typedef struct _NetstatRec *NetstatWidget;  /* see NetstatP.h */
typedef struct _NetstatClassRec *NetstatWidgetClass;  /* see NetstatP.h */


extern WidgetClass netstatWidgetClass;

#endif /* _XawNetstat_h */
/* DON'T ADD STUFF AFTER THIS #endif */

