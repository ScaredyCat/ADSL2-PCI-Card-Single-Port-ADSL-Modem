#ifndef _TAPIDEMO_H
#define _TAPIDEMO_H
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
   Description  :
   \file

   \remarks

   \note Changes:
*******************************************************************************/

/* Added this define into configure.in (--enable-tapi-signalling) */
/* #define QOS_SUPPORT */
/* #define TAPI_SIGNAL_SUPPORT */


/* ============================= */
/* Includes                      */
/* ============================= */

#include "common.h"

/* TESTING - flag used when tapidemo starts on different versions */
#define VERSION_1_2 /* Default is version 1.2 - common voice api */
/* If flag above is not set version 1.1 is used */

#ifdef AM5120
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#endif

#ifdef QOS_SUPPORT 
/* QOS_INIT_SESSION defined in drv_qos.h */
#include "drv_tapi_qos.h"
#endif /* QOS_SUPPORT */

/* ============================= */
/* Global Defines                */
/* ============================= */

#ifdef VXWORKS

#elif (defined (LINUX))

enum
{
   ERROR = -1,
   OK    = 0,
   FALSE = 0,
   TRUE  = 1
};

enum
{
   WAIT_FOREVER = -1,
   NO_WAIT = 0
};

#endif /* VXWORKS | LINUX */


#ifdef DEBUG
#define deb(msg) printf msg;
#else
#define deb(msg)
#endif

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */


/* ============================= */
/* Global Structures and enums   */
/* ============================= */

/** PCM port for start/stop pcm connection
   (PCM_SOCKET_PORT .. (PCM_SOCKET_PORT + MAX_SYS_PCM_RES)) */
enum { PCM_SOCKET_PORT = 5100 };

/** UDP port for voice data (UDP_PORT .. (UDP_PORT + MAX_SYS_CH_RES)) */
enum { UDP_PORT = 5000 };

/** Phone digit representation */
enum
{
   /** Used to start PCM calls */
   DIGIT_NINE = 0x09, 
   /** Used to enable/disable, ... features */
   DIGIT_ONE = 0x01,
   DIGIT_TWO = 0x02,
   DIGIT_THREE = 0x03,
   DIGIT_SEVEN = 0x07,
   DIGIT_STAR = 0x0A,
   DIGIT_ZERO = 0x0B,
   /** Used to start conference, calling another party */
   DIGIT_HASH = 0x0C,
} DIGIT_t;

/** Sign for no parameters for ioctl() call */
enum { NO_PARAM = 0 };

/** Flag for no device FD */
enum { NO_DEVICE_FD = -1 };

/** Flag for no socket */
enum { NO_SOCKET = -1 };

/** Flag for PCM call, will be send to external phone */
enum { PCM_CALL_FLAG = 0x080 };

/** Flag for VoIP call, will be send to external phone */
enum { VOIP_CALL_FLAG = 0x0 };

/** Flag indicating start of command message */
enum { COMM_MSG_START_FLAG = 0x0FF };

/** Flag indicating end of command message */
enum { COMM_MSG_END_FLAG = 0x077 };

/* Max. phone number or ... than can be dialed */
enum { MAX_DIALED_NUM = 20 };

/** States of state machine */
typedef enum
{
   US_NOTHING = 0,
   US_READY,
   US_ACTIVE_RX,
   US_ACTIVE_TX,
   US_INCCALL,
   US_HOOKOFF,
   US_DIALTONE,
   US_BUSYTONE,
   US_DIALING,
   US_CALLING,
   US_RINGBACK,
   US_RINGING,
   US_BUSY,
   US_CONFERENCE
} STATE_MACHINE_STATES_t;


/* --------------------------------------------------------- */
/*                    CONFERENCE START                       */
/* --------------------------------------------------------- */

/** Type of mapping */
typedef enum _MAPPING_TYPE_t
{
   NO_MAPPING = -1,
   PHONE_PHONE = 0,
   PHONE_DATA = 1,
   PHONE_PCM = 2,
   PCM_PCM = 3,
   PCM_DATA = 4
} MAPPING_TYPE_t;

