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
   Module      : voip.c
   Date        : 2006-04-04
   Description : This file contains the implementation of the functions for
                 the tapi demo working with VoIP stuff (coder module, codecs)
   \file 

   \note Changes: 
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "voip.h"
#include "cid.h"

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* --------------------------------------------------------- */
/*             RTP Payload Type Tables START                 */
/* --------------------------------------------------------- */

/** Payload types table - Upstream */
enum
{
   RTP_ENC_TYPE_MLAW_UP = 0,
   RTP_ENC_TYPE_ALAW_UP = 8,
   RTP_ENC_TYPE_G729_UP = 18,
   RTP_ENC_TYPE_G723_53_UP = 4,
   RTP_ENC_TYPE_G723_63_UP = 4,
#ifdef VERSION_1_2
   RTP_ENC_TYPE_G7221_24_UP = 9,
   RTP_ENC_TYPE_G7221_32_UP = 9,
   RTP_ENC_TYPE_G722_64_UP = 9,
#endif /* VERSION_1_2 */
   RTP_ENC_TYPE_G728_UP = 15,
   RTP_ENC_TYPE_ILBC_152_UP = 99
};

/** Payload types table - Downstream */
enum
{
   RTP_ENC_TYPE_MLAW_DOWN = 0,
   RTP_ENC_TYPE_ALAW_DOWN = 8,
   RTP_ENC_TYPE_G729_DOWN = 18,
   RTP_ENC_TYPE_G723_53_DOWN = 4,
   RTP_ENC_TYPE_G723_63_DOWN = 4,
#ifdef VERSION_1_2
   RTP_ENC_TYPE_G7221_24_DOWN = 9,
   RTP_ENC_TYPE_G7221_32_DOWN = 9,
   RTP_ENC_TYPE_G722_64_DOWN = 9,
#endif /* VERSION_1_2 */
   RTP_ENC_TYPE_G728_DOWN = 15,
   RTP_ENC_TYPE_ILBC_152_DOWN = 99
};

/* --------------------------------------------------------- */
/*               RTP Payload Type Tables END                 */
/* --------------------------------------------------------- */

/** Jitter buffer configuration. */

/** Packet adaptation */
static const IFX_int32_t JB_PACKET_ADAPTATION = IFX_TAPI_JB_PKT_ADAPT_VOICE;
/** Local adaptation */
static const IFX_int32_t JB_LOCAL_ADAPTATION  = IFX_TAPI_JB_LOCAL_ADAPT_OFF;
/** Play out dealy, value between 0..16 */
static const IFX_int32_t JB_SCALING    = 0x16;
/** Initial size of jitter buffer in timestamps of 125 us */
static const IFX_int32_t JB_INITIAL_SIZE      = 0x0050;
/** Maximum size of jitter buffer in timestamps of 125 us */
static const IFX_int32_t JB_MAXIMUM_SIZE      = 0x05A0;
/** Minimum size of jitter buffer in timestamps of 125 us */
static const IFX_int32_t JB_MINIMUM_SIZE      = 0x0050;

/** Default encoder type */
enum { DEFAULT_CODER_TYPE = IFX_TAPI_ENC_TYPE_MLAW };

/** Default packetisation time [ms] */
enum { DEFAULT_PACKETISATION_TIME = 10 };

/** Current encoder type to use */
static IFX_TAPI_ENC_TYPE_t eCurrEncType = DEFAULT_CODER_TYPE;

/** Current packetisation time to use */
static IFX_int32_t nCurrPacketTime = DEFAULT_PACKETISATION_TIME;


/** Structure holding status for data channel */
typedef struct _VOIP_DATA_CH_t
{
   /** IFX_FALSE if its free, not mapped. Otherwise IFX_TRUE if mapped. */
   IFX_boolean_t fMapped;

   /** Status of encoder/decoder. IFX_TRUE - started, IFX_FALSE - stopped */
   IFX_boolean_t fCodecStarted;

   /** IFX_FALSE - don´t start codec, IFX_TRUE - start codec */
   IFX_boolean_t fStartCodec;

   /** Which type of coder to use. */
   IFX_TAPI_ENC_TYPE_t nCodecType;

   /** Length of frame in miliseconds OR packetisation time. */
   IFX_int32_t nFrameLen;
} VOIP_DATA_CH_t;


