/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tt.h
 *
 * $Id: sdp_tt.h,v 1.8 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef SDP_TT_H
#define SDP_TT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Time, "t="
*/

#define SDP_MAX_TT_STARTTIME_SIZE	64
#define SDP_MAX_TT_ENDTIME_SIZE		64

typedef struct sdpTt {
	char		StartTime_[SDP_MAX_TT_STARTTIME_SIZE];
	char		Endtime_[SDP_MAX_TT_ENDTIME_SIZE];
} SdpTt;

CCLAPI RCODE		sdpTtParse(const char* timeText, SdpTt*);
CCLAPI RCODE		sdpTt2Str(SdpTt*, char* strbuf, UINT32* buflen);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TT_H */
