#include <stdio.h>
#include "xcap_low.h"
#include "xcapapi.h"
#include "xcap_datatype.h"
#include "xcapxml_parser.h"

#define BUF_SIZE 65535
#define DEFAULT_OWNERNAME_LEN 32
#define DEFAULT_URI_DOC_LEN	256
#define DEFAULT_URI_NODE_LEN	256
#define DEFAULT_URI_LEN	512

#define XCAP_INI_FILE_NAME "PCAUA.ini"

char docOwner[DEFAULT_OWNERNAME_LEN];

unsigned int _g_connect_times=0;
bool isInitSet = false;
bool isLMSConnectSuccess = true;

char msg_hdr[1024],recv_buf[BUF_SIZE];
char uri[DEFAULT_URI_LEN],uri_doc[DEFAULT_URI_DOC_LEN],uri_node[DEFAULT_URI_NODE_LEN];

//add for caching XCAP data
bool _isResourceListModified = true;
bool _isPresRulesModified = true;
BuddyList *_cachedResLst = NULL;
RuleList *_cachedPresRules = NULL;

void SetPresRulesCache(RuleList *setPresRules)
{
	_cachedPresRules = setPresRules;
}

void SetResLstCache(BuddyList *setResLst)
{
	_cachedResLst = setResLst;
}

BuddyList *GetResLstCache()
{
	return _cachedResLst;
}

RuleList *GetPresRulesCache()
{
	return _cachedPresRules;
}

void ClearPresRulesCache()
{
	if(_cachedPresRules!=NULL)
	{
		FreeRuleList(_cachedPresRules);
		_cachedPresRules = NULL;

	}
}

void ClearResLstCache()
{
	if(_cachedResLst!=NULL)
	{
		FreeBuddyList(_cachedResLst);
		_cachedResLst = NULL;
	}
}

BuddyList *DupResLst(BuddyList *srcLst)
{
	BuddyList *pCurrentSrc= srcLst;
	BuddyList *pTop=NULL,*pCurrentDst=NULL,*pTmp;
	//dup list data to cache
	while(pCurrentSrc!=NULL)
	{
		if(pTop==NULL)
		{
			pTop =NewBuddyList();
			if(pCurrentSrc->buddy!=NULL)
			{
//				pTop->buddy= NewBuddy();
				memcpy(pTop->buddy,pCurrentSrc->buddy,sizeof(BuddyInfo));
			}
			else
			{
				FreeBuddy(pTop->buddy);
				pTop->buddy = NULL;

			}
			memcpy(pTop->group,pCurrentSrc->group,DEFAULT_GROUPNAME_SIZE*sizeof(char));
			pCurrentDst = pTop;
		}
		else 
		{
			pCurrentDst->next = NewBuddyList();
			pTmp= pCurrentDst->next;
			if(pCurrentSrc->buddy!=NULL)
			{
//				pTmp->buddy = NewBuddy();
				memcpy(pTmp->buddy,pCurrentSrc->buddy,sizeof(BuddyInfo));
			}
			else
			{
				FreeBuddy(pTmp->buddy);
				pTmp->buddy = NULL;
			}
			memcpy(pTmp->group,pCurrentSrc->group,DEFAULT_GROUPNAME_SIZE*sizeof(char));
			pCurrentDst = pCurrentDst->next;
		}
		pCurrentSrc = pCurrentSrc->next;
	}
	return pTop;
}

RuleList *DupPresRule(RuleList *srcLst)
{
	RuleList *pCurrentSrc= srcLst;
	RuleList *pTop=NULL,*pCurrentDst=NULL,*pTmp;
	//dup list data to cache
	while(pCurrentSrc!=NULL)
	{
		if(pTop==NULL)
		{
			pTop =NewRuleList();
			if(pCurrentSrc->rule!=NULL)
			{
				pTop->rule = NewRule();
				memcpy(pTop->rule,pCurrentSrc->rule,sizeof(Rule));
			}
			pTop->ruleid = pCurrentSrc->ruleid;
			pCurrentDst = pTop;
		}
		else 
		{
			pCurrentDst->next = NewRuleList();
			pTmp= pCurrentDst->next;
			if(pCurrentSrc->rule!=NULL)
			{
				pTmp->rule = NewRule();
				memcpy(pTmp->rule,pCurrentSrc->rule,sizeof(Rule));
			}
			pTmp->ruleid = pCurrentSrc->ruleid;
			pCurrentDst = pCurrentDst->next;
		}
		pCurrentSrc = pCurrentSrc->next;
	}

	return pTop;
}

bool UpdatePresRulesCache(RuleList *srcLst)
{
	if(srcLst==NULL)
		return false;

	ClearPresRulesCache();
	SetPresRulesCache(DupPresRule(srcLst));
	_isPresRulesModified = false;
	return true;
}

bool UpdateResourceLstCache(BuddyList *srcLst)
{
	if(srcLst==NULL)
		return false;

	ClearResLstCache();
	SetResLstCache(DupResLst(srcLst));
	_isResourceListModified = false;
	return true;
}

//dup a rule list from cache
RuleList *GetAllRuleFromCache()
{
	return DupPresRule(GetPresRulesCache());

}
//dup a buddy list from cache
BuddyList *GetAllBuddyListFromCache()
{
	return DupResLst(GetResLstCache());
}



void ClearCache()
{
	ClearResLstCache();
	ClearPresRulesCache();
	_isResourceListModified = true;
	_isPresRulesModified = true;
}

bool GetXCAPSetting(char *s_ip,unsigned int s_port)
{
	FILE *stream;

	if((stream = fopen(XCAP_INI_FILE_NAME,"r"))==NULL)
		return false;

	char ini_buf[BUF_SIZE];

	int ini_len = fread(ini_buf,sizeof(char),BUF_SIZE,stream);
	fclose(stream);
	
	return true;
	
}

void SetOwnerName(char *_owner)
{
	strcpy(docOwner,_owner);
}

char *GetOwnerName()
{
	return docOwner;
}

void ConfigXCAPServer(char *s_ip,unsigned int s_port)
{

	
	SetXCAPServer(s_ip,s_port);
}
//return socket descriptor
int  ConnectToServer()
{
	int sd;
	init();
	if((sd =doConnect())<0)
	{
		WSACleanup();
		isLMSConnectSuccess = false;
		return -1;
	}
	isLMSConnectSuccess = true;
	return sd;	
}

bool DisConnect(int sd)
{
	closeSock(sd);
	WSACleanup();
		
	return true;
}

