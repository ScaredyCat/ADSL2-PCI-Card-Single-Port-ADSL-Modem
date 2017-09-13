#ifndef _DSP_FRAMEWORK_HEADER
#define _DSP_FRAMEWORK_HEADER
/*********************************************************
* File: DSPFramework.h
* Author: Shiue, De-Hui
* Date: 2005/09/15
* Description: DSP Framework Library header file
*/


#define MAX_CHANNEL     1

//*******************************************************************************************
//************************ The Parameters For Voice Frame ***********************************
/*
    Frame Types
*/
#define FT_NO_TRANSMIT  0
#define FT_VOICE        1
#define FT_SID          2
#define FT_TONE         3


/*
    Voice Codec Type ID
*/
#define VPM_CODEC_UNKNOWN       0
#define VPM_CODEC_LINEAR16      1
#define VPM_CODEC_G711_ULAW     2
#define VPM_CODEC_G711_ALAW     3
#define VPM_CODEC_G726_40K      4
#define VPM_CODEC_G726_32K      5
#define VPM_CODEC_G726_24K      6
#define VPM_CODEC_G726_16K      7
#define VPM_CODEC_G729AB        8
#define VPM_CODEC_G723_1A       9
#define VPM_CODEC_LAST          10 // dummy
#define VPM_WAVEFORM_CODER_MAX  7


/*
    Number of TDM Samples Per Codec Frame
*/
#define CODEC_UNKNOWN_SAMPLES_PER_FRAME 80
#define LINEAR16_SAMPLES_PER_FRAME      80
#define G711_ULAW_SAMPLES_PER_FRAME     80
#define G711_ALAW_SAMPLES_PER_FRAME     80
#define G726_40K_SAMPLES_PER_FRAME      80      // ゲ斗 8 计
#define G726_32K_SAMPLES_PER_FRAME      80      // ゲ斗 2 计
#define G726_24K_SAMPLES_PER_FRAME      80      // ゲ斗 8 计
#define G726_16K_SAMPLES_PER_FRAME      80      // ゲ斗 4 计
#define G729AB_SAMPLES_PER_FRAME        80
#define G723_1A_SAMPLES_PER_FRAME       240

#define MAX_PCM_FRAME_SIZE  240
#define MIN_PCM_FRAME_SIZE  80  // add for frameworkReady()

#define TDB_LEN     (MAX_PCM_FRAME_SIZE*3) // unit: short; the value depend on platform


/*
    Number of Bytes Per Encoded Frame
*/
#define CODEC_UNKNOWN_BYTES_PER_FRAME   0
#define LINEAR16_BYTES_PER_FRAME        (LINEAR16_SAMPLES_PER_FRAME*2)
#define G711_ULAW_BYTES_PER_FRAME       G711_ULAW_SAMPLES_PER_FRAME
#define G711_ALAW_BYTES_PER_FRAME       G711_ALAW_SAMPLES_PER_FRAME
#define G726_40K_BYTES_PER_FRAME        (G726_40K_SAMPLES_PER_FRAME*5/8)    // ゲ斗 5 计
#define G726_32K_BYTES_PER_FRAME        (G726_32K_SAMPLES_PER_FRAME*4/8)
#define G726_24K_BYTES_PER_FRAME        (G726_24K_SAMPLES_PER_FRAME*3/8)    // ゲ斗 3 计
#define G726_16K_BYTES_PER_FRAME        (G726_16K_SAMPLES_PER_FRAME*2/8)
#define G729AB_BYTES_PER_FRAME          10
#define G723_1A_BYTES_PER_FRAME         24 // 6.3k:24 Bytes;  5.3k: 20 Bytes

#define MAX_ENC_FRAME_SIZE  LINEAR16_BYTES_PER_FRAME
#define PDB_LEN     MAX_ENC_FRAME_SIZE // unit: byte


/*
    Number of Bytes Per SID Frame
*/
#define G729AB_BYTES_PER_SID_FRAME      2
#define G723_1A_BYTES_PER_SID_FRAME     4
#define G711_A2_BYTES_PER_SID_FRAME     11
#define G711_A2_DTX_PERIOD              10 // unit:frame



//*******************************************************************************************
//*************************************** Messages ******************************************
/*
    Command Message
*/
#define CMD_CHAN_CONFIGURE      1
#define CMD_CHAN_ACTIVATE       2
#define CMD_CHAN_DEACTIVATE     3
#define CMD_TONE_GEN_START      4
#define CMD_TONE_GEN_STOP       5
#define CMD_PING                10
#define CMD_SET_RX_GAIN         11
#define CMD_SET_TX_GAIN         12
#define CMD_SET_KEYTONE_GAIN    13
#define CMD_AEC_START           14
#define CMD_AEC_STOP            15
#define CMD_SET_CODING_RATE     18  // currently only for G.723.1A; 0=>6.3k, 1=>5.3k(default)


