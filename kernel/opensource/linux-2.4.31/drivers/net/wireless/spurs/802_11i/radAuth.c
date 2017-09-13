
#ifdef WLAN_HOSTOS_LINUX
#include <linux/ctype.h>
#include <asm/byteorder.h>
#elif defined(WLAN_HOSTOS_VXWORKS)
#include <netinet/in.h>
#endif //WLAN_HOSTOS_LINUX

#define INCLUDE_OS_DEP
#include "wlan_include.h"

#include "ifx_wdrv_types.h"
#include "ifx_wdrv_debug.h"
#include "sibapi.h"
#include "if_wlan.h"
#include "radMain.h"
#include "ifxadm_wdrv_init.h"
#include "1xproto.h"
#include "inet_addr.h"

#include "radMain.h"
#include "radAuth.h"
#include "radDS.h"
#include "radDef.h"

static USER_REQUEST_ENTRY  *UserReqTmp;

#ifdef WLAN_HOSTOS_LINUX
extern struct socket *gRadAuthSocketId;
extern struct socket *gRadAccSocketId;
#elif defined(WLAN_HOSTOS_VXWORKS)
extern int gRadAuthSocketId;
extern int gRadAccSocketId;
#endif //WLAN_HOSTOS_LINUX

//This function is the interface that called by user to execute the authentication job.
//pRadInputAuth : Contain the kind of User info.
//pRadSrvAuth   : Contain the Radius Server info.
//CallBackfPtr  : Contain the Call Back Function.
INT16 RadAuthentication (RAD_CLI_AUTH_INPUT *pRadInputAuth, 
                         RAD_SRV_INFO *pRadSrvAuth,
                         void (*CallBackfPtr)(UINT16, UINT8 *))
{

    static  UINT8 RadPktAuth[LENOFTXPKT];
    UINT8   RadReqAuth[LENOFAUTHENTICATOR];
    UINT8   RadHiddenPwd[LENOFPWD]; // + LENOFPWDPAD];  if the length reach 128 bytes, then it need not padding, else just padding to 128 bytes.
    UINT8   *pUserPwd = NULL;
    UINT16  RadPktLen, HiddenPwdLen = 0;
    UINT32  RandNum;
    struct  sockaddr_in RadSrvAddr;
    INT16   i, SrvIndex = 0;

    //Check if the user request table is still having space or not.
    if (gNumOfUserEntry > MAXUSERREQUEST)
        return FAILED;

    bzero((void *)&RadSrvAddr, sizeof(RadSrvAddr));
    RadSrvAddr.sin_family = AF_INET;
    RadSrvAddr.sin_port = htons(RADIUSAUTHPORT);    //1812.

    RadSrvAddr.sin_addr.s_addr = htonl(pRadSrvAuth->ServerIPAddress);


    if (CheckInputOfAuth(pRadInputAuth) == FAILED){
        PRINTLOCM("CheckInputOfAuth return failed");
        return FAILED;
    }
    
    bzero((void *)&RadPktAuth, LENOFTXPKT);

    //Generating a random unpredictable 16 bytes number.
	for (i = 0; i < LENOFAUTHENTICATOR; i += sizeof(UINT32))
	{
            os_getRand((UINT8*)&RandNum, sizeof(RandNum));
		memcpy(RadReqAuth + i, &RandNum, sizeof(UINT32));
	}
    UserReqTmp = (USER_REQUEST_ENTRY *)KNL_KNL_MALLOC(sizeof(USER_REQUEST_ENTRY));
    //Starting to insert data into authentication packet.
    RadPktAuth[PKTTYPE] = ACCESSREQUEST;
    if(gRadPktIdentifier == 255)
        gRadPktIdentifier = 0;
    else
        gRadPktIdentifier++;
    RadPktAuth[PKTIDENTIFIER] = gRadPktIdentifier;
    memcpy(RadPktAuth + PKTAUTHENTICATOR, RadReqAuth, LENOFAUTHENTICATOR);
    //For User Request Table Use.
    memcpy(UserReqTmp->RequestAuth, RadReqAuth, LENOFAUTHENTICATOR);
    RadPktLen = 20;

    if (WDEBUG_LEVEL( _bDisp802_1x_) )
    {
        printf("\n[ RAD ] %s: Authenticator=\n   ",__FUNCTION__);
        for(i = 0; i < 16; i++)
            printf(" %02x",RadReqAuth[i]);
    }
    
    if (pRadInputAuth->ProtocolType == RADPAP)
        pUserPwd = pRadInputAuth->pUserInfoPAP->UserPasswd;
    else if (pRadInputAuth->ProtocolType == RADOTHERS)
        pUserPwd = pRadInputAuth->pUserInfoOTHERS->UserPasswd;
    else if (pRadInputAuth->ProtocolType == RADMSCHAP)
        pUserPwd = pRadInputAuth->pUserInfoMSCHAP->UserPasswd;
    else if (pRadInputAuth->ProtocolType == RADMSCHAPCPW1)
        pUserPwd = pRadInputAuth->pMsChapCpw1->UserPasswd;
    else if (pRadInputAuth->ProtocolType == RADMSCHAPCPW2)
        pUserPwd = pRadInputAuth->pMsChapCpw2->UserPasswd;

    if ((pUserPwd != NULL) || (pRadInputAuth->ProtocolType == RADIAPP))
    {
        if (pUserPwd != NULL)
            memcpy(UserReqTmp->UserPasswd, pUserPwd, strlen((char *)pUserPwd));
        
        HidePwdOfAuth(pUserPwd, RadReqAuth, RadHiddenPwd, &HiddenPwdLen, 
                ((char *)pRadSrvAuth->Secret));
    }
    else
    {
        UserReqTmp->UserPasswd[0] = 0;
    }

    FillAttributesOfAuth(pRadInputAuth, RadHiddenPwd, HiddenPwdLen, &RadPktLen, 
            RadPktAuth, pRadSrvAuth->Secret);

    //Transmitting packets.
    if ((i = sendto_async(gRadAuthSocketId, RadPktAuth, RadPktLen, 0, (struct sockaddr *)
            &RadSrvAddr, sizeof(RadSrvAddr))) < 0)
    {
        DEBUG_8202(_bDispErrors_, "Err %s: sendto FAILED",__FUNCTION__);
        goto _bad;
    }
    else
    {
        if (WDEBUG_LEVEL( _bDisp802_1x_) )
        {
            printf("\n[ RAD ] %s: sent %d bytes to %s, dump hex:",
                __FUNCTION__,i, inet_ntoa(RadSrvAddr.sin_addr));
            
            _1xDumpHex(RadPktAuth, 40); //i);
        }
        //Add the information of send packet to the user request table.
        for(i = 0; i < MAXUSERREQUEST; i++)
        {
            if (gUserReqTable[i] == 0)  //Find out the Empty entry to record user info.
            {
                UserReqTmp->pPacketTransmitted = (UINT8 *)KNL_KNL_MALLOC(RadPktLen);        
                memcpy(UserReqTmp->pPacketTransmitted, RadPktAuth, RadPktLen);
                SetRadSrv(pRadSrvAuth);
                for (SrvIndex = 0; SrvIndex < NUMOFSERVER; SrvIndex++)
                {
                    UserReqTmp->SrvIP[SrvIndex] = 0;
                }
                SrvIndex = 0;               
                while (pRadSrvAuth != NULL)
                {
                    UserReqTmp->SrvIP[SrvIndex] = pRadSrvAuth->ServerIPAddress;
                    pRadSrvAuth = pRadSrvAuth->NextRadSrv;
                    SrvIndex++;
                }
                UserReqTmp->CallBackfPtr = CallBackfPtr;
                UserReqTmp->SendTime = KNL_SECONDS();
                UserReqTmp->PacketIdentifier = gRadPktIdentifier;
                UserReqTmp->TxCount = MAXRETXCOUNT;
                gUserReqTable[i] = UserReqTmp;
                gNumOfUserEntry++;
                return SUCCESSFUL;
            }
        }
    }
    
_bad:
    KNL_MEM_DEALLOC(UserReqTmp);
    return FAILED;
}


