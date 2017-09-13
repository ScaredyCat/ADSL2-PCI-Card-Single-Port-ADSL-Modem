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
   Module      : lib_tapi_signal.c
   Desription  : TAPI Library for tone signal handling.
*******************************************************************************/


/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef VXWORKS
#include "vxworks.h"
#include <vxWorks.h>
#include <stdio.h>
#include <stdlib.h>
#include <ioLib.h>
#include <sys/ioctl.h>
#include <logLib.h>
#include <string.h>
#endif /* VXWORKS */

#include "ifx_types.h"

#ifdef TAPI_V21
#ifdef TAPI_SIGNAL_SUPPORT

#include "lib_tapi_signal.h"
#include "tapidemo.h"


/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Array used to assign names to the signal values reported by the TAPI */
static IFX_char_t* Signals[] =
{
   "IFX_TAPI_SIG_NONE"                  /* 0x00 */,
   "IFX_TAPI_SIG_DISRX"                 /* 0x1 */ ,
   "IFX_TAPI_SIG_DISTX"                 /* 0x2 */ ,
   "IFX_TAPI_SIG_DIS"                   /* 0x4 */ ,
   "IFX_TAPI_SIG_DISMASK"               /* IFX_TAPI_SIG_DIS |
                                          IFX_TAPI_SIG_DISRX |
                                          IFX_TAPI_SIG_DISTX */,
   "IFX_TAPI_SIG_CEDRX"                 /* 0x8 */,
   "IFX_TAPI_SIG_CEDTX"                 /* 0x10 */,
   "IFX_TAPI_SIG_CED"                   /* 0x20 */,
   "IFX_TAPI_SIG_CEDMASK"               /* IFX_TAPI_SIG_CED |
                                          IFX_TAPI_SIG_CEDTX |
                                          IFX_TAPI_SIG_CEDRX */,
   "IFX_TAPI_SIG_CNGFAXRX"              /* 0x40 */,
   "IFX_TAPI_SIG_CNGFAXTX"              /* 0x80 */,
   "IFX_TAPI_SIG_CNGFAX"                /* 0x100 */,
   "IFX_TAPI_SIG_CNGFAXMASK"            /* IFX_TAPI_SIG_CNGFAX |
                                          IFX_TAPI_SIG_CNGFAXRX |
                                          IFX_TAPI_SIG_CNGFAXTX */,
   "IFX_TAPI_SIG_CNGMODRX"              /* 0x200 */,
   "IFX_TAPI_SIG_CNGMODTX"              /* 0x400 */,
   "IFX_TAPI_SIG_CNGMOD"                /* 0x800 */,
   "IFX_TAPI_SIG_CNGMODMASK"            /* IFX_TAPI_SIG_CNGMOD |
                                          IFX_TAPI_SIG_CNGMODRX |
                                          IFX_TAPI_SIG_CNGMODTX */,
   "IFX_TAPI_SIG_PHASEREVRX"            /* 0x1000 */,
   "IFX_TAPI_SIG_PHASEREVTX"            /* 0x2000 */,
   "IFX_TAPI_SIG_PHASEREV"              /* 0x4000 */,
   "IFX_TAPI_SIG_PHASEREVMASK"          /* IFX_TAPI_SIG_PHASEREV |
                                          IFX_TAPI_SIG_PHASEREVRX |
                                          IFX_TAPI_SIG_PHASEREVTX */,
   "IFX_TAPI_SIG_AMRX"                  /* 0x8000 */,
   "IFX_TAPI_SIG_AMTX"                  /* 0x10000 */,
   "IFX_TAPI_SIG_AM"                    /* 0x20000 */,
   "IFX_TAPI_SIG_AMMASK"                /* IFX_TAPI_SIG_AM |
                                          IFX_TAPI_SIG_AMRX |
                                          IFX_TAPI_SIG_AMTX */,
   "IFX_TAPI_SIG_TONEHOLDING_ENDRX"     /* 0x40000 */,
   "IFX_TAPI_SIG_TONEHOLDING_ENDTX"     /* 0x80000 */,
   "IFX_TAPI_SIG_TONEHOLDING_END"       /* 0x100000 */,
   "IFX_TAPI_SIG_TONEHOLDING_ENDMASK"   /* IFX_TAPI_SIG_TONEHOLDING_END |
                                          IFX_TAPI_SIG_TONEHOLDING_ENDRX |
                                          IFX_TAPI_SIG_TONEHOLDING_ENDTX */,
   "IFX_TAPI_SIG_CEDENDRX"              /* 0x200000 */,
   "IFX_TAPI_SIG_CEDENDTX"              /* 0x400000 */,
   "IFX_TAPI_SIG_CEDEND"                /* 0x800000 */,
   "IFX_TAPI_SIG_CEDENDMASK"            /* IFX_TAPI_SIG_CEDEND |
                                          IFX_TAPI_SIG_CEDENDRX |
                                          IFX_TAPI_SIG_CEDENDTX */,
   "IFX_TAPI_SIG_TONEHOLDING"           /* 0xFFFFFFFD */,
   "IFX_TAPI_SIG_TIMEOUT"               /* 0xFFFFFFFE */ ,
   /* special signals for multiple signals or'd in one call */
   "IFX_TAPI_SIG_CEDTX | CEDENDTX | PHASEREV | AM",
   "IFX_TAPI_SIG_CEDRX | CEDENDRX | PHASEREV | AM",
   /* Signals a call progress tone detection. This signal is enabled with
       the interface \ref IFX_TAPI_TONE_CPTD_START and stopped with
       \ref IFX_TAPI_TONE_CPTD_STOP. It can not be activate with
       \ref IFX_TAPI_SIG_DETECT_ENABLE */
   "IFX_TAPI_SIG_CPTD"              /* 0x1000000 */,
   /* Signals the V8bis detection on the receive path */
   "IFX_TAPI_SIG_V8BISRX"           /* 0x2000000 */,
   /* Signals the V8bis detection on the transmit path */
   "IFX_TAPI_SIG_V8BISTX"           /* 0x4000000 */,
   /* Signals that the Caller-ID transmission has finished */
   "IFX_TAPI_SIG_CIDENDTX"          /* 0x8000000 */,
   "Unknown Signal"
};


