///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000-2003 Intel Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <syslog.h>
#include "upnp_igd_ctrlpt.h"

char * IgdGetIpAddrStr(char *ifname);
long readcmd(const char *cmd, char **data);
long readfile(const char *pathname, char **data);

#define IGDC_ASYNC_RETRY 2 
#define IGDC_WAIT_RESP_TIME 5

static int AddPortActionDone=0;
static int async_retry=0;

/* Device totol Pormapping Number */
//int DeviceNumOfPortMapping;   
extern int DeviceNumOfPortCount;
extern int sip_ext_port[MAX_PORT_MAPPING];
extern int waittime_addport;
extern int ext_port_cnt;
/*
   Mutex for protecting the global device list
   in a multi-threaded, asynchronous environment.
   All functions should lock this mutex before reading
   or writing the device list. 
 */
ithread_mutex_t DeviceListMutex;

UpnpClient_Handle ctrlpt_handle = -1;

char *IgdDeviceType[] = {
		"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
		"urn:schemas-upnp-org:device:WANDevice:1",
		"urn:schemas-upnp-org:device:WANConnectionDevice:1"

};
char *IgdServiceType[] = {
    "urn:schemas-upnp-org:service:Layer3Forwarding:1",
    "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",
    "urn:schemas-upnp-org:service:WANIPConnection:1"
};
char *IgdServiceName[] = { 
		"Layer3Forwarding", 
		"WANCommonInterfaceConfig",
		"WANIPConnection"
};
char *IgdTagName[] = { 
		"deviceType", 
		"serviceType",
		"serviceId",
		"SCPDURL",
		"eventSubURL",
		"controlURL",
};
/*
   Global arrays for storing variable names and counts for 
   IGD_SERVICE_LAYER3FD, IGD_SERVICE_WANIFCONF and IGD_SERVICE_WANIPCONN services 
 */
char *IgdVarName[IGD_SERVICE_SERVCOUNT][IGD_MAXVARS] = {
    {"DefaultConnectionService", "", "", "","", "", "","", "", "","", "", "", "","", "", ""},
    {"EnabledForInternet","WANAccessType","Layer1UpstreamMaxBitRate","Layer1DownstreamMaxBitRate",
     "PhysicalLinkStatus","TotalBytesSent","TotalBytesReceived","TotalPacketsSent",
     "TotalPacketsReceived","","","","","","","",""},
    {"ConnectionType","PossibleConnectionTypes","ConnectionStatus","LastConnectionError",
     "Uptime","RSIPAvailable","NATEnabled","PortMappingNumberOfEntries",
     "RemoteHost","ExternalPort","PortMappingProtocol","InternalPort",
     "InternalClient","PortMappingEnabled","PortMappingDescription","PortMappingLeaseDuration",
     "ExternalIPAddress"}
};
char *GetGenericResp="GetGenericPortMappingEntryResponse";
char *GetNATRSIPResp="GetNATRSIPStatusResponse";
char *GetConnTypeInfo="GetConnectionTypeInfo";
char *GetExternalIP="GetExternalIPAddress";
char *GetStatusInfo="GetStatusInfo";
char *GetGenericPortMapping="GetGenericPortMappingEntry";
char *AddPortMapping="AddPortMapping";

char IgdVarCount[IGD_SERVICE_SERVCOUNT] =
    { IGD_LAYER3FD_VARCOUNT, IGD_WANIFCONF_VARCOUNT, IGD_WANIPCONN_VARCOUNT };

/*
   Timeout to request during subscriptions 
 */
int default_timeout = 1801;

/*
   The first node in the global device list, or NULL if empty 
 */
struct IgdDeviceNode *GlobalDeviceList = NULL;

struct IgdDevicePortMapInfo PortMapList[MAX_PORT_MAPPING+1];

/********************************************************************************
 * IgdCtrlPointDeleteNode
 *
 * Description: 
 *       Delete a device node from the global device list.  Note that this
 *       function is NOT thread safe, and should be called from another
 *       function that has already locked the global device list.
 *
 * Parameters:
 *   node -- The device node
 *
 ********************************************************************************/
int
IgdCtrlPointDeleteNode( struct IgdDeviceNode *node )
{
    int rc,
      service,
      var;

    if( NULL == node ) {
        SampleUtil_Print( "ERROR: IgdCtrlPointDeleteNode: Node is empty" );
        return IGD_ERROR;
    }

    for( service = 0; service < IGD_SERVICE_SERVCOUNT; service++ ) {
        /*
           If we have a valid control SID, then unsubscribe 
         */
        if( strcmp( node->device.IgdService[service].SID, "" ) != 0 ) {
            rc = UpnpUnSubscribe( ctrlpt_handle,
                                  node->device.IgdService[service].SID );
            if( UPNP_E_SUCCESS == rc ) {
                SampleUtil_Print
                    ( "Unsubscribed from Igd %s EventURL with SID=%s",
                      IgdServiceName[service],
                      node->device.IgdService[service].SID );
            } else {
                SampleUtil_Print
                    ( "Error unsubscribing to Igd %s EventURL -- %d",
                      IgdServiceName[service], rc );
            }
        }
    
        for( var = 0; var < IgdVarCount[service]; var++ ) {
            if( node->device.IgdService[service].VariableStrVal[var] ) {
                free( node->device.IgdService[service].
                      VariableStrVal[var] );
            }
        }
    }
		IgdCtrlPointFreeDeviceContent(node->device.devicecontent);
    SampleUtil_StateUpdate( NULL, NULL, node->device.UDN, DEVICE_REMOVED );
    // The device node removed, AddPortActionDone should be clear ...
    AddPortActionDone=0;
    free( node );
    node = NULL;

    return IGD_SUCCESS;
}

/********************************************************************************
 * IgdCtrlPointRemoveDevice
 *
 * Description: 
 *       Remove a device from the global device list.
 *
 * Parameters:
 *   UDN -- The Unique Device Name for the device to remove
 *
 ********************************************************************************/
int
IgdCtrlPointRemoveDevice( char *UDN )
{
    struct IgdDeviceNode *curdevnode,
     *prevdevnode;

    ithread_mutex_lock( &DeviceListMutex );

    curdevnode = GlobalDeviceList;
    if( !curdevnode ) {
        SampleUtil_Print
            ( "WARNING: IgdCtrlPointRemoveDevice: Device list empty" );
    } else {
        if( 0 == strcmp( curdevnode->device.UDN, UDN ) ) {
            GlobalDeviceList = curdevnode->next;
            IgdCtrlPointDeleteNode( curdevnode );
        } else {
            prevdevnode = curdevnode;
            curdevnode = curdevnode->next;

            while( curdevnode ) {
                if( strcmp( curdevnode->device.UDN, UDN ) == 0 ) {
                    prevdevnode->next = curdevnode->next;
                    IgdCtrlPointDeleteNode( curdevnode );
                    break;
                }

                prevdevnode = curdevnode;
                curdevnode = curdevnode->next;
            }
        }
    }

    ithread_mutex_unlock( &DeviceListMutex );

    return IGD_SUCCESS;
}

