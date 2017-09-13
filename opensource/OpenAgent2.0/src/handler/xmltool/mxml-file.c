/*
 * "$Id: mxml-file.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $"
 *
 * File loading code for mini-XML, a small XML-like file parsing library.
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
 *   mxmlLoadFile()         - Load a file into an XML node tree.
 *   mxmlLoadString()       - Load a string into an XML node tree.
 *   mxmlSaveAllocString()  - Save an XML node tree to an allocated string.
 *   mxmlSaveFile()         - Save an XML tree to a file.
 *   mxmlSaveString()       - Save an XML node tree to a string.
 *   mxmlSetErrorCallback() - Set the error message callback.
 *   mxml_add_char()        - Add a character to a buffer, expanding as needed.
 *   mxml_get_entity()      - Get the character corresponding to an entity...
 *   mxml_file_getc()       - Get a character from a file.
 *   mxml_file_putc()       - Write a character to a file.
 *   mxml_load_data()       - Load data into an XML node tree.
 *   mxml_parse_element()   - Parse an element for any attributes...
 *   mxml_string_getc()     - Get a character from a string.
 *   mxml_write_name()      - Write a name string.
 *   mxml_write_node()      - Save an XML node to a file.
 *   mxml_write_string()    - Write a string, escaping & and < as needed.
 *   mxml_write_ws()        - Do whitespace callback...
 */

/*
 * Include necessary headers...
 */

#include "config.h"
#include "mxml.h"

/*
 * Global error handler...
 */

extern void	(*mxml_error_cb)(const char *);


/*
 * Local functions...
 */

static int		mxml_add_char(int ch, char **ptr, char **buffer,
			              int *bufsize);
static int		mxml_get_entity(mxml_node_t *parent, void *p,
					int (*getc_cb)(void *));
/***** Add "//" by Frank 11/16/2005 *****/	
//static int		mxml_file_getc(void *p);
//static int		mxml_file_putc(int ch, void *p);
/***** Add "//" by Frank 11/16/2005 *****/	
static mxml_node_t	*mxml_load_data(mxml_node_t *top, void *p,
			                mxml_type_t (*cb)(mxml_node_t *),
			                int (*getc_cb)(void *));
static int		mxml_parse_element(mxml_node_t *node, void *p,
			                   int (*getc_cb)(void *));
static int		mxml_string_getc(void *p);
static int		mxml_string_putc(int ch, void *p);
static int		mxml_write_name(const char *s, void *p,
					int (*putc_cb)(int, void *));
static int		mxml_write_node(mxml_node_t *node, void *p,
			                int (*cb)(mxml_node_t *, int),
					int col,
					int (*putc_cb)(int, void *));
static int		mxml_write_string(const char *s, void *p,
					  int (*putc_cb)(int, void *));
static int		mxml_write_ws(mxml_node_t *node, void *p, 
			              int (*cb)(mxml_node_t *, int), int ws,
				      int col, int (*putc_cb)(int, void *));


/*
 * 'mxmlLoadFile()' - Load a file into an XML node tree.
 *
 * The nodes in the specified file are added to the specified top node.
 * If no top node is provided, the XML file MUST be well-formed with a
 * single parent node like <?xml> for the entire file. The callback
 * function returns the value type that should be used for child nodes.
 * If MXML_NO_CALLBACK is specified then all child nodes will be either
 * MXML_ELEMENT or MXML_TEXT nodes.
 *
 * The constants MXML_INTEGER_CALLBACK, MXML_OPAQUE_CALLBACK,
 * MXML_REAL_CALLBACK, and MXML_TEXT_CALLBACK are defined for loading
 * child nodes of the specified type.
 */

//mxml_node_t *				/* O - First node or NULL if the file could not be read. */
//mxmlLoadFile(mxml_node_t *top,		/* I - Top node */
//             FILE        *fp,		/* I - File to read from */
//             mxml_type_t (*cb)(mxml_node_t *node))
					/* I - Callback function or MXML_NO_CALLBACK */
//{
//  return (mxml_load_data(top, fp, cb, mxml_file_getc));
//}


/*
 * 'mxmlLoadString()' - Load a string into an XML node tree.
 *
 * The nodes in the specified string are added to the specified top node.
 * If no top node is provided, the XML string MUST be well-formed with a
 * single parent node like <?xml> for the entire string. The callback
 * function returns the value type that should be used for child nodes.
 * If MXML_NO_CALLBACK is specified then all child nodes will be either
 * MXML_ELEMENT or MXML_TEXT nodes.
 *
 * The constants MXML_INTEGER_CALLBACK, MXML_OPAQUE_CALLBACK,
 * MXML_REAL_CALLBACK, and MXML_TEXT_CALLBACK are defined for loading
 * child nodes of the specified type.
 */

mxml_node_t *				/* O - First node or NULL if the string has errors. */
mxmlLoadString(mxml_node_t *top,	/* I - Top node */
               const char  *s,		/* I - String to load */
               mxml_type_t (*cb)(mxml_node_t *node))
					/* I - Callback function or MXML_NO_CALLBACK */
{
  return (mxml_load_data(top, &s, cb, mxml_string_getc));
}


/*
 * 'mxmlSaveAllocString()' - Save an XML node tree to an allocated string.
 *
 * This function returns a pointer to a string containing the textual
 * representation of the XML node tree.  The string should be freed
 * using the free() function when you are done with it.  NULL is returned
 * if the node would produce an empty string or if the string cannot be
 * allocated.
 */

