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
   Module      : conference_support.c
   Date        : 2005-12-01
   Description : This file enhance tapi demo with conferencing.
   \file 

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "conference.h"
#include "pcm.h"
#include "voip.h"
#include "analog.h"
#include "qos.h"
#include "tapi_signal.h"


/* ============================= */
/* Local structures              */
/* ============================= */


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


/**
   Put phone on hold, because call to another phone will be made.

   \todo Implement it.

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_PutOnHold(IFX_void_t)
{
   return IFX_SUCCESS;
}

/**
   Resume phone, which is on hold.

   \todo Implement it.

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_ResumeOnHold(IFX_void_t)
{
   return IFX_SUCCESS;
}


/**
   Configure LEC and turn it on or off for analog phone or for PCM.

   \todo Implement it.

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t CONFERENCE_Set_LEC(IFX_void_t)
{
   return IFX_SUCCESS;
}


/**
   Restores mapping (can restore whole table or just for specific channel).

   \param pConf         - pointer to conference structure
   \param nChNum        - RESTORE_ALL_CH_MAPPING restore whole table,
                          otherwise channel number
   \param nWhichChCheck - DESTINATION_CH the channel to whom we were mapped
                          or ADDED_CH the channel which was added/mapped
                          or BOTH_CH can be added or destination, but number is
                          important.
   \param nChType       - Channel type which must be unmapped from other 
                          channels

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_RestoreMapping(CONFERENCE_t* pConf,
                                       IFX_int32_t nChNum,
                                       WHICH_CH_CHECK_t nWhichChCheck,
                                       CH_TYPE_CHECK_t nChType)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t deleted_pairs = 0;
   IFX_int32_t cnt = 0;
   IFX_boolean_t status = IFX_FALSE;


   if (IFX_NULL == pConf)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (MAX_MAPPING > pConf->nMappingCnt)
   {
      cnt = pConf->nMappingCnt - 1;

      TRACE(TAPIDEMO, DBG_LEVEL_LOW, 
           ("Mapping table has %d pairs. \n", (int)pConf->nMappingCnt));

      while (0 <= cnt)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_LOW,
              ("Restoring mapping: ch %d, map_ch %d, action type %d, "
               "mapping/unmapping %d.\n", 
              (int) pConf->MappingTable[cnt].nCh,
              (int) pConf->MappingTable[cnt].nAddedCh,
              (int) pConf->MappingTable[cnt].nMappingType,
              (int) pConf->MappingTable[cnt].fMapping));

         /* Check if restoring for all channels or just specific one and
            we can check if this one is destination channel or was added. */
         status = (RESTORE_ALL_CH_MAPPING == nChNum)
                  || ((pConf->MappingTable[cnt].nAddedCh == nChNum)
                     && (ADDED_CH == nWhichChCheck))
                  || ((pConf->MappingTable[cnt].nCh == nChNum)
                     && (DESTINATION_CH == nWhichChCheck));

         if (!status)
         {
            /* PCM has one special case, when pcm is mapped to pcm. This 
               mapping must also be removed regarding destination, added ch
               when pcm is removed from conference. */
            if ((PCM_PCM == pConf->MappingTable[cnt].nMappingType)
               && ((nChNum == pConf->MappingTable[cnt].nCh)
               || (nChNum == pConf->MappingTable[cnt].nAddedCh)))
            {
               TRACE(TAPIDEMO, DBG_LEVEL_LOW,
                    ("Special case with PCM mapping\n"));

               status = IFX_TRUE;
            }
         }

         if (status)
         {
            switch (pConf->MappingTable[cnt].nMappingType)
            {
               case PHONE_PHONE:
                  if ((ALL_CH == nChType) || (PHONE_CH == nChType))
                  {
                     ret = ALM_MapToPhone(pConf->MappingTable[cnt].nCh,
                                          pConf->MappingTable[cnt].nAddedCh,
                                          ~pConf->MappingTable[cnt].fMapping,
                                          pConf);
                     if (IFX_ERROR == ret)
                     {
                        return IFX_ERROR;
                     }
                     deleted_pairs++;
                     /* Status, this pair is cleared */
                     pConf->MappingTable[cnt].nMappingType = NO_MAPPING;
                  }
               break;
               case PHONE_DATA:
                  if ((ALL_CH == nChType) || (PHONE_CH == nChType)
                     || (DATA_CH == nChType))
                  {
                     ret = VOIP_MapPhoneToData(pConf->MappingTable[cnt].nCh,
                                               pConf->MappingTable[cnt].nAddedCh,
                                               ~pConf->MappingTable[cnt].fMapping,
                                               pConf);
                     if (IFX_ERROR == ret)
                     {
                        return IFX_ERROR;
                     } 
                     deleted_pairs++;
                     /* Status, this pair is cleared */
                     pConf->MappingTable[cnt].nMappingType = NO_MAPPING;
                  }
               break;
               case PHONE_PCM:
                  if ((ALL_CH == nChType) || (PHONE_CH == nChType)
                     || (PCM_CH == nChType))
                  {
                     ret = PCM_MapToPhone(pConf->MappingTable[cnt].nCh,
                                          pConf->MappingTable[cnt].nAddedCh,
                                          ~pConf->MappingTable[cnt].fMapping,
                                          pConf);
                     if (IFX_ERROR == ret)
                     {
                        return IFX_ERROR;
                     }
                     deleted_pairs++;
                     /* Status, this pair is cleared */
                     pConf->MappingTable[cnt].nMappingType = NO_MAPPING;
                  }
               break;
               case PCM_PCM:
                  if ((ALL_CH == nChType) || (PCM_CH == nChType))
                  {
                     ret = PCM_MapToPCM(pConf->MappingTable[cnt].nCh,
                                        pConf->MappingTable[cnt].nAddedCh,
                                        ~pConf->MappingTable[cnt].fMapping,
                                        pConf);
                     if (IFX_ERROR == ret)
                     {
                        return IFX_ERROR;
                     }
                     deleted_pairs++;
                     /* Status, this pair is cleared */
                     pConf->MappingTable[cnt].nMappingType = NO_MAPPING;
                  }
               break;
               case PCM_DATA:
                  if ((ALL_CH == nChType) || (PCM_CH == nChType)
                     || (DATA_CH == nChType))
                  {
                     ret = PCM_MapToData(pConf->MappingTable[cnt].nCh,
                                         pConf->MappingTable[cnt].nAddedCh,
                                         ~pConf->MappingTable[cnt].fMapping,
                                         pConf);
                     if (IFX_ERROR == ret)
                     {
                        return IFX_ERROR;
                     }
                     deleted_pairs++;
                     /* Status, this pair is cleared */
                     pConf->MappingTable[cnt].nMappingType = NO_MAPPING;
                  }
               break;
               case NO_MAPPING:
                  /* This mapping pair is empty */
               break;
               default:
                  /* Wrong action */
               break;
            } /* switch */
         } /* if */

         /* Go to previous mapping pair, until we get to first one */
         cnt--;
      } /* while */
   } /* if */

   return ret;
} /* CONFERENCE_RestoreMapping() */