/********************************************************************************
 * IgdCtrlPointRemoveAll
 *
 * Description: 
 *       Remove all devices from the global device list.
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
int
IgdCtrlPointRemoveAll( void )
{
    struct IgdDeviceNode *curdevnode,
     *next;

    ithread_mutex_lock( &DeviceListMutex );

    curdevnode = GlobalDeviceList;
    GlobalDeviceList = NULL;

    while( curdevnode ) {
        next = curdevnode->next;
        IgdCtrlPointDeleteNode( curdevnode );
        curdevnode = next;
    }

    ithread_mutex_unlock( &DeviceListMutex );

    return IGD_SUCCESS;
}

/********************************************************************************
 * IgdCtrlPointRefresh
 *
 * Description: 
 *       Clear the current global device list and issue new search
 *	 requests to build it up again from scratch.
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
int
IgdCtrlPointRefresh( void )
{
    int rc;

    IgdCtrlPointRemoveAll(  );

    /*
       Search for all devices of type Igddevice version 1, 
       waiting for up to 5 seconds for the response 
     */
    rc = UpnpSearchAsync( ctrlpt_handle, IGDC_WAIT_RESP_TIME, IgdDeviceType[0], NULL );
    if( UPNP_E_SUCCESS != rc ) {
        SampleUtil_Print( "Error sending search request%d", rc );
        return IGD_ERROR;
    }

    return IGD_SUCCESS;
}

/********************************************************************************
 * IgdCtrlPointGetVar
 *
 * Description: 
 *       Send a GetVar request to the specified service of a device.
 *
 * Parameters:
 *   service -- The service
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   varname -- The name of the variable to request.
 *
 ********************************************************************************/
int
IgdCtrlPointGetVar( int service,
                   int devnum,
                   char *varname )
{
    struct IgdDeviceNode *devnode;
    int rc;

    ithread_mutex_lock( &DeviceListMutex );

    rc = IgdCtrlPointGetDevice( devnum, &devnode );

    if( IGD_SUCCESS == rc ) {
        rc = UpnpGetServiceVarStatusAsync( ctrlpt_handle,
                                           devnode->device.
                                           IgdService[service].ControlURL,
                                           varname,
                                           IgdCtrlPointCallbackEventHandler,
                                           NULL );
        if( rc != UPNP_E_SUCCESS ) {
            SampleUtil_Print
                ( "Error in UpnpGetServiceVarStatusAsync -- %d", rc );
            rc = IGD_ERROR;
        }
    }

    ithread_mutex_unlock( &DeviceListMutex );

    return rc;
}

/********************************************************************************
 * IgdCtrlPointSendAction
 *
 * Description: 
 *       Send an Action request to the specified service of a device.
 *
 * Parameters:
 *   service -- The service
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   actionname -- The name of the action.
 *   param_name -- An array of parameter names
 *   param_val -- The corresponding parameter values
 *   param_count -- The number of parameters
 *
 ********************************************************************************/
int
IgdCtrlPointSendAction( int service,
                       int devnum,
                       char *actionname,
                       char **param_name,
                       char **param_val,
                       int param_count )
{
    struct IgdDeviceNode *devnode;
    IXML_Document *actionNode = NULL;
    int rc = IGD_SUCCESS;
    int param;

    ithread_mutex_lock( &DeviceListMutex );

    rc = IgdCtrlPointGetDevice( devnum, &devnode );
    if( IGD_SUCCESS == rc ) {
        if( 0 == param_count ) {
            actionNode =
                UpnpMakeAction( actionname, IgdServiceType[service], 0,
                                NULL );
        } else {
            for( param = 0; param < param_count; param++ ) {
                if( UpnpAddToAction
                    ( &actionNode, actionname, IgdServiceType[service],
                      param_name[param],
                      param_val[param] ) != UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "ERROR: IgdCtrlPointSendAction: Trying to add action param" );
                    //return -1; // TBD - BAD! leaves mutex locked
                }
            }
        }

        rc = UpnpSendActionAsync( ctrlpt_handle,
                                  devnode->device.IgdService[service].
                                  ControlURL, IgdServiceType[service],
                                  NULL, actionNode,
                                  IgdCtrlPointCallbackEventHandler, NULL );

        if( rc != UPNP_E_SUCCESS ) {
            SampleUtil_Print( "Error in UpnpSendActionAsync -- %d", rc );
            rc = IGD_ERROR;
        }
    }

    ithread_mutex_unlock( &DeviceListMutex );

    if( actionNode )
        ixmlDocument_free( actionNode );

    return rc;
}

/********************************************************************************
 * IgdCtrlPointGetDevice
 *
 * Description: 
 *       Given a list number, returns the pointer to the device
 *       node at that position in the global device list.  Note
 *       that this function is not thread safe.  It must be called 
 *       from a function that has locked the global device list.
 *
 * Parameters:
 *   devnum -- The number of the device (order in the list,
 *             starting with 1)
 *   devnode -- The output device node pointer
 *
 ********************************************************************************/
int
IgdCtrlPointGetDevice( int devnum,
                      struct IgdDeviceNode **devnode )
{
    int count = devnum;
    struct IgdDeviceNode *tmpdevnode = NULL;

    if( count )
        tmpdevnode = GlobalDeviceList;

    while( --count && tmpdevnode ) {
        tmpdevnode = tmpdevnode->next;
    }

    if( !tmpdevnode ) {
        SampleUtil_Print( "Error finding IgdDevice number -- %d", devnum );
        return IGD_ERROR;
    }

    *devnode = tmpdevnode;
    return IGD_SUCCESS;
}

/********************************************************************************
 * IgdCtrlPointPrintList
 *
 * Description: 
 *       Print the universal device names for each device in the global device list
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
int
IgdCtrlPointPrintList(  )
{
    struct IgdDeviceNode *tmpdevnode;
    int i = 0;

    ithread_mutex_lock( &DeviceListMutex );

    SampleUtil_Print( "IgdCtrlPointPrintList:" );
    tmpdevnode = GlobalDeviceList;
    while( tmpdevnode ) {
        SampleUtil_Print( " %3d -- %s", ++i, tmpdevnode->device.UDN );
        tmpdevnode = tmpdevnode->next;
    }
    SampleUtil_Print( "" );
    ithread_mutex_unlock( &DeviceListMutex );

    return IGD_SUCCESS;
}

/********************************************************************************
 * IgdCtrlPointAddDevice
 *
 * Description: 
 *       If the device is not already included in the global device list,
 *       add it.  Otherwise, update its advertisement expiration timeout.
 *
 * Parameters:
 *   DescDoc -- The description document for the device
 *   location -- The location of the description document URL
 *   expires -- The expiration time for this advertisement
 *
 ********************************************************************************/
