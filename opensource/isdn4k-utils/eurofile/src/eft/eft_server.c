/* $Id: eft_server.c,v 1.3 1999/10/06 18:16:22 he Exp $ */
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
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h> /*NEW libc2: realpath*/

#include <pwd.h>
#include <sys/types.h>

#include <tdu_user.h>
#include "sbv.h"
#include "tdu.h"
#include <eft.h>
#include "eft_private.h"
#include "fileheader.h"

/*
 * global variable for eft flat dir. Is tried as default directory when
 * server thinks that the client does not support navigation.
 */
const char * eft_flat_dir_name = EFT_METAFILE_PREFIX "_flat";
/*
 * map unknown users to this if not NULL
 */
char * eft_map_to_user = NULL;


static int assoc_ind( struct tdu_user *usr, struct tdu_param *par)
{
	struct eft * eft = usr->priv;
	unsigned char calling[TDU_PLEN_ADDR+1]="", udata[TDU_PLEN_UDATA+1]="",
		*user="", *pass="";
	struct tdu_assoc_param *respar=par->res.assoc, *inpar=par->par.assoc;

	par->other_reason[0] = 0;
	par->other_reason[1] = 0;

	/*
	 * the tdu layer checks that the maximum udata len is not exceeded,
	 * which should prevent overflow attacks here.
	 */
	tdu_mk_printable(udata, inpar->udata, inpar->udata_len);
	tdu_mk_printable(calling, inpar->calling_addr, inpar->calling_len);

	tdu_printf(TDU_LOG_TRC," assoc_ind, ident_len=%d\n",inpar->ident_len);

	eft_set_flags(eft,eft_get_flags(eft)|EFT_FLAG_STRICT_TREE);

	if( inpar->ident_len > 0 ){
 		user = inpar->ident;
		/* Prevent possible buffer overflows by string functions
		 * depending on zero termination.
		 * The struct tdu_assoc_param contains space for adding
		 * an extra delimiting \0 charater.
		 * Be also careful with missing pass here
		 */
		user[inpar->ident_len]=0;
		pass = user;
		while( pass-user < inpar->ident_len ){
			if(*pass == '/'){
				*pass = 0;
				pass++;
				break;
			}
			pass++;
		};
		tdu_printf(TDU_LOG_AP1,"IND: ASSOC  id=%s calling=%s udata=%s\n",user,calling,udata ); 
		/* 
		 * A leading '+' is used to indicate that the server shall
		 * apply the case-insensitiveness or other compatibility
		 * hacks for transfer names.
		 * The '+' is not part of the user id, thus stripping it.
		 */
		if( user[0] == '+' ){
			long flags;
			user++;

			flags = eft_get_flags(eft);
			eft_set_flags(eft,flags|EFT_FLAG_CASEFIX_TN|
				      EFT_FLAG_CASEFIX_FS|EFT_FLAG_SLASHFIX);
		}
		/* 20.11.98, ms@msdatec.de: mapping to user eft_map_to_user, if 	  failed 
		 *
		 * Slightly modified and moved here (1999-01-07) -- HE:
		 *
		 * Now mapping to default user only if user is not in
		 * passwd database.
		 *
		 * FIXME:
		 * This check might be inappropriate if other authentification
		 * methods (not based on /etc/passwd) are used.
		 */
		
		if (eft_map_to_user) {
			struct passwd *pw;
			/*
			 */
			setpwent();
			while( (pw=getpwent()) ){
				if( strcmp(user,pw->pw_name) == 0) break;
			};
			endpwent();
			
			if( ! pw ){
				/* user is unknown (not in /etc/passwd).*/
				tdu_printf(TDU_LOG_LOG, "user %s not known, trying alternate authentification as user \"%s\" instead\n",
					   user, eft_map_to_user);
				user = eft_map_to_user;
				pass = "";
			}
		}

		strcpy(eft->user_name,user);
		par->reason = eft->check_user(eft,user,pass,eft_peer_phone(eft));
	} else {
		strcpy(eft->user_name,"(empty)");
		tdu_printf(TDU_LOG_AP1,"IND: ASSOC  with empty id" 
			   " calling=%s udata=%s\n", calling, udata ); 
		par->reason = eft->check_user(eft,"","",eft_peer_phone(eft));
	}
	
	respar->resp_timeout = 15;
	respar->called_len = LIMIT(strlen(eft->address), TDU_PLEN_ADDR);
	memcpy( respar->called_addr, eft->address, respar->called_len);

	if(! eft_signature ){
		respar->udata_len = -1;
	} else {
		respar->udata_len = LIMIT(strlen(eft_signature), TDU_PLEN_UDATA);
		memcpy( respar->udata, eft_signature, respar->udata_len);
	}

	if( par->reason ) {
		tdu_printf(TDU_LOG_AP1,"REJ: ASSOC  user=%s from=%s\n",
		   user, eft_peer_phone(eft)); 
	} else {
		tdu_printf(TDU_LOG_AP1,"ACP: ASSOC  user=%s from=%s\n",
		   user, eft_peer_phone(eft)); 
	}

	tdu_printf(TDU_LOG_LOG,"user \"%s\" from %s %s\n",
		   user, eft_peer_phone(eft), 
		   par->reason ? "rejected" : "accepted");
	if( par->reason == TDU_RE_OTHER_REASON ) 
		par->other_reason[0] = EFT_RE_ID_REJECTED;
 
	return par->reason;
}

