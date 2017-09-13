#ifndef _CID_H
#define _CID_H
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
   Module      : cid.h
   Date        : 2005-11-30
   Description : This file enchance tapi demo with CID.
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

enum
{
   /** Number of CID elements, we use CLI number, name, date */
   CID_ELEM_COUNT  = 3,
   /** Index of element location */
   CID_IDX_DATE    = 0,
   CID_IDX_CLI_NUM = 1,
   CID_IDX_NAME    = 2
};

enum 
{
   /** Maximum number of users */ 
   CID_MAX_USERS = 4,
   /** Maximum number of standards */
   CID_MAX_STANDARDS = 5
};

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t CID_SetupData(IFX_uint32_t nIdx, PHONE_t* pPhone);
IFX_void_t CID_Display(PHONE_t* pPhone);
IFX_return_t CID_SetStandard(IFX_int32_t nCID_Standard);
IFX_TAPI_CID_STD_t CID_GetStandard(IFX_void_t);
IFX_return_t CID_ConfDriver(IFX_uint32_t nDataCh_FD);
IFX_return_t CID_Send(IFX_uint32_t nDataCh_FD,
                      IFX_uint8_t nHookState,
                      IFX_TAPI_CID_MSG_t *pCID_Msg);

#endif /* _CID_H */
