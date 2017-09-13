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
   Module      : tapi_signal.c
   Date        : 2005-11-29
   Description : This file contains the implementation of the functions for
                 the tapi demo working with tapi signals (modem, fax tone 
                 signals)
   \file 

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "tapi_signal.h"

#ifdef LINUX

/* \todo Just empty functions, otherwise this two functions exists in 
   VxWorks. For linux still searching for appropriate substitution. */

IFX_return_t tickGet()
{
#if 0
   /** Returns system tick in milliseconds. */
   #define Get_Tick()   (UINT32)(jiffies * 1000 / HZ)
#endif
   return IFX_SUCCESS;
}

IFX_return_t sysClkRateGet()
{
#if 0
   /** Returns systen tick rate */
   #define Get_ClkRate()   (UINT32)(HZ)
#endif
   return IFX_SUCCESS;
}

#endif /* LINUX */

/* ============================= */
/* Global Structures             */
/* ============================= */

/** Holding timer value */
struct timeval tv;

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


#ifdef TAPI_SIGNAL_SUPPORT

/**
   Set timeout value for timer.

   \return NULL - no timeout, otherwise pointer to timeval structure
*/
struct timeval* TAPI_SIG_SetTimer(IFX_void_t)
{
   /* Poll connection state every 500 ms */
   IFX_int32_t nTimeout = WAIT_FOREVER; /* \todo 500; */
	struct timeval* pTimeOut = IFX_NULL; /* Wait forever by default */


	pTimeOut = &tv;

	/* Set timeout value */
	switch (nTimeout)
	{
	   case WAIT_FOREVER:
	      pTimeOut = IFX_NULL;
	      break;

	   case NO_WAIT:
	      tv.tv_sec = 0;
	      tv.tv_usec = 0;
	      break;

	   default:
	      tv.tv_sec = nTimeout / 1000;
	      tv.tv_usec = (nTimeout % 1000) * 1000;
	      break;
	} 

   return pTimeOut;
} /* TAPI_SIG_SetTimer() */


/**
   On timeout a TIMEOUT signal is send.

   \param  pConn      - pointer to phone connection
   \param  nStatus    - phone status

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TAPI_SIG_CheckTimeOut(CONNECTION_t* pConn,
                                   IFX_int32_t nStatus)
{
   IFX_return_t ret = IFX_SUCCESS;


   if (IFX_NULL == pConn)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   } 

   if (pConn->nTimeout != WAIT_FOREVER)
   {
      IFX_int32_t _tmptick;
/** \todo      if ((_tmptick = tickGet()) > pPhone->nTickExpire)*/
      {
         if (US_ACTIVE_TX == nStatus)
         {
            ret = HandleSignal(pConn->nUsedCh_FD, TAPI_SIGNAL_CALLER,
                               &pConn->nCurrentSignal,
                               TAPI_SIGNAL_TIMEOUT,
                               &pConn->nTimeout);
             if (IFX_ERROR == ret)
             {
                /* Handle error */
                return ret;
             }
         }

         if (US_ACTIVE_RX == nStatus)
         {
            ret = HandleSignal(pConn->nUsedCh_FD, TAPI_SIGNAL_CALLEE,
                               &pConn->nCurrentSignal,
                               TAPI_SIGNAL_TIMEOUT,
                               &pConn->nTimeout);
             if (IFX_ERROR == ret)
             {
                /* Handle error */
                return ret;
             }
         }
      }
   } /* if */

   return ret;
} /* TAPI_SIG_TimeOutSignal() */


/**
   Handle tapi signals.

   \param  pConn      - pointer to phone connection
   \param  nStatus    - phone status
   \param  pStatus    - pointer to status structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TAPI_SIG_SigHandling(CONNECTION_t* pConn,
                                  IFX_int32_t nStatus,
                                  IFX_TAPI_CH_STATUS_t* pStatus)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t tapi_signal;
   IFX_int32_t sigindex;


   if ((IFX_NULL == pConn) || (IFX_NULL == pStatus))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   } 

   if (US_ACTIVE_TX == nStatus)
   {
      tapi_signal = pStatus->signal;
      sigindex = 1;

      while (tapi_signal)
      {
         if (tapi_signal & sigindex)
         {
            ret = HandleSignal(pConn->nUsedCh_FD, TAPI_SIGNAL_CALLER,
                               &pConn->nCurrentSignal, sigindex,
                               &pConn->nTimeout);
            if (IFX_ERROR == ret)
            {
               /* Handle error */
               return ret;
            } 

            tapi_signal &= ~sigindex;
            if (pConn->nTimeout != WAIT_FOREVER)
            {
               int _tmptick = tickGet();
               pConn->nTickExpire =
                  ((pConn->nTimeout * sysClkRateGet()) / 1000)
                     + _tmptick /* tickGet()*/;
            }
         } 
         sigindex <<= 1;
      }
   }

   if (US_ACTIVE_RX == nStatus)
   {
      tapi_signal = pStatus->signal;
      sigindex = 1;

      while (tapi_signal)
      {
         if (tapi_signal & sigindex)
         {
            ret = HandleSignal(pConn->nUsedCh_FD, TAPI_SIGNAL_CALLEE,
                               &pConn->nCurrentSignal, sigindex,
                               &pConn->nTimeout);
             if (IFX_ERROR == ret)
             {
                /* Handle error */
                return ret;
             }

            tapi_signal &= ~sigindex;
            if (pConn->nTimeout != WAIT_FOREVER)
            {
               int _tmptick = tickGet();
               /* TRACE (TAPIDEMO, DBG_LEVEL_LOW, 
                        ("\nfd %d RX-Signal timeout set %d to.\n", 
                        pPhone->nDataCh_FD, pPhone->nTimeout)); */
               pConn->nTickExpire =
                  ((pConn->nTimeout * sysClkRateGet()) / 1000)
                     + _tmptick /* tickGet()*/;
            }
         }
         sigindex <<= 1;
      }
   }

   return ret;
} /* TAPI_SIG_SigHandling() */


