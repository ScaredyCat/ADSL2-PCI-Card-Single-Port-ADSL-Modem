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
   Module      : tapi_demo.h
   Date        : 2003-03-24
   Description : This file contains the implementation of the functions for
                 the tapi demo
   \file

   \remarks
      Demonstrates a simple PBX with voice over IP with Linux.

   \note Changes:
      Date: 28.11.2005 it is working on new board with new drivers, kernel.
                       CID structure changed.
                       Removed EXCHFD_SUPPORT, its obsolete.
                       Extracted QOS, TAPI SIGNALLING into different files.
      Date: 15.12.2005 Added conference support.
      Date: 21.04.2006 Major changes, added PCM, Features support. Also added
                       Event handling (bitfields or messages).
*******************************************************************************/


/* ============================= */
/* Includes                      */
/* ============================= */

#include "tapidemo.h"
#include "qos.h"
#include "tapi_signal.h"
#include "cid.h"
#include "conference.h"
#include "abstract.h"
#include "event_handling.h"
#include "pcm.h"
#include "voip.h"
#include "analog.h"
#include "feature.h"
#ifdef FXO
#include "../../duslic-0.0.2/src/duslic_io.h"
#include "../../danube_daa-0.3.2.1/src/drv_cpc5621_interface.h"
#include "duslic_setting.h"
#include "FXO_600OHM.BYT"
#endif /* FXO */

/* ============================= */
/* Debug interface               */
/* ============================= */

CREATE_TRACE_GROUP(TAPIDEMO);

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */
#define VMMC_TEST
#undef VMMC_TEST

#define VMMC_CODER_TEST
#undef VMMC_CODER_TEST
// #define VMMC_TONE_TEST
#undef VMMC_TONE_TEST

#define VMMC_PCM_TEST
#undef VMMC_PCM_TEST

#ifdef FXO
/* Must work with a global var here, since event structure is out of changeable
   scope :-/ */
IFX_boolean_t bFXO_LocalRinging = IFX_FALSE;
IFX_boolean_t bFXO_CallActive = IFX_FALSE;
#endif /* FXO */


#ifdef VMMC_PCM_TEST
/** At the moment we have only one PCM highway on board */
static IFX_int32_t nPCM_Highway = 0;

/** ALaw, uLaw, Linear and 8 or 16 bit. */
static IFX_TAPI_PCM_RES_t ePCM_Resolution = IFX_TAPI_PCM_RES_ALAW_8BIT;

/** Sample rate, minimum value is 512 kHz and can increase by following step:
   n*512kHz, n = 1..16 */
static IFX_int32_t nPCM_Rate = 2048;



/** Structure representing timeslot (RX, TX) */
typedef struct _TIMESLOT_t
{
   IFX_int32_t nRX;
   IFX_int32_t nTX;
} TIMESLOT_t;



/** Each PCM uses two timeslots, one for RX and other for TX.
    NOTICE: On Slave board this values are swapped. So what is RX on Master
            board is TX on Slave board and vice versa. And also this sequence
            is valid only for caller, not for callee. Callee will get from
            which channel caller is calling and will take according values. */
const TIMESLOT_t IFX_PCM_TIMESLOT_PAIRS[4] =
{
   {0, 1},
   {2, 3},
   {4, 5},
   {6, 7}
};

#endif /* VMMC_PCM_TEST */

#define VMMC_AUDIO_TEST
#undef  VMMC_AUDIO_TEST

/* ============================= */
/* Local variable definition     */
/* ============================= */

/** Array of CID structures (holding name, numbers, dates, ...) */
static IFX_TAPI_CID_MSG_ELEMENT_t CidMsgData[MAX_SYS_CH_RES][CID_ELEM_COUNT];

/** Strings describing each state */
static const char* STATE_NAMES[] =
{
   "US_NOTHING",
   "US_READY",
   "US_ACTIVE_RX",
   "US_ACTIVE_TX",
   "US_INCCALL",
   "US_HOOKOFF",
   "US_DIALTONE",
   "US_BUSYTONE",
   "US_DIALING",
   "US_CALLING",
   "US_RINGBACK",
   "US_RINGING",
   "US_BUSY",
   "US_CONFERENCE"
};

/** Holding program arguments */
static PROGRAM_ARG_t oProgramArg;

/** Holding information about each connection */
static CTRL_STATUS_t oCtrlStatus;

/** Program version */
static const char* const ABC_PBX_VERSION = "0.9.0.4";

#if defined(LINUX)
/** program options */
/* DO NOT FORGET TO CHANGE THE OPTION_PARAMETERS[] !!! */

static struct option programOptions[] =
{
   /* name, has_arg, *flag, val */
   {"help"         , 0, 0, 'h'},
   {"debug"        , 1, 0, 'd'},
   {"ip-address"   , 1, 0, 'i'},
   {"wait"         , 0, 0, 'w'},
   {"qos"          , 0, 0, 'q'},
   {"cid"          , 1, 0, 'c'},
   {"conferencing" , 1, 0, 'k'},
   {"pcm"          , 1, 0, 'p'},
   {"encoder type" , 1, 0, 'e'},
   {"packetization time", 1, 0, 'f'},
   {0              , 0, 0,  0 }
};

/** Mask which parameters are handled */
const IFX_char_t OPTION_PARAMETERS[] = "hd:i:wc:qc:k:p:e:f:";
#endif /* LINUX */

#if defined(LINUX)
/** Strings describing command line parameters */
static const IFX_char_t helpItems[][200] =
{
   {"Help screen"},
   {"Set debug level:\n"
     "    4 - OFF (default) \n"
     "    3 - HIGH\n"
     "    2 - NORMAL\n"
     "    1 - LOW"},
   {"Specify network IP address of the board for connections, "
    "by default retrieve it automatically"},
   {"Wait after each state machine step (default is OFF - 0)"},
   {"Qos support : packets redirected via udp redirector (default is OFF - 0)"},
   {"CID support (default OFF - not used): showing CID on phone LCD:\n"
     "    0 - TELCORDIA, Belcore, USA\n"
     "    1 - ETSI FSK, Europe\n"
     "    2 - ETSI DTMF, Europe\n"
     "    3 - SIN BT, Great Britain\n"
     "    4 - NTT, Japan"},
   {"Conferencing support (default is ON - 1): more than two phone connection."
    " HASH - '#' is used to start another call."},
   {"PCM (default is OFF): using two boards connected over PCM.\n"
    "    Start/stop uses ethernet, but voice stream uses PCM:\n"
    "    - One board is master <-p m>,\n"
    "    - The other one is slave <-p s>"},
   {"Encoder type (default is IFX_TAPI_ENC_TYPE_MLAW):\n"
    " 1 - IFX_TAPI_ENC_TYPE_G723_63 [G723, 6.3 kBit/s]\n"
    " 2 - IFX_TAPI_ENC_TYPE_G723_53 [G723, 5.3 kBit/s]\n"
    " ..."},
/*   {" 7 - IFX_TAPI_ENC_TYPE_G729_AB [G729 A and B (silence compression),"
      " 8 kBit/s]\n"
    " 8 - IFX_TAPI_ENC_TYPE_MLAW [G711 µ-law, 64 kBit/s]\n"
    " 9 - IFX_TAPI_ENC_TYPE_ALAW [G711 A-law, 64 kBit/s]"},
   {"12 - IFX_TAPI_ENC_TYPE_G726_16 [G726, 16 kBit/s]\n"
    "13 - IFX_TAPI_ENC_TYPE_G726_24 [G726, 24 kBit/s]\n"
    "14 - IFX_TAPI_ENC_TYPE_G726_32 [G726, 32 kBit/s]"},
   {"15 - IFX_TAPI_ENC_TYPE_G726_40 [G726, 40 kBit/s]\n"
    "16 - IFX_TAPI_ENC_TYPE_G729_E [G729 E, 11.8 kBit/s]\n"
    "17 - IFX_TAPI_ENC_TYPE_ILBC_133 [iLBC, 13.3 kBit/s]"},
   {"18 - IFX_TAPI_ENC_TYPE_ILBC_152 [iLBC, 15.2 kBit/s]"},
*/   {"Packetisation time (default is 10 ms): Length of frames in milliseconds"},
   {""}
};
#endif /* LINUX */


/* ============================= */
/* Local function declaration    */
/* ============================= */
static IFX_return_t ABC_PBX_InitBoard(IFX_void_t);
static IFX_int32_t ABC_PBX_Help(IFX_void_t);
static IFX_return_t ABC_PBX_Setup(PROGRAM_ARG_t* pProgramArg,
                                  CTRL_STATUS_t* pCtrl);
static IFX_return_t ABC_PBX_StateMachine(CTRL_STATUS_t* pCtrl);
static IFX_int32_t ABC_PBX_StateTrans(CTRL_STATUS_t* pCtrl,
                                      PHONE_t* pPhone,
                                      IFX_int32_t nState,
                                      CONNECTION_t* pConn);
static IFX_int32_t ABC_PBX_HandleState(CTRL_STATUS_t* pCtrl,
                                       PHONE_t* pPhone,
                                       CONNECTION_t* pConn);
static IFX_return_t ABC_PBX_FillAddrByNumber(CTRL_STATUS_t* pCtrl,
                                             PHONE_t* pPhone,
                                             CONNECTION_t* pConn,
                                             IFX_int32_t nPhoneNumber,
                                             IFX_boolean_t fIsPCM_Num);
#ifdef LINUX
static IFX_int32_t ABC_PBX_SetDefaultAddr(PROGRAM_ARG_t* pProgramArg);
static IFX_void_t ABC_PBX_ReadOptions(IFX_int32_t argc,
                                      IFX_char_t* argv[],
                                      PROGRAM_ARG_t* pProgramArg);
#endif /* LINUX */


#ifndef LINUX
extern gethostname();
extern inet_aton();
extern sendto();
extern recvfrom();
extern socket();
extern bind();
#endif /* LINUX */


/* ============================= */
/* Local function definition     */
/* ============================= */

#ifdef VMMC_TEST
/* Test ONLY */
/* Number of channels */
const IFX_int32_t MAX_CH = 2;
int VMMC_Voice_Test(void)
{
   fd_set rfds, trfds;
   IFX_int32_t fdcfg, fd[MAX_CH], i;
   IFX_int32_t width = 0;
   IFX_char_t buf[400];
   IFX_int32_t rlen = 0;
   IFX_int32_t wlen = 0;

   FD_ZERO(&rfds);
   /* open device file descriptor */
   /* fdcfg = open(CTRL_DEVICE, O_RDWR);
   FD_SET (fdcfg, &rfds);
   width = fdcfg; */
   /* open channel file descriptor for channel 0 */
   //fd[0] = open(DEVICE_NAME_1, O_RDWR, 0x644);
   /* get file descriptor for this data channel */
   fd[0] = Common_GetDeviceOfCh(0);
   FD_SET(fd[0], &rfds);
   width = fd[0];
   if (width < fd[0])
   {
      width = fd[0];
   } /* if */
   //fd[1] = open(DEVICE_NAME_2, O_RDWR, 0x644);
   /* get file descriptor for this data channel */
   fd[1] = Common_GetDeviceOfCh(1);
   /* open channel file descriptor for channel 1 */
   FD_SET(fd[1], &rfds);
   if (width < fd[1])
   {
      width = fd[1];
   } /* if */

   /* Loop where reading voice packets from device and write it to
      another device */

   while (1)
   {
      /* now wait for events and data */
      memcpy((void*)&trfds, (void*)&rfds, sizeof(fd_set));
      select(width + 1, &trfds, IFX_NULL, IFX_NULL, IFX_NULL);

      /* wait for all channels */
      for (i = 0; i < MAX_CH; i++)
      {
         if (FD_ISSET(fd[i], &trfds))
         {
            /* we received data from local phone */
            rlen = read(fd[i], buf, sizeof(buf));
            if (0 < rlen)
            {
               /* send it to other local phone */
               if (0 == i)
               {
                  wlen = write(fd[1], buf, rlen);
                  //printf("o");
               }
               else
               {
                  wlen = write(fd[0], buf, rlen);
                  //printf("O");
               }
               if (0 >= wlen)
               {
                  /* Nothing was written == 0, or error occured -1 */
               }
            }
         }
      }
   } /* while */

   /* restore every changes made */

   /* close all open fds, sockets */
   //close(fd[0]);
   //close(fd[1]);

   return 0;
}
#endif /* VMMC_TEST */


#ifdef VMMC_TONE_TEST
IFX_return_t VMMC_Tone_Test (CTRL_STATUS_t* pCtrl)
{
   fd_set rfds, trfds;
   IFX_int32_t rwd = 0;
   IFX_return_t ret = IFX_SUCCESS;
   /* wait forever by default */
   struct timeval *p_time_out = IFX_NULL;
   /* hook status, digit status, line status is only checked on
      analog lines, that mean phone channel */
   IFX_TAPI_CH_STATUS_t stat[MAX_SYS_CH_RES];
   IFX_int32_t i = 0;
   STATE_MACHINE_STATES_t state = US_NOTHING;
   IFX_int32_t action_flag = 0;
   IFX_int32_t event_flag = 0;
   CONNECTION_t *pCon = IFX_NULL;
   IFX_int32_t fd_dev_ctrl = -1;
   IFX_int32_t fd_ch = -1;
   IFX_int32_t fd_data_ch = -1;
   IFX_int32_t fd_phone_ch = -1;
   IFX_int32_t ready = 0, digit = 0;


   /* check input arguments */
   if (pCtrl == IFX_NULL)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }


   ioctl(4, IFX_TAPI_RING_START, NO_PARAM);
   sleep (5);
   ioctl(4, IFX_TAPI_RING_STOP, NO_PARAM);
   sleep (5);

 /* Wait for all channels */
  #if 0
     ioctl(3, IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_DISABLED);
     ioctl(3, IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_STANDBY);
     ioctl(3, IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_ACTIVE);

     sleep (5);

     ioctl(3, IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_DISABLED);
     ioctl(3, IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_STANDBY);
     ioctl(3, IFX_TAPI_LINE_FEED_SET, IFX_TAPI_LINE_FEED_ACTIVE);

     printf ("VMMC_Tone_Test: start Busy Tone on ch 0 \n");
     ioctl(4, IFX_TAPI_TONE_BUSY_PLAY, NO_PARAM);
     sleep (10);
     ioctl(4, IFX_TAPI_TONE_STOP, NO_PARAM);
     printf ("VMMC_Tone_Test: Tone stop on ch 0 \n");

     sleep (5);

     printf ("VMMC_Tone_Test: start Busy Tone on ch 1 \n");
     ioctl(5, IFX_TAPI_TONE_BUSY_PLAY, NO_PARAM);
     sleep (10);
     ioctl(5, IFX_TAPI_TONE_STOP, NO_PARAM);
     sleep (2);

#endif
#if 0
	 printf ("VMMC_Tone_Test: start Dial Tone \n");
     ioctl(4, IFX_TAPI_TONE_DIALTONE_PLAY, NO_PARAM);
     sleep (20);
     ioctl(4, IFX_TAPI_TONE_STOP, NO_PARAM);
     sleep (2);

     printf ("VMMC_Tone_Test: start Ringback Tone \n");
     ioctl(4, IFX_TAPI_TONE_RINGBACK_PLAY, NO_PARAM);
     sleep (20);
     ioctl(4, IFX_TAPI_TONE_STOP, NO_PARAM);
     sleep (2);
#endif


