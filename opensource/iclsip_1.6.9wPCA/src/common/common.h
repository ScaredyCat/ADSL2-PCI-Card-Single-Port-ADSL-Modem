#ifndef _COMMON_H
#define _COMMON_H
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

   EXCEPT FOR ANY LIABILITY DUE TO WILFUL ACTS OR GROSS NEGLIGENCE AND
   EXCEPT FOR ANY PERSONAL INJURY INFINEON SHALL IN NO EVENT BE LIABLE FOR
   ANY CLAIM OR DAMAGES OF ANY KIND, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ****************************************************************************
   Module      : file_h.h
   Description :
*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */

/** system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#ifdef LINUX
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
/** 
  * Added network support, TCP/IP, UDP, timers, sockets, ...
  */
/* #include <linux/errno.h> */
#include <errno.h>
#include <netdb.h>              /* h_errno */
#include <sys/socket.h>         /* socket, gethostbyaddr */
/* linux system includes */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/poll.h>
/* tolower(), ... functions */
#include <ctype.h>

#endif /* LINUX */

#ifdef VXWORKS
	#include <vxworks.h>
   #include <sockLib.h>
   #include <inetLib.h>
   #include <ioLib.h>
   #include <sys/ioctl.h>
	#include <sysLib.h>
	#include <tickLib.h>
   #include <taskLib.h>

   #include <hostLib.h>


struct option {
	const char *name;
	int has_arg;
	unsigned long flag;
	int val;
};

#endif /* VXWORKS */


/** Drivers interfaces needed headers */
#include "ifx_types.h"

#ifdef VMMC
#include "vmmc_io.h"
#else
#include "vinetic_io.h"
#endif

#include "drv_tapi_io.h"

/** system include */
#include "system.h"


/* ============================= */
/* Debug interface               */
/* ============================= */

/** Trace levels */
enum
{
   DBG_LEVEL_OFF = 4, /* None reporting */
   DBG_LEVEL_HIGH = 3, /* Error reporting */
   DBG_LEVEL_NORMAL = 2, /* Status reporting */
   DBG_LEVEL_LOW = 1  /* More info reporting */
} DEBUG_LEVEL_t;

