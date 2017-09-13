/* $Id: assoc.c,v 1.3 1999/10/06 18:16:22 he Exp $ */
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
#include <string.h>
#include <tdu_user.h>
#include "tdu.h"


tdu_handler tdu_released = tdu_aborted;

/*
 * Check if association is deactivated ( released or aborted )
 */
static int tdu_not_associated( struct tdu_fsm * fsm )
{
	return ( * (fsm -> regime_handler) == fsm->idle.handler ); 
}

/*
 * Check if in idle state of association regime as slave
 */
static int in_idle_assoc_regime_slave( struct tdu_fsm * fsm )
{
	return ( 
		( (fsm -> regime_handler) == &( fsm->assoc.handler ) )
		&& 
		( fsm->assoc.handler == fsm->assoc.slave_handler )
		);
}

/*
 * Check if in idle state of association regime as master
 */
int in_idle_assoc_regime_master( struct tdu_fsm * fsm )
{
	return (
		( (fsm -> regime_handler) == &( fsm->assoc.handler ) )
		&& 
		( fsm->assoc.handler == fsm->assoc.master_handler )
		);
}

/* 
 * process events until the association regime ceases to be established
 */
int tdu_wait_for_release(struct tdu_fsm * fsm, struct timeval * timeout)
{
	tdu_printf(TDU_LOG_TRC, "tdu_wait_for_release()\n");
	return tdu_wait( fsm, timeout, tdu_not_associated );
}

/*
 * Fill in parameters in a t_associate request packet
 */
int tdu_add_assoc_params( struct tdu_buf * tb, struct tdu_assoc_param * par)
{
	int ret=0;
	unsigned char byte;

	if( par->udata && (par->udata_len >= 0) )
		ret += tdu_add_string_par(tb, TDU_PI_USER_DATA, par->udata
					  , LIMIT(par->udata_len,TDU_PLEN_UDATA) );
	if( par->req_ident >= 0){
		ret += tdu_add_byte_par(tb,TDU_PI_REQUEST_IDENT,
					par->req_ident? TDU_BIT_REQ_ID : 0);
	}	

	if( par->ident && (par->ident_len >= 0) )
		ret += tdu_add_string_par(tb, TDU_PI_IDENTIFICATION, par->ident
					  , LIMIT(par->ident_len,TDU_PLEN_IDENT) );
	if( par->expl_conf >= 0 ){
		ret += tdu_add_byte_par(tb,TDU_PI_EX_CONF_1ST_LAST_BLNO,
					par->expl_conf? TDU_BIT_EXPL_CONF : 0);
	}

	byte = 0;
	if( par->symm_service ) byte|= TDU_BIT_SYMM_SERVICE;
	if( par->basic_kernel ) byte|= TDU_BIT_BASIC_KERNEL;

	ret += tdu_add_byte_par(tb,TDU_PI_SERVICE_CLASS,
				byte);

	ret += tdu_add_byte_par(tb,TDU_PI_APPL_RESPONSE_TIMEOUT,
				12);
	
	if( par->appl_name && (par->appl_len >= 0) )
		ret += tdu_add_string_par(tb, TDU_PI_APPLICATION_NAME,
					  par->appl_name
					  , LIMIT(par->appl_len,TDU_PLEN_APPLNAME) );
	if( par->calling_addr && (par->calling_len >= 0) )
		ret += tdu_add_string_par(tb, TDU_PI_CALLING_ADDRESS
					  , par->calling_addr
					  , LIMIT(par->calling_len,TDU_PLEN_ADDR) );	
	if( par->called_addr && (par->called_len >= 0) )
		ret += tdu_add_string_par(tb, TDU_PI_CALLED_ADDRESS
					  , par->called_addr
					  , LIMIT(par->called_len,TDU_PLEN_ADDR) );

	return ret;
}

/*
 * Parse parameters of an association_request or positive response message.
 * returns:
 *   0 on success 
 *   an integer reason code from the TDU_RE_* set when an error is detected.
 */
