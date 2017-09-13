#ifndef RADCLIDS_H
#define RADCLIDS_H

#include "radDef.h"

//Data Structure of Radius Server information.
typedef struct serverinfo{
	UINT32  ServerIPAddress;					//Radius Server IP Address.
	UINT16	ServerType;							//Authentication or Accounting server.
	UINT8	Secret[LENOFSECRET];				//Share Secret used in server and client.
	struct  serverinfo	*NextRadSrv;			//Point to the next Radius Server.
}RAD_SRV_INFO;

//Data Structure of Radius Server Table information.
typedef struct servertableinfo{
	UINT32  ServerIPAddress;					//Radius Server IP Address.
	UINT16	ServerType;							//Authentication or Accounting server.
	UINT8	Secret[LENOFSECRET];				//Share Secret used in server and client.
}RAD_SRV_TABLE_INFO;

typedef struct chapuser{
	UINT8   UserName[LENOFUSERNAME];			//CHAP User Name.
	UINT8	Identifier;							//CHAP Identifier.
	UINT8	Challenge[LENOFCHAPCHALLENGE];		//CHAP Challenge.
	UINT8	Response[LENOFCHAPRESPONSE];		//CHAP Response.
}CHAP_USER_INFO;

typedef struct papuser{
	UINT8	UserName[LENOFUSERNAME];			//PAP User Name.
	UINT8	UserPasswd[LENOFPWD];				//PAP Password.
}PAP_USER_INFO;

typedef struct otheruser{
	UINT8	UserName[LENOFUSERNAME];			//Other User Name.
	UINT8	UserPasswd[LENOFPWD];				//Other User Password.
}OTHER_USER_INFO;

typedef struct mschapuser{
	UINT8	UserName[LENOFUSERNAME];			//MSCHAP User Name.
	UINT8	UserPasswd[LENOFPWD];				//MSCHAP Password.
	UINT8	Identifier;							//MSCHAP Identifier.
	UINT8	Challenge[LENOFMSCHAPCHALLENGE];	//MSCHAP Challenge.
	UINT8	ResponseLM[LENOFMSCHAPLMRESPONSE];	//Lan Management.
	UINT8	ResponseNT[LENOFMSCHAPNTRESPONSE];	//NT.
}MSCHAP_USER_INFO;


typedef struct mschapcpw1{
	UINT8	UserName[LENOFUSERNAME];
	UINT8	UserPasswd[LENOFPWD];
	UINT8	Identifier;
	UINT8	LMOldPwd[LENOFMSCHAPLMOLDPWD];
	UINT8	LMNewPwd[LENOFMSCHAPLMNEWPWD];
	UINT8	NTOldPwd[LENOFMSCHAPNTOLDPWD];
	UINT8	NTNewPwd[LENOFMSCHAPNTNEWPWD];
	UINT16	NewLMPwdLength;
}MSCHAPCPW1_INFO;

typedef struct mschapcpw2{
	UINT8	UserName[LENOFUSERNAME];
	UINT8	UserPasswd[LENOFPWD];
	UINT8	Identifier;
	UINT8	OldNTHash[LENOFMSCHAPOLDNTHASH];
	UINT8	OldLMHash[LENOFMSCHAPOLDLMHASH];
	UINT8	LMResponse[LENOFMSCHAPLMRESPONSE];
	UINT8	NTResponse[LENOFMSCHAPNTRESPONSE];
	UINT8	LMEncriptedPwd[LENOFMSCHAPLMENCPWD];
	UINT8	NTEncriptedPwd[LENOFMSCHAPNTENCPWD];
}MSCHAPCPW2_INFO;

typedef struct eapuser{
	void    *portControl;
	UINT8	UserName[LENOFUSERNAME];
	UINT8	CalledStaId[LENOFSTAID];
	UINT8	CallingStaId[LENOFSTAID];
	UINT32	FramedMtu;
	UINT8	*EapMsg;	//[LENOFEAPMESSAGE];
	UINT8	*State;		//[LENOFEAPSTATE];
}EAP_USER_INFO;

