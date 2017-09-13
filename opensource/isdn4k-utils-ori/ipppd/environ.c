/*
 * environ.c - Environment handling.
 *
 * Copyright (c) 1999 Michael Mueller <Michael.Mueller4@post.rwth-aachen.de>
 * All rights reserved.
 *
 * This piece of software is released under the GPL.
 *
 * The function script_setenv was taken from the original pppd.
 */

char environ_rcsid[] = "$Id: environ.c,v 1.1 1999/06/25 06:47:17 paul Exp $";

#include <string.h>
#include <malloc.h>
#include "environ.h"

char **script_env = NULL;		/* Env. variable values for scripts */
int s_env_nalloc;			/* #word avail at script_env */

/*
 * script_setenv - set an environment variable value to be used
 * for scripts that we run (e.g. ip-up, auth-up, etc.)
 */
void
script_setenv(char *var, char *value)
{
    int vl;                             /* length of the value to set */
    int i;
    char *p;
    char *newstring;

    if ( !var )				/* no idea which variable to set */
	return;

    if ( !value ) {			/* no value given, so remove variable */
	script_unsetenv(var);
	return;
    }

    vl = strlen(var);
    newstring = (char *)malloc(vl + strlen(value) + 2);
    if ( newstring == NULL )
	return;
    strcpy(newstring, var);
    newstring[vl] = '=';
    strcpy(newstring+vl+1, value);

    /* check if variable is already set */
    if (script_env != NULL) {
        for (i = 0; (p = script_env[i]) != 0; ++i) {
            if (strncmp(p, var, vl) == 0 && p[vl] == '=') {
                free(p);
                script_env[i] = newstring;
                return;
            }
        }
    } else {
        i = 0;
        script_env = (char **) malloc(16 * sizeof(char *));
        if (script_env == NULL)
            return;
        s_env_nalloc = 16;
    }

    if (i + 1 >= s_env_nalloc) {
        int new_n = i + 17;
        char **newenv = (char **) realloc((void *)script_env,
                        new_n * sizeof(char *));
        if (newenv == 0)
            return;
        script_env = newenv;
        s_env_nalloc = new_n;
    }

    script_env[i] = newstring;
    script_env[i+1] = 0;
}

void script_unsetenv(char *var)
{
    int i;
    int vl;
    char *p;

    if ( !var || !script_env )
	return;

    vl = strlen(var);
    for ( i = 0; (p = script_env[i]) != 0; ++i) {
	if (strncmp(p, var, vl) == 0 && p[vl] == '=') {
		free(p);
		for ( ; i+1 < s_env_nalloc ; i++ )
		    script_env[i] = script_env[i+1];
		script_env[i] = NULL;
		return;
	}
    }

    return;
}
 
void script_unsetenv_prefix(char *prefix)
{
    int i,j;
    int vl;
    char *p;

    if ( !prefix || !script_env )
	return;

    vl = strlen(prefix);
    for ( i = 0; (p = script_env[i]) != 0; ++i) {
	if (strncmp(p, prefix, vl) == 0) {
	    free(p);
	    for ( j=i ; j+1 < s_env_nalloc ; j++ )
		script_env[j] = script_env[j+1];
	    script_env[j] = NULL;
	    i--;	/* we have to go one step back since we moved all
			   variables behind the current one forward */
	}
    }

    return;
}
 