/** representing local or remote call */
typedef enum _TYPE_OF_CALL_t
{
   UNKNOWN_CALL_TYPE = -1,
   EXTERN_VOIP_CALL = 0,
   LOCAL_CALL = 1,
   PCM_CALL = 2,
   SET_FEATURE = 3,
#ifdef FXO
   FXO_CALL = 4
#endif /* FXO */
} TYPE_OF_CALL_t;

/** Conference status */
/** Also used for flag if not in conference */
enum
{
   NO_CONFERENCE = 0,
   CONFERENCE_EXISTS = 1
};

/** Flag which channel to check when restoring mapping. */
typedef enum
{
   ALL_CH = -1,
   DESTINATION_CH = 0,
   ADDED_CH = 1,
   BOTH_CH = 3
} WHICH_CH_CHECK_t;

/** Flag which type of channel to check when restoring mapping. */
typedef enum
{
   ALL_CH_TYPE = -1,
   DATA_CH = 0,
   PHONE_CH = 1,
   PCM_CH = 2
} CH_TYPE_CHECK_t;

/** Max peers in conference */
enum { MAX_PEERS_IN_CONF = 4 };

/** minimum parties for phone conference (without us :-) of course), we are the
   third one party */
enum { MIN_PARTY_CONFERENCE = 2 };

/** How many conferences can we have. Cannot be bigger than number of phones. */
enum { MAX_CONFERENCES = 2 };

/** How many mappings can exists:
   if 2 phone channels and 2 data channels there can be maximum of:
   phone on phone (1) and each phone on data, so 8, maximum 9, etc. */
enum { MAX_MAPPING = 64 };

/** Flag to restore whole mapping table */
enum { RESTORE_ALL_CH_MAPPING = -1 };

/** Flag for no free data channel */
enum { NO_FREE_DATA_CH = -1 };

/** Status no external peer in conference */
enum { NO_EXTERNAL_PEER = 0 };

/** Status no PCM peer in conference */
enum { NO_PCM_PEER = 0 };

/** Represents mapping table */
typedef struct _MAPPING_TABLE_t
{
   /** The channel to whom channel is mapped */
   IFX_uint32_t nCh;
   /** Channel which is mapped */
   IFX_uint32_t nAddedCh;
   /** Type of mapping:
      phone to phone, phone to data, phone to pcm, ...
      data to data is done by driver automaticaly */
   MAPPING_TYPE_t nMappingType;
   /** We were mapping IFX_TRUE and unmapping IFX_FALSE */
   IFX_boolean_t fMapping;
} MAPPING_TABLE_t;


/** Conference support */
typedef struct _CONFERENCE_t
{
   /** Conference status, IFX_TRUE this conference is active, IFX_FALSE
       is free for use. */
   IFX_boolean_t fActive;

   /** Number of peers in conference */
   IFX_int32_t nPeersCnt;

   /** Holds channel mapping structure
       Data channel mapping is done automaticaly. */
   MAPPING_TABLE_t MappingTable[MAX_MAPPING];

   /** How many mappings are currently */
   IFX_int32_t nMappingCnt;

   /** Number of external peers in conference */
   IFX_int32_t nExternalPeersCnt;

   /** Number of PCM peers (channels) in conference */
   IFX_int32_t nPCM_PeersCnt;

   /** Number of local peers in conference */
   IFX_int32_t nLocalPeersCnt;
} CONFERENCE_t;


/* --------------------------------------------------------- */
/*                    CONFERENCE END                         */
/* --------------------------------------------------------- */


/** Represents connection */
typedef struct _PEER_t
{
   /** Structure for external phone */
   struct {
      /** Socket for transferring data, voice to external phone */
      IFX_int32_t nSocket;
      /** Data channel number of this peer. */
      IFX_int32_t nCh;
      /** Addr we send data to */
      struct sockaddr_in oToAddr;
   } oRemote;
   /** Structure for PCM phone */
   struct {
      /** Socket used only for start/stop connection. */
      IFX_int32_t nSocket;
      /** PCM channel number of this peer. */
      IFX_int32_t nCh;
      /** Addr we send data to */
      struct sockaddr_in oToAddr;
   } oPCM;
   /** Pointer to connection structure - other local phone*/
   struct {
      struct _PHONE_t* pPhone;
      /** Phone channel number of this peer. */
      IFX_int32_t nPhoneCh;
      /** When local connection is made by using data ch, this FD is used
          for receiveing data. */
      IFX_int32_t nDataCh_FD;
   } oLocal;
} PEER_t;


