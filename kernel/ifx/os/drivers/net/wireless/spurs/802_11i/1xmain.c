/*****************************************************************************/
/* The information contained in this file is confidential and proprietary to */
/* ADMtek Incorporated. No part of this file may be reproduced or distributed*/
/* , in any form or by any means for any purpose, without the express written*/
/* permission of ADMtek Incorporated.                                        */
/*                                                                           */
/*(c) COPYRIGHT 2001,2002 ADMtek Incorporated, ALL RIGHTS RESERVED.          */
/*    http://www.admtek.com.tw                                               */
/*                                                                           */
/*****************************************************************************/
/*  File Name: 1xmain.c                                                      */
/*  File Description: 802.1x task main program                               */
/*                                                                           */
/*  History:                                                                 */
/*      Date        Author      Version     Description                      */
/*      ----------  ---------   -------     --------------                   */
/*      02/21/2002  Joy Lin     V1.0        Initial Version                  */
/*                                                                           */
/*****************************************************************************/
#ifdef WLAN_HOSTOS_LINUX
#include <linux/interrupt.h>
#include <linux/ctype.h>
#include <asm/signal.h>
#include <asm/errno.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/net.h>
#include <net/sock.h>
#include <asm/byteorder.h>
#include <linux/types.h>
#include <linux/etherdevice.h>

#elif defined(WLAN_HOSTOS_VXWORKS)
#include <netinet/if_ether.h>
#include <taskLib.h>
#include <eventLib.h>
#endif //WLAN_HOSTOS_LINUX

#define INCLUDE_OS_DEP
#include "wlan_include.h"

#include "ifxadm_wdrv_mgmt.h"
#include "ifx_wdrv_csr_access.h"
#include "radDef.h"
#include "1xdefs.h"
#include "1xtdfs.h"
#include "1xvars.h"
#include "1xproto.h"
#include "radDS.h"
#include "sibapi.h"
#include "wpa_cfg.h"
#include "if_wlan.h"
#include "radRecv.h"
#include "ifx_wdrv_debug.h"

//*************************
// Global Variables
//*************************
RAD_SRV_INFO  RadiusServer[NUMOFSERVER];
extern struct wlan_buf_listHead eapolintrq;
extern UINT8 ANonce[40];

extern WLAN_CONTENT_ID WLANDrvObjPtr;

static struct tasklet_struct dot1xTask_toWlan_tasklet;
static struct wlan_buf_listHead dot1xTask_toWlan_q  = { LIST_HEAD_INIT(dot1xTask_toWlan_q.lh), SPIN_LOCK_UNLOCKED };

//*************************
// Static Variables
//*************************
#ifdef WLAN_HOSTOS_LINUX
extern struct socket *gRadAuthSocketId;
pid_t dot1x_thread_pid=0;
#elif defined(WLAN_HOSTOS_VXWORKS)
#define _1xTask_KILL BIT_0
#define _1xTask_EV1 BIT_1
static int _1xTaskId = 0;
#endif //WLAN_HOSTOS_LINUX
static int dot1xTask(void *cpu_pointer);

struct dot1x_port_del{
    struct list_head lh;
    UINT8 mac[6];
};
static struct list_head dot1x_del_lh;
static spinlock_t dot1x_del_spl;

static HashTableType    _1xHashTable;
static UINT8 GNoStations;
static int _1xInit (void);
static void DelPortControl_task(void);

void incPortCount(void)
{
    GNoStations++;
}

void decPortCount(void)
{
    GNoStations--;
}

int servedPortCount(void)
{    
    return GNoStations;
}

