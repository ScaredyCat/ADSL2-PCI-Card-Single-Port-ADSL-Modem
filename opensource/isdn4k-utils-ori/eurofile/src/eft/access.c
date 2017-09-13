/* $Id: access.c,v 1.4 2001/03/01 14:59:12 paul Exp $ */
/* Copyright 1997 by Henner Eisen

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
#include <string.h>
#include <tdu_user.h>
#include "tdu.h"

/*
 * access regime event handler used during transfer regime by receiver
 * while loading (reading) data from peer.
 */
int tdu_access_loading( struct tdu_fsm *fsm, int event, struct tdu_buf * tb)
{

	tdu_printf( TDU_LOG_TRC, " tdu_access_loading(): unexpected event %s received"
		" while transfer regime is established\n",
		tdu_cmd_descr(event) );
	switch (event) {
	case TDU_CI_T_ABORT:
		fsm->assoc.handler (fsm, event, tb);
		break;
	case TDU_CI_RESPONSE_TIMEOUT:
		tdu_p_except_req( fsm,TDU_RE_DELAY_EXPIRED);
		break;
	case TDU_CI_T_P_EXCEPTION:
	default:
		tdu_printf( TDU_LOG_ERR, "tdu_access_loading(): unexpected command received by receiver\n" );
		tdu_transfer_abort(fsm);
	};
	return 0;
}

/*
 * access regime event handler used during transfer regime by sender 
 * while saving (writing) data to peer. 
 */
int tdu_access_saving( struct tdu_fsm *fsm, int event, struct tdu_buf * tb)
{

	tdu_printf( TDU_LOG_TRC, " tdu_access_saving(): unexpected event %s received"
		" while transfer regime is established\n",
		tdu_cmd_descr(event) );
	switch (event) {
	case TDU_CI_T_ABORT:
		fsm->assoc.handler (fsm, event, tb);
		break;
	case TDU_CI_RESPONSE_TIMEOUT:
		tdu_p_except_req( fsm,TDU_RE_DELAY_EXPIRED);
		break;
	case TDU_CI_T_P_EXCEPTION:
	default:
		tdu_printf( TDU_LOG_ERR, "tdu_access_saving(): unexpected command received by sender\n" );
		tdu_transfer_abort(fsm);
	};
	return 0;
}

int tdu_p_except_req( struct tdu_fsm * fsm, int reason )
{
	tdu_send_p_exception(fsm,reason);
	if( fsm->regime_handler == & fsm->transfer.handler )
		tdu_transfer_abort(fsm);
	tdu_access_set_idle(fsm);
	return reason>0 ? reason : -1;
}

/* 
 * Check if in idle state of access regime. returns 1 if in idle state,
 * 0 if not, but a negative value if the idle state is known never to be
 * reachable without user intervention.
 */
static int in_idle_access_regime( struct tdu_fsm * fsm )
{
	int ret;

	/* somewhat ugly, but we need this */
        if ( (fsm -> regime_handler == & fsm->access.handler) &&
	     (fsm -> access.handler == tdu_await_end_access)     ) return -2;

        ret =  ( fsm -> regime_handler == & fsm->access.handler )
		&& ( fsm->access.handler == fsm->access.idle_handler );

	if( ret ) return ret;
/* From the following states, the idle access regime cannot be reached
 * without additional local user intervention*/
	if (  fsm->regime_handler == &fsm->idle.handler ) return -3;
	if ( (fsm->regime_handler == &fsm->assoc.handler ) &&
	     (fsm->assoc.handler == fsm->assoc.master_handler)) return -2;
	return 0;
}

/* 
 * process events until the access regime's idle state is reached again.
 */
int tdu_wait_for_idle(struct tdu_fsm * fsm, struct timeval * timeout)
{
	return tdu_wait( fsm, timeout, in_idle_access_regime );
}


void tdu_access_set_idle(struct tdu_fsm * fsm)
{
		fsm -> access.handler = fsm->access.idle_handler;
		fsm -> regime_handler = & fsm->access.handler;
		tdu_del_timer(fsm);
		fsm->wait=0;
}

/*
 * access regime handler, waiting for confirmation of t_end_access 
 */