/*
 * called from the tdu layer when an access indication is received.
 */
static int access_ind( struct tdu_user *usr, struct tdu_param *par)
{
	struct eft * eft = usr->priv;

	par->other_reason[0] = 0;
	par->other_reason[1] = 0;
	
	tdu_printf(TDU_LOG_TRC,"eft_server access_ind()\n");

	par->res.access->udata_len = 3;
	par->res.access->window = 8;
#if 1
	/* t_dir applies to group A, application name subset */
	par->res.access->udata[0] = 0x89;
#else
	par->res.access->udata[0] = 0x99;
#endif
	par->res.access->udata[1] = 0x24;
	par->res.access->udata[2] = 0x4f; /* navigation supported */

	par->reason = eft->setup_user(eft);
	eft_set_home(eft);
	tdu_printf(TDU_LOG_LOG,"access_ind: user set up.\n");

	tdu_printf(TDU_LOG_AP2,"IND: ACCESS features=XXX_log_peer_supported_feastures_here\n");

	return( par->reason );
}

/*
 * Write a log record about transferred file into to a log file.
 *
 * The format is the same as used by wu-ftpd 2.4.2 although slightly
 * different semantics (because eft != ftp) apply to certain fields.
 * Thus, you should be able to use or modify existing tools for 
 * parsing your log file.
 */
static void eft_log_xfer(struct eft * eft){
#define MAX_LOGREC_LEN MAXPATHLEN+400
	char msg[MAX_LOGREC_LEN];
	struct tdu_fsm * fsm = eft->fsm;
	struct transfer_regime *tr = & fsm->transfer;
	int tr_sec = tr->tv_end.tv_sec  - tr->tv_begin.tv_sec;
	int bytes = tr->byte_cnt - fsm->stream->hdr.len, g;
	time_t t = time(NULL);

	if( eft->xferlog <= 0 ) return;
	g='r';
	if( eft->flags & EFT_FLAG_ANONYMOUS ){
		g = (eft->flags & EFT_FLAG_GUEST) ? 'g' : 'a';
	};

        snprintf(msg, MAX_LOGREC_LEN,
		 "%.24s %d %s %d %s %c %s %c %c %s eft %d %s\n",
		 ctime(&t),
		 tr_sec,
		 eft_peer_phone(eft), /* phone number instead of domain name*/
		 bytes,
		 eft->fn,
		 'b',  /* we currently support binary transfers only */
		 "_",  /* for transfer options like on-the-fly tar/gzip, which
			* is currently not supported */
		 tr->outbound ? 'o' : 'i',
		 g,
		 eft->user_name,
		 0, /* authenticated e-mail address does not make sense with
		       eurofile as we are probably outside internet domain */
		 "*" /* dto */
		);
	write(eft->xferlog,msg,strlen(msg));
}

static void eft_write_xfer_log(struct tdu_user * usr, int abnormal)
{
	usr->transfer_end = NULL;
	eft_log_xfer(usr->priv);
	if(abnormal){
		tdu_printf(TDU_LOG_AP2, "TRF: ABORT \n");
	} else {
		tdu_printf(TDU_LOG_AP3, "TRF: FINISH\n");
	}
}

/* 
 * This is called from the tdu protocol layer when a request for a download
 * is indicated. Basically, it provides us the name of the file to be
 * downlowded. This callback functions needs to do the following:
 * - map the transfer name of the file to a POSIX file name
 * - decide whether we want to grant the peer read access permission to
 *   file
 * - open the file in question
 * - return a tdu_stream object back to the tdu layer. The tdu layer then
 *   will use the tdu_stream object and its methods for reading the file
 */   
