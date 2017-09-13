/* $Id: eft.c,v 1.1 1999/06/30 17:18:11 he Exp $ */
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
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


#include <tdu_user.h>
#include "tdu.h"
#include <eft.h>
#include "eft_private.h"
#include "fileheader.h"
#include <malloc.h>
#include <netdb.h>
#include "extra_version_.h"

/* Table with extended reason codes for EUROFile */

static struct tdu_descr re_descr[] = {
{"identifier rejected",			EFT_RE_ID_REJECTED,0},
{"disk full",				EFT_RE_DISK_FULL,0},
{"file access impossible",		EFT_RE_FILE_ACCESS_IMPOSSIBLE,0},
{"reserved",				EFT_RE_RESERVED,0},
{"user interrupt of communication",	EFT_RE_USER_INTERRUPT,0},
{"user abort",				EFT_RE_USER_ABORT,0},
{"extended format not available",	EFT_RE_NO_EXTENDED_FMT,0},
{"log access impossible",		EFT_RE_NO_LOG_ACCESS,0},
{"compression format not supported",	EFT_RE_CMPR_FMT_NOT_SUPPORTED,0},
{"incorrect recovery FCS",		EFT_RE_WRONG_FCS,0},
{"coding error in compressed data",	EFT_RE_CMPR_CODING_ERROR,0},
{NULL, 			0,0}
};

/* ETS 300 383 (1995) Annex D.2 */
const char *eft_signature="EUROSFT92/1/UNIX/eftp4linux/" E4L_VERSION E4L_EXTRA_VERSION_REV E4L_EXTRA_VERSION_CHANGED;

const char * eft_str_reason(int reason)
{
	return tdu_str_reason(reason);
}

const char * eft_str_other_reason(int reason)
{
	static const char unknown[] = "Unknown_EftOtherReason_Code";
	char * des = tdu_des(re_descr,reason);
	return des ? des : unknown;
}

/*
 * Set the address (used as local called/calling address parameter in
 * T-Associate request and confirmation.
 * 
 * returns 0 on success or something else when string is too long;
*/
int eft_set_address(struct eft * eft, char * address)
{
	int ret=0;
	if(address){
		if(strlen(address)>TDU_PLEN_ADDR){
			fprintf(stderr,"eftd: address \"%s\" too long (truncated at %d chars).\n", address, TDU_PLEN_ADDR);
			ret = -1;
		}
		strncpy(eft->address,address,TDU_PLEN_ADDR);
		eft->address[TDU_PLEN_ADDR]=0;
	} else {
		eft->address = NULL;
	}
	return ret;
}

char * eft_re_descr(int re)
{
	return tdu_des(re_descr,re);
}

int eft_attach_socket(struct eft *eft, int s)
{
	eft->fsm->socket = s;
	return 0;
}

int eft_get_socket(struct eft *eft)
{
	return eft->fsm->socket;
}

int eft_is_up(struct eft * eft)
{
	return eft->fsm->up;
}


int eft_select( struct eft *eft, int n, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout)
{
	tdu_printf(TDU_LOG_TRC,"eft_select()\n");
	return tdu_select(eft->fsm, n, readfds, writefds, exceptfds,
			  timeout);
}


void eft_print_time( struct tdu_fsm * fsm )
{
	struct transfer_regime *tr = & fsm->transfer;
	int usec;
	double sec;

	usec = ( tr->tv_end.tv_usec - tr->tv_begin.tv_usec );  
	sec =     ( tr->tv_end.tv_sec  - tr->tv_begin.tv_sec  )
		+ usec / 1000000.0;
	if( usec < 0 ) sec += 1.0;

	printf("%d packets, %d bytes transferred in %f seconds (%f bytes/sec)\n",
	       tr->pkt_cnt, tr->byte_cnt,
	       sec, tr->byte_cnt/sec );
	printf("(%d bytes file header, %d bytes data)\n", fsm->stream->hdr.len,
	       tr->byte_cnt - fsm->stream->hdr.len);
}

/*
 * private, no function beside eft_make_instance should need to know about
 * this 
 */
struct eft_instance{
	struct eft h; /* handle for eft machine */
	struct tdu_stream eft_tdu_stream;
	struct tdu_user eft_tdu_user;
	struct tdu_fsm eft_tdu_fsm;
	struct fileheader_par eft_fh;
#define MAX_ISDN_NO 20
	unsigned char isdn_no[MAX_ISDN_NO+1] /* of peer*/;
	unsigned char user_name[TDU_PLEN_IDENT+1] /* of peer*/;
	char address[TDU_PLEN_ADDR+1]; /* T_ASSOCICATE parameter */
};

/*
 * Set the default address of the eft instance
 * (contributed by Matthias Stolte <ms@msdatec.de>; slightly modified)
 */
static void set_default_address(struct eft * eft)
{
	char hostname[TDU_PLEN_ADDR+1];
	struct hostent *phinfo;
	
	if (0 == gethostname(hostname, sizeof(hostname))) {
		phinfo = gethostbyname(hostname);
		if (NULL == phinfo) {
			herror(hostname);
			}
		else {
			eft_set_address(eft, phinfo->h_name);
			/* FIXME: user defineable log mask isn't read until now */			
			tdu_printf(TDU_LOG_LOG, "address set to \"%s\" initially\n", eft->address);
			}
		}
	else
		perror("gethostname");
}


