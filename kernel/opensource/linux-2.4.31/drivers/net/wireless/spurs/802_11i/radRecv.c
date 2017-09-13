#ifdef WLAN_HOSTOS_LINUX
#include <linux/socket.h>
#include <net/sock.h>
#endif //WLAN_HOSTOS_LINUX

#define INCLUDE_OS_DEP
#include "wlan_include.h"

#include "ifx_wdrv_types.h"
#include "ifx_wdrv_debug.h"
#include "ifxadm_wdrv_init.h"
#include "radMain.h"
#include "inet_addr.h"
#include "1xproto.h"
#include "radRecv.h"
#include "radMD5.h"
#include "radDef.h"


const UINT8 AttrLenTable[] = {
    0, 0, 0, LENOFCHAPPASSWORD, LENOFNASIPADDRESS, LENOFNASPORT,    //Attr 0-5
    LENOFSERVICETYPE,   LENOFFRAMEDPROTOCOL, LENOFFRAMEDIPADDRESS,  //Attr 6-8
    LENOFFRAMEDIPNETMASK,   LENOFFRAMEDROUTING, 0, LENOFFRAMEDMTU,  //Attr 9-12
    LENOFFRAMEDCOMPRESSION, LENOFLOGINIPHOST,   LENOFLOGINSERVICE,  //Attr 13-15
    LENOFLOGINTCPPORT, 0,   0, 0, 0, 0, 0, LENOFFRAMEDIPXNETWORK,   //Attr 16-23
    0, 0, 0, LENOFSESSIONTIMEOUT, LENOFIDLETIMEOUT,                 //Attr 24-28
    LENOFTERMINATIONACTION, 0, 0, 0, 0, 0, 0, LENOFLOGINLATGROUP,   //Attr 29-36
    LENOFFRAMEDAPPLETALKLINK, LENOFFRAMEDAPPLETALKNETWORK, 0,       //Attr 37-39
    LENOFACCSTATUSTYPE, LENOFACCDELAYTIME, LENOFACCINPUTOCTETS,     //Attr 40-42
    LENOFACCOUTPUTOCTETS, 0, LENOFACCAUTHENTIC,                     //Attr 43-45
    LENOFACCSESSIONTIME, LENOFACCINPUTPACKETS,                      //Attr 46-47
    LENOFACCOUTPUTPACKETS, LENOFACCTERMINATECAUSE,                  //Attr 48-49
    0, LENOFACCLINKCOUNT, LENOFACCTINPUTGIGAWORDS,                  //Attr 50-52
    LENOFACCTOUTPUTGIGAWORDS, 0, LENOFEVENTTIMESTAMP, 0, 0, 0, 0,   //Attr 53-59
    0, LENOFNASPORTTYPE, LENOFPORTLIMIT, 0, LENOFTUNNELTYPE,        //Attr 60-64
    LENOFTUNNELMEDIUMTYPE, 0, 0, 0, 0, LENOFARAPPASSWORD,           //Attr 65-70
    LENOFARAPFEATURES, LENOFARAPZONEACCESS, LENOFARAPSECURITY, 0,   //Attr 71-74
    LENOFPASSWORDRETRY, LENOFPROMPT, 0, 0, 0,                       //Attr 75-79
    LENOFMESSAGEAUTHENTICATOR, 0, 0, LENOFTUNNELPERFERENCE,         //Attr 80-83
    LENOFARAPCHALLENGERESPONSE, LENOFACCINTERIMINTERVAL,            //Attr 84-85
    LENOFACCTUNNELPACKETSLOST, 0, 0, 0, 0, 0};                      //Attr 86-91

//Release the momory of the User Request Table.
void ReleaseMemory (UINT8 ReqUserTableId)
{
    KNL_MEM_DEALLOC(gUserReqTable[ReqUserTableId]->pPacketTransmitted);
    KNL_MEM_DEALLOC(gUserReqTable[ReqUserTableId]);
    gUserReqTable[ReqUserTableId] = 0;  //Assign to "0", then it can be used next time.
    if (gNumOfUserEntry) gNumOfUserEntry--;
}

static UINT8 Concatenated[LENOFRXPKT + LENOFSECRET];

