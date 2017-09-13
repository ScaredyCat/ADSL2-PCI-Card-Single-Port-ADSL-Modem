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

#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>

#include <net/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "gateway.h"
//#include "sample_util.h"
#include "portmap.h"
#include <arpa/inet.h>
#include "gate.h"
#include <unistd.h>


pthread_mutex_t DevMutex = PTHREAD_MUTEX_INITIALIZER;

char *GateDeviceType[] = {"urn:schemas-upnp-org:device:InternetGatewayDevice:1"
                        ,"urn:schemas-upnp-org:device:WANDevice:1"
                         ,"urn:schemas-upnp-org:device:WANConnectionDevice:1"};

char *GateServiceType[] = {"urn:schemas-upnp-org:service:Layer3Forwarding:1"
                          ,"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1"
                          ,"urn:schemas-upnp-org:service:WANIPConnection:1"};

char *GateServiceId[] = {"urn:upnp-org:serviceId:L3Forwarding1"
                        ,"urn:upnp-org:serviceId:WANCommonIFC1"
                        ,"urn:upnp-org:serviceId:WANIPConn1"};
                        
int wan_connected_status;                        

char *GetFirstDocumentItem( IN IXML_Document * doc,
		IN const char *item )
{
	IXML_NodeList *nodeList = NULL;
	IXML_Node *textNode = NULL;
	IXML_Node *tmpNode = NULL;

	char *ret = NULL;

	nodeList = ixmlDocument_getElementsByTagName( doc, ( char * )item );

	if( nodeList ) {
		if( ( tmpNode = ixmlNodeList_item( nodeList, 0 ) ) ) {
			textNode = ixmlNode_getFirstChild( tmpNode );
			if(ixmlNode_getNodeValue( textNode ))
				ret = strdup( ixmlNode_getNodeValue( textNode ) );
		}
	}

	if( nodeList )
		ixmlNodeList_free( nodeList );
	return ret;
}

int getProtoNum(char * proto)
{
	if (strcmp(proto,"TCP")==0)
                return 6;
        else if (strcmp(proto,"UDP")==0)
                return 17;
	else return 0;
}

char *getProtoName(int prt)
{
	switch (prt)
	{
		case 6:
			return "TCP";
			break;
		case 17:
			return "UDP";
			break;
		default:
			return "TCP";
	}
}

int chkIPADDRstring(char * addr) {
        struct in_addr dmy;
        return inet_aton(addr, &dmy);
}

void GateDeviceDestruct(struct Gate *gate)
{
	if (gate->gate_udn)
		free(gate->gate_udn);
	if (gate->m_ipcon){
		IPCon_Destruct(gate->m_ipcon);
		free(gate->m_ipcon);
	}
}

int GateDeviceCallbackEventHandler(struct Gate *gate,
		Upnp_EventType EventType,
		void *Event,
		void *Cookie)
{
//syslog(LOG_ERR, "Callback Event Handler, type = %d", EventType);
	switch ( EventType)
	{

		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
			GateDeviceHandleSubscriptionRequest(gate, (struct Upnp_Subscription_Request *) Event);
			break;

		case UPNP_CONTROL_GET_VAR_REQUEST:
			GateDeviceHandleGetVarRequest(gate, (struct Upnp_State_Var_Request *) Event);
			break;

		case UPNP_CONTROL_ACTION_REQUEST:
			GateDeviceHandleActionRequest(gate, (struct Upnp_Action_Request *) Event);
			break;

		default:

//			syslog(LOG_DEBUG, "Error in DeviceCallbackEventHandler: unknown event type %d\n", EventType);
                        break;
	}

	return(0);
}


int GateDeviceStateTableInit (struct Gate *gate,
		char* DescDocURL)
{
	Upnp_Document DescDoc = NULL;
	int ret = UPNP_E_SUCCESS;

	if (UpnpDownloadXmlDoc(DescDocURL, &DescDoc) != UPNP_E_SUCCESS)
	{
//		syslog(LOG_ERR, "DeviceStateTableInit -- Error Parsing %s\n", DescDocURL);
		ret =UPNP_E_INVALID_DESC;
	}

	gate->gate_udn = GetFirstDocumentItem(DescDoc, "UDN");

	return (ret);
}

int GateDeviceHandleSubscriptionRequest (struct Gate *gate,
		struct Upnp_Subscription_Request *sr_event)
{
	Upnp_Document PropSet=NULL;
	char *address;
//	address = m_ipcon->IPCon_GetIpAddrStr();
	address = IPCon_GetIpAddrStr(gate->m_ipcon);

