#ifndef CCLMIME_MIME_H
#define CCLMIME_MIME_H

#include "mimetypes/types.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
#include "cm_def.h"
*/
#include <common/cm_def.h>

typedef struct _mime* MimeMime;

CCLAPI RCODE	mimeMimeNew(MimeMime *, MimeMsg, int);
CCLAPI void		mimeMimeFree(MimeMime *);
CCLAPI RCODE	mimeMimeGetNumParts(MimeMime, int *);
CCLAPI RCODE	mimeMimeGetPart(MimeMime, size_t, MimeMsg *);
CCLAPI RCODE	mimeMimeAddPart(MimeMime, MimeMsg);
CCLAPI RCODE	mimeMimeGetMsg(MimeMime, MimeMsg *);
CCLAPI RCODE	mimeMimeGetBoundary(MimeMime, char *);
CCLAPI RCODE	mimeMimeSetBoundary(MimeMime, char *);
CCLAPI RCODE	mimeMimeGetBoundaryLen(MimeMime, int *);


#ifdef  __cplusplus
}
#endif

#endif /* CCLMIME_MIME_H */