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
   Module      : pcm.c
   Date        : 2006-03-29
   Description : This file contains the implementation of the functions for
                 the tapi demo working with PCM
   \file 

   \remarks PCM is only used when connection with phones is already established.
            Start and stop of the connection still uses ethernet connection 
            over two boards.
      
   \note Changes: 
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "pcm.h"
#include "cid.h"

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** At the moment we have only one PCM highway on board */
static IFX_int32_t nPCM_Highway = 0;

/** ALaw, uLaw, Linear and 8 or 16 bit. */
static IFX_TAPI_PCM_RES_t ePCM_Resolution = IFX_TAPI_PCM_RES_ALAW_8BIT;

/** Sample rate, minimum value is 512 kHz and can increase by following step:
   n*512kHz, n = 1..16 */
static IFX_int32_t nPCM_Rate = 2048;

/** Status of used pcm channels, IFX_FALSE - used, IFX_TRUE - not used */
static IFX_boolean_t fPCM_ChFree[MAX_SYS_PCM_RES];

/** Socket on which we expect incoming data (start/stop connection). Voice is 
    going through PCM. This sockets follow ordinary sockets (VoIP connection).
 */
IFX_int32_t rgnPCM_Sockets[MAX_SYS_PCM_RES];

/** Structure representing timeslot (RX, TX) */
typedef struct _TIMESLOT_t
{
   IFX_int32_t nRX;
   IFX_int32_t nTX;
} TIMESLOT_t;

/** Each PCM uses two timeslots, one for RX and other for TX.
    NOTICE: On Slave board this values are swapped. So what is RX on Master
            board is TX on Slave board and vice versa. And also this sequence 
            is valid only for caller, not for callee. Callee will get from
            which channel caller is calling and will take according values. */
const TIMESLOT_t PCM_TIMESLOT_PAIRS[MAX_SYS_PCM_RES] =
{

#ifdef EASY3332
   {0, 1},
   {2, 3},
   {4, 5},
   {6, 7}
#elif EASY334
   {0, 1},
   {2, 3},
   {4, 5},
   {6, 7},
   {8, 9},
   {10, 11},
   {12, 13},
   {14, 15}
#else
 /* Other board with different number of timeslots. */
#endif
};


/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   PCM Initialization.

   \param pProgramArg  - pointer to program arguments

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t PCM_Init(PROGRAM_ARG_t* pProgramArg)
{
   IFX_int32_t i = 0;


   if (IFX_NULL == pProgramArg)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_FALSE;
   }

   for (i = 0; i < MAX_SYS_PCM_RES; i++)
   {
      fPCM_ChFree[i] = IFX_TRUE;

      if (NO_SOCKET == PCM_InitSocket(pProgramArg, i))
      {
         /* Error initializing PCM sockets. */
         return IFX_ERROR;
      }
   }

   return IFX_SUCCESS;
} /* PCM_Init() */


