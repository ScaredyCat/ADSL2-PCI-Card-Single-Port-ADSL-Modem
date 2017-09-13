
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
 * Source file for Unix kernel logger library
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "loglib.h"

/* Server's FIFO name */

static char    *server_name = DATADIR "/KLOGD.S";
static char    *client_name = DATADIR "/KLOG.";

/*
 * Open the server's FIFO, create it of not there
 */

err_t   OpenServer(int      *fd)

{
    struct stat buffer;

    if (stat(server_name, &buffer) == -1)
    {
        if (mkfifo(server_name, 0666) == -1)
        {
            perror("mkfifo");
            return(ERR_SERVER);
        }
    }

    if ((*fd = open(server_name, O_RDWR)) == -1)
    {
        perror("open server");
        return(ERR_SERVER);
    }

    return(ERR_OK);
}

/*
 * Open the clients's FIFO, create it of not there
 */

err_t   OpenClient( char    *name,
                    int     *fd)

{
    struct stat buffer;

    if (stat(name, &buffer) == -1)
    {
        if (mkfifo(name, 0666) == -1)
        {
            perror("mkfifo");
            return(ERR_SERVER);
        }
    }

    if ((*fd = open(name, O_RDWR)) == -1)
    {
        perror("open client");
        return(ERR_SERVER);
    }

    return(ERR_OK);
}

/*
 * Make up a client's name
 */

void    CreateClientName(char    *name)

{
    sprintf(name, "%s%d", client_name, (int) getpid());

    return;
}

/*
 * Delete Server
 */

void    DeleteServer(void)

{
    if (unlink(server_name))
    {
        perror("unlink of server");
    }

    return;
}

/*
 * Delete Client
 */

void    DeleteClient(char   *name)

{
    if (unlink(name))
    {
        char buffer[80];

        sprintf(buffer, "unlink of client %s", name);
        perror(buffer);
    }

    return;
}
