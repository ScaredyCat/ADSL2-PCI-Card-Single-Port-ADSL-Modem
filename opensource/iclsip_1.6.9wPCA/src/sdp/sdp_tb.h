/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tb.h
 *
 * $Id: sdp_tb.h,v 1.9 2006/12/05 09:41:33 tyhuang Exp $
 */

#ifndef SDP_TB_H
#define SDP_TB_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Bandwidth, "b="
*/
#define SDP_TB_MODIFIER_NONE			0x0
#define SDP_TB_MODIFIER_CONFERENCE_TOTAL	0x1
#define SDP_TB_MODIFIER_APPLICATION_SPECIFIC	0x2		
#define SDP_TB_MODIFIER_EXTENSION		0x3

#define SDP_MAX_TB_EXTENTIONNAME_SIZE		32

static char BANDWIDTH_MODIFIER_TOKEN[7][5] = {
	"NONE",
	"CT",
	"AS",
	"X-",
	"RR",
	"RS",
	"\0"
};

typedef struct sdpTb {
	UINT16		modifier_;
	char		pExtentionName[SDP_MAX_TB_EXTENTIONNAME_SIZE];
	UINT32		bandwidth_;
} SdpTb;

CCLAPI RCODE		sdpTbParse(const char* bandwidthText, SdpTb*);
CCLAPI RCODE		sdpTb2Str(SdpTb*, char* strbuf, UINT32* buflen);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TB_H */
