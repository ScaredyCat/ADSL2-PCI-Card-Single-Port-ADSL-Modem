/* $Id: fileheader.c,v 1.4 2001/03/01 14:59:12 paul Exp $ */
/*
  Copyright 1998 by Henner Eisen

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

/*
  A (rather incomplete) implementation of the T-protocol
  defined in ETS 300 075. 

  This file containing stuff used for processing of file headers.
*/ 

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <pwd.h>
#include <string.h>


#include <tdu_user.h>
#include "tdu.h"
#include "fileheader.h"

/* Table with various information on tdu file headers */

static struct tdu_descr tdu_fh_descr[] = {
{"FILE_TYPE",		TDU_FH_FILE_TYPE, 0},
{"EXECUTION_ORDER",	TDU_FH_EXECUTION_ORDER, 0},
{"TRANSFER_NAME",	TDU_FH_TRANSFER_NAME, 0},
{"FILE_NAME",		TDU_FH_FILE_NAME, 0},
{"DATE",		TDU_FH_DATE, 0},
{"FILE_LENGTH",		TDU_FH_FILE_LENGTH, 0},
{"DESTINATION_CODE",	TDU_FH_DESTINATION_CODE, 0},
{"FILE_CODING",		TDU_FH_FILE_CODING, 0},
{"DESTINATION_NAME",	TDU_FH_DESTINATION_NAME, 0},
{"COST", 		TDU_FH_COST, 0},
{"USER_FIELD",		TDU_FH_USER_FIELD, 0},
{"LOAD_ADDRESS",	TDU_FH_LOAD_ADDRESS, 0},
{"EXEC_ADDRESS_ABS",	TDU_FH_EXEC_ADDRESS_ABS, 0},
{"EXEC_ADDRESS_REL",	TDU_FH_EXEC_ADDRESS_REL, 0},
{"COMPRESSION_MODE",	TDU_FH_COMPRESSION_MODE, 0},
{"DEVICE", 		TDU_FH_DEVICE, 0},
{"FILE_CHECKSUM",	TDU_FH_FILE_CHECKSUM, 0},
{"AUTHOR_NAME",		TDU_FH_AUTHOR_NAME, 0},
{"FUTURE_FILE_LENGTH",	TDU_FH_FUTURE_FILE_LENGTH, 0},
{"PERMITTED_ACTIONS",	TDU_FH_PERMITTED_ACTIONS, 0},
{"LEGAL_QUALIFICATION",	TDU_FH_LEGAL_QUALIFICATION, 0},
{"CREATION",		TDU_FH_CREATION, 0},
{"LAST_READ_ACCESS",	TDU_FH_LAST_READ_ACCESS, 0},
{"ID_OF_LAST_MODIFIER",	TDU_FH_ID_OF_LAST_MODIFIER, 0},
{"ID_OF_LAST_READER",	TDU_FH_ID_OF_LAST_READER, 0},
{"RECIPIENT",		TDU_FH_RECIPIENT, 0},
{"TELEMATIC_FT_VERSION",TDU_FH_TELEMATIC_FT_VERSION, 0},
{0,0,0}
};


