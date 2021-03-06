
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef _WIN32
#include <windows.h>
#define strncasecmp(s1,s2,n)    strICmpN(s1,s2,n)
#define strcasecmp(s1,s2)       strICmp(s1,s2)
#endif /*_WIN32*/
/*
#include "cm_def.h"
#include "cm_utl.h"
*/
#include <common/cm_def.h>
#include <common/cm_utl.h>

#include "stream.h"
/* #include "address.h" */
#include "mime_hdr.h"
#include "mimetypes/header0.h"


#define HEADER_MODIFIED 1

static int header_parse    __P ((header_t, const char *, int));
static int header_get_fvalue (header_t, const char *, char *, size_t, size_t *);
static int header_set_fvalue (header_t, const char *, char *);
static void header_free_cache __P ((header_t));
static int fill_blurb      __P ((header_t));
static int header_read     __P ((stream_t, char *, size_t, off_t, size_t *));
static int header_readline __P ((stream_t, char *, size_t, off_t, size_t *));
static int header_write    __P ((stream_t, const char *, size_t, off_t,
				 size_t *));

/*
	mimeHdrNew: Create a new header, if data of size len is not NULL, 
	it is parsed.
*/
CCLAPI RCODE	mimeHdrNew(MimeHdr *phdr, const char *data, int len)
{
	MimeHdr hdr;
	int status = 0;

	hdr = calloc (1, sizeof (*hdr));
	if (hdr == NULL)
		return ENOMEM;

	hdr->owner = NULL;

	status = header_parse (hdr, data, len);

	*phdr = hdr;
	return status;
}


/*
	mimeHdrFree: Free a header object and free its resource.
*/

CCLAPI void		mimeHdrFree(MimeHdr *phdr)
{
	 if (phdr && *phdr)
    {
      header_t header = *phdr;

      /* Can we destroy ?.  */
/*
	modified by txyu 2003/2/24
    if (header->owner == owner)
	{
*/
	  stream_destroy (&(header->stream), header);

	  if (header->hdr)
	    free (header->hdr);

	  if (header->blurb)
	    free (header->blurb);

	  header_free_cache (header);

	  if (header->mstream)
	    stream_destroy (&(header->mstream), NULL);

	  free (header);
/*	} 
*/
      *phdr = NULL;
    }
}



/* FIXME: grossly inneficient, to many copies and reallocating.
   This all header business need a good rewrite.  */
