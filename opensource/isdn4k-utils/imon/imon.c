/* $Id: imon.c,v 1.6 2002/07/15 11:58:32 paul Exp $
 *
 * iMON , extended version.
 * original iMON source (c) Michael Knigge
 * heavily modified and extended by Fritz Elfert
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: imon.c,v $
 * Revision 1.6  2002/07/15 11:58:32  paul
 * Explicitly set stdio buffer for /dev/isdninfo to 2048, as it has
 * happened that stdio used 1024, and /dev/isdninfo doesn't support
 * partial reads (0 bytes are returned). Usually not a problem, but
 * with more than one channel connected, a line of > 1024 bytes is
 * returned.
 * Also optimized the curses usage a bit, explicitly writing spaces
 * so that background colours are set correctly everywhere.
 * Bumped displayed version to 2.2.
 *
 * Revision 1.5  2001/08/22 11:12:00  paul
 * imon wasn't devfs-compliant yet.
 *
 * Revision 1.4  2000/06/13 10:51:06  armin
 * fixed status output if channels are in
 * a different order.
 *
 * Revision 1.3  1997/05/17 12:23:35  fritz
 * Corrected some Copyright notes to refer to GPL.
 *
 * Revision 1.2  1997/03/20 00:19:01  luethje
 * inserted the line #include <errno.h> in avmb1/avmcapictrl.c and imon/imon.c,
 * some bugfixes, new structure in isdnlog/isdnrep/isdnrep.c.
 *
 * Revision 1.1  1997/02/17 00:09:03  fritz
 * New CVS tree
 *
 */

#include <errno.h>
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#ifdef HAVE_NCURSES_H
#	include <ncurses.h>
#endif
#ifdef HAVE_CURSES_H
#	include <curses.h>
#endif
#ifdef HAVE_CURSES_NCURSES_H
#	include <curses/ncurses.h>
#endif
#ifdef HAVE_NCURSES_CURSES_H
#	include <ncurses/curses.h>
#endif

#include <linux/isdn.h>

#define KEY_Q                81
#define KEY_q               113

#define CURSOR_INVISIBLE      0   
#define CURSOR_NORMAL         1
#define CURSOR_BOLD           2

#define WHITE_ON_BLUE         1
#define YELLOW_ON_BLUE        2
#define RED_ON_BLUE           3
#define GREEN_ON_BLUE         4
#define WHITE_ON_BLACK        5

struct phone_entry {
   struct phone_entry *next;
   char phone[30];
   char name[30];
};

static int  enable_quit = 1;
static int  show_names  = 0;
static int  stat_x;
static int  stat_y;
static FILE *isdninfo;
static int  color;
static char *phonebook;
static struct phone_entry *phones;
static char idmap_line[2048];
static char chmap_line[2048];
static char drmap_line[2048];
static char usage_line[2048];
static char flags_line[2048];
static char phone_line[2048];

WINDOW *statwin;
WINDOW *stathdr;
time_t current_time;

static int Star(char *, char *);

/*
 * Wildmat routines
 */
static int wildmat(char *s, char *p) {
  register int   last;
  register int   matched;
  register int   reverse;

  for ( ; *p; s++, p++)
    switch (*p) {
    case '\\':
      /* Literal match with following character; fall through. */
      p++;
    default:
      if (*s != *p)
	return(0);
      continue;
    case '?':
      /* Match anything. */
      if (*s == '\0')
	return(0);
      continue;
    case '*':
      /* Trailing star matches everything. */
      return(*++p ? Star(s, p) : 1);
    case '[':
      /* [^....] means inverse character class. */
      if ((reverse = (p[1] == '^')))
	p++;
      for (last = 0, matched = 0; *++p && (*p != ']'); last = *p)
	/* This next line requires a good C compiler. */
	if (*p == '-' ? *s <= *++p && *s >= last : *s == *p)
	  matched = 1;
      if (matched == reverse)
	return(0);
      continue;
    }
  return(*s == '\0');
}

static int Star(char *s, char *p) {
  while (wildmat(s, p) == 0)
    if (*++s == '\0')
      return(0);
  return(1);
}

/*
 * Read phonebook file and create a linked
 * list of phone entries.
 */
