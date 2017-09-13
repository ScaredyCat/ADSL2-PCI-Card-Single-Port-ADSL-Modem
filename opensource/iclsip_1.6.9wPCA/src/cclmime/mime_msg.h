#ifndef CCLMIME_MESSAGE_H
#define CCLMIME_MESSAGE_H

#include "mimetypes/types.h"

#ifdef  __cplusplus
extern "C" {
#endif


/*#include "cm_def.h"*/
#include <common/cm_def.h>
#include "mime_hdr.h"
#include "mime_body.h"


typedef struct _message* MimeMsg;

CCLAPI RCODE	mimeMsgNew(MimeMsg *);
CCLAPI RCODE	mimeMsgNewFromText(MimeMsg *, const char *, size_t);
CCLAPI void		mimeMsgFree(MimeMsg *);

CCLAPI RCODE	mimeMsgGetHdr(MimeMsg, MimeHdr *);
CCLAPI RCODE	mimeMsgSetHdr(MimeMsg, MimeHdr);

CCLAPI RCODE	mimeMsgGetBdy(MimeMsg, MimeBdy *);
CCLAPI RCODE	mimeMsgSetBdy(MimeMsg, MimeBdy);

CCLAPI RCODE	mimeMsgIsMultiPart(MimeMsg, int *);
CCLAPI RCODE	mimeMsgCountParts(MimeMsg, int *);
CCLAPI RCODE	mimeMsgGetPart(MimeMsg, int, MimeMsg *);

CCLAPI RCODE	mimeMsgLines(MimeMsg, size_t *);
CCLAPI RCODE	mimeMsgSize (MimeMsg, size_t *);

CCLAPI RCODE	mimeMsgGetStrm(MimeMsg, stream_t *);
CCLAPI RCODE	mimeMsgSetStrm(MimeMsg, stream_t);



#ifdef  __cplusplus
}
#endif

#endif /* CCLMIME_MESSAGE_H */