int eft_load_ind( struct tdu_user * usr, struct tdu_param * par )
{
	unsigned char fnbuf[MAXPATHLEN], *fn="",
		*tn = par->par.transfer->designation;
	/* struct tdu_stream * ts = tdu_user_get_stream(usr); */
	int fd, len=par->par.transfer->designation_len, stat_d=0,size;
	struct stat s[1];
	struct eft * eft = usr->priv;

	usr->fsm->transfer.outbound = 1;
	par->other_reason[0] = 0;
	par->other_reason[1] = 0;
	/* FIXME: is len limited? */
	/* len has at least been checked to be >=0 by lower layer */
	tn[len]=0;
#if 0
	/* special hack to support certain rare non-conformaning application */
	{
		int i=0;
		while( tn[i] ){
			if( tn[i] == '\\') tn[i] = '/';
			i++;
		}
	}
#endif
	if( strcmp(tn, "EUROSFT92/NAVIGATION/S-LIST") == 0 ) {
		tdu_printf(TDU_LOG_AP3,"IND: SLIST \n" ); 
		fd = eft_store_slist(eft,".",NULL);
		eft_stream_set_names(eft, tn, fn, TDU_FH_CLASS_EFT_LIST);
		stat_d = 1;
	} else if( strcmp(tn, "EUROSFT92/NAVIGATION/S-FILESTORE") == 0 ) {
		tdu_printf(TDU_LOG_AP3,"IND: PWD   \n" ); 
		fd = eft_store_cwd(eft);
		eft_stream_set_names(eft, tn, fn, TDU_FH_CLASS_EFT_STRING);
	} else if( strcmp(tn, "EUROSFT92/NAVIGATION/LIST") == 0 ) {
		tdu_printf(TDU_LOG_AP3,"IND: LIST  \n" ); 
		par->reason = TDU_RE_UNKNOWN_FILE;
		tdu_printf(TDU_LOG_AP3,"REJ: LIST   reason=%s\n", 
			   eft_str_reason(par->reason) ); 
		return( par->reason );
	} else if( eft_acceptable_tname(tn) ) {
		fn = eft_fn_by_tn(fnbuf,tn,eft_get_flags(eft));
		tdu_printf(TDU_LOG_AP2,"IND: LOAD   t_name=%s f_name=%s\n",tn,fn); 
		if( realpath(fn,eft->fn) && eft_file_is_tabu(eft->fn) ) {
			par->other_reason[0] = EFT_RE_FILE_ACCESS_IMPOSSIBLE;
			tdu_printf(TDU_LOG_AP2,"REJ: LOAD   reason=%s[tabu]\n",
				   eft_str_other_reason(par->other_reason[0]));
			return( par->reason = TDU_RE_OTHER_REASON);
		}
		fd = open(fn, O_RDONLY, 0640 );
		if( fd < 0 ){
			switch ( errno ) {
			case ENOENT:
				par->reason = TDU_RE_UNKNOWN_FILE;
				break;
			default:
				par->reason = TDU_RE_OTHER_REASON;
				par->other_reason[0] =
					EFT_RE_FILE_ACCESS_IMPOSSIBLE;
			}
			tdu_printf(TDU_LOG_AP2,"REJ: LOAD   reason=%s[%s]\n", 
				   par->reason == TDU_RE_OTHER_REASON ?
				   eft_str_other_reason(par->other_reason[0]) :
				   eft_str_reason(par->reason),
				   strerror(errno));
			return( par->reason );
		}
		eft_stream_set_names(eft, par->par.transfer->designation, fn,
				     TDU_FH_CLASS_EFT_DATA);
		usr->transfer_end = eft_write_xfer_log;
	} else {
		tdu_printf(TDU_LOG_AP2,"IND: LOAD   t_name=%s\n",tn);
		par->reason = TDU_RE_ERRON_DESIGN; 
		tdu_printf(TDU_LOG_AP2,"REJ: LOAD   reason=%s\n",
			   eft_str_reason(par->reason)); 
		return( par->reason);
	}

	if( fd < 0 ){
		perror("cannot open local file to be transferred");
		par->reason = TDU_RE_UNKNOWN_FILE;
		tdu_printf(TDU_LOG_AP2,"REJ: LOAD   reason=%s\n",
			   eft_str_reason(par->reason)); 
		return( par->reason );
	}
	tdu_printf(TDU_LOG_LOG,"File \"%s\" opened for reading, fd=%d\n", fn,fd);
	eft_stream_init_fd(eft,fd,1);
	fstat(fd,s);
	if( stat_d ){
		size = s->st_size;
		stat(".",s);
		s->st_size = size;
	}
	eft_stream_set_stat(eft, s);
	return 0;
}


