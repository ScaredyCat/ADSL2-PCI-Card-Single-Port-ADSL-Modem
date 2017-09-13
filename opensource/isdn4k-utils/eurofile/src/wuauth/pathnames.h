/* Copyright (c) 1989 The Regents of the University of California. All rights
 * reserved.
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
 * University of California, Berkeley and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)$Id: pathnames.h,v 1.1 1999/06/30 17:19:36 he Exp $ based on
 * pathnames.h 5.2 (Berkeley) 6/1/90 
 */


#define _PATH_EXECPATH  "/bin/eft-exec"

#ifdef USE_ETC
#define _PATH_FTPUSERS  "/etc/eftusers"
#define _PATH_FTPACCESS "/etc/eftaccess"
#define _PATH_CVT       "/etc/eftconversions"
#define _PATH_PRIVATE   "/etc/eftgroups"
#else
#ifdef USE_I4L_CONFDIR
#define _PATH_FTPUSERS  CONFIG_I4L_CONFDIR "/eftusers"
#define _PATH_FTPACCESS CONFIG_I4L_CONFDIR "/eftaccess"
#define _PATH_CVT       CONFIG_I4L_CONFDIR "/eftconversions"
#define _PATH_PRIVATE   CONFIG_I4L_CONFDIR "/eftgroups"
#else
#ifdef USE_ETC_EFTD
#define _PATH_FTPUSERS  "/etc/eftd/eftusers"
#define _PATH_FTPACCESS "/etc/eftd/eftaccess"
#define _PATH_CVT       "/etc/eftd/eftconversions"
#define _PATH_PRIVATE   "/etc/eftd/eftgroups"
#else
#ifdef USE_LOCAL_ETC
#define _PATH_FTPUSERS  "/usr/local/etc/eftusers"
#define _PATH_FTPACCESS "/usr/local/etc/eftaccess"
#define _PATH_CVT       "/usr/local/etc/eftconversions"
#define _PATH_PRIVATE   "/usr/local/etc/eftgroups"
#else
#define _PATH_FTPUSERS  "/usr/local/lib/eftd/eftusers"
#define _PATH_FTPACCESS "/usr/local/lib/eftd/eftaccess"
#define _PATH_CVT       "/usr/local/lib/eftd/eftconversions"
#define _PATH_PRIVATE   "/usr/local/lib/eftd/eftgroups"
#endif
#endif
#endif
#endif

#ifdef USE_VAR
#ifdef USE_PID
#define _PATH_PIDNAMES  "/var/pid/eft.pids-%s"
#else
#ifdef VAR_RUN
#define _PATH_PIDNAMES  "/var/run/eft.pids-%s"
#else
#define _PATH_PIDNAMES  "/var/adm/eft.pids-%s"
#endif
#endif
#ifdef USE_LOG
#define _PATH_XFERLOG   "/var/log/xferlog"
#else
#define _PATH_XFERLOG   "/var/adm/xferlog"
#endif
#else
#ifndef _PATH_PIDNAMES
#define _PATH_PIDNAMES  "/usr/local/lib/eftd/pids/%s"
#endif
#ifndef _PATH_XFERLOG
#define _PATH_XFERLOG   "/usr/local/logs/xferlog"
#endif
#endif

#ifndef _PATH_UTMP
#ifdef UTMP_FILE
#define _PATH_UTMP UTMP_FILE
#endif
#endif

#ifndef _PATH_WTMP
#ifdef WTMP_FILE
#define _PATH_WTMP WTMP_FILE
#endif
#endif

#ifndef _PATH_UTMP
#define _PATH_UTMP      "/etc/utmp"
#endif
#ifndef _PATH_WTMP
#define _PATH_WTMP      "/usr/adm/wtmp"
#endif
#ifndef _PATH_LASTLOG
#ifdef SOLARIS_2
#define _PATH_LASTLOG   "/var/adm/lastlog"
#else
#define _PATH_LASTLOG   "/usr/adm/lastlog"
#endif
#endif

#ifndef _PATH_BSHELL
#define _PATH_BSHELL    "/bin/sh"
#endif

#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL   "/dev/null"
#endif

#ifdef  HOST_ACCESS
#ifdef USE_ETC
#define _PATH_FTPHOSTS  "/etc/efthosts"
#else
#ifdef USE_I4L_CONFDIR
#define _PATH_FTPHOSTS  CONFIG_I4L_CONFDIR "/efthosts"
#else
#ifdef USE_ETC_FTPD
#define _PATH_FTPHOSTS  "/etc/eftd/efthosts"
#else
#ifdef USE_LOCAL_ETC
#define _PATH_FTPHOSTS  "/usr/local/etc/efthosts"
#else
#define _PATH_FTPHOSTS  "/usr/local/lib/eftd/efthosts"
#endif
#endif
#endif
#endif
#endif