/** Status array of data channels. */
static VOIP_DATA_CH_t rgoDataChStat[MAX_SYS_CH_RES];

/** Socket on which we expect incoming data (events and voice)
    Also used to send data to external peer. There is linear  transofrmation
    between sockets and data channel. Socket with index 1 belongs to data
    channel 1, socket with index 2 to data channel 2, etc. */
static IFX_int32_t rgnSockets[MAX_SYS_CH_RES];


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
   VoIP Init.

   \param pProgramArg  - pointer to program arguments

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t VOIP_Init(PROGRAM_ARG_t* pProgramArg)
{
   IFX_int32_t i = 0;
   IFX_TAPI_PKT_RTP_PT_CFG_t rtpPTConf;
   IFX_TAPI_JB_CFG_t jbCfgVoice;

#ifdef VERSION_1_2
   IFX_int32_t fd_ch = -1;
#endif /* VERSION_1_2 */


   if (IFX_NULL == pProgramArg)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_FALSE;
   }

   memset(&rtpPTConf, 0, sizeof(IFX_TAPI_PKT_RTP_PT_CFG_t));

   /* Payload types table - Upstream */

   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_MLAW] = RTP_ENC_TYPE_MLAW_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_ALAW] = RTP_ENC_TYPE_ALAW_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G729] = RTP_ENC_TYPE_G729_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G723_53] = RTP_ENC_TYPE_G723_53_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G723_63] = RTP_ENC_TYPE_G723_63_UP;
#ifdef VERSION_1_2
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G7221_24] = RTP_ENC_TYPE_G7221_24_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G7221_32] = RTP_ENC_TYPE_G7221_32_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G722_64] = RTP_ENC_TYPE_G722_64_UP;
#endif /* VERSION_1_2 */
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_G728] = RTP_ENC_TYPE_G728_UP;
   rtpPTConf.nPTup[IFX_TAPI_ENC_TYPE_ILBC_152] = RTP_ENC_TYPE_ILBC_152_UP;

   /* Payload types table - Downstream */

   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_MLAW] = RTP_ENC_TYPE_MLAW_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_ALAW] = RTP_ENC_TYPE_ALAW_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G729] = RTP_ENC_TYPE_G729_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G723_53] = RTP_ENC_TYPE_G723_53_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G723_63] = RTP_ENC_TYPE_G723_63_DOWN;
#ifdef VERSION_1_2
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G7221_24] = RTP_ENC_TYPE_G7221_24_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G7221_32] = RTP_ENC_TYPE_G7221_32_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G722_64] = RTP_ENC_TYPE_G722_64_DOWN;
#endif /* VERSION_1_2 */
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_G728] = RTP_ENC_TYPE_G728_DOWN;
   rtpPTConf.nPTdown[IFX_TAPI_ENC_TYPE_ILBC_152] = RTP_ENC_TYPE_ILBC_152_DOWN;

   memset(&jbCfgVoice, 0, sizeof(IFX_TAPI_JB_CFG_t));

   /* Adaptive JB */
   jbCfgVoice.nJbType = IFX_TAPI_JB_TYPE_ADAPTIVE;
   /* Optimization for voice */
   jbCfgVoice.nPckAdpt = JB_PACKET_ADAPTATION;
   jbCfgVoice.nLocalAdpt = JB_LOCAL_ADAPTATION;
   jbCfgVoice.nScaling = JB_SCALING;
   /* Initial JB size 10 ms = 0x0050 * 125 µs */
   jbCfgVoice.nInitialSize = JB_INITIAL_SIZE;
   /* Minimum JB size 10 ms = 0x0050 * 125 µs */
   jbCfgVoice.nMinSize = JB_MINIMUM_SIZE;
   /* Maximum JB size 180 ms = 0x05A0 * 125 µs */
   jbCfgVoice.nMaxSize = JB_MAXIMUM_SIZE;

   for (i = 0; i < MAX_SYS_CH_RES; i++)
   {
      rgoDataChStat[i].fMapped = IFX_FALSE;
      rgoDataChStat[i].fCodecStarted = IFX_FALSE;
      rgoDataChStat[i].fStartCodec = IFX_TRUE;
      rgoDataChStat[i].nCodecType = eCurrEncType;
      rgoDataChStat[i].nFrameLen = nCurrPacketTime;

      /* Also initialize sockets */
      if (NO_SOCKET == VOIP_InitUdpSocket(pProgramArg, i))
      {
         /* Error initializing VoIP sockets */
         return IFX_ERROR;
      }
#if 1
#ifdef VERSION_1_2
      fd_ch = VOIP_GetFD_OfCh(i);

      /* Program the channel (addressed by fd) with
         the specified payload types. */
      ioctl(fd_ch, IFX_TAPI_PKT_RTP_PT_CFG_SET, (IFX_int32_t) &rtpPTConf);

      /* Set jitter buffer */
      ioctl(fd_ch, IFX_TAPI_JB_CFG_SET, (IFX_int32_t) &jbCfgVoice);
#endif /* VERSION_1_2 */
#endif
      
   }

   return IFX_SUCCESS;
} /* VOIP_Init() */