	pthread_mutex_lock(&DevMutex);

//syslog(LOG_ERR, "Subscription Request |%s|", sr_event->ServiceId);
//	if (strcmp(sr_event->UDN, gate->gate_udn) == 0)
	{
		if (strcmp(sr_event->ServiceId, GateServiceId[GATE_SERVICE_LAYER3]) == 0)
		{
#if 0			
			UpnpAddToPropertySet(&PropSet, "OSMajorVersion","5");
			UpnpAddToPropertySet(&PropSet, "OSMinorVersion","1");
			UpnpAddToPropertySet(&PropSet, "OSBuildNumber","2600");
			UpnpAddToPropertySet(&PropSet, "OSMachineName","Linux IGD");
			UpnpAcceptSubscriptionExt(gate->device_handle, sr_event->UDN,
					sr_event->ServiceId, PropSet, sr_event->Sid);
			UpnpDocument_free(PropSet);
#else
		UpnpAddToPropertySet(&PropSet, "DefaultConnectionService",
													"uuid:608007c2-cd7a-4242-80a2-ec2aab3d3369:WANConnectionDevice:1,urn:upnp-org:serviceId:WANIPConn1");
		UpnpAcceptSubscriptionExt(gate->device_handle, sr_event->UDN,
					sr_event->ServiceId, PropSet, sr_event->Sid);
		UpnpDocument_free(PropSet);
#endif			
		}
		else if (strcmp(sr_event->ServiceId, GateServiceId[GATE_SERVICE_CONFIG]) == 0)
		{
//			UpnpAddToPropertySet(&PropSet, "PhysicalLinkStatus", m_ipcon->IPCon_GetIfStatStr());
			UpnpAddToPropertySet(&PropSet, "PhysicalLinkStatus", IPCon_GetIfStatStr(gate->m_ipcon));
			UpnpAddToPropertySet(&PropSet, "EnabledForInternet", "1");
			UpnpAcceptSubscriptionExt(gate->device_handle, sr_event->UDN, sr_event->ServiceId, PropSet, sr_event->Sid);
			UpnpDocument_free(PropSet);
		}
		else if (strcmp(sr_event->ServiceId, GateServiceId[GATE_SERVICE_CONNECT])==0)
		{
			UpnpAddToPropertySet(&PropSet, "PossibleConnectionTypes","IP_Routed");
			if(wan_connected_status)
				UpnpAddToPropertySet(&PropSet, "ConnectionStatus","Connected");
			else	
				UpnpAddToPropertySet(&PropSet, "ConnectionStatus","Disconnected");
			UpnpAddToPropertySet(&PropSet, "X_Name","Local Area Connection");
			UpnpAddToPropertySet(&PropSet, "ExternalIPAddress",address);
			UpnpAddToPropertySet(&PropSet, "PortMappingNumberOfEntries","0");
			UpnpAcceptSubscriptionExt(gate->device_handle, sr_event->UDN,	sr_event->ServiceId, PropSet, sr_event->Sid);
			UpnpDocument_free(PropSet);
		}
	}
	pthread_mutex_unlock(&DevMutex);
	if (address) free(address);
	return(1);
}

int GateDeviceHandleGetVarRequest(struct Gate *gate,
		struct Upnp_State_Var_Request *cgv_event)
{
	int getvar_succeeded = 0;
	cgv_event->CurrentVal = NULL;
	pthread_mutex_lock(&DevMutex);

//syslog(LOG_ERR, "Var Request ");
		if (strcmp(cgv_event->ServiceID,"urn:upnp-org:serviceId:WANIPConn1") ==0)
		{
			if (strcmp(cgv_event->StateVarName,"ConnectionStatus")==0)
			{
					if(wan_connected_status)
						cgv_event->CurrentVal = ixmlCloneDOMString("Connected");
					else
						cgv_event->CurrentVal = ixmlCloneDOMString("Disconnected");	
					getvar_succeeded = 1;
			}		
		}	
    if (getvar_succeeded)
    {
	    cgv_event->ErrCode = UPNP_E_SUCCESS;
		}
    else
    {
//		syslog(LOG_ERR,"Error in UPNP_CONTROL_GET_VAR_REQUEST callback:\n");
//		syslog(LOG_ERR,"   Unknown variable name = %s\n",  cgv_event->StateVarName);
		cgv_event->ErrCode = 404;
		strcpy(cgv_event->ErrStr, "Invalid Variable");
    }
		pthread_mutex_unlock(&DevMutex);
		return(cgv_event->ErrCode == UPNP_E_SUCCESS);
}