int tdu_await_end_access( struct tdu_fsm *fsm, int event, struct tdu_buf *tb)
{
	int err, pi;

	tdu_printf(TDU_LOG_TRC, "tdu_await_end_access(), event = %s\n",
			tdu_cmd_descr(event) ); 
	switch (event) {
	case TDU_CI_T_RESPONSE_POSITIVE:
		pi = tdu_get_next_pi(tb);
		err = tdu_check_response(tb,pi,fsm->wait);
		if( err ) goto error;
		fsm->assoc.handler = tdu_assoc_master;
		/* now fall through */
	case TDU_CI_T_END_ACCESS:
		/* if t_end_acces_req conflict, no change of 
		   role ./. association event handler handler */
		fsm -> regime_handler = & fsm->assoc.handler ;
		tdu_del_timer(fsm);
		break;
	case TDU_CI_RESPONSE_TIMEOUT:
		tdu_printf(TDU_LOG_ERR, "timeout while waiting for t_end_access confirmation\n");
		tdu_abort_req(fsm,TDU_RE_DELAY_EXPIRED,NULL);
		break;
	case TDU_CI_T_ABORT:
	default:
	error:
		tdu_printf(TDU_LOG_ERR, "unexpected event %s while waiting for t_end_access confirmation" 
		       " confirmation\n", tdu_cmd_descr(event) );
		tdu_abort_req(fsm,TDU_RE_PROTOCOL_CONFLICT,NULL);
	};
	fsm->wait = 0;
	return 0;
}

/*
 * Request termination of access regime.
 * Specifying user data and non-default reason parameter not yet supported.
 */
int tdu_end_access_req(struct tdu_fsm * fsm)
{
	int ret;
	struct tdu_buf tb[1];

	tdu_init_tb(tb);

	tdu_printf(TDU_LOG_TRC, "tdu_end_access_req(),\n" ); 

	ret = in_idle_access_regime( fsm );

	if( ret == 0 ) {
		tdu_printf( TDU_LOG_ERR, "tdu_end_access_req: not in idle state of "
			"access regime\n");
		return -1;
	}

	if( ret < 0 ) return ret;

	tdu_add_reason( tb, 0, TDU_RE_OTHER_REASON, "%");
	tdu_add_ci_header( tb, TDU_CI_T_END_ACCESS);
	ret = tdu_send_packet(tb,fsm);
	if ( ret < 0 ) { 
		perror("tdu_end_access_req:send_packet");
		tdu_abort(fsm);
		return ret;
	}

	fsm->access.handler = tdu_await_end_access;
	tdu_start_timer( fsm );
	fsm->wait = TDU_CI_T_END_ACCESS;
	return ret;
}       

/*
 * Fill in parameters in a t_access request packet
 */
int tdu_add_access_params( struct tdu_buf * tb, struct tdu_access_param * par)
{
	unsigned char byte = 0;
	int ret=0;
	
	if( par->udata && (par->udata_len >= 0) )
		ret += tdu_add_string_par(tb, TDU_PI_USER_DATA, par->udata
					  , LIMIT(par->udata_len,TDU_PLEN_UDATA) );

	ret += tdu_add_byte_par(tb,TDU_PI_TRANSFER_MODE,
				par->transfer_mode);

	byte = par->recovery ? 1 : 0;
	byte |= ( (  (par->transfer_size) >> 9 ) & 0x0e );
	byte |= ( (  (par->window - 1 )   << 5 ) & 0xe0 );
	ret += tdu_add_byte_par(tb,TDU_PI_SIZE_RECOVERY_WINDOW,byte);

	byte = par->role? TDU_ROLE_MASTER : TDU_ROLE_SLAVE;
	byte |= (par->functions) & 0xfe; /*  functions supported by us */
	ret += tdu_add_byte_par(tb,TDU_PI_ROLE_FUNCTION,byte);

	return ret;
}


int tdu_send_access( struct tdu_fsm * fsm, int resp ) 
{
	struct tdu_buf tb[1];

	tdu_printf(TDU_LOG_TRC, "tdu_send_access()\n");

	tdu_init_tb(tb);
	tdu_add_access_params( tb, & fsm->access.local ); 
	if( resp ) { 
		/*FIXME: the Role should be removed*/
		tdu_add_reason(tb, TDU_CI_T_ACCESS, 0, NULL);
		tdu_add_ci_header( tb, TDU_CI_T_RESPONSE_POSITIVE );
	} else {
		tdu_add_ci_header( tb, TDU_CI_T_ACCESS );
	}
	return tdu_send_packet( tb, fsm );
}

/*
 * Parse parameters of an access_request or positive response message.
 * returns:
 *   0 on success 
 *   an integer reason code from the TDU_RE_* set when an error is detected.
 */
