/****************************************************************************
                  Copyright (c) 2006  Infineon Technologies AG
                     Am Campeon 1-12; 81726 Munich, Germany

   THE DELIVERY OF THIS SOFTWARE AS WELL AS THE HEREBY GRANTED NON-EXCLUSIVE,
   WORLDWIDE LICENSE TO USE, COPY, MODIFY, DISTRIBUTE AND SUBLICENSE THIS
   SOFTWARE IS FREE OF CHARGE.

   THE LICENSED SOFTWARE IS PROVIDED "AS IS" AND INFINEON EXPRESSLY DISCLAIMS
   ALL REPRESENTATIONS AND WARRANTIES, WHETHER EXPRESS OR IMPLIED, INCLUDING
   WITHOUT LIMITATION, WARRANTIES OR REPRESENTATIONS OF WORKMANSHIP,
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, DURABILITY, THAT THE
   OPERATING OF THE LICENSED SOFTWARE WILL BE ERROR FREE OR FREE OF ANY
   THIRD PARTY CLAIMS, INCLUDING WITHOUT LIMITATION CLAIMS OF THIRD PARTY
   INTELLECTUAL PROPERTY INFRINGEMENT.

   EXCEPT FOR ANY LIABILITY DUE TO WILLFUL ACTS OR GROSS NEGLIGENCE AND
   EXCEPT FOR ANY PERSONAL INJURY INFINEON SHALL IN NO EVENT BE LIABLE FOR
   ANY CLAIM OR DAMAGES OF ANY KIND, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ****************************************************************************
   Module      : qos_support.c
   Date        : 2005-11-29
   Description : This file contains the implementation of the functions for
                 the tapi demo working with quality of service
   \file 

   \remarks QOS now only works for first called peer, not an array of peers.
      
   \note Changes: Only support in LINUX at the moment. 
                  Look drv_qos.h for further info.
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "qos.h"

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */


#ifdef QOS_SUPPORT

/**
   Turn QoS Service on

   \param pProgramArg - pointer to program arguments
*/
IFX_void_t QOS_TurnServiceOn(PROGRAM_ARG_t *pProgramArg)
{
   if (IFX_NULL == pProgramArg)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return;
   }

   pProgramArg->oArgFlags.nQos = 1;
} /* QOS_TurnServiceOn() */


/**
   Initialize qos pair, only used with external calls.

   \param nPhoneNumber - phone number with 4 digits
                         (iiip iii=IP-number, p=port-number)
   \param pCtrl        - pointer to status control structure
   \param pConn        - pointer to phone connection
*/
IFX_void_t QOS_InitializePairStruct(IFX_int32_t nPhoneNumber,
                                    CTRL_STATUS_t* pCtrl, 
                                    CONNECTION_t* pConn)
{
   IFX_int32_t qos_port = -1;


   if ((IFX_NULL == pConn) || (IFX_NULL == pCtrl))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return;
   }

   /* For Qos support, use another port for voice */
   if (pCtrl->pProgramArg->oArgFlags.nQos)
   {
      qos_port = nPhoneNumber % 10 + UDP_QOS_PORT;
   }

    /* Initialize pair struct */
   if ((pCtrl->pProgramArg->oArgFlags.nQos)
       || (0 == pConn->oSession.nInSession))
   {
      pConn->oSession.oPair.srcPort = htons(UDP_QOS_PORT + pConn->nUsedCh);
      pConn->oSession.oPair.srcAddr =
         pCtrl->pProgramArg->oMy_IP_Addr.sin_addr.s_addr;
      pConn->oSession.oPair.destPort = htons(qos_port);
      pConn->oSession.oPair.destAddr =
         pConn->oConnPeer.oRemote.oToAddr.sin_addr.s_addr;
   } 
} /* QOS_InitializePairStruct() */


/**
   Cleans QoS session

   \param pCtrl       - pointer to status control structure
   \param pProgramArg - pointer to program arguments
*/
IFX_void_t QOS_CleanUpSession(CTRL_STATUS_t* pCtrl, PROGRAM_ARG_t *pProgramArg)
{
   IFX_int32_t i = 0;


   if ((IFX_NULL == pProgramArg) || (IFX_NULL == pCtrl))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return;
   }

   /* \todo Should QOS session be cleaned also after call to
      one phone ended? */

   /* Clean up qos sessions */
   if (pProgramArg->oArgFlags.nQos)
   {
      for (i = 0; i < MAX_SYS_LINE_CH; i++)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Device cleanuped\n"));
         ioctl(pCtrl->rgoPhones[i].nDataCh_FD, FIO_QOS_CLEAN, 0);
      }
      pProgramArg->oArgFlags.nQos = 0;
   }
} /* QOS_CleanUpSession() */


/**
   Handle QOS Session.

   \param  pCtrl - pointer to status control structure
   \param  pConn - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t QOS_HandleService(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn)
{
   if ((IFX_NULL == pConn) || (IFX_NULL == pCtrl))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   } 

   /* In case of Qos support, data are not to be handled by the application in
      case of remote call, once a qos session was initiated, only tapidemo 
      events which are COMM_MSG_t byte long. */

