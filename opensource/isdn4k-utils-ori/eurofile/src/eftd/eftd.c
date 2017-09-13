/* $Id: eftd.c,v 1.6 2001/03/01 14:59:13 paul Exp $ */
/*
  Copyright 1998 by Henner Eisen

    This code is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/ 

/*
 * eft server. Experimental and incomplete right now. Use with care.
 */

/* for strsignal() */
#define _GNU_SOURCE       

#include <sys/types.h>
#include <sys/param.h>
#include <time.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <linux/x25.h>
/* for error mask setting */
#include <tdu_user.h>
#include <eft.h>
/* is in net/x25.h, not in the public header file linux/x25.h. Why?*/ 
#ifndef X25_ADDR_LEN
#define X25_ADDR_LEN 16
#endif
#ifndef X25_MAX_CUD_LEN
#define X25_MAX_CUD_LEN 128
#endif

#include <pwd.h>

#ifdef  __USE_GNU
/* Return a string describing the meaning of the signal number in SIG.  */
extern char *strsignal __P ((int __sig));
#endif


static time_t session_start;

static void eft_log_accept(char *eftdev, pid_t pid){

	tdu_printf(TDU_LOG_AP1,"SVS: ACCEPT process=eftd[%d] device=%s\n",pid,eftdev);
}

static void eft_log_disconnect(pid_t pid){

	tdu_printf(TDU_LOG_AP1,"SVS: CLEAR  process=eftd[%d] duration=%.f\n", pid, difftime(time(NULL),session_start));
	
}


/*
 * This is passed to the eft_accept() function and checks whether the
 * user requesting the login should be granted access to our eft server.
 *
 * returns 0, if access is permitted or an ETS 300 075 error code otherwise.
 *
 * user_profile is a pointer which is passed transparently through
 * eft_accept() to us. It can be used by us to store data related to
 * the accepted user (like home directory, uid, gid). Such data might be
 * useful to other service routines that want to do further user specific
 * setups later.
*/

#ifdef CONFIG_EFTD_WUAUTH
extern int wuftp_check_user (char *, char *, unsigned char *);
extern char autherrmsg[];
extern char *eft_access;
extern int use_accessfile;
extern int xferlog;
extern int guest;
extern int anonymous;

#endif

static int eft_check_user( struct eft *eft, char* user, char* pass, char *isdn_no )
#ifdef CONFIG_EFTD_WUAUTH
{
	int verified; 
	long flags=eft_get_flags(eft);

	tdu_printf(TDU_LOG_LOG,"checking wu user (user=\"%s\", pass=\"%s\")\n",user,"xxx" /* pass */);
	if( *user == 0 )
		user = "ftp";
	verified = wuftp_check_user(user, pass, isdn_no);
	printf("user check: ruid=%d, euid=%d\n",getuid(),geteuid());
	/* 
	 * Be paranoid about buggy authentification functions that claim
	 * success but are still running with super user priviliges.
	 */
	if (verified && !geteuid()){
		tdu_printf(TDU_LOG_ERR, "eftd: BUG in authentification procedure.\n (claims success, but process runs still with root priviliges).\nRejecting login for security reasons.\n");
		verified=0;
	}

	if( ! verified ){
		setreuid(-1,-1); /* nobody */
		setregid(-1,-1);
		tdu_printf(TDU_LOG_WARN, "autentification of user \"%s\" failed.", user);   
		/* seems better then TDU_RE_WRONG_ID, but EFT_RE_ID_REJECTED
		 * needs to be appended by caller */
		  return TDU_RE_OTHER_REASON;	
	}
	setreuid(geteuid(),geteuid());
	setregid(getegid(),getegid());
	tdu_printf(TDU_LOG_LOG,"eftd(wu-auth): user \"%s\" logged in.\n",user);
	/*
	 * passing this additional info my means global variables is ugly but
	 * currently difficult to avoid unless we want to heavily change the
	 * wu-ftp auth code.
	 */
	if( guest ) flags |= EFT_FLAG_GUEST;
	if( anonymous ) flags |= EFT_FLAG_ANONYMOUS;
#if 0	
	/* 
	 * used as long as the new (mangling) transfer name mapping
	 * is not yet operational
	 */
	flags |= EFT_FLAG_DETERM_TN;
#endif
	eft_set_flags(eft,flags);
	/* tdu_printf(TDU_LOG_TMP,"xferlog=%d\n",xferlog); */
	eft_set_xferlog(eft,xferlog);

	return 0;
}
#else
{
	struct passwd *pw;
	int error = TDU_RE_WRONG_ID;
	
	tdu_printf(TDU_LOG_LOG,"eft_check_user(): access will always be granted!\n");

	/* 
	 * BE CAREFUL with this #if branch!:
	 *
	 * For now only one particular anonymous eft user supported.
	 * And no chroot is performed yet.
	 * Be aware that for other users, additional checks need to be
	 * performed -- i.e. like ftpd does.
	 */
	user = "ftp";

	setpwent();
	while( (pw=getpwent()) ){
		if( strcmp(user,pw->pw_name) == 0) break;
	};
	endpwent();
	
	if( ! pw ){
		tdu_printf(TDU_LOG_LOG, "User %s not in pw file\n",user);
		goto reject;
	}
	if( pw->pw_uid <=100 ) goto reject; 

	/* Don't forget to check passwd when adding support for
	 other users! */

	/* If check was successful and we are already running under the
	   proper uid, don't change anything.
	   */
	
	if( getuid() == pw->pw_uid  && geteuid() == pw->pw_uid ) return 0;

	if( chdir(pw->pw_dir) ){
		perror("eftd: chdir()");
		goto reject;
	}

	if( chroot(".") ){
		perror("eftd: chroot()");
		goto reject;
	}

	if( setregid(pw->pw_gid, pw->pw_gid) ){
		perror("eftd: setregid()");
		goto reject;
	}

	if( setreuid(pw->pw_uid, pw->pw_uid) ){
		perror("eftd: setreuid()");
		goto reject;
	}

	if(access(".", R_OK|X_OK)){
		perror("eftd: access()");
		goto reject;
	}

	eft_set_flags(eft,EFT_FLAG_GUEST|EFT_FLAG_ANONYMOUS);
	eft_set_xferlog(eft,1);

	return 0;
	
reject:
	tdu_printf(TDU_LOG_LOG,"eftd: user rejected\n");
	return error;
}
#endif