#if 0
   sleep (120);

   for (i = 0; i < 2; i++)
   {
      /* get file descriptor for this channel */
      fd_ch = Common_GetDeviceCh(i);
      pCon = ABSTRACT_GetCON_OfDataCh(pCtrl, i);
      if (IFX_NULL == pCon)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("pCon not found for data ch %d. "
      		"(File: %s, line: %d)\n",
      		(int) i, __FILE__, __LINE__));
         break;
      }

     printf ("VMMC_Tone_Test: get fd_phone_ch\n");
     fd_phone_ch = ABSTRACT_GetDEV_OfPhoneCh(pCtrl, pCon->nPhoneCh);

     ioctl(fd_phone_ch, IFX_TAPI_RING_STOP, NO_PARAM);


   } /* for */
#endif


   printf ("VMMC_Tone_Test: exit\n");
   return IFX_SUCCESS;

}
#endif /* VMMC_TONE_TEST */


#ifdef VMMC_CODER_TEST

IFX_return_t VMMC_Coder_Test (CTRL_STATUS_t* pCtrl)
{
   fd_set rfds, trfds;
   IFX_int32_t rwd = 0;
   IFX_return_t ret = IFX_SUCCESS;
   /* Wait forever by default */
   struct timeval* p_time_out = IFX_NULL;
   /* Hook status, digit status, line status is only checked on
      analog lines, that mean phone channel */
   /*IFX_TAPI_CH_STATUS_t stat[MAX_SYS_CH_RES];*/
   STATE_MACHINE_STATES_t state = US_NOTHING;
   IFX_int32_t i = 0;
   IFX_int32_t action_flag = 0;
   IFX_int32_t event_flag = 0;
   PHONE_t* phone = IFX_NULL;
   CONNECTION_t* pConn = IFX_NULL;


   if (IFX_NULL == pCtrl)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   sleep (15);

   printf ("tapidemo: calling decode start\n");
   ioctl(4, IFX_TAPI_DEC_START, 0);
   sleep (5);
   printf ("tapidemo: calling encode start\n");
   ioctl(4, IFX_TAPI_ENC_START, 0);
   sleep (5);
   printf ("tapidemo: calling encode stop\n");
   ioctl(4, IFX_TAPI_ENC_STOP, 0);
   sleep (5);
   printf ("tapidemo: calling decode stop\n");
   ioctl(4, IFX_TAPI_DEC_STOP, 0);
   sleep (5);
   printf ("tapidemo: done\n");

#if 0
   for (i = 0; i < 1; i++)
   {
      phone = ABSTRACT_GetPHONE_OfDataCh(pCtrl, i, &pConn);

      /* Some data channels are free not connected to phone */
      if (IFX_NULL == pConn)
      {
	     printf ("VMMC_Coder_Test: pConn is NULL\n");
      }
	  else
	  {
	     printf ("VMMC_Coder_Test: Calling VOIP_StartCodec\n");
         VOIP_StartCodec (pConn->nUsedCh);
      }
   }
#endif

   return IFX_SUCCESS;
}

#endif // VMMC_CODER_TEST


#ifdef FXO
/**
   Initialize DUSLIC channels (currently only channel 0 used)

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
static IFX_return_t DUSLIC_InitChannel(void)
{
   DUSLIC_PCM_CONFIG   pcmCfg = {0};
   IFX_int32_t fd;

   fd = Common_GetDusOfCh(0);

   /* Config PCM spec. */
   pcmCfg.nTimeslotRX = 0;
   pcmCfg.nTimeslotTX = 1;
   pcmCfg.nTxHighway = DS_PCM_HIGHWAY_A;
   pcmCfg.nRxHighway = DS_PCM_HIGHWAY_A;
/*   pcmCfg.nResolution = DS_PCM_LINEAR_16_BIT; */
   pcmCfg.nResolution = DS_PCM_A_LAW_8_BIT;
   pcmCfg.nRate = DS_PCM_SAMPLE_8K;
   pcmCfg.nClockMode = DS_PCM_SINGLE_CLOCK;
   pcmCfg.nTxRxSlope = DS_PCM_TX_RISING | DS_PCM_RX_FALLING;
   pcmCfg.nDriveMode = DS_PCM_DRIVEN_ENTIRE;
   pcmCfg.nShift = DS_PCM_NO_SHIFT;

   if (ioctl(fd, FIO_DUSLIC_PCM_CONFIG, &pcmCfg) < 0)
   {
      printf("%s() - DUSLIC PCM_CONFIG failed!\n", __FUNCTION__);
      return IFX_ERROR;
   }
   if (ioctl(fd, FIO_DUSLIC_PCM_ACTIVE, 1) < 0)
   {
      printf("%s() - DUSLIC PCM_ACTIVE failed!\n", __FUNCTION__);
      return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/**
   Initialize DUSLIC device

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
static IFX_return_t DUSLIC_Init(void)
{
   DUSLIC_IO_INIT init;
   IFX_int32_t fd;

   fd = Common_GetDusCtrlCh();

   memset(&init, 0, sizeof(init));
   init.p_ac_coef = ac_coef;
   init.ac_coef_size = sizeof(ac_coef);
   init.p_dc_coef = dc_coef;
   init.dc_coef_size = sizeof(dc_coef);
   init.p_gen_coef = gen_coef;
   init.gen_coef_size = sizeof(gen_coef);
   init.param = DUS_IO_INIT_DEFAULT;

   /* Init Duslic driver. */
#if 0
   if(ioctl(fd, FIO_DUSLIC_INIT, 0) < 0)
#else
   if(ioctl(fd, FIO_DUSLIC_INIT, (int)&init) < 0)
   {
      printf("%s() - DUSLIC INIT failed!\n", __FUNCTION__);
      return IFX_ERROR;
   }
#endif
   /* Init Duslic channel. */
   return DUSLIC_InitChannel();
}

/**
   Initialize CPC device

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
static IFX_return_t CPC5621_Init(void)
{
   CPC5621_CONFIG cpcCfg={0};
   IFX_int32_t fd;

   fd = Common_GetCpcCtrlCh();

   if (ioctl(fd, FIO_CPC5621_INIT, 0) < 0)
   {
      printf("%s() - CPC5621 INIT failed\n", __FUNCTION__);
      return IFX_ERROR;
    }

    cpcCfg.Cid_tMin  = 2000; //2000;  /* ms */
    cpcCfg.Ring_tMin = 200/*30*/;    /* ms, 2 ~ 3 ring pulse time */
    cpcCfg.Bat_tMin  = 250 /*150*/;    /* ms, 1 Battery pulse time */
    cpcCfg.RingCount = 5;
    if (ioctl(fd, FIO_CPC5621_CONFIG, &cpcCfg) < 0)
    {
       printf("%s() - CPC5621 CONFIG failed\n", __FUNCTION__);
       return IFX_ERROR;
    }
    return IFX_SUCCESS;
}

#define CPC5621_ONHOOK  1
#define CPC5621_OFFHOOK 0

static IFX_return_t CPC5621_SetHookState(int state)
{
   IFX_return_t ret;
   IFX_int32_t fd;
   fd = Common_GetCpcCtrlCh();
   if ((ret = ioctl(fd, FIO_CPC5621_SET_OH, state)) < 0)
   {
     printf ("%s() - CPC set HOOK to %d failed (%d)\n", __FUNCTION__, state, ret);
     return IFX_ERROR;
   }
   return IFX_SUCCESS;
}

/*******************************************************************************
Description:
Arguments:
Return:
*******************************************************************************/
static void PCM_Open(int fd, int ch)
{
    IFX_TAPI_MAP_DATA_t dataMap = {0};
    IFX_TAPI_PCM_CFG_t tmpPCMConfig = {0};

    printf("%s():%d \n", __FUNCTION__, __LINE__);
    /* Add data channel. */
    dataMap.nDstCh = ch;
    dataMap.nChType = IFX_TAPI_MAP_TYPE_PCM;
    dataMap.nRecStart = 0;
    dataMap.nPlayStart = 0;
    ioctl(fd, IFX_TAPI_MAP_DATA_ADD, &dataMap);

    /* Config PCM channel. */
    ioctl(fd, IFX_TAPI_PCM_CFG_SET, &tmpPCMConfig);
    tmpPCMConfig.nTimeslotRX = 0;
    tmpPCMConfig.nTimeslotTX = 1;
#if 1
    tmpPCMConfig.nHighway = 0;
    tmpPCMConfig.nResolution = IFX_TAPI_PCM_RES_ALAW_8BIT;
    tmpPCMConfig.nRate = 2048;
#endif
    ioctl(fd, IFX_TAPI_PCM_CFG_SET, &tmpPCMConfig);

    /* Active PCM channel. */
    if (ioctl(fd, IFX_TAPI_PCM_ACTIVATION_SET, 1) == IFX_ERROR)
       printf("%s():%d - Error\n", __FUNCTION__, __LINE__);
}


/*******************************************************************************
Description:
Arguments:
Return:
*******************************************************************************/
static void PCM_Close(int fd, int ch)
{
    IFX_TAPI_MAP_DATA_t dataMap = {0};

    printf("%s():%d \n", __FUNCTION__, __LINE__);
    /* Add data channel. */
    dataMap.nDstCh = ch;
    dataMap.nChType = IFX_TAPI_MAP_TYPE_PCM;
    dataMap.nRecStart = 0;
    dataMap.nPlayStart = 0;
    ioctl(fd, IFX_TAPI_MAP_DATA_REMOVE, &dataMap);

    /* Active PCM channel. */
    ioctl(fd, IFX_TAPI_PCM_ACTIVATION_SET, 0);
}

static IFX_return_t FXO_ActivateCon(IFX_int32_t nDataCh)
{
   IFX_TAPI_MAP_PHONE_t pcmmap = {0};
   IFX_TAPI_MAP_DATA_t datamap = {0};
   IFX_return_t ret;
   IFX_TAPI_PCM_CFG_t tmpPCMConfig = {0};
   int fd;

   /*ioctl(fd_cpc_ctrl, FIO_CPC5621_SET_CID, 0);*/
   fd = Common_GetDeviceOfCh(nDataCh);

   /* Config PCM channel. */
   tmpPCMConfig.nTimeslotRX = 1;
   tmpPCMConfig.nTimeslotTX = 0;
   tmpPCMConfig.nHighway = 0;
   tmpPCMConfig.nResolution = IFX_TAPI_PCM_RES_ALAW_8BIT;
   tmpPCMConfig.nRate = 2048;
   ioctl(fd, IFX_TAPI_PCM_CFG_SET, &tmpPCMConfig);

   /* Active PCM channel. */
   if (ioctl(fd, IFX_TAPI_PCM_ACTIVATION_SET, 1) == IFX_ERROR)
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, ("%s():%d - Error\n", __FUNCTION__, __LINE__));

    TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("%s() - activate FXO connection ch%i\n",
          __FUNCTION__, (int)nDataCh));

    /* set up voice connection */
    pcmmap.nPhoneCh = nDataCh;
    pcmmap.nChType = IFX_TAPI_MAP_TYPE_PCM;
    ret = ioctl(Common_GetDeviceOfCh(nDataCh), IFX_TAPI_MAP_PHONE_ADD, (IFX_int32_t) &pcmmap);
    datamap.nChType = IFX_TAPI_MAP_TYPE_PCM;
    datamap.nDstCh = nDataCh;
    ret = ioctl(Common_GetDeviceOfCh(nDataCh), IFX_TAPI_MAP_DATA_ADD, (IFX_int32_t) &datamap);
    CPC5621_SetHookState(CPC5621_OFFHOOK);

    return ret;
}

static IFX_return_t FXO_DeactivateCon(IFX_int32_t nDataCh)
{
    IFX_TAPI_MAP_PHONE_t pcmmap;
    IFX_TAPI_MAP_DATA_t datamap = {0};
    IFX_return_t ret;
    int fd;

    fd = Common_GetDeviceOfCh(nDataCh);

    TRACE(TAPIDEMO, DBG_LEVEL_LOW,("%s() - deactivate FXO connection ch%i\n",
          __FUNCTION__, (int)nDataCh));

    /* Active PCM channel. */
    if (ioctl(fd, IFX_TAPI_PCM_ACTIVATION_SET, 0) == IFX_ERROR)
       TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("%s():%d - Error\n", __FUNCTION__, __LINE__));
    /* Tear down voice connection */
    CPC5621_SetHookState(CPC5621_ONHOOK);
    datamap.nChType = IFX_TAPI_MAP_TYPE_PCM;
    datamap.nDstCh = nDataCh;
    ret = ioctl(Common_GetDeviceOfCh(nDataCh), IFX_TAPI_MAP_DATA_REMOVE, (IFX_int32_t) &datamap);
    pcmmap.nPhoneCh = nDataCh;
    pcmmap.nChType = IFX_TAPI_MAP_TYPE_PCM;
    ret = ioctl(Common_GetDeviceOfCh(nDataCh), IFX_TAPI_MAP_PHONE_REMOVE, (IFX_int32_t) &pcmmap);

    return ret;
}

#endif /* FXO */

/**
   Displays program usage with arguments.

   \return status (IFX_SUCCESS or IFX_ERROR)
*/
static IFX_int32_t ABC_PBX_Help(IFX_void_t)
{
#if defined(LINUX)

    IFX_int32_t i = 0;


    TRACE(TAPIDEMO, DBG_LEVEL_LOW,
         ("Program usage. (File: %s, line: %d)\n",
          __FILE__, __LINE__));

    printf("Usage: <application name> [options]\r\n");
    printf("Following options defined:\r\n\n");
    printf("Notice CTRL-C stops all working streams (application)\n\n");
    i = 0;
    while (programOptions[i].name)
    {
        printf("(%d) '--%s' OR '-%c' : %s\n",
               (int)i + 1, programOptions[i].name,
               programOptions[i].val, helpItems[i]);
        i++;
    }

#endif /* LINUX */

    return IFX_SUCCESS;
} /* ABC_PBX_Help() */


#if defined(LINUX)