char *					/* O - Allocated string or NULL */
mxmlSaveAllocString(mxml_node_t *node,	/* I - Node to write */
                    int         (*cb)(mxml_node_t *node, int ws))
					/* I - Whitespace callback or MXML_NO_CALLBACK */
{
  int	bytes;				/* Required bytes */
  char	buffer[8192];			/* Temporary buffer */
  char	*s;				/* Allocated string */


 /*
  * Write the node to the temporary buffer...
  */

  bytes = mxmlSaveString(node, buffer, sizeof(buffer), cb);

  if (bytes <= 0)
    return (NULL);

  if (bytes < (int)(sizeof(buffer) - 1))
  {
   /*
    * Node fit inside the buffer, so just duplicate that string and
    * return...
    */
    return (strdup(buffer));
    
  }

 /*
  * Allocate a buffer of the required size and save the node to the
  * new buffer...
  */
  /****** Add by Frank 11/15/2005  *****/
  #ifdef LEADFLY
    if ((s = npalloc(bytes +1)) == NULL)
  
  #else
  /****** End Frank 11/15/2005  *****/
    if ((s = malloc(bytes + 1)) == NULL)
  /****** Add by Frank 11/15/2005  *****/    
  #endif
  /****** End Frank 11/15/2005  *****/
    return (NULL);
  mxmlSaveString(node, s, bytes + 1, cb);

 /*
  * Return the allocated string...
  */

  return (s);
}


/*
 * 'mxmlSaveFile()' - Save an XML tree to a file.
 *
 * The callback argument specifies a function that returns a whitespace
 * character or nul (0) before and after each element. If MXML_NO_CALLBACK
 * is specified, whitespace will only be added before MXML_TEXT nodes
 * with leading whitespace and before attribute names inside opening
 * element tags.
 */

//int					/* O - 0 on success, -1 on error. */
//mxmlSaveFile(mxml_node_t *node,		/* I - Node to write */
//             FILE        *fp,		/* I - File to write to */
//	     int         (*cb)(mxml_node_t *node, int ws))
					/* I - Whitespace callback or MXML_NO_CALLBACK */
//{
//  int	col;				/* Final column */


 /*
  * Write the node...
  */

//  if ((col = mxml_write_node(node, fp, cb, 0, mxml_file_putc)) < 0)
//    return (-1);

//  if (col > 0)
//    if (putc('\n', fp) < 0)
//      return (-1);

 /*
  * Return 0 (success)...
  */

//  return (0);
//}


/*
 * 'mxmlSaveString()' - Save an XML node tree to a string.
 *
 * This function returns the total number of bytes that would be
 * required for the string but only copies (bufsize - 1) characters
 * into the specified buffer.
 */

int					/* O - Size of string */
mxmlSaveString(mxml_node_t *node,	/* I - Node to write */
               char        *buffer,	/* I - String buffer */
               int         bufsize,	/* I - Size of string buffer */
               int         (*cb)(mxml_node_t *node, int ws))
					/* I - Whitespace callback or MXML_NO_CALLBACK */
{
  int	col;				/* Final column */
  char	*ptr[2];			/* Pointers for putc_cb */


 /*
  * Write the node...
  */

  ptr[0] = buffer;
  ptr[1] = buffer + bufsize;

  if ((col = mxml_write_node(node, ptr, cb, 0, mxml_string_putc)) < 0)
    return (-1);

  if (col > 0)
    mxml_string_putc('\n', ptr);

 /*
  * Nul-terminate the buffer...
  */

  if (ptr[0] >= ptr[1]) {
    buffer[bufsize - 1] = '\0';
    return (-2);
  }
  else
    ptr[0][0] = '\0';

 /*
  * Return the number of characters...
  */

  return (ptr[0] - buffer);
}


/*
 * 'mxmlSetErrorCallback()' - Set the error message callback.
 */

void
mxmlSetErrorCallback(void (*cb)(const char *))
					/* I - Error callback function */
{
  mxml_error_cb = cb;
}


/*
 * 'mxml_add_char()' - Add a character to a buffer, expanding as needed.
 */

static int				/* O  - 0 on success, -1 on error */
mxml_add_char(int  ch,			/* I  - Character to add */
              char **bufptr,		/* IO - Current position in buffer */
	      char **buffer,		/* IO - Current buffer */
	      int  *bufsize)		/* IO - Current buffer size */
{
  char	*newbuffer;			/* New buffer value */

  if (*bufptr >= (*buffer + *bufsize - 4))
  {
   /*
    * Increase the size of the buffer...
    */

    if (*bufsize < 1024)    
      (*bufsize) *= 2;
    else
      (*bufsize) += 1024; 
      
    /********** Add by Frank 11/22/2005 **********/       
    #ifdef  LEADFLY
      if((newbuffer =  npalloc(*bufsize)) == NULL)             
      {      
        npfree(*buffer);     
            
        mxml_error("Unable to expand string buffer to %d bytes!", *bufsize);

        return (-1);
      }
       
      memcpy(newbuffer, *buffer, (strlen(*buffer)+1)); 
                          
      *bufptr = newbuffer + (*bufptr - *buffer);      
      
      npfree(*buffer);
         
      *buffer = newbuffer;
    /********** End Frank 11/22/2005 **********/    
    /********** Add by Frank 11/22/2005 **********/
    #else 
    /********** End Frank 11/22/2005  **********/    
      if ((newbuffer = realloc(*buffer, *bufsize)) == NULL)
      {       
        free(*buffer);
        
        mxml_error("Unable to expand string buffer to %d bytes!", *bufsize);

        return (-1);
      }

      *bufptr = newbuffer + (*bufptr - *buffer);
      *buffer = newbuffer;
    /********** Add by Frank 11/22/2005 **********/ 
    #endif
    /********** End Frank 11/22/2005  **********/
  }  
       
  if (ch < 128)
  {
   /*
    * Single byte ASCII...
    */

    *(*bufptr)++ = ch;
  }
  else if (ch < 2048)
  {
   /*
    * Two-byte UTF-8...
    */

    *(*bufptr)++ = 0xc0 | (ch >> 6);
    *(*bufptr)++ = 0x80 | (ch & 0x3f);
  }
  else if (ch < 65536)
  {
   /*
    * Three-byte UTF-8...
    */

    *(*bufptr)++ = 0xe0 | (ch >> 12);
    *(*bufptr)++ = 0x80 | ((ch >> 6) & 0x3f);
    *(*bufptr)++ = 0x80 | (ch & 0x3f);
  }
  else
  {
   /*
    * Four-byte UTF-8...
    */

    *(*bufptr)++ = 0xf0 | (ch >> 18);
    *(*bufptr)++ = 0x80 | ((ch >> 12) & 0x3f);
    *(*bufptr)++ = 0x80 | ((ch >> 6) & 0x3f);
    *(*bufptr)++ = 0x80 | (ch & 0x3f);
  }

  return (0);
}


