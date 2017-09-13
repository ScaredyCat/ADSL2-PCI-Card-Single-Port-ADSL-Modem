/*
 * "$Id: mxml-node.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $"
 *
 * Node support code for mini-XML, a small XML-like file parsing library.
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
 *   mxmlAdd()        - Add a node to a tree.
 *   mxmlDelete()     - Delete a node and all of its children.
 *   mxmlNewElement() - Create a new element node.
 *   mxmlNewInteger() - Create a new integer node.
 *   mxmlNewOpaque()  - Create a new opaque string.
 *   mxmlNewReal()    - Create a new real number node.
 *   mxmlNewText()    - Create a new text fragment node.
 *   mxmlNewTextf()   - Create a new formatted text fragment node.
 *   mxmlRemove()     - Remove a node from its parent.
 *   mxml_new()       - Create a new node.
 */

/*
 * Include necessary headers...
 */

#include "config.h"
#include "mxml.h"

#include "../../tools/logger.h"
/*
 * Local functions...
 */

static mxml_node_t	*mxml_new(mxml_node_t *parent, mxml_type_t type);


/*
 * 'mxmlAdd()' - Add a node to a tree.
 *
 * Adds the specified node to the parent. If the child argument is not
 * NULL, puts the new node before or after the specified child depending
 * on the value of the where argument. If the child argument is NULL,
 * puts the new node at the beginning of the child list (MXML_ADD_BEFORE)
 * or at the end of the child list (MXML_ADD_AFTER). The constant
 * MXML_ADD_TO_PARENT can be used to specify a NULL child pointer.
 */

void
mxmlAdd(mxml_node_t *parent,		/* I - Parent node */
        int         where,		/* I - Where to add, MXML_ADD_BEFORE or MXML_ADD_AFTER */
        mxml_node_t *child,		/* I - Child node for where or MXML_ADD_TO_PARENT */
	mxml_node_t *node)		/* I - Node to add */
{
#ifdef DEBU
//  fprintf(stderr, "mxmlAdd(parent=%p, where=%d, child=%p, node=%p)\n", parent,
//          where, child, node);

  /***** Add by Frank 11/17/2005 printf() *****/   
  printf("mxmlAdd(parent=%p, where=%d, child=%p, node=%p)\n", parent,
            where, child, node);
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!parent || !node)
    return;

#if DEBU > 1
//  fprintf(stderr, "    BEFORE: node->parent=%p\n", node->parent);

  /***** Add by Frank 11/17/2005 printf() *****/
  printf("    BEFORE: node->parent=%p\n", node->parent);  
  if (parent)
  {
//    fprintf(stderr, "    BEFORE: parent->child=%p\n", parent->child);
//    fprintf(stderr, "    BEFORE: parent->last_child=%p\n", parent->last_child);
//    fprintf(stderr, "    BEFORE: parent->prev=%p\n", parent->prev);
//    fprintf(stderr, "    BEFORE: parent->next=%p\n", parent->next);

    /********** Add by Frank 11/17/2005  **********/
    printf("    BEFORE: parent->child=%p\n", parent->child);
    printf("    BEFORE: parent->last_child=%p\n", parent->last_child);
    printf("    BEFORE: parent->prev=%p\n", parent->prev);
    printf("    BEFORE: parent->next=%p\n", parent->next);
    /********** End Frank 11/17/2005  ***********/
  }
#endif /* DEBUG > 1 */

 /*
  * Remove the node from any existing parent...
  */

  if (node->parent)
    mxmlRemove(node);

 /*
  * Reset pointers...
  */

  node->parent = parent;

  switch (where)
  {
    case MXML_ADD_BEFORE :
        if (!child || child == parent->child || child->parent != parent)
	{
	 /*
	  * Insert as first node under parent...
	  */

	  node->next = parent->child;

	  if (parent->child)
	    parent->child->prev = node;
	  else
	    parent->last_child = node;

	  parent->child = node;
	}
	else
	{
	 /*
	  * Insert node before this child...
	  */

	  node->next = child;
	  node->prev = child->prev;

	  if (child->prev)
	    child->prev->next = node;
	  else
	    parent->child = node;

	  child->prev = node;
	}
        break;

    case MXML_ADD_AFTER :
        if (!child || child == parent->last_child || child->parent != parent)
	{
	 /*
	  * Insert as last node under parent...
	  */

	  node->parent = parent;
	  node->prev   = parent->last_child;

	  if (parent->last_child)
	    parent->last_child->next = node;
	  else
	    parent->child = node;

	  parent->last_child = node;
        }
	else
	{
	 /*
	  * Insert node after this child...
	  */

	  node->prev = child;
	  node->next = child->next;

	  if (child->next)
	    child->next->prev = node;
	  else
	    parent->last_child = node;

	  child->next = node;
	}
        break;
  }

