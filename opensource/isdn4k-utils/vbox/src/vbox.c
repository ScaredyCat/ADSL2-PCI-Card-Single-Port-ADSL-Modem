/*
** $Id: vbox.c,v 1.11 2002/01/31 20:10:20 paul Exp $
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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <ncurses.h>
#include <panel.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <getopt.h>

#include "libvbox.h"
#include "vbox.h"
#include "streamio.h"

/** Variables *************************************************************/

static char loginname[VBOX_MAX_USERNAME + 1];
static char loginpass[VBOX_MAX_PASSWORD + 1];

static char *messagesmp = NULL;
static int   messagesnr = 0;
static char *vbasename  = NULL;
static char *vboxdname  = NULL;
static char *playerbin  = NULL;
static int   vboxdport  = -1;
static int   usecolors  = FALSE;
static int   leaveloop  = FALSE;
static int   leavevbox  = FALSE;
static int   monocolor  = FALSE;
static int   ledstatus  = TRUE;
static int   rloadtime  = 0;
static int   messagenr  = 0;
static int   messageyp  = 0;
static int   sndvolume  = 10;
static int   newmessys  = FALSE;
static int   forcepass  = FALSE;

static struct statusled statusleds[] =
{
	{  1, CTRL_NAME_STOP     , 0, "" },
	{  5, CTRL_NAME_REJECT   , 0, "" },
	{  7, CTRL_NAME_ANSWERNOW, 0, "" },
	{  9, CTRL_NAME_ANSWERALL, 0, "" },
	{ 11, CTRL_NAME_AUDIO    , 0, "" },
	{ 13, CTRL_NAME_SUSPEND  , 0, "" },
	{ -1, NULL               , 0, "" }
};

static struct colortable colortable[] =
{
	{ NULL             ,  0, 0           , 0          , A_NORMAL, A_REVERSE        },
	{ "C_BACKGROUND"   ,  1, COLOR_WHITE , COLOR_BLACK, A_NORMAL, A_NORMAL         },
	{ "C_STATUSBAR"    ,  2, COLOR_WHITE , COLOR_BLUE , A_NORMAL, A_REVERSE        },
	{ "C_STATUSBAR_HL" ,  3, COLOR_YELLOW, COLOR_BLUE , A_BOLD  , A_REVERSE|A_BOLD },
   { "C_POWERLED_ON"  ,  4, COLOR_GREEN , COLOR_BLUE , A_NORMAL, A_REVERSE|A_BOLD },
	{ "C_POWERLED_OFF" ,  5, COLOR_RED   , COLOR_BLUE , A_NORMAL, A_REVERSE        },
	{ "C_STATUSLED_ON" ,  6, COLOR_YELLOW, COLOR_BLUE , A_BOLD  , A_REVERSE|A_BOLD },
	{ "C_STATUSLED_OFF",  7, COLOR_BLACK , COLOR_BLUE , A_NORMAL, A_REVERSE        },
   { "C_LIST"         ,  8, COLOR_WHITE , COLOR_BLACK, A_NORMAL, A_NORMAL         },
	{ "C_LIST_SELECTED",  9, COLOR_WHITE , COLOR_RED  , A_NORMAL, A_REVERSE        },
	{ "C_INFOTEXT"     , 10, COLOR_GREEN , COLOR_BLACK, A_NORMAL, A_NORMAL         },
	{ "C_HELP"         , 11, COLOR_WHITE , COLOR_BLUE , A_NORMAL, A_REVERSE        },
   { "C_HELP_BORDER"  , 12, COLOR_YELLOW, COLOR_BLUE , A_BOLD  , A_REVERSE|A_BOLD },
	{ "C_STATUS"       , 13, COLOR_WHITE , COLOR_RED  , A_NORMAL, A_REVERSE        },
	{ "C_STATUS_BORDER", 14, COLOR_YELLOW, COLOR_RED  , A_BOLD  , A_REVERSE|A_BOLD },
	{ "C_INFO"         , 15, COLOR_WHITE , COLOR_YELLOW, A_NORMAL, A_REVERSE        },
   { "C_INFO_BORDER"  , 16, COLOR_YELLOW, COLOR_YELLOW, A_BOLD  , A_REVERSE|A_BOLD },
   { NULL             , -1, 0           , 0          , 0       , 0                }
};

static char *colornames[] =
{
	"BLACK", "RED", "GREEN", "BROWN", "BLUE", "MAGENTA", "CYAN", "GRAY",
	"DARKGRAY", "LIGHTRED", "LIGHTGREEN", "YELLOW", "LIGHTBLUE", "LIGHTMAGENTA",
	"LIGHTCYAN", "WHITE",
	NULL
};

static struct option arguments[] =
{
	{ "version"	   , no_argument		  , NULL, 'v' },
	{ "help"		   , no_argument		  , NULL, 'h' },
	{ "mono"		   , no_argument		  , NULL, 'o' },
	{ "force"	   , no_argument		  , NULL, 'f' },
	{ "noledstatus", no_argument		  , NULL, 's' },
	{ "hostname"   , required_argument , NULL, 'm' },
	{ "reload"     , required_argument , NULL, 'r' },
	{ "port"       , required_argument , NULL, 'p' },
	{ "playcmd"    , required_argument , NULL, 'c' },
	{ NULL		   , 0					  , NULL,  0  }
};

/** Prototypes ************************************************************/

static int    message(char *, char *, ...);
static int    sort_message_list(const void *, const void *);
static int    count_new_messages(void);
static int    delete_message(char *);
static int    get_color_nr(char *);
static int    load_message(int, int);
static int    init_screen(void);
static void   exit_screen(void);
static void   draw_main_screen(void);
static void   draw_status_bar(void);
static void   draw_bottom_bar(void);
static void   draw_ctrl_status(void);
static void   draw_message_list(void);
static void   draw_message_line(int, int, int);
static void   clear_screen(void);
static void   process_input(void);
static void   get_message_list(void);
static void   sig_handling_resize(int);
static void   sig_handling_interrupt(int);
static void   one_line_down(void);
static void   one_line_up(void);
static void   init_locale(void);
static void   play_message(int);
static void   toggle_new_flag(int);
static void   toggle_delete_flag(int);
static void   transfer_message_list(void);
static void   delete_selected_messages(void);
static void   parse_vboxrc(void);
static void   parse_colors(char *, char *);
static void   usage(void);
static void   version(void);
static void   help(void);
static void   status(void);
static void   statuscontrol(int);
static void   set_window_background(WINDOW *, chtype);
static void   messageinfo(int);
static chtype color(int);