RCODE	mimeHdrSetVal(MimeHdr header, const char *fn, const char *fv)
{
  char *blurb;
  int replace =1; /* always replace the value of fn*/
  size_t len;

  if (header == NULL || fn == NULL)
    return EINVAL;

  /* An fv of NULL means delete the field, but only do it if replace
     was also set to true! */
  if (fv == NULL && !replace)
    return EINVAL;

  /* Overload.  */
  if (header->_set_value)
    return header->_set_value (header, fn, fv, replace);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  /* Easy approach: if replace, overwrite the field-{name,value} and readjust
     the pointers by calling header_parse () this is wastefull, we're just
     fragmenting the memory it can be done better.  But that may imply a
     rewite of the headers ... for another day.  */

  /* If replace, remove all fields in the header blurb that have the
     same name as the field we are writing.

     Algorithm:

     for i = 0, ... i < max_hdrs
     - if ith field has name 'fn' memmove() all following fields up over
     this field
     - reparse the headers
     - restart the for loop at the ith field
   */
  if (replace)
    {
      size_t name_len;
      size_t i;
      size_t fn_len;		/* Field Name len.  */
      size_t fv_len;		/* Field Value len.  */
      len = header->blurb_len;
      /* Find FN in the header fields... */
      for (name_len = strlen (fn), i = 0; i < header->hdr_count; i++)
	{
	  fn_len = header->hdr[i].fn_end - header->hdr[i].fn;
	  fv_len = header->hdr[i].fv_end - header->hdr[i].fv;
	  if (fn_len == name_len &&
	      strncasecmp (header->hdr[i].fn, fn, fn_len) == 0)
	    {
	      blurb = header->blurb;
	      /* ... and if its NOT the last field, move the next field
	         through to last field into its place, */
	      if ((i + 1) < header->hdr_count)
		{
		  memmove (header->hdr[i].fn, header->hdr[i + 1].fn,
			   header->hdr[header->hdr_count - 1].fv_end
			   - header->hdr[i + 1].fn + 3);
		}
	      /* or if it is the last, just truncate the fields. */
	      else
		{
		  header->hdr[i].fn[0] = '\n';
		  header->hdr[i].fn[1] = '\0';
		}
	      /* Readjust the pointers. */
	      /* FIXME: I'm not sure this 3 will work when the
	         original data looked like:
	         Field  :   Value
	         Test this... and why not just do a strlen(blurb)?
	       */
	      len -= fn_len + fv_len + 3;	/* :<sp>\n */
	      i--;
	      blurb = header->blurb;
	      header_parse (header, blurb, len);
	      free (blurb);
	      header->flags |= HEADER_MODIFIED;
	    }
	}
    }

  /* If FV is NULL, then we are done. */
  if (!fv)
    return 0;

  /* Replacing was taken care of above, now write the new header.
     header.  Really not cute.
     COLON SPACE NL =  3 ;  */
  len = strlen (fn) + strlen (fv) + 3;
  /* Add one for the NULL and leak a bit by adding one more
     it will be the separtor \n from the body if the first
     blurb did not have it.  */
  blurb = calloc (header->blurb_len + len + 2, 1);
  if (blurb == NULL)
    return ENOMEM;

  sprintf (blurb, "%s: %s", fn, fv);

  /* Strip off trailing newlines and LWSP. */
  while (blurb[strlen (blurb) - 1] == '\n' ||
	 blurb[strlen (blurb) - 1] == ' ' ||
	 blurb[strlen (blurb) - 1] == '\t')
    {
      blurb[strlen (blurb) - 1] = '\0';
    }
  len = strlen (blurb);
  blurb[len] = '\n';
  len++;

  /* Prepend the rest of the headers.  */
  if (header->blurb)
    {
      memcpy (blurb + len, header->blurb, header->blurb_len);
      free (header->blurb);
      header->blurb = NULL;
    }
  else
    blurb[len] = '\n';

  /* before parsing the new blurb make sure it is properly terminated
     by \n\n. The trailing NL separator.  */
  if (blurb[header->blurb_len + len - 1] != '\n'
      || blurb[header->blurb_len + len - 2] != '\n')
    {
      blurb[header->blurb_len + len] = '\n';
      len++;
    }
  header_parse (header, blurb, len + header->blurb_len);
  free (blurb);
  header->flags |= HEADER_MODIFIED;
  return 0;
}


CCLAPI 
RCODE	mimeHdrGetVal(MimeHdr header, const char *name, char *buffer, 
					  size_t buflen , size_t *pn)
{
  size_t i = 0;
  size_t name_len;
  size_t total = 0, fn_len = 0, fv_len = 0;
  size_t threshold;
  int err = 0;

  if (header == NULL || name == NULL)
    return EINVAL;

  /* First scan our cache headers for hits.  */
  err = header_get_fvalue (header, name, buffer, buflen, pn);
  switch (err)
    {
    case EINVAL: /* Permanent failure.  */
      err = ENOENT;
    case ENOMEM:
      if (pn)
	*pn = 0;
    case 0:
      return err;
    }

  /* Try the provided cache.  */
  if (header->_get_fvalue)
    err = header->_get_fvalue (header, name, buffer, buflen, pn);
  if (err == 0)
    return 0;

  if (header->_get_value)
    {
      char buf[1024]; /* should suffice for field-value. */
      size_t len = 0;
      err = header->_get_value (header, name, buf, sizeof (buf), &len);
      if (err == 0)
	{
	  /* Save in the fast header buffer.  */
	  header_set_fvalue (header, name, buf);
	  if (buffer && buflen > 0)
	    {
	      buflen--;
	      buflen = (len < buflen) ? len : buflen;
	      memcpy (buffer, buf, buflen);
	      buffer[buflen] = '\0';
	    }
	  else
	    buflen = len;
	  if (pn)
	    *pn = buflen;
	}
      else
        {
	  /* Cache permanent failure also.  */
	  header_set_fvalue (header, name, NULL);
        }
      return err;
    }

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  /* We set the threshold to be 1 less for the null.  */
  threshold = --buflen;

  /* Caution: We may have more then one value for a field name, for example
     a "Received" field-name is added by each passing MTA.  The way that the
     parsing (_parse()) is done it's not take to account.  So we just stuff
     in the buffer all the field-values to a corresponding field-name.
     FIXME: Should we kosher the output ? meaning replace occurences of
     " \t\r\n" for spaces ? for now we don't.
   */
  for (name_len = strlen (name), i = 0; i < header->hdr_count; i++)
    {
      fn_len = header->hdr[i].fn_end - header->hdr[i].fn;
      if (fn_len == name_len &&
	  strncasecmp (header->hdr[i].fn, name, fn_len) == 0)
	{
	  fv_len = (header->hdr[i].fv_end - header->hdr[i].fv);
	  /* FIXME:FIXME:PLEASE: hack, add a space/nl separator  */
	  /*
	  if (total && (threshold - 2) > 0)
	    {
	      if (buffer)
		{
		  *buffer++ = '\n';
		  *buffer++ = ' ';
		}
	      threshold -= 2;
	      total += 2;
	    }
          */
	  total += fv_len;
	  /* Can everything fit in the buffer.  */
	  if (buffer && threshold > 0)
	    {
	      buflen = (fv_len < threshold) ? fv_len : threshold;
	      memcpy (buffer, header->hdr[i].fv, buflen);
	      buffer += buflen;
	      threshold -= buflen;
	    }

	  /* Jump out after the first header we found. -sr */
	  break;
	}
    }
  if (buffer)
    *buffer = '\0'; /* Null terminated.  */
  if (pn)
    *pn = total;

  return  (total == 0) ? ENOENT : 0;
}


