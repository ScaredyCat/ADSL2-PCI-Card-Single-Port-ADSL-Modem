/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_sess.h
 *
 * $Id: sdp_sess.h,v 1.11 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef SDP_SESS_H
#define SDP_SESS_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_to.h"
#include "sdp_tc.h"
#include "sdp_tk.h"
#include "sdp_tb.h"
#include "sdp_tses.h"
#include "sdp_tz.h"
#include "sdp_ta.h"
#include "sdp_mses.h"
#include "sdp_def.h"

#define SDP_DEFAULT_LIST_SIZE		8

/* version of SDP*/
#define SDP_VERSION			0x0

/* Network type*/
#define SDP_NETTYPE_NONE		0x0
#define SDP_NETTYPE_IN			0x1

/* Address type*/
#define SDP_ADDRTYPE_NONE		0x0
#define SDP_ADDRTYPE_IP4		0x1
#define SDP_ADDRTYPE_IP6		0x2

typedef struct sdpSessObj*	SdpSess;

/*--------------------------------------------------------------------------
// sdpsession
*/

CCLAPI SdpSess	sdpSessNew(void);
CCLAPI SdpSess	sdpSessNewFromText(const char* sdpText);
CCLAPI void	sdpSessFree(SdpSess);
CCLAPI RCODE	sdpSess2Str(SdpSess, char* strbuf, UINT32* buflen);
CCLAPI SdpSess	sdpSessDup(SdpSess);

CCLAPI RCODE	sdpSessSetTv(SdpSess, UINT16 version);
CCLAPI RCODE	sdpSessGetTv(SdpSess, UINT16 *version);

CCLAPI RCODE	sdpSessSetTo(SdpSess, SdpTo*);
CCLAPI RCODE	sdpSessGetTo(SdpSess, SdpTo*);

CCLAPI RCODE	sdpSessSetTs(SdpSess, char* name);
CCLAPI RCODE	sdpSessGetTs(SdpSess, char* strbuf, UINT32* buflen);

CCLAPI RCODE	sdpSessSetTi(SdpSess, char* info);
CCLAPI RCODE	sdpSessGetTi(SdpSess, char* strbuf, UINT32* buflen);

CCLAPI RCODE	sdpSessSetTu(SdpSess, char* uri);
CCLAPI RCODE	sdpSessGetTu(SdpSess, char* strbuf, UINT32* buflen);

CCLAPI RCODE	sdpSessAddTeAt(SdpSess, char* email, int index);
CCLAPI RCODE	sdpSessGetTeAt(SdpSess, int index, char* strbuf, UINT32* buflen);
CCLAPI RCODE	sdpSessDelTeAt(SdpSess, int index);
CCLAPI int	sdpSessGetTeSize(SdpSess);

CCLAPI RCODE	sdpSessAddTpAt(SdpSess, char* phnnum, int index);
CCLAPI RCODE	sdpSessGetTpAt(SdpSess, int index, char* strbuf, UINT32* buflen);
CCLAPI RCODE	sdpSessDelTpAt(SdpSess, int index);
CCLAPI int	sdpSessGetTpSize(SdpSess);

CCLAPI RCODE	sdpSessSetTc(SdpSess, SdpTc*);
CCLAPI RCODE	sdpSessGetTc(SdpSess, SdpTc*);

CCLAPI RCODE	sdpSessSetTk(SdpSess, SdpTk*);
CCLAPI RCODE	sdpSessGetTk(SdpSess, SdpTk*);

CCLAPI RCODE	sdpSessAddTbAt(SdpSess, SdpTb*, int index);
CCLAPI RCODE	sdpSessGetTbAt(SdpSess, int index, SdpTb*);
CCLAPI RCODE	sdpSessDelTbAt(SdpSess, int index);
CCLAPI int	sdpSessGetTbSize(SdpSess);

CCLAPI RCODE	sdpSessAddTsessAt(SdpSess, SdpTsess, int index);
CCLAPI SdpTsess	sdpSessGetTsessAt(SdpSess, int index);
CCLAPI RCODE	sdpSessDelTsessAt(SdpSess, int index);
CCLAPI int	sdpSessGetTsessSize(SdpSess);

CCLAPI RCODE	sdpSessSetTz(SdpSess, SdpTz*);
CCLAPI RCODE	sdpSessGetTz(SdpSess, SdpTz*);

CCLAPI RCODE	sdpSessAddTaAt(SdpSess, SdpTa*, int index);
CCLAPI RCODE	sdpSessGetTaAt(SdpSess, int index, SdpTa*);
CCLAPI RCODE	sdpSessDelTaAt(SdpSess, int index);
CCLAPI int	sdpSessGetTaSize(SdpSess);

CCLAPI RCODE	sdpSessAddMsessAt(SdpSess, SdpMsess, int index);
CCLAPI SdpMsess	sdpSessGetMsessAt(SdpSess, int index);
CCLAPI RCODE	sdpSessDelMsessAt(SdpSess, int index);
CCLAPI int	sdpSessGetMsessSize(SdpSess);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_SESS_H */