/* ============================= */
/* Local function definition     */
/* ============================= */

/**
   Function to enable the detection of a signal and report via TAPI exception.

   \param   fd      -  filedescriptor to tapi channel
   \param   signal  -  signal to enable

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
   if multiple signals should be enabled, this function has to be called
   separately for each signal.
*/
IFX_return_t EnableSignalDetection(IFX_int32_t fd, IFX_int32_t signal)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_SIG_DETECTION_t sigdetection;


   TRACE(TAPIDEMO, DBG_LEVEL_LOW,
          ("EnableSignalDetection for %s on fd %d\n",
            SignalToName(signal), (int) fd));

   sigdetection.sig = signal;

   ret = ioctl(fd, IFX_TAPI_SIG_DETECT_ENABLE, (IFX_int32_t) &sigdetection);
   if (IFX_SUCCESS != ret)
   {
      IFX_int32_t lasterr = 0;

      ioctl(fd, FIO_VINETIC_LASTERR, (IFX_int32_t) &lasterr);

      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
             ("EnableSignalDetection() for %s failed (lasterr: 0x%x)\n",
               SignalToName(signal), (int) lasterr));
   }

   return ret;
} /* EnableSignalDetection() */


/**
   Function to disable the detection of a signal

   \param   fd      -  filedescriptor to tapi channel
   \param   signal  -  signal to disable

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
   if multiple signals should be disabled, this function has to be called
   separately for each signal.
*/
IFX_return_t DisableSignalDetection(IFX_int32_t fd, IFX_int32_t signal)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_SIG_DETECTION_t sigdetection;


   TRACE(TAPIDEMO, DBG_LEVEL_LOW, 
         ("DisableSignalDetection() for %s on fd %d\n",
           SignalToName(signal), (int) fd));
   sigdetection.sig = signal;

   ret = ioctl(fd, IFX_TAPI_SIG_DETECT_DISABLE, (IFX_int32_t) &sigdetection);
   if (IFX_SUCCESS != ret)
   {
      IFX_int32_t lasterr = 0;

      ioctl(fd, FIO_VINETIC_LASTERR, (IFX_int32_t) &lasterr);

      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("DisableSignalDetection for %s failed (lasterror: 0x%x)\n",
             SignalToName(signal), (int) lasterr));
   }

   return (ret);
} /* DisableSignalDetection() */ 


/**
   Function to switch a transmission channel to a uncompressed codec.

   \param   fd      -  filedescriptor to tapi channel

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
   This function switches a transmission channel to G711 coded, sets
   a fixed jitter buffer, disables silence suppresion and comfort noise
   generation. Further event transmission and auto suppresion are turned off.
*/
IFX_return_t ClearChannel(IFX_int32_t fd)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_JB_CFG_t tapi_jb_conf;


   TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Clear Channel: fd %d\n", (int) fd));
   /** \todo fixme: Save current configuration ? */

   /* switch to G.711 */
   ret = ioctl(fd, IFX_TAPI_ENC_TYPE_SET, IFX_TAPI_ENC_TYPE_MLAW);
   if (IFX_SUCCESS == ret)
   {
      /* set fixed jitter buffer */
      memset(&tapi_jb_conf, 0x00, sizeof(IFX_TAPI_JB_CFG_t));
      tapi_jb_conf.nJbType      =  IFX_TAPI_JB_TYPE_FIXED;;
      tapi_jb_conf.nPckAdpt     =  IFX_TAPI_JB_PKT_ADAPT_DATA;
      tapi_jb_conf.nLocalAdpt   =  IFX_TAPI_JB_LOCAL_ADAPT_OFF;
      tapi_jb_conf.nScaling     =  8;   /* don't care */
      tapi_jb_conf.nInitialSize =  400; /* 50 ms * 8 */
      tapi_jb_conf.nMaxSize     =  400; /* don't care */
      tapi_jb_conf.nMinSize     =  400; /* don't care */

      ret = ioctl(fd, IFX_TAPI_JB_CFG_SET, (IFX_int32_t) &tapi_jb_conf);
   }

   if (IFX_SUCCESS == ret)
   {
      /* disable silence suppression and confort noise generation */
      ret = ioctl(fd, IFX_TAPI_ENC_VAD_CFG_SET, IFX_TAPI_ENC_VAD_NOVAD);
   } 

   if (IFX_SUCCESS == ret)
   {
      /* event transmission off, auto suppression off -> dtmf only in-band */
      ret = ioctl(fd, IFX_TAPI_PKT_EV_OOB_SET, IFX_TAPI_PKT_EV_OOB_ONLY);
   }

   if (IFX_SUCCESS != ret)
   {
      IFX_int32_t lasterr = 0;

      ioctl(fd, FIO_VINETIC_LASTERR, (IFX_int32_t) &lasterr);
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
             ("Clear Channel failed: Last error: 0x%x\n", (int) lasterr));
   } 

   return ret;
} /* ClearChannel() */