/**
   Reads program arguments.

   \param nArgCnt     - number of arguments
   \param pArgv       - array of arguments
   \param pProgramArg - pointer to program arguments structure

   \return
*/
static IFX_void_t ABC_PBX_ReadOptions(IFX_int32_t nArgCnt,
                                      IFX_char_t* pArgv[],
                                      PROGRAM_ARG_t* pProgramArg)
{
   const IFX_char_t* const PCM_FLAG_MASTER = "m";
   const IFX_char_t* const PCM_FLAG_SLAVE = "s";

   IFX_int32_t option_index = 0, option = 0;
   IFX_int32_t arg_value = 0;


   TRACE(TAPIDEMO, DBG_LEVEL_LOW,
        ("Reads program arguments. (File: %s, line: %d)\n",
         __FILE__, __LINE__));

   if ((IFX_NULL == pProgramArg) || (IFX_NULL == pArgv))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return;
   }

   /* Set some default arguments */

   /* Reset set parameters */
   memset(pProgramArg, 0, sizeof(PROGRAM_ARG_t));
   /* Set default IP-Address  */
   ABC_PBX_SetDefaultAddr(pProgramArg);

   /* Turn conferencing on */
   pProgramArg->oArgFlags.nConference = 1;

   /* Trace level is set to off by default */
   SetTraceLevel(TAPIDEMO, DBG_LEVEL_OFF);

   if (1 >= nArgCnt)
   {
      /* None arguments passed to program */
      return;
   }

   while((option = getopt_long(nArgCnt, pArgv,
                               OPTION_PARAMETERS, programOptions,
                               (int *) option_index)) != -1)
   {
      switch (option)
      {
         case 'h':
               pProgramArg->oArgFlags.nHelp = 1;
               break;
         case 'd':
               arg_value = atoi(optarg);
               if ((arg_value > DBG_LEVEL_OFF)
                   || (arg_value < DBG_LEVEL_LOW))
               {
                  arg_value = DBG_LEVEL_OFF;
               }
               SetTraceLevel(TAPIDEMO, arg_value);
               break;
         case 'i':
               inet_aton(optarg, &pProgramArg->oMy_IP_Addr.sin_addr);
               break;
         case 'w':
               pProgramArg->oArgFlags.nWait = 1;
               break;
         case 'q':
               /* QOS */
               QOS_TurnServiceOn(pProgramArg);
               break;
         case 'c':
               /* CID */
               pProgramArg->oArgFlags.nCID = 1;
               arg_value = atoi(optarg);
               CID_SetStandard(arg_value);
               break;
         case 'k':
               /* Conferencing */
               arg_value = atoi(optarg);
               if (0 == arg_value)
               {
                  pProgramArg->oArgFlags.nConference = 0;
               }
               else
               {
                  pProgramArg->oArgFlags.nConference = 1;
               }
               break;
         case 'p':
               /* Using PCM */
               if (0 == strcmp(PCM_FLAG_MASTER, optarg))
               {
                  printf("master on\n");
                  pProgramArg->oArgFlags.nPCM_Master = 1;
               }
               else if (0 == strcmp(PCM_FLAG_SLAVE, optarg))
               {
                  printf("slave on\n");
                  pProgramArg->oArgFlags.nPCM_Slave = 1;
               }
               break;
         case 'e':
               /* Set encoder type */
               pProgramArg->oArgFlags.nEncTypeDef = 1;
               arg_value = atoi(optarg);
               VOIP_SetEncoderType(arg_value);
               break;
         case 'f':
               /* Set packetisation time */
               pProgramArg->oArgFlags.nFrameLen = 1;
               arg_value = atoi(optarg);
               VOIP_SetFrameLen(arg_value);
               break;
         default:
               /* Unknown argument */
               /* Go out if unknown arg */
               TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                    ("Unknown or incomplete arguments. (File: %s, line: %d)\n",
                    __FILE__, __LINE__));
               break;
        } /* switch */
    } /* while */
} /* ABC_PBX_ReadOptions() */


#endif /* defined(LINUX) */


/**
   Get ip address of board and set it.

   \param pProgramArg - pointer to program arguments structure

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)
*/
static IFX_int32_t ABC_PBX_SetDefaultAddr(PROGRAM_ARG_t* pProgramArg)
{
   IFX_char_t buffer[256];

#ifdef LINUX

   struct hostent* he = IFX_NULL;

#else /* LINUX */

   IFX_int32_t addr = 0;

#endif /* LINUX */

   if (IFX_NULL == pProgramArg)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (gethostname(buffer, sizeof(buffer)) == -1)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("gethostname(), %s. (File: %s, line: %d)\n",
            strerror(errno), __FILE__, __LINE__));
      return IFX_ERROR;
   }

#ifdef LINUX

   he = gethostbyname(buffer);

   if (IFX_NULL == he)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("gethostbyname(), %s. (File: %s, line: %d)\n",
            strerror(errno), __FILE__, __LINE__));
   }
   else
   {
      pProgramArg->oMy_IP_Addr.sin_addr.s_addr =
         *(IFX_uint32_t *)he->h_addr_list[0];

      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
           ("Default IP-Address for host %s is %s\n",
             buffer, inet_ntoa(pProgramArg->oMy_IP_Addr.sin_addr)));
   }

#else /* LINUX */

   /* There is also possibility to use resolvGetHostByName(), but
      in file configAll.h support for library resolvLib,
      #define INCLUDE_DNS_RESOLVER must be added and new image
      prepared */
   addr = hostGetByName(buffer);

   pProgramArg->oMy_IP_Addr.sin_addr.s_addr = addr;

   TRACE(TAPIDEMO, DBG_LEVEL_LOW,
        ("Default IP-Address for host %s is %s\n",
          buffer, inet_ntoa(pProgramArg->oMy_IP_Addr.sin_addr)));

#endif /* LINUX */

   return IFX_SUCCESS;
} /* ABC_PBX_SetDefaultAddr() */


/**
   Make ip address from phone number.

   \param pCtrl   - handle to control structure
   \param pPhone  - pointer to PHONE
   \param pConn    - pointer to phone connection
   \param nPhoneNumber - phone number with 4 digits
                         (iiip iii=IP-number, p=port-number)
   \param fIsPCM_Num - IFX_TRUE we have phone number of PCM phone,
                       otherwise normal phone number.

   \return IFX_ERROR if an error, otherwise IFX_SUCCESS (no error occurred)

   \remark This address will be used to call the external phone.
*/
static IFX_return_t ABC_PBX_FillAddrByNumber(CTRL_STATUS_t* pCtrl,
                                             PHONE_t* pPhone,
                                             CONNECTION_t* pConn,
                                             IFX_int32_t nPhoneNumber,
                                             IFX_boolean_t fIsPCM_Num)
{
   IFX_int32_t port;
   IFX_int32_t ip;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone) || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("nNumber = %d\n", (int) nPhoneNumber));
   /* Calculate ip */
   ip = nPhoneNumber / 10;

   if (IFX_TRUE == fIsPCM_Num)
   {
      /* Increase port by max resource channels, because PCM sockets
         follow UDP sockets which are used for VoIP */
      port = nPhoneNumber % 10 + PCM_SOCKET_PORT;
   }
   else
   {
      /* Port number for voice data and signalling events */
      port = nPhoneNumber % 10 + UDP_PORT;
   }

   /* Check if ip is valid */
   if ((0 > ip) || (255 < ip))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH, ("Invalid IP-number %d. \
           (File: %s, line: %d)\n", (int)ip, __FILE__, __LINE__));
      return IFX_ERROR;
   }

   if (IFX_TRUE == fIsPCM_Num)
   {
      bzero((char*) &pConn->oConnPeer.oPCM.oToAddr,
            (int) sizeof(struct sockaddr_in));
      pConn->oConnPeer.oPCM.oToAddr.sin_family = AF_INET;
      pConn->oConnPeer.oPCM.oToAddr.sin_port = htons(port);

#if defined(VXWORKS)
          /* Only for VxWorks inet_makeaddr
              1st Parameter is Network Part, Ex: 10.1.1.0 should be used 0x000A0101
              2nd Parameter is Host Part, depend on how mane bits have been used
                 by Network Part
              In Our Case, we always regard the mask is 255.255.255.0, so
                 shift right 8 bits
          */
          pConn->oConnPeer.oPCM.oToAddr.sin_addr =
              inet_makeaddr((pCtrl->nMy_IP_Addr >> 8), ip);
#else /* VXWORKS */
          pConn->oConnPeer.oPCM.oToAddr.sin_addr =
              inet_makeaddr(pCtrl->nMy_IP_Addr, ip);
#endif /* VXWORKS */

      if (pCtrl->pProgramArg->oArgFlags.nQos)
      {
         /* QoS support */
         QOS_InitializePairStruct(nPhoneNumber, pCtrl, pConn);
      }

      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
            ("pConn->oUsedAddr.sin_addr.s_addr = %08X:%d\n",
            ntohl((int)pConn->oConnPeer.oPCM.oToAddr.sin_addr.s_addr),
            ntohs((int)pConn->oConnPeer.oPCM.oToAddr.sin_port)));
   }
   else
   {
      bzero((char*) &pConn->oConnPeer.oRemote.oToAddr,
            (int) sizeof(struct sockaddr_in));
      pConn->oConnPeer.oRemote.oToAddr.sin_family = AF_INET;
      pConn->oConnPeer.oRemote.oToAddr.sin_port = htons(port);

#if defined(VXWORKS)
          /* Only for VxWorks inet_makeaddr
              1st Parameter is Network Part, Ex: 10.1.1.0 should be used 0x000A0101
              2nd Parameter is Host Part, depend on how mane bits have been used
                 by Network Part
              In Our Case, we always regard the mask is 255.255.255.0, so
                 shift right 8 bits
          */
          pConn->oConnPeer.oRemote.oToAddr.sin_addr =
              inet_makeaddr((pCtrl->nMy_IP_Addr >> 8), ip);
#else /* VXWORKS */
          pConn->oConnPeer.oRemote.oToAddr.sin_addr =
              inet_makeaddr(pCtrl->nMy_IP_Addr, ip);
#endif /* VXWORKS */

      if (pCtrl->pProgramArg->oArgFlags.nQos)
      {
         /* QoS support */
         QOS_InitializePairStruct(nPhoneNumber, pCtrl, pConn);
      }

      TRACE(TAPIDEMO, DBG_LEVEL_LOW,
            ("pConn->oUsedAddr.sin_addr.s_addr = %08X:%d\n",
            ntohl((int)pConn->oConnPeer.oRemote.oToAddr.sin_addr.s_addr),
            ntohs((int)pConn->oConnPeer.oRemote.oToAddr.sin_port)));
   }

   return IFX_SUCCESS;
} /* ABC_PBX_FillAddrByNumber() */


/**
   Clear phone buffer containing DTMF, pulse digits.

   \param nDataCh_FD  - file descriptor for data channel
*/
static IFX_void_t ABC_PBX_ClearDigitBuff(IFX_int32_t nDataCh_FD)
{
   IFX_int32_t ready = 0;
   IFX_int32_t digit = 0;


   /* Check for DTMF digit */
   /* Check the DTMF dialing status, but on data device not control device */
   do
   {
      ready = 0;
      ioctl(nDataCh_FD, IFX_TAPI_TONE_DTMF_READY_GET, (IFX_int32_t) &ready);
      if (1 == ready)
      {
         /* Digit arrived or was in buffer */
         ioctl(nDataCh_FD, IFX_TAPI_TONE_DTMF_GET, (IFX_int32_t) &digit);
      }
      else
      {
         /* No digit in the buffer, buffer is cleared */
         break;
      }
   }
   while (1 == ready);

   /* Check for Pulse digit */
   /* Check the Pulse dialing status, but on data device not control device */
   do
   {
      ready = 0;
      ioctl(nDataCh_FD, IFX_TAPI_PULSE_READY, (IFX_int32_t) &ready);
      if (1 == ready)
      {
         /* Digit arrived or was in buffer */
         ioctl(nDataCh_FD, IFX_TAPI_PULSE_GET, (IFX_int32_t) &digit);
      }
      else
      {
         /* No digit in the buffer, buffer is cleared */
         break;
      }
   }
   while (1 == ready);

   TRACE(TAPIDEMO, DBG_LEVEL_LOW,
        ("Buffer with DTMF, pulse digits cleared, flushed\n"));

} /* ABC_PBX_ClearDigitBuff() */


/**
   Initialize main structures used for testing

   \param pProgramArg - pointer to program arguments
   \param pCtrl       - pointer to status control structure

   \return IFX_SUCCESS if everything OK, otherwise IFX_ERROR
*/
static IFX_return_t ABC_PBX_Setup(PROGRAM_ARG_t* pProgramArg,
                                  CTRL_STATUS_t* pCtrl)
{
   IFX_int32_t ch = 0;
   PHONE_t* phone = IFX_NULL;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pProgramArg))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Set all variables in all used structures to zero */
   memset(pCtrl, 0, sizeof(CTRL_STATUS_t));

   /* Add default ip address and demo arguments */
   pCtrl->nMy_IP_Addr =
      ntohl((int)pProgramArg->oMy_IP_Addr.sin_addr.s_addr) & 0xffffff00;
   pCtrl->pProgramArg = pProgramArg;

   /* Initialize other stuff also */
   if (IFX_ERROR == ALM_Init(pCtrl->pProgramArg))
   {
      return IFX_ERROR;
   }

   if (IFX_ERROR == VOIP_Init(pCtrl->pProgramArg))
   {
      return IFX_ERROR;
   }

   if ((oProgramArg.oArgFlags.nPCM_Master)
       || (oProgramArg.oArgFlags.nPCM_Slave))
   {
      if (IFX_ERROR == PCM_Init(pCtrl->pProgramArg))
      {
         return IFX_ERROR;
      }
   }

   /* Unmap all channels */
   ABSTRACT_UnmapDefaults(pCtrl);

   /* Do startup mapping of channels */
   ABSTRACT_DefaultMapping(pCtrl);

   /* Channels are already open */
   for (ch = 0; ch < MAX_SYS_LINE_CH; ch++)
   {
      /* Prepare structure */
      phone = &pCtrl->rgoPhones[ch];

      phone->oCID_Msg.message = CidMsgData[ch];
      phone->nIdx = ch;

      /* Now setup FDs and sockets, because startup mapping is done. */
      phone->nPhoneCh_FD =
         Common_GetDeviceOfCh(phone->nPhoneCh);
      phone->nDataCh_FD = Common_GetDeviceOfCh(phone->nDataCh);

      /* Set sockets */
      phone->nSocket = VOIP_GetSocket(phone->nDataCh);

      /* set PCM sockets */
      if ((oProgramArg.oArgFlags.nPCM_Master)
          || (oProgramArg.oArgFlags.nPCM_Slave))
      {
         phone->nPCM_Socket = PCM_GetSocket(phone->nPCM_Ch);
      }

      if (oProgramArg.oArgFlags.nCID)
      {
         TRACE(TAPIDEMO, DBG_LEVEL_LOW,
              ("Set up CID data, driver for channel %d\n", (int)ch));

         CID_SetupData(ch, phone);

         /* Shows CID data */
         CID_Display(phone);

         /* Configure CID for this channel */
         if (IFX_ERROR == CID_ConfDriver(phone->nPhoneCh_FD))
         {
            TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                  ("CID initialization failed for channel %d. \
                  File: %s, line: %d)\n",
                  (int)ch, __FILE__, __LINE__));
            Common_Close_FDs();
            return IFX_ERROR;
         }
      }
   } /* for */

   Common_GetVersions();

   return IFX_SUCCESS;
} /* ABC_PBX_Setup() */


/**
   Initialize board, tapi, pcm, open devices.

   \return IFX_SUCCESS on OK, otherwise IFX_ERROR
*/
static IFX_return_t ABC_PBX_InitBoard(IFX_void_t)
{
   IFX_boolean_t we_are_master = IFX_FALSE;
   IFX_int32_t phone_ch_cnt = 0;
   IFX_int32_t data_ch_cnt = 0;
   IFX_int32_t pcm_ch_cnt = 0;


   SEPARATE
   /* set global fds */
   if ((IFX_ERROR) == Common_Set_FDs())
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("Error setting fds. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      Common_Close_FDs();
      return IFX_ERROR;
   }

   /* Initialize system, but you can use also SYSTEM_AM_UNSUPPORTED,
      but there are many more Access Modes for this system */
   if (IFX_ERROR == Common_InitSystem(SYSTEM_AM_DEFAULT))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("System initialization failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      Common_Close_FDs();
      return IFX_ERROR;
   }

   /* Initialize PCM if used */
   if ((oProgramArg.oArgFlags.nPCM_Master)
       || (oProgramArg.oArgFlags.nPCM_Slave))
   {
      if (oProgramArg.oArgFlags.nPCM_Master)
      {
         we_are_master = IFX_TRUE;
      }

      if (IFX_ERROR == Common_InitPCM(we_are_master))
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
               ("PCM initialization failed. (File: %s, line: %d)\n",
               __FILE__, __LINE__));
         Common_Close_FDs();
         return IFX_ERROR;
      }
   }

   /* Initialize chip */
   if (IFX_ERROR == Common_InitSilicon())
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("Silicon initialization failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      Common_Close_FDs();
      return IFX_ERROR;
   }

   /* Display number of all channels on this board. */
   Common_GetCapabilities(&phone_ch_cnt, &data_ch_cnt, &pcm_ch_cnt);

   SEPARATE

   return IFX_SUCCESS;
} /* InitBoard() */


