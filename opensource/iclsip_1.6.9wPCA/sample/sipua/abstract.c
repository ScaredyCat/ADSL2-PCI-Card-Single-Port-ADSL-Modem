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
   Module      : abstract.h
   Date        : 2006-01-25
   Description : This file contains abstraction functions between channels,
                 sockets and devices AND objects using them.

   \file 

   \remarks
      Abstraction.

   \note Changes:

*******************************************************************************/


/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "abstract.h"
#include "voip.h"
#include "pcm.h"

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */


/** How channels are represented by device, socket.
    Device /dev/vin11 will get status, data from phone channel 0 and 
    data channel 0. If at startup phone channel 0 is mapped to data channel 1
    then, phone will send actions to dev/vin11 and data will send actions to
    /dev/vin12.
    -1 means no channel at startup */

/** Holds default phone channel to data, pcm channel mappings
    defined at startup. */
#ifdef EASY3332

static const STARTUP_MAP_TABLE_t STARTUP_MAPPING[MAX_SYS_LINE_CH] = 
{
   /** phone, data, pcm */
   {  0,  0,  0  },
   {  1,  1,  1  },
/* others are free and can be used later for conferencing */
/*   { -1,  2,  2  },
     { -1,  3,  3  } */
};

#elif EASY334

static const STARTUP_MAP_TABLE_t STARTUP_MAPPING[MAX_SYS_LINE_CH] = 
{
   /** phone, data, pcm */
   {  0,  0,  0  },
   {  1,  1,  1  },
   {  2,  2,  2  },
   {  3,  3,  3  },
/* others are free and can be used later for conferencing
 */   
/*   { -1, -1,  4  },
     { -1, -1,  5  },
     { -1, -1,  6  },
     { -1, -1,  7  }*/
};
#elif VMMC
static const STARTUP_MAP_TABLE_t STARTUP_MAPPING[MAX_SYS_LINE_CH] = 
{
   /** phone, data, pcm */
   {  0,  0,  0 },
   {  1,  1,  1 }
};

#endif /* EASY334 */


/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   Return pointer to startup mapping table.
  
   \return Pointer to startup mapping table is returned.
*/

const STARTUP_MAP_TABLE_t* ABSTRACT_GetStartupMapTbl(IFX_void_t)
{
   return STARTUP_MAPPING;
} /* ABSTRACT_GetStartupMapTbl() */


/**
   Map channels at startup.

   \param pCtrl - handle to connection control structure

   \return IFX_SUCCESS everything ok, otherwise IFX_ERROR

   \remark By default mapping is 0 - 0 - 0, 1 - 1 - 1. It means that phone
           channel 0 is mapped to data 0 and pcm 0
*/
IFX_return_t ABSTRACT_DefaultMapping(CTRL_STATUS_t* pCtrl)
{
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   IFX_int32_t data_ch = 0;
   IFX_int32_t fd_ch = -1;


   if (IFX_NULL == pCtrl)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

   for (i = 0; i < MAX_SYS_LINE_CH; i++)
   {
      data_ch = STARTUP_MAPPING[i].nDataCh;
      /* Map phone channel x to data channel y */ 
      datamap.nDstCh = STARTUP_MAPPING[i].nPhoneCh;

      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Map phone ch %d to data ch %d\n",
            datamap.nDstCh, (int) data_ch));

      /* Get file descriptor for this data channel */
      fd_ch = Common_GetDeviceOfCh(data_ch);

      ret = ioctl(fd_ch, IFX_TAPI_MAP_DATA_ADD, (IFX_int32_t) &datamap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error mapping phone ch %d to data ch %d using fd &d. \
              (File: %s, line: %d)\n", 
              (int) data_ch, (int) fd_ch,
              __FILE__, __LINE__));
         return IFX_ERROR; 
      } 

      VOIP_ReserveDataCh(data_ch);

      pCtrl->rgoPhones[i].nDataCh = data_ch;
      pCtrl->rgoPhones[i].nPhoneCh = STARTUP_MAPPING[i].nPhoneCh;
      pCtrl->rgoPhones[i].nPCM_Ch = STARTUP_MAPPING[i].nPCM_Ch;

      PCM_ReserveCh(STARTUP_MAPPING[i].nPCM_Ch);
   }

   return ret;
} /* ABSTRACT_DefaultMapping() */