/**
   Function to disable the non-linear-processor of the VINETIC

   \param   fd -  filedescriptor to tapi channel

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
*/
IFX_return_t DisableNLP(IFX_int32_t fd)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_LEC_CFG_t tapi_lec_conf;


   memset(&tapi_lec_conf, 0x00, sizeof(IFX_TAPI_LEC_CFG_t));

   ret = ioctl(fd, IFX_TAPI_LEC_PHONE_CFG_GET, (IFX_int32_t) &tapi_lec_conf);

   if (IFX_SUCCESS == ret)
   {
      tapi_lec_conf.nLen = IFX_TAPI_LEC_LEN_MAX;
      tapi_lec_conf.bNlp = IFX_TAPI_LEC_NLP_OFF;
      ret = ioctl(fd, IFX_TAPI_LEC_PHONE_CFG_SET, (IFX_int32_t) &tapi_lec_conf);
   } 

   if (IFX_SUCCESS != ret)
   {
      IFX_int32_t lasterr;

      ioctl(fd, FIO_VINETIC_LASTERR, &lasterr);
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("DisableNLP failed. Last error: 0x%x\n", (int) lasterr));
   }

   return ret;
} /* DisableNLP() */


/**
   Function to disable the line echo canceler of the VINETIC

   \param   fd      -  filedescriptor to tapi channel

   \return  status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t DisableLEC(IFX_int32_t fd)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_LEC_CFG_t tapi_lec_conf;


   memset(&tapi_lec_conf, 0x00, sizeof(IFX_TAPI_LEC_CFG_t));

   ret = ioctl(fd, IFX_TAPI_LEC_PHONE_CFG_GET, (IFX_int32_t) &tapi_lec_conf);

   if (IFX_SUCCESS == ret)
   {
      tapi_lec_conf.nGainIn = IFX_TAPI_LEC_GAIN_OFF;
      tapi_lec_conf.nGainOut = IFX_TAPI_LEC_GAIN_OFF;
      ret = ioctl(fd, IFX_TAPI_LEC_PHONE_CFG_SET, (IFX_int32_t) &tapi_lec_conf);
   } 

   if (IFX_SUCCESS != ret)
   {
      IFX_int32_t lasterr;

      ioctl(fd, FIO_VINETIC_LASTERR, &lasterr);
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
            ("Disable LEC failed: Last error: 0x%x\n", (int) lasterr));
   }

   return (ret);
} /* DisableLEC() */


/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Helper function to display the name assigned to a signal value

   \param   nSignal  -  signal value

   \return  pointer to corresponding signal name string