/**
   Set VOIP encoder, vocoder type.

   \param nEncoderType - new encoder type

   \return IFX_SUCCESS if OK, IFX_ERROR if errors
*/
IFX_return_t VOIP_SetEncoderType(IFX_int32_t nEncoderType)
{
   IFX_TAPI_ENC_TYPE_t encoder_type = DEFAULT_CODER_TYPE;


   memset(&encoder_type, 0, sizeof(encoder_type));

   if ((0 > nEncoderType) || (IFX_TAPI_ENC_TYPE_MAX < nEncoderType))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s), will use default"
            " DEFAULT_CODER_TYPE encoder type. "
            "(File: %s, line: %d)\n", __FILE__, __LINE__));
      /* Don´t send error, but use default value. */
      encoder_type = DEFAULT_CODER_TYPE;
   } 
   else
   {
      encoder_type = (IFX_TAPI_ENC_TYPE_t) (nEncoderType);
   } 

   eCurrEncType = encoder_type;

   return IFX_SUCCESS;
} /* VOIP_SetEncoderType() */


/**
   Set packetization time to use.

   \param nFrameLen - Frame len in ms or packetisation time

   \return IFX_SUCCESS if OK, IFX_ERROR if errors
*/
IFX_return_t VOIP_SetFrameLen(IFX_int32_t nFrameLen)
{
   IFX_int32_t frame_len = DEFAULT_PACKETISATION_TIME;


   if ((0 >= nFrameLen) || (1000 < nFrameLen))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s), will use default %d [ms]"
            " packetisation time. (File: %s, line: %d)\n",
            DEFAULT_PACKETISATION_TIME, __FILE__, __LINE__));
      /* Don´t send error, but use default value. */
      frame_len = DEFAULT_PACKETISATION_TIME;
   } 
   else
   {
      frame_len = nFrameLen;
   } 

   nCurrPacketTime = frame_len;

   return IFX_SUCCESS;
} /* VOIP_SetFrameLen() */


/**
   Starts the encoder/decoder.

   \param nDataCh  - data channel

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
IFX_return_t VOIP_StartCodec(IFX_int32_t nDataCh)
{
   IFX_int32_t data_fd = -1;


   if ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   data_fd = VOIP_GetFD_OfCh(nDataCh);

   if ((IFX_TRUE == rgoDataChStat[nDataCh].fStartCodec)
      && (IFX_FALSE == rgoDataChStat[nDataCh].fCodecStarted))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
           ("Start encoder/decoder on dev %d, data ch %d\n",
           (int) data_fd, (int) nDataCh));

#if 0
#ifdef VERSION_1_2

      /* Set coder type */
      if (IFX_ERROR == ioctl(data_fd, IFX_TAPI_ENC_TYPE_SET,
                             rgoDataChStat[nDataCh].nCodecType))
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Setting encoder type %d failed. (File: %s, line: %d)\n",
               rgoDataChStat[nDataCh].nCodecType, __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Set packetisation time */
      if (IFX_ERROR == ioctl(data_fd, IFX_TAPI_ENC_FRAME_LEN_SET,
                             rgoDataChStat[nDataCh].nFrameLen))
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Setting frame length (packetisation time) %d failed."
               " (File: %s, line: %d)\n", rgoDataChStat[nDataCh].nFrameLen,
               __FILE__, __LINE__));
         return IFX_ERROR;
      }

