#ifndef CCLMIME_BODY_H
#define CCLMIME_BODY_H

/* #include "mimetypes/types.h" */

#ifdef  __cplusplus
extern "C" {
#endif

/*#include "cm_def.h"*/
#include <common/cm_def.h>


typedef body_t	MimeBdy;

CCLAPI RCODE	mimeBdyNew (MimeBdy *);
CCLAPI void		mimeBdyFree (MimeBdy *);
CCLAPI RCODE	mimeBdyLines (MimeBdy, size_t *);
CCLAPI RCODE	mimeBdySize (MimeBdy, size_t *);
CCLAPI RCODE	mimeBdyGetStrm(MimeBdy, stream_t *);
CCLAPI RCODE	mimeBdySetStrm(MimeBdy, stream_t);


#ifdef  __cplusplus
}
#endif

#endif /* CCLMIME_BODY_H */