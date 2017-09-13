#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef UNIX
#include <unistd.h>
#endif /*UNIX*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define close(fd)      _close(fd)
#endif /*_WIN32*/

/*
#include "cm_def.h"
#include "cm_utl.h"
*/
#include <common/cm_def.h>
#include <common/cm_utl.h>

#include "stream.h"
#include "mutil.h"
#include "mime_body.h"
#include "mimetypes/body0.h"


#define BODY_MODIFIED 0x10000


static int lazy_create    __P ((body_t));
static int _body_flush    __P ((stream_t));
static int _body_get_fd   __P ((stream_t, int *));
static int _body_read     __P ((stream_t, char *, size_t, off_t, size_t *));
static int _body_readline __P ((stream_t, char *, size_t, off_t, size_t *));
static int _body_truncate __P ((stream_t, off_t));
static int _body_size     __P ((stream_t, off_t *));
static int _body_write    __P ((stream_t, const char *, size_t, off_t, size_t *));


static int _body_get_size   __P ((body_t, size_t *));
static int _body_get_lines  __P ((body_t, size_t *));

static int _body_get_lines0 (stream_t stream, size_t *plines);
static int _body_get_lines1 (stream_t stream, size_t *plines);
static int _body_get_size0 (stream_t stream, size_t *psize);


CCLAPI RCODE	mimeBdyNew(MimeBdy *pbody)
{
  body_t body;

  if (pbody == NULL)
    return EINVAL;

  body = calloc (1, sizeof (*body));
  if (body == NULL)
    return ENOMEM;

  /* body->owner = owner; */
  body->owner=NULL;

  *pbody = body;
  return 0;
}


CCLAPI void		mimeBdyFree(MimeBdy *pbody)
{
  if (pbody && *pbody)
    {
      body_t body = *pbody;
/*
      if (body->owner == owner)
	{
*/
	  if (body->filename)
	    {
	      /* FIXME: should we do this?  */
	      remove (body->filename);
	      free (body->filename);
	    }

	  if (body->stream)
	    stream_destroy (&(body->stream), body);

	  if (body->fstream)
	    {
	      stream_close (body->fstream);
	      stream_destroy (&(body->fstream), NULL);
	    }

	  free (body);
/*
	}
*/
	  *pbody = NULL;
    }
}


CCLAPI
RCODE	mimeBdyLines (MimeBdy body, size_t *plines)
{
  if (body == NULL)
    return EINVAL;
  if (body->_lines)
    return body->_lines (body, plines);
  /* Fall on the stream.  */
  if (body->stream)
    return _body_get_lines1 (body->stream, plines);
  if (plines)
    *plines = 0;
  return 0;
}


CCLAPI
RCODE mimeBdySize (MimeBdy body, size_t *psize)
{
  if (body == NULL)
    return EINVAL;
  if (body->_size)
    return body->_size (body, psize);
  /* Fall on the stream.  */
  if (body->stream)
    return _body_get_size0 (body->stream, psize);
  if (psize)
    *psize = 0;
  return 0;
}

CCLAPI
RCODE	mimeBdyGetStrm(MimeBdy body, stream_t *pstream)
{
  if (body == NULL || pstream == NULL)
    return EINVAL;

  if (body->stream == NULL)
    {
      int fd;
      int status = stream_create (&body->stream, MU_STREAM_RDWR, body);
      if (status != 0)
	return status;
      /* Create the temporary file.  */
      fd = lazy_create (body);
      if (fd == -1)
	return errno;
      status = file_stream_create (&body->fstream, body->filename, MU_STREAM_RDWR);
      if (status != 0)
	return status;
      status = stream_open (body->fstream);
      close (fd);
      if (status != 0)
	return status;
      stream_set_fd (body->stream, _body_get_fd, body);
      stream_set_read (body->stream, _body_read, body);
      stream_set_readline (body->stream, _body_readline, body);
      stream_set_write (body->stream, _body_write, body);
      stream_set_truncate (body->stream, _body_truncate, body);
      stream_set_size (body->stream, _body_size, body);
      stream_set_flush (body->stream, _body_flush, body);
      /* Override the defaults.  */
      body->_lines = (int (*)(struct _body *,unsigned int *))_body_get_lines;
      body->_size = (int (*)(struct _body *,unsigned int *))_body_get_size;
    }
  *pstream = body->stream;
  return 0;
}