#endif /* VERSION_1_2 */
#endif

      /* Start encoding */
      if (IFX_ERROR == ioctl(data_fd, IFX_TAPI_ENC_START, 0))
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Start encoder failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Start decoding */
      if (IFX_ERROR == ioctl(data_fd, IFX_TAPI_DEC_START, 0))
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Start decoder failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }

      rgoDataChStat[nDataCh].fCodecStarted = IFX_TRUE;
   } /* if */

   return IFX_SUCCESS;
} /* VOIP_StartCodec() */


/**
   Stops the encoder/decoder.

   \param nDataCh  -  data channel

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_return_t VOIP_StopCodec(IFX_int32_t nDataCh)
{
   IFX_int32_t data_fd = -1;


   if ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
           __FILE__, __LINE__));
      return IFX_ERROR;
   }

   data_fd = VOIP_GetFD_OfCh(nDataCh);

   if (IFX_TRUE == rgoDataChStat[nDataCh].fCodecStarted)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
           ("Stop encoder/decoder on dev %d, data ch %d\n",
           (int) data_fd, (int) nDataCh));

      /* Stop encoding */
      if (IFX_ERROR == ioctl(data_fd, IFX_TAPI_ENC_STOP, 0))
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Stop encoder failed. (File: %s, line: %d)\n",
              __FILE__, __LINE__));
         return IFX_ERROR;
      }

      /* Stop decoding */
      if (IFX_ERROR == ioctl(data_fd, IFX_TAPI_DEC_STOP, 0))
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Stop decoder failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         return IFX_ERROR;
      }

      rgoDataChStat[nDataCh].fCodecStarted = IFX_FALSE;
   }

   return IFX_SUCCESS;
} /* VOIP_StopCodec() */


/**
   Set codec flag if coder can be started or not.

   \param nDataCh - data channel
   \param fStartCodec - IFX_TRUE set flag to IFX_TRUE,
                        or IFX_FALSE is IFX_FALSE

   \return Flag is set to proper value
*/
IFX_void_t VOIP_SetCodecFlag(IFX_int32_t nDataCh, IFX_boolean_t fStartCodec)
{
   if ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return;
   }

   rgoDataChStat[nDataCh].fStartCodec = fStartCodec;
} /* VOIP_SetCodecFlag() */


/**
   Return free data channel.

   \return free data channel or NO_FREE_DATA_CH
*/
IFX_int32_t VOIP_GetFreeDataCh(IFX_void_t)
{
   IFX_uint32_t i = 0;
   IFX_int32_t free_ch = NO_FREE_DATA_CH;


   for (i = 0; i < MAX_SYS_CH_RES; i++)
   {
      if (IFX_FALSE == rgoDataChStat[i].fMapped)
      {
         free_ch = i;
         break;
      }
   }

   return free_ch;
} /* VOIP_GetFreeDataCh() */


/**
   Free mapped data channel.

   \param nDataCh - data channel number

   \return IFX_SUCCESS - data channel is freed, otherwise IFX_ERROR
*/
IFX_return_t VOIP_FreeDataCh(IFX_int32_t nDataCh)
{
   if ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   rgoDataChStat[nDataCh].fMapped = IFX_FALSE;

   return IFX_SUCCESS;
} /* VOIP_FreeDataCh() */


/**
   Reserve data channel as mapped.

   \param nDataCh - data channel number

   \return IFX_SUCCESS - data channel is reserved, otherwise IFX_ERROR
*/
IFX_return_t VOIP_ReserveDataCh(IFX_int32_t nDataCh)
{
   if ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (IFX_TRUE == rgoDataChStat[nDataCh].fMapped)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Data ch %d, already reserved\n", (int) nDataCh));
      return IFX_ERROR;
   }

   rgoDataChStat[nDataCh].fMapped = IFX_TRUE;

   return IFX_SUCCESS;
} /* VOIP_GetFreeDataCh() */