/* 
 * This is called from the tdu protocol layer when a t_directory indication.'
 * is received. Basically, pattern to selcet the requested filenames is
 * provided. This callback function needs to do the following:
 * - Check designatation parameter for validity and decide whether we
 *   want to grant the peer read access permission to the directory contents
 *   in question.
 * - build a diretory contents file.
 * - return a tdu_stream object attached to the directory contents
 *   back to the tdu layer. The tdu layer then will use the tdu_stream
 *   object and its methods for reading diretory contents.
 */   
int eft_dir_ind( struct tdu_user * usr, struct tdu_param * par )
{
	unsigned char fn[TDU_PLEN_DESIGNATION+1];
	/* struct tdu_stream * ts = tdu_user_get_stream(usr); */
	int i, fd, len=par->par.transfer->designation_len, size, extended
		, class;
	struct eft * eft = usr->priv;
	struct stat s[1];

	usr->fsm->transfer.outbound = 1;
	par->other_reason[0] = 0;
	par->other_reason[1] = 0;
	extended = (par->par.transfer->udata_len >= 2) &&
		(par->par.transfer->udata[1] == 0x40);
	strncpy(fn,par->par.transfer->designation,len);
	tdu_printf(TDU_LOG_AP3,"IND: DIR    extended=%d\n",extended ); 

	for( i=0; i< len; i++ ){
		if( fn[i] == '/' ) fn[i] = 0;
	}
	fn[len] = 0;
	/*
	 * we currently just show the contents of the working directory
	 */
	if( (fd = eft_store_dir(eft,".",NULL,extended)) < 0 ){
		/* FIXME: wrong error code ??? */
		*par->other_reason = EFT_RE_FILE_ACCESS_IMPOSSIBLE;
		par->reason_len = 1;
		return ( par->reason = TDU_RE_OTHER_REASON );
	}

	eft_stream_init_fd(eft,fd,1);
	class = extended  ?  TDU_FH_CLASS_EFT_XDIR : TDU_FH_CLASS_EFT_DIR;
	eft_stream_set_names(eft, "", "", class);
	/* 
	 * size from generated directory content file, other attributes from
	 * directory's stat entries
	 */
	fstat(fd,s);
	size = s->st_size;
	stat(".",s);
	s->st_size = size;
	eft_stream_set_stat(eft, s);
	return 0;
}



/* 
 * This is called from the tdu protocol layer when the peer indicates that
 * it wants to upload a file to us.
 * Basically, it provides us the name of the file to be uploaded. This
 * callback function needs to do the following:
 * - decide whether we want to grant the peer write access permission to
 *   the file and current directory
 * - open/create the file in question
 * - return a tdu_stream object back to the tdu layer. The tdu layer then
 *   will use the tdu_stream object and its methods for writing to the file.
 */   