*/
IFX_char_t* SignalToName(IFX_int32_t nSignal)
{
   IFX_int32_t index = 0;


   switch (nSignal)
   {
      case IFX_TAPI_SIG_NONE:
         index = 0;
         break;
      case IFX_TAPI_SIG_DISRX:
         index = 1;
         break;
      case IFX_TAPI_SIG_DISTX:
         index = 2;
         break;
      case IFX_TAPI_SIG_DIS:
         index = 3;
         break;
      case (IFX_TAPI_SIG_DIS | IFX_TAPI_SIG_DISRX | IFX_TAPI_SIG_DISTX):
         index = 4;
         break;
      case IFX_TAPI_SIG_CEDRX:
         index = 5;
         break;
      case IFX_TAPI_SIG_CEDTX:
         index = 6;
         break;
      case IFX_TAPI_SIG_CED:
         index = 7;
         break;
      case (IFX_TAPI_SIG_CED | IFX_TAPI_SIG_CEDRX | IFX_TAPI_SIG_CEDTX):
         index = 8;
         break;
      case IFX_TAPI_SIG_CNGFAXRX:
         index = 9;
         break;
      case IFX_TAPI_SIG_CNGFAXTX:
         index = 10;
         break;
      case IFX_TAPI_SIG_CNGFAX:
         index = 11;
         break;
      case (IFX_TAPI_SIG_CNGFAX
            | IFX_TAPI_SIG_CNGFAXRX
            | IFX_TAPI_SIG_CNGFAXTX):
         index = 12;
         break;
      case IFX_TAPI_SIG_CNGMODRX:
         index = 13;
         break;
      case IFX_TAPI_SIG_CNGMODTX:
         index = 14;
         break;
      case IFX_TAPI_SIG_CNGMOD:
         index = 15;
         break;
      case (IFX_TAPI_SIG_CNGMOD
            | IFX_TAPI_SIG_CNGMODRX
            | IFX_TAPI_SIG_CNGMODTX):
         index = 16;
         break;
      case IFX_TAPI_SIG_PHASEREVRX:
         index = 17;
         break;
      case IFX_TAPI_SIG_PHASEREVTX:
         index = 18;
         break;
      case IFX_TAPI_SIG_PHASEREV:
         index = 19;
         break;
      case (IFX_TAPI_SIG_PHASEREV
            | IFX_TAPI_SIG_PHASEREVRX
            | IFX_TAPI_SIG_PHASEREVTX):
         index = 20;
         break;
      case IFX_TAPI_SIG_AMRX:
         index = 21;
         break;
      case IFX_TAPI_SIG_AMTX:
         index = 22;
         break;
      case IFX_TAPI_SIG_AM:
         index = 23;
         break;
      case (IFX_TAPI_SIG_AM | IFX_TAPI_SIG_AMRX | IFX_TAPI_SIG_AMTX):
         index = 24;
         break;
      case IFX_TAPI_SIG_TONEHOLDING_ENDRX:
         index = 25;
         break;
      case IFX_TAPI_SIG_TONEHOLDING_ENDTX:
         index = 26;
         break;
      case IFX_TAPI_SIG_TONEHOLDING_END:
         index = 27;
         break;
      case (IFX_TAPI_SIG_TONEHOLDING_END
            | IFX_TAPI_SIG_TONEHOLDING_ENDRX
            | IFX_TAPI_SIG_TONEHOLDING_ENDTX):
         index = 28;
         break;
      case IFX_TAPI_SIG_CEDENDRX:
         index = 29;
         break;
      case IFX_TAPI_SIG_CEDENDTX:
         index = 30;
         break;
      case IFX_TAPI_SIG_CEDEND:
         index = 31;
         break;
      case (IFX_TAPI_SIG_CEDEND
            | IFX_TAPI_SIG_CEDENDRX
            | IFX_TAPI_SIG_CEDENDTX):
         index = 32;
         break;
     case TAPI_SIGNAL_TONEHOLDING:
         index = 33;
         break;
     case TAPI_SIGNAL_TIMEOUT:
         index = 34;
         break;
      /* special signals for multiple signals or'd in one call */
      case (IFX_TAPI_SIG_CEDTX | IFX_TAPI_SIG_CEDENDTX
             | IFX_TAPI_SIG_PHASEREV | IFX_TAPI_SIG_AM):
         index = 35;
         break;
      case (IFX_TAPI_SIG_CEDRX | IFX_TAPI_SIG_CEDENDRX
             | IFX_TAPI_SIG_PHASEREV | IFX_TAPI_SIG_AM):
         index = 36;
         break;
      case IFX_TAPI_SIG_CPTD:
         index = 37;
         break;
      case IFX_TAPI_SIG_V8BISRX:
         index = 38;
         break;
      case IFX_TAPI_SIG_V8BISTX:
         index = 39;
         break;
      case IFX_TAPI_SIG_CIDENDTX:
         index = 40;
         break;
      default:
         index = 41;
         break;
   } /* switch */

   return Signals[index];
} /* SignalToName() */


/**
   Helper function to manually trigger a signal exception in the TAPI.
   Used for debugging only.

   \param   fd      -  filedescriptor to tapi channel
   \param   signal  -  signal to trigger

   \return  status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t SetSignal(IFX_int32_t fd, IFX_int32_t signal)
{
   if (0 < fd)
   {
      /*return (ioctl(fd, IFXPHONE_SET_SIGNAL, (int)signal));*/
   }

   return IFX_ERROR;
} /* SetSignal() */


/**
   Setup initial signal detection config in transmit direction (Caller).

   \param   fd  -  filedescriptor to tapi channel

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
   This function is called when the calling subscriber moves from ringback
   state to active state.
*/
IFX_return_t TxSignalSetup(IFX_int32_t fd)
{
   IFX_return_t ret = IFX_SUCCESS;


   ret = EnableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXTX);
   if (IFX_SUCCESS == ret)
   {
      ret = EnableSignalDetection(fd, IFX_TAPI_SIG_CNGMODTX);
   }

   /* Attention: Detection of phase reversals (IFX_TAPI_SIG_PHASEREV) and
      amplitude modulation (IFX_TAPI_SIG_AM) have no direction information
      and have to be sent together with the corresponding CED detection
      command */
   if (IFX_SUCCESS == ret)
   {
      ret = EnableSignalDetection(fd, (IFX_TAPI_SIG_CEDRX
                                        | IFX_TAPI_SIG_CEDENDRX
                                        | IFX_TAPI_SIG_PHASEREV
                                        | IFX_TAPI_SIG_AM));
   }

   if (IFX_SUCCESS == ret)
   {
      ret = EnableSignalDetection(fd, IFX_TAPI_SIG_DISRX);
   }

   return ret;
} /* TxSignalSetup() */


