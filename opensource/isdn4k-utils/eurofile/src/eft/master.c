/* $Id: master.c,v 1.1 1999/06/30 17:18:33 he Exp $ */
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

  This file containing stuff only needed by the T-protocol's master.
*/ 

#include <stdio.h>
#include <unistd.h>
#include <tdu_user.h>
#include "tdu.h"

/*
 * access regime event handler, idle master waiting for events 
 */
int tdu_master_idle( struct tdu_fsm *fsm, int event, struct tdu_buf * tb)
{
	int ret=0;
	
	tdu_printf(TDU_LOG_TRC, "tdu_master_idle(), event = %s\n",
			tdu_cmd_descr(event) ); 
	switch (event) {
	case TDU_CI_T_END_ACCESS:
		ret = tdu_end_access_received(fsm);
		break;
	case TDU_CI_T_TYPED_DATA:
		ret = tdu_typed_data_received(fsm, tb);
		break;
	case TDU_CI_T_ABORT:
	case TDU_CI_SOCK_READ_ERROR:
	case TDU_CI_TC_DISCONNECT:
		ret = fsm->assoc.handler (fsm, event, tb);
		break;
	default:
		tdu_printf( TDU_LOG_ERR, "tdu_master_idle(): unexpected event %s ignored\n",
			tdu_cmd_descr(event) );
	};
	return ret;
}

/*
 * access regime, master waiting for confirmation of a t_dir request 
 */
int tdu_await_dir( struct tdu_fsm *fsm, int event, struct tdu_buf * tb)
{
	int err;

	tdu_printf(TDU_LOG_TRC, "tdu_await_dir(), event = %s\n",
			tdu_cmd_descr(event) ); 
	switch (event) {
	case TDU_CI_T_RESPONSE_POSITIVE:
		err = tdu_generic_response_received(fsm,tb);
		if( err ) break;
		fsm->transfer.handler = tdu_tr_receiving;
		fsm->access.handler   = tdu_access_loading;
		fsm->regime_handler = & fsm->transfer.handler;
		gettimeofday( & fsm->transfer.tv_begin, NULL );
		tdu_trans_start(fsm);
		/*		tdu_del_timer(fsm);*/
		tdu_start_timer(fsm);
		break;
	case TDU_CI_T_ABORT:
		fsm->assoc.handler (fsm, event, tb);
		break;
	case TDU_CI_T_RESPONSE_NEGATIVE:
		err = tdu_generic_response_received(fsm,tb);
		if(err) break;
		goto error;
	case TDU_CI_RESPONSE_TIMEOUT:
		tdu_send_p_exception(fsm,TDU_RE_DELAY_EXPIRED);
		/* fall through */
	case TDU_CI_T_P_EXCEPTION:
	default:
	error:
		tdu_printf(TDU_LOG_ERR, "unexpected event %s while waiting for t_dir/t_load confirm\n",
		       tdu_cmd_descr(event) );
		tdu_access_set_idle(fsm);
	};
	fsm->wait = 0;
	return 0;
}

/*
 * access regime, master waiting for confirmation of a t_save request
 */
int tdu_await_save( struct tdu_fsm *fsm, int event, struct tdu_buf * tb)
{
	int err;

	tdu_printf(TDU_LOG_TRC,"tdu_await_save(), event = %s\n",
			tdu_cmd_descr(event) ); 
	switch (event) {
	case TDU_CI_T_RESPONSE_POSITIVE:
		err = tdu_generic_response_received(fsm,tb);
		if( err ) break;
		fsm->transfer.handler = tdu_tr_sending;
		fsm->access.handler   = tdu_access_saving;
		fsm->regime_handler = & fsm->transfer.handler;
		tdu_trans_start(fsm);
		tdu_del_timer(fsm);
		break;
	case TDU_CI_T_ABORT:
		fsm->assoc.handler (fsm, event, tb);
		break;
	case TDU_CI_T_RESPONSE_NEGATIVE:
		err = tdu_generic_response_received(fsm,tb);
		if(err) break;
		goto error;
	case TDU_CI_RESPONSE_TIMEOUT:
		tdu_send_p_exception(fsm,TDU_RE_DELAY_EXPIRED);
		/* fall through */
	case TDU_CI_T_P_EXCEPTION:
	default:
	error:
		tdu_printf(TDU_LOG_ERR,"unexpected event %s while waiting for t_save" 
		       " confirm\n", tdu_cmd_descr(event) );
		tdu_access_set_idle(fsm);
	};
	fsm->wait = 0;
	return 0;
}

/*
 * access regime, master waiting for confirmation of a t_load request 
 */
tdu_handler tdu_await_load = tdu_await_dir;

/*
 * Fill in parameters in a t_{load,save}_request packet
 */
