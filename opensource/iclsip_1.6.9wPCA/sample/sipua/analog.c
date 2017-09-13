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
   Module      : analog.c
   Date        : 2006-04-14
   Description : This file contains the implementation of the functions for
                 the tapi demo working with ALM module
   \file

   \note Changes:
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "analog.h"

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** LEC configuration for phone. */

/** LEC type selection, not used anymore */
/*const IFX_int32_t LEC_TYPE = IFX_TAPI_LEC_TYPE_NE;*/
/** Gain for input */
const IFX_int32_t INPUT_GAIN = IFX_TAPI_LEC_GAIN_MEDIUM;
/** Gain for output */
const IFX_int32_t OUTPUT_GAIN = IFX_TAPI_LEC_GAIN_MEDIUM;
/** LEC tail length [ms], not supported currently */
const IFX_int32_t LEC_TAIL_LEN = 0;
/** Switch NLP on or off */
const IFX_int32_t SET_NLP = IFX_TAPI_LEC_NLP_ON;

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   ALM Init.

   \param pProgramArg - pointer to program arguments

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t ALM_Init(PROGRAM_ARG_t* pProgramArg)
{
   IFX_int32_t i = 0;
   IFX_TAPI_LEC_CFG_t lecConf;

#ifdef VERSION_1_2
   IFX_int32_t fd_ch = -1;
#endif /* VERSION_1_2 */

   /* Activate LEC for Phone */
   memset(&lecConf, 0, sizeof(IFX_TAPI_LEC_CFG_t));

   /*ioctl(fd, IFX_TAPI_LEC_PHONE_CFG_GET, (IFX_int32_t) &lecConf);*/

   /* Select LEC Type, not used anymore */;
   /*lecConf.nType = LEC_TYPE;*/
   /* Activate LEC */;
   lecConf.nGainIn = INPUT_GAIN;
   lecConf.nGainOut = OUTPUT_GAIN;
   lecConf.nLen = 16;
   /* Activate NLP */;
   lecConf.bNlp = SET_NLP;

   for (i = 0; i < MAX_SYS_LINE_CH; i++)
   {
#if 1
#ifdef VERSION_1_2
      fd_ch = ALM_GetFD_OfCh(i);
      /*if (ioctl(fd_ch, IFX_TAPI_LEC_PHONE_CFG_SET, (IFX_int32_t) &lecConf) == IFX_ERROR)
         printf("%s():%d - Error\n", __FUNCTION__, __LINE__);*/
#endif /* VERSION_1_2 */
#endif
   }

   return IFX_SUCCESS;
} /* ALM_Init() */


/**
   Map phone channel to phone chanel.

   \param nPhoneCh    - target phone channel
   \param nAddPhoneCh - which phone channel to add
   \param fDoMapping  - flag if mapping should be done (IFX_TRUE),
                        or unmapping (IFX_FALSE)
   \param pConf       - pointer to conference structure

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t ALM_MapToPhone(IFX_int32_t nPhoneCh,
                            IFX_int32_t nAddPhoneCh,
                            IFX_boolean_t fDoMapping,
                            CONFERENCE_t* pConf)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_PHONE_t phonemap;
   IFX_int32_t fd_ch = -1;


   if ((IFX_NULL == pConf)
       || (0 > nPhoneCh) || (MAX_SYS_LINE_CH < nPhoneCh)
       || (0 > nAddPhoneCh) || (MAX_SYS_LINE_CH < nAddPhoneCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&phonemap, 0, sizeof(IFX_TAPI_MAP_PHONE_t));

   fd_ch = ALM_GetFD_OfCh(nPhoneCh);

   phonemap.nPhoneCh = nAddPhoneCh;
   if (IFX_TRUE == fDoMapping)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Map phone ch %d to phone ch %d\n",
            (int) nAddPhoneCh, (int) nPhoneCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_PHONE_ADD,
                  (IFX_int32_t) &phonemap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Error mapping phone ch %d to phone ch %d using fd %d. \
              (File: %s, line: %d)\n",
              (int) nAddPhoneCh, (int) nPhoneCh, (int) fd_ch,
              __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Add new pair of mapping */
      pConf->MappingTable[pConf->nMappingCnt].nCh = nPhoneCh;
      pConf->MappingTable[pConf->nMappingCnt].nAddedCh = nAddPhoneCh;
      pConf->MappingTable[pConf->nMappingCnt].nMappingType = PHONE_PHONE;
      pConf->MappingTable[pConf->nMappingCnt].fMapping = IFX_TRUE;
      pConf->nMappingCnt++;
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Unmap phone ch %d from phone ch %d\n",
            (int) nAddPhoneCh, (int) nPhoneCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_PHONE_REMOVE,
                  (IFX_int32_t) &phonemap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Error unmapping phone ch %d from phone ch %d using fd %d. \
              (File: %s, line: %d)\n",
              (int) nAddPhoneCh, (int) nPhoneCh, (int) fd_ch,
              __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* ALM_MapToPhone() */


/**
   Get file descriptor of device for phone channel.

   \param nPhoneCh - phone channel

   \return device connected to this channel or NO_DEVICE_FD if none
*/
IFX_int32_t ALM_GetFD_OfCh(IFX_int32_t nPhoneCh)
{
   if ((0 > nPhoneCh) || (MAX_SYS_LINE_CH < nPhoneCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return NO_DEVICE_FD;
   }

   return Common_GetDeviceOfCh(nPhoneCh);
} /* ALM_GetFD_OfCh() */