/**
   Setup initial signal detection config in receive direction (Callee).

   \param   fd  -  filedescriptor to tapi channel

   \return  status (IFX_SUCCESS or IFX_ERROR)

   \remark
   This function is called when the called subscriber moves from ringing
   state to active state
*/
IFX_return_t RxSignalSetup(IFX_int32_t fd)
{
   IFX_return_t ret = IFX_SUCCESS;


   ret = EnableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXRX);
   if (IFX_SUCCESS == ret)
   {
     ret = EnableSignalDetection(fd, IFX_TAPI_SIG_CNGMODRX);
   } 

   /* Attention: Detection of phase reversals (IFX_TAPI_SIG_PHASEREV) and
      amplitude modulation (IFX_TAPI_SIG_AM) have no direction information
      and have to be sent together with the corresponding CED detection
      command */
   if (IFX_SUCCESS == ret)
   {
      ret = EnableSignalDetection(fd, (IFX_TAPI_SIG_CEDTX
                                         | IFX_TAPI_SIG_CEDENDTX
                                         | IFX_TAPI_SIG_PHASEREV
                                         | IFX_TAPI_SIG_AM));
   } 

   if (IFX_SUCCESS == ret)
   {
      ret = EnableSignalDetection(fd, IFX_TAPI_SIG_DISTX);
   } 

   return ret;
} /* RxSignalSetup() */


/**
   reset transmit signal handler
   This function is called e.g. on hook interrupts

   \param   fd  -  filedescriptor to tapi channel
            pCurrentSignal - Pointer to current signalling state
            pTimeout       - Pointer to timeout value until next
                             signalling update?

   \return  status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t TxSignalHandlerReset(IFX_int32_t fd,
                                 IFX_int32_t *pCurrentSignal, 
                                 IFX_int32_t *pTimeout)
{
   IFX_return_t ret;


   if ((IFX_NULL == pCurrentSignal) || (IFX_NULL == pTimeout))
   {   
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }
 
   ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXTX);
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODTX);
   }

   /* Attention: Detection of phase reversals (IFX_TAPI_SIG_PHASEREV) and
      amplitude modulation (IFX_TAPI_SIG_AM) have no direction information
      and have to be sent together with the corresponding CED detection
      command */
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, (IFX_TAPI_SIG_CEDRX
                                          | IFX_TAPI_SIG_CEDENDRX
                                          | IFX_TAPI_SIG_PHASEREV
                                          | IFX_TAPI_SIG_AM));
   }
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISRX);
   }
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, IFX_TAPI_SIG_TONEHOLDING_ENDRX);
   }
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, IFX_TAPI_SIG_TONEHOLDING_ENDTX);
   }

   *pCurrentSignal   =  IFX_TAPI_SIG_NONE;
   *pTimeout         =  WAIT_FOREVER;

   return (ret);
} /* TxSignalHandlerReset() */


/**
   reset receiver signal handler
   This function is called e.g. on hook interrupts

   \param   fd  -  filedescriptor to tapi channel
            pCurrentSignal - Pointer to current signalling state
            pTimeout       - Pointer to timeout value until next
                             signalling update?

   \return  status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t RxSignalHandlerReset(IFX_int32_t fd,
                                 IFX_int32_t *pCurrentSignal,
                                 IFX_int32_t *pTimeout)
{
   IFX_return_t ret;


   if ((IFX_NULL == pCurrentSignal) || (IFX_NULL == pTimeout))
   {   
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXRX);
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODRX);
   }

   /* Attention: Detection of phase reversals (IFX_TAPI_SIG_PHASEREV) and
      amplitude modulation (IFX_TAPI_SIG_AM) have no direction information
      and have to be sent together with the corresponding CED detection
      command. */
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, (IFX_TAPI_SIG_CEDTX
                                          | IFX_TAPI_SIG_CEDENDTX
                                          | IFX_TAPI_SIG_PHASEREV
                                          | IFX_TAPI_SIG_AM));
   }
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISTX);
   }
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, IFX_TAPI_SIG_TONEHOLDING_ENDRX);
   }
   if (IFX_SUCCESS == ret)
   {
      ret = DisableSignalDetection(fd, IFX_TAPI_SIG_TONEHOLDING_ENDTX);
   }

   *pCurrentSignal   =  IFX_TAPI_SIG_NONE;
   *pTimeout         =  WAIT_FOREVER;

   return ret;
} /* RxSignalHandlerReset() */