void _1xStop(void)
{
#ifdef WLAN_HOSTOS_LINUX
    if(dot1x_thread_pid){
        if (kill_proc (dot1x_thread_pid, SIGKILL, 1)) {
            WDEBUG_DESC(WDL_WPA, "unable to signal terminate dot1xTask kernel thread\n");
            return;
        } else{
            WDEBUG_DESC(WDL_WPA, "terminating dot1xTask kernel thread successfully\n");
            dot1x_thread_pid = 0;
        }
    }
#elif defined(WLAN_HOSTOS_VXWORKS)
    if(_1xTaskId){
        if (eventSend(_1xTaskId, _1xTask_KILL) ){
            WDEBUG_DESC(WDL_WPA, "_1xStop: event set ERROR");
        }
    }
#endif //WLAN_HOSTOS_LINUX

    drainEapolintrQ();
    PortControlReInit();  //to free all portControl db
    DelPortControl_task();

}
/*-----------------------------------------------------------------------------
* ROUTINE NAME - _1xStart
*------------------------------------------------------------------------------
* FUNCTION: This function create the 802.1x daemon task.
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

int _1xStart (void)
{
#ifdef WLAN_HOSTOS_LINUX
    dot1x_thread_pid = kernel_thread(dot1xTask,NULL, CLONE_FS | CLONE_FILES | CLONE_SIGHAND);
    if (dot1x_thread_pid < 0){
        dot1x_thread_pid = 0;
        WDEBUG_DESC(WDL_WPA, "unable to start dot1xTask kernel thread\n");
        return NOK;
    }
#elif defined(WLAN_HOSTOS_VXWORKS)

#define TASK_SSTACK_SIZE    0x4000  // 0x2000, jess.hsieh@IFADM , 12KB stack [2004/11/6] in knl_sup.c

   _1xTaskId = taskSpawn(DOT1X_TASK_NAME, DOT1X_TASK_PRIO, VX_PRIVATE_ENV, TASK_SSTACK_SIZE, (FUNCPTR)dot1xTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (_1xTaskId==ERROR){
        _1xTaskId = 0;
        WDEBUG_DESC(WDL_WPA, "creating dot1x task fail");
        return NOK;
    }
#endif //WLAN_HOSTOS_LINUX

    return OK;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - _1xInit
*------------------------------------------------------------------------------
* FUNCTION: Initialize 802.1x task 
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
void PortControlReInit_(PORT_CONTROL *port)
{
    DelPortControl(port->hashEntry.mac);
}
void PortControlReInit(void)
{
    PRINTLOC;
    iterateAllPortControl((pHANDLE) PortControlReInit_);
}
void PortControlInit(void)
{

    INIT_LIST_HEAD(&dot1x_del_lh);
    spin_lock_init(&dot1x_del_spl);
    GNoStations = 0;
    wlanHashTabInit(&_1xHashTable);
}

void clearPortControl(PORT_CONTROL *port)
{

    if (port->individualKeyIdx != KEY_UNASSIGNED)
    {
        //clear individual key
        SetIndividualKey(port->individualKeyIdx, NULL, NULL, 0, NULL);
    }

//    hashEntry_t    hashEntry;     /* NO need to initialize this */

    port->reAuthWhen = 0;     /* Reauthentication Timer FSM */

    port->portStatus = PORT_UNAUTHORIZED;     /* Unauthorized=0/Authorized=1 */   
    port->apStatus=0;       /* Authenticator PAE FSM */
    port->baStatus=0;       /* Backend Authentication FSM */
    port->reqCount=0;       /* Backend Authentication FSM */
    
    port->currentId = 1;      /* EPAOL's Identifier */
    port->reAuthCount=0;    /* Authenticator PAE FSM */   
    port->txWhen=0;         /* Authenticator PAE FSM */
    port->quietWhile=0;     /* Authenticator PAE FSM */
    port->aWhile=0;         /* Backend Authentication FSM */
    port->individualKeyIdx = KEY_UNASSIGNED;
   
    port->flags=0;

    memset(port->TxRxMICKey, 0, sizeof(port->TxRxMICKey));	    //First 8 bytes is the Tx-key, the next 8 bytes is Rx-Key.
    memset(port->StaSessionKey, 0, sizeof(port->StaSessionKey));
    memset(port->userName, 0, sizeof(port->userName)); //32
    
    memset(port->radRespState, 0, sizeof(port->radRespState)); //38
    port->ReTxCount=0;