/**
   Activate PCM

   \param nDataCh_FD - data channel file descriptor
   \param fMaster - IFX_TRUE - activate pcm on master board,
                    IFX_FALSE activate pcm on slave board
   \param fActivate - IFX_TRUE - activate pcm, IFX_FALSE - deactivate pcm
   \pararm nTimeSlotIdx - which pair of timeslots to use

   \return IFX_TRUE all ok, otherwise IFX_FALSE
*/
IFX_boolean_t PCM_SetActivation(IFX_int32_t nDataCh_FD,
                                IFX_boolean_t fMaster,
                                IFX_boolean_t fActivate,
                                IFX_int32_t nTimeSlotIdx)
{
   IFX_TAPI_PCM_CFG_t pcmConf;
   IFX_int32_t activated;


   if ((0 > nDataCh_FD) || (0 > nTimeSlotIdx)
       || (MAX_SYS_PCM_RES < nTimeSlotIdx))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_FALSE;
   }

   memset(&pcmConf, 0, sizeof (IFX_TAPI_PCM_CFG_t));

   pcmConf.nHighway = nPCM_Highway;
   pcmConf.nResolution = ePCM_Resolution;
   pcmConf.nRate = nPCM_Rate;

   /* Connection between master and slave is crossed */
   if (IFX_TRUE == fMaster)
   {
      pcmConf.nTimeslotRX = PCM_TIMESLOT_PAIRS[nTimeSlotIdx].nRX;
      pcmConf.nTimeslotTX = PCM_TIMESLOT_PAIRS[nTimeSlotIdx].nTX;
   }
   else
   {
      pcmConf.nTimeslotRX = PCM_TIMESLOT_PAIRS[nTimeSlotIdx].nTX;
      pcmConf.nTimeslotTX = PCM_TIMESLOT_PAIRS[nTimeSlotIdx].nRX;
   }

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, 
        ("RX timeslot %d, TX timeslot %d\n",
         (int) pcmConf.nTimeslotRX, (int) pcmConf.nTimeslotTX));

   ioctl(nDataCh_FD, IFX_TAPI_PCM_CFG_SET, (IFX_int32_t) &pcmConf);

   /* PCM channel has been configured, however the communication
      is not active yet */
   if (IFX_TRUE == fActivate)
   {
      ioctl(nDataCh_FD, IFX_TAPI_PCM_ACTIVATION_SET, (IFX_int32_t) 1);
   }
   else
   {
      ioctl(nDataCh_FD, IFX_TAPI_PCM_ACTIVATION_SET, (IFX_int32_t) 0);
   }

   /* Check if activated */
   ioctl(nDataCh_FD, IFX_TAPI_PCM_ACTIVATION_GET, &activated);
   if (1 == activated)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, 
           ("PCM activated on dev %d\n", (int) nDataCh_FD));
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, 
           ("PCM NOT activated on dev %d\n", (int) nDataCh_FD));
   }

   /* Now PCM communication is started - test it */
   return IFX_TRUE;
} /* PCM_Activate() */


