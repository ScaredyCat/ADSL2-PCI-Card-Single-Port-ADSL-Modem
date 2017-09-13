/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tr.h
 *
 * $Id: sdp_tr.h,v 1.8 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef SDP_TR_H
#define SDP_TR_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Repeate Time, "r=" 
*/
#define SDP_MAX_TR_OFFSETLIST_SIZE	8

#define SDP_MAX_TR_REPEATINTERVAL_SIZE	64
#define SDP_MAX_TR_ACTIVEDURATION_SIZE	64
#define SDP_MAX_TR_OFFSET_SIZE		64

typedef struct sdpTr {
	char	repeatInterval_[SDP_MAX_TR_REPEATINTERVAL_SIZE];
	char	activeDuration_[SDP_MAX_TR_ACTIVEDURATION_SIZE];
	char	pOffsetList_[SDP_MAX_TR_OFFSETLIST_SIZE][SDP_MAX_TR_OFFSET_SIZE];		/* list of "char*" */
	UINT32	offsetlist_size_;
} SdpTr;

CCLAPI RCODE	sdpTrParse(const char* reptimeText, SdpTr*);
CCLAPI RCODE	sdpTr2Str(SdpTr*, char* strbuf, UINT32* buflen);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TR_H */