int GateDeviceHandleActionRequest(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	int result = 0;
	
//syslog(LOG_ERR, "Action Request |%s|", ca_event->ServiceID);
	pthread_mutex_lock(&DevMutex);
//	if (strcmp(ca_event->DevUDN, gate->gate_udn) == 0)
//	{
		if (strcmp(ca_event->ServiceID,"urn:upnp-org:serviceId:WANIPConn1") ==0)
		{
				if (strcmp(ca_event->ActionName,"SetConnectionType") == 0)
					result=UPNP_E_SUCCESS;
//				     	result = GateDeviceSetConnectionType(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"GetConnectionTypeInfo") == 0)
					result = GateDeviceGetConnectionTypeInfo(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"RequestConnection") == 0)
					result=UPNP_E_SUCCESS;
//					result = GateDeviceRequestConnection(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"RequestTermination") == 0)
					result=UPNP_E_SUCCESS;
//					result = GateDeviceRequestTermination(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"ForceTermination") == 0)
					result=UPNP_E_SUCCESS;
//					result = GateDeviceForceTermination(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"SetAutoDisconnectTime") == 0)
					result=UPNP_E_SUCCESS;
//					result = GateDeviceSetAutoDisconnectTime(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"SetIdleDisconnectTime") == 0)
					result=UPNP_E_SUCCESS;
//					result = GateDeviceSetIdleDisconnectTime(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"SetWarnDisconnectDelay") == 0)
					result=UPNP_E_SUCCESS;
//					result = GateDeviceSetWarnDisconnectDelay(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"GetStatusInfo") == 0)
					result = GateDeviceGetStatusInfo(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"GetAutoDisconnectTime") == 0)
					result=UPNP_E_SUCCESS;
//					result = GateDeviceGetAutoDisconnectTime(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"GetIdleDisconnectTime") == 0)
					result=UPNP_E_SUCCESS;
//					result = GateDeviceGetIdleDisconnectTime(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"GetWarnDisconnectDelay") == 0)
					result=UPNP_E_SUCCESS;
//					result = GateDeviceGetWarnDisconnectDelay(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"GetNATRSIPStatus") == 0)
					result = GateDeviceGetNATRSIPStatus(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"GetGenericPortMappingEntry") == 0)
					result = GateDeviceGetGenericPortMappingEntry(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"GetSpecificPortMappingEntry") == 0)
					result = GateDeviceGetSpecificPortMappingEntry(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"GetExternalIPAddress") == 0)
					result = GateDeviceGetExternalIPAddress(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"AddPortMapping") == 0)
					result = GateDeviceAddPortMapping(gate, ca_event);
				else if (strcmp(ca_event->ActionName,"DeletePortMapping") == 0)
					result = GateDeviceDeletePortMapping(gate, ca_event);
				else result = 0;
		}
		else if (strcmp(ca_event->ServiceID,GateServiceId[GATE_SERVICE_CONFIG]) == 0)
		{
			if (strcmp(ca_event->ActionName,"GetCommonLinkProperties") == 0)
				result = GateDeviceGetCommonLinkProperties(gate, ca_event);
			else if (strcmp(ca_event->ActionName, "X") == 0)
				result = GateDeviceX(gate, ca_event);
			else if (strcmp(ca_event->ActionName,"GetTotalBytesSent") == 0)
				result = GateDeviceGetTotalBytesSent(gate, ca_event);
			else if (strcmp(ca_event->ActionName,"GetTotalBytesReceived") == 0)
				result = GateDeviceGetTotalBytesReceived(gate, ca_event);
			else if (strcmp(ca_event->ActionName,"GetTotalPacketsSent") == 0)
				result = GateDeviceGetTotalPacketsSent(gate, ca_event);
			else if (strcmp(ca_event->ActionName,"GetTotalPacketsReceived") == 0)
				result = GateDeviceGetTotalPacketsReceived(gate, ca_event);
			else if (strcmp(ca_event->ActionName,"GetEnabledForInternet") == 0)
				printf("GetEnabledForInternet!!!\n");
			else if (strcmp(ca_event->ActionName,"SetEnabledForInternet") == 0)
				printf("SetEnabledForInternet!!!\n");
			else result = 0;
		}
//	}
	
	pthread_mutex_unlock(&DevMutex);

	return(result);
}

int GateDeviceX(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	ca_event->ErrCode = 401;
	strcpy(ca_event->ErrStr, "Invalid Action");
	ca_event->ActionResult = NULL;
	return (ca_event->ErrCode);
}

int GateDeviceGetCommonLinkProperties(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char result_str[500];
	char tmp_str[500];

				sprintf(tmp_str, "<NewWANAccessType>Ethernet</NewWANAccessType><NewLayer1UpstreamMaxBitRate>%lu</NewLayer1UpstreamMaxBitRate><NewLayer1DownstreamMaxBitRate>%lu</NewLayer1DownstreamMaxBitRate><NewPhysicalLinkStatus>Up</NewPhysicalLinkStatus>",
								100*1000000,100*1000000);
        ca_event->ErrCode = UPNP_E_SUCCESS;
        sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>", ca_event->ActionName,
                "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
                tmp_str,ca_event->ActionName);
        ca_event->ActionResult = UpnpParse_Buffer(result_str);

        return(ca_event->ErrCode);


}

