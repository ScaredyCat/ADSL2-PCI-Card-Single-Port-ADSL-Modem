/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef UNIX
#include <unistd.h>
#endif /*UNIX*/

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define open          _open
#define close         _close
#endif /*_WIN32*/

#include "stream.h"
#include "muerror.h"

struct _file_stream
{
  FILE *file;
  int offset;

  char *filename;
  stream_t cache;
};

static void
_file_destroy (stream_t stream)
{
  struct _file_stream *fs = stream_get_owner (stream);

  if (fs->filename)
    free (fs->filename);

  if (fs->cache)
    stream_destroy (&fs->cache, stream_get_owner (fs->cache));
  free (fs);
}

static int
_file_read (stream_t stream, char *optr, size_t osize,
	    off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream_get_owner (stream);
  size_t n;
  int err = 0;

  if (!fs->file)
    {
      if (nbytes)
	*nbytes = 0;
      return 0;
    }

  if (fs->offset != offset)
    {
      if (fseek (fs->file, offset, SEEK_SET) != 0)
	return errno;
      fs->offset = offset;
    }

  n = fread (optr, sizeof(char), osize, fs->file);
  if (n == 0)
    {
      if (ferror(fs->file))
	err = errno;
    }
  else
    fs->offset += n;

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_file_readline (stream_t stream, char *optr, size_t osize,
		off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream_get_owner (stream);
  size_t n = 0;
  int err = 0;

  if (!fs->file)
    {
      if (nbytes)
	*nbytes = 0;
      return 0;
    }

  if (fs->offset != offset)
    {
      if (fseek (fs->file, offset, SEEK_SET) != 0)
	return errno;
      fs->offset = offset;
    }

  if (fgets (optr, osize, fs->file) != NULL)
    {
      char *tmp = optr;
      while (*tmp) tmp++; /* strlen(optr) */
      n = tmp - optr;
      /* !!!!! WTF ??? */
      if (n == 0)
	n++;
      else
	fs->offset += n;
    }
  else
    {
      if (ferror (fs->file))
	err = errno;
    }

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_file_write (stream_t stream, const char *iptr, size_t isize,
	    off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream_get_owner (stream);
  size_t n;
  int err = 0;

  if (!fs->file)
    {
      if (nbytes)
	*nbytes = 0;
      return 0;
    }

  if (fs->offset != offset)
    {
      if (fseek (fs->file, offset, SEEK_SET) != 0)
	return errno;
      fs->offset = offset;
    }

  n = fwrite (iptr, sizeof(char), isize, fs->file);
  if (n != isize)
    {
      if (feof (fs->file) == 0)
	err = EIO;
      clearerr(fs->file);
      n = 0;
    }
  else
    fs->offset += n;

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_stdin_file_read (stream_t stream, char *optr, size_t osize,
		  off_t offset, size_t *pnbytes)
{
  int status = 0;
  size_t nbytes;
  struct _file_stream *fs = stream_get_owner (stream);
  int fs_offset = fs->offset;

  if (offset < fs_offset)
    return stream_read (fs->cache, optr, osize, offset, pnbytes);
  else if (offset > fs_offset)
    {
      int status = 0;
      size_t n, left = offset - fs_offset + 1;
      char *buf = malloc (left);
      if (!buf)
	return ENOMEM;
      while (left > 0
	     && (status = stream_read (stream, buf, left, fs_offset, &n)) == 0
	     && n > 0)
	{
	  size_t k;
	  status = stream_write (fs->cache, buf, n, fs_offset, &k);
	  if (status)
	    break;
	  if (k != n)
	    {
	      status = EIO;
	      break;
	    }
	  
	  fs_offset += n;
	  left -= n;
	}
      free (buf);
      if (status)
	return status;
    }
  
  if (feof (fs->file))
    nbytes = 0;
  else
    {
  status = _file_read (stream, optr, osize, fs_offset, &nbytes);
  if (status == 0 && nbytes)
    {
      size_t k;

      status = stream_write (fs->cache, optr, nbytes, fs_offset, &k);
      if (status)
	return status;
      if (k != nbytes)
	return EIO;
      }
    }
  if (pnbytes)
    *pnbytes = nbytes;
  return status;
}

static int
_stdin_file_readline (stream_t stream, char *optr, size_t osize,
		      off_t offset, size_t *pnbytes)
{
  int status;
  size_t nbytes;
  struct _file_stream *fs = stream_get_owner (stream);
  int fs_offset = fs->offset;
  
  if (offset < fs->offset)
    return stream_readline (fs->cache, optr, osize, offset, pnbytes);
  else if (offset > fs->offset)
    return ESPIPE;

  fs_offset = fs->offset;
  status = _file_readline (stream, optr, osize, fs_offset, &nbytes);
  if (status == 0)
    {
      size_t k;

      status = stream_write (fs->cache, optr, nbytes, fs_offset, &k);
      if (status)
	return status;
      if (k != nbytes)
	return EIO;
    }
  if (pnbytes)
    *pnbytes = nbytes;
  return status;
}

static int
_stdout_file_write (stream_t stream, const char *iptr, size_t isize,
		    off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream_get_owner (stream);
  return _file_write (stream, iptr, isize, fs->offset, nbytes);
}

static int
_file_truncate (stream_t stream, off_t len)
{
  struct _file_stream *fs = stream_get_owner (stream);
#ifdef _WIN32
  if (fs->file && SetEndOfFile (fs->file) != 0)
    return errno;
#else
  if (fs->file && ftruncate (fileno(fs->file), len) != 0)
    return errno;
#endif
  return 0;
}

static int
_file_size (stream_t stream, off_t *psize)
{
  struct _file_stream *fs = stream_get_owner (stream);
  struct stat stbuf;
  if (!fs->file)
    {
      if (psize)
	*psize = 0;
      return 0;
    }
  fflush (fs->file);
  if (fstat(fileno(fs->file), &stbuf) == -1)
    return errno;
  if (psize)
    *psize = stbuf.st_size;
  return 0;
}

static int
_file_flush (stream_t stream)
{
  struct _file_stream *fs = stream_get_owner (stream);
  if (fs->file)
    return fflush (fs->file);
  return 0;
}

static int
_file_get_fd (stream_t stream, int *pfd)
{
  struct _file_stream *fs = stream_get_owner (stream);
  int status = 0;
  if (pfd)
    {
      if (fs->file)
	*pfd = fileno (fs->file);
      else
	status = EINVAL;
    }
  return status;
}

static int
_file_close (stream_t stream)
{
  struct _file_stream *fs = stream_get_owner (stream);
  int err = 0;

  if (!stream)
    return EINVAL;

  if (fs->file)
    {
      int flags = 0;

      stream_get_flags (stream, &flags);

      if ((flags & MU_STREAM_NO_CLOSE) == 0)
	{
	  if (fclose (fs->file) != 0)
	    err = errno;
	}
      
      fs->file = NULL;
    }
  return err;
}

static int
_file_open (stream_t stream)
{
  struct _file_stream *fs = stream_get_owner (stream);
  int flg;
  int fd;
  const char *mode;
  char* filename = 0;
  int flags = 0;

  assert(fs);

  filename = fs->filename;

  assert(filename);

  if (fs->file)
    {
      fclose (fs->file);
      fs->file = NULL;
    }

  stream_get_flags(stream, &flags);

  /* Map the flags to the system equivalent.  */
  if (flags & MU_STREAM_WRITE && flags & MU_STREAM_READ)
    return EINVAL;
  else if (flags & MU_STREAM_WRITE)
    flg = O_WRONLY;
  else if (flags & MU_STREAM_RDWR)
    flg = O_RDWR;
  else /* default */
    flg = O_RDONLY;

  /* Local folders should not block it is local disk ???
     We simply ignore the O_NONBLOCK flag
     But take care of the APPEND.  */
  if (flags & MU_STREAM_APPEND)
    flg |= O_APPEND;

  /* Handle CREAT with care, not to follow symlinks.  */
  if (flags & MU_STREAM_CREAT)
    {
      /* First see if the file already exists.  */
      fd = open(filename, flg);
      if (fd == -1)
	{
	  /* Oops bail out.  */
	  if (errno != ENOENT)
	    return errno;
	  /* Race condition here when creating the file ??.  */
	  fd = open(filename, flg|O_CREAT|O_EXCL, 0600);
	  if (fd < 0)
	    return errno;
	}
    }
  else
    {
      fd = open (filename, flg);
      if (fd < 0)
        return errno;
    }

  /* We have to make sure that We did not open
     a symlink. From Casper D. in bugtraq.  */
  if ((flg & MU_STREAM_CREAT) ||
      (flg & MU_STREAM_RDWR) ||
      (flg & MU_STREAM_WRITE))
    {
      struct stat fdbuf;
#ifndef _WIN32
      struct stat filebuf;
#endif

      /* The next two stats should never fail.  */
      if (fstat(fd, &fdbuf) == -1)
	return errno;
#ifndef _WIN32
      if (lstat(filename, &filebuf) == -1)
	return errno;
#endif

      /* Now check that: file and fd reference the same file,
	 file only has one link, file is plain file.  */
      if (
#ifndef _WIN32
	  fdbuf.st_dev != filebuf.st_dev
	  || fdbuf.st_ino != filebuf.st_ino
	  || fdbuf.st_nlink != 1
#else
	  fdbuf.st_nlink != 1
#endif
#ifndef _WIN32
	  || filebuf.st_nlink != 1
#endif
	  || (fdbuf.st_mode & S_IFMT) != S_IFREG)
	{
	  mu_error ("%s must be a plain file with one link\n", filename);
	  close (fd);
	  return EINVAL;
	}
    }
  /* We use FILE * object.  */
  if (flags & MU_STREAM_APPEND)
    mode = "a";
  else if (flags & MU_STREAM_RDWR)
    mode = "r+b";
  else if (flags & MU_STREAM_WRITE)
    mode = "wb";
  else /* Default readonly.  */
    mode = "rb";

  fs->file = fdopen (fd, mode);
  if (fs->file == NULL)
    {
      int ret = errno;
      return ret;
    }

  return 0;
}

int
file_stream_create (stream_t *stream, const char* filename, int flags)
{
  struct _file_stream *fs;
  int ret;

  if (stream == NULL)
    return EINVAL;

  fs = calloc (1, sizeof (struct _file_stream));
  if (fs == NULL)
    return ENOMEM;

  if ((fs->filename = strdup(filename)) == NULL)
  {
    free (fs);
    return ENOMEM;
  }

  ret = stream_create (stream, flags|MU_STREAM_NO_CHECK, fs);
  if (ret != 0)
    {
      free (fs);
      free (fs->filename);
      return ret;
    }

  stream_set_open (*stream, _file_open, fs);
  stream_set_close (*stream, _file_close, fs);
  stream_set_fd (*stream, _file_get_fd, fs);
  stream_set_read (*stream, _file_read, fs);
  stream_set_readline (*stream, _file_readline, fs);
  stream_set_write (*stream, _file_write, fs);
  stream_set_truncate (*stream, _file_truncate, fs);
  stream_set_size (*stream, _file_size, fs);
  stream_set_flush (*stream, _file_flush, fs);
  stream_set_destroy (*stream, _file_destroy, fs);

  return 0;
}

int
stdio_stream_create (stream_t *stream, FILE* file, int flags)
{
  struct _file_stream *fs;
  int ret;

  if (stream == NULL)
    return EINVAL;

  if (file == NULL)
    return EINVAL;

  fs = calloc (1, sizeof (struct _file_stream));
  if (fs == NULL)
    return ENOMEM;

  fs->file = file;

  ret = stream_create (stream, flags|MU_STREAM_NO_CHECK, fs);
  if (ret != 0)
    {
      free (fs);
      return ret;
    }

  if ((ret = memory_stream_create (&fs->cache, 0, MU_STREAM_RDWR))
      || (ret = stream_open (fs->cache)))
    {
      stream_destroy (stream, fs);
      free (fs);
      return ret;
    }

  /* We don't need to open the FILE, just return success. */

  stream_set_open (*stream, NULL, fs);
  stream_set_close (*stream, _file_close, fs);
  stream_set_fd (*stream, _file_get_fd, fs);
  stream_set_read (*stream, _stdin_file_read, fs);
  stream_set_readline (*stream, _stdin_file_readline, fs);
  stream_set_write (*stream, _stdout_file_write, fs);
  stream_set_flush (*stream, _file_flush, fs);
  stream_set_destroy (*stream, _file_destroy, fs);

  return 0;
}