/**
   Return VOIP socket according to channel idx.

   \param nChIdx - channel index

   \return socket or NO_SOCKET on error
*/
IFX_int32_t VOIP_GetSocket(IFX_int32_t nChIdx)
{
   if ((0 > nChIdx) || (MAX_SYS_CH_RES < nChIdx))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return NO_SOCKET;
   }

   return rgnSockets[nChIdx];
} /* VOIP_GetSocket() */


/**
   Map phone channel to data channel.

   \param nDataCh     - target data channel
   \param nAddPhoneCh - which phone channel to add
   \param fDoMapping - IFX_TRUE do mapping, IFX_FALSE do unmapping
   \param pConf      - pointer to conference

   \return IFX_SUCCESS if ok, otherwise IFX_ERROR
*/
IFX_return_t VOIP_MapPhoneToData(IFX_int32_t nDataCh,
                                 IFX_int32_t nAddPhoneCh,
                                 IFX_boolean_t fDoMapping,
                                 CONFERENCE_t* pConf)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_TAPI_MAP_DATA_t datamap;
   IFX_int32_t fd_ch = -1;


   if ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh)
       || (0 > nAddPhoneCh) || (MAX_SYS_LINE_CH < nAddPhoneCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
           __FILE__, __LINE__));
      return IFX_ERROR;
   }

   memset(&datamap, 0, sizeof(IFX_TAPI_MAP_DATA_t));

   /* Get file descriptor for this channel */
   fd_ch = VOIP_GetFD_OfCh(nDataCh);

   datamap.nDstCh = nAddPhoneCh;
   if (IFX_TRUE == fDoMapping)
   {
      /* Map phone channel to data channel */
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Map phone ch %d to data ch %d\n",
           (int)nAddPhoneCh, (int)nDataCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_DATA_ADD, (IFX_int32_t) &datamap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Error mapping phone ch %d to data ch %d using fd %d. \
	           (File: %s, line: %d)\n",
		        (int) nAddPhoneCh, (int) nDataCh, (int) fd_ch,
		         __FILE__, __LINE__));
         return IFX_ERROR;
      }

      if (IFX_NULL != pConf)
      {
         /* Add new pair of mapping */
         pConf->MappingTable[pConf->nMappingCnt].nCh = nDataCh;
         pConf->MappingTable[pConf->nMappingCnt].nAddedCh = nAddPhoneCh;
         pConf->MappingTable[pConf->nMappingCnt].nMappingType = PHONE_DATA;
         pConf->MappingTable[pConf->nMappingCnt].fMapping = IFX_TRUE;
         pConf->nMappingCnt++;
      }
   }
   else
   {
      /* Unmap phone channel to data channel */
      TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Unmap phone ch %d from data ch %d\n",
           (int)nAddPhoneCh, (int)nDataCh));

      ret = ioctl(fd_ch, IFX_TAPI_MAP_DATA_REMOVE, (IFX_int32_t) &datamap);
      if (IFX_ERROR == ret)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Error unmapping phone ch %d from data ch %d using fd %d. \
	           (File: %s, line: %d)\n",
		        (int) nAddPhoneCh, (int) nDataCh, (int) fd_ch,
		         __FILE__, __LINE__));
         return IFX_ERROR;
      }
   }

   return ret;
} /* VOIP_MapPhoneToData() */


