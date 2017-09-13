
/*
 *
 * Copyright (C) Eicon Technology Corporation, 2000.
 *
 * This source file is supplied for the exclusive use with Eicon
 * Technology Corporation's range of DIVA Server Adapters.
 *
 * Eicon File Revision :    1.1  
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY OF ANY KIND WHATSOEVER INCLUDING ANY 
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


/*
 * Internal include file for Diva card library
 */

#if !defined(DILOAD_H)
#define DILOAD_H

int DivaLoad( char **dsp_names,parameter_t *options, char *msg);
/* define IOCTL commands */


#define DI_START_LOAD (90)
#define DI_TASK       (91)
#define DI_END_LOAD   (92)
#define DI_PARAMS     (93)

/* define return codes for errors. */
#define ERR_ETDD_OK	0
#define ERR_ETDD_ACCESS 1
#define ERR_ETDD_DSP	2
#define ERR_ETDD_IOCTL	3
#define ERR_ETDD_NOMEM	4
#define ERR_ETDD_OPEN	5
#define ERR_ETDD_READ	6

/* define type for holding DSP code */

typedef struct
{
    word            size;       /* length of buffer */
    char            *buffer;    /* buffer holding DSP code */
} di_load_t;

#endif /* of DILOAD_H */