int tdu_parse_access( struct tdu_access_param *par, struct tdu_buf *tb)
{
	int pi, pv;

	tdu_printf(TDU_LOG_TRC, "tdu_parse_access()\n");
	do {
		tdu_printf(TDU_LOG_DBG, "parsing parameter %d=%s\n",*tb->data,
			   tdu_param_descr(*tb->data) );
		pi = tdu_get_next_pi(tb);
		if( pi < 0 ) return -pi;
		switch( pi ){
		case TDU_PI_RESULT_REASON:
			tdu_printf(TDU_LOG_DBG, "result/reason\n");
			if( *tb->ci == TDU_CI_T_RESPONSE_POSITIVE ){
				if( (pv=tdu_parse_byte(tb)) < 0 ) 
					return TDU_RE_SYNTAX_ERROR;
				if( pv != TDU_CI_T_ACCESS )
					return TDU_RE_PROTOCOL_CONFLICT;
			};
			tb->data = tb->pn;
			break;
		case TDU_PI_ROLE_FUNCTION:
			if( (pv=tdu_parse_byte(tb)) < 0 ){
				if( tb->data + 1 == tb->pn ){
					tb->data++;
				} else {
					return TDU_RE_SYNTAX_ERROR;
				}
			}
			par->role = pv & TDU_BIT_ROLE;
			par->functions = pv & (~TDU_BIT_ROLE);
			break;
		case TDU_PI_SIZE_RECOVERY_WINDOW:
			tdu_printf(TDU_LOG_DBG, "size/recov/win\n");
			if( (pv=tdu_parse_byte(tb)) < 0 )
					return TDU_RE_SYNTAX_ERROR;
			par->transfer_size = 512 << ((pv & TDU_MASK_SIZE) >>1);
			par->window = 1 + ((pv & TDU_MASK_WINDOW) >>5);
			par->recovery = TDU_MASK_RECOVERY;
			tdu_printf(TDU_LOG_DBG,"peer's window size is %d\n",
				   par->window);
			break;
		case TDU_PI_TRANSFER_MODE:
			tdu_printf(TDU_LOG_DBG, "transfer mode\n");
			if( (pv=tdu_parse_byte(tb)) < 0 ) 
				return TDU_RE_SYNTAX_ERROR;
			par->transfer_mode = pv & TDU_BIT_TRANSFER_MODE;
			break;
		case TDU_PI_USER_DATA:
			tdu_printf(TDU_LOG_DBG, "user data\n");
			par->udata_len =
				tdu_parse_string(tb, par->udata,
						 TDU_PLEN_UDATA);
			break;
		default:
			tdu_printf(TDU_LOG_LOG, "tdu_parse_access(): unexpected"
				   " parameter '%s' ignored\n",
				   tdu_param_descr(pi) );
			tb->data = tb->pn;
		};
	} while( tb->pn < tb->tail );
	return 0;
}

/*
 * access regime handler, waiting for t_access confirmation 
 */
int tdu_await_access( struct tdu_fsm *fsm, int event, struct tdu_buf *tb)
{
	int err, pi;

	tdu_printf(TDU_LOG_TRC, "tdu_await_access(), event = %s\n",
	       tdu_cmd_descr(event) ); 
	switch (event) {
	case TDU_CI_T_RESPONSE_POSITIVE:
		tdu_del_timer(fsm);
		err = tdu_parse_access( & fsm->access.remote, tb );
		if( err ) {
			tdu_abort_req( fsm, err, NULL );
		} else {
			if( fsm->access.local.role == TDU_ROLE_SLAVE ) {
				/* requested new role is slave */
				fsm->assoc.handler 
					= fsm->assoc.slave_handler;
				fsm->access.idle_handler 
					= fsm->access.idle_slave_handler;
			} else {
				/* requested new role is master */
				fsm->assoc.handler 
					= fsm->assoc.master_handler;
				fsm->access.idle_handler 
					= fsm->access.idle_master_handler;
			}
			tdu_access_set_idle(fsm);
		}
		break;
	case TDU_CI_T_RESPONSE_NEGATIVE:
		pi = tdu_get_next_pi(tb);
		err = tdu_check_response(tb,pi,fsm->wait);
		if( err ) goto error;
		tdu_del_timer(fsm);
		fsm->assoc.handler = fsm->assoc.master_handler;
		break;
	case TDU_CI_T_ABORT:
		tdu_abort(fsm);
		break;
	default:
	error:
		tdu_printf(TDU_LOG_ERR,"unexpected event %s while waiting for t_access" 
		       " confirmation\n", tdu_cmd_descr(event) );
		tdu_abort_req(fsm,TDU_RE_PROTOCOL_CONFLICT,NULL);
	};
	fsm->wait = 0;
	return 0;
}