/*
 * 'mxml_get_entity()' - Get the character corresponding to an entity...
 */

static int				/* O - Character value or EOF on error */
mxml_get_entity(mxml_node_t *parent,	/* I - Parent node */
		void        *p,		/* I - Pointer to source */
                int         (*getc_cb)(void *))
					/* I - Get character function */
{
  int	ch;				/* Current character */
  char	entity[64],			/* Entity string */
	*entptr;			/* Pointer into entity */


  entptr = entity;

  while ((ch = (*getc_cb)(p)) != EOF)
    if (!isalnum(ch) && ch != '#')
      break;
    else if (entptr < (entity + sizeof(entity) - 1))
      *entptr++ = ch;
    else
    {
      mxml_error("Entity name too long under parent <%s>!",
	         parent ? parent->value.element.name : "null");
      break;
    }

  *entptr = '\0';

  if (ch != ';')
  {
    mxml_error("Character entity \"%s\" not terminated under parent <%s>!",
	       entity, parent ? parent->value.element.name : "null");
    return (EOF);
  }

  if (entity[1] == '#')
  {
    if (entity[2] == 'x')
      ch = strtol(entity + 3, NULL, 16);
    else
      ch = strtol(entity + 2, NULL, 10);
  }
  else if ((ch = mxmlEntityGetValue(entity)) < 0)
    mxml_error("Entity name \"%s;\" not supported under parent <%s>!",
	       entity, parent ? parent->value.element.name : "null");

  return (ch);
}


/*
 * 'mxml_file_getc()' - Get a character from a file.
 */

//static int				/* O - Character or EOF */
//mxml_file_getc(void *p)			/* I - Pointer to file */
//{
//  int	ch,				/* Character from file */
//	temp;				/* Temporary character */
//  FILE	*fp;				/* Pointer to file */


 /*
  * Read a character from the file and see if it is EOF or ASCII...
  */

//  fp = (FILE *)p;
//  ch = getc(fp);

//  if (ch == EOF || !(ch & 0x80))
//    return (ch);

 /*
  * Got a UTF-8 character; convert UTF-8 to Unicode and return...
  */

//  if ((ch & 0xe0) == 0xc0)
//  {
   /*
    * Two-byte value...
    */

//    if ((temp = getc(fp)) == EOF || (temp & 0xc0) != 0x80)
//      return (EOF);

//    ch = ((ch & 0x1f) << 6) | (temp & 0x3f);
//  }
//  else if ((ch & 0xf0) == 0xe0)
//  {
   /*
    * Three-byte value...
    */

//    if ((temp = getc(fp)) == EOF || (temp & 0xc0) != 0x80)
//      return (EOF);

//    ch = ((ch & 0x0f) << 6) | (temp & 0x3f);

//    if ((temp = getc(fp)) == EOF || (temp & 0xc0) != 0x80)
//      return (EOF);

//    ch = (ch << 6) | (temp & 0x3f);
//  }
//  else if ((ch & 0xf8) == 0xf0)
//  {
   /*
    * Four-byte value...
    */

//    if ((temp = getc(fp)) == EOF || (temp & 0xc0) != 0x80)
//      return (EOF);

//    ch = ((ch & 0x07) << 6) | (temp & 0x3f);

//    if ((temp = getc(fp)) == EOF || (temp & 0xc0) != 0x80)
//      return (EOF);

//    ch = (ch << 6) | (temp & 0x3f);

//    if ((temp = getc(fp)) == EOF || (temp & 0xc0) != 0x80)
//      return (EOF);

//    ch = (ch << 6) | (temp & 0x3f);
//  }
//  else
//    return (EOF);

//  return (ch);
//}


/*
 * 'mxml_file_putc()' - Write a character to a file.
 */

//static int				/* O - 0 on success, -1 on failure */
//mxml_file_putc(int  ch,			/* I - Character to write */
//               void *p)			/* I - Pointer to file */
//{
//  if (ch < 128)
//    return (putc(ch, (FILE *)p));
//  else if (ch < 2048)
//  {
   /*
    * Two-byte UTF-8 character...
    */

//    if (putc(0xc0 | (ch >> 6), (FILE *)p) < 0)
//      return (-1);
//    else
//      return (putc(0x80 | (ch & 0x3f), (FILE *)p));
//  }
//  else if (ch < 65536)
//  {
   /*
    * Three-byte UTF-8 character...
    */

//    if (putc(0xe0 | (ch >> 12), (FILE *)p) < 0)
//      return (-1);
//    else if (putc(0x80 | ((ch >> 6) & 0x3f), (FILE *)p) < 0)
//      return (-1);
//    else
//      return (putc(0x80 | (ch & 0x3f), (FILE *)p));
//  }
//  else
//  {
   /*
    * Four-byte UTF-8 character...
    */