/**
   Map pcm channel to phone channel.

   \param nDstCh     - target channel
   \param nAddCh     - which channel to add
   \param fDoMapping - IFX_TRUE do mapping, IFX_FALSE do unmapping
   \param pConf      - pointer to conference

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t PCM_MapToPhone(IFX_int32_t nDstCh,
                            IFX_int32_t nAddCh,
                            IFX_boolean_t fDoMapping,
                            CONFERENCE_t* pConf)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_PHONE_t phonemap;
   IFX_int32_t fd_ch = -1;


   if ((0 > nDstCh) || (MAX_SYS_PCM_RES < nDstCh)
       || (0 > nAddCh) || (MAX_SYS_LINE_CH < nAddCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&phonemap, 0, sizeof(IFX_TAPI_MAP_PHONE_t));

   fd_ch = Common_GetDeviceOfCh(nAddCh);

   phonemap.nPhoneCh = nDstCh;
   phonemap.nChType = IFX_TAPI_MAP_TYPE_PCM;
   if (IFX_TRUE == fDoMapping)
   {
      /* Map pcm channel to phone channel */ 
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Map phone ch %d to pcm ch %d\n",
            (int)nAddCh, (int)nDstCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_PHONE_ADD, (IFX_int32_t) &phonemap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error mapping phone ch %d to pcm ch %d using fd %d. \
              (File: %s, line: %d)\n", 
              (int) nAddCh, (int) nDstCh, (int) fd_ch,
              __FILE__, __LINE__));
         return IFX_ERROR;
      }

      if (IFX_NULL != pConf)
      {
         /* Add new pair of mapping */
         pConf->MappingTable[pConf->nMappingCnt].nCh = nDstCh;
         pConf->MappingTable[pConf->nMappingCnt].nAddedCh = nAddCh;
         pConf->MappingTable[pConf->nMappingCnt].nMappingType = PHONE_PCM;
         pConf->MappingTable[pConf->nMappingCnt].fMapping = IFX_TRUE;
         pConf->nMappingCnt++;
      }
   }
   else
   {
      /* Unmap pcm channel from phone channel */ 
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Unmap phone ch %d from pcm ch %d\n",
            (int)nAddCh, (int)nDstCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_PHONE_REMOVE, (IFX_int32_t) &phonemap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error unmapping phone ch %d from pcm ch %d using fd %d. \
              (File: %s, line: %d)\n", 
              (int) nAddCh, (int) nDstCh, (int) fd_ch,
              __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* PCM_MapToPhone() */


/**
   Map pcm channel to pcm channel.

   \param nDstCh     - target channel
   \param nAddCh     - which channel to add
   \param fDoMapping - IFX_TRUE do mapping, IFX_FALSE do unmapping
   \param pConf      - pointer to conference

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t PCM_MapToPCM(IFX_int32_t nDstCh,
                          IFX_int32_t nAddCh,
                          IFX_boolean_t fDoMapping,
                          CONFERENCE_t* pConf)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_PCM_t pcmmap;
   IFX_int32_t fd_ch = -1;


   if ((IFX_NULL == pConf) || (0 > nDstCh) || (MAX_SYS_PCM_RES < nDstCh)
       || (0 > nAddCh) || (MAX_SYS_PCM_RES < nAddCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&pcmmap, 0, sizeof(IFX_TAPI_MAP_PCM_t));

   fd_ch = Common_GetDeviceOfCh(nAddCh);

   pcmmap.nDstCh = nDstCh;
   pcmmap.nChType = IFX_TAPI_MAP_TYPE_PCM;
   if (IFX_TRUE == fDoMapping)
   {
      /* Map pcm channel to pcm channel */ 
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Map pcm ch %d to pcm ch %d\n",
            (int)nAddCh, (int)nDstCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_PCM_ADD, (IFX_int32_t) &pcmmap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error mapping pcm ch %d to pcm ch %d using fd %d. \
              (File: %s, line: %d)\n", 
              (int) nAddCh, (int) nDstCh, (int) fd_ch,
              __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Add new pair of mapping */
      pConf->MappingTable[pConf->nMappingCnt].nCh = nDstCh;
      pConf->MappingTable[pConf->nMappingCnt].nAddedCh = nAddCh;
      pConf->MappingTable[pConf->nMappingCnt].nMappingType = PCM_PCM;
      pConf->MappingTable[pConf->nMappingCnt].fMapping = IFX_TRUE;
      pConf->nMappingCnt++;
   }
   else
   {
      /* Unmap pcm channel from pcm channel */ 
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Unmap pcm ch %d from pcm ch %d\n",
            (int)nAddCh, (int)nDstCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_PCM_REMOVE, (IFX_int32_t) &pcmmap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error unmapping pcm ch %d to pcm ch %d using fd %d. \
              (File: %s, line: %d)\n", 
              (int) nAddCh, (int) nDstCh, (int) fd_ch,
              __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* PCM_MapToPCM() */


/**
   Map pcm channel to data channel.

   \param nDstCh     - target channel
   \param nAddCh     - which channel to add
   \param fDoMapping - IFX_TRUE do mapping, IFX_FALSE do unmapping
   \param pConf      - pointer to conference

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t PCM_MapToData(IFX_int32_t nDstCh,
                           IFX_int32_t nAddCh,
                           IFX_boolean_t fDoMapping,
                           CONFERENCE_t* pConf)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_int32_t fd_ch = -1;


   if ((IFX_NULL == pConf) || (0 > nDstCh) || (MAX_SYS_PCM_RES < nDstCh)
       || (0 > nAddCh) || (MAX_SYS_CH_RES < nAddCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

   fd_ch = Common_GetDeviceOfCh(nAddCh);

   datamap.nDstCh = nDstCh;
   datamap.nChType = IFX_TAPI_MAP_TYPE_PCM;
   if (IFX_TRUE == fDoMapping)
   {
      /* Map pcm channel to data channel */ 
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Map data ch %d to pcm ch %d\n",
            (int)nAddCh, (int) nDstCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_DATA_ADD, (IFX_int32_t) &datamap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error mapping data ch %d to pcm ch %d using fd %d. \
              (File: %s, line: %d)\n", 
              (int) nAddCh, (int) nDstCh, (int) fd_ch,
              __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Add new pair of mapping */
      pConf->MappingTable[pConf->nMappingCnt].nCh = nDstCh;
      pConf->MappingTable[pConf->nMappingCnt].nAddedCh = nAddCh;
      pConf->MappingTable[pConf->nMappingCnt].nMappingType = PCM_DATA;
      pConf->MappingTable[pConf->nMappingCnt].fMapping = IFX_TRUE;
      pConf->nMappingCnt++;
   }
   else
   {
      /* Unmap pcm channel from data channel */ 
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Unmap data ch %d from pcm ch %d\n",
            (int)nAddCh, (int) nDstCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_DATA_REMOVE, (IFX_int32_t) &datamap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error unmapping data ch %d to pcm ch %d using fd %d. \
              (File: %s, line: %d)\n", 
              (int) nAddCh, (int) nDstCh, (int) fd_ch,
              __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* PCM_MapToData() */



/**
   Init PCM socket (used for start/stop pcm connection)

   \param pProgramArg - pointer to program arguments
   \param nChIdx - channel index

   \return socket number or NO_SOCKET if error.
*/
IFX_int32_t PCM_InitSocket(PROGRAM_ARG_t* pProgramArg, IFX_int32_t nChIdx)
{
   IFX_int32_t s = NO_SOCKET;
   struct sockaddr_in my_addr;


   /* check input arguments */
   if ((IFX_NULL == pProgramArg) || (0 > nChIdx) || (MAX_SYS_PCM_RES < nChIdx))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return NO_SOCKET;
   }

   rgnPCM_Sockets[nChIdx] = NO_SOCKET;

   s = socket(PF_INET, SOCK_DGRAM, 0);
   if (NO_SOCKET == s)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("PCM_InitSocket: Can't create UDP socket, %s. \
           (File: %s, line: %d)\n", 
            strerror(errno), __FILE__, __LINE__));
      return NO_SOCKET;
   }

   bzero((IFX_char_t*) &my_addr, (IFX_int32_t) sizeof(my_addr));
   my_addr.sin_family = AF_INET;
   my_addr.sin_port = htons(PCM_SOCKET_PORT + nChIdx);


   if (pProgramArg->oMy_IP_Addr.sin_addr.s_addr)
   {
      my_addr.sin_addr.s_addr = pProgramArg->oMy_IP_Addr.sin_addr.s_addr;
   }
   else
   {
      my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   }

   if (-1 == (bind(s, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("InitUdpSocket: Can't bind to port, %s. (File: %s, line: %d)\n", 
            strerror(errno), __FILE__, __LINE__));
      return NO_SOCKET;
   }

   rgnPCM_Sockets[nChIdx] = s;
#ifndef VXWORKS
   /* make the socket non blocking */
   fcntl(s, F_SETFL, O_NONBLOCK);
#endif /* VXWORKS */

   return s;
} /* PCM_InitSocket() */


