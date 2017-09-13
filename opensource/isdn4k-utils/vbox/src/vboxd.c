/*
** $Id: vboxd.c,v 1.5 1998/04/28 08:34:52 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include "config.h"

#if TIME_WITH_SYS_TIME
#   include <sys/time.h>
#   include <time.h>
#else
#   if HAVE_SYS_TIME_H
#      include <sys/time.h>
#   else
#      include <time.h>
#   endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <getopt.h>
#include <utime.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "vboxd.h"
#include "libvbox.h"
#include "streamio.h"

/** Variables *************************************************************/

static streamio_t *accesslist = NULL;
static char       *vbasename  = NULL;

static char *client_hostaddr    = NULL;
static char *client_hostname    = NULL;
static char *client_user        = NULL;
static char *client_spool       = NULL;
static char *client_home        = NULL;
static char *server_vboxdrc     = NULL;
static int   client_access      = VBOXD_ACC_NOTHING;    /* Default no access */
static int   client_1st_timeout = 30;
static int   client_2nd_timeout = 600;

/** Prototypes ************************************************************/

static void leave_program(int);
static void start_connection(void);
static void message(char *, ...);
static int  wildmat(char *, char *);
static int  check_client_access_login(char *, char *, char *, char *);
static int  check_client_access_start(char *, char *);
static int  get_next_command(char *, int, int);
static void handle_client_input(void);
static void handle_commands(char *);
static void usage(void);
static void version(void);
static void exit_on_signal(int);

/** Server prototypes *****************************************************/

static void srv_help(int, char **);
static void srv_quit(int, char **);
static void srv_list(int, char **);
static void srv_noop(int, char **);
static void srv_count(int, char **);
static void srv_login(int, char **);
static void srv_header(int, char **);
static void srv_message(int, char **);
static void srv_toggle(int, char **);
static void srv_delete(int, char **);
static void srv_statusctrl(int, char **);
static void srv_createctrl(int, char **);
static void srv_removectrl(int, char **);

/** Structures ************************************************************/

static struct servercmds commands[] =
{
	{ "help"      , srv_help       },
	{ "quit"      , srv_quit       },
	{ "list"      , srv_list       },
	{ "noop"      , srv_noop       },
	{ "count"     , srv_count      },
   { "login"     , srv_login      },
	{ "header"    , srv_header     },
	{ "message"   , srv_message    },
	{ "toggle"    , srv_toggle     },
	{ "delete"    , srv_delete     },
	{ "statusctrl", srv_statusctrl },
	{ "createctrl", srv_createctrl },
	{ "removectrl", srv_removectrl },
	{ NULL        , NULL           }
};

static struct option arguments[] =
{
	{ "version"		, no_argument			, NULL, 'v' },
	{ "help"			, no_argument			, NULL, 'h' },
	{ "timeout"		, required_argument	, NULL, 't' },
	{ "file"       , required_argument  , NULL, 'f' },
	{ NULL			, 0						, NULL, 0   }
};

/**************************************************************************/
/** The magic main...                                                    **/
/**************************************************************************/

int main(int argc, char **argv)
{
	int opts;

	client_hostaddr    = NULL;
	client_hostname    = NULL;
	client_user        = NULL;
	client_spool       = NULL;
	client_1st_timeout = 20;
	client_2nd_timeout = 600;
	client_access      = VBOXD_ACC_NOTHING;
	server_vboxdrc     = VBOXDRC;

	if (!(vbasename = rindex(argv[0], '/')))
		vbasename = argv[0];
	else
		vbasename++;

	while ((opts = getopt_long(argc, argv, "vhf:t:", arguments, (int *)0)) != EOF)
	{
		switch (opts)
		{
			case 't':
				client_2nd_timeout = xstrtol(optarg, 600);
				break;

			case 'f':
				server_vboxdrc = optarg;
				break;

			case 'v':
				version();
				break;

			case 'h':
			default:
				usage();
				break;
		}
	}

	/*
	 * Install signal handlers.
	 */

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT , exit_on_signal);
	signal(SIGHUP , exit_on_signal);
	signal(SIGTERM, exit_on_signal);

	openlog("vboxd", LOG_CONS|LOG_PID, LOG_DAEMON); 

	/*
	 * Read the access list into memory. If we can't get it, we exit with
	 * error.
	 */

	if (!(accesslist = streamio_open(server_vboxdrc)))
	{
		syslog(LOG_ERR, "can't read access list (%s).", strerror(errno));

		message("%s Can't read access list (%s). Goodbye!\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));

		leave_program(1);
	}

	/*
	 * Get remote host information and check if the remote host is in the
	 * access list (no user or password check is done now).
	 */

	start_connection();

	message("%s vbox message server %s (%s) is ready (timeouts %d/%d secs).\r\n", VBOXD_VAL_SERVEROK, VERSION, VERDATE, client_1st_timeout, client_2nd_timeout);

	/*
	 * Handle the incomming commands.
	 */

	handle_client_input();

	leave_program(0);
	return 0;
}