/*
 * This is to perform further setup after access regime has been established.
 *
 */
static int eft_setup_user(struct eft * eft)
{
#ifdef CONFIG_EFTD_WUAUTH
#if 0	/*
	 * FIXME: for stuff like sending messages, which can only be
	 * execuded after access regime is established, we need to
	 * implement a hook function in the tdu state machine which
	 * is called after access regime is established.
	 */
	eft_msg(eft,autherrmsg);
#endif
	/* tdu_printf(TDU_LOG_LOG,"errmsg=%s\n",autherrmsg); */
#endif
	/* if( ! eft_remote_has_navigation(eft) ) */ chdir(eft_flat_dir_name);
	return 0;
}

/* 
 *Print the possible parameters and a short description
 */
static void show_help()
{
  /*
   * Some inactive options are commented/#ifdef'ed out, mostly because they
   * correspond to wu-ftpd options that are not used by eftd yet
   * (and maybe some of them never will :-)
   */
	printf("usage: eftd [options]\n"\
	"possible options are:\n"
	       "  [-?]                 Shows this nice help and exits\n"
	/* '-a' has a mandatory argument in getopt() ?! */
	       "  [-a [ACCESS_FILE]]   (Partially wu-ftpd compatible) access config file\n"
	       "  [-b LOG_BOOK_FILE]   File for recording log[book] ( -l option) events\n" 
#if 0
	       "  [-c INTERFACE_NAME]  Name of the isdn interface\n" (see main()) */
#endif
	       "  [-d DEBUG_LOG_LEVEL] Log level for events written to stderr\n"
	       "  [-D DEBUG_MASK]      (low level) Bitmask controlling debug output\n"
#if 0
	       /* -f isn't processed in main()?! Added it to garbage at end of "switch" */
	       "  [-f PARAM]           (please fill or delete)\n"
	       "  [-"EFTD_CONFIG_FILE_PARAM_STR" CONFIG_FILE]     set location of config file\n"
#endif
	       "  [-h]                 Same as '?'\n"
#if 0
	       "  [-i]                 (please fill or delete)\n"
	       wu-ftpd: xferlog config
#endif
	       "  [-I]                 Log /dev/isdnctrl to stderr\n"
	       "  [-l LOG_LEVEL]       Log level for eft logbook (-b option) events\n"	
	       "  [-L LOG_MASK]        Log file bitmask (low level, prefer -l instead)\n"
	       "  [-m]                 Allow serving multiple eft connections simultaneously\n"
	       "  [-n ADDRESS]         Set name of server (ETS 300 075 address) to ADDRESS\n"
#if 0
	       "  [-o]                 (please fill or delete)\n"	
#endif
	       "  [-s]                 Single process, serves first connection only\n"
#if 0
	       "  [-t PARAM]           (please fill or delete)\n"	
	       "  [-T PARAM]           (please fill or delete)\n"	
	       "  [-u PARAM]           (please fill or delete)\n"
#endif
	       "  [-U DEFAULTUSER]     If user unknown, user DEFAULTUSER is used\n"
	       "  [-V]                 Shows version and exits\n"
	       "  [-x X25_ADDRESS]     X.25 address[es] (default empty) to listen on\n"
	       "\n");
}