/**
   Stops non-blocking ringing on phone line.

   \param nPhoneCh_FD - file descriptor of phone channel
*/
IFX_void_t TAPI_SIG_SetPhoneRinging(IFX_int32_t nPhoneCh_FD)
{
   /* CID sending with modems could influence signal detection, so
      no ringing on phones */
   ioctl(nPhoneCh_FD, IFX_TAPI_RING_STOP, NO_PARAM);
} /* TAPI_SIG_SetPhoneRinging() */


/**
   Setup initial signal detection config in transmit direction (Caller).

   \param  nDataCh_FD - file descriptor of data channel

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TAPI_SIG_TxSigConfig(IFX_int32_t nDataCh_FD)
{
   return TxSignalSetup(nDataCh_FD);
} /* TAPI_SIG_TxSigConfig() */


/**
   Setup initial signal detection config in receive direction (Callee).

   \param  nDataCh_FD - file descriptor of data channel

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TAPI_SIG_RxSigConfig(IFX_int32_t nDataCh_FD)
{
   return RxSignalSetup(nDataCh_FD);
} /* TAPI_SIG_RxSigConfig() */


/**
   Reset transmit signal handler.

   \param  pConn      - pointer to phone connection
   \param  nDataCh_FD - file descriptor of data channel

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TAPI_SIG_ResetTxSigHandler(CONNECTION_t* pConn)
{
   if (IFX_NULL == pConn)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   } 

   return TxSignalHandlerReset(pConn->nUsedCh_FD, 
                               &pConn->nCurrentSignal,
                               &pConn->nTimeout);
} /* TAPI_SIG_ResetTxSigHandler() */


/**
   Reset receiver signal handler.

   \param  pConn      - pointer to phone connection
   \param  nDataCh_FD - file descriptor of data channel

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TAPI_SIG_ResetRxSigHandler(CONNECTION_t* pConn)
{
   if (IFX_NULL == pConn)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   } 

   return RxSignalHandlerReset(pConn->nUsedCh_FD,
                               &pConn->nCurrentSignal,
                               &pConn->nTimeout);
} /* TAPI_SIG_ResetRxSigHandler() */


/* ---------------------------------------------------------------------- */
#else /* TAPI_SIGNAL_SUPPORT */
/* ---------------------------------------------------------------------- */


struct timeval* TAPI_SIG_SetTimer(IFX_void_t)
{
   return IFX_NULL;
}


IFX_return_t TAPI_SIG_CheckTimeOut(CONNECTION_t* pConn, IFX_int32_t nStatus)
{
   return IFX_SUCCESS;
}


IFX_return_t TAPI_SIG_SigHandling(CONNECTION_t* pConn,
                                  IFX_int32_t nStatus,
                                  IFX_TAPI_CH_STATUS_t* pStatus)
{
   return IFX_SUCCESS;
}


IFX_void_t TAPI_SIG_SetPhoneRinging(IFX_int32_t nPhoneCh_FD)
{
   /* cid sending with modems could influence signal detection, but if
      we don´t use TAPI signalling then phones can ring. */
   ioctl(nPhoneCh_FD, IFX_TAPI_RING_START, NO_PARAM);
}


IFX_return_t TAPI_SIG_TxSigConfig(IFX_int32_t nDataCh_FD)
{
   return IFX_SUCCESS;
}


IFX_return_t TAPI_SIG_RxSigConfig(IFX_int32_t nDataCh_FD)
{
   return IFX_SUCCESS;
}


IFX_return_t TAPI_SIG_ResetTxSigHandler(CONNECTION_t* pConn)
{
   return IFX_SUCCESS;
}


IFX_return_t TAPI_SIG_ResetRxSigHandler(CONNECTION_t* pConn)
{
   return IFX_SUCCESS;
}

#endif /* TAPI_SIGNAL_SUPPORT */
