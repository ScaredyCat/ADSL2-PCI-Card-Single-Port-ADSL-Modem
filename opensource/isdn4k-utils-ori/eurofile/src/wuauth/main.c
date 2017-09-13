
#include <syslog.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#define FTP_NAMES
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <setjmp.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <time.h>


#include "config.h"
#include "extensions.h"

#ifdef SHADOW_PASSWORD
#include <shadow.h>
#endif

#include "pathnames.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64 
#endif

char * eft_access = _PATH_FTPACCESS;

char	remoteaddr [MAXHOSTNAMELEN];
char	remotehost [MAXHOSTNAMELEN];
/* nameserved==1: remoteaddr!=remotehost */
/* nameserved==0: remoteaddr==remotehost */
int	nameserved = 1;
char guestpw[MAXHOSTNAMELEN];
char privatepw[MAXHOSTNAMELEN];

int	log_commands = 0;
int	logging = 0;

/* File transfer logging */
int	xferlog = 0;
int	log_outbound_xfers = 0;
int	log_incoming_xfers = 0;
/* --he char	logfile[MAXPATHLEN];*/
char * logfile = _PATH_XFERLOG;


int	guest;
int	anonymous = 1; 
/* Make use of /etc/eftaccess ? */
int	use_accessfile = 0;
/* How many attempts for the user to login ? */
int	lgi_failure_threshold = 5;	
/* The password structure of the logged in user */
struct passwd *pw;
/* The options, if (un)compressing etc. is allowed */
int	mangleopts = 0;
char	autherrmsg [256];

#define SPT_NONE	0	/* don't use it at all */
#define SPT_REUSEARGV	1	/* cover argv with title information */
#define SPT_BUILTIN	2	/* use libc builtin */
#define SPT_PSTAT	3	/* use pstat(PSTAT_SETCMD, ...) */
#define SPT_PSSTRINGS	4	/* use PS_STRINGS->... */
#define SPT_SYSMIPS	5	/* use sysmips() supported by NEWS-OS 6 */
#define SPT_SCO		6	/* write kernel u. area */
#define SPACELEFT(buf, ptr)  (sizeof buf - ((ptr) - buf))

#ifdef HAVE_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64  /* may be too big */
#endif

#ifndef TRUE
#define  TRUE   1
#endif

#ifndef FALSE
#define  FALSE  !TRUE
#endif

extern int errno;
extern int pidfd
;
extern char *ctime(const time_t *);
#ifndef NO_CRYPT_PROTO
extern char *crypt(const char *, const char *);
#endif
extern FILE *ftpd_popen(char *program, char *type, int closestderr),
 *fopen(const char *, const char *),
 *freopen(const char *, const char *, FILE *);
extern int ftpd_pclose(FILE *iop),
  fclose(FILE *);
extern char *wu_getline(),
 *realpath(const char *pathname, char *result);
extern char version[];
extern char *home;              /* pointer to home directory for glob */
extern char cbuf[];
extern off_t restart_point;

#ifdef VIRTUAL
int virtual_mode=0;
char virtual_root[MAXPATHLEN];
char virtual_banner[MAXPATHLEN];
#endif

#ifdef LOG_FAILED
#define MAXUSERNAMELEN	32
char the_user[MAXUSERNAMELEN];
#endif

SIGNAL_TYPE lostconn(int sig);
SIGNAL_TYPE randomsig(int sig);
SIGNAL_TYPE myoob(int sig);
FILE *getdatasock(char *mode),
 *dataconn(char *name, off_t size, char *mode);
void reply(int, char *fmt, ...);
void lreply(int, char *fmt, ...);

#ifdef NEED_SIGFIX
extern sigset_t block_sigmask;  /* defined in sigfix.c */
#endif

struct aclmember *entry = NULL;

void end_login(void);
void send_data(FILE *, FILE *, off_t);
void dolog(struct sockaddr_in *);

static char ttyline[20];

int checkuser(char *name)
{
    register FILE *fd;
    register char *p;
    char line[BUFSIZ];

    if ((fd = fopen(_PATH_FTPUSERS, "r")) != NULL) {
        while (fgets(line, sizeof(line), fd) != NULL)
            if ((p = strchr(line, '\n')) != NULL) {
                *p = '\0';
                if (line[0] == '#')
                    continue;
                if (strcmp(line, name) == 0) {
                    (void) fclose(fd);
                    return (1);
                }
            }
        (void) fclose(fd);
    }
    return (0);
}


/* Helper function for sgetpwnam(). */
char * sgetsave(char *s)
{
    char *new;
    
    new = (char *) malloc(strlen(s) + 1);

    if (new == NULL) {
        strcpy (autherrmsg, "Local resource failure: malloc");
        return NULL;
        /* NOTREACHED */
    }
    (void) strcpy(new, s);
    return (new);
}