static void readphonebook() {
  FILE *f;
  struct phone_entry *p;
  struct phone_entry *q;
  char line[255];
  char *s;

  if (!(f = fopen(phonebook, "r"))) {
    fprintf(stderr, "Can't open %s\n",phonebook);
  }
  p = phones;
  while (p) {
    q = p->next;
    free(p);
    p = q;
  }
  phones = NULL;
  while (fgets(line,sizeof(line),f)) {
    if ((s = strchr(line,'\n')))
      *s = '\0';
    if (line[0]=='#')
      continue;
    s = strchr(line,'\t');
    *s++ = '\0';
    if (strlen(line) && strlen(s)) {
      q = malloc(sizeof(struct phone_entry));
      q->next = phones;
      strcpy(q->phone,line);
      strcpy(q->name,s);
      phones = q;
    }
  }
  fclose(f);
  if (!phones)
    show_names = 0;
}

/*
 * Find a phone number in list of phone entries.
 * If found, return corresponding name, else
 * return number string.
 */
char *find_name(char *num) {
  struct phone_entry *p = phones;
  static char tmp[30];

  sprintf(tmp, "%-28.28s", num);
  if (!show_names)
	return(tmp);
  while (p) {
    if (wildmat(num, p->phone)) {
      sprintf(tmp, "%-28.28s", p->name);
      break;
    }
    p = p->next;
  }
  return tmp;
}

void cleanup(int dummy) {
  /*
   * okay, all done. clear the screen and quit ncurses
   */
  fclose(isdninfo);
  clear();
  if (color == TRUE) {
    attron(COLOR_PAIR(WHITE_ON_BLACK));
    attroff(A_BOLD);
  }
  endwin();
  exit(0);
}

void usage(void) {
  fprintf(stderr,"usage: imon [-q][-p PhoneBookFile]\n");
  exit(1);
}

/*
 * init ncurses and check if colors can be used.
 */
int imon_init_ncurses(int *color) {
  initscr();
  noecho();
  nonl();
  refresh();
  cbreak();
  nodelay(stdscr,TRUE);
  keypad(stdscr,TRUE);
  curs_set(CURSOR_INVISIBLE);
  statwin = newpad(ISDN_MAX_CHANNELS, 100);
  stathdr = newpad(3, 100);
  if ((*color = has_colors()) == TRUE) {
    start_color();
    init_pair(WHITE_ON_BLUE,   COLOR_WHITE,  COLOR_BLUE);
    init_pair(YELLOW_ON_BLUE,  COLOR_YELLOW, COLOR_BLUE);
    init_pair(RED_ON_BLUE,     COLOR_RED,    COLOR_BLUE);
    init_pair(GREEN_ON_BLUE,   COLOR_GREEN,  COLOR_BLUE);
    init_pair(WHITE_ON_BLACK,  COLOR_WHITE,  COLOR_BLACK);
  }
  return(TRUE);
}

/*
 * Draw the static part of the screen.
 */
int imon_draw_mask(int color) {
  int  line;
  int  col;
  
  if (color == TRUE) {
    attron(A_BOLD);
    wattron(statwin,A_BOLD);
    wattron(stathdr,A_BOLD);
    attron(COLOR_PAIR(WHITE_ON_BLUE));
  }
  move(0, 0);
  addch(ACS_ULCORNER);
  for(col=2; col<COLS; col++)
    addch(ACS_HLINE);
  addch(ACS_URCORNER);
  for (line=1; line<LINES-4; line++) {
    move(line, 0);
    addch(ACS_VLINE);
    for(col=2; col<COLS; col++)
      addch(' ');
    addch(ACS_VLINE);
  }
  move(LINES-4, 0);
  addch(ACS_LLCORNER);
  for(col=2; col<COLS; col++)
    addch(ACS_HLINE);
  addch(ACS_LRCORNER);
  if (color == TRUE) {
    attroff(COLOR_PAIR(WHITE_ON_BLUE));
    attron(COLOR_PAIR(WHITE_ON_BLACK));
  }
  if (enable_quit) {
    move(LINES-2, 1);
    addstr("Press Q to quit");
  }
  if (phonebook) {
    move(LINES-2, 17);
    addstr("Press R - read Phonebook, S - Show ");
    addstr((show_names)?"phonenumbers":"names");
  }
  if(color == TRUE) {
    attroff(COLOR_PAIR(WHITE_ON_BLACK));
    attron(COLOR_PAIR(YELLOW_ON_BLUE));
    wattroff(stathdr, COLOR_PAIR(WHITE_ON_BLACK));
    wattron(stathdr, COLOR_PAIR(YELLOW_ON_BLUE));
  }
  for(line=0; line<3; line++) {
      mvwprintw(stathdr, line, 0, "%100s", "");
  }
  mvaddstr(1, 2, "iMON 2.2");
  mvaddstr(1, COLS-34 , "Last Update: ");
  mvwaddstr(stathdr, 0, 0, "Nr. LineID       Status    Phone Number                Usage       Type");
  wmove(stathdr, 1, 0);
  for(col=0; col<100; col++)
    waddch(stathdr, ACS_HLINE);
  if(color == TRUE) {
    attroff(COLOR_PAIR(YELLOW_ON_BLUE));
    wattroff(stathdr, COLOR_PAIR(YELLOW_ON_BLUE));
  }
  return(TRUE);
}

