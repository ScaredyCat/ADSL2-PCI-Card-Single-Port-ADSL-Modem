
/* Copyright (c) 1993, 1994  Washington University in Saint Louis
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * Washington University in Saint Louis and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASHINGTON UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASHINGTON
 * UNIVERSITY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <time.h>
#include <pwd.h>
#include <setjmp.h>
#include <grp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>

#if defined(HAVE_STATVFS)
#include <sys/statvfs.h>
#elif defined(HAVE_SYS_VFS)
#include <sys/vfs.h>
#elif defined(HAVE_SYS_MOUNT)
#include <sys/mount.h>
#endif

#include <arpa/ftp.h>

#include "pathnames.h"
#include "extensions.h"

extern char remotehost[];

/*************************************************************************/
/* FUNCTION  : msg_massage                                               */
/* PURPOSE   : Scan a message line for magic cookies, replacing them as  */
/*             needed.                                                   */
/* ARGUMENTS : pointer input and output buffers                          */
/*************************************************************************/

void
#ifdef __STDC__
msg_massage(char *inbuf, char *outbuf)
#else
msg_massage(inbuf,outbuf)
char *inbuf;
char *outbuf;
#endif
{
    char *inptr = inbuf;
    char *outptr = outbuf;
    char buffer[MAXPATHLEN];
    time_t curtime;
    int limit;
    extern struct passwd *pw;
    struct aclmember *entry;

    (void) acl_getclass(buffer);
    limit = acl_getlimit(buffer, NULL);

    while (*inptr) {
        if (*inptr != '%')
            *outptr++ = *inptr;
        else {
            entry = NULL;
            switch (*++inptr) {
            case 'E':
                if ( (getaclentry("email", &entry)) && ARG0 )
                    sprintf(outptr, "%s", ARG0); 
                else
                    *outptr = '\0';
                break;
            case 'N': 
                sprintf(outptr, "%d", acl_countusers(buffer)); 
                break; 
            case 'M':
	        if (limit > 0){
                  sprintf(outptr, "%d", limit);
  	        }else{
		  strcpy(outptr,"unlimited");
		}
                break;
            case 'T':
 	        (void) time(&curtime);
                strncpy(outptr, ctime(&curtime), 24);
                *(outptr + 24) = '\0';
                break;

            case 'F':
#if defined(HAVE_STATVFS) || defined(HAVE_SYS_VFS) || defined(HAVE_SYS_MOUNT)
                sprintf(outptr, "%lu", getSize("."));
#endif
                break;

            case 'C':
#define HAVE_GETCWD 1 /* Hack by HE to force getcwd, #include'ing config.h
		       * should be better but causes other problems */
#ifdef HAVE_GETCWD
                (void) getcwd(outptr, MAXPATHLEN);
#else
                (void) getwd(outptr);
#endif
                break;

            case 'R':
                strcpy(outptr, remotehost);
                break;

/*
            case 'L':
                strcpy(outptr, hostname);
                break;
*/

            case 'U':
                if (pw)
		  strcpy(outptr, pw->pw_name);
		else
		  strcpy(outptr, "[unknown]");
                break;

/*
            case 's':
                strncpy(outptr, shuttime, 24);
                *(outptr + 24) = '\0';
                break;

            case 'd':
                strncpy(outptr, disctime, 24);
                *(outptr + 24) = '\0';
                break;

            case 'r':
                strncpy(outptr, denytime, 24);
                *(outptr + 24) = '\0';
                break;
*/

/* KH : cookie %u for RFC931 name */
/*
            case 'u':
                if (authenticated) strncpy(outptr, authuser, 24);
                else strcpy(outptr,"[unknown]");
                *(outptr + 24) = '\0'; 
                break;
*/

            case '%':
                *outptr++ = '%';
                *outptr = '\0';
                break;

            default:
                *outptr++ = '%';
                *outptr++ = '?';
                *outptr = '\0';
                break;
            }
            while (*outptr)
                outptr++;
        }
        inptr++;
    }
    *outptr = '\0';
}

int reply (int msg, char *str) {
	fprintf (stderr, "%d - %s\n", msg, str);
}


void expand_id(void) {
    struct aclmember *entry = NULL;
    struct passwd *pwent;
    struct group *grent;
    char buf[BUFSIZ];

    while (getaclentry("upload", &entry) && ARG0 && ARG1 && ARG2 != NULL) {
        if (ARG3 && ARG4) {
            pwent = getpwnam(ARG3);
            grent = getgrnam(ARG4);

            if (pwent)  sprintf(buf, "%d", pwent->pw_uid);
            else        sprintf(buf, "%d", 0);
            ARG3 = (char *) malloc(strlen(buf) + 1);
            strcpy(ARG3, buf);

            if (grent)  sprintf(buf, "%d", grent->gr_gid);
	    else        sprintf(buf, "%d", 0);
            ARG4 = (char *) malloc(strlen(buf) + 1);
            strcpy(ARG4, buf);
            endgrent();
        }
    }
}