/**
   Search for first free conference and return index of it.

   \param pCtrl  - handle to connection control structure

   \return conference index if free found, otherwise NO_CONFERENCE

*/
IFX_int32_t CONFERENCE_GetIdx(CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t i = 0;
   IFX_int32_t conf_idx = NO_CONFERENCE;


   if (IFX_NULL == pCtrl)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return US_DIALING;
   }

   for (i = 0;i < MAX_CONFERENCES; i++)
   {
      if (IFX_FALSE == pCtrl->rgoConferences[i].fActive)
      {
         conf_idx = i + 1;
         break;
      }
   }

   return conf_idx;
} /* CONFERENCE_GetIdx() */


/**
   Start conference, put this PHONE into new state.

   \param pCtrl  - handle to connection control structure
   \param pPhone - pointer to PHONE

   \return state US_DIALING, conference not possible or US_CONFERENCE

   \todo solve problem if wrong dialing, so this phone is not connected
   \todo when on hold will be implemented, new button, for example * and then 
         all can talk and listen.
*/
STATE_MACHINE_STATES_t CONFERENCE_Start(CTRL_STATUS_t* pCtrl,
                                        PHONE_t* pPhone)
{
   STATE_MACHINE_STATES_t ret = US_DIALING;
   IFX_boolean_t status = IFX_FALSE;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return US_DIALING;
   }

   /* Can start conference only if already in connection with one 
      party and we also didn´t reach maximum parties in conference
      and we are not in conference yet OR if we are in conference we must
      be master (the channel who started conference) */
   /* we started call and are in connection */
   status = (NO_CONFERENCE == pPhone->nConfIdx)
            || ((NO_CONFERENCE != pPhone->nConfIdx)
            && (IFX_TRUE == pPhone->nConfStarter));

   if ((US_ACTIVE_TX == pPhone->nStatus)
       && (MAX_PEERS_IN_CONF > pPhone->nConnCnt)
       && status)
   {
      if (NO_CONFERENCE == pPhone->nConfIdx)
      {
         /* We start conference, because more than two phones
            are in connection. */
         pCtrl->nConfCnt++;
         if (MAX_CONFERENCES < pCtrl->nConfCnt)
         {
            /* Reached maximum of possible conferences. */
            TRACE (TAPIDEMO, DBG_LEVEL_HIGH,
                  ("Reached maximum conferences that are possible %d.\n"
                   , MAX_CONFERENCES));

            return US_DIALING;
         } 
         pPhone->nConfStarter = IFX_TRUE;
         pPhone->nConfIdx = CONFERENCE_GetIdx(pCtrl);
         if (NO_CONFERENCE == pPhone->nConfIdx)
         {
            /* All conferences are occupied. */
            TRACE (TAPIDEMO, DBG_LEVEL_HIGH, ("All conferences occupied.\n"));
            return US_DIALING;
         }
         pCtrl->rgoConferences[pPhone->nConfIdx].fActive = IFX_TRUE;
      } 
      ret = US_CONFERENCE;
   }
   else
   {
      TRACE (TAPIDEMO, DBG_LEVEL_NORMAL,
            ("No connection to the phone exists OR we reached max."
             " parties in conference OR we are not the initiator.\n"));
      ret = US_NOTHING;
   }

   return ret;
} /* CONFERENCE_Start() */


