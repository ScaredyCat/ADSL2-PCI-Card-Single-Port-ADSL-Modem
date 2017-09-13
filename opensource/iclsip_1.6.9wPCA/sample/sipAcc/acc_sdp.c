/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * acc_sdp.c
 *
 * $Id: acc_sdp.c,v 1.5 2004/12/09 10:52:39 tyhuang Exp $
 */
#include <stdio.h>
#include <stdlib.h>

#include <common/cm_def.h>
#include <sdp/sdp.h>
#include <common/cm_trace.h>

#define	TO_TST_STR	"o=iolo 2890844526 2890842807 IN IP4 140.96.102.166\r\n"
#define	TC_TST_STR	"c=IN IP4 $\r\n"
#define	TR_TST_STR	"r=604800 3600 0 90000\r\n"
#define	TA_TST_STR	"a=rtpmap:96 L8/8000\r\n"
#define	TZ_TST_STR	"z=2882844526 -1h 2898848240 0\r\n"
#define	TT_TST_STR	"t=2882844526 2898848240\r\n"
#define	TB_TST_STR	"b=X-ABC:128\r\n"
#define	TK_TST_STR	"k=clear:123456\r\n"
#define	TM_TST_STR	"m=audio 49170 RTP/AVP 0\r\n"
const char	SDPSESS_TST_STR[700] = {
			"v=0\r\n"
			"o=iolo 2890844526 2890842807 IN IP4 140.96.102.166\r\n"
			"s=SJTSAI\r\n"
			"i=This is a test SDP\r\n"
			"u=http://www.kimo.com.tw\r\n"
			"e=sjtsai@csie.nctu.edu.tw\r\n"
			"e=sjtsai@itri.org.tw\r\n"
			"p=03-5914498\r\n"
			"p=03-5834016\r\n"
			"c=IN IP4 140.96.102.166/5\r\n"
			"b=X-ABC:128\r\n"
			"t=2882844526 2898848240\r\n"
			"r=604800 3600 0 90000\r\n"
			"r=123455 1223 0 12333\r\n"
			"t=1234567890 1234567899\r\n"
			"z=2882844526 -1h 2898848070 0\r\n"
			"k=base64:abcdefg\r\n"
			"a=SENDRECV\r\n"
			"m=audio 1214/3 RTP/AVP 0\r\n"
			"m=video 51237 RTP/AVP 31\r\n"
			"i=What is this??\r\n"
			"c=IN IP4 140.96.102.166/5\r\n"
			"b=X-ABC:128\r\n"
			"k=base64:abcdefg\r\n"
			"m=audio 2345 RTP/AVP 96 97\r\n"
			"a=rtpmap:96 L8/8000\r\n"
			"a=rtpmap:97 L16/8000\r\n"
			"m=video 51231 RTP/AVP 31\r\n"
			"m=ctrl 51232 udp test\r\n"
			};

RCODE accSdpSess(SdpSess sdp)
{
	int i = 0;

	/**************************************************** 
		Access Media Session (m=)
	****************************************************/
	for( i=0; i<sdpSessGetMsessSize(sdp); i++) {
		SdpMsess	mediaSess = sdpSessGetMsessAt(sdp,i);
		SdpTm		media;

		if( sdpMsessGetTm(mediaSess,&media)!=RC_OK ) {
			printf( "\n!!ERROR!! %s:%d: sdpMsessGetTm() error.\n",
				__CMFILE__,__LINE__);
			return RC_ERROR;
		}
	}

	/**************************************************** 
		Access SDP Session name (s=)
	****************************************************/
	{
		char*	sessname = (char*)malloc(128);
		int	ret;
		UINT32	len = 128;

		ret = sdpSessGetTs(sdp,sessname,&len);
		if ( ret==RC_SDP_VALUE_NOT_SET ) {
			printf( "\n!!ERROR!! %s:%d: Session name is not set.\n",
				__CMFILE__,__LINE__);
			return RC_ERROR;
		}
		else if ( ret==RC_SDP_BUFFER_LENGTH_INSUFFICIENT ) {
			sessname = (char*)realloc(sessname,len);
			ret = sdpSessGetTs(sdp,sessname,&len);
		}	
		free(sessname);
	}
	return RC_OK;
}