/* Save the result of a getpwnam.  Used for USER command, since the data
 * returned must not be clobbered by any other command (e.g., globbing). */
struct passwd * sgetpwnam(char *name)
{
    static struct passwd save;
    register struct passwd *p;
    char *sgetsave(char *s);

    if ((p = getpwnam(name)) == NULL)
        return p;

    if (save.pw_name)   free(save.pw_name);
    if (save.pw_gecos)  free(save.pw_gecos);
    if (save.pw_dir)    free(save.pw_dir);
    if (save.pw_shell)  free(save.pw_shell);
    if (save.pw_passwd) free(save.pw_passwd);

    save = *p;
    save.pw_name = sgetsave(p->pw_name);
    if (save.pw_name==NULL) return NULL;
    save.pw_passwd = sgetsave(p->pw_passwd);
    if (save.pw_passwd==NULL) return NULL;
#ifdef SHADOW_PASSWORD
        if (p) {
           struct spwd *spw;
	   setspent();
           if ((spw = getspnam(p->pw_name)) != NULL) {
               int expired = 0;
	       /*XXX Does this work on all Shadow Password Implementations? */
	       /* it is supposed to work on Solaris 2.x*/
               time_t now;
               long today;
               
               now = time((time_t*) 0);
               today = now / (60*60*24);
               
               if ((spw->sp_expire > 0) && (spw->sp_expire < today)) expired++;
               if ((spw->sp_max > 0) && (spw->sp_lstchg > 0) &&
		   (spw->sp_lstchg + spw->sp_max < today)) expired++;
	       free(save.pw_passwd);
               save.pw_passwd = sgetsave(expired?"":spw->sp_pwdp);
    	       if (save.pw_passwd==NULL) return NULL;
           }
/* Don't overwrite the password if the shadow read fails, getpwnam() is NIS
   aware but getspnam() is not. */
/* Shadow passwords are optional on Linux.  --marekm */
#if !defined(LINUX) && !defined(UNIXWARE)
           else{
	     free(save.pw_passwd);
	     save.pw_passwd = sgetsave("");
    	     if (save.pw_passwd==NULL) return NULL;
	   }
#endif
/* marekm's fix for linux proc file system shadow passwd exposure problem */
	   endspent();		
        }
#endif
    save.pw_gecos = sgetsave(p->pw_gecos);
    if (save.pw_gecos==NULL) return NULL;
    save.pw_dir = sgetsave(p->pw_dir);
    if (save.pw_dir==NULL) return NULL;
    save.pw_shell = sgetsave(p->pw_shell);
    if (save.pw_shell==NULL) return NULL;
#ifdef M_UNIX
    ret = &save;
DONE:
    endpwent();
#endif
    return(&save);
}
#ifdef SKEY
/*
 * From Wietse Venema, Eindhoven University of Technology. 
 */
/* skey_challenge - additional password prompt stuff */
char   *skey_challenge(char *name, struct passwd *pwd, int pwok)
{
    static char buf[128];
    char sbuf[40];
    struct skey skey;

    /* Display s/key challenge where appropriate. */

    if (pwd == NULL || skeychallenge(&skey, pwd->pw_name, sbuf))
	sprintf(buf, "Password required for %s.", name);
    else
	sprintf(buf, "%s %s for %s.", sbuf,
		pwok ? "allowed" : "required", name);
    return (buf);
}
#endif

int login_attempts;             /* number of failed login attempts */
int askpasswd;                  /* had user command, ask for passwd */


