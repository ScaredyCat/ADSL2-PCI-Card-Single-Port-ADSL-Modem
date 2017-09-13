
/*
 *
 * Copyright (C) Eicon Technology Corporation, 2000.
 *
 * This source file is supplied for the exclusive use with Eicon
 * Technology Corporation's range of DIVA Server Adapters.
 *
 * Eicon File Revision :    1.4  
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
 * Internal include file for kernel logger
 */

#if !defined(_KLOGLIB_H)
#define _KLOGLIB_H

#include "divalog.h"
#include "divas.h"

/* following should be defined in stdlib.h */

extern int 	opterr;
extern char *optarg;

/* make sure NULL is defined */

#if !defined(NULL)
#define NULL ((void *) 0)
#endif

typedef unsigned int uint_t;

/* define some standard types */

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE 1
#endif

typedef unsigned short int bool_t;

typedef unsigned int err_t;

/* define a MACRO to get the number of elements in an array */

#if !defined DIM
#define DIM(A)  ((sizeof(A)) / (sizeof(A[0])))
#endif

/* following are values to use for flags for client/server communication */

#define KLOG_TELL_WHEN_EMPTY	(1 << 0) /* client->server to request info */
#define KLOG_NO_MORE_LOG        (1 << 1) /* server->client no more logs there */
#define KLOG_RESET_LOG          (1 << 2) /* client->server, reset the log */

/*
 * define type used for sending to log message to client
 */

typedef struct
{
    uint_t      flags;      /* use defines above */
    klog_t      data;
} clog_t;

/*
 * define type used for client/server communication
 */

typedef enum
{
    KLOG_OPEN_CLIENT = 0,
    KLOG_CLOSE_CLIENT = 1
} klog_req_t;

typedef struct
{
    klog_req_t  request;        /* type of server request */
    uint_t      flags;          /* additional information (see above) */
    uint_t      logs;           /* DIVAS_LOG_XXX bit-mask */
    uint_t      card_id;        /* card number to log */
    char        buffer[80];     /* information */
} klog_msg_t;

/*
 * define error codes
 */

#define ERR_OK          (0)
#define ERR_CLIENT      (1)
#define ERR_SERVER      (2)

/* Open the klog server */

extern err_t    OpenServer(int *fd);

/* Create a client name */

extern void     CreateClientName(char *name);

/* Open the klog client */

extern err_t    OpenClient(char *name, int *fd);

/* Delete the klog server connection */

extern void     DeleteServer(void);

/* Delete the klog client connection */

extern void     DeleteClient(char *name);

/* Eicon specific function to print out a coded message */

extern void xlog(FILE *, char *, uint_t);

#endif /* of _KLOGLIB_H */
