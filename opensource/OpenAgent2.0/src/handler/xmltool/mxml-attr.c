/*
 * "$Id: mxml-attr.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $"
 *
 * Attribute support code for mini-XML, a small XML-like file parsing library.
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
 *   mxmlElementGetAttr() - Get an attribute.
 *   mxmlElementSetAttr() - Set an attribute.
 */

/*
 * Include necessary headers...
 */

#include "config.h"
#include "mxml.h"

#include "../../tools/logger.h"

/*
 * 'mxmlElementGetAttr()' - Get an attribute.
 *
 * This function returns NULL if the node is not an element or the
 * named attribute does not exist.
 */

const char *				/* O - Attribute value or NULL */
mxmlElementGetAttr(mxml_node_t *node,	/* I - Element node */
                   const char  *name)	/* I - Name of attribute */
{
  int	i;				/* Looping var */
  mxml_attr_t	*attr;			/* Cirrent attribute */


#ifdef DEBU
//  fprintf(stderr, "mxmlElementGetAttr(node=%p, name=\"%s\")\n",
//          node, name ? name : "(null)");
  
  /***** Add by Frank 11/16/2005 *****/
  printf("mxmlElementGetAttr(node=%p, name=\"%s\")\n",
          node, name ? name : "(null)");
  /***** End Frank 11/16/2005 *****/
          
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!node || node->type != MXML_ELEMENT || !name)
    return (NULL);

 /*
  * Look for the attribute...
  */

  for (i = node->value.element.num_attrs, attr = node->value.element.attrs;
       i > 0;
       i --, attr ++)
    if (strstr(attr->name, name))
      return (attr->value);

 /*
  * Didn't find attribute, so return NULL...
  */

  return (NULL);
}


/*
 * 'mxmlElementSetAttr()' - Set an attribute.
 *
 * If the named attribute already exists, the value of the attribute
 * is replaced by the new string value. The string value is copied
 * into the element node. This function does nothing if the node is
 * not an element.
 */

void
mxmlElementSetAttr(mxml_node_t *node,	/* I - Element node */
                   const char  *name,	/* I - Name of attribute */
                   const char  *value)	/* I - Attribute value */
{
  int		i;			/* Looping var */
  mxml_attr_t	*attr;			/* New attribute */

#ifdef DEBU
//  fprintf(stderr, "mxmlElementSetAttr(node=%p, name=\"%s\", value=\"%s\")\n",
//          node, name ? name : "(null)", value ? value : "(null)");

  /********** Add by Frank 11/16/2005  **********/
  
  printf("mxmlElementSetAttr(node=%p, name=\"%s\", value=\"%s\")\n",
          node, name ? name : "(null)", value ? value : "(null)"); 
  /********** End Frank 11/16/2005  **********/
           
#endif /* DEBUG */

 /*
  * Range check input...
  */

  if (!node || node->type != MXML_ELEMENT || !name)
    return;

 /*
  * Look for the attribute...
  */

  for (i = node->value.element.num_attrs, attr = node->value.element.attrs;
       i > 0;
       i --, attr ++)
    if (!strcmp(attr->name, name))
    {
     /*
      * Replace the attribute value and return...
      */
      /********** Add by Frank 11/15/2005 **********/
      #ifdef LEADFLY
        npfree(attr->value);
      #else
      /********** End Frank 11/15/2005 **********/
           
        free(attr->value);
      /********** Add by Frank 11/15/2005 **********/
      #endif
      /********** End Frank 11/15/2005 **********/
      
      if (value)
         attr->value = strdup(value);
         
      else
        attr->value = NULL;

      return;
    }

 /*
  * Attribute not found, so add a new one...
  */

  if (node->value.element.num_attrs == 0)
  {
    /********** Add by Frank 11/17/2005  **********/
    #ifdef LEADFLY
      attr = (mxml_attr_t *)npalloc(sizeof(mxml_attr_t));        
    #else  
    /********** End Frank 11/15/2005  **********/
      attr = malloc(sizeof(mxml_attr_t));
    /********** Add by Frank 11/17/2005  **********/
    #endif
    /********** End Frank 11/15/2005  **********/
       
  }
  else
  {
    /***** Add by Frank 11/22/2005 *******/
    #ifdef   LEADFLY               
      attr = (mxml_attr_t *)npalloc((node->value.element.num_attrs + 1) * sizeof(mxml_attr_t));               
      memcpy(attr, node->value.element.attrs, (node->value.element.num_attrs)*sizeof(mxml_attr_t)); 
      npfree(node->value.element.attrs);
         
    #else 
    /***** End Frank 11/22/2005 *******/  
      attr = (mxml_attr_t*)realloc(node->value.element.attrs,
                                  (node->value.element.num_attrs + 1) * sizeof(mxml_attr_t));
    /********** Add by Frank 11/16/2005 **********/                              
    #endif  
    /********** End Frank 11/16/2005 **********/ 
    
  } 
  
  if (!attr) 
  {
//    mxml_error("Unable to allocate memory for attribute '%s' in element %s!",
//               name, node->value.element.name);
    LOG(m_handler, ERROR, "Unable to allocate memory for attribute '%s' in element %s!\r\n",
               name, node->value.element.name);
    return;
  }

  node->value.element.attrs = attr;
  attr += node->value.element.num_attrs;

  attr->name = strdup(name);

  if (value)
    attr->value = strdup(value);
         
  else
    attr->value = NULL;

  if (!attr->name || (!attr->value && value))
  {
  
    if (attr->name){ 
       /***** Add by Frank 11/15/2005 *****/
      #ifdef LEADFLY     
        npfree(attr->name);
           
      #else 
      /***** End Frank 11/15/2005 *****/     
        free(attr->name);
      /***** Add by Frank 11/15/2005 *****/
      #endif      
      /***** End Frank 11/15/2005 *****/  
    }  
    if (attr->value){
      /***** Add by Frank 11/15/2005  *****/
      #ifdef LEADFLY
        npfree(attr->value);
      /***** End Frank 11/15/2005  *****/       
      #else
        free(attr->value);
      /***** Add by Frank 11/15/2005  *****/
      #endif
      /***** End Frank 11/15/2005  *****/  
    }  
//    mxml_error("Unable to allocate memory for attribute '%s' in element %s!",
//               name, node->value.element.name);
      LOG(m_handler, ERROR, "Unable to allocate memory for attribute '%s' in element %s!\r\n",
                 name, node->value.element.name);
    return;
  }
    
  node->value.element.num_attrs ++;
}


/*
 * End of "$Id: mxml-attr.c,v 1.1.1.1 2007-05-30 03:22:40 andyy Exp $".
 */