static char * tdu_print_file_par( int ct, unsigned char *pkt, unsigned char *end,
				       int level )
{
	unsigned char pi, *ret;
	char *name;
	int len=0;

	if( pkt >= end ) goto packet_too_small;
	
	pi = *(pkt++);
	name = tdu_des(tdu_fh_descr,pi);
	tdu_printf( ct, " %s(", name ? name : "FIle header parameter unknown " ); 

	if( pkt >= end )  goto  packet_too_small; 
	if( (len = *(pkt++)) == 0xff ) {
		if( pkt+2 > end )  goto packet_too_small; 
		len = 0x100 * *(pkt++)  +  *(pkt++);
	}
	tdu_printf (ct,"len=%d)", len);
	ret = pkt + len;
	if( level < 3 ) {
		return ret;
	}

	tdu_printf(ct,"=");
	switch ( pi ) {
	case TDU_FH_TRANSFER_NAME:
	case TDU_FH_FILE_NAME:
	case TDU_FH_DATE:
	case TDU_FH_DESTINATION_NAME:
	case TDU_FH_COST:
	case TDU_FH_USER_FIELD:
	case TDU_FH_AUTHOR_NAME:
	case TDU_FH_LEGAL_QUALIFICATION:
	case TDU_FH_CREATION:
	case TDU_FH_LAST_READ_ACCESS:
	case TDU_FH_ID_OF_LAST_MODIFIER:
	case TDU_FH_ID_OF_LAST_READER:
	case TDU_FH_RECIPIENT:
	case TDU_FH_TELEMATIC_FT_VERSION:
		tdu_print_txt(ct,pkt,ret);
		break;
	case TDU_FH_FILE_LENGTH:
	case TDU_FH_FUTURE_FILE_LENGTH:
	case TDU_FH_FILE_CHECKSUM:
		tdu_print_li( ct, pkt, ret );
		break;
	default:
		tdu_print_hex ( ct, pkt,ret );
	};
	return ret;
	
	packet_too_small : tdu_printf( TDU_LOG_ERR, "FH packet to small!\n" );
	return end;
}


unsigned char * tdu_print_file_header( int ct, unsigned char *pkt, unsigned char *end, int level )
{
	unsigned char ci;
	int len=0, garbage=0;

	if( pkt >= end ) goto packet_too_small;

	if( level < 1 ) { 
		tdu_printf(ct,"\n");
		return pkt;
	}
	ci = *(pkt++);
	
	tdu_printf( ct, " %s(", ci == 0x30 ? "File_Header" : "FH magic unknown" ); 

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
	tdu_printf (ct, "len=%d", len);
	if ( garbage ) tdu_printf(ct, "[+%d]", garbage);
	tdu_printf( ct, ")" );

	if( level < 2 ) { 
		tdu_printf(ct,"\n");
		return pkt;
	}
	tdu_printf( ct, ":  " );

	while( pkt < end ) pkt = tdu_print_file_par( ct, pkt, end, level );
	tdu_printf(ct,"\n");
	return pkt;
	
	packet_too_small : tdu_printf( TDU_LOG_ERR,"FH packet to small!\n" );
	return end;
}

/*
 * Header parser that just logs the header contents
 *
 * FIXME: integrate the logging functinality into the standard parser
 * and remove this when the latter is done and working.
 */
int tdu_fh_parse_print(struct tdu_stream *ts, struct tdu_buf * tb, int maxlen)
{
	
	unsigned char *fh_tail, *max_tail = tb->data + maxlen, *start=tb->data;
	unsigned char ci = *(tb->data++);
	int len;

	tdu_printf( TDU_LOG_FH, " %s",  ci == TDU_PI_FILE_HEADER ? 
		    "File_Header" : "FH magic unknown" ); 
	if( (len = tdu_parse_le(tb)) < 0 ) goto oflow;
	tdu_printf( TDU_LOG_FH, "[%d](", len );

	/* FIXME: parse headers extending more than one buffer */
	fh_tail = tb->data + len;
	if( fh_tail > max_tail ) goto oflow;

	while( tb->data < fh_tail ) 
		tb->data = tdu_print_file_par(TDU_LOG_FH, tb->data, fh_tail,4);
	tdu_printf(TDU_LOG_FH,")\n");
	ts->hdr.len = fh_tail - start;
	ts->hdr.parse = NULL;
	return (fh_tail - start);

oflow:
	ts->hdr.len = 0;
	ts->hdr.parse = NULL;
	tdu_printf( TDU_LOG_ERR, 
		    "Buffer overflow while parsing file header\n");
	return -TDU_RE_SYNTAX_ERROR;
}

/*
 * Parse an ETS 300-075 file header encoded time string and return
 * the equivalent unix epoche time_t type;
 *
 */
