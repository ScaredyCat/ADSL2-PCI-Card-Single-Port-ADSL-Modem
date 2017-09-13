/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * ua_sdp.h
 *
 * $Id: ua_sdp.h,v 1.31 2004/10/08 07:13:58 tyhuang Exp $
 */

#ifndef UA_SDP_H
#define UA_SDP_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sip/sip_cm.h>
#include "ua_cm.h"
#include <sdp/sdp.h>
#include "ua_mgr.h"

/*create a new SDP */
/*input the rtp address and port number, Audio Type and buffer and buflen */
CCLAPI SdpSess uaSDPNew(IN UaMgr,
			IN const char* ,/* rtp ip address*/
			IN CodecLst,	/* audio codec type*/
			IN const int,	/* audio port number*/
			IN CodecLst,	/* video codec type*/
			IN const int,	/* video port number*/
			IN const char*);/*setting for bandwidth */

/*duplicate a sdp object .add by tyhuang 2003.7.4*/
CCLAPI SdpSess uaSDPDup(IN SdpSess);

/*free sdp object*/
CCLAPI RCODE uaSDPFree(IN SdpSess);

/*get rtp address contained in sdp, input buf and buflen for stored address */
CCLAPI RCODE uaSDPGetDestAddress(IN SdpSess,IN OUT char*, IN int);

/*return rtp listen port contained in sdp, if not found then return -1 */
CCLAPI int uaSDPGetMediaPort(IN SdpSess,IN UAMediaType);

/* return number of port 
 * add by tyhuang 2003.10.20	
 */
CCLAPI int uaSDPGetMediaNumOfPort(IN SdpSess _this,IN UAMediaType _mediatype);
/*get media atrribute*/ 
/*original sdp, codec type, media type*/
CCLAPI UAMediaAttr uaSDPGetMediaAttr(IN SdpSess,IN int, IN UAMediaType);

/* get media codec */
CCLAPI 
UaCodec uaSDPGetMediaCodec(IN SdpSess, IN UAMediaType,IN const char*);

/* get media ip address */
CCLAPI RCODE uaSDPGetMediaAddr(IN SdpSess,IN UAMediaType,IN OUT char*, IN int);

/* input SdpSess  and user acceptable mediatype and one codecnumber 
 * if match will return the codec number,else return unknow media type 
 */
CCLAPI int uaSDPGetFirstAcceptCodec(IN SdpSess,IN UAMediaType,IN const int);

CCLAPI CodecLst uaSDPGetCodecList(IN SdpSess,IN UAMediaType);

/*----------- Codec List Operation ------------*/

/*create a new UaCodec object 
 * modified by tyhuang
 */
/* uaCodecNew(codec-type, attribute, ptime, samplerate, channel, codecname) */
CCLAPI UaCodec uaCodecNew(IN int, IN UAMediaAttr, IN int, IN int, IN int, IN char*);
CCLAPI UaCodec uaCodecDup(IN UaCodec);
CCLAPI void	uaCodecFree(IN UaCodec);
CCLAPI int	uaCodecGetType(IN UaCodec);
CCLAPI UAMediaAttr	uaCodecGetAttr(IN UaCodec);
CCLAPI int	uaCodecGetPTime(IN UaCodec);
CCLAPI int	uaCodecGetSamplerate(IN UaCodec);
CCLAPI int	uaCodecGetChannel(IN UaCodec);
CCLAPI char	*uaCodecGetCodecname(IN UaCodec);
CCLAPI char	*uaCodecGetCodecFmtp(IN UaCodec);
CCLAPI RCODE	uaCodecSetCodecFmtp(IN UaCodec,IN const char*);

/*Codec list has the order sequence, first item with the higher priority*/
/*create a new List store Codec*/
CCLAPI CodecLst uaCodecLstNew(void);

/*Free Codec List*/
CCLAPI void uaCodecLstFree(IN CodecLst);

/*Add a Codec into List, put at tail*/
CCLAPI RCODE uaCodecLstAddTail(IN CodecLst,IN UaCodec);

/*Add a Codec into List, put at pos*/
/* input (codeclist, pos, codectype)*/
CCLAPI RCODE uaCodecLstAddAt(IN CodecLst,IN int, IN UaCodec);

/*get a Codec into List, put at pos*/
/* input (codeclist, pos)*/
CCLAPI UaCodec uaCodecLstGetAt(IN CodecLst _this,IN int _pos);

/*Remove a Codec from List, remove UAAudioType or UAVideoType*/
CCLAPI RCODE uaCodecLstRemove(IN CodecLst,IN int);

/*Get Count*/
CCLAPI int uaCodecLstGetCount(IN CodecLst);

/*-------------not test------------------------*/
CCLAPI 
SdpSess uaSDPNewEx(UaMgr _uamgr,
				   const char*	addr,	 /*rtp address*/
				   int	numofaddr,
				   UAMediaAttr	attr,	/* sendrecv,sendonly,reconly */
				   UaSDPMedia	_media,	/*media list*/
				   unsigned int	bandwidth);
				   
CCLAPI
UaSDPMedia uaSDPGetMedia(IN SdpSess _sdp,IN int pos);

CCLAPI 
UaSDPMedia uaSDPMediaNew(IN UAMediaType type,
						 IN const char* address,
						 IN unsigned int numberofaddr,
						 IN unsigned int port,
						 IN unsigned int bandwidth,
						 IN unsigned int ptime,
						 IN UaCodec _codec);
CCLAPI void uaSDPMediaFree(IN UaSDPMedia);
CCLAPI RCODE uaSDPMediaAddCodec(IN UaSDPMedia, IN UaCodec);
CCLAPI RCODE uaSDPMediaAddCodecAt(IN UaSDPMedia,IN int, IN UaCodec);
CCLAPI UaCodec uaSDPMediaGetCodecAt(IN UaSDPMedia,IN int );
CCLAPI RCODE uaSDPMediaRemoveCodec(IN UaSDPMedia,IN int );
CCLAPI int uaSDPMediaGetCodecCount(IN UaSDPMedia);
CCLAPI RCODE uaSDPMediaSetAttr(IN UaSDPMedia ,IN UAMediaAttr);
CCLAPI UAMediaAttr	uaSDPMediaGetAttr(IN UaSDPMedia);
CCLAPI UAMediaType	uaSDPMediaGetType(UaSDPMedia);
CCLAPI char* uaSDPMediaGetAddress(UaSDPMedia);
CCLAPI unsigned int uaSDPMediaGetNumOfAddress(UaSDPMedia );
CCLAPI unsigned int uaSDPMediaGetPort(UaSDPMedia);
CCLAPI int	uaSDPMediaGetPTime(IN UaSDPMedia);
CCLAPI unsigned int uaSDPMediaGetBandwidth(UaSDPMedia );

CCLAPI char* uaSDPGetSessionConnection(IN SdpSess _this);
CCLAPI UAMediaAttr uaSDPGetSessionAttr(IN SdpSess _this);
/*-------------not implement end------------------------*/

/*----------- Internal APIs -----------*/
/*duplicate a sdp and change into hold sdp*/ 
/* int: 0 based on rfc2543, 1 based on rfc3264*/
SdpSess uaSDPHoldSDP(IN SdpSess,IN int);


#ifdef  __cplusplus
}
#endif

#endif /* UA_SDP_H */
