/*=========================================================================*\
  Copyright (c) 2000-2002 Computer & Communications Research Laboratories,
                          Industrial Technology Research Institute
  
  RTP error
  
  Author: Jiun-Yao Huang <jyhuang@wizs.org>
  Revision: $Id: rtp_error.h,v 1.7 2002/01/03 05:08:16 jyhuang Exp $
  Description:
\*=========================================================================*/
#ifndef	__RTP_ERROR_H__
#define	__RTP_ERROR_H__

#define	RTP_EINVAL	1	/* Invalid argument */
#define	RTP_EIO		2	/* I/O error */
#define	RTP_EVERSION	3	/* incorrect RTP version */
#define	RTP_EPKT	4	/* bad rtp packet */
#define	RTP_ETIMEOUT	5	/* time out */
#define	RTP_ECONN	6	/* error on connection */
#define	RTP_ESEQ	7	/* invalid seq # */
#define	RTP_EBUF	8	/* error on buffer 
                                   (e.g., insubfficient buffer size) */
#define	RTP_EUNKNOWN	99	/* unknown error */

#endif /* #ifndef __RTP_ERROR_H__ */
