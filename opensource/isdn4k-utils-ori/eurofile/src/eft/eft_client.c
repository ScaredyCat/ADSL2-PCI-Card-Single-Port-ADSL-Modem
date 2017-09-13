/* $Id: eft_client.c,v 1.3 2001/03/01 14:59:12 paul Exp $ */
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
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>


#include <tdu_user.h>
#include "tdu.h"
#include <eft.h>
#include "eft_private.h"
#include "fileheader.h"
#include "sbv.h"

int eft_save_fd(struct eft * eft, unsigned char * target, int fd, int close)
{
	struct tdu_stream * ts = eft->ts;
	struct tdu_fsm * fsm = ts->fsm;
	struct tdu_transfer_param par[1];
	struct stat s[1];
	int len;

	eft_stream_init_fd(eft,fd,close);

	fstat(fd,s);
	eft_stream_set_stat(eft, s);

	len=LIMIT( strlen(target), TDU_PLEN_DESIGNATION);
	memcpy(par->designation, target, len);
	par->designation_len = len;
	par->udata_len = -1;
	par->recovery_point = -1;
	tdu_t_save_req( fsm, par );

	tdu_wait_for_idle( fsm, 0 );
	eft_print_time( fsm );
	return 0;
}


int eft_save(struct eft * eft, unsigned char * target, unsigned char * src)
{
	int fd;

	fd = open(src, O_RDONLY, 0640 );
	if( fd < 0 ){
		perror("cannot open local file to be transferred");
		return 0;
	}
	tdu_printf(TDU_LOG_LOG,"File \"%s\" opened for reading, fd=%d\n", src,fd);
	eft_stream_set_names(eft, target, src, TDU_FH_FILE_TYPE_DATA);
	return eft_save_fd(eft, target, fd,1);
}


int eft_dir_fd(struct eft * eft, int fd, unsigned char * pattern, int extended)
{
	struct tdu_fsm * fsm = eft->ts->fsm;
	struct tdu_transfer_param par[1];
	int len;

	/* set up receiver for the directory contents */

	eft_stream_init_fd(eft,fd,0);
	/* FIXME: maybe move header initialization to a more central place*/
	len=LIMIT( strlen(pattern), TDU_PLEN_DESIGNATION);
	memcpy(par->designation, pattern, len);
	par->designation_len = len;

	if( extended == 0 ){
		/* request simple directory */
		par->udata_len = -1;
	} else {
		/* request extended directory */
		par->udata_len = 2;
		par->udata[0] = 0x30;
		par->udata[1] = 0x40;
	}
	par->recovery_point = -1;

	/* printf("Contents of directory:\n");*/ /* new line and flush on stdout */
	tdu_t_dir_req( fsm, par );
	tdu_wait_for_idle( fsm, 0 );
	eft_print_time( fsm );
	return 0;
}

/* 
 * Extended directory request.
 * Currently only prints file header attributes mandatory in xdir response.
 */
int eft_xdir_txt(struct eft * eft, unsigned char * pattern)
{
	int ret, fd2, len, m;
	struct tdu_buf tb[1];
	struct fileheader_par fhp;
	char s[20];
	unsigned char *sz;
	int i, dumb=0;

	eft->fh->fh_type=0;
	fd2 = eft_get_tmp(eft);
#if 0
	/* This #if branch for testing xdir fallback */
	ret = eft_dir_fd(eft, fd2, pattern, 0);
#else
	ret = eft_dir_fd(eft, fd2, pattern, 1);
#endif
	if( eft->fh->fh_type != TDU_FH_FILE_TYPE_DESCRIPTION ){
		printf("eft: not a description file, server does not to "
		       "support extended directory format\n");
		dumb=1;
	}
	len = 0;
	m = 0;
	do {
		m += len;
		lseek(fd2, m, SEEK_SET);
		tdu_init_tb(tb);
		tb->data = tb->head;
		len = read(fd2, tb->data, tb->end - tb->data);
		if( len <= 0 ) break;
		tb->tail = tb->data+len;
		if( dumb ) {
			/* Fallback parsing text formatted t-dir
			 * response file */
			memset(&fhp,0,sizeof(fhp));
			for(i=0;i<13;i++){
				if(tb->data[i] == ' '){
					sz = tb->data+i;
					break;
				}
			}
			*sz=0;
			sz++;
			sz[10]=0;
			strcpy(fhp.fh_t_name,tb->data);
			strcpy(fhp.fh_name,tb->data);
			fhp.fh_s.st_size=atoi(sz);
			for(i=0;i<13;i++){
				if(sz[i] == '\n'){
					len=(sz-(tb->data))+i+1;
					break;
				}
			}
		} else {
			len = tdu_parse_fh(&fhp,tb);
					}
		i = strftime(s, 20, "%Y-%m-%d %H:%M:%S", 
			     localtime(&fhp.fh_s.st_mtime));
		printf("%-12s %8ld  %19s %s\n", fhp.fh_t_name,
		       fhp.fh_s.st_size, s, fhp.fh_name);
	} while ( len > 0 );

	return ret;
}