static time_t fh2timet(unsigned char *ts, int len){
	int tmp;
	struct tm t={0,0,0,0,0,0,0,0,0,0};
	/*
	 * FIXME: (or better fix ETS protocol :-) this is not y2k safe due to
	 * limititations in ETSI protocol.
	 *
	 * The window boundary is chosen arbitrarily because I
	 * currently don't know of any official ETSI recommendation.
	 *
	 * This should be changed as soon as an official value
	 * agreed among all EUROFILE implementors is recommended.
	 */
	int y2k_wrap=70;

	if(len>1){
		tmp = 10*(ts[0]-'0') + (ts[1]-'0');
		ts +=2;
		len-=2;
		/* y2k compatibilty hack */
		if( tmp < y2k_wrap ) tmp += 100;
		t.tm_year = tmp;
	}
	
	if(len>1){
		tmp = 10*(ts[0]-'0') + (ts[1]-'0');
		ts +=2;
		len-=2;
		t.tm_mon = tmp-1;  /* tm_mon is in the range 0 to 11 */
	}
	
	if(len>1){
		tmp = 10*(ts[0]-'0') + (ts[1]-'0');
		ts +=2;
		len-=2;
		t.tm_mday = tmp;
	}
	
	if(len>1){
		tmp = 10*(ts[0]-'0') + (ts[1]-'0');
		ts +=2;
		len-=2;
		t.tm_hour = tmp;
	}

	if(len>1){
		tmp = 10*(ts[0]-'0') + (ts[1]-'0');
		ts +=2;
		len-=2;
		t.tm_min = tmp;
	}

	if(len>1){
		tmp = 10*(ts[0]-'0') + (ts[1]-'0');
		ts +=2;
		len-=2;
		t.tm_sec = tmp;
	}
	return mktime(&t);
}

/*
 * Header parser that stores parameters in a struct fileheader_par where
 * appropriate
 */
int tdu_parse_fh(struct fileheader_par * par, struct tdu_buf * tb)
{
	int ci, pi, len;
	char t[TDU_FH_PLEN_DATE+1];
	unsigned char * start = tb->data, *fh_tail;

	tdu_printf(TDU_LOG_TRC, "tdu_parse_fh()\n");
	ci = *(tb->data++);
	tdu_printf( TDU_LOG_DBG, " %s",  ci == TDU_PI_FILE_HEADER ? 
		    "File_Header" : "FH magic unknown" ); 
	len = tdu_parse_le(tb);
	/* FIXME: parse headers extending more than one buffer */
	fh_tail = tb->data + len;
	if(fh_tail > tb->tail) {
		tdu_printf( TDU_LOG_ERR, "tdu_parse_fh: tb overflow\n");
		return -TDU_RE_SYNTAX_ERROR;
	}
	tdu_printf( TDU_LOG_DBG|TDU_LOG_FH, "[%d](", len );
	tb->pn = tb->data;

	while( tb->data < fh_tail ) {
		unsigned char * this = tb->data;
		pi = tdu_get_next_pi(tb);
		tdu_printf(TDU_LOG_DBG, "parsing parameter %d\n",pi);
		if( pi < 0 ) return -pi;
		switch( pi ){
		case TDU_FH_FILE_LENGTH:
			tdu_printf(TDU_LOG_DBG, "length\n");
			len = tdu_parse_li(tb);
			par->fh_s.st_size = len;
			break;
		case TDU_FH_TRANSFER_NAME:
			tdu_printf(TDU_LOG_DBG, "transfer name\n");
			len = tdu_parse_string(tb, par->fh_t_name,
					       TDU_PLEN_DESIGNATION);
			par->fh_t_name[len]=0;
			break;
		case TDU_FH_FILE_NAME:
			tdu_printf(TDU_LOG_DBG, "file name\n");
			len = tdu_parse_string(tb, par->fh_name,
					       TDU_FH_PLEN_NAME);
			par->fh_name[len]=0;
			break;
		case TDU_FH_DATE:
			tdu_printf(TDU_LOG_DBG, "date \n");
			len = tdu_parse_string(tb, t, TDU_FH_PLEN_DATE);
			t[len]=0;
			par->fh_s.st_mtime = fh2timet(t,len);
			break;
		case TDU_FH_FILE_TYPE:
			tdu_printf(TDU_LOG_DBG, "type \n");
			par->fh_type = tdu_parse_byte(tb);
			break;
		default:
			/* FIXME: parse more parameters */
			tdu_printf(TDU_LOG_DBG, "tdu_parse_fh(): skipping unexpected"
				   " parameter '%s' ignored\n",
				   tdu_param_descr(pi) );
			tb->data = tb->pn;
		};
		tdu_print_file_par(TDU_LOG_FH, this, fh_tail,4);
	}
	tdu_printf( TDU_LOG_DBG|TDU_LOG_FH, ")");
	return (tb->data - start);
}