/**
   Handle received signal for transmit and receive direction
   and setup the current channel accordingly.
   This function is called when a signal is received in the execption
   handler.

   \param   fd             - Filedescriptor to tapi channel
            pCurrentSignal - Pointer to current signalling state
            NewSignal      - Received signal
            pTimeout       - Pointer to timeout value until next 
                             signalling update?

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t HandleSignal(IFX_int32_t fd, 
                         IFX_int32_t Direction,
                         IFX_int32_t *pCurrentSignal,
                         IFX_int32_t NewSignal,
                         IFX_int32_t *pTimeout)
{
   IFX_return_t ret = IFX_SUCCESS;
   const IFX_char_t* DIRNAME[2] = {"TAPI_SIGNAL_CALLEE", "TAPI_SIGNAL_CALLER"};


   if ((IFX_NULL == pCurrentSignal) || (IFX_NULL == pTimeout))
   {   
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   TRACE(TAPIDEMO, DBG_LEVEL_LOW,
         ("\nfd %d (%s): Current State %s -- New Signal %s received\n",
          (int) fd, DIRNAME[Direction],
           SignalToName(*pCurrentSignal), SignalToName(NewSignal)));

   /* Restore Timeout setting for next iteration */
   *pTimeout = WAIT_FOREVER;

   switch (*pCurrentSignal)
   {
      /* No Previous signal detected */
      case IFX_TAPI_SIG_NONE:
         switch (NewSignal)
         {
            case IFX_TAPI_SIG_CNGFAX:
            case IFX_TAPI_SIG_CNGFAXRX:
            case IFX_TAPI_SIG_CNGFAXTX:
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  /* CNG detected */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODTX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXTX);
                  }
               }
               else
               {
                  /* CNG detected */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODRX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXRX);
                  }
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = ClearChannel(fd);
               }
               *pCurrentSignal = IFX_TAPI_SIG_CNGFAX;
               *pTimeout = 5000; /* set timeout to 5000 ms */
               break;

            case IFX_TAPI_SIG_CNGMOD: /* 2 */
            case IFX_TAPI_SIG_CNGMODTX: /* 2 */
            case IFX_TAPI_SIG_CNGMODRX: /* 2 */
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  /* CNG detected */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODTX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXTX);
                  }
               }
               else
               {
                  /* CNG detected */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODRX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXRX);
                  }
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = ClearChannel(fd);
               }
               *pCurrentSignal = IFX_TAPI_SIG_CNGMOD;
               *pTimeout = 5000; /* set timeout to 5000 ms */
               break;

            case IFX_TAPI_SIG_CED: /* 3 */
            case IFX_TAPI_SIG_CEDTX: /* 3 */
            case IFX_TAPI_SIG_CEDRX: /* 3 */
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CEDRX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODTX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXTX);
                  }
               }
               else
               {
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODRX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXRX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CEDTX);
                  }
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = ClearChannel(fd);
               }
               *pCurrentSignal = IFX_TAPI_SIG_CED;
               break;

            case IFX_TAPI_SIG_DIS:
            case IFX_TAPI_SIG_DISTX:
            case IFX_TAPI_SIG_DISRX:
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODTX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXTX);
                  }
                  /* Attention: Detection of phase reversals 
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding
                      CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, (IFX_TAPI_SIG_CEDRX |
                                                        IFX_TAPI_SIG_CEDENDRX |
                                                        IFX_TAPI_SIG_PHASEREV |
                                                        IFX_TAPI_SIG_AM));
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISRX);
                  }
               } /* if */
               else
               {
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGMODRX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXRX);
                  }
                  /* Attention: Detection of phase reversals
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation 
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding
                     CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, (IFX_TAPI_SIG_CEDTX
                                                      | IFX_TAPI_SIG_CEDENDTX
                                                      | IFX_TAPI_SIG_PHASEREV
                                                      | IFX_TAPI_SIG_AM));
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISTX);
                  } 
               } /* else */

               if (IFX_SUCCESS == ret)
               {
                  ret = ClearChannel(fd);
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = EnableSignalDetection(fd,
                           IFX_TAPI_SIG_TONEHOLDING_ENDRX);
               }
               if (IFX_SUCCESS == ret)
               {
                  ret = EnableSignalDetection(fd,
                           IFX_TAPI_SIG_TONEHOLDING_ENDTX);
               }
               *pCurrentSignal = TAPI_SIGNAL_TONEHOLDING;
               break;

            default:
               TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Unhandled state %s in %s\n",
                  SignalToName(NewSignal), SignalToName(*pCurrentSignal)));
               break;
         } /* switch */
         break;

