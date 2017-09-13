#ifndef _PCM_H
#define _PCM_H
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
   Module      : pcm.h
   Date        : 2006-03-29
   Description : This file enchance tapi demo with PCM support.
   \file 

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/** Flag for no pcm channel */
enum { NO_FREE_PCM_CH = -1 };

/** Flag for no timeslot */
enum { NO_FREE_TIMESLOT = -1 };

/* ============================= */
/* Global Variable definition    */
/* ============================= */

extern IFX_boolean_t testek;

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t PCM_Init(PROGRAM_ARG_t* pProgramArg);
IFX_boolean_t PCM_SetActivation(IFX_int32_t nDataCh_FD,
                               IFX_boolean_t fMaster,
                               IFX_boolean_t fActivate,
                               IFX_int32_t nTimeSlotIdx);
IFX_int32_t PCM_InitSocket(PROGRAM_ARG_t* pProgramArg,
                           IFX_int32_t nChIdx);
IFX_int32_t PCM_GetSocket(IFX_int32_t nChIdx);
IFX_return_t PCM_HandleSocketData(CTRL_STATUS_t* pCtrl,
                                  PHONE_t* pPhone,
                                  CONNECTION_t* pConn,
                                  IFX_int32_t nCID_NameIdx);
IFX_boolean_t PCM_CheckPhoneNum(IFX_char_t* prgnDialNum,
                                IFX_int32_t nDialNrCnt);
IFX_boolean_t PCM_StartConnection(PROGRAM_ARG_t* pProgramArg,
                                  IFX_int32_t nPCM_Ch,
                                  IFX_int32_t nTimeSlotIdx);
IFX_boolean_t PCM_EndConnection(PROGRAM_ARG_t* pProgramArg,
                                IFX_int32_t nPCM_Ch,
                                IFX_int32_t nTimeSlotIdx);
IFX_return_t PCM_MapToPhone(IFX_int32_t nDstCh,
                            IFX_int32_t nAddCh,
                            IFX_boolean_t fDoMapping,
                            CONFERENCE_t* pConf);
IFX_return_t PCM_MapToPCM(IFX_int32_t nDstCh,
                          IFX_int32_t nAddCh,
                          IFX_boolean_t fDoMapping,
                          CONFERENCE_t* pConf);
IFX_return_t PCM_MapToData(IFX_int32_t nDstCh,
                            IFX_int32_t nAddCh,
                            IFX_boolean_t fDoMapping,
                            CONFERENCE_t* pConf);
IFX_return_t PCM_FreeCh(IFX_int32_t nPCM_Ch);
IFX_return_t PCM_ReserveCh(IFX_int32_t nPCM_Ch);
IFX_int32_t PCM_GetFreeCh(IFX_void_t);
IFX_int32_t PCM_GetFD_OfCh(IFX_int32_t nPCM_Ch);


#endif /* _PCM_H */
