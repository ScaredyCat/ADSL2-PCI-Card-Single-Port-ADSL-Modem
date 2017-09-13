/* $Id: eftp.c,v 1.6 1999/10/08 00:31:06 he Exp $ */
/*
  Copyright 1997 by Henner Eisen

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
   CAUTION: this description is somewhat outdated.


   This is an experimental and incomplete program. The purpose of
   this program is NOT to provide a perfect eft (EUROFILE transfer) client
   for isdn4linux. It's more intended to be a demonstration 

   - to show, how X.25 sockets can be used to connect to a remote computer
     using the ISO-8208 (X.25 DTE-DTE) protocol on top of isdn4linux.
   - to test data transfer on top of x.25 on top of isdn4linux
     if your isdn peer can run an eft server.
   - to give some insights to the EUROFILE protocol (there is no public
     documentation available anywhere else).

   The whole eft protocol is implemented inside a library, usable via the
   interface defined in eft.h. My intention is to continue developing 
   and maintaining the core eft protocol implementation. Based on the library,
   uers are welcome to write other eft clients (and servers).
   (Think, i.e., about a kde version, http-to-eft gateway, or just a more
   sophisticated command line interface than this one).

   This program does not close low layer isdn connections by itsself. PLEASE
   monitor the isdn connection when you are using this program (this is
   because the current linux x.25 was initially nor intended for DTE-DTE
   mode on top of isdn, thus, it doesn't close the link after the x.25
   connections are cleared). If the connection isn't cleared by your peer
   after the end of your eft session, use the "isdnctrl hangup" command to
   do this manually. Or use a script wrapper (such as eftp.sh).

   And security was not a primary issue when writing this program (i.e.
   buffer overflow checks might be missing where necessary). Thus, a bad guy
   might exploit all kinds of bugs in order to break into your system.

   As this program is not tested well, please ask the operator of
   your remote eft server before trying to connect to it. 
   Until now, this client hasn't caused any harm, but as testing was very
   limited, I cannot guaranty this.


   Now, if you want to test it, do the following:


   Set up an outgoing isdn network interface, configured for encap x25iface,
   l2_prot x75i and your eft serving peer's remote phone number. 

   Create an appropriate x25 route to this interface.

   (You can use the "ix25test" script to do all of that).

   Assuming your user name on the eft server is "myname" and your password is
   "mypass" and the X.25 address that is routed to your isdn interface is "05".
   Type

   eftp -x 05 myname/mypass

   from the shell's prompt.

   This will try to connect to the remote eft server. A lot of debugging
   output (protocol trace) will appear on your terminal while this is going on.
   
   If the connection was successful you can type

       dir *

   from the "eftp>" prompt. You should get a listing of the server's directory.
   Assuming the directory contains a file "TEST.GIF" type

       get TEST.GIF
 
   to download that file to your computer. The transfer header of the file
   received is not evaluated, just skipped. The contents of the transferred
   file is written to your disk as received (i.e. neither owner nor
   permissions of the file are retreived from the transfer header, no code
   conversion for text files is performed, ...).

   Do submit a file to the remote computer type

       put FILE

   Furthermore, there are two other commands, namely "quit" and "help".

   Good luck.

   */

/* for strsignal() */
#define _GNU_SOURCE       

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <linux/x25.h>
#include <string.h>
#include <stdlib.h>
#include <glob.h>
#include <fnmatch.h>
#include <ctype.h>

#include <tdu_user.h>
#include <eft.h>

#define MAX_COMMAND_LINE_LEN 4096

/* is in net/x25.h, not in the public header file linux/x25.h. Why?*/ 
#ifndef X25_MAX_CUD_LEN
#define X25_MAX_CUD_LEN 128
#endif

#undef PIPE_DEBUG
#ifdef CONFIG_EFTP_READLINE

#include <readline/readline.h>
#include <readline/history.h>

#endif

/*
  If multi_prompt is 0, the mget and mput commands prompt for each matching
  filename. Otherwise, they just transmit all matching files.
*/
static int multi_prompt = 0;

