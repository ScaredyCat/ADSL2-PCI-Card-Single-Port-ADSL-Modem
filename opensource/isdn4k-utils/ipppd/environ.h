/*
 * environ.h - Environment handling.
 *
 * Copyright (c) 1999 Michael Mueller <Michael.Mueller4@post.rwth-aachen.de>
 * All rights reserved.
 *
 * This piece of software is released under the GPL.
 *
 * The function script_setenv was originally taken from the original pppd.
 *
 * $Id: environ.h,v 1.1 1999/06/25 06:47:20 paul Exp $
 *
 */

#ifndef __IPPPD_ENVIRON_H__
#define __IPPPD_ENVIRON_H__

void script_setenv(char *var, char *value);
void script_unsetenv(char *var);
void script_unsetenv_prefix(char *prefix);
extern char **script_env;

#endif