RCODE	accSDP(void)
{
	SdpTo		sdpO;
	SdpTc		sdpConn;
	SdpTz		timeAdj;
	SdpTt		time;
	SdpTr		repTime;
	SdpTa		att;
	SdpTb		bandwidth;
	SdpTk		key;
	SdpTm		media;
	SdpSess		sdp,tmpsdp;

	/*char*		temp = (char*)malloc(10000);*/
	char	temp[10000];
	int		tlen = 10000;
	UINT32		len = tlen;
	int		ret = RC_OK;

	printf( "\n-------------------------------------------------------------\n"
		"<SDP> module test\n"
		"-------------------------------------------------------------\n"); 

	printf( "\nCASE sdp_1: test <sdp> module ... ");
	if( sdpToParse(TO_TST_STR, &sdpO)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpToParse() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpTo2Str(&sdpO,temp,&len)!=RC_OK || strcmp(temp,TO_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTo2Str() error.\n",
			__CMFILE__,__LINE__);
		
		printf("origianl:\n%s\n",TO_TST_STR);
		printf("after   :\n%s\n",temp);
		return RC_ERROR;
	}
	len = tlen;

	if( sdpTcParse(TC_TST_STR, &sdpConn)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTcParse() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpTc2Str(&sdpConn,temp,&len)!=RC_OK || strcmp(temp,TC_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTc2Str() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	len = tlen;

	if( sdpTrParse(TR_TST_STR, &repTime)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTrParse() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpTr2Str(&repTime,temp,&len)!=RC_OK || strcmp(temp,TR_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTr2Str() error.\n",
			__CMFILE__,__LINE__);
		printf("original:\n%s\n after:\n%s\n",TR_TST_STR,temp);
		return RC_ERROR;
	}
	len = tlen;

	if( sdpTaParse(TA_TST_STR, &att)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTaParse() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpTa2Str(&att,temp,&len)!=RC_OK || strcmp(temp,TA_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTa2Str() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	len = tlen;

	if( sdpTzParse(TZ_TST_STR, &timeAdj)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTzParse() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpTz2Str(&timeAdj,temp,&len)!=RC_OK || strcmp(temp,TZ_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTz2Str() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	len = tlen;
	
	if( sdpTtParse(TT_TST_STR, &time)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTtParse() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpTt2Str(&time,temp,&len)!=RC_OK || strcmp(temp,TT_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTt2Str() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	len = tlen;
	
	if( sdpTbParse(TB_TST_STR, &bandwidth)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTbParse() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpTb2Str(&bandwidth,temp,&len)!=RC_OK || strcmp(temp,TB_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTb2Str() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	len = tlen;

	if( sdpTkParse(TK_TST_STR, &key)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTkParse() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpTk2Str(&key,temp,&len)!=RC_OK || strcmp(temp,TK_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTk2Str() error.\n",
			__CMFILE__,__LINE__);
		printf("original:\n%s after:\n%s",TK_TST_STR,temp);
		return RC_ERROR;
	}
	len = tlen;

	if( sdpTmParse( TM_TST_STR, &media)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTmParse() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpTm2Str(&media,temp,&len)!=RC_OK || strcmp(temp,TM_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpTm2Str() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	len = tlen;

	if( !(sdp = sdpSessNewFromText(SDPSESS_TST_STR)) ) {
		printf( "\n!!ERROR!! %s:%d: sdpSessNewFromText() error.\n",
			__CMFILE__,__LINE__);
		return RC_ERROR;
	}
	if( sdpSess2Str(sdp,temp,&len)!=RC_OK || strcmp(temp,SDPSESS_TST_STR)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: sdpSess2Str() error.\n",
			__CMFILE__,__LINE__);
		printf("origianl sdp:\n%s\n",SDPSESS_TST_STR);
		printf("after sdp:\n%s\n",temp);
		return RC_ERROR;
	}
	len = tlen;
	if ((tmpsdp=sdpSessDup(sdp))!=NULL) {
		if( sdpSess2Str(tmpsdp,temp,&len)!=RC_OK || strcmp(temp,SDPSESS_TST_STR)!=0 ) {
			printf( "\n!!ERROR!! %s:%d: sdpSess2Str() error.\n",
				__CMFILE__,__LINE__);
			printf("origianl sdp:\n%s\n",SDPSESS_TST_STR);
			printf("after sdp:\n%s\n",temp);
			return RC_ERROR;
		}
		sdpSessFree(tmpsdp);
	}
	if( accSdpSess(sdp)!=RC_OK )
		return RC_ERROR;
	sdpSessFree(sdp);

	printf("!!SUCCESS!!\n");

	return ret;
}
