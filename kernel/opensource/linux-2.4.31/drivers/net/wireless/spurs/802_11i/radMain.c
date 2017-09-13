#ifdef WLAN_HOSTOS_LINUX

#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/signal.h>
#include <asm/errno.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <asm/byteorder.h>

#elif defined(WLAN_HOSTOS_VXWORKS)
#include <netinet/in.h>
#endif //WLAN_HOSTOS_LINUX

#define INCLUDE_OS_DEP
#include "wlan_include.h"

#include "ifx_wdrv_types.h"
#include "ifx_wdrv_debug.h"
#include "ifxadm_wdrv_init.h"
#include "if_wlan.h"

#include "radMain.h"
#include "radAuth.h"
#include "radMD5.h"
#include "radDef.h"
#include "radRecv.h"

#ifdef WLAN_HOSTOS_LINUX
struct socket *gRadAuthSocketId = NULL;
struct socket *gRadAccSocketId = NULL;
#elif defined(WLAN_HOSTOS_VXWORKS)
int gRadAuthSocketId = 0;
int gRadAccSocketId = 0;
#endif //WLAN_HOSTOS_LINUX
    
UINT8               gRadPktIdentifier, gNumOfUserEntry, gNumOfRadSrv;
RAD_SRV_TABLE_INFO  *gSrvTable[NUMOFSERVER];
USER_REQUEST_ENTRY  *gUserReqTable[MAXUSERREQUEST];


//Set Authentication or Accounting Server.
void SetRadSrv (RAD_SRV_INFO *RadSrv)
{
    RAD_SRV_INFO *RadSrvTmp;
    int i = 0;
    
    RadSrvTmp = RadSrv;
    while (RadSrvTmp != NULL)
    {
        memcpy(gSrvTable[i], RadSrvTmp, sizeof(RAD_SRV_TABLE_INFO));
        i++;
        RadSrvTmp = RadSrvTmp->NextRadSrv;
    }
    
    gNumOfRadSrv = i;
    
    while (i < NUMOFSERVER) //Radius Server less then NUMOFSERVER, clean the Radius Server Table.
    {
        memset(gSrvTable[i], 0, sizeof(RAD_SRV_TABLE_INFO));
        i++;        
    }
}