//do necessary first...
//check group existence
// 0: not exist, 1: exist, 2: server error
int isGroupExist(char *groupname)
{
	char tmp_buf[64];
	int iRet=0;
	int sd;
	char *owner = docOwner;
	if(!isLMSConnectSuccess)
		return SERVER_ERROR;
	if((sd = ConnectToServer())<=0)
		return SERVER_ERROR;
	
	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);
	memset(tmp_buf,0,64);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);
	if(groupname ==NULL)
		sprintf(tmp_buf,"list[@name=\"%s\"]",DEFAULT_GROUPNAME);
	else
		sprintf(tmp_buf,"list[@name=\"%s\"]",groupname);
	
	composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);
	composeURINodeSel(uri_node, NULL, tmp_buf, NULL, NULL);

	sprintf(uri,"GET /%s/~~/%s HTTP/1.1\r\n",uri_doc,uri_node);
	int hdr_len = fillGETReqHdr( msg_hdr,uri);

	iRet = sendGETReq(msg_hdr,recv_buf,sd);
	DisConnect(sd);

	switch (iRet) {
	case 200:
		return GRP_EXIST;
	case 404:
		return GRP_NOT_FOUND;
	case 500:
	default:
		return SERVER_ERROR;
	}
}

bool CreateContactDoc()
{
	int sd;
	if(!isLMSConnectSuccess)
		return false;
	if((sd = ConnectToServer())<=0)
		return false;

	int iRet;
	char *owner = docOwner;

	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);

	composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);

	sprintf(uri,"PUT /%s HTTP/1.1\r\n",uri_doc);

	static char XMLcnt[1024],tmp_buf[256];
	memset(XMLcnt,0,1024);
	memset(tmp_buf,0,256);

	//compose the xml content for creating default document and list
	strcat(XMLcnt,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
	strcat(XMLcnt,"<resource-lists xmlns=\"urn:ietf:params:xml:ns:resource-lists\">\r\n");
		
	sprintf(tmp_buf,"<list name=\"%s\" uri=\"sip:%s@example.com\">\r\n",DEFAULT_GROUPNAME,DEFAULT_GROUPNAME);
   	strcat(XMLcnt,tmp_buf);
	
     	strcat(XMLcnt,"</list>\r\n");
		
       strcat(XMLcnt,"</resource-lists>\r\n");

   	int hdr_len = fillPUTReqHdr( msg_hdr,uri,strlen(XMLcnt));
	if((iRet = sendPUTReq(msg_hdr,XMLcnt,recv_buf,sd)) > 201)
	{
		DisConnect(sd);
		return false;
	}

	DisConnect(sd);
	_isResourceListModified = true;
	return true;
}

int GetListCount(BuddyList *bdlist)
{
	if (bdlist==NULL)
		return 0;
	int ret=0;
	BuddyList *current = bdlist;

	while (current!=NULL) {
		current = current->next;
		ret++;
	}
	return ret;
}

bool ModifyBuddy(BuddyInfo *modifybuddy,char *groupname)
{
	//if buddy is chaned to another group....delete buddy from original group and add it in the new group
	//if the group is not chaned ....do the same thing as add buddy...

	//find original group
	char *owner = docOwner;
	BuddyList *tmpBuddyList = SearchBuddyByTel(modifybuddy->m_telephone);
	if(!tmpBuddyList)//not found..
		return false;

	if(!stricmp(tmpBuddyList->group,groupname))//group do not changed
	{
		FreeBuddyList(tmpBuddyList);
		return AddBuddyIntoList(modifybuddy,groupname);
	}

	if(!DelBuddy(modifybuddy->m_uri) )//del failed
	{
		FreeBuddyList(tmpBuddyList);
		return false;
	}
	FreeBuddyList(tmpBuddyList);
	return AddBuddyIntoList(modifybuddy,groupname);
}

bool DelBuddy(char *buddyuri)
{
	if (buddyuri==NULL)
		return false;

	if (!isLMSConnectSuccess)
		return false;
	int sd;
	if ((sd = ConnectToServer())<= 0)
		return false;
	//compose uri...

	char tmp_buf[256];
	int iRet;
	char *owner = docOwner;

	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);
	
	sprintf(tmp_buf,"entry[@uri=\"%s\"]", buddyuri);
	composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);
	composeURINodeSel(uri_node, NULL, "list", tmp_buf, NULL);

	sprintf(uri,"DELETE /%s/~~/%s HTTP/1.1\r\n",uri_doc,uri_node);

	int hdr_len = fillDelReqHdr( msg_hdr, uri );
	
	if ((iRet = sendDELReq(msg_hdr, recv_buf, sd))!=200) {
		DisConnect(sd);
		return false;
	}
	_isResourceListModified = true;
	DisConnect(sd);
	return true;
}


bool CreateGroup(char *groupname)
{
	char *owner = docOwner;
	char g_name[32];
	if(owner ==NULL)
		return false;

	if(!isLMSConnectSuccess)
		return false;
	int sd;
	if((sd = ConnectToServer())<=0)
		return false;
	//compose uri...

	char tmp_buf[256];
	int iRet;
	
	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);

	sprintf(tmp_buf,"list[@name=\"%s\"]",groupname);
	composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);
	composeURINodeSel(uri_node, "resource-lists", tmp_buf, NULL, NULL);

	sprintf(uri,"PUT /%s/~~/%s HTTP/1.1\r\n",uri_doc,uri_node);

	static char XMLcnt[BUF_SIZE];
	memset(XMLcnt,0,BUF_SIZE);
	memset(tmp_buf,0,256);

	if(groupname==NULL)
		strcpy(g_name,DEFAULT_GROUPNAME);
	else
		strcpy(g_name,groupname);
	
	//compose xml content according to the buddy content
	sprintf(tmp_buf,"<list name=\"%s\" uri=\"sip:%s@example.com\">\r\n",g_name,g_name);
   	strcat(XMLcnt,tmp_buf);
	strcat(XMLcnt,"</list>\r\n");

	
	int hdr_len = fillPUTReqHdr( msg_hdr,uri,strlen(XMLcnt));
	
	if((iRet = sendPUTReq(msg_hdr,XMLcnt,recv_buf,sd)) > 201)
	{
		DisConnect(sd);
		return false;
	}
	_isResourceListModified = true;
	DisConnect(sd);
	return true;
	
}

