#include <stdlib.h>
#include <syslog.h>
#include "config.h"
#include "pmlist.h"
#include "gatedevice.h"

struct portMap* pmlist_NewNode(int enabled, int duration, char *remoteHost,
         char *externalPort, char *internalPort,
         char *protocol, char *internalClient, char *desc)
{
	struct portMap* temp;
	temp = (struct portMap*) malloc(sizeof(struct portMap));
	temp->m_PortMappingEnabled = enabled;
	temp->m_PortMappingLeaseDuration = duration;
	
	if (strlen(remoteHost) < sizeof(temp->m_RemoteHost)) strcpy(temp->m_RemoteHost, remoteHost);
		else strcpy(temp->m_RemoteHost, "");
	if (strlen(externalPort) < sizeof(temp->m_ExternalPort)) strcpy(temp->m_ExternalPort, externalPort);
		else strcpy(temp->m_ExternalPort, "");
	if (strlen(internalPort) < sizeof(temp->m_InternalPort)) strcpy(temp->m_InternalPort, internalPort);
		else strcpy(temp->m_InternalPort, "");
	if (strlen(protocol) < sizeof(temp->m_PortMappingProtocol)) strcpy(temp->m_PortMappingProtocol, protocol);
		else strcpy(temp->m_PortMappingProtocol, "");
	if (strlen(internalClient) < sizeof(temp->m_InternalClient)) strcpy(temp->m_InternalClient, internalClient);
		else strcpy(temp->m_InternalClient, "");
	if (strlen(desc) < sizeof(temp->m_PortMappingDescription)) strcpy(temp->m_PortMappingDescription, desc);
		else strcpy(temp->m_PortMappingDescription, "");

	temp->next = NULL;
	temp->prev = NULL;
	
	return temp;
}
	
struct portMap* pmlist_Find(char *externalPort, char *proto, char *internalClient)
{
	struct portMap* temp;
	
	temp = pmlist_Head;
	if (temp == NULL)
		return NULL;
	
	do 
	{
		if ( (strcmp(temp->m_ExternalPort, externalPort) == 0) &&
				(strcmp(temp->m_PortMappingProtocol, proto) == 0) &&
				(strcmp(temp->m_InternalClient, internalClient) == 0) )
			return temp; // We found a match, return pointer to it
		else
			temp = temp->next;
	} while (temp != NULL);
	
	// If we made it here, we didn't find it, so return NULL
	return NULL;
}

struct portMap* pmlist_FindByIndex(int index)
{
	int i=0;
	struct portMap* temp;

	temp = pmlist_Head;
	if (temp == NULL)
		return NULL;
	do
	{
		if (i == index)
			return temp;
		else
		{
			temp = temp->next;	
			i++;
		}
	} while (temp != NULL);

	return NULL;
}	

struct portMap* pmlist_FindSpecific(char *externalPort, char *protocol)
{
	struct portMap* temp;
	
	temp = pmlist_Head;
	if (temp == NULL)
		return NULL;
	
	do
	{
		if ( (strcmp(temp->m_ExternalPort, externalPort) == 0) &&
				(strcmp(temp->m_PortMappingProtocol, protocol) == 0))
			return temp;
		else
			temp = temp->next;
	} while (temp != NULL);

	return NULL;
}

int pmlist_IsEmtpy(void)
{
	if (pmlist_Head)
		return 0;
	else
		return 1;
}

int pmlist_Size(void)
{
	struct portMap* temp;
	int size = 0;
	
	temp = pmlist_Head;
	if (temp == NULL)
		return 0;
	
	while (temp->next)
	{
		size++;
		temp = temp->next;
	}
	size++;
	return size;
}	

int pmlist_FreeList(void)
{
	struct portMap* temp;
	
	temp = pmlist_Head;
	if (temp) // We have a list
	{
		while(temp->next) // While there's another node left in the list
		{
	      pmlist_DeletePortMapping(temp->m_PortMappingProtocol, temp->m_ExternalPort,
            temp->m_InternalClient, temp->m_InternalPort);
			temp = temp->next; // Move to the next element.
 			free(temp->prev);
			temp->prev = NULL; // Probably unecessary, but i do it anyway. May remove.
		}
      pmlist_DeletePortMapping(temp->m_PortMappingProtocol, temp->m_ExternalPort,
            temp->m_InternalClient, temp->m_InternalPort);
		free(temp); // We're at the last element now, so delete ourselves.
		pmlist_Head = pmlist_Tail = NULL;
	}
	return 1;
}
		