void
IgdCtrlPointAddDevice( IXML_Document * DescDoc,
                      char *location,
                      int expires )
{
    char *deviceType = NULL;
    char *friendlyName = NULL;
    char presURL[200];
    char *baseURL = NULL;
    char *relURL = NULL;
    char *UDN = NULL;
    char *serviceId[IGD_SERVICE_SERVCOUNT] = { NULL, NULL, NULL };
    char *eventURL[IGD_SERVICE_SERVCOUNT] = { NULL, NULL ,NULL};
    char *controlURL[IGD_SERVICE_SERVCOUNT] = { NULL, NULL ,NULL};
    Upnp_SID eventSID[IGD_SERVICE_SERVCOUNT];
    int TimeOut[IGD_SERVICE_SERVCOUNT] =
        { default_timeout, default_timeout, default_timeout};
    struct IgdDeviceNode *deviceNode;
    struct IgdDeviceNode *tmpdevnode;
    int ret = 1;
    int found = 0;
    int service, var;
    struct IgdDeviceContent *currDeviceContent;
    
    ithread_mutex_lock( &DeviceListMutex );
		
    /*
       Read key elements from description document 
     */
    UDN = SampleUtil_GetFirstDocumentItem( DescDoc, "UDN" );
    
    deviceType = SampleUtil_GetFirstDocumentItem( DescDoc, "deviceType" );
    friendlyName =  SampleUtil_GetFirstDocumentItem( DescDoc, "friendlyName" );
    baseURL = SampleUtil_GetFirstDocumentItem( DescDoc, "URLBase" );
    relURL = SampleUtil_GetFirstDocumentItem( DescDoc, "presentationURL" );
    ret = UpnpResolveURL( ( baseURL ? baseURL : location ),relURL,presURL );

    if( UPNP_E_SUCCESS != ret )
        SampleUtil_Print( "Error generating presURL from %s + %s", baseURL,
                          relURL );
    if( strcmp( deviceType, IgdDeviceType[0] ) == 0 ) {
        SampleUtil_Print( "Found Igd device" );

        // Check if this device is already in the list
        tmpdevnode = GlobalDeviceList;
        while( tmpdevnode ) {
            if( strcmp( tmpdevnode->device.UDN, UDN ) == 0 ) {
                found = 1;
                break;
            }
            tmpdevnode = tmpdevnode->next;
        }
        if( found ) {
            // The device is already there, so just update 
            // the advertisement timeout field
            tmpdevnode->device.AdvrTimeOut = expires;
        } else {
        		IXML_Document *SubDescDoc = NULL;
        		char *scpURL;
        		currDeviceContent = malloc(sizeof(struct IgdDeviceContent));
						IgdCtrlPointGetDeviceContent(DescDoc,currDeviceContent);
            /*
               Create a new device node 
             */
            deviceNode =
                ( struct IgdDeviceNode * )
                malloc( sizeof( struct IgdDeviceNode ) );
            strcpy( deviceNode->device.UDN, UDN );
            strcpy( deviceNode->device.DescDocURL, location );
            strcpy( deviceNode->device.FriendlyName, friendlyName );
            strcpy( deviceNode->device.PresURL, presURL );
            deviceNode->device.AdvrTimeOut = expires;

            for( service = 0; service < IGD_SERVICE_SERVCOUNT; service++ ) {
            		eventURL[service]=malloc( strlen( ( baseURL ? baseURL : location ) ) + strlen( currDeviceContent->eventURL[service] ) + 1 );
           					if( eventURL[service] ) 
                				UpnpResolveURL( ( baseURL ? baseURL : location ), currDeviceContent->eventURL[service], eventURL[service] );
            		controlURL[service]=malloc( strlen( ( baseURL ? baseURL : location ) ) + strlen( currDeviceContent->controlURL[service] ) + 1 );
           					if( controlURL[service] ) 
                				UpnpResolveURL( ( baseURL ? baseURL : location ), currDeviceContent->controlURL[service], controlURL[service] );
            		serviceId[service]=malloc( strlen( ( baseURL ? baseURL : location ) ) + strlen( currDeviceContent->serviceId[service] ) + 1 );
           					if( serviceId[service] ) 
                				UpnpResolveURL( ( baseURL ? baseURL : location ), currDeviceContent->serviceId[service], serviceId[service] );
            	
                strcpy( deviceNode->device.IgdService[service].ServiceId,
                        serviceId[service] );
                strcpy( deviceNode->device.IgdService[service].ServiceType,
                        IgdServiceType[service] );
                strcpy( deviceNode->device.IgdService[service].ControlURL,
                        controlURL[service] );
                strcpy( deviceNode->device.IgdService[service].EventURL,
                        eventURL[service] );
                
                for( var = 0; var < IgdVarCount[service]; var++ ) {
                    deviceNode->device.IgdService[service].
                        VariableStrVal[var] =
                        ( char * )malloc( IGD_MAX_VAL_LEN );
                    strcpy( deviceNode->device.IgdService[service].
                            VariableStrVal[var], "" );
                }
            }
            //keep the content of the device
            deviceNode->device.devicecontent=currDeviceContent;
            deviceNode->next = NULL;
            memset(&(deviceNode->device.deviceVarState),0,sizeof(struct IgdDeviceVarState));
            // Insert the new device node in the list
            if( ( tmpdevnode = GlobalDeviceList ) ) {

                while( tmpdevnode ) {
                    if( tmpdevnode->next ) {
                        tmpdevnode = tmpdevnode->next;
                    } else {
                        tmpdevnode->next = deviceNode;
                        break;
                    }
                }
            } else {
                GlobalDeviceList = deviceNode;
            }
            //Notify New Device Added
            SampleUtil_StateUpdate( NULL, NULL, deviceNode->device.UDN,
                                    DEVICE_ADDED );
            
            for( service = 0; service < IGD_SERVICE_SERVCOUNT; service++ ) {
            		scpURL =
                malloc( strlen( ( baseURL ? baseURL : location ) ) + strlen( currDeviceContent->scpdURL[service] ) + 1 );
                	
                if( scpURL ) 
                		UpnpResolveURL( ( baseURL ? baseURL : location ), currDeviceContent->scpdURL[service], scpURL );
                if( ( ret =UpnpDownloadXmlDoc( scpURL, &SubDescDoc )))
                {
                	SampleUtil_Print
                        ( "Error obtaining device description from %s",scpURL);
              	}	
                else
                {		
           					
                		ret =
                    		UpnpSubscribe( ctrlpt_handle, eventURL[service],
                        		           &TimeOut[service],
                            		       eventSID[service] );

                		if( ret == UPNP_E_SUCCESS ) {
                			 strcpy( deviceNode->device.IgdService[service].SID,
                       eventSID[service] );
                   		 SampleUtil_Print
                      		  ( "Subscribed to EventURL with SID=%s",
                          		eventSID[service] );
                       if(!service)
                       			UpnpUnSubscribe(ctrlpt_handle, eventSID[service]);  		
                       			
                		} else {
                    		SampleUtil_Print
                        		( "Error Subscribing to EventURL -- %d", ret );
                   		strcpy( eventSID[service], "" );
                		}
                }		
                if(scpURL)
                		free(scpURL);
                if(SubDescDoc)	
                		free(SubDescDoc);
            }  
        		upnpc_send_signal(IGDC_ADD_DEVICE);
        }
        
    }
    
    ithread_mutex_unlock( &DeviceListMutex );

    if( deviceType )
        free( deviceType );
    if( friendlyName )
        free( friendlyName );
    if( UDN )
        free( UDN );
    if( baseURL )
        free( baseURL );
    if( relURL )
        free( relURL );
    for( service = 0; service < IGD_SERVICE_SERVCOUNT; service++ ) {
        if( serviceId[service] )
            free( serviceId[service] );
        if( controlURL[service] )
            free( controlURL[service] );
        if( eventURL[service] )
            free( eventURL[service] );
    }
}