//Checking the required inputs for a particular protocol type of authentication.
INT16 CheckInputOfAuth (RAD_CLI_AUTH_INPUT *pRadInputAuthTmp)
{

    CHAP_USER_INFO      *pChapUser;
    PAP_USER_INFO       *pPapUser;
    OTHER_USER_INFO     *pOtherUser;
    MSCHAP_USER_INFO    *pMschapUser;
    MSCHAPCPW1_INFO     *pMsChapCpw1;
    MSCHAPCPW2_INFO     *pMsChapCpw2;
    EAP_USER_INFO       *pEapUser;
    IAPP_USER_INFO      *pIappUser;

    if(pRadInputAuthTmp->ProtocolType == RADCHAP)
    {
        pChapUser = pRadInputAuthTmp->pUserInfoCHAP;
        if (pChapUser == NULL)
            return FAILED;
        if (strlen((char *)pChapUser->UserName) == 0 ||
            strlen((char *)pChapUser->Challenge) < 5 ||
            strlen((char *)pChapUser->Response) == 0)
            return FAILED;
        return SUCCESSFUL;
    }

    if (pRadInputAuthTmp->ProtocolType == RADPAP)
    {
        pPapUser = pRadInputAuthTmp->pUserInfoPAP;
        if (pPapUser == NULL)
            return FAILED;
        if (strlen((char *)pPapUser->UserName) == 0 ||
            strlen((char *)pPapUser->UserPasswd) == 0)
            return FAILED;
        return SUCCESSFUL;
    }

    if (pRadInputAuthTmp->ProtocolType == RADOTHERS)
    {
        pOtherUser = pRadInputAuthTmp->pUserInfoOTHERS;
        if (pOtherUser == NULL)
            return FAILED;
        if (strlen((char *)pOtherUser->UserName) == 0 ||
            strlen((char *)pOtherUser->UserPasswd) == 0)
            return FAILED;
        return SUCCESSFUL;
    }

    if (pRadInputAuthTmp->ProtocolType == RADMSCHAP)
    {
        pMschapUser = pRadInputAuthTmp->pUserInfoMSCHAP;
        if (pMschapUser == NULL)
            return FAILED;
        if (strlen((char *)pMschapUser->UserName) == 0 ||
            strlen((char *)pMschapUser->UserPasswd) == 0)
            return FAILED;
        return SUCCESSFUL;
    }

    if (pRadInputAuthTmp->ProtocolType == RADMSCHAPCPW1)
    {
        pMsChapCpw1 = pRadInputAuthTmp->pMsChapCpw1;
        if (pMsChapCpw1 == NULL)
            return FAILED;
        if (strlen((char *)pMsChapCpw1->UserName) == 0 ||
            strlen((char *)pMsChapCpw1->UserPasswd) == 0)
            return FAILED;
        return SUCCESSFUL;
    }

    if (pRadInputAuthTmp->ProtocolType == RADMSCHAPCPW2)
    {
        pMsChapCpw2 = pRadInputAuthTmp->pMsChapCpw2;
        if (pMsChapCpw2 == NULL)
            return FAILED;
        if (strlen((char *)pMsChapCpw2->UserName) == 0 ||
            strlen((char *)pMsChapCpw2->UserPasswd) == 0)
            return FAILED;
        return SUCCESSFUL;
    }

    if (pRadInputAuthTmp->ProtocolType == RADEAP)
    {
        pEapUser = pRadInputAuthTmp->pUserInfoEAP;
        if ((pEapUser == NULL) || (strlen((char *)pEapUser->UserName) == 0))
            return FAILED;
        return SUCCESSFUL;
    }
    
    if (pRadInputAuthTmp->ProtocolType == RADIAPP)
    {
        pIappUser = pRadInputAuthTmp->pUserInfoIAPP;
        if ((pIappUser == NULL) || (strlen((char *)pIappUser->UserName) == 0))
            return FAILED;
        return SUCCESSFUL;
    }

    return FAILED;
}