int GateDeviceGetTotalBytesSent(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char result_str[500];
	char dev[15];
	char *iface;
	FILE *stream;
	unsigned long bytes=0, total=0;

	/* Read sent from /proc */
	stream = fopen ( "/proc/net/dev", "r" );
	if ( stream != NULL )
	{
//		iface=m_ipcon->IPCon_GetIfName();
		iface=IPCon_GetIfName(gate->m_ipcon);
		while ( getc ( stream ) != '\n' );
		while ( getc ( stream ) != '\n' );

		while ( !feof( stream ) )
		{
			fscanf ( stream, "%[^:]:%*u %*u %*u %*u %*u %*u %*u %*u %lu %*u %*u %*u %*u %*u %*u %*u\n", dev, &bytes );
			if ( strcmp ( dev, iface )==0 )
				total += bytes;
		}
		fclose ( stream );
	}
	else
	{
		total=1;
	}

        ca_event->ErrCode = UPNP_E_SUCCESS;
        sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n<NewTotalBytesSent>%lu</NewTotalBytesSent>\n</u:%sResponse>",
		ca_event->ActionName,
                "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
                total, ca_event->ActionName);
        ca_event->ActionResult = UpnpParse_Buffer(result_str);

        return(ca_event->ErrCode);

}

int GateDeviceGetTotalBytesReceived(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char result_str[500];
	char dev[15];
        char *iface;
        FILE *stream;
        unsigned long bytes=0,total=0;

	/* Read received from /proc */
        stream = fopen ( "/proc/net/dev", "r" );
        if ( stream != NULL )
        {
//                iface=m_ipcon->IPCon_GetIfName();
                iface=IPCon_GetIfName(gate->m_ipcon);

                while ( getc ( stream ) != '\n' );
                while ( getc ( stream ) != '\n' );

                while ( !feof( stream ) )
                {
                        fscanf ( stream, "%[^:]:%lu %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u\n", dev, &bytes );
                        if ( strcmp ( dev, iface )==0 )
                                total += bytes;
                }
		fclose ( stream );
        }
	else
	{
                total=1;
        }

        ca_event->ErrCode = UPNP_E_SUCCESS;
        sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n<NewTotalBytesReceived>%lu</NewTotalBytesReceived>\n</u:%sResponse>",
		ca_event->ActionName,
                "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
                total, ca_event->ActionName );
        ca_event->ActionResult = UpnpParse_Buffer(result_str);

        return(ca_event->ErrCode);

}

int GateDeviceGetTotalPacketsSent(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char result_str[500];
	char dev[15];
        char *iface;
        FILE *stream;
        unsigned long pkt=0, total=0;

        /* Read sent from /proc */
        stream = fopen ( "/proc/net/dev", "r" );
        if ( stream != NULL )
        {
//                iface=m_ipcon->IPCon_GetIfName();
                iface=IPCon_GetIfName(gate->m_ipcon);
                while ( getc ( stream ) != '\n' );
                while ( getc ( stream ) != '\n' );

                while ( !feof( stream ) )
                {
                        fscanf ( stream, "%[^:]:%*u %*u %*u %*u %*u %*u %*u %*u %*u %lu %*u %*u %*u %*u %*u %*u\n", dev, &pkt );
                        if ( strcmp ( dev, iface )==0 )
                                total += pkt;
		}
                fclose ( stream );
        }
        else
        {
                total=1;
        }

        ca_event->ErrCode = UPNP_E_SUCCESS;
        sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n<NewTotalPacketsSent>%lu</NewTotalPacketsSent>\n</u:%sResponse>", ca_event->ActionName,
                "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
                total, ca_event->ActionName);
        ca_event->ActionResult = UpnpParse_Buffer(result_str);

        return(ca_event->ErrCode);

}

int GateDeviceGetTotalPacketsReceived(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char result_str[500];
	char dev[15];
        char *iface;
        FILE *stream;
        unsigned long pkt=0, total=0;

        /* Read sent from /proc */
        stream = fopen ( "/proc/net/dev", "r" );
        if ( stream != NULL )
        {
//                iface=m_ipcon->IPCon_GetIfName();
                iface=IPCon_GetIfName(gate->m_ipcon);
                while ( getc ( stream ) != '\n' );
                while ( getc ( stream ) != '\n' );

                while ( !feof( stream ) )
                {
                        fscanf ( stream, "%[^:]:%*u %lu %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u\n", dev, &pkt );
                        if ( strcmp ( dev, iface )==0 )
                                total += pkt;
                }
                fclose ( stream );
        }
        else
	{
                total=1;
        }

        ca_event->ErrCode = UPNP_E_SUCCESS;
        sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n<NewTotalPacketsReceived>%lu</NewTotalPacketsReceived>\n</u:%sResponse>", ca_event->ActionName,
                "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
                total, ca_event->ActionName);
        ca_event->ActionResult = UpnpParse_Buffer(result_str);

        return(ca_event->ErrCode);

}