int tdu_parse_assoc( struct tdu_assoc_param *par, struct tdu_buf *tb)
{
	int pi, pv;

	tdu_printf(TDU_LOG_TRC, "tdu_parse_assoc()\n");
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
				if( pv != TDU_CI_T_ASSOCIATE )
					return TDU_RE_PROTOCOL_CONFLICT;
			};
			tb->data = tb->pn;
			break;
		case TDU_PI_CALLED_ADDRESS:
			tdu_printf(TDU_LOG_DBG, "called addr\n");
			par->called_len = 
				tdu_parse_string(tb, par->called_addr,
						 TDU_PLEN_ADDR);
			break;
		case TDU_PI_CALLING_ADDRESS:
			tdu_printf(TDU_LOG_DBG, "calling addr\n");
			par->calling_len = 
				tdu_parse_string(tb, par->calling_addr,
						 TDU_PLEN_ADDR);
			break;
		case TDU_PI_APPLICATION_NAME:
			tdu_printf(TDU_LOG_DBG, "appl name\n");
			par->appl_len = 
				tdu_parse_string(tb, par->appl_name,
						 TDU_PLEN_APPLNAME);
			break;
		case TDU_PI_APPL_RESPONSE_TIMEOUT:
			tdu_printf(TDU_LOG_DBG, "resp timout\n");
			par->resp_timeout = tdu_parse_li(tb);
			break;
		case TDU_PI_SERVICE_CLASS:
			tdu_printf(TDU_LOG_DBG, "svc class\n");
			if( (pv=tdu_parse_byte(tb)) < 0 ) 
				return TDU_RE_SYNTAX_ERROR;
			par->symm_service = pv & TDU_BIT_SYMM_SERVICE;
			par->basic_kernel = pv & TDU_BIT_BASIC_KERNEL;
			break;
		case TDU_PI_EX_CONF_1ST_LAST_BLNO:
			tdu_printf(TDU_LOG_DBG, "expl conf\n");
			if( (pv=tdu_parse_byte(tb)) < 0 ) 
				return TDU_RE_SYNTAX_ERROR;
			par->expl_conf = pv & TDU_BIT_EXPL_CONF;
			break;
		case TDU_PI_IDENTIFICATION:
			tdu_printf(TDU_LOG_DBG, "ident\n");
			par->ident_len = 
				tdu_parse_string(tb, par->ident,
						 TDU_PLEN_IDENT);
			break;
		case TDU_PI_REQUEST_IDENT:
			tdu_printf(TDU_LOG_DBG, "req ident\n");
			if( (pv=tdu_parse_byte(tb)) < 0 ) 
				return TDU_RE_SYNTAX_ERROR;
			par->req_ident = pv & TDU_BIT_REQ_ID;
			break;
		case TDU_PI_USER_DATA:
			tdu_printf(TDU_LOG_DBG, "user data\n");
			par->udata_len = 
				tdu_parse_string(tb, par->udata,
						 TDU_PLEN_UDATA);
			break;
		default:
			tdu_printf(TDU_LOG_LOG, "tdu_parse_assoc(): unexpected"
				   " parameter '%s' ignored\n",
				   tdu_param_descr(pi) );
			tb->data = tb->pn;
		};
	} while( tb->pn < tb->tail );
	return 0;
}


void tdu_assoc_set_master(struct tdu_fsm *);
void tdu_assoc_set_slave(struct tdu_fsm *);

/* 
 * Set application response timer to the value received from peer
 * during association establishment.
 */
static void tdu_set_timeout( struct tdu_fsm * fsm)
{
	fsm->assoc.resp_timeout = fsm->assoc.remote.resp_timeout;
	if( fsm->assoc.resp_timeout <= 0 ) fsm->assoc.resp_timeout = 600;
	if( fsm->assoc.resp_timeout > 254 ) fsm->assoc.resp_timeout = 254;
}


static int tdu_send_assoc( struct tdu_fsm * fsm, int resp ) 
{
	struct tdu_buf tb[1];

	tdu_init_tb(tb);
	tdu_printf(TDU_LOG_TRC, "tdu_send_assoc()\n");
	tdu_add_assoc_params( tb, & fsm->assoc.local ); 
	if( resp ) { 
		tdu_add_reason(tb, TDU_CI_T_ASSOCIATE, 0, NULL);
		tdu_add_ci_header( tb, TDU_CI_T_RESPONSE_POSITIVE );
	} else {
		tdu_add_ci_header( tb, TDU_CI_T_ASSOCIATE );
	}
	return tdu_send_packet( tb, fsm );
}
  