#if DEBU > 1
//  fprintf(stderr, "    AFTER: node->parent=%p\n", node->parent);
  
  /***** Add by Frank 11/17/2005 printf() *****/
  printf("    AFTER: node->parent=%p\n", node->parent);
  if (parent)
  {
//    fprintf(stderr, "    AFTER: parent->child=%p\n", parent->child);
//    fprintf(stderr, "    AFTER: parent->last_child=%p\n", parent->last_child);
//    fprintf(stderr, "    AFTER: parent->prev=%p\n", parent->prev);
//    fprintf(stderr, "    AFTER: parent->next=%p\n", parent->next);
    
    /************ Add by Frank 11/17/2005  ************/
    printf("    AFTER: parent->child=%p\n", parent->child);
    printf("    AFTER: parent->last_child=%p\n", parent->last_child);
    printf("    AFTER: parent->prev=%p\n", parent->prev);
    printf("    AFTER: parent->next=%p\n", parent->next);
    /*********** Add by Frank 11/17/2005  **********/
  }
#endif /* DEBUG > 1 */
}


/*
 * 'mxmlDelete()' - Delete a node and all of its children.
 *
 * If the specified node has a parent, this function first removes the
 * node from its parent using the mxmlRemove() function.
 */

void
mxmlDelete(mxml_node_t *node)		/* I - Node to delete */
{
  int	i;				/* Looping var */


#ifdef DEBU
//  fprintf(stderr, "mxmlDelete(node=%p)\n", node);
  
  /***** Add by Frank 11/17/2005 printf() *****/
  printf("mxmlDelete(node=%p)\n", node);
  /***** End Frank 11/17/2005 printf() *****/
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!node)
    return;

 /*
  * Remove the node from its parent, if any...
  */

  mxmlRemove(node);

 /*
  * Delete children...
  */

  while (node->child)
    mxmlDelete(node->child);

 /*
  * Now delete any node data...
  */
  /***** Add by Frank 11/17/2005 *****/
  #ifdef LEADFLY
    switch (node->type)
    {
      case MXML_ELEMENT :
          if (node->value.element.name){          
     
	        npfree(node->value.element.name);	    

          }
	      if (node->value.element.num_attrs)
	      {
	        for (i = 0; i < node->value.element.num_attrs; i ++)
	        {
	          if (node->value.element.attrs[i].name){	          
         
	            npfree(node->value.element.attrs[i].name);	          

	          }
	          if (node->value.element.attrs[i].value)

	            npfree(node->value.element.attrs[i].value);

	        }

            npfree(node->value.element.attrs);

	      }
          break;
      case MXML_INTEGER :
         /* Nothing to do */
          break;
      case MXML_OPAQUE :
          if (node->value.opaque)

	        npfree(node->value.opaque);
          break;
        
       /***** Add "//" by Frank 11/15/2005 *****/
//    case MXML_REAL :
         /* Nothing to do */
//        break;
      case MXML_TEXT :
          if (node->value.text.string)

	        npfree(node->value.text.string);
          break;
    }

 /*
  * Free this node...
  */

    npfree(node);
   /***** End Frank 11/17/2005 *****/  

  #else
    switch (node->type)
    {
      case MXML_ELEMENT :
          if (node->value.element.name)
	        free(node->value.element.name);

	      if (node->value.element.num_attrs)
	      {
	        for (i = 0; i < node->value.element.num_attrs; i ++)
	        {
	          if (node->value.element.attrs[i].name)
	            free(node->value.element.attrs[i].name);
	          if (node->value.element.attrs[i].value)
	            free(node->value.element.attrs[i].value);
	        }

            free(node->value.element.attrs);
	      }
          break;
      case MXML_INTEGER :
         /* Nothing to do */
          break;
      case MXML_OPAQUE :
          if (node->value.opaque)
	        free(node->value.opaque);
          break;
//      case MXML_REAL :
         /* Nothing to do */
//          break;
      case MXML_TEXT :
          if (node->value.text.string)
	        free(node->value.text.string);
          break;
    }

 /*
  * Free this node...
  */

    free(node);
    
  #endif
}