//    if (putc(0xf0 | (ch >> 18), (FILE *)p) < 0)
//      return (-1);
//    else if (putc(0x80 | ((ch >> 12) & 0x3f), (FILE *)p) < 0)
//      return (-1);
//    else if (putc(0x80 | ((ch >> 6) & 0x3f), (FILE *)p) < 0)
//      return (-1);
//    else
//      return (putc(0x80 | (ch & 0x3f), (FILE *)p));
//  }
//}


/*
 * 'mxml_load_data()' - Load data into an XML node tree.
 */

static mxml_node_t *			/* O - First node or NULL if the file could not be read. */
mxml_load_data(mxml_node_t *top,	/* I - Top node */
               void        *p,		/* I - Pointer to data */
               mxml_type_t (*cb)(mxml_node_t *),
					/* I - Callback function or MXML_NO_CALLBACK */
               int         (*getc_cb)(void *))
					/* I - Read function */
{
  mxml_node_t	*node,			/* Current node */
		*parent;		/* Current parent node */
  int		ch,			/* Character from file */
		whitespace;		/* Non-zero if whitespace seen */
  char		*buffer,		/* String buffer */
		*bufptr;		/* Pointer into buffer */
  int		bufsize;		/* Size of buffer */
  mxml_type_t	type;			/* Current node type */


 /*
  * Read elements and other nodes from the file...
  */
  #ifdef LEADFLY
    if ((buffer = npalloc(64)) == NULL)
  #else
    if ((buffer = malloc(64)) == NULL)
  #endif
    {
      mxml_error("Unable to allocate string buffer!");
      return (NULL);
    }

  bufsize    = 64;
  bufptr     = buffer;
  parent     = top;
  whitespace = 0;

  if (cb && parent)
    type = (*cb)(parent);
  else
    type = MXML_TEXT;

  while ((ch = (*getc_cb)(p)) != EOF)
  {
    //add by andy
    if (ch == '\n' || ch == '\r'){
        continue;
    }
    
    if ((ch == '<' || (isspace(ch) && type != MXML_OPAQUE)) && bufptr > buffer)
    {
     /*
      * Add a new value node...
      */

      *bufptr = '\0';

      switch (type)
      {
	case MXML_INTEGER :
            node = mxmlNewInteger(parent, strtol(buffer, &bufptr, 0));
	    break;

	case MXML_OPAQUE :
            node = mxmlNewOpaque(parent, buffer);
	    break;
     
      /****** Add "//" by Frank 11/16/2005 *****/
//	  case MXML_REAL :
//            node = mxmlNewReal(parent, strtod(buffer, &bufptr));
//	      break;
      /****** End Frank 11/16/2005 *****/

	case MXML_TEXT :
            node = mxmlNewText(parent, whitespace, buffer);
	    break;

        default : /* Should never happen... */
	    node = NULL;
	    break;
      }	  

      if (*bufptr)
      {
       /*
        * Bad integer/real number value...
	*/

        mxml_error("Bad %s value '%s' in parent <%s>!",
	           type == MXML_INTEGER ? "integer" : "real", buffer,
		   parent ? parent->value.element.name : "null");
	    break;
      }

      bufptr     = buffer;
      whitespace = isspace(ch) && type == MXML_TEXT;

      if (!node)
      {
       /*
	* Just print error for now...
	*/

	    mxml_error("Unable to add value node of type %d to parent <%s>!",
	               type, parent ? parent->value.element.name : "null");
	    break;
      }
    }
    else if (isspace(ch) && type == MXML_TEXT)
      whitespace = 1;

   /*
    * Add lone whitespace node if we have an element and existing
    * whitespace...
    */

    if (ch == '<' && whitespace && type == MXML_TEXT)
    {
      mxmlNewText(parent, whitespace, "");
      whitespace = 0;
    }

    if (ch == '<')
    {
     /*
      * Start of open/close tag...
      */

      bufptr = buffer;

      while ((ch = (*getc_cb)(p)) != EOF)
        if (isspace(ch) || ch == '>' || (ch == '/' && bufptr > buffer))
	  break;
	else if (ch == '&')
	{
	  if ((ch = mxml_get_entity(parent, p, getc_cb)) == EOF)
	    goto error;

	  if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	    goto error;
	}
	else if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	  goto error;
	else if ((bufptr - buffer) == 3 && !strncmp(buffer, "!--", 3))
	  break;

      *bufptr = '\0';

      if (!strcmp(buffer, "!--"))
      {
       /*
        * Gather rest of comment...
	*/

	while ((ch = (*getc_cb)(p)) != EOF)
	{
	  if (ch == '>' && bufptr > (buffer + 4) &&
	      !strncmp(bufptr - 2, "--", 2))
	    break;
	  else
	  {
            if (ch == '&')
	      if ((ch = mxml_get_entity(parent, p, getc_cb)) == EOF)
		goto error;

	    if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	      goto error;
	  }
	}

       /*
        * Error out if we didn't get the whole comment...
	*/

        if (ch != '>')
	  break;

       /*
        * Otherwise add this as an element under the current parent...
	*/

	*bufptr = '\0';

	if (!mxmlNewElement(parent, buffer))
	{
	 /*
	  * Just print error for now...
	  */

	  mxml_error("Unable to add comment node to parent <%s>!",
	             parent ? parent->value.element.name : "null");
	  break;
	}
      }
      else if (buffer[0] == '!')
      {
       /*
        * Gather rest of declaration...
	*/

	do
	{
	  if (ch == '>')
	    break;
	  else
	  {
            if (ch == '&')
	      if ((ch = mxml_get_entity(parent, p, getc_cb)) == EOF)
		goto error;

	    if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	      goto error;
	  }
	}
        while ((ch = (*getc_cb)(p)) != EOF);

       /*
        * Error out if we didn't get the whole declaration...
	*/

        if (ch != '>')
	  break;

       /*
        * Otherwise add this as an element under the current parent...
	*/

	*bufptr = '\0';

	node = mxmlNewElement(parent, buffer);
	if (!node)
	{
	 /*
	  * Just print error for now...
	  */

	  mxml_error("Unable to add declaration node to parent <%s>!",
	             parent ? parent->value.element.name : "null");
	  break;
	}

       /*
	* Descend into this node, setting the value type as needed...
	*/

	parent = node;

	if (cb && parent)
	  type = (*cb)(parent);
      }
      else if (buffer[0] == '/')
      {
       /*
        * Handle close tag...
	*/

        if (!parent || strcmp(buffer + 1, parent->value.element.name))
	{
	 /*
	  * Close tag doesn't match tree; print an error for now...
	  */

	  mxml_error("Mismatched close tag <%s> under parent <%s>!",
	             buffer, parent->value.element.name);
          break;
	}

       /*
        * Keep reading until we see >...
	*/

        while (ch != '>' && ch != EOF)
	  ch = (*getc_cb)(p);

       /*
	* Ascend into the parent and set the value type as needed...
	*/

	parent = parent->parent;

	if (cb && parent)
	  type = (*cb)(parent);
      }
      else
      {
       /*
        * Handle open tag...
	*/

        node = mxmlNewElement(parent, buffer);

	if (!node)
	{
	 /*
	  * Just print error for now...
	  */

	  mxml_error("Unable to add element node to parent <%s>!",
	             parent ? parent->value.element.name : "null");
	  break;
	}

        if (isspace(ch))
          ch = mxml_parse_element(node, p, getc_cb);
        else if (ch == '/')
	{
	  if ((ch = (*getc_cb)(p)) != '>')
	  {
	    mxml_error("Expected > but got '%c' instead for element <%s/>!",
	               ch, buffer);
            break;
	  }

	  ch = '/';
	}

	if (ch == EOF)
	  break;

        if (ch != '/')
	{
	 /*
	  * Descend into this node, setting the value type as needed...
	  */

	  parent = node;

	  if (cb && parent)
	    type = (*cb)(parent);
	}
      }

      bufptr  = buffer;
    }
    else if (ch == '&')
    {
     /*
      * Add character entity to current buffer...
      */

      if ((ch = mxml_get_entity(parent, p, getc_cb)) == EOF)
	goto error;

      if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	goto error;
    }
    else if (type == MXML_OPAQUE || !isspace(ch))
    {
     /*
      * Add character to current buffer...
      */

      if (mxml_add_char(ch, &bufptr, &buffer, &bufsize))
	goto error;
    }
  }

 /*
  * Free the string buffer - we don't need it anymore...
  */
  #ifdef LEADFLY
    npfree(buffer);

  /***** Add by Frank 11/16/2005 *****/
  #else  
    free(buffer);
  #endif
  /***** End Frank 11/16/2005 *****/
 /*
  * Find the top element and return it...
  */

  if (parent)
  {
    while (parent->parent != top && parent->parent)
      parent = parent->parent;
  }

  return (parent);

 /*
  * Common error return...
  */

error:
  #ifdef LEADFLY
    npfree(buffer);
  
  /***** Add by Frank 11/16/2005 *****/ 
  #else
    free(buffer);
  #endif
  /***** End Frank 11/16/2005 *****/
  return (NULL);
}