/**************************************************************************/
/** The magic main...                                                    **/
/**************************************************************************/

int main(int argc, char **argv)
{
	struct servent *vboxdserv;
	int             dimension;
	char           *pass;
	int             opts;

	if (!(vbasename = rindex(argv[0], '/')))
	{
		vbasename = argv[0];
	}
	else vbasename++;

	init_locale();

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT , SIG_IGN);

	vboxdname = "localhost";
	vboxdport = -1;
	rloadtime = 60;
	monocolor = FALSE;
	ledstatus = TRUE;
	playerbin = PLAY;
	forcepass = FALSE;

	*loginname = '\0';
	*loginpass = '\0';

	parse_vboxrc();

	while ((opts = getopt_long(argc, argv, "vhofsm:r:p:c:", arguments, (int *)0)) != EOF)
	{
		switch (opts)
		{
			case 'o':
				monocolor = TRUE;
				break;

			case 'f':
				forcepass = TRUE;
				break;

			case 's':
				ledstatus = FALSE;
				break;

			case 'm':
				vboxdname = optarg;
				break;

			case 'p':
				vboxdport = xstrtol(optarg, -1);
				break;

			case 'r':
				rloadtime = xstrtol(optarg, 60);
				break;

			case 'c':
				playerbin = optarg;
				break;

			case 'h':
				usage();
				break;

			case 'v':
				version();
				break;
		}
	}

	/*
	 * Check if login and password are given. If not, we prompt for user
	 * input.
	 */

	if ((!*loginname) || (!*loginpass) || (forcepass))
	{
		printf("\n");
		printf("Username: ");
		fflush(stdout);

		fgets(loginname, VBOX_MAX_USERNAME, stdin);

		loginname[strlen(loginname) - 1] = '\0';

		if ((pass = getpass("Password: ")))
		{
			xstrncpy(loginpass, pass, VBOX_MAX_PASSWORD);

			memset(pass, '\0', strlen(pass));
		}
		else *loginpass = '\0';

		printf("\n");
		fflush(stdout);
	}

	if ((!loginname) || (!*loginname) || (!loginpass) || (!*loginpass))
	{
		fprintf(stderr, "%s: you must enter a login name and a password.\n", vbasename);

		exit(5);
	}

	/*
	 * Connect to the server, login and get the list with all messages
	 * for the user.
	 */

	if (vboxdport == -1)
	{
		if (!(vboxdserv = getservbyname("vboxd", "tcp")))
		{
			fprintf(stderr, "%s: can't get service 'vboxd/tcp' - please read the manual.\n", vbasename);

			exit(5);
		}

		vboxdport = ntohs(vboxdserv->s_port);
	}

	fprintf(stderr, "Connecting to '%s:%d'...\n", vboxdname, vboxdport);

	if (vboxd_connect(vboxdname, vboxdport) != VBOXC_ERR_OK)
	{
		fprintf(stderr, "%s: can't connect to '%s:%d'.\n", vbasename, vboxdname, vboxdport);

		exit(5);
	}

	if (vboxd_login(loginname, loginpass) != VBOXC_ERR_OK)
	{
		memset(loginpass, '\0', VBOX_MAX_PASSWORD);

		fprintf(stderr, "%s: can't login as '%s'.\n", vbasename, loginname);

		vboxd_disconnect();
		exit(5);
	}

	memset(loginpass, '\0', VBOX_MAX_PASSWORD);

	fprintf(stderr, "Transfering message list...\n");

	get_message_list();

	/*
	 * Do a endless loop: initialize ncurses, draw the screen, process user
	 * action & exit ncurses. This loop will only end if the variable leave-
	 * vbox is true.
	 */

	leavevbox = FALSE;
	dimension = TRUE;

	if (init_screen())
	{
		while (!leavevbox)
		{
			if ((LINES >= 24) && (COLS >= 80))
			{
				draw_main_screen();
				process_input();
			}
			else
			{
				dimension = FALSE;
				leavevbox = TRUE;
			}
		}

		exit_screen();
	}
	else fprintf(stderr, "%s: can't initialize screen.\n", vbasename);

	if (!dimension)
	{
		fprintf(stderr, "%s: screen dimensions too small - need 80x24 or greater.\n", vbasename);
	}

	vboxd_disconnect();

	if (messagesmp) free(messagesmp);
	return 0;
}

/**************************************************************************/
/** usage(): Displays usage message.                                     **/
/**************************************************************************/

static void usage(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s OPTION [ OPTION ] [ ... ]\n", vbasename);
	fprintf(stderr, "\n");
	fprintf(stderr, "-m, --hostname NAME   Connect to host NAME (localhost).\n");
	fprintf(stderr, "-p, --port PORT       Connect to port PORT (vboxd/tcp).\n");
	fprintf(stderr, "-c, --playcmd PROG    Use PROG to play messages (%s).\n", playerbin);
	fprintf(stderr, "-r, --reload SECS     Reload message list all SECS seconds (%d).\n", rloadtime);
	fprintf(stderr, "-o, --mono            Force mono color (color if terminal have).\n");
	fprintf(stderr, "-f, --force           Force login prompt.\n");
	fprintf(stderr, "-s, --noledstatus     Don't get led status from server (get status).\n");
	fprintf(stderr, "-v, --version         Display program version.\n");
	fprintf(stderr, "-h, --help            Display usage message.\n");
	fprintf(stderr, "\n");

	exit(1);
}

/**************************************************************************/
/** version(): Displays program version.                                 **/
/**************************************************************************/

static void version(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "%s version %s (%s)\n", vbasename, VERSION, VERDATE);
	fprintf(stderr, "\n");

	exit(1);
}