/**
   Unmaps all default mapping of channels (phone ch to data, pcm ch).

   \param pCtrl - handle to connection control structure

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t ABSTRACT_UnmapDefaults(CTRL_STATUS_t* pCtrl)
{
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   IFX_int32_t fd_ch = -1;


   if (IFX_NULL == pCtrl)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

   for (i = 0; i < MAX_SYS_LINE_CH; i++)
   {
      datamap.nDstCh = i;

      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Unmap phone ch %d from data ch %d\n",
            (int)i, (int)i));

      fd_ch = Common_GetDeviceOfCh(i);

      ret = ioctl(fd_ch, IFX_TAPI_MAP_DATA_REMOVE,
                    (IFX_int32_t) &datamap); 
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error unmapping phone ch %d from data ch %d using fd %d. \
              (File: %s, line: %d)\n", 
              (int) i, (int) i, (int) fd_ch, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Tell this data channel it is not mapped with phone channel */
      /* Because at startup it is already not mapped in our application, but
         its mapped only in VINETIC CHIP. */
      VOIP_FreeDataCh(i);

      /* PCM channels are not mapped at startup with phone channels. */

      pCtrl->rgoPhones[i].nDataCh = -1;
      pCtrl->rgoPhones[i].nPhoneCh = i;
   }
  
   return ret;
} /* CONFERENCE_UnmapDefaults() */