/*
 * 'mxml_parse_element()' - Parse an element for any attributes...
 */

static int				/* O - Terminating character */
mxml_parse_element(mxml_node_t *node,	/* I - Element node */
                   void        *p,	/* I - Data to read from */
                   int         (*getc_cb)(void *))
					/* I - Data callback */
{
  int	ch,				/* Current character in file */
	quote;				/* Quoting character */
  char	*name,				/* Attribute name */
	*value,				/* Attribute value */
	*ptr;				/* Pointer into name/value */
  int	namesize,			/* Size of name string */
	valsize;			/* Size of value string */




 /*
  * Initialize the name and value buffers...
  */
  /***** Add by Frank 11/16/2005 *****/
  #ifdef LEADFLY
    if ((name = npalloc(64)) == NULL)
  
  #else
  /***** End Frank 11/16/2005 *****/
    if ((name = malloc(64)) == NULL)
  /***** Add by Frank 11/16/2005 *****/
  #endif
  /***** End Frank 11/16/2005 *****/
  {
    mxml_error("Unable to allocate memory for name!");
    return (EOF);
  }

  namesize = 64;
  /***** Add by Frank 11/16/2005 *****/
  #ifdef LEADFLY
    if ((value = npalloc(64)) == NULL)
  
  #else
  /***** End Frank 11/16/2005 *****/
    if ((value = malloc(64)) == NULL)
  /***** Add by Frank 11/16/2005 *****/  
  #endif
  /***** End Frank 11/16/2005 *****/
  {
    #ifdef LEADFLY
      npfree(name);
    
    /***** Add by Frank 11/16/2005  *****/
    #else
      free(name);
    #endif
    /***** End Frank 11/16/2005 *****/
    mxml_error("Unable to allocate memory for value!");
    return (EOF);
  }

  valsize = 64;

 /*
  * Loop until we hit a >, /, ?, or EOF...
  */

  while ((ch = (*getc_cb)(p)) != EOF)
  {
#if DEBUG > 1
//    fprintf(stderr, "parse_element: ch='%c'\n", ch);
    
    /***** Add by Frank 11/14/2005 *****/
    printf("parse_element: ch='%c'\n", ch);
    /***** End Frank 11/16/2005 *****/
#endif /* DEBUG > 1 */

   /*
    * Skip leading whitespace...
    */

    if (isspace(ch))
      continue;

   /*
    * Stop at /, ?, or >...
    */

    if (ch == '/' || ch == '?')
    {
     /*
      * Grab the > character and print an error if it isn't there...
      */

      quote = (*getc_cb)(p);

      if (quote != '>')
      {
        mxml_error("Expected '>' after '%c' for element %s, but got '%c'!",
	           ch, node->value.element.name, quote);
        ch = EOF;
      }

      break;
    }
    else if (ch == '>')
      break;

   /*
    * Read the attribute name...
    */

    name[0] = ch;
    ptr     = name + 1;

    if (ch == '\"' || ch == '\'')
    {
     /*
      * Name is in quotes, so get a quoted string...
      */

      quote = ch;

      while ((ch = (*getc_cb)(p)) != EOF)
      {
        if (ch == '&')
	  if ((ch = mxml_get_entity(node, p, getc_cb)) == EOF)
	    goto error;

	if (mxml_add_char(ch, &ptr, &name, &namesize))
	  goto error;

	if (ch == quote)
          break;
      }
    }
    else
    {
     /*
      * Grab an normal, non-quoted name...
      */

      while ((ch = (*getc_cb)(p)) != EOF)
	if (isspace(ch) || ch == '=' || ch == '/' || ch == '>' || ch == '?')
          break;
	else
	{
          if (ch == '&')
	    if ((ch = mxml_get_entity(node, p, getc_cb)) == EOF)
	      goto error;

	  if (mxml_add_char(ch, &ptr, &name, &namesize))
	    goto error;
	}
    }

    *ptr = '\0';

    if (ch == '=')
    {
     /*
      * Read the attribute value...
      */

      if ((ch = (*getc_cb)(p)) == EOF)
      {
        mxml_error("Missing value for attribute '%s' in element %s!",
	           name, node->value.element.name);
        return (EOF);
      }

      if (ch == '\'' || ch == '\"')
      {
       /*
        * Read quoted value...
	*/

        quote = ch;
	ptr   = value;

        while ((ch = (*getc_cb)(p)) != EOF)
	  if (ch == quote)
	    break;
	  else
	  {
	    if (ch == '&')
	      if ((ch = mxml_get_entity(node, p, getc_cb)) == EOF)
	        goto error;
	      
	    if (mxml_add_char(ch, &ptr, &value, &valsize))
	      goto error;
	  }

        *ptr = '\0';
      }
      else
      {
       /*
        * Read unquoted value...
	*/

	value[0] = ch;
	ptr      = value + 1;

	while ((ch = (*getc_cb)(p)) != EOF)
	  if (isspace(ch) || ch == '=' || ch == '/' || ch == '>')
            break;
	  else
	  {
	    if (ch == '&')
	      if ((ch = mxml_get_entity(node, p, getc_cb)) == EOF)
	        goto error;
	      
	    if (mxml_add_char(ch, &ptr, &value, &valsize))
	      goto error;
	  }

        *ptr = '\0';
      }

     /*
      * Set the attribute with the given string value...
      */

      mxmlElementSetAttr(node, name, value);
    }
    else
    {
     /*
      * Set the attribute with a NULL value...
      */

      mxmlElementSetAttr(node, name, NULL);
    }

   /*
    * Check the end character...
    */

    if (ch == '/' || ch == '?')
    {
     /*
      * Grab the > character and print an error if it isn't there...
      */

      quote = (*getc_cb)(p);

      if (quote != '>')
      {
        mxml_error("Expected '>' after '%c' for element %s, but got '%c'!",
	           ch, node->value.element.name, quote);
        ch = EOF;
      }

      break;
    }
    else if (ch == '>')
      break;
  }

 /*
  * Free the name and value buffers and return...
  */
  #ifdef LEADFLY
    npfree(name);
    npfree(value);
  
  /***** Add by Frank 11/16/2005  *****/
  #else
    free(name);
    free(value);
  #endif
  /***** End Frank 11/16/2005  *****/
  return (ch);

 /*
  * Common error return point...
  */

error:
  #ifdef LEADFLY
    npfree(name);
    npfree(value);
  
  /***** Add by Frank 11/16/2005  *****/
  #else
    free(name);
    free(value);
  #endif
  /***** End Frank 11/16/2005  *****/
  return (EOF);
}