/********************************************************************************
 * IgdStateUpdate
 *
 * Description: 
 *       Update a Igd state table.  Called when an event is
 *       received.  Note: this function is NOT thread save.  It must be
 *       called from another function that has locked the global device list.
 *
 * Parameters:
 *   UDN     -- The UDN of the parent device.
 *   Service -- The service state table to update
 *   ChangedVariables -- DOM document representing the XML received
 *                       with the event
 *   State -- pointer to the state table for the Igd  service
 *            to update
 *
 ********************************************************************************/
void
IgdStateUpdate( char *UDN,
               int Service,
               IXML_Document * ChangedVariables,
               char **State )
{
    IXML_NodeList *properties,
     *variables;
    IXML_Element *property,
     *variable;
    int length,
      length1;
    int i,j;
    char *tmpstate = NULL;
    char *eventval;
    struct IgdDeviceNode *tmpdevnode;
		
    SampleUtil_Print( "Igd State Update (service %d): ", Service );

		tmpdevnode = GlobalDeviceList;
		eventval=SampleUtil_GetFirstDocumentItem( ChangedVariables, "e:ConnectionStatus" );
    if(eventval) //received connect Notify
    {
    		SampleUtil_Print("Received ConnectionStatus Notify");
    		if(strcmp(eventval,"Connected")==0)
    		{
    			
    			if(!tmpdevnode->device.deviceVarState.Conn_status)
    			{
    					tmpdevnode->device.deviceVarState.Conn_status=1;
    					upnpc_send_signal(IGDC_GET_CONN_NOTIFY);
    			}			
    		}
    		else
    		{
    			if(tmpdevnode->device.deviceVarState.Conn_status)
    			{
    					tmpdevnode->device.deviceVarState.Conn_status=0;
    					upnpc_send_signal(IGDC_GET_DISCONN_NOTIFY);
    					AddPortActionDone=0;
    			}	
    		}		
    		free(eventval);	
		}
}

/********************************************************************************
 * IgdCtrlPointHandleEvent
 *
 * Description: 
 *       Handle a UPnP event that was received.  Process the event and update
 *       the appropriate service state table.
 *
 * Parameters:
 *   sid -- The subscription id for the event
 *   eventkey -- The eventkey number for the event
 *   changes -- The DOM document representing the changes
 *
 ********************************************************************************/
void
IgdCtrlPointHandleEvent( Upnp_SID sid,
                        int evntkey,
                        IXML_Document * changes )
{
    struct IgdDeviceNode *tmpdevnode;
    int service;

    ithread_mutex_lock( &DeviceListMutex );
    tmpdevnode = GlobalDeviceList;
    
    while( tmpdevnode ) {
        for( service = 0; service < IGD_SERVICE_SERVCOUNT; service++ ) {
            if( strcmp( tmpdevnode->device.IgdService[service].SID, sid ) ==
                0 ) {
                SampleUtil_Print( "Received Igd %s Event: %d for SID %s",
                                  IgdServiceName[service], evntkey, sid );

                IgdStateUpdate( tmpdevnode->device.UDN, service, changes,
                               ( char ** )&tmpdevnode->device.
                               IgdService[service].VariableStrVal );
                break;
            }
        }
        tmpdevnode = tmpdevnode->next;
    }
    ithread_mutex_unlock( &DeviceListMutex );
}

/********************************************************************************
 * IgdCtrlPointHandleSubscribeUpdate
 *
 * Description: 
 *       Handle a UPnP subscription update that was received.  Find the 
 *       service the update belongs to, and update its subscription
 *       timeout.
 *
 * Parameters:
 *   eventURL -- The event URL for the subscription
 *   sid -- The subscription id for the subscription
 *   timeout  -- The new timeout for the subscription
 *
 ********************************************************************************/
void
IgdCtrlPointHandleSubscribeUpdate( char *eventURL,
                                  Upnp_SID sid,
                                  int timeout )
{
    struct IgdDeviceNode *tmpdevnode;
    int service;

    ithread_mutex_lock( &DeviceListMutex );

    tmpdevnode = GlobalDeviceList;
    while( tmpdevnode ) {
        for( service = 0; service < IGD_SERVICE_SERVCOUNT; service++ ) {

            if( strcmp
                ( tmpdevnode->device.IgdService[service].EventURL,
                  eventURL ) == 0 ) {
                SampleUtil_Print
                    ( "Received Igd %s Event Renewal for eventURL %s",
                      IgdServiceName[service], eventURL );
                strcpy( tmpdevnode->device.IgdService[service].SID, sid );
                break;
            }
        }

        tmpdevnode = tmpdevnode->next;
    }

    ithread_mutex_unlock( &DeviceListMutex );
}
IgdCtrlPointHandleGetVarError( char *controlURL,
                         			char *varName,
                         			DOMString varValue )
{
	 if(strcmp(varName, "PortMappingNumberOfEntries")==0)
	 {
	 		//We do'nt use now!
	 }	
}	                         			
void
IgdCtrlPointHandleGetVar( char *controlURL,
                         char *varName,
                         DOMString varValue )
{

    struct IgdDeviceNode *tmpdevnode;
    int service;

    ithread_mutex_lock( &DeviceListMutex );

    tmpdevnode = GlobalDeviceList;
    while( tmpdevnode ) {
        for( service = 0; service < IGD_SERVICE_SERVCOUNT; service++ ) {
            if( strcmp
                ( tmpdevnode->device.IgdService[service].ControlURL,
                  controlURL ) == 0 ) {
                SampleUtil_StateUpdate( varName, varValue,
                                        tmpdevnode->device.UDN,
                                        GET_VAR_COMPLETE );
                IgdGetVarStateUpdate( varName, varValue,
                                        tmpdevnode->device.UDN);                       
                break;
            }
        }
        tmpdevnode = tmpdevnode->next;
    }

    ithread_mutex_unlock( &DeviceListMutex );
}

