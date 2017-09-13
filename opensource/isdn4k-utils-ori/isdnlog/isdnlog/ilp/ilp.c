/* ilp.c -  isdnlog to procfs interface in /proc/isdnlog 
 *
 * Copyright 2000 by Leopold Toetsch <lt@toetsch.at>
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
 * Changes:
 *
 * 0.10 15.12.2000 lt Initial Version
 * 0.11 21.12.2000 lt STANDALONE test mode, bug fixes
 * 0.12 31.01.2001 lt fixed too long aliases (thx Reinhard Karcher)
 *
 */

#ifdef STANDALONE
#define print_msg(l,f,x,y) printf(f,x,y)
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

struct callt {
	char num[2][30];
	char area[2][30];
	char alias[2][30];
	char vorwahl[2][30];
  	int dialin;
  	double pay;
  	time_t connect;
} call;

typedef struct callt CALL;

#define _ME(call)  (call->dialin ? 1 : 0)
#define _OTHER(call)  (call->dialin ? 0 : 1)
#define CONNECT 7
#define RELEASE 0x77

time_t cur_time;

char *double2clock(double d) {
  return " 0:01:23";
}

#else
#include "isdnlog.h"
#endif

#define PROC_ISDNLOG "/proc/isdnlog"

#define NE(s) (*s ? s : "-")

void procinfo(int chan, CALL *call, int state) 
{
    static int fd = -1;
    static int errcount = 0;
    char s[82];
    char *msn;
    char *alias;
    char *st;
    char *p;
    size_t len;

    print_msg(PRT_INFO, "procinfo: chan %d, state %d\n", chan, state);
        
    /* check for valid B channels and for too much errors */
    if (chan < 1 || chan > 2 || errcount > 2)
	return;
	
    /* special state to clean up */	
    if (state == -1) {
	if (fd != -1)
	    close(fd);
	fd = -1;
	return;
    }	
    
    /* open /proc/isdnlog for writing */
    if (fd == -1) {
	fd = open(PROC_ISDNLOG, O_WRONLY|O_NONBLOCK, O_FSYNC);
	if (fd == -1) {
	    print_msg(PRT_ERR, "Failed to open '%s' for writing: %s\n", 
		PROC_ISDNLOG, strerror(errno));
	    errcount++;
	    return;
	}    
    }
    /* make msn i.e. rightmost 3 digits of own num */
    if (strlen(call->num[_ME(call)]) <= 3)
	msn = call->num[_ME(call)];
    else
	msn = call->num[_ME(call)] + strlen(call->num[_ME(call)]) - 3;
	
    /* alias is alias | area | country */
    alias = *call->alias[_OTHER(call)] ? call->alias[_OTHER(call)] :
	    *call->area[_OTHER(call)] ? call->area[_OTHER(call)] :
	    call->vorwahl[_OTHER(call)]; /* FIXME no country in call? */
	    
    /* format message for channel */
    p = s;
    p += sprintf(s, "%c%2d", chan + '0', chan);	/* channel "1" or "2" */
    switch (state) {
	case CONNECT:
	    st = call->dialin ? "CON__IN" : "CON_OUT";
	    break;
	case RELEASE:
	    st = "HANG_UP";
	    break;
	default:
	    st = "UNKNOWN";    
	    break;
    }    
    p += sprintf(p, " %7s %-3s %c %-25s %-18.18s %-8s",
	st,
	NE(msn),
	call->dialin ? '<' : '>',
	NE(call->num[_OTHER(call)]),
	NE(alias),
	double2clock((double) (cur_time - call->connect)));
    if (!call->dialin) 
	p += sprintf(p, " %7.3f", call->pay);
    strcpy(p, "\n");
    len = strlen(s);
    if (write(fd, s, len) != len) {
	    print_msg(PRT_ERR, "Write error '%s': %s\n", 
		PROC_ISDNLOG, strerror(errno));
	    errcount++;
    }
}

#ifdef STANDALONE

int main(int argc, char *argv[]) {
  time_t now;
  call.dialin = 1;
  strcpy(call.num[0], "41");
  strcpy(call.num[1], argc > 1 ? argv[1] : "1234567");
  strcpy(call.alias[0], "");
  strcpy(call.alias[1], "");
  strcpy(call.area[0], "Hbgtn");
  strcpy(call.area[1], "Wien");
  time(&now);
  call.connect = now;
  call.pay = 1.23;
  procinfo(2, &call, CONNECT);

  return 0;
}

#endif
