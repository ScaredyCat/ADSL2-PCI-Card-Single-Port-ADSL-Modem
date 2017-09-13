/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tc.h
 *
 * $Id: sdp_tc.h,v 1.8 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef SDP_TC_H
#define SDP_TC_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Connection, "c="
*/
#define SDP_TC_FLAG_NONE		0x0
#define SDP_TC_FLAG_TTL			0x1
#define SDP_TC_FLAG_NUMOFADDR		0x2

#define SDP_MAX_TC_CONNADDR_SIZE	128

typedef struct sdpTc {
	UINT32		conn_flags_;

	UINT16		networkType_;
	UINT16		addressType_;
	char		pConnAddr_[SDP_MAX_TC_CONNADDR_SIZE];
	UINT32		ttl_;			/* must set conn_flags_ */
	UINT32		numOfAddr_;		/* must set conn_flags_ */
} SdpTc;

CCLAPI RCODE		sdpTcParse(const char* conndataText, SdpTc*);
CCLAPI RCODE		sdpTc2Str(SdpTc*, char* strbuf, UINT32* buflen);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TC_H */
