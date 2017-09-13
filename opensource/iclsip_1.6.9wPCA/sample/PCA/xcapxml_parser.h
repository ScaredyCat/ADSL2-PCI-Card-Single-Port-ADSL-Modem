#ifndef _XCAPXMLPARSER
#define _XCAPXMLPARSER

#include <stdio.h>
#include "xcap_datatype.h"


BuddyList *FromXMLtoBuddyList(const char* xmldata, size_t len);
char* FromPREStoXML();

RuleList*  FromXMLtoRuleList( const char* xmldata, size_t len );

#endif