/*
    Response message
*/
#define RESP_CHAN_CONFIGURED        1
#define RESP_CHAN_ACTIVATED         2
#define RESP_CHAN_DEACTIVATED       3
#define RESP_TONE_GEN_STARTED       4
#define RESP_TONE_GEN_STOPPED       5
#define RESP_PING_RESP              10
#define RESP_RX_GAIN_MODIFIED       11
#define RESP_TX_GAIN_MODIFIED       12
#define RESP_KEYTONE_GAIN_MODIFIED  13
#define RESP_AEC_STARTED            14
#define RESP_AEC_STOPPED            15
#define RESP_CODING_RATE_MODIFIED   18  // currently only for G.723.1A;


/*
    Event message
*/
#define EVENT_CHANNEL_OUTOFRANGE        30
#define EVENT_CMB_FULL                  33
#define EVENT_UNKNOWN_CMD_ID            34
#define EVENT_CODECTYPE_INCORRECT       35
#define EVENT_TONEDIRECTION_INCORRECT   36
#define EVENT_TONE_ID_INCORRECT         37
#define EVENT_TONE_DIR_INCORRECT        38
#define EVENT_AEC_INIT_FAIL             39
#define EVENT_TIMING_INCORRECT          41
#define EVENT_GAIN_VALUE_INCORRECT      42
#define EVENT_CHAN_INIT_FAIL            43


//*******************************************************************************************
//************************ The Parameters For Tone Gen/Det **********************************
/*
    Tone Generation Direction
*/
#define TONE_DIR_RX     0x1
#define TONE_DIR_TX     0x2
#define TONE_DIR_KEY    0x4

/*
    Tone Generation Key Type
*/
#define DTMF_0  0
#define DTMF_1  1
#define DTMF_2  2
#define DTMF_3  3
#define DTMF_4  4
#define DTMF_5  5
#define DTMF_6  6
#define DTMF_7  7
#define DTMF_8  8
#define DTMF_9  9
#define DTMF_A  10
#define DTMF_B  11
#define DTMF_C  12
#define DTMF_D  13
#define DTMF_STAR   14
#define DTMF_SHARP  15
#define DIAL_TONE                       16  // Dial Tone , 350Hz + 440Hz, Continue Tone
#define RECALL_DIAL_TONE                17  // Recall Dial Tone , 350Hz + 440Hz, burst 3x(0.1s on- 0.1s off) - Continue Tone
#define RINGING_BACK_TONE               18  // Audible Ringing back Tone, 440Hz + 480Hz, 2.0s on - 4.0s off, REPEATED
#define RINGING_BACK_TONE_PABX          19  // Audible Ringing back Tone (PABX), 440Hz + 480Hz, 1.0s on - 3.0s off, REPEATED
#define BUSY_TONE                       20  // Busy Tone, 480Hz + 620Hz, 0.5s on - 0.5s off, REPEATED
#define CONGESTION_TONE                 21  // Congestion Tone, 480Hz + 620Hz, 0.25s on - 0.25s off, REPEATED
#define SPECIAL_INFORMATION_TONE        22  // Special Information Tone, 950Hz/1400Hz/1800Hz, 3x 0.33 on
#define WARNING_TONE                    23  // Warning Tone - Operator Intervening, 440Hz, Continue Tone, 2.0s on - 10.0s off - 0.5s on - 10.0s off, REPEATED
#define WARNING_TONE_PABX               24  // Warning Tone - Operator Intervening (PABX), 440Hz, Continue Tone, 1.5s on - 8.0s off - 0.5s on - 8.0s off, REPEATED
#define WAITING_TONE_PABX               25  // Waiting Tone (PABX), 440Hz,  0.3s on - 10.0s off, REPEATED
#define RECORD_TONE                     26  // Record Tone, 1400Hz, 0.5s on - 15.0s off, REPEATED
#define EXECUTIVE_OVERRIDE_TONE_PABX    27  // Executive Override Tone (PABX), 440Hz,  3.0s on
#define INTERCEPT_TONE_PABX             28  // Intercept Tone (PABX), 440Hz/620Hz,  0.20s on- 0.25s off
#define CONFIRMATION_TONE_PABX          29  // Confirmation Tone (PABX), 350Hz + 440Hz,  3x0.1s on - 2x0.1s off
#define RECEIVER_OFF_HOOK_TONE          30  // Receiver off-hook tone, 1400Hz + 2060Hz + 2450Hz + 2600Hz, 0.1s on - 0.1s off, REPEATED
#define RING_TONE                       31  // Audible Ring Tone, 440Hz + 480Hz, 1s on, 2s off, REPEATED
#define KEY_PAD_ECHO                    32  // Key Pad Echo Tone, 620Hz, 0.1s on

#define LAST_TONE                       32





//*************************************************************************************************************
//******************************************* Function Prototypes *********************************************

