#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* #include "property.h" */
/*
#include "cm_def.h"
#include "cm_utl.h"
*/
#include <common/cm_def.h>
#include <common/cm_utl.h>

#include "mimetypes/stream0.h"

static int refill (stream_t, off_t);

CCLAPI RCODE	mimeStrmNew(stream_t *pstream, int flags)
{
  stream_t stream;
  if (pstream == NULL)
    return EINVAL;
  stream = calloc (1, sizeof (*stream));
  if (stream == NULL)
    return ENOMEM;
  stream->owner = NULL;
  stream->flags = flags;
  /* By default unbuffered, the buffering scheme is not for all models, it
     really makes sense for network streams, where there is no offset.  */
  /* stream->rbuffer.bufsiz = BUFSIZ; */
  *pstream = stream;
  return 0;
}

CCLAPI 
RCODE	mimeStrmOpen (stream_t stream)
{
  if (stream == NULL)
    return EINVAL;
  stream->state = MU_STREAM_STATE_OPEN;

  if (stream->_open)
    return stream->_open (stream);
  return  0;
}

CCLAPI
RCODE	mimeStrmClose(stream_t stream)
{
  if (stream == NULL)
    return EINVAL;
  stream->state = MU_STREAM_STATE_CLOSE;
  /* Clear the buffer of any residue left.  */
  if (stream->rbuffer.base)
    {
      stream->rbuffer.ptr = stream->rbuffer.base;
      stream->rbuffer.count = 0;
      memset (stream->rbuffer.base, '\0', stream->rbuffer.bufsiz);
    }
  if (stream->_close)
    return stream->_close (stream);
  return  0;
}


CCLAPI void	mimeStrmFree(stream_t *pstream)
{
   if (pstream && *pstream)
    {
      stream_t stream = *pstream;
      if ((stream->flags & MU_STREAM_NO_CHECK))
	{
	  stream_close(stream);
	  if (stream->rbuffer.base)
	    free (stream->rbuffer.base);

	  if (stream->_destroy) 
	    stream->_destroy (stream);

	  free (stream);
	}
      *pstream = NULL;
    }
}

/* We have to be clear about the buffering scheme, it is not designed to be
   used as a full-fledged buffer mechanism.  It is a simple mechanism for
   networking. Lots of code between POP and IMAP can be shared this way.
   - First caveat; the code maintains its own offset (rbuffer.offset member)
   and if it does not match the requested one, the data is flushed
   and the underlying _read is called. It is up to the latter to return
   EISPIPE when appropriate.
   - Again, this is targeting networking stream to make readline()
   a little bit more efficient, instead of reading a char at a time.  */

CCLAPI 
RCODE	mimeStrmRead(stream_t is, char *buf, size_t count, 
					 off_t offset, size_t *pnread)
{
  int status = 0;
  if (is == NULL || is->_read == NULL)
    return EINVAL;

  is->state = MU_STREAM_STATE_READ;

  /* Sanity check; noop.  */
  if (count == 0)
    {
      if (pnread)
	*pnread = 0;
      return 0;
    }

  /* If rbuffer.bufsiz == 0.  It means they did not want the buffer
     mechanism.  Good for them.  */
  if (is->rbuffer.bufsiz == 0)
    status = is->_read (is, buf, count, offset, pnread);
  else
    {
      size_t residue = count;
      size_t r;

      /* If the amount requested is bigger than the buffer cache size,
	 bypass it.  Do no waste time and let it through.  */
      if (count > is->rbuffer.bufsiz)
	{
	  r = 0;
	  /* Drain the buffer first.  */
	  if (is->rbuffer.count > 0 && offset == is->rbuffer.offset)
	    {
	      (void)memcpy(buf, is->rbuffer.ptr, is->rbuffer.count);
	      is->rbuffer.offset += is->rbuffer.count;
	      residue -= is->rbuffer.count;
	      buf += is->rbuffer.count;
	      offset += is->rbuffer.count;
	    }
	  is->rbuffer.count = 0;
	  status = is->_read (is, buf, residue, offset, &r);
	  is->rbuffer.offset += r;
	  residue -= r;
	  if (pnread)
	    *pnread = count - residue;
	  return status;
	}

      /* Fill the buffer, do not want to start empty hand.  */
      if (is->rbuffer.count <= 0 || offset != is->rbuffer.offset)
	{
	  status = refill (is, offset);
	  if (status != 0)
	    return status;
	  /* Reached the end ??  */
	  if (is->rbuffer.count == 0)
	    {
	      if (pnread)
		*pnread = 0;
	      return status;
	    }
	}

      /* Drain the buffer, if we have less then requested.  */
      while (residue > (size_t)(r = is->rbuffer.count))
	{
	  (void)memcpy (buf, is->rbuffer.ptr, (size_t)r);
	  /* stream->rbuffer.count = 0 ... done in refill */
	  is->rbuffer.ptr += r;
	  is->rbuffer.offset += r;
	  buf += r;
	  residue -= r;
	  status = refill (is, is->rbuffer.offset);
	  if (status != 0)
	    {
	      /* We have something in the buffer return the error on the
		 next call .  */
	      if (count != residue)
		{
		  if (pnread)
		    *pnread = count - residue;
		  status = 0;
		}
	      return status;
	    }
	  /* Did we reach the end.  */
	  if (is->rbuffer.count == 0)
	    {
	      if (pnread)
		*pnread = count - residue;
	      return status;
	    }
	}
      (void)memcpy(buf, is->rbuffer.ptr, residue);
      is->rbuffer.count -= residue;
      is->rbuffer.ptr += residue;
      is->rbuffer.offset += residue;
      if (pnread)
	*pnread = count;
    }
  return status;
}