//add buddy into buddy list
bool AddBuddyIntoList(BuddyInfo *addbuddy,char *groupname)
{
	char g_name[32];
	char *owner = docOwner;
	int sd;

	if(addbuddy==NULL || owner==NULL)
		return false;
	
	memset(g_name,0,32);
	if(groupname==NULL || !strcmp(groupname,DEFAULT_GROUPNAME))
		//using default group	
		strcpy(g_name,DEFAULT_GROUPNAME);
	else
		strcpy(g_name,groupname);	
	if (isGroupExist(g_name) != GRP_EXIST)
		return false;
	if (!isLMSConnectSuccess)
		return false;
	if ((sd = ConnectToServer())<0)
		return false;
	//compose uri...

	char tmp_buf[256],tmp_buf1[256];
	int iRet;

	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);
	memset(tmp_buf,0,256);
	memset(tmp_buf1,0,256);

	sprintf(tmp_buf,"list[@name=\"%s\"]",g_name);
	sprintf(tmp_buf1,"entry[@uri=\"%s\"]",addbuddy->m_uri);
	composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);
	composeURINodeSel(uri_node, NULL, tmp_buf, tmp_buf1, NULL);

	sprintf(uri,"PUT /%s/~~/%s HTTP/1.1\r\n",uri_doc,uri_node);

	static char XMLcnt[BUF_SIZE];
	memset(XMLcnt,0,BUF_SIZE);
	memset(tmp_buf,0,256);
	
	//compose xml content according to the buddy content
	sprintf(tmp_buf,"<entry uri=\"%s\">\r\n",addbuddy->m_uri);
   	strcat(XMLcnt,tmp_buf);

	if (addbuddy->m_department[0] != 0) {
		sprintf(tmp_buf, "<Department>%s</Department>\r\n", addbuddy->m_department);
   		strcat(XMLcnt,tmp_buf);
	}
	if (addbuddy->m_division[0] != 0) {
		sprintf(tmp_buf, "<Division>%s</Division>\r\n", addbuddy->m_division);
   		strcat(XMLcnt,tmp_buf);
	}
	if (addbuddy->m_remark[0] != 0) {
		sprintf(tmp_buf,"<Remark>%s</Remark>\r\n", addbuddy->m_remark);
   		strcat(XMLcnt,tmp_buf);
	}
	if (addbuddy->m_telephone[0] != 0) {
		sprintf(tmp_buf,"<Tel>%s</Tel>\r\n", addbuddy->m_telephone);
   		strcat(XMLcnt,tmp_buf);
	}
	if (addbuddy->m_pic[0] != 0) {
		sprintf(tmp_buf,"<Pic>%s</Pic>\r\n",addbuddy->m_pic);
   		strcat(XMLcnt,tmp_buf);
	}
	if (addbuddy->m_displayname[0] != 0) {
		sprintf(tmp_buf, "<display-name>%s</display-name>\r\n", addbuddy->m_displayname);
   		strcat(XMLcnt,tmp_buf);
	}
     	strcat(XMLcnt,"</entry>\r\n");
       
	
	int hdr_len = fillPUTReqHdr( msg_hdr,uri,strlen(XMLcnt));
	
	if((iRet = sendPUTReq(msg_hdr,XMLcnt,recv_buf,sd)) > 201)
	{
		DisConnect(sd);
		return false;
	}
	_isResourceListModified = true;
	DisConnect(sd);
	return true;
	
}


//get the number of the buddy in list
int GetBuddyCount()
{
	char tmp_buf[256];
	int iRet=0;
	int BuddyCount = -1;
	char *owner = docOwner;
	BuddyList *bdlist = NULL;
	if(!isLMSConnectSuccess)
		return false;

	if(_isResourceListModified )
	{
		int sd;
		if((sd = ConnectToServer())<=0)
			return BuddyCount;
		memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
		memset(uri_node,0,DEFAULT_URI_NODE_LEN);
		memset(tmp_buf,0,256);
		memset(uri,0,DEFAULT_URI_LEN);
		memset(msg_hdr,0,1024);
		memset(recv_buf,0,BUF_SIZE);
	
		composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);

		sprintf(uri,"GET /%s HTTP/1.1\r\n",uri_doc);

		int hdr_len = fillGETReqHdr( msg_hdr,uri);

		if((iRet = sendGETReq(msg_hdr,recv_buf,sd))!=200)
		{
			DisConnect(sd);
			return BuddyCount;
		}
		DisConnect(sd);
		bdlist = FromXMLtoBuddyList(recv_buf,strlen(recv_buf)) ;
		
		UpdateResourceLstCache(bdlist);
		
	}
	else
	{
		//get from cache
		bdlist = DupResLst(GetResLstCache());
	}
	//parse the recv_buf to get the buddy count....

	BuddyCount = 0;
	BuddyList *tmpPtr = bdlist;
	if(bdlist!=NULL)
	{
		BuddyCount++;
		while(tmpPtr->next!=NULL)
		{
			if(tmpPtr->buddy==NULL)//only store group name
			{
				tmpPtr = tmpPtr->next;
				continue;
			}
			tmpPtr = tmpPtr->next;
			BuddyCount++;
		}
		FreeBuddyList(bdlist);
	}
	return BuddyCount;
}

char *test_DefaultGroupBuddy()
{
	static char tmpxml[1500],tmp_buf[256];

	memset(tmpxml,0,1500);

		
	sprintf(tmp_buf,"<list name=\"%s\" uri=\"sip:%s@example.com\" subscribeable=\"true\">\r\n",DEFAULT_GROUPNAME,DEFAULT_GROUPNAME);
   	strcat(tmpxml,tmp_buf);

	strcat(tmpxml,"<entry name=\"alan\" uri=\"alan@example\">\r\n");
	strcat(tmpxml,"<ID>900183</ID>\r\n");
	strcat(tmpxml,"<Department>CCL</Department>\r\n");
	strcat(tmpxml,"<Division>k200</Division>\r\n");
	strcat(tmpxml,"</entry>\r\n");
	strcat(tmpxml,"<entry name=\"alan2\" uri=\"alan2@example\">\r\n");
	strcat(tmpxml,"<ID>9001832</ID>\r\n");
	strcat(tmpxml,"<Department>CCL2</Department>\r\n");
	strcat(tmpxml,"<Division>k2002</Division>\r\n");
	strcat(tmpxml,"</entry>\r\n");

	strcat(tmpxml,"</list>\r\n");
/*
	sprintf(tmp_buf,"<list name=\"%s\" uri=\"sip:%s@example.com\" subscribeable=\"true\">\r\n","Friends","Friends");
   	strcat(tmpxml,tmp_buf);

	strcat(tmpxml,"<entry name=\"alan3\" uri=\"alan@example\">\r\n");
	strcat(tmpxml,"<ID>9001833</ID>\r\n");
	strcat(tmpxml,"<Department>CCL3</Department>\r\n");
	strcat(tmpxml,"<Division>k2003</Division>\r\n");
	strcat(tmpxml,"</entry>\r\n");
	strcat(tmpxml,"<entry name=\"alan4\" uri=\"alan4@example\">\r\n");
	strcat(tmpxml,"<ID>9001834</ID>\r\n");
	strcat(tmpxml,"<Department>CCL4</Department>\r\n");
	strcat(tmpxml,"<Division>k2004</Division>\r\n");
	strcat(tmpxml,"</entry>\r\n");

	strcat(tmpxml,"</list>\r\n");
		
       strcat(tmpxml,"</resource-lists>\r\n");
*/
	return tmpxml;


}

