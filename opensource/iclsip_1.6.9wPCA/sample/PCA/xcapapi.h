#ifndef _XCAPAPI
#define _XCAPAPI


#include <stdio.h>
#include "xcap_datatype.h"


//for configuration
void SetOwnerName(char *_owner);
char *GetOwnerName();
void ConfigXCAPServer(char *s_ip,unsigned int s_port);
int ConnectToServer();
bool DisConnect(int sd);

//for resource-list: buddy management
bool CreateContactDoc();
bool AddBuddyIntoList(BuddyInfo *addbuddy,char *groupname);
int GetBuddyCount();
BuddyInfo*	GetBuddyByUri(char *group, char *uri);
char*		GetBuddyAttr(char *group, char *uri, TagType attrtype);
BuddyList *GetAllBuddyList();
BuddyList *SearchBuddy(BuddyInfo *searchedBuddy);
BuddyList*SearchBuddyByName(char *_name);
BuddyList*SearchBuddyByTel(char *_tel);
int GetListCount(BuddyList *bdlist);
bool ModifyBuddy(BuddyInfo *modifybuddy,char *groupname);
bool	DelBuddy(char *buddyuri);
BuddyList *RemoveEmptyBuddyFromList(BuddyList *bdlist);

//for resource-list: group management
#define GRP_NOT_FOUND	0
#define GRP_EXIST	1
#define SERVER_ERROR	2
int isGroupExist(char *groupname); // 0: not exist, 1: exist, 2: server error
GroupList *GetAllGroup();
void AddGroupIntoGroupList(GroupList *_gList,char *_gName);
bool CheckExistence(GroupList*_gList,char *_gName);
bool CreateGroup(char *groupname);
bool DelGroup(char *g_name);
BuddyList *GetGroupMembers(char *_groupname);
bool ModifyGroupName(char *_orgGroupName, char *_newGroupName);


//for testing.
char *test_DefaultGroupBuddy();
char *test_Allcontactlist_xml();

//for Auth-Rules
bool AddNewRule(char *_uri,char *_action);
bool SetRuleAction2Allow(char *_uri);
bool SetRuleAction2Block(char *_uri);
bool CheckRuleDocExistence();
bool CreateRuleDoc();
bool isRuleExist(char *_uri);
bool GetRuleActionByURI(char *_uri, char *_action);

//for cache
void ClearCache();

//for check chinese
bool isChinese(const char* str);

#endif