/**************************************************************************/
/** process_input(): Main input loop.                                    **/
/**************************************************************************/

static void process_input(void)
{
	int timerreload;
	int timerstatus;
	int c;

	signal(SIGWINCH, sig_handling_resize);
	signal(SIGINT  , sig_handling_interrupt);
	signal(SIGTERM , sig_handling_interrupt);
	signal(SIGHUP  , sig_handling_interrupt);
	signal(SIGQUIT , SIG_IGN);

	timerstatus = 0;
	timerreload = 0;
	messageyp   = 4;
   messagenr   = 0;
	leaveloop   = FALSE;

	noecho();
	cbreak();
	setscrreg(4, LINES - 3);
	scrollok(stdscr, FALSE);
	keypad(stdscr, TRUE);

	draw_message_line(messageyp, messagenr, TRUE);

	while (!leaveloop)
	{
		timeout(1000);
		curs_set(0);
		move((LINES - 2), 0);

		c = wgetch(stdscr);

		if (c != ERR) timerreload = 0;

		switch (c)
		{
			case 27:
				c = wgetch(stdscr);
				break;

			case ERR:
				if (++timerstatus > 30)
				{
					timerstatus = 0;

					draw_ctrl_status();
				}
				if (++timerreload > rloadtime)
				{
					timerreload = 0;

					transfer_message_list();
				}
				break;

			case 'Q':
			case 'q':
				delete_selected_messages();
				leavevbox = TRUE;
				leaveloop = TRUE;
				break;

			case 'R':
			case 'r':
				transfer_message_list();
				break;

			case KEY_DOWN:
				one_line_down();
				break;

			case KEY_UP:
				one_line_up();
				break;

			case '+':
				if (sndvolume < 1000)
				{
					sndvolume++;

					draw_bottom_bar();
				}
				break;

			case '-':
				if (sndvolume > 0)
				{
					sndvolume--;

					draw_bottom_bar();
				}
				break;

			case 'D':
			case 'd':
				toggle_delete_flag(messagenr);
				one_line_down();
				break;

			case 'T':
			case 't':
			case 'N':
			case 'n':
				toggle_new_flag(messagenr);
				draw_bottom_bar();
				one_line_down();
				break;

			case '\r':
			case '\n':
				play_message(messagenr);
				draw_message_line(messageyp, messagenr, TRUE);
				draw_bottom_bar();
				break;

			case 'H':
			case 'h':
				help();
				break;

			case 'S':
			case 's':
				status();
				break;

			case 'I':
			case 'i':
				messageinfo(messagenr);
				break;

			default:
				beep();
				break;
		}
	}
}

/**************************************************************************/
/** status(): Displays status window.                                    **/
/**************************************************************************/

static void status(void)
{
	WINDOW *win;
	PANEL  *pan;
	int     c;
	int     noop;
	int     done;
	int     max;
   int     led;
	int     w;
	int     h;
	char   *b;

	if (ledstatus)
	{
		led = 0;

		while (statusleds[led].name) led++;

		h = (4 + led);
		w = 48;
		b = " Press 'Q' to quit ";

		if ((win = newwin(h, w, 1, 0)))
		{
			if ((pan = new_panel(win)))
			{
				set_window_background(win, color(13));
				wattrset(win, color(14));
				box(win, ACS_VLINE, ACS_HLINE);

				mvwprintw(win, (h - 1), ((w - strlen(b)) / 2), "%s", b);

				wattrset(win, color(13));

				max = 0;
				led = 0;

				while (statusleds[led].name)
				{
					max = led;

					mvwprintw(win, 0, statusleds[led].x, "%d", led);
					mvwprintw(win, (2 + led), 3, "%d) %s '%s' control...", led, (statusleds[led].status ? "Remove" : "Create"), statusleds[led].name);

					led++;
				}

				wmove(win, (h - 2), (w - 2));
				update_panels();
				wrefresh(win);

				done = FALSE;
				noop = 0;

				cbreak();
				noecho();
				wtimeout(win, 1000);

				while (!done)
				{
					switch ((c = wgetch(win)))
					{
						case ERR:
							if (++noop > 30)
							{
								vboxd_put_message("NOOP");

								noop = 0;
							}
							break;

						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							c -= 48;

							if ((c >= 0) && (c <= max))
							{
								statuscontrol(c);

								done = TRUE;
							}

							break;

						case 'Q':
						case 'q':
							done = TRUE;
							break;
					}
				}

				del_panel(pan);
			}

			delwin(win);
		}
	}
	else message("\r\n", "You must enable 'ledstatus' first!%s", " [RETURN]");
}

/**************************************************************************/
/** statuscontrol(): Removes or creates a control file.                  **/
/**************************************************************************/

static void statuscontrol(int nr)
{
	char *resp;
	char *todo;
	char *answer;
	int   status;

	status = statusleds[nr].status;

	message("", "%s '%s'...", (status ? "Removing" : "Creating"), statusleds[nr].name);

	if (status)
	{
		todo = "removectrl";
		resp = VBOXD_VAL_REMOVECTRLOK;
	}
	else
	{
		todo = "createctrl";
		resp = VBOXD_VAL_CREATECTRLOK;
	}

	vboxd_put_message("%s %s", todo, statusleds[nr].name);

	if ((answer = vboxd_get_message()))
	{
		if (vboxd_test_response(resp))
		{
			if (answer[4] != '1') message("\r\n", "Can't %s '%s'! [RETURN]", (status ? "remove" : "create"), statusleds[nr].name);

			draw_ctrl_status();
		}
	}

	draw_bottom_bar();
}

/**************************************************************************/
/** help(): Displays the help window.                                    **/
/**************************************************************************/