typedef struct iappuser{
	UINT32	ClientInfo;
	UINT16  ServiceType;
	UINT8	UserName[LENOFUSERNAME];
	UINT8	CalledStaId[LENOFSTAID];
}IAPP_USER_INFO;

//Attribute's link list.
typedef struct attributetype{
	UINT8	AttrType;							//Type of the attribute.
	UINT32	NumericalValue;						//The numerical value of the attribute.
	UINT8	StringValue[LENOFSERVICESTRING];	//The string value of the attribute.
	struct	attributetype	*NextAttr;			//Pointer to the next attribute.
}ATTRIBUTE_TYPE;

//Request User Info data structure.
typedef struct userq{
	UINT8		UserName[LENOFUSERNAME];			//Name of the user.
	UINT8		UserPasswd[LENOFPWD];				//Password of the user.
	UINT8		RequestAuth[LENOFAUTHENTICATOR];	//The request authenticator transmitted.
	UINT8		*pPacketTransmitted;				//Pointer to the RADIUS packet transmitted.
	UINT32		SrvIP[NUMOFSERVER];					//IP of the Radius Server.
	void		(*CallBackfPtr)(UINT16 , UINT8 *);	//Call Back function pointer.
	UINT32		SendTime;							//Request send time.
	UINT8		PacketIdentifier;					//Identifier of the packet transmitted.
	UINT8		TxCount;							//Count the re-transmit times.
	UINT8		svrIdx;
	UINT8		dummy;
	void		*portControl;						//Used by EAP packet, must become a parameter and return back.
	UINT32		ClientInfo;                         //point to client_database struct
}USER_REQUEST_ENTRY;

//Authentication Input Info.
typedef	struct radinputauth{
	UINT8				ProtocolType;		//Protocol Type, like PRO_CHAP, PRO_PAP.
	CHAP_USER_INFO		*pUserInfoCHAP;		//CHAP user info.
	PAP_USER_INFO		*pUserInfoPAP;		//PAP user info.
	OTHER_USER_INFO		*pUserInfoOTHERS;	//Other user info.
	MSCHAP_USER_INFO	*pUserInfoMSCHAP;	//MSCHAP user info.
	MSCHAPCPW1_INFO		*pMsChapCpw1;		//Change user password info MSCHAP V1.
	MSCHAPCPW2_INFO		*pMsChapCpw2;		//Change user password info MSCHAP V2.
	EAP_USER_INFO		*pUserInfoEAP;		//EAP user info.
	IAPP_USER_INFO		*pUserInfoIAPP;		//IAPP user info.
	UINT32				NasIPAddress;		//IP Address of the Network Access Server.
	UINT8				NasId[LENOFNASID];	//Identifier of NAS.
	INT32				NasPort;			//Network Access Server port.
	INT32				NasPortType;		//Network Access Server Port Type.
	ATTRIBUTE_TYPE		*pAttributeType;	//Pointer to a data structure which save the info for services that can be hinted to server.
}RAD_CLI_AUTH_INPUT;

//Accounting Statistic.
typedef struct acctstat{
	UINT32	InputBytes;		//Total number of bytes received from the user.
	UINT32	OutputBytes;	//Total number of bytes sent to the user.
	UINT32	SessionTime;	//Duration of the session.
	UINT32	InputPackets;	//Total number of packets received from the user.
	UINT32	OutputPackets;	//Total no of packets sent to user.
	UINT32	TerminateCause;	//The cause of termination of the session.
}ACCOUNT_STAT;

//Accounting Input Info.
typedef struct radinputacc{
	UINT8	UserName[LENOFUSERNAME];	//User name.
	UINT8	SessionId[LENOFSESSIONID];	//Session Id.
	UINT32	NasIPAddress;				//IP Address of the Network Access Server.
	UINT8	NasId[LENOFNASID];			//Identifier of NAS.
	INT32	NasPort;					//Network Access Server Port.
	INT32	NasPortType;				//Network Access Server Port Type.
	UINT32	AuthType;					//Type of authentication.
	ACCOUNT_STAT	*pAccountStat;		//Accounting statistics to be recorded.
	ATTRIBUTE_TYPE	*pAttributeType;	//Pointer to a Attribute link list.
}RAD_CLI_ACC_INPUT;

#endif