/********************************************************************************
 * IgdCtrlPointCallbackEventHandler
 *
 * Description: 
 *       The callback handler registered with the SDK while registering
 *       the control point.  Detects the type of callback, and passes the 
 *       request on to the appropriate function.
 *
 * Parameters:
 *   EventType -- The type of callback event
 *   Event -- Data structure containing event data
 *   Cookie -- Optional data specified during callback registration
 *
 ********************************************************************************/
int
IgdCtrlPointCallbackEventHandler( Upnp_EventType EventType,
                                 void *Event,
                                 void *Cookie )
{
    SampleUtil_PrintEvent( EventType, Event );
    switch ( EventType ) {
            /*
               SSDP Stuff 
             */
        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
            {
            		
                struct Upnp_Discovery *d_event =
                    ( struct Upnp_Discovery * )Event;
                IXML_Document *DescDoc = NULL;
                int ret;

                if( d_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print( "Error in Discovery Callback -- %d",
                                      d_event->ErrCode );
                }
                if( ( ret =
                      UpnpDownloadXmlDoc( d_event->Location,
                                          &DescDoc ) ) !=
                    UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error obtaining device description from %s -- error = %d",
                          d_event->Location, ret );
                } else {
                		char *output,*ptr1,*ptr2;
                		int ret=0,len;
                		
                		/* Should we check the IP(device) same with out gateway */
                		ptr1= &(d_event->Location[7]); //http://
                		ptr2=strstr(ptr1,":");
                		if(ptr2)
                			len = (int)(ptr2-ptr1);
                		SampleUtil_Print("=====IgdCtrlPointAddDevice: d_event->Location=%s,DestAddr=%x,len=%d=====\n",d_event->Location,d_event->DestAddr->sin_addr.s_addr,len);
                		readcmd("netstat -rn|grep ^0.0.0.0|cut -c17-31",&output);
                		/* If the IP match then add device */
                		if(memcmp(ptr1,output,len)==0) 
                		{
                				SampleUtil_Print("=====IgdCtrlPointAddDevice: The IP and the gateway match!\n");	
                    		IgdCtrlPointAddDevice( DescDoc, d_event->Location,
                        		                  d_event->Expires );
                        ret = 1;		                  
                    }
                    else
                    		SampleUtil_Print("=====IgdCtrlPointAddDevice: The IP and the gateway did not match!\n");	
                    if(output)
                    	free(output);
                    if(!ret)
                    	break;	                      
										async_retry=0;                                          
                }

                if( DescDoc )
                    ixmlDocument_free( DescDoc );
								
                IgdCtrlPointPrintList(  );
                break;
            }

        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
            /*
               Nothing to do here... 
             */
            if(!GlobalDeviceList) //can not find any device, exit the process
    					upnpc_send_signal(IGDC_SHUT_DOWN);
            break;

        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
            {
                struct Upnp_Discovery *d_event =
                    ( struct Upnp_Discovery * )Event;

                if( d_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error in Discovery ByeBye Callback -- %d",
                          d_event->ErrCode );
                }

                SampleUtil_Print( "Received ByeBye for Device: %s",
                                  d_event->DeviceId );
                IgdCtrlPointRemoveDevice( d_event->DeviceId );

                SampleUtil_Print( "After byebye:" );
                IgdCtrlPointPrintList(  );

                break;
            }

            /*
               SOAP Stuff 
             */
        case UPNP_CONTROL_ACTION_COMPLETE:
            {
                struct Upnp_Action_Complete *a_event =
                    ( struct Upnp_Action_Complete * )Event;

                if( a_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error in  Action Complete Callback -- %d",
                          a_event->ErrCode );
                    IgdActionCallbackErr(a_event->ActionRequest);
                    break;      
                }

                /*
                   No need for any processing here, just print out results.  Service state
                   table updates are handled by events. 
                 */
                IgdActionStateUpdate(a_event->ActionResult); 
                
                break;
            }

        case UPNP_CONTROL_GET_VAR_COMPLETE:
            {
                struct Upnp_State_Var_Complete *sv_event =
                    ( struct Upnp_State_Var_Complete * )Event;

                if( sv_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error in Get Var Complete Callback -- %d",
                          sv_event->ErrCode );
                    IgdCtrlPointHandleGetVarError(sv_event->CtrlUrl,
                                             sv_event->StateVarName,
                                             sv_event->CurrentVal );      
                } else {
                    IgdCtrlPointHandleGetVar( sv_event->CtrlUrl,
                                             sv_event->StateVarName,
                                             sv_event->CurrentVal );
                }

                break;
            }

            /*
               GENA Stuff 
             */
        case UPNP_EVENT_RECEIVED:
            {
                struct Upnp_Event *e_event = ( struct Upnp_Event * )Event;
                IgdCtrlPointHandleEvent( e_event->Sid, e_event->EventKey,
                                        e_event->ChangedVariables );
                break;
            }

        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
        case UPNP_EVENT_RENEWAL_COMPLETE:
            {
                struct Upnp_Event_Subscribe *es_event =
                    ( struct Upnp_Event_Subscribe * )Event;

                if( es_event->ErrCode != UPNP_E_SUCCESS ) {
                    SampleUtil_Print
                        ( "Error in Event Subscribe Callback -- %d",
                          es_event->ErrCode );
                } else {
                    IgdCtrlPointHandleSubscribeUpdate( es_event->
                                                      PublisherUrl,
                                                      es_event->Sid,
                                                      es_event->TimeOut );
                }

                break;
            }

        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
            {
                int TimeOut = default_timeout;
                Upnp_SID newSID;
                int ret;

                struct Upnp_Event_Subscribe *es_event =
                    ( struct Upnp_Event_Subscribe * )Event;
                ret =
                    UpnpSubscribe( ctrlpt_handle, es_event->PublisherUrl,
                                   &TimeOut, newSID );

                if( ret == UPNP_E_SUCCESS ) {
                    SampleUtil_Print( "Subscribed to EventURL with SID=%s",
                                      newSID );
                    IgdCtrlPointHandleSubscribeUpdate( es_event->
                                                      PublisherUrl, newSID,
                                                      TimeOut );
                } else {
                    SampleUtil_Print
                        ( "Error Subscribing to EventURL -- %d", ret );
                }
                break;
            }

            /*
               ignore these cases, since this is not a device 
             */
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        case UPNP_CONTROL_GET_VAR_REQUEST:
        case UPNP_CONTROL_ACTION_REQUEST:
            break;
    }

    return 0;
}

