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
   Module      : event_handling_msg.c
   Date        : 2006-03-28
   Description : This file contains the implementations handling events by using
                 messages.
   \file 

   \remarks 
      
   \note 
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "event_handling.h"


#ifndef BITFIELD_EVENTS

/* Used structures for event handling, like IFX_TAPI_EVENT_t */
#include "drv_tapi_event_io.h"


/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** For scanning event in all channels. Also defined in drv_tapi_event.h */
#if 0
enum
{
   IFX_TAPI_EVENT_ALL_CHANNELS = 0xFFFF
};
#endif

/** Structure holding event */
IFX_TAPI_EVENT_t tapiEvent;

/** 1 if event exists in fifo, 0 fifo has no events */
IFX_int32_t haveEvent;

/* ============================= */
/* Local function declaration    */
/* ============================= */

static IFX_return_t EVENT_Handle_CID(IFX_uint32_t nDataCh);

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Check for events.

   \param nDevCtrl_FD  - file descriptor for control device
   \param nDataCh      - data channel

   \return IFX_SUCCESS if some events exists, otherwise IFX_ERROR
*/
IFX_return_t EVENT_Check(IFX_int32_t nDevCtrl_FD,
                         IFX_int32_t nDataCh)
{
   if ( (CHECK_ALL_CH != nDataCh) &&
        ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh)
        || (0 > nDevCtrl_FD)) )
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&tapiEvent, 0, sizeof(tapiEvent));

   if (CHECK_ALL_CH == nDataCh)
   {
      /* Check events on all channels */
      tapiEvent.ch = IFX_TAPI_EVENT_ALL_CHANNELS;
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Check for event on all channels\n"));
   }
   else
   {
      tapiEvent.ch = nDataCh;
      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
           ("Check for event on channel %d\n", (int) nDataCh));
   }

   /* Get the status of this data channel */
   /* IFX_ERROR - error, 0 or IFX_SUCCES no more events in fifo, but ok.
      1 - more events in fifo */
   haveEvent =
       ioctl(nDevCtrl_FD, IFX_TAPI_EVENT_GET, (IFX_int32_t) &tapiEvent);

   if (IFX_ERROR == haveEvent)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Channel %d -> Exception error when getting status. \
           (File: %s, line: %d)\n",
           (int) nDataCh, __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Got event.\n"));
      return IFX_SUCCESS;
   }
} /* EVENT_Check() */


/**
   Check is there is some event on this channel. 

   \param nDataCh - data channel

   \return IFX_TRUE event exists, otherwise IFX_FALSE
*/
IFX_boolean_t EVENT_Exists(IFX_int32_t nDataCh)
{
   IFX_boolean_t event_exists = IFX_FALSE;


   event_exists = (tapiEvent.ch == nDataCh)
                   && (tapiEvent.id != IFX_TAPI_EVENT_NONE);

   TRACE(TAPIDEMO, DBG_LEVEL_LOW,
        ("Event exists %s\n", (IFX_TRUE == event_exists) ? "Yes" : "No"));

   return event_exists;
} /* EVENT_Exists() */