/*
 * Read at most n-1 characters.
 * Stop when a newline has been read, or the count runs out.
 */
CCLAPI RCODE	mimeStrmReadLine(stream_t is, char *buf, size_t count, 
					     int offset, int *pnread)
{
  int status = 0;

  if (is == NULL)
    return EINVAL;

  is->state = MU_STREAM_STATE_READ;

  switch (count)
    {
    case 1:
      /* why would they do a thing like that?
	 stream_readline() is __always null terminated.  */
      if (buf)
	*buf = '\0';
    case 0: /* Buffer is empty noop.  */
      if (pnread)
	*pnread = 0;
      return 0;
    }

  /* Use the provided readline.  */
  if (is->rbuffer.bufsiz == 0 &&  is->_readline != NULL)
    status = is->_readline (is, buf, count, offset, pnread);
  else if (is->rbuffer.bufsiz == 0) /* No Buffering.  */
    {
      size_t n, nr = 0;
      char c;
      /* Grossly inefficient hopefully they override this */
      for (n = 1; n < count; n++)
	{
	  status = is->_read (is, &c, 1, offset, &nr);
	  if (status != 0) /* Error.  */
	    return status;
	  else if (nr == 1)
	    {
	      *buf++ = c;
	      offset++;
	      if (c == '\n') /* Newline is stored like fgets().  */
		break;
	    }
	  else if (nr == 0)
	    {
	      if (n == 1) /* EOF, no data read.  */
		n = 0;
	      break; /* EOF, some data was read.  */
	    }
	}
      *buf = '\0';
      if (pnread)
	*pnread = (n == count) ? n - 1: n;
    }
  else /* Buffered.  */
    {
      char *s = buf;
      char *p, *nl;
      size_t len;
      size_t total = 0;

      count--;  /* Leave space for the null.  */

      /* If out of range refill.  */
      /*      if ((offset < is->rbuffer.offset */
      /*	   || offset > (is->rbuffer.offset + is->rbuffer.count))) */
      if (offset != is->rbuffer.offset)
	{
	  status = refill (is, offset);
	  if (status != 0)
	    return status;
	  if (is->rbuffer.count == 0)
	    {
	      if (pnread)
		*pnread = 0;
	      return 0;
	    }
	}

      while (count != 0)
	{
	  /* If the buffer is empty refill it.  */
	  len = is->rbuffer.count;
	  if (len <= 0)
	    {
	      status = refill (is, is->rbuffer.offset);
	      if (status != 0)
		{
		  if (s != buf)
		    break;
		}
	      len = is->rbuffer.count;
	      if (len == 0)
		break;
	    }
	  p = is->rbuffer.ptr;

	  /* Scan through at most n bytes of the current buffer,
	     looking for '\n'.  If found, copy up to and including
	     newline, and stop.  Otherwise, copy entire chunk
	     and loop.  */
	  if (len > count)
	    len = count;
	  nl = memchr ((void *)p, '\n', len);
	  if (nl != NULL)
	    {
	      len = ++nl - p;
	      is->rbuffer.count -= len;
	      is->rbuffer.ptr = nl;
	      is->rbuffer.offset += len;
	      (void)memcpy ((void *)s, (void *)p, len);
	      total += len;
	      s[len] = 0;
	      if (pnread)
		*pnread = total;
	      return 0;
	    }
	  is->rbuffer.count -= len;
	  is->rbuffer.ptr += len;
	  is->rbuffer.offset += len;
	  (void)memcpy((void *)s, (void *)p, len);
	  total += len;
	  s += len;
	  count -= len;
        }
      *s = 0;
      if (pnread)
	*pnread = s - buf;
    }
  return status;
}


CCLAPI RCODE	mimeStrmWrite(stream_t os, const char *buf, size_t count, 
					     off_t offset, size_t *pnwrite)
{
  int nleft;
  int err = 0;
  size_t nwriten = 0;
  size_t total = 0;

  if (os == NULL || os->_write == NULL)
      return EINVAL;
  os->state = MU_STREAM_STATE_WRITE;

  nleft = count;
  /* First try to send it all.  */
  while (nleft > 0)
    {
      err = os->_write (os, buf, nleft, offset, &nwriten);
      if (err != 0 || nwriten == 0)
        break;
      nleft -= nwriten;
      total += nwriten;
      buf += nwriten;
    }
  if (pnwrite)
    *pnwrite = total;
  return err;
}




static int
refill (stream_t stream, off_t offset)
{
  if (stream->_read)
    {
      int status;
      if (stream->rbuffer.base == NULL)
	{
	  stream->rbuffer.base = calloc (1, stream->rbuffer.bufsiz);
	  if (stream->rbuffer.base == NULL)
	    return ENOMEM;
	}
      stream->rbuffer.ptr = stream->rbuffer.base;
      stream->rbuffer.offset = offset;
      stream->rbuffer.count = 0;
      status = stream->_read (stream, stream->rbuffer.ptr,
			      stream->rbuffer.bufsiz, offset,
			      (size_t *)&(stream->rbuffer.count));
      return status;
    }
  return ENOSYS;
}