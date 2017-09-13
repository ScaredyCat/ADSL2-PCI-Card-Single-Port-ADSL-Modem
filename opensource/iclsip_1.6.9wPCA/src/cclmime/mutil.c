/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <limits.h>
#ifndef _WIN32
#include <netdb.h>
#include <pwd.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef UNIX
#include <unistd.h>
#include <sys/param.h>
#include <sys/wait.h>
#endif /*UNIX*/

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define mktemp       _mktemp
#define O_CREAT      _O_CREAT
#define O_EXCL       _O_EXCL
#define O_RDWR       _O_RDWR
#endif /*_WIN32*/

/*
#include "cm_def.h"
#include "cm_utl.h"
*/
#include <common/cm_def.h>
#include <common/cm_utl.h>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "address.h"
#include "muerror.h"
#include "iterator.h"
#include "mutil.h"
#include "parse822.h"
/*#include "mu_auth.h"*/

/* convert a sequence of hex characters into an integer */

unsigned long
mu_hex2ul (char hex)
{
  if (hex >= '0' && hex <= '9')
   return hex - '0';

  if (hex >= 'a' && hex <= 'z')
    return hex - 'a';

  if (hex >= 'A' && hex <= 'Z')
    return hex - 'A';

  return -1;
}

size_t
mu_hexstr2ul (unsigned long *ul, const char *hex, size_t len)
{
  size_t r;

  *ul = 0;

  for (r = 0; r < len; r++)
    {
      unsigned long v = mu_hex2ul (hex[r]);

      if (v == (unsigned long)-1)
	return r;

      *ul = *ul * 16 + v;
    }
  return r;
}

/*
char *
mu_get_homedir (void)
{
  char *homedir = getenv ("HOME");
  if (homedir)
    homedir = strdup (homedir);
  else
    {
      struct mu_auth_data *auth = mu_get_auth_by_uid (getuid ());
      if (!auth)
	return NULL;
      homedir = strdup (auth->dir);
      mu_auth_data_free (auth);
    }
  return homedir;
}
*/


/* Smart strncpy that always add the null and returns the number of bytes
   written.  */
size_t
mu_cpystr (char *dst, const char *src, size_t size)
{
  size_t len = src ? strlen (src) : 0 ;
  if (dst == NULL || size == 0)
    return len;
  if (len >= size)
    len = size - 1;
  memcpy (dst, src, len);
  dst[len] = '\0';
  return len;
}

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif

int
mu_get_host_name (char **host)
{
  char hostname[MAXHOSTNAMELEN + 1];
  struct hostent *hp = NULL;
  char *domain = NULL;

  gethostname (hostname, sizeof hostname);
  hostname[sizeof (hostname) - 1] = 0;

  if ((hp = gethostbyname (hostname)))
    domain = hp->h_name;
  else
    domain = hostname;

  domain = strdup (domain);

  if (!domain)
    return ENOMEM;

  *host = domain;

  return 0;
}

static char *mu_user_email_domain = 0;

int
mu_get_user_email_domain (const char **domain)
{
  int err = 0;

  if (!mu_user_email_domain)
    {
      if ((err = mu_get_host_name (&mu_user_email_domain)))
	return err;
    }

  *domain = mu_user_email_domain;

  return 0;
}

int
mu_set_user_email_domain (const char *domain)
{
  char* d = NULL;
  
  if (!domain)
    return EINVAL;
  
  d = strdup (domain);

  if (!d)
    return ENOMEM;

  if (mu_user_email_domain)
    free (mu_user_email_domain);

  mu_user_email_domain = d;

  return 0;
}
/* Create and open a temporary file. Be very careful about it, since we
   may be running with extra privilege i.e setgid().
   Returns file descriptor of the open file.
   If namep is not NULL, the pointer to the malloced file name will
   be stored there. Otherwise, the file is unlinked right after open,
   i.e. it will disappear after close(fd). */

#ifndef P_tmpdir
# define P_tmpdir "/tmp"
#endif

int
mu_tempfile (const char *tmpdir, char **namep)
{
  char *filename;
  int fd;

  if (!tmpdir)
    tmpdir = (getenv ("TMPDIR")) ? getenv ("TMPDIR") : P_tmpdir;

  filename = malloc (strlen (tmpdir) + /*'/'*/1 + /* "muXXXXXX" */8 + 1);
  if (!filename)
    return -1;
  sprintf (filename, "%s/muXXXXXX", tmpdir);

#ifdef HAVE_MKSTEMP
  {
    int save_mask = umask (077);
    fd = mkstemp (filename);
    umask (save_mask);
  }
#else
  if (mktemp (filename))
    fd = open (filename, O_CREAT|O_EXCL|O_RDWR, 0600);
  else
    fd = -1;
#endif

  if (fd == -1)
    {
      mu_error ("Can not open temporary file: %s", strerror(errno));
      free (filename);
      return -1;
    }

  if (namep)
    *namep = filename;
  else
    {
      unlink (filename);
      free (filename);
    }

  return fd;
}

/* Create a unique temporary file name in tmpdir. The function
   creates an empty file with this name to avoid possible race
   conditions. Returns a pointer to the malloc'ed file name.
   If tmpdir is NULL, the value of the environment variable
   TMPDIR or the hardcoded P_tmpdir is used, whichever is defined. */

char *
mu_tempname (const char *tmpdir)
{
  char *filename = NULL;
  int fd = mu_tempfile (tmpdir, &filename);
  close (fd);
  return filename;
}