/**
   Handles channel exceptions, events

   \param  pPhone  - pointer to PHONE
   \param  pCtrl   - pointer to status control structure
   \param  nDevIdx - file descriptor for channel

   \return New target state for the PHONE or US_NOTHING
*/
STATE_MACHINE_STATES_t EVENT_Handle(PHONE_t* pPhone,
                                    CTRL_STATUS_t* pCtrl,
                                    IFX_int32_t nDevIdx)
{
   STATE_MACHINE_STATES_t ret = US_NOTHING;
   IFX_char_t nDigit;


   if (IFX_NULL == pPhone)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   TRACE(TAPIDEMO, DBG_LEVEL_LOW, 
         ("dev %d, phone ch %d, data ch %d -> Event ID: %d\n\r",
         (int) nDevIdx, (int) pPhone->nPhoneCh, (int) pPhone->nDataCh,
         tapiEvent.id));

   switch (tapiEvent.id)
   {
      /* ON HOOK state */
      case IFX_TAPI_EVENT_FXS_ONHOOK:
      {
         TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
             ("Hook changes for phone channel %d. \
             (File: %s, line: %d)\n", 
             (int) pPhone->nPhoneCh, __FILE__, __LINE__));

         /* Phone is ONHOOK */
         ret |= US_READY;
         break;
      }

      /* OFF HOOK state */
      case IFX_TAPI_EVENT_FXS_OFFHOOK:
      {
         TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
             ("Hook changes for phone channel %d. \
             (File: %s, line: %d)\n", 
             (int) pPhone->nPhoneCh, __FILE__, __LINE__));

         /* Hook off */
         ret |= US_HOOKOFF;
         break;
      }

      /* Flash hook detected */
      case IFX_TAPI_EVENT_FXS_FLASH:
      {
         TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, 
              ("TAPI: Phone ch%d -> Flashhook detected. (File: %s, line: %d)\n",
              (int) pPhone->nPhoneCh, __FILE__, __LINE__));
         break;
      }

      /* Ground key detected */
      case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW:
      case IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH:
      {
         TRACE (TAPIDEMO, DBG_LEVEL_NORMAL,
               ("TAPI: Phone ch%d -> ground key detected. \
               (File: %s, line: %d)\n", (int) pPhone->nPhoneCh, __FILE__, __LINE__));
         break;
      }

      /* Ground key polarity detected */
      case IFX_TAPI_EVENT_FAULT_LINE_GK_NEG:
      case IFX_TAPI_EVENT_FAULT_LINE_GK_POS:
      {
         TRACE (TAPIDEMO, DBG_LEVEL_NORMAL,
               ("TAPI: Phone ch%d -> ground key polarity detected. \
               (File: %s, line: %d)\n", (int) pPhone->nPhoneCh, __FILE__, __LINE__));
         break;
      }

      case IFX_TAPI_EVENT_PULSE_DIGIT:
      {
         nDigit = tapiEvent.data.pulse.digit & 0x1f;
         if (DIGIT_STAR == nDigit)
            nDigit = 0;

         pPhone->nDialedNum[pPhone->nDialNrCnt] = nDigit;
         pPhone->nDialNrCnt++;
            
         if (DIGIT_HASH == nDigit)
         {
            ret |= US_CONFERENCE;
         }
         else
         {
            TRACE (TAPIDEMO, DBG_LEVEL_NORMAL,
                  ("TAPI: Phone ch%d -> Pulse Digit: %d \n\r",
                  (int) pPhone->nPhoneCh, (int)nDigit));
            ret |= US_DIALING;
         }
         break;
      }

      /*  MASK = 0x00000001 : Do not react on DTMF Exeptions.
          There is another function to read digit stored in
          the digit buffer */
      case IFX_TAPI_EVENT_DTMF_DIGIT:
      {
         nDigit = tapiEvent.data.dtmf.digit & 0x1f;
         if (DIGIT_STAR == nDigit)
         {
            nDigit = 0;
         }

         pPhone->nDialedNum[pPhone->nDialNrCnt] = nDigit;
         pPhone->nDialNrCnt++;

         if (DIGIT_HASH == nDigit)
         {
            ret |= US_CONFERENCE;
         }
         else
         {
            TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
                  ("TAPI: Data ch%d -> DTMF Digit: %d \n\r",
                  (int) pPhone->nDataCh, nDigit));

            ret |= US_DIALING;
         }
         break;
      }
      default:
         /* There are plenty more, but not handled at the moment */ 
         break;
   } /* switch */

   /** \todo, handle CID tapi signals, only if TAPI SIGNALLING enabled */
   if (pCtrl->pProgramArg->oArgFlags.nCID)
   {
#ifdef TAPI_SIGNAL_SUPPORT
      CID_Handle_CID(pPhone->nDataCh);
#endif /* TAPI_SIGNAL_SUPPORT */
   }

   return ret;
} /* EVENT_Handle() */


/**
   Check if DTMF dialing has occur.

   \param nDataCh - data channel

   \return IFX_TRUE - DTMF dialing occured, otherwise IFX_FALSE
*/
IFX_boolean_t EVENT_DTMF_Dialing(IFX_int32_t nDataCh)
{
   if ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_FALSE;
   }

   switch (tapiEvent.id)
   {
      case IFX_TAPI_EVENT_DTMF_DIGIT:
         return IFX_TRUE;
         break;
      default:
         return IFX_FALSE;
         break;
   }

   return IFX_FALSE;
} /* EVENT_DTMF_Dialing() */


/**
   Cid transmission handling, uses control device and channels fds.
   BUT detection of signalling must be ENABLED.

   \param nDataCh  - data channel

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR

   \remark TAPI signalling must be enabled.
*/
IFX_return_t EVENT_Handle_CID(IFX_uint32_t nDataCh)
{
   IFX_return_t ret = IFX_SUCCESS;


   if ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }


   switch (tapiEvent.id)
   {
      /* Check cid signals */
      case IFX_TAPI_EVENT_CID_TX_SEQ_END:
      case IFX_TAPI_EVENT_CID_TX_INFO_END:
      {
         TRACE(TAPIDEMO, DBG_LEVEL_LOW,
               ("CID: Data ch %d, tx ended succesfully\n",
               (int) nDataCh));
         break;
      }

      /* Check cid errors */
      case IFX_TAPI_EVENT_CID_TX_RINGCAD_ERR:
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
               ("CID: Data Ch %d, ring cadence settings error! "
               "(File: %s, line: %d)\n",
               (int) nDataCh, __FILE__, __LINE__));
         ret = IFX_ERROR;
         break;
      }

      case IFX_TAPI_EVENT_CID_TX_NOACK_ERR:
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
               ("CID: Data ch %d, no acknowledge during CID sequence! "
               "(File: %s, line: %d)\n",
               (int) nDataCh, __FILE__, __LINE__));
         ret = IFX_ERROR;
         break;
      }
     
      default:
         break;
   } /* switch */

   return ret;
} /* EVENT_Handle_CID() */


#endif /* BITFIELD_EVENTS */