/**
   Add local phone to conference.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pDst   - pointer to called PHONE (local phone which
                   will be added to conference, connection )

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_AddLocalPeer(CTRL_STATUS_t* pCtrl,
                                     PHONE_t* pPhone,
                                     PHONE_t* pDstPhone,
                                     CONNECTION_t* pConn)
{
   CONFERENCE_t* pConf = IFX_NULL;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t i = 0;
   PHONE_t* first_phone = IFX_NULL;
   CONNECTION_t* conn = IFX_NULL;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone) || (IFX_NULL == pDstPhone))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Get conference if exists */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   } 

   if ((0 < pPhone->nConnCnt) && (IFX_NULL != pConf)
       && (IFX_TRUE == pPhone->nConfStarter))
   {
      /* If we have conference and we are the initiator */
      
      /* Conference with another local phone */
      if ((MIN_PARTY_CONFERENCE == pPhone->nConnCnt)
         && (LOCAL_CALL == pPhone->rgoConn[0].fType))
      {
         /* CONFERENCE WAS STARTED, adding THIRD party to the connection */
         /* Called third party, am starting conference, must stop codec 
            for called first local phone */
         first_phone = pPhone->rgoConn[0].oConnPeer.oLocal.pPhone;
         VOIP_StopCodec(first_phone->nDataCh);

         TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("AddLocalPeer() -> starting "
                                         "conference with 3 local "
                                         "parties.\n"));
         
         if (NO_EXTERNAL_PEER == pConf->nExternalPeersCnt)
         {
            /* Also stop codec for initiator phone, its not needed in local
               connections. */
            VOIP_StopCodec(pPhone->nDataCh);

            /* Map first called phone ch to initiator phone ch. */
            ret = ALM_MapToPhone(pPhone->nPhoneCh,
                                 first_phone->nPhoneCh,
                                 IFX_TRUE, pConf);
         }
         pConf->nLocalPeersCnt++;
      }

      /* If first call was PCM and mapping table is empty, then we add one 
         mapping (phone to pcm). */
      if ((PCM_CALL == pPhone->rgoConn[0].fType)
          && (0 == pConf->nMappingCnt))
      {
         /* Add new pair of mapping */
         pConf->MappingTable[pConf->nMappingCnt].nCh = pPhone->nPCM_Ch;
         pConf->MappingTable[pConf->nMappingCnt].nAddedCh = pPhone->nPhoneCh;
         pConf->MappingTable[pConf->nMappingCnt].nMappingType = PHONE_PCM;
         pConf->MappingTable[pConf->nMappingCnt].fMapping = IFX_TRUE;
         pConf->nMappingCnt++;
      }

      /* Map called phone ch to initiator phone ch. */
      ret = ALM_MapToPhone(pPhone->nPhoneCh,
                           pDstPhone->nPhoneCh,
                           IFX_TRUE, pConf);

      /* Don´t start codec for this called local phone, am using mapping. */
      VOIP_SetCodecFlag(pConn->oConnPeer.oLocal.pPhone->nDataCh, IFX_FALSE);
      
      for (i = 0; i < pPhone->nConnCnt - 1; i++)
      {
         conn = &pPhone->rgoConn[i];
         switch (conn->fType)
         {
            case EXTERN_VOIP_CALL:
               /* Because of external call also map phone channel to 
                  all data channel(s) that are participating in conference */
               ret = VOIP_MapPhoneToData(conn->nUsedCh,
                                         pDstPhone->nPhoneCh,
                                         IFX_TRUE,
                                         pConf);
               /* Maybe conference was started and we have already connection
                  to another external peer, so increase first time also for
                  this external peer */
               if (NO_EXTERNAL_PEER == pConf->nExternalPeersCnt)
               {
                  pConf->nExternalPeersCnt++;
               }
               break;
            case PCM_CALL:
               /* In connection also with PCM call, map this phone channel to
                  PCM channel */
               ret = PCM_MapToPhone(conn->nUsedCh,
                                    pDstPhone->nPhoneCh,
                                    IFX_TRUE, pConf);
               /* Maybe conference was started and we have already connection
                  to another external peer, so increase first time also for
                  this external peer */
               if (NO_PCM_PEER == pConf->nPCM_PeersCnt)
               {
                  pConf->nPCM_PeersCnt++;
               }
               break;
            case LOCAL_CALL:
               /* Map previous called phone ch with new called phone ch. */
               ret = ALM_MapToPhone(conn->oConnPeer.oLocal.pPhone->nPhoneCh,
                                    pDstPhone->nPhoneCh,
                                    IFX_TRUE, pConf);

               /* Don´t start codec for local phone, am using mapping */
               VOIP_SetCodecFlag(conn->oConnPeer.oLocal.pPhone->nDataCh,
                                 IFX_FALSE);
               break;
            default:
               /* Unknown call, do nothing */
            break;
         } /* switch */
      } /* for */

      pConf->nLocalPeersCnt++;
   } /* if */

   /* If first time calling local phone this flag is set */
   pPhone->fLocalPeerCalled = IFX_TRUE;

   return ret;
} /* CONFERENCE_AddLocalPeer() */