/* ------------------------------------------------------------------ */
/* -----      END of case IFX_TAPI_SIG_NONE:    -----  */

      case IFX_TAPI_SIG_CNGFAX:
      case IFX_TAPI_SIG_CNGMOD:
         switch (NewSignal)
         {
            case IFX_TAPI_SIG_DIS: /* 2a */
            case IFX_TAPI_SIG_DISTX: /* 2a */
            case IFX_TAPI_SIG_DISRX: /* 2a */
               if (Direction == TAPI_SIGNAL_CALLER)
               {
                  /* Attention: Detection of phase reversals
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation 
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding
                     CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection (fd, (IFX_TAPI_SIG_CEDRX
                                                        | IFX_TAPI_SIG_CEDENDRX
                                                        | IFX_TAPI_SIG_PHASEREV
                                                        | IFX_TAPI_SIG_AM));
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection (fd, IFX_TAPI_SIG_DISRX);
                  } 
               }
               else
               {
                  /* Attention: Detection of phase reversals
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation 
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding
                     CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, (IFX_TAPI_SIG_CEDTX
                                                        | IFX_TAPI_SIG_CEDENDTX
                                                        | IFX_TAPI_SIG_PHASEREV
                                                        | IFX_TAPI_SIG_AM));
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISTX);
                  }
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = EnableSignalDetection(fd, 
                           IFX_TAPI_SIG_TONEHOLDING_ENDTX);
               }
               /* EnableT38() */
               *pCurrentSignal = TAPI_SIGNAL_TONEHOLDING;
               break;
            case TAPI_SIGNAL_TIMEOUT: /* 2b */
               /* timeout expired ? */
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  /* Attention: Detection of phase reversals 
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation 
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding 
                     CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, (IFX_TAPI_SIG_CEDRX
                                                        | IFX_TAPI_SIG_CEDENDRX
                                                        | IFX_TAPI_SIG_PHASEREV
                                                        | IFX_TAPI_SIG_AM));
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISRX);
                  }
               }
               else
               {
                  /* Attention: Detection of phase reversals 
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation 
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding 
                     CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, (IFX_TAPI_SIG_CEDTX
                                                        | IFX_TAPI_SIG_CEDENDTX
                                                        | IFX_TAPI_SIG_PHASEREV
                                                        | IFX_TAPI_SIG_AM));
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISTX);
                  }
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = EnableSignalDetection(fd, 
                           IFX_TAPI_SIG_TONEHOLDING_ENDRX);
               }
               if (IFX_SUCCESS == ret)
               {
                  ret = EnableSignalDetection(fd, 
                           IFX_TAPI_SIG_TONEHOLDING_ENDTX);
               }
               *pCurrentSignal = TAPI_SIGNAL_TONEHOLDING;
               break;

            case IFX_TAPI_SIG_CED:  /* 2c */
            case IFX_TAPI_SIG_CEDTX:  /* 2c */
            case IFX_TAPI_SIG_CEDRX:  /* 2c */
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CEDRX);
                  }
               }
               else
               {
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_CEDTX);
                  }
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = DisableNLP(fd);
               }
               *pCurrentSignal = IFX_TAPI_SIG_CED;
               break;
            default:
               TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Unhandled state %s in %s\n",
                  SignalToName(NewSignal), SignalToName(*pCurrentSignal)));
               break;
         } /* switch */
         break;

/* ------------------------------------------------------------------ */
/* -----      END of case IFX_TAPI_SIG_CNGFAX:    -----  */
/* -----      END of case IFX_TAPI_SIG_CNGMOD:    -----  */

      case IFX_TAPI_SIG_CED:
      case IFX_TAPI_SIG_CEDTX:
      case IFX_TAPI_SIG_CEDRX:
         switch (NewSignal)
         {
            case IFX_TAPI_SIG_PHASEREV:
            case IFX_TAPI_SIG_PHASEREVTX:
            case IFX_TAPI_SIG_PHASEREVRX:
               if (TAPI_SIGNAL_CALLER == Direction)
               {
               }
               else
               {
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = DisableLEC(fd);
               }
               *pCurrentSignal = IFX_TAPI_SIG_CED;
               break;
            case IFX_TAPI_SIG_AM:
            case IFX_TAPI_SIG_AMTX:
            case IFX_TAPI_SIG_AMRX:
               /* handled by driver automatically */
               if (TAPI_SIGNAL_CALLER == Direction)
               {
               }
               else
               {
               }

               *pCurrentSignal = IFX_TAPI_SIG_CED;
               break;
            case IFX_TAPI_SIG_CEDEND:
            case IFX_TAPI_SIG_CEDENDTX:
            case IFX_TAPI_SIG_CEDENDRX:
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  /* Attention: Detection of phase reversals 
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation 
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding 
                     CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, (IFX_TAPI_SIG_CEDRX
                                                        | IFX_TAPI_SIG_CEDENDRX
                                                        | IFX_TAPI_SIG_PHASEREV
                                                        | IFX_TAPI_SIG_AM));
                  }
               }
               else
               {
                  /* Attention: Detection of phase reversals 
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation 
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding 
                     CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, (IFX_TAPI_SIG_CEDTX
                                                        | IFX_TAPI_SIG_CEDENDTX
                                                        | IFX_TAPI_SIG_PHASEREV
                                                        | IFX_TAPI_SIG_AM));
                  }
               }

               *pCurrentSignal = IFX_TAPI_SIG_CEDEND;
               *pTimeout = 200; /* set timeout to 200 ms */
               break;
            default:
               TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Unhandled state %s in %s\n",
                  SignalToName(NewSignal), SignalToName(*pCurrentSignal)));
               break;
         } /* switch */
         break;