/*
 * process an association establishment indication
 */
static int assoc_req_received(struct tdu_fsm *fsm, struct tdu_buf * tb)
{
	int err;
	struct tdu_param par;

	tdu_printf(TDU_LOG_TRC, "assoc_req_received()\n");
	par.reason = TDU_RE_ROLE_REFUSED_U;
	par.other_reason[0] = 0;

	if( ! fsm->assoc.slave_handler ) goto refuse;
	if( ! fsm->user->t_associate)    goto refuse;

	tdu_set_default_assoc_param( &fsm->assoc.remote );
	tdu_set_default_assoc_param( &fsm->assoc.local );

	par.reason = tdu_parse_assoc(&fsm->assoc.remote,tb);
	if ( par.reason ) goto refuse;

	par.par.assoc = &fsm->assoc.remote;
	par.res.assoc = &fsm->assoc.local;
	err = fsm->user->t_associate(fsm->user,&par);
	if( err>0 ) goto refuse;

	/* remove parameters not allowed in response */
	
	par.res.assoc->calling_len = -1;
	par.res.assoc->appl_len    = -1;
	par.res.assoc->expl_conf   = -1;
	par.res.assoc->req_ident   = -1;


	if( tdu_send_assoc(fsm,1) > 0 ){
		tdu_assoc_set_slave(fsm);
		tdu_assoc_set_idle(fsm);
		tdu_set_timeout(fsm);
		return 0;
	} else {
		tdu_abort(fsm);
		return -3;
	}

refuse:
	tdu_send_response_neg(fsm,TDU_CI_T_ASSOCIATE,
			      par.reason,par.other_reason);
	return -3;
}

/*
 * refuse accepting association request.
 */
int dont_accept(struct tdu_fsm * fsm, struct tdu_buf * dummy)
{
	tdu_printf(TDU_LOG_TRC, "dont_acept()\n");

	tdu_abort_req(fsm,TDU_RE_PROTOCOL_CONFLICT,NULL);
	return 0;
}

/*
 * Set master mode of association regime
 */
void tdu_assoc_set_master(struct tdu_fsm * fsm)
{
		fsm -> assoc.handler = fsm->assoc.master_handler;
		fsm -> assoc.idle_assoc_req_processor = dont_accept;
}

/*
 * Set slave mode of association regime
 */
void tdu_assoc_set_slave(struct tdu_fsm * fsm)
{
	tdu_printf(TDU_LOG_TRC, "tdu_assoc_set_slave()\n");

	fsm -> assoc.handler = fsm->assoc.slave_handler;
	fsm -> assoc.idle_assoc_req_processor = dont_accept;
}

/* 
 * Pepare fsm for usage as listener (server) before any regime is 
 * established
 */
void tdu_assoc_allow_listen(struct tdu_fsm * fsm)
{
		fsm -> assoc.idle_assoc_req_processor = assoc_req_received;
}

/* 
 * Set idle state of association regime
 */
void tdu_assoc_set_idle(struct tdu_fsm * fsm)
{
	tdu_printf(TDU_LOG_TRC, "tdu_assoc_set_idle()\n");

	fsm -> regime_handler = & fsm->assoc.handler;
	tdu_del_timer(fsm);
	fsm->wait=0;
}


static void tdu_release_received(struct tdu_fsm * fsm)
{
	tdu_printf(TDU_LOG_TRC, "tdu_release_received()\n");
	if( fsm->regime_handler == & fsm->assoc.handler ){
		tdu_send_response_pos(fsm, TDU_CI_T_RELEASE);
		fsm->idle.handler = tdu_released;
		fsm->regime_handler = & fsm->idle.handler;
		tdu_del_timer(fsm);
	} else {
		tdu_abort_req(fsm,TDU_RE_PROTOCOL_CONFLICT,NULL);
	}
}	

/*
 * association regime handler for master
 */
