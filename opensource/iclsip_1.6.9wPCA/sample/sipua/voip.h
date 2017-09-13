#ifndef _VOIP_H
#define _VOIP_H
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
   Module      : voip.h
   Date        : 2006-04-04
   Description : This file enchance tapi demo with VOIP support.
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

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t VOIP_Init(PROGRAM_ARG_t* pProgramArg);
IFX_return_t VOIP_SetEncoderType(IFX_int32_t nEncoderType);
IFX_return_t VOIP_SetFrameLen(IFX_int32_t nFrameLen);
extern IFX_return_t VOIP_StopCodec(IFX_int32_t nDataCh);
extern IFX_return_t VOIP_StartCodec(IFX_int32_t nDataCh);
extern IFX_int32_t VOIP_GetFreeDataCh(IFX_void_t);
extern IFX_return_t VOIP_MapPhoneToData(IFX_int32_t nDataCh,
                                        IFX_int32_t nAddPhoneCh,
                                        IFX_boolean_t fDoMapping,
                                        CONFERENCE_t* pConf);
IFX_return_t VOIP_ReserveDataCh(IFX_int32_t nDataCh);
IFX_return_t VOIP_FreeDataCh(IFX_int32_t nDataCh);
IFX_int32_t VOIP_GetSocket(IFX_int32_t nChIdx);
IFX_int32_t VOIP_InitUdpSocket(PROGRAM_ARG_t* pProgramArg,
                               IFX_int32_t nChIdx);
IFX_return_t VOIP_HandleSocketData(CTRL_STATUS_t* pCtrl,
                                   PHONE_t* pPhone,
                                   CONNECTION_t* pConn,
                                   IFX_boolean_t fHandleData,
                                   IFX_int32_t nCID_NameIdx);
IFX_return_t VOIP_HandleData(CTRL_STATUS_t* pCtrl,
                             PHONE_t* pPhone,
                             IFX_int32_t nDataCh_FD,
                             CONNECTION_t* pConn,
                             IFX_boolean_t fHandleData);
IFX_void_t VOIP_SetCodecFlag(IFX_int32_t nDataCh, IFX_boolean_t fStartCodec);
IFX_int32_t VOIP_GetFD_OfCh(IFX_int32_t nDataCh);

#endif /* _VOIP_H */