/*
   Prompts for "y" or "n" if multi_prompt != 0, 
   returns 1 for "y" or 0 for "n".
   If stdin is not a TTY, it returns 1 always.
*/
static int multi_prompt_yes(char* cmdname, char* filename)
{
	int c,r;
	if( !multi_prompt || !isatty(STDIN_FILENO))  return 1;
	while(1) {
		fprintf(stderr,"%s %s (y/n)? ",cmdname,filename);
		fflush(stderr);
		r = c = tolower(getchar());
		while(c!='\n' && c!=EOF)
			c = getchar();
//		fprintf(stderr,"\n");
		if('y' == r) return 1;
		if('n' == r) return 0;
	}
	/* not reached */
}

/*
 * A simple parser for the eftp command line
 */
/*
 * FIXME: return values of local commands (!, lcd, lpwd, ldir)
 */
int process_cmd_line (struct eft *eft, unsigned char *buf)
{
	static int mget_case_ignore = 0;
	char **pp = (char **) & buf, *c, *arg1, *arg2, *cmd;

	int ret = 1, count, r;
	
        if( *buf == '!' ){
		r = system(buf+1);
		if( (r<0) || (r==127) ){
			perror("system()");
			return 0;
		}
		return 0;
	}
        
	c = strsep(pp," \t\n");
	arg1 = strsep(pp," \t\n");
	arg2 = strsep(pp," \t\n");
	if( arg2 == 0 || *arg2 == 0) arg2 = arg1;
	
	if( c == NULL ) return -1; 
	if( strcmp(c, "dir") == 0 ){
		if( (arg1 == NULL) || (*arg1 == 0) ) arg1 = "*";
		ret = eft_dir_fd ( eft, 1, arg1, 0 );
	} else if( strcmp(c, "xdir") == 0 ){
		if( (arg1 == NULL) || (*arg1 == 0) ) arg1 = "*";
		ret = eft_xdir_txt ( eft, arg1 );
	} else if( strcmp(c, "msg") == 0 ){
		if( (arg1 == NULL) || (*arg1 == 0) ) {
			char obuf[255];
			/* FIXME: this might block as it does not use the
			 * eft select loop */
			eft_prompt("EFT-Message: ");
			count = read( STDIN_FILENO, obuf, 254);
			if ( count < 0 ) {
				perror("while reading msg line");
			} else {
				obuf[count] = 0;
				ret = eft_msg ( eft, obuf );
			}
		} else {
			ret = eft_msg ( eft, arg1 );
		}
	} else if( strcmp(c, "get") == 0 ) {
		if( (arg1 == NULL) || (*arg1 == 0) ){
			fprintf(stderr,"eftp get: file name is missing\n");
			return 0;
		}
		ret = eft_load( eft, arg2, arg1 );
	} else if( strcmp(c, "put") == 0 ) {
		if( (arg1 == NULL) || (*arg1 == 0) ){
			fprintf(stderr,"eftp put: file name is missing\n");
			return 0;
		}
		ret = eft_save( eft, arg2, arg1 );

		/*	Is this necessary as the same functionality is
		 *	provided by !ls ? Maybe, if we could provide
		 *      the local output in the same format as dir/xdir
		 */ 
        } else if( (strcmp(c, "ldir") == 0) || 
                   (strcmp(c, "lls") == 0) ) {
		ret = 0; /* Local problems should not cause a disconnect */
		if(! arg1) arg1="";
		cmd = (char*)malloc(strlen(c)+strlen(arg1)+2);
                if( cmd ){ 
			sprintf(cmd,"%s %s",c+1,arg1);
			r = system(cmd);
			if( (r<0) || (r==127) ){
				perror("system()");
			}
			free(cmd);
		}
        } else if( strcmp(c, "lcd") == 0 ) {
                char* cwd = NULL;    
		ret = 0;
                if(arg1 && *arg1) {
			if( strcmp(arg1,"~") == 0 ){
				r = chdir(getenv("HOME"));
			} else {
				r = chdir(arg1);
			}
			if(r<0){
				perror("chdir()");
			}
                }
		cwd = getcwd(NULL,0);
                printf("Local directory now %s\n",cwd);
	} else if( strcmp(c, "mput") == 0 ) {
		glob_t glob_result;
		int r;
		size_t i;
		char* p = arg1;		
		if( (arg1 == NULL) || (*arg1 == 0) ) {
			fprintf(stderr,"eftp mput: file name pattern is missing\n");
			return 0;
		}
		while(p) {
			r = glob(p,0,NULL,&glob_result);
			for(i=0; i<glob_result.gl_pathc; i++) {
				if( multi_prompt_yes("mput", glob_result.gl_pathv[i]) ) {
					ret = eft_save( eft, glob_result.gl_pathv[i], 
							     glob_result.gl_pathv[i] );
					if(ret)	return ret;  /* or should we ignore it? */
				}
	                }
			if(arg1 == arg2)	/* only one argument */
				p = NULL;
			else if(p == arg1)	/* at least two arguments */
				p = arg2;
			else 	p = strsep(pp," \t\n");	/* more arguments */
		}
                return 0;
	} else if( strcmp(c, "mget") == 0 ) {
		char* p = arg1;		
		if( (arg1 == NULL) || (*arg1 == 0) ) {
			fprintf(stderr,"eftp mget: file name pattern is missing\n");
			return 0;
		}
		while(p) {
			FILE* f = tmpfile();
			int tmp_fd = fileno(f);
			char s[1024];
			ret = eft_dir_fd ( eft, tmp_fd, p, 0 );
			fflush(f);
			fseek(f,0,SEEK_SET);
			while(fgets(s,sizeof(s),f))
			{
				char* fname = strtok(s," \t\n");

/* 
 * glibc2 (at least in default environment) seems not to know
 * about FNM_CASEFOLD
 * FIXME: should this be removed or repleced by something else?
 *
 * For now, avoid glibc2 problems by
 */
#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0
#endif			       
				
				if( 0 == fnmatch(p, fname,
						 mget_case_ignore? FNM_CASEFOLD:0) ){
					if( multi_prompt_yes("mget",fname) ) {
						ret = eft_load( eft, fname, fname );
						if(ret)	return ret;  /* or should we ignore it? */
					}
				}
			}
			fclose(f);
			if(arg1 == arg2)	/* only one argument */
				p = NULL;
			else if(p == arg1)	/* at least two arguments */
				p = arg2;
			else 	p = strsep(pp," \t\n");	/* more arguments */
		}
                return 0;
	} else if( strcmp(c, "prompt") == 0 ) {
		if( (arg1 == NULL) || (*arg1 == 0) )
			multi_prompt = !multi_prompt;
		else if( strcmp(arg1,"on") == 0)
			multi_prompt = 1;
		else if( strcmp(arg1,"off") == 0)
			multi_prompt = 0;
		else {
			fprintf(stderr,"eftp prompt: need \"on\" or \"off\" or no argument\n");
			return 0;
		}
		fprintf(stderr,"Interactive mode %s.\n", multi_prompt? "on":"off");
		ret = 0;
	} else if( strcmp(c, "case") == 0 ) {
		if( (arg1 == NULL) || (*arg1 == 0) )
			mget_case_ignore = !mget_case_ignore;
		else if( strcmp(arg1,"on") == 0)
			mget_case_ignore = 1;
		else if( strcmp(arg1,"off") == 0)
			mget_case_ignore = 0;
		else {
			fprintf(stderr,"eftp case: need \"on\" or \"off\" or no argument\n");
			return 0;
		}
		fprintf(stderr,"Case mapping %s.\n", mget_case_ignore? "on":"off");
		ret = 0;
	} else if( strcmp(c, "quit") == 0  ||
		   strcmp(c, "close") == 0 ||
		   strcmp(c, "stop") == 0  ||
		   strcmp(c, "end") == 0   ||
		   strcmp(c, "exit") == 0  ||
		   strcmp(c, "bye") == 0  ) {
		return -1;
	} else if( strcmp(c, "help") == 0 || strcmp(c,"?") == 0 ) {
		fprintf(stderr, "known commands:\n"
			"get FILE [LOCAL_NAME]	get file from remote\n"
			"dir PATTERN		get directory from remote\n"
			"xdir PATTERN		get directory with extended information from remote\n"
			"put FILE [REMOTE_NAME]	put file to remote\n"
			"list			list all directories\n"
			"slist			list all subdirectories\n"
			"cd [DIR]		change working directory\n"
			"mkdir DIR		make remote directory\n"
			"pwd			print working directory\n"
			"msg MESSAGE		submit a printable message\n"
			"lcd [DIR]		change local directory\n"
			"lls			list local directory\n"
			"! COMMAND		execute shell command\n"
			"mput PATTERN		put multiple files to remote\n"
			"mget PATTERN		get multiple files from remote\n"
			"prompt [on|off]		set or toggle interactive prompting for mget and mput\n"
			"case [on|off]		set or toggle mget case sensitivity\n"
			"quit			close connections and quit\n"
			"(mkdir and list are not supported by most servers, many even don't\n"
			" support xdir, slist, cd, pwd, nor msg)\n"  
			);
	} else if( strcmp(c, "list") == 0 ) {
		ret = eft_list_fd( eft, 1, 0 );
	} else if( strcmp(c, "slist") == 0 ) {
		ret = eft_slist_fd( eft, 1, 0 );
	} else if( strcmp(c, "cd") == 0 ) {
		if( (arg1 == NULL) || (*arg1 == 0) ) arg1 = NULL;
		ret = eft_cd( eft, arg1 );
	} else if( strcmp(c, "pwd") == 0 ) {
		char dir[EFT_MAX_FSTORE_LEN+1];
		ret = eft_getcwd( eft, dir );
		if(ret>=0) printf("Current filestore is %s\n",dir);
	} else if( strcmp(c, "mkdir") == 0 ) {
		if( (arg1 == NULL) || (*arg1 == 0) ){
			fprintf(stderr,"eftp mkdir: dir name is missing\n");
			return 0;
		}
		ret = eft_mkdir( eft, arg1 );
	} else {
		fprintf( stderr, "unknown command, type \"?\" or \"help\"\n");
	}
	return ret;
}