/** Represents connection */
typedef struct _CONNECTION_t
{
   /** Connection status, IFX_TRUE - active, IFX_FALSE - not active */
   IFX_boolean_t   fActive;

   /* ---------------------------------------------------------------
          Information of channels, sockets used to call the phone
    */

   /** 1 - local, 0 - remote, 2 - pcm */
   TYPE_OF_CALL_t fType;
   /** CID */
   IFX_TAPI_CID_MSG_t oCID_Msg;
   /** For external actions (IP + port) or PCM actions (IP + port + ) */
   struct sockaddr_in oUsedAddr;
   /** Socket on which call to peer was made (only VoIP or PCM connections). */
   IFX_int32_t nUsedSocket;
   /** Channel on which call to peer was made (can be data, pcm, phone).
       It depends of the flag fType. */
   IFX_int32_t nUsedCh;
   /** Channel file descriptor */
   IFX_int32_t nUsedCh_FD;

   /* ------------------------------------------- 
               Information of called phone
    */
   /** Called peer */
   PEER_t oConnPeer;

#ifdef TAPI_SIGNAL_SUPPORT
   /** TAPI_SIGNAL_SUPPORT: Current signal state */
   IFX_int32_t nCurrentSignal;
   /** TAPI_SIGNAL_SUPPORT: Expiration time for signal */
   IFX_int32_t nTickExpire;
   /** Timeout */
   IFX_int32_t nTimeout;
#endif /* TAPI_SIGNAL_SUPPORT */

#ifdef QOS_SUPPORT
    struct {
        /** QOS_SUPPORT: 1 - session is started, 0 - session stopped */
        IFX_int32_t nInSession;
        /** Session pair */
        QOS_INIT_SESSION oPair;
    } oSession;
#endif

} CONNECTION_t;


/** Connection structure, represents abstraction object. Basically represents
    phone with phone channel and device on which phone events are recorded. */
typedef struct _PHONE_t
{
   /** Phone index */
   IFX_int32_t        nIdx;
   /** Phone channel number */
   IFX_int32_t        nPhoneCh;
   /** Data channel number, phone channel is mapped to this data channel at
       start. Used for DTMF and other stuff. */
   IFX_int32_t        nDataCh;
   /** PCM channel number */
   IFX_int32_t        nPCM_Ch;
   /** Socket on which we expect incoming data (events and voice)
       Also used to send data to external peer */
   IFX_int32_t        nSocket;
   /** Socket used to start/stop connection over PCM. PCM at the moment only
       support voice, data streaming, BUT no inicialization. */
   IFX_int32_t        nPCM_Socket;
   /** Device file descriptor this structure is connected to (events coming
       from phone channel. Basically first device is connected to first 
       structure, second device to second structure, etc. */
   IFX_int32_t        nPhoneCh_FD;
   /** Device file descriptor this structure is connected to (events coming
       from data channel. */
   IFX_int32_t        nDataCh_FD;
   /** Current state of phone. */
   STATE_MACHINE_STATES_t nStatus;

   /** CID: description and number of source phone */
   IFX_TAPI_CID_MSG_t oCID_Msg;

   /** Count of dialed numbers. */
   IFX_int32_t        nDialNrCnt;
   /** Array of dialed numbers. */
   IFX_char_t         nDialedNum[MAX_DIALED_NUM];

   /** Which action requested from other phone, peer. */
   STATE_MACHINE_STATES_t nDstState;

   /** Array of connections with other phones */
   CONNECTION_t rgoConn[MAX_PEERS_IN_CONF];

   /** Count of connections. This number - 1 represents number of
       active connections. Index to last one in array represents empty
       connection. */
   IFX_int32_t nConnCnt;

   /** CONFERENCE, conference index which it belongs if greater than zero,
      otherwise does not belong to conference. */
   IFX_uint32_t nConfIdx;

   /** CONFERENCE, this phone started conference:
       IFX_FALSE - no,
       IFX_TRUE - yes */
   IFX_boolean_t nConfStarter;

   /** We called local peer?
       IFX_FALSE - no,
       IFX_TRUE - yes */
   IFX_boolean_t fLocalPeerCalled;

   /** We called external peer?
       IFX_FALSE - no,
       IFX_TRUE - yes */
   IFX_boolean_t fExtPeerCalled;

   /** We called PCM peer?
       IFX_FALSE - no,
       IFX_TRUE - yes */
   IFX_boolean_t fPCM_PeerCalled;

   /** If local call then this phone channel send us some action. Used when removing 
       local phone from conference. */
   CONNECTION_t* pConnFromLocal;

} PHONE_t;