/**
   Implements a simple state machine for PBX

   \param  pCtrl - pointer to status control structure

   \return status (IFX_SUCCESS or IFX_ERROR)

   \remark
      Actions are done from other sources, for example other channels
      wanting to call one channel. This can also be from a remote side via
      sockets.
*/
static IFX_return_t ABC_PBX_StateMachine(CTRL_STATUS_t* pCtrl)
{
   fd_set rfds, trfds;
   IFX_int32_t rwd = 0;
   IFX_return_t ret = IFX_SUCCESS;
   /* Wait forever by default */
   struct timeval* p_time_out = IFX_NULL;
   /* Hook status, digit status, line status is only checked on
      analog lines, that mean phone channel */
   /*IFX_TAPI_CH_STATUS_t stat[MAX_SYS_CH_RES];*/
   STATE_MACHINE_STATES_t state = US_NOTHING;
   IFX_int32_t i = 0;
   IFX_int32_t action_flag = 0;
   IFX_int32_t event_flag = 0;
   PHONE_t* phone = IFX_NULL;
   CONNECTION_t* conn = IFX_NULL;
   IFX_int32_t fd_dev_ctrl = -1;
   IFX_int32_t fd_ch = -1;
   IFX_int32_t socket = NO_SOCKET;
   PHONE_t* tmp_phone = IFX_NULL;
   CONNECTION_t* tmp_conn = IFX_NULL;
   IFX_boolean_t handle_data = IFX_FALSE;
   IFX_boolean_t event_on_phone_ch = IFX_FALSE;
#ifdef FXO
   CPC5621_EXCEPTION cpc_excp;
   IFX_int32_t fd_cpc_ctrl = -1;
   fd_set xfds, txfds;
#endif /* FXO */

   if (IFX_NULL == pCtrl)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   FD_ZERO(&rfds);
   rwd = -1;

   fd_dev_ctrl = Common_GetDevCtrlCh();

   /* Add config fd to fd set */
   if (0 < fd_dev_ctrl)
   {
      FD_SET(fd_dev_ctrl, &rfds);
      pCtrl->rwd = fd_dev_ctrl;
   }
   else
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Control device not open. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   /* Set up file descriptor table without sockets */
   for (i = 0; i < MAX_SYS_CH_RES; i++)
   {
      phone = &pCtrl->rgoPhones[i];
      phone->nStatus = US_READY;
      fd_ch = Common_GetDeviceOfCh(i);
      if (0 < fd_ch)
      {
         /* Put file descriptors into the read set */
         FD_SET(fd_ch, &rfds);
         /* Set the max width */
         if ((fd_ch) > (pCtrl->rwd))
         {
            pCtrl->rwd = fd_ch;
         }
         /* Set possible sockets */
         socket = VOIP_GetSocket(i);
         if (NO_SOCKET != socket)
         {
             FD_SET(socket, &rfds);
         }
         /* Set the max width */
         if (socket > (pCtrl->rwd))
         {
             pCtrl->rwd = socket;
         }
      } /* if */
      else
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Voice, data device for channel %d not open. \
              (File: %s, line: %d)\n",
              (int) i, __FILE__, __LINE__));
         return IFX_ERROR;
      }
   } /* for */

   /* Set up file descriptor table with PCM sockets */
   if ((oProgramArg.oArgFlags.nPCM_Master)
       || (oProgramArg.oArgFlags.nPCM_Slave))
   {
      for (i = 0; i < MAX_SYS_PCM_RES; i++)
      {
         socket = PCM_GetSocket(i);
         if (NO_SOCKET != socket)
         {
             FD_SET(socket, &rfds);
         }
         /* Set the max width */
         if (socket > (pCtrl->rwd))
         {
             pCtrl->rwd = socket;
         }
      }
   }


#ifdef FXO
   FD_ZERO(&xfds);

   fd_cpc_ctrl = Common_GetCpcCtrlCh();

   /* Add config fd to fd set */
   if (0 < fd_cpc_ctrl)
   {
      FD_SET(fd_cpc_ctrl, &xfds);
      /* Set the max width */
      if (fd_cpc_ctrl > (pCtrl->rwd))
      {
         pCtrl->rwd = fd_cpc_ctrl;
      }
   }
#endif /* FXO */

   rwd = pCtrl->rwd;

   /* TAPI SIGNAL SUPPORT */
   p_time_out = TAPI_SIG_SetTimer();

   /* Clear DTMF buffer for each phone channel */
   for (i = 0; i < MAX_SYS_CH_RES; i++)
   {
      /* Get file descriptor for this channel */
      fd_ch = Common_GetDeviceOfCh(i);
      ABC_PBX_ClearDigitBuff(fd_ch);
   }

   while (1)
   {
      event_flag = 0;
      /* Update the local file descriptor by the copy in the task parameter */
      memcpy((void*)&trfds, (void*)&rfds, sizeof(fd_set));
#ifdef FXO
      memcpy((void*)&txfds, (void*)&xfds, sizeof(fd_set));
      ret = select(pCtrl->rwd + 1, &trfds, IFX_NULL, &txfds, p_time_out);
#else
      ret = select(pCtrl->rwd + 1, &trfds, IFX_NULL, IFX_NULL, p_time_out);
#endif
      if (0 < ret)
      {
#ifdef FXO

          if (FD_ISSET(fd_cpc_ctrl, &txfds))
          {
             if (ioctl(fd_cpc_ctrl, FIO_CPC5621_EXCEPTION, (IFX_uint32_t)&cpc_excp) == -1)
             {
                printf("fxodemo:Error: Get CPC5621 exception failed\n\r");
                return -1;
             }

             /* Ringing to FXO port from PSTN CO side. Every ringing cycle has one exception. */
             if (cpc_excp.Bits.ringOn || cpc_excp.Bits.ringOff)
             {
                if (!bFXO_LocalRinging && !bFXO_CallActive)
                {
                   IFX_int32_t fd_tmp;
                   TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("FXO Port: Ringing !\n"));
                   ioctl(fd_cpc_ctrl, FIO_CPC5621_SET_CID, 1);
                   bFXO_LocalRinging = IFX_TRUE;
                   fd_tmp = Common_GetDeviceOfCh(0);
                   ioctl (fd_tmp, IFX_TAPI_RING_START, 0);
                   fd_tmp = Common_GetDeviceOfCh(1);
                   ioctl (fd_tmp, IFX_TAPI_RING_START, 0);
                }
                if (bFXO_LocalRinging && cpc_excp.Bits.ringOff)
                {
                   TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("FXO Port: Ringing off\n"));
                }
             }
             /* FXO port has a battery change event */
             if (cpc_excp.Bits.battery)
             {
                /* Do some thing if needed */
                printf("FXO Port: Battery change!\n");
#if 0 /* Was commented out before... */
                ioctl(pCtrl->cpc.dev_fd, FIO_CPC5621_SET_CID, 0);
                ioctl(pCtrl->con[0].dev_fd, IFXPHONE_CIDRX_START, 0);
#endif
             }
            /* FXO port has a polarity change */
            if (cpc_excp.Bits.polarityOnH || cpc_excp.Bits.polarityOffH)
            {
               /* Do some thing if needed */
               printf("FXO Port: Polarity change!\n");
            }
            /* if the CID bit of the CPC5621 sets to 0 (receive CID mode),
               the CPC driver will start a timer. When that timer timeout,
               the CID bit will change to 1 (normal mode), and send a exception event. */
            if (cpc_excp.Bits.cidTimeOut)
            {
               printf("FXO Port: CID timer expired, CID bit sets to normal mode.\n");
            }
         }
#endif
         if (FD_ISSET(fd_dev_ctrl, &trfds))
         {
            if (IFX_SUCCESS == EVENT_Check(fd_dev_ctrl, CHECK_ALL_CH))
            {
               event_flag = 1;
            }
         }

         /* Wait for all PCM channels and sockets. */
         if ((oProgramArg.oArgFlags.nPCM_Master)
             || (oProgramArg.oArgFlags.nPCM_Slave))
         {
            for (i = 0; i < MAX_SYS_PCM_RES; i++)
            {
               socket = PCM_GetSocket(i);
               if ((NO_SOCKET != socket) && FD_ISSET(socket, &trfds))
               {
                  tmp_conn = IFX_NULL;
                  tmp_phone = ABSTRACT_GetPHONE_Of_PCM_Socket(pCtrl,
                                                              socket,
                                                              &tmp_conn);
                  if (IFX_NULL == tmp_phone)
                  {
                     TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                          ("Phone not found for PCM socket idx %d. "
                          "(File: %s, line: %d)\n",
                          (int) i, __FILE__, __LINE__));
                     break;
                  }

                  if (IFX_NULL == tmp_conn)
                  {
                     /* It seems we are making first connection with new phone. */
                     tmp_conn = &tmp_phone->rgoConn[tmp_phone->nConnCnt];
                     tmp_conn->nUsedSocket = socket;
                     tmp_conn->nUsedCh_FD = PCM_GetFD_OfCh(i);
                     tmp_conn->nUsedCh = i;
                  }

                  if (IFX_SUCCESS == PCM_HandleSocketData(pCtrl, tmp_phone,
                                                          tmp_conn,
                                                          CID_IDX_NAME))
                  {
                     /* Action from other phone received, handle it. */
                     state = ABC_PBX_HandleState(pCtrl, tmp_phone, tmp_conn);
                     while (US_NOTHING != state)
                     {
                        ABC_PBX_StateTrans(pCtrl, tmp_phone, state, tmp_conn);
                        state = ABC_PBX_HandleState(pCtrl, tmp_phone, tmp_conn);
                     }
                  }
               }
            } /* for */
         } /* if */

         /* Wait for all VoIP channels and sockets. */
         for (i = 0; i < MAX_SYS_CH_RES; i++)
         {
            state = US_NOTHING;
            conn = IFX_NULL;
            phone = ABSTRACT_GetPHONE_OfDataCh(pCtrl, i, &conn);
            /* Some data channels are free not connected to phones. */
            if (IFX_NULL == phone)
            {
               /* Skip this data channel at the moment. */
               continue;
            }

            /* Get file descriptor for this channel */
            fd_ch = VOIP_GetFD_OfCh(i);

            if (FD_ISSET(fd_ch, &trfds))
            {
               if (IFX_NULL == conn)
               {
                  if ((1 == phone->nConnCnt)
                      && (LOCAL_CALL == phone->rgoConn[0].fType ))
                  {
                     /* Connected with one local phone, after all others leave
                        conference. */
                     conn = &phone->rgoConn[0];
                  }
                  else
                  {
                     /* It seems we are making new connection with new phone
                        OR someone is calling us. */
                     conn = &phone->rgoConn[phone->nConnCnt];
                     conn->nUsedCh = i;
                     conn->nUsedCh_FD = fd_ch;
                  }
               }

               /* QoS Support */
               if ((!pCtrl->pProgramArg->oArgFlags.nQos)
                   || ((pCtrl->pProgramArg->oArgFlags.nQos)
                   && (IFX_SUCCESS == QOS_HandleService(pCtrl, conn))))
               {
                  handle_data = IFX_TRUE;
               }
               else
               {
                  handle_data = IFX_FALSE;
               }

               /* We received data from phone */
               VOIP_HandleData(pCtrl, phone, fd_ch, conn, handle_data);
            }

            socket = VOIP_GetSocket(i);
            if ((NO_SOCKET != socket) && FD_ISSET(socket, &trfds))
            {
               tmp_conn = IFX_NULL;
               tmp_phone = ABSTRACT_GetPHONE_OfSocket(pCtrl, socket, &tmp_conn);
               if (IFX_NULL == tmp_phone)
               {
                  TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                       ("Phone not found for socket %d. "
                       "(File: %s, line: %d)\n",
                       (int) socket, __FILE__, __LINE__));
                  break;
               }

               if (IFX_NULL == tmp_conn)
               {
                  /* It seems we are making first connection with new phone. */
                  tmp_conn = &tmp_phone->rgoConn[tmp_phone->nConnCnt];
                  tmp_conn->nUsedSocket = socket;
                  tmp_conn->nUsedCh_FD = fd_ch;
                  tmp_conn->nUsedCh = i;
               }

               /* QoS Support */
               if ((!pCtrl->pProgramArg->oArgFlags.nQos)
                   || ((pCtrl->pProgramArg->oArgFlags.nQos)
                   && (IFX_SUCCESS == QOS_HandleService(pCtrl, tmp_conn))))
               {
                  handle_data = IFX_TRUE;
               }
               else
               {
                  handle_data = IFX_FALSE;
               }

               /* We received data over socket, external phone */
               if (IFX_SUCCESS == VOIP_HandleSocketData(pCtrl, tmp_phone,
                                                        tmp_conn, handle_data,
                                                        CID_IDX_NAME))
               {
                  /* Action from other phone received, handle it. */
                  state = ABC_PBX_HandleState(pCtrl, tmp_phone, tmp_conn);
                  while (US_NOTHING != state)
                  {
                     ABC_PBX_StateTrans(pCtrl, tmp_phone, state, tmp_conn);
                     state = ABC_PBX_HandleState(pCtrl, tmp_phone, tmp_conn);
                  }
               }
            }

            event_on_phone_ch = IFX_FALSE;
            if (!((1 == event_flag) && EVENT_Exists(i)))
            {
               event_on_phone_ch = IFX_TRUE;
               /* No event on data ch, maybe on phone ch */
               /* Hooks, pulse dialing are registered only on phone ch. */
               phone = ABSTRACT_GetPHONE_OfDataCh(pCtrl, i, &conn);

               if (IFX_NULL == phone)
               {
                  TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                       ("Phone not found for data/phone ch %d. "
                       "(File: %s, line: %d)\n",
                       (int) i, __FILE__, __LINE__));
                  break;
               }
            }

            /* If we got an event, then handle it */
            if ((1 == event_flag) && (EVENT_Exists(i)
                || EVENT_Exists(phone->nPhoneCh)))
            {
               if (IFX_TRUE == event_on_phone_ch)
               {
                  state = EVENT_Handle(phone, pCtrl, phone->nPhoneCh);
               }
               else
               {
                  state = EVENT_Handle(phone, pCtrl, i);
               }

               if (IFX_NULL == conn)
               {
#if 0
                  if ((US_READY == state) && (0 < phone->nConnCnt))
                  {
                     /* We hang up, so use last, old connection */
                     conn = &phone->rgoConn[phone->nConnCnt - 1];
                  }
                  else
#endif
                  {
                     /* It seems we are making new connection with new phone. */
                     conn = &phone->rgoConn[phone->nConnCnt];
                     conn->nUsedCh = i;
                     conn->nUsedCh_FD = fd_ch;
                  }
               }

               /* TAPI SIGNAL SUPPORT */
               TAPI_SIG_CheckTimeOut(conn, state);

               if (US_CONFERENCE == state)
       	       {
                  /* CONFERENCE */
  	               if (pCtrl->pProgramArg->oArgFlags.nConference)
		            {
            		   if (US_DIALING == phone->nStatus)
		               {
                        if (0 < phone->nConnCnt)
                        {
                           /* If in the middle of dialing and # is pressed
                              cancel dialing and start again AND already in
                              connection with one or more peers. */
			                  TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
			                       ("{#} pressed, restarting dialing for new phone.\n"));
	                        state = US_CONFERENCE;
                        }
                        else
                        {
                           state = US_DIALING;
                        }
                     }
		               else
	                  {
		                  state = CONFERENCE_Start(pCtrl, phone);
	                  }
	               }
                  else
                  {
                     state = US_DIALING;
                  }
				   } /* if */

               /* TAPI SIGNAL SUPPORT */
               /*TAPI_SIG_SigHandling(conn, state, ?&stat[i]?);*/
            }

            while (US_NOTHING != state)
            {
               ABC_PBX_StateTrans(pCtrl, phone, state, conn);
               state = ABC_PBX_HandleState(pCtrl, phone, conn);
            }

            phone->pConnFromLocal = IFX_NULL;
         } /* for */

         /* This part is doing further actions */
         action_flag = 1;
         while (action_flag)
         {
            IFX_int32_t iteration = 0, dummy = 0;

            if (pCtrl->pProgramArg->oArgFlags.nWait)
            {
               printf("Iteration: %d\n", (int) iteration++);
               scanf("%d", (int *) dummy);
            }
            action_flag = 0;
            for (i = 0; i < MAX_SYS_CH_RES; i++)
            {
               conn = IFX_NULL;
               phone = ABSTRACT_GetPHONE_OfDataCh(pCtrl, i, &conn);

               /* Some data channels are free not connected to phone */
               if (IFX_NULL == phone)
               {
                  /* Skip this data channel at the moment. */
                  continue;
               }

               if (US_NOTHING != phone->nDstState)
               {
                  TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
                       ("Action %s on %s\n",
                        STATE_NAMES[phone->nDstState],
                        phone->oCID_Msg.message[CID_IDX_NAME].string.element));

                  if (IFX_NULL != phone->pConnFromLocal)
                  {
                     /* Got action from local phone */
                     conn = phone->pConnFromLocal;
                     phone->pConnFromLocal = IFX_NULL;
                  }
                  else if (0 < phone->nConnCnt)
                  {
                     /* Last one is always the one with which new connection
                        is started if there exists previous ones. */
                     conn = &phone->rgoConn[phone->nConnCnt - 1];
                  }

                  if (IFX_NULL == conn)
                  {
                     /* It seems we are making new connection with new phone. */
                     conn = &phone->rgoConn[phone->nConnCnt];
                  }

                  state = ABC_PBX_HandleState(pCtrl, phone, conn);
                  while (US_NOTHING != state)
                  {
                     ABC_PBX_StateTrans(pCtrl, phone, state, conn);
                     state = ABC_PBX_HandleState(pCtrl, phone, conn);
                  }
                  action_flag = 1;
                  phone->pConnFromLocal = IFX_NULL;
               }
            } /* for */
         } /* while */
      } /* if */
      else if (0 == ret)
      {
         /* Signal timeouts are handled via tick counter */
         TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
              ("Timeout occured on select() call.\n"));
      }
      else
      {
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Error in select(). (File: %s, line: %d)\n",
              __FILE__, __LINE__));
      }
   } /* while */

   return IFX_SUCCESS;
} /* ABC_PBX_StateMachine() */


