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
   Module      : cid.c
   Date        : 2005-11-29
   Description : This file contains the implementation of the functions for
                 the tapi demo working with Caller ID.
   \file 

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "cid.h"


/* ============================= */
/* Local structures              */
/* ============================= */

/** Structure holding CID for one user (phone number, date, user name) */
typedef struct _CID_USER_t
{
   IFX_uint32_t nMonth;
   IFX_uint32_t nDay;
   IFX_uint32_t nHour;
   IFX_uint32_t nMn;
   IFX_char_t sName[IFX_TAPI_CID_MSG_LEN_MAX];
   IFX_char_t sNumber[IFX_TAPI_CID_MSG_LEN_MAX];
} CID_USER_t;


/** Structure holding standard number and standard description */
typedef struct _CID_STANDARD_t
{
   IFX_int32_t  nNumber;
   IFX_char_t  *sDescription;
} CID_STANDARD_t;

/* ============================= */
/* Global Structures             */
/* ============================= */


/* ============================= */
/* Global function declaration   */
/* ============================= */


/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Current CID standard to use */
static IFX_TAPI_CID_STD_t eCurrCID_Std = IFX_TAPI_CID_STD_ETSI_FSK;

/** Predefined CID data for 4 users */
const static CID_USER_t CID_USERS[CID_MAX_USERS] =
{
   {11, 3, 10, 44, "Lisa\0", "12345\0"},
   {11, 3, 10, 44, "Homer\0", "67890\0"},
   {11, 3, 10, 44, "March\0", "33333\0"},
   {11, 3, 10, 44, "Bart\0", "44444\0"}
};

/** CID standards with description */
const static CID_STANDARD_t CID_STANDARDS[CID_MAX_STANDARDS] =
{
   {IFX_TAPI_CID_STD_TELCORDIA, "TELCORDIA, Belcore, USA\0"},
   {IFX_TAPI_CID_STD_ETSI_FSK, "ETSI FSK, Europe\0"},
   {IFX_TAPI_CID_STD_ETSI_DTMF, "ETSI DTMF, Europe\0"},
   {IFX_TAPI_CID_STD_SIN, "SIN BT, Great Britain\0"},
   {IFX_TAPI_CID_STD_NTT, "NTT, Japan\0"},
};


/* ============================= */
/* Local function definition     */
/* ============================= */


/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   Prepare CID message.

   \param  nIdx - cid user index, by default its the same as channel number
   \param  pPhone - pointer to PHONE

   \return status (IFX_SUCCESS or IFX_ERROR)

   \remark
      This test assumes straights connections between phone and data channel
*/
IFX_return_t CID_SetupData(IFX_uint32_t nIdx,
                           PHONE_t* pPhone)
{
   IFX_uint8_t element = 0;

   /** \todo CID_SetupData() according to CID types we have different
             structure of CID messages */

   if ((IFX_NULL == pPhone) || (0 > nIdx) || (CID_MAX_USERS < nIdx))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&pPhone->oCID_Msg.message[0], 0, sizeof(pPhone->oCID_Msg.message));

   TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Setup datetime element in CID.\n"));

   if ((0 != CID_USERS[nIdx].nMonth) && (0 != CID_USERS[nIdx].nDay))
   {  
      pPhone->oCID_Msg.message[element].date.elementType =
         IFX_TAPI_CID_ST_DATE;

      /* Only 1 - 12 allowed */
      pPhone->oCID_Msg.message[element].date.month =
         (CID_USERS[nIdx].nMonth % 13);

      /* Only 1 - 31 allowed */
      pPhone->oCID_Msg.message[element].date.day = (CID_USERS[nIdx].nDay % 32);
      /* Only 0 - 23 allowed */
      pPhone->oCID_Msg.message[element].date.hour = (CID_USERS[nIdx].nHour % 24);
      /* Only 0 - 59 allowed */
      pPhone->oCID_Msg.message[element].date.mn = (CID_USERS[nIdx].nMn % 60);
      element ++;
   }

   TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Setup number CLI element in CID.\n"));

   if (IFX_NULL != CID_USERS[nIdx].sNumber) 
   {
      pPhone->oCID_Msg.message[element].string.elementType =
         IFX_TAPI_CID_ST_CLI;

      pPhone->oCID_Msg.message[element].string.len =
         strlen(CID_USERS[nIdx].sNumber);

      strncpy(pPhone->oCID_Msg.message[element].string.element,
              CID_USERS[nIdx].sNumber,
              sizeof(pPhone->oCID_Msg.message[element].string.element));
      element ++;
   } 

   TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Setup name element in CID.\n"));
   if (IFX_NULL != CID_USERS[nIdx].sName)
   {
      pPhone->oCID_Msg.message[element].string.elementType =
         IFX_TAPI_CID_ST_NAME;

      pPhone->oCID_Msg.message[element].string.len =
         strlen(CID_USERS[nIdx].sName);

      strncpy(pPhone->oCID_Msg.message[element].string.element,
              CID_USERS[nIdx].sName,
              sizeof(pPhone->oCID_Msg.message[element].string.element));
      element++;
   } 

   /* Put these elements in the cid message */
   if (0 < element)
   {
      pPhone->oCID_Msg.nMsgElements = element;
      return IFX_SUCCESS;
   }

   return IFX_ERROR;
} /* CidSetupData() */