CCLAPI RCODE	mimeHdrCountFields(MimeHdr header, size_t *pcount)
{
  if (header == NULL)
    {
      if (pcount)
        *pcount = 0;
      return EINVAL;
    }

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  if (pcount)
    *pcount = header->hdr_count;
  return 0;

}

CCLAPI RCODE	mimeHdrGetFieldName(MimeHdr header, size_t index, char *buf, 
									size_t buflen, size_t *nwritten)
{
  size_t len;

  if (header == NULL)
    return EINVAL;

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  if (header->hdr_count == 0 || index > header->hdr_count || index == 0)
    return ENOENT;

  index--;
  len = (header->hdr[index].fn_end - header->hdr[index].fn);
  if (buf && buflen)
    {
      /* save one for the null */
      --buflen;
      len = (len > buflen) ? len : len;
      memcpy (buf, header->hdr[index].fn, len);
      buf[len] = '\0';
    }
  if (nwritten)
    *nwritten = len;
  return 0;
}



CCLAPI RCODE	mimeHdrGetFieldValue(MimeHdr header, size_t index, 
									 char *buf, size_t buflen, size_t *nwritten)
{
  size_t len;

  if (header == NULL || index < 1)
    return EINVAL;

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  if (header->hdr_count == 0 || index > header->hdr_count || index == 0)
    return ENOENT;

  index--;
  len = header->hdr[index].fv_end - header->hdr[index].fv;
  if (buf && buflen > 0)
    {
      /* save one for the null */
      --buflen;
      len = (len > buflen) ? buflen : len;
      memcpy (buf, header->hdr[index].fv, len);
      buf[len] = '\0';
    }

  if (nwritten)
    *nwritten = len;
  return 0;
}




CCLAPI RCODE	mimeHdrGetStrm(MimeHdr header, stream_t *pstream)
{
  if (header == NULL || pstream == NULL)
    return EINVAL;
  if (header->stream == NULL)
    {
      int status = stream_create (&(header->stream), MU_STREAM_RDWR, header);
      if (status != 0)
	return status;
      stream_set_read (header->stream, header_read, header);
      stream_set_readline (header->stream, header_readline, header);
      stream_set_write (header->stream, header_write, header);
    }
  *pstream = header->stream;
  return 0;
}

CCLAPI 
RCODE	mimeHdrSetStrm(MimeHdr header, stream_t stream)
{
  if (header == NULL)
    return EINVAL;
  /*if (header->owner != owner)
    return EACCES;*/
  header->stream = stream;
  return 0;
}