int ExecuteAction (UINT8 *pRadRecvPkt, UINT8 SrvIndex)
{

    UINT8   PktIdentifier, ReqUserTableId, AttrType, AttrLen;
    UINT16  PktLenTmp, AttrOffset;
    MD5_CTX Context;

    PktIdentifier = *(pRadRecvPkt + PKTIDENTIFIER);

    //Check receive packet's identifier is in the user request table or not.
    for(ReqUserTableId = 0; ReqUserTableId < MAXUSERREQUEST; ReqUserTableId++)
        if ((gUserReqTable[ReqUserTableId] != 0) && 
            (gUserReqTable[ReqUserTableId]->PacketIdentifier == PktIdentifier))
            break;

    if (ReqUserTableId == MAXUSERREQUEST){
        return FAILED;
    }

    //Check receive packet's packet type is correct or not.
    if (*pRadRecvPkt != ACCESSACCEPT && *pRadRecvPkt != ACCESSREJECT &&
        *pRadRecvPkt != ACCESSCHALLENGE && *pRadRecvPkt != ACCOUNTINGRESPONSE){
        return FAILED;
    }

    //Check receive packet's length is correct or not.
    memcpy(&PktLenTmp, pRadRecvPkt + PKTLENGTH, 2);
    PktLenTmp = ntohs(PktLenTmp);
    
    if ((PktLenTmp > LENOFRXPKT) || (PktLenTmp < LENOFMINPKT)){
        return FAILED;
    }

    //Check the Authenticator of the response packet.
    bzero(&Concatenated, sizeof(Concatenated));
    memcpy(Concatenated, pRadRecvPkt, PktLenTmp);
    memcpy(Concatenated + PKTAUTHENTICATOR, gUserReqTable[ReqUserTableId]->RequestAuth, 
            LENOFAUTHENTICATOR);
//  memcpy(Concatenated + PktLenTmp, gUserReqTable[ReqUserTableId]->pRadiusServer->Secret, LENOFSECRET);
//  PktLenTmp = PktLenTmp + strlen((char *)(gUserReqTable[ReqUserTableId]->pRadiusServer->Secret));
//  memcpy(Concatenated + PktLenTmp, gSrvTable[gUserReqTable[ReqUserTableId]->SrvIndex]->Secret, LENOFSECRET);
    memcpy(Concatenated + PktLenTmp, gSrvTable[SrvIndex]->Secret, LENOFSECRET);
//  PktLenTmp = PktLenTmp + strlen((char *)(gSrvTable[gUserReqTable[ReqUserTableId]->SrvIndex]->Secret));
    PktLenTmp = PktLenTmp + strlen((char *)(gSrvTable[SrvIndex]->Secret));
    MD5Init(&Context);
    MD5Update(&Context, Concatenated, (unsigned int)PktLenTmp);
    MD5Final(&Context);

    //Compare the response Authenticator with the Calculate Authenticator.
    if (memcmp(pRadRecvPkt + PKTAUTHENTICATOR, Context.digest, LENOFAUTHENTICATOR) == 0)
    {
        DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: Authenticator is CORRECT",__FUNCTION__);
    }
    else
    {
        DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: Authenticator is ERROR",__FUNCTION__);
        return FAILED;
    }
//  PktLenTmp = PktLenTmp - strlen((char *)(gSrvTable[gUserReqTable[ReqUserTableId]->SrvIndex]->Secret));
    PktLenTmp = PktLenTmp - strlen((char *)(gSrvTable[SrvIndex]->Secret));

    AttrOffset = PKTATTRIBUTE;
    if (PktLenTmp > AttrOffset)
    {
        for (;;)
        {
            AttrType = *(pRadRecvPkt + AttrOffset);
            //DEBUG_8202(_bDisp802_1x_, ("[ RAD ] %s: AttrType 0x%02x", __FUNCTION__,AttrType));
            if (AttrType > ATUNNELSERVERAUTHID || AttrType < AUSERNAME){
                return FAILED;
            }
            //Fixed length.
            if ((AttrLen = AttrLenTable[AttrType]))
            {
                if (AttrLen != *(pRadRecvPkt + AttrOffset + 1)){
                    return FAILED;
                }
                AttrOffset += AttrLen;
            }
            //Unfixed length.
            else
            {
                switch(AttrType)
                {
                    case AFILTERID:             //Attr 17.
                    case AREPLYMESSAGE:         //Attr 18.
                    case ACALLBACKNUMBER:       //Attr 19.
                    case ACALLBACKID:           //Attr 20.
                    case AFRAMEDROUTE:          //Attr 22.
                    case ASTATE:                //Attr 24.
                    case ACLASS:                //Attr 25.
                    case APROXYSTATE:           //Attr 33.
                    case ALOGINLATSERVICE:      //Attr 34.
                    case ALOGINLATNODE:         //Attr 35.
                    case AFRAMEDAPPLETALKZONE:  //Attr 39.
                    case ALOGINLATPORT:         //Attr 63.
                    case ATUNNELCLIENTENDPOINT: //Attr 66.
                    case ATUNNELSERVERENDPOINT: //Attr 67.
                    case AARAPSECURITYDATA:     //Attr 74.
                    case ACONFIGURATIONTOKEN:   //Attr 78.
                    case AEAPMESSAGE:           //Attr 79.
                    case ATUNNELPRIVATEGROUPID: //Attr 81.
                    case ATUNNELASSIGNMENTID:   //Attr 82.
                    case AFRAMEDPOOL:           //Attr 88.
                    case ATUNNELCLIENTAUTHID:   //Attr 90.
                    case ATUNNELSERVERAUTHID:   //Attr 91.
                        if (*(pRadRecvPkt + AttrOffset + 1) < 3){
                            return FAILED;  //Length error.
                        }
                        break;

                    case ATUNNELPASSWORD:       //Attr 69.
                        if (*(pRadRecvPkt + AttrOffset + 1) < 5){
                            return FAILED;  //Length error.
                        }
                        break;

                    case AVENDORSPECIFIC:       //Attr 26.
                        if (*(pRadRecvPkt + AttrOffset + 1) < 7){
                            return FAILED;
                        }
                        break;

                    default:
                        break;
                }                    
                AttrOffset += *(pRadRecvPkt + AttrOffset + 1);
            }
            if (AttrOffset >= PktLenTmp)
                break;
        }
    }

    //Return the receive packet data to the user.
    if ((*pRadRecvPkt != ACCOUNTINGRESPONSE) &&
        (gUserReqTable[ReqUserTableId]->CallBackfPtr != NULL))
    {
        //(*(gUserReqTable[ReqUserTableId]->CallBackfPtr))(gUserReqTable[ReqUserTableId]->portControl, (UINT8 *) pRadRecvPkt);
        gUserReqTable[ReqUserTableId]->svrIdx = SrvIndex;
        (*(gUserReqTable[ReqUserTableId]->CallBackfPtr))(ReqUserTableId, pRadRecvPkt);
    }
        
    DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: SendTime: %d, ReqUserTableId: %d",__FUNCTION__, 
        gUserReqTable[ReqUserTableId]->SendTime, ReqUserTableId);
      
    ReleaseMemory(ReqUserTableId);
    return SUCCESSFUL;

}