#ifdef  __cplusplus
extern "C" {
#endif

/*
    DSP Framework function
*/
void channelControl_Init(void);
void channelControl(void);
int  frameworkReady(void); //if DSP framework is ready to run return 1 else 0

/*
    functions to get/put Command Message Blocks
*/
typedef struct { // CMB Data Structures
    short commandID;
    short serialNumber;
    short channelID;
    short param[2];
} t_CMB;

int getCMB(t_CMB *returnMsg);    // if successful return non-zero value else return zero
int putCMB(t_CMB msg);    // if successful return non-zero value else return zero

/*
    functions to get/put PCM Frames
*/
int getPCMFrame(int ch, int smpno, short *buffer);   // if successful return the smpno (in shorts) else return value (-1)

int putPCMFrame(int ch, int smpno, short *txBuffer);   // if successful return the PCMFrame size(in shorts) else return value (-1)

/*
    functions to get/put Voice Packets
*/
int getPacket(int ch, int *codecType, unsigned char *buffer, int *frameType, int *e_extPara);   // if successful return the packet size(in bytes) else return value (-1)
                                                                                                // e_extPara: for G.723.1A: 0=>6.3k, 1=>5.3k; for G.711/G.726/L16PCM: encFrameSize(bytes)
int putPacket(int ch, int codecType, unsigned char *buffer, int frameType, int d_extPara);      // if successful return the packet size(in bytes) else return value (-1)
                                                                                                // d_extPara: for G.723.1A: 0=>6.3k, 1=>5.3k; for G.711/G.726/L16PCM: encFrameSize(bytes)


//*************************************************************************************************************
//******************************************* Functions To Debug *********************************************
void    openPrintOutStream(void);  // open a output stream for fprintf() used in framework
                                   // if the platform is WinCE, the output file "printOut.txt" will be opened.
                                   // otherwise, the default output stream is stdout
void    closePrintOutStream(void); // close the opened stream by openPrintOutStream()

int     getRxTDBSize(int ch); // return the "farend to nearend" pcm buffer size
int     getTxTDBSize(int ch); // return the "nearend to farend" pcm buffer size

typedef struct {
    int    vdb_recoder_op;      // 0:disable; 1:recode all DSP I/O bit streams to files; 2:read input bit stream from files, and record output bit stream to files.
    int    codecs_recoder_op;   // 0:disable; 1:recode all CODEC I/O bit streams to files; 2:read input bit stream from files, and record output bit stream to files.
    int    ec_recoder_op;       // 0:disable; 1:recode all Echo Canceller I/O bit streams to files; 2:read input bit stream from files, and record output bit stream to files.(available in AEC started)
    int    PDB_debuger_op;      // 0:disable; 1:print some information when exception happened in PDB I/F
    int    TDB_debuger_op;      // 0:disable; 1:print some information when exception happened in TDB I/F; 2: print more information relative to TDB I/F
} t_dspDebugerCfg;

/*
    if opcode == 1  the file name for [TDB input]       is "CH?_txPCMStream.pcm"
                                      [TDB output]      is "CH?_rxPCMStream.pcm"
                                      [PDB input]       is "CH?_rxPackets.bin"
                                      [PDB output]      is "CH?_txPackets.bin"
                                      [Encoder input]   is "CH?_encInput.pcm"
                                      [Encoder output]  is "CH?_encOutput.bin"
                                      [Decoder input]   is "CH?_decInput.bin"
                                      [Decoder output]  is "CH?_decOutput.pcm"
                                      [EC Rin]          is "CH?_Rin_EC.pcm"
                                      [EC Sin]          is "CH?_Sin_EC.pcm"
                                      [EC Sout]         is "CH?_Sout_EC.pcm"

    if opcode == 2  the file name for [TDB input]       is "CH?_txPCMStream.pcm"
                                      [TDB output]      is "CH?_sim_rxPCMStream.pcm"
                                      [PDB input]       is "CH?_rxPackets.bin"
                                      [PDB output]      is "CH?_sim_txPackets.bin"
                                      [Encoder input]   is "CH?_encInput.pcm"
                                      [Encoder output]  is "CH?_sim_encOutput.bin"
                                      [Decoder input]   is "CH?_decInput.bin"
                                      [Decoder output]  is "CH?_sim_decOutput.pcm"
                                      [EC Rin]          is "CH?_Rin_EC.pcm"
                                      [EC Sin]          is "CH?_Sin_EC.pcm"
                                      [EC Sout]         is "CH?_sim_Sout_EC.pcm"
                      
    ?: means channel number
*/

typedef struct {                // a data structure used in encoded bitstream files
    unsigned int    codecType;
    int             frameType;
    int             extPara;
    unsigned char   encFrame[PDB_LEN];
} t_encPacket;

void    dspDebugerCfg(int chn, t_dspDebugerCfg *cfg);  // only available in DEBUG version

#ifdef  __cplusplus
}
#endif

#endif