/**************************************************************************/
/** leave_program(): Frees all resources and exist.                      **/
/**************************************************************************/

static void leave_program(int status)
{
	syslog(LOG_INFO, "connection closed.");

	if (client_hostaddr) free(client_hostaddr);
	if (client_hostname) free(client_hostname);
	if (client_spool   ) free(client_spool   );
	if (client_user    ) free(client_user    );

	streamio_close(accesslist);

	message("%s .\r\n", VBOXD_VAL_SERVERQUIT);
	fflush(stdout);

	closelog();

	exit(status);
}

/**************************************************************************/
/** version(): Prints package version                                    **/
/**************************************************************************/

static void version(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "%s version %s (%s)\n", vbasename, VERSION, VERDATE);
	fprintf(stderr, "\n");

	exit(1);
}

/**************************************************************************/
/** usage(): Prints usage message.                                       **/
/**************************************************************************/

static void usage(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s [ OPTION ] [ ... ]\n", vbasename);
	fprintf(stderr, "\n");
	fprintf(stderr, "-f, --file FILE      Overwrites \"%s\".\n", VBOXDRC);
   fprintf(stderr, "-t, --timeout SECS   Close connection after SECS idle time (default 600).\n");
	fprintf(stderr, "-v, --version        Displays program version.\n");
	fprintf(stderr, "-h, --help           Displays this message.\n");
	fprintf(stderr, "\n");

	exit(1);
}

/**************************************************************************/
/** exit_on_signal(): Exit program on signal.                            **/
/**************************************************************************/

static void exit_on_signal(int s)
{
	syslog(LOG_INFO, "server dies on signal %d.", s);

	message("%s Server dies on signal %d.\r\n", VBOXD_VAL_TEMPERROR, s);

	leave_program(s);
}

/**************************************************************************/
/** handle_client_input(): Handle clients input.                         **/
/**************************************************************************/

static void handle_client_input(void)
{
	char line[VBOXD_LEN_CMDLINE + 1];
	int  rc;

	while (TRUE)
	{
		fflush(stdout);

		rc = get_next_command(line, VBOXD_LEN_CMDLINE, client_1st_timeout);

		switch (rc)
		{
			case VBOXD_ERR_TIMEOUT:
				message("%s Timeout after %d seconds, closing connection.\r\n", VBOXD_VAL_TEMPERROR, client_1st_timeout);
				leave_program(1);
				break;

			case VBOXD_ERR_TOOLONG:
				syslog(LOG_WARNING, "line too long.");
				message("%s Line too long.\r\n", VBOXD_VAL_BADCOMMAND);
				break;

			case VBOXD_ERR_EOF:
				syslog(LOG_WARNING, "receive EOF - pipe broken.");
				message("%s Receive EOF - I think the pipe is broken.\r\n", VBOXD_VAL_TEMPERROR);
				leave_program(1);
				break;

			case VBOXD_ERR_OK:
				handle_commands(line);
				break;
		}
	}
}

/**************************************************************************/
/** handle_commands(): Handles incomming commands.                       **/
/**************************************************************************/

