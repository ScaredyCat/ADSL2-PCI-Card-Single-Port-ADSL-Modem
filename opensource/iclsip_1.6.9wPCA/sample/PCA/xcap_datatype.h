#ifndef _XCAPDATATYPE
#define _XCAPDATATYPE

#define DEFAULT_GROUPNAME_SIZE 32
#define DEFAULT_GROUPNAME "DEFAULT_LIST"

#define DEFAULT_AUTHRULE_FILENAME "pres-rules.xml"
#define DEFAULT_AUTHRULE_AUID "pres-rules"

typedef enum {
	XCAP_DISPLAY_NAME,
	XCAP_ID,
	XCAP_Department,
	XCAP_Division,
	XCAP_Remark,
	XCAP_Tel,
	XCAP_Pic,
	XCAP_NULL,
	RULE_ID,
	RULE_Action
} TagType;


typedef struct i_BuddyInfo
{
	char	m_displayname[128];
	char	m_uri[64];
	char	m_division[64];
	char	m_department[64];
	char	m_telephone[64];
	char	m_remark[255];
	char	m_pic[255];
} BuddyInfo;

typedef struct i_BuddyList
{
	BuddyInfo *buddy;
	char group[DEFAULT_GROUPNAME_SIZE];
	struct i_BuddyList *next;
} BuddyList;


typedef struct i_GroupList
{
	char groupname[DEFAULT_GROUPNAME_SIZE];
	struct i_GroupList *next;
}GroupList;

typedef struct i_Rule
{
        char m_id[64];
        char m_action[64];
} Rule;

typedef struct i_RuleList
{
	Rule*	rule;
	int	ruleid;
	struct i_RuleList *next;
} RuleList;

BuddyInfo *	NewBuddy();
void		FreeBuddy(BuddyInfo *freebd);
BuddyList *	NewBuddyList();
void		FreeBuddyList(BuddyList *bdlist);

GroupList *	NewGroupList();
void		FreeGroupList(GroupList *freeGList);

Rule *		NewRule( );
void		FreeRule( Rule* rule );
RuleList *	NewRuleList( );
void		FreeRuleList( RuleList* rlist );


#endif