#ifdef APPS_INCLUDE_SNMP    
    port->portNumber = servedPortCount();

    port->mibFlags=0;
    port->reAuthPeriod  = DEF_REAUTHPERIOD;
    
    port->quietPeriod   = DEF_QUIETPERIOD;
    port->txPeriod      = DEF_TXPERIOD;
    port->suppTimeout   = DEF_SUPPTIMEOUT;
    port->serverTimeout = DEF_SERVERTIMEOUT;
    
    port->authPortControl = 2; //auto
    port->maxReq        = DEF_MAXREQ;
    port->framesRx=0;    
    
    port->octetsRx=0;
    port->octetsTx=0;
    
    port->framesTx=0;
    port->entersConnecting=0;
    port->entersAuthenticating=0;
    port->authSuccessWhileAuthenticating=0;
    
    port->backendResponses=0;
    port->eapolStartFramesRx=0;
    port->eapolRespIdFramesRx=0;
    port->eapolRespFramesRx=0;


#else
    port->UnUsed=0;
#endif

}
static void dot1x_xmit_toWlan_(void)
{
    while( !listEmptyWlanBufInfo(&dot1xTask_toWlan_q) ) {
        WLAN_BUF_INFO* pBufInfo = deQWlanBufInfo(&dot1xTask_toWlan_q);
        wlandev_xmit_lan2Wlan(detachWlanBufWbuf(pBufInfo));
        freeWlanBufInfo(pBufInfo);
    }

#ifdef WLAN_HOSTOS_VXWORKS
    atomic_set(&dot1xTask_toWlan_tasklet.count, 0);
#endif //WLAN_HOSTOS_VXWORKS
}
void dot1x_xmit_toWlan(WLAN_BUF_INFO* pBufInfo)
{
    enQWlanBufInfo(&dot1xTask_toWlan_q, pBufInfo);
    tasklet_schedule(&dot1xTask_toWlan_tasklet);
}
static int _1xInit (void)
{

    dot1xTask_toWlan_tasklet.next = NULL;
    dot1xTask_toWlan_tasklet.state =  0;
    atomic_set(&dot1xTask_toWlan_tasklet.count,0);
    dot1xTask_toWlan_tasklet.func = dot1x_xmit_toWlan_;
    dot1xTask_toWlan_tasklet.data =  (unsigned long)0;

    PortControlInit();

    Dot1xCfg_ReAuthEnabled(&ReAuthEnabled);
    Dot1xCfg_ReAuthPeriod(&ReAuthPeriod);

    GroupTK1Key   = &GTK[0];
    GroupTxMICKey = &GTK[16];
    memset(MulticastKey, 0x00, 16);
    memset(GTK, 0x00, 40);
    WPA_GenerateNonce(ANonce);

    return OK;
}

void _1xPeriodicTask(void)   //one second
{   
    UINT8 authType;
    
    SIBCfg_AuthenticationAlg(&authType);

    _1xTimeoutHandler();

    if (authType == AUTH_MODE_8021X_EAP){
        CheckTimeOutEntry();
        //CheckPacketForAuth();
    }
}

#ifdef WLAN_HOSTOS_LINUX
static wait_queue_head_t DummyWQ;
#endif //WLAN_HOSTOS_LINUX