RCODE	mimeHdrLines(MimeHdr header, size_t *plines)
{
  int n;
  size_t lines = 0;
  if (header == NULL || plines == NULL)
    return EINVAL;

  /* Overload.  */
  if (header->_lines)
    return header->_lines (header, plines);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  for (n = header->blurb_len - 1; n >= 0; n--)
    {
      if (header->blurb[n] == '\n')
	lines++;
    }
  if (plines)
    *plines = lines;
  return 0;
}

RCODE	mimeHdrSize(MimeHdr header, size_t *psize)
{
  if (header == NULL)
      return EINVAL;

  /* Overload.  */
  if (header->_size)
    return header->_size (header, psize);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  if (psize)
    *psize = header->blurb_len;
  return 0;
}



/*************************************************************************/

/* For the cache header if the field exist but with no corresponding
   value, it is a permanent failure i.e.  the field does not exist
   in the header return EINVAL to notify header_get_value().  */
static int
header_get_fvalue (header_t header, const char *name, char *buffer,
		  size_t buflen, size_t *pn)
{
  size_t i, fn_len, fv_len = 0;
  size_t name_len;
  int err = ENOENT;

  for (i = 0, name_len = strlen (name); i < header->fhdr_count; i++)
    {
      fn_len = header->fhdr[i].fn_end - header->fhdr[i].fn;
      if (fn_len == name_len
	  && strcasecmp (header->fhdr[i].fn, name) == 0)
	{
	  fv_len = header->fhdr[i].fv_end - header->fhdr[i].fv;

	  /* Permanent failure.  */
	  if (fv_len == 0)
	    {
	      err = EINVAL;
	      break;
	    }

	  if (buffer && buflen > 0)
	    {
	      buflen--;
	      fv_len = (fv_len < buflen) ? fv_len : buflen;
	      memcpy (buffer, header->fhdr[i].fv, fv_len);
	      buffer[fv_len] = '\0';
	    }
	  err = 0;
	  break;
	}
    }
  if (pn)
    *pn = fv_len;
  return err;

}


/* We try to cache the headers here to reduce networking access
   especially for IMAP.  When the buffer is NULL it means that
   the field does not exist on the server and we should not
   attempt to contact the server again for this field.  */
static int
header_set_fvalue (header_t header, const char *name, char *buffer)
{
  struct _hdr *thdr;
  thdr = realloc (header->fhdr, (header->fhdr_count + 1) * sizeof(*thdr));
  if (thdr)
    {
      size_t len = strlen (name);
      char *field = malloc (len + 1);
      if (field == NULL)
	return ENOMEM;
      memcpy (field, name, len);
      field[len] = '\0';
      thdr[header->fhdr_count].fn = field;
      thdr[header->fhdr_count].fn_end = field + len;

      if (buffer)
	{
          len = strlen (buffer);
          field =  malloc (len + 1);
          if (field == NULL)
	    return ENOMEM;
	  memcpy (field, buffer, len);
	  field[len] = '\0';
	  thdr[header->fhdr_count].fv = field;
	  thdr[header->fhdr_count].fv_end = field + len;
	}
      else
	{
	  thdr[header->fhdr_count].fv = NULL;
	  thdr[header->fhdr_count].fv_end = NULL;
	}
      header->fhdr_count++;
      header->fhdr = thdr;
      return 0;
    }
  return ENOMEM;
}

static void
header_free_cache (header_t header)
{
  /* Clean up our fast header cache.  */
  if (header->fhdr)
    {
      size_t i;
      for (i = 0; i < header->fhdr_count; i++)
	{
	  if (header->fhdr[i].fn)
	    free (header->fhdr[i].fn);
	  if (header->fhdr[i].fv)
	    free (header->fhdr[i].fv);
	}
      free (header->fhdr);
      header->fhdr = NULL;
      header->fhdr_count = 0;
    }
}