char *test_Allcontactlist_xml()
{
	static char tmpxml[1500],tmp_buf[256];

	memset(tmpxml,0,1500);

	strcat(tmpxml,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
	strcat(tmpxml,"<resource-lists xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\r\n");
		
	sprintf(tmp_buf,"<list name=\"%s\" uri=\"sip:%s@example.com\" subscribeable=\"true\">\r\n",DEFAULT_GROUPNAME,DEFAULT_GROUPNAME);
   	strcat(tmpxml,tmp_buf);

	strcat(tmpxml,"<entry name=\"alan\" uri=\"alan@example\">\r\n");
	strcat(tmpxml,"<ID>900183</ID>\r\n");
	strcat(tmpxml,"<Department>CCL</Department>\r\n");
	strcat(tmpxml,"<Division>k200</Division>\r\n");
	strcat(tmpxml,"</entry>\r\n");
	strcat(tmpxml,"<entry name=\"alan2\" uri=\"alan2@example\">\r\n");
	strcat(tmpxml,"<ID>9001832</ID>\r\n");
	strcat(tmpxml,"<Department>CCL2</Department>\r\n");
	strcat(tmpxml,"<Division>k2002</Division>\r\n");
	strcat(tmpxml,"</entry>\r\n");

	strcat(tmpxml,"</list>\r\n");

	sprintf(tmp_buf,"<list name=\"%s\" uri=\"sip:%s@example.com\" subscribeable=\"true\">\r\n","Friends","Friends");
   	strcat(tmpxml,tmp_buf);

	strcat(tmpxml,"<entry name=\"alan3\" uri=\"alan@example\">\r\n");
	strcat(tmpxml,"<ID>9001833</ID>\r\n");
	strcat(tmpxml,"<Department>CCL3</Department>\r\n");
	strcat(tmpxml,"<Division>k2003</Division>\r\n");
	strcat(tmpxml,"</entry>\r\n");
	strcat(tmpxml,"<entry name=\"alan4\" uri=\"alan4@example\">\r\n");
	strcat(tmpxml,"<ID>9001834</ID>\r\n");
	strcat(tmpxml,"<Department>CCL4</Department>\r\n");
	strcat(tmpxml,"<Division>k2004</Division>\r\n");
	strcat(tmpxml,"</entry>\r\n");

	strcat(tmpxml,"</list>\r\n");
		
       strcat(tmpxml,"</resource-lists>\r\n");

	return tmpxml;
}

//get All Buddies 
BuddyList *GetAllBuddyList()
{
	char tmp_buf[256];
	int iRet=0;
	int BuddyCount = -1;
	char *owner = docOwner;
	BuddyList *retList;

	if(!isLMSConnectSuccess)
		return false;

	if(_isResourceListModified)
	{
		int sd;
		if((sd = ConnectToServer())<= 0)
			return NULL;
		memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
		memset(uri_node,0,DEFAULT_URI_NODE_LEN);
		memset(tmp_buf,0,256);
		memset(uri,0,DEFAULT_URI_LEN);
		memset(msg_hdr,0,1024);
		memset(recv_buf,0,BUF_SIZE);
	
	
		composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);

		sprintf(uri,"GET /%s HTTP/1.1\r\n",uri_doc);

		int hdr_len = fillGETReqHdr( msg_hdr,uri);

		if((iRet = sendGETReq(msg_hdr,recv_buf,sd))!=200)
		{
			DisConnect(sd);
			return NULL;
		}
		//parse the recv_buf to get the all buddy list....
		retList = FromXMLtoBuddyList(recv_buf,strlen(recv_buf));
		DisConnect(sd);

		UpdateResourceLstCache(retList);
		
	}
	else
	{
		retList = DupResLst(GetResLstCache());

	}
	return retList;
}

BuddyList *RemoveEmptyBuddyFromList(BuddyList *bdlist)
{
	if(bdlist==NULL)
		return NULL;

	BuddyList *currentPtr,*prePtr;
	currentPtr = prePtr = bdlist;
	while(currentPtr != NULL)
	{
		if(currentPtr->buddy==NULL)
		{
			if(currentPtr == prePtr)
			{
				bdlist = currentPtr->next;
				free(currentPtr);
				currentPtr = prePtr = bdlist;
			}
			else
			{
				prePtr->next = currentPtr->next;
				free(currentPtr);
				currentPtr = prePtr->next;
			}
		}
		else
		{
			prePtr = currentPtr;
			currentPtr = currentPtr->next;
		}
	}
	
	return bdlist;
}

//del by buddy telephone, this is unique
BuddyList *DelBuddyFromList(BuddyList *bdlist,char *tel)
{
	if (bdlist ==NULL)
		return NULL;
	if (tel==NULL)
		return NULL;
	BuddyList *currentPtr,*prePtr;
	currentPtr = prePtr = bdlist;

	if(!strcmp(currentPtr->buddy->m_telephone, tel))//head element is matched..
	{
		bdlist = bdlist->next;
		FreeBuddy(currentPtr->buddy);
		free(currentPtr);
		return bdlist;
	}

	while(currentPtr!=NULL)
	{
		if(!strcmp(currentPtr->buddy->m_telephone,tel))//matched
		{
			prePtr->next = currentPtr->next;
			FreeBuddy(currentPtr->buddy);
			free(currentPtr);
			return bdlist;
		}

		prePtr = currentPtr;
		currentPtr = currentPtr->next;
	}
	return bdlist;
}

BuddyList *GetGroupMembers(char *_groupname)
{

	if(_groupname==NULL)
		return NULL;
	BuddyList *allBuddyList = GetAllBuddyList();
	if(allBuddyList==NULL)
		return NULL;
	allBuddyList = RemoveEmptyBuddyFromList(allBuddyList);
	BuddyList *currentPtr = allBuddyList;
	char tmpname[64];
	while(currentPtr!=NULL)
	{
		if(_stricmp(currentPtr->group,_groupname))
		{
			strcpy(tmpname,currentPtr->buddy->m_telephone);
			currentPtr = currentPtr->next;
			allBuddyList = DelBuddyFromList(allBuddyList,tmpname);
		}
		else
			currentPtr = currentPtr->next;
	}

	return allBuddyList;
	

}

//when delete the group...you must move the member in this group to DEFAULT GROUP first
bool DelGroup(char *g_name)
{
	char *owner= docOwner;
	if( owner==NULL)
		return false;

	//compose uri...

	char tmp_buf[256];
	int iRet;

	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,2048);
	memset(recv_buf,0,BUF_SIZE);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);
	