int tdu_assoc_master ( struct tdu_fsm *fsm, int event, struct tdu_buf *tb)
{
	tdu_printf(TDU_LOG_TRC, "tdu_assoc_master(), event = %s\n",
			tdu_cmd_descr(event) ); 
	switch (event) {
	case TDU_CI_T_RELEASE:
		tdu_release_received(fsm);
		break;
	case TDU_CI_T_ABORT:
		/* fall through */
	case TDU_CI_SOCK_READ_ERROR:
	case TDU_CI_TC_DISCONNECT:
		tdu_abort(fsm);
		break;
	default:
		tdu_abort_req(fsm,TDU_RE_PROTOCOL_CONFLICT,NULL);
	};
	return 0;
}

/*
 * Association regime handler for slave.
 * If "basic kernel" service class gets
 * supported once, this needs to be changed.
 */
int tdu_assoc_slave ( struct tdu_fsm *fsm, int event, struct tdu_buf *tb)
{
	tdu_printf(TDU_LOG_TRC, "tdu_assoc_slave(), event = %s\n",
			tdu_cmd_descr(event) ); 
	switch (event) {
	case TDU_CI_T_ACCESS:
		tdu_access_received(fsm,tb);
		break;
	case TDU_CI_T_RELEASE:
		tdu_release_received(fsm);
		break;
	case TDU_CI_T_ABORT:
		/* fall through */
	case TDU_CI_SOCK_READ_ERROR:
	case TDU_CI_TC_DISCONNECT:
		tdu_abort(fsm);
		break;
	default:
		tdu_abort_req(fsm,TDU_RE_PROTOCOL_CONFLICT,NULL);
	};
	return 0;
}

/*
 * association regime handler, waiting for confirmation of t_release 
 */
int tdu_await_release( struct tdu_fsm *fsm, int event, struct tdu_buf *tb)
{
	int err, pi;

	tdu_printf(TDU_LOG_TRC, "tdu_await_release(), event = %s\n",
	       tdu_cmd_descr(event) ); 
	switch (event) {
	case TDU_CI_T_RESPONSE_POSITIVE:
		pi = tdu_get_next_pi(tb);
		err = tdu_check_response(tb,pi,fsm->wait);
		if( err ) goto error;
		/* fall through */
	case TDU_CI_T_ABORT:
		/* fall through */
	case TDU_CI_SOCK_READ_ERROR:
	case TDU_CI_TC_DISCONNECT:
		tdu_abort(fsm);
		break;
	case TDU_CI_RESPONSE_TIMEOUT:
	default:
	error:
		tdu_printf(TDU_LOG_ERR, "unexpected event %s while waiting for t_release" 
		       " confirmation\n", tdu_cmd_descr(event) );
		tdu_abort_req(fsm,TDU_RE_PROTOCOL_CONFLICT,NULL);
	};
	fsm->wait = 0;
	return 0;
}

/*
 * Request termination of association regime.
 * Specifying user data not yet supported.
 *
 * FIXME: this is ugly and return logic not consistent with tdu_end_access_req
 */
int tdu_release_req(struct tdu_fsm * fsm)
{
	int ret;
	struct tdu_buf tb[1];

	tdu_printf(TDU_LOG_TRC, "tdu_release_req()\n");

	ret = in_idle_assoc_regime_master( fsm );
	if( ret <= 0 ) {
		tdu_printf( TDU_LOG_ERR, "tdu_release_req: not in idle master"
			    " state of association regime\n");
		return ret;
	}

	tdu_init_tb(tb);
	tdu_add_ci_header(tb,TDU_CI_T_RELEASE);
	ret = tdu_send_packet(tb,fsm);

	if ( ret < 0 ) { 
		perror("tdu_release_req:send_packet");
		tdu_abort(fsm);
		return ret;
	}
	fsm -> assoc.handler  = tdu_await_release;
	fsm->wait = TDU_CI_T_RELEASE;
	tdu_start_timer(fsm);
	return ret;
}       

/*
 * idle regime handler, waiting for t_associate confirmation 
 */