//Hiding user's password by MD5 algorithm.
void HidePwdOfAuth (UINT8 *UserPwd, UINT8 *ReqAuth, UINT8 *HiddenPwd,   
                    UINT16 *LenOfPwd, UINT8 *pSecret)                   
{

    UINT16  PwdLen, ByteCount16, i, j;  
    UINT8   Concatenated[LENOFSECRET + LENOFAUTHENTICATOR]; //Max length : 48+16=64 Bytes.
    UINT8   Digest[LENOFDIGEST], PreResult[LENOFDIGEST];
    MD5_CTX Context;

    //Clear the password array.
    memset(HiddenPwd, 0, LENOFPWD);

    if (UserPwd != NULL)
    {
        PwdLen = strlen((char *)UserPwd);
        memcpy(HiddenPwd, UserPwd, PwdLen);
    }
    else 
        PwdLen = 16;

    //Add padding array.
    if (PwdLen % 16)
        PwdLen = PwdLen + (16 - (PwdLen % 16));

    *LenOfPwd = PwdLen;
    memcpy(PreResult, ReqAuth, LENOFAUTHENTICATOR);
    bzero(&Context, sizeof(Context));

    DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: len %d, Secret: %s", __FUNCTION__,
            strlen(pSecret), pSecret);

    for (i = 1; i <= PwdLen / 16; i++)
    {
        MakeStrOfAuth(PreResult, Concatenated, &ByteCount16, pSecret);      
        
        MD5Init(&Context);
        MD5Update(&Context, Concatenated, (unsigned int)ByteCount16);
        MD5Final(&Context);
        memcpy((char *)Digest, (char *)Context.digest, LENOFDIGEST);

        for (j = 0; j < LENOFDIGEST; j++)
        {
            HiddenPwd[j + ((i - 1) * 16)] ^= Digest[j];
            PreResult[j] = HiddenPwd[j + ((i - 1) * 16)];
        }
    }
}


//Concatenates the shared secret and the another string.
void MakeStrOfAuth (UINT8 *InStr, UINT8 *OutStr, UINT16 *ByteCount16, UINT8 *pSecret)
{

    UINT16  i;

    memset(OutStr, 0, LENOFSECRET + LENOFAUTHENTICATOR);

    i = strlen((char *)pSecret);
    memcpy(OutStr, pSecret, i);
    memcpy(OutStr + i, InStr, LENOFDIGEST);
    *ByteCount16 = i + LENOFDIGEST;

    if (WDEBUG_LEVEL( _bDisp802_1x_) )
    {
        printf("\n[ RAD ] %s: OutStr=",__FUNCTION__);
        
        _1xDumpHex(OutStr, *ByteCount16);
    }

}