//	move group members to DEFAULT GROUP first...
	BuddyList *groupMemList  =GetGroupMembers(g_name);

	if(groupMemList!=NULL)
	{
		BuddyList *currentPtr = groupMemList;

		while(currentPtr!=NULL)
		{
			if(!AddBuddyIntoList(currentPtr->buddy, DEFAULT_GROUPNAME))
			{
				FreeBuddyList(groupMemList);
				return false;
			}

			currentPtr = currentPtr->next;
		}
			
		FreeBuddyList(groupMemList);
	}

	if(!isLMSConnectSuccess)
		return false;	
	int sd;
	if((sd = ConnectToServer())<= 0)
		return false;
	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);
//	Delete the group now...
	sprintf(tmp_buf,"list[@name=\"%s\"]",g_name);
	composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);
	composeURINodeSel(uri_node, NULL, tmp_buf, NULL, NULL);

	sprintf(uri,"DELETE /%s/~~/%s HTTP/1.1\r\n",uri_doc,uri_node);

	int hdr_len = fillDelReqHdr( msg_hdr,uri);
	
	if((iRet = sendDELReq(msg_hdr,recv_buf,sd))!=200)
	{
		DisConnect(sd);
		return false;
	}

	_isResourceListModified = true;
	DisConnect(sd);
	return true;
}

bool ModifyGroupName(char *_orgGroupName, char *_newGroupName)
{
	char *owner= docOwner;
	if( owner==NULL)
		return false;

	//compose uri...

	char tmp_buf[256];
	int iRet;

	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);

	if (isGroupExist(_newGroupName) != GRP_EXIST)
		return false;
	if (!CreateGroup(_newGroupName))
		return false;
	
//	move group members to new GROUP ...
	BuddyList *groupMemList  =GetGroupMembers(_orgGroupName);

	if(groupMemList!=NULL)
	{
		BuddyList *currentPtr = groupMemList;

		while(currentPtr!=NULL)
		{
			if(!AddBuddyIntoList(currentPtr->buddy, _newGroupName))
			{
				FreeBuddyList(groupMemList);
				return false;
			}

			currentPtr = currentPtr->next;
		}
			
		FreeBuddyList(groupMemList);
	}

	if(!isLMSConnectSuccess)
		return false;		
	int sd;
	if((sd = ConnectToServer())<=0)
		return false;
	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);
//	Delete the group now...
	sprintf(tmp_buf,"list[@name=\"%s\"]",_orgGroupName);
	composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);
	composeURINodeSel(uri_node, NULL, tmp_buf, NULL, NULL);

	sprintf(uri,"DELETE /%s/~~/%s HTTP/1.1\r\n",uri_doc,uri_node);

	int hdr_len = fillDelReqHdr( msg_hdr,uri);
	
	if((iRet = sendDELReq(msg_hdr,recv_buf,sd))!=200)
	{
		DisConnect(sd);
		return false;
	}

	_isResourceListModified = true;
	DisConnect(sd);
	return true;
}

BuddyInfo* GetBuddyByUri(char *group, char *uri)
{
	char tmp_buf[256],tmp_buf1[256];
	int iRet=0;
	int BuddyCount = -1;
	char *owner= docOwner;
	if(!isLMSConnectSuccess)
		return false;
	int sd;
	if((sd = ConnectToServer())<=0)
		return NULL;
	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);
	memset(tmp_buf,0,256);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);
	
	sprintf(tmp_buf,"list[@name=\"%s\"]", group);
	sprintf(tmp_buf1,"entry[@uri=\"%s\"]", uri);
	composeURIDOCSel(uri_doc, NULL, NULL, owner, NULL);
	composeURINodeSel(uri_node, NULL, tmp_buf, tmp_buf1, NULL);

	sprintf(uri,"GET /%s/~~/%s HTTP/1.1\r\n",uri_doc,uri_node);

	int hdr_len = fillGETReqHdr( msg_hdr,uri);

	if((iRet = sendGETReq(msg_hdr,recv_buf,sd))!=200)
	{
		DisConnect(sd);
		return NULL;
	}

	//parse the recv_buf to get the all buddy list....
	BuddyList *retList = FromXMLtoBuddyList(recv_buf,strlen(recv_buf));

	if(retList==NULL)
	{
		DisConnect(sd);
		return NULL;
	}
	BuddyInfo *retBuddy = NewBuddy();

	memcpy(retBuddy,retList->buddy,sizeof(BuddyInfo));

	FreeBuddyList(retList);

	DisConnect(sd);
	return retBuddy;
}

char *GetBuddyAttr(char *group, char *uri, TagType attrtype)
{
	BuddyInfo *retbuddy = GetBuddyByUri(group, uri);
	if (retbuddy==NULL)
		return NULL;

	char *retattr = (char *)calloc(64*sizeof(char), sizeof(char));
	switch (attrtype) {
	case XCAP_DISPLAY_NAME:
		strcpy(retattr, retbuddy->m_displayname);
		break;

//	case XCAP_ID:
//		strcpy(retattr, retbuddy->m_id);
//		break;

	case XCAP_Department:
		strcpy(retattr, retbuddy->m_department);
		break;

	case XCAP_Division:
		strcpy(retattr, retbuddy->m_division);
		break;

	case XCAP_Remark:
		strcpy(retattr, retbuddy->m_remark);
		break;

	default:
		break;
	}

	FreeBuddy(retbuddy);
	return retattr;
}

BuddyList* SearchBuddyByTel(char *_tel)
{
	BuddyList *allBuddyList = GetAllBuddyList();
	if(allBuddyList ==NULL)
		return NULL;
	
	allBuddyList = RemoveEmptyBuddyFromList(allBuddyList);
	BuddyList *currentPtr=allBuddyList, *retBuddyList=NULL;

	while (currentPtr!=NULL) {
		if (!_stricmp(currentPtr->buddy->m_telephone, _tel)) {
			retBuddyList = NewBuddyList();
			memcpy(retBuddyList->buddy, currentPtr->buddy, sizeof(BuddyInfo));
			strcpy(retBuddyList->group, currentPtr->group);
			retBuddyList->next=NULL;
			break;
		}
		currentPtr = currentPtr->next;
	}

	FreeBuddyList(allBuddyList);
	return retBuddyList;
}