int tdu_await_assoc(  struct tdu_fsm *fsm, int event, struct tdu_buf *tb)
{
	int err;

	tdu_printf(TDU_LOG_TRC, "tdu_await_assoc(), event = %s\n",
	       tdu_cmd_descr(event) ); 
	switch (event) {
	case TDU_CI_T_RESPONSE_POSITIVE:
		tdu_del_timer(fsm);
		tdu_set_default_assoc_param( &fsm->assoc.remote );
		err = tdu_parse_assoc( & fsm->assoc.remote, tb );
		if( err ) {
			tdu_abort_req( fsm, err, NULL );
		} else {
			/* initial role is master */
			tdu_assoc_set_master(fsm);
			tdu_assoc_set_idle(fsm);
			tdu_set_timeout(fsm);
			tdu_printf(TDU_LOG_DBG,"Timeout set to %d\n",fsm->assoc.resp_timeout);
		}
		break;
	case TDU_CI_T_ASSOCIATE:
		/* association establish request conflict. The initator
		   of the physical connection wins */
		tdu_printf(TDU_LOG_LOG,"resolving association req conflict\n");
		if( fsm->idle.initiator ) {
			/* ignore, continue waiting for t_response */
		} else {
			fsm->assoc.idle_assoc_req_processor(fsm,tb);
		}
		break;
 	case TDU_CI_T_RESPONSE_NEGATIVE:
		tdu_printf(TDU_LOG_ERR,"association req rejected by peer\n");
		tdu_del_timer(fsm);
		fsm->idle.handler = tdu_aborted;
		break;
	case TDU_CI_T_ABORT:
		/* fall through */
	case TDU_CI_SOCK_READ_ERROR:
	case TDU_CI_TC_DISCONNECT:
		tdu_abort(fsm);
		break;
	default:
		tdu_printf(TDU_LOG_ERR,"unexpected event %s while waiting for t_associate" 
		       " confirmation\n", tdu_cmd_descr(event) );
		tdu_abort_req(fsm,TDU_RE_PROTOCOL_CONFLICT,NULL);
	};
	fsm->wait = 0;
	return 0;
}
/*
 * Check if not awaiting association establishment  
 */
static int not_associating( struct tdu_fsm * fsm )
{
	return ! (   (  fsm->regime_handler ==  &fsm->idle.handler )
		     &&(fsm->idle.handler    ==   tdu_await_assoc  ) );
}

/* 
   process events until no longer awaiting association regime establishment
   returns: >0 if idle state of association regime master is reached
            <0 if other state != awaiting association is reached
*/
int tdu_wait_for_not_associating(struct tdu_fsm * fsm, struct timeval * timeout)
{
	int ret;
	tdu_printf(TDU_LOG_TRC, "wait_for_not_associating()\n");
	ret = tdu_wait( fsm, timeout, not_associating );

	if ( in_idle_assoc_regime_master(fsm) ) return 1;
	if ( in_idle_assoc_regime_slave(fsm) ) return 1;
	return -1;
}

/*
 * establish association regime
 */
int tdu_assoc_req( struct tdu_fsm * fsm, struct tdu_assoc_param * par )
{
	/* when the association regime is established, we need to become
	   master. Reject, if this is not possible*/
	tdu_printf(TDU_LOG_TRC, "tdu_assoc_req()\n");
	if ( (! tdu_before_regime(fsm) )     ||  
	     (! fsm->assoc.master_handler) ){
		tdu_printf(TDU_LOG_ERR,"tdu_assoc_req(): no association request possible\n");
		return -1;
	}
	fsm->assoc.local = *par;
	/* FIXME: contents of *par needs checking */
	
	if( tdu_send_assoc( fsm, 0 ) > 0 ){
		fsm->idle.handler = tdu_await_assoc;
		fsm->wait = TDU_CI_T_ASSOCIATE;
		tdu_start_timer( fsm );
		return 1;
	}
	return 0;
}


void tdu_set_default_assoc_param( struct tdu_assoc_param * par )
{
	tdu_printf(TDU_LOG_TRC, "tdu_set_default_assoc_param()\n");

	par->called_len = -1;
	par->calling_len = -1;
	par->appl_len = -1;
	par->resp_timeout = 0;
	par->symm_service=1;
	par->basic_kernel=0;
	par->expl_conf= TDU_BIT_EXPL_CONF;
	par->ident_len = -1;
	par->req_ident = 0;
	par->udata_len = -1;
}

