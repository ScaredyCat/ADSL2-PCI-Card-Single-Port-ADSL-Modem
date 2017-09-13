#ifndef _ABSTRACT_H
#define _ABSTRACT_H
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
   Date        : 2005-11-30
   Description : This file makes abstraction.
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

/** represents startup mapping table */
typedef struct _STARTUP_MAP_TABLE_t
{
   /** Phone channel, index */
   IFX_int32_t nPhoneCh;
   
   /** Connected data channel, just index, if -1 no connection to data
      channel */
   IFX_int32_t nDataCh;

   /** Connected PCM channel, just index, if -1 no connection to PCM
       channel.
       NOTICE: PCM channel is not mapped at start, but when PCM call
               is started.  */
   IFX_int32_t nPCM_Ch;

} STARTUP_MAP_TABLE_t;


/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

const STARTUP_MAP_TABLE_t* ABSTRACT_GetStartupMapTbl(IFX_void_t);
PHONE_t* ABSTRACT_GetPHONE_OfPhoneCh(CTRL_STATUS_t *pCtrl,
                                     IFX_int32_t nPhoneCh);
PHONE_t* ABSTRACT_GetPHONE_OfDataCh(CTRL_STATUS_t *pCtrl,
                                    IFX_int32_t nDataCh,
                                    CONNECTION_t** pConn);
IFX_int32_t ABSTRACT_GetDEV_OfDataCh(CTRL_STATUS_t *pCtrl,
                                     IFX_int32_t nDataCh);
IFX_int32_t ABSTRACT_GetDEV_OfPhoneCh(CTRL_STATUS_t *pCtrl,
                                      IFX_int32_t nPhoneCh);
IFX_boolean_t ABSTRACT_EventsFromPhoneCh(IFX_int32_t nDevIdx,
                                         IFX_boolean_t fDTMF_Dialing);
PHONE_t* ABSTRACT_GetPHONE_OfSocket(CTRL_STATUS_t *pCtrl,
                                    IFX_int32_t nSocket,
                                    CONNECTION_t** pConn);
IFX_return_t ABSTRACT_UnmapDefaults(CTRL_STATUS_t *pCtrl);
IFX_return_t ABSTRACT_DefaultMapping(CTRL_STATUS_t *pCtrl);
PHONE_t* ABSTRACT_GetPHONE_Of_PCM_Socket(CTRL_STATUS_t *pCtrl,
                                         IFX_int32_t nPCM_Socket,
                                         CONNECTION_t** pConn);

#endif /* _ABSTRACT_H */
