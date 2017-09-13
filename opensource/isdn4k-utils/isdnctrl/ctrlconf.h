/* $Id: ctrlconf.h,v 1.3 1998/11/21 14:03:34 luethje Exp $
 *
 * ISDN accounting for isdn4linux. (Utilities)
 *
 * Copyright 1995, 1997 by Stefan Luethje (luethje@sl-gw.lake.de)
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
 * $Log: ctrlconf.h,v $
 * Revision 1.3  1998/11/21 14:03:34  luethje
 * isdnctrl: added dialmode into the config file
 *
 * Revision 1.2  1997/06/26 21:25:15  luethje
 * Added the trigger function to the config file.
 *
 * Revision 1.1  1997/06/24 23:35:26  luethje
 * isdnctrl can use a config file
 *
 *
 */

#include "../lib/libisdn.h"

/*****************************************************************************/

#define CONF_SEC_ISDNCTRL         "ISDNCTRL"
#define CONF_ENT_INTERFACES       "INTERFACES"
#define CONF_SEC_INTERFACE        "INTERFACE"
#define CONF_SEC_SLAVE            "SLAVE"
#define CONF_ENT_NAME             "NAME"
#define CONF_ENT_EAZ              "EAZ"
#define CONF_ENT_PHONE_IN         "PHONE_IN"
#define CONF_ENT_PHONE_OUT        "PHONE_OUT"
#define CONF_ENT_SECURE           "SECURE"
#define CONF_ENT_CALLBACK         "CALLBACK"
#define CONF_ENT_CBHUP            "CBHUP"
#define CONF_ENT_CBDELAY          "CBDELAY"
#define CONF_ENT_DIALMAX          "DIALMAX"
#define CONF_ENT_HUPTIMEOUT       "HUPTIMEOUT"
#define CONF_ENT_IHUP             "IHUP"
#define CONF_ENT_CHARGEHUP        "CHARGEHUP"
#define CONF_ENT_CHARGEINT        "CHARGEINT"
#define CONF_ENT_L2_PROT          "L2_PROT"
#define CONF_ENT_L3_PROT          "L3_PROT"
#define CONF_ENT_ENCAP            "ENCAP"
#define CONF_ENT_SDELAY           "SDELAY"
#define CONF_ENT_ADDSLAVE         "ADDSLAVE"
#define CONF_ENT_TRIGGERCPS       "TRIGGER"
#define CONF_ENT_BIND             "BIND"
#define CONF_ENT_PPPBIND          "PPPBIND"
#define CONF_ENT_DIALMODE         "DIALMODE"

/*****************************************************************************/

int writeconfig(int fd, char *file);
int readconfig(int fd, char *file);

/*****************************************************************************/