int eft_save_ind( struct tdu_user * usr, struct tdu_param * par )
{
	unsigned char fnbuf[MAXPATHLEN+1],
		*tn = par->par.transfer->designation,
		*fn = fnbuf;
	struct tdu_stream * ts = tdu_user_get_stream(usr);
	int fd, cd=0, len=par->par.transfer->designation_len;
	struct eft * eft = usr->priv;

	usr->fsm->transfer.outbound = 0;
	par->reason = 0;
	par->other_reason[0] = 0;
	par->other_reason[1] = 0;
	tn[len]=0;
	fn[0]=0;

	if( strcmp(tn, "EUROSFT92/NAVIGATION/SELECT") == 0 ) {
		tdu_printf(TDU_LOG_AP2,"IND: CHDIR \n" ); 
		fd = eft_get_tmp(eft);
		cd = 1;
	} else if( eft_acceptable_tname(tn) &&
		   (strncmp("EUROSFT92/",tn,10)!=0) ){
		fn = eft_fn_by_tn(fnbuf,tn,eft_get_flags(eft));
		tdu_printf(TDU_LOG_AP2,"IND: SAVE   t_name=%s f_name=%s\n",tn,fn);
		if( realpath(fn,eft->fn) && eft_file_is_tabu(eft->fn) ) {
			par->other_reason[0] = EFT_RE_FILE_ACCESS_IMPOSSIBLE;
			tdu_printf(TDU_LOG_AP2,"REJ: SAVE   reason=%s[tabu]\n",
				   eft_str_other_reason(par->other_reason[0]) ); 
			return( par->reason = TDU_RE_OTHER_REASON);
		}
		fd = open(fn, O_WRONLY | O_CREAT, 0640 );
	} else {
		tdu_printf(TDU_LOG_AP2,"IND: SAVE   t_name=%s\n",tn);
		par->reason = TDU_RE_ERRON_DESIGN;
		tdu_printf(TDU_LOG_AP2,"REJ: SAVE   reason=%s\n",
			   eft_str_reason(par->reason) ); 

		return( par->reason );
	}

	if( fd < 0 ){
		tdu_printf(TDU_LOG_DBG,"cannot create local file to be transferred\n");
		switch ( errno ) {
		case ENOENT:
			par->reason = TDU_RE_UNKNOWN_FILE;
			break;
		case EEXIST:
			par->reason = TDU_RE_FILE_EXISTS;
			break;
		case ENOSPC:
			par->reason = TDU_RE_OTHER_REASON;
			par->other_reason[0] = EFT_RE_DISK_FULL;
			break;
		default:
			par->reason = TDU_RE_OTHER_REASON;
			par->other_reason[0] =
				EFT_RE_FILE_ACCESS_IMPOSSIBLE;
		}
		tdu_printf(TDU_LOG_AP2,"REJ: SAVE   reason=%s[%s]\n", 
			   par->reason == TDU_RE_OTHER_REASON ?
			   eft_str_other_reason(par->other_reason[0]) :
			   eft_str_reason(par->reason),
			   strerror(errno)
			); 
		return( par->reason );
	}
	tdu_printf(TDU_LOG_DBG,"File \"%s\" opened for writing, fd=%d\n", fn,fd);
	
	tdu_stream_init_fd(ts,fd,1);
        if( cd ){
		ts->close = eft_cd_close;
	} else {
		usr->transfer_end = eft_write_xfer_log;
	}
	return 0;
}



int eft_accept_user(struct eft * eft)
{
	int ret, sk = eft_get_socket(eft);
	struct tdu_user * user = eft_get_user(eft);
	unsigned char isdn_no[21];
	time_t t;

	eft_get_peer_phone(isdn_no,sk);
	eft_add_peer_phone(eft,isdn_no);
	t = time(NULL);
	tdu_printf(TDU_LOG_LOG,"awaiting SBV connection from %s\n", isdn_no);
	ret = tdu_sbv_accept(sk);
	if( ret ){
		tdu_printf(TDU_LOG_ERR,"SBV connection failed, ret=%d\n",ret);
		return ret;
		}
	tdu_printf(TDU_LOG_LOG,"SBV connection accepted\n");

	user->t_associate = assoc_ind;
	user->t_access = access_ind;
	user->t_typed_data = eft_msg2stdout;
	user->t_load = eft_load_ind;
	user->t_save = eft_save_ind;
	user->t_dir = eft_dir_ind;
	user->t_rename = NULL;
	user->t_delete = NULL;
	user->transfer_end = NULL;

	tdu_init_slave(eft->fsm);
	eft->fsm->idle.initiator = 0;
	tdu_initial_set_idle(eft->fsm);
	tdu_assoc_allow_listen(eft->fsm);

	if (eft_is_up(eft)){
		tdu_printf(TDU_LOG_DBG,
			   "eft_accept_user(): waiting for associate request\n");
		ret = tdu_dispatch_packet(eft->fsm);
	}
	tdu_printf(TDU_LOG_DBG,"eft_accept_user: ret=%d\n",ret);

	return ret;
}

int eft_server_mainloop( struct eft * eft )
{
	while(eft_is_up(eft)){
		tdu_dispatch_packet(eft->fsm);
	}
	return 0;
}

void eft_set_auth(struct eft * eft, 
		  int (*check_user)(struct eft *, char *, char *,char *),
		  int (*setup_user)(struct eft *),
		  int (*cleanup_user)(struct eft *)
		  )
{
	eft->check_user = check_user ? *check_user : NULL;
	eft->setup_user = setup_user ? *setup_user : NULL;
	eft->cleanup_user = cleanup_user ? *cleanup_user : NULL;
}