/*
 * establish access regime
 */
int tdu_access_req( struct tdu_fsm * fsm, struct tdu_access_param * par )
{

	if ( ! in_idle_assoc_regime_master( fsm ) ){
		tdu_printf(TDU_LOG_TRC, "tdu_access_req(): no idle association master state\n");
		return -1;
	}
	fsm->access.local = *par;
	/* FIXME: contents of *par needs checking */
	
	if( tdu_send_access( fsm, 0 ) > 0 ){
		fsm->assoc.handler = tdu_await_access;
		tdu_start_timer( fsm );
		fsm->wait = TDU_CI_T_ACCESS;
		return 1;
	}
	return 0;
}


void tdu_set_default_access_param( struct tdu_access_param * par, struct tdu_user *usr, struct tdu_stream * st )
{
	par->functions = 0;
	if( usr ){
		if( usr->t_typed_data ) par->functions |= TDU_BIT_TYPED_DATA;
		if( usr->t_dir )        par->functions |= TDU_BIT_DIRECTORY;
		if( usr->t_load)        par->functions |= TDU_BIT_LOAD;
		if( usr->t_save)        par->functions |= TDU_BIT_SAVE;
		if( usr->t_delete)      par->functions |= TDU_BIT_DELETE;
		if( usr->t_rename)      par->functions |= TDU_BIT_RENAME;
	}
	if( st && st->t_read_restart ){
		par->functions |= TDU_BIT_READ_RESTART;
	}
	par->transfer_size = 1024;
	par->window = 1;
	par->recovery = 0;
	par->transfer_mode = 0;
	par->udata_len = -1;
}

/* 
 * process events until the association regime's idle master state is reached.
 */
int tdu_wait_for_end_access(struct tdu_fsm * fsm, struct timeval * timeout)
{
	tdu_printf(TDU_LOG_TRC, "tdu_wait_for_end_access()\n");

	return tdu_wait( fsm, timeout, in_idle_assoc_regime_master );
}


/* 
 * perform the work requierd by a t_end_access request
 */
int tdu_end_access_received(struct tdu_fsm *fsm)
{
	tdu_printf(TDU_LOG_TRC, "tdu_end_access_received()\n");
	if( fsm->assoc.slave_handler ){
		tdu_send_response_pos(fsm,TDU_CI_T_END_ACCESS);
		fsm->assoc.handler = fsm->assoc.slave_handler;
		fsm->regime_handler = & fsm -> assoc.handler;
	} else {
		/* FIXME? is this the proper error code? */
		tdu_abort_req( fsm, TDU_RE_ROLE_REFUSED_U, NULL ); 
	}
	return 0;
}


void tdu_access_set_master(struct tdu_fsm * fsm)
{
	fsm->access.idle_handler = fsm->access.idle_master_handler;
}


void tdu_access_set_slave(struct tdu_fsm * fsm)
{
	fsm->access.idle_handler = fsm->access.idle_slave_handler;
}

/* 
 * perform the work required upon reception of a t_access_req tdu
 */
int tdu_access_received(struct tdu_fsm *fsm, struct tdu_buf * tb)
{
	struct tdu_param par;

	tdu_printf(TDU_LOG_TRC, "tdu_access_received()\n");
	par.reason = TDU_RE_ROLE_REFUSED_U;
	par.other_reason[0] = 0;

	tdu_set_default_access_param( &fsm->access.remote, 0, 0);
	tdu_set_default_access_param( &fsm->access.local, fsm->user,
				      fsm->stream);

	par.reason = tdu_parse_access(&fsm->access.remote,tb);
	if ( par.reason ) goto refuse;

	if( fsm->access.remote.role == TDU_ROLE_SLAVE ){
		fsm->access.local.role = TDU_ROLE_MASTER;
	} else {
		fsm->access.local.role = TDU_ROLE_SLAVE;
	}

	if( fsm->user->t_access){
		par.par.access = &fsm->access.remote;
		par.res.access = &fsm->access.local;
		par.reason = fsm->user->t_access(fsm->user,&par);
		if( par.reason ) goto refuse;
	}

	if( (! fsm->access.idle_slave_handler) && 
	    (fsm->access.local.role == TDU_ROLE_SLAVE)  )  goto refuse;
	if( (! fsm->access.idle_master_handler) && 
	    (fsm->access.local.role == TDU_ROLE_MASTER) )  goto refuse;

	if( tdu_send_access(fsm, 1) > 0 ){
		if (fsm->access.local.role == TDU_ROLE_SLAVE){
			tdu_access_set_slave(fsm);
		} else {
			tdu_access_set_master(fsm);
		}
		tdu_access_set_idle(fsm);
		return 0;
	} else {
		tdu_abort(fsm);
		return -3;
	}

refuse:
	tdu_send_response_neg(fsm,TDU_CI_T_ACCESS,
			      par.reason,par.other_reason);
	/* FIXME: is aborting here really appropriate ? */
	tdu_abort(fsm);
	return -3;
}