#if 0
int GateDeviceSetConnectionType(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}
#endif

int GateDeviceGetConnectionTypeInfo(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char result_str[500];

	ca_event->ErrCode = UPNP_E_SUCCESS;
	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>", ca_event->ActionName,
		"urn:schemas-upnp-org:service:WANIPConnection:1",
		"<NewConnectionType>IP_Routed</NewConnectionType>\n<NewPossibleConnectionTypes>IP_Routed</NewPossibleConnectionTypes>",
		ca_event->ActionName);
	ca_event->ActionResult = UpnpParse_Buffer(result_str);

	return(ca_event->ErrCode);
}

#if 0
int GateDeviceRequestConnection(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}

int GateDeviceRequestTermination(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}

int GateDeviceForceTermination(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}

int GateDeviceSetAutoDisconnectTime(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}

int GateDeviceSetIdleDisconnectTime(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}

int GateDeviceSetWarnDisconnectDelay(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}
#endif

int GateDeviceGetStatusInfo(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{

	long int uptime;
	char result_str[500];
        
	uptime = (time(NULL) - gate->startup_time);
	ca_event->ErrCode = UPNP_E_SUCCESS;
        sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n<NewConnectionStatus>%s</NewConnectionStatus><NewLastConnectionError>ERROR_NONE</NewLastConnectionError><NewUptime>%li</NewUptime>\n</u:%sResponse>", ca_event->ActionName,
	                "urn:schemas-upnp-org:service:WANIPConnection:1",
	                "Connected",uptime,
	                ca_event->ActionName);
        ca_event->ActionResult = UpnpParse_Buffer(result_str);
        return(ca_event->ErrCode);
}

#if 0
int GateDeviceGetAutoDisconnectTime(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}

int GateDeviceGetIdleDisconnectTime(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}

int GateDeviceGetWarnDisconnectDelay(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	return(UPNP_E_SUCCESS);
}
#endif

int GateDeviceGetNATRSIPStatus(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char result_str[500];

	ca_event->ErrCode = UPNP_E_SUCCESS;
	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>", ca_event->ActionName,
		"urn:schemas-upnp-org:service:WANIPConnection:1",
		"<NewRSIPAvailable>0</NewRSIPAvailable>\n<NewNATEnabled>1</NewNATEnabled>",
		ca_event->ActionName);
	ca_event->ActionResult = UpnpParse_Buffer(result_str);

	return(ca_event->ErrCode);
}

int GateDeviceGetExternalIPAddress(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char result_str[500];
//	char result_parm[500];
	char result_parm[100];
	char *ip_address;

//	ip_address = m_ipcon->IPCon_GetIpAddrStr();	
	ip_address = IPCon_GetIpAddrStr(gate->m_ipcon);	

	ca_event->ErrCode = UPNP_E_SUCCESS;
	sprintf(result_parm,"<NewExternalIPAddress>%s</NewExternalIPAddress>", ip_address);
	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>", ca_event->ActionName,
		"urn:schemas-upnp-org:service:WANIPConnection:1",result_parm, ca_event->ActionName);
	ca_event->ActionResult = UpnpParse_Buffer(result_str);
	
	
	if (ip_address) free(ip_address);

	return(ca_event->ErrCode);
}

