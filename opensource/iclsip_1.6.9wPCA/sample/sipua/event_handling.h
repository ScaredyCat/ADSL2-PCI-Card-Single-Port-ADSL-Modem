#ifndef _EVENT_HANDLING_H
#define _EVENT_HANDLING_H
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
   Module      : event_handling_bitfields.h
   Date        : 2006-03-28
   Description : This file represents event handling in old way
                 (using bitfields)
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
   CHECK_ALL_CH = -1
};

/* ============================= */
/* Global Variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t EVENT_Check(IFX_int32_t nDevCtrl_FD,
                         IFX_int32_t nDataCh);
IFX_boolean_t EVENT_Exists(IFX_int32_t nDataCh);
STATE_MACHINE_STATES_t EVENT_Handle(PHONE_t *pPhone,
                                    CTRL_STATUS_t* pCtrl,
                                    IFX_int32_t nDevIdx);
IFX_boolean_t EVENT_DTMF_Dialing(IFX_int32_t nDataCh);

#endif /* _EVENT_HANDLING_H */
