
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
 * Source file for Unix kernel logger client program
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <memory.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "sys.h"
#include "divalog.h"
#include "loglib.h"

static
int         terminating = FALSE;    /* are we finishing ? */

static
int         send_close = TRUE;      /* should we send a close message ? */

static
klog_msg_t  message;            /* message to server */

static
int         read_fd = -1;       /* our read pointer */

static
int         server_fd = -1;     /* server's command file descriptor */

static
uint_t      active_logs = DIVAS_LOG_DEBUG;	/* our active logs */

static
uint_t      active_card = 0;	/* active card */

extern 		void	log_idi_req(FILE *stream, void * buf);
extern 		void	log_idi_cb(FILE *stream, void * buf);

/*
 * Send server a message
 */

static
int     SendServerMessage(  int         fd,
                            klog_msg_t  *msg)

{
    int     size;

    /* write to specified file descriptor  */

    if ((size = write(fd, msg, sizeof(klog_msg_t))) != sizeof(klog_msg_t))
    {
        if (size < 0)
        {
            perror("divalog: write to server");
        }
        else
        {
            fprintf(stderr, "divalog: bad size on write to server");
        }
        return(1);
    }

    return(0);
}

/*
 * Finish up
 */

static
void     Terminate(int dummy)

{

    if (terminating)
    {
        return;
    }

    terminating = TRUE;

    /* tell server we're gone if required */

    if (send_close)
    {
        message.request = KLOG_CLOSE_CLIENT;

        (void) SendServerMessage(server_fd, &message);
    }
    close(read_fd);

    /* delete our own connection */

    DeleteClient(message.buffer);

    exit(0);

    return;
}

/*
 * Set up a signal catcher for all signals to terminate gracefully
 */

static
void     InitialiseSignalCatcher(void)

{
    struct sigaction act;

    int     i;

    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = Terminate;
    sigemptyset(&act.sa_mask);


    for (i = 0; i < NSIG; i++)
    {
        (void) sigaction(i, &act, NULL);
    }

    return;
}

/*
 * Read the next entry from the log:
 */

static
int     ReadLog(int     fd,
                clog_t  *entry)

{
    int         size;

    /* get the entry from the log */

    if ((size = read(fd, entry, sizeof(clog_t))) != sizeof(clog_t))
    {
        if (size < 0)
        {
            perror("divalog: read from server");
        }
        else
        {
            fprintf(stderr, "divalog: bad size on read from server");
        }
        return(1);
    }

    return(0);
}

/*
 * parse string into card number
 * return non-zero if error
 */

static
int			parse_card_id(char *arg, uint_t *card_id)

{
	char	*a = arg;

	while (*a)
	{
		if (!isdigit(*a))
		{
   			return -1;
		}
		a++;
	}

	*card_id = atoi(arg);
	if ((*card_id < 1) || ( *card_id > 16 ))
	{
		return -1;
	}

	return 0;
}
/*
 * Tell user how to use the program
 */

void    usage(void)
{
    fprintf(stderr, "usage: divalog [-a N] [-z] [[-w] | [-f file] [-x] [-c] [-i]\n");
    fprintf(stderr, "       -a N to specify adapter number (N defaults to 1)\n");
    fprintf(stderr, "       -z to clear log of all entries\n");
    fprintf(stderr, "       -w to wait for entries to be logged\n");
    fprintf(stderr, "       -f file to log to specified file\n");
    fprintf(stderr, "       -x read ISDN trace\n");
    fprintf(stderr, "       -c read CAPI trace\n");
    fprintf(stderr, "       -i read IDI trace\n");
    return;
}

/*
 * display single entry
 */

static
void	display_prefix(FILE *fp, int card_id, unsigned int ms)

{
	unsigned int     sec, min, hour;

    sec = ms / 1000;
    ms %= 1000;
    min = (sec / 60) % 60;
    sec %= 60;
    hour = min / 60;

	if (card_id == -1)
	{
		fprintf(fp, "  ");
	}
	else
	{
		fprintf(fp, "%d ", card_id + 1);
	}

    fprintf(fp, "%3d:%02d:%02d.%03d ", hour, min, sec, ms);

	return;
}

/*
 * display single entry
 */

static
void	display_entry(FILE *fp, clog_t *e)

