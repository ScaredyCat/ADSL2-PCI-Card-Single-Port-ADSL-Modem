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

#ifndef _GATE_H_
#define _GATE_H_

#include "upnp/upnp.h"

#include "ipcon.h"
#include "pmlist.h"

#include "upnp/upnptools.h"
#include "upnp/ixml.h"

typedef IXML_Document* Upnp_Document;
#define UpnpDocument_free ixmlDocument_free
#define UpnpParse_Buffer ixmlParseBuffer

struct Gate {
	struct PortMapList	m_list;
	UpnpDevice_Handle	device_handle;
	char			*gate_udn;
	struct IPCon		*m_ipcon;

	// State Variables
	char	m_ConnectionType[50];
	char	m_PossibleConnectionTypes[50];
	char	m_ConnectionStatus[20];
	long int startup_time;
	char	LastConnectionError[35];
	long int m_AutoDisconnectTime;
	long int m_IdleDisconnectTime;
	long int m_WarnDisconnectDelay;
	int	RSIPAvailable;
	int	NATEnabled;
	char	m_ExternalIPAddress[20];
	int	m_PortMappingNumberOfEntries;
	int	m_PortMappingEnabled;
};

void	GateDeviceConstruct(struct Gate *gate);
void	GateDeviceDestruct(struct Gate *gate);
int	GateDeviceHandleSubscriptionRequest	(struct Gate *gate, struct Upnp_Subscription_Request *sr_event);
int	GateDeviceStateTableInit		(struct Gate *gate, char *DescDocURL);
int	GateDeviceHandleGetVarRequest		(struct Gate *gate, struct Upnp_State_Var_Request *cg_event);
int	GateDeviceHandleActionRequest		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceCallbackEventHandler		(struct Gate *gate, Upnp_EventType EventType, void *Event, void *Cookie);
int	GateDeviceSetConnectionType		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetConnectionTypeInfo		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
//int	GateDeviceRequestConnection		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
//int	GateDeviceRequestTermination		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
//int	GateDeviceForceTermination		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
//int	GateDeviceSetAutoDisconnectTime		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
//int	GateDeviceSetIdleDisconnectTime		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
//int	GateDeviceSetWarnDisconnectDelay	(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetStatusInfo			(struct Gate *gate, struct Upnp_Action_Request *ca_event);
//int	GateDeviceGetAutoDisconnectTime		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
//int	GateDeviceGetIdleDisconnectTime		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
//int	GateDeviceGetWarnDisconnectDelay	(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetNATRSIPStatus		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetGenericPortMappingEntry	(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetSpecificPortMappingEntry	(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetExternalIPAddress		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceAddPortMapping		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceDeletePortMapping		(struct Gate *gate, struct Upnp_Action_Request *ca_event);

int	GateDeviceX				(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetCommonLinkProperties	(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetTotalBytesSent		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetTotalBytesReceived		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetTotalPacketsSent		(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int	GateDeviceGetTotalPacketsReceived	(struct Gate *gate, struct Upnp_Action_Request *ca_event);
int upnp_send_byebye(char *desc_url);
#endif