static int tdu_add_transfer_params( struct tdu_buf * tb, struct tdu_transfer_param * par)
{
	int ret=0;
	
	tdu_printf( TDU_LOG_TRC, "tdu_add_transfer_params():\n");
	if( par->recovery_point >= 0 )
		ret += tdu_add_int_par(tb,TDU_PI_RECOVERY_POINT,
				       par->recovery_point);

	if( par->designation && (par->designation_len >= 0) ){
		ret += tdu_add_string_par
			(tb, TDU_PI_DESIGNATION, par->designation,
			 LIMIT(par->designation_len, TDU_PLEN_DESIGNATION));
	} else {
		tdu_printf(TDU_LOG_ERR, 
			   "designation parameter is mandatory but missing\n");
		return -1;
	}

	if( par->udata && (par->udata_len >= 0) ){
		ret += tdu_add_string_par
			(tb, TDU_PI_USER_DATA, par->udata,
			 LIMIT(par->udata_len,TDU_PLEN_UDATA));
	}
	return ret;
}

/*
 * perform a directory request
 */
int tdu_t_dir_req( struct tdu_fsm *fsm, struct tdu_transfer_param * par)
{
	struct tdu_buf tb[1];

	tdu_init_tb(tb);
	tdu_printf( TDU_LOG_TRC, "tdu_t_dir_req():\n");

	if( par->recovery_point >= 0 ){
		tdu_printf(TDU_LOG_ERR, "recovery not allowd for t_dir\n");
		return TDU_RE_SYNTAX_ERROR;
	}
	if( tdu_add_transfer_params( tb, par ) < 0 ) 
		return TDU_RE_SYNTAX_ERROR;

	tdu_add_ci_header(tb, TDU_CI_T_DIRECTORY);
		
	fsm->transfer.pkt_cnt=0;
	fsm->transfer.byte_cnt=0;
				
	if( tdu_send_packet( tb, fsm ) < 0 ){
		perror( "tdu_t_dir_req: send_packet" );
		return -1;
	} else {
		fsm->access.handler = tdu_await_dir;
		fsm->regime_handler = & fsm -> access.handler;
		tdu_start_timer(fsm);
		fsm->wait = TDU_CI_T_DIRECTORY;
	}
	return 0;
}

/*
 * request downloading a file from the peer
 */
int tdu_t_load_req( struct tdu_fsm *fsm, struct tdu_transfer_param * par)
{
	struct tdu_buf tb[1];

	tdu_init_tb(tb);
	tdu_printf( TDU_LOG_TRC, "tdu_t_load_req():\n");
	if( tdu_add_transfer_params( tb, par ) < 0 ) 
		return TDU_RE_SYNTAX_ERROR;

	tdu_add_ci_header(tb, TDU_CI_T_LOAD);
		
	fsm->transfer.pkt_cnt=0;
	fsm->transfer.byte_cnt=0;

	if( tdu_send_packet( tb, fsm ) < 0 ){
		perror( "tdu_t_load_req: send_packet" );
		return -1;
	} else {
		fsm->access.handler = tdu_await_load;
		fsm->regime_handler = & fsm -> access.handler;
		tdu_start_timer(fsm);
		fsm->wait = TDU_CI_T_LOAD;
	}
	return 0;
}

/*
 * request transmitting a file to the peer
 */
int tdu_t_save_req( struct tdu_fsm *fsm, struct tdu_transfer_param * par)
{
	struct tdu_buf tb[1];

	tdu_init_tb(tb);
	tdu_printf( TDU_LOG_TRC, "tdu_t_save_req():\n");
	if( tdu_add_transfer_params( tb, par ) < 0 ) 
		return TDU_RE_SYNTAX_ERROR;

	tdu_add_ci_header(tb, TDU_CI_T_SAVE);
		
	fsm->transfer.pkt_cnt=0;
	fsm->transfer.byte_cnt=0;

	if( tdu_send_packet( tb, fsm ) < 0 ){
		perror( "tdu_t_load_req: send_packet" );
		return -1;
	} else {
		fsm->access.handler = tdu_await_save;
		fsm->regime_handler = & fsm -> access.handler;
		tdu_start_timer(fsm);
		fsm->transfer.last      = -1;
		fsm->transfer.confirmed = -1;
		fsm->wait = TDU_CI_T_SAVE;
	}
	return 0;
}


void tdu_init_master(struct tdu_fsm * fsm)
{
	fsm->assoc.master_handler = tdu_assoc_master;
	fsm->assoc.slave_handler  = NULL; /*change to slave should fail*/
	tdu_assoc_set_master(fsm);
	fsm->access.idle_master_handler = tdu_master_idle;
	fsm->access.idle_slave_handler  = NULL; /* Requests for establishing
						   access regime as slave 
						   will be rejected */
	tdu_access_set_master(fsm);
	fsm->assoc.resp_timeout  = 23; /* This will be reset when establishing
					  association regime */
}



