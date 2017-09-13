#ifndef _CONFERENCE_H
#define _CONFERENCE_H
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
   Module      : conference.h
   Date        : 2005-12-01
   Description : This file enchance tapi demo with conferencing.
   \file 

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "abstract.h"

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */


/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t CONFERENCE_PutOnHold(IFX_void_t);
IFX_return_t CONFERENCE_ResumeOnHold(IFX_void_t);
IFX_return_t CONFERENCE_MapPhoneToPhone(IFX_int32_t nPhoneCh,
                                        IFX_int32_t nAddPhoneCh,
                                        IFX_boolean_t fDoMapping,
                                        CONFERENCE_t *pConf);
STATE_MACHINE_STATES_t CONFERENCE_Start(CTRL_STATUS_t* pCtrl,
                                        PHONE_t* pPhone);
IFX_return_t CONFERENCE_RestoreMapping(CONFERENCE_t *pConf,
                                       IFX_int32_t nChNum,
                                       WHICH_CH_CHECK_t nWhichChCheck,
                                       CH_TYPE_CHECK_t nChType);
IFX_return_t CONFERENCE_GetNewPeer(CTRL_STATUS_t* pCtrl,
                                   PHONE_t* pPhone);
IFX_return_t CONFERENCE_AddLocalPeer(CTRL_STATUS_t *pCtrl,
                                     PHONE_t *pPhone,
                                     PHONE_t *pDstPhone,
                                     CONNECTION_t* pConn);
IFX_return_t CONFERENCE_AddExternalPeer(CTRL_STATUS_t *pCtrl,
                                        PHONE_t *pPhone,
                                        CONNECTION_t* pConn);
IFX_return_t CONFERENCE_AddPCM_Peer(CTRL_STATUS_t *pCtrl,
                                    PHONE_t *pPhone,
                                    CONNECTION_t* pConn);
IFX_return_t CONFERENCE_RemovePeer(CTRL_STATUS_t *pCtrl,
                                   PHONE_t *pPhone,
                                   CONNECTION_t** pConn,
                                   CONFERENCE_t *pConf);
IFX_return_t CONFERENCE_End(CTRL_STATUS_t *pCtrl,
                            PHONE_t *pPhone,
                            CONFERENCE_t *pConf);

#endif /* _CONFERENCE_H */