static void help(void)
{
	WINDOW *win;
	PANEL  *pan;
	int     h;
   int     w;
	char   *t;
	char   *b;
	int     n;
	int     done;

   w = 37;
   h = 12;
	n = 0;
	t = " HELP ";
   b = " Press 'Q' to quit ";

	if ((win = newwin(h, w, ((LINES - h) / 2), ((COLS - w) / 2))))
	{
		if ((pan = new_panel(win)))
		{
			set_window_background(win, color(11));
			wattrset(win, color(12));
			box(win, ACS_VLINE, ACS_HLINE);

			mvwprintw(win,       0, ((w - strlen(t)) / 2), "%s", t);
			mvwprintw(win, (h - 1), ((w - strlen(b)) / 2), "%s", b);

			wattrset(win, color(11));

			mvwprintw(win,  2, 3, "RETURN             Play message");
			mvwprintw(win,  3, 3, "R           Reload message list");
			mvwprintw(win,  4, 3, "N            Toggle read/unread");
			mvwprintw(win,  5, 3, "D        Toggle delete/undelete");
			mvwprintw(win,  6, 3, "S                Status control");
			mvwprintw(win,  7, 3, "I           Message information");
			mvwprintw(win,  8, 3, "+/-                  Volume +/-");
			mvwprintw(win,  9, 3, "Q                     Quit vbox");

			update_panels();
			wrefresh(win);
			move((LINES - 2), 0);

			cbreak();
			noecho();
			wtimeout(win, 1000);

			done = FALSE;

			while (!done)
			{
				switch (wgetch(win))
				{
					case ERR:
						if (++n > 30)
						{
							vboxd_put_message("NOOP");

							n = 0;
						}
						break;

					case 'Q':
					case 'q':
						done = TRUE;
						break;
				}
			}

			del_panel(pan);
		}

		delwin(win);
	}
}

/**************************************************************************/
/** parse_vboxrc(): Reads and parse ~/.vboxrc.                           **/
/**************************************************************************/

void parse_vboxrc(void)
{
	struct passwd *userpw;
	streamio_t    *vboxrc;
	char           rcname[PATH_MAX + 1];
	char           rcline[128 + 1];
	char          *cmd;
	char          *arg;

	if ((userpw = getpwuid(getuid())))
	{
		xstrncpy(rcname, userpw->pw_dir, PATH_MAX);
		xstrncat(rcname, "/.vboxrc"    , PATH_MAX);

		if ((vboxrc = streamio_open(rcname)))
		{
			while (streamio_gets(rcline, 128, vboxrc))
			{
				cmd = strtok(rcline, "\t ");
				arg = strtok(NULL  , "\t ");

				if ((cmd) && (arg))
				{
					if (strncasecmp(cmd, "C_", 2) == 0)
					{
						parse_colors(cmd, arg);
					}
					else
					{
						if (strcasecmp(cmd, "USERNAME") == 0)
						{
							xstrncpy(loginname, arg, VBOX_MAX_USERNAME);

							continue;
						}

						if (strcasecmp(cmd, "PASSWORD") == 0)
						{
							xstrncpy(loginpass, arg, VBOX_MAX_PASSWORD);

							continue;
						}

						if (strcasecmp(cmd, "VOLUME") == 0)
						{
							sndvolume = atoi(arg);

							if (sndvolume < 0  ) sndvolume = 0;
							if (sndvolume > 999) sndvolume = 999;
						}
					}
				}
			}

			streamio_close(vboxrc);
		}
	}
}

/**************************************************************************/
/** parse_colors(): Parse a colorname.                                   **/
/**************************************************************************/

static void parse_colors(char *cmd, char *arg)
{
	char *ctext;
	char *cback;
	int   i;

	ctext = arg;

	if ((cback = index(ctext, ':'))) *cback++ = '\0';

	i = 1;

	while (colortable[i].pair > 0)
	{
		if (strcasecmp(colortable[i].name, cmd) == 0)
		{
			colortable[i].fg = get_color_nr(ctext);
			colortable[i].bg = get_color_nr(cback);

			colortable[i].cattr = (colortable[i].fg > 7 ? A_BOLD : A_NORMAL);

			while (colortable[i].bg > 7) colortable[i].bg -= 8;
			while (colortable[i].fg > 7) colortable[i].fg -= 8;

			break;
		}

		i++;
	}
}

/**************************************************************************/
/** get_color_nr(): Returns the color number of a color in colortable.   **/
/**************************************************************************/

static int get_color_nr(char *cname)
{
	int i;

	if ((cname) && (*cname))
	{
		for (i = 0; i < 16; i++)
		{
			if (strcasecmp(colornames[i], cname) == 0) return(i);
		}
	}

	return(0);
}

/**************************************************************************/
/** delete_selected_messages(): Deletes all selected messages.           **/
/**************************************************************************/

static void delete_selected_messages(void)
{
	struct messageline *msgline;
	int                 i;
	int                 s;

	if ((messagesmp) && (messagesnr > 0))
	{
		for (s = 0, i = 0; i < messagesnr; i++)
		{
			msgline = (struct messageline *)(messagesmp + (sizeof(struct messageline) * i));

			if (msgline->delete) s++;
		}

		if (s > 0)
		{
			i = message("yn", "Really delete all marked messages (y/n)? ");

			if (i == 'y')
			{
				for (i = 0; i < messagesnr; i++)
				{
					msgline = (struct messageline *)(messagesmp + (sizeof(struct messageline) * i));

					if (msgline->delete)
					{
						message("", "Removing message #%d...", i);

						if (!delete_message(msgline->messagename)) break;
					}
				}
			}
		}
	}
}

/**************************************************************************/
/** delete_message(): Deletes one message.                               **/
/**************************************************************************/

static int delete_message(char *name)
{
	vboxd_put_message("delete %s", name);

	if (vboxd_get_message())
	{
		if (vboxd_test_response(VBOXD_VAL_DELETEOK)) returnok();

		message("\r\n", "Can't delete; invalid server response! %s", "[RETURN]");
	}
	else message("\r\n", "Can't delete; no server response! %s", "[RETURN]");

	returnerror();
}

/**************************************************************************/
/** toggle_new_flag(): Toggles the message new flag.                     **/
/**************************************************************************/

