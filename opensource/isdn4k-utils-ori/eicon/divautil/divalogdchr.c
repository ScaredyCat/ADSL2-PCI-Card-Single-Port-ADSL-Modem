
/*
 *
 * Copyright (C) Eicon Technology Corporation, 2000.
 *
 * This source file is supplied for the exclusive use with Eicon
 * Technology Corporation's range of DIVA Server Adapters.
 *
 * Eicon File Revision :    1.3  
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
 * Source file for Unix kernel logger daemon process
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>

#include "divas.h"
#include "divalog.h"
#include "loglib.h"

#include "linuxcfg.h"

/* define two poll rates for XLOG both measures in milliseconds */

#define	SLOW_POLL	(5000)		/* no client is displaying XLOG */
#define	FAST_POLL	(100)		/* client actively displaying XLOG */

/* max number of clients supported */

#define MAX_CLIENTS (10)

/*
 * define a type for holding information about each client
 */

typedef struct
{
    char        name[80];       /* name of client */
    int         fd;             /* file descriptor */
    int         read_ptr;       /* read pointer in log */
    uint_t      flags;          /* additional information (see above) */
    uint_t      logs;           /* active logs */
    uint_t      card_id;        /* card being logged */
    uint_t      discarded;      /* number of entries discarded  */
} klog_client_t;

/* number of log entries buffered */

#define MIN_NUM_ENTRIES     (10)
#define DEFAULT_NUM_ENTRIES (1024)
#define MAX_NUM_ENTRIES     (100000)

static
int             num_entries = DEFAULT_NUM_ENTRIES;

static
klog_client_t   client[MAX_CLIENTS];    /* data structure for each client */

static
int             server_fd = -1;         /* server's command file descriptor */

static
int             log_fd = -1;            /* klog device */

static
klog_t          *log = NULL;            /* log buffer */

static
int             write_ptr = 0;          /* write pointer into log buffer */

static
int             oldest_ptr = 0;         /* oldest entry in log buffer */

static
int				timeout = SLOW_POLL;	/* poll timeout - slow by default */

/*
 * update poll timer, depening on whether anyone looking for XLOG
 */

static
void	update_xlog_requests(void)

{
	int		i;

	timeout = SLOW_POLL;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if ((client[i].fd != -1) && (client[i].logs & DIVAS_LOG_XLOG))
        {
			timeout = FAST_POLL;
			break;
        }
	}

	return;
}

/*
 * Function to close client
 */

void    close_client(   klog_client_t   *client,
                        clog_t          *message)

{
    if (message)
    {
        message->flags = KLOG_NO_MORE_LOG;
        (void) write(client->fd, message, sizeof(clog_t));
    }

    /* clear this client's information and close our end of the FIFO */

    close(client->fd);

    memset(client, 0, sizeof(klog_client_t));

    client->fd = -1;

	update_xlog_requests();

    return;
}

/*
 * send a request down to specified card, looking for xlog
 */

static
void	req_xlog(int card_num)

{
    if (ioctl(log_fd, DIA_IOCTL_XLOG_REQ, &card_num) == -1)
    {
        perror("divalogd: error in log device");
    }

    return;
}

/*
 * tell driver which cards and log types we're interested in
 */

static
void		req_xlogs(void)

{
    int         	i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if ((client[i].fd != -1) && (client[i].logs & DIVAS_LOG_XLOG))
		{
			req_xlog(client[i].card_id);
		}
	}

    return;
}

/*
 * Handle log event
 */

int     HandleLogEvent(int  fd)

{
    int         n;
    int         i;
	klog_t		*k;

	k = &log[write_ptr];

    if ((n = read(fd, k, sizeof(klog_t))) != sizeof(klog_t))
    {
        if (n < 0)
        {
            perror("divalogd: server message read");
        }
        else
        {
            fprintf(stderr,
                    "divalogd: bad size (%d) on server message read", n);
        }
        return(1);
    }

	/* if we just read an XLOG message, send request for next one */

	if ((k->type == KLOG_XLOG_MSG) || (k->type == KLOG_XTXT_MSG))
	{
		req_xlog(k->card);
	}

    /* update write pointer */

    write_ptr++;
    write_ptr %= num_entries;

    /* see if we have to bump the pointer to the oldest log entry */

    if (write_ptr == oldest_ptr)
    {
        oldest_ptr++;
        oldest_ptr %= num_entries;
    }

    /* check to see if we have to bump any client readers */

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if ((client[i].fd != -1) && (write_ptr == client[i].read_ptr))
        {
            client[i].read_ptr++;
            client[i].read_ptr %= num_entries;
            client[i].discarded++;
        }
    }

    return(0);
}

/*
 * tell driver which cards and log types we're interested in
 */

static
int     update_active_logs(void)