static int
fill_blurb (header_t header)
{
  int status;
  char buf[1024];
  size_t nread;

  if (header->_fill == NULL)
    return 0;

  /* The entire header is now ours(part of header_t), clear all the
     overloading.  */
  header_free_cache (header);
  header->_get_fvalue = NULL;
  header->_get_value = NULL;
  header->_set_value = NULL;
  header->_size = NULL;
  header->_lines = NULL;

  if (header->mstream == NULL)
    {
      status = memory_stream_create (&header->mstream, NULL, MU_STREAM_RDWR);
      if (status != 0)
	return status;
      stream_open (header->mstream);
      header->stream_len = 0;
    }

  /* Bring in the entire header.  */
  do
    {
      nread = 0;
      status = header->_fill (header, buf, sizeof buf,
			      header->stream_len, &nread) ;
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      stream_destroy (&(header->mstream), NULL);
	      header->stream_len = 0;
	    }
	  return status;
	}
      if (nread > 0)
	{
	  status = stream_write (header->mstream, buf, nread, header->stream_len, NULL);
	  if (status != 0)
	    {
	      stream_destroy (&(header->mstream), NULL);
	      header->stream_len = 0;
	      return status;
	    }
	  header->stream_len += nread;
	}
    }
  while (nread > 0);

  /* parse it. */
  {
    char *blurb;
    size_t len = header->stream_len;
    blurb = calloc (1, len + 1);
    if (blurb)
      {
	stream_read (header->mstream, blurb, len, 0, &len);
	status = header_parse (header, blurb, len);
      }
    free (blurb);
  }
  stream_destroy (&header->mstream, NULL);
  header->stream_len = 0;
  return status;
}


/* 
   2003/2/24 txyu
   The function, header_pase, is copied from GNU header_parse() without 
   any modification 
*/
/* Parsing is done in a rather simple fashion, meaning we just consider an
   entry to be a field-name an a field-value.  So they maybe duplicate of
   field-name like "Received" they are just put in the array, see _get_value()
   on how to handle the case. in the case of error .i.e a bad header construct
   we do a full stop and return what we have so far.  */
static int
header_parse (header_t header, const char *blurb, int len)
{
  char *header_end;
  char *header_start;
  char *header_start2;
  struct _hdr *hdr;

  /* Nothing to parse.  */
  if (blurb == NULL)
    return 0;

  header->blurb_len = len;
  /* Why "+ 1", if for a terminating NULL, where is written? */
  header->blurb = calloc (1, header->blurb_len + 1);
  if (header->blurb == NULL)
    return ENOMEM;
  memcpy (header->blurb, blurb, header->blurb_len);

  if (header->hdr)
    free (header->hdr);
  header->hdr = NULL;
  header->hdr_count = 0;

  /* Get a header, a header is:
     field-name LWSP ':'
       LWSP field-value '\r' '\n'
       *[ (' ' | '\t') field-value '\r' '\n' ]
  */
  /* First loop goes through the blurb */
  for (header_start = header->blurb;  ; header_start = ++header_end)
    {
      char *fn, *fn_end, *fv, *fv_end;

      if (header_start[0] == ' '
	  || header_start[0] == '\t'
	  || header_start[0] == '\n')
	break;

      /* Second loop extract one header field. */
      for (header_start2 = header_start;  ;header_start2 = ++header_end)
	{
	  header_end = memchr (header_start2, '\n', len);
	  if (header_end == NULL)
	    break;
	  else
	    {
	      len -= (header_end - header_start2 + 1);
	      if (len < 0)
		{
		  header_end = NULL;
		  break;
		}
	      if (header_end[1] != ' '
		  && header_end[1] != '\t')
		break; /* New header break the inner for. */
	    }
	  /* *header_end = ' ';  smash LF ? NO */
	}

      if (header_end == NULL)
	break; /* Bail out.  */

      /* Now save the header in the data structure.  */

      /* Treats unix "From " specially.  */
      if ((header_end - header_start >= 5)
      	  && strncmp (header_start, "From ", 5) == 0)
	{
	  fn = header_start;
	  fn_end = header_start + 5;
	  fv = header_start + 5;
	  fv_end = header_end;
	}
      else /* Break the header in key: value */
	{
	  char *colon = memchr (header_start, ':', header_end - header_start);

#define ISLWSP(c) (((c) == ' ' || (c) == '\t'))
	  /* Houston we have a problem.  */
	  if (colon == NULL)
	    break; /* Disregard the rest and bailout.  */

	  fn = header_start;
	  fn_end = colon;
	  /* Shrink any LWSP after the field name -- CRITICAL for 
	   later name comparisons to work correctly! */
	  while(ISLWSP(fn_end[-1]))
	    fn_end--;

	  fv = colon + 1;
	  fv_end = header_end;

	  /* Skip any LWSP before the field value -- unnecessary, but
	   might make some field values look a little tidier. */
	  while(ISLWSP(fv[0]))
	    fv++;
	}
#undef ISLWSP
      /* Allocate a new slot for the field:value.  */
      hdr = realloc (header->hdr, (header->hdr_count + 1) * sizeof (*hdr));
      if (hdr == NULL)
	{
	  free (header->blurb);
	  free (header->hdr);
	  header->blurb = NULL;
	  header->hdr = NULL;
	  return ENOMEM;
	}
      hdr[header->hdr_count].fn = fn;
      hdr[header->hdr_count].fn_end = fn_end;
      hdr[header->hdr_count].fv = fv;
      hdr[header->hdr_count].fv_end = fv_end;
      header->hdr = hdr;
      header->hdr_count++;
    } /* for (header_start ...) */

 return 0;
}