int GateDeviceGetGenericPortMappingEntry(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	int i = 0;
	int index = 0;
	char *mapindex = NULL;
	int action_succeeded = 0;
	char result_parm[500];
	char result_str[500];
	struct PortMap		*itr;		/* Needed for double link list */

	if ((mapindex = GetFirstDocumentItem(ca_event->ActionRequest, "NewPortMappingIndex")))
	{
		index = atoi(mapindex);
//		for (list<PortMap *>::iterator itr = m_list.m_pmap.begin(); itr != m_list.m_pmap.end(); itr++,i++)
		/* Change this to use a double link list */
		for (itr = gate->m_list.m_pmap; itr; itr=itr->next,i++)
		{
			if (i == index)
			{
				sprintf(result_parm,"<NewRemoteHost>%s</NewRemoteHost><NewExternalPort>%d</NewExternalPort><NewProtocol>%s</NewProtocol><NewInternalPort>%d</NewInternalPort><NewInternalClient>%s</NewInternalClient><NewEnabled>%d</NewEnabled><NewPortMappingDescription>%s</NewPortMappingDescription><NewLeaseDuration>%li</NewLeaseDuration>",
					"",itr->m_ExternalPort, itr->m_PortMappingProtocol, itr->m_InternalPort,
				       	itr->m_InternalClient, 1, itr->m_PortMappingDescription, itr->m_PortMappingLeaseDuration);		
				action_succeeded = 1;
			}
	
		}
		if (action_succeeded)
		{
			ca_event->ErrCode = UPNP_E_SUCCESS;
	                sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>", ca_event->ActionName,
	                        "urn:schemas-upnp-org:service:WANIPConnection:1",result_parm, ca_event->ActionName);
	                ca_event->ActionResult = UpnpParse_Buffer(result_str);
		}
		else
		{
			ca_event->ErrCode = 713;
                        strcpy(ca_event->ErrStr, "SpecifiedArrayIndexInvalid");
                        ca_event->ActionResult = NULL;
		}

	}
	else
	{
//	         syslog(LOG_DEBUG, "Failure in GateDeviceGetGenericortMappingEntry: Invalid Args");
	         ca_event->ErrCode = 402;
                 strcpy(ca_event->ErrStr, "Invalid Args");
                 ca_event->ActionResult = NULL;
	}

	if (mapindex) free(mapindex);
	return(ca_event->ErrCode);
}

int GateDeviceGetSpecificPortMappingEntry(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char *ext_port=NULL;
	char *proto=NULL;
	char result_parm[500];
	char result_str[500];
	int prt;
	int action_succeeded = 0;
	struct PortMap		*itr;		/* Needed for double link list */

	if ((ext_port = GetFirstDocumentItem(ca_event->ActionRequest, "NewExternalPort")) 
		&& (proto = GetFirstDocumentItem(ca_event->ActionRequest,"NewProtocol")))
	{
		prt = getProtoNum(proto);
		if ((prt == 17) || (prt == 6))
		{
//			for (list<PortMap *>::iterator itr = m_list.m_pmap.begin(); itr != m_list.m_pmap.end(); itr++)
			/* Change this to use a double link list */
			for (itr = gate->m_list.m_pmap; itr; itr=itr->next)
			{
				if (((itr->m_ExternalPort == atoi(ext_port)) && (strcmp(itr->m_PortMappingProtocol,proto) == 0)))
				{
					sprintf(result_parm,"<NewInternalPort>%d</NewInternalPort><NewInternalClient>%s</NewInternalClient><NewEnabled>1</NewEnabled><NewPortMappingDescription>%s</NewPortMappingDescription><NewLeaseDuration>%li</NewLeaseDuration>",
							itr->m_InternalPort, 
							itr->m_InternalClient,
							itr->m_PortMappingDescription,
							itr->m_PortMappingLeaseDuration);
					action_succeeded = 1;
				}
			}
			if (action_succeeded)
			{
				ca_event->ErrCode = UPNP_E_SUCCESS;
		                sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>", ca_event->ActionName,
		                        "urn:schemas-upnp-org:service:WANIPConnection:1",result_parm, ca_event->ActionName);
		                ca_event->ActionResult = UpnpParse_Buffer(result_str);
			}
			else
			{
//				syslog(LOG_DEBUG, "Failure in GateDeviceGetSpecificPortMappingEntry: PortMapping Doesn't Exist...");
		                ca_event->ErrCode = 714;
		                strcpy(ca_event->ErrStr, "NoSuchEntryInArray");
		                ca_event->ActionResult = NULL;
			}
		}
		else
		{
//		        syslog(LOG_DEBUG, "Failure in GateDeviceGetSpecificPortMappingEntry: Invalid NewProtocol=%s\n",proto);
                        ca_event->ErrCode = 402;
                        strcpy(ca_event->ErrStr, "Invalid Args");
                        ca_event->ActionResult = NULL;
		}
	}
	else
	{
//		syslog(LOG_DEBUG, "Failure in GateDeviceGetSpecificPortMappingEntry: Invalid Args");
		ca_event->ErrCode = 402;
		strcpy(ca_event->ErrStr, "Invalid Args");
		ca_event->ActionResult = NULL;
	}
	
	return (ca_event->ErrCode);
}