/* 
 * looks in argv if opt exists and returns its index. Otherwise 0 is 
 * returned
 */
int haveopt(const char optchar, const int argc, char** argv)
{
	int i = 0;
	
	while (++i < argc) {
		if (strlen(argv[i]) == 2){
			if (argv[i][1] == '-'){
				return(0); /* '--' forces end of option-scanning */
			} else {
				if (argv[i][1] == optchar) return(i);
			}
		}
	};
	return(0);  
}

static void child_handler(int sig)
{
	int status;
	
	if( sig == SIGCHLD ){
		/* write(2,"child exited\n",13); */
		/* clean up possible zombie child processes */ 
		waitpid(0, &status, WNOHANG);
		/* write(2,"cleaned up\n",11); */
	} else {
		write(2,"invalid signal\n",15);
	}
	signal(SIGCHLD,child_handler);
}

/*
 * compute a bitmask for controlling debug/log messages verbosity
 */
static unsigned int level2mask(int vlevel)
{
	int mask=0;

	if(vlevel >= 1) mask |= TDU_LOG_AP1;
	if(vlevel >= 2) mask |= TDU_LOG_AP2;
	if(vlevel >= 3) mask |= TDU_LOG_AP3;
	if(vlevel >= 4) mask |= (TDU_LOG_ERR | TDU_LOG_IER | TDU_LOG_OER);
	if(vlevel >= 5) mask |= (-1 ^ TDU_LOG_TMP ^ TDU_LOG_TRC
				 ^ TDU_LOG_DBG ^ TDU_LOG_ISDNLOG);
	if(vlevel >= 6) mask = -1;

	return mask;
}


