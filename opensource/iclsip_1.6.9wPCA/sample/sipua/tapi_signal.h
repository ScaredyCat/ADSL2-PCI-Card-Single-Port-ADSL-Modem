#ifndef _TAPI_SIGNAL_H
#define _TAPI_SIGNAL_H
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
   Module      : tapi_signal.h
   Date        : 2005-11-29
   Description : This file contains the implementation of the functions for
                 the tapi demo working with tapi signalling.
   \file 

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "lib_tapi_signal.h"

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

struct timeval* TAPI_SIG_SetTimer(IFX_void_t);
IFX_return_t TAPI_SIG_CheckTimeOut(CONNECTION_t* pConn,
                                   IFX_int32_t nStatus);
IFX_return_t TAPI_SIG_SigHandling(CONNECTION_t* pConn,
                                  IFX_int32_t nStatus,
                                  IFX_TAPI_CH_STATUS_t *pStatus);
IFX_void_t TAPI_SIG_SetPhoneRinging(IFX_int32_t nPhoneCh_FD);
IFX_return_t TAPI_SIG_TxSigConfig(IFX_int32_t nDataCh_FD);
IFX_return_t TAPI_SIG_RxSigConfig(IFX_int32_t nDataCh_FD);
IFX_return_t TAPI_SIG_ResetTxSigHandler(CONNECTION_t* pConn);
IFX_return_t TAPI_SIG_ResetRxSigHandler(CONNECTION_t* pConn);

   
#endif /** _TAPI_SIGNAL_H */
