/* $Id: linuxcfg.h,v 1.2 2000/07/08 14:18:52 armin Exp $
 *
 * Eicon-ISDN driver for Linux. (Config)
 *
 * Copyright 2000    by Armin Schindler (mac@melware.de)
 * Copyright 2000    Cytronics & Melware (info@melware.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: linuxcfg.h,v $
 * Revision 1.2  2000/07/08 14:18:52  armin
 * Changes for devfs.
 *
 * Revision 1.1  2000/03/25 12:56:40  armin
 * First checkin of new version 2.0
 * - support for 4BRI, includes orig Eicon
 *   divautil files and firmware updated (only etsi).
 *
 *
 */


#ifdef EICONCTRL 

/* config for native eicon driver with I4L */

#define main(a,b)	Divaload_main(a,b)
#define DIVAS_DEVICE	"/dev/isdnctrl"
#define DIVAS_DEVICE_DFS	"/dev/isdn/isdnctrl"
#define	ioctl(a,b,c)	Divas_ioctl(a,b,c)
extern int Divas_ioctl(int, int, void *);


#else 

/* config for standalone */

#define DIVAS_DEVICE	"/dev/Divas"
#define DIVAS_DEVICE_DFS	"/dev/Divas"

#endif