//Initializing the Authentication and Accounting socket.
//The SocketId must be open when the TASK is starting.
int GetSocketId(void)
{
#ifdef WLAN_HOSTOS_LINUX

    struct socket *sock;
    struct sockaddr_in sin;
    int error;
    UINT16 Port = RADNASPORT ; //RADIUSAUTHPORT;

    removeRadiusSocketId();

    /* First create a socket */
    error = sock_create(PF_INET,SOCK_DGRAM,IPPROTO_UDP,&sock);
    if (error<0) {
        (void)printk(KERN_ERR "1 GetSocketId: Error during creation of socket; terminating\n");
        goto retErr;
    }
    gRadAuthSocketId = sock;


    sin.sin_family	     = AF_INET;
    sin.sin_addr.s_addr  = INADDR_ANY;
    sin.sin_port         = htons((unsigned short)Port);

    error = sock->ops->bind(sock,(struct sockaddr*)&sin,sizeof(sin));
    if (error<0)
    {
        (void)printk(KERN_ERR "radAuth: Error binding socket. This means that some other \n");
        (void)printk(KERN_ERR "        daemon is (or was a short time ago) using port %i.\n",Port);
        return 0;	
    }

    /* Grrr... setsockopt() does this. */
    sock->sk->reuse   = 1;

    error = sock_create(PF_INET,SOCK_DGRAM,IPPROTO_UDP,&sock);
    if (error<0) {
        (void)printk(KERN_ERR "2 GetSocketId: Error during creation of socket; terminating\n");
        goto retErr;
    }
    gRadAccSocketId = sock;

    
#elif defined(WLAN_HOSTOS_VXWORKS)

    removeRadiusSocketId();

    if ((gRadAuthSocketId = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        printf("1 GetSocketId: Error during creation of socket; terminating\n");
        goto retErr;
    }
    if ((gRadAccSocketId = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        printf("2 GetSocketId: Error during creation of socket; terminating\n");
        goto retErr;
    }
#endif //WLAN_HOSTOS_LINUX

    WDEBUG("gRadAuthSocketId:0x%x\n", (unsigned int) gRadAuthSocketId);
    WDEBUG("gRadAccSocketId:0x%x\n", (unsigned int) gRadAccSocketId);

    return OK;

retErr:
    removeRadiusSocketId();
    return ERROR;
}

void removeRadiusSocketId(void)
{
#ifdef WLAN_HOSTOS_LINUX
    struct socket *sock;

    if (gRadAuthSocketId){
        sock=gRadAuthSocketId;
        gRadAuthSocketId = NULL;
        sock_release(sock);
    }
    if (gRadAccSocketId){
        sock=gRadAccSocketId;
        gRadAccSocketId = NULL;
        sock_release(sock);
    }
#elif defined(WLAN_HOSTOS_VXWORKS)
    if (gRadAuthSocketId){
        close(gRadAuthSocketId);
        gRadAuthSocketId = 0;
    }
    if (gRadAccSocketId){
        close(gRadAccSocketId);
        gRadAccSocketId = 0;
    }
#endif //WLAN_HOSTOS_LINUX

}


int sendto_async(
#ifdef WLAN_HOSTOS_LINUX
    struct socket *sock, 
#elif defined(WLAN_HOSTOS_VXWORKS)
    int sock,
#endif //WLAN_HOSTOS_LINUX
    const char *Buffer,const size_t Length, UINT32 flags, struct  sockaddr *addr, int addr_len)
{
#ifdef WLAN_HOSTOS_LINUX
	struct msghdr	msg;
	mm_segment_t	oldfs;
	struct iovec	iov;
	int 		len;

    if(addr){
		msg.msg_name=addr;
		msg.msg_namelen=addr_len;
	} else{
        	msg.msg_name     = NULL;
        	msg.msg_namelen  = 0;
       }
	msg.msg_iov	 = &iov;
	msg.msg_iovlen   = 1;
	msg.msg_control  = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags    = MSG_DONTWAIT|MSG_NOSIGNAL;    
	msg.msg_iov->iov_base = (char*) Buffer;
	msg.msg_iov->iov_len  = (__kernel_size_t)Length;
	

	if (sock->sk)
	{
		oldfs = get_fs(); set_fs(KERNEL_DS);
		len = sock_sendmsg(sock,&msg,(size_t)(Length));
		set_fs(oldfs);
	} else
	{
		return -ECONNRESET;
	}
	
	return len;	
#elif defined(WLAN_HOSTOS_VXWORKS)
    return sendto(sock, Buffer, Length, flags, addr, addr_len);
#endif //WLAN_HOSTOS_LINUX
}

int recvfrom_async(
#ifdef WLAN_HOSTOS_LINUX
    struct socket *sock, 
#elif defined(WLAN_HOSTOS_VXWORKS)
    int sock,
#endif //WLAN_HOSTOS_LINUX
    void * ubuf, size_t size, unsigned flags, struct sockaddr *addr, int *addr_len)
{
#ifdef WLAN_HOSTOS_LINUX

	struct iovec iov;
	struct msghdr msg;
	int len=0;
	mm_segment_t		oldfs;

	msg.msg_control=NULL;
	msg.msg_controllen=0;
	msg.msg_iovlen=1;
	msg.msg_iov=&iov;
	iov.iov_len=size;
	iov.iov_base=ubuf;
	msg.msg_name=addr;
	msg.msg_namelen=*addr_len;
//	if (sock->file->f_flags & O_NONBLOCK)
		flags |= MSG_DONTWAIT;

	oldfs = get_fs(); set_fs(KERNEL_DS);
	len = sock_recvmsg(sock, &msg, size, flags);
	set_fs(oldfs);

	return len;
#elif defined(WLAN_HOSTOS_VXWORKS)
    return recvfrom(sock, ubuf, size, flags, addr, addr_len);
#endif //WLAN_HOSTOS_LINUX
}



//The function CHECK the User Request Table.
//If one packet stay in user request table is time out, then re-send it to Radius Server.
//If the re-send times is over the defined times, ignore it and release the enty memory.
int CheckTimeOutEntry(void)
{

    INT16   i, j, SrvIndex = 0;
    static  UINT8 RadPkt[LENOFTXPKT];
    UINT8   RadHiddenPwd[LENOFPWD];
    UINT8   Digest[16], Secret[LENOFSECRET], PwdPosition = 0;
    UINT16  EntryCount, PktLen, ByteCount16 = 0;
    UINT32  TNow;
    struct  sockaddr_in RadSrvAddr;

    TNow = KNL_SECONDS();
    for(EntryCount = 0; EntryCount < MAXUSERREQUEST; EntryCount++)
    {
        if (gUserReqTable[EntryCount] != 0)
        {
            if ((TNow - gUserReqTable[EntryCount]->SendTime) >= MAXWAITSECOND)
            {
                //If a packet had transmitted "MAXRETXCOUNT" times, change to the next Radius Server.
                //If the next Radius Server is NULL, release the memory of the user request table,
                //else set "gUserReqTable[EntryCount]->TxCount = MAXRETXCOUNT".
                if ((gUserReqTable[EntryCount]->TxCount) == 1)
                {
                    for (SrvIndex = 0; SrvIndex < NUMOFSERVER; SrvIndex++)
                    {
                        if (gUserReqTable[EntryCount]->SrvIP[SrvIndex] != 0)
                        {
                            gUserReqTable[EntryCount]->SrvIP[SrvIndex] = 0;
                            break;
                        }
                    }
                    SrvIndex++;
                    if ((SrvIndex >= NUMOFSERVER) || (gUserReqTable[EntryCount]->SrvIP[SrvIndex] == 0))
                    {
                        //Reaching the max Re-Send times for each Radius Server. So, release memory.
                        KNL_MEM_DEALLOC(gUserReqTable[EntryCount]->pPacketTransmitted);
                        KNL_MEM_DEALLOC(gUserReqTable[EntryCount]);
                        DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: Free Timeout Entry %d, SrvIndex %d (%x)\n",
                                __FUNCTION__,EntryCount,SrvIndex,(unsigned int)TNow);
                        gUserReqTable[EntryCount] = 0;
                        if (gNumOfUserEntry) gNumOfUserEntry--;
                        continue;
                    }
                }

                //Get the correct Server Index.
                for (SrvIndex = 0; SrvIndex < NUMOFSERVER; SrvIndex++)
                {
                    if (gUserReqTable[EntryCount]->SrvIP[SrvIndex] != 0)
                        break;
                }

                //Re-Send packet.
                //Check if the Radius Server IP is in Server Table or NOT.
                for (i = 0; i < gNumOfRadSrv; i++)
                {
                    DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: ReqTab_SrvIP %x, SrvTab_SrvIP %x", 
                            __FUNCTION__,(unsigned int)  gUserReqTable[EntryCount]->SrvIP[SrvIndex],
                            (unsigned int) gSrvTable[i]->ServerIPAddress);
                    
                    if (gUserReqTable[EntryCount]->SrvIP[SrvIndex] == gSrvTable[i]->ServerIPAddress)
                        break;
                }
                if (i < gNumOfRadSrv)
                {
                    PktLen = gUserReqTable[EntryCount]->pPacketTransmitted[2];
                    PktLen = (PktLen << 8) + gUserReqTable[EntryCount]->pPacketTransmitted[3];

                    //If the Radius Server was changed, the password string and EAP Authenticator must be changed.
                    if (gUserReqTable[EntryCount]->TxCount == 1)
                    {
                        gUserReqTable[EntryCount]->TxCount = MAXRETXCOUNT + 1;  //Because after resend, it will mirus 1.
                        //If the User Password != '\0', hidden the Password. (Because the secret is changed with the SERVER IP.)
                        if (gUserReqTable[EntryCount]->UserPasswd[0] != 0)
                        {
                            HidePwdOfAuth(gUserReqTable[EntryCount]->UserPasswd, 
                                    gUserReqTable[EntryCount]->RequestAuth,
                                    RadHiddenPwd, &ByteCount16, gSrvTable[i]->Secret);
                            PwdPosition = PKTATTRIBUTE + 
                                    gUserReqTable[EntryCount]->pPacketTransmitted[21] + 2;
                            memcpy(gUserReqTable[EntryCount]->pPacketTransmitted + PwdPosition, 
                                    RadHiddenPwd, ByteCount16);
                            memcpy(RadPkt, gUserReqTable[EntryCount]->pPacketTransmitted, PktLen);
                        }

                        //EAP Packet. Must make a new EAP Authenticator. (Because the secret is changed with the SERVER IP.)
                        if (gUserReqTable[EntryCount]->portControl != NULL)
                        {
                            for (j = 0; j < 16; j++)
                            {
                                gUserReqTable[EntryCount]->pPacketTransmitted[PktLen - 16 + j] = 0;
                            }
                            memset(Secret, 0, LENOFSECRET);
                            
                            ByteCount16 = strlen((char *)gSrvTable[i]->Secret);
                            memcpy(Secret, gSrvTable[i]->Secret, ByteCount16);
                            HMAC_MD5(gUserReqTable[EntryCount]->pPacketTransmitted, PktLen, 
                                     Secret, ByteCount16, Digest);
                            memcpy(gUserReqTable[EntryCount]->pPacketTransmitted + PktLen - 16, 
                                    Digest, 16);
                        }
                    }

                    memcpy(RadPkt, gUserReqTable[EntryCount]->pPacketTransmitted, PktLen);
                    bzero((void *)&RadSrvAddr, sizeof(RadSrvAddr));
                    RadSrvAddr.sin_family = AF_INET;

                    RadSrvAddr.sin_addr.s_addr = htonl(gUserReqTable[EntryCount]->SrvIP[SrvIndex]);


                    if (gUserReqTable[EntryCount]->pPacketTransmitted[0] == ACCESSREQUEST)
                    {
                        RadSrvAddr.sin_port = htons(RADIUSAUTHPORT);  //1812.
                        if (sendto_async(gRadAuthSocketId, RadPkt, PktLen, 0, 
                                (struct sockaddr *)&RadSrvAddr, sizeof(RadSrvAddr)) == -1)
                        {
                            DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: Re-Send Authenticator packet FAILED.\n",__FUNCTION__);
                        }
                        else
                        {
                            gUserReqTable[EntryCount]->TxCount--;
                            gUserReqTable[EntryCount]->SendTime = TNow;
                            DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: Re-Send Authenticator packet SUCCESS (%x).\n",__FUNCTION__,(unsigned int)TNow);
                        }                           
                    }
                    if (gUserReqTable[EntryCount]->pPacketTransmitted[0] == ACCOUNTINGREQUEST)
                    {
                        PRINTERRM("acc not supported");
                        RadSrvAddr.sin_port = htons(RADIUSACCPORT);   //1813.
                        if (sendto_async(gRadAccSocketId, RadPkt, PktLen, 0, 
                                (struct sockaddr *)&RadSrvAddr, sizeof(RadSrvAddr)) == -1)
                        {
                            DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: Re-Send Accounting packet FAILED.\n",__FUNCTION__);
                        }
                        else
                        {
                            gUserReqTable[EntryCount]->TxCount--;
                            gUserReqTable[EntryCount]->SendTime = TNow;
                            DEBUG_8202(_bDisp802_1x_, "[ RAD ] %s: Re-Send Accounting packet SUCCESS.\n",__FUNCTION__);
                        }
                    }
                }
                else
                {
                    gUserReqTable[EntryCount]->TxCount = 1;
                    DEBUG_8202(_bDispErrors_, "Err %s: Invalid Radius Server IP.\n",__FUNCTION__);
                }
            }
        }
    }
    return OK;
}