/**
   Clear phone after connection.

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  pConn  - pointer to phone connection
   \param  eState - new state information
   \param  nDataCh_FD - data channel file descriptor

   \return Phone is cleared.
*/
static IFX_void_t ABC_PBX_ClearPhone(CTRL_STATUS_t* pCtrl,
                                     PHONE_t* pPhone,
                                     CONNECTION_t* pConn,
                                     STATE_MACHINE_STATES_t eState,
                                     IFX_int32_t nDataCh_FD)
{
   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return;
   }

   pPhone->nDialNrCnt = 0;
   pPhone->nConfIdx = NO_CONFERENCE;
   pPhone->fExtPeerCalled = IFX_FALSE;
   pPhone->fPCM_PeerCalled = IFX_FALSE;
   pPhone->fLocalPeerCalled = IFX_FALSE;
   pPhone->nConnCnt = 0;
   pPhone->nStatus = eState;

   if (IFX_NULL != pConn)
   {
      if (PCM_CALL == pConn->fType)
      {
         if ((pCtrl->pProgramArg->oArgFlags.nPCM_Master)
            || (pCtrl->pProgramArg->oArgFlags.nPCM_Slave))
         {
            PCM_EndConnection(pCtrl->pProgramArg, pConn->nUsedCh,
                              pConn->nUsedCh);

            /* Unmap phone and pcm channels */
            PCM_MapToPhone(pConn->nUsedCh,
                           pPhone->nPhoneCh,
                           IFX_FALSE, IFX_NULL);

            PCM_FreeCh(pConn->nUsedCh);
         }
      }
      else
      {
         /* Stop coder */
         VOIP_StopCodec(pConn->nUsedCh);

         if (pPhone->nDataCh != pConn->nUsedCh)
         {
            /* Unmap phone from data ch if not our first data ch. */
            VOIP_MapPhoneToData(pConn->nUsedCh,
                                pPhone->nPhoneCh,
                                IFX_FALSE,
                                IFX_NULL);
            VOIP_FreeDataCh(pConn->nUsedCh);
            /* Also stop playing busytone on this data ch. */

            /* Start playing busy tone on first data channel, otherwise
               there won't be any busy tone. */
            if (US_BUSYTONE == eState)
            {
               ioctl(pPhone->nDataCh_FD, IFX_TAPI_TONE_BUSY_PLAY, NO_PARAM);
            }
         }

         if (pCtrl->pProgramArg->oArgFlags.nQos)
         {
            /* QoS Support */
            QOS_StopSession(pCtrl, pConn);
         }

        /* TAPI SIGNAL SUPPORT */
         TAPI_SIG_ResetTxSigHandler(pConn);
      }
   }
} /* ABC_PBX_ClearPhone() */


/**
   Handles a state transition on our phone, connection

   \param  pCtrl  - pointer to status control structure
   \param  pPhone - pointer to PHONE
   \param  nState - new state information
   \param  pConn  - pointer to phone connection

   \return status (IFX_SUCCESS or IFX_ERROR)
   \remark
      The new state will be handled in ABC_PBX_HandleState().
      This function may invoke actions.
*/
static IFX_int32_t  ABC_PBX_StateTrans(CTRL_STATUS_t* pCtrl,
                                       PHONE_t* pPhone,
                                       IFX_int32_t nState,
                                       CONNECTION_t* pConn)
{
   CONFERENCE_t *pConf = IFX_NULL;
   IFX_int32_t fd_data_ch = -1;
   IFX_int32_t fd_phone_ch = -1;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone) || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }


   fd_data_ch = VOIP_GetFD_OfCh(pConn->nUsedCh);
   fd_phone_ch = pPhone->nPhoneCh_FD;

   if (nState != pPhone->nStatus)
   {
      TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
           ("%s: State Change %s -> %s\n",
            pPhone->oCID_Msg.message[CID_IDX_NAME].string.element,
            STATE_NAMES[pPhone->nStatus], STATE_NAMES[nState]));
   }

   switch (pPhone->nStatus)
   {
      case US_READY:
         pPhone->nDialNrCnt = 0;
         pPhone->nConfIdx = NO_CONFERENCE;
         pPhone->fExtPeerCalled = IFX_FALSE;
         pPhone->fPCM_PeerCalled = IFX_FALSE;
         pPhone->fLocalPeerCalled = IFX_FALSE;
         pPhone->nConnCnt = 0;

         /* Set line to standby mode */
         ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
               IFX_TAPI_LINE_FEED_STANDBY);

         switch (nState)
         {
            case US_HOOKOFF:
#ifdef FXO
               if (bFXO_LocalRinging)
               {
                  IFX_int32_t fd_tmp;
                  printf("%s() - activate FXO connection\n", __FUNCTION__);
                  /* stop ringing */
                  fd_tmp = Common_GetDeviceOfCh(0);
                  ioctl(fd_tmp, IFX_TAPI_RING_STOP, 0);
                  fd_tmp = Common_GetDeviceOfCh(1);
                  ioctl(fd_tmp, IFX_TAPI_RING_STOP, 0);
                  ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
                        IFX_TAPI_LINE_FEED_ACTIVE);
                  /* set up voice connection */
                  FXO_ActivateCon(pConn->nUsedCh);
                  /* update global vars */
                  bFXO_LocalRinging = IFX_FALSE;
                  bFXO_CallActive   = IFX_TRUE;
                  pPhone->nStatus   = US_ACTIVE_RX;

                  /*ABC_PBX_SetAction(pCtrl, pPhone, pConn, US_ACTIVE_TX);*/
                  break;
               }
               else
               {
#endif /* FXO */
                  ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
                        IFX_TAPI_LINE_FEED_ACTIVE);

                  ioctl(fd_data_ch, IFX_TAPI_TONE_DIALTONE_PLAY, NO_PARAM);
                  pPhone->nStatus = US_DIALTONE;
#ifdef FXO
               }