/**
   Initialize socket for incoming voice packets

   \param pProgramArg - pointer to program arguments
   \param nChIdx - channel index

   \return socket number or NO_SOCKET if error.
*/
IFX_int32_t VOIP_InitUdpSocket(PROGRAM_ARG_t* pProgramArg, IFX_int32_t nChIdx)
{
   IFX_int32_t s;
   struct sockaddr_in my_addr;


   if ((IFX_NULL == pProgramArg) || (0 > nChIdx) || (MAX_SYS_CH_RES < nChIdx))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return NO_SOCKET;
   }

   rgnSockets[nChIdx] = NO_SOCKET;

   s = socket(PF_INET, SOCK_DGRAM, 0);
   if (NO_SOCKET == s)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("InitUdpSocket: Can't create UDP socket, %s. \
           (File: %s, line: %d)\n", 
            strerror(errno), __FILE__, __LINE__));
      return NO_SOCKET;
   }

   bzero((IFX_char_t*) &my_addr, (IFX_int32_t) sizeof(my_addr));
   my_addr.sin_family = AF_INET;
   my_addr.sin_port = htons(UDP_PORT + nChIdx);

	if (pProgramArg->oMy_IP_Addr.sin_addr.s_addr)
	{
      my_addr.sin_addr.s_addr = pProgramArg->oMy_IP_Addr.sin_addr.s_addr;
	}
   else
	{
      my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

   if (-1 == (bind(s, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("InitUdpSocket: Can't bind to port, %s. (File: %s, line: %d)\n", 
            strerror(errno), __FILE__, __LINE__));
      return NO_SOCKET;
   }

   rgnSockets[nChIdx] = s;
#ifndef VXWORKS
   /* make the socket non blocking */
   fcntl(s, F_SETFL, O_NONBLOCK);
#endif /* VXWORKS */

   return s;
} /* VOIP_InitUdpSocket() */


/**
   Handles data from socket

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pConn  - pointer to phone connection
   \param fHandleData - IFX_TRUE we handle data, if IFX_FALSE QoS 
                        is handling data.
   \param nCID_NameIdx - CID index where name is located

   \return IFX_SUCCESS on ok otherwise IFX_ERROR
*/
IFX_return_t VOIP_HandleSocketData(CTRL_STATUS_t* pCtrl,
                                   PHONE_t* pPhone,
                                   CONNECTION_t* pConn,
                                   IFX_boolean_t fHandleData,
                                   IFX_int32_t nCID_NameIdx)
{
   static IFX_char_t buf[400];
   IFX_int32_t size = 0;
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t event = 0, from_ch = 0;
   COMM_MSG_t msg;
   

   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone) || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   size = sizeof(pConn->oConnPeer.oRemote.oToAddr);

   /* got data from socket */
   ret = recvfrom(pConn->nUsedSocket,
                  buf, sizeof(buf), 0,
                 (struct sockaddr *) &pConn->oConnPeer.oRemote.oToAddr,
                 /* Must be like that, otherwise you get compiler
                    warnings */
                 (int *) (IFX_int32_t) &size);

   if (0 > ret)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("No data read from socket, %s. (File: %s, line: %d)\n", 
            strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }
   else if (0 == ret)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("No data read from socket.\n"));
      return IFX_ERROR;
   }
   else if (sizeof(msg) != ret)
   {
      if (IFX_TRUE == fHandleData)
      {
         /* Don`t use QoS so application handle the data */
         ret = write(pConn->nUsedCh_FD, buf, ret);
         if (0 > ret)
         {
            TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
                 ("Error writing data to device, %s. (File: %s, line: %d)\n", 
                  strerror(errno), __FILE__, __LINE__));
            return IFX_ERROR;
         }
         else if (0 == ret)
         {
            TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("No data written to device.\n"));
            return IFX_SUCCESS;
         }
         return IFX_ERROR;
      }
   }

   /* When we reach this point, we received our message, hopefully :-) */
   memcpy(&msg, &buf[0], sizeof(msg));

   if ((COMM_MSG_START_FLAG == msg.nMarkStart)
       && (COMM_MSG_END_FLAG == msg.nMarkEnd))
   {

      /* ch-nr of calling remote phone */
      from_ch = msg.nCh;
      /* event to send to local phone */
      event = msg.nAction;

      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Calling %s\n",
           pPhone->oCID_Msg.message[nCID_NameIdx].string.element));

      /* Status from where we got command */
      pPhone->nDstState = event;
      pConn->fType = EXTERN_VOIP_CALL;
      pConn->oConnPeer.oRemote.nCh = from_ch;
      pConn->oConnPeer.oRemote.oToAddr.sin_port = htons(UDP_PORT + from_ch);

      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("%s: rcvd 0x%02X from %s:%d\n",
           pPhone->oCID_Msg.message[nCID_NameIdx].string.element, buf[0],
           inet_ntoa(pConn->oConnPeer.oRemote.oToAddr.sin_addr),
           ntohs(pConn->oConnPeer.oRemote.oToAddr.sin_port)));

      return IFX_SUCCESS;
   }
   else
   {
      /* If we came here then receive wrong message */
      return IFX_ERROR;
   }
} /* VOIP_HandleSocketData() */