static int dot1xTask(void *cpu_pointer)
{
#ifdef WLAN_HOSTOS_LINUX
    DECLARE_WAITQUEUE(main_wait,current);
#elif defined(WLAN_HOSTOS_VXWORKS)
    UINT32 evBuf=0;
#endif //WLAN_HOSTOS_LINUX
    UINT32 timeo;
    UINT8 authType;

#ifdef WLAN_HOSTOS_LINUX
    sprintf(current->comm, DOT1X_TASK_NAME);
    daemonize();

    init_waitqueue_head(&DummyWQ);

    /* Block all signals except SIGTERM, SIGKILL, SIGSTOP and SIGHUP */
    spin_lock_irq(&current->sigmask_lock);
    siginitsetinv(&current->blocked, sigmask(SIGTERM)|sigmask(SIGKILL) | sigmask(SIGSTOP)| sigmask(SIGHUP));
    recalc_sigpending(current);
    spin_unlock_irq(&current->sigmask_lock);
#endif //WLAN_HOSTOS_LINUX

    if (_1xInit() != OK){
        PRINTERR;
        goto dot1xTaskOut;
    }

    SIBCfg_AuthenticationAlg(&authType);
    
    if (authType == AUTH_MODE_8021X_EAP){
        /* Initialize RADIUS Client */
        if (RadCliInit() != OK){
            PRINTERR;
            goto dot1xTaskOut;
        }
        
#ifdef WLAN_HOSTOS_LINUX
    if (gRadAuthSocketId->sk==NULL){
        PRINTERR;
        goto dot1xTaskOut;
    }
    
    add_wait_queue_exclusive(gRadAuthSocketId->sk->sleep,&(main_wait));
#elif defined(WLAN_HOSTOS_VXWORKS)
#endif //WLAN_HOSTOS_LINUX

    }

    timeo = RUN_AT(HZ);
    
    while (1)
    {
        DelPortControl_task();  //this is first priority in the task
        
        eapolintr();

        if (authType == AUTH_MODE_8021X_EAP){
            CheckPacketForAuth();
        }

        if ( time_after_eq(jiffies, timeo) ){

            _1xTimeoutHandler();

            if (authType == AUTH_MODE_8021X_EAP){
                CheckTimeOutEntry();
                //CheckPacketForAuth();
            }

            timeo = RUN_AT(HZ);
        }
#ifdef WLAN_HOSTOS_LINUX
        (void)interruptible_sleep_on_timeout(&DummyWQ,1);	

        if (signal_pending(current)!=0){
            (void)printk(KERN_NOTICE "\ndot1xTask: Ring Ring - signal received\n");
            break;		  
        }
#elif defined(WLAN_HOSTOS_VXWORKS)
        //this will block, max one second, until an event is coming...
        eventReceive((_1xTask_KILL |_1xTask_EV1),(EVENTS_WAIT_ANY | EVENTS_KEEP_UNWANTED),os_sec2Ticks(1),&evBuf);
        if( evBuf & _1xTask_KILL ){
            printf("\ndot1xTask: KILL event received\n");
            break;
        }
#endif //WLAN_HOSTOS_LINUX
	
    }

    if (authType == AUTH_MODE_8021X_EAP){

#ifdef WLAN_HOSTOS_LINUX
        remove_wait_queue(gRadAuthSocketId->sk->sleep,&(main_wait));
#elif defined(WLAN_HOSTOS_VXWORKS)
#endif //WLAN_HOSTOS_LINUX

        removeRadiusSocketId();

    }

dot1xTaskOut:

#ifdef WLAN_HOSTOS_LINUX
    dot1x_thread_pid = 0;
#elif defined(WLAN_HOSTOS_VXWORKS)
    _1xTaskId = 0;
#endif //WLAN_HOSTOS_LINUX
    WDEBUG( "dot1xTask: kDaemon has ended\n");
    return 0;
}


/*--------------------------------------------------------------
* ROUTINE NAME - _1xEnqueue_EapReqID
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    
---------------------------------------------------------------*/
int _1xEnqueue_EapReqID(char *pDa)
{
#define EAPOL_START_PKGLEN      10
    pHANDLE wb;
    UINT8 *buf;
    WLAN_BUF_INFO *m = allocWlanBufInfoWithWbuf(WLANDrvObjPtr, MAC_ADDR_LEN+EAPOL_START_PKGLEN, &wb);

    if (!m){
        PRINTERRM("no mem to alloc");
        return NOK;
    }

    buf = wbuf_getData(wb)+ MAC_ADDR_LEN;    //to simulate incoming ethernet packet
    
    DEBUG_8202(_bDisp802_1x_, "[ 1X ] STA associated, send EAP/Req-ID to it\n");    
    memcpy(buf, pDa, 6);
    buf[6] = 0x88;
    buf[7] = 0x8E;
    buf[8] = EAPOL_PROTOCOLVERSION;
    buf[9] = EAPOL_START;

    wbuf_put(wb, MAC_ADDR_LEN+EAPOL_START_PKGLEN);
    
    enQWlanBufInfo(&eapolintrq, m);

#ifdef WLAN_HOSTOS_VXWORKS
    if (!_1xTaskId ||eventSend(_1xTaskId, _1xTask_EV1) ){
        WDEBUG_DESC(WDL_WPA, "_1xEnqueue_EapReqID: _1xTaskId:0x%x, eventSend return ERROR\n", _1xTaskId);
        drainEapolintrQ();
    }
#endif //WLAN_HOSTOS_VXWORKS
    
    return OK;
    
#undef EAPOL_START_PKGLEN    

}