{
    int         	i;
	dia_log_t		logs;

    /* tell driver we're interested in logging */

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (client[i].fd != -1)
        {
			logs.card_id = client[i].card_id;
			logs.log_types = client[i].logs;

    		if (ioctl(log_fd, DIA_IOCTL_LOG, &logs) == -1)
    		{
        		perror("divalogd: error in log device");
        		return -1;
    		}
		}
	}

    return 0;
}

/*
 * Handle server event: a client has sent us a message
 */

int     HandleServerEvent(int   fd)

{
    klog_msg_t      message;        /* message from client */
    int             i;              /* handly scratch counter */
    int             size;           /* size of information read */

    /* get the client message */

    if ((size = read(fd, &message, sizeof(klog_msg_t))) != sizeof(klog_msg_t))
    {
        if (size < 0)
        {
            perror("divalogd: server message read");
        }
        else
        {
            fprintf(stderr, "divalogd: bad size on server message read");
        }
        return(1);
    }

    switch (message.request)
    {
    case (KLOG_OPEN_CLIENT) :

        /* look for an empty client slot */

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (client[i].fd == -1)
            {
                break;
            }
        }

        if (i == MAX_CLIENTS)
        {
            fprintf(stderr, "divalogd: too many clients\n");
            break;
        }

        if (OpenClient(message.buffer, &client[i].fd))
        {
            fprintf(stderr, "divalogd: error in opening client %s\n",
                    message.buffer);
            break;
        }

        /* set the client read pointer to oldest entry in the buffer */

        if (message.flags & KLOG_RESET_LOG)
        {
            int     j;

            write_ptr = oldest_ptr = 0;

            for (j = 0; j < MAX_CLIENTS; j++)
            {
                client[j].read_ptr = 0;
            }
        }
        else
        {
            client[i].read_ptr = oldest_ptr;
        }

        /* save client's name and the request flags */

        strcpy(client[i].name, message.buffer);

        client[i].flags = message.flags;
        client[i].card_id = message.card_id;
        client[i].logs = message.logs;
        client[i].discarded = FALSE;

        /*
         * after all that, if log is empty and client has requested 
         * notification, close the client
         */

        if ((oldest_ptr == write_ptr) &&
            (message.flags & KLOG_TELL_WHEN_EMPTY))
        {
            clog_t  client_msg;

            client_msg.flags = KLOG_NO_MORE_LOG;
            client_msg.data.time_stamp = 0;
            client_msg.data.buffer[0] = '\0';
            close_client(&client[i], &client_msg);
        }
		else
		{
			update_active_logs();
			update_xlog_requests();
		}

        break;

    case (KLOG_CLOSE_CLIENT) :

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (!(strcmp(client[i].name, message.buffer)))
            {
                break;
            }
        }

        if (i == MAX_CLIENTS)
        {
            fprintf(stderr, "divalogd: client %s not found for close\n",
                message.buffer);
            break;
        }

        close_client(&client[i], NULL);

        break;

    default :
        fprintf(stderr, "divalogd: unknown client request %d\n", message.request);
        break;
    }

    return(0);
}

/*
 * Tell client that we've discarded one or more entries
 */

static
int     tell_client_discarded(klog_client_t *client)

{
    static
    char    buffer[80];
    int     size;
    clog_t  entry;

    /* fill in a dummy entry tellinmg client we've discarded entries */

    sprintf(buffer, "WARNING ! divalog buffer overflow - %d %s discarded",
           client->discarded, (client->discarded == 1) ? "entry" : "entries");

    client->discarded = 0;

    entry.flags = 0;
    entry.data.time_stamp = 0;
    entry.data.type = KLOG_TEXT_MSG;
    entry.data.length = strlen(buffer);
    entry.data.code = 0;
    strcpy(entry.data.buffer, buffer);

    if ((size = write(client->fd, &entry,sizeof(clog_t))) != sizeof(clog_t))
    {
        if (size < 0)
        {
            perror("divalogd: write to client");
        }
        else
        {
            fprintf(stderr, "divalogd: bad size on write to client");
        }
        return(1);
    }

    return(0);
}

/*
 * Handle client event: a client is ready to accept data
 */

int     HandleClientEvent(klog_client_t *client)

{
    int     size;
    clog_t  entry;

    if (client->discarded)
    {
        return(tell_client_discarded(client));
    }

    if (client->read_ptr != write_ptr)
    {
        /* write next entry to client */

        memcpy(&entry.data, &log[client->read_ptr], sizeof(klog_t));

        client->read_ptr++;
        client->read_ptr %= num_entries;

        if ((client->read_ptr == write_ptr)
            && (client->flags & KLOG_TELL_WHEN_EMPTY))
        {
            entry.flags = KLOG_NO_MORE_LOG;
        }
        else
        {
            entry.flags = 0;
        }

        if ((size = write(client->fd, &entry,sizeof(clog_t))) != sizeof(clog_t))
        {
            if (size < 0)
            {
                perror("divalogd: write to client");
            }
            else
            {
                fprintf(stderr, "divalogd: bad size on write to client");
            }
            return(1);
        }

        /* if we're closing the client, go and do it */

        if (entry.flags)
        {
            close_client(client, NULL);
        }
    }

    return(0);
}

