/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * sdp_tok.h
 *
 * $Id: sdp_tok.h,v 1.6 2004/02/25 03:33:51 ljchuang Exp $
 */

#ifndef SDP_TOK_H
#define SDP_TOK_H

#ifdef  __cplusplus
extern "C" {
#endif

/* sdp fields tokens */
#define SDP_TV_TOKEN			"v="
#define SDP_TO_TOKEN			"o="
#define SDP_TS_TOKEN			"s="
#define SDP_TI_TOKEN			"i="
#define SDP_TU_TOKEN			"u="
#define SDP_TE_TOKEN			"e="
#define SDP_TP_TOKEN			"p="
#define SDP_TC_TOKEN			"c="
#define SDP_TR_TOKEN			"r="
#define SDP_TZ_TOKEN			"z="
#define SDP_TT_TOKEN			"t="
#define SDP_TK_TOKEN			"k="
#define SDP_TA_TOKEN			"a="
#define SDP_TM_TOKEN			"m="
#define SDP_TB_TOKEN			"b="

#define SDP_CRLF			"\r\n"

typedef struct sdpToken {
	char*			pToken;
	struct sdpToken*	pNext;
} SdpListElm;

typedef SdpListElm*		SdpList;

/* methods */
/**************************************************************
* isCRLF :
*	Check if the given string is SDP_CRLF(SDP_CRLF).
*	return 0 if false, 1 if true.
**************************************************************/
int isCRLF(const char* str);

/**************************************************************
* sdpTokParse :
*	Parse text into SDP token list.
*
**************************************************************/
SdpList sdpTokParse(const char* pSdpStr);
SdpList sdpTokParseC(const char* pSdpStr);

/**************************************************************
* sdpLineParse :
*	Parse text into Line list.
*
**************************************************************/
SdpList sdpLineParse(const char* pSdpStr);

/**************************************************************
* sdpListFree :
*	Free the allocated memory of SdpList.
*
**************************************************************/
void sdpListFree(SdpList tokList);

#ifdef  __cplusplus
}
#endif

#endif /* SDP_TOK_H */
