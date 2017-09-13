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
   Module      : feature.c
   Date        : 2005-04-06
   Description : This file contains the implementation of the functions for
                 the tapi demo working with additional features:
                 - AGC ( Automated Gain Control )
                 
   \file 

   \remarks
      
   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "feature.h"


/* ============================= */
/* Local structures              */
/* ============================= */

/** AGC related stuff */
/** min/max parameters: */

/** "Compare Level", this is the target level in 'dB', MAX is 0dB. */
const IFX_int32_t AGC_CONFIG_COM_MAX = 0;
/** "Compare Level", this is the target level in 'dB', MIN is -50dB. */
const IFX_int32_t AGC_CONFIG_COM_MIN = -50;
/** "Maximum Gain", maximum gain that we'll be applied to the signal in
    'dB', MAX is 48dB. */
const IFX_int32_t AGC_CONFIG_GAIN_MAX = 48;
/** "Maximum Gain", maximum gain that we'll be applied to the signal in
    'dB', MIN is 0dB. */
const IFX_int32_t AGC_CONFIG_GAIN_MIN = 0;
/** "Maximum Attenuation for AGC", maximum attenuation that we'll be applied 
    to the signal in 'dB', MAX is 0dB. */
const IFX_int32_t AGC_CONFIG_ATT_MAX = 0;
/** "Maximum Attenuation for AGC", maximum attenuation that we'll be applied 
    to the signal in 'dB', MIN is -42dB. */
const IFX_int32_t AGC_CONFIG_ATT_MIN = -42;
/** "Minimum Input Level", signals below this threshold won't be processed 
    by AGC in 'dB', MAX is -25 dB. */
const IFX_int32_t AGC_CONFIG_LIM_MAX = -25;
/** "Minimum Input Level", signals below this threshold won't be processed 
    by AGC in 'dB', MIN is -60 dB. */
const IFX_int32_t AGC_CONFIG_LIM_MIN = -60;


/** Action types */
enum 
{
   NO_FEATURE_ACTION = -1,
   FEATURE_ACTION_DISABLE = 0,
   FEATURE_ACTION_ENABLE
};

/** Feature list */
enum 
{
   NO_FEATURE = -1,
   FEATURE_AGC = 0
};


/* ============================= */
/* Global Structures             */
/* ============================= */


/* ============================= */
/* Global function declaration   */
/* ============================= */

IFX_return_t FEATURE_AGC_Enable(IFX_int32_t nDataCh_FD,
                                IFX_boolean_t fEnable);
IFX_return_t FEATURE_AGC_Cfg(IFX_int32_t nDataCh_FD,
                             IFX_int32_t nAGC_CompareLvl,
                             IFX_int32_t nAGC_MaxGain,
                             IFX_int32_t nAGC_MaxAttenuation,
                             IFX_int32_t nAGC_MinInputLvl);


/* ============================= */
/* Local variable definition     */
/* ============================= */


/* ============================= */
/* Local function definition     */
/* ============================= */


/* ============================= */
/* Global function definition    */
/* ============================= */


/* ----------------------------------------------------------------------------
                                 COMMON 
 */


/**
   According to phone number perform action for specific feature.

   \param nDataCh_FD - data channel file descriptor
   \param nDialedNum - dialed number

   \return IFX_SUCCESS if action for feature performed, otherwise IFX_ERROR
*/
IFX_return_t FEATURE_Action(IFX_int32_t nDataCh_FD,
                            IFX_int32_t nDialedNum)
{
   IFX_int32_t feature_id = NO_FEATURE;
   IFX_int32_t action = NO_FEATURE_ACTION;


   if ((0 > nDataCh_FD) || (0 > nDialedNum))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Setup features */
   /* Number is following:
      07<xx><y>
      xx - feature id
      y - 1 -> enable this feature
          0 -> disable this feature
    */

   /* Extract action and feature id */
   action = nDialedNum % 10; 
   nDialedNum /= 10; 
   feature_id = nDialedNum % 100; 

   switch (feature_id)
   {
      case FEATURE_AGC:
         if (FEATURE_ACTION_ENABLE == action)
         {
            TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, 
                 ("Configure AGC feature with com = 0, gain = 0, "
                  "att = 0 and lim = 0\n"));

            FEATURE_AGC_Cfg(nDataCh_FD, /* com */ 0, /* gain */ 0
                            , /* att */0, /* lim */0);

            TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, 
                 ("ENABLE AGC feature\n"));

            FEATURE_AGC_Enable(nDataCh_FD, IFX_TRUE);
         }
         else
         {
            TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
                 ("DISABLE AGC feature\n"));

            FEATURE_AGC_Enable(nDataCh_FD, IFX_FALSE);
         }
         break;
      default:
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
              ("Unknown feature id %d or no feature id. (File: %s, line: %d)\n", 
              (int) feature_id, __FILE__, __LINE__));
         return IFX_ERROR;
         break;
   };

   return IFX_SUCCESS;
}