/********************************************************************************
 * IgdCtrlPointVerifyTimeouts
 *
 * Description: 
 *       Checks the advertisement  each device
 *        in the global device list.  If an advertisement expires,
 *       the device is removed from the list.  If an advertisement is about to
 *       expire, a search request is sent for that device.  
 *
 * Parameters:
 *    incr -- The increment to subtract from the timeouts each time the
 *            function is called.
 *
 ********************************************************************************/
void
IgdCtrlPointVerifyTimeouts( int incr )
{
    struct IgdDeviceNode *prevdevnode,
     *curdevnode;
    int ret;
    static int dopormap=0;
    

    ithread_mutex_lock( &DeviceListMutex );

    prevdevnode = NULL;
    curdevnode = GlobalDeviceList;
    while( curdevnode ) {
        curdevnode->device.AdvrTimeOut -= incr;
        //SampleUtil_Print("Advertisement Timeout: %d\n", curdevnode->device.AdvrTimeOut);

        if( curdevnode->device.AdvrTimeOut <= 0 ) {
            /*
               This advertisement has expired, so we should remove the device
               from the list 
             */
            if( GlobalDeviceList == curdevnode )
                GlobalDeviceList = curdevnode->next;
            else
                prevdevnode->next = curdevnode->next;
            IgdCtrlPointDeleteNode( curdevnode );
            if( prevdevnode )
                curdevnode = prevdevnode->next;
            else
                curdevnode = GlobalDeviceList;
            if(GlobalDeviceList)  
            		async_retry=0;
        } else {

            if( curdevnode->device.AdvrTimeOut < 2 * incr ) {
                /*
                   This advertisement is about to expire, so send
                   out a search request for this device UDN to 
                   try to renew 
                 */
                ret = UpnpSearchAsync( ctrlpt_handle, incr,
                                       curdevnode->device.UDN, NULL );
                if( ret != UPNP_E_SUCCESS )
                    SampleUtil_Print
                        ( "Error sending search request for Device UDN: %s -- err = %d",
                          curdevnode->device.UDN, ret );
            }

            prevdevnode = curdevnode;
            curdevnode = curdevnode->next;
        }

    }
    if(!GlobalDeviceList)
    {
    	int rc;
    	//if device didn't exist in 5min refresh
    	if(async_retry >= IGDC_ASYNC_RETRY)
    	{
#if 0  // Consider shut down if 5 min not success  		
    			//SampleUtil_Print("UpnpSearchAsync\n");
    			rc = UpnpSearchAsync( ctrlpt_handle, IGDC_WAIT_RESP_TIME, IgdDeviceType[0], NULL );
   				if( UPNP_E_SUCCESS != rc ) {
        	SampleUtil_Print( "Error sending search request%d", rc );
    			}
#endif
					upnpc_send_signal(IGDC_SHUT_DOWN);    			
    			async_retry=0;
    	}
    	else
    			async_retry++;	
    	
  	}	
    ithread_mutex_unlock( &DeviceListMutex );
}

/********************************************************************************
 * IgdCtrlPointTimerLoop
 *
 * Description: 
 *       Function that runs in its own thread and monitors advertisement
 *       and subscription timeouts for devices in the global device list.
 *
 * Parameters:
 *    None
 *
 ********************************************************************************/
void *
IgdCtrlPointTimerLoop( void *args )
{
    int incr = 30;              // how often to verify the timeouts, in seconds
    int i=0;

    while( 1 ) {
    		
        isleep( 1 );
        if(i++ >= incr)
        {
        	IgdCtrlPointVerifyTimeouts( incr );
        	i = 0;
        }	
    }
}

/********************************************************************************
 * IgdCtrlPointStart
 *
 * Description: 
 *		Call this function to initialize the UPnP library and start the IGD Control
 *		Point.  This function creates a timer thread and provides a callback
 *		handler to process any UPnP events that are received.
 *
 * Parameters:
 *		None
 *
 * Returns:
 *		IGD_SUCCESS if everything went well, else IGD_ERROR
 *
 ********************************************************************************/
int
IgdCtrlPointStart( print_string printFunctionPtr,
                  state_update updateFunctionPtr ,char *ifname)
{
    ithread_t timer_thread;
    int rc;
    short int port = 0;
    char *ip_address = NULL;
    
		
    SampleUtil_Initialize( printFunctionPtr );
    SampleUtil_RegisterUpdateFunction( updateFunctionPtr );
		if(!(ip_address=IgdGetIpAddrStr(ifname)))
		{
			SampleUtil_Print( "Get interface ip fail");
			return IGD_ERROR;
		}	
		SampleUtil_Print( "UPnP interface ip %s",ip_address);
    ithread_mutex_init( &DeviceListMutex, 0 );

    SampleUtil_Print( "Intializing UPnP with ipaddress=%s port=%d",
                      ip_address, port );
    rc = UpnpInit( ip_address, port );
    if( UPNP_E_SUCCESS != rc ) {
        SampleUtil_Print( "WinCEStart: UpnpInit() Error: %d", rc );
        UpnpFinish(  );
        return IGD_ERROR;
    }

    if( NULL == ip_address )
        ip_address = UpnpGetServerIpAddress(  );
    if( 0 == port )
        port = UpnpGetServerPort(  );

    SampleUtil_Print( "UPnP Initialized (%s:%d)", ip_address, port );

    SampleUtil_Print( "Registering Control Point" );
    rc = UpnpRegisterClient( IgdCtrlPointCallbackEventHandler,
                             &ctrlpt_handle, &ctrlpt_handle );
    if( UPNP_E_SUCCESS != rc ) {
        SampleUtil_Print( "Error registering CP: %d", rc );
        UpnpFinish(  );
        return IGD_ERROR;
    }
    
		if(ip_address)
				free(ip_address);
				
    SampleUtil_Print( "Control Point Registered" );

    IgdCtrlPointRefresh(  );

    // start a timer thread
    ithread_create( &timer_thread, NULL, IgdCtrlPointTimerLoop, NULL );
    return IGD_SUCCESS;
}
/********************************************************************************
 * IgdCtrlPointStop
 *
 * Description: 
 *		Call this function to un-register the UPnP library and stop 
 *		the IGD Control point 
 *
 * Parameters:
 *		None
 *
 * Returns:
 *		IGD_SUCCESS if everything went well, else IGD_ERROR
 *
 ********************************************************************************/
int
IgdCtrlPointStop( void )
{
    IgdCtrlPointRemoveAll();
    UpnpUnRegisterClient( ctrlpt_handle );
    UpnpFinish();
    SampleUtil_Finish();

    return IGD_SUCCESS;
}