/*
 * Header usable by a tdu_stream object as parse method to parse
 * a fileheader into file_header_par object. The tdu_streams hdr.data
 * must point to an instance of a struct file_header_par.
 */
int tdu_fh_parse(struct tdu_stream *ts, struct tdu_buf * tb, int maxlen)
{
	/* FIXME: parse headers extending several tb's */
	unsigned char *fh_tail, *start=tb->data;
	struct fileheader_par * par = ts->hdr.data;
	int len;

	len = tdu_parse_fh(par, tb);
	fh_tail = start + len;
	if( len < 0 ) {
		/* probably buffer overflow or other parse error */
		ts->hdr.len = 0;
		ts->hdr.parse = NULL;
		tdu_printf( TDU_LOG_ERR, 
		    "tdu_fh_parse: error when parsing file header\n");
		return len;
	}

	ts->hdr.len = fh_tail - start;
	ts->hdr.parse = NULL;

	return fh_tail - start;
}

/*
 * used by tdu_stream object as parse method to prepend an empty file
 * header to a file 
 */
int tdu_fh_get_zero_hdr(struct tdu_stream *ts, unsigned char * buf, int maxlen)
{
	*(buf++) = TDU_PI_FILE_HEADER;
	*(buf++) = 0;
	ts->hdr.len = 2;
	ts->hdr.read = NULL;
	return 2;
}

/*
 * Make a fileheader from a struct fileheader_par and store it in a tdu_buf.
 */