#endif /* FXO */
               break;
            case US_INCCALL:
               pPhone->nStatus = US_INCCALL;
               break;
            case US_RINGING:
               /* TAPI SIGNAL SUPPORT */
               TAPI_SIG_SetPhoneRinging(fd_phone_ch);
               pPhone->nStatus = US_RINGING;
               break;
            default:
               ABC_PBX_ClearPhone(pCtrl, pPhone, pConn, US_READY, fd_data_ch);
               break;
         } /* switch */
      break;
      case US_DIALING:
         switch (nState)
         {
            case US_CALLING:
               pPhone->nStatus = US_CALLING;
               break;
            case US_BUSY:
               ioctl(fd_data_ch, IFX_TAPI_TONE_BUSY_PLAY, NO_PARAM);
               pPhone->nStatus = US_BUSYTONE;
               pPhone->nConnCnt = 0;
               break;
            case US_READY:
               pPhone->nDialNrCnt = 0;
               ioctl(fd_data_ch, IFX_TAPI_TONE_LOCAL_PLAY, NO_PARAM);
               pPhone->nStatus = US_READY;
               pPhone->nConnCnt = 0;

               /* Set line to standby mode */
               ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
                     IFX_TAPI_LINE_FEED_STANDBY);
               break;
         } /* switch */
      break;
      case US_CALLING:
         switch (nState)
         {
            case US_RINGBACK:
               ioctl(fd_data_ch, IFX_TAPI_TONE_RINGBACK_PLAY, NO_PARAM);
               pPhone->nStatus = US_RINGBACK;
            break;
            case US_BUSY:
               ioctl(fd_data_ch, IFX_TAPI_TONE_BUSY_PLAY, NO_PARAM);
               pPhone->nStatus = US_BUSYTONE;
            break;
            case US_HOOKOFF:
               ioctl(fd_data_ch, IFX_TAPI_TONE_DIALTONE_PLAY, NO_PARAM);
               pPhone->nStatus = US_DIALTONE;
            break;
         } /* switch */
         break;
      case US_RINGBACK:
         switch (nState)
         {
            case US_READY:
               pPhone->nDialNrCnt = 0;
               pPhone->nStatus = US_READY;
               pPhone->fLocalPeerCalled = IFX_FALSE;

               /* CONFERENCE, master has left the conference so stop it */
               if (pCtrl->pProgramArg->oArgFlags.nConference)
               {
                  /* Get conference */
                  if ((NO_CONFERENCE != pPhone->nConfIdx)
                      && (MAX_CONFERENCES >= pPhone->nConfIdx))
                  {
                     pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
                  }

                  if ((IFX_NULL != pConf)
                      && (IFX_TRUE == pPhone->nConfStarter))
                  {
                     /* End with conference */
                     CONFERENCE_End(pCtrl, pPhone, pConf);
                  }
                  else
                  {
                     /* Calling first phone, that is ringing */
                     pConn = &pPhone->rgoConn[0];
                     ABC_PBX_SetAction(pCtrl, pPhone, pConn, US_READY);
                     ioctl(fd_data_ch, IFX_TAPI_TONE_LOCAL_PLAY, NO_PARAM);
                  }
               }
               else
               {
                  /* Don't use conference */
                  pConn = &pPhone->rgoConn[0];
                  ABC_PBX_SetAction(pCtrl, pPhone, pConn, US_READY);
                  ioctl(fd_data_ch, IFX_TAPI_TONE_LOCAL_PLAY, NO_PARAM);
               }

               pPhone->nConnCnt = 0;

               /* Set line to standby mode */
               ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
                     IFX_TAPI_LINE_FEED_STANDBY);
               break;
            case US_ACTIVE_TX:
               /* We called the phone and it pick up, we have connection */
               ioctl(fd_data_ch, IFX_TAPI_TONE_LOCAL_PLAY, NO_PARAM);

               /* Connection is active */
               pConn->fActive = IFX_TRUE;

               /* Create new connection */
               if (pCtrl->pProgramArg->oArgFlags.nConference)
               {
                  if ((IFX_TRUE == pPhone->nConfStarter)
                     && (NO_CONFERENCE != pPhone->nConfIdx)
                     && (MAX_CONFERENCES >= pPhone->nConfIdx))
                  {
                     /* Increasing count of phones, peers in conference */
                     if (0 == pCtrl->rgoConferences[pPhone->nConfIdx - 1].nPeersCnt)
                     {
                        /* First time starting conference, add peer that is already
                           in connection - normal connection between two phones. */
                        pCtrl->rgoConferences[pPhone->nConfIdx - 1].nPeersCnt++;
                     }
                     pCtrl->rgoConferences[pPhone->nConfIdx - 1].nPeersCnt++;
                  }

                  if ((IFX_TRUE != pPhone->nConfStarter)
                       || ((IFX_TRUE == pPhone->nConfStarter)
                       && (NO_CONFERENCE == pPhone->nConfIdx)))
                  {
                    if (PCM_CALL == pConn->fType)
                    {
                       /* PCM call - START */
                       if ((oProgramArg.oArgFlags.nPCM_Master)
                           || (oProgramArg.oArgFlags.nPCM_Slave))
                       {
                           /* Use same timeslots as called PCM phone. */
                           PCM_StartConnection(&oProgramArg, pConn->nUsedCh,
                                               pConn->oConnPeer.oPCM.nCh);
                       }
                    }
                    else
                    {
                       /* Ordinary call (over phone or data channel) */

                       /* Start codec, but only once for channel who started
                          conference, only at startup of normal connection or
                          conferencing. For other data channels start it
                          normally */
                       VOIP_StartCodec(pConn->nUsedCh);

                       if (pCtrl->pProgramArg->oArgFlags.nQos)
                       {
                          /* QoS Support */
                          QOS_StartSession(pCtrl, pConn, NO_ACTION);
                       }

                       /* TAPI SIGNAL SUPPORT */
                       TAPI_SIG_TxSigConfig(fd_data_ch);
                    }
                  }
                  else
                  {
                     if (PCM_CALL == pConn->fType)
                     {
                        /* PCM call - START */
                        if ((oProgramArg.oArgFlags.nPCM_Master)
                            || (oProgramArg.oArgFlags.nPCM_Slave))
                        {
                           /* Use same timeslots as called PCM phone. */
                           PCM_StartConnection(&oProgramArg, pConn->nUsedCh,
                                               pConn->oConnPeer.oPCM.nCh);
                        }
                     }
                     else if (EXTERN_VOIP_CALL == pConn->fType)
                     {
                       /* Start codec for new data channel that is
                          part of conference. */
                       VOIP_StartCodec(pConn->nUsedCh);

                       if (pCtrl->pProgramArg->oArgFlags.nQos)
                       {
                          /* QoS Support */
                          QOS_StartSession(pCtrl, pConn, NO_ACTION);
                       }

                       /* TAPI SIGNAL SUPPORT */
                       TAPI_SIG_TxSigConfig(fd_data_ch);
                     }
                  }
               } /* if */
               else
               {
                 if (PCM_CALL == pConn->fType)
                 {
                    /* PCM call - START */
                    if ((oProgramArg.oArgFlags.nPCM_Master)
                        || (oProgramArg.oArgFlags.nPCM_Slave))
                    {
                        /* Use same timeslots as called PCM phone. */
                        PCM_StartConnection(&oProgramArg, pConn->nUsedCh,
                                            pConn->oConnPeer.oPCM.nCh);
                    }
                 }
                 else
                 {
                     /* Start coder */
                     VOIP_StartCodec(pConn->nUsedCh);

                     if (pCtrl->pProgramArg->oArgFlags.nQos)
                     {
                        /* QoS Support */
                        QOS_StartSession(pCtrl, pConn, NO_ACTION);
                     }

                     /* TAPI SIGNAL SUPPORT */
                     TAPI_SIG_TxSigConfig(fd_data_ch);
                 }
               } /* else */
               pPhone->nStatus = US_ACTIVE_TX;
               break;
         } /* switch */
         break;
      case US_BUSYTONE:
         switch (nState)
         {
            case US_READY:
               pPhone->nDialNrCnt = 0;
               pPhone->nConnCnt = 0;
               ioctl(fd_data_ch, IFX_TAPI_TONE_LOCAL_PLAY, NO_PARAM);
               pPhone->nStatus = US_READY;

               /* Set line to standby mode */
               ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
                     IFX_TAPI_LINE_FEED_STANDBY);

               break;
         } /* switch */
         break;
      case US_DIALTONE:
         switch (nState)
         {
            case US_DIALING:
               ioctl(fd_data_ch, IFX_TAPI_TONE_LOCAL_PLAY, NO_PARAM);
               pPhone->nStatus = US_DIALING;
               break;
            case US_READY:
               pPhone->nDialNrCnt = 0;
               ioctl(fd_data_ch, IFX_TAPI_TONE_LOCAL_PLAY, NO_PARAM);
               pPhone->nStatus = US_READY;
               pPhone->nConnCnt = 0;

               /* Set line to standby mode */
               ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
                     IFX_TAPI_LINE_FEED_STANDBY);

               break;
         } /* switch */
      break;
      case US_RINGING:
         switch (nState)
         {
            case US_HOOKOFF:
               /* We were called from somebody and pick up the phone,
                  connection is made */

               ioctl(fd_phone_ch, IFX_TAPI_RING_STOP, NO_PARAM);

               ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
                     IFX_TAPI_LINE_FEED_ACTIVE);

               if (PCM_CALL == pConn->fType)
               {
                  if ((oProgramArg.oArgFlags.nPCM_Master)
                     || (oProgramArg.oArgFlags.nPCM_Slave))
                  {
                     /* We are the called phone, so use his first channel. */
                     pConn->nUsedCh = pPhone->nPCM_Ch;
                     pConn->nUsedCh_FD = PCM_GetFD_OfCh(pPhone->nPCM_Ch);

                     PCM_StartConnection(&oProgramArg, pConn->nUsedCh,
                                         pConn->nUsedCh);

                     /* Map pcm channel with phone channel of initiator */
                     PCM_MapToPhone(pConn->nUsedCh, pPhone->nPhoneCh,
                                    IFX_TRUE, IFX_NULL);
                  }
               }
               else
               {
                  /* Ordinary call */

                  /* Start coder */
                  VOIP_StartCodec(pConn->nUsedCh);

                  if (pCtrl->pProgramArg->oArgFlags.nQos)
                  {
                     /* QoS Support */
                     QOS_StartSession(pCtrl, pConn, SET_ADDRESS_PORT);
                  }

                  if (pCtrl->pProgramArg->oArgFlags.nCID)
                  {
                     /* Send OFFHOOK CID message to caller */
                     if (IFX_ERROR == CID_Send(fd_data_ch,
                                            IFX_TAPI_LINE_HOOK_STATUS_OFFHOOK,
                                            &pPhone->oCID_Msg))
                     {
                        return US_NOTHING;
                     }
                  }

                  /* TAPI SIGNAL SUPPORT */
                  TAPI_SIG_RxSigConfig(fd_data_ch);
               }

               pPhone->nStatus = US_ACTIVE_RX;
               ABC_PBX_SetAction(pCtrl, pPhone, pConn, US_ACTIVE_TX);
               break;
            case US_READY:
               ioctl(fd_phone_ch, IFX_TAPI_RING_STOP, NO_PARAM);

               ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
                     IFX_TAPI_LINE_FEED_STANDBY);

               pPhone->nStatus = US_READY;
               pPhone->nConnCnt = 0;
               break;
         } /* switch */
      break;
      case US_ACTIVE_TX:
         switch (nState)
         {
            case US_READY:
#ifdef FXO
               if (bFXO_CallActive)
               {
                  FXO_DeactivateCon(pConn->nUsedCh);
                  bFXO_CallActive = IFX_FALSE;
               }
#endif /* FXO */

               /* CONFERENCE, master has left the conference so stop it */
               if (pCtrl->pProgramArg->oArgFlags.nConference)
               {
                  /* Get conference */
                  if ((NO_CONFERENCE != pPhone->nConfIdx)
                      && (MAX_CONFERENCES >= pPhone->nConfIdx))
                  {
                     pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
                  }

                  pPhone->nStatus = US_READY;

                  if ((IFX_NULL != pConf)
                      && (IFX_TRUE == pPhone->nConfStarter))
                  {
                     /* End with conference */
                     CONFERENCE_End(pCtrl, pPhone, pConf);
                  }
                  else
                  {
                     /* Normal call between two phones */
                     pConn = &pPhone->rgoConn[0];

                     ABC_PBX_SetAction(pCtrl, pPhone, pConn, US_BUSY);

                     ABC_PBX_ClearPhone(pCtrl, pPhone, pConn,
                                        US_READY, fd_data_ch);
                  }

                  pPhone->nDialNrCnt = 0;
                  pPhone->nConnCnt = 0;
               } /* if */
               else
               {
                  pConn = &pPhone->rgoConn[0];

                  ABC_PBX_ClearPhone(pCtrl, pPhone, pConn,
                                     US_READY, fd_data_ch);

                  ABC_PBX_SetAction(pCtrl, pPhone, pConn, US_BUSY);
               } /* else */
               break;
            case US_BUSY:
               /* One of the phones left us and sended US_BUSY,
                  remove it from conference if conference exists. */

               if (pCtrl->pProgramArg->oArgFlags.nConference)
               {
                  /* Get conference */
                  if ((NO_CONFERENCE != pPhone->nConfIdx)
                      && (MAX_CONFERENCES >= pPhone->nConfIdx))
                  {
                     pConf = &pCtrl->rgoConferences[pPhone->nConfIdx - 1];
                  }

                  if (IFX_NULL != pConf)
                  {
                     CONFERENCE_RemovePeer(pCtrl, pPhone, &pConn, pConf);
                  }
                  else
                  {
                     pPhone->nConfIdx = NO_CONFERENCE;
                     pConn = &pPhone->rgoConn[0];

                     /* Last phone leaved */
                     ioctl(fd_data_ch, IFX_TAPI_TONE_BUSY_PLAY, NO_PARAM);

                     ABC_PBX_ClearPhone(pCtrl, pPhone, pConn,
                                        US_BUSYTONE, fd_data_ch);
                  }
               } /* if */
               else
               {
                  pPhone->nStatus = US_BUSYTONE;
                  pConn = &pPhone->rgoConn[0];

                  ABC_PBX_ClearPhone(pCtrl, pPhone, pConn,
                                     US_BUSYTONE, fd_data_ch);

                  ioctl(fd_data_ch, IFX_TAPI_TONE_BUSY_PLAY, NO_PARAM);
               } /* else */
               break;
            /* CONFERENCE */
            case US_CONFERENCE:
               /* We are in the middle of connection, will make another one */
               /* simulation of HOOK OFF */
               pPhone->nStatus = US_DIALTONE;
               pPhone->nDialNrCnt = 0;

               ioctl(fd_data_ch, IFX_TAPI_TONE_DIALTONE_PLAY, NO_PARAM);
            break;
         } /* switch */
      break;
      case US_ACTIVE_RX:
         switch (nState)
         {
            case US_READY:
#ifdef FXO
               if (bFXO_CallActive)
               {
                  FXO_DeactivateCon(pConn->nUsedCh);
                  bFXO_CallActive = IFX_FALSE;
               }
#endif /* FXO */
               ABC_PBX_ClearPhone(pCtrl, pPhone, pConn, US_READY, fd_data_ch);

               ABC_PBX_SetAction(pCtrl, pPhone, pConn, US_BUSY);

               /* Set line to standby mode */
               ioctl(fd_phone_ch, IFX_TAPI_LINE_FEED_SET,
                     IFX_TAPI_LINE_FEED_STANDBY);

               break;
             case US_BUSY:
               ABC_PBX_ClearPhone(pCtrl, pPhone, pConn,
                                  US_BUSYTONE, fd_data_ch);

               ioctl(fd_data_ch, IFX_TAPI_TONE_BUSY_PLAY, NO_PARAM);
              break;
         } /* switch */
      break;
      default:
         /* If come here, then problems with state machine */
      break;
   } /* switch */

   return IFX_SUCCESS;
} /* ABC_PBX_StateTrans() */


/**
   Converts digits to number

   \param prgnDialNum - pointer to array of digits
   \param nDialNrCnt  - number of digits

   \return Dialed number (if 0 error)
*/
static IFX_int32_t ABC_PBX_DigitsToNum(IFX_char_t* prgnDialNum,
                                       IFX_int32_t nDialNrCnt)
{
   const IFX_int32_t digit2number[32] = {
       -1,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1,  0, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

   IFX_int32_t i = 0;
   IFX_int32_t dialed_number = 0;


   if ((IFX_NULL == prgnDialNum) || (0 > nDialNrCnt) || (9 < nDialNrCnt))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return dialed_number;
   }

   for (i = 0; i < nDialNrCnt; i++)
   {
      if ((0 > (IFX_int8_t) prgnDialNum[i])
          || (32 < (IFX_int8_t) prgnDialNum[i]))
      {
         /* Error, our digit outside conversion table */
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Digit %d outside transform table. (File: %s, line: %d)\n",
               prgnDialNum[i], __FILE__, __LINE__));
         return dialed_number;
      }
      prgnDialNum[i] = digit2number[(int) prgnDialNum[i]];
      dialed_number = dialed_number * 10 + prgnDialNum[i];
   }

   return dialed_number;
}


/**
   Check dialed number.

   \param prgnDialNum - array of dialed digits
   \param nDialNrCnt  - number of digits
   \param nDialedNum  - dialed number
   \param nChNum      - channel number

   \return UNKNOWN_CALL_TYPE if wrong number or call type and dialed number
           and also channel number if phone is called.
*/
TYPE_OF_CALL_t ABC_PBX_CheckDialedNum(IFX_char_t* prgnDialNum,
                                      IFX_int32_t nDialNrCnt,
                                      IFX_int32_t* nDialedNum,
                                      IFX_int32_t* nChNum)
{
   if ((IFX_NULL == prgnDialNum) || (IFX_NULL == nDialedNum)
       || (IFX_NULL == nChNum) || (0 > nDialNrCnt))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return UNKNOWN_CALL_TYPE;
   }

   /* 0x0b = '0' => DIGIT_ZERO */
   if ((1 <= nDialNrCnt) && (2 >= nDialNrCnt)
       && (DIGIT_ZERO != prgnDialNum[0]))
   {
      /* Number <xx>; but first must not be 0. Phone channel counting
         from 0, but we type from 1. */
      *nDialedNum = ABC_PBX_DigitsToNum(&prgnDialNum[0],
                                        nDialNrCnt);
      /* Subract 1, because if calling channel 0, 1 was typed */
      (*nDialedNum)--;
      *nChNum = *nDialedNum;
      if (MAX_SYS_LINE_CH < *nDialedNum)
      {
         /* Calling unknown phone channel */
         TRACE (TAPIDEMO, DBG_LEVEL_HIGH,
                ("Invalid number entered (%d), phone chanel"
                " does not exists.\n", (int) *nDialedNum));

         return UNKNOWN_CALL_TYPE;
      }
      else
      {
         return LOCAL_CALL;
      }
   }

#ifdef FXO
   if ((2 == nDialNrCnt)
       && (DIGIT_ZERO == prgnDialNum[0])
       && (8 == prgnDialNum[1]))
   {
      /* Number 00 connects FXS to FXO */
      return FXO_CALL;
   }

#endif /* FXO */
   if ((5 <= nDialNrCnt) && (6 >= nDialNrCnt)
       && (DIGIT_ZERO == prgnDialNum[0])
       && ((DIGIT_ZERO == prgnDialNum[1])
       || (DIGIT_ONE == prgnDialNum[1])
       || (DIGIT_TWO == prgnDialNum[1])))
   {
      /* Number 0<xxx><yy>; xxx - 0..255 (ip addr), yy - phone channel.
         Phone channel counting from 0, but we type from 1. */
      *nDialedNum = ABC_PBX_DigitsToNum(&prgnDialNum[1],
                                        nDialNrCnt - 1);
      /* Subract 1, because if calling channel 0, 1 was typed */
      /* translate channel or port from 1..4 to 0..3 */
      (*nDialedNum)--;

      /* Extract channel number */
      if (6 == nDialNrCnt)
      {
         /* Two digits */
         *nChNum = (*nDialedNum) % 100;
      }
      else
      {
         /* One digit */
         *nChNum = (*nDialedNum) % 10;
      }
      return EXTERN_VOIP_CALL;
   }

   if ((6 <= nDialNrCnt) && (7 >= nDialNrCnt)
       && (DIGIT_ZERO == prgnDialNum[0])
       && (DIGIT_NINE == prgnDialNum[1]))
   {
      if ((oProgramArg.oArgFlags.nPCM_Master)
          || (oProgramArg.oArgFlags.nPCM_Slave))
      {
         /* Number 09<xxx><yy>; xxx - 0..255 (ip addr), yy - pcm channel.
            Phone channel counting from 0, but we type from 1. */
         *nDialedNum = ABC_PBX_DigitsToNum(&prgnDialNum[2],
                                           nDialNrCnt - 2);
         /* Subract 1, because if calling channel 0, 1 was typed */
         (*nDialedNum)--;

         /* Extract channel number */
         if (7 == nDialNrCnt)
         {
            /* Two digits */
            *nChNum = (*nDialedNum) % 100;
         }
         else
         {
            /* One digit */
            *nChNum = (*nDialedNum) % 10;
         }
         return PCM_CALL;
      }
   }

   if ((5 <= nDialNrCnt)
       && (DIGIT_ZERO == prgnDialNum[0])
       && (DIGIT_SEVEN == prgnDialNum[1]))
   {
      /* Number 07<xx><y>; xx - 0..99 (feature id), y - action (on, off, ...). */
      *nDialedNum = ABC_PBX_DigitsToNum(&prgnDialNum[2],
                                        nDialNrCnt - 2);
      return SET_FEATURE;
   }

   return UNKNOWN_CALL_TYPE;
}