static UINT8 gRadRecvBuf[LENOFRXPKT];



void CheckPacketForAuth (void)
{
    int                 i, SrvAddrLen, ByteCount32 =0;
    UINT8               SrvIndex;
    UINT32              SrcSrvAddr;
    struct sockaddr_in  RadSrvAddr; 
#ifdef WLAN_HOSTOS_LINUX
    struct sock *sk;
#elif defined(WLAN_HOSTOS_VXWORKS)
    fd_set              Recvfds;
    struct timeval      TimeVal;
#endif //WLAN_HOSTOS_LINUX

    if (gNumOfRadSrv == 0)
        return;

#ifdef WLAN_HOSTOS_LINUX
    sk = gRadAuthSocketId->sk;
    if (!skb_queue_empty(&(sk->receive_queue))) /* Do we have data ? */
#elif defined(WLAN_HOSTOS_VXWORKS)
    FD_ZERO(&Recvfds);
    FD_SET(gRadAuthSocketId, &Recvfds);
    TimeVal.tv_sec = 0;
    TimeVal.tv_usec = 0;
    if (select(gRadAuthSocketId+1, &Recvfds, (fd_set *)NULL, (fd_set *)NULL, &TimeVal))
#endif //WLAN_HOSTOS_LINUX
    {
            bzero((char *) &RadSrvAddr, sizeof(RadSrvAddr));
            SrvAddrLen = sizeof(RadSrvAddr);
            ByteCount32 = recvfrom_async(gRadAuthSocketId, gRadRecvBuf, LENOFRXPKT, 0, (struct sockaddr *)&RadSrvAddr, (int *)&SrvAddrLen);
    }            

    
            if (ByteCount32)
            {
                gRadRecvBuf[ByteCount32] = 0;
                if (WDEBUG_LEVEL( _bDisp802_1x_) )
                {
                    printf("\n[ RAD ] %s: got %d bytes from %s, dump hex:",
                        __FUNCTION__,ByteCount32,inet_ntoa(RadSrvAddr.sin_addr));
                
                    _1xDumpHex(gRadRecvBuf, 40); //ByteCount32);
                }
                
                SrcSrvAddr = ntohl(RadSrvAddr.sin_addr.s_addr);
                SrvIndex = 0;
                for (i = 0; i < gNumOfRadSrv; i++)
                {
                    if ((gSrvTable[i] != 0) &&
                        (gSrvTable[i]->ServerIPAddress == SrcSrvAddr))
                    {
                        SrvIndex = i;
                        break;
                    }
                }
                //If the server is valid, then take action.
                if (SrvIndex < gNumOfRadSrv)
                {
                    if (ExecuteAction(gRadRecvBuf, SrvIndex) == SUCCESSFUL)
                    {
                        DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: ExecuteAction() SUCCESSFUL",__FUNCTION__);
                    }
                    else
                    {
                        DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: ExecuteAction() FAILED",__FUNCTION__);
                        PRINTERRM("no ReleaseMemory: potential memory leak");
                    }
                    //this guy which response RADIUS packet is not first one
                    if ((gNumOfRadSrv > 1) && (SrvIndex > 0))
                        AdjustRadiusServer(gNumOfRadSrv, SrvIndex);
                }
                else
                    DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: Rsp RADIUS server not in gSrvTable[]",__FUNCTION__);
            }


}