int wuftp_check_user (char *user, char *passw, char *isdnno) {
    char *shell;
    char *getusershell();
    int passwarn = 0;
    int rval = 1;
    char *xpasswd, *salt;

    if(access_init()) return 0;

    strcpy (remotehost, isdnno);
    strcpy (remoteaddr, isdnno);
    nameserved = 0;
    anonymous = 0;
    acl_remove ();
    if (!strcasecmp(user, "ftp") || !strcasecmp(user, "anonymous") ||
	!user[0]) {
      struct aclmember *entry = NULL;
      int machineok=1;

      if (checkuser("ftp") || checkuser("anonymous")) {
          sprintf (autherrmsg, "User %s access denied.", user);
          syslog(LOG_NOTICE,
               "EFT LOGIN REFUSED (eft in %s) FROM %s, %s",
               _PATH_FTPUSERS, remotehost, user);
          return 0;

        /*
        ** Algorithm used:
        ** - if no "guestserver" directive is present,
        **     anonymous access is allowed, for backward compatibility.
        ** - if a "guestserver" directive is present,
        **     anonymous access is restricted to the isdn-numbers listed.
        **
        ** the format of the "guestserver" line is
        ** guestserver [<isdn-number-rule1> [<isdn-number-ruleN>]]
        ** that is, "guestserver" will forbid anonymous access on all machines
        ** while "guestserver 40* 30*" will allow anonymous access on
        ** all callers coming from Hamburg or Berlin.
        **
        */
      } else if (getaclentry("guestserver", &entry)
                 && entry->arg[0] && (int)strlen(entry->arg[0]) > 0) {
	int machinecount = 0;

        machineok=0;
        for (machinecount=0;
             entry->arg[machinecount] && (entry->arg[machinecount])[0];
             machinecount++) {

             if (!fnmatch(entry->arg[machinecount], isdnno,0)) {
                machineok++;
                break;
	     }
        }
      }

      if (!machineok) {
	strcpy (autherrmsg, "Guest login not allowed from given number.");
        syslog(LOG_NOTICE,
               "EFT LOGIN REFUSED (number not in guestservers) FROM %s, %s",
               remotehost, user);
	return 0;
      } else if ((pw = sgetpwnam("ftp")) != NULL) {
        anonymous = 1;      /* for the access_ok call */
        if (access_ok(530) < 1) {
	    sprintf (autherrmsg, "User %s access denied.", user);
            syslog(LOG_NOTICE,
                   "EFT LOGIN REFUSED (access denied) FROM %s, %s",
                   remotehost, user);
	    return 0;
        } else {
            askpasswd = 1;
            if (use_accessfile)
                acl_setfunctions();
        } 
      } else {
        sprintf (autherrmsg, "User %s unknown.", user);
        syslog(LOG_NOTICE,
              "EFT LOGIN REFUSED (ftp not in /etc/passwd) FROM %s, %s",
                   remotehost, user);
	return 0;
      }
    }
#ifdef ANON_ONLY
/* H* fix: define the above to completely DISABLE logins by real users,
   despite ftpusers, shells, or any of that rot.  You can always hang your
   "real" server off some other port, and access-control it. */

    else {  /* "ftp" or "anon" -- MARK your conditionals, okay?! */
      sprintf (autherrmsg, "User %s unknown.", user);
      syslog (LOG_NOTICE,
        "EFT LOGIN REFUSED (not anonymous) FROM %s, %s",
          remotehost, user);
      return;
    }
/* fall here if username okay in any case */
#endif /* ANON_ONLY */

    if ((pw = sgetpwnam(user)) != NULL) {
	char *cp;

        if ((shell = pw->pw_shell) == NULL || *shell == 0)
            shell = _PATH_BSHELL;
        while ((cp = getusershell()) != NULL)
            if (strcmp(cp, shell) == 0)
                break;
        endusershell();
        if (cp == NULL || checkuser(user)) {
	    sprintf (autherrmsg, "User %s access denied.", user);
            syslog(LOG_NOTICE,
             "EFT LOGIN REFUSED (bad shell or username in %s) FROM %s, %s",
                _PATH_FTPUSERS, remotehost, user);
            pw = (struct passwd *) NULL;
            return 0;
        }
        /* if user is a member of any of the guestgroups, cause a chroot() */
        /* after they log in successfully                                  */
        if (use_accessfile)             /* see above.  _H*/
            guest = acl_guestgroup(pw);
    }
    if (access_ok(530) < 1) {
	sprintf (autherrmsg, "User %s access denied.", user);
        syslog(LOG_NOTICE, "EFT LOGIN REFUSED (access denied) FROM %s, %s",
		remotehost, user);
        return;
    } else
        if (use_accessfile)             /* see above.  _H*/
            acl_setfunctions();

#ifdef SKEY
#ifdef SKEY_NAME
    /* this is the old way, but freebsd uses it */
    pwok = skeyaccess(user, NULL, remotehost, remoteaddr);
#else
    /* this is the new way */
    pwok = skeyaccess(pw, NULL, remotehost, remoteaddr);
#endif
#else
#endif
    askpasswd = 1;
    /* Delay before reading passwd after first failed attempt to slow down
     * passwd-guessing programs. */
    if (login_attempts)
        sleep((unsigned) login_attempts);

    if (!askpasswd) return 1;	/* Hey man, we got it! */


    if (!anonymous) {    /* "ftp" is only account allowed no password */
        *guestpw = '\0';
        if (pw == NULL)
          salt = "xx";
        else
          salt = pw->pw_passwd;
          xpasswd = crypt(passw, salt);
        /* The strcmp does not catch null passwords! */
      if (pw !=NULL && *pw->pw_passwd != '\0' &&
          strcmp(xpasswd, pw->pw_passwd) == 0) {
            rval = 0;
           }
        if(rval){
          strcpy (autherrmsg, "Login incorrect.");
#ifdef LOG_FAILED
          if (! strcmp (passw, "NULL"))
                syslog(LOG_NOTICE, "REFUSED \"NULL\" from %s, %s",
                        remotehost, the_user);
          else
          	syslog(LOG_INFO, "failed login from %s, %s",
                              remotehost, the_user);
#endif
          acl_remove();

          pw = NULL;
          if (++login_attempts >= lgi_failure_threshold) {
              syslog(LOG_NOTICE, "repeated login failures from %s",
                     remotehost);
              exit(0);
          }
          return 0;
        }
/* ANONYMOUS USER PROCESSING STARTS HERE */
  } else {
        char *pwin, *pwout = guestpw;
        struct aclmember *entry = NULL;
        int valid;

        if (!*passw) {
            strcpy(guestpw, "[none_given]");
        } else {
            int cnt = sizeof(guestpw) - 2;

            for (pwin = passw; *pwin && cnt--; pwin++)
                if (!isgraph(*pwin))
                    *pwout++ = '_';
                else
                    *pwout++ = *pwin;
        }
    }


    /* if logging is enabled, open logfile before chroot or set group ID */
    if (log_outbound_xfers || log_incoming_xfers) {
        xferlog = open(logfile, O_WRONLY | O_APPEND | O_CREAT, 0660);
        if (xferlog < 0) {
            syslog(LOG_ERR, "cannot open logfile %s: %s", logfile,
                   strerror(errno));
            xferlog = 0;
        }
    }

    enable_signaling(); /* we can allow signals once again: kinch */
    /* if autogroup command applies to user's class change pw->pw_gid */
    if (anonymous && use_accessfile) {  /* see above.  _H*/
        (void) acl_autogroup(pw);
        guest = acl_guestgroup(pw);     /* the new group may be a guest */
        anonymous=!guest;
    }

/* END AUTHENTICATION */
    login_attempts = 0;         /* this time successful */
/* SET GROUP ID STARTS HERE */
    (void) setegid((gid_t) pw->pw_gid);
    (void) initgroups(pw->pw_name, pw->pw_gid);

    expand_id();

    if (anonymous || guest) {
        char *sp;
        /* We MUST do a chdir() after the chroot. Otherwise the old current
         * directory will be accessible as "." outside the new root! */
#ifdef VIRTUAL
        if (virtual_mode && !guest) {
            if (pw->pw_dir)
                free(pw->pw_dir);
            pw->pw_dir = sgetsave(virtual_root);
        }
#endif
        /* determine root and home directory */

        if ((sp = strstr(pw->pw_dir, "/./")) == NULL) {
            if (chroot(pw->pw_dir) < 0 || chdir("/") < 0) {
                strcpy (autherrmsg, "Can't set guest privileges.");
                goto bad;
            }
        } else{
          *sp++ = '\0';
          if (chroot(pw->pw_dir) < 0 || chdir(++sp) < 0) {
            strcpy (autherrmsg, "Can't set guest privileges.");
            goto bad;
          }
        }
    }

#ifdef HAVE_SETREUID
    if (setreuid(-1, (uid_t) pw->pw_uid) < 0) {
#else
    if (seteuid((uid_t) pw->pw_uid) < 0) {
#endif
        strcpy (autherrmsg, "Can't set uid.");
        goto bad;
    }

    if (!anonymous && !guest) {
        if (chdir(pw->pw_dir) < 0) {
            if (chdir("/") < 0) {
                sprintf(autherrmsg, "User %s: can't change directory to %s.",
                      pw->pw_name, pw->pw_dir);
                goto bad;
            } else
                strcpy(autherrmsg, "No directory! Logging in with home=/");
        }
    }

    if (anonymous) {
        strcpy (autherrmsg, "Guest login ok, access restrictions apply.");
        if (logging)
            syslog(LOG_INFO, "ANONYMOUS FTP LOGIN FROM %s, %s",
                   remotehost, passw);
    } else {
        sprintf (autherrmsg, "User %s logged in.%s", pw->pw_name, guest ?
              "  Access restrictions apply." : "");
        if (logging)
            syslog(LOG_INFO, "FTP LOGIN FROM %s [%s], %s",
                   remotehost, remoteaddr, pw->pw_name);
    } /* anonymous */

    /* home = pw->pw_dir;    */      /* home dir for globbing */
    return 1;

  bad:
    /* Forget all about it... */
    if (xferlog)
        close(xferlog);
    xferlog = 0;
    delay_signaling(); /* we can't allow any signals while euid==0: kinch */
    (void) seteuid((uid_t) 0);
    pw = NULL;
    anonymous = 0;
    guest = 0;

    return 0;
}