static void handle_commands(char *line)
{
	char *av[VBOXD_LEN_ARGLIST];
	int   ac;
	char *arg;
	int   cc;

	/*
	 * Parse command line and fill the internal argument structure. The
	 * first argument is allways the command.
	 */

	av[0] = strtok(line, "\t ");

	if (!av[0])
	{
		message("%s No command given.\r\n", VBOXD_VAL_BADCOMMAND);

		return;
	}

	ac = 1;
	cc = 0;

	while ((arg = strtok(NULL, "\t ")) && (ac < VBOXD_LEN_ARGLIST))
	{
		av[ac++] = arg;
	}

	/*
	 * Check if the command is in the command list. If found we call the
	 * command with the internal argument list as parameters.
	 */

	while (commands[cc].name)
	{
		if (strcasecmp(commands[cc].name, av[0]) == 0)
		{
			if (commands[cc].func)
			{
				client_1st_timeout = client_2nd_timeout;

				commands[cc].func(ac, av);

				return;
			}
		}

		cc++;
	}

	/*
	 * If the command was not found in the command list we print a error
	 * message (nothing else is done).
	 */

	syslog(LOG_INFO, "unknown command \"%s\" requested.", av[0]);

	message("%s Unknown (or not yet implemented) command \"%s\".\r\n", VBOXD_VAL_BADCOMMAND, av[0]);
}

/**************************************************************************/
/** get_next_command(): Wait for clients next command.                   **/
/**************************************************************************/