BuddyList* SearchBuddyByName(char *_name)
{
	BuddyList *allBuddyList = GetAllBuddyList();
	if (allBuddyList ==NULL)
		return NULL;

	allBuddyList = RemoveEmptyBuddyFromList(allBuddyList);
	BuddyList *currentPtr=allBuddyList, *retBuddyList=NULL;

	while (currentPtr!=NULL) {	
		if (!_stricmp(currentPtr->buddy->m_displayname, _name)) {
			retBuddyList = NewBuddyList();
			memcpy(retBuddyList->buddy, currentPtr->buddy, sizeof(BuddyInfo));
			strcpy(retBuddyList->group, currentPtr->group);
			retBuddyList->next=NULL;
			break;
		}
		currentPtr = currentPtr->next;
	}

	FreeBuddyList(allBuddyList);
	return retBuddyList;	
}

BuddyList *SearchBuddy(BuddyInfo *searchedBuddy)
{
	BuddyList *retBuddyList = GetAllBuddyList();
	char *owner = docOwner;

	if(retBuddyList==NULL)
		return NULL;
	BuddyList *currentPtr;
	char tmpbuf[64];
	retBuddyList = RemoveEmptyBuddyFromList(retBuddyList);

	if (searchedBuddy->m_uri[0] != 0) {
		currentPtr  = retBuddyList;
		while (currentPtr!=NULL) {
			if (currentPtr->buddy->m_uri[0] == 0) {	
				currentPtr = currentPtr->next;
				continue;
			}
			if (strstr(currentPtr->buddy->m_uri, searchedBuddy->m_uri)==NULL) {
				strcpy(tmpbuf,currentPtr->buddy->m_telephone);
				currentPtr = currentPtr->next;
				retBuddyList = DelBuddyFromList(retBuddyList,tmpbuf);
				continue;
			}

			currentPtr = currentPtr->next;
		}
	}

	if (searchedBuddy->m_displayname[0] != 0) {
		currentPtr  = retBuddyList;
		while (currentPtr!=NULL) {
			if (currentPtr->buddy->m_displayname[0] == 0) {	
				currentPtr = currentPtr->next;
				continue;
			}
			if (strstr(currentPtr->buddy->m_displayname, searchedBuddy->m_displayname)==NULL) {
				strcpy(tmpbuf,currentPtr->buddy->m_telephone);
				currentPtr = currentPtr->next;
				retBuddyList = DelBuddyFromList(retBuddyList,tmpbuf);
				continue;
			}

			currentPtr = currentPtr->next;
		}
	}
	
	if (searchedBuddy->m_department[0] != 0) {
		currentPtr  = retBuddyList;
		while (currentPtr!=NULL) {
			if (currentPtr->buddy->m_department[0] == 0) {	
				currentPtr = currentPtr->next;
				continue;
			}
			
			if (strstr(currentPtr->buddy->m_department,searchedBuddy->m_department)==NULL) {
				strcpy(tmpbuf,currentPtr->buddy->m_telephone);
				currentPtr = currentPtr->next;
				retBuddyList = DelBuddyFromList(retBuddyList,tmpbuf);
				continue;
			}

			currentPtr = currentPtr->next;
		}
	}

	if (searchedBuddy->m_division[0] != 0) {
		currentPtr  = retBuddyList;
		while (currentPtr!=NULL) {
			if (currentPtr->buddy->m_division[0] == 0) {	
				currentPtr = currentPtr->next;
				continue;
			}
			
			if (strstr(currentPtr->buddy->m_division,searchedBuddy->m_division)==NULL) {
				strcpy(tmpbuf,currentPtr->buddy->m_telephone);
				currentPtr = currentPtr->next;
				retBuddyList = DelBuddyFromList(retBuddyList,tmpbuf);
				continue;
			}
			currentPtr = currentPtr->next;
		}
	}

/*	if(searchedBuddy->isID)
	{
		currentPtr  = retBuddyList;
		while(currentPtr!=NULL)
		{
		
			if(strstr(currentPtr->group,searchedBuddy->m_id)==NULL)//not found substring...so delete from list
			{
				strcpy(tmpbuf,currentPtr->buddy->m_telephone);
				currentPtr = currentPtr->next;
				retBuddyList = DelBuddyFromList(retBuddyList,tmpbuf);
				continue;
			}
			currentPtr = currentPtr->next;
		}
	}
*/
	if (searchedBuddy->m_remark[0] != 0) {
		currentPtr  = retBuddyList;
		while (currentPtr!=NULL) {
			if (currentPtr->buddy->m_remark[0] == 0) {
				currentPtr = currentPtr->next;
				continue;
			}
			if (strstr(currentPtr->buddy->m_remark,searchedBuddy->m_remark)==NULL) {
				strcpy(tmpbuf,currentPtr->buddy->m_telephone);
				currentPtr = currentPtr->next;
				retBuddyList = DelBuddyFromList(retBuddyList,tmpbuf);
				continue;
			}
			currentPtr = currentPtr->next;
		}
	}
	if (searchedBuddy->m_telephone[0] != 0) {
		currentPtr  = retBuddyList;
		while (currentPtr!=NULL) {
			if (currentPtr->buddy->m_telephone[0] == 0) {	
				currentPtr = currentPtr->next;
				continue;
			}
			if (strstr(currentPtr->buddy->m_telephone,searchedBuddy->m_telephone)==NULL) {
				strcpy(tmpbuf,currentPtr->buddy->m_telephone);
				currentPtr = currentPtr->next;
				retBuddyList = DelBuddyFromList(retBuddyList,tmpbuf);
				continue;
			}
			currentPtr = currentPtr->next;
		}
	}

	return retBuddyList;
}

bool CheckExistence(GroupList*_gList,char *_gName)
{
	if((_gList==NULL)||(_gName==NULL))
		return FALSE;
	GroupList *tmpPtr = _gList;
	bool isExist = false;
	
	while(tmpPtr!=NULL)
	{
		if(!_stricmp(tmpPtr->groupname,_gName))
			isExist = TRUE;
		tmpPtr = tmpPtr->next;
	}
	return isExist;
}

void AddGroupIntoGroupList(GroupList *_gList,char *_gName)
{
	if((_gList==NULL)||(_gName==NULL))
		return;
	if(CheckExistence(_gList,_gName))//if the group already added
		return;
	
	GroupList *tmpPtr = _gList;
	while(tmpPtr->next!=NULL)
		tmpPtr = tmpPtr->next;
	if((tmpPtr->next = NewGroupList())==NULL)
		return;
	
	strcpy(tmpPtr->next->groupname,_gName);
	
}

GroupList *GetAllGroup()
{
	char *owner = docOwner;
	if(!owner)
		return NULL;

	BuddyList *myBuddyList= GetAllBuddyList();
	if(!myBuddyList)
		return NULL;

	BuddyList *tmpList = myBuddyList;
	GroupList *myGroupList = NewGroupList();
	if(!myGroupList)
	{
		FreeBuddyList(myBuddyList);
		return NULL;
	}
	strcpy(myGroupList->groupname,DEFAULT_GROUPNAME);
	while(tmpList!=NULL)
	{
		if(_stricmp(tmpList->group,""))
		{	
			if(stricmp(tmpList->group,DEFAULT_GROUPNAME))
				AddGroupIntoGroupList(myGroupList, tmpList->group);
		}
		tmpList = tmpList->next;
	}
	FreeBuddyList(myBuddyList);
	return myGroupList;
}

