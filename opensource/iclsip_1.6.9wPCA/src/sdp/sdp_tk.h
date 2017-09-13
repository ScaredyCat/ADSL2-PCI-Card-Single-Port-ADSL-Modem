/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tk.h
 *
 * $Id: sdp_tk.h,v 1.8 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef SDP_TK_H
#define SDP_TK_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

/*---------------------------------------------
// SDP Type Encryption Key, "k="
*/
#define SDP_TK_METHOD_NONE	0x0
#define SDP_TK_METHOD_PROMPT	0x1
#define SDP_TK_METHOD_CLEAR	0x2
#define SDP_TK_METHOD_BASE64	0x3
#define SDP_TK_METHOD_URI	0x4

#define SDP_MAX_TK_KEY_SIZE	64

typedef struct sdpTk {
	UINT16		pMethod_; /* = SDP_ENCRYKEY_METHOD_XXXX */
	char		pKey_[SDP_MAX_TK_KEY_SIZE];				
} SdpTk;

CCLAPI RCODE		sdpTkParse(const char* encrykeyText, SdpTk*);
CCLAPI RCODE		sdpTk2Str(SdpTk*, char* strbuf, UINT32* buflen);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TK_H */
