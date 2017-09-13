/* tty line ISDN status monitor
 *
 * (c) 1995-97 Volker Götz
 *
 * $Id: imontty.h,v 1.3 2002/01/31 18:59:24 paul Exp $
 */

#ifndef ISDN_MAX_CHANNELS
# define ISDN_MAX_CHANNELS      64       /* largest common denominator */
#endif
#ifndef ISDN_MAX_DRIVERS
# define ISDN_MAX_DRIVERS       32       /* ditto */
#endif

#define		IM_BUFSIZE	10 + (22 * ISDN_MAX_CHANNELS)
#define		IM_VERSION	"0.5"