/** Flags for command line arguments */
typedef struct _ARG_FLAGS_t
{
   /** If 1 then CID is send when calling other phone, but only local */
   IFX_int32_t nCID : 1;
   /** If 1 then conference is used */
   IFX_int32_t nConference : 1;
   /** If 1 then help is displayed */
   IFX_int32_t nHelp : 1;
   /** If 1 then wait after each state machine step */
   IFX_int32_t nWait : 1;
   /** If 1 then download CRAM file */
   IFX_int32_t nCRAM : 1;
   /** 1 if quality of service is used, otherwise 0 */
   IFX_int32_t nQos : 1;
   /** 1 if PCM is used and we are master, otherwise 0 */
   IFX_int32_t nPCM_Master : 1;
   /** 1 if PCM is used and we are slave, otherwise 0  */
   IFX_int32_t nPCM_Slave : 1;
   /** 1 for new encoder type, otherwise 0  */
   IFX_int32_t nEncTypeDef : 1;
   /** 1 for new packetisation time, otherwise 0  */
   IFX_int32_t nFrameLen : 1;
} ARG_FLAGS_t;


/** Demo structure holding command line arguments as flags */
typedef struct _PROGRAM_ARG_t
{
    /** Set parameters */
    ARG_FLAGS_t oArgFlags;
    /** Contains IP address for connection in network format */
    struct sockaddr_in oMy_IP_Addr;
    /** Filename to download */
    IFX_char_t sCRAMfile[256];
} PROGRAM_ARG_t;


/** Control structure for all channels, holding status of connections and
    also program arguments */
typedef struct _CTRL_STATUS_t
{
   /** Holds array of phones */
   PHONE_t rgoPhones[MAX_SYS_LINE_CH];
   /** Holds array of conferences */
   CONFERENCE_t rgoConferences[MAX_CONFERENCES];
   /** Conference index, if 0 no conference yet */
   IFX_uint32_t nConfCnt;
   /** Program arguments */
   PROGRAM_ARG_t* pProgramArg;
   /** Holds control device file descriptor, like for /dev/vin10, but
       highest file descriptor, used for FD_ISSET */
   IFX_int32_t rwd;
   /** Contains IP address for connection but as number */
   IFX_uint32_t nMy_IP_Addr;
} CTRL_STATUS_t;


/** Message used to communicate between boards */
typedef struct _COMM_MSG_t
{
   /** Mark start of message - must match, otherwise wrong message */
   IFX_uint8_t nMarkStart;
   
   /** Channel number of caller */
   IFX_uint8_t nCh;

   /** Action (OFF_HOOK, ...) */
   IFX_uint8_t nAction;

   /** 1 - PCM or 0 - VoIP call */
   IFX_uint8_t fPCM;

   /** Mark end of message - must match, otherwise wrong message */
   IFX_uint8_t nMarkEnd;
} COMM_MSG_t;


/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

extern IFX_return_t ABC_PBX_SetAction(CTRL_STATUS_t* pCtrl,
                                      PHONE_t* pPhone,
                                      CONNECTION_t* pConn,
                                      IFX_int32_t nAction);

#endif /* _SYSTEM_H */