/*
 * 'mxml_string_getc()' - Get a character from a string.
 */

static int				/* O - Character or EOF */
mxml_string_getc(void *p)		/* I - Pointer to file */
{
  int		ch;			/* Character */
  const char	**s;			/* Pointer to string pointer */


  s = (const char **)p;

  if ((ch = *s[0]) != 0)
  {
   /*
    * Got character; convert UTF-8 to integer and return...
    */

    (*s)++;

    if (!(ch & 0x80))
      return (ch);
    else if ((ch & 0xe0) == 0xc0)
    {
     /*
      * Two-byte value...
      */

      if ((*s[0] & 0xc0) != 0x80)
        return (EOF);

      ch = ((ch & 0x1f) << 6) | (*s[0] & 0x3f);

      (*s)++;

      return (ch);
    }
    else if ((ch & 0xf0) == 0xe0)
    {
     /*
      * Three-byte value...
      */

      if ((*s[0] & 0xc0) != 0x80 ||
          (*s[1] & 0xc0) != 0x80)
        return (EOF);

      ch = ((((ch & 0x0f) << 6) | (*s[0] & 0x3f)) << 6) | (*s[1] & 0x3f);

      (*s) += 2;

      return (ch);
    }
    else if ((ch & 0xf8) == 0xf0)
    {
     /*
      * Four-byte value...
      */

      if ((*s[0] & 0xc0) != 0x80 ||
          (*s[1] & 0xc0) != 0x80 ||
          (*s[2] & 0xc0) != 0x80)
        return (EOF);

      ch = ((((((ch & 0x07) << 6) | (*s[0] & 0x3f)) << 6) |
             (*s[1] & 0x3f)) << 6) | (*s[2] & 0x3f);

      (*s) += 3;

      return (ch);
    }
    else
      return (EOF);
  }
  else
    return (EOF);
}