int main(int argc, char** argv)
{
	int s, ns, foo, i, si[2], ni=0, smax=-1;
	fd_set rfds;
	struct sockaddr_x25 sx25[2];
	struct linger ling = { 1     /* Linger active */,
                               500   /* wait up to 5 seconds on close */ };
	struct x25_calluserdata cud;
	struct x25_facilities facilities;
	struct eft *eft;
	int on = 1, err, dont_loop=0, multi=0;
	pid_t pid, pidm;
	time_t t;
	int c, status, llevel=0, dlevel=0;
	
	extern char *optarg;
	sigset_t sig_pipe;
	char * opt_eft_address = NULL, dev_name[EFT_DEV_NAME_LEN], *eftdev,
		* logfile_name = NULL;

        sigemptyset(&sig_pipe);
        sigaddset(&sig_pipe, SIGPIPE);


	if ((haveopt('?', argc, argv)) || (haveopt('h', argc, argv))) {
		show_help();
		exit(0);
	}
	
	if (haveopt('V', argc, argv)) { /* put out version and exit */
		printf(E4L_VERSION"\n");
		/* maybe we should better use an own option for this */ 
		tdu_printf(TDU_LOG_LOG,"\nThis is ALPHA test software (incomplete, non-protocol-"
	       "conformant, buggy, etc).\n\n ABSOLUTELEY NO WARRENTY!\n\n"
	       "Copyright 1997 by Henner Eisen (eis@baty.hanse.de)\n"
	       "The GNU Library General Public License, Version 2, applies.\n\n\n");
		exit(0);
	}
		
	openlog("eftd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
	/* This specifies the default amount of (debugging) output printed
	   to stderr */
	tdu_stderr_mask = TDU_LOG_ERR | TDU_LOG_IER |TDU_LOG_OER;
	
	tdu_log_prefix("eftd[%d] %s: ",NULL);

	/* has somebody ever thought of getopt_long? ;-) */
	while ((c = getopt(argc, argv, "a::b:d:D:f:iIl:L:mn:osS:t:T:u:U:x:")) != EOF) {
		switch (c) {
			
		case 'a':
#ifdef CONFIG_EFTD_WUAUTH
			use_accessfile = 1;
			if(optarg) eft_access = optarg;
#else
			fprintf(stderr,"eftd: wu-ftp access file not supported\n");
#endif
			break;
			
		case 'b': 
				logfile_name = optarg;
			break;

		case 'd':
			if( optarg ){
				dlevel=atoi(optarg);
			} else {
				dlevel++;
			}
			break;
			
		case 'D':
			/* user selected debugging mask, use -1 for all */
			tdu_stderr_mask = atoi(optarg);
			break;
			
		case 'I':
			tdu_stderr_mask |= TDU_LOG_ISDNLOG;
			break;

		case 'l':
			if( optarg ){
				llevel=atoi(optarg);
			} else {
				llevel++;
			}
			break;

		case 'L':
			/* user selected logbook file message mask,
			   use -1 for all */
			tdu_logfile_mask = atoi(optarg);
			break;
			
		case 'm':
			multi = 1;
			signal(SIGCHLD,child_handler);
			break;

		case 'n':
			if( optarg ){
				opt_eft_address = optarg;
			} else {
				tdu_printf(TDU_LOG_ERR, "NULL address in "
					   "command line ignored\n");
			}
			break;	
			
		case 's':
			dont_loop = 1;
			break;
			
		case 'U':
			eft_map_to_user = optarg;
			if( ! optarg )
				tdu_printf(TDU_LOG_ERR, "NULL mapped user name in command line ignored\n");
			break;	

		case 'x':
			if(ni<2){
				sx25[ni].sx25_family = AF_X25;
				strncpy(sx25[ni].sx25_addr.x25_addr, optarg, X25_ADDR_LEN);
				ni++;
			} else {
				fprintf(stderr,"too many -x options, ignored\n");
			}
			break;

		case 'f': case 'i': case 'o': case 't': case 'T': case 'u':
			fprintf(stderr, "eftd: option '%c' not yet supported\n", c);

		default:
			show_help();
			exit(1);
			}
	}

	if( ni < 1 ){
		/* the default x25 address to listen on is empty address */
		sx25[0].sx25_family = AF_X25;
		sx25[0].sx25_addr.x25_addr[0] = 0;
		ni = 1;
	}

	tdu_stderr_mask  |= level2mask(dlevel);   
	tdu_logfile_mask |= level2mask(llevel);
	/* FIXME: make default location an autoconf option*/
	if( ! logfile_name ) logfile_name = "/var/log" "/eftd.log";
	if(llevel) tdu_open_log(logfile_name);
	/* tdu_printf(TDU_LOG_DBG, "LogBook level %d, mask %d, Stderr level %d, mask %d\n",llevel,tdu_logfile_mask,dlevel,tdu_stderr_mask); */
	if( tdu_stderr_mask & TDU_LOG_ISDNLOG ){
		tdu_open_isdnlog("/dev/isdnctrl");
	}
	tdu_isdnlog();

	for(i=0;i<ni;i++){
		s = socket(AF_X25, SOCK_SEQPACKET, 0);
		if (s < 0) {
			perror("eftd: socket creation failed");
			fprintf(stderr,"\t(Maybe your kernel was not compiled"
				" with X.25 PLP support enabled\n"
				"\tor it was compiled as a module but the x25 "
				"module was not loaded)\n"); 
			exit(1);
		}
		/* first byte of every packet to be interpreted as
		   the X.25 Q-bit value */
		setsockopt(s, SOL_X25, X25_QBITINCL, &on, sizeof(on));
		
		/* 
		 * A dedicated address to listen on might have been supplied by
		 * a command line argument.
		 */
		if (bind(s, (struct sockaddr *)(&sx25[i]), sizeof(sx25[0])) < 0) {
			perror("eftd: bind failed");
			exit(1);
		} else {
			tdu_printf(TDU_LOG_DBG,"socket %d bound to %s\n",s,sx25[i].sx25_addr.x25_addr);
		}

		if( ioctl( s, SIOCX25GFACILITIES, &facilities ) != 0 ){
			perror("eftd: SIOCX25GFACILITIES failed");
			return 1;
		}
		/* winsize 7 is o.k., but I observed problems when connected
		   to AVM clients. Those problems disappear with winsize 6*/
#if 0
		facilities.winsize_in  = 7;
		facilities.winsize_out = 7;
#else
		facilities.winsize_in  = 2;
		facilities.winsize_out = 2; 
#endif
		/* 
		 * eft requires a packet size of (at least) 1024 bytes
		 */
		facilities.pacsize_in  = X25_PS1024;
		facilities.pacsize_out = X25_PS1024;
		
		if( ioctl( s, SIOCX25SFACILITIES, &facilities ) != 0 ){
			perror("eftp: SIOCX25SFACILITIES failed");
			return 1;
		}

		if ( listen(s, 1+multi) < 0 ) {
			perror("eftd: listen failed");
			exit(1);
		}
		smax = (s>smax) ? s : smax;
		si[i] = s;
	};
	
	while (1){
		foo = sizeof sx25;
		tdu_isdnlog();
		t = time(NULL);
		tdu_printf(TDU_LOG_LOG,"**************************************\n"
			   "eftd[%d]: start waiting for incoming X.25 connection at"
			   " %s\n", getpid(), ctime(&t) );

		FD_ZERO(&rfds);
		for(i=0;i<ni;i++){
			FD_SET(si[i], &rfds);
		};
		s = select(smax+1, &rfds, NULL, NULL, NULL);
		if( s < 0 ) {
			if( errno == EINTR ){
				/* this usually occurs when children exit */
				tdu_isdnlog();
				continue;
			} else {
				perror("eftd: select() failed");
				tdu_isdnlog();
				exit(1);
			}
		}
		s = -1;
		for(i=0;i<ni;i++){
			if( FD_ISSET(si[i], &rfds) ) s = si[i];
		};
		
		ns = accept(s, (struct sockaddr *)&sx25, &foo);
		if (ns < 0) {
			if( errno == EINTR ){
				/* this usually occurs when children exit */
				tdu_isdnlog();
				continue;
			}
			perror("eftd: accept() failed");
			tdu_isdnlog();
			exit(1);
		}
		tdu_isdnlog();

		/* check user data field */
		if( ioctl( ns, SIOCX25GCALLUSERDATA, &cud ) != 0 ){
			perror("eftp: SIOCX25GCALLUSERDATA failed");
			return 1;
		}
		if( (cud.cudlength == 13) && 
		    (strcmp( cud.cuddata+4, "EUROSFT92") == 0) ){
			/* this is the only valid id */
			;
			/* Some clients use misformatted strings, which
			 * causes this check to fail
			 */
		} else if( (cud.cudlength == 9) && 
			   (strcmp( cud.cuddata, "EUROSFT92") == 0) ){
			/* teles does this wrong */
			tdu_printf(TDU_LOG_LOG,"eftd: misaligned EUROSFT92 "
				   "cud, nevertheless accepting\n"); 
		} else if( cud.cudlength == 0 ) {
			tdu_printf(TDU_LOG_LOG,"eftd: connection without cud "
				   "present, trying EUROFile\n"); 
		} else {
			tdu_printf(TDU_LOG_LOG,"eftd: non-EUROFile connection "
				   "might be present, closing\n"); 
			/* FIXME: add a cause/diagnostic value before closing*/
			
			close(ns);
			sleep(5);
			eft_dl_disconnect(eftdev);
			continue;
		}

		session_start=time(NULL);
		eftdev = eft_get_device(dev_name,EFT_DEV_NAME_LEN,ns);
		if( ! eftdev ){
			/* FIXME: hard coded dev name no longer necessary */
			fprintf(stderr,	"eftd: device not found, set to "
				"default \"isdneftd\"\n");
			eftdev = "isdneftd";
		}
		
		if( dont_loop ){
			pid = 0;
		} else {
			pid = fork();
		}

		if( pid < 0 ){
			perror("eftd: fork failed, sleeping");
			close(ns);
			eft_dl_disconnect(eftdev); /* will probably fail, too*/
			sleep(50);
			continue;
		} else if ( pid > 0 ){
			/* we are the parent process */
			close(ns);
			/* in multi mode, a dedicated supervisor process
			 * will be forked somewhere else and wait for the
			 * child. Thus, we can continue to accept new
			 * connections at once.
			 */
			if( multi ) continue;
			
			eft_log_accept(eftdev,pid);
			if( wait(&status) != pid ){
				perror("eftd: wait failed");
				eft_dl_disconnect(eftdev);
				exit(1);
			}
			if(WIFSIGNALED(status)){
				tdu_printf(TDU_LOG_ERR, "internal error in eftd[%d]: %s\n\tyou might try to debug eftd using gdb\n",pid ,strsignal(WTERMSIG(status)));
			}
			/* after child has closed locical (X.25/ISDN-B3)
			 * connection, disconnect lower layer (including isdn
			 * physical) connections. This needs to be done by the
			 * privileged parent process due to permission and
			 * chroot() reasons.
			 */ 
			eft_log_disconnect(pid);
			eft_dl_disconnect(eftdev);
			continue;
		} /* else 
		   *         we are the forked child process in charge of
		   *         processing the accepted connection
		   */
		close(s);
		  /* 
		   * In multi mode, we need to fork an extra privileged
		   * supervisor process for each accepted connection
		   * that can clean up lower layers after the session
		   * is finished.
		   *
		   * This is a waste of resources, a better way
		   * would be to register a signal handler for the
		   * main process which takes care of cleanig up
		   * the connections after each child processes has 
		   * finished.
		   */
		if( multi ){
			pidm = fork();
			if( pidm < 0 ){
				/* temporary lack of resources */
				perror("eftd: forking of supervisor failed");
				close(ns);
				/* will probably fail, too: */
				eft_dl_disconnect(eftdev);
				exit(1);
			} else if( pidm > 0 ){
				/* we are the supervising parent process */
				close(ns);
				eft_log_accept(eftdev,pidm);
				tdu_printf(TDU_LOG_LOG, "eftd supervisor %d waiting for %d to finish\n", getpid(), pidm);
				if( wait(&status) != pidm ){
					perror("eftd: supervisor's wait failed");
					eft_dl_disconnect(eftdev);
					exit(1);
				}
				if(WIFSIGNALED(status)){
				tdu_printf(TDU_LOG_ERR, "internal error in eftd[%d]: %s\n\tyou might try to debug eftd using gdb\n",pidm,strsignal(WTERMSIG(status)));
				}

				eft_log_disconnect(pidm);
				eft_dl_disconnect(eftdev);
				tdu_printf(TDU_LOG_LOG, "supervisor exiting lower layer\n");
				exit(0);
			}
		}
		/* 
		 * We only arrive here if we are are the forked child
		 * process in charge of serving the eurofile session
		 */
		tdu_log_prefix("eftd[%d] %s: ",NULL);

		pid = getpid();
		tdu_printf(TDU_LOG_LOG, "eftd (pid=%d): X.25 DTE-DTE "
			   "connection accepted from device %s\n",
			   pid,eftdev);
		/*
		 * Now, the x.25 DTE-DTE connection is up. On top of that,
		 * the higher layer eft connection needs to be established.
		 * There are several higer layers, but this is taken care of
		 * by the eft_accept_user() function.
		 */
		
		eft = eft_make_instance();
		if(opt_eft_address) eft_set_address(eft,opt_eft_address);
		/* 
		 * Attach the connected x.25 socket to the eft protocol state
		 * machine 
		 */
		eft_attach_socket(eft,ns);
		/* block SIGPIPE such that peer initiated disconnects will
		 * result in write error indications
		 */
 		if( sigprocmask(SIG_BLOCK, &sig_pipe, NULL) )
			perror("sigprocmask()");
		#if 1
		setsockopt(ns,SOL_SOCKET,SO_LINGER,&ling,sizeof(ling));
		#endif
		/*
		 * attach authentication methods to the protocol state machine
		 */
		eft_set_auth(eft, eft_check_user, eft_setup_user, NULL);

		err = eft_accept_user(eft);
		if( err ) { 
			tdu_printf(TDU_LOG_LOG,"eftd: error, user not accepted\n");
		} else {
			tdu_printf(TDU_LOG_LOG,"eftd: user logged in.\n");
		}
		eft_server_mainloop(eft);
		tdu_printf(TDU_LOG_AP2, "SES: END    duration=%.f\n",
			   difftime(time(NULL),session_start));
#if 0		
		/* Core dump test. Dumping will not work after setre[ug]id */
		*((int *) 0) = 0;
#endif
		tdu_printf(TDU_LOG_LOG, "eftd: waiting for connection to "
			   "terminate\n");
		if(close(ns)) perror("close()");
		tdu_printf(TDU_LOG_LOG, "eftd child (pid %d) terminating\n", pid);
		return 0;
	}
}
