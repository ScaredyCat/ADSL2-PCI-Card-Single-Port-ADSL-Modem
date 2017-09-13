/* $Id: tdu_log.c,v 1.2 2001/03/01 14:59:12 paul Exp $ */
/*
  Copyright 1997 by Henner Eisen

    This code is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/ 
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <tdu_user.h>
#include "tdu.h"
#include "fileheader.h"

char * (*tdu_extended_reason)(int) = NULL;

/* Table with various information on tdu protocol data units */

static struct tdu_descr cmd_descr[] = {
{"T_RESPONSE_POSITIVE", TDU_CI_T_RESPONSE_POSITIVE,0},
{"T_RESPONSE_NEGATIVE", TDU_CI_T_RESPONSE_NEGATIVE,0},
{"T_ASSOCIATE",		TDU_CI_T_ASSOCIATE,0},
{"T_RELEASE",		TDU_CI_T_RELEASE,0},
{"T_ABORT",		TDU_CI_T_ABORT,0},
{"T_ACCESS",		TDU_CI_T_ACCESS,1},
{"T_END_ACCESS",	TDU_CI_T_END_ACCESS,1},
{"T_DIRECTORY",		TDU_CI_T_DIRECTORY,1},
{"T_LOAD",		TDU_CI_T_LOAD,1},
{"T_SAVE",		TDU_CI_T_SAVE,1},
{"T_RENAME",		TDU_CI_T_RENAME,1},
{"T_DELETE",		TDU_CI_T_DELETE,1},
{"T_TYPED_DATA",	TDU_CI_T_TYPED_DATA,1},
{"T_WRITE",		TDU_CI_T_WRITE,0},
{"T_TRANSFER_REJECT",	TDU_CI_T_TRANSFER_REJECT,0},
{"T_READ_RESTART",      TDU_CI_T_READ_RESTART,0},
{"T_P_EXCEPTION",	TDU_CI_T_P_EXCEPTION,1},
{"RESPONSE_TIMEOUT",	TDU_CI_RESPONSE_TIMEOUT,2},
{"SOCKET_READ_ERROR",	TDU_CI_SOCK_READ_ERROR,2},
{"TC disconnected",	TDU_CI_TC_DISCONNECT,2},
{"Empty TDU",		TDU_CI_EMPTY,2},
{NULL, 			0,0}
};

/* Table with various information on result/reason codes.
   According to ETS 300 075, 7.2.1 and 7.2.2*/

static struct tdu_descr tdu_res_descr[] = {
{"provider: called address incorrect",	TDU_RE_CALLED_ADDR_INCOR_P,0},
{"user: called address incorrect",	TDU_RE_CALLED_ADDR_INCOR_U,0},
{"provider: calling address incorrect", TDU_RE_CALLING_ADDR_INCOR_P,0},
{"user: calling address incorrect",	TDU_RE_CALLING_ADDR_INCOR_U,0},
{"provider: role refused",		TDU_RE_ROLE_REFUSED_P,0},
{"user: role refused",			TDU_RE_ROLE_REFUSED_U,0},
{"insufficient primitives handled",	TDU_RE_INSUFF_PRIMITIVES,0},
{"application name unknown",		TDU_RE_APPL_NAME_UNKNOWN,0},
{"provider: service class refused",	TDU_RE_SVC_CLASS_REFUSED_P,0},
{"user: service class refused",		TDU_RE_SVC_CLASS_REFUSED_U,0},
{"erroneous revovery point",		TDU_RE_ERRON_RECOV_POINT,0},
{"erroneous designation",		TDU_RE_ERRON_DESIGN,0},
{"no answer to the request",		TDU_RE_NO_ANSWER,0},
{"unknown file",			TDU_RE_UNKNOWN_FILE,0},
{"already existing file",		TDU_RE_FILE_EXISTS,0},
{"erroneous file",			TDU_RE_ERRON_FILE,0},
{"erroneous new name",			TDU_RE_ERRON_NEW_NAME,0},
{"new name already in use",		TDU_RE_NEW_NAME_IN_USE,0},
{"wrong identity/identification",	TDU_RE_WRONG_ID,0},
{"erroneous user data",			TDU_RE_ERRON_UDATA,0},
{"service unknown",			TDU_RE_SVC_UNKNOWN,0},
{"group forbidden",			TDU_RE_GROUP_FORBIDDEN,0},
{"other reason",			TDU_RE_OTHER_REASON,0},
{"repeated errors/negative acknowledges",TDU_RE_REPEATED_ERROR,0},
{"delay expired",			TDU_RE_DELAY_EXPIRED,0},
{"unknown message",			TDU_RE_UNKNOWN_MESSAGE,0},
{"syntax error/missing parameter",	TDU_RE_SYNTAX_ERROR,0},
{"unrecoverable lower layer error",	TDU_RE_LOWER_LAYER_ERROR,0},
{"protocol conflict",			TDU_RE_PROTOCOL_CONFLICT,0},
{"primitive not handled",		TDU_RE_PRIMITIVE_NOT_HANDLED,0},
{NULL, 			0,0}
};