bool CreateRuleDoc()
{
	if(!isLMSConnectSuccess)
		return false;
	int sd;
	if((sd = ConnectToServer())<= 0)
		return false;

	int iRet;
	char *owner = docOwner;

	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);

	composeURIDOCSel(uri_doc, NULL, DEFAULT_AUTHRULE_AUID, owner, DEFAULT_AUTHRULE_FILENAME);

	sprintf(uri,"PUT /%s HTTP/1.1\r\n",uri_doc);

	static char XMLcnt[1024],tmp_buf[256];
	memset(XMLcnt,0,1024);
	memset(tmp_buf,0,256);

	//compose the xml content for creating default document and list
	strcat(XMLcnt,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
	strcat(XMLcnt,"<cr:ruleset\r\n");
	strcat(XMLcnt,"xmlns:cr=\"urn:ietf:params:xml:ns:common-policy\"\r\n");
	strcat(XMLcnt,"xmlns:rpid=\"urn:ietf:params:xml:ns:rpid\"\r\n");
	strcat(XMLcnt,"xmlns=\"urn:ietf:params:xml:ns:pres-rules\"\r\n");
	strcat(XMLcnt,"xmlns:rs=\"urn:ietf:params:xml:ns:pidf:status:rpid-status\"\r\n");
	strcat(XMLcnt,"xmlns:ts=\"urn:ietf:params:xml:ns:pidf:rpid-tuple\"\r\n");
	strcat(XMLcnt,"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\r\n");
	strcat(XMLcnt,"</cr:ruleset>\r\n");


   	int hdr_len = fillPUTReqHdr( msg_hdr,uri,strlen(XMLcnt));
	if((iRet = sendPUTReq(msg_hdr,XMLcnt,recv_buf,sd)) > 201)
	{
		DisConnect(sd);
		return false;
	}


	DisConnect(sd);
	_isPresRulesModified = true;
	return true;
}

bool CheckRuleDocExistence()
{
	char tmp_buf[256];
	int iRet=0;
	char *owner = docOwner;

	if(!isLMSConnectSuccess)
		return false;
	int sd;
	if((sd = ConnectToServer())<=0)
		return false;
	
	memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
	memset(uri_node,0,DEFAULT_URI_NODE_LEN);
	memset(tmp_buf,0,256);
	memset(uri,0,DEFAULT_URI_LEN);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);
	
	composeURIDOCSel(uri_doc, NULL, DEFAULT_AUTHRULE_AUID, owner, DEFAULT_AUTHRULE_FILENAME);

	sprintf(uri,"GET /%s HTTP/1.1\r\n",uri_doc);

	int hdr_len = fillGETReqHdr( msg_hdr,uri);

	if((iRet = sendGETReq(msg_hdr,recv_buf,sd))!=200)
	{
		DisConnect(sd);
		return false;
	}

	DisConnect(sd);
	return true;	
}

RuleList *GetAllRules()
{
	char tmp_buf[256];
	int iRet=0;
	int BuddyCount = -1;
	char *owner = docOwner;
	RuleList *pRetRule = NULL;

	if(!isLMSConnectSuccess)
		return NULL;

	if(_isPresRulesModified)
	{
		int sd;
		if((sd = ConnectToServer())<=0)
			return NULL;
		memset(uri_doc,0,DEFAULT_URI_DOC_LEN);
		memset(uri_node,0,DEFAULT_URI_NODE_LEN);
		memset(tmp_buf,0,256);
		memset(uri,0,DEFAULT_URI_LEN);
		memset(msg_hdr,0,1024);
		memset(recv_buf,0,BUF_SIZE);
	
		composeURIDOCSel(uri_doc, NULL, DEFAULT_AUTHRULE_AUID, owner, DEFAULT_AUTHRULE_FILENAME);

		sprintf(uri,"GET /%s HTTP/1.1\r\n",uri_doc);

		int hdr_len = fillGETReqHdr( msg_hdr,uri);
		memset(recv_buf,0,BUF_SIZE);
		if((iRet = sendGETReq(msg_hdr,recv_buf,sd))!=200)
		{
			DisConnect(sd);
			return NULL;
		}
		DisConnect(sd);
		pRetRule = FromXMLtoRuleList(recv_buf,strlen(recv_buf));
		UpdatePresRulesCache(pRetRule);
		
	}
	else
		pRetRule = DupPresRule(GetPresRulesCache());
	//parse the recv_buf to get the all buddy list....
	return pRetRule;

}

int GetRuleCounts(RuleList *_countRules)
{
	int count = 0;
	RuleList *tmpPtr = _countRules;
	while(tmpPtr!=NULL)
	{
		count++;
		tmpPtr = tmpPtr->next;
	}

	return count;
}

//generate rule id..
int GenRuleID()
{
	return GetRuleCounts(GetAllRules())+1;
}

bool AddRule(RuleList *_addRule)
{
	char *owner = docOwner;
	if(_addRule==NULL)
		return false;
	if(!CheckRuleDocExistence())
	{
		if(!CreateRuleDoc())
			return false;
	}
	if(!isLMSConnectSuccess)
		return false;	
	int sd;
	if((sd = ConnectToServer())<=0)
		return false;
	//compose uri...

	char tmp_buf[256];
	int iRet;

	memset(uri_doc,0,64);
	memset(uri,0,128);
	memset(msg_hdr,0,1024);
	memset(recv_buf,0,BUF_SIZE);


	sprintf(tmp_buf,"cr:rule[@id=%d]",_addRule->ruleid);
	composeURIDOCSel(uri_doc, NULL, DEFAULT_AUTHRULE_AUID, owner, DEFAULT_AUTHRULE_FILENAME);
	composeURINodeSel(uri_node, "cr:ruleset", tmp_buf, NULL, NULL);

	sprintf(uri,"PUT /%s/~~/%s HTTP/1.1\r\n",uri_doc,uri_node);

	static char XMLcnt[BUF_SIZE];
	memset(XMLcnt,0,BUF_SIZE);
	memset(tmp_buf,0,256);
	
	//compose xml content according to the rule 
	sprintf(XMLcnt,"<cr:rule id=\"%d\"  xmlns:cr=\"urn:ietf:params:xml:ns:common-policy\">\r\n",_addRule->ruleid);
	strcat(XMLcnt,"<cr:conditions>\r\n");
	strcat(XMLcnt,"<cr:identity>\r\n");
	sprintf(tmp_buf,"<cr:id>%s</cr:id>",_addRule->rule->m_id);
	strcat(XMLcnt,tmp_buf);
	strcat(XMLcnt,"</cr:identity>\r\n");
	strcat(XMLcnt,"</cr:conditions>\r\n");
	strcat(XMLcnt,"<cr:actions>\r\n");
	sprintf(tmp_buf,"<sub-handling>%s</sub-handling>\r\n",_addRule->rule->m_action);
	strcat(XMLcnt,tmp_buf);
	strcat(XMLcnt,"</cr:actions>\r\n");
	strcat(XMLcnt,"<cr:transformations>\r\n");
	strcat(XMLcnt,"<provide-tuples>\r\n");
	strcat(XMLcnt,"<all-tuples></all-tuples>\r\n");
	strcat(XMLcnt,"</provide-tuples>\r\n");
	strcat(XMLcnt,"</cr:transformations>\r\n");
	strcat(XMLcnt,"</cr:rule>\r\n");
	
	int hdr_len = fillPUTReqHdr( msg_hdr,uri,strlen(XMLcnt));
	
	if((iRet = sendPUTReq(msg_hdr,XMLcnt,recv_buf,sd)) > 201)
	{
		DisConnect(sd);
		return false;
	}


	DisConnect(sd);
	_isPresRulesModified = true;
	return true;
}

