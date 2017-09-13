#ifndef RADCLIVAR_H
#define RADCLIVAR_H

#include "radDef.h"
#include "radDS.h"

extern	UINT8	gRadPktIdentifier;		//Packet Identifier.
extern	UINT8	gNumOfRadSrv;			//Number of Radius Server.

#ifdef WLAN_HOSTOS_LINUX
extern struct socket *gRadAuthSocketId;
extern struct socket *gRadAccSocketId;
#elif defined(WLAN_HOSTOS_VXWORKS)
extern	int	gRadAuthSocketId;		//Socket Identifier for Authentication.
extern	int	gRadAccSocketId;		//Socket Identifier for Accounting.
#endif //WLAN_HOSTOS_LINUX

extern	UINT8	gNumOfUserEntry;

//Table for holding the pointers of the servers.
extern	RAD_SRV_TABLE_INFO	*gSrvTable[NUMOFSERVER];

//Table for holding the pointers of the user request.
extern	USER_REQUEST_ENTRY	*gUserReqTable[MAXUSERREQUEST];

extern RAD_SRV_INFO RadiusServer[];

#endif