static void imon_read_status() {
  /*
   * read the 6 important lines
   */   
  fgets(idmap_line, sizeof idmap_line, isdninfo);
  fgets(chmap_line, sizeof chmap_line, isdninfo);
  fgets(drmap_line, sizeof drmap_line, isdninfo);
  fgets(usage_line, sizeof usage_line, isdninfo);
  fgets(flags_line, sizeof flags_line, isdninfo);
  fgets(phone_line, sizeof phone_line, isdninfo);
#ifdef TEST_IMON
  strcpy(idmap_line, "idmap: TA250034 - - - - - - - - - - - - - - -");
  strcpy(chmap_line, "chmap: 0 1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1");
  strcpy(drmap_line, "drmap: 0 0 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1");   
  strcpy(usage_line, "usage: 1 2 0 3 4 0 5 0 129 0 130 131 132 0 0 133");
  strcpy(flags_line, "flags: 0 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ?");
  strcpy(phone_line, "phone: 05114711 0815 ??? 5555 666666666 ??? 4711 ??? 875257 ??? 77848 7890 2345467 ??? ??? 88888");
#endif
}

/*
 * Update the status window.
 */
int imon_draw_status(int color, time_t *current_time) {
  struct tm *now;

  char *ptr_idmap;
  char *ptr_chmap;
  char *ptr_drmap;
  char *ptr_usage;
  char *ptr_flags;
  char *ptr_phone;
  char *s_phone;

  char drvid[15];
  char s_drvid[15];
  char *status;
  char phone[30];
  char *s_usage;
  char *inout;

  int  usage;
  int  chanum;
  int  drvnum;
  int  flags[ISDN_MAX_DRIVERS];
  int  line;
  int  len;
   
  ptr_idmap = idmap_line + 7;
  ptr_chmap = chmap_line + 7;
  ptr_drmap = drmap_line + 7;
  ptr_usage = usage_line + 7;
  ptr_flags = flags_line + 7;
  ptr_phone = phone_line + 7;

  /*
   * get flags for all drivers
   */   
  for(line=0; line<ISDN_MAX_DRIVERS; line++) {
      sscanf(ptr_flags, "%d %n", &flags[line], &len);
      ptr_flags += len;
  }

  /*
   * now print info for every line
   */
  for(line=0; line<ISDN_MAX_CHANNELS; line++) {
    sscanf(ptr_idmap, "%s %n", drvid, &len);
    sprintf(s_drvid,"%-13s",drvid);
    ptr_idmap += len;
    sscanf(ptr_usage, "%d %n", &usage, &len);
    ptr_usage += len;
    sscanf(ptr_phone, "%s %n", phone, &len);
    ptr_phone += len;
    sscanf(ptr_chmap, "%d %n", &chanum, &len);
    ptr_chmap += len;
    sscanf(ptr_drmap, "%d %n", &drvnum, &len);
    ptr_drmap += len;

    /* A channel-number of -1 indicates an nonexistent channel */     
    if (chanum==-1) {
      if (color) {
	wattroff(statwin, A_BOLD);
	wattron(statwin, COLOR_PAIR(WHITE_ON_BLUE));
      }
      mvwprintw(statwin, line, 0, "%2d%98s", line, "");
      continue;
    }
    /*
     * if usage&7 is zero, there is no connection
     */
    if (usage&64)
      wattron(statwin, A_BOLD);
    else
      wattroff(statwin, A_BOLD);
    if (!(usage & 7)) {
      if (color)
	wattron(statwin, COLOR_PAIR(GREEN_ON_BLUE));
      status = (usage&64) ? "Exclusive" : "Offline";
      s_phone = "";
      s_usage = "";
      inout = "";
    }
    else {
      int online = (flags[drvnum] & (1<<chanum));
      if (color) {
	if (online)
	  wattron(statwin, COLOR_PAIR(RED_ON_BLUE));
        else {
          wattron(statwin, A_BOLD);
	  wattron(statwin, COLOR_PAIR(YELLOW_ON_BLUE));
	}
      }
      status = online ? "Online" : "Calling";
      inout = (usage&ISDN_USAGE_OUTGOING)?"Outgoing":"Incoming";
      switch (usage & 7) {
	case ISDN_USAGE_RAW:
	  s_usage = "Raw";
	  break;
	case ISDN_USAGE_MODEM:
	  s_usage = "Modem";
	  break;
	case ISDN_USAGE_NET:
	  s_usage = "Net";
	  break;
	case ISDN_USAGE_VOICE:
	  s_usage = "Voice";
	  break;
	case ISDN_USAGE_FAX:
	  s_usage = "Fax";
	  break;
	default:
	  s_usage = "-----";
	  break;
      }
      s_phone = find_name(phone);
    }
    mvwprintw(statwin, line, 0, "%2d  %-13s%-10s%-28s%-10s%-9s%26s", line, s_drvid, status, s_phone, s_usage, inout, "");
  }
  if (color == TRUE)
    attron(COLOR_PAIR(YELLOW_ON_BLUE));
  now = localtime(current_time);
  mvprintw(1, COLS-21, "%04d-%02d-%02d %02d:%02d:%02d", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour,now->tm_min,now->tm_sec);
  prefresh(stathdr, 0,      stat_x, 3, 2, 5, COLS-3);
  prefresh(statwin, stat_y, stat_x, 5, 2, LINES-5, COLS-3);
  return(TRUE);
}