/* Table with various information on tdu parameters */

static struct tdu_descr tdu_par_descr[] = {
{"USER_DATA",			TDU_PI_USER_DATA,0},
{"CALLED_ADDRESS",		TDU_PI_CALLED_ADDRESS,1},
{"CALLING_ADDRESS",		TDU_PI_CALLING_ADDRESS,1},
{"Result/Reason",		TDU_PI_RESULT_REASON,1},
{"Role/Function",		TDU_PI_ROLE_FUNCTION,0},
{"APPLICATION_NAME",		TDU_PI_APPLICATION_NAME,0},
{"APPL_RESPONSE_TIMOUT",	TDU_PI_APPL_RESPONSE_TIMEOUT,0},
{"Size/Recovery/Window",	TDU_PI_SIZE_RECOVERY_WINDOW,1},
{"DESIGNATION",			TDU_PI_DESIGNATION,1},
{"NEW_NAME",			TDU_PI_NEW_NAME,1},
{"REQUEST_IDENT",		TDU_PI_REQUEST_IDENT,1},
{"Identification",		TDU_PI_IDENTIFICATION,1},
{"ExplCnf/1st/last/bl#",	TDU_PI_EX_CONF_1ST_LAST_BLNO,0},
{"TRANSFER_MODE",		TDU_PI_TRANSFER_MODE,1},
{"RECOVERY_POINT",		TDU_PI_RECOVERY_POINT,1},
{"SERVICE_CLASS",		TDU_PI_SERVICE_CLASS,0},
/* not a parameter but compatible */ 
{"File_Header",			TDU_PI_FILE_HEADER,0},
{NULL,				0,0}
};


const char * tdu_cmd_descr(int ci)
{
	static const char unknown[] = "Unknown_TDU_Command";
	char * des = tdu_des(cmd_descr,ci);
	return des ? des : unknown;
}


const char * tdu_param_descr(int pi)
{
	static const char unknown[] = "Unknown_TDU_Parameter";
	char* des = tdu_des(tdu_par_descr,pi);
	return des ? des : unknown;
}

FILE * tdu_logfile;
unsigned int tdu_stderr_mask = 0,
	tdu_logfile_mask = 0,
	tdu_stderr_dirty = 0,
	nl_err=1,
	nl_log=1;
pid_t	log_pid=0;
char 	log_prefix[20]="e4l[%d]:%s: ", 
	time_format[20]="%Y-%m-%d %H:%M:%S";


int tdu_open_log(const char * fname)
{
	tdu_logfile = fopen(fname,"a");
	if( ! tdu_logfile ) {
		perror("fopen log file");
		tdu_logfile_mask = 0;
	} else {
		setvbuf(tdu_logfile,NULL,_IOLBF,4098);
	}
	return tdu_logfile != NULL;
}

/*
 * set/change log prefix and date format 
 */
int tdu_log_prefix(const char * id_fmt, const char *tm_fmt)
{
	if( id_fmt) strncpy(log_prefix, id_fmt, sizeof(log_prefix));
	if( tm_fmt)  strncpy(time_format, tm_fmt, sizeof(time_format));
	log_pid = getpid();
	return 0;
}


int tdu_printf( int context, char * fmt, ... )
{
	int ret, nl, i;
	va_list ap;
	char tst[20];

	/*
	 * Check for trailing newline might fail if newline is part of last
	 * argument. It is also not bullet proof if several processes
	 * are writing to the same output channel in parallel. Setting
	 * line buffered io mode should help for most scenarios.
	 */ 
	nl = ( fmt[strlen(fmt)-1] == '\n' );
	if( nl_err || nl_log ){ 
		struct tm * tm_time;
		time_t t=0;
		time(&t);
		tm_time = localtime(&t);
		i = strftime(tst, sizeof(tst), time_format, tm_time);
	}

	va_start(ap, fmt);
	if( context & tdu_stderr_mask ){
		if( nl_err ) fprintf(stderr, log_prefix, log_pid, tst); 
		ret = vfprintf(stderr, fmt, ap);
		tdu_stderr_dirty = 1;
		nl_err = nl;
	}
	if( ret < 0 ) return ret;
	
	va_start(ap, fmt);
	if( context & tdu_logfile_mask ){
		if( nl_log ) fprintf(tdu_logfile, log_prefix, log_pid, tst); 
		ret = vfprintf(tdu_logfile, fmt, ap);
		nl_log = nl;
	}
	va_end(ap);
	return ret;
}


