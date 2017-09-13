/*
 *
 * $Id: userpass.c,v 1.3 2001/11/07 14:38:17 calle Exp $
 *
 * userpass.c - pppd plugin to provide username password
 *
 * Copyright 2000 Carsten Paeth <calle@calle.in-berlin.de>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 * $Log: userpass.c,v $
 * Revision 1.3  2001/11/07 14:38:17  calle
 * show version info.
 *
 * Revision 1.2  2001/05/01 12:43:49  calle
 * - new pppd 2.4.1 looks in /usr/lib/pppd/VERSION for plugins
 * - now depends on pppd version
 * - supports incoming and outgoing calls together with ppp option "demand"
 * - new options: voicecallwakeup and coso (caller,local,remote)
 * - peer samples not ready.
 *
 * Revision 1.1  2000/05/18 14:58:35  calle
 * Plugin for pppd to support PPP over CAPI2.0.
 *
 *
 */
#include "pppd.h"

#include "patchlevel.h"
#ifdef VERSION
char pppd_version[] = VERSION;
#endif

static char *revision = "$Revision: 1.3 $";

static char username[MAXNAMELEN+1];
static char password[MAXSECRETLEN+1];

static option_t options[] = {
{ "username", o_string, username, "username", OPT_STATIC, 0, MAXNAMELEN },
{ "password", o_string, password, "password", OPT_STATIC, 0, MAXSECRETLEN },
{ 0 }
};

static void copystr(char *to, char *from)
{
	while (*from)
		*to++ = *from++;
	*to = 0;
}

static int userpass(char *user, char *passwd)
{
    if (username) copystr(user, username);
    if (passwd) copystr(passwd, password);
    return 1;
}

void plugin_init(void)
{
    info("userpass: %s", revision);
    add_options(options);
    pap_passwd_hook = userpass;
}
