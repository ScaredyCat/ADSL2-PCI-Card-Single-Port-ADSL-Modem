/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_ta.h
 *
 * $Id: sdp_ta.h,v 1.8 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef sdpTa_H
#define sdpTa_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Attribute, "a="
*/

#define	SDP_TA_FLAG_NONE		0x0
#define SDP_TA_FLAG_VALUE		0x1

#define SDP_MAX_TA_ATTRIB_SIZE		32
#define SDP_MAX_TA_VALUE_SIZE		128

typedef struct sdpTa {
	UINT32		sessionatt_flag_;

	char		pAttrib_[SDP_MAX_TA_ATTRIB_SIZE];
	char		pValue_[SDP_MAX_TA_VALUE_SIZE];
} SdpTa;

CCLAPI RCODE		sdpTaParse(const char* attText, SdpTa*);

/* sdpTa2Str() 
	It's suggested that $strbuf's length $buflen>=SDP_MAX_TA_2STR_SIZE,
	or sdpTa2Str() will return NULL when $strbuf is not big enough.
 */
CCLAPI RCODE		sdpTa2Str(SdpTa*, char* strbuf, UINT32* buflen);

#ifdef  __cplusplus
}
#endif

#endif /* sdpTa_H */