/**
   Prints a trace message.

   \param name     - Name of the trace group
   \param level    - level of this message
   \param message  - a printf compatible formated string + opt. arguments

   \return

   \remark
      The string will be redirected via printf if level is higher or equal to
      the actual level for this trace group ( please see SetTraceLevel ).
*/
#define TRACE(name,level,message) \
      do \
      { \
          if(level >= G_nTraceGroup##name) \
            { \
               printf message; \
            } \
      } while(0)


/**
   Set the actual level of a trace group.

   \param name - Name of the trace group
   \param new_level - new trace level
*/
#define SetTraceLevel(name, new_level) {G_nTraceGroup##name = new_level;}

/**
   Create a log group.

   \param name - name of the log group

   \return

   \remark
      This has to be done once in the project. We do it in the file
      tapidemo.c. The default level of this log group is DBG_LEVEL_HIGH.
*/
#define CREATE_TRACE_GROUP(name) \
        IFX_uint32_t G_nTraceGroup##name = DBG_LEVEL_HIGH

/**
   Make a log group extern so other modules, files can use it also.

   \param name - name of the log group

   \return

   \remark
      This has to be done once in the project. We do it in the file
      tapidemo.c. The default level of this log group is DBG_LEVEL_HIGH.
*/
#define DECLARE_TRACE_GROUP(name) \
        extern IFX_uint32_t G_nTraceGroup##name

DECLARE_TRACE_GROUP(TAPIDEMO);


/* ============================= */
/* Debug interface               */
/* ============================= */


/* ============================= */
/* Global Defines                */
/* ============================= */

#ifdef VIN_2CPE

/** maximum number of system channel resources (coder, signalling) */
#define MAX_SYS_CH_RES                  4
/** maximum number of system channel resources (pcm) */
#define MAX_SYS_PCM_RES                 4
/** maximum number of system (analog, alm) line channels */
#define MAX_SYS_LINE_CH                 2

#elif VMMC

/** maximum number of system channel resources
   (coder, signalling, pcm, alm) */
#define MAX_SYS_CH_RES                  4
/** maximum number of system channel resources (pcm) */
#define MAX_SYS_PCM_RES                 4
/** maximum number of system (analog) line channels */
#define MAX_SYS_LINE_CH                 2
/** maximum number of system (audio) channels */
#define MAX_SYS_AUDIO_CH                 1

#else /* VMMC */

/** maximum number of system channel resources
   (coder, signalling, pcm, alm) */
/** \todo Basically my board easy334 had 4 data ch and 8 pcm channels */
#define MAX_SYS_CH_RES                  4 /*8*/
/** maximum number of system (analog) line channels */
#define MAX_SYS_LINE_CH                 4
/** maximum number of system channel resources (pcm) */
#define MAX_SYS_PCM_RES                 8

#endif /* VMMC */

#define SEPARATE \
                    printf ("############################################\n\r");

/* ============================= */
/* Global Variables              */
/* ============================= */

/** FDs for system, dev control and channels */
/*extern IFX_int32_t fdSys, fdDevCtrl, fdDevCh[MAX_SYS_CH_RES];*/

/* ============================= */
/* Global Structures             */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* fd operations */
IFX_return_t Common_Set_FDs(IFX_void_t);
IFX_void_t Common_Close_FDs(IFX_void_t);
IFX_int32_t Common_GetDevSys(IFX_void_t);
IFX_int32_t Common_GetDevCtrlCh(IFX_void_t);
IFX_int32_t Common_GetDeviceOfCh(IFX_int32_t nChNum);
IFX_int32_t* Common_GetDevChArray(IFX_void_t);
#ifdef FXO
IFX_int32_t Common_GetDusCtrlCh(IFX_void_t);
IFX_int32_t Common_GetDusOfCh(IFX_int32_t nChNum);
IFX_int32_t Common_GetCpcCtrlCh(IFX_void_t);
#endif /* FXO */
/* system operations */
IFX_return_t Common_InitSystem(sys_acc_mode_t sysAccMode);
IFX_return_t Common_ResetSystem(IFX_int32_t dev_num);
/* silicon initialization */
IFX_return_t Common_InitSilicon();
IFX_return_t Common_GetVersions();

/* pcm initialization */
IFX_return_t Common_InitPCM(IFX_boolean_t fMaster);

IFX_return_t Common_GetCapabilities(IFX_int32_t *nPhoneChCnt,
                                    IFX_int32_t *nDataChCnt,
                                    IFX_int32_t *nPCM_ChCnt);
#if 0

/* NOT USED at the moment */

/* basic io operations */
IFX_return_t Common_ReadCmd(IFX_int32_t fdDevCtrl,
                            IFX_uint16_t cmd1,
                            IFX_uint16_t cmd2,
                            IFX_uint16_t *data);
IFX_return_t Common_WriteCmd(IFX_int32_t fdDevCtrl,
                             IFX_uint16_t cmd1,
                             IFX_uint16_t cmd2,
                             IFX_uint16_t *data);
IFX_return_t Common_ReadReg(IFX_int32_t fdDevCtrl,
                            IFX_uint16_t cmd1,
                            IFX_uint16_t offset,
                            IFX_int32_t len,
                            IFX_uint16_t *values);
IFX_return_t Common_WriteReg(IFX_int32_t fdDevCtrl,
                             IFX_uint16_t cmd1,
                             IFX_uint16_t offset,
                             IFX_int32_t len,
                             IFX_uint16_t *values);
/* misc */
IFX_char_t  Common_Get_DTMF_Ascii(IFX_uint8_t digit);
IFX_return_t Common_Close8kHzDigitalLoop(IFX_int32_t fdDevCtrl,
                                         IFX_uint8_t ch,
                                         IFX_boolean_t b_close);

#endif /* 0 */

#endif /* _COMMON_H */