/* Also use QoS or udp redirect on local calls */
/*   if ((EXTERN_VOIP_CALL == pConn->fType) */
   if ((PCM_CALL != pConn->fType)
       && (pCtrl->pProgramArg->oArgFlags.nQos)
       && (1 == pConn->oSession.nInSession))
   {
      TRACE (TAPIDEMO, DBG_LEVEL_LOW,
            ("Qos - UDP redirect will handle data instead of tapidemo.\n"));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* QOS_HandleService() */


/**
   Starts qos session.

   \param pCtrl - pointer to status control structure
   \param pConn - pointer to phone connection
   \param nAction - flag to setup port, address of session
*/
IFX_void_t QOS_StartSession(CTRL_STATUS_t* pCtrl,
                            CONNECTION_t* pConn, 
                            QOS_ACTION_t nAction)
{
   if ((IFX_NULL == pConn) || (IFX_NULL == pCtrl))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return;
   }

   /* Remote call : start qos session */
   /* Also use QoS or udp redirect on local calls */
/*   if ((EXTERN_VOIP_CALL == pConn->fType) */
   if ((PCM_CALL != pConn->fType)
         && (pCtrl->pProgramArg->oArgFlags.nQos)
         && (0 == pConn->oSession.nInSession))
   {
      if (EXTERN_VOIP_CALL == pConn->fType)
      {
         /* Start qos session */
         if (SET_ADDRESS_PORT == nAction)
         {
            pConn->oSession.oPair.srcPort = htons(pConn->nUsedCh + UDP_QOS_PORT);
            pConn->oSession.oPair.srcAddr =
               pCtrl->pProgramArg->oMy_IP_Addr.sin_addr.s_addr;
            pConn->oSession.oPair.destPort =
             htons( (ntohs(pConn->oConnPeer.oRemote.oToAddr.sin_port))
                     - UDP_PORT + UDP_QOS_PORT );
            pConn->oSession.oPair.destAddr =
             pConn->oConnPeer.oRemote.oToAddr.sin_addr.s_addr;
         } 
      }
      else /* Local call */
      {
         /* Will use ports as channel numbers in local connection. */
         pConn->oSession.oPair.srcPort = pConn->nUsedCh;
         pConn->oSession.oPair.destPort =
            pConn->oConnPeer.oLocal.pPhone->nDataCh;
      }

      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
            ("FIO_QOS_START --> src:%08X:%d, dst:%08X:%d\n",
            (int) pConn->oSession.oPair.srcAddr,
            (int) pConn->oSession.oPair.srcPort,
            (int) pConn->oSession.oPair.destAddr,
            (int) pConn->oSession.oPair.destPort));

      ioctl(pConn->nUsedCh_FD, FIO_QOS_START, &pConn->oSession.oPair);

      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
            ("FIO_QOS_ACTIVATE --> src:%08X:%d\n",
            (int) pConn->oSession.oPair.srcAddr,
            (int) pConn->oSession.oPair.srcPort));
      
      ioctl(pConn->nUsedCh_FD, FIO_QOS_ACTIVATE, pConn->oSession.oPair.srcPort);
      pConn->oSession.nInSession = 1;
   }
} /* QOS_StartSession() */


/**
   Stops qos session.

   \param pCtrl - pointer to status control structure
   \param pConn - pointer to phone connection
*/
IFX_void_t QOS_StopSession(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn)
{
   if ((IFX_NULL == pConn) || (IFX_NULL == pCtrl))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return;
   }

   /* Remote call : stop qos session */
/* Also use QoS or udp redirect on local calls */
/*   if ((EXTERN_VOIP_CALL == pConn->fType) */
   if ((PCM_CALL != pConn->fType)
        && (pCtrl->pProgramArg->oArgFlags.nQos)
        && (1 == pConn->oSession.nInSession))
   {
      /* Stop qos session */
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
       ("FIO_QOS_STOP: src:%08X:%d, dst:%08X:%d\n",
        (int) pConn->oSession.oPair.srcAddr,
        (int) pConn->oSession.oPair.srcPort,
        (int) pConn->oSession.oPair.destAddr,
        (int) pConn->oSession.oPair.destPort));

/*      ioctl(pConn->nUsedCh_FD, FIO_QOS_STOP, pConn->oSession.oPair.srcPort);*/
      ioctl(pConn->nUsedCh_FD, FIO_QOS_STOP, 65535);
      pConn->oSession.nInSession = 0;
   }
} /* QOS_StopSession() */

/* ---------------------------------------------------------------------- */
#else /* QOS_SUPPORT */
/* ---------------------------------------------------------------------- */

IFX_void_t QOS_TurnServiceOn(PROGRAM_ARG_t *pDemo)
{
   printf("QoS support not available\n");
} 


IFX_void_t QOS_InitializePairStruct(IFX_int32_t number,
                                    CTRL_STATUS_t* pCtrl, 
                                    CONNECTION_t* pConn)
{
   /* empty function */
} 


IFX_void_t QOS_CleanUpSession(CTRL_STATUS_t* pCtrl, PROGRAM_ARG_t *pDemo)
{
   /* empty function */
} 


IFX_return_t QOS_HandleService(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn)
{
   /* Don´t have QoS support so application can handle the data. */  
   return IFX_SUCCESS;
} 


IFX_void_t QOS_StartSession(CTRL_STATUS_t* pCtrl,
                            CONNECTION_t* pConn, 
                            QOS_ACTION_t nAction)
{
   /* empty function */
} 


IFX_void_t QOS_StopSession(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn)
{
   /* empty function */
} 

#endif /* QOS_SUPPORT */