int
IgdCtrlPointGetDeviceContent(IXML_Document *DescDoc, struct IgdDeviceContent *DeviceContent)
{
		int i,j;
		IXML_NodeList *nodeList[6] ={NULL,NULL,NULL,NULL,NULL,NULL};
		IXML_NodeList *tmpnodeList[6] ={NULL,NULL,NULL,NULL,NULL,NULL};
    IXML_Node *textNode=NULL;
	
		for(i=0;i<6;i++)
		{
				tmpnodeList[i] = nodeList[i] = ixmlDocument_getElementsByTagName( DescDoc, IgdTagName[i] );
		}
		for(i=0;i<IGD_SERVICE_SERVCOUNT;i++)
		{
				if( nodeList[0] ) {
        		if( ( textNode = ixmlNodeList_item( nodeList[0], 0 ) ) ){ 
        				textNode = ixmlNode_getFirstChild( textNode );
        				if(ixmlNode_getNodeValue( textNode ))
									DeviceContent->deviceType[i]= strdup( ixmlNode_getNodeValue( textNode ) );
        		}
    		}	
    		
    		if( nodeList[1] ) {
        		if( ( textNode = ixmlNodeList_item( nodeList[1], 0 ) ) ){ 
        			  textNode = ixmlNode_getFirstChild( textNode );
        				if(ixmlNode_getNodeValue( textNode ))
									DeviceContent->serviceType[i]= strdup( ixmlNode_getNodeValue( textNode ) );
        		}
    		}
    		if( nodeList[2] ) {
        		if( ( textNode = ixmlNodeList_item( nodeList[2], 0 ) ) ){ 
        				textNode = ixmlNode_getFirstChild( textNode );	
        				if(ixmlNode_getNodeValue( textNode ))
									DeviceContent->serviceId[i]= strdup( ixmlNode_getNodeValue( textNode ) );
        		}
    		}
    		if( nodeList[3] ) {
        		if( ( textNode = ixmlNodeList_item( nodeList[3], 0 ) ) ){ 
        				textNode = ixmlNode_getFirstChild( textNode );
        				if(ixmlNode_getNodeValue( textNode ))
									DeviceContent->scpdURL[i]= strdup( ixmlNode_getNodeValue( textNode ) );
        		}
    		}
    		if( nodeList[4] ) {
        		if( ( textNode = ixmlNodeList_item( nodeList[4], 0 ) ) ){ 
        				textNode = ixmlNode_getFirstChild( textNode );
        				if(ixmlNode_getNodeValue( textNode ))
										DeviceContent->eventURL[i]= strdup( ixmlNode_getNodeValue( textNode ) );
        		}
    		}
    		if( nodeList[5] ) {
        		if( ( textNode = ixmlNodeList_item( nodeList[5], 0 ) ) ){ 
        				textNode = ixmlNode_getFirstChild( textNode );
        				if(ixmlNode_getNodeValue( textNode ))	
									DeviceContent->controlURL[i]= strdup( ixmlNode_getNodeValue( textNode ) );
        		}
    		}	
    		for(j=0;j<6;j++)
				{
						nodeList[j]=nodeList[j]->next;
				}	
		}	
		for(i=0;i<6;i++)
		{
				if(tmpnodeList[i])
						free(tmpnodeList[i]);
		}	
}	
int IgdCtrlPointFreeDeviceContent(struct IgdDeviceContent *DeviceContent)
{
		int i;
		for(i=0;i<IGD_SERVICE_SERVCOUNT;i++)
		{
					if(DeviceContent->deviceType[i])
							free(DeviceContent->deviceType[i]);
					if(DeviceContent->serviceType[i])
							free(DeviceContent->serviceType[i]);
					if(DeviceContent->serviceId[i])
							free(DeviceContent->serviceId[i]);
					if(DeviceContent->scpdURL[i])
							free(DeviceContent->scpdURL[i]);
					if(DeviceContent->eventURL[i])
							free(DeviceContent->eventURL[i]);
					if(DeviceContent->controlURL[i])
							free(DeviceContent->controlURL[i]);						
		}	
}	
void IgdActionCallbackErr(IXML_Document *request)
{
	char *xmlbuff = NULL;
	char *varVal=NULL;
	int i,fail_extport;
	
	xmlbuff = ixmlPrintDocument( request );
	
	if(strstr(xmlbuff,GetGenericPortMapping)!=0)
	{
			if(DeviceNumOfPortCount!=0)
				upnpc_send_signal(IGDC_SOAP_PORT_MAPPING);
			else
				upnpc_send_signal(IGDC_SOAP_PORT_MAPPING_FORCE);	
	}	
	if(strstr(xmlbuff,AddPortMapping)!=0)
	{
		varVal=SampleUtil_GetFirstDocumentItem( request, "NewExternalPort" );
		if(varVal)
		{
			waittime_addport = 0;
			fail_extport = atoi(varVal);
			for(i=0;i<MAX_PORT_MAPPING;i++)
			{
				if(sip_ext_port[i]==fail_extport)
				{
					sip_ext_port[i] = 0;
					break;
				}	
			}
			free(varVal);
		}	
	}	
	if(xmlbuff)
				ixmlFreeDOMString( xmlbuff );
		return;		
}	 
void
IgdActionStateUpdate( IXML_Document *result)
{	
		char *varVal=NULL;
		struct IgdDeviceNode *tmpdevnode;
		char *xmlbuff = NULL;
		int list_index;
		
		xmlbuff = ixmlPrintDocument( result );
		tmpdevnode = GlobalDeviceList;
		if(strstr(xmlbuff,GetGenericResp)!=0)
		{
			//SampleUtil_Print("@@@@@@@IgdActionStateUpdate@@@@@=%d,DeviceNumOfPortCount=%d,DeviceNumOfPortMapping=%d\n",DeviceNumOfPortMapping,DeviceNumOfPortCount,DeviceNumOfPortMapping);
			 list_index = DeviceNumOfPortCount;	
			 PortMapList[list_index].valid=1;	
			 varVal=SampleUtil_GetFirstDocumentItem( result, "NewRemoteHost" );
			 PortMapList[list_index].rm_host=varVal;
			 varVal=SampleUtil_GetFirstDocumentItem( result, "NewLeaseDuration" );
			 PortMapList[list_index].lease_time=varVal;
			 varVal=SampleUtil_GetFirstDocumentItem( result, "NewExternalPort" );
			 PortMapList[list_index].ext_port=varVal;
			 varVal=SampleUtil_GetFirstDocumentItem( result, "NewProtocol" );
  	 	 PortMapList[list_index].ext_proto=varVal;
			 varVal=SampleUtil_GetFirstDocumentItem( result, "NewInternalPort" );
 	 		 PortMapList[list_index].int_port=varVal;
			 varVal=SampleUtil_GetFirstDocumentItem( result, "NewInternalClient" );
			 if(varVal) //special case
			 	PortMapList[list_index].client_ip=varVal;
			 else
			 	PortMapList[list_index].client_ip=strdup("0.0.0.0");
			 varVal=SampleUtil_GetFirstDocumentItem( result, "NewPortMappingDescription" );		
 	 		 PortMapList[list_index].desc=varVal;		
			 DeviceNumOfPortCount++;
			 upnpc_send_signal(IGDC_GET_PORT_MAPPING_NUBER);
		}	
		if(strstr(xmlbuff,GetNATRSIPResp)!=0)
		{
			varVal=SampleUtil_GetFirstDocumentItem( result, "NewNATEnabled" );
			if(varVal)
			{
				if(strcmp(varVal,"1")==0)
						tmpdevnode->device.deviceVarState.nat_enable=1;
				else
						tmpdevnode->device.deviceVarState.nat_enable=0;
				free(varVal);
				varVal=NULL;
			}
		}	
		if(strstr(xmlbuff,GetConnTypeInfo)!=0)
		{
			varVal=SampleUtil_GetFirstDocumentItem( result, "NewConnectionType" );
			if(varVal)
			{
				if(strcmp(varVal,"IP_Routed")==0)
						tmpdevnode->device.deviceVarState.conn_type=1;
				else
						tmpdevnode->device.deviceVarState.conn_type=0;		
				free(varVal);
				varVal=NULL;
			}
		}	
		if(strstr(xmlbuff,GetExternalIP)!=0)
		{
			varVal=SampleUtil_GetFirstDocumentItem( result, "NewExternalIPAddress" );
			syslog(LOG_DEBUG, "%s:ExternalIPAddress: %s\n", __FUNCTION__, varVal);
			if(varVal)
			{
				strcpy(tmpdevnode->device.deviceVarState.externalIP,varVal);
				free(varVal);
				varVal=NULL;
			}
			else
			{
				syslog(LOG_DEBUG, "%s:ExternalIPAddress is NULL!\n", __FUNCTION__);
				strcpy(tmpdevnode->device.deviceVarState.externalIP,'0');	
			}	
		}
		if(strstr(xmlbuff,GetStatusInfo)!=0)
		{
			varVal=SampleUtil_GetFirstDocumentItem( result, "NewConnectionStatus" );
			if(varVal)
			{	
				if(strcmp(varVal,"Connected")==0)
    		{
    			
    			if(!tmpdevnode->device.deviceVarState.Conn_status)
    					tmpdevnode->device.deviceVarState.Conn_status=1;
    		}
    		free(varVal);
				varVal=NULL;
    	}	
		}
		if(strstr(xmlbuff,AddPortMapping)!=0)
		{
				waittime_addport = 0;
		}	
		IgdGetDeviceCanAddPort();
uleave:			 		
		if(xmlbuff)
				ixmlFreeDOMString( xmlbuff );
		return;			
}
int IgdGetDeviceCanAddPort(void)
{
		struct IgdDeviceNode *tmpdevnode;

		tmpdevnode = GlobalDeviceList;
		if(tmpdevnode)
		{
			if((tmpdevnode->device.deviceVarState.nat_enable==1)&&
					(tmpdevnode->device.deviceVarState.conn_type==1)&&
					(tmpdevnode->device.deviceVarState.Conn_status==1))
			{
				if(!AddPortActionDone)
				{
					upnpc_send_signal(IGDC_GET_PORT_MAPPING_NUBER);
					//upnpc_send_signal(IGDC_SOAP_PORT_MAPPING);
					AddPortActionDone=1;
					return 1;
				}	
			}	
		}	
		return 0;
}	
void
IgdGetVarStateUpdate(const char *varName,
                     const char *varValue,
                     const char *UDN)
{
		struct IgdDeviceNode *tmpdevnode;
		
		tmpdevnode = GlobalDeviceList;
		if(strcmp(varName,"ConnectionStatus")==0)
		{
				if(strcmp(varValue,"Connected")==0)
						tmpdevnode->device.deviceVarState.Conn_status=1;
				else
						tmpdevnode->device.deviceVarState.Conn_status=0;		
		}	
		if(strcmp(varName,"ExternalIPAddress")==0)
		{
			syslog(LOG_DEBUG, "%s:ExternalIPAddress: %s\n",__FUNCTION__,varName);
				if(varValue)
				{
						strcpy(tmpdevnode->device.deviceVarState.externalIP,varValue);
						//upnpc_send_signal(IGDC_GET_EXTERNALIP);
				}
				else
				{
					syslog(LOG_DEBUG, "%s:ExternalIPAddress is NULL!\n", __FUNCTION__);
					strcpy(tmpdevnode->device.deviceVarState.externalIP,'0');		
				}	
		}		
		IgdGetDeviceCanAddPort();
}
int IgdGetSockfd()
{
	static int sockfd = -1;
	
	if (sockfd == -1)
	{
		if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
		{
			perror("user: socket creating failed");
			return (-1);
		}
	}
	return sockfd;
}