/*--------------------------------------------------------------
* ROUTINE NAME - _1xEnqueue_EapKey
*---------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    
---------------------------------------------------------------*/
int _1xEnqueue_EapKey(char *pDa)
{
    return _1xEnqueue_EapReqID(pDa);
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - ether_input_eapol
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/
/*
void ether_input_eapol(ifp, eh, m)
	struct ifnet *ifp;
	register struct ether_header *eh;
	struct mbuf *m;
*/    
void ether_input_eapol(struct ether_header *eh, WLAN_BUF_INFO *m)
{
    UINT8 authType;

//    DEBUG_8202(_bDisp802_1x_, ("[ 1X ] ether_input_eapol() start"));
    
	if (!isWdrvStarted()) 
	{
		freeWlanBufInfo(m);
		return;
	}

    if ((memcmp(SIBCfg_getWlanMac(), eh->ether_dhost, sizeof(eh->ether_dhost)) != 0)
        || (eh->ether_shost[0] & 1))
    {
        freeWlanBufInfo(m); 	/* free the packet chain */
        return; 
    }
    
    SIBCfg_AuthenticationAlg(&authType);
    
    if ((authType != AUTH_MODE_8021X_EAP) && (authType != AUTH_MODE_8021X_PSK))
    {
        freeWlanBufInfo(m);	/* free the mbuf chain */
        return;
    }

    enQWlanBufInfo(&eapolintrq, m);

#ifdef WLAN_HOSTOS_VXWORKS
    if (!_1xTaskId ||eventSend(_1xTaskId, _1xTask_EV1) ){
        WDEBUG_DESC(WDL_WPA, "ether_input_eapol: _1xTaskId:0x%x, eventSend return ERROR\n", _1xTaskId);
        drainEapolintrQ();
    }
#endif //WLAN_HOSTOS_VXWORKS


}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - PortDbInquiry
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None
* OUTPUT:   None
* RETURN:   NULL if not found
* Called by:
* Calling:  None
* NOTE:
*-----------------------------------------------------------------------------*/

PORT_CONTROL *PortDbInquiry(UINT8 *sta)
{
    return (PORT_CONTROL*)wlanHashTabSearch(&_1xHashTable, sta);
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - AddPortControl
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None
* OUTPUT:   None
* RETURN:   NULL if add failure
* Called by:
* Calling:  None
* NOTE:
*-----------------------------------------------------------------------------*/

PORT_CONTROL *AddPortControl(UINT8 *sta)
{
    PORT_CONTROL *port=NULL;
    
    if (!is_valid_ether_addr(sta)){
        if(WDEBUG_LEVEL(WDL_WPA)){
            PRINTMAC(sta, "AddPortControl: invalid MAC");
        }
        return NULL;
    }

    if(servedPortCount() >= DOT1X_MAX_PORT_NUMBER){
        WDEBUG_DESC(WDL_WPA, "AddPortControl: service port max num reached:%d\n", servedPortCount());
        return NULL;
    }

    port = (PORT_CONTROL*) os_getMemory(sizeof(PORT_CONTROL));
    if(!port){
        WDEBUG_DESC(WDL_WPA, "AddPortControl: no mem to alloc\n");
        return NULL;
    }

    if(wlanHashTabInsert(&_1xHashTable, &port->hashEntry, sta)){
        os_freeMemory(port);
        WDEBUG_DESC(WDL_WPA, "AddPortControl: wlanHashTabInsert return NOK\n");
        return NULL;
    }

    DEBUG_8202(_bDisp802_1x_, "[ 1X ] STA: 0x%02x%02x%02x%02x%02x%02x added to PortControl", 
                                sta[0], sta[1], sta[2], sta[3], sta[4], sta[5]);

    incPortCount();
    if (servedPortCount() == 1)
    {
        UINT32 Value32;

        //Setup Key ID of unicast packet in CSR47.
	    Value32 = WLAN_REG_READ (CSR47);
        Value32 &= 0xff8fffff;              //Clear bit 22,21,20.

        WLAN_REG_WRITE (CSR47, Value32);

        //re-generate GTK
        WPA_CalcGTK();
    }

    port->individualKeyIdx = KEY_UNASSIGNED;  //this is needed to disable checking inside clearPortControl
    clearPortControl(port);

    return port;
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - DelPortControl
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None
* OUTPUT:   None
* RETURN:   None
* Called by:
* Calling:  None
* NOTE:
*-----------------------------------------------------------------------------*/
static void DelPortControl_process(UINT8 *sta)
{
    PORT_CONTROL *port = PortDbInquiry(sta);

    if(!port){
        if(WDEBUG_LEVEL(WDL_WPA) ){
            PRINTMAC(sta, "DelPortControl: sta not found ");
        }
        return;
    }
    
    DEBUG_8202(_bDisp802_1x_, "[ 1X ] STA: 0x%02x%02x%02x%02x%02x%02x deleted from PortControl", 
                                        sta[0], sta[1], sta[2], sta[3], sta[4], sta[5]);

    //clear individual key
    if (port->individualKeyIdx != KEY_UNASSIGNED)
    {
        SetIndividualKey(port->individualKeyIdx, NULL, NULL, 0, NULL);
    }

    //don't initial the part of MIB records
#ifdef APPS_INCLUDE_SNMP
    PRINTTODO;
    while(1);
    //memset(&PortControl[hTable[HKey].dbIdx], 0, 107);
#endif

    decPortCount();        
    wlanHashTabRemove(&_1xHashTable, &port->hashEntry);

    os_freeMemory(port);

}

static void DelPortControl_task(void)
{
    while(!list_empty(&dot1x_del_lh)){
        unsigned long flags;
        struct dot1x_port_del *ptr = (struct dot1x_port_del*)dot1x_del_lh.next;
        spin_lock_irqsave(&dot1x_del_spl, flags);
        list_del(&ptr->lh);
        spin_unlock_irqrestore(&dot1x_del_spl, flags);
        DelPortControl_process(ptr->mac);
        os_freeMemory(ptr);
    }
}
static void DelPortControl_submit(UINT8 *sta)
{
    unsigned long flags;
    struct dot1x_port_del *ptr = (struct dot1x_port_del *)os_getMemory(sizeof(struct dot1x_port_del));
    if(!ptr){
        PRINTERRM("DelPortControl_submit: no mem to alloc");
        return;
    }
    
    COPY_MAC(ptr->mac, sta);
    spin_lock_irqsave(&dot1x_del_spl, flags);
    list_add_tail(&ptr->lh, &dot1x_del_lh);
    spin_unlock_irqrestore(&dot1x_del_spl, flags);
}
void DelPortControl(UINT8 *sta)
{
    DelPortControl_submit(sta);
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - GetDot1xKey
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None
* OUTPUT:   None
* RETURN:   NULL if not found
* Called by: !!! NOT USED
* Calling:  None
* NOTE:
*-----------------------------------------------------------------------------*/

UINT8 *GetDot1xKey(UINT8 *sta)
{
    PORT_CONTROL *p;
    
    if ((p = PortDbInquiry(sta)) == NULL)
        return (NULL);
    else
        return (p->TxRxMICKey);
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - DumpPortControl
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None
* OUTPUT:   None
* RETURN:   
* Called by:
* Calling:  None
* NOTE:
*-----------------------------------------------------------------------------*/
#ifdef IFX_WLAN_DEBUG
void DumpPortControl_(PORT_CONTROL *port)
{
    int j;

    printf("%02x%02x%02x%02x%02x%02x  %02d %02d %02d %02d %02d        %02d     %02d     %02d    %02d     %05d      %04x\n",
        port->hashEntry.mac[0],port->hashEntry.mac[1],port->hashEntry.mac[2],
        port->hashEntry.mac[3],port->hashEntry.mac[4],port->hashEntry.mac[5],
        port->portStatus, port->apStatus, port->baStatus, port->currentId,
        port->reAuthCount, port->reqCount, port->txWhen, port->quietWhile,
        port->aWhile, port->reAuthWhen, port->flags);
    printf("              keyIdx: %d, key:",port->individualKeyIdx);
    for (j=0; j<16; j++)
        printf(" %02x",port->TxRxMICKey[j]);
    printf("\n");
    printf("user-name : %s \n", port->userName);
}
void DumpPortControl(void)
{
    printf("\ndump PORT CONTROL (GNoStations = %d)", servedPortCount());
    printf("\nMAC           PS AS BS ID ReAuthCnt ReqCnt txWhen quiet aWhile reAuthWhen flags");
    printf("\n--------------------------------------------------------------------------------\n");
    iterateAllPortControl((pHANDLE)DumpPortControl_);
}
/*-----------------------------------------------------------------------------
* ROUTINE NAME - DumpClientDb
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT:    None
* OUTPUT:   None
* RETURN:   
* Called by:
* Calling:  None
* NOTE:
*-----------------------------------------------------------------------------*/
#include "client_db.h"
void DumpClientDb(void)
{
    DumpCliDbAll();
}

#define _NUMOFCHAR_PERLINE      20

void _1xDumpHex(UINT8 *buf, int len)
{
    int i;
    
    for (i=0; i < len; i++) {
        if ((i % _NUMOFCHAR_PERLINE) == 0) printf("\n");
            printf(" %02x", buf[i]);
    }
    printf("\n");
}
#endif //IFX_WLAN_DEBUG


#define DigToNum(x) ((x > '9') ? (x-'a'+0xa) : (x-'0'))

// *src will be change to lower case
void TransAsciiToHex(UINT8 *dest, UINT8 *src, int len)
{
    int i;
    
    for (i=0; i< (len*2); i++)
    {     
        src[i] = tolower(src[i]);
    }
    for (i=0; i<len; i++)
    {
        dest[i] = ((DigToNum(src[2*i])) << 4) + (DigToNum(src[2*i+1]));
    }
}


/*-----------------------------------------------------------------------------
* ROUTINE NAME - UpdateRadiusServerInfo
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

void UpdateRadiusServerInfo(RADIUS_CONF *nvRadConf)
{
    RAD_SRV_INFO  *pS;
    int i, j=0;

    for (i=0; i<NUMOFSERVER; i++)
    {
        if ( *((UINT32 *)(nvRadConf[i].ip)) != 0L )
        {
            if (j != 0)
                RadiusServer[j-1].NextRadSrv = &RadiusServer[j];
            
            pS = &RadiusServer[j];
            pS->ServerType = SRVFORAUTHACC;
            memcpy( &(pS->ServerIPAddress), nvRadConf[i].ip, 4);
            pS->ServerIPAddress = htonl(pS->ServerIPAddress);
            memcpy ((char *)pS->Secret, nvRadConf[i].secret, RADIUS_SECRET_LEN);
            pS->Secret[RADIUS_SECRET_LEN] = '\0';
            pS->NextRadSrv = NULL;
            j++;
        }
    }
    if (j == 0) // no RADIUS server set
    {
        RadiusServer[j].ServerIPAddress = 0L;
        RadiusServer[j].NextRadSrv = NULL;
    }
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - AdjustRadiusServer
*------------------------------------------------------------------------------
* FUNCTION: swap RadiusServer[0] & RadiusServer[hasRspRad]
*
* INPUT   : None
* OUTPUT  : None
* RETURN  : None
* NOTE:
*-----------------------------------------------------------------------------*/

void AdjustRadiusServer(int totalNumOfRad, int hasRspRad)
{
    RAD_SRV_INFO Tmp;
    int i;

    memcpy(&Tmp, &RadiusServer[0], sizeof(RAD_SRV_INFO));
    
    memcpy(&RadiusServer[0], &RadiusServer[hasRspRad], sizeof(RAD_SRV_INFO));
    
    memcpy(&RadiusServer[hasRspRad], &Tmp, sizeof(RAD_SRV_INFO));
    
    // link them
    for (i=0; i< (totalNumOfRad-1); i++)
        RadiusServer[i].NextRadSrv = &RadiusServer[i+1];
        
    RadiusServer[i].NextRadSrv = NULL;
}


/*-----------------------------------------------------------------------------
* ROUTINE NAME - Notify1xCfg_ReAuthEnabled
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : 
* OUTPUT  : 
* RETURN  : 
* NOTE:
*-----------------------------------------------------------------------------*/
void Notify1xCfg_ReAuthEnabled_(PORT_CONTROL *port)
{
    if (port->portStatus == PORT_AUTHORIZED)
        port->reAuthWhen = ReAuthPeriod;
}
void Notify1xCfg_ReAuthEnabled(UINT8 newValue)
{
    if ((ReAuthEnabled == REAUTHENABLED_FALSE) && (newValue == REAUTHENABLED_TRUE))
    {
        iterateAllPortControl((pHANDLE)Notify1xCfg_ReAuthEnabled_);
    }
    ReAuthEnabled = newValue;        
}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - Notify1xCfg_ReAuthPeriod
*------------------------------------------------------------------------------
* FUNCTION: 
*
* INPUT   : 
* OUTPUT  : 
* RETURN  : 
* NOTE:
*-----------------------------------------------------------------------------*/
void Notify1xCfg_ReAuthPeriod_(PORT_CONTROL *port)
{
    if (port->reAuthWhen) 
        port->reAuthWhen = ReAuthPeriod;
}
void Notify1xCfg_ReAuthPeriod(UINT16 newValue)
{
    ReAuthPeriod = newValue;

    if (ReAuthEnabled == REAUTHENABLED_TRUE)
    {
        iterateAllPortControl((pHANDLE )Notify1xCfg_ReAuthPeriod_);
    }
}


/*
 * iteratePortControl
 *
 * if port equal NULL, then it will return the head
 * if not, it will return the one next from port
 */
static PORT_CONTROL* iteratePortControl(PORT_CONTROL *port) 
{
    HashTableType* HashTable = &_1xHashTable;

    if (!port ){
        if (!list_empty(&HashTable->hash_lh)){
            return (PORT_CONTROL*)HashTable->hash_lh.next;
        }else{
            return (PORT_CONTROL*)NULL;
        }
    }

    if( !(port->hashEntry.lh.next) ){
        return (PORT_CONTROL*)NULL;
    }
    
    if( &HashTable->hash_lh == port->hashEntry.lh.next ){
        return (PORT_CONTROL*)NULL;
    }else{
        return (PORT_CONTROL*) port->hashEntry.lh.next;
    }

}

void iterateAllPortControl(pHANDLE func)     //this func must have/need an pointer to PORT_CONTROL as only argument
{
    void (*routine)(PORT_CONTROL *) = (void (*)(PORT_CONTROL *))func;
    PORT_CONTROL *port;
    int i;
    
    for(i=0,port = iteratePortControl(NULL) ; port && i<DOT1X_MAX_PORT_NUMBER ; i++, port = iteratePortControl(port)){
        
        if(routine){
            routine(port);
        }else{
            PRINTMAC( port->hashEntry.mac, "iterateAllPortControl ");
        }
        
    }

}

/*-----------------------------------------------------------------------------
* ROUTINE NAME - Mib_PortCtrl_GetNext
*------------------------------------------------------------------------------
* FUNCTION: get next PortControl with non-zero staAddr
*
* INPUT   : 
* OUTPUT  : 
* RETURN  : 
* NOTE:
*-----------------------------------------------------------------------------*/
#ifdef APPS_INCLUDE_SNMP 
int Mib_PortCtrl_GetNext(PORT_CONTROL **pNext, PORT_CONTROL *pCurrent)
{
    
    *pNext = iteratePortControl(pCurrent);
    if(*pNext)
        return 0;
    else
        return -1;
}
#endif