int GateDeviceAddPortMapping(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char *remote_host=NULL;
	char *ext_port=NULL;
	char *proto=NULL; 
	char *int_port=NULL;
	char *int_ip=NULL;
	char *desc=NULL;
	char *lease=NULL;
	int prt,result=0,extport_conflic=0;
	char num[5];
	char result_str[500];
	char *address = NULL;
	Upnp_Document  PropSet= NULL;	
	int action_succeeded = 0;
	struct PortMap		*itr;		/* Needed for double link list */
	struct PortMap		*itr_work;	/* Needed for double link list */
	char *test=NULL;
	
//	address = m_ipcon->IPCon_GetIpAddrStr();
	address = IPCon_GetIpAddrStr(gate->m_ipcon);
//syslog(LOG_ERR, "AddPortMapping");
	
	if (((ext_port = GetFirstDocumentItem(ca_event->ActionRequest, "NewExternalPort"))
		&& (proto    = GetFirstDocumentItem(ca_event->ActionRequest,"NewProtocol"))
		&& (int_port = GetFirstDocumentItem(ca_event->ActionRequest, "NewInternalPort"))
		&& (int_ip   = GetFirstDocumentItem(ca_event->ActionRequest, "NewInternalClient"))
		&& (desc     = GetFirstDocumentItem(ca_event->ActionRequest, "NewPortMappingDescription"))))
	{
		remote_host = GetFirstDocumentItem(ca_event->ActionRequest, "NewRemoteHost");
		lease = GetFirstDocumentItem(ca_event->ActionRequest, "NewLeaseDuration");
		if(!lease)
			lease = strdup('0');
		prt = getProtoNum(proto);
		if ((prt == 6) || (prt ==17) )
		{
			if (chkIPADDRstring(int_ip)!=0)
			{
//				for (list<PortMap *>::iterator itr=m_list.m_pmap.begin(); itr != m_list.m_pmap.end(); itr++)
				/* Change this to use a double link list */
				for (itr = gate->m_list.m_pmap; itr;)
				{
					if ((itr->m_ExternalPort == atoi(ext_port)) 
						&& (strcmp(itr->m_PortMappingProtocol,proto) == 0) 
						&& (strcmp(itr->m_InternalClient,int_ip) == 0))
					{
//						m_list.delPortForward(itr->m_PortMappingProtocol, 
						PortMap_delPortForward(itr->m_PortMappingProtocol, 
								itr->m_ExternalIP,
								itr->m_ExternalPort, itr->m_InternalClient,
								itr->m_InternalPort,itr->m_RemoteHost);
//						delete *itr;
//						m_list.m_pmap.erase(itr);
//						itr--;
						itr_work=itr;
						NODE_DEL(gate->m_list.m_pmap, itr_work);
						itr=itr_work->prev;
						if(!itr)
							itr=gate->m_list.m_pmap;
						//free(itr);
						free(itr_work);
					} else {
						//if the externel port are conflict with different client 
						if (itr->m_ExternalPort == atoi(ext_port)) 
						{
								extport_conflic=1;
								result=718;
								break;
						}		
						itr=itr->next;
					}
				}
			
//				result=m_list.PortMapAdd(NULL, proto, address, atoi(ext_port), int_ip, atoi(int_port), 1, desc, 0);
				if(!extport_conflic)
						result=PortMapAdd(&(gate->m_list), remote_host, proto, address, atoi(ext_port), int_ip, atoi(int_port), 1, desc, atoi(lease));
				if (result==1)
				{
					int size=0;
					for (itr = gate->m_list.m_pmap; itr; itr=itr->next) {
						size++;
					}
					sprintf(num,"%d",size);
					PropSet= UpnpCreatePropertySet(1,"PortMappingNumberOfEntries", num);
					UpnpNotifyExt(gate->device_handle, ca_event->DevUDN,ca_event->ServiceID,PropSet);
					UpnpDocument_free(PropSet);
//					syslog(LOG_ERR, "AddPortMap: RemoteHost: %s Prot: %d ExtPort: %d Int: %s.%d\n",
//						remote_host, prt, atoi(ext_port), int_ip, atoi(int_port));
					action_succeeded = 1;
					//refresh the upnp for WEB
					PortMapList_Refresh(&(gate->m_list));
				}
				else
				{
					if (result==718)
					{
//						syslog(LOG_ERR,"Failure in GateDeviceAddPortMapping: RemoteHost: %s Prot:%d ExtPort: %d Int: %s.%d\n",
//							remote_host,prt, atoi(ext_port),int_ip, atoi(int_port));
						ca_event->ErrCode = 718;
						strcpy(ca_event->ErrStr, "ConflictInMappingEntry");
						ca_event->ActionResult = NULL;
					}
				}
			}
			else
			{
//		                 syslog(LOG_ERR, "Failure in GateDeviceAddPortMapping: Invalid NewInternalClient=%s\n",int_ip);
                                 ca_event->ErrCode = 402;
                                 strcpy(ca_event->ErrStr, "Invalid Args");
                                 ca_event->ActionResult = NULL;
			}
		}
		else
		{
//		      syslog(LOG_ERR, "Failure in GateDeviceAddPortMapping: Invalid NewProtocol=%s\n",proto);
                      ca_event->ErrCode = 402;
                      strcpy(ca_event->ErrStr, "Invalid Args");
                      ca_event->ActionResult = NULL;
		}
	}
	else
	{
//		syslog(LOG_ERR, "Failiure in GateDeviceAddPortMapping: Invalid Arguments!");
                ca_event->ErrCode = 402;
                strcpy(ca_event->ErrStr, "Invalid Args");
                ca_event->ActionResult = NULL;
	}
	
	if (action_succeeded)
	{
		ca_event->ErrCode = UPNP_E_SUCCESS;
		sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>",
			ca_event->ActionName, "urn:schemas-upnp-org:service:WANIPConnection:1", "", ca_event->ActionName);
		ca_event->ActionResult = UpnpParse_Buffer(result_str);
	}

	if (ext_port) free(ext_port);
	if (int_port) free(int_port);
	if (proto) free(proto);
	if (int_ip) free(int_ip);
	if (desc) free(desc);
	if (remote_host) free(remote_host);	
	if(lease) free(lease);
	if (address) free(address);
	return(ca_event->ErrCode);
}