int eft_list_fd(struct eft * eft, int fd, int close)
{
	return eft_load_fd(eft, fd, "EUROSFT92/NAVIGATION/LIST", close);
}


int eft_slist_fd(struct eft * eft, int fd, int close)
{
	return eft_load_fd(eft, fd, "EUROSFT92/NAVIGATION/S-LIST", close);
}


int eft_getcwd (struct eft * eft, unsigned char * dir)
{
	int ret, fd=eft_get_tmp(eft);
	char fstore[EFT_MAX_FSTORE_LEN+1];

	ret = eft_load_fd(eft, fd, "EUROSFT92/NAVIGATION/S-FILESTORE", 0);
	if(ret < 0) goto close;
	if( lseek(fd, 0, SEEK_SET) < 0 ){
		perror("eft_getcwd: lseek()");
		goto close;
	}
	if( (ret=read(fd,fstore,EFT_MAX_FSTORE_LEN)) < 0){
		perror("eft_getcwd: read()");
		goto close;
	}
	fstore[ret]=0;
	strcpy(dir,fstore);
close:
	close(fd);
	return ret;
}


int eft_save_string (struct eft * eft, unsigned char * designation, char * str)
{
	int ret, fd;

	fd = eft_get_string_fd(eft,str);

	if( fd < 0 ){
		return 0;
	}

	eft_stream_set_names(eft,designation,"",TDU_FH_CLASS_EFT_STRING);
	ret = eft_save_fd(eft, designation, fd, 1);
/* not necessary
	close(fd);
	*/
	return ret;
}


int eft_cd (struct eft * eft, unsigned char * dir)
{
	if( ! dir ) dir = "EUROSFT92/NAVIGATION/RESET";
	return eft_save_string (eft, "EUROSFT92/NAVIGATION/SELECT", dir);
}


int eft_mkdir (struct eft * eft, unsigned char * dir)
{
	if( ! dir ) return 0;
	return eft_save_string (eft, "EUROSFT92/NAVIGATION/CREATE", dir);
}


static int eft_access(struct eft * eft)
{
	struct tdu_fsm * fsm = eft->ts->fsm;
	struct tdu_access_param par;
	int ret;

	tdu_set_default_access_param(&par, fsm->user, fsm->stream);
	par.role = TDU_ROLE_MASTER;
	par.window = 8;
	par.udata_len=3;
#if 1
	/* t_dir applies to group A, application name subset */
	par.udata[0] = 0x89;
#else
	par.udata[0] = 0x99;
#endif
	par.udata[1] = 0x24;
	par.udata[2] = 0x4f;

	tdu_access_req(fsm,&par);
	ret = tdu_wait_for_idle( fsm, 0 );
	return ret;
}


static int eft_end_access(struct eft * eft)
{
	struct tdu_fsm * fsm = eft->ts->fsm;

	tdu_printf(TDU_LOG_TRC, "eft_end_access()\n");

	tdu_end_access_req(fsm);
	tdu_wait_for_end_access( fsm, 0 );
	return 0;
}


int eft_load(struct eft * eft, unsigned char * target, unsigned char * src)
{
	int fd;

	tdu_printf(TDU_LOG_TRC, "eft_load()\n");

	fd = open(target, O_WRONLY | O_CREAT, 0640 );
	if( fd < 0 ){
		perror("cannot open local load target file");
		return 0;
	}
	tdu_printf(TDU_LOG_LOG,"File \"%s\" opened for writing, fd=%d\n", target, fd);
	return( eft_load_fd(eft, fd, src, 1) );
}