/* ----------------------------------------------------------------------------
                        AGC - Automated Gain Control, START
 */


/**
   Enable AGC (Automated Gain Control).

   \param  nDataCh_FD - data channel file descriptor
   \param  fEnable - IFX_TRUE enable it, IFX_FALSE disable it

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t FEATURE_AGC_Enable(IFX_int32_t nDataCh_FD,
                                IFX_boolean_t fEnable)
{
   IFX_return_t ret = IFX_FALSE;

#ifndef VERSION_1_2
/* Only supported in version 1.1. */

#ifdef BITFIELD_EVENTS   
   IFX_TAPI_ENC_AGC_MODE_t nMode;


   if (0 > nDataCh_FD)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   nMode = fEnable ? IFX_TAPI_ENC_AGC_MODE_ENABLE
                   : IFX_TAPI_ENC_AGC_MODE_DISABLE;

   ret = ioctl(nDataCh_FD, IFX_TAPI_ENC_AGC_ENABLE, nMode);
#endif /* BITFIELD_EVENTS */

#endif /* VERSION_1_2 */

   return ret;
} /* FEATURE_AGC_Enable() */



/**
   Configure AGC.

   \param  nDataCh_FD - data channel file descriptor
   \param  nAGC_CompareLvl - compare level
   \param  nAGC_MaxGain - maximum gain
   \param  nAGC_MaxAttenuation - maximum attenuation
   \param  nAGC_MinInputLvl - minimum input level

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t FEATURE_AGC_Cfg(IFX_int32_t nDataCh_FD,
                             IFX_int32_t nAGC_CompareLvl,
                             IFX_int32_t nAGC_MaxGain,
                             IFX_int32_t nAGC_MaxAttenuation,
                             IFX_int32_t nAGC_MinInputLvl)
{
   IFX_return_t ret = IFX_FALSE;

#ifndef VERSION_1_2
/* Only supported in version 1.1. */

#ifdef BITFIELD_EVENTS
   IFX_TAPI_ENC_AGC_CFG_t cfg_agc;

   /* check input arguments */
   if (0 > nDataCh_FD)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&cfg_agc, 0, sizeof(IFX_TAPI_ENC_AGC_CFG_t));

   /* By default use MIN values */
   if ((AGC_CONFIG_COM_MIN < nAGC_CompareLvl)
       && (AGC_CONFIG_COM_MAX > nAGC_CompareLvl))
   {
      cfg_agc.com = nAGC_CompareLvl;
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Compare Level %d for AGC outside interval [%d..%d], will"
            " use MIN value\n",
            (int) nAGC_CompareLvl, (int) AGC_CONFIG_COM_MIN,
            (int) AGC_CONFIG_COM_MAX));
      cfg_agc.com = AGC_CONFIG_COM_MIN;
   }

   if ((AGC_CONFIG_GAIN_MIN < nAGC_MaxGain)
       && (AGC_CONFIG_GAIN_MAX > nAGC_MaxGain))
   {
      cfg_agc.gain = nAGC_MaxGain;
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Maximum Gain %d for AGC outside interval [%d..%d], will"
            " use MIN value\n",
           (int) nAGC_MaxGain, (int) AGC_CONFIG_GAIN_MIN,
           (int) AGC_CONFIG_GAIN_MAX));
      cfg_agc.gain = AGC_CONFIG_GAIN_MIN;
   }

   if ((AGC_CONFIG_ATT_MIN < nAGC_MaxAttenuation)
       && (AGC_CONFIG_ATT_MAX > nAGC_MaxAttenuation))
   {
      cfg_agc.att = nAGC_MaxAttenuation;
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Maximum Attenuation %d for AGC outside interval [%d..%d], will"
            " use MIN value\n",
            (int) nAGC_MaxAttenuation, (int) AGC_CONFIG_ATT_MIN,
            (int) AGC_CONFIG_ATT_MAX));
      cfg_agc.att = AGC_CONFIG_ATT_MIN;
   }

   if ((AGC_CONFIG_LIM_MIN < nAGC_MinInputLvl)
       && (AGC_CONFIG_LIM_MAX > nAGC_MinInputLvl))
   {
      cfg_agc.lim = nAGC_MinInputLvl;
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Minimum Input Level %d for AGC outside interval [%d..%d], will"
            " use MIN value\n",
            (int) nAGC_MinInputLvl, (int) AGC_CONFIG_LIM_MIN,
            (int) AGC_CONFIG_LIM_MAX));
      cfg_agc.lim = AGC_CONFIG_LIM_MIN;
   }

   ret = ioctl(nDataCh_FD, IFX_TAPI_ENC_AGC_CFG, (IFX_int32_t) &cfg_agc);

#endif /* BITFIELD_EVENTS */

#endif /* VERSION_1_2 */
   
   return ret;
} /* FEATURE_AGC_Cfg() */

/* ----------------------------------------------------------------------------
                        AGC - Automated Gain Control, END
 */