/**
   Add external phone to conference.

   \param pCtrl      - pointer to status control structure
   \param pPhone     - pointer to PHONE
   \param pDst       - pointer to pointer of destination connection structure
   \param pCurrPeer  - pointer to pointer of peer
   \param nChChanged - status if transfer call procedure to another data ch

   \return IFX_SUCCESS if ok otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_AddExternalPeer(CTRL_STATUS_t* pCtrl,
                                        PHONE_t* pPhone,
                                        CONNECTION_t* pConn)
{
   CONFERENCE_t* p_conf = IFX_NULL;
   IFX_uint32_t new_data_ch = 0;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_uint32_t phone_ch = 0;
   IFX_uint32_t i = 0;
   CONNECTION_t* conn = IFX_NULL;
   IFX_boolean_t first_called = IFX_FALSE;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone) || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Get conference */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      p_conf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

   if (IFX_FALSE == pPhone->fExtPeerCalled)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("AddExternalPeer() -> "
                                      "first VOIP EXTERN call\n"));

      /* First time making connection with other external phone */
      /* but we could be connected with local or PCM phone(s) */
      pPhone->fExtPeerCalled = IFX_TRUE;
      first_called = IFX_TRUE;
      /* Save caller channel */
      pConn->nUsedCh = pPhone->nDataCh;
      pConn->nUsedCh_FD = pPhone->nDataCh_FD;
      pConn->nUsedSocket = pPhone->nSocket;

      if ((0 < pPhone->nConnCnt)
         && (IFX_TRUE == pPhone->nConfStarter))
      {
         /* Map initiator to data channel, but is already mapped at start. */
         /*ret = VOIP_MapPhoneToData((*pDst)->nDataCh, pPhone->nPhoneCh,
                                   pCtrl, p_conf);*/
      }

      new_data_ch = pPhone->nDataCh;
   }
   else if ((IFX_TRUE == pPhone->nConfStarter)
          && (IFX_NULL != p_conf))
   {
      /* This channel is already in connection with other phone, so
         we must use different channel for new external phone */
      /* So primary data channel is used for dialing the number
         and when ext call, a new free data channel must be used
         and leave old one in state talking */

      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("AddExternalPeer() -> "
                                      "next VOIP EXTERN call\n"));

      /* If first call was PCM and mapping table is empty, then we add one 
         mapping (phone to pcm). */
      if ((PCM_CALL == pPhone->rgoConn[0].fType)
          && (0 == p_conf->nMappingCnt))
      {
         /* Add new pair of mapping */
         p_conf->MappingTable[p_conf->nMappingCnt].nCh = pPhone->nPCM_Ch;
         p_conf->MappingTable[p_conf->nMappingCnt].nAddedCh = pPhone->nPhoneCh;
         p_conf->MappingTable[p_conf->nMappingCnt].nMappingType = PHONE_PCM;
         p_conf->MappingTable[p_conf->nMappingCnt].fMapping = IFX_TRUE;
         p_conf->nMappingCnt++;
      }

      /* Get free data channel */
      new_data_ch = VOIP_GetFreeDataCh();

      if (NO_FREE_DATA_CH == new_data_ch)
      {
         /* There is no more free data channels */
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("No more free data channels. \
              (File: %s, line: %d)\n", 
               __FILE__, __LINE__));
         /* Put this PHONE back to active for other phones in conference. */
         pPhone->nStatus = US_ACTIVE_TX;
         return IFX_ERROR;
      }

      /* Save caller channel */
      pConn->nUsedCh = new_data_ch;
      pConn->nUsedCh_FD = VOIP_GetFD_OfCh(new_data_ch);
      pConn->nUsedSocket = VOIP_GetSocket(new_data_ch);

      TRACE(TAPIDEMO, DBG_LEVEL_LOW, 
           ("Use data ch %d, socket %d for new external connection.\n",
            (int) new_data_ch, (int) pConn->nUsedSocket));

      /* Maybe conference was started and we have already connection to
         another external peer, so increase first time also for this external
         peer. */
      if (NO_EXTERNAL_PEER == p_conf->nExternalPeersCnt)
      {
         p_conf->nExternalPeersCnt++;
      }

      /* Map initiator to this new data channel - its important that initiator
         is mapped first, so its also mapped to signal module. */
      ret = VOIP_MapPhoneToData(new_data_ch, pPhone->nPhoneCh,
                                IFX_TRUE, p_conf);
      VOIP_ReserveDataCh(new_data_ch);
   }

   for (i = 0; i < pPhone->nConnCnt - 1; i++)
   {
      conn = &pPhone->rgoConn[i];

      switch (conn->fType)
      {
         case PCM_CALL:
            /* Map PCM to other data channels in conference */
            ret = PCM_MapToData(pPhone->nPCM_Ch, conn->nUsedCh,
                                IFX_TRUE, p_conf);
         break;
         case LOCAL_CALL:
            /* Because of external call also map phone channel to 
               data channel */
            phone_ch = (conn->oConnPeer).oLocal.pPhone->nPhoneCh;
            ret = VOIP_MapPhoneToData(new_data_ch, phone_ch,
                                      IFX_TRUE, p_conf);
            /* Only map phones if first time calling external phone and 
               no conference before. */
            if ((IFX_TRUE == first_called)
                && (MIN_PARTY_CONFERENCE == pPhone->nConnCnt))
            {
               /* Map phone to phone ch */
               ret = ALM_MapToPhone(pPhone->nPhoneCh, phone_ch,
                                    IFX_TRUE, p_conf);
               /* Stop codec on previous local phones. At the moment
                  we are connected only with one local phone and not
                  in conference. If more local phones then we are in
                  conference and phones are mapped. */
               VOIP_StopCodec(conn->oConnPeer.oLocal.pPhone->nDataCh);
            }
         break;
         default:
            /* External call, do nothing */
         break;
      } 
   } /* for */

   if (IFX_NULL != p_conf)
   {
      p_conf->nExternalPeersCnt++;
   }

   return ret;
} /* CONFERENCE_AddExternalPeer() */



