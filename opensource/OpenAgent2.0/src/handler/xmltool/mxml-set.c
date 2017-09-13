/*
 * "$Id: mxml-set.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $"
 *
 * Node set functions for mini-XML, a small XML-like file parsing library.
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
 *   mxmlSetElement() - Set the name of an element node.
 *   mxmlSetInteger() - Set the value of an integer node.
 *   mxmlSetOpaque()  - Set the value of an opaque node.
 *   mxmlSetReal()    - Set the value of a real number node.
 *   mxmlSetText()    - Set the value of a text node.
 *   mxmlSetTextf()   - Set the value of a text node to a formatted string.
 */

/*
 * Include necessary headers...
 */

#include "config.h"
#include "mxml.h"


/*
 * 'mxmlSetElement()' - Set the name of an element node.
 *
 * The node is not changed if it is not an element node.
 */

int					/* O - 0 on success, -1 on failure */
mxmlSetElement(mxml_node_t *node,	/* I - Node to set */
               const char  *name)	/* I - New name string */
{
 /*
  * Range check input...
  */

  if (!node || node->type != MXML_ELEMENT || !name)
    return (-1);

 /*
  * Free any old element value and set the new value...
  */

  if (node->value.element.name){
    #ifdef LEADFLY
      npfree(node->value.element.name);
    #else
      free(node->value.element.name);
    #endif
  }
  node->value.element.name = strdup(name);
  
  return (0);
}


/*
 * 'mxmlSetInteger()' - Set the value of an integer node.
 *
 * The node is not changed if it is not an integer node.
 */

int					/* O - 0 on success, -1 on failure */
mxmlSetInteger(mxml_node_t *node,	/* I - Node to set */
               int         integer)	/* I - Integer value */
{
 /*
  * Range check input...
  */

  if (!node || node->type != MXML_INTEGER)
    return (-1);

 /*
  * Set the new value and return...
  */

  node->value.integer = integer;

  return (0);
}


/*
 * 'mxmlSetOpaque()' - Set the value of an opaque node.
 *
 * The node is not changed if it is not an opaque node.
 */

int					/* O - 0 on success, -1 on failure */
mxmlSetOpaque(mxml_node_t *node,	/* I - Node to set */
              const char  *opaque)	/* I - Opaque string */
{
 /*
  * Range check input...
  */

  if (!node || node->type != MXML_OPAQUE || !opaque)
    return (-1);

 /*
  * Free any old opaque value and set the new value...
  */

  if (node->value.opaque){
    #ifdef LEADFLY
      npfree(node->value.opaque);
    #else
      free(node->value.opaque);
    #endif
  }
  node->value.opaque = strdup(opaque);
  
  return (0);
}


/*
 * 'mxmlSetReal()' - Set the value of a real number node.
 *
 * The node is not changed if it is not a real number node.
 */

//int					/* O - 0 on success, -1 on failure */
//mxmlSetReal(mxml_node_t *node,		/* I - Node to set */
//            double      real)		/* I - Real number value */
//{
 /*
  * Range check input...
  */

//  if (!node || node->type != MXML_REAL)
//    return (-1);

 /*
  * Set the new value and return...
  */

//  node->value.real = real;

//  return (0);
//}


/*
 * 'mxmlSetText()' - Set the value of a text node.
 *
 * The node is not changed if it is not a text node.
 */

int					/* O - 0 on success, -1 on failure */
mxmlSetText(mxml_node_t *node,		/* I - Node to set */
            int         whitespace,	/* I - 1 = leading whitespace, 0 = no whitespace */
	    const char  *string)	/* I - String */
{
 /*
  * Range check input...
  */

  if (!node || node->type != MXML_TEXT || !string)
    return (-1);

 /*
  * Free any old string value and set the new value...
  */

  if (node->value.text.string){
    #ifdef LEADFLY
      npfree(node->value.text.string);

    /***** Add by Frank 11/17/2005 *****/
    #else
      free(node->value.text.string);
    #endif
    /***** End Frank 11/17/2005 *****/
  }
  node->value.text.whitespace = whitespace;
  
  node->value.text.string     = strdup(string);

  return (0);
}


/*
 * 'mxmlSetTextf()' - Set the value of a text node to a formatted string.
 *
 * The node is not changed if it is not a text node.
 */

int					/* O - 0 on success, -1 on failure */
mxmlSetTextf(mxml_node_t *node,		/* I - Node to set */
             int         whitespace,	/* I - 1 = leading whitespace, 0 = no whitespace */
             const char  *format,	/* I - Printf-style format string */
	     ...)			/* I - Additional arguments as needed */
{
  va_list	ap;			/* Pointer to arguments */


 /*
  * Range check input...
  */

  if (!node || node->type != MXML_TEXT || !format)
    return (-1);

 /*
  * Free any old string value and set the new value...
  */

  if (node->value.text.string){
    #ifdef LEADFLY
      npfree(node->value.text.string);

    /***** Add by Frank 11/17/2005 *****/
    #else
      free(node->value.text.string);
    #endif
    /***** End Frank 11/16/2005 *****/
  }
  va_start(ap, format);

  node->value.text.whitespace = whitespace;
  node->value.text.string     = mxml_strdupf(format, ap);

  va_end(ap);

  return (0);
}


/*
 * End of "$Id: mxml-set.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $".
 */