/**
   Displays cid structure, which has 3 elements:
   1.) date,
   2.) CLI number
   3.) name

   \param pPhone - pointer to PHONE

   \return output on screen
*/
IFX_void_t CID_Display(PHONE_t* pPhone)
{
   /* check input arguments */
   if (IFX_NULL == pPhone)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return;
   }

   SEPARATE
   printf("There are %d usefull CIDs\n", MAX_SYS_LINE_CH);
   printf("Element count %d\n", pPhone->oCID_Msg.nMsgElements);
   printf("Day: %d, Month: %d, Hour: %d, Minutes: %d\n",
      pPhone->oCID_Msg.message[CID_IDX_DATE].date.day,
      pPhone->oCID_Msg.message[CID_IDX_DATE].date.month,
      pPhone->oCID_Msg.message[CID_IDX_DATE].date.hour,
      pPhone->oCID_Msg.message[CID_IDX_DATE].date.mn);

   printf("CLI number is %s\n",
      pPhone->oCID_Msg.message[CID_IDX_CLI_NUM].string.element);

   printf("Name si %s\n", 
      pPhone->oCID_Msg.message[CID_IDX_NAME].string.element);
  SEPARATE
} /* Show_CID_Number() */


/**
   Set CID standard to use

   \param nCID_Standard - CID standard number

   \return IFX_SUCCESS if OK, IFX_ERROR if errors
*/
IFX_return_t CID_SetStandard(IFX_int32_t nCID_Standard)
{
   IFX_TAPI_CID_STD_t cid_standard = IFX_TAPI_CID_STD_ETSI_FSK;


   memset(&cid_standard, 0, sizeof(cid_standard));

   if ((0 > nCID_Standard) || (CID_MAX_STANDARDS < nCID_Standard))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s), will use default %s CID standard. "
            "(File: %s, line: %d)\n",
            CID_STANDARDS[IFX_TAPI_CID_STD_ETSI_FSK].sDescription,
            __FILE__, __LINE__));
      /* Don´t send error, but use default value, ETSI-FSK (Europe). */
      cid_standard = IFX_TAPI_CID_STD_ETSI_FSK;
   } 
   else
   {
      cid_standard = (IFX_TAPI_CID_STD_t) (nCID_Standard);
   } 

   eCurrCID_Std = cid_standard;

   TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Used CID standard %d => %s\n",
         (int) CID_STANDARDS[cid_standard].nNumber,
         CID_STANDARDS[cid_standard].sDescription));

   return IFX_SUCCESS;
} /* CID_GetStandard() */


/**
   Return CID standard to use

   \return CID standard
*/
IFX_TAPI_CID_STD_t CID_GetStandard(IFX_void_t)
{
   /* set standard */
   return eCurrCID_Std;
}


/**
   Configures driver for CID services

   \param nDataCh_FD - file descriptor of data channel

   \return IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_ConfDriver(IFX_uint32_t nDataCh_FD)
{
   IFX_TAPI_CID_CFG_t cid_std;


   if (0 > nDataCh_FD)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR; 
   }

   memset(&cid_std, 0, sizeof(cid_std));
   cid_std.nStandard = eCurrCID_Std;
   /* Selection of CID standard to be used */
   if (IFX_ERROR == ioctl(nDataCh_FD, IFX_TAPI_CID_CFG_SET,
                          (IFX_int32_t) &cid_std))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("CID configuration set failed for this channel. \
            (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   return IFX_SUCCESS;
} /* CID_ConfDriver() */


/**
   Sends a caller id sequence according to hook state, standard, ...

   \param nDataCh_FD - file descriptor of data channel
   \param nHookState - state of hook ONHOOK or OFFHOOK
   \param pCID_Msg   - pointer to CID message
    
   \return IFX_SUCCESS if OK, otherwise IFX_ERROR
*/
IFX_return_t CID_Send(IFX_uint32_t nDataCh_FD,
                      IFX_uint8_t nHookState,
                      IFX_TAPI_CID_MSG_t *pCID_Msg)
{
   if (IFX_NULL == pCID_Msg)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Stop ringing before any cid transmission. */
   if (IFX_ERROR == ioctl(nDataCh_FD, IFX_TAPI_RING_STOP, 0))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, ("CID: ring stop error!\n\r"));
      return IFX_ERROR;
   }

   /* Message type is call setup. */
   pCID_Msg->messageType = IFX_TAPI_CID_MT_CSUP;
   if (nHookState & IFX_TAPI_LINE_HOOK_STATUS_OFFHOOK)
   {
      /* Offhook cid */
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("CID: offhook tx\n\r"));
      pCID_Msg->txMode = IFX_TAPI_CID_HM_OFFHOOK;
   }
   else
   {
      /* Onhook cid */
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("CID: onhook tx\n\r"));
      pCID_Msg->txMode = IFX_TAPI_CID_HM_ONHOOK;
   }

   /* Transmit the caller id */
   /* If the complete on-hook sequence is required (e.g. Telcordia standard) */
   /* send CID */
   if (IFX_ERROR == ioctl(nDataCh_FD, IFX_TAPI_CID_TX_SEQ_START,
       (IFX_int32_t) pCID_Msg))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, ("CID: seq tx error!\n\r"));
      return IFX_ERROR;
   }

#if 0
   /* At the moment only SEQ START is used. */
   
   /* Otherwise, if only the FSK transmission is required */
   if (IFX_ERROR == ioctl(nDataCh_FD, IFX_TAPI_CID_TX_INFO_START,
        (IFX_int32_t) pCID_Msg))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, ("CID: seq tx error!\n\r"));
      return IFX_ERROR;
   } 
#endif 

   return IFX_SUCCESS;
} /* CID_Send() */

