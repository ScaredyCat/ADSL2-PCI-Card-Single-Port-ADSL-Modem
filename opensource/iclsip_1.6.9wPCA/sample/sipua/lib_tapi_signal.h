/*******************************************************************************
                  Copyright (c) 2006  Infineon Technologies AG
                     Am Campeon 1-12; 81726 Munich, Germany

                               No Warranty
   Because the program is licensed free of charge, there is no warranty for
   the program, to the extent permitted by applicable law.  Except when
   otherwise stated in writing the copyright holders and/or other parties
   provide the program "as is" without warranty of any kind, either
   expressed or implied, including, but not limited to, the implied
   warranties of merchantability and fitness for a particular purpose. The
   entire risk as to the quality and performance of the program is with
   you.  should the program prove defective, you assume the cost of all
   necessary servicing, repair or correction.

   In no event unless required by applicable law or agreed to in writing
   will any copyright holder, or any other party who may modify and/or
   redistribute the program as permitted above, be liable to you for
   damages, including any general, special, incidental or consequential
   damages arising out of the use or inability to use the program
   (including but not limited to loss of data or data being rendered
   inaccurate or losses sustained by you or third parties or a failure of
   the program to operate with any other programs), even if such holder or
   other party has been advised of the possibility of such damages.
********************************************************************************
   Module      : lib_tapi_signal.h
   Desription  : TAPI signal handling.
*******************************************************************************/
#ifndef _LIB_TAPI_SIGNAL_H
#define _LIB_TAPI_SIGNAL_H


/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/** custom defined signals, just used by us */
enum
{
   TAPI_SIGNAL_CALLER = 1,
   TAPI_SIGNAL_CALLEE = 0,
   TAPI_SIGNAL_TONEHOLDING = 0xFFFFFFFD,
   TAPI_SIGNAL_TIMEOUT = 0xFFFFFFFE
};


/* ============================= */
/* Global function declaration   */
/* ============================= */

extern IFX_return_t SetSignal(IFX_int32_t fd, IFX_int32_t signal);
extern IFX_char_t* SignalToName(IFX_int32_t signal);
extern IFX_return_t HandleSignal(IFX_int32_t fd,
                                IFX_int32_t Direction,
                                IFX_int32_t *pCurrentSignal,
                                IFX_int32_t NewSignal,
                                IFX_int32_t *pTimeout);
extern IFX_return_t TxSignalSetup(IFX_int32_t fd);
extern IFX_return_t RxSignalSetup(IFX_int32_t fd);
extern IFX_return_t TxSignalHandlerReset(IFX_int32_t fd,
                                         IFX_int32_t *pCurrentSignal, 
                                         IFX_int32_t *pTimeout);
extern IFX_return_t RxSignalHandlerReset(IFX_int32_t fd, 
                                         IFX_int32_t *pCurrentSignal, 
                                         IFX_int32_t *pTimeout);

#endif /* _LIB_TAPI_SIGNAL_H */