int eft_load_fd(struct eft * eft, int fd, unsigned char * src, int close)
{
	struct tdu_stream * ts = eft->ts;
	struct tdu_fsm * fsm = ts->fsm;
	struct tdu_transfer_param par[1];
	int len;

	tdu_printf(TDU_LOG_TRC, "eft_load_fd()\n");

	ts->fsm = fsm;
	ts->fd = fd;
	ts->t_write     = tdu_fd_t_write;
	ts->t_write_end = tdu_fd_t_write_end;
	if(close) {
		ts->close   = tdu_fd_close;
	} else {
		ts->close   = tdu_no_close;
	}
	ts->abort       = tdu_std_abort;
	ts->hdr.read    = NULL;
	ts->hdr.parse   = tdu_fh_parse_print;
	ts->hdr.len     = 0;

	len=LIMIT( strlen(src), TDU_PLEN_DESIGNATION);
	memcpy(par->designation, src, len);
	par->designation_len = len;
	par->udata_len = -1;
	par->recovery_point = -1;
	tdu_t_load_req( fsm, par );

	tdu_wait_for_idle( fsm, 0 );
	eft_print_time( fsm );
	return 0;
}


static int eft_associate(struct eft * eft, unsigned char * ident )
{
	struct tdu_fsm * fsm = eft->ts->fsm;
	struct tdu_assoc_param par;
	unsigned char  
		*cname = "eftp4linux-" E4L_VERSION,
		*aname = "!K";

	tdu_printf(TDU_LOG_TRC, "eft_associate()\n");

	tdu_set_default_assoc_param( &par );

	par.calling_len = LIMIT(strlen(cname), TDU_PLEN_ADDR); 
	memcpy( par.calling_addr, cname, par.calling_len);

	par.appl_len = LIMIT(strlen(aname), TDU_PLEN_APPLNAME );
	memcpy( par.appl_name, aname, par.appl_len);

	par.resp_timeout = 12; /* initially, in violation to ETS 300 075,
				  but we do this for paranoia reasons */
	par.symm_service = 1;
	par.basic_kernel= 0;

	par.ident_len = -1;
	if( ident ){
		par.ident_len = LIMIT(strlen(ident), TDU_PLEN_IDENT );
		memcpy( par.ident, ident, par.ident_len);
	}
	par.req_ident = TDU_BIT_REQ_ID;
	par.req_ident = 0;

	if(! eft_signature){
		par.udata_len = -1;
	} else {
		par.udata_len = LIMIT(strlen(eft_signature), TDU_PLEN_UDATA);
		memcpy( par.udata, eft_signature, par.udata_len);
	}
	tdu_assoc_req(fsm,&par);
	return tdu_wait_for_not_associating( fsm, 0 );
}


int eft_connect(struct eft * eft, unsigned char * ident)
{
	int ret, sk=eft_get_socket(eft);
	tdu_extended_reason = eft_re_descr;

	tdu_printf(TDU_LOG_TRC, "eft_connect()\n");

	tdu_printf(TDU_LOG_DBG,"eft_connect(): requesting SBV connection\n");
	ret = tdu_sbv_connect(sk);
	if( ret < 0 ) return ret;

	eft->fsm->idle.initiator = 1;
	eft->fsm->user->t_typed_data = eft_msg2stdout;
	tdu_init_master(eft->fsm);
	tdu_initial_set_idle(eft->fsm);

	tdu_printf(TDU_LOG_DBG,"eft_connect(): requesting eft_associate()\n");
	ret = eft_associate( eft, ident );
	tdu_printf(TDU_LOG_DBG,"eft_associate returned %d\n",ret);
	if( ret < 0 ) return ret;
	
	tdu_printf(TDU_LOG_DBG,"eft_connect(): requesting eft_access()\n");
	ret = eft_access( eft );
	tdu_printf(TDU_LOG_DBG,"eft_access returned %d\n",ret);
	if( ret < 0 ) eft_release(eft);

	return ret;
}


int eft_disconnect(struct eft * eft)
{
	int ret, sk=eft_get_socket(eft);

	tdu_printf(TDU_LOG_TRC, "eft_disconnect()\n");

	tdu_printf(TDU_LOG_DBG,"eft_disconnect(): requesting eft_end_access()\n");
	if( (ret = eft_end_access(eft)) ) return ret;

	tdu_printf(TDU_LOG_DBG,"eft_disconnect(): requesting eft_release()\n");
	ret = eft_release(eft);

	tdu_printf(TDU_LOG_DBG,"eft_disconnect(): requesting SBV disconnect\n");
	tdu_sbv_disconnect(sk);
	return ret;
}

void eft_prompt(char *prompt)
{
	tdu_prompt(prompt);
}