/*
 * 'mxmlNewElement()' - Create a new element node.
 *
 * The new element node is added to the end of the specified parent's child
 * list. The constant MXML_NO_PARENT can be used to specify that the new
 * element node has no parent.
 */

mxml_node_t *				/* O - New node */
mxmlNewElement(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
               const char  *name)	/* I - Name of element */
{
  mxml_node_t	*node;			/* New node */


#ifdef DEBU
//  fprintf(stderr, "mxmlNewElement(parent=%p, name=\"%s\")\n", parent,
//          name ? name : "(null)");
  /***** Add by Frank 11/17/2005  *****/
  printf("mxmlNewElement(parent=%p, name=\"%s\")\n", parent,
          name ? name : "(null)");
  /***** End Frank 11/17/2005 *****/
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!name)
    return (NULL);

 /*
  * Create the node and set the element name...
  */

  if ((node = mxml_new(parent, MXML_ELEMENT)) != NULL)
    node->value.element.name = strdup(name);
    
  return (node);
}


/*
 * 'mxmlNewInteger()' - Create a new integer node.
 *
 * The new integer node is added to the end of the specified parent's child
 * list. The constant MXML_NO_PARENT can be used to specify that the new
 * integer node has no parent.
 */

mxml_node_t *				/* O - New node */
mxmlNewInteger(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
               int         integer)	/* I - Integer value */
{
  mxml_node_t	*node;			/* New node */


#ifdef DEBU
//  fprintf(stderr, "mxmlNewInteger(parent=%p, integer=%d)\n", parent, integer);
  
  /***** Add by Frank 11/17/2005  *****/
  printf("mxmlNewInteger(parent=%p, integer=%d)\n", parent, integer);
  /***** End Frank 11/17/2005 *****/
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!parent)
    return (NULL);

 /*
  * Create the node and set the element name...
  */

  if ((node = mxml_new(parent, MXML_INTEGER)) != NULL)
    node->value.integer = integer;

  return (node);
}


/*
 * 'mxmlNewOpaque()' - Create a new opaque string.
 *
 * The new opaque node is added to the end of the specified parent's child
 * list. The constant MXML_NO_PARENT can be used to specify that the new
 * opaque node has no parent. The opaque string must be nul-terminated and
 * is copied into the new node.
 */

mxml_node_t *				/* O - New node */
mxmlNewOpaque(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
              const char  *opaque)	/* I - Opaque string */
{
  mxml_node_t	*node;			/* New node */


#ifdef DEBU
//  fprintf(stderr, "mxmlNewOpaque(parent=%p, opaque=\"%s\")\n", parent,
//          opaque ? opaque : "(null)");
          
  /***** Add by Frank 11/17/2005  *****/
  printf("mxmlNewOpaque(parent=%p, opaque=\"%s\")\n", parent,
          opaque ? opaque : "(null)");
  /***** End Frank 11/17/2005 *****/        
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!parent || !opaque)
    return (NULL);

 /*
  * Create the node and set the element name...
  */

  if ((node = mxml_new(parent, MXML_OPAQUE)) != NULL)
    node->value.opaque = strdup(opaque);
  
  return (node);
}


/*
 * 'mxmlNewReal()' - Create a new real number node.
 *
 * The new real number node is added to the end of the specified parent's
 * child list. The constant MXML_NO_PARENT can be used to specify that
 * the new real number node has no parent.
 */

//mxml_node_t *				/* O - New node */
//mxmlNewReal(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
//            double      real)		/* I - Real number value */
//{
//  mxml_node_t	*node;			/* New node */


//#ifdef DEBUG
//  fprintf(stderr, "mxmlNewReal(parent=%p, real=%g)\n", parent, real);
//#endif /* DEBUG */

 /*
  * Range check input...
  */

//  if (!parent)
//    return (NULL);

 /*
  * Create the node and set the element name...
  */

//  if ((node = mxml_new(parent, MXML_REAL)) != NULL)
//    node->value.real = real;

//  return (node);
//}


/*
 * 'mxmlNewText()' - Create a new text fragment node.
 *
 * The new text node is added to the end of the specified parent's child
 * list. The constant MXML_NO_PARENT can be used to specify that the new
 * text node has no parent. The whitespace parameter is used to specify
 * whether leading whitespace is present before the node. The text
 * string must be nul-terminated and is copied into the new node.  
 */

