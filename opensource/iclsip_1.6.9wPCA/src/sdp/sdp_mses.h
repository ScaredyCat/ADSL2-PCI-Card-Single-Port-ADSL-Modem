/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_mses.h
 *
 * $Id: sdp_mses.h,v 1.10 2006/12/05 09:41:33 tyhuang Exp $
 */

#ifndef SDP_MSES_H
#define SDP_MSES_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_tm.h"
#include "sdp_tb.h"
#include "sdp_tk.h"
#include "sdp_tc.h"
#include "sdp_ta.h"
#include "sdp_def.h"

typedef struct sdpMsessObj*	SdpMsess;

CCLAPI SdpMsess	sdpMsessNew(void);
CCLAPI SdpMsess	sdpMsessDup(SdpMsess);
CCLAPI void	sdpMsessFree(SdpMsess);
CCLAPI RCODE	sdpMsess2Str(SdpMsess, char* strbuf, UINT32* buflen);

CCLAPI RCODE	sdpMsessSetTm(SdpMsess, SdpTm*);
CCLAPI RCODE	sdpMsessGetTm(SdpMsess, SdpTm*);

CCLAPI RCODE	sdpMsessSetTi(SdpMsess, char* title);
CCLAPI RCODE	sdpMsessGetTi(SdpMsess, char* strbuf, UINT32* buflen);

CCLAPI RCODE	sdpMsessSetTb(SdpMsess, SdpTb*);
CCLAPI RCODE	sdpMsessGetTb(SdpMsess, SdpTb*);

CCLAPI RCODE	sdpMsessSetTk(SdpMsess, SdpTk*);
CCLAPI RCODE	sdpMsessGetTk(SdpMsess, SdpTk*);

CCLAPI RCODE	sdpMsessAddTcAt(SdpMsess, SdpTc*, int index);
CCLAPI RCODE	sdpMsessDelTcAt(SdpMsess, int index);
CCLAPI int	sdpMsessGetTcSize(SdpMsess);
CCLAPI RCODE	sdpMsessGetTcAt(SdpMsess, int index, SdpTc*);

CCLAPI RCODE	sdpMsessAddTaAt(SdpMsess, SdpTa*, int index);
CCLAPI RCODE	sdpMsessDelTaAt(SdpMsess, int index);
CCLAPI int	sdpMsessGetTaSize(SdpMsess);
CCLAPI RCODE	sdpMsessGetTaAt(SdpMsess, int index, SdpTa*);

CCLAPI RCODE	sdpMsessAddTbAt(SdpMsess, SdpTb*, int index);
CCLAPI RCODE	sdpMsessDelTbAt(SdpMsess, int index);
CCLAPI int	sdpMsessGetTbSize(SdpMsess);
CCLAPI RCODE	sdpMsessGetTbAt(SdpMsess, int index, SdpTb*);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_MSES_H */