/**
   Get pointer of PHONE according to phone channel.
  
   \param pCtrl    - pointer to status control structure
   \param nPhoneCh - phone channel

   \return pointer to PHONE with this phone channel or IFX_NULL if none
*/
PHONE_t* ABSTRACT_GetPHONE_OfPhoneCh(CTRL_STATUS_t* pCtrl,
                                     IFX_int32_t nPhoneCh)
{
   IFX_int32_t i = 0;
   PHONE_t* p_phone = IFX_NULL;


   if ((IFX_NULL == pCtrl) || (0 > nPhoneCh) || (MAX_SYS_LINE_CH < nPhoneCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_NULL;
   }

   for (i = 0; i < MAX_SYS_LINE_CH; i++)
   {
      if (nPhoneCh == pCtrl->rgoPhones[i].nPhoneCh)
      {
         p_phone = &pCtrl->rgoPhones[i];
         break;
      }
   }

   return p_phone;
} /* ABSTRACT_GetPHONE_ByPhoneCh() */


/**
   Get pointer of PHONE according to data channel.
  
   \param pCtrl   - pointer to status control structure
   \param nDataCh - data channel
   \param pConn   - pointer to pointer of phone connections

   \return pointer to PHONE with this data channel or IFX_NULL if none
*/
PHONE_t* ABSTRACT_GetPHONE_OfDataCh(CTRL_STATUS_t* pCtrl,
                                    IFX_int32_t nDataCh,
                                    CONNECTION_t** pConn)
{
   IFX_int32_t i = 0;
   PHONE_t* phone = IFX_NULL;
   IFX_int32_t j = 0;


   if ((IFX_NULL == pCtrl) || (0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh)
       || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_NULL;
   }

   for (i = 0; i < MAX_SYS_LINE_CH; i++)
   {
      if (0 < pCtrl->rgoPhones[i].nConnCnt)
      {
         /* Connections to other phones exists and maybe this phone 
            uses additional mapped data channel for connection. */
         for (j = 0;j < pCtrl->rgoPhones[i].nConnCnt; j++)
         {
            if ((EXTERN_VOIP_CALL == pCtrl->rgoPhones[i].rgoConn[j].fType)
                && (nDataCh == pCtrl->rgoPhones[i].rgoConn[j].nUsedCh))
            {
               phone = &pCtrl->rgoPhones[i];
               *pConn = &pCtrl->rgoPhones[i].rgoConn[j];
               break;
            }
         }
      }

      if (nDataCh == pCtrl->rgoPhones[i].nDataCh)
      {
         phone = &pCtrl->rgoPhones[i];
         break;
      }
   }

   return phone;
} /* ABSTRACT_GetPHONE_ByDataCh() */


/**
   Get file descriptor of device for phone channel.
  
   \param pCtrl   - pointer to status control structure
   \param nPhoneCh - phone channel

   \return device connected to this phone channel or -1 if none
*/
IFX_int32_t ABSTRACT_GetDEV_OfPhoneCh(CTRL_STATUS_t* pCtrl,
                                      IFX_int32_t nPhoneCh)
{
   if ((IFX_NULL == pCtrl) || (0 > nPhoneCh) || (MAX_SYS_LINE_CH < nPhoneCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return NO_DEVICE_FD;
   }

   return Common_GetDeviceOfCh(nPhoneCh);
}


/**
   Distinct between events from phone or data channels.
  
   \param nDevIdx - device number
   \param fDTMF_Dialing - IFX_TRUE if DTMF dialing, otherwise IFX_FALSE

   \return IFX_TRUE if signals from phone channel
           and IFX_FALSE if signals from data channel
*/
IFX_boolean_t ABSTRACT_EventsFromPhoneCh(IFX_int32_t nDevIdx,
                                         IFX_boolean_t fDTMF_Dialing)
{
   if ((0 > nDevIdx) || (MAX_SYS_CH_RES < nDevIdx))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_FALSE;
   }

   if (nDevIdx >= MAX_SYS_LINE_CH)
   {
      /* Event from device with higher number of max phone channel. */
      return IFX_FALSE;
   }

   if (IFX_TRUE == fDTMF_Dialing)
   {
      /* DTMF is send from data channel */
      return IFX_FALSE;
   }

   return IFX_TRUE;
} /* ABSTRACT_EventsFromPhoneCh() */


/**
   Get PHONE according to socket.
  
   \param pCtrl   - pointer to status control structure
   \param nSocket - socket
   \param pConn   - pointer to pointer of phone connections

   \return pointer to PHONE or IFX_NULL if none
*/
PHONE_t* ABSTRACT_GetPHONE_OfSocket(CTRL_STATUS_t* pCtrl,
                                    IFX_int32_t nSocket,
                                    CONNECTION_t** pConn)
{
   IFX_int32_t i = 0;
   PHONE_t* phone = IFX_NULL;
   IFX_int32_t j = 0;


   if ((IFX_NULL == pCtrl) || (NO_SOCKET == nSocket)
      || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_NULL;
   }

   for (i = 0; i < MAX_SYS_LINE_CH; i++)
   {
      /* Socket index corresponds to data channel number */
      if (0 < pCtrl->rgoPhones[i].nConnCnt)
      {
         /* Connections to other phones exists and maybe this phone 
            uses additional socket for connection. */
         for (j = 0;j < pCtrl->rgoPhones[i].nConnCnt; j++)
         {
            if ((EXTERN_VOIP_CALL == pCtrl->rgoPhones[i].rgoConn[j].fType)
                && (nSocket == pCtrl->rgoPhones[i].rgoConn[j].nUsedSocket))
            {
               phone = &pCtrl->rgoPhones[i];
               *pConn = &pCtrl->rgoPhones[i].rgoConn[j];
               break;
            }
         }
      }

      if (nSocket == pCtrl->rgoPhones[i].nSocket)
      {
         phone = &pCtrl->rgoPhones[i];
         break;
      }
   }

   return phone;
} /* ABSTRACT_GetPHONE_OfSocket() */


/**
   Get PHONE according to PCM socket.
  
   \param pCtrl   - pointer to status control structure
   \param nSocket - PCM socket
   \param pConn   - pointer to pointer of phone connections

   \return pointer to PHONE or IFX_NULL if none
*/
PHONE_t* ABSTRACT_GetPHONE_Of_PCM_Socket(CTRL_STATUS_t* pCtrl,
                                         IFX_int32_t nPCM_Socket,
                                         CONNECTION_t** pConn)
{
   IFX_int32_t i = 0;
   PHONE_t* phone = IFX_NULL;
   IFX_int32_t j = 0;


   /* check input arguments */
   if ((IFX_NULL == pCtrl) || (NO_SOCKET == nPCM_Socket)
      || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_NULL;
   }

   for (i = 0; i < MAX_SYS_LINE_CH; i++)
   {
      /* Socket index corresponds to pcm channel number */
      if (0 < pCtrl->rgoPhones[i].nConnCnt)
      {
         /* Connections to other phones exists and maybe this phone 
            uses additional socket for connection. */
         for (j = 0;j < pCtrl->rgoPhones[i].nConnCnt; j++)
         {
            if ((PCM_CALL == pCtrl->rgoPhones[i].rgoConn[j].fType)
               && (nPCM_Socket == pCtrl->rgoPhones[i].rgoConn[j].nUsedSocket))
            {
               phone = &pCtrl->rgoPhones[i];
               *pConn = &pCtrl->rgoPhones[i].rgoConn[j];
               break;
            }
         }
      }

      if (nPCM_Socket == pCtrl->rgoPhones[i].nPCM_Socket)
      {
         phone = &pCtrl->rgoPhones[i];
         break;
      }
   }

   return phone;
} /* ABSTRACT_GetPHONE_Of_PCM_Socket() */