/**
   Add pcm phone to conference.

   \param pCtrl      - pointer to status control structure
   \param pPhone     - pointer to PHONE
   \param pDst       - pointer to pointer of destination connection structure
   \param pCurrPeer  - pointer to pointer of peer
   \param nChChanged - status if transfer call procedure to another data ch

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_AddPCM_Peer(CTRL_STATUS_t* pCtrl,
                                    PHONE_t* pPhone,
                                    CONNECTION_t* pConn)
{
   CONFERENCE_t* p_conf = IFX_NULL;
   IFX_uint32_t new_pcm_ch = 0;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_uint32_t i = 0;
   IFX_uint32_t phone_ch = 0;
   CONNECTION_t* conn = IFX_NULL;
   IFX_boolean_t first_called = IFX_FALSE;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Get conference */
   if ((NO_CONFERENCE != pPhone->nConfIdx)
       && (MAX_CONFERENCES >= pPhone->nConfIdx))
   {
      p_conf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
   }

   if (IFX_FALSE == pPhone->fPCM_PeerCalled)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("AddPCM_Peer() -> first PCM call\n"));

      /* First time making connection with other pcm phone, but we could be
         connected with local phone(s) or external phone(s) */
      pPhone->fPCM_PeerCalled = IFX_TRUE;
      first_called = IFX_TRUE;
      /* Save caller channel */
      pConn->nUsedCh = pPhone->nPCM_Ch;
      pConn->nUsedCh_FD = PCM_GetFD_OfCh(pPhone->nPCM_Ch);

      if (MIN_PARTY_CONFERENCE == pPhone->nConnCnt)
      {
         /* NO CONFERENCE YET */
         if (IFX_FALSE == pPhone->fExtPeerCalled)
         {
            /* Stop codec also on initiator. */
            VOIP_StopCodec(pPhone->nDataCh);

            /* Map first called phone ch to initiator phone ch. */
            ret = ALM_MapToPhone(pPhone->nPhoneCh,
                           pPhone->rgoConn[0].oConnPeer.oLocal.pPhone->nPhoneCh,
                           IFX_TRUE, p_conf);
         }
         else
         {
         }
      }

      /* Map phone channel to pcm channel */
      new_pcm_ch = pPhone->nPCM_Ch;

      ret = PCM_MapToPhone(pPhone->nPCM_Ch, pPhone->nPhoneCh,
                           IFX_TRUE, p_conf);

      PCM_ReserveCh(pPhone->nPCM_Ch);
   } /* if */
   else if ((IFX_TRUE == pPhone->nConfStarter) && (IFX_NULL != p_conf))
   {
      /* This pcm channel is already in connection with other pcm phone, so
         we must use different channel for new pcm phone */

      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("AddPCM_Peer() -> next PCM call\n"));
      
      /* If first call was PCM and mapping table is empty, then we add one 
         mapping (phone to pcm) for the first call. */
      if ((PCM_CALL == pPhone->rgoConn[0].fType)
          && (0 == p_conf->nMappingCnt))
      {
         /* Add new pair of mapping */
         p_conf->MappingTable[p_conf->nMappingCnt].nCh = pPhone->nPCM_Ch;
         p_conf->MappingTable[p_conf->nMappingCnt].nAddedCh = pPhone->nPhoneCh;
         p_conf->MappingTable[p_conf->nMappingCnt].nMappingType = PHONE_PCM;
         p_conf->MappingTable[p_conf->nMappingCnt].fMapping = IFX_TRUE;
         p_conf->nMappingCnt++;
      }

      /* Get free pcm channel */
      new_pcm_ch = PCM_GetFreeCh();
      if (NO_FREE_PCM_CH == new_pcm_ch)
      {
         /* There is no more free pcm channels */
         /* Wrong input arguments */
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("No more free pcm channels. \
              (File: %s, line: %d)\n", 
               __FILE__, __LINE__));
         /* Put this PHONE back to active for other phones in conference. */
         pPhone->nStatus = US_ACTIVE_TX;
         return IFX_ERROR;
      }

      PCM_ReserveCh(new_pcm_ch);

      /* Save caller channel */
      pConn->nUsedCh = new_pcm_ch;
      pConn->nUsedCh_FD = PCM_GetFD_OfCh(new_pcm_ch);

      TRACE(TAPIDEMO, DBG_LEVEL_LOW, 
           ("Use pcm ch %d, socket %d for new external connection.\n",
            (int) new_pcm_ch, (int) pConn->nUsedCh_FD));
      
      /* Maybe conference was started and we have already connection to
         another pcm peer, so increase first time also for this pcm
         peer */
      if (NO_PCM_PEER == p_conf->nPCM_PeersCnt)
      {
         p_conf->nPCM_PeersCnt++;
      }

      /* Map initiator to new pcm channel */
      ret = PCM_MapToPhone(new_pcm_ch, pPhone->nPhoneCh, IFX_TRUE, p_conf);

   } /* else if */

   if (IFX_TRUE == pPhone->nConfStarter)
   {
      for (i = 0; i < pPhone->nConnCnt - 1; i++)
      {
         conn = &pPhone->rgoConn[i];
         switch (conn->fType)
         {
            case EXTERN_VOIP_CALL:
               /* Map PCM to data channel */
               ret = PCM_MapToData(new_pcm_ch, conn->nUsedCh,
                                   IFX_TRUE, p_conf);
            break;
            case LOCAL_CALL:
               /* We have local connection, so map  phone ch to this pcm ch. */
               phone_ch = conn->oConnPeer.oLocal.pPhone->nPhoneCh;

               ret = PCM_MapToPhone(new_pcm_ch, phone_ch, IFX_TRUE, p_conf);

               /* Stop codec on previous local phone. */
               VOIP_StopCodec(conn->oConnPeer.oLocal.pPhone->nDataCh);
            break;
            case PCM_CALL:
               if (IFX_FALSE == first_called)
               {
                  /* We have pcm connection, so map phone ch to this pcm ch. */
                  ret = PCM_MapToPCM(new_pcm_ch, conn->nUsedCh,
                                     IFX_TRUE, p_conf);
               }
            break;
            default:
               /* Unknown call, do nothing */
            break;
         } 
      } /* for */

      if (IFX_NULL != p_conf)
      {
         p_conf->nPCM_PeersCnt++;
      }
   } /* if */

   return ret;
} /* CONFERENCE_AddPCM_Peer() */