mxml_node_t *				/* O - New node */
mxmlNewText(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
            int         whitespace,	/* I - 1 = leading whitespace, 0 = no whitespace */
	    const char  *string)	/* I - String */
{
  mxml_node_t	*node;			/* New node */


#ifdef DEBU
//  fprintf(stderr, "mxmlNewText(parent=%p, whitespace=%d, string=\"%s\")\n",
//          parent, whitespace, string ? string : "(null)");
          
  /***** Add by Frank 11/17/2005  *****/
  printf("mxmlNewText(parent=%p, whitespace=%d, string=\"%s\")\n",
          parent, whitespace, string ? string : "(null)");
  /***** End Frank 11/17/2005 *****/
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!parent || !string)
    return (NULL);

 /*
  * Create the node and set the text value...
  */

  if ((node = mxml_new(parent, MXML_TEXT)) != NULL)
  {
    node->value.text.whitespace = whitespace;
    node->value.text.string     = strdup(string);
    
  }

  return (node);
}


/*
 * 'mxmlNewTextf()' - Create a new formatted text fragment node.
 *
 * The new text node is added to the end of the specified parent's child
 * list. The constant MXML_NO_PARENT can be used to specify that the new
 * text node has no parent. The whitespace parameter is used to specify
 * whether leading whitespace is present before the node. The format
 * string must be nul-terminated and is formatted into the new node.  
 */

mxml_node_t *				/* O - New node */
mxmlNewTextf(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
             int         whitespace,	/* I - 1 = leading whitespace, 0 = no whitespace */
	     const char  *format,	/* I - Printf-style frmat string */
	     ...)			/* I - Additional args as needed */
{
  mxml_node_t	*node;			/* New node */
  va_list	ap;			/* Pointer to arguments */


#ifdef DEBU
//  fprintf(stderr, "mxmlNewTextf(parent=%p, whitespace=%d, format=\"%s\", ...)\n",
//          parent, whitespace, format ? format : "(null)");

   /***** Add by Frank 11/17/2005 *****/
   printf("mxmlNewTextf(parent=%p, whitespace=%d, format=\"%s\", ...)\n",
          parent, whitespace, format ? format : "(null)");
   /***** End Frank 11/17/2005 *****/
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!parent || !format)
    return (NULL);

 /*
  * Create the node and set the text value...
  */

  if ((node = mxml_new(parent, MXML_TEXT)) != NULL)
  {
    va_start(ap, format);

    node->value.text.whitespace = whitespace;
    node->value.text.string     = mxml_strdupf(format, ap);

    va_end(ap);
  }

  return (node);
}


/*
 * 'mxmlRemove()' - Remove a node from its parent.
 *
 * Does not free memory used by the node - use mxmlDelete() for that.
 * This function does nothing if the node has no parent.
 */

void
mxmlRemove(mxml_node_t *node)		/* I - Node to remove */
{
#ifdef DEBU
//  fprintf(stderr, "mxmlRemove(node=%p)\n", node);
  
  /***** Add by Frank 11/17/2005 *****/
  printf("mxmlRemove(node=%p)\n", node);
  /***** End Frank 11/17/2005 *****/
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!node || !node->parent)
    return;

 /*
  * Remove from parent...
  */

#if DEBU > 1
//  fprintf(stderr, "    BEFORE: node->parent=%p\n", node->parent);
  
  /***** Add by Frank 11/17/2005 *****/
  printf("    BEFORE: node->parent=%p\n", node->parent);
  /***** End Frank 11/17/2005 *****/
  if (node->parent)
  {
//    fprintf(stderr, "    BEFORE: node->parent->child=%p\n", node->parent->child);
//    fprintf(stderr, "    BEFORE: node->parent->last_child=%p\n", node->parent->last_child);
    
    /***** Add by Frank 11/17/2005 *****/
    
    printf("    BEFORE: node->parent->child=%p\n", node->parent->child);
    printf("    BEFORE: node->parent->last_child=%p\n", node->parent->last_child);
    /***** End Frank 11/17/2005 *****/
  }