static void toggle_new_flag(int nr)
{
	struct messageline *msgline;
	char               *answer;

	if ((messagesmp) && (messagesnr > 0))
	{
		message("", "Toggle message new flag...");

		msgline = (struct messageline *)(messagesmp + (sizeof(struct messageline) * nr));

		vboxd_put_message("toggle %s", msgline->messagename);

		if ((answer = vboxd_get_message()))
		{
			if (vboxd_test_response(VBOXD_VAL_TOGGLE))
			{
				msgline->mtime = xstrtoul(&answer[4], 0);
				msgline->new   = msgline->mtime > 0 ? TRUE : FALSE;

				draw_message_line(messageyp, messagenr, TRUE);
			}
			else message("\r\n", "Can't toggle; invalid server response! %s", "[RETURN]");
		}
		else message("\r\n", "Can't toggle; no answer from server! %s", "[RETURN]");
	}
}

/**************************************************************************/
/** toggle_delete_flag(): Toggles the message delete flag.               **/
/**************************************************************************/

static void toggle_delete_flag(int nr)
{
	struct messageline *msgline;

	if ((messagesmp) && (messagesnr > 0))
	{
		msgline = (struct messageline *)(messagesmp + (sizeof(struct messageline) * nr));

		msgline->delete = msgline->delete ? FALSE : TRUE;

		draw_message_line(messageyp, messagenr, TRUE);
	}
}

/**************************************************************************/
/** transfer_message_list(): Transfers the message list.                 **/
/**************************************************************************/

static void transfer_message_list(void)
{
	message("", "Transfering message list...");

	get_message_list();

	if (newmessys)
	{
		draw_message_list();
		messagenr = 0;
		messageyp = 4;
		draw_message_line(messageyp, messagenr, TRUE);
	}

	draw_bottom_bar();
}

/**************************************************************************/
/** color(): Returns the attributes for a given color.                   **/
/**************************************************************************/

static chtype color(int c)
{
	if ((usecolors) && (!monocolor))
	{
		return(COLOR_PAIR(colortable[c].pair) | colortable[c].cattr);
	}
	else return(COLOR_PAIR(0) | colortable[c].mattr);
}

/**************************************************************************/
/** draw_main_screen(): Draw the hole main screen.                       **/
/**************************************************************************/

static void draw_main_screen(void)
{
	clear_screen();
	draw_bottom_bar();
	draw_status_bar();
	draw_message_list();
}

/**************************************************************************/
/** clear_screen(): Clears screen and set background.                    **/
/**************************************************************************/

static void clear_screen(void)
{
	set_window_background(stdscr, COLTAB(1));
}

/**************************************************************************/
/** init_screen(): Initialize the ncurses screen and the colors.         **/
/**************************************************************************/

static int init_screen(void)
{
	int i;

	if (initscr())
	{
		if (start_color() != ERR)
		{
			if ((has_colors()) && (!monocolor))
			{
				if ((COLORS >= 8) && (COLOR_PAIRS >= 64))
				{
					i = 1;

					while (colortable[i].pair > 0)
					{
						init_pair(colortable[i].pair, colortable[i].fg, colortable[i].bg);

						i++;
					}

					usecolors = TRUE;
				}
			}
		}

		returnok();
	}

	returnerror();
}

/**************************************************************************/
/** exit_screen(): Exit ncurses screen management.                       **/
/**************************************************************************/

static void exit_screen(void)
{
	endwin();
	printf("%c%c%c\n", 27, 'c', 12);
	fflush(stdout);
}

/**************************************************************************/
/** sig_handling_resize(): Signal handler for screen resizing.           **/
/**************************************************************************/

static void sig_handling_resize(int s)
{
#ifdef HAVE_RESIZETERM

	struct winsize win;
	int            newsizec;
	int            newsizel;

	newsizel = LINES;
	newsizec = COLS;

   if (ioctl(0, TIOCGWINSZ, &win) == 0)
	{
      if (win.ws_row != 0) newsizel = win.ws_row;
      if (win.ws_col != 0) newsizec = win.ws_col;
   }

	if (resizeterm(newsizel, newsizec) == ERR) leavevbox = TRUE;

#endif

	leaveloop = TRUE;
}

/**************************************************************************/
/** sig_handling_interrupt(): Signal handler for ctrl-c or other breaks. **/
/**************************************************************************/

static void sig_handling_interrupt(int s)
{
	leaveloop = TRUE;
	leavevbox = TRUE;

	/*
	 * We *not* re-set the interrupt handler, so a 2nd control-c will
	 * break the program!
	 */
}

/**************************************************************************/
/** get_message_list(): Get the list with all messages. The list replace **/
/**                     any old messages. If the function can't get the  **/
/**                     new list, the old one is not touched.            **/
/**************************************************************************/

