#ifndef CCLMIME_MEM_STREAM_H
#define CCLMIME_MEM_STREAM_H

#include "mimetypes/types.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*#include "cm_def.h"*/
#include <common/cm_def.h>

CCLAPI RCODE	mimeMemStrmNew(stream_t *, const char *, int flags);

#ifdef  __cplusplus
}
#endif

#endif /* CCLMIME_MEM_STREAM_H */