struct eft * eft_make_instance()
{
	struct eft_instance * p;

	p=malloc(sizeof(*p));
	if( ! p ) return NULL;
	memset(p,0,sizeof(*p));

	p->h.ts               = & p-> eft_tdu_stream;
	p->h.fsm              = & p-> eft_tdu_fsm;
	p->h.usr              = & p-> eft_tdu_user;
	p->h.fh               = & p-> eft_fh;
	p->eft_tdu_stream.fsm =   p->h.fsm;
	p->eft_tdu_fsm.stream =   p->h.ts;
	p->eft_tdu_user.fsm   =   p->h.fsm;
	p->eft_tdu_fsm.user   =   p->h.usr;
	p->h.usr->priv        = & p->h;
	p->h.isdn_no 	      =   p->isdn_no;
	p->h.user_name 	      =   p->user_name;
	p->h.tmp_fd = eft_make_tmp();
	p->h.address	      =   p->address;

	set_default_address(&p->h);
	return &p->h;
/* FIXME: what about/ where should go file header stuff */
}

int eft_release(struct eft * eft)
{
	struct tdu_fsm * fsm = eft->ts->fsm;

	tdu_printf(TDU_LOG_TRC, "eft_release_req()\n");

	if( tdu_release_req(fsm) >= 0) return -1;
	tdu_wait_for_release( fsm, 0 );
	return 0;
}

int eft_msg(struct eft * eft, unsigned char * msg)
{
	struct tdu_fsm * fsm = eft->ts->fsm;

	return tdu_typed_data_req(fsm,msg);
}

struct tdu_user * eft_get_user(struct eft *eft)
{
	return eft->fsm->user;
}
int eft_msg2stdout(struct tdu_user * ts, struct tdu_param* par)
{
	par->par.udata->udata[par->par.udata->udata_len] = 0;
	printf( "EFT-Message: %s\n", par->par.udata->udata);
	tdu_printf(TDU_LOG_AP2, "MSG: message=\"%s\"\n", par->par.udata->udata);
	return 0;
}

int eft_remote_has_navigation(struct eft * eft)
{
	struct tdu_access_param * r_par = & eft->fsm->access.remote;

	if ( r_par->udata_len < 3 ) return 0;
	return r_par->udata[3] == 0x4f;
}

void  eft_stream_init_fd(struct eft * eft, int fd, int close)
{
	struct tdu_stream * ts = eft->ts;

	tdu_stream_init_fd(ts,fd,close);

	ts->hdr.data    = eft->fh;
	ts->hdr.read    = tdu_fh_get_fd_hdr;
#if 1
	ts->hdr.parse   = tdu_fh_parse;
#endif
}


void  eft_stream_set_stat(struct eft * eft, struct stat * s)
{
	eft->fh->fh_s = *s;
	eft->fh->fh_xdirrec = 0;
}


void  eft_stream_set_names(struct eft * eft, unsigned char * t_name,
			  unsigned char * name, int type)
{
	tdu_printf(TDU_LOG_TRC,"eft_stream_set_names(): class=%d\n",type);
	strcpy(eft->fh->fh_t_name, t_name);
	strcpy(eft->fh->fh_name, name);
	eft->fh->fh_type = type;
}

#if 0
void  eft_stream_set_length(struct eft * eft, int l)
{
	eft->fh->fh_s.st_size = l;
}
#endif

/*
 * associate a peer's phone number with the eft object
 */
void eft_add_peer_phone(struct eft *eft , unsigned char * str)
{
	strncpy(eft->isdn_no,str,MAX_ISDN_NO);
	eft->isdn_no[MAX_ISDN_NO]=0;
}

/*
 * retrun a string pointer to the peer's phone number
 */
unsigned char * eft_peer_phone(struct eft *eft)
{
	return eft->isdn_no;
}

void eft_set_flags(struct eft * eft, long flags)
{
	eft -> flags = flags;
}

long eft_get_flags(struct eft * eft)
{
	return eft -> flags;
}

void eft_set_xferlog(struct eft * eft, int fd)
{
	eft -> xferlog = fd;
	/* tdu_printf(TDU_LOG_TMP, "xferlog now %d\n",eft->xferlog); */
}

/*
 * copy the called address (as returned in called address parameter of
 * T-Associate response) with possible unprintable characters replaced
 * to a destinatiion array called[]. It must be at least of size
 * TDU_PLEN_ADDR+1.
*/
int eft_printable_called_addr(struct eft * eft, unsigned char *called)
{
	tdu_mk_printable(called,eft->fsm->assoc.remote.called_addr,
			 eft->fsm->assoc.remote.called_len);
		return eft->fsm->assoc.remote.called_len;
}

/*
 * copy the association user data (as returned in called address parameter
 * with T-Associate response) with possible unprintable character replaced
 * to a destinatiion array udata[]. It must be at least of size
 * TDU_PLEN_UDATA+1.
 * 
*/
int eft_printable_assoc_udata(struct eft * eft, 
					  unsigned char *udata)
{
	tdu_mk_printable(udata,eft->fsm->assoc.remote.udata,
			 eft->fsm->assoc.remote.udata_len);
		return eft->fsm->assoc.remote.udata_len;
}





