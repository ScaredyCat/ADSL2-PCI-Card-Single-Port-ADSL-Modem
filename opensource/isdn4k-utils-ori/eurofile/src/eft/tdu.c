/* $Id: tdu.c,v 1.2 2001/03/01 14:59:12 paul Exp $ */
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
 * A (rather incomplete) implementation of the T-protocol
 * defined in ETS 300 075. 
 *
 * This file containing common stuff.
 *
 */ 

#include <unistd.h>
#include <string.h>
#include <time.h>
#include "tdu_user.h"
#include "tdu.h"
#include "sbv.h"
#include "extra_version_.h"

const char e4l_version[] = E4L_VERSION;

long tdu_log_mask = 0;

/* Check if transfer regime is established */

static int in_transfer( struct tdu_fsm * fsm )
{
	return ( fsm -> regime_handler ) == & ( fsm->transfer.handler ) ;
}

/*
 * diconnect underlaying transport layer 
 */ 
void tdu_tc_disconnect(struct tdu_fsm * fsm)
{
}


/*
  event handler used when association is aborted or released
  */
int tdu_aborted( struct tdu_fsm *fsm, int event, struct tdu_buf *tb)
{
	if( event == TDU_CI_TC_DISCONNECT ) return 0;
	tdu_printf(TDU_LOG_ERR, "unexpected event %s received while association is released/aborted\n" 
	       , tdu_cmd_descr(event) );
	return 0;
}


void tdu_start_timer( struct tdu_fsm * fsm )
{
	fsm -> expires = time(0) + fsm -> assoc.resp_timeout;
}


void tdu_del_timer( struct tdu_fsm * fsm )
{
	fsm -> expires = 0;
}

/* 
 * returns the remaining time before the next application response timer
 * will expire.
 * Intentional side effect:   If a pending response timeout is detected,
 * an appropriate fsm event will be generated.
 */
static time_t check_timeout( struct tdu_fsm * fsm )
{
	time_t remaining;
	
	if( ! fsm -> expires ) return 0;
	
	remaining = difftime( fsm -> expires, time(0) )+1;
	if( remaining > 0 ) {
		return remaining;
	}
	
	/* if timer had expired we generate an application response timeout
	   event. When the event is processed the expire timer should be reset.
	   However, we are paranoid and restart the timer before generating
	   the event. */
	fsm -> expires = time(0) + fsm -> assoc.resp_timeout;
	tdu_printf( TDU_LOG_ERR, "generating application response timeout event\n");
	(*(fsm -> regime_handler)) ( fsm, TDU_CI_RESPONSE_TIMEOUT,
				     NULL );
	
	if( ! fsm -> expires ) return 0;
	return difftime( fsm -> expires, time(0) );
}

/*
 * return a timeout pointer suitable for select() that does not expire after
 * the currently valid application response timer 
 */
struct timeval * get_tdu_timeval( time_t remaining, 
				  struct timeval * max_tout, 
				  struct timeval * act_tout )
{	
	if( remaining <= 0 ) return max_tout;

	if( max_tout  &&  ( (max_tout -> tv_sec) < remaining ) )
		return max_tout;
		
	act_tout -> tv_sec  = remaining;
	act_tout -> tv_usec = 0;
	return act_tout;
}

/* 
 * return a string pointer describing the tdu command/parameter/... 
 */
char * tdu_des( struct tdu_descr *first, int id )
{
	struct tdu_descr *t=first;

	while( t->code != id ){ 
		if ( (++t)->descr == NULL ) break;
	};
	return t->descr;
}


static int tdu_try_sending( struct tdu_fsm * );

/*
 * Get a packet from the receive queue (which is just the socket read from)
 * and deliver it to the state machine's event handler.
 *
 * FIXME: the log level should allow finer grained control of logging
 * incoming packets (such as recognize TDU_LOG_REW)
 */