CCLAPI 
RCODE	mimeBdySetStrm(MimeBdy body, stream_t stream)
{
  if (body == NULL)
   return EINVAL;
  /*if (body->owner != owner)
    return EACCES;*/
  /* make sure we destroy the old one if it is own by the body */
  stream_destroy (&(body->stream), body);
  body->stream = stream;
  body->flags |= BODY_MODIFIED;
  return 0;
}



/* Stub function for the body stream.  */

static int
_body_get_fd (stream_t stream, int *fd)
{
  body_t body = stream_get_owner (stream);
  return stream_get_fd (body->fstream, fd);
}

static int
_body_read (stream_t stream,  char *buffer, size_t n, off_t off, size_t *pn)
{
  body_t body = stream_get_owner (stream);
  return stream_read (body->fstream, buffer, n, off, pn);
}

static int
_body_readline (stream_t stream, char *buffer, size_t n, off_t off, size_t *pn)
{
  body_t body = stream_get_owner (stream);
  return stream_readline (body->fstream, buffer, n, off, pn);
}

static int
_body_write (stream_t stream, const char *buf, size_t n, off_t off, size_t *pn)
{
  body_t body = stream_get_owner (stream);
  return stream_write (body->fstream, buf, n, off, pn);
}

static int
_body_truncate (stream_t stream, off_t n)
{
  body_t body = stream_get_owner (stream);
  return stream_truncate (body->fstream, n);
}

static int
_body_size (stream_t stream, off_t *size)
{
  body_t body = stream_get_owner (stream);
  return stream_size (body->fstream, size);
}

static int
_body_flush (stream_t stream)
{
  body_t body = stream_get_owner (stream);
  return stream_flush (body->fstream);
}

/* Default function for the body.  */
static int
_body_get_lines (body_t body, size_t *plines)
{
  return _body_get_lines1 (body->fstream, plines);
}

static int
_body_get_size (body_t body, size_t *psize)
{
  return _body_get_size0 (body->fstream, psize);
}

static int
_body_get_lines1 (stream_t stream, size_t *plines)
{
  /* int status =  stream_flush (stream); */
  int status=0;
  size_t lines = 0;
  if (status == 0)
    {
      char buf[128];
      size_t n = 0;
      off_t off = 0;
      while ((status = stream_readline (stream, buf, sizeof buf,
					off, &n)) == 0 && n > 0)
	{
	  if (buf[n - 1] == '\n')
	    lines++;
	  off += n;
	}
    }
  if (plines)
    *plines = lines;
  return status;
}

static int
_body_get_lines0 (stream_t stream, size_t *plines)
{
  int status =  stream_flush (stream);
  
  size_t lines = 0;
  if (status == 0)
    {
      char buf[128];
      size_t n = 0;
      off_t off = 0;
      while ((status = stream_readline (stream, buf, sizeof buf,
					off, &n)) == 0 && n > 0)
	{
	  if (buf[n - 1] == '\n')
	    lines++;
	  off += n;
	}
    }
  if (plines)
    *plines = lines;
  return status;
}


static int
_body_get_size0 (stream_t stream, size_t *psize)
{
  off_t off = 0;
  int status = stream_size (stream, &off);
  if (psize)
    *psize = off;
  return status;
}


#ifndef P_tmpdir
#  define P_tmpdir "/tmp"
#endif

static int
lazy_create (body_t body)
{
  return mu_tempfile (NULL, &body->filename);
}