/**
   Remove peer from conference.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pConn  - pointer to pointer of phone connections
   \param pConf  - pointer to conference structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR

   \remark Each data channel can be connected only to one external, but 
           can be connected/mapped with many local or pcm peers.
           Because of deleting connection, pConn will point to first
           connection.
*/
IFX_return_t CONFERENCE_RemovePeer(CTRL_STATUS_t* pCtrl,
                                   PHONE_t* pPhone,
                                   CONNECTION_t** pConn,
                                   CONFERENCE_t* pConf)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_boolean_t peer_removed = IFX_FALSE;
   CONNECTION_t* conn = IFX_NULL;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone)
       || (IFX_NULL == pConf) || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   conn = *pConn;
   if (IFX_NULL == conn)
   {
      /* Connections are missing */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("No phone connections to delete. (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (LOCAL_CALL == conn->fType)
   {
      /* LOCAL */
      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
           ("Removing local party from conference, called phone ch %d\n",
           (int) conn->oConnPeer.oLocal.pPhone->nPhoneCh));

      /* Unmap everything from this local phone channel */
      ret = CONFERENCE_RestoreMapping(pConf,
                                      conn->oConnPeer.oLocal.pPhone->nPhoneCh,
                                      ADDED_CH, PHONE_CH);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error restoring mapping for phone ch %d. \
              (File: %s, line: %d)\n", 
              (int) conn->oConnPeer.oLocal.pPhone->nPhoneCh,
              __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Can start codec again on this local peer */
      VOIP_SetCodecFlag(conn->oConnPeer.oLocal.pPhone->nDataCh, IFX_TRUE);
               
      /* Flag peer was removed */
      peer_removed = IFX_TRUE;

      pConf->nLocalPeersCnt--;
      if (0 == pConf->nLocalPeersCnt)
      {
         pPhone->fLocalPeerCalled = IFX_FALSE;
      }
   } /* if */
   else if (EXTERN_VOIP_CALL == conn->fType)
   {
      /* REMOTE */

      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
           ("Removing called external party %s:%d from "
           "conference, used data ch %d for calling.\n",
           inet_ntoa(conn->oConnPeer.oRemote.oToAddr.sin_addr),
           ntohs(conn->oConnPeer.oRemote.oToAddr.sin_port),
           (int) conn->nUsedCh));

      /* Unmap everything from this data channel */
      ret = CONFERENCE_RestoreMapping(pConf, conn->nUsedCh,
                                      DESTINATION_CH, DATA_CH);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error restoring mapping for data ch %d. \
              (File: %s, line: %d)\n", 
              (int) conn->nUsedCh, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Stop codec on this data channel connected to remote peer */
      VOIP_StopCodec(conn->nUsedCh);

      /* Flag peer was removed */
      peer_removed = IFX_TRUE;

      pConf->nExternalPeersCnt--;
      if (0 == pConf->nExternalPeersCnt)
      {
         pPhone->fExtPeerCalled = IFX_FALSE;
      }
   } /* else if */
   else if (PCM_CALL == conn->fType)
   {
      /* PCM */

      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
           ("Removing called pcm party from conference, pcm ch %d\n",
           (int) conn->nUsedCh));

      /* Unmap teverything from this pcm channel */
      ret = CONFERENCE_RestoreMapping(pConf,
                                      conn->nUsedCh,
                                      DESTINATION_CH, PCM_CH);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Error restoring mapping for pcm ch %d. \
              (File: %s, line: %d)\n", 
              (int) conn->nUsedCh, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      PCM_EndConnection(pCtrl->pProgramArg, conn->nUsedCh,
                        conn->oConnPeer.oPCM.nCh);

      /* Flag peer was removed */
      peer_removed = IFX_TRUE;

      pConf->nPCM_PeersCnt--;
      if (0 == pConf->nPCM_PeersCnt)
      {
         pPhone->fPCM_PeerCalled = IFX_FALSE;
      }
   } /* if */

   if (IFX_TRUE == peer_removed)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
           ("Peer removed, now remove phone connection AND peer from "
            "conference\n"));

      /* Remove this connection from array. Move last one to this location 
         and clear last one, if we are not the last one. */
      if (0 < pPhone->nConnCnt)
      {
         if (conn != &pPhone->rgoConn[(pPhone->nConnCnt - 1)])
         {
            memcpy(conn,
                   &pPhone->rgoConn[pPhone->nConnCnt - 1],
                   sizeof(CONNECTION_t));
         }
         memset(&pPhone->rgoConn[pPhone->nConnCnt - 1],
                0, sizeof(CONNECTION_t));

         /* Status we have one less peer */
         pPhone->nConnCnt--;
      }

      if (0 < pCtrl->rgoConferences[pPhone->nConfIdx - 1].nPeersCnt)
      {
         pCtrl->rgoConferences[pPhone->nConfIdx - 1].nPeersCnt--;
      }

      /* Put in status talking, because there are still some
         phones left in conference. */
      pPhone->nStatus = US_ACTIVE_TX;
   }

   if (MIN_PARTY_CONFERENCE > pPhone->nConnCnt)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
           ("Conference has gone, only two phones in connection.\n"));

      /* Only two phones are talking, no conference */
      pCtrl->rgoConferences[pPhone->nConfIdx].fActive = IFX_FALSE;
      pPhone->nConfIdx = NO_CONFERENCE;
      pCtrl->nConfCnt--;
      conn = &pPhone->rgoConn[0];

      /* If this peer is local, remove mapping and start codec. */
      if (LOCAL_CALL == conn->fType)
      {
         /* Remove all remain mappings. */
         ret = CONFERENCE_RestoreMapping(pConf, RESTORE_ALL_CH_MAPPING,
                                         ALL_CH, ALL_CH_TYPE);

         if (IFX_ERROR == ret)
         {
            TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
                 ("Error restoring mapping for data ch %d, phone ch %d. \
                 (File: %s, line: %d)\n", 
                 (int) conn->oConnPeer.oLocal.pPhone->nDataCh,
                 (int) conn->oConnPeer.oLocal.pPhone->nPhoneCh,
                 __FILE__, __LINE__));
            return IFX_ERROR;
         }

         /* Also check if codec for initiator is stopped, if YES start it.
            Codec of initator can be started if we had conference only 
            with local or pcm phones. */
         VOIP_SetCodecFlag(conn->nUsedCh, IFX_TRUE);
         VOIP_StartCodec(conn->nUsedCh);

         /* Also start codec for called phone */
         VOIP_SetCodecFlag(conn->oConnPeer.oLocal.pPhone->nDataCh, IFX_TRUE);
         VOIP_StartCodec(conn->oConnPeer.oLocal.pPhone->nDataCh);
      }
      else if (EXTERN_VOIP_CALL == conn->fType) 
      {
         /* Only one peer remain and its external. */
      }
      else if (PCM_CALL == conn->fType) 
      {
         /* Only one PCM peer remain. */

         /* Also stop codec for initiator if started. */
         VOIP_StopCodec(conn->nUsedCh);
      }

      /* Clear conference content. */
      memset(pConf, 0, sizeof(CONFERENCE_t));
   } /* if */

   /* pConn has changed, because one peer, connection was removed. */
   *pConn = conn;

   return ret;
} /* CONFERENCE_RemovePeer() */


