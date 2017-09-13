/* 
 * Copyright (C) 2000-2002 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/* 
 * ua_content.h
 *
 * $Id: ua_content.h,v 1.5 2004/09/24 05:45:32 tyhuang Exp $
 */
#ifndef UA_CONTENT_H
#define UA_CONTENT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "ua_cm.h"
#include <stdio.h>

/* create a UaContent object */
CCLAPI UaContent uaContentNew(IN char*, IN int , IN void*);

/* free a UaContent object */
CCLAPI void uaContentFree(IN UaContent);

/* duplicate a object */
CCLAPI UaContent uaContentDup(IN UaContent);

/*get content type*/
CCLAPI char* uaContentGetType(IN UaContent);

/*get content length */
CCLAPI int uaContentGetLength(IN UaContent);

/*get content */
CCLAPI void* uaContentGetBody(IN UaContent); 

#ifdef  __cplusplus
}
#endif

#endif