static void get_message_list(void)
{
	struct messageline *mrgline;
	struct messageline *msgline;
	char               *newline;
	char               *tmpmessagesmp;
	char               *newmessagesmp;
	int                 newmessagesnr;
	int                 o;
	int                 n;

	newmessagesmp = NULL;
	newmessagesnr = 0;
	newmessys     = FALSE;

	vboxd_put_message("list");

	while ((newline = vboxd_get_message()))
	{
		if ((!vboxd_test_response(VBOXD_VAL_LIST)) || (strlen(newline) < 5))
		{
			if (newmessagesmp) free(newmessagesmp);

			return;
		}

		if (newline[4] == '.') break;

		if (newline[4] == '+')
		{
			newmessagesnr++;

			if ((tmpmessagesmp = realloc(newmessagesmp, (sizeof(struct messageline) * newmessagesnr))))
			{
				newmessagesmp = tmpmessagesmp;

				msgline = (struct messageline *)(newmessagesmp + (sizeof(struct messageline) * (newmessagesnr - 1)));

				msgline->ctime        = 0;
				msgline->mtime        = 0;
				msgline->compression  = 0;
				msgline->size         = 0;
				msgline->new          = FALSE;
				msgline->delete       = FALSE;

				strcpy(msgline->messagename, "");
				strcpy(msgline->name       , "");
				strcpy(msgline->callerid   , "");
				strcpy(msgline->phone      , "");
				strcpy(msgline->location   , "");
			}
			else newmessagesnr--;

			continue;
		}

		if (newmessagesnr > 0)
		{
			msgline = (struct messageline *)(newmessagesmp + (sizeof(struct messageline) * (newmessagesnr - 1)));

			switch (newline[4])
			{
				case 'F':
					xstrncpy(msgline->messagename, &newline[6], NAME_MAX);
					break;

				case 'N':
					xstrncpy(msgline->name, &newline[6], VAH_MAX_NAME);
					break;

				case 'I':
					xstrncpy(msgline->callerid, &newline[6], VAH_MAX_CALLERID);
					break;

				case 'P':
					xstrncpy(msgline->phone, &newline[6], VAH_MAX_PHONE);
					break;

				case 'L':
					xstrncpy(msgline->location, &newline[6], VAH_MAX_LOCATION);
					break;

				case 'T':
					msgline->ctime = (time_t)xstrtoul(&newline[6], 0);
					break;

				case 'M':
					msgline->mtime = (time_t)xstrtoul(&newline[6], 0);
					msgline->new = msgline->mtime > 0 ? TRUE : FALSE;
					break;

				case 'C':
					msgline->compression = (int)xstrtol(&newline[6], 6);
					break;

				case 'S':
					msgline->size = (int)xstrtol(&newline[6], 0);
					break;
			}
		}
	}

	if (newmessagesnr > 0)
	{
		if (messagesmp)
		{
			if (messagesnr > 0)
			{
				/*
				 * Try to merge the old and the new message list, so no status
				 * flags are lost.
				 */

				for (n = 0; n < newmessagesnr; n++)
				{
					msgline = (struct messageline *)(newmessagesmp + (sizeof(struct messageline) * n));

					for (o = 0; o < messagesnr; o++)
					{
						mrgline = (struct messageline *)(messagesmp + (sizeof(struct messageline) * o));

						if (strcmp(mrgline->messagename, msgline->messagename) == 0)
						{
							msgline->delete = mrgline->delete;
						}
					}
				}
			}

			free(messagesmp);
		}

		if (newmessagesnr != messagesnr) newmessys = TRUE;

		messagesmp = newmessagesmp;
		messagesnr = newmessagesnr;

		qsort(messagesmp, messagesnr, sizeof(struct messageline), sort_message_list);
	}
}

/**************************************************************************/
/** sort_message_list(): Sorts the message list.                         **/
/**************************************************************************/

static int sort_message_list(const void *a, const void *b)
{
	struct messageline *linea = (struct messageline *)a;
	struct messageline *lineb = (struct messageline *)b;

	if (lineb->ctime == linea->ctime) return( 0);
	if (lineb->ctime  < linea->ctime) return(-1);

	return(1);
}

/**************************************************************************/
/** init_locale(): Starts and initialize the locale functions.           **/
/**************************************************************************/

static void init_locale(void)
{
}

/**************************************************************************/
/** one_line_down(): Moves cursor in message list one line down.         **/
/**************************************************************************/

static void one_line_down(void)
{
	if ((messagesnr > 0) && (messagesmp) && (messagenr < (messagesnr - 1)))
	{
		draw_message_line(messageyp, messagenr, FALSE);

		messagenr++;

		if (messageyp >= (LINES - 3))
		{
			wrefresh(stdscr);
			scrollok(stdscr, TRUE);
			wscrl(stdscr, 1);
			scrollok(stdscr, FALSE);
		}
		else messageyp++;

		draw_message_line(messageyp, messagenr, TRUE);
	}
	else beep();
}

/**************************************************************************/
/** one_line_up(): Moves cursor in message list one line up.             **/
/**************************************************************************/

static void one_line_up(void)
{
	if ((messagesnr > 0) && (messagesmp) && (messagenr > 0))
	{
		draw_message_line(messageyp, messagenr, FALSE);

		messagenr--;

		if (messageyp <= 4)
		{
			wrefresh(stdscr);
			scrollok(stdscr, TRUE);
			wscrl(stdscr, -1);
			scrollok(stdscr, FALSE);
		}
		else messageyp--;

		draw_message_line(messageyp, messagenr, TRUE);
	}
	else beep();
}

/**************************************************************************/
/** draw_message_line(): Draws one message line.                         **/
/**************************************************************************/

static void draw_message_line(int y, int nr, int selected)
{
	struct tm          *msgtime;
	char                strtime[32];
	struct messageline *msgline;
	chtype              msgattr;
	int                 secs;
	int                 mins;

	if ((messagesmp) && (messagesnr >= nr))
	{
		msgline = (struct messageline *)(messagesmp + (sizeof(struct messageline) * nr));
		msgattr = (selected ? COLTAB(9) : COLTAB(8));

		attrset(msgattr);
		mvhline(y, 0, ' ', COLS);

		mvprintw(y, 1, "%s", msgline->mtime > 0 ? "+" : "");
		mvprintw(y, 1, "%s", msgline->delete ? "-" : "");
		
                /* I hate trigraphs */
		strcpy(strtime, "??""-??""?-""??""??"" ??"":??"":??");

		if ((msgtime = localtime(&msgline->ctime)))
		{
			strftime(strtime, 30, "%d-%b-%Y %H:%M:%S", msgtime);
		}

		secs = get_message_ptime(msgline->compression, msgline->size);
		mins = (secs / 60);
		secs = (secs - (mins * 60));

		mvprintw(y, 3, "%s", strtime);
		mvprintw(y, 25, "%02d:%02d", mins, secs), 
		mvprintw(y, 32, "%s", msgline->name);

		if ((strcmp(msgline->phone, "*** Unknown ***")) != 0 && (strcmp(msgline->phone, "<not supported>") != 0) && (strcmp(msgline->phone, "") != 0))
		{
			mvprintw(y, COLS - 3 - strlen(msgline->phone), "%s P", msgline->phone);
		}
		else mvprintw(y, COLS - 3 - strlen(msgline->callerid), "%s I", msgline->callerid);

		move(LINES - 2, 0);

		if (selected) wrefresh(stdscr);
	}
}

/**************************************************************************/
/** draw_ctrl_status(): Gets and draws the control status leds.          **/
/**************************************************************************/

