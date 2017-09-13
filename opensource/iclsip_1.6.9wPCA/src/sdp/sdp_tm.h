/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tm.h
 *
 * $Id: sdp_tm.h,v 1.10 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef SDP_TM_H
#define SDP_TM_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Media Announcement, "m="
*/
#define SDP_TM_FLAG_NONE		0x0
#define SDP_TM_FLAG_NUMOFPORT		0x1
#define SDP_TM_FLAG_PORT_CHOOSE		0x2

#define	SDP_TM_MEDIA_TYPE_NONE 		0x0
#define	SDP_TM_MEDIA_TYPE_AUDIO		0x1
#define	SDP_TM_MEDIA_TYPE_VIDEO		0x2
#define	SDP_TM_MEDIA_TYPE_APPLICATION	0x3
#define	SDP_TM_MEDIA_TYPE_DATA		0x4
#define	SDP_TM_MEDIA_TYPE_UNKNOWN       0x5

#define	SDP_TM_PROTOCOL_NONE		0x0
#define	SDP_TM_PROTOCOL_RTPAVP		0x1
#define	SDP_TM_PROTOCOL_UDP		0x2

#define SDP_MAX_TM_TOKEN_SIZE           64
#define SDP_MAX_TM_FMT_SIZE		8

typedef struct sdpTm {
	UINT32          Tm_flags_;

	UINT16          Tm_type_;
	char            unknown_tm_type_[SDP_MAX_TM_TOKEN_SIZE];
	
	UINT16          port_;		
	/* set flag to SDP_TM_FLAG_PORT_CHOOSE for "$" */
	UINT16          numOfPort_;	/* must set flag */
	UINT16          protocol_;

	char            fmtList_[SDP_MAX_TM_FMT_SIZE][SDP_MAX_TM_TOKEN_SIZE];
	UINT16          fmtSize_;	
	/* fmtSize must between 1 ~ SDP_MAX_TM_FMT_SIZE */
} SdpTm;

CCLAPI RCODE		sdpTmParse(const char* mediaText, SdpTm*);   
CCLAPI RCODE		sdpTm2Str(SdpTm*, char* strbuf, UINT32* buflen);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TM_H */