int tdu_dispatch_packet( struct tdu_fsm *fsm )
{
	struct tdu_buf tb[1];
	unsigned ci;
	int len, log_level=1, ct=TDU_LOG_IN, err=0;

	tdu_printf(TDU_LOG_TRC, "dispatch_packet()\n");
	len = tdu_recv_packet( tb, fsm);
	/* what should be really done with empty packets? */
	if( len == 0 ) err = TDU_CI_EMPTY;
	if( len < 0 ) err = -len;
	if( err ) {
		fsm->up = 0;
		tdu_printf(TDU_LOG_LOG, "dispatch_packet(): generating "
			   "error event %s\n", tdu_cmd_descr(err) );
		(*(fsm -> regime_handler)) ( fsm, err, NULL );
		return -1;
	}

	tb->pn = tb->tail;
	ci = *tb->data;
	tb->ci = tb->data;

	if( ci == TDU_CI_T_RESPONSE_NEGATIVE ||
	    ci == TDU_CI_T_P_EXCEPTION ||
	    ci == TDU_CI_T_TRANSFER_REJECT ||
	    ci == TDU_CI_T_ABORT                ){
		ct |= TDU_LOG_IER;
		log_level = 4;
	}

	tdu_printf( ct, "RECEIVED: "); 
log_level=4;
	tdu_print_cmd( ct, tb->ci, tb->tail, log_level);
	tb->data++;
	if( (len=tdu_parse_le(tb)) < 0 ) goto packet_too_small;
	/* remove trailing garbage */ 
	tb->p0 = tb->data;
	if( tb->p0+len < tb->tail ){
		tb->tail = tb->p0 + len; 
	}

	return (*(fsm -> regime_handler)) ( fsm, ci, tb );
	
	packet_too_small : tdu_printf( TDU_LOG_IER, "CMD packet to small!\n" );
	return -3;
}

/*
 * This should behave like the select() system call except that it might
 * additionally process T-protocol events before it returns. 
 * 
 * Timeout handling is currently incomplete, ugly and untested. In particular,
 * timeout parameter != NULL won't work.
 */
int tdu_select( struct tdu_fsm *fsm, int n, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout)
{
	int nmax = n > fsm->socket+1 ? n : fsm->socket+1;
	int selval, i;
	time_t remaining;
	fd_set rfds, wfds, efds;
	struct timeval *tout, act_tout;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);

	tdu_printf(TDU_LOG_TRC, "tdu_select()\n");
	while (fsm->up) {
		remaining = check_timeout( fsm );
		tdu_try_sending(fsm);
		if( ! fsm->up ) goto error;
		remaining = check_timeout( fsm );
		tout = get_tdu_timeval( remaining, timeout, &act_tout );
		if(readfds)  rfds = *readfds;
		if(writefds) wfds = *writefds;
		if(exceptfds)efds = *exceptfds;
		FD_SET( fsm->socket, &rfds );
		/* FD_SET( fsm->socket, &efds ); */
		tdu_printf(TDU_LOG_WAIT, "tdu_select() max wait=%ld\n", tout? tout->tv_sec:-1);
		selval = select( nmax, &rfds, &wfds, &efds, tout );
		tdu_printf(TDU_LOG_WAIT, "tdu_select(): select() returned %d\n",selval);
		for (i=0; i<nmax; i++) if(FD_ISSET(i,&rfds)) 
			tdu_printf(TDU_LOG_LOG, "%d ",i);
		if( selval < 0 ) return selval;
		if( selval == 0 ) {
			if( timeout && tout == timeout ) goto out;
		} else {
			if( FD_ISSET( fsm->socket, &rfds ) ) {
				if( tdu_dispatch_packet( fsm ) < 0 ) goto error; 
			} else {
				goto out;
			}
		}
	}
out:	
	if(readfds)  *readfds   = rfds;
	if(writefds) *writefds  = wfds;
	if(exceptfds)*exceptfds = efds;
	return selval;
error:
	return -2;
}

/*
 * This checks whether any mass transfer data is to be submitted. If
 * so, the data is submitted until the send window is full.
 */