/*
 * Wait for an event to happen
 */

int     WaitEvent(void)

{
    struct pollfd   fds[MAX_CLIENTS + 2];   /* list of file descriptor info */
    unsigned long   nfds = 2;               /* number of files of interest */
    int             n;                      /* number of files active */
    int             i, j;                   /* handy scratch counters */

    /* set up the poll array */

    memset(fds, 0, sizeof(fds));

    /* we're interested in reading from server command or klog files */

    fds[0].fd = log_fd;
    fds[0].events = POLLIN;

    fds[1].fd = server_fd;
    fds[1].events = POLLIN;

    while (TRUE)
    {
        /* if any client is not up to date then make sure we service it */

        nfds = 0;

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if ((client[i].fd != -1) && (client[i].read_ptr != write_ptr))
            {
                fds[2 + nfds].fd = client[i].fd;
                fds[2 + nfds].events = POLLOUT;
                nfds++;
            }
        }

        nfds += 2;      /* we always have server and log to service */

        if ((n = poll(fds, nfds, timeout)) < 0)
        {
            perror("divalogd: poll");
            return(1);
        }

		if (n == 0)
		{
			req_xlogs();
			continue;
		}

        /* check log device */

        if (fds[0].revents & POLLIN)
        {
            if (HandleLogEvent(fds[0].fd))
            {
                return(1);
            }
        }

        /* check for server command */

        if (fds[1].revents & POLLIN)
        {
            if (HandleServerEvent(fds[1].fd))
            {
                return(1);
            }
        }

        /* check for clients that are ready to receive and aren't up to date */

        for (i = 2; i < (int) nfds; i++)
        {
            if (fds[i].revents & POLLOUT)
            {
                /* find the client owning this file descriptor */

                for (j = 0; j < MAX_CLIENTS; j++)
                {
                    if (client[j].fd == fds[i].fd)
                    {
                        if (HandleClientEvent(&client[j]))
                        {
                            return(1);
                        }
                    }
                }
            }
        }
    }
}

/*
 * Display usage of command line
 */

static
void    usage(void)
{
    fprintf(stderr, "usage: divalogd [num_entries]\n");
    fprintf(stderr, 
            "       where num_entries is the number of entries to buffer\n");
    fprintf(stderr, "       num_entries must be between %d and %d\n",
            MIN_NUM_ENTRIES, MAX_NUM_ENTRIES);
    return;
}

/*
 * Load the number of entries
 */

void    get_num_entries(char *argv[])
{
    int     i;

    num_entries = 0;

    i = 0;
    while ((i < 10) && (argv[1][i]))
    {
	if (!strcmp(argv[1], "-h"))
	{
		usage();
		exit(1);
	}
        if ((argv[1][i] < '0') || (argv[1][i] > '9'))
        {
            usage();
            fprintf(stderr, "divalogd: non-numeric number of entries\n");
            exit(1);
        }
        num_entries = (num_entries * 10) + ((int) argv[1][i] - '0');
        i++;
    }

    if ((i == 10) || 
        (num_entries < MIN_NUM_ENTRIES) || (num_entries > MAX_NUM_ENTRIES))
    {
        usage();
        fprintf(stderr,
                "divalogd: invalid number of entries\n");
        exit(1);
    }

    return;
}

/*
 * start here
 */

int     main(int argc, char *argv[])

{
    int         	i;
	dia_log_t		active_logs;

    if (!((argc == 1) || (argc == 2)))
    {
        usage();
        fprintf(stderr, "divalogd: invalid command line\n");
        exit(1);
    }

    if (argc == 2)
    {
        get_num_entries(argv);
    }

    /* get a buffer for the log entries */

    if (!(log = malloc(num_entries * sizeof(klog_t))))
    {
        fprintf(stderr, "divalogd: memory allocation failure\n");
        exit(1);
    }

    /* open the driver */

    if ((log_fd = open(DIVAS_DEVICE_DFS, O_RDONLY)) < 0)
    if ((log_fd = open(DIVAS_DEVICE, O_RDONLY)) == -1)
    {
        perror("divalogd: open of log device");
        return 1;
    }

    /* tell driver we're interested in logging */

	active_logs.card_id = 0;
	active_logs.log_types = DIVAS_LOG_DEBUG;

    if (ioctl(log_fd, DIA_IOCTL_LOG, &active_logs) == -1)
    {
        perror("divalogd: error in log device");
        return 1;
    }

    /* open the server's (our) file descriptor */

    if (OpenServer(&server_fd))
    {
        return(1);
    }

    /* initialise client information */

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        client[i].fd = -1;
    }

    /* wait for something to happen */

    if (WaitEvent())
    {
        return(1);
    }

    return(0);
}