void tdu_prompt(char * prompt)
{
	fprintf(stderr,prompt);
	tdu_stderr_dirty = 0;
}      


unsigned char * tdu_print_cmd( int ct, unsigned char *pkt, unsigned char *end,
			       int level )
{
	unsigned char ci, ci2;
	int len=0, garbage=0;

	if( pkt >= end ) goto packet_too_small;

	if( level < 1 ) { 
		tdu_printf(ct,"\n");
		return pkt;
	}
	ci = *(pkt++);
	tdu_printf(ct, " %s(", tdu_cmd_descr(ci));

	if( pkt < end ) len = *(pkt++);
	if( len == 0xff ) {
		if( pkt+2 >= end ) goto  packet_too_small;
		len = 0x100 * *(pkt)  +  *(pkt+1);
		pkt += 2;
	}

	if( pkt+len < end ){
		garbage = end - pkt - len; 
		/* remove trailing garbage */
		end = pkt + len;
	}
	if( ci == TDU_CI_T_RESPONSE_POSITIVE ||
	    ci == TDU_CI_T_RESPONSE_NEGATIVE ){
		if( pkt+2 > end ) goto  packet_too_small;
		ci2 = *(pkt+2);
		tdu_printf(ct, "%s,", tdu_cmd_descr(ci2));
	}
	tdu_printf(ct, "len=%d", len);
	if ( garbage ) tdu_printf(ct,"[+%d]", garbage);
	tdu_printf(ct, ")" );

	if( level < 2 ) { 
		tdu_printf(ct,"\n");
		return pkt;
	}
	tdu_printf(ct, ":  " );

	/* Never log t-write contents */
	if( ci == TDU_CI_T_WRITE ) {
		pkt = tdu_print_par( ct, pkt, end, ci, level );
		tdu_printf(ct,"\n");
		return end;
	}

	while( pkt < end ) pkt = tdu_print_par( ct, pkt, end, ci, level );
	tdu_printf(ct,"\n");
	return pkt;
	
	packet_too_small : tdu_printf(ct, "CMD packet to small!\n" );
	return end;
}

void tdu_print_hex( int ct, unsigned char * pkt, unsigned char * end)
{
	tdu_printf(ct,"0x");
	while ( pkt < end ) 
		tdu_printf(ct, "%02x ", (int) *(pkt++) );
}


void tdu_print_txt( int ct, unsigned char * pkt, unsigned char * end)
{
	unsigned char tmp;

	while ( pkt < end ){
		tmp = *pkt & 0x7f;
		if( tmp < 32 ){
			tdu_printf(ct, "^%c", (int) ( *pkt + 64 ) );
		} else if ( *pkt == 0x7f ) {
			tdu_printf(ct, "^?" );
		} else {
			tdu_printf(ct, "%c", (int) *pkt );
		}
		pkt++;
	};
}


void tdu_print_li( int ct, unsigned char * pkt, unsigned char * end)
{
	int i = 0;
	while ( pkt < end ) {
		i = i * 256;
		i += *(pkt++);
	}
	tdu_printf(ct, "%d", i );
}


static void tdu_print_srw( int ct, unsigned char * pkt, unsigned char * end)
{
	int log2, size=512;

	if ( pkt >= end ) return;

	/* packet size */
	log2 = (0x0e & *pkt) >> 1;
	while( log2-- ) size <<= 1;
	tdu_printf(ct,"%d/", size);

	/* recovery */
	tdu_printf(ct,"%sAccepted/", (*pkt & 0x01) ? "" : "Not");

	/* window size */
	tdu_printf(ct,"%d", ((*pkt & 0xe0) >> 5) + 1 );
}


static void tdu_print_mode( int ct, unsigned char * pkt, unsigned char * end)
{
	if ( pkt >= end ) return;
	tdu_printf(ct,"%sBasicMode", (*pkt & 0x01) ? "" : "No");
}


static void tdu_print_role( int ct, unsigned char * pkt, unsigned char * end)
{
	if ( pkt >= end ) return;
	if( *pkt & 0x01 ) {
		tdu_printf(ct, "Master(" );
		if (*pkt & 0x02) tdu_printf(ct," ReadRestart");
		if (*pkt & 0x04) tdu_printf(ct," TypedData");
	} else {
		tdu_printf(ct, "Slave(" );
		if (*pkt & 0x02) tdu_printf(ct," ReadRestart");
		if (*pkt & 0x04) tdu_printf(ct," TypedData");
		if (*pkt & 0x08) tdu_printf(ct," Directory");
		if (*pkt & 0x10) tdu_printf(ct," Delete");
		if (*pkt & 0x20) tdu_printf(ct," Rename");
		if (*pkt & 0x40) tdu_printf(ct," Save");
		if (*pkt & 0x80) tdu_printf(ct," Load");
	}
	tdu_printf(ct," )");
}

