#ifndef CCLMIME_HDR_H
#define CCLMIME_HDR_H

/* #include "mimetypes/types.h" */

#ifdef  __cplusplus
extern "C" {
#endif

/*#include "cm_def.h"*/
#include <common/cm_def.h>

#define MU_HEADER_UNIX_FROM                 "From "
#define MU_HEADER_RETURN_PATH               "Return-Path"
#define MU_HEADER_RECEIVED                  "Received"
#define MU_HEADER_DATE                      "Date"
#define MU_HEADER_FROM                      "From"
#define MU_HEADER_SENDER                    "Sender"
#define MU_HEADER_RESENT_FROM               "Resent-From"
#define MU_HEADER_SUBJECT                   "Subject"
#define MU_HEADER_SENDER                    "Sender"
#define MU_HEADER_RESENT_SENDER             "Resent-SENDER"
#define MU_HEADER_TO                        "To"
#define MU_HEADER_RESENT_TO                 "Resent-To"
#define MU_HEADER_CC                        "Cc"
#define MU_HEADER_RESENT_CC                 "Resent-Cc"
#define MU_HEADER_BCC                       "Bcc"
#define MU_HEADER_RESENT_BCC                "Resent-Bcc"
#define MU_HEADER_REPLY_TO                  "Reply-To"
#define MU_HEADER_RESENT_REPLY_TO           "Resent-Reply-To"
#define MU_HEADER_MESSAGE_ID                "Message-ID"
#define MU_HEADER_RESENT_MESSAGE_ID         "Resent-Message-ID"
#define MU_HEADER_IN_REPLY_TO               "In-Reply-To"
#define MU_HEADER_REFERENCE                 "Reference"
#define MU_HEADER_REFERENCES                "References"
#define MU_HEADER_ENCRYPTED                 "Encrypted"
#define MU_HEADER_PRECEDENCE                "Precedence"
#define MU_HEADER_STATUS                    "Status"
#define MU_HEADER_CONTENT_LENGTH            "Content-Length"
#define MU_HEADER_CONTENT_LANGUAGE          "Content-Language"
#define MU_HEADER_CONTENT_TRANSFER_ENCODING "Content-transfer-encoding"
#define MU_HEADER_CONTENT_ID                "Content-ID"
#define MU_HEADER_CONTENT_TYPE              "Content-Type"
#define MU_HEADER_CONTENT_DESCRIPTION       "Content-Description"
#define MU_HEADER_CONTENT_DISPOSITION       "Content-Disposition"
#define MU_HEADER_CONTENT_MD5               "Content-MD5"
#define MU_HEADER_MIME_VERSION              "MIME-Version"
#define MU_HEADER_X_UIDL                    "X-UIDL"
#define MU_HEADER_X_UID                     "X-UID"
#define MU_HEADER_X_IMAPBASE                "X-IMAPbase"



typedef header_t	MimeHdr;

CCLAPI RCODE	mimeHdrNew(MimeHdr *, const char *, int);
CCLAPI void		mimeHdrFree(MimeHdr *);

CCLAPI RCODE	mimeHdrSetVal(MimeHdr, const char *, const char *);
CCLAPI RCODE	mimeHdrGetVal(MimeHdr, const char *, char *, size_t , size_t *);

CCLAPI RCODE	mimeHdrCountFields(MimeHdr, size_t *);
CCLAPI RCODE	mimeHdrGetFieldName(MimeHdr, size_t, char *, size_t, size_t *);
CCLAPI RCODE	mimeHdrGetFieldValue(MimeHdr, size_t, char *, size_t, size_t *);

CCLAPI RCODE	mimeHdrGetStrm(MimeHdr, stream_t *);
CCLAPI RCODE	mimeHdrSetStrm(MimeHdr, stream_t);
CCLAPI RCODE	mimeHdrLines(MimeHdr , size_t *);
CCLAPI RCODE	mimeHdrSize(MimeHdr header, size_t *psize);


#ifdef  __cplusplus
}
#endif

#endif /* CCLMIME_HDR_H */
