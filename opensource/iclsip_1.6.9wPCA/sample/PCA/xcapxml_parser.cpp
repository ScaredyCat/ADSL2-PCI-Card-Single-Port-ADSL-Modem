/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 *
 * 
 * xcapxml_parser.c
 *
 */

/* We use standard I/O for debugging.  */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>


/* It is useful to test things that ``must'' be true when debugging.  */
/*
#ifndef _WIN32_WCE
	#include <assert.h>
#endif
*/
#include "xcapxml_parser.h"
#include "xmlparse.h"
#include "xcap_datatype.h"

TagType CurrentTag=XCAP_NULL;
//XML_Parser p;
//Eventcnt = 0;
BuddyList *CurrentPtr;
int isUsedBuddyList = 0;
int isGroupSet=-1;
int isUsedRuleList;

char* UTF8ToBig5(const char* str,int n)
{
	wchar_t wcs[512];
	char  cstr[1024];

	memset(wcs,0,sizeof(wcs));
	memset(cstr,0,sizeof(cstr));
	if (str == 0)
		return "";

	int length = MultiByteToWideChar(CP_UTF8,0,str,n,wcs,sizeof(wcs));

	length = WideCharToMultiByte(GetACP(),0,wcs,length,cstr,sizeof(cstr),NULL,NULL);
	if ((length < 1024)&&(length > 0))
		return cstr;
	else
		return "";
}

void default_hndl(void *data, const char *s, int len)
{
//  fwrite(s, len, sizeof(char), stdout);
}

void printcurrent(XML_Parser p)
{
	XML_SetDefaultHandler(p, default_hndl);
	XML_DefaultCurrent(p);
	XML_SetDefaultHandler(p, (XML_DefaultHandler) 0);
}

void proc_hndl(void *data, const char *target, const char *pidata)
{
	//printf("\n%4d: Processing Instruction - ", Eventcnt++);
	//printcurrent(p);
}

void char_hndl(void *data, const char *txt, int txtlen)
{
	char* tmpstr = (char*)malloc(sizeof(char) * (txtlen + 1) );
	strncpy(tmpstr, txt, txtlen);
	tmpstr[txtlen] = '\0';

	BuddyList *bdHead = (BuddyList *)data;
	BuddyList *CurrentBuddy = bdHead;

	if(CurrentBuddy==NULL)
  		return;
	//find the current buddy
	while(CurrentBuddy->next!=NULL)
		CurrentBuddy = CurrentBuddy->next;
  
  
	switch (CurrentTag) {
//	case XCAP_ID:
//		strcpy(CurrentBuddy->buddy->m_id,tmpstr);
//		break;

	case XCAP_Pic:
		strcpy(CurrentBuddy->buddy->m_pic, UTF8ToBig5(tmpstr,-1));
		break;

	case XCAP_DISPLAY_NAME:
		strcpy(CurrentBuddy->buddy->m_displayname, UTF8ToBig5(tmpstr,-1));
		break;

	case XCAP_Department:
		strcpy(CurrentBuddy->buddy->m_department, UTF8ToBig5(tmpstr,-1));
		break;

	case XCAP_Division:
		strcpy(CurrentBuddy->buddy->m_division, UTF8ToBig5(tmpstr,-1));
		break;

	case XCAP_Remark:
		strcpy(CurrentBuddy->buddy->m_remark, UTF8ToBig5(tmpstr,-1));
		break;

	case XCAP_Tel:
		strcpy(CurrentBuddy->buddy->m_telephone,tmpstr);
		break;
  
	default:
		break;
	}

	CurrentTag = XCAP_NULL;
	free(tmpstr);
}