int tdu_fh_stat(struct tdu_buf * tb, struct fileheader_par * fh)
{
	unsigned char t[13], t_fmt[]="%y%m%d%H%M%S", *owner,
		permissions[2];
	struct tm * tm_time;
        struct passwd * pwent;
	int t_len, coding, type;

	tm_time = gmtime(&fh->fh_s.st_atime);
	t_len = strftime( t, 13, t_fmt, tm_time);
	tdu_add_string_par(tb, TDU_FH_LAST_READ_ACCESS, t, t_len);

	tm_time = gmtime(&fh->fh_s.st_ctime);
	t_len = strftime( t, 13, t_fmt, tm_time);
	tdu_add_string_par(tb, TDU_FH_CREATION, t, t_len);

	pwent = getpwuid(fh->fh_s.st_uid);
	if( pwent == NULL ){
		snprintf(t,13,"%d",fh->fh_s.st_uid);
		owner = t;
	} else {
		owner = pwent->pw_name;
	}
	tdu_add_string_par(tb, TDU_FH_AUTHOR_NAME, owner, strlen(owner));
/* 
 *  permitted actions to be added
 */
	permissions[0] = 0x40;
	permissions[1] = 0x00;
	if( access(fh->fh_name,R_OK) == 0 ) 
		permissions[1] |= TDU_FH_PERMITTED_ACTION_READ;
	if( access(fh->fh_name,W_OK) == 0 )
		permissions[1] |= ( 
			TDU_FH_PERMITTED_ACTION_EXTEND |
				    TDU_FH_PERMITTED_ACTION_INSERT |
				    TDU_FH_PERMITTED_ACTION_REPLACE ) ;
/* 
 *  FIXME: additionally check permissions of the directory in order to set
 *  the TDU_FH_PERMITTED_ACTION_ERASE flag appropriately.
 */
	tdu_add_string_par(tb, TDU_FH_PERMITTED_ACTIONS, permissions, 2);



	/*
	 * several clients get confused when confronted with optional
	 * parameters before the mandatory ones.
	 * Unfortunatly, description file records and ordiary headers
	 * have different mandatory attributes. Thus, we need
	 * to support different order here.
	 */
	if(! fh->fh_xdirrec){
		if(fh->fh_name[0] != 0 )
			tdu_add_string_par(tb, TDU_FH_FILE_NAME, fh->fh_name,
					   strlen(fh->fh_name));
		if(fh->fh_t_name[0] != 0 )
			tdu_add_string_par(tb,TDU_FH_TRANSFER_NAME, fh->
					   fh_t_name,strlen(fh->fh_t_name));
	}
/* 
 * file type/coding; default: data/bin,
 *                 directory: text/printable,
 *        extended directory: description/not_present
 */
	switch(fh->fh_type){
	case TDU_FH_CLASS_EFT_DIR:
	case TDU_FH_CLASS_EFT_LIST:
	case TDU_FH_CLASS_EFT_STRING:
		coding = TDU_FH_FILE_CODING_VTX;
		type = TDU_FH_FILE_TYPE_TEXT;
		break;
	case TDU_FH_CLASS_EFT_DATA:
		coding = TDU_FH_FILE_CODING_BINARY;
		type = TDU_FH_FILE_TYPE_DATA;
		break;
	case TDU_FH_CLASS_EFT_XDIR:
	case TDU_FH_CLASS_EFT_DESCR:
		coding = -1 /* shall not be present */;
		type = TDU_FH_FILE_TYPE_DESCRIPTION;
		break;
	default:
		coding = TDU_FH_FILE_CODING_BINARY;
		type = fh->fh_type;
	};

	if( coding >= 0 )	
		tdu_add_byte_par(tb, TDU_FH_FILE_CODING, coding);
	tdu_add_byte_par(tb, TDU_FH_FILE_TYPE, type);

	tm_time = gmtime(& fh->fh_s.st_mtime);
	t_len = strftime( t, 13, t_fmt, tm_time);
	tdu_add_string_par(tb, TDU_FH_DATE, t, t_len);

	tdu_add_int_par(tb, TDU_FH_FILE_LENGTH, fh->fh_s.st_size);

	if(fh->fh_xdirrec){
		if(fh->fh_name[0] != 0 )
			tdu_add_string_par(tb, TDU_FH_FILE_NAME, fh->fh_name,
					   strlen(fh->fh_name));
		if(fh->fh_t_name[0] != 0 )
			tdu_add_string_par(tb,TDU_FH_TRANSFER_NAME, fh->
					   fh_t_name,strlen(fh->fh_t_name));
	}

	tdu_add_ci_header(tb,TDU_PI_FILE_HEADER);

	return 0;
}


int tdu_fh_get_fd_hdr(struct tdu_stream *ts, unsigned char * buf, int maxlen)
{
	struct tdu_buf tb[1];

	tdu_init_tb(tb);
	tdu_fh_stat(tb,ts->hdr.data);
	ts->hdr.len = tb->tail - tb->data;
	/* FIXME:
	 * handle lenght appropriatly even when not
	 * fitting in on hdr.read() request
	 */
	if( ts->hdr.len <= maxlen ){ 
		memcpy(buf,tb->data,ts->hdr.len);
	} else {
		tdu_printf(TDU_LOG_ERR, "tdu_fh_get_fd_hdr(): "
			    "header too long\n");
		return tdu_fh_get_zero_hdr(ts, buf, maxlen);
	}
	ts->hdr.read = NULL;
	return ts->hdr.len;
}

/*
 * writes a file header to a stream (useful for creating description files/
 * extended directory listings)
 */
void eft_fh_fwrite( FILE * str, unsigned char * tname, unsigned char * fname,
		    struct stat * s )
{
	struct fileheader_par hp;
	struct tdu_buf tb[1];
					
	strncpy(hp.fh_t_name,tname, TDU_PLEN_DESIGNATION);
	hp.fh_t_name[TDU_PLEN_DESIGNATION]=0;

	strncpy(hp.fh_name, fname, TDU_FH_PLEN_NAME);
	hp.fh_name[TDU_FH_PLEN_NAME]=0;

	hp.fh_type = TDU_FH_CLASS_EFT_DATA;
	hp.fh_s = *s;

	hp.fh_xdirrec = 1;

	tdu_init_tb(tb);
	tdu_fh_stat(tb,&hp);
	fwrite(tb->data, 1, tb->tail-tb->data, str);
}