static void draw_ctrl_status(void)
{
	char   *answer;
	int     i;
	int     x;
	chtype  statuschar;
	chtype  statusattr;

	i = 0;
	x = 0;

	while (statusleds[i].name)
	{
		statuschar = '-';
		statusattr = COLTAB(7);

		if (ledstatus)
		{
			if (i == 0) message("", "Getting control status...");

			vboxd_put_message("statusctrl %s", statusleds[i].name);

			statusleds[i].status = TRUE;

			if ((answer = vboxd_get_message()))
			{
				if (vboxd_test_response(VBOXD_VAL_STATUSCTRLOK))
				{
					statuschar = ACS_DIAMOND;

					if (answer[4] == '1')
					{
						statusleds[i].status = TRUE;
						statusattr = i ? COLTAB(6) : COLTAB(5);
					}
					else
					{
						statusleds[i].status = FALSE;
						statusattr = i ? COLTAB(7) : COLTAB(4);
					}
				}
			}
		}

		attrset(statusattr);
		mvaddch(0, statusleds[i].x, statuschar);

		x = statusleds[i].x;

		i++;
	}

	draw_bottom_bar();
}

/**************************************************************************/
/** draw_status_bar(): Draws the top status bar. This function also get  **/
/**                    the control status information.                   **/
/**************************************************************************/

static void draw_status_bar(void)
{
	char *helpmsg = "HELP";
	int   i;
   int   x;

	attrset(COLTAB(2));

	mvhline(0, 0, ' ', COLS);
	mvvline(0, 3, ACS_VLINE, 1);
	mvvline(0, (COLS - strlen(helpmsg) - 7), ACS_VLINE, 1);

	wattrset(stdscr, COLTAB(2));
	mvprintw(0, COLS - strlen(helpmsg) - 5, "( ) %s", helpmsg);
	wattrset(stdscr, COLTAB(3));
	mvprintw(0, COLS - strlen(helpmsg) - 4, "H");

	i = 0;
	x = 0;

	while (statusleds[i].name)
	{
		x = statusleds[i].x;

		i++;
	}

	attrset(COLTAB(2));
	mvaddch(0, (x + 2), ACS_VLINE);
	printw(" %s %s", PACKAGE, VERSION);

	draw_ctrl_status();
}

/**************************************************************************/
/** draw_message_list(): Draws the message list.                         **/
/**************************************************************************/

static void draw_message_list(void)
{
	char phone[30];
	int m;
	int y;

	printstring(phone, "%28.28s", "Number");

	wattrset(stdscr, COLTAB(10));
	mvprintw(2, 1, "* %-20.20s", "Incoming date");
	mvprintw(2, 25, "%5.5s", "Len");
	mvprintw(2, 32, "%-28.28s", "Name");
	mvprintw(2, COLS - 1 - strlen(phone), "%s", phone);

	mvhline(3, 1, ACS_HLINE, (COLS - 2));

	y = 4;
	m = 0;

	while (m < messagesnr)
	{
		draw_message_line(y, m, FALSE);

		y++;
		m++;

		if (y >= (LINES - 2)) break;
	}
}

/**************************************************************************/
/** play_message(): Play the selected message.                           **/
/**************************************************************************/

static void play_message(int msg)
{
	struct messageline *msgline;
	char                msgname[sizeof("/tmp/vboxXXXXXX\0")];
	char               *command;
	char               *answer;
	int                 size;
	int                 have;
	int                 fd;

	if ((!messagesmp) || (messagesnr < 1)) return;

	msgline = (struct messageline *)(messagesmp + (sizeof(struct messageline) * msg));
	if (!msgline)
	{
		message("\r\n", "No message found! (can't happen?) %s", "[RETURN]");
		return;
	}
	strcpy(msgname, "/tmp/vboxXXXXXX");
	if ((fd = mkstemp(msgname)) == -1)
	{
		message("\r\n", "Can't open temporary file! %s", "[RETURN]");
		return;
	}

	message("", "Searching message...");

	vboxd_put_message("message %s", msgline->messagename);

	if ((answer = vboxd_get_message()))
	{
		if (vboxd_test_response(VBOXD_VAL_MESSAGE))
		{
			if ((size = (int)xstrtol(&answer[4], 0)) > 0)
			{
				message("", "Transfering message (%d bytes)...", size);

				if ((have = load_message(fd, size)) == size)
				{
					message("", "Searching end sequence...");

					if ((answer = vboxd_get_message()))
					{
						if (!vboxd_test_response(VBOXD_VAL_MESSAGE))
						{
							message("\r\n", "Can't get end sequence (bad response)! %s", "[RETURN]");
						}
					}
					else message("\r\n", "Can't get end sequence! %s", "[RETURN]");

					close_and_mone(fd);

					size = 100 + strlen(msgname) + strlen(playerbin);

					if ((command = malloc(size)))
					{
						message("", "Playing message...");

						printstring(command, "%s %s %d 1>/dev/null 2>/dev/null", playerbin, msgname, sndvolume);
						system(command);
						free(command);

						if (msgline->new) toggle_new_flag(msg);
					}
				}
				else message("\n\r", "Can only get %d of %d bytes! %s", have, size, "[RETURN]");
			}
			else message("\r\n", "Message size is zero! %s", "[RETURN]");
		}
		else message("\r\n", "Can't get init sequence (bad response)! %s", "[RETURN]");
	}
	else message("\r\n", "Can't get init sequence! %s", "[RETURN]");

	if (fd != -1) close(fd);

	unlink(msgname);
}

/**************************************************************************/
/** load_message(): Loads the message from server.                       **/
/**************************************************************************/

static int load_message(int fd, int size)
{
   struct timeval timeval;
   fd_set         rmask;

	unsigned char temp[256];
	int  have;
	int  take;
   int  rc;

	have = 0;

	while (have < size)
	{
      VBOX_ONE_FD_MASK(&rmask, vboxd_r_fd);

      timeval.tv_sec  = VBOXD_GET_MSG_TIMEOUT;
      timeval.tv_usec = 0;

      rc = select((vboxd_r_fd + 1), &rmask, NULL, NULL, &timeval);

		if (rc <= 0)
		{
			if ((rc < 0) && (errno == EINTR)) continue;

			break;
		}

		if (!FD_ISSET(vboxd_r_fd, &rmask)) break;

		if ((take = (size - have)) > 255) take = 255;

		rc = read(vboxd_r_fd, temp, take);

		if (rc <= 0)
		{
			if ((rc < 0) && (errno == EINTR)) continue;

			break;
		}

		have += rc;

		write(fd, temp, rc);
	}

	return(have);
}