static struct eft *eft;
static int disconnect_please = 0;

#ifdef CONFIG_EFTP_READLINE

static void readline_callback_handler(void)
{
	static char* line_read = NULL;
	int r;
	
	/* If the buffer has already been allocated, return the memory
	     to the free pool. */
	if( line_read ) {
		free(line_read);
		line_read = (char *)NULL;
	}
	if( *rl_line_buffer ) {
		line_read = strdup(rl_line_buffer);
		add_history(line_read);
/*
 * XXX: caution: if the cmd is "msg", process_cmd_line might prompt and
 *     read() from stdin now from this callback, which has caused problems.
 *     Probably, readline() is not designed for this.
 * This needs further investigation (and a fix :-). 
 *
 * (Not a showstopper, as the msg cmd doesn't serve anything. It's just there
 * for completeness and the problem is not triggered when the message is
 * provided by means of the msg cmd parameter)
 */
		if( (r=process_cmd_line(eft, line_read)) < 0 ){
			if( r < -1) fprintf(stderr,"cmd execution error %d<0\n",r);
			disconnect_please = 1;
		}
	}
}
#endif

#define MAX_CALLBACK_LINE_SIZE 1026
char pipe_callback_line_buf[MAX_CALLBACK_LINE_SIZE];
int interactive = 0, pipe_buf_pos = 0;

