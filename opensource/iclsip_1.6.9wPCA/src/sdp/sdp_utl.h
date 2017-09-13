/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_utl.h
 *
 * $Id: sdp_utl.h,v 1.7 2004/09/24 05:47:14 tyhuang Exp $
 */

#ifndef SDP_UTILS_H
#define SDP_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <common/cm_def.h>
#include "sdp_def.h"

RCODE	sdpTpNewFromText(char* phonenumText, char** phonenum);
RCODE	sdpTeNewFromText(char* emailText, char** email);
RCODE	sdpTuNewFromText(char* uriText, char** uri);
RCODE	sdpTsNewFromText(char* nameText, char** name);
RCODE	sdpSiNewFromText(char* infoText, char** info);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_UTILS_H */