static int tdu_try_sending( struct tdu_fsm * fsm )
{
	fd_set rfds, wfds;
	struct timeval no_wait = { 0, 0 };
	struct tdu_buf tb[1];
	int cnt, total=0, res, s1, s2, s3, s4, s5 , s6, eof=0;

	tdu_printf(TDU_LOG_TRC,"tdu_try_sending()\n");

	while (	! eof ) {
		tdu_init_tb( tb );
		FD_ZERO(&wfds);
		FD_SET( fsm->socket, &wfds );
		FD_ZERO(&rfds);
		FD_SET( fsm->stream->fd, &rfds );
		s1 = s2 = s3 = s4 = s5 = s6 = 0;
		res = (
		(s1 = in_transfer(fsm) ) && 
		(s2 = (fsm->transfer.last) < 0 ) &&
		(s3 = (fsm->transfer.handler == tdu_tr_sending ) ) &&
	        (s4 = (select(fsm->stream->fd+1,&rfds,0,0,&no_wait) > 0)) &&
		(s5 = (select(fsm->socket+1, 0, &wfds,0,&no_wait) > 0)) &&
		(s6 = (fsm->transfer.pkt_cnt  <= 
		       (fsm->transfer.confirmed	+ fsm->access.remote.window)))
		);
		if( ! res ){
			tdu_printf(TDU_LOG_DBG, "tdu_try_sending(): nothing to send %d %d %d %d %d %d\n",
				   s1,s2,s3,s4,s5,s6 );
			break;
		}
		tdu_printf(TDU_LOG_DBG, "tdu_try_sending: trying to read\n");
		cnt = fsm->stream->read( fsm->stream, tb );
		tdu_printf(TDU_LOG_DBG, "tdu_try_sending: %d bytes read\n",cnt);
		if( cnt < 0 ){
			tdu_transfer_rej_req(fsm,TDU_RE_OTHER_REASON,
					     "posix read() error");
			return cnt;
		}
		eof = tb->eof;
		tb->md = tb-> data;
		if( eof ){
			cnt = tdu_t_write_end_req(fsm, tb);
		} else {
			cnt = tdu_t_write_req(fsm, tb);
		}
		tdu_printf( TDU_LOG_LOG, "tdu_try_sending: %d bytes written\n",cnt);
		total += cnt;
	}
	return total;
}

/* 
 * process events until a generic condition is detected
 * Returns a positive integer on success;
 *
 * (* condition)() shall return 1 if the condition is reached, 0 otherwise.
 * However, if the condition is known to be never reachable, a negative
 * value shall be returned.
 */
int tdu_wait( struct tdu_fsm * fsm, struct timeval * timeout
	      , int (* condition)(struct tdu_fsm *) )
{
	int selval, ret;
	time_t remaining;
	fd_set rfds, efds;
	struct timeval * tout, act_tout;

	tdu_printf(TDU_LOG_TRC,"tdu_wait()\n");
	while (fsm->up) {
		if( (ret=condition( fsm )) ) return ret;
		tdu_try_sending( fsm ) ;
		remaining = check_timeout( fsm );
		if( ! fsm->up ) return -4;
		if( (ret=condition( fsm )) ) return ret;
		FD_ZERO(&rfds);
		FD_SET( fsm->socket, &rfds );
		FD_ZERO(&efds);
		FD_SET( fsm->socket, &efds );
		tout = get_tdu_timeval( remaining, timeout, &act_tout );
		tdu_printf(TDU_LOG_WAIT,"tdu_wait() max wait=%ld ... ",
			   tout? tout->tv_sec:-1);
		selval = select( fsm->socket+1, &rfds, NULL, &efds, tout );
		tdu_printf(TDU_LOG_WAIT, " select() exited\n");
		if( selval < 0 ) return -TDU_ERR_TIMEOUT;
		if( selval == 0 ) {
			if( tout == timeout ) {
				return -TDU_ERR_TIMEOUT;
			};
		} else {
			if( FD_ISSET( fsm->socket, &rfds ) )
				if( tdu_dispatch_packet(fsm) < 0 ) goto error;
			if( FD_ISSET( fsm->socket, &efds ) ){
				tdu_printf(TDU_LOG_ERR,"tdu_wait(): select() indicates exeception\n");
				goto error;
			}
		}
	}
	return 0;
error:
	return -4;
}

/*
 * check a positive or negative response for correctness.
 * tb->data must point to the parameter value of first pi.
 * (semantics of pi as returned from first tdu_get_next_pi())
 */
