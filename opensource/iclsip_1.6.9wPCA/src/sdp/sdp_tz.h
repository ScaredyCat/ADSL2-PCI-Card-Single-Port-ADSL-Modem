/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tz.h
 *
 * $Id: sdp_tz.h,v 1.8 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef SDP_TZ_H
#define SDP_TZ_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Time Zone Adjustment, "z="
*/
#define SDP_MAX_TZ_ADJUSTMENT_SIZE		8

#define SDP_MAX_TT_ADJUSTMENT_ADJTIME_SIZE	64
#define SDP_MAX_TT_ADJUSTMENT_OFFSET_SIZE	64

typedef struct sdpTz 
{
	struct adjustment {
		char	pAdjTime_[SDP_MAX_TT_ADJUSTMENT_ADJTIME_SIZE];
		char	pOffset_[SDP_MAX_TT_ADJUSTMENT_OFFSET_SIZE];
	} adjustment_[SDP_MAX_TZ_ADJUSTMENT_SIZE];
	UINT32		adjustment_size_;	/* size > 0 */
} SdpTz;

CCLAPI RCODE		sdpTzParse(const char* timeadjText, SdpTz*);
CCLAPI RCODE		sdpTz2Str(SdpTz*, char* strbuf, UINT32* buflen);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TZ_H */