/**
   Clear everything after conference is finished.

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pConf  - pointer to conference structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t CONFERENCE_End(CTRL_STATUS_t* pCtrl,
                            PHONE_t* pPhone,
                            CONFERENCE_t* pConf)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_uint32_t conf_idx = 0;
   CONNECTION_t* conn = IFX_NULL;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone) || (IFX_NULL == pConf))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Clear structure, because master phone has ended conference */
   TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Clearing conference phones\n"));

   /* Only unmap channels if conference was made, otherwise an error will
      occur */
   /* Unmap/map channels back */
   ret = CONFERENCE_RestoreMapping(pConf, RESTORE_ALL_CH_MAPPING,
                                   ALL_CH, ALL_CH_TYPE);
   if (IFX_ERROR == ret)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Error restoring mapping (all chanels). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   pPhone->nConnCnt--;
   if (0 > pPhone->nConnCnt)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Peer index less than ZERO! (File: %s, line: %d)\n", 
            __FILE__, __LINE__));

      pPhone->nConnCnt = 0;
   }

   do
   {
      conn = &pPhone->rgoConn[pPhone->nConnCnt];
      /* We started conference so end it */
      if (IFX_TRUE == conn->fActive)
      {
         /* Send all phones which are in active connection BUSY */
         ABC_PBX_SetAction(pCtrl, pPhone, conn, US_BUSY);
      }
      else
      {
         /* Send all phones which are called (last phone ringing, ...) READY */
         /* Send calling phone (is ringing) to stop ringing */
         ABC_PBX_SetAction(pCtrl, pPhone, conn, US_READY);

         TRACE(TAPIDEMO, DBG_LEVEL_LOW,
               ("Stop ringback tone on data ch %d\n", (int) conn->nUsedCh_FD));

         /* Stop ringing also on this channel */
         ioctl(conn->nUsedCh_FD, IFX_TAPI_TONE_LOCAL_PLAY, NO_PARAM);
      }
      /* Restore some data */
      if (LOCAL_CALL == conn->fType)
      {
         VOIP_SetCodecFlag(conn->oConnPeer.oLocal.pPhone->nDataCh, IFX_TRUE);
         conn->oConnPeer.oLocal.pPhone->nConfIdx = NO_CONFERENCE;
         conn->oConnPeer.oLocal.pPhone->nConnCnt = 0;
      }
      else if (PCM_CALL == conn->fType)
      {
         PCM_FreeCh(conn->nUsedCh);

         PCM_EndConnection(pCtrl->pProgramArg, conn->nUsedCh,
                           conn->oConnPeer.oPCM.nCh);
      }
      else if (EXTERN_VOIP_CALL == conn->fType)
      {
         VOIP_FreeDataCh(conn->nUsedCh);

         /* Stop coder */
         VOIP_StopCodec(conn->nUsedCh);

         /* QoS Support */
         if (pCtrl->pProgramArg->oArgFlags.nQos)
         {
            /* QoS Support */
            QOS_StopSession(pCtrl, conn);
         }

         /* TAPI SIGNAL SUPPORT */
         TAPI_SIG_ResetTxSigHandler(conn);
      }
      else
      {
         /* Unknown call */
      }

      /* Remove phone connection. */
      memset(conn, 0, sizeof(CONNECTION_t));
      pPhone->nConnCnt--;
   }
   while (0 <= pPhone->nConnCnt);

   conf_idx = pPhone->nConfIdx;
   pPhone->nConfIdx = NO_CONFERENCE;
   pPhone->nConnCnt = 0;
   pPhone->fPCM_PeerCalled = IFX_FALSE;
   pPhone->fExtPeerCalled = IFX_FALSE;
   pPhone->fLocalPeerCalled = IFX_FALSE;

   /* Also clear conference content. */
   memset(pConf, 0, sizeof(CONFERENCE_t));
   
   pCtrl->nConfCnt--;

   return ret;
} /* CONFERENCE_End() */