/**
   Return PCM socket according to channel idx.

   \param nChIdx - channel index

   \return socket or NO_SOCKET on error
*/
IFX_int32_t PCM_GetSocket(IFX_int32_t nChIdx)
{
   if ((0 > nChIdx) || (MAX_SYS_PCM_RES < nChIdx))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return NO_SOCKET;
   }

   return rgnPCM_Sockets[nChIdx];
}


/**
   Handle data from PCM socket.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pConn  - pointer to phone connection
   \param nCID_NameIdx - CID index where name is located


   \return IFX_SUCCESS message received and handled otherwise IFX_ERROR
*/
IFX_return_t PCM_HandleSocketData(CTRL_STATUS_t* pCtrl,
                                  PHONE_t* pPhone,
                                  CONNECTION_t* pConn,
                                  IFX_int32_t nCID_NameIdx)
{
   static IFX_char_t buf[400];
   IFX_int32_t size = 0;
   IFX_int32_t ret = 0;
   IFX_int32_t event = 0, from_ch = 0;
   COMM_MSG_t msg;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone) || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   size = sizeof(pConn->oConnPeer.oPCM.oToAddr);

   /* Got data from socket */
   ret = recvfrom(pConn->nUsedSocket, buf, sizeof(buf), 0,
                 (struct sockaddr *) &pConn->oConnPeer.oPCM.oToAddr,
                 /* Must be like that, otherwise you get compiler
                    warnings */
                 (int *) (IFX_int32_t) &size);

   if (0 > ret)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("No data read from PCM socket, %s. (File: %s, line: %d)\n", 
            strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else if (0 == ret)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("No data read from PCM socket.\n"));
      return IFX_SUCCESS;
   }
   else if (sizeof(msg) != ret)
   {
      /* Should not come here, because PCM socket is used only to
         initialize call (start it) or stop it. And is not used for
         data, voice - PCM is used for this part. */ 
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("PCM sockets are only used to initialize (start) "
            "connection or stop it.\n"));
      return IFX_ERROR;
   } 

   /* When we reach this point, we received our message, hopefully :-) */
   memcpy(&msg, &buf[0], sizeof(msg));

   if ((COMM_MSG_START_FLAG == msg.nMarkStart)
       && (COMM_MSG_END_FLAG == msg.nMarkEnd))
   {

      if (PCM_CALL_FLAG != msg.fPCM)
      {
         /* Got wrong message */
         return IFX_ERROR;
      }

      /* ch-nr of calling remote phone */
      from_ch = msg.nCh;
      /* event to send to called phone */
      event = msg.nAction;

      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Calling %s\n",
            pPhone->oCID_Msg.message[nCID_NameIdx].string.element));

      /* Status from where we got command */
      pPhone->nDstState = event;
      pConn->fType = PCM_CALL;
      pConn->oConnPeer.oPCM.nCh = from_ch;
      pConn->oConnPeer.oPCM.oToAddr.sin_port =
         htons(PCM_SOCKET_PORT + from_ch);

      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("%s: rcvd 0x%02X from %s:%d\n",
           pPhone->oCID_Msg.message[nCID_NameIdx].string.element, buf[0],
           inet_ntoa(pConn->oConnPeer.oPCM.oToAddr.sin_addr),
           ntohs(pConn->oConnPeer.oPCM.oToAddr.sin_port)));

      return IFX_SUCCESS;
   }
   else
   {
      /* If we came here then receive wrong message */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Received wrong msg from PCM socket %d. (File: %s, line: %d)\n",
            (int) pConn->nUsedSocket, __FILE__, __LINE__));
      return IFX_ERROR;
   }
} /* PCM_HandleSocketData() */