/**
   State handling without transition

   \param pCtrl  - pointer to status control structure
   \param pPhone - pointer to PHONE
   \param pConn  - pointer to phone connection

   \return New target state or US_NOTHING if nothing to do

   \remark
   This function may invoke actions for the following conditions:
   - channel tries to call another channel -> check if available
   - channel is ready and will be ringing -> ring back tone
   - channel should be called but is not ready  -> busy tone
*/
static IFX_int32_t ABC_PBX_HandleState(CTRL_STATUS_t* pCtrl,
                                       PHONE_t* pPhone,
                                       CONNECTION_t* pConn)
{
   PHONE_t* p_dst_phone = IFX_NULL;
   IFX_uint32_t action = US_NOTHING;
   IFX_int32_t number = 0;
   IFX_int32_t called_ch = -1;
   TYPE_OF_CALL_t call_type = UNKNOWN_CALL_TYPE;
   CONNECTION_t* new_conn = IFX_NULL;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone) || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return US_NOTHING;
   }

   action = pPhone->nDstState;
   pPhone->nDstState = US_NOTHING;
   TRACE (TAPIDEMO, DBG_LEVEL_LOW,
         ("State handling %s in %s\n",
         pPhone->oCID_Msg.message[CID_IDX_NAME].string.element,
         STATE_NAMES[pPhone->nStatus]));

   switch (pPhone->nStatus)
   {
      case US_DIALING:
         TRACE (TAPIDEMO, DBG_LEVEL_LOW,
               ("Dialed numbers count %d and number is %d\n",
               (int)pPhone->nDialNrCnt,
               (int)pPhone->nDialedNum[pPhone->nDialNrCnt-1]));

         call_type = ABC_PBX_CheckDialedNum(&pPhone->nDialedNum[0],
                                            pPhone->nDialNrCnt,
                                            &number,
                                            &called_ch);
         switch (call_type)
         {
            case LOCAL_CALL:
               TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Making local call, number "
                                                "%d\n", (int) number));
               pPhone->nDialNrCnt = 0;
               p_dst_phone = &pCtrl->rgoPhones[number];
               if (p_dst_phone == pPhone)
               {
                  TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
                        ("Calling our own phone number, channel\n"));
                  return US_BUSY;
               }

               /* Use new empty connection */
               new_conn = &pPhone->rgoConn[pPhone->nConnCnt];
               pPhone->nConnCnt++;
               new_conn->fType = LOCAL_CALL;

               /* Link myself to called phone */
               new_conn->oConnPeer.oLocal.pPhone = p_dst_phone;
               /* Save caller stuff */
               new_conn->nUsedCh_FD = pPhone->nDataCh_FD;
               new_conn->nUsedCh = pPhone->nDataCh;
               /* Save called stuff */
               new_conn->oConnPeer.oLocal.nDataCh_FD = p_dst_phone->nDataCh_FD;
               new_conn->oConnPeer.oLocal.nPhoneCh = p_dst_phone->nPhoneCh;

               if (pCtrl->pProgramArg->oArgFlags.nConference)
               {
                  CONFERENCE_AddLocalPeer(pCtrl, pPhone, p_dst_phone, new_conn);
               }

               /* I want the called phone to establish a local call with me */
               if (IFX_ERROR == ABC_PBX_SetAction(pCtrl, pPhone,
                                                  new_conn, US_INCCALL))
               {
                  pPhone->nConnCnt--;
                  return US_NOTHING;
               }

               if (pCtrl->pProgramArg->oArgFlags.nCID)
               {
                  /* Send ONHOOK CID message to called phone */
                  if (IFX_ERROR
                       == CID_Send(VOIP_GetFD_OfCh(p_dst_phone->nDataCh),
                                   0, &pPhone->oCID_Msg))
                  {
                     return US_NOTHING;
                  }
               }

               return US_CALLING;
               break;
            case EXTERN_VOIP_CALL:
               TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Making external call, number "
                                                "%d\n", (int) number));
               pPhone->nDialNrCnt = 0;

               /* Use new empty connection */
               new_conn = &pPhone->rgoConn[pPhone->nConnCnt];
               pPhone->nConnCnt++;
               new_conn->fType = EXTERN_VOIP_CALL;

               if (pCtrl->pProgramArg->oArgFlags.nConference)
               {
                  if (IFX_SUCCESS != CONFERENCE_AddExternalPeer(pCtrl,
                                                                pPhone,
                                                                new_conn))
                  {
                     return US_NOTHING;
                  }
               }

               /* Prepare for external call */
               if (ABC_PBX_FillAddrByNumber(pCtrl, pPhone, new_conn,
                                            number, IFX_FALSE) == -1)
               {
                  TRACE (TAPIDEMO, DBG_LEVEL_HIGH,
                        ("Error getting peer address (%d)\n", __LINE__));
                  return US_BUSY;
               }

               /* Save caller stuff */
               new_conn->nUsedSocket = VOIP_GetSocket(new_conn->nUsedCh);
               /* Save called stuff */
               new_conn->oConnPeer.oRemote.nCh = called_ch;

               /* I want the called phone to establish a external call with me */
               if (IFX_ERROR == ABC_PBX_SetAction(pCtrl, pPhone,
                                                  new_conn, US_INCCALL))
               {
                  pPhone->nConnCnt--;
                  return US_NOTHING;
               }
               return US_CALLING;
               break;
            case PCM_CALL:
               /* We are using PCM? */
               if ((oProgramArg.oArgFlags.nPCM_Master)
                   || (oProgramArg.oArgFlags.nPCM_Slave))
               {

                  TRACE (TAPIDEMO, DBG_LEVEL_LOW, ("Making PCM call "
                                                   "%d\n", (int) number));

                  pPhone->nDialNrCnt = 0;

                  /* Use new empty connection */
                  new_conn = &pPhone->rgoConn[pPhone->nConnCnt];
                  pPhone->nConnCnt++;
                  new_conn->fType = PCM_CALL;

                  if (pCtrl->pProgramArg->oArgFlags.nConference)
                  {
                     if (IFX_SUCCESS != CONFERENCE_AddPCM_Peer(pCtrl,
                                                               pPhone,
                                                               new_conn))
                     {
                        return US_NOTHING;
                     }
                  }

                  /* Prepare for PCM call */
                  if (ABC_PBX_FillAddrByNumber(pCtrl, pPhone, new_conn,
                                               number, IFX_TRUE) == -1)
                  {
                     TRACE (TAPIDEMO, DBG_LEVEL_HIGH,
                           ("Error getting peer address (%d)\n", __LINE__));
                     return US_BUSY;
                  }

                  /* Save caller stuff */
                  new_conn->nUsedSocket = PCM_GetSocket(new_conn->nUsedCh);
                  /* Save called stuff */
                  new_conn->oConnPeer.oPCM.nCh = called_ch;

                  /* I want the called phone to establish a PCM call with me */
                  if (IFX_ERROR == ABC_PBX_SetAction(pCtrl, pPhone,
                                                     new_conn, US_INCCALL))
                  {
                     pPhone->nConnCnt--;
                     return US_NOTHING;
                  }

                  return US_CALLING;
               }
               else
               {
                  /* PCM is not supported */
                  return US_NOTHING;
               }
               break;
            case SET_FEATURE:
               /* Setup the feature, but "call" is not stored in PHONE */
               FEATURE_Action(Common_GetDeviceOfCh(pPhone->nPhoneCh), number);
               /* Clear up digit buffer so we can make other call,
                  activate another feature */
               pPhone->nDialNrCnt = 0;
               return US_NOTHING;
               break;
#ifdef FXO
            case FXO_CALL:
               if (bFXO_CallActive == IFX_TRUE)
               {
                  printf("%s() - FXO connection busy\n", __FUNCTION__);
                  return US_ACTIVE_TX;
               }
               else
               {
                  printf("%s() - establishing FXO connection for dialing ch%i\n", __FUNCTION__, (int)pPhone->nDataCh);
#if 0
                  pPhone->nDialNrCnt = 0;
                  p_dst_phone = &pCtrl->rgoPhones[2];

                  /* Use new empty connection */
                  new_conn = &pPhone->rgoConn[pPhone->nConnCnt];
                  pPhone->nConnCnt++;
                  new_conn->fType = LOCAL_CALL;

                  /* Link myself to called phone */
                  new_conn->oConnPeer.oLocal.pPhone = p_dst_phone;
                  /* Save caller stuff */
                  new_conn->nUsedCh_FD = pPhone->nDataCh_FD;
                  new_conn->nUsedCh = pPhone->nDataCh;
                  /* Save called stuff */
                  new_conn->oConnPeer.oLocal.nDataCh_FD = p_dst_phone->nDataCh_FD;
                  new_conn->oConnPeer.oLocal.nPhoneCh = p_dst_phone->nPhoneCh;

                  if (IFX_ERROR == ABC_PBX_SetAction(pCtrl, pPhone,
                                                     new_conn, US_NOTHING))
                  {
                     pPhone->nConnCnt--;
                     return US_NOTHING;
                  }
#endif
                  pConn->fType = FXO_CALL;
                  FXO_ActivateCon(pPhone->nDataCh);
                  bFXO_CallActive = IFX_TRUE;
                  pPhone->nStatus = US_ACTIVE_TX;
                  return US_NOTHING;
               }
               break;
#endif /* FXO */
            default:
               return US_NOTHING;
               break;
         }
         break;
      case US_RINGING:
         switch (action)
         {
            case  US_READY:
               return US_READY;
               break;
         }
         break;
      case US_RINGBACK:
         switch (action)
         {
            case US_ACTIVE_TX:
               return US_ACTIVE_TX;
               break;
         }
      case US_CALLING:
         switch (action)
         {
            case US_RINGBACK:
               return US_RINGBACK;
            break;
            case US_BUSYTONE:
               return US_BUSY;
            break;
         }
         break;
      case  US_READY:
         switch (action)
         {
            case  US_INCCALL:
               /* Someone is calling us, so no conference, use first one :-). */
               ABC_PBX_SetAction(pCtrl, pPhone, pConn, US_RINGBACK);
               return US_RINGING;
            break;
         }
         break;
      case US_ACTIVE_TX:
         switch (action)
         {
            case US_BUSY:
                  return US_BUSY;
               break;
            default:
               break;
         }
         break;
      case US_ACTIVE_RX:
         switch (action)
         {
            case US_BUSY:
                  return US_BUSY;
               break;
            default:
               break;
         }
         break;
      case US_CONFERENCE:
         /* nothing is done here */
      break;
      default:
         break;
   } /* switch */

   /* Not able to react on incoming call */
   if (US_INCCALL == action)
   {
      ABC_PBX_SetAction(pCtrl, pPhone, pConn, US_BUSYTONE);
   }

   return US_NOTHING;
} /* ABC_PBX_HandleState() */



/**
   Get connection of local called phone.

   \param pPhone   - pointer to PHONE
   \param nPhoneCh - called phone channel

   \return phone connection, IFX_NULL on error.
*/
CONNECTION_t* ABC_PBX_GetConn(PHONE_t* pPhone, IFX_int32_t nPhoneCh)
{
   IFX_int32_t i = 0;


   if (IFX_NULL == pPhone)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_NULL;
   }

   if (0 == pPhone->nConnCnt)
   {
      /* Don't have any connection, return first one. */
      return &pPhone->rgoConn[0];
   }

   for (i = 0; i < pPhone->nConnCnt; i++)
   {
      if (LOCAL_CALL == pPhone->rgoConn[i].fType)
      {
         /* Search for this connection, if exists. */
         if (nPhoneCh == pPhone->rgoConn[i].oConnPeer.oLocal.nPhoneCh)
         {
            return &pPhone->rgoConn[i];
         }
      }
   }

   /* This connection does not exists, return last free one. */
   return &pPhone->rgoConn[pPhone->nConnCnt];
} /* ABC_PBX_GetConn() */


/**
   SetAction    : set action of the phone we are talking to.

   \param pCtrl   - pointer to status control structure
   \param pPhone  - pointer to PHONE
   \param pConn   - pointer to phone connection
   \param nAction - which action to send to other phone

   \return       IFX_SUCCESS - no error, otherwise IFX_ERROR
*/
IFX_return_t ABC_PBX_SetAction(CTRL_STATUS_t* pCtrl,
                               PHONE_t* pPhone,
                               CONNECTION_t* pConn,
                               IFX_int32_t nAction)
{
   PHONE_t* p_dst_phone = IFX_NULL;
   IFX_int32_t ret;
   struct sockaddr_in* addr;
   COMM_MSG_t msg;
   CONNECTION_t* p_conn = IFX_NULL;


   if ((IFX_NULL == pCtrl) || (IFX_NULL == pPhone) || (IFX_NULL == pConn))
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

   TRACE(TAPIDEMO, DBG_LEVEL_LOW,
        ("SetAction: Data ch %d, phone ch %d, fType=%d, nAction=%d(%s), ",
        (int) pPhone->nDataCh, (int) pPhone->nPhoneCh, (int) pConn->fType,
        (int) nAction, STATE_NAMES[nAction]));
   switch (pConn->fType)
   {
      case  LOCAL_CALL:
         /* Set action to local phone, for example tell him that
            we are calling */
         p_dst_phone = pConn->oConnPeer.oLocal.pPhone;

         if (IFX_NULL == p_dst_phone)
         {
            TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                 ("p_dst_phone is NULL, target phone is missing!"
                  " (File: %s, line: %d)\n",
                  __FILE__, __LINE__));
            return IFX_ERROR;
         }

         TRACE(TAPIDEMO, DBG_LEVEL_LOW,
              ("p_dst_phone: data ch %d, phone ch %d\n",
              (int) p_dst_phone->nDataCh, (int) p_dst_phone->nPhoneCh));

         /* More phones can call us */
         /* Status from where we got command. */
         /* Called phone has always only one connection with caller phone */
         p_dst_phone->nDstState = nAction;
         p_conn = ABC_PBX_GetConn(p_dst_phone, pPhone->nPhoneCh);
         if (IFX_NULL == p_conn)
         {
            /* Could not get phone connection */
            return IFX_ERROR;
         }
         p_dst_phone->pConnFromLocal = p_conn;
         p_conn->fType = LOCAL_CALL;
         p_conn->oConnPeer.oLocal.pPhone = pPhone;
         p_conn->oConnPeer.oLocal.nPhoneCh = pPhone->nPhoneCh;
         p_conn->oConnPeer.oLocal.nDataCh_FD = pPhone->nDataCh_FD;
         p_conn->nUsedCh = p_dst_phone->nDataCh;
         p_conn->nUsedCh_FD = p_dst_phone->nDataCh_FD;
         break;
      case EXTERN_VOIP_CALL:
         msg.nAction = nAction;
         msg.nCh = pConn->nUsedCh;
         msg.nMarkStart = COMM_MSG_START_FLAG;
         msg.nMarkEnd = COMM_MSG_END_FLAG;
         msg.fPCM = VOIP_CALL_FLAG;

         /* Set action to external phone, for example tell him that
            we are calling */
         addr = &pConn->oConnPeer.oRemote.oToAddr;

         TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
              ("Data ch%d, Socket %d: send %d,%d,%d to %s:%d\n",
              (int) pConn->nUsedCh, (int) pConn->nUsedSocket,
              msg.nCh, msg.nAction, msg.fPCM,
              inet_ntoa(addr->sin_addr),
              ntohs(addr->sin_port)));

         /* If conferencing is used then we have to use right socket on this side,
            otherwise we can have one local socket and more external sockets and
            its a mess. */
         ret = sendto(pConn->nUsedSocket, &msg, sizeof(msg), 0,
                     (struct sockaddr *) addr,
                     sizeof(*addr));
         if (sizeof(msg) != ret)
         {
            TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                 ("Error sending data (command %d byte(s)), %s. \
                  (File: %s, line: %d)\n",
                  sizeof(msg), strerror(errno), __FILE__, __LINE__));
         }
         break;
      case PCM_CALL:
         msg.nAction = nAction;
         msg.nCh = pConn->nUsedCh;
         msg.nMarkStart = COMM_MSG_START_FLAG;
         msg.nMarkEnd = COMM_MSG_END_FLAG;
         msg.fPCM = PCM_CALL_FLAG;

         /* Set action to PCM phone, for example tell him that we are calling */
         addr = &pConn->oConnPeer.oPCM.oToAddr;

         TRACE(TAPIDEMO, DBG_LEVEL_NORMAL,
              ("PCM ch%d, Socket %d: send %d,%d,%d to %s:%d\n",
               (int) pConn->nUsedCh, (int) pConn->nUsedSocket,
               msg.nCh, msg.nAction, msg.fPCM,
               inet_ntoa(addr->sin_addr),
               ntohs(addr->sin_port)));

         /* If conferencing is used then we have to use right socket on this side,
            otherwise we can have one local socket and more external sockets and
            its a mess. */
         ret = sendto(pConn->nUsedSocket, &msg, sizeof(msg), 0,
                     (struct sockaddr *) addr,
                     sizeof(*addr));
         if (sizeof(msg) != ret)
         {
            TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
                 ("Error sending PCM - data (command %d byte), %s. \
                  (File: %s, line: %d)\n",
                  sizeof(msg), strerror(errno), __FILE__, __LINE__));
         }
         break;
      case FXO_CALL:
         break;
      default:
         TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
              ("Wrong call type, %d. (File: %s, line: %d)\n",
               (int) pConn->fType, __FILE__, __LINE__));
   } /* of switch */

   return IFX_SUCCESS;
} /* ABC_PBX_SetAction() */