int tdu_check_response( struct tdu_buf * tb, int first_pi, int ci)
{
	int err = 0;

	if( first_pi >= 0) {
		if (first_pi != TDU_PI_RESULT_REASON) { 
			err = TDU_RE_SYNTAX_ERROR;
		} else if (*tb->data != ci) {
			err = TDU_RE_PROTOCOL_CONFLICT;
		}
	} else {
		err = -first_pi;
	}
	return err;
}


int tdu_abort(struct tdu_fsm * fsm)
{
	if( in_transfer( fsm ) )  fsm->stream->abort(fsm->stream);
	fsm->idle.handler = tdu_aborted;
	fsm->regime_handler = & fsm->idle.handler;
	/* FIXME: we should use the tc_connection state instead of the
	 * socket state for determining wether connection needs to be
	 * closed
	 * FIXME: we might want TC connection independent of socket API
	 */
#if 0
	if( fsm->socket >= 0 && close( fsm->socket ) )
		perror("tdu_abort():close()");
#else
	if( fsm->socket >= 0 ) tdu_sbv_disconnect(fsm->socket);
#endif
	fsm->up = 0;
	fsm->socket = -1;
	return 0;
}


static int tdu_send_abort(struct tdu_fsm * fsm, int reason, unsigned char * other_reason)
{
	struct tdu_buf tb[1];

	tdu_init_tb( tb );
	tdu_add_reason( tb, 0, reason, other_reason );
	tdu_add_ci_header( tb, TDU_CI_T_ABORT );
	return tdu_send_packet( tb, fsm );
}


int tdu_abort_req(struct tdu_fsm * fsm, int reason, unsigned char * other)
{
	tdu_send_abort( fsm, reason, other );
#if 0
	tdu_del_timer( fsm );
#else
/* for tests */
	fsm -> expires = time(0) + 5;
#endif
	tdu_abort(fsm);
	return 0;
}


int tdu_send_response_pos( struct tdu_fsm * fsm, int ci)
{
	struct tdu_buf tb[1];

	tdu_init_tb( tb );
	tdu_add_reason( tb, ci, 0, NULL );
	tdu_add_ci_header( tb, TDU_CI_T_RESPONSE_POSITIVE );
	return tdu_send_packet( tb, fsm );
}


int tdu_send_response_neg( struct tdu_fsm * fsm, int ci,
			   int reason, unsigned char * other)
{
	struct tdu_buf tb[1];

	tdu_init_tb( tb );
	tdu_add_reason( tb, ci, reason, other );
	tdu_add_ci_header( tb, TDU_CI_T_RESPONSE_NEGATIVE );
	return tdu_send_packet( tb, fsm );
}

/*
 * idle regime handler, waiting for any event during the initial phase 
 */
static int initial_idle_handler(  struct tdu_fsm *fsm, int event,
				  struct tdu_buf *tb)
{
	int ret = 0;

	tdu_printf(TDU_LOG_TRC,"initial_idle_handler(), event = %s\n",
	       tdu_cmd_descr(event) ); 

	switch (event) {
	case TDU_CI_T_ASSOCIATE:
		ret = fsm->assoc.idle_assoc_req_processor(fsm,tb);
		break;
	case TDU_CI_T_ABORT:
		tdu_abort(fsm);
		break;
	default:
		tdu_printf(TDU_LOG_ERR, "unexpected event %s while waiting for t_associate" 
		       " request\n", tdu_cmd_descr(event) );
		tdu_abort_req(fsm,TDU_RE_PROTOCOL_CONFLICT,NULL);
	};
	return ret;
}

/*
 * Set initial idle state of fsm
 */
void tdu_initial_set_idle(struct tdu_fsm * fsm)
{
		fsm -> idle.handler = initial_idle_handler;
		fsm -> regime_handler = & fsm -> idle.handler;
		fsm->up = 1;
		tdu_del_timer(fsm);
}

/*
 * Check whether no regime establishment was initiated yet
 */
int tdu_before_regime( struct tdu_fsm * fsm )
{
	return 	(    fsm->regime_handler == & fsm->idle.handler ) 
		&& ( fsm->idle.handler == initial_idle_handler );
}
