#ifndef _QOS_H
#define _QOS_H
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
   Module      : qos.h
   Date        : 2005-11-29
   Description : This file enhance tapi demo with quality of service
   \file 

   \remarks
      
   \note Changes: Only supported in LINUX at the moment.
                  Look drv_qos.h for further info.
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

/* ============================= */
/* Global Defines                */
/* ============================= */

/** UDP port to use for QOS */
enum
{
   UDP_QOS_PORT = 6000
};

typedef enum
{
   NO_ACTION = 0,
   /** Setup port address */
   SET_ADDRESS_PORT = 1
} QOS_ACTION_t;

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

IFX_void_t QOS_TurnServiceOn(PROGRAM_ARG_t *pProgramArg);
IFX_void_t QOS_InitializePairStruct(IFX_int32_t nPhoneNumber,
                                    CTRL_STATUS_t* pCtrl, 
                                    CONNECTION_t* pConn);
IFX_void_t QOS_CleanUpSession(CTRL_STATUS_t* pCtrl, PROGRAM_ARG_t *pProgramArg);
IFX_return_t QOS_HandleService(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn);
IFX_void_t QOS_StartSession(CTRL_STATUS_t* pCtrl,
                            CONNECTION_t* pConn, 
                            QOS_ACTION_t nAction);
IFX_void_t QOS_StopSession(CTRL_STATUS_t* pCtrl, CONNECTION_t* pConn);


#endif /* _QOS_H */