RuleList *SearchRuleByURI(char *_uri)
{
	RuleList *_myList = GetAllRules();
	if(_myList ==NULL)
		return NULL;
	RuleList *tmpPtr= _myList;
	RuleList *retRule = NULL;

	while(tmpPtr!=NULL)
	{
		if(!_stricmp(tmpPtr->rule->m_id,_uri))//matched!!
		{
			retRule = NewRuleList();
			retRule->next = NULL;
			retRule->ruleid = tmpPtr->ruleid;
			memcpy(retRule->rule,tmpPtr->rule,sizeof(Rule));
			break;
			
		}
		tmpPtr = tmpPtr->next;
	}
	FreeRuleList(_myList);
	
	return retRule;
}

int GetRuleIDByURI(char *_uri)
{
	if(_uri ==NULL)
		return -1;
	RuleList *_myList = SearchRuleByURI(_uri);
	int retID=-1;
	if(_myList!=NULL)
	{
		retID = _myList->ruleid;
		FreeRuleList(_myList);
	}
	return retID;
}
bool AddNewRule(char *_uri,char *_action)
{
	RuleList *_myList = NewRuleList();

	_myList->ruleid = GenRuleID();
	strcpy(_myList->rule->m_id,_uri);
	if(_action!=NULL)
		strcpy(_myList->rule->m_action,_action);//default action
	else
		strcpy(_myList->rule->m_action,"allow");
	bool ret = AddRule(_myList);
	
	FreeRuleList(_myList);

	return ret;
}
bool SetRuleAction2Allow(char *_uri)
{
	if(_uri == NULL)
		return false;
	int ruleID = GetRuleIDByURI(_uri);
	if(ruleID<0)
		return false;
	RuleList *_myList = NewRuleList();

	_myList->ruleid = ruleID;
	strcpy(_myList->rule->m_id,_uri);
	strcpy(_myList->rule->m_action,"allow");

	AddRule(_myList);//do modify...because of the same rule id
	FreeRuleList(_myList);
	
	return true;
}

bool SetRuleAction2Block(char *_uri)
{
	if(_uri == NULL)
		return false;
	int ruleID = GetRuleIDByURI(_uri);
	if(ruleID<0)
		return false;
	RuleList *_myList = NewRuleList();

	_myList->ruleid = ruleID;
	strcpy(_myList->rule->m_id,_uri);
	strcpy(_myList->rule->m_action,"block");

	AddRule(_myList);//do modify...because of the same rule id
	FreeRuleList(_myList);
	
	return true;
}

bool isRuleExist(char *_uri)
{
	if(_uri==NULL)
		return false;
	RuleList *_myList = SearchRuleByURI(_uri);
	if(_myList==NULL)
		return false;
	FreeRuleList(_myList);

	return true;
}

bool GetRuleActionByURI(char *_uri, char *_action)
{
	if((_uri==NULL)||(_action==NULL))
		return false;
	RuleList *searchRet = SearchRuleByURI(_uri);
	if(searchRet!=NULL)//if found...
	{
		strcpy(_action,searchRet->rule->m_action);
		FreeRuleList(searchRet);
		return true;
	}
	return false;

}

// used for resource-lists
BuddyInfo *NewBuddy()
{
	return (BuddyInfo *)calloc(sizeof(BuddyInfo), sizeof(char));
}

void FreeBuddy(BuddyInfo *freebd)
{
	if (freebd!=NULL) free(freebd);
}

BuddyList *NewBuddyList()
{
	BuddyList *bdlist = (BuddyList *)calloc(sizeof(BuddyList), sizeof(char));
	bdlist->buddy = NewBuddy();

	return bdlist;
}

void FreeBuddyList(BuddyList *bdlist)
{
	if (bdlist==NULL)
		return;

	if (bdlist->buddy != NULL)
		FreeBuddy(bdlist->buddy);
	if (bdlist->next != NULL)
		FreeBuddyList(bdlist->next);
	free(bdlist);
}

GroupList *NewGroupList()
{
	return (GroupList*)calloc(sizeof(GroupList), sizeof(char));
}

void FreeGroupList(GroupList *freeGList)
{
	if (freeGList==NULL)
		return;

	if (freeGList->next!=NULL)
		FreeGroupList(freeGList->next);
	free(freeGList);
}

// used for pres-rule ---authorization rules
Rule*      NewRule( )
{
	Rule *newrule = (Rule *)malloc(sizeof(Rule));
	memset(newrule->m_id,0,64);
	memset(newrule->m_action,0,64);
	return newrule;
}
void  FreeRule( Rule* rule )
{
        if (rule!=NULL)
                free(rule);
}
RuleList*  NewRuleList( )
{
        RuleList *rlist = (RuleList *)malloc(sizeof(RuleList));
        rlist->rule = NewRule();
        rlist->ruleid = 0;
        rlist->next = NULL;
        return rlist;
}
void  FreeRuleList( RuleList* rlist )
{
        if (rlist!=NULL) {
                if(rlist->rule!=NULL)
                        FreeRule(rlist->rule);

                if(rlist->next!=NULL)
                        FreeRuleList(rlist->next);

                free(rlist);
        }
}

//for check chinese
bool isChinese(const char* str) 
{
	if (str == NULL)
		return false;

	int len = strlen(str);
	for (int i = 0; i<len; i++) {
		if (str[i] <0)
			return true;
	}
	return false;
}