/* ------------------------------------------------------------------ */
/* -----      END of case IFX_TAPI_SIG_CED:    -----  */
/* -----      END of case IFX_TAPI_SIG_CEDTX:    -----  */
/* -----      END of case IFX_TAPI_SIG_CEDRX:    -----  */

      case IFX_TAPI_SIG_CEDEND:
      case IFX_TAPI_SIG_CEDENDTX:
      case IFX_TAPI_SIG_CEDENDRX:
         switch (NewSignal)
         {
            case IFX_TAPI_SIG_DIS:
            case IFX_TAPI_SIG_DISTX:
            case IFX_TAPI_SIG_DISRX:
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  /* dis detected -- this is a fax transmission */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISRX);
                  }
               }
               else
               {
                  /* dis detected -- this is a fax transmission */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISTX);
                  }
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = EnableSignalDetection(fd, 
                           IFX_TAPI_SIG_TONEHOLDING_ENDTX);
               }
               if (IFX_SUCCESS == ret)
               {
                  ret = EnableSignalDetection(fd, 
                           IFX_TAPI_SIG_TONEHOLDING_ENDRX);
               }
               *pCurrentSignal = TAPI_SIGNAL_TONEHOLDING;
               break;

            case TAPI_SIGNAL_TIMEOUT:
               /* timeout */
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  /* dis detected -- this is a fax transmission */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISRX);
                  }
               }
               else
               {
                  /* dis detected -- this is a fax transmission */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = DisableSignalDetection(fd, IFX_TAPI_SIG_DISTX);
                  }
               }

               if (IFX_SUCCESS == ret)
               {
                  ret = EnableSignalDetection(fd,
                           IFX_TAPI_SIG_TONEHOLDING_ENDTX);
               }
               if (IFX_SUCCESS == ret)
               {
                  ret = EnableSignalDetection(fd,
                           IFX_TAPI_SIG_TONEHOLDING_ENDRX);
               }
               *pCurrentSignal = TAPI_SIGNAL_TONEHOLDING;
               break;

            default:
               TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Unhandled state %s in %s\n",
                  SignalToName(NewSignal), SignalToName(*pCurrentSignal)));
               break;
         } /* switch */
         break;

/* ------------------------------------------------------------------ */
/* -----      END of case IFX_TAPI_SIG_CEDEND:    -----  */
/* -----      END of case IFX_TAPI_SIG_CEDENDTX:    -----  */
/* -----      END of case IFX_TAPI_SIG_CEDENDRX:    -----  */

      case TAPI_SIGNAL_TONEHOLDING: /* 5a */
         switch (NewSignal)
         {
            case IFX_TAPI_SIG_TONEHOLDING_END: /* 6a */
            case IFX_TAPI_SIG_TONEHOLDING_ENDTX: /* 6a */
            case IFX_TAPI_SIG_TONEHOLDING_ENDRX: /* 6a */
               /* ist das sicher, dass hier die übertragung zuende ist,
                  bzw. voice call weitergeht ? */
               if (IFX_SUCCESS == ret)
               {
                  ret = DisableSignalDetection(fd, 
                           IFX_TAPI_SIG_TONEHOLDING_ENDTX);
               }
               if (IFX_SUCCESS == ret)
               {
                  ret = DisableSignalDetection(fd,
                           IFX_TAPI_SIG_TONEHOLDING_ENDRX);
               }
               if (TAPI_SIGNAL_CALLER == Direction)
               {
                  if (IFX_SUCCESS == ret)
                  {
                     ret = EnableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXTX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = EnableSignalDetection(fd, IFX_TAPI_SIG_CNGMODTX);
                  }

                  /* Attention: Detection of phase reversals
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding
                     CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = EnableSignalDetection(fd, (IFX_TAPI_SIG_CEDRX
                                                        | IFX_TAPI_SIG_CEDENDRX
                                                        | IFX_TAPI_SIG_PHASEREV
                                                        | IFX_TAPI_SIG_AM));
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = EnableSignalDetection(fd, IFX_TAPI_SIG_DISRX);
                  }
               } /* if */
               else
               {
                  if (IFX_SUCCESS == ret)
                  {
                     ret = EnableSignalDetection(fd, IFX_TAPI_SIG_CNGFAXRX);
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = EnableSignalDetection(fd, IFX_TAPI_SIG_CNGMODRX);
                  }

                  /* Attention: Detection of phase reversals
                     (IFX_TAPI_SIG_PHASEREV) and amplitude modulation
                     (IFX_TAPI_SIG_AM) have no direction information
                     and have to be sent together with the corresponding
                     CED detection command. */
                  if (IFX_SUCCESS == ret)
                  {
                     ret = EnableSignalDetection(fd, (IFX_TAPI_SIG_CEDTX
                                                        | IFX_TAPI_SIG_CEDENDTX
                                                        | IFX_TAPI_SIG_PHASEREV
                                                        | IFX_TAPI_SIG_AM));
                  }
                  if (IFX_SUCCESS == ret)
                  {
                     ret = EnableSignalDetection(fd, IFX_TAPI_SIG_DISTX);
                  }
               } /* else */

               *pCurrentSignal = IFX_TAPI_SIG_NONE;
               break;
            default:
               TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Unhandled state %s in %s\n",
                  SignalToName(NewSignal), SignalToName(*pCurrentSignal)));
               break;
         } /* switch */
         break;

/* ------------------------------------------------------------------ */
/* -----      END of case TAPI_SIGNAL_TONEHOLDING:    -----  */

      default:
         /* unhandled signal */
         TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Unhandled state %s in %s\n",
            SignalToName(NewSignal), SignalToName(*pCurrentSignal)));
         break;
   } /* switch */

   TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("\nfd %d: Next State %s\n",
          (int) fd, SignalToName(*pCurrentSignal)));

   return IFX_SUCCESS;
} /* HandleSignal() */

#endif /* TAPI_SIGNAL_SUPPORT */
#endif /* TAPI_V21 */


