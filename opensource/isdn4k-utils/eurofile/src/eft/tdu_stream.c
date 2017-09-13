/* $Id: tdu_stream.c,v 1.1 1999/06/30 17:18:46 he Exp $ */
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
/*
  A (rather incomplete) implementation of the T-protocol
  defined in ETS 300 075. 

  This file containing stuff for processing tdu streams ( = object
  serving as source or target of mass transfer data).
*/ 

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <tdu_user.h>
#include "tdu.h"
#include "fileheader.h"

/*
 * FIXME: if anything failed in any of read/write/close routines, an
 * appropriate error code must be passed to the caller somehow such
 * that the calling protocol layer can generate the proper error reply.
 */

int tdu_no_close (struct tdu_stream *ts)
{
	tdu_printf( TDU_LOG_TRC, "calling tdu_no_close\n");
	return 0;
}


int tdu_fd_close (struct tdu_stream *ts)
{
	tdu_printf( TDU_LOG_TRC, "calling tdu_fd_close\n");
	/* FIXME: somehow hook xfer log record here*/
	/* if( ts->fsm->user->priv){;}; */
	return  close( ts->fd );
}


int tdu_std_abort (struct tdu_stream *ts)
{
	tdu_printf( TDU_LOG_TRC, "tdu_std_abort() called\n");
	ts->close(ts);
	return 0;
}


int tdu_fd_t_write (struct tdu_stream *ts, struct tdu_buf * tb)
{
	int cnt=0, hlen=0; 
	tdu_printf( TDU_LOG_TRC, "calling tdu_fd_t_write len=%d\n",
		    tb->tail - tb->data);

	if ( ts->hdr.parse ){ 
		hlen = ts->hdr.parse(ts, tb, tb->tail - tb->data);
		if( hlen < 0 ) return -1;
	}

	cnt = write( ts->fd, tb->data, tb->tail - tb->data );
	if ( cnt < 0 ) {
		perror( "tdu_fd_t_write() error" );
		switch(errno){
		case ENOSPC:
			cnt = -TDU_RE_OTHER_REASON;
			break;
		}
		return cnt;
	}
	return cnt+hlen;
}


int tdu_fd_t_write_end (struct tdu_stream *ts, struct tdu_buf * tb)
{
	int cnt;
	struct transfer_regime * tr = & ts->fsm->transfer;
	tdu_printf( TDU_LOG_TRC, "calling tdu_fd_t_write_end len=%d\n",
		    tb->tail-tb->data);
	cnt = tdu_fd_t_write( ts, tb );
	gettimeofday( & tr->tv_end, NULL );
	if ( cnt < 0 ) perror( "tdu_fd_t_write_end() error" );
	return cnt;
}


int tdu_fd_read (struct tdu_stream *ts, struct tdu_buf * tb)
{
	int cnt, maxlen = tb->end - tb->md, hlen=0;
	
	tb->data = tb->md;
	if( maxlen > 1000 ) maxlen = 1000;
	tb->eof = 0;
	tdu_printf( TDU_LOG_TRC, "calling tdu_fd_read\n");

	/* FIXME: what about headers not fitting on the first packet? */
	if ( ts->hdr.read ){ 
		hlen = ts->hdr.read(ts,tb->data,maxlen);
		if( hlen < 0 ) return hlen;
		tb->data += hlen;
	}

	maxlen -= hlen;
	cnt = read( ts->fd, tb->data, maxlen );
	/* FIXME: this simple eof check might fail, in particular when
	   reading from a pipe */
	if ( cnt < maxlen ) tb->eof = 1;
	if( cnt < 0 ){
		perror( "tdu_fd_read()" );
		return cnt;
	} else {
		tb->data = tb->md;
		tb->tail = tb->data + cnt + hlen;
		return cnt+hlen;
	} 
}

/*
 * Inititialize a tdu_stream object. Needs to be called before each transfer.
 */
void tdu_stream_init_fd(struct tdu_stream * ts, int fd, int close)
{
	ts->fd = fd;
	ts->t_write     = tdu_fd_t_write;
	ts->t_write_end = tdu_fd_t_write_end;
	ts->read        = tdu_fd_read;
	if(close) {
		ts->close   = tdu_fd_close;
	} else {
		ts->close   = tdu_no_close;
	}
	ts->abort       = tdu_std_abort;
	ts->hdr.read    = tdu_fh_get_zero_hdr;
	ts->hdr.parse   = tdu_fh_parse_print;
	ts->hdr.len     = 0;
}