static int
header_write (stream_t os, const char *buf, size_t buflen,
	      off_t off, size_t *pnwrite)
{
  header_t header = stream_get_owner (os);
  int status;

  if (header == NULL)
    return EINVAL;

  if ((size_t)off != header->stream_len)
    return ESPIPE;

  /* Skip the obvious.  */
  if (buf == NULL || *buf == '\0' || buflen == 0)
    {
      if (pnwrite)
        *pnwrite = 0;
      return 0;
    }

  if (header->mstream == NULL)
    {
      status = memory_stream_create (&header->mstream, NULL, MU_STREAM_RDWR);
      if (status != 0)
	return status;
      status = stream_open (header->mstream);
      if (status != 0)
      {
	stream_destroy(&header->mstream, NULL);
	return status;
      }
      header->stream_len = 0;
    }

  status = stream_write (header->mstream, buf, buflen, header->stream_len, &buflen);
  if (status != 0)
    {
      stream_destroy (&header->mstream, NULL);
      header->stream_len = 0;
      return status;
    }
  header->stream_len += buflen;

  /* We detect an empty line .i.e "^\n$" this signal the end of the
     header.  */
  if (header->stream_len)
    {
      int finish = 0;
      char nlnl[2];
      nlnl[1] = nlnl[0] = '\0';
      stream_read (header->mstream, nlnl, 1, 0, NULL);
      if (nlnl[0] == '\n')
	{
	  finish = 1;
	}
      else
	{
	  stream_read (header->mstream, nlnl, 2, header->stream_len - 2, NULL);
	  if (nlnl[0] == '\n' && nlnl[1] == '\n')
	    {
	      finish = 1;
	    }
	}
      if (finish)
	{
	  char *blurb;
	  size_t len = header->stream_len;
	  blurb = calloc (1, len + 1);
	  if (blurb)
	    {
	      stream_read (header->mstream, blurb, len, 0, &len);
	      status = header_parse (header, blurb, len);
	    }
	  free (blurb);
	  stream_destroy (&header->mstream, NULL);
	  header->stream_len = 0;
	}
  }

  if (pnwrite)
    *pnwrite = buflen;
  return 0;

}

static int
header_read (stream_t is, char *buf, size_t buflen, off_t off, size_t *pnread)
{
  int len;
  header_t header = stream_get_owner (is);

  if (is == NULL || header == NULL)
    return EINVAL;

  if (buf == NULL || buflen == 0)
    {
      if (pnread)
	*pnread = 0;
      return 0;
    }

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  len = header->blurb_len - off;
  if (len > 0)
    {
      len = (buflen < (size_t)len) ? buflen : (size_t)len;
      memcpy (buf, header->blurb + off, len);
    }
  else
    len = 0;

  if (pnread)
    *pnread = len;
  return 0;
}

static int
header_readline (stream_t is, char *buf, size_t buflen, off_t off, size_t *pn)
{
  int len;
  header_t header = stream_get_owner (is);

  if (is == NULL || header == NULL)
    return EINVAL;

  if (buf == NULL || buflen == 0)
    {
      if (pn)
	*pn = 0;
      return 0;
    }

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  buflen--; /* Space for the null.  */

  len = header->blurb_len - off;
  if (len > 0)
    {
      char *nl = memchr (header->blurb + off, '\n', len);
      if (nl)
	len = nl - (header->blurb + off) + 1;
      len = (buflen < (size_t)len) ? buflen : (size_t)len;
      memcpy (buf, header->blurb + off, len);
    }
  else
    len = 0;
  if (pn)
    *pn = len;
  buf[len] = '\0';
  return 0;
}

