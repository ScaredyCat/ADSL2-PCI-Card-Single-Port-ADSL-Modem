#ifndef _VID_CODEC_
#define _VID_CODEC_

//#include "xvid.h"

#define VIDEO_TYPE		int
#define VIDEO_FORMAT	int
#define FRAME_TYPE		int
#define VIDEO_NULL		0				//M
#define VIDEO_H261		1
#define VIDEO_H263		2
#define VIDEO_MPEG4     3
#define SIF_NULL			0	//M
#define SIF_SQCIF			1	//M
#define SIF_QCIF			2	//M
#define SIF_CIF				3	//M
#define SIF_4CIF			4	//M
#define SIF_16CIF			5	//M
#define VIDPARAM_FIXED_QUANTIZER		0
#define VIDPARAM_FIXED_FRAME_RATE		1
#define VIDPARAM_QUANTIZER				2
#define VIDPARAM_FRAME_RATE				3
#define VIDPARAM_BIT_RATE				4
#define VIDPARAM_ENCODE_PICTURE_FORMAT	5
#define VIDPARAM_H323_PAYLOAD			6
#define VIDPARAM_DECODE_PICTURE_FORMAT	7
#define VIDPARAM_ENCODE_PARAM			8	//M
#define VIDPARAM_DECODE_PARAM			9	//M
#define VIDPARAM_DECODE_CODING_TYPE		10	//M
#define VIDPARAM_TRAMSMIT_SIZE			11	//add by cckao 0521

#define VIDPARAM_MP4_FRAMERATE  12//hola 0710
#define VIDPARAM_MP4_BITRATE    13//hola 0710
#define VIDPARAM_MP4_QUANT     14//hola 0710

#define VIDPARAM_H263_FRAMERATE  16//hola 1013
#define VIDPARAM_H263_BITRATE    17//hola 1013
#define VIDPARAM_H263_QUANT      18//hola 1013
#define VIDPARAM_H263_TOTALSIZE	 19//hola 1013


//#define VIDPARAM_NOSUPPORT				999
#define Enc_I_FRAME		0					//FRAME_TYPE
#define Enc_P_FRAME		1					//FRAME_TYPE
#define Enc_FRAME_AUTO	2					//FRAME_TYPE

#define DEC_CODINGTYPE_INTRA 0				//20030516
#define DEC_CODINGTYPE_INTER 1				//20030516

#define ENC_MIN_QUANTIZER	1				//20030520
#define ENC_MAX_QUANTIZER	30				//20030520
 
#define PACKET_AMOUNT	100//5

#define MAX_WIDTH	352
#define MAX_HEIGHT	288

struct H261_ENCODE_PARAM{
	unsigned int	iVideoFormat;
	bool		bH323PAYLOAD;		//add by cckao 0521
	int		iBitRate;
	int		iFrameRate;
	unsigned int	iTxSize;			//add by cckao 0521
};

struct H263_ENCODE_PARAM{	
	unsigned int	iVideoFormat;
	bool		bUnrestructedVector;
	bool		bArithmeticCoding;
	bool		bAdvancedPrediction;
	bool		bPbFrames;
	bool		bDeblockingFilter;
	bool		bModifiedQuant;
	bool		bSliceStructure;
	bool		bErrorCompensation;
	bool		bH323PAYLOAD;		//add by cckao 0521
	int			iQuantizer;
	int		iBitRate;
	int		iFrameRate;
	int		iKeyInterval;
	unsigned int	iTxSize;			//add by cckao 0521
};

struct H261_DECODE_PARAM{
	unsigned int	iVideoFormat;
	unsigned int	iFrameRate;
	int				iCodingType;	//for getfunction	//20030516
	bool			bH323PAYLOAD;		//add by cckao 0521
};

struct H263_DECODE_PARAM{
	unsigned int	iVideoFormat;
	unsigned int	iFrameRate;
	int				iCodingType;					//for getfunction	//20030516
	bool			bH323PAYLOAD;		//add by cckao 0521
};

struct IMAGE_DATA{
	int iLines;
	int iPels;
	int color_space; //add by Alan 941209
	unsigned char*	pY;	//M
	unsigned char*	pU;	//M
	unsigned char*	pV;	//M
};

struct CODEC_DATA{
	unsigned char*	packetBuff[PACKET_AMOUNT];
	unsigned int	iEncPacketBuffLen[PACKET_AMOUNT];//M
	int	iValidPacketCount;
	int max_packetBuff;
};

//#ifdef __cplusplus
//extern "C"
//{
//#endif

int	Video_Init();
int	Video_UnInit();

int	Video_EncodeInit(VIDEO_TYPE vidType, void* param);
int	Video_EncodeUnInit();
int	Video_SetEncodeParam(VIDEO_TYPE vidType, int param_id, void *param_val);
int	Video_GetEncodeParam(VIDEO_TYPE vidType, int param_id, void *param_val);
int	Video_EncodeFrame(FRAME_TYPE frameType, int iBufferOffset, IMAGE_DATA *pImageData, CODEC_DATA *pCodec);

int Video_EncodeFrame_ex(FRAME_TYPE frameType, int iBufferOffset, IMAGE_DATA *pImageData);
int Video_EncodeFrame_ex2(FRAME_TYPE frameType, int iBufferOffset, unsigned char *pucInImage,unsigned char *pucOutStream,int iId,int *OutLeng);
int Video_EncodeFrame_ex_GetNextPacket(int *Max,int *curr,unsigned char **packet,int *leng);
int Video_EncodeFrame_ex2_GetNextPacket(int *Max/*from 1*/,int *curr/*from 1*/,unsigned char **outpacket,int *leng,int iId);

int	Video_DecodeInit(VIDEO_TYPE vidType, void* param);
int	Video_DecodeUnInit();
int	Video_SetDecodeParam(VIDEO_TYPE vidType, int param_id, void *param_val);
int	Video_GetDecodeParam(VIDEO_TYPE vidType, int param_id, void *param_val);
int	Video_DecodeFrame(int iBufferOffset, CODEC_DATA* pCodecData, IMAGE_DATA *pImageData);

#ifdef _FOUR_DECODER
int	Video_DecodeInit_ex(int id,VIDEO_TYPE vidType, void* param);
int	Video_DecodeUnInit_ex(int id);
int	Video_SetDecodeParam_ex(int id,VIDEO_TYPE vidType, int param_id, void *param_val);
int	Video_GetDecodeParam_ex(int id,VIDEO_TYPE vidType, int param_id, void *param_val);
int	Video_DecodeFrame_ex(int id,int iBufferOffset, CODEC_DATA* pCodecData, IMAGE_DATA *pImageData);
#endif

//#ifdef __cplusplus
//}
//#endif

#endif //_VID_CODEC_


