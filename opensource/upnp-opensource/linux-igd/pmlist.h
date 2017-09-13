/******************************************************************************
*  Copyright (c) 2002 Linux UPnP Internet Gateway Device Project              *    
*  All rights reserved.                                                       *
*                                                                             *   
*  This file is part of The Linux UPnP Internet Gateway Device (IGD).         *
*                                                                             *
*  The Linux UPnP IGD is free software; you can redistribute it and/or modify *
*  it under the terms of the GNU General Public License as published by       *
*  the Free Software Foundation; either version 2 of the License, or          *
*  (at your option) any later version.                                        *
*                                                                             *    
*  The Linux UPnP IGD is distributed in the hope that it will be useful,      *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*  GNU General Public License for more details.                               *
*                                                                             *   
*  You should have received a copy of the GNU General Public License          * 
*  along with Foobar; if not, write to the Free Software                      *
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  *
*                                                                             *  
*                                                                             *  
******************************************************************************/

#ifndef _PMLIST_H_
#define _PMLIST_H_

#include "portmap.h"
#include <syslog.h>

#define IPTABLES "/usr/sbin/iptables"

/* The following functions are used for doublely linked
 * lists. It will use any structure, as long as it has
 * a next and prev pointer.
 */

#define NODE_ADD(head, node)	\
	{	node->next=head;		\
		node->prev=NULL;		\
		if(head) (head)->prev=node;	\
		head=node;			\
	}

/* This enqueues a node to the end of a list. As we don't have a temp node,
   we fake it out by using the node's next variable, which will be NULL, but
   is of the correct type */
#define NODE_ENQUEUE(head, node)	\
	{	node->next=head;				\
		if(node->next)					\
			while(node->next->next)			\
				node->next=node->next->next;	\
		node->prev=node->next;				\
		node->next=NULL;				\
		if(!head) head=node;				\
		else node->prev->next=node;				\
	}

#define NODE_DEL(head, node)	\
	{	if(node->next) node->next->prev=node->prev;	\
		if(node->prev) node->prev->next=node->next;	\
		else head=node->next;				\
	}

struct PortMapList
{
	struct PortMap	*m_pmap;
};

void	PortMapList_Construct(struct PortMapList *pmlist);
void	PortMapList_Destruct(struct PortMapList *pmlist);

int	PortMapAdd(struct PortMapList *pmlist, char *RemoteHost, char *Proto, char *ExtIP, int ExtPort,
		char *IntIP, int IntPort, int Enabled, char *Desc, int LeaseDuration);
int	PortMapDelete(struct PortMapList *pmlist, char  *Proto, int ExtPort);

int	PortMap_addPortForward(char *Proto, char *ExtIP, int ExtPort, char *IntIP,
		int IntPort, int Enabled, char *Desc, char *RemoteHost);
int	PortMap_delPortForward(char *Proto, char *ExtIP, int ExtPort, char* IntIP, int IntPort, char *RemoteHost);

#endif
