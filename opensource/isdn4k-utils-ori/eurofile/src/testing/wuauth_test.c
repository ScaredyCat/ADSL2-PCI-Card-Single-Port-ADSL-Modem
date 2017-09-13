/* $Id: wuauth_test.c,v 1.1 1999/06/30 17:19:22 he Exp $ */

# include <stdio.h>
# include <syslog.h>

extern int wuftp_check_user (char *user, char *pw, char *isdnno);
extern char autherrmsg [];
extern int use_accessfile;
extern char * eft_access;


#define BUFSIZE 512

void main () {
	char	bufus [BUFSIZE];
	char	bufpw [BUFSIZE];
	char	bufin [BUFSIZE];
	char   *c;
	int	x;

	printf("accessfile=%s\n",eft_access);
	use_accessfile=1;

	openlog("eftd", LOG_PID | LOG_NDELAY, LOG_DAEMON);

	for (;;) {	
		puts ("Enter a username: ");
		/* Yes, gets() is not buffer overflow save, but this
		 * is only a test program and can be used by root only*/
		c = gets (bufus);
		if ( c==NULL ) break;
		puts ("Enter a password: ");
		gets (bufpw);
		puts ("Enter a ISDN-Number: ");
		gets (bufin);
		x = wuftp_check_user (bufus, bufpw, bufin);
		printf ("check returned: %d\n", x);
		printf ("user-message:   %s\n", autherrmsg);
	}
}