void startElement(void *userData, const char *name, const char **atts)
{
	char* nsname;
  	char* element;
	char* tmpstr = strdup(name);
	static char currentGroup[64];
	BuddyList *CurrentBuddy = (BuddyList *)userData;
	int i;
	
	if (CurrentBuddy!=NULL)//find the last buddy in the list
		while (CurrentBuddy->next!=NULL)
			CurrentBuddy = CurrentBuddy->next;

	nsname = strtok(tmpstr, "|");
	element = strtok(NULL, "| ");
	//element = nsname;
	
	if (element==NULL) {
		free(tmpstr);
		return;
	}
	if (_stricmp(element, "list") == 0) {
		for (i = 0; atts[i]; i += 2) {
			if (_stricmp(atts[i], "name") == 0) {
				strcpy(currentGroup,atts[i+1]);
				isGroupSet = 1;
				if (!isUsedBuddyList)
					isUsedBuddyList = 1;
				else {
					CurrentBuddy->next = NewBuddyList();
					CurrentBuddy = CurrentBuddy->next;
				}

				//only store group name
				FreeBuddy(CurrentBuddy->buddy);
				strcpy(CurrentBuddy->group,currentGroup);
				CurrentBuddy->buddy = NULL;
			}
		}
	}
	
	if (_stricmp(element, "entry") == 0) {

		if (!isUsedBuddyList)
			isUsedBuddyList = 1;
		else {
			CurrentBuddy->next = NewBuddyList();
			CurrentBuddy = CurrentBuddy->next;
		}
		if (isGroupSet)
			strcpy(CurrentBuddy->group,currentGroup);
		else
			strcpy(CurrentBuddy->group,DEFAULT_GROUPNAME);
		isGroupSet = -1;
		for (i = 0; atts[i]; i += 2) {
			if (_stricmp(atts[i], "uri") == 0) {
				strcpy(CurrentBuddy->buddy->m_uri,atts[i+1]);
			}
		}
	}
	
//	if (_stricmp(element, "ID") == 0) 
//		CurrentTag = XCAP_ID;

	if (_stricmp(element, "display-name") == 0) 
		CurrentTag = XCAP_DISPLAY_NAME;

	if (_stricmp(element, "Department") == 0) 
		CurrentTag = XCAP_Department;
	
	if (_stricmp(element, "Division") == 0) 
		CurrentTag = XCAP_Division;

	if (_stricmp(element, "Remark") == 0) 
		CurrentTag = XCAP_Remark;

	if (_stricmp(element, "Tel") == 0) 
		CurrentTag = XCAP_Tel;
	
	if (_stricmp(element, "Pic") == 0) 
		CurrentTag = XCAP_Pic;

	free(tmpstr);
}

void endElement(void *userData, const char *name)
{
}

void ns_start(void *data, const char *prefix, const char *uri)
{
}

void ns_end(void *data, const char *prefix)
{
}

BuddyList *FromXMLtoBuddyList(const char* xmldata, size_t len)
{
	XML_Parser parser = XML_ParserCreateNS(NULL, '|');
	BuddyList *bdlist = NewBuddyList();

	isUsedBuddyList = 0;
	isGroupSet = -1;

	char *tmp_ptr = strstr(xmldata,"xml version");
	if(tmp_ptr == NULL)//no contained <?xml version....>
  		tmp_ptr = (char *)xmldata;
	else
  		tmp_ptr = strstr(xmldata,"<resource-lists");
  	
	XML_SetUserData(parser, (void *)bdlist);
	XML_SetElementHandler(parser, startElement, endElement);
	XML_SetCharacterDataHandler(parser, char_hndl);
	XML_SetProcessingInstructionHandler(parser, proc_hndl);
	XML_SetNamespaceDeclHandler(parser, ns_start, ns_end);

	int tmp_len = strlen(tmp_ptr);
	if (!XML_Parse(parser, tmp_ptr, tmp_len, 1)) {
		fprintf( stderr, 
			 "%s at line %d\n",
			 XML_ErrorString(XML_GetErrorCode(parser)),
			 XML_GetCurrentLineNumber(parser));
	}
	XML_ParserFree(parser);

	if (!isUsedBuddyList) {
		FreeBuddyList(bdlist);
		return NULL;
	}

	return bdlist;
}