int IgdExtrenalIPInfo(FILE	*fptr, int flag)
{
		struct IgdDeviceNode *tmpdevnode;
		
		tmpdevnode = GlobalDeviceList;
		
		if(!tmpdevnode)
				return -1;
    if(flag)
    {
    	if(tmpdevnode->device.deviceVarState.externalIP==0)
    		fprintf(fptr,"EXT_IP=%s\n","0");
	else	
    		fprintf(fptr,"EXT_IP=%s\n",tmpdevnode->device.deviceVarState.externalIP);
    }	
    else
    	fprintf(fptr,"EXT_IP%s\n","0.0.0.0");	
    return 0;
}

char * IgdGetIpAddrStr(char *ifname)
{
	struct ifreq ifr;
	struct sockaddr_in *saddr;
	int fd;
	char *address;
	
	address = (char*)malloc(20);
	fd = IgdGetSockfd();
	if (fd >= 0 )
	{
		strcpy(ifr.ifr_name, ifname);
		ifr.ifr_addr.sa_family = AF_INET;
		if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
		{
			saddr = (struct sockaddr_in *)&ifr.ifr_addr;
			strcpy(address,inet_ntoa(saddr->sin_addr));
			return address;
		}
		else
		{
			close(fd);
			return NULL;
		}
	}
	return NULL;
}


long
readcmd(const char *cmd, char **data)
{
	char buff[256];
	char filename[64];
	long len;

	snprintf(filename, 64, "/var/run/upnp.tmp.%d", getpid());
	snprintf(buff, 256, "%s > %s", cmd, filename);

	system(buff);
	len=readfile(filename, data);
	unlink(filename);
	
	return(len);
}
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h> 
#include <time.h> 

long
readfile(const char *pathname, char **data) 
{
	int fd;
	struct stat st;
	long len;

	fd = open (pathname, O_RDONLY);
	if (fd == -1) return -1;
	
	if (fstat (fd, &st) == -1) {
		close(fd);
		return -2;
	}
	
	*data = (char*)malloc(st.st_size + 1);
	if (*data==NULL) {
		close(fd);
		return -3;
	}
	
	len = read (fd, *data, st.st_size);
	if (len!=st.st_size) {
		close(fd);
		return -4;
	}
	close (fd);
        (*data)[len]='\0';

	return len;
}	