void do_resize(int dummy) {
  endwin();
  imon_init_ncurses(&color);
  imon_draw_mask(color);
  imon_draw_status(color, &current_time);
  refresh();
  signal(SIGWINCH, do_resize);
}

int main(int argc, char **argv) {
  int   quit , i;
  fd_set fdset;
  struct timeval timeout;
  static char  fbuf[2048];
  
  /*
   * check parameters
   */
  phonebook = NULL;
  phones = NULL;
  for (i=1;i<argc;i++) {
    if (argv[i][0] == '-')
      switch (argv[i][1]) {
      case 'q':
	enable_quit = 0;
	break;
      case 'p':
	if (i<argc) {
	  phonebook = argv[++i];
	  show_names = 1;
	} else usage();
	break;
      default:
	usage();
      } else
        usage();
  }

  if (!(isdninfo = fopen("/dev/isdninfo", "r"))) {
   if (!(isdninfo = fopen("/dev/isdn/isdninfo", "r"))) {
    perror("Can't open /dev/isdn/isdninfo nor /dev/isdninfo");
    return 1;
   }
  }
  setvbuf(isdninfo, fbuf, _IOFBF, sizeof(fbuf));       /* up to 2048 needed */

  if (phonebook)
    readphonebook();
  
  /*
   * initialize ncurses and draw the main screen
   */
  imon_init_ncurses(&color);
  imon_draw_mask(color);
  stat_x = 0;
  stat_y = 0;
  refresh();
  quit = 0;

  /*
   * Install Signal-Handlers
   */
  signal(SIGTERM,cleanup);
  signal(SIGWINCH,do_resize);
  if (enable_quit) {
    signal(SIGHUP,cleanup);
    signal(SIGINT,cleanup);
  } else {
    sigblock(SIGHUP);
    sigblock(SIGINT);
  }
  /*
   * loop until the user pressed  "Q"
   */
  do {
    FD_ZERO(&fdset);
    FD_SET(0,&fdset);
    FD_SET(fileno(isdninfo),&fdset);
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    switch (select(32,&fdset,(fd_set *)0,(fd_set *)0,NULL)) {
    case 0:
      fprintf(stderr,"timeout\n");
      break;
    case 1:
    case 2:
      if (FD_ISSET(0,&fdset)) {
	switch (getch()) {
	case KEY_Q:
	case KEY_q:
	  quit = enable_quit;
	  break;
	case 'R':
	case 'r':
	  if (phonebook)
	    readphonebook();
	  break;
	case 'S':
	case 's':
	  if (phones)
	    show_names = (show_names)?0:1;
	  break;
	case KEY_LEFT:
	  if (stat_x > 0)
	    stat_x--;
	  break;
	case KEY_RIGHT:
	  if (stat_x < (100-COLS-4))
	    stat_x++;
	  break;
	case KEY_UP:
	  if (stat_y > 0)
	    stat_y--;
	  break;
	case KEY_DOWN:
	  if (stat_y < ((ISDN_MAX_CHANNELS + 3) - (LINES-6)))
	    stat_y++;
	  break;
	}
      }
      if (FD_ISSET(fileno(isdninfo),&fdset)) {
	time(&current_time);
        imon_read_status();
      }
      break;
    case -1:
      if (errno != EINTR) {
	perror("select");
	sleep(5);
	return 2;
      }
    }
    imon_draw_status(color, &current_time);
    refresh();
  } while (!quit);
  cleanup(0);
  return 0;
}