/** ---------------------------------------------------------------------- */


/* ============================= */
/* Global function definition    */
/* ============================= */


/* ---------------------------------------------------------------------- */

/**
   Main function.

   \param argc - number of arguments
   \param argv - array of arguments

   \return 0 or err code
*/
#if defined(LINUX)

int main(int argc, char *argv [])
{

#else /* defined(LINUX) */

IFX_int32_t abc_pbx_start(int nTraceLvl,
                          char *pIP_Addr,
                          int nCID,
                          int nQos,
                          int nConference,
                          int nWait,
                          int nHelp)
{

#endif /* defined(LINUX) */

   int ret = OK;


   /* Reset set parameters */
   memset((IFX_uint8_t *) &oProgramArg, 0, sizeof(PROGRAM_ARG_t));
   /* Print version */
   printf("TAPI Demo Version %s\n", ABC_PBX_VERSION);

   /* Read options */

#if defined(VXWORKS)

   /* Read arguments and setup application accordingly */

   /** \todo Maybe some parser of command line arguments and just one
             input argumet to main function - char* (array of arguments) */

   /* Get ip address of board and set it */
   ABC_PBX_SetDefaultAddr(&oProgramArg);
   /* Set default IP-Address */
   inet_aton(pIP_Addr, &oProgramArg.oMy_IP_Addr.sin_addr);

   /* Turn conferencing on by default */
   oProgramArg.oArgFlags.nConference = 1;

   /* Trace level is set to off by default */
   SetTraceLevel(TAPIDEMO, DBG_LEVEL_OFF);

   if ((nTraceLvl > DBG_LEVEL_OFF)
       || (nTraceLvl < DBG_LEVEL_LOW))
   {
      nTraceLvl = DBG_LEVEL_OFF;
   }
   SetTraceLevel(TAPIDEMO, nTraceLvl);

   if (1 == nWait)
   {
      oProgramArg.oArgFlags.nWait = 1;
   }
   if (1 == nQos)
   {
      QOS_TurnServiceOn(&oProgramArg);
   }

   if (0 < nCID)
   {
      oProgramArg.oArgFlags.nCID = 1;
      /* In VxWorks input CID standard starts from 1 and not from 0,
         because 0 means do NOT use CID. */
      CID_SetStandard(nCID - 1);
   }

#else /* VXWORKS */

   ABC_PBX_ReadOptions(argc, argv, &oProgramArg);

#endif /* VXWORKS */

   /* check demo_help parameter */
   if (oProgramArg.oArgFlags.nHelp)
   {
      return ABC_PBX_Help();
   }

   TRACE(TAPIDEMO, DBG_LEVEL_LOW, ("Using IP-Address: %s\n",
         inet_ntoa(oProgramArg.oMy_IP_Addr.sin_addr)));

   /* Initialize board, devices, chip */
   if (IFX_ERROR == ABC_PBX_InitBoard())
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("ABC_PBX_InitBoard() failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return ERROR;
   }

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Device initialized\n"));
#ifdef VMMC_AUDIO_TEST
   VMMC_Audio_Test (&oCtrlStatus);
#endif
#ifdef VMMC_TEST //winder, need to be initail after Coder initailized.
   if (ioctl(Common_GetDevCtrlCh(), FIO_VOICE_TEST, (IFX_int32_t) 0) != IFX_SUCCESS)
   {
      printf("FIO_VOICE_TEST ioctl error!!\n");
      return ERROR;
   }
   VMMC_Voice_Test();
#else
   /* Prepare objects, structure */
   if (IFX_ERROR == ABC_PBX_Setup(&oProgramArg, &oCtrlStatus))
   {
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("ABC_PBX_Setup() failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return ERROR;
   }

   TRACE(TAPIDEMO, DBG_LEVEL_NORMAL, ("Objects are prepared\n"));

#ifdef VMMC_PCM_TEST
   printf ("tapidemo: calling VMMC_Pcm_Test\n");
   ret = PCM_Init (oCtrlStatus.pProgramArg);
   printf ("tapidemo: PCM_Init, ret: %d\n", ret);
   VMMC_Pcm_Test ();
#else
#ifdef FXO
   if (IFX_ERROR == DUSLIC_Init())
   {
       TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("DUSLIC_Init() failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return ERROR;
  }
   if (IFX_ERROR == CPC5621_Init())
   {
        TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
            ("CPC5621_Init() failed. (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return ERROR;
 }
 /*PCM_Open(Common_GetDeviceOfCh(2), 2);*/

#endif

   /* Start actual program */
   ABC_PBX_StateMachine(&oCtrlStatus);
#endif

   /* This part never executes. */

   /* QoS Support */
   QOS_CleanUpSession(&oCtrlStatus, &oProgramArg);
#endif /* VMMC_TEST */
   /* Close devices, ... */
   Common_Close_FDs();

   return ret;
} /* main() */

/* To test audio channel*/
//////////// sanat//////////////////////////////////////////////

#ifdef VMMC_AUDIO_TEST
IFX_return_t VMMC_Audio_Test (CTRL_STATUS_t* pCtrl)
{
   fd_set rfds, trfds;
   IFX_int32_t rwd = 0;
   IFX_return_t ret = IFX_SUCCESS;
   /* wait forever by default */
   struct timeval *p_time_out = IFX_NULL;
   /* hook status, digit status, line status is only checked on
      analog lines, that mean phone channel */
   IFX_TAPI_CH_STATUS_t stat[MAX_SYS_CH_RES];
   IFX_int32_t i = 0;
   STATE_MACHINE_STATES_t state = US_NOTHING;
   IFX_int32_t action_flag = 0;
   IFX_int32_t event_flag = 0;
   CONNECTION_t *pCon = IFX_NULL;
   IFX_int32_t fd_dev_ctrl = -1;
   IFX_int32_t fd_ch = -1,fd_ch1 = -1,fd_ch2 = -1;
   IFX_int32_t fd_data_ch = -1;
   IFX_int32_t fd_phone_ch = -1;
   IFX_int32_t ready = 0, digit = 0;


   /* check input arguments */
   if (pCtrl == IFX_NULL)
   {
      /* Wrong input arguments */
      TRACE(TAPIDEMO, DBG_LEVEL_HIGH,
           ("Invalid input argument(s). (File: %s, line: %d)\n",
            __FILE__, __LINE__));
      return IFX_ERROR;
   }

      /*only one channel*/
      /* get file descriptor for this channel */
      fd_ch = Common_GetDeviceOfCh(0);
	  printf("fd_ch = %d \n",fd_ch);
      fd_ch1 = Common_GetDeviceOfCh(1);
      printf("fd_ch1 = %d \n",fd_ch1);
      fd_ch2 = Common_GetDeviceOfCh(2);
      printf("fd_ch2 = %d \n",fd_ch2);
	  {
	    int choice;
	    while(1)
		{
		  system ("clear");
		  printf("********AUDIO MUDULE Testing *********\n");
		  printf(" 1-> IFX_TAPI_MAP_DATA_ADD \n");
		  printf(" 2-> IFX_TAPI_MAP_DATA_REMOVE \n");
		  printf(" 3-> IFX_TAPI_MAP_PCM_ADD \n");
		  printf(" 4-> IFX_TAPI_MAP_PCM_REMOVE \n");
		  printf(" 5-> IFX_TAPI_AUDIO_VOLUME_SET \n");
		  printf(" 6-> IFX_TAPI_AUDIO_MODE_SET \n");
		  printf(" 7-> IFX_TAPI_AUDIO_ROOM_TYPE_SET \n");
		  printf(" 8-> IFX_TAPI_AUDIO_MUTE_SET \n");
		  printf(" 9-> IFX_TAPI_AUDIO_RING_START \n");
		  printf(" 10-> IFX_TAPI_AUDIO_RING_STOP \n");
		  printf(" 11-> IFX_TAPI_AUDIO_ICA_SET \n");
		  printf(" 12-> IFX_TAPI_MAP_DATA_ADD : AUX \n");
		  printf(" 13-> IFX_TAPI_MAP_DATA_REMOVE :AUX  \n");
		  printf(" 14-> IFX_TAPI_MAP_PCM_ADD : AUX \n");
		  printf(" 15-> IFX_TAPI_MAP_PCM_REMOVE :AUX  \n");
		  printf(" 16-> IFX_TAPI_EVENT_EXT_DTMF_CFG  \n");
		  printf(" 17-> key press  \n");
		  printf(" 18-> key release  \n");
		  printf(" 20-> exit \n");

		  printf(" \n********Enter your choice***** \n");
          scanf("%d",&choice);
		  switch(choice)
		  {
		     case 1:
		     {
                IFX_TAPI_MAP_DATA_t datamap;
                memset(&datamap,0,sizeof(datamap));
                datamap.nDstCh = 0;
                datamap.nChType=  IFX_TAPI_MAP_TYPE_AUDIO;
                ioctl(fd_ch1, IFX_TAPI_MAP_DATA_ADD,&datamap);
				break;
		     }
			 case 2:
			 {
                IFX_TAPI_MAP_DATA_t datamap;
                memset(&datamap,0,sizeof(datamap));
                datamap.nDstCh = 0;
                datamap.nChType=  IFX_TAPI_MAP_TYPE_AUDIO;
                ioctl(fd_ch1, IFX_TAPI_MAP_DATA_REMOVE,&datamap);
				break;
			 }
			 case 3:
			 {
                IFX_TAPI_MAP_PCM_t pcmmap;
                memset(&pcmmap,0,sizeof(pcmmap));
                pcmmap.nDstCh = 0;
                pcmmap.nChType=  IFX_TAPI_MAP_TYPE_AUDIO;
                ioctl(fd_ch1,IFX_TAPI_MAP_PCM_ADD,&pcmmap);
				break;
			 }
			 case 4:
			 {
                IFX_TAPI_MAP_PCM_t pcmmap;
                memset(&pcmmap,0,sizeof(pcmmap));
                pcmmap.nDstCh = 0;
                pcmmap.nChType=  IFX_TAPI_MAP_TYPE_AUDIO;
                ioctl(fd_ch1,IFX_TAPI_MAP_PCM_REMOVE,&pcmmap);
				break;
			 }
			 case 5:
			 {
                ioctl(fd_ch,IFX_TAPI_AUDIO_VOLUME_SET,3);
				break;
			 }
			 case 6:
			 {
                ioctl(fd_ch,IFX_TAPI_AUDIO_MODE_SET,IFX_TAPI_AUDIO_MODE_HANDSET );
                /*ioctl(fd_ch2,IFX_TAPI_AUDIO_MODE_SET,IFX_TAPI_AUDIO_MODE_HANDSET );*/
				break;
			 }
			 case 7:
			 {
                ioctl(fd_ch,IFX_TAPI_AUDIO_ROOM_TYPE_SET,IFX_TAPI_AUDIO_ROOM_TYPE_MUFFLED );
				break;
			 }
			 case 8:
			 {
                ioctl(fd_ch,IFX_TAPI_AUDIO_MUTE_SET,IFX_ENABLE );
				break;
			 }
			 case 9:
			 {
                ioctl(fd_ch,IFX_TAPI_AUDIO_RING_START,1);
				break;
			 }
			 case 10:
			 {
                ioctl(fd_ch,IFX_TAPI_AUDIO_RING_STOP,1);
				break;
			 }
			 case 11:
			 {
                ioctl(fd_ch,IFX_TAPI_AUDIO_ICA_SET,IFX_ENABLE );
				break;
			 }
		     case 12:
		     {
                IFX_TAPI_MAP_DATA_t datamap;
                memset(&datamap,0,sizeof(datamap));
                datamap.nDstCh = 0;
                datamap.nChType=  IFX_TAPI_MAP_TYPE_AUDIO_AUX;
                ioctl(fd_ch1, IFX_TAPI_MAP_DATA_ADD,&datamap);
				break;
		     }
		     case 13:
		     {
                IFX_TAPI_MAP_DATA_t datamap;
                memset(&datamap,0,sizeof(datamap));
                datamap.nDstCh = 0;
                datamap.nChType=  IFX_TAPI_MAP_TYPE_AUDIO_AUX;
                ioctl(fd_ch1, IFX_TAPI_MAP_DATA_REMOVE,&datamap);
				break;
		     }
		     case 14:
		     {
                IFX_TAPI_MAP_PCM_t datamap;
                memset(&datamap,0,sizeof(datamap));
                datamap.nDstCh = 0;
                datamap.nChType=  IFX_TAPI_MAP_TYPE_AUDIO_AUX;
                ioctl(fd_ch,IFX_TAPI_MAP_PCM_ADD,&datamap);
				break;
		     }
		     case 15:
		     {
                IFX_TAPI_MAP_DATA_t datamap;
                memset(&datamap,0,sizeof(datamap));
                datamap.nDstCh = 0;
                datamap.nChType=  IFX_TAPI_MAP_TYPE_AUDIO_AUX;
                ioctl(fd_ch,IFX_TAPI_MAP_PCM_REMOVE,&datamap);
				break;
		     }
		     case 16:
		     {
                IFX_TAPI_EVENT_EXT_DTMF_CFG_t  cfg;
                memset(&cfg,0,sizeof(cfg));
				cfg.local = 1;
                ioctl(fd_ch,IFX_TAPI_EVENT_EXT_DTMF_CFG,&cfg);
				break;
		     }
		     case 17:
		     {
                IFX_TAPI_EVENT_EXT_DTMF_t  dtmf;
                memset(&dtmf,0,sizeof(dtmf));
				dtmf.event = 7;
				dtmf.duration = 0;
				dtmf.action = IFX_TAPI_START;
                ioctl(fd_ch,IFX_TAPI_EVENT_EXT_DTMF,&dtmf);
				break;
		     }
		     case 18:
		     {
                IFX_TAPI_EVENT_EXT_DTMF_t  dtmf;
                memset(&dtmf,0,sizeof(dtmf));
				dtmf.event = 7;
				dtmf.duration = 0;
				dtmf.action = IFX_TAPI_STOP;
                ioctl(fd_ch,IFX_TAPI_EVENT_EXT_DTMF,&dtmf);
				break;
		     }
			 case 20:
			 {
                return IFX_SUCCESS;
			 }
			 default :
			 {
			    printf(" INVALID choice\n ");
				break;
			 }

		  }
	    }
   }
   printf ("VMMC_Audio_Test: exit\n");
   return IFX_SUCCESS;

}

#endif/*VMMC_AUDIO_TEST*/

