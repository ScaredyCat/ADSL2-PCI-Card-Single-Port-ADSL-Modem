/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tses.h
 *
 * $Id: sdp_tses.h,v 1.8 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef SDP_TSES_H
#define SDP_TSES_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_tt.h"
#include "sdp_tr.h"
#include "sdp_def.h"

typedef struct sdpTsessObj*	SdpTsess;

CCLAPI SdpTsess	sdpTsessNew(void);
CCLAPI SdpTsess	sdpTsessDup(SdpTsess);
CCLAPI void	sdpTsessFree(SdpTsess);
CCLAPI RCODE	sdpTsess2Str(SdpTsess, char* strbuf, UINT32* buflen);

CCLAPI RCODE	sdpTsessSetTt(SdpTsess, SdpTt*);
CCLAPI RCODE	sdpTsessGetTt(SdpTsess, SdpTt*);

CCLAPI RCODE	sdpTsessAddTrAt(SdpTsess, SdpTr*, int index);
CCLAPI RCODE	sdpTsessDelTrAt(SdpTsess, int index);
CCLAPI int	sdpTsessGetTrSize(SdpTsess);
CCLAPI RCODE	sdpTsessGetTrAt(SdpTsess, int index, SdpTr*);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TSES_H */