/*
 * Parse udata parameter of an arbitrary tdu.
 * returns:
 *   0 on success 
 *   an integer reason code from the TDU_RE_* set when an error is detected.
 */
int tdu_parse_udata( struct tdu_udata_param *par, struct tdu_buf *tb)
{
	int pi;

	tdu_printf(TDU_LOG_TRC, "tdu_parse_udata()\n");
	do {
		tdu_printf(TDU_LOG_DBG, "parsing parameter %d=%s\n",*tb->data,
			   tdu_param_descr(*tb->data) );
		pi = tdu_get_next_pi(tb);
		if( pi < 0 ) return -pi;
		switch( pi ){
		case TDU_PI_USER_DATA:
			tdu_printf(TDU_LOG_DBG, "user data\n");
			par->udata_len =
				tdu_parse_string(tb, par->udata,
						 TDU_PLEN_UDATA);
			break;
		default:
			tdu_printf(TDU_LOG_LOG, "tdu_parse_udata(): unexpected"
				   " parameter '%s' ignored\n",
				   tdu_param_descr(pi) );
			tb->data = tb->pn;
		};
	} while( tb->pn < tb->tail );
	return 0;
}


int tdu_typed_data_received(struct tdu_fsm * fsm, struct tdu_buf * tb)
{
	struct tdu_param param;
	struct tdu_udata_param udata;

	tdu_parse_udata(&udata,tb);
	param.par.udata = &udata;

	if( (fsm->access.local.functions & TDU_BIT_TYPED_DATA) &&
 	    fsm->user->t_typed_data )
		fsm->user->t_typed_data(fsm->user, &param);		
	return( param.reason );
}

int tdu_typed_data_req( struct tdu_fsm * fsm, unsigned char * msg){
	struct tdu_buf tb[1];

	tdu_init_tb(tb);
	tdu_printf(TDU_LOG_TRC,"typed data request\n");
	if( fsm->access.remote.functions & TDU_BIT_TYPED_DATA ){
		tdu_add_string_par(tb,TDU_PI_USER_DATA, msg,
				   LIMIT(strlen(msg),254));
		tdu_add_ci_header(tb,TDU_CI_T_TYPED_DATA);
		tdu_send_packet(tb,fsm);
		return 0;
	} else {
		return TDU_RE_PRIMITIVE_NOT_HANDLED;
	}
}


/*
 * send a t_p_execption report tdu to peer 
 */
int tdu_send_p_exception(struct tdu_fsm *fsm, int reason)
{
	struct tdu_buf tb[1];

	tdu_printf(TDU_LOG_TRC, "tdu_send_p_execption()\n");

	tdu_init_tb(tb);

	tdu_add_reason( tb, 0, reason, 0);
	tdu_add_ci_header( tb, TDU_CI_T_P_EXCEPTION);
	return tdu_send_packet(tb,fsm);
}

/*
 * check a generic positive or negative response for correctness
 * and requests a t_p_execption if errors are detected.
 * (semantics of pi as returned from first tdu_get_next_pi())
 */
int tdu_generic_response_received (struct tdu_fsm *fsm, struct tdu_buf * tb)
{
	int pi, err=0;

	tdu_printf(TDU_LOG_TRC, "tdu_generic_response_received");
	
	pi = tdu_get_next_pi(tb);
	err = tdu_check_response(tb,pi,fsm->wait);
	if( err ) {
		/* tb->data points to the parameter value of first pi */
		tdu_printf(TDU_LOG_ERR
			   , "Bad response %s(%s) does not match request %s!\n"
			   , tdu_cmd_descr(*tb->ci)
			   , tdu_cmd_descr(*tb->data)
			   , tdu_cmd_descr(fsm->wait)
			   );
		err = tdu_p_except_req(fsm,err);
	}
	return err;
}