/**************************************************************************/
/** message(): Prints a message into the bottom bar. The function can    **/
/**            also wait for a special keypress. While waiting, a NOOP   **/
/**            message is send all 10 seconds.                           **/
/**************************************************************************/

static int message(char *keys, char *fmt, ...)
{
	char    message[256];
	va_list arg;
	int     k;
	int     timernoop;
	int     returnkey;

	va_start(arg, fmt);
	vnprintstring(message, 255, fmt, arg);
	va_end(arg);

	attrset(COLTAB(2));
	mvhline(LINES - 1, 0, ' ', COLS);
	mvprintw(LINES - 1, 1, "%s", message);
	wrefresh(stdscr);

	returnkey = -1;

	if ((keys) && (*keys))
	{
		timeout(1000);
		noecho();
		cbreak();
		beep();
		refresh();

		timernoop = 0;

		while (returnkey == -1)
		{
			k = wgetch(stdscr);

			if (k == ERR)
			{
				if (timernoop++ > 10)
				{
					vboxd_put_message("noop");

					timernoop = 0;
				}

				continue;
			}

			if (index(keys, k)) returnkey = k;
		}

		draw_bottom_bar();
	}

	return(returnkey);
}

/**************************************************************************/
/** draw_bottom_bar(): Draws the bottom bar.                             **/
/**************************************************************************/

static void draw_bottom_bar(void)
{
	struct passwd *pass;
	char           temp[32];
	char           svol[16];

   attrset(COLTAB(2));
   mvhline(LINES - 1, 0, ' ', COLS);

	printstring(temp, "%d/%d", messagesnr, count_new_messages());
	printstring(svol, "%d", sndvolume);

	mvhline(LINES - 1, COLS - strlen(temp) - 3, ACS_VLINE, 1);
	mvhline(LINES - 1, COLS - strlen(temp) - strlen(svol) - 6, ACS_VLINE, 1);

	mvprintw(LINES - 1, COLS - strlen(temp) - 1, "%s", temp);
	mvprintw(LINES - 1, COLS - strlen(temp) - strlen(svol) - 4, "%s", svol);

	mvprintw(LINES - 1, 1, "%s", loginname);

	if ((pass = getpwuid(getuid())))
	{
		mvprintw(LINES - 1, 2 + strlen(loginname), "[%s]", pass->pw_name);
	}
}

/**************************************************************************/
/** count_new_messages(): Counts new messages in the message list.       **/
/**************************************************************************/

static int count_new_messages(void)
{
	struct messageline *msgline;
	int                 newmsgs;
	int                 i;

	newmsgs = 0;

	if ((messagesmp) && (messagesnr > 0))
	{
		for (i = 0; i < messagesnr; i++)
		{
			msgline = (struct messageline *)(messagesmp + (sizeof(struct messageline) * i));

			if (msgline->new) newmsgs++;
		}
	}

	return(newmsgs);
}

/**************************************************************************/
/** set_window_background(): Clears a windows.                           **/
/**************************************************************************/

static void set_window_background(WINDOW *win, chtype col)
{
	int y;

	wattrset(win, col);

	for (y = 0; y < (win->_maxy + 1); y++)
	{
		mvwhline(win, y, 0, ' ', (win->_maxx + 1));
	}
}

static void messageinfo(int nr)
{
	struct messageline *msgline;
	WINDOW             *win;
	PANEL              *pan;
	int                 h;
   int                 w;
	char               *t;
	char               *b;
	int                 n;
	int                 done;

	if ((!messagesmp) || (messagesnr < nr)) return;

	msgline = (struct messageline *)(messagesmp + (sizeof(struct messageline) * nr));

   w = 51;
   h = 15;
	n = 0;
	t = " INFO ";
   b = " Press 'Q' to quit ";

	if ((win = newwin(h, w, ((LINES - h) / 2), ((COLS - w) / 2))))
	{
		if ((pan = new_panel(win)))
		{
			set_window_background(win, color(15));
			wattrset(win, color(16));
			box(win, ACS_VLINE, ACS_HLINE);

			mvwprintw(win,       0, ((w - strlen(t)) / 2), "%s", t);
			mvwprintw(win, (h - 1), ((w - strlen(b)) / 2), "%s", b);

			wattrset(win, color(15));

			mvwprintw(win,  2, 3, "Filename   : %-32.32s", msgline->messagename);
			mvwprintw(win,  3, 3, "CTime      : %ld", msgline->ctime);
			mvwprintw(win,  4, 3, "MTime      : %ld", msgline->mtime);
			mvwprintw(win,  5, 3, "Compression: %d", msgline->compression);
			mvwprintw(win,  6, 3, "Size       : %d", msgline->size);
			mvwprintw(win,  7, 3, "Name       : %-32.32s", msgline->name);
			mvwprintw(win,  8, 3, "CallerID   : %-32.32s", msgline->callerid);
			mvwprintw(win,  9, 3, "Phone      : %-32.32s", msgline->phone);
			mvwprintw(win, 10, 3, "Location   : %-32.32s", msgline->location);
			mvwprintw(win, 11, 3, "Flag new   : %s", (msgline->new ? "Yes" : "No"));
			mvwprintw(win, 12, 3, "Flag delete: %s", (msgline->delete ? "Yes" : "No"));

			update_panels();
			wrefresh(win);
			wmove(win, (h - 2), (w - 2));

			cbreak();
			noecho();
			wtimeout(win, 1000);

			done = FALSE;

			while (!done)
			{
				switch (wgetch(win))
				{
					case ERR:
						if (++n > 30)
						{
							vboxd_put_message("NOOP");

							n = 0;
						}
						break;

					case 'Q':
					case 'q':
						done = TRUE;
						break;
				}
			}

			del_panel(pan);
		}

		delwin(win);
	}
}