/*
 * 'mxml_string_putc()' - Write a character to a string.
 */

static int				/* O - 0 on success, -1 on failure */
mxml_string_putc(int  ch,		/* I - Character to write */
                 void *p)		/* I - Pointer to string pointers */
{
  char	**pp;				/* Pointer to string pointers */


  pp = (char **)p;

  if (ch < 128)
  {
   /*
    * Plain ASCII doesn't need special encoding...
    */

    if (pp[0] < pp[1])
      pp[0][0] = ch;

    pp[0] ++;
  }
  else if (ch < 2048)
  {
   /*
    * Two-byte UTF-8 character...
    */

    if ((pp[0] + 1) < pp[1])
    {
      pp[0][0] = 0xc0 | (ch >> 6);
      pp[0][1] = 0x80 | (ch & 0x3f);
    }

    pp[0] += 2;
  }
  else if (ch < 65536)
  {
   /*
    * Three-byte UTF-8 character...
    */

    if ((pp[0] + 2) < pp[1])
    {
      pp[0][0] = 0xe0 | (ch >> 12);
      pp[0][1] = 0x80 | ((ch >> 6) & 0x3f);
      pp[0][2] = 0x80 | (ch & 0x3f);
    }

    pp[0] += 3;
  }
  else
  {
   /*
    * Four-byte UTF-8 character...
    */

    if ((pp[0] + 2) < pp[1])
    {
      pp[0][0] = 0xf0 | (ch >> 18);
      pp[0][1] = 0x80 | ((ch >> 12) & 0x3f);
      pp[0][2] = 0x80 | ((ch >> 6) & 0x3f);
      pp[0][3] = 0x80 | (ch & 0x3f);
    }

    pp[0] += 4;
  }

  return (0);
}


/*
 * 'mxml_write_name()' - Write a name string.
 */

static int				/* O - 0 on success, -1 on failure */
mxml_write_name(const char *s,		/* I - Name to write */
                void       *p,		/* I - Write pointer */
		int        (*putc_cb)(int, void *))
					/* I - Write callback */
{
  char		quote;			/* Quote character */
  const char	*name;			/* Entity name */


  if (*s == '\"' || *s == '\'')
  {
   /*
    * Write a quoted name string...
    */

    if ((*putc_cb)(*s, p) < 0)
      return (-1);

    quote = *s++;

    while (*s && *s != quote)
    {
      if ((name = mxmlEntityGetName(*s)) != NULL)
      {
	if ((*putc_cb)('&', p) < 0)
          return (-1);

        while (*name)
	{
	  if ((*putc_cb)(*name, p) < 0)
            return (-1);

          name ++;
	}

	if ((*putc_cb)(';', p) < 0)
          return (-1);
      }
      else if ((*putc_cb)(*s, p) < 0)
	return (-1);

      s ++;
    }

   /*
    * Write the end quote...
    */

    if ((*putc_cb)(quote, p) < 0)
      return (-1);
  }
  else
  {
   /*
    * Write a non-quoted name string...
    */

    while (*s)
    {
      if ((*putc_cb)(*s, p) < 0)
	return (-1);

      s ++;
    }
  }

  return (0);
}


/*
 * 'mxml_write_node()' - Save an XML node to a file.
 */