//  fprintf(stderr, "    BEFORE: node->child=%p\n", node->child);
//  fprintf(stderr, "    BEFORE: node->last_child=%p\n", node->last_child);
//  fprintf(stderr, "    BEFORE: node->prev=%p\n", node->prev);
//  fprintf(stderr, "    BEFORE: node->next=%p\n", node->next);
  
  /***** Add by Frank 11/17/2005 *****/
  printf("    BEFORE: node->child=%p\n", node->child);
  printf("    BEFORE: node->last_child=%p\n", node->last_child);
  printf("    BEFORE: node->prev=%p\n", node->prev);
  printf("    BEFORE: node->next=%p\n", node->next);
  /***** End Frank 11/17/2005 *****/
#endif /* DEBUG > 1 */

  if (node->prev)
    node->prev->next = node->next;
  else
    node->parent->child = node->next;

  if (node->next)
    node->next->prev = node->prev;
  else
    node->parent->last_child = node->prev;

  node->parent = NULL;
  node->prev   = NULL;
  node->next   = NULL;

#if DEBU > 1
//  fprintf(stderr, "    AFTER: node->parent=%p\n", node->parent);
  
  /***** Add by Frank 11/17/2005 *****/
  printf("    AFTER: node->parent=%p\n", node->parent);
  /***** End Frank 11/17/2005 *****/
  if (node->parent)
  {
//    fprintf(stderr, "    AFTER: node->parent->child=%p\n", node->parent->child);
//    fprintf(stderr, "    AFTER: node->parent->last_child=%p\n", node->parent->last_child);
    
    /***** Add by Frank 11/17/2005 *****/
    printf("    AFTER: node->parent->child=%p\n", node->parent->child);
    printf("    AFTER: node->parent->last_child=%p\n", node->parent->last_child);
    /***** End Frank 11/17/2005 *****/
  }
//  fprintf(stderr, "    AFTER: node->child=%p\n", node->child);
//  fprintf(stderr, "    AFTER: node->last_child=%p\n", node->last_child);
//  fprintf(stderr, "    AFTER: node->prev=%p\n", node->prev);
//  fprintf(stderr, "    AFTER: node->next=%p\n", node->next);
  
  /***** Add by Frank 11/17/2005 *****/
  printf("    AFTER: node->child=%p\n", node->child);
  printf("    AFTER: node->last_child=%p\n", node->last_child);
  printf("    AFTER: node->prev=%p\n", node->prev);
  printf("    AFTER: node->next=%p\n", node->next);
  /***** End Frank 11/17/2005 *****/
#endif /* DEBUG > 1 */
}


/*
 * 'mxml_new()' - Create a new node.
 */

static mxml_node_t *			/* O - New node */
mxml_new(mxml_node_t *parent,		/* I - Parent node */
         mxml_type_t type)		/* I - Node type */
{
  mxml_node_t	*node;			/* New node */


#if DEBU > 1
//  fprintf(stderr, "mxml_new(parent=%p, type=%d)\n", parent, type);
  
  /***** Add by Frank 11/17/2005 *****/
  printf("mxml_new(parent=%p, type=%d)\n", parent, type);
  /***** End Frank 11/17/2005 *****/
#endif /* DEBUG > 1 */

 /*
  * Allocate memory for the node...
  */
#ifdef LEADFLY
  if ((node = (mxml_node_t *)npalloc(1*sizeof(mxml_node_t))) == NULL)

  /***** Add by Frank 11/16/2005 *****/
#else
  if ((node = calloc(1, sizeof(mxml_node_t))) == NULL)

#endif
  /***** End Frank 11/16/2005 *****/
  {
    LOG(m_handler, ERROR, "Can't allocate enough memory for a new xml node\r\n");  
#if DEBU > 1
//  fputs("    returning NULL\n", stderr);
    
  /***** Add by Frank 11/17/2005 *****/
  printf("    returning NULL\n");
  /***** End Frank 11/17/2005 *****/
#endif /* DEBUG > 1 */

    return (NULL);
  }

#if DEBU > 1
//  fprintf(stderr, "    returning %p\n", node);
  
  /***** Add by Frank 11/17/2005 *****/
  printf("    returning %p\n", node);
  /***** End Frank 11/17/2005 *****/
#endif /* DEBUG > 1 */

 /*
  * Set the node type...
  */

  node->type = type;

 /*
  * Add to the parent if present...
  */

  if (parent)
    mxmlAdd(parent, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, node);

 /*
  * Return the new node...
  */

  return (node);
}


/*
 * End of "$Id: mxml-node.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $".
 */