int RadCliInit(void)
{
    int i;

    gNumOfRadSrv = 0;
    gNumOfUserEntry = 0;
    gRadPktIdentifier = 0;

    //The memory will be allocate when user call RadAuthentication() & RadAccounting().
    for (i = 0; i < MAXUSERREQUEST; i++)
        gUserReqTable[i] = 0;

    for (i = 0; i < NUMOFSERVER; i++){
        gSrvTable[i] = (RAD_SRV_TABLE_INFO *)KNL_KNL_MALLOC(sizeof(RAD_SRV_TABLE_INFO));        
        if (!gSrvTable[i]) {
            PRINTERRM("no mem to alloc");
            goto RadCliInitRetErr;
        }
    }
    
    if (GetSocketId() == OK){
        PRINTLOCM("RADCLIINIT success");
    }else{
        PRINTLOCM("RADCLIINIT FAIL");
        goto RadCliInitRetErr;
    }

    return OK;

RadCliInitRetErr:
    RadCli_UnInit();
    return NOK;
}

void RadCli_UnInit(void)
{
    int i;
    int EntryCount;
    
//    removeRadiusSocketId();

    for(EntryCount = 0; EntryCount < MAXUSERREQUEST; EntryCount++){
        if (gUserReqTable[EntryCount] != 0){
            KNL_MEM_DEALLOC(gUserReqTable[EntryCount]->pPacketTransmitted);
            KNL_MEM_DEALLOC(gUserReqTable[EntryCount]);
            gUserReqTable[EntryCount] = 0;
            if (gNumOfUserEntry) gNumOfUserEntry--;
        }
    }

    for (i = 0; i < NUMOFSERVER; i++){
        if (gSrvTable[i]) {
            os_freeMemory(gSrvTable[i]);
            gSrvTable[i] = NULL;
        }

    }
}



