/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 *
 *
 * sipAcc.h
 *
 * $Id: sipAcc.h,v 1.3 2004/09/24 06:02:01 tyhuang Exp $
 */

#include <common/cm_def.h>
#include <common/cm_trace.h>

RCODE	accCommon(void);
RCODE	accAdt(void);
RCODE	accLow(void);
RCODE	accSDP(void);
RCODE	accSip(void);