static void pipe_callback_read(void)
{
	int r, l, size, offset, line_complete=0;
	char *start, *eor=NULL;

	l = read(STDIN_FILENO, pipe_callback_line_buf+pipe_buf_pos,
		 MAX_CALLBACK_LINE_SIZE-pipe_buf_pos-1);
	pipe_callback_line_buf[MAX_CALLBACK_LINE_SIZE-1]=0;
	pipe_callback_line_buf[pipe_buf_pos+l]=0;
#ifdef PIPE_DEBUG
	fprintf(stderr,"%d chars read\n",l);
	fprintf(stderr,"read buffer is: '%s'\n",pipe_callback_line_buf+pipe_buf_pos);
#endif
	if( l<0 ) goto disconnect;
	if( (l==0) ){
#ifdef PIPE_DEBUG
		fprintf(stderr,"EOF condition detected\n");
#endif
		if (pipe_buf_pos==0) goto disconnect;
		line_complete=1;
#ifdef PIPE_DEBUG
		fprintf(stderr,"command line terminated by EOF\n");
#endif
	}
	pipe_buf_pos += l;
	eor = pipe_callback_line_buf;
	if( l>0 ){
		start = strsep(&eor,"\n\r");
		if( !eor ){
#ifdef PIPE_DEBUG
			fprintf(stderr,"no line end found, ");
			fprintf(stderr,"current buffer is: '%s'\n",pipe_callback_line_buf);
#endif
			if( pipe_buf_pos >= (MAX_CALLBACK_LINE_SIZE-1) ){
#ifdef PIPE_DEBUG
				fprintf(stderr,"eftp command line too long\n");
#endif
				pipe_buf_pos=0;
			}
			return;
		} else {
			line_complete = 1;
#ifdef PIPE_DEBUG
			fprintf(stderr,"line end detected\n");
			if(*eor) fprintf(stderr,"there are still more commands in the buffer:'%s'\n",eor);
#endif
		}
	}
	if(line_complete){
#ifdef PIPE_DEBUG
		fprintf(stderr,"executing command line: '%s'\n",
			pipe_callback_line_buf);
#endif
		if( (r=process_cmd_line(eft, pipe_callback_line_buf)) < 0 ){
			if( r < -1) 
#ifdef PIPE_DEBUG
				fprintf(stderr,"cmd execution error %d<0\n",r);
#endif
			goto disconnect;
		}
	}
	offset = ( eor - pipe_callback_line_buf);
	pipe_buf_pos -= offset;
	if( l==0 ) goto disconnect;
	size = MAX_CALLBACK_LINE_SIZE - offset;
	memmove(pipe_callback_line_buf, eor, size);
#ifdef PIPE_DEBUG
	fprintf(stderr,"shifted: buffer is now: '%s'\n",pipe_callback_line_buf);
#endif
	
	if(! pipe_buf_pos && interactive) eft_prompt("eftp> ");

	return;
disconnect:
	disconnect_please = 1;
	return;
}

