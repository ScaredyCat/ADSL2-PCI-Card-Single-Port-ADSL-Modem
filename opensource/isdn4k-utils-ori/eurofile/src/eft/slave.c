/* $Id: slave.c,v 1.2 1999/10/06 18:16:22 he Exp $ */
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

  This file containing stuff only needed by the T-protocol's slave.
*/ 

#include <stdio.h>
#include <unistd.h>
#include <tdu_user.h>
#include "tdu.h"

	
int not_handled( struct tdu_fsm * fsm, int ci )
{
	int ret;

	ret = tdu_send_response_neg(fsm,ci,TDU_RE_PRIMITIVE_NOT_HANDLED,0);
	if( ret < 0 ) tdu_abort(fsm);
	return ret;
}


/*
 * Parse parameters of file trans related requests or positive response
 * message (should be useable for t_dir/load/save/rename/delete).
 * returns:
 *   0 on success 
 *   an integer reason code from the TDU_RE_* set when an error is detected.
 */
int tdu_parse_trf( struct tdu_transfer_param *par, struct tdu_buf *tb)
{
	int pi;
	par->designation_len = -1;
	par->udata_len = -1;
	par->new_len = -1;
	par->recovery_point=-1; 

	tdu_printf(TDU_LOG_TRC, "tdu_parse_trf()\n");
	do {
		tdu_printf(TDU_LOG_DBG, "parsing parameter %d=%s\n",*tb->data,
			   tdu_param_descr(*tb->data) );
		pi = tdu_get_next_pi(tb);
		if( pi < 0 ) return -pi;
		switch( pi ){
		case TDU_PI_DESIGNATION:
			tdu_printf(TDU_LOG_DBG, "designation\n");
			par->designation_len =
				tdu_parse_string(tb, par->designation,
						 TDU_PLEN_DESIGNATION);
			break;
		case TDU_PI_USER_DATA:
			tdu_printf(TDU_LOG_DBG, "user data\n");
			par->udata_len =
				tdu_parse_string(tb, par->udata,
						 TDU_PLEN_UDATA);
			break;
		case TDU_PI_NEW_NAME:
			tdu_printf(TDU_LOG_DBG, "new name\n");
			par->new_len =
				tdu_parse_string(tb, par->new_name,
						 TDU_PLEN_NEW_NAME);
			break;
		case TDU_PI_RECOVERY_POINT:
			tdu_printf(TDU_LOG_DBG, "recovery point\n");
			if( (par->recovery_point=tdu_parse_li(tb)) < 0 ) 
				return TDU_RE_SYNTAX_ERROR;
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
 * Does the work to be performed after having received a t_load or t_dir
 * indication
 */
int tdu_load_dir_received(struct tdu_fsm * fsm, struct tdu_buf * tb,
			  int load_dir)
{
	struct tdu_param par;
	struct tdu_transfer_param tp;
	int ret;
	int (*func)(struct tdu_user *, struct tdu_param *);

	tdu_printf(TDU_LOG_TRC, "tdu_load_dir_received()\n");

	par.reason = TDU_RE_PRIMITIVE_NOT_HANDLED;
	par.other_reason[0] = 0;

	switch( load_dir ){
	case TDU_CI_T_LOAD:
		func = fsm->user->t_load;
		if( ! ( (fsm->access.local.functions & TDU_BIT_LOAD) &&
			 func ) ) goto error;
		break;
	case TDU_CI_T_DIRECTORY:
		func = fsm->user->t_dir;
		if( ! ( (fsm->access.local.functions & TDU_BIT_DIRECTORY) &&
			func ) ) goto error;
		break;
	default:
		goto error;
	}

	par.reason = tdu_parse_trf(&tp,tb);
	if( par.reason ) goto error;
	if( tp.designation_len < 0 ) {
		par.reason = TDU_RE_SYNTAX_ERROR;
		goto error;
	}
	if( tp.recovery_point >= 0 ){
		tdu_printf(TDU_LOG_ERR, "recovery not allowd for t_dir\n");
		par.reason = TDU_RE_SYNTAX_ERROR;
		goto error;
	}
	par.par.transfer = &tp;
	if( ! func(fsm->user, &par) ){
		/* request was allowd to be performed */
		ret = tdu_send_response_pos( fsm, load_dir); 
		if( ret < 0 ) { 
			tdu_abort(fsm);
			return ret;
		}
		/* 
		 * As everything was successful, establish transfer regime
		 * as sender ...
		 */
		fsm->transfer.pkt_cnt=0;
		fsm->transfer.byte_cnt=0;

		fsm->transfer.last      = -1;
		fsm->transfer.confirmed = -1;

		fsm->transfer.handler = tdu_tr_sending;
		fsm->access.handler   = tdu_access_saving;
		fsm->regime_handler = & fsm->transfer.handler;
		tdu_trans_start(fsm);
		tdu_del_timer(fsm);
		/* 
		 * ... and finally loop until the transmission is finished
		 */
		return tdu_wait_for_idle(fsm,0);
	}
	if( par.reason ) goto error;

error:
	ret = tdu_send_response_neg( fsm, load_dir,
				     par.reason, par.other_reason);
	if( ret < 0 ) tdu_abort(fsm);
	return ret;
}

/*
 * Does the work to be performed after having received a t_save indication
 */
int tdu_save_received(struct tdu_fsm * fsm, struct tdu_buf * tb)
{
	struct tdu_param par;
	struct tdu_transfer_param tp;
	int ret;

	tdu_printf(TDU_LOG_TRC, "tdu_load_dir_received()\n");

	par.reason = TDU_RE_PRIMITIVE_NOT_HANDLED;
	par.other_reason[0] = 0;

	if( ! ( (fsm->access.local.functions & TDU_BIT_LOAD) &&
		fsm->user->t_save ) ) goto error;

	par.reason = tdu_parse_trf(&tp,tb);
	if( par.reason ) goto error;
	if( tp.designation_len < 0 ) {
		par.reason = TDU_RE_SYNTAX_ERROR;
		goto error;
	}
	if( tp.recovery_point >= 0 ){
		tdu_printf(TDU_LOG_ERR, "recovery not allowd\n");
		par.reason = TDU_RE_SYNTAX_ERROR;
		goto error;
	}


	par.par.transfer = &tp;
	if( ! fsm->user->t_save(fsm->user, &par) ){
		/* request was allowd and is to be performed */
		ret = tdu_send_response_pos( fsm, TDU_CI_T_SAVE); 
		if( ret < 0 ) { 
			tdu_abort(fsm);
			return ret;
		}
		/* 
		 * As everything was successful, establish transfer regime
		 * as sender ...
		 */
		fsm->transfer.handler = tdu_tr_receiving;
		fsm->access.handler   = tdu_access_loading;
		fsm->regime_handler = & fsm->transfer.handler;

		tdu_trans_start(fsm);
		tdu_start_timer(fsm);

		/* 
		 * ... and finally loop until the transmission is finished
		 */
		return tdu_wait_for_idle(fsm,0);
	}
	if( par.reason ) goto error;

error:
	ret = tdu_send_response_neg( fsm, TDU_CI_T_SAVE,
				     par.reason, par.other_reason);
	if( ret < 0 ) tdu_abort(fsm);
	return ret;
}



/*
 * access regime event handler, idle slave waiting for events 
 */
int tdu_slave_idle( struct tdu_fsm *fsm, int event, struct tdu_buf * tb)
{	
	tdu_printf( TDU_LOG_TRC, "tdu_slave_idle(): event = %d\n", event);

	switch (event) {
	case TDU_CI_T_END_ACCESS:
		tdu_end_access_received(fsm);
		break;
	case TDU_CI_T_ABORT:
	case TDU_CI_SOCK_READ_ERROR:
	case TDU_CI_TC_DISCONNECT:
		fsm->assoc.handler (fsm, event, tb);
		break;
	case TDU_CI_T_P_EXCEPTION:
		tdu_access_set_idle(fsm);
		break;
	case TDU_CI_T_TYPED_DATA:
		tdu_typed_data_received(fsm,tb);
		break;
	case TDU_CI_T_DIRECTORY:
	case TDU_CI_T_LOAD:
		tdu_load_dir_received(fsm,tb,event);
		break;
	case TDU_CI_T_SAVE:
		tdu_save_received(fsm,tb);
		break;
	case TDU_CI_T_DELETE:
	case TDU_CI_T_RENAME:
		/* FIXME: implement this! */
		not_handled( fsm, event );
		break;
	default:
		tdu_printf( TDU_LOG_ERR, "tdu_slave_idle(): unexpected event %s ignored\n",
			tdu_cmd_descr(event) );
#if 0
		/* possible hack for testing purpose */ 
		tdu_echo_pos(fsm,event,tb);
#endif
	};
	return 0;
}


void tdu_init_slave(struct tdu_fsm * fsm)
{
	fsm->assoc.master_handler = NULL; /*change to master should fail*/
	fsm->assoc.slave_handler  = tdu_assoc_slave;
	tdu_assoc_set_slave(fsm);
	fsm->access.idle_slave_handler   = tdu_slave_idle;
	fsm->access.idle_master_handler  = NULL; /* Requests for establishing
						   access regime as master 
						   will be rejected */
	tdu_access_set_slave(fsm);
	fsm->regime_handler = & fsm->assoc.handler;
	fsm->assoc.resp_timeout  = 23; /* This will be reset when establishing
					  association regime */
}