int pmlist_PushBack(struct portMap* item)
{
	int action_succeeded = 0;

	if (pmlist_Tail) // We have a list, place on the end
	{
		pmlist_Tail->next = item;
		item->prev = pmlist_Tail;
		item->next = NULL;
		pmlist_Tail = item;
		action_succeeded = 1;
	}
	else // We obviously have no list, because we have no tail :D
	{
		pmlist_Head = pmlist_Tail = pmlist_Current = item;
		item->prev = NULL;
		item->next = NULL;
 		action_succeeded = 1;
	}
	if (action_succeeded == 1)
	{
		 pmlist_AddPortMapping(item->m_PortMappingProtocol,
         item->m_ExternalPort, item->m_InternalClient, item->m_InternalPort);	
		return 1;
	}
	else
		return 0;
}

		
int pmlist_Delete(struct portMap* item)
{
	struct portMap *temp;
	int action_succeeded = 0;

	temp = pmlist_Find(item->m_ExternalPort, item->m_PortMappingProtocol, item->m_InternalClient);
	if (temp) // We found the item to delete
	{
		pmlist_DeletePortMapping(item->m_PortMappingProtocol, item->m_ExternalPort, 
				item->m_InternalClient, item->m_InternalPort);
		if (temp == pmlist_Head) // We are the head of the list
		{
			if (temp->next == NULL) // We're the only node in the list
			{
				pmlist_Head = pmlist_Tail = pmlist_Current = NULL;
				free (temp);
				action_succeeded = 1;
			}
			else // we have a next, so change head to point to it
			{
				pmlist_Head = temp->next;
				pmlist_Head->prev = NULL;
				free (temp);
				action_succeeded = 1;	
			}
		}
		else if (temp == pmlist_Tail) // We are the Tail, but not the Head so we have prev
		{
			pmlist_Tail = pmlist_Tail->prev;
			free (pmlist_Tail->next);
			pmlist_Tail->next = NULL;
			action_succeeded = 1;
		}
		else // We exist and we are between two nodes
		{
			temp->prev->next = temp->next;
			temp->next->prev = temp->prev;
			pmlist_Current = temp->next; // We put current to the right after a extraction
			free (temp);	
			action_succeeded = 1;
		}
	}
	else  // We're deleting something that's not there, so return 0
		action_succeeded = 0;

	if (action_succeeded == 1)
	{
		return 1;
	}
	else 
		return 0;
}

int pmlist_AddPortMapping (char *protocol, char *externalPort, char *internalClient, char *internalPort)
{
	char command[500];
	memset(command, 0x00, sizeof(command));
//HANK2005/10/27 09:59�W��	
//	sprintf(command, "%s -t nat -A PREROUTING -i %s -p %s --dport %s -j DNAT --to %s:%s", IPTABLES, extInterfaceName, protocol, externalPort, internalClient, internalPort);	
	sprintf(command, "%s -t nat -A IFX_NAPT_DNAT_PM -i %s -p %s --dport %s -j DNAT --to %s:%s", IPTABLES, extInterfaceName, protocol, externalPort, internalClient, internalPort);
	syslog(LOG_DEBUG, command);
	system (command);
	memset(command, 0x00, sizeof(command));
	sprintf(command, "%s -I FORWARD -p %s -d %s --dport %s -j ACCEPT", IPTABLES, protocol, internalClient, internalPort);
	syslog(LOG_DEBUG, command);
	system (command);	
	
	return 1;
}

int pmlist_DeletePortMapping(char *protocol, char *externalPort, char *internalClient, char *internalPort)
{
	char command[500];

	memset(command, 0x00, sizeof(command));
	sprintf(command, "%s -t nat -D IFX_NAPT_DNAT_PM -i %s -p %s --dport %s -j DNAT --to %s:%s",
			IPTABLES, extInterfaceName, protocol, externalPort, internalClient, internalPort);
	syslog(LOG_DEBUG, command);
	system(command);	
	memset(command, 0x00, sizeof(command));	
	sprintf(command, "%s -D FORWARD -p %s -d %s --dport %s -j ACCEPT", IPTABLES, protocol, internalClient, internalPort);
	syslog(LOG_DEBUG, command);
	system (command);	
		
	return 1;
}