/**
   Handles data from chip

   \param pCtrl - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param nDataCh_FD - data channel file descriptor
   \param pConn - pointer to phone connection
   \param fHandleData - IFX_TRUE we handle data, if IFX_FALSE QoS 
                        is handling data.

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
IFX_return_t VOIP_HandleData(CTRL_STATUS_t* pCtrl,
                             PHONE_t* pPhone,
                             IFX_int32_t nDataCh_FD,
                             CONNECTION_t* pConn,
                             IFX_boolean_t fHandleData)
{
   IFX_return_t ret = IFX_SUCCESS;
   IFX_int32_t len = 0;
   static IFX_char_t buf[400];


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Get data from driver - DTMF, talking on phone */
   ret = read(nDataCh_FD, buf, sizeof(buf));

   if (0 > ret)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("No data read from device. (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (0 < ret)
   {
      switch (pPhone->nStatus)
      {
         case  US_ACTIVE_RX:
         case  US_ACTIVE_TX:
            if (LOCAL_CALL == pConn->fType)
            {
               /* Send data to local phone only if no conference exists,
                  otherwise we have channel mapping */
               if (NO_CONFERENCE == pPhone->nConfIdx)
               {
                  if (IFX_TRUE == fHandleData)
                  {
                     /* Don`t use QoS so application handle the data */

                     /* Local call */
                     len = write(pConn->oConnPeer.oLocal.pPhone->nDataCh_FD,
                                 buf, ret);
                     if (0 > len)
                     {
                        TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                             ("Error writing data to device %d, %s. \
                             (File: %s, line: %d)\n",
                             (int) pConn->oConnPeer.oLocal.pPhone->nDataCh_FD,
                             strerror(errno), __FILE__, __LINE__));
                        return IFX_ERROR;
                     }
                     else if (0 == len)
                     {
                        TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
                             ("No data written to device %d.\n",
                             (int) pConn->oConnPeer.oLocal.pPhone->nDataCh_FD));
                        return IFX_ERROR;
                     }
                  }
                       
                  return IFX_SUCCESS;
               }
            }
            else if (EXTERN_VOIP_CALL == pConn->fType)
            {
               if (IFX_TRUE == fHandleData)
               {
                  /* Don`t use QoS so application handle the data */

                  /* External call */
                  len = sendto(pConn->nUsedSocket, buf, ret, 0,
                              (struct sockaddr *) &pConn->oConnPeer.oRemote.oToAddr,
                              sizeof(pConn->oConnPeer.oRemote.oToAddr));

                  if (ret != len)
                  {
                     TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
                          ("Error calling sendto(), %s, socket %d, data ch %d."
                          " (File: %s, line: %d)\n", 
                          strerror(errno), (int) pConn->nUsedSocket,
                          (int) pConn->nUsedCh, __FILE__, __LINE__));
                     return IFX_ERROR;
                  }
               }
            }
         break;
      default:
         /* Nothing to do*/
         break;
      } /* switch */
   } /* if */

   return IFX_SUCCESS;
} /* VOIP_HandleData() */


/**
   Get file descriptor of device for data channel.
  
   \param nDataCh - data channel

   \return device connected to this channel or NO_DEVICE_FD if none
*/
IFX_int32_t VOIP_GetFD_OfCh(IFX_int32_t nDataCh)
{
   if ((0 > nDataCh) || (MAX_SYS_CH_RES < nDataCh))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, 
           ("Invalid input argument(s). (File: %s, line: %d)\n", 
            __FILE__, __LINE__));
      return NO_DEVICE_FD;
   }

   return Common_GetDeviceOfCh(nDataCh);
} /* VOIP_GetFD_OfCh() */
