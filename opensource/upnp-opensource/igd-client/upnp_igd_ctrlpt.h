#ifndef UPNP_IGD_CTRLPT_H
#define UPNP_IGD_CTRLPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "ithread.h"
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "upnp.h"
#include "upnptools.h"
#include "sample_util.h"

#define IGD_SERVICE_SERVCOUNT	3
#define IGD_SERVICE_LAYER3FD	0
#define IGD_SERVICE_WANIFCONF	1
#define IGD_SERVICE_WANIPCONN	2	

#define IGD_LAYER3FD_VARCOUNT		1
#define IGD_LAYER3FD_DEFCONNSERVICE		0

#define IGD_WANIFCONF_VARCOUNT		9
#define IGD_WANIFCONF_ENFORINET		0
#define IGD_WANIFCONF_WANAESTYPE	1
#define IGD_WANIFCONF_UPMAXBRATE 	2
#define IGD_WANIFCONF_DNMAXBRATE	3
#define IGD_WANIFCONF_PHYLNKSTS   4
#define IGD_WANIFCONF_TOBYTESENT  5
#define IGD_WANIFCONF_TOBYTERCV		6
#define IGD_WANIFCONF_TOPKTSENT		7
#define IGD_WANIFCONF_TOPKTRCV		8

#define IGD_WANIPCONN_VARCOUNT		17	
#define IGD_WANIPCONN_CONNTYPE			0
#define IGD_WANIPCONN_POSSCONNTYPE 	1
#define IGD_WANIPCONN_CONNSTATUS		2
#define IGD_WANIPCONN_LSTCONNERR		3
#define IGD_WANIPCONN_UPTIME				4
#define IGD_WANIPCONN_RSIPAVAL			5
#define IGD_WANIPCONN_NATENABLE			6
#define IGD_WANIPCONN_PORTMAPNUMENT 7
#define IGD_WANIPCONN_REMOTEHOST		8
#define IGD_WANIPCONN_EXTPORT				9
#define IGD_WANIPCONN_PPORMAPPROT		10
#define IGD_WANIPCONN_INTPORT				11
#define IGD_WANIPCONN_INTCLIENT			12
#define IGD_WANIPCONN_PORTMAPEN			13
#define IGD_WANIPCONN_PORTMAPDESC		14
#define IGD_WANIPCONN_PORTMAPLSDU		15
#define IGD_WANIPCONN_EXTIPADDR			16

#define IGD_MAX_VAL_LEN			18

#define IGD_SUCCESS				0
#define IGD_ERROR				(-1)
#define IGD_WARNING				1

/* This should be the maximum VARCOUNT from above */
#define IGD_MAXVARS				IGD_WANIPCONN_VARCOUNT

#define IGDC_ADD_DEVICE							  0
#define IGDC_DEL_DEVICE							  1
#define IGDC_SOAP_PORT_MAPPING			  2
#define IGDC_SOAP_PORT_MAPPING_FORCE	3	
#define IGDC_GET_DISCONN_NOTIFY				4
#define IGDC_GET_CONN_NOTIFY					5
#define IGDC_GET_EXTERNALIP						6
#define IGDC_GET_PORT_MAPPING_NUBER		7
#define IGDC_GET_PORT_MAPPING_INDEX 	8	
#define IGDC_SHUT_DOWN              	9

#define MAX_PORT_MAPPING 256


extern char IGDDeviceType[];
extern char *IgdServiceType[];
extern char *IgdServiceName[];
extern char *IgdVarName[IGD_SERVICE_SERVCOUNT][IGD_MAXVARS];
extern char IgdVarCount[];

struct igd_service {
    char ServiceId[NAME_SIZE];
    char ServiceType[NAME_SIZE];
    char *VariableStrVal[IGD_MAXVARS];
    char EventURL[NAME_SIZE];
    char ControlURL[NAME_SIZE];
    char SID[NAME_SIZE];
};

extern struct IgdDeviceNode *GlobalDeviceList;

struct IgdDeviceContent {
		char *deviceType[IGD_SERVICE_SERVCOUNT];
		char *serviceType[IGD_SERVICE_SERVCOUNT];	
    char *serviceId[IGD_SERVICE_SERVCOUNT];
    char *scpdURL[IGD_SERVICE_SERVCOUNT];
    char *eventURL[IGD_SERVICE_SERVCOUNT];
    char *controlURL[IGD_SERVICE_SERVCOUNT];
};

struct IgdDeviceVarState {
		int nat_enable;
		int conn_type;
		int Conn_status;
		char externalIP[20];
};

struct IgdDevice {
    char UDN[250];
    char DescDocURL[250];
    char FriendlyName[250];
    char PresURL[250];
    int  AdvrTimeOut;
    struct igd_service IgdService[IGD_SERVICE_SERVCOUNT];
    struct IgdDeviceContent *devicecontent;
    struct IgdDeviceVarState deviceVarState;
};

struct IgdDeviceNode {
    struct IgdDevice device;
    struct IgdDeviceNode *next;
};

struct IgdDevicePortMapInfo {
		int valid;
		char *rm_host;
    char *ext_port;
    char *ext_proto;
    char *int_port;
    char *client_ip;
    char *lease_time;
    char *desc;
};


extern ithread_mutex_t DeviceListMutex;

extern UpnpClient_Handle ctrlpt_handle;

int		IgdCtrlPointDeleteNode(struct IgdDeviceNode*);
int		IgdCtrlPointRemoveDevice(char*);
int		IgdCtrlPointRemoveAll( void );
int		IgdCtrlPointRefresh( void );


int		IgdCtrlPointSendAction(int, int, char *, char **, char **, int);
int		IgdCtrlPointSendActionNumericArg(int devnum, int service, char *actionName, char *paramName, int paramValue);

int		IgdCtrlPointGetVar(int, int, char*);
int		IgdCtrlPointGetDevice(int, struct IgdDeviceNode **);
int		IgdCtrlPointPrintList( void );
int		IgdCtrlPointPrintDevice(int);
void	IgdCtrlPointAddDevice (IXML_Document *, char *, int); 
void    IgdCtrlPointHandleGetVar(char *,char *,DOMString);
void	IgdStateUpdate(char*,int, IXML_Document * , char **);
void	IgdCtrlPointHandleEvent(Upnp_SID, int, IXML_Document *); 
void	IgdCtrlPointHandleSubscribeUpdate(char *, Upnp_SID, int); 
int		IgdCtrlPointCallbackEventHandler(Upnp_EventType, void *, void *);
void	IgdCtrlPointVerifyTimeouts(int);
int		IgdCtrlPointStart( print_string printFunctionPtr, state_update updateFunctionPtr ,char *ifname);
int		IgdCtrlPointStop( void );

void upnpc_sp_setup(void);
int upnpc_sp_fd_set(fd_set *rfds);
int upnpc_sp_read(fd_set *rfds);

#ifdef __cplusplus
};
#endif

#endif //UPNP_IGD_CTRLPT_H
