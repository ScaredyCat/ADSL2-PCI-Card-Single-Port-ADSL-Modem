/*
 * "$Id: mxml-private.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $"
 *
 * Private functions for mini-XML, a small XML-like file parsing library.
 *
 * Copyright 2003 by Michael Sweet.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contents:
 *
 *   mxml_error()      - Display an error message.
 *   mxml_integer_cb() - Default callback for integer values.
 *   mxml_opaque_cb()  - Default callback for opaque values.
 *   mxml_real_cb()    - Default callback for real number values.
 */

/*
 * Include necessary headers...
 */

#include "config.h"
#include "mxml.h"

/*
 * Error callback function...
 */

void	(*mxml_error_cb)(const char *) = NULL;


/*
 * 'mxml_error()' - Display an error message.
 */

void
mxml_error(const char *format,		/* I - Printf-style format string */
           ...)				/* I - Additional arguments as needed */
{
  va_list	ap;			/* Pointer to arguments */
  char		*s;			/* Message string */


 /*
  * Range check input...
  */

  if (!format)
    return;

 /*
  * Format the error message string...
  */

  va_start(ap, format);

  s = mxml_strdupf(format, ap);

  va_end(ap);

 /*
  * And then display the error message...
  */

  if (mxml_error_cb)
    (*mxml_error_cb)(s);
  else
  {
//    fputs("mxml: ", stderr);
//    fputs(s, stderr);
//    putc('\n', stderr);

    /***** Add by Frank 11/17/2005 *****/
    printf("mxml: ");
    printf("%s", s);
    printf("\n");
    /***** End Frank 11/17/2005 *****/
  }

 /*
  * Free the string...
  */
#ifdef LEADFLY
  npfree(s);
#else
  free(s);
#endif
}


/*
 * 'mxml_integer_cb()' - Default callback for integer values.
 */

mxml_type_t				/* O - Node type */
mxml_integer_cb(mxml_node_t *node)	/* I - Current node */
{
  (void)node;

  return (MXML_INTEGER);
}


/*
 * 'mxml_opaque_cb()' - Default callback for opaque values.
 */

mxml_type_t				/* O - Node type */
mxml_opaque_cb(mxml_node_t *node)	/* I - Current node */
{
  (void)node;

  return (MXML_OPAQUE);
}


/*
 * 'mxml_real_cb()' - Default callback for real number values.
 */
 
/***** Add "//" by Frank 11/16/2005 *****/
//mxml_type_t				/* O - Node type */
//mxml_real_cb(mxml_node_t *node)		/* I - Current node */
//{
//  (void)node;

//  return (MXML_REAL);
//}
/***** End Frank 11/16/2005 *****/

/*
 * End of "$Id: mxml-private.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $".
 */