//Filling the attributes into packets.
void FillAttributesOfAuth (RAD_CLI_AUTH_INPUT *pRadInputAuthTmp, UINT8 Pwd[], 
        UINT16 PwdLen, UINT16 *PktLen, UINT8 RadPkt[], UINT8 * pSecret)
{
UINT8 ApSsid[33];
const char __digits[] = "0123456789abcdef";

    UINT8               StaId[2], Digest[16], Secret[LENOFSECRET];
    UINT16              PktLenTmp, ByteCount16, i, j;
    UINT32              ByteCount32, VendorId;
    CHAP_USER_INFO      *pChapUser;
    PAP_USER_INFO       *pPapUser;
    OTHER_USER_INFO     *pOtherUser;
    MSCHAP_USER_INFO    *pMschapUser;
    MSCHAPCPW1_INFO     *pMsChapCpw1;
    MSCHAPCPW2_INFO     *pMsChapCpw2;
    EAP_USER_INFO       *pEapUser = pRadInputAuthTmp->pUserInfoEAP;
    IAPP_USER_INFO      *pIappUser  = pRadInputAuthTmp->pUserInfoIAPP;
    ATTRIBUTE_TYPE      *pAttributeType;
    register char       *cp;

    PktLenTmp = *PktLen;
    if (pRadInputAuthTmp->ProtocolType == RADCHAP)
    {
        pChapUser = pRadInputAuthTmp->pUserInfoCHAP;

        RadPkt[PktLenTmp] = AUSERNAME;
        ByteCount16 = strlen((char *)pChapUser->UserName);
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pChapUser->UserName, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        //For User Request Table Use.
        memcpy(UserReqTmp->UserName, pChapUser->UserName, ByteCount16);

        RadPkt[PktLenTmp] = ACHAPPASSWORD;
        RadPkt[PktLenTmp + 1] = LENOFCHAPPASSWORD;
        RadPkt[PktLenTmp + 2] = pChapUser->Identifier;
        memcpy(RadPkt + PktLenTmp + 3, pChapUser->Response, LENOFCHAPRESPONSE);
        PktLenTmp += LENOFCHAPPASSWORD;

        RadPkt[PktLenTmp] = ACHAPCHALLENGE;
        ByteCount16 = strlen((char *)pChapUser->Challenge);
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pChapUser->Challenge, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        UserReqTmp->portControl = NULL;
    }

    else if (pRadInputAuthTmp->ProtocolType == RADPAP)
    {
        pPapUser = pRadInputAuthTmp->pUserInfoPAP;

        RadPkt[PktLenTmp] = AUSERNAME;
        ByteCount16 = strlen((char *)pPapUser->UserName);
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pPapUser->UserName, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        //For User Request Table Use.
        memcpy(UserReqTmp->UserName, pPapUser->UserName, ByteCount16);

        RadPkt[PktLenTmp] = AUSERPASSWORD;
        ByteCount16 = PwdLen;
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, Pwd, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        UserReqTmp->portControl = NULL;
    }

    else if (pRadInputAuthTmp->ProtocolType == RADOTHERS)
    {
        pOtherUser = pRadInputAuthTmp->pUserInfoOTHERS;

        RadPkt[PktLenTmp] = AUSERNAME;
        ByteCount16 = strlen((char *)pOtherUser->UserName);
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pOtherUser->UserName, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        //For User Request Table Use.   
        memcpy(UserReqTmp->UserName, pOtherUser->UserName, ByteCount16);

        RadPkt[PktLenTmp] = AUSERPASSWORD;
        ByteCount16 = PwdLen;
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, Pwd, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        UserReqTmp->portControl = NULL;
    }

    else if (pRadInputAuthTmp->ProtocolType == RADMSCHAP)
    {
        pMschapUser = pRadInputAuthTmp->pUserInfoMSCHAP;

        RadPkt[PktLenTmp] = AUSERNAME;
        ByteCount16 = strlen((char *)pMschapUser->UserName);
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pMschapUser->UserName, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        //For User Request Table Use.
        memcpy(UserReqTmp->UserName, pMschapUser->UserName, ByteCount16);

        RadPkt[PktLenTmp] = AUSERPASSWORD;
        ByteCount16 = PwdLen;
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, Pwd, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        *PktLen = PktLenTmp;

        //Filling the MSCHAP authentication attributes.
        //The variable "i" is used to count the length of the attribute "Vendor-Specific".
        RadPkt[PktLenTmp] = AVENDORSPECIFIC;
        VendorId = htonl(MSCHAPVENDORID);
        memcpy(RadPkt + PktLenTmp + 2, &VendorId, 4);
        PktLenTmp += 6;
        i = 6;

        if ((ByteCount16 = strlen((char *)pMschapUser->Challenge)))
        {
            RadPkt[PktLenTmp] = MSCHAPCHALLENGE;
            ByteCount16 = strlen((char *)pMschapUser->Challenge);
            RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
            memcpy(RadPkt + PktLenTmp + 2, pMschapUser->Challenge, ByteCount16);
            PktLenTmp += ByteCount16 + 2;
            i += ByteCount16 + 2; 
        }

        RadPkt[PktLenTmp] = MSCHAPRESPONSE;
        RadPkt[PktLenTmp + 1] = LENOFMSCHAPRESPONSE;
        RadPkt[PktLenTmp + 2] = pMschapUser->Identifier;
        RadPkt[PktLenTmp + 3] = MSCHAPRESPONSEFLAG;
        memcpy(RadPkt + PktLenTmp + 4, pMschapUser->ResponseLM, LENOFMSCHAPLMRESPONSE);
        memcpy(RadPkt + PktLenTmp + 4 + LENOFMSCHAPLMRESPONSE, pMschapUser->ResponseNT, LENOFMSCHAPNTRESPONSE);
        //+52bytes.
        PktLenTmp += LENOFMSCHAPRESPONSE;
        i += LENOFMSCHAPRESPONSE;
        //Save the length of attribute "Vendor-Specific" to it's length field.
        RadPkt[*PktLen + 1] = i;
        UserReqTmp->portControl = NULL;
    }

    else if (pRadInputAuthTmp->ProtocolType == RADMSCHAPCPW1)
    {
        pMsChapCpw1 = pRadInputAuthTmp->pMsChapCpw1;

        RadPkt[PktLenTmp] = AUSERNAME;
        ByteCount16 = strlen((char *)pMsChapCpw1->UserName);
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pMsChapCpw1->UserName, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        //For User Request Table Use.
        memcpy(UserReqTmp->UserName, pMsChapCpw1->UserName, ByteCount16);

        RadPkt[PktLenTmp] = AUSERPASSWORD;
        ByteCount16 = PwdLen;
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, Pwd, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        *PktLen = PktLenTmp;

        //Filling the MsChapCpw1 authentication attributes.
        //The variable "i" is used to count the length of the attribute "Vendor-Specific".
        RadPkt[PktLenTmp] = AVENDORSPECIFIC;
        VendorId = htonl(MSCHAPVENDORID);
        memcpy(RadPkt + PktLenTmp + 2, &VendorId, 4);
        PktLenTmp += 6;     
        i = 6;
        
        RadPkt[PktLenTmp] = MSCHAPCPW1;
        RadPkt[PktLenTmp + 1] = LENOFMSCHAPCPW1;
        RadPkt[PktLenTmp + 2] = MSCHAPCPW1CODE;
        RadPkt[PktLenTmp + 3] = pMsChapCpw1->Identifier;
        memcpy(RadPkt + PktLenTmp + 4, pMsChapCpw1->LMOldPwd, LENOFMSCHAPLMOLDPWD);
        memcpy(RadPkt + PktLenTmp + 4 + 16, pMsChapCpw1->LMNewPwd, LENOFMSCHAPLMNEWPWD);
        memcpy(RadPkt + PktLenTmp + 4 + 32, pMsChapCpw1->NTOldPwd, LENOFMSCHAPNTOLDPWD);
        memcpy(RadPkt + PktLenTmp + 4 + 48, pMsChapCpw1->NTNewPwd, LENOFMSCHAPNTNEWPWD);
        ByteCount16 = htons(pMsChapCpw1->NewLMPwdLength);
        memcpy(RadPkt + PktLenTmp + 4 + 64, &ByteCount16, 2);
        ByteCount16 = htons(MSCHAPCPW1FLAG);
        memcpy(RadPkt + PktLenTmp + 4 + 64 + 2, &ByteCount16, 2);
        PktLenTmp += LENOFMSCHAPCPW1;
        i += LENOFMSCHAPCPW1;
        RadPkt[*PktLen + 1] = i;
        UserReqTmp->portControl = NULL;
    }

    else if (pRadInputAuthTmp->ProtocolType == RADMSCHAPCPW2)
    {
        pMsChapCpw2 = pRadInputAuthTmp->pMsChapCpw2;

        RadPkt[PktLenTmp] = AUSERNAME;
        ByteCount16 = strlen((char *)pMsChapCpw2->UserName);
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pMsChapCpw2->UserName, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        //For User Request Table Use.
        memcpy(UserReqTmp->UserName, pMsChapCpw2->UserName, ByteCount16);

        RadPkt[PktLenTmp] = AUSERPASSWORD;
        ByteCount16 = PwdLen;
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, Pwd, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        *PktLen = PktLenTmp;

        //Filling the MsChapCpw2 authentication attributes.
        //The variable "i" is used to count the length of the attribute "Vendor-Specific".
        RadPkt[PktLenTmp] = AVENDORSPECIFIC;
        VendorId = htonl(MSCHAPVENDORID);
        memcpy(RadPkt + PktLenTmp + 2, &VendorId, 4);
        PktLenTmp += 6;
        i = 6;

        RadPkt[PktLenTmp] = MSCHAPCPW2;
        RadPkt[PktLenTmp + 1] = LENOFMSCHAPCPW2;
        RadPkt[PktLenTmp + 2] = MSCHAPCPW2CODE;
        RadPkt[PktLenTmp + 3] = pMsChapCpw2->Identifier;
        memcpy(RadPkt + PktLenTmp + 4, pMsChapCpw2->OldNTHash, LENOFMSCHAPOLDNTHASH);
        memcpy(RadPkt + PktLenTmp + 4 + 16, pMsChapCpw2->OldLMHash, LENOFMSCHAPOLDLMHASH);
        memcpy(RadPkt + PktLenTmp + 4 + 32, pMsChapCpw2->LMResponse, LENOFMSCHAPLMRESPONSE);
        memcpy(RadPkt + PktLenTmp + 4 + 56, pMsChapCpw2->NTResponse, LENOFMSCHAPNTRESPONSE);
        ByteCount16 = htons(MSCHAPCPW2FLAG);
        memcpy(RadPkt + PktLenTmp + 4 + 80, &ByteCount16, 2);
        PktLenTmp += LENOFMSCHAPCPW2;
        i += LENOFMSCHAPCPW2;
        RadPkt[*PktLen + 1] = i;
        
        *PktLen = PktLenTmp;
        ByteCount16 = 0;    //Record sequence number.
        //This part is for "LM-ENC-PW".
        for (j = 0; j < NUMOFFRAGSMSCHAPENCPW; j++)
        {
//          PktLenTmp = *PktLen;
            RadPkt[PktLenTmp] = AVENDORSPECIFIC;
            VendorId = htonl(MSCHAPVENDORID);
            memcpy(RadPkt + PktLenTmp + 2, &VendorId, 4);
            PktLenTmp += 6;
            i = 6;

            RadPkt[PktLenTmp] = MSCHAPLMENCPW;
            RadPkt[PktLenTmp + 1] = (LENOFMSCHAPENCPWFRAG + 6);
            RadPkt[PktLenTmp + 2] = MSCHAPLMENCPWCODE;
            RadPkt[PktLenTmp + 3] = pMsChapCpw2->Identifier;
            ByteCount16 = htons(ByteCount16++);
            memcpy(RadPkt + PktLenTmp + 4, &ByteCount16, 2);
            memcpy(RadPkt + PktLenTmp + 6, pMsChapCpw2->LMEncriptedPwd +
                   (j * LENOFMSCHAPENCPWFRAG), LENOFMSCHAPENCPWFRAG);
            PktLenTmp += (LENOFMSCHAPENCPWFRAG + 6);
            i += (LENOFMSCHAPENCPWFRAG + 6);
            RadPkt[*PktLen + 1] = i;
            *PktLen = PktLenTmp;
        }

        ByteCount16 = 0;    //Record sequence number.
        //This part is for "NT-ENC-PW".
        for (j = 0; j < NUMOFFRAGSMSCHAPENCPW; j++)
        {
//          PktLenTmp = *PktLen;
            RadPkt[PktLenTmp] = AVENDORSPECIFIC;
            VendorId = htonl(MSCHAPVENDORID);
            memcpy(RadPkt + PktLenTmp + 2, &VendorId, 4);
            PktLenTmp += 6;
            i = 6;

            RadPkt[PktLenTmp] = MSCHAPNTENCPW;
            RadPkt[PktLenTmp + 1] = LENOFMSCHAPENCPWFRAG + 6;
            RadPkt[PktLenTmp + 2] = MSCHAPNTENCPWCODE;
            RadPkt[PktLenTmp + 3] = pMsChapCpw2->Identifier;
            ByteCount16 = htons(ByteCount16++);
            memcpy(RadPkt + PktLenTmp + 4, &ByteCount16, 2);
            memcpy(RadPkt + PktLenTmp + 6, pMsChapCpw2->NTEncriptedPwd +
                   (j * LENOFMSCHAPENCPWFRAG), LENOFMSCHAPENCPWFRAG);
            PktLenTmp += (LENOFMSCHAPENCPWFRAG + 6);
            i += (LENOFMSCHAPENCPWFRAG + 6);
            RadPkt[*PktLen + 1] = i;
            *PktLen = PktLenTmp;
        }
        UserReqTmp->portControl = NULL;
    }
    else if (pRadInputAuthTmp->ProtocolType == RADEAP)
    {
        pEapUser = pRadInputAuthTmp->pUserInfoEAP;
        
        //User-Name
        RadPkt[PktLenTmp] = AUSERNAME;
        ByteCount16 = strlen((char *)pEapUser->UserName);
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pEapUser->UserName, ByteCount16);
                PktLenTmp += ByteCount16 + 2;
        //For User Request Table Use.   
        memcpy(UserReqTmp->UserName, pEapUser->UserName, ByteCount16);
    }
    else if (pRadInputAuthTmp->ProtocolType == RADIAPP)
    {
        pIappUser = pRadInputAuthTmp->pUserInfoIAPP;
        
        //User-Name
        RadPkt[PktLenTmp] = AUSERNAME;
        ByteCount16 = strlen((char *)pIappUser->UserName);
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pIappUser->UserName, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
        //For User Request Table Use.   
        memcpy(UserReqTmp->UserName, pIappUser->UserName, ByteCount16);
        
        RadPkt[PktLenTmp] = AUSERPASSWORD;
        ByteCount16 = PwdLen;
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, Pwd, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
    }

    //NAS Identifier.
    if ((ByteCount16 = strlen((char *)pRadInputAuthTmp->NasId)) != 0)
    {
        RadPkt[PktLenTmp] = ANASIDENTIFIER;
        RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
        memcpy(RadPkt + PktLenTmp + 2, pRadInputAuthTmp->NasId, ByteCount16);
        PktLenTmp += ByteCount16 + 2;
    }

    //NAS IP Address.
    if (pRadInputAuthTmp->NasIPAddress != 0x00000000 && pRadInputAuthTmp->NasIPAddress != 0xffffffff)
    {
        RadPkt[PktLenTmp] = ANASIPADDRESS;
        RadPkt[PktLenTmp + 1] = LENOFNASIPADDRESS;
        ByteCount32 = htonl(pRadInputAuthTmp->NasIPAddress);
        memcpy(RadPkt + PktLenTmp + 2, &ByteCount32, 4);
        PktLenTmp += LENOFNASIPADDRESS;
    }

    //NAS Port.
    if (pRadInputAuthTmp->NasPort != ANOATTRIBUTE)
    {
        RadPkt[PktLenTmp] = ANASPORT;
        RadPkt[PktLenTmp + 1] = LENOFNASPORT;
        ByteCount32 = htonl(pRadInputAuthTmp->NasPort);
        memcpy(RadPkt + PktLenTmp + 2, &ByteCount32, 4);
        PktLenTmp += LENOFNASPORT;
    }

    //NAS Port Type.
    if (pRadInputAuthTmp->NasPortType != ANOATTRIBUTE)
    {
        RadPkt[PktLenTmp] = ANASPORTTYPE;
        RadPkt[PktLenTmp + 1] = LENOFNASPORTTYPE;
        ByteCount32 = htonl(pRadInputAuthTmp->NasPortType);
        memcpy(RadPkt + PktLenTmp + 2, &ByteCount32, 4);
        PktLenTmp += LENOFNASPORTTYPE;
    }

    //Fill the other attributes info into the packet.
    pAttributeType = pRadInputAuthTmp->pAttributeType;
    for ( ; ; )
    {
        if (pAttributeType == NULL)
            break;

        RadPkt[PktLenTmp] = pAttributeType->AttrType;
        if ((ByteCount16 = strlen((char *)pAttributeType->StringValue)) != 0)
        {
            RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
            memcpy(RadPkt + PktLenTmp + 2, pAttributeType->StringValue, ByteCount16);
            PktLenTmp += ByteCount16 + 2;
        }
        else
        {
            RadPkt[PktLenTmp + 1] = LENOFNUMATTR;
            ByteCount32 = htonl(pAttributeType->NumericalValue);
            memcpy(RadPkt + PktLenTmp + 2, &ByteCount32, 4);
            PktLenTmp += LENOFNUMATTR;
        }
        pAttributeType = pAttributeType->NextAttr;
    }

    if (pRadInputAuthTmp->ProtocolType == RADEAP)
    {
        RadPkt[PktLenTmp] = ACALLEDSTATIONID;
        RadPkt[PktLenTmp + 1] = 0x0e;   //Fixed length.
        for (i = 0; i < 6; i++)
        {
            StaId[0] = pEapUser->CalledStaId[i] >> 4;
            StaId[1] = pEapUser->CalledStaId[i] & 0xf;
            
            if (StaId[0] > 0x09)
                RadPkt[PktLenTmp + 2 + (i * 2)] = tolower(StaId[0]) + 0x57;
            else
                RadPkt[PktLenTmp + 2 + (i * 2)] = StaId[0] + 0x30;

            if (StaId[1] > 0x9)
                RadPkt[PktLenTmp + 2 + (i * 2 + 1)] = tolower(StaId[1]) + 0x57;
            else
                RadPkt[PktLenTmp + 2 + (i * 2 + 1)] = StaId[1] + 0x30;
        }
        PktLenTmp += 14;
        
        RadPkt[PktLenTmp] = ACALLINGSTATIONID;
        RadPkt[PktLenTmp + 1] = 0x0e;   //Fixed length.
        for (i = 0; i < 6; i++)
        {
            StaId[0] = pEapUser->CallingStaId[i] >> 4;
            StaId[1] = pEapUser->CallingStaId[i] & 0xf;
            
            if (StaId[0] > 0x09)
                RadPkt[PktLenTmp + 2 + (i * 2)] = tolower(StaId[0]) + 0x57;
            else
                RadPkt[PktLenTmp + 2 + (i * 2)] = StaId[0] + 0x30;

            if (StaId[1] > 0x9)
                RadPkt[PktLenTmp + 2 + (i * 2 + 1)] = tolower(StaId[1]) + 0x57;
            else
                RadPkt[PktLenTmp + 2 + (i * 2 + 1)] = StaId[1] + 0x30;
        }
        PktLenTmp += 14;

        RadPkt[PktLenTmp] = AFRAMEDMTU;
        RadPkt[PktLenTmp + 1] = LENOFFRAMEDMTU;
        ByteCount32 = htonl(pEapUser->FramedMtu);
        memcpy(RadPkt + PktLenTmp + 2, &ByteCount32, 4);
        PktLenTmp += LENOFFRAMEDMTU;

        if (pEapUser->State[0] == ASTATE)
        {
            ByteCount16 = *(pEapUser->State + 1);
            memcpy(RadPkt + PktLenTmp, pEapUser->State, ByteCount16);
            PktLenTmp += ByteCount16;
        }
        
        //The EAP message must be cut to several fragment of length (255-2=253).
        memcpy(&ByteCount16, pEapUser->EapMsg + 2, 2);
        ByteCount16 = ntohs(ByteCount16);
        
        i = 0;
        if (ByteCount16 > 0xff)
        {
            while(ByteCount16 > 0xfd)
            {
                RadPkt[PktLenTmp] = AEAPMESSAGE;
                RadPkt[PktLenTmp + 1] = 0xff;
                memcpy(RadPkt + PktLenTmp + 2, pEapUser->EapMsg + (i * 0xfd), 0xfd);
                PktLenTmp += 0xff;              
                ByteCount16 -= 0xfd;
                i++;
            }
        }
        if ((ByteCount16 > 0) && (ByteCount16 < 0xfd))
        {
            RadPkt[PktLenTmp] = AEAPMESSAGE;
            RadPkt[PktLenTmp + 1] = ByteCount16 + 2;
            memcpy(RadPkt + PktLenTmp + 2, pEapUser->EapMsg + (i * 0xfd), ByteCount16);
            PktLenTmp += ByteCount16 + 2;
        }

        RadPkt[PktLenTmp] = AMESSAGEAUTHENTICATOR;
        RadPkt[PktLenTmp + 1] = LENOFMESSAGEAUTHENTICATOR;
        
        memset(&RadPkt[PktLenTmp + 2], 0 , 16);
        PktLenTmp += LENOFMESSAGEAUTHENTICATOR;
        ByteCount16 = htons(PktLenTmp);
        memcpy(RadPkt + PKTLENGTH, &ByteCount16, 2);    //Filling the length of Packet.
        
        memset(Secret, 0, LENOFSECRET);
        i = strlen(pSecret);
        memcpy(Secret, pSecret, i);
        HMAC_MD5(RadPkt, PktLenTmp, Secret, i, Digest);
        memcpy(RadPkt + PktLenTmp - 16, Digest, 16);

        UserReqTmp->portControl = pEapUser->portControl;
        //When the Call back function is called, we need to transmit portControl as a parameter.
    }
    else if (pRadInputAuthTmp->ProtocolType == RADIAPP)
    {
        //Service Type.
        RadPkt[PktLenTmp] = ASERVICETYPE;
        RadPkt[PktLenTmp + 1] = LENOFSERVICETYPE;
        ByteCount32 = htonl(pIappUser->ServiceType);
        memcpy(RadPkt + PktLenTmp + 2, &ByteCount32, 4);
        PktLenTmp += LENOFSERVICETYPE;
        
        RadPkt[PktLenTmp] = ACALLEDSTATIONID; // format: 00-10-A4-23-19-C0:AP1, where AP1 is SSID
        
        cp = &RadPkt[PktLenTmp + 2];
        for (i = 0; i < 6; i++)
        {
            *cp++ = __digits[pIappUser->CalledStaId[i] >> 4];
            *cp++ = __digits[pIappUser->CalledStaId[i] & 0xf];
            if (i == 5) *cp = ':';
            else        *cp++ = '-';
        }
        SIBCfg_ServiceArea(ApSsid);
        RadPkt[PktLenTmp + 1] = 20 + ApSsid[0];
        memcpy(&RadPkt[PktLenTmp + 20], ApSsid+1, ApSsid[0]);
        PktLenTmp += RadPkt[PktLenTmp + 1];
        
        RadPkt[PktLenTmp] = AMESSAGEAUTHENTICATOR;
        RadPkt[PktLenTmp + 1] = LENOFMESSAGEAUTHENTICATOR;
        
        memset(&RadPkt[PktLenTmp + 2], 0 , 16);
        PktLenTmp += LENOFMESSAGEAUTHENTICATOR;
        ByteCount16 = htons(PktLenTmp);
        memcpy(RadPkt + PKTLENGTH, &ByteCount16, 2);    //Filling the length of Packet.
        
        memset(Secret, 0, LENOFSECRET);
        i = strlen(pSecret);
        memcpy(Secret, pSecret, i);
        HMAC_MD5(RadPkt, PktLenTmp, Secret, i, Digest);
        memcpy(RadPkt + PktLenTmp - 16, Digest, 16);

        UserReqTmp->ClientInfo = pIappUser->ClientInfo;
        //When the Call back function is called, we need to transmit portControl as a parameter.
    }

    *PktLen = PktLenTmp;
    //It needs to be transfered by "htons".
    PktLenTmp = htons(PktLenTmp);
    memcpy(RadPkt + PKTLENGTH, &PktLenTmp, 2);  //Filling the length of Packet.

}