char* FromPREStoXML()
{
	return NULL;
}

/**
 * parser for authorization rule
 */
void    char_hndl_4rule(void *data, const char *txt, int txtlen)
{
	char* tmpstr = (char*)malloc(sizeof(char) * (txtlen + 1) );
	strncpy(tmpstr, txt, txtlen);
	tmpstr[txtlen] = '\0';

	RuleList *rHead = (RuleList *)data;
	RuleList *CurrentRule = rHead;

        if ( CurrentRule == NULL ) {
		free(tmpstr);
		return;
	}
	if ( strlen(tmpstr) == 0 ) {
		free(tmpstr);
		return;
	}
	//find the current buddy
	while( CurrentRule->next!=NULL )
		CurrentRule = CurrentRule->next;
	switch (CurrentTag) {
	case RULE_ID:
		memset(CurrentRule->rule->m_id,0,64);
		strcpy(CurrentRule->rule->m_id,tmpstr);
		break;
	case RULE_Action:
		memset(CurrentRule->rule->m_action,0,64);
		strcpy(CurrentRule->rule->m_action,tmpstr);
		break;
	default:
		break;
	}
	CurrentTag = XCAP_NULL;
	free(tmpstr);

}

void	startElement_4rule(void *userData, const char *name, const char **atts)
{
	char* nsname;
	char* element;
	char* tmpstr = strdup(name);
	RuleList *rhead = (RuleList *)userData;
	RuleList *CurrentRule = rhead;
        
	if (CurrentRule!=NULL)	//find the last buddy in the list
		while(CurrentRule->next!=NULL)
			CurrentRule = CurrentRule->next;
	nsname = strtok(tmpstr, "|");
	element = strtok(NULL, "| ");
//	element = nsname;
	if (element==NULL) {
		free(tmpstr);
		return;
	}
	if (_stricmp(element, "rule") == 0) {
		if (!isUsedRuleList) {
			isUsedRuleList = 1;
			//do nothing..
		}
		else {
			CurrentRule->next = NewRuleList();
			CurrentRule = CurrentRule->next;
		}
		if ( _stricmp(atts[0], "id") == 0)
			CurrentRule->ruleid = atoi(atts[1]);
	}
	if (_stricmp(element, "id") == 0) 
		CurrentTag = RULE_ID;

	if (_stricmp(element, "sub-handling") == 0) 
		CurrentTag = RULE_Action;

	free(tmpstr);
}

RuleList*  FromXMLtoRuleList( const char* xmldata, size_t len )
{
	XML_Parser parser = XML_ParserCreateNS(NULL, '|');
	
	RuleList *rlist = NewRuleList();
	isUsedRuleList = 0;

	char *tmp_ptr = strstr(xmldata,"xml version");
	if(tmp_ptr == NULL) //no contained <?xml version....>
		tmp_ptr = (char *)xmldata;
	else
		tmp_ptr = strstr(xmldata,"<cr:ruleset");
	XML_SetUserData(parser, (void *)rlist);
	XML_SetElementHandler(parser, startElement_4rule, endElement);
	XML_SetCharacterDataHandler(parser, char_hndl_4rule);
	XML_SetProcessingInstructionHandler(parser, proc_hndl);
	XML_SetNamespaceDeclHandler(parser, ns_start, ns_end);
	   
	int tmp_len = strlen(tmp_ptr);
	char tmp1[256];
	int tmp2;
	if (!XML_Parse(parser, tmp_ptr, tmp_len, 1)) {
//		fprintf( stderr, 
//			 "%s at line %d\n",
//			 XML_ErrorString(XML_GetErrorCode(parser)),
//			 XML_GetCurrentLineNumber(parser));
		strcpy(tmp1,XML_ErrorString(XML_GetErrorCode(parser)));
		tmp2 = XML_GetCurrentLineNumber(parser);
	}

	XML_ParserFree(parser);
	if (!isUsedRuleList) {
		FreeRuleList(rlist);
		return NULL;
	}
	return rlist;
}