int GateDeviceDeletePortMapping(struct Gate *gate,
		struct Upnp_Action_Request *ca_event)
{
	char *ext_port=NULL;
	char *proto=NULL;
	int prt,result=0;
	char num[5];
	char result_str[500];
	Upnp_Document  PropSet= NULL;
	int action_succeeded = 0;
	struct PortMap		*itr;		/* Needed for double link list */

	if (((ext_port = GetFirstDocumentItem(ca_event->ActionRequest, "NewExternalPort")) &&
		(proto = GetFirstDocumentItem(ca_event->ActionRequest, "NewProtocol"))))
	{

		prt = getProtoNum(proto);
		if ((prt == 6) || (prt == 17))
		{
//			result=m_list.PortMapDelete(proto, atoi(ext_port));
			result=PortMapDelete(&(gate->m_list), proto, atoi(ext_port));
			if (result==1)
			{
				int size=0;
				for (itr = gate->m_list.m_pmap; itr; itr=itr->next) {
					size++;
				}
//				syslog(LOG_DEBUG, "DeletePortMap: Proto:%s Port:%s\n",proto, ext_port);
				sprintf(num,"%d",size);
				PropSet= UpnpCreatePropertySet(1,"PortMappingNumberOfEntries", num);
				UpnpNotifyExt(gate->device_handle, ca_event->DevUDN,ca_event->ServiceID,PropSet);
				UpnpDocument_free(PropSet);
				action_succeeded = 1;
				//refresh the upnp for WEB
				PortMapList_Refresh(&(gate->m_list));
			}
			else
			{
//				syslog(LOG_DEBUG, "Failure in GateDeviceDeletePortMapping: DeletePortMap: Proto:%s Port:%s\n",proto, ext_port);
				ca_event->ErrCode = 714;
				strcpy(ca_event->ErrStr, "NoSuchEntryInArray");
				ca_event->ActionResult = NULL;
			}
		}
		else
		{
//			syslog(LOG_DEBUG, "Failure in GateDeviceDeletePortMapping: Invalid NewProtocol=%s\n",proto);
			ca_event->ErrCode = 402;
                        strcpy(ca_event->ErrStr, "Invalid Args");
                        ca_event->ActionResult = NULL;
		}
	}
	else
	{
//	        syslog(LOG_DEBUG, "Failiure in GateDeviceDeletePortMapping: Invalid Arguments!");
       	        ca_event->ErrCode = 402;
       	        strcpy(ca_event->ErrStr, "Invalid Args");
	        ca_event->ActionResult = NULL;
	}

	if (action_succeeded)
	{
		ca_event->ErrCode = UPNP_E_SUCCESS;
		sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\">\n%s\n</u:%sResponse>",
			ca_event->ActionName, "urn:schemas-upnp-org:service:WANIPConnection:1", "", ca_event->ActionName);
		ca_event->ActionResult = UpnpParse_Buffer(result_str);
	}

	if (ext_port) free(ext_port);
	if (proto) free(proto);
	return(ca_event->ErrCode);
}

int upnp_send_byebye(char *DescDocURL)
{
	int i;
	char *dev_udn;
	Upnp_Document DescDoc = NULL;
	int ret = UPNP_E_SUCCESS;

	if (UpnpDownloadXmlDoc(DescDocURL, &DescDoc) != UPNP_E_SUCCESS)
	{
//		syslog(LOG_ERR, "DeviceStateTableInit -- Error Parsing %s\n", DescDocURL);
			ret =UPNP_E_INVALID_DESC;
	}
	dev_udn = GetFirstDocumentItem(DescDoc, "UDN");
	for(i=0;i<3;i++) 
		ServiceShutdown(dev_udn,GateServiceType[i],DescDocURL,1800);
	free(DescDoc);	
	free(dev_udn);
	
	return ret;
}