/* FIXME: should not go into this source file */
#include "eft.h"

static void tdu_print_reason( int ct, unsigned char * pkt, unsigned char * end, int ci)
{
	int rci;

	if( pkt < end && ( ci == TDU_CI_T_RESPONSE_NEGATIVE ||
			   ci == TDU_CI_T_RESPONSE_POSITIVE )  ) {
		rci = *(pkt++);
		tdu_printf(ct, "%s ", tdu_cmd_descr( rci )  );
		if( rci == TDU_CI_T_WRITE ){
			int count = 0;
			if( pkt < end ) count += *(pkt++); 
			if( pkt < end ) { count *= 256; count += *(pkt++);}
			tdu_printf(ct, "packet No %d",count);
		}
	}	
	while ( pkt < end ){
		tdu_printf(ct, "%s ", tdu_str_reason(*pkt) );
		if( *pkt == TDU_RE_ERRON_UDATA && (pkt+1 < end)){
			tdu_print_reason( ct, pkt+1, end, ci);
			return;
		}
		if( *pkt == TDU_RE_OTHER_REASON ){
			char * ext_re = NULL;

			pkt++;
			/* FIXME: should not belong here */
			if( (pkt+1) == end && tdu_extended_reason) 
				ext_re = tdu_extended_reason(*pkt);
			while ( pkt < end ) 
				tdu_printf(ct, "%c", (int) *(pkt++) );
			tdu_printf(ct," ");
			if( ext_re ) tdu_printf(ct, "[%s] ",ext_re);
		};
		pkt++;
	};
}

unsigned char * tdu_print_par( int ct, unsigned char *pkt, unsigned char *end, int ci, int level )
{
	unsigned char pi, *ret;
	int len=0;

	if( pkt >= end ) goto packet_too_small;
	
	pi = *(pkt++);
	tdu_printf(ct, " %s(", tdu_param_descr(pi));

	if( pkt >= end )  goto  packet_too_small; 
	if( (len = *(pkt++)) == 0xff ) {
		if( pkt+2 > end )  goto packet_too_small; 
		len = 0x100 * *(pkt++)  +  *(pkt++);
	}
	tdu_printf(ct,"len=%d)", len);
	ret = pkt + len;
	if( level < 3 ) {
		return ret;
	}

	tdu_printf(ct,"=");
	switch ( pi ) {
	case TDU_PI_RESULT_REASON:
		tdu_print_reason(ct,pkt,ret,ci);
		break;
	case TDU_PI_SIZE_RECOVERY_WINDOW:
		tdu_print_srw(ct,pkt,ret);
		break;
	case TDU_PI_ROLE_FUNCTION:
		tdu_print_role(ct,pkt,ret);
		break;
	case TDU_PI_TRANSFER_MODE:
		tdu_print_mode(ct,pkt,ret);
		break;
	case TDU_PI_APPLICATION_NAME:
	case TDU_PI_CALLED_ADDRESS:
	case TDU_PI_CALLING_ADDRESS:
	case TDU_PI_DESIGNATION:
	case TDU_PI_NEW_NAME:
	case TDU_PI_IDENTIFICATION:
		tdu_print_txt(ct,pkt,ret);
		break;
	default:
		tdu_print_hex(ct,pkt,ret );
	};
	return ret;
	
	packet_too_small : tdu_printf(ct, "PAR packet to small!\n" );
	return end;
}

const char * tdu_str_reason(int reason)
{
	static const char unknown[] = "Unknown_TDU_ReasonCode";
	char * des = tdu_des(tdu_res_descr,reason);
	return des ? des : unknown;
}

/*
 * Replace unprintable characters from string type tdu parameters.
 * Thus should be applied before printing those as strings in order
 * to prevent them from screwing up your terminal state.
 *
 * The data array must be at least dimensioned max(len+1,1), which is essential
 * to be verified before calling in order to prevent buffer overflow attacks.
 * The last data[len] (or data[0] if len<0) will be terminated by zero on exit.
 */
int tdu_mk_printable(unsigned char * pr_data, const unsigned char * in_data,
		     int len){
	int i;

	if( len < 0 ) {
		pr_data[0]=0;
		return 0;
	}
	pr_data[len]=0;
	
	memcpy(pr_data, in_data, len);
	for(i=0; i < len; i++){
		if( ((0x7f & pr_data[i]) < 32) || (pr_data[i] == 0x7f) )
			pr_data[i]='?';
		
	}
	return 0;
}