/**
   Return free PCM channel.

   \return free PCM channel or NO_FREE_PCM_CH
*/
IFX_int32_t PCM_GetFreeCh(IFX_void_t)
{
   IFX_int32_t i = 0;
   IFX_int32_t free_ch = NO_FREE_PCM_CH;


   for (i = 0; i < MAX_SYS_PCM_RES; i++)
   {
      if (IFX_TRUE == fPCM_ChFree[i])
      {
         free_ch = i;
         break;
      }
   }

   return free_ch;
} /* PCM_GetFreeCh() */


/**
   Free mapped PCM channel.

   \param nPCM_Ch - pcm channel number

   \return IFX_SUCCESS - pcm channel is freed, otherwise IFX_ERROR
*/
IFX_return_t PCM_FreeCh(IFX_int32_t nPCM_Ch)
{
   if ((0 > nPCM_Ch) || (MAX_SYS_PCM_RES < nPCM_Ch))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   fPCM_ChFree[nPCM_Ch] = IFX_TRUE;

   return IFX_SUCCESS;
} /* PCM_FreeCh() */


/**
   Reserve pcm channel as mapped.

   \param nPCM_Ch - pcm channel number

   \return IFX_SUCCESS - pcm channel is reserved, otherwise IFX_ERROR
*/
IFX_return_t PCM_ReserveCh(IFX_int32_t nPCM_Ch)
{
   if ((0 > nPCM_Ch) || (MAX_SYS_PCM_RES < nPCM_Ch))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (IFX_FALSE == fPCM_ChFree[nPCM_Ch])
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("PCM ch %d, already reserved\n", (int) nPCM_Ch));
   }

   fPCM_ChFree[nPCM_Ch] = IFX_FALSE;

   return IFX_SUCCESS;
} /* PCM_ReserveCh() */


