#ifndef CCLMIME_STREAM_H
#define CCLMIME_STREAM_H

#include "mimetypes/types.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
#include "cm_def.h"
*/
#include <common/cm_def.h>

#define MU_STREAM_READ	   0x00000001
#define MU_STREAM_WRITE	   0x00000002
#define MU_STREAM_RDWR     0x00000004
#define MU_STREAM_APPEND   0x00000008
#define MU_STREAM_CREAT	   0x00000010
#define MU_STREAM_NONBLOCK 0x00000020
/* Stream will be destroy on stream_destroy without checking the owner. */
#define MU_STREAM_NO_CHECK 0x00000040
#define MU_STREAM_SEEKABLE 0x00000080
#define MU_STREAM_NO_CLOSE 0x00000100


#define MU_STREAM_STATE_OPEN  1
#define MU_STREAM_STATE_READ  2
#define MU_STREAM_STATE_WRITE 4
#define MU_STREAM_STATE_CLOSE 8


CCLAPI RCODE	mimeStrmNew(stream_t *, int);
CCLAPI void		mimeStrmFree(stream_t *pstream);
CCLAPI RCODE	mimeStrmOpen (stream_t stream);
CCLAPI RCODE	mimeStrmClose(stream_t stream);

CCLAPI RCODE	mimeStrmRead(stream_t, char *, size_t, off_t, size_t *);
CCLAPI RCODE	mimeStrmReadLine(stream_t is, char *buf, size_t count, 
					     int offset, int *pnread);
CCLAPI RCODE	mimeStrmWrite(stream_t os, const char *buff, size_t buffSize, 
					     off_t offset, size_t *pnwrite);


#ifdef  __cplusplus
}
#endif

#endif /* CCLMIME_STREAM_H */
