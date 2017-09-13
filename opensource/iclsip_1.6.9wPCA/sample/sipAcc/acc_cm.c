/* 
 * Copyright (C) 2000-2001 Computer & Communications Research Laboratories,
 *			   Industrial Technology Research Institute
 */
/*
 * acc_cm.c
 *
 * $Id: acc_cm.c,v 1.4 2005/01/20 09:56:20 tyhuang Exp $
 */
#include <stdio.h>

#include <common/cm_def.h>
#include <common/cm_utl.h>
#include <low/cx_thrd.h>
#include <low/cx_sock.h>
#include <low/cx_misc.h>
#include <common/cm_trace.h>

#define TCR_TEST_MSG	"*** This is test message! ***"
#define TCR_TEST_FILE	"tstFileLogger.txt"
#define TCR_TEST_PORT	19944

int	tstMsgCBOK = 0;

void tstCB(const char* msg)
{
	if( !strCon(msg,TCR_TEST_MSG) ) {
		tstMsgCBOK = 0;
		printf( "%s:%d: Log message is not sent to callback function.\n",
			__CMFILE__,__LINE__);
		return;
	}
	else
		tstMsgCBOK++;
}

RCODE	tstConsoleLogger(void)
{
	if( TCRConsoleLoggerON()!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRConsoleLoggerON() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( TCRPrint(2,"%s\n",TCR_TEST_MSG)<0 ) {
		printf("\n!!ERROR!! %s:%d: TCRPrint() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( TCRConsoleLoggerOFF()!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRConsoleLoggerOFF() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}

	return RC_OK;
ERR:
	return RC_ERROR;
}

RCODE	tstFileLogger(void)
{
	FILE	*f;
	char	input[512], *p;

	if( TCRFileLoggerON(TCR_TEST_FILE)!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRFileLoggerON() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( TCRPrint(2,"%s\n",TCR_TEST_MSG)<0 ) {
		printf("\n!!ERROR!! %s:%d: TCRPrint() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( TCRFileLoggerOFF()!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRFileLoggerOFF() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	f = fopen(TCR_TEST_FILE, "r");
	if( (p=fgets(input,512,f)) == NULL ) {
		printf("\n!!ERROR!! %s:%d: Log message is not writen to file.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( !strCon(input,TCR_TEST_MSG) ) {
		printf("\n!!ERROR!! %s:%d: Log message is not writen to file.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	fclose(f);

	return RC_OK;
ERR:
	return RC_ERROR;
}

RCODE	tstMsgCB(void)
{
	if( TCRSetMsgCB(tstCB)!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRSetMsgCB() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( TCRPrint(2,"%s\n",TCR_TEST_MSG)<0 ) {
		printf("\n!!ERROR!! %s:%d: TCRPrint() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( TCRSetMsgCB(NULL)!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRSetMsgCB() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( tstMsgCBOK!=1 ) {
		printf("\n!!ERROR!! %s:%d: log message not delivered.\n",__CMFILE__,__LINE__);
		goto ERR;
	}

	return RC_OK;
ERR:
	return RC_ERROR;
}

RCODE	tstSockLogger(void)
{
	tstMsgCBOK = 0;
	TCRSetMsgCB(tstCB);
	if( TCRSockServerON(TCR_TEST_PORT)!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRSockServerON() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( TCRSockLoggerON("127.0.0.1",TCR_TEST_PORT)!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRSockLoggerON() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( TCRPrint(2,"%s\n",TCR_TEST_MSG)<0 ) {
		printf("\n!!ERROR!! %s:%d: TCRPrint() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}

	sleeping(500);
	if( tstMsgCBOK!=2 ) {
		printf("\n!!ERROR!! %s:%d: log message not delivered.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if( TCRSockLoggerOFF()!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRSockLoggerOFF() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	if ( TCRSockServerOFF()!=RC_OK ) {
		printf("\n!!ERROR!! %s:%d: TCRSockServerOFF() error.\n",__CMFILE__,__LINE__);
		goto ERR;
	}
	TCRSetMsgCB(NULL);

	return RC_OK;
ERR:
	return RC_ERROR;
}

RCODE	accTrace(void)
{
	RCODE	ret = RC_OK;

	cxSockInit();
	TCRBegin();
	TCRSetTraceLevel(2);
	if( tstConsoleLogger()!=RC_OK ||
	    tstFileLogger()!=RC_OK ||
	    tstMsgCB()!=RC_OK ||
	    tstSockLogger()!=RC_OK ) {
		ret = RC_ERROR;
	}
	TCREnd();
	cxSockClean();

	return ret;
}

#define	TSTSTR		" \t\n\r Hello World\tTrimWS\t\r\n "
#define	TSTSTR_UPPER	" \t\n\r HELLO WORLD\tTRIMWS\t\r\n "
#define TSTSUBSTR_UPPER	"HELLO WORLD"
#define TSTSUBSTR_MIX	"Hello World"
#define	TRIMWS_RESULT	"Hello World\tTrimWS"
#define QUOTE_RESULT	"\" \t\n\r Hello World\tTrimWS\t\r\n \""
#define UNQUOTE_RESULT	" \t\n\r Hello World\tTrimWS\t\r\n "
#define ATTACH_RESULT	" \t\n\r Hello World\tTrimWS\t\r\n HELLO WORLD"

RCODE	accUtl(void)
{	
	RCODE	ret = RC_OK;
	char	sbuf[128];
	char	*tmp;

	strcpy(sbuf,TSTSTR);
	trimWS(sbuf);
	if( strcmp(sbuf,TRIMWS_RESULT)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: trimWS() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	
	strcpy(sbuf,TSTSTR);
	quote(sbuf);
	if( strcmp(sbuf,QUOTE_RESULT)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: quote() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	unquote(sbuf);
	if( strcmp(sbuf,UNQUOTE_RESULT)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: unquote() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	if( strCon(sbuf,TSTSUBSTR_MIX)!=TRUE || 
	    strCon(sbuf,TSTSUBSTR_UPPER)!=FALSE ) {
		printf( "\n!!ERROR!! %s:%d: strCon() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	if( strICon(sbuf,TSTSUBSTR_MIX)!=TRUE || 
	    strICon(sbuf,TSTSUBSTR_UPPER)!=TRUE ) {
		printf( "\n!!ERROR!! %s:%d: strICon() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	if( strICmp(sbuf,TSTSTR_UPPER)!=0 || 
	    strICmp(sbuf,TSTSUBSTR_UPPER)==0 ) {
		printf( "\n!!ERROR!! %s:%d: strICmp() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	if( strICmpN(sbuf,TSTSTR_UPPER,10)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: strICmpN() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	tmp = strAtt(sbuf,TSTSUBSTR_UPPER);
	if( strcmp(tmp,ATTACH_RESULT)!=0 ) {
		printf( "\n!!ERROR!! %s:%d: strAtt() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	free(tmp);
	strCpySpn(sbuf,TSTSUBSTR_UPPER,"HELO ",128);
	if( strcmp(sbuf,"HELLO ")!=0 ) {
		printf( "\n!!ERROR!! %s:%d: strCpySpn() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}
	strCpyCSpn(sbuf,TSTSUBSTR_UPPER,"W ",128);
	if( strcmp(sbuf,"HELLO")!=0 ) {
		printf( "\n!!ERROR!! %s:%d: strCpyCSpn() error.\n",
			__CMFILE__,__LINE__);
		ret = RC_ERROR;
	}

	return ret;
}

RCODE	accCommon(void)
{
	RCODE	ret = RC_OK;
	printf( "\n-------------------------------------------------------------\n"
		"<Common> module test\n"
		"-------------------------------------------------------------\n"); 
	printf("\nCASE cm_1: test <cm_utl> module ... "); 
	if( accUtl()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

	printf( "\nCASE cm_2: test <cm_trace> module ... \n"); 
	if( accTrace()==RC_OK )
		printf("!!SUCCESS!!\n");
	else
		ret = RC_ERROR;

	return ret;
}