static int				/* O - Column or -1 on error */
mxml_write_node(mxml_node_t *node,	/* I - Node to write */
                void        *p,		/* I - File to write to */
	            int         (*cb)(mxml_node_t *, int),
					/* I - Whitespace callback */
		        int         col,	/* I - Current column */
		        int         (*putc_cb)(int, void *))
{
  int		i,			/* Looping var */
		    width;			/* Width of attr + value */
  mxml_attr_t	*attr;			/* Current attribute */
  char		s[255];			/* Temporary string */


  while (node != NULL)
  {
   /*
    * Print the node value...
    */

    switch (node->type)
    {
      case MXML_ELEMENT :
          col = mxml_write_ws(node, p, cb, MXML_WS_BEFORE_OPEN, col, putc_cb);

          if ((*putc_cb)('<', p) < 0)
	        return (-1);
	      if (mxml_write_name(node->value.element.name, p, putc_cb) < 0)
	        return (-1);

          col += strlen(node->value.element.name) + 1;

	  for (i = node->value.element.num_attrs, attr = node->value.element.attrs;
	       i > 0;
	       i --, attr ++)
	  {
	    width = strlen(attr->name);

	    if (attr->value)
	      width += strlen(attr->value) + 3;

	    if ((col + width) > MXML_WRAP)
	    {
//	        if ((*putc_cb)('\n', p) < 0)
          
          /***** Add by Frank 11/18/2005 *****/
          
          if((*putc_cb)(' ', p) < 0)
          
          /***** End Frank 11/18/2005 *****/
	      return (-1);

	      col = 0;
	    }
	    else
	    {
	      if ((*putc_cb)(' ', p) < 0)
	        return (-1);

	      col ++;
	    }

        if (mxml_write_name(attr->name, p, putc_cb) < 0)
	      return (-1);

	    if (attr->value)
	    {
          if ((*putc_cb)('=', p) < 0)
		    return (-1);
          if ((*putc_cb)('\"', p) < 0)
		    return (-1);
	      if (mxml_write_string(attr->value, p, putc_cb) < 0)
		    return (-1);
          if ((*putc_cb)('\"', p) < 0)
		    return (-1);
        }

        col += width;
	  }

	  if (node->child)
	  {
       /*
	    * The ? and ! elements are special-cases and have no end tags...
	    */

	    if (node->value.element.name[0] == '?')
	    {
          if ((*putc_cb)('?', p) < 0)
	        return (-1);
          if ((*putc_cb)('>', p) < 0)
		    return (-1);
          if ((*putc_cb)('\n', p) < 0)
		    return (-1);

          col = 0;
        }
	    else if ((*putc_cb)('>', p) < 0)
	      return (-1);
	    else
	      col ++;

            col = mxml_write_ws(node, p, cb, MXML_WS_AFTER_OPEN, col, putc_cb);

	    if ((col = mxml_write_node(node->child, p, cb, col, putc_cb)) < 0)
	      return (-1);

        if (node->value.element.name[0] != '?' &&
	        node->value.element.name[0] != '!')
	    {
          col = mxml_write_ws(node, p, cb, MXML_WS_BEFORE_CLOSE, col, putc_cb);

          if ((*putc_cb)('<', p) < 0)
	        return (-1);
          if ((*putc_cb)('/', p) < 0)
		    return (-1);
          if (mxml_write_string(node->value.element.name, p, putc_cb) < 0)
		    return (-1);
          if ((*putc_cb)('>', p) < 0)
		    return (-1);

          col += strlen(node->value.element.name) + 3;

          col = mxml_write_ws(node, p, cb, MXML_WS_AFTER_CLOSE, col, putc_cb);
	    }
	  }
	  else if (node->value.element.name[0] == '!')
	  {
	    if ((*putc_cb)('>', p) < 0)
	      return (-1);
	    else
	      col ++;

        col = mxml_write_ws(node, p, cb, MXML_WS_AFTER_OPEN, col, putc_cb);
      }
	  else
	  {
            if ((*putc_cb)('/', p) < 0)
	      return (-1);
            if ((*putc_cb)('>', p) < 0)
	      return (-1);

	    col += 2;

            col = mxml_write_ws(node, p, cb, MXML_WS_AFTER_OPEN, col, putc_cb);
	  }
          break;

      case MXML_INTEGER :
	      if (node->prev)
	      {
	        if (col > MXML_WRAP)
	        {
	          if ((*putc_cb)('\n', p) < 0)
	          return (-1);

	          col = 0;
	        }
	        else if ((*putc_cb)(' ', p) < 0)
	          return (-1);
	        else
	          col ++;
          }

          sprintf(s, "%d", node->value.integer);
	      if (mxml_write_string(s, p, putc_cb) < 0)
	        return (-1);

	      col += strlen(s);
          break;

      case MXML_OPAQUE :
          if (mxml_write_string(node->value.opaque, p, putc_cb) < 0)
	        return (-1);

          col += strlen(node->value.opaque);
          break;

//      case MXML_REAL :
//	  if (node->prev)
//	  {
//	    if (col > MXML_WRAP)
//	    {
//	      if ((*putc_cb)('\n', p) < 0)
//	        return (-1);

//	      col = 0;
//	    }
//	    else if ((*putc_cb)(' ', p) < 0)
//	      return (-1);
//	    else
//	      col ++;
//          }

          //sprintf(s, "%f", node->value.real);
//          sprintf(s, "%ld", node->value.real);
//	  if (mxml_write_string(s, p, putc_cb) < 0)
//	    return (-1);

//	  col += strlen(s);
//          break;

      case MXML_TEXT :
	  if (node->value.text.whitespace && col > 0)
	  {
	    if (col > MXML_WRAP)
	    {
	      if ((*putc_cb)('\n', p) < 0)
	        return (-1);

	      col = 0;
	    }
	    else if ((*putc_cb)(' ', p) < 0)
	      return (-1);
	    else
	      col ++;
          }

          if (mxml_write_string(node->value.text.string, p, putc_cb) < 0)
	    return (-1);

	  col += strlen(node->value.text.string);
          break;
    }

   /*
    * Next node...
    */

    node = node->next;
  }

  return (col);
}


/*
 * 'mxml_write_string()' - Write a string, escaping & and < as needed.
 */

static int				/* O - 0 on success, -1 on failure */
mxml_write_string(const char *s,	/* I - String to write */
                  void       *p,	/* I - Write pointer */
		  int        (*putc_cb)(int, void *))
					/* I - Write callback */
{
  const char	*name;			/* Entity name, if any */


  while (*s)
  {
    if ((name = mxmlEntityGetName(*s)) != NULL)
    {
      if ((*putc_cb)('&', p) < 0)
        return (-1);

      while (*name)
      {
	if ((*putc_cb)(*name, p) < 0)
          return (-1);
        name ++;
      }

      if ((*putc_cb)(';', p) < 0)
        return (-1);
    }
    else if ((*putc_cb)(*s, p) < 0)
      return (-1);

    s ++;
  }

  return (0);
}


/*
 * 'mxml_write_ws()' - Do whitespace callback...
 */

static int				/* O - New column */
mxml_write_ws(mxml_node_t *node,	/* I - Current node */
              void        *p,		/* I - Write pointer */
              int         (*cb)(mxml_node_t *, int),
					/* I - Callback function */
	      int         ws,		/* I - Where value */
	      int         col,		/* I - Current column */
              int         (*putc_cb)(int, void *))
					/* I - Write callback */
{
  int	ch;				/* Whitespace character */


  if (cb && (ch = (*cb)(node, ws)) != 0)
  {
    if ((*putc_cb)(ch, p) < 0)
      return (-1);
    else if (ch == '\n')
      col = 0;
    else if (ch == '\t')
    {
      col += MXML_TAB;
      col = col - (col % MXML_TAB);
    }
    else
      col ++;
  }

  return (col);
}


/*
 * End of "$Id: mxml-file.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $".
 */