static void pr_usage(FILE * f, char *cmd)
{
	fprintf(f,"usage: %s eftp [ -i ISDN_NO | -x X25_ADDRESS  ]"
		       " [ -u USER[/PASSWORD] ] [-p] [-h] [-r]\n", cmd);
}

static void show_help(char *cmd)
{
	pr_usage(stderr, cmd);
	printf("\t-i\t isdn no of EUROFILE server\n"
	       "\t-x\t X.25 address of EUROFILE server\n"
	       "\t-u\t user name used to login on server. A password may\n"
	       "\t\t be appended, separated by a '/' character\n"
	       "\t-p\t inhibit prompting for a password\n"
	       "\t-r\t inhibit use of readline command line editing\n"
	       "\t-h\t show help message\n"
		);
}

int main(int argc, char **argv)
{
/*
 * CAUTION: The initial code part might execute suid root.
 * See further remarks below if you change eftp.c
 */
	struct sockaddr_x25 x25bind, x25connect;
	struct x25_route_struct x25_route;
	int s, on=1, selval, prompt_for_pw = 1, use_readline=1;
	unsigned char called[TDU_PLEN_ADDR+1], udata[TDU_PLEN_UDATA+1];
	uid_t ruid, euid;

	fd_set rfds;

	unsigned char * ident;
	char c, *isdn_no=NULL, *x25_no=NULL, *user=NULL,
		*arg0 = *argv&&**argv ? *argv : "eftp",
		*buf;	/* Has to be dynamic memory for getdelim() */
	
	extern char * optarg;
	size_t buflen = MAX_COMMAND_LINE_LEN;
	struct x25_calluserdata cud;
	struct x25_facilities facilities;
	pid_t pid;
	struct linger ling = { 1     /* Linger active */,
                               500   /* wait up to 5 seconds on close */ };
	sigset_t sig_pipe;

#ifdef CONFIG_EFTP_READLINE
	rl_readline_name = "eftp";
	rl_inhibit_completion = 1;
#else
	use_readline = 0;
#endif

        sigemptyset(&sig_pipe);
        sigaddset(&sig_pipe, SIGPIPE);

	buf = (char*)malloc(buflen);

	printf("\nThis is ALPHA test software (incomplete, non-protocol-"
	       "conformant, buggy, etc).\n\n ABSOLUTELEY NO WARRENTY!\n\n"
	       "Copyright 1997 by Henner Eisen (eis@baty.hanse.de)\n"
	       "The GNU (Library) General Public License, Version 2, applies.\n\n\n");

	if (argc < 1) {
		pr_usage(stderr,arg0);
		exit(1);
	}

	while ((c = getopt(argc, argv, "i:x:u:prh")) != EOF) {
		switch (c) {
		case 'u':
			user = optarg;
			break;
		case 'i':
			isdn_no = optarg;
			break;
		case 'x':
			x25_no = optarg;
			break;
		case 'p':
			prompt_for_pw=0;
			break;
		case 'r':
			use_readline = 0;
			break;
		case 'h':
			show_help(arg0);
			exit(0);
		}
	}

	ruid = getuid();
	euid = geteuid();
	if( isdn_no ){
		/* 
		 * If the destinatioin is given by means of an isdn number,
		 * we will try to dynamically create an isdn X.25 network
		 * interface and an x.25 route through it. In order to
		 * reliably hang up the connection later -- even if this
		 * programm crashes -- we will fork a child process in charge
		 * of the real work and ourselves will only wait for that
		 * child to exit such that we can clean up the low layer
		 * connection afterwards.
		 *
		 * In order to dynamically create isdn network interfaces
		 * and to set them up, we need certain priviliges (write
		 * access to /dev/isdnctrl and netadmin capability). However,
		 * we don't want to grant those priviliges to the executing
		 * eftp program. Forking also allows us to run the child
		 * process with fewer priviliges than ourselves.
		 */  
		pid_t pid;

		/*
		 * First we check if the user is allowed to open outgoing
		 * isdn connections. We assume that the user is allowed to
		 * do so if he has write permissions to /dev/ttyI0.
		 * (In that case, the user could also open outgoing
		 * connections by writing AT commands to /dev/ttyI0).
		 */
#define EFTP_PERMISSION_CHECK_FILE "/dev/ttyI0"
		if( access(EFTP_PERMISSION_CHECK_FILE,W_OK) ){
			perror("eftp: User is not allowed to open outgoing "
			       "isdn connections: \n\t" 
			       EFTP_PERMISSION_CHECK_FILE );
			exit(1);
		}

		fprintf(stderr, "Setting up isdn x25 network interface\n");
		if( eft_get_x25route(&x25connect,&x25_route,isdn_no) ){
			fprintf(stderr,"eftp: unable to get an X.25 route for isdn number %s\n",isdn_no);
			exit(1);
		}
		pid = fork();
		if( pid<0 ){
			perror("eftp: fork()");
			exit(1);
		} else if( pid > 0 ) {
			/* 
			 * parent process
			 *
			 * We first wait until the child no longer needs the
			 * x25 route and clear the route (needs netadmin
			 * capability)
			 */
			int status, err=0;
			
			setreuid(euid,euid);
			close(s);
			eft_wait_release_route();
			eft_release_route(&x25_route);
			/*
			 * Finally, wait for the child to exit and clear
			 * the low layer isdn connection and remove
			 * dynamically created interfaces (needs netadmin
			 * capability and write access to /dev/isdnctrl)
			 */
			if( wait(&status) != pid ){
				perror("eftp supervisor: wait failed");
				err = 1;
			} else {
				if(WIFSIGNALED(status)){
					tdu_printf(TDU_LOG_ERR,
						   "internal error in eftp[%d]: %s\n\tyou might try to debug eftp using gdb\n",
						   pid, strsignal(WTERMSIG(status)));
					err = 2;
				}
			}
			eft_dl_disconnect( x25_route.device );
			eft_release_device( x25_route.device );
			exit(err);
		} else {
			/* Child process
			 *
			 * Just contiunue processing the protocol.
			 */
			;
		}
	} else if( x25_no ){  
		strncpy(x25connect.sx25_addr.x25_addr, x25_no, 15);
	} else {
		fprintf(stderr, "Neither isdn nor X.25 address specified.\n"
			" Assuming route to empty X.25 address\n");
		strcpy(x25connect.sx25_addr.x25_addr, "");
	}
	/* 
	 * Never remove this, otherwise a suid eftp might continue to
	 * run with full root priviliges!
	 */
	setreuid(ruid,ruid);
	/* 
	 * Further, any code before the above statement must be carefully
	 * written to resist any kind of suid attacks.
	 *
	 * Be aware of this if you change eftp.c! In particular, make shure
	 * that new/modified code does not allow for buffer overflow attacks
	 * by means of command line arguments or environment variables.
	 * The same holds for code of any function called from above.
	 */

	/* build ident string [uid/password] from various input sources */
	ident = NULL;
	if( user ) {
		int ulen = strlen(user);
		char * pos = strchr(user,'/');
		if(pos){
			prompt_for_pw = 0;
		}
		if( prompt_for_pw ){
			char pwbuf[20];
			struct termios saved, noecho;
			fprintf(stderr,"password: ");
			tcgetattr (STDIN_FILENO, &saved);
			noecho = saved;
			noecho.c_lflag &= ~ECHO;
			tcsetattr (STDIN_FILENO, TCSANOW, &noecho);
			if( fgets(pwbuf,20,stdin) ){
				ident=malloc(strlen(pwbuf)+ulen+2);
				if(ident){
					strcpy(ident,user);
					strcat(ident,"/");
					strcat(ident,pwbuf);
					/* remove traling new line left by fgets*/
					ident[strlen(ident)-1]=0;
				}
			}
			tcsetattr (STDIN_FILENO, TCSANOW, &saved);
		} else {
			ident = user;
		}
	}

	/*
	 * First section of this program establishes an X.25 DTE-DTE
	 * connection to the remote eft server using the socket interface.
	 * Several set ups have to be performed before.
	 */

	x25bind.sx25_family = AF_X25;
	strcpy(x25bind.sx25_addr.x25_addr, "" );

	x25connect.sx25_family = AF_X25;

	s = socket(AF_X25, SOCK_SEQPACKET, 0);
	if (s < 0) {
		perror("eftp: socket creation failed");
		fprintf(stderr,"\t(Maybe your kernel was not compiled with "
			"X.25 PLP support enabled\n"
			"\tor it was compiled as a module but the x25 "
			"module is not loaded)\n"); 
		return 1;
	}
	
	/* In order to recognize the lower layer SBV messages we need
	   to be aware of the Q-bit included with each X.25 packet,
	 */
	if (setsockopt(s, SOL_X25, X25_QBITINCL, &on, sizeof(on)) < 0 ) {
		perror("eftp: setsockopt() failed");
		return 1;
	}

	/* eft requires a packet size of (at least) 1024 bytes
	 */
	if( ioctl( s, SIOCX25GFACILITIES, &facilities ) != 0 ){
		perror("eftp: SIOCX25GFACILITIES failed");
		return 1;
	}
	printf("current facilies wi %d wo %d pi %d po %d\n",
	       facilities.winsize_in,
	       facilities.winsize_out,
	       facilities.pacsize_in,
	       facilities.pacsize_out);

	facilities.winsize_in  = 7;
	/* avm server hangs with outgoing winsize=7 */
	facilities.winsize_out = 6;
	facilities.pacsize_in  = X25_PS1024;
	facilities.pacsize_out = X25_PS1024;

	printf("requested facilies wi %d wo %d pi %d po %d\n",
	       facilities.winsize_in,
	       facilities.winsize_out,
	       facilities.pacsize_in,
	       facilities.pacsize_out);

	/* Unfortunatly, this doesn't work with kernels  up to at least 2.1.90
	   unless the socket is in the listen state (x.25 kernel bug)
	   unless af_x25.c is patched.  */
	printf("Trying to set X.25 facilities on socket ...\n");
	if( ioctl( s, SIOCX25SFACILITIES, &facilities ) != 0 ){
		perror("eftp: SIOCX25SFACILITIES failed");
		return 1;
	}
	/*
	 * request of eft service is indicated to the server by a special
	 * direct call user data string in our x25 call request packet
	 */
	cud.cuddata[0] = 0;
	cud.cuddata[1] = 0;
	cud.cuddata[2] = 0;
	cud.cuddata[3] = 0;
	strncpy( cud.cuddata+4, "EUROSFT92", X25_MAX_CUD_LEN-4 );
	cud.cudlength = 13;
	if( ioctl( s, SIOCX25SCALLUSERDATA, &cud ) != 0 ){
		perror("eftp: SIOCX25SCALLUSERDATA failed");
		return 1;
	}

	/* connect to destination x25 address of socket: The address is not
           needed by eft when operating over isdn in X.25-DTE-DTE mode. But as
           we currently cannot set the ISDN number of our peer from this
	   programm we use x25 addresses to select different peers. The mapping
	   X25 address -> peer's ISDN number  must be set up by assigning
	   the peer's ISDN number as the outgoing number to an isdn network
	   interface (using the isdnctrl utility) and by setting up an
	   X.25 route (using the x25route utility) that associates an
	   x25 address with that network interface.
	*/

	if (bind(s, (struct sockaddr *)&x25bind, sizeof (x25bind)) < 0) {
		perror("eftp: unable to bind address to socket");
		exit(1);
	}

	fprintf(stderr, "Trying to establish X.25 DTE-DTE connection to "
		"x25 address \"%s\" ...\n", x25connect.sx25_addr.x25_addr);
	if (connect(s, (struct sockaddr *)&x25connect, sizeof (x25connect)) < 0) {
		perror("eftp: Unable to connect to remote host");
		return 1;
	}
	fprintf(stderr,"eftp: X.25 connection established.\n");

	if(isdn_no){
		/* Now we are connected and don't need the X.25 route
		 * any longer. We tell our parent to release it. Thus,
		 * the route is free for re-use by other eftp clients.
		 * Relasing it also disables other users to play dirty
		 * tricks on us by piggybacking other X.25 connection
		 * throug our isdn connection.
		 */
		eft_signal_release_route(&x25_route);
	}

	if( ioctl( s, SIOCX25GFACILITIES, &facilities ) != 0 ){
		perror("eftp: SIOCX25GFACILITIES failed");
		return 1;
	}
	printf("active facilies wi %d wo %d pi %d po %d\n",
	       facilities.winsize_in,
	       facilities.winsize_out,
	       facilities.pacsize_in,
	       facilities.pacsize_out);

	/*
	 * Now, the x.25 DTE-DTE connection is up. On top of that,
	 * the higer layer eft connection needs to be established.
	 * There are several higer layers, but this is taken care of
	 * be the eft_connect() function.
	 */

	eft = eft_make_instance();

	/* This specifies the amount of (debugging) output printed to stderr*/
	/* tdu_stderr_mask = TDU_LOG_FH | TDU_LOG_REW | TDU_LOG_ERR; */
	tdu_stderr_mask = TDU_LOG_ERR | TDU_LOG_IER | TDU_LOG_OER /* | TDU_LOG_DBG
		| TDU_LOG_HASH  | TDU_LOG_TMP */ ;
#if 0
	/* for maximum amount of debugging output use */
	   tdu_stderr_mask = -1 /* ^ TDU_LOG_TMP ^ TDU_LOG_TRC  */; 
	   /*	   */
#endif
	/* Attach the connected x.25 socket to the eft protocol state
	 * machine */
	eft_attach_socket(eft,s);
	/* 
	 * block SIGPIPE such that peer initiated disconnects will
	 * result in write error indications
	 */
	if( sigprocmask(SIG_BLOCK, &sig_pipe, NULL) )
		perror("sigprocmask()");
	#if 1
	setsockopt(s,SOL_SOCKET,SO_LINGER,&ling,sizeof(ling));
	#endif	
	/* and finally establish logical eft connection */
	if( eft_connect( eft, ident) < 0 ) {
		fprintf(stderr, "eftp: connection failed\n");
		goto Ende;
	}

	fprintf(stderr,"eftp: logged in");
	if(eft_printable_called_addr(eft,called) > 0)
		fprintf(stderr," at %s",called);
	if(eft_printable_assoc_udata(eft,udata))
		fprintf(stderr,", udata=%s",udata);
	fprintf(stderr,"\n");

	/*
	 * Now the main loop processing commands from stdin
	 */

	if(isatty(STDIN_FILENO)) interactive = 1;
	if(interactive){
		if(use_readline){
#ifdef CONFIG_EFTP_READLINE
			rl_callback_handler_install("eftp> ",
						    readline_callback_handler);
#endif
		} else {
			eft_prompt("eftp> ");
		}
	}
	while ( !disconnect_please && eft_is_up(eft) ) {
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		selval = eft_select(eft, STDIN_FILENO+1,
				    &rfds, NULL, NULL, NULL);
		if( selval < 0 ) {
			disconnect_please = 1;
		} else {
			if(interactive && use_readline){
#ifdef CONFIG_EFTP_READLINE
				rl_callback_read_char();
#endif
			} else {
				pipe_callback_read();
			}
		}
	}
	if( interactive && use_readline ){
#ifdef CONFIG_EFTP_READLINE
		rl_callback_handler_remove();
#endif
	}

	printf("eftp: requesting eft_disconnect()\n");
	eft_disconnect( eft );
	/* XXX why this? Without sleep, disconnect processing does not
	 * complete properly on the server side.
	 * this is probably caused by unclean termination of association regime
	 * (wait criteron for end of association falsly claimed to early) 
	 */ 
	sleep(1);
	close(s);

Ende:	pid = getpid();

	printf( "eftp (pid %d) terminating\n", pid);
	return 0;
}