/**
   Start connection on PCM, but basically activate PCM.

   \param pProgramArg - pointer to program arguments
   \param nPCM_Ch - pcm channel number
   \param nTimeSlotIdx - index of timeslot pair to use

   \return IFX_TRUE if activated, otherwise IFX_FALSE
*/
IFX_boolean_t PCM_StartConnection(PROGRAM_ARG_t* pProgramArg,
                                  IFX_int32_t nPCM_Ch,
                                  IFX_int32_t nTimeSlotIdx)
{
   if (IFX_NULL == pProgramArg)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_FALSE;
   }

   if ((pProgramArg->oArgFlags.nPCM_Master)
       || (pProgramArg->oArgFlags.nPCM_Slave))
   {
      if (pProgramArg->oArgFlags.nPCM_Master)
      {
         PCM_SetActivation(PCM_GetFD_OfCh(nPCM_Ch),
                           IFX_TRUE, IFX_TRUE, nTimeSlotIdx);
      }
      else
      {
         PCM_SetActivation(PCM_GetFD_OfCh(nPCM_Ch),
                           IFX_FALSE, IFX_TRUE, nTimeSlotIdx);
      }
   }
   return IFX_TRUE;
} /* PCM_StartConnection() */


/**
   Stop connection on PCM, but basically deactivate PCM.

   \param pProgramArg - pointer to program arguments
   \param nPCM_Ch - pcm channel number
   \param nTimeSlotIdx - index of timeslot pair to use

   \return IFX_TRUE if deactivated, otherwise IFX_FALSE
*/
IFX_boolean_t PCM_EndConnection(PROGRAM_ARG_t *pProgramArg,
                                IFX_int32_t nPCM_Ch,
                                IFX_int32_t nTimeSlotIdx)
{
   if (IFX_NULL == pProgramArg)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_FALSE;
   }

   if ((pProgramArg->oArgFlags.nPCM_Master)
       || (pProgramArg->oArgFlags.nPCM_Slave))
   {
      if (pProgramArg->oArgFlags.nPCM_Master)
      {
         PCM_SetActivation(PCM_GetFD_OfCh(nPCM_Ch),
                           IFX_TRUE, IFX_FALSE, nTimeSlotIdx);
      }
      else
      {
         PCM_SetActivation(PCM_GetFD_OfCh(nPCM_Ch),
                           IFX_FALSE, IFX_FALSE, nTimeSlotIdx);
      }
   }
   return IFX_TRUE;
} /* PCM_EndConnection() */


/**
   Get file descriptor of device for pcm channel.
  
   \param nPCM_Ch - pcm channel

   \return device connected to this channel or NO_DEVICE_FD if none
*/
IFX_int32_t PCM_GetFD_OfCh(IFX_int32_t nPCM_Ch)
{
   if ((0 > nPCM_Ch) || (MAX_SYS_PCM_RES < nPCM_Ch))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return NO_DEVICE_FD;
   }

   return Common_GetDeviceOfCh(nPCM_Ch);
} /* PCM_GetFD_OfCh() */