{
	unsigned int     ms, sec, min, hour;

    ms = e->data.time_stamp;
    sec = ms / 1000;
    ms %= 1000;
    min = (sec / 60) % 60;
    sec %= 60;
    hour = min / 60;

	if ((e->data.card != -1)
		&& (e->data.card != active_card))
	{
		return;
	}

	switch (e->data.type)
	{
	case (KLOG_TEXT_MSG):
		if (active_logs & DIVAS_LOG_DEBUG)
		{
    		display_prefix(fp, e->data.card, e->data.time_stamp);
			fprintf(fp, "%s\n", e->data.buffer);
        	break;
		}
	case (KLOG_XTXT_MSG):
		if (active_logs & DIVAS_LOG_XLOG)
		{
    		display_prefix(fp, e->data.card, e->data.time_stamp);
			fprintf(fp, "%s\n", e->data.buffer);
		}
        break;
	case (KLOG_IDI_REQ):
		if (active_logs & DIVAS_LOG_IDI)
		{
    		display_prefix(fp, e->data.card, e->data.time_stamp);
			fprintf(fp, "IDI ");
			log_idi_req(fp, e->data.buffer);
		}
        break;
	case (KLOG_IDI_CALLBACK):
		if (active_logs & DIVAS_LOG_IDI)
		{
    		display_prefix(fp, e->data.card, e->data.time_stamp);
			fprintf(fp, "IDI ");
			log_idi_cb(fp, e->data.buffer);
		}
        break;
    case (KLOG_XLOG_MSG):
		if (active_logs & DIVAS_LOG_XLOG)
		{
			int j; char xlogb[120]; 
			char *xp = xlogb; unsigned short code;
        	unsigned short *code_p = (unsigned short *) xp;
        	code = (unsigned short) e->data.code;
        	*code_p = code;
        	xp += sizeof(unsigned short);
        	for (j = 0; j  < (int) e->data.length; j++)
        	{
            	*xp++ = e->data.buffer[j];
        	}
    		display_prefix(fp, e->data.card, e->data.time_stamp);
        	xlog(fp, (char *) &e->data.code,
                        e->data.length + sizeof(word));
        }
		break;
	default:
    	display_prefix(fp, e->data.card, e->data.time_stamp);
        fprintf(fp, "UNKNOWN LOG MESSAGE TYPE (%d) of length (%d)\n",
                    (int) e->data.type, (int) e->data.length);
		break;
    }

    fflush(fp);

	return;
}

/*
 * start here
 */

int     main(int argc, char *argv[])

{
    FILE        *fp = stdout;
    clog_t      entry;
    char        *file_name = NULL;
    int         c;
    bool_t      reset_log = FALSE;

    /* by default, finish when we've seen all log entries */

    message.flags = KLOG_TELL_WHEN_EMPTY;

    opterr = 0;
    while ((c = getopt(argc, argv, "hwzxcif:a:")) != -1)
    {
        switch (c)
        {
        case 'h':
            usage();
            return(0);
            break;

        case 'f':
            if (!message.flags)
            {
                fprintf(stderr, "divalog: invalid combination of options\n");
                usage();
                return(1);
            }
            file_name = optarg;
            break;

        case 'a':
			if (parse_card_id(optarg, &active_card))
			{
                fprintf(stderr, "divalog: invalid adapter number\n");
                usage();
                return(1);
			}
			active_card--;
            break;

        case 'w':
            if (file_name)
            {
                fprintf(stderr, "divalog: invalid combination of options\n");
                usage();
                return(1);
            }
            message.flags = 0;              /* keep reading for ever */
            break;

        case 'z':
            reset_log = TRUE;
            break;

        case 'x':
            active_logs |= DIVAS_LOG_XLOG;
            break;

        case 'i':
            active_logs |= DIVAS_LOG_IDI;
            break;

        case 'c':
            active_logs |= DIVAS_LOG_CAPI;
            break;

        default:
            fprintf(stderr, "divalog: invalid command line option\n");
            usage();
            return(1);
            break;
        }
    }

    if (file_name)
    {
        if (!(fp = fopen(file_name, "w")))
        {
            fprintf(stderr, "divalog: unable to open file \"%s\"\n", file_name);
            return(2);
        }
    }

    if (reset_log)
    {
        message.flags |= KLOG_RESET_LOG;
    }

    /* open the server's file descriptor */

    if (OpenServer(&server_fd))
    {
        return(1);
    }

    /* generate a name for ourselves and open our connection to server */

    CreateClientName(message.buffer);

    if (OpenClient(message.buffer, &read_fd))
    {
        return(1);
    }

    /* tell server we're here */

    message.request = KLOG_OPEN_CLIENT;
    message.logs = active_logs;
    message.card_id = active_card;

    if (SendServerMessage(server_fd, &message))
    {
        return(1);
    }

    InitialiseSignalCatcher();

    while (!ReadLog(read_fd, &entry))
    {
        if (entry.data.time_stamp || entry.data.buffer[0])
		{
			display_entry(fp, &entry);
		}

        if (entry.flags & KLOG_NO_MORE_LOG)
        {
            send_close = FALSE;
            Terminate(0);
        }
    }

    return(0);
}