static int get_next_command(char *line, int linelen, int timeout)
{
	struct timeval timeval;
	fd_set         rmask;
	char          *stop;
	int            p;
	int            c;
	int            rc;

	*line = '\0';

	p = 0;
	c = 0;

	while (TRUE)
	{
		VBOX_ONE_FD_MASK(&rmask, STDIN_FILENO);

		timeval.tv_sec  = timeout;
		timeval.tv_usec = 0;

		rc = select((STDIN_FILENO + 1), &rmask, NULL, NULL, &timeval);

		if (rc < 0)
		{
			if (errno == EINTR) continue;

			syslog(LOG_ERR, "can't select (%s).", strerror(errno));

			message("%s can't select (%s).\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));

			return(VBOXD_ERR_TIMEOUT);
		}

		if ((rc == 0) || (!FD_ISSET(STDIN_FILENO, &rmask))) return(VBOXD_ERR_TIMEOUT);

		rc = read(STDIN_FILENO, &c, 1);

		if (rc == 0) return(VBOXD_ERR_EOF);

		if (rc < 0)
		{
			syslog(LOG_ERR, "can't read (%s).", strerror(errno));

         message("%s can't read (%s).\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));

			return(VBOXD_ERR_TIMEOUT);
		}

		if (c == '\n')
		{
			if ((stop = rindex(line, '\r'))) *stop = '\0';

			return(VBOXD_ERR_OK);
		}

		line[p + 0] = c;
		line[p + 1] = '\0';

		if (p++ >= linelen) return(VBOXD_ERR_TOOLONG);
	}

	return(VBOXD_ERR_TIMEOUT);
}

/**************************************************************************/
/** start_connection(): Starts the connection to the client.             **/
/**************************************************************************/

static void start_connection(void)
{
	struct hostent     *hostptr;
	struct sockaddr_in  sockptr;
	int                 socklen;

	socklen = sizeof(sockptr);

	if (getpeername(STDIN_FILENO, (struct sockaddr *)&sockptr, &socklen) < 0)
	{
		syslog(LOG_ERR, "can't get peername (%s).", strerror(errno));

		message("%s I can't get your name (%s). Goodbye!\r\n", VBOXD_VAL_ACCESSDENIED, strerror(errno));

		leave_program(1);
	}

	if (sockptr.sin_family != AF_INET)
	{
		syslog(LOG_ERR, "bad address family.");

		message("%s Bad address family. Goodbye!\r\n", VBOXD_VAL_ACCESSDENIED);

		leave_program(1);
	}

	client_hostaddr = strdup(inet_ntoa(sockptr.sin_addr));

	hostptr = gethostbyaddr((char *)&sockptr.sin_addr, sizeof(sockptr.sin_addr), AF_INET);

	if (!hostptr)
		client_hostname = strdup(inet_ntoa(sockptr.sin_addr));
	else
		client_hostname = strdup(hostptr->h_name);

	if ((!client_hostaddr) || (!client_hostname))
	{
		syslog(LOG_ERR, "out of memory (%s).", strerror(errno));

		message("%s Out of memory (%s). Goodbye!\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));

		leave_program(1);
	}

	client_access = VBOXD_ACC_NOTHING;

	if (!check_client_access_start(client_hostname, client_hostaddr))
	{
		syslog(LOG_INFO, "host %s not in access list.", client_hostname);

		message("%s You are not in my access list. Goodbye!\r\n", VBOXD_VAL_ACCESSDENIED);

		leave_program(1);
	}
}

/**************************************************************************/
/** check_client_access_start(): Checks if the client can login as       **/
/**                              unknown user (server startup).          **/
/**************************************************************************/

static int check_client_access_start(char *name, char *addr)
{
   char  line[VBOXD_LEN_ACCESSLINE + 1];
   char *list[3];
   char *p;
   int   i;

   accesslist = streamio_reopen(accesslist);

   while (streamio_gets(line, VBOXD_LEN_ACCESSLINE, accesslist))
   {
		if ((line[0] != 'L') || (line[1] != ':')) continue;

      /*
       * Split the current line into seperate fields.
       */

      for (list[0] = line, i = 0, p = line; ((*p) && (i < 3)); p++)
      {
         if (*p == ':')
         {
            *p = '\0';

            list[++i] = (p + 1);
         }
      }

		if (i != 2) continue;

		/*
		 * Check if the hostname or the hosts ip address matchs the current
		 * pattern.
		 */

		if ((name) || (addr))
		{
			if (!wildmat(name, list[1]))
			{
				if (!wildmat(addr, list[1])) continue;
			}
		}

		/*
		 * Check if the client has access to count new messages. If true,
		 * the "login" is correct.
		 */

		if ((strcasecmp(list[2], "yes")) == 0 || (strcasecmp(list[2], "y") == 0))
		{
			client_access |= VBOXD_ACC_COUNT;

			returnok();
		}
	}

	returnerror();
}

/**************************************************************************/
/** check_client_access_login(): Checks if the client can login as user. **/
/**************************************************************************/

static int check_client_access_login(char *name, char *addr, char *user, char *pass)
{
	char  temp[PATH_MAX + 1];
	char  line[VBOXD_LEN_ACCESSLINE + 1];
	char *list[7];
	char *p;
	int   i;

	accesslist = streamio_reopen(accesslist);

	if (client_spool) free(client_spool);
	if (client_user ) free(client_user );

	client_spool  = NULL;
	client_home   = NULL;
	client_user   = NULL;
	client_access = VBOXD_ACC_COUNT;

	while (streamio_gets(line, VBOXD_LEN_ACCESSLINE, accesslist))
	{
		/*
		 * Split the current line into seperate fields.
		 */

		if ((line[0] != 'A') || (line[1] != ':')) continue;

		for (list[0] = line, i = 0, p = line; ((*p) && (i < 6)); p++)
		{
			if (*p == ':')
			{
				*p = '\0';

				list[++i] = (p + 1);
			}
		}

		if (i != 6) continue;

		/*
		 * Check if the hostname or the hosts ip address matchs the current
		 * pattern.
		 */

		if (!wildmat(name, list[1]))
		{
			if (!wildmat(addr, list[1])) continue;
		}

		/*
		 * Check if the user and the password are correct. If the password
		 * field contains '!' or '-' user is unable to login.
		 */

		if (strcmp(user, list[3]) != 0) continue;

		if (*list[4])
		{
			if (!pass) continue;

			if (strcmp(list[4], "-" ) == 0) continue;
			if (strcmp(list[4], "!" ) == 0) continue;
			if (strcmp(list[4], pass) != 0) continue;
		}

		/*
		 * Save name and spool directory. If memory allocation fails we return
		 * an error so the complete check will fail!
		 */

		client_user  = strdup(list[3]);
		client_home  = strdup(list[5]);
		client_spool = strdup(list[6]);

		if ((client_home) && (client_spool))
		{
			if (*client_spool != '/')
			{
				xstrncpy(temp, client_home , PATH_MAX);
				xstrncat(temp, "/"         , PATH_MAX);
				xstrncat(temp, client_spool, PATH_MAX);

				free(client_spool);

				client_spool = strdup(temp);
			}
		}

		if ((!client_user) || (!client_spool) || (!client_home))
		{
			if (client_spool) free(client_spool);
			if (client_user ) free(client_user );
			if (client_home ) free(client_home );

			returnerror();
		}

		/*
		 * Check users access: '*' for all, 'R' for read access and 'W' for
		 * write access.
		 */

		if (strcmp(list[2], "*") == 0)
		{
			client_access = (VBOXD_ACC_COUNT|VBOXD_ACC_READ|VBOXD_ACC_WRITE);
		}

		if (index(list[2], 'R')) client_access |= VBOXD_ACC_READ;
		if (index(list[2], 'W')) client_access |= VBOXD_ACC_WRITE;

		returnok();
	}

	returnerror();
}

/**************************************************************************/
/** wildmat(): Matchs pattern.                                           **/
/**************************************************************************/

static int wildmat(char *text, char *pattern)
{
	if ((text) && (pattern))
	{
		if (fnmatch(pattern, text, FNM_PERIOD) == 0) returnok();
	}

	returnerror();
}

/**************************************************************************/
/** message(): Prints message to stdout.                                 **/
/**************************************************************************/

static void message(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vfprintf(stdout, fmt, arg);
   va_end(arg);
}

/**************************************************************************/
/**                                                                      **/
/**                                                                      **/
/**                     S E R V E R   C O M M A N D S                    **/
/**                                                                      **/
/**                                                                      **/
/**************************************************************************/
/** login      <username> [password]                                     **/
/** delete     <message>                                                 **/
/** toggle     <message>                                                 **/
/** message    <message>                                                 **/
/** header     <message>                                                 **/
/** statusctrl <control>                                                 **/
/** count                                                                **/
/** noop                                                                 **/
/** list                                                                 **/
/** help                                                                 **/
/** quit                                                                 **/
/**************************************************************************/

/**************************************************************************/
/** srv_noop(): Does nothing.                                   [server] **/
/**************************************************************************/

static void srv_noop(int argc, char **argv)
{
}

/**************************************************************************/
/** srv_header(): Gets a vbox message header.                   [server] **/
/**************************************************************************/

static void srv_header(int argc, char **argv)
{
	vaheader_t  header;
	int         fd;
	char       *msgname;

	if ((!client_spool) || (!client_user) || (!(client_access & VBOXD_ACC_READ)))
	{
		message("%s Access denied (no read access).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (argc != 2)
	{
		message("%s usage: %s <message>\r\n", VBOXD_VAL_BADARGS, argv[0]);

		return;
	}

	if (chdir(client_spool) != 0)
	{
		message("%s Access denied (messagebox unaccessable).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (!(msgname = rindex(argv[1], '/')))
	{
		msgname = argv[1];
	}
	else msgname++;
	
	if ((fd = open(msgname, O_RDONLY)) != -1)
	{
		if (header_get(fd, &header))
		{
			message("%s %d\r\n", VBOXD_VAL_HEADER, sizeof(vaheader_t));
			pullmsg(stdout);

			write(STDOUT_FILENO, (char *)&header, sizeof(vaheader_t));

			message("%s .\r\n", VBOXD_VAL_HEADER);
			pullmsg(stdout);
		}
		else message("%s Not a vbox message.\r\n", VBOXD_VAL_BADMESSAGE);
	}
	else message("%s Can't open message (%s).\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));
}

/**************************************************************************/
/** srv_message(): Gets a vbox message.                         [server] **/
/**************************************************************************/

static void srv_message(int argc, char **argv)
{
	struct stat  status;
	vaheader_t   header;
	char        *block;
	char        *msgname;
	int          fd;

	if ((!client_spool) || (!client_user) || (!(client_access & VBOXD_ACC_READ)))
	{
		message("%s Access denied (no read access).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (argc != 2)
	{
		message("%s usage: %s <message>\r\n", VBOXD_VAL_BADARGS, argv[0]);

		return;
	}

	if (chdir(client_spool) != 0)
	{
		message("%s Access denied (messagebox unaccessable).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (!(msgname = rindex(argv[1], '/')))
	{
		msgname = argv[1];
	}
	else msgname++;
	
	if ((fd = open(msgname, O_RDONLY)) != -1)
	{
		if (fstat(fd, &status) == 0)
		{
			if (header_get(fd, &header))
			{
				if (lseek(fd, 0, SEEK_SET) == 0)
				{
					if ((block = malloc(status.st_size)))
					{
						if (read(fd, block, status.st_size) == status.st_size)
						{
							message("%s %d\r\n", VBOXD_VAL_MESSAGE, status.st_size);
                     pullmsg(stdout);

							write(STDOUT_FILENO, block, status.st_size);

							message("%s .\r\n", VBOXD_VAL_MESSAGE);
							pullmsg(stdout);

							free(block);
						}
						else message("%s Can't read message (%s).\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));
					}
					else message("%s Not enough memory (%s).\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));
				}
				else message("%s Can't seek back (%s).\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));
			}
			else message("%s Not a vbox message.\r\n", VBOXD_VAL_BADMESSAGE);
		}
		else message("%s Can't get message status (%s).\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));

		close(fd);
	}
	else message("%s Can't open message (%s).\r\n", VBOXD_VAL_TEMPERROR, strerror(errno));
}

/**************************************************************************/
/** srv_list(): List available messages.                        [server] **/
/**************************************************************************/

static void srv_list(int argc, char **argv)
{
	struct stat    status;
	struct dirent *tmp;
	vaheader_t     header;
	DIR           *dir;
	int            fd;

	if ((!client_spool) || (!client_user) || (!(client_access & VBOXD_ACC_READ)))
	{
		message("%s Access denied (no read access).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (chdir(client_spool) != 0)
	{
		message("%s Access denied (messagebox unaccessable).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (!(dir = opendir(".")))
	{
		message("%s Access denied (can't open directory).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	while ((tmp = readdir(dir)))
	{
		if (strcmp(tmp->d_name, "." ) == 0) continue;
		if (strcmp(tmp->d_name, "..") == 0) continue;

		if ((fd = open(tmp->d_name, O_RDONLY)) != -1)
		{
			if (fstat(fd, &status) == 0)
			{
				if (header_get(fd, &header))
				{
					message("%s +\r\n"    , VBOXD_VAL_LIST);
					message("%s F %s\r\n" , VBOXD_VAL_LIST, tmp->d_name);
					message("%s T %lu\r\n", VBOXD_VAL_LIST, ntohl(header.time));
					message("%s M %lu\r\n", VBOXD_VAL_LIST, status.st_mtime);
					message("%s C %d\r\n" , VBOXD_VAL_LIST, ntohl(header.compression));
					message("%s S %d\r\n" , VBOXD_VAL_LIST, status.st_size);
					message("%s N %s\r\n" , VBOXD_VAL_LIST, header.name);
					message("%s I %s\r\n" , VBOXD_VAL_LIST, header.callerid);
					message("%s P %s\r\n" , VBOXD_VAL_LIST, header.phone);
					message("%s L %s\r\n" , VBOXD_VAL_LIST, header.location);
				}
			}

			close(fd);
		}
	}

	message("%s .\r\n", VBOXD_VAL_LIST);

	closedir(dir);
}

/**************************************************************************/
/** srv_login(): Login as user.                                 [server] **/
/**************************************************************************/

static void srv_login(int argc, char **argv)
{
	char *user = NULL;
	char *pass = NULL;
	int   r;

	if (argc >= 2) user = argv[1];
	if (argc >= 3) pass = argv[2];

	r = check_client_access_login(client_hostname, client_hostaddr, user, pass);

	if (r)
	{
		message("%s Welcome %s; messagebox is %s (", VBOXD_VAL_LOGINOK, client_user, client_spool);

		if (client_access & VBOXD_ACC_READ ) message("r");
		if (client_access & VBOXD_ACC_WRITE) message("w");

		message(").\r\n");
	}
	else message("%s Access denied.\r\n", VBOXD_VAL_ACCESSDENIED);
}

/**************************************************************************/
/** srv_help(): Prints the help message.                        [server] **/
/**************************************************************************/

static void srv_help(int argc, char **argv)
{
   message("%s Commands require special access:\r\n"                                , VBOXD_VAL_HELP);
   message("%s \r\n"                                                                , VBOXD_VAL_HELP);
	message("%s LIST                               List all messages.\r\n"           , VBOXD_VAL_HELP);
	message("%s COUNT                              Count new messages.\r\n"          , VBOXD_VAL_HELP);
   message("%s DELETE     <message>               Delete a message.\r\n"            , VBOXD_VAL_HELP);
	message("%s MESSAGE    <message>               Get a message.\r\n"               , VBOXD_VAL_HELP);
   message("%s HEADER     <message>               Get a message header.\r\n"        , VBOXD_VAL_HELP);
   message("%s TOGGLE     <message>               Toggle message new flag.\r\n"     , VBOXD_VAL_HELP);
   message("%s STATUSCTRL <control>               Check if control exists.\r\n"     , VBOXD_VAL_HELP);
   message("%s CREATECTRL <control>               Create a control file.\r\n"       , VBOXD_VAL_HELP);
   message("%s REMOVECTRL <control>               Remove a control file.\r\n"       , VBOXD_VAL_HELP);
   message("%s \r\n"                                                                , VBOXD_VAL_HELP);
   message("%s Commands available for all clients:\r\n"                             , VBOXD_VAL_HELP);
   message("%s \r\n"                                                                , VBOXD_VAL_HELP);
	message("%s LOGIN      <username> [password]   Login as user (gives access).\r\n", VBOXD_VAL_HELP);
	message("%s NOOP                               Does nothing.\r\n"                , VBOXD_VAL_HELP);
	message("%s HELP                               Display command list.\r\n"        , VBOXD_VAL_HELP);
	message("%s QUIT                               Quit.\r\n"                        , VBOXD_VAL_HELP);
	message("%s .\r\n"                                                               , VBOXD_VAL_HELP);
}

/**************************************************************************/
/** srv_count(): Counts new messages.                           [server] **/
/**************************************************************************/

static void srv_count(int argc, char **argv)
{
	struct stat    status;
	struct dirent *tmp;
	time_t         newest;
	int            mcount;
	DIR           *dir;

	if ((!client_spool) || (!client_user) || (!(client_access & VBOXD_ACC_READ)))
	{
		message("%s Access denied (no read access).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (chdir(client_spool) != 0)
	{
		message("%s Access denied (messagebox unaccessable).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (!(dir = opendir(".")))
	{
		message("%s Access denied (can't open directory).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	newest = 0;
	mcount = 0;

	while ((tmp = readdir(dir)))
	{
		if (strcmp(tmp->d_name, "." ) == 0) continue;
		if (strcmp(tmp->d_name, "..") == 0) continue;

		if (stat(tmp->d_name, &status) == 0)
		{
			if (status.st_mtime > 0)
			{
				mcount++;

				if (status.st_mtime > newest) newest = status.st_mtime;
			}
		}
	}

	closedir(dir);

	message("%s %d %ld\r\n", VBOXD_VAL_COUNT, mcount, newest);
}

/**************************************************************************/
/** srv_toggle(): Toggle message new flag.                      [server] **/
/**************************************************************************/

static void srv_toggle(int argc, char **argv)
{
	struct utimbuf  utimeb;
	struct stat     status;
	char           *msgname;

	if ((!client_spool) || (!client_user) || (!(client_access & VBOXD_ACC_WRITE)))
	{
		message("%s Access denied (no write access).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (chdir(client_spool) != 0)
	{
		message("%s Access denied (messagebox unaccessable).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (argc != 2)
	{
		message("%s usage: %s <message>.\r\n", VBOXD_VAL_BADARGS, argv[0]);

		return;
	}

	if (!(msgname = rindex(argv[1], '/')))
	{
		msgname = argv[1];
	}
	else msgname++;

	if (stat(msgname, &status) == 0)
	{
		utimeb.actime  = status.st_atime;
		utimeb.modtime = (status.st_mtime > 0 ? 0 : status.st_ctime);
			
		if (utime(msgname, &utimeb) != 0)
		{
			message("%s Can't set new modification time.\r\n", VBOXD_VAL_TEMPERROR);
		}
		else message("%s %lu\r\n", VBOXD_VAL_TOGGLE, utimeb.modtime);
	}
	else message("%s: Can't get file status.\r\n", VBOXD_VAL_TEMPERROR);
}

/**************************************************************************/
/** srv_delete(): Delete a message.                             [server] **/
/**************************************************************************/

static void srv_delete(int argc, char **argv)
{
	char *msgname;

	if ((!client_spool) || (!client_user) || (!(client_access & VBOXD_ACC_WRITE)))
	{
		message("%s Access denied (no write access).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (chdir(client_spool) != 0)
	{
		message("%s Access denied (messagebox unaccessable).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (argc != 2)
	{
		message("%s usage: %s <message>.\r\n", VBOXD_VAL_BADARGS, argv[0]);

		return;
	}

	if (!(msgname = rindex(argv[1], '/')))
	{
		msgname = argv[1];
	}
	else msgname++;

	if (unlink(msgname) != 0)
	{
		message("%s: Can't delete message.\r\n", VBOXD_VAL_TEMPERROR);
	}
	else message("%s .\r\n", VBOXD_VAL_DELETEOK);
}

/**************************************************************************/
/** srv_statusctrl(): Check if control file exists.             [server] **/
/**************************************************************************/

static void srv_statusctrl(int argc, char **argv)
{
	char *msgname;

	if ((!client_home) || (!client_user) || (!(client_access & VBOXD_ACC_READ)))
	{
		message("%s Access denied (no read access).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (argc != 2)
	{
		message("%s usage: %s <control>.\r\n", VBOXD_VAL_BADARGS, argv[0]);

		return;
	}

	if (!(msgname = rindex(argv[1], '/')))
	{
		msgname = argv[1];
	}
	else msgname++;

	if (strncmp(msgname, CTRL_NAME_MAGIC, strlen(CTRL_NAME_MAGIC)) == 0)
	{
		message("%s %d\r\n", VBOXD_VAL_STATUSCTRLOK, ctrl_ishere(client_home, msgname));
	}
	else message("%s bad control name.\r\n", VBOXD_VAL_BADARGS);
}

/**************************************************************************/
/** srv_createctrl(): Create control file.                      [server] **/
/**************************************************************************/

static void srv_createctrl(int argc, char **argv)
{
	char *msgname;

	if ((!client_home) || (!client_user) || (!(client_access & VBOXD_ACC_WRITE)))
	{
		message("%s Access denied (no write access).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (argc != 2)
	{
		message("%s usage: %s <control>.\r\n", VBOXD_VAL_BADARGS, argv[0]);

		return;
	}

	if (!(msgname = rindex(argv[1], '/')))
	{
		msgname = argv[1];
	}
	else msgname++;

	if (strncmp(msgname, CTRL_NAME_MAGIC, strlen(CTRL_NAME_MAGIC)) == 0)
	{
		message("%s %d\r\n", VBOXD_VAL_CREATECTRLOK, ctrl_create(client_home, msgname));
	}
	else message("%s bad control name.\r\n", VBOXD_VAL_BADARGS);
}

/**************************************************************************/
/** srv_removectrl(): Remove control file.                      [server] **/
/**************************************************************************/

static void srv_removectrl(int argc, char **argv)
{
	char *msgname;

	if ((!client_home) || (!client_user) || (!(client_access & VBOXD_ACC_WRITE)))
	{
		message("%s Access denied (no write access).\r\n", VBOXD_VAL_ACCESSDENIED);

		return;
	}

	if (argc != 2)
	{
		message("%s usage: %s <control>.\r\n", VBOXD_VAL_BADARGS, argv[0]);

		return;
	}

	if (!(msgname = rindex(argv[1], '/')))
	{
		msgname = argv[1];
	}
	else msgname++;

	if (strncmp(msgname, CTRL_NAME_MAGIC, strlen(CTRL_NAME_MAGIC)) == 0)
	{
		message("%s %d\r\n", VBOXD_VAL_REMOVECTRLOK, ctrl_remove(client_home, msgname));
	}
	else message("%s bad control name.\r\n", VBOXD_VAL_BADARGS);
}

/**************************************************************************/
/** srv_quit(): Quits the connection.                           [server] **/
/**************************************************************************/

static void srv_quit(int argc, char **argv)
{
	leave_program(0);
}
