/* $Id: transfer.c,v 1.1 1999/06/30 17:18:48 he Exp $ */
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

  This file containing stuff used during transfer regime.
*/ 

#include <stdio.h>
#include <unistd.h>
#include <tdu_user.h>
#include "tdu.h"

static int t_write_received (struct tdu_fsm *, struct tdu_buf * tb);
static int transfer_rej_received (struct tdu_fsm *, struct tdu_buf * tb);

/*
 * transfer regime event handler for receiver receiving data from remote
 */
int tdu_tr_receiving( struct tdu_fsm *fsm, int event, struct tdu_buf * tb)
{
	tdu_printf( TDU_LOG_TRC, "tdu_tr_receiving(): event = %s\n",
		    tdu_cmd_descr(event));
	
	switch (event) {
	case TDU_CI_T_TRANSFER_REJECT:
		/* should only happen with symmetrical service class */
		transfer_rej_received(fsm, tb);
		break;
	case TDU_CI_T_WRITE:
		t_write_received (fsm, tb);
		break;
	case TDU_CI_T_P_EXCEPTION:
	default:
		tdu_printf( TDU_LOG_DBG, "tdu_tr_receiving(), event %s passed down\n",
			tdu_cmd_descr(event) );

		(*(fsm->access.handler)) (fsm, event, tb);
	}
	return 0;
};


static int sender_resp_pos_received (struct tdu_fsm *, struct tdu_buf * tb);
static int sender_resp_neg_received (struct tdu_fsm *, struct tdu_buf * tb);

/*
 * transfer regime event handler for sender sending data to remote
 */
int tdu_tr_sending( struct tdu_fsm *fsm, int event, struct tdu_buf * tb)
{
	switch (event) {
	case TDU_CI_T_RESPONSE_POSITIVE:
		sender_resp_pos_received( fsm, tb );
		break;
	case TDU_CI_T_RESPONSE_NEGATIVE:
		sender_resp_neg_received( fsm, tb );
		break;
	case TDU_CI_T_TRANSFER_REJECT:
		tdu_printf( TDU_LOG_ERR, "tdu_tr_sending(): received Transfer Reject\n");
		transfer_rej_received( fsm, tb);
		break;
	case TDU_CI_T_READ_RESTART:
		tdu_printf( TDU_LOG_ERR, "tdu_tr_sending(): received Read Restat Request -- not supported\n");
		tdu_abort_req( fsm, TDU_RE_PRIMITIVE_NOT_HANDLED, 0);
		break;
	default:
		tdu_printf( TDU_LOG_DBG, "tdu_tr_sending(), event %s passed down\n",
			tdu_cmd_descr(event) );

		(*(fsm->access.handler)) (fsm, event, tb);
	}
	return 0;
};

void tdu_trans_start(struct tdu_fsm *fsm)
{
	fsm->transfer.pkt_cnt=0;
	fsm->transfer.byte_cnt=0;
	gettimeofday( & fsm->transfer.tv_begin, NULL );
}

/*
 * Parse an ExplicitConfirmation/First/Last/BlockNumber parameter.
 * Returns the block number and modifies the data pointed to by the
 * other parameters.
 */
static int tdu_get_block(struct tdu_buf *tb, 
			   int *first, int *last, int *conf)
{
	unsigned char byte;

	byte   = *tb->data++;
	*first = byte  &  TDU_BIT_FIRST_BLOCK;
	*last  = byte  &  TDU_BIT_LAST_BLOCK;
	*conf  = byte  &  TDU_BIT_EXPL_CONF;
	return tdu_parse_li(tb);
}

/*
 * Confirm a t_write request. response is positive if reason==0, negative
 * otherwise.
 */
static int confirm_t_write(struct tdu_fsm *fsm, int block, int last,
			   int reason)
{
	struct tdu_buf tb[1];

	tdu_init_tb( tb );

	/* only negative responses to last block contain reason code */
	if( last && reason ) {
		tb->data--;
		*(tb->data) = reason;
	}
	tdu_stick_li(tb,block);
	tdu_add_reason(tb, TDU_CI_T_WRITE, 0 , NULL);
	if(reason){
		tdu_add_ci_header(tb,TDU_CI_T_RESPONSE_NEGATIVE);
	} else {
		tdu_add_ci_header(tb,TDU_CI_T_RESPONSE_POSITIVE);
	}
	return tdu_send_packet(tb,fsm);
}

/*
 * Notify a user about finished mass transfer phase. 
 * This indication is not part of ETS 300 075 and provided for
 * convenience of upper layers which might want to run some hooks
 * (such as writing log messages) when mass transfer is finished.
 */
static void tdu_transfer_end_notify(struct tdu_fsm *fsm, int abnormal)
{
	if(fsm->user->transfer_end) 
		fsm->user->transfer_end(fsm->user,abnormal);
}

/*
 * process a received t_write tdu
 */
static int t_write_received (struct tdu_fsm *fsm, struct tdu_buf * tb)
{
	int pi, first, last, conf, cnt, block;
	int ret;

	tdu_printf(TDU_LOG_HASH,"#");

	pi=tdu_get_next_pi(tb); 
	if ( pi != TDU_PI_EX_CONF_1ST_LAST_BLNO ){
		tdu_p_except_req(fsm, pi);
		return pi;
	}
	block = tdu_get_block(tb,&first,&last,&conf);
	if(block < 0){
		tdu_p_except_req(fsm, block);
		return block;
	}
	
	tdu_printf(TDU_LOG_WRI, "t_write_received: block %d first=%d last=%d ExplicitConf=%d\n",
	       block, first, last, conf);

	cnt = tb->tail - tb->data;
	if( last ) {
		tdu_printf( TDU_LOG_DBG, "calling fsm->stream->t_write_end()\n");
		ret = fsm->stream->t_write_end(fsm->stream, tb);
	} else {
		tdu_printf( TDU_LOG_DBG, "calling fsm->stream->t_write()\n");
		ret = fsm->stream->t_write(fsm->stream, tb);
	}
	fsm->transfer.byte_cnt += ret;
	fsm->transfer.pkt_cnt++;
	if( ret == cnt ) {
		tdu_printf(TDU_LOG_DBG, "t_write[end]_ind successful\n");
		if(last) {
			/* send positve response if closing succeeded.
			 * send negative response if closing failed.
			 * (the latter may be replaced by transfer reject)
			 */
			confirm_t_write(fsm, block, 1,
					fsm->stream->close(fsm->stream));
			tdu_access_set_idle(fsm);
			tdu_transfer_end_notify(fsm,0);
		} else {
			confirm_t_write(fsm, block, 0, 0);
			tdu_start_timer(fsm);
		}
			
	} else {
		tdu_printf(TDU_LOG_ERR, "t_write_ind failed, ret=%d\n",ret);
		confirm_t_write(fsm, block, 0, -1);
		/* FIXME: use proper reason code*/
		tdu_transfer_rej_req(fsm, TDU_RE_OTHER_REASON,
				     "local write error");
	}
	return ret;
}

/*
 * process a positive response while sending 
 */
static int sender_resp_pos_received (struct tdu_fsm *fsm, struct tdu_buf * tb)
{
	int pi, err;

	tdu_printf(TDU_LOG_TRC, "sender_resp_pos_received()");
	
	pi = tdu_get_next_pi(tb);
	err = tdu_check_response(tb,pi,TDU_CI_T_WRITE);
	if( err ) return tdu_p_except_req(fsm,err);

	tb->data++;
	fsm->transfer.confirmed = tdu_parse_li(tb);

	tdu_printf(TDU_LOG_REW, " received pos ack for pkt=%d (last=%d)\n",
		   fsm->transfer.confirmed, fsm->transfer.last);
	/* integrity check */
	if( fsm->transfer.confirmed > fsm->transfer.pkt_cnt ||
	    fsm->transfer.confirmed < fsm->transfer.pkt_cnt - 10 ){
		tdu_printf(TDU_LOG_ERR,"sequence error\n");
		/* is this the correct error code? */
		return tdu_p_except_req( fsm, TDU_RE_PROTOCOL_CONFLICT);
	}
	tdu_start_timer( fsm );
	if( fsm->transfer.confirmed == fsm->transfer.last ){
		gettimeofday( & fsm->transfer.tv_end, NULL );
		tdu_access_set_idle(fsm);
		fsm->stream->close(fsm->stream);
		tdu_transfer_end_notify(fsm,0);
	}
	return 0;
}

/*
 * process a negative response while sending 
 */
static int sender_resp_neg_received (struct tdu_fsm *fsm, struct tdu_buf * tb)
{
	int pi, err, block, digits=0, tmp;
	unsigned char *reason_code=NULL;

	tdu_printf(TDU_LOG_TRC, "sender_resp_neg_received()");
	
	pi = tdu_get_next_pi(tb);
	err = tdu_check_response(tb,pi,TDU_CI_T_WRITE);
	if( err ) return tdu_p_except_req(fsm,err);
	tb->data++;

	/*
	 * Only responses to the the last t_write may contain a reason
	 * code. Unfortunately, the length of the reason code
	 * is not known and the block number is carried in the same
	 * parameter as the reason code. Thus, we don't know the 
	 * last byte belonging to the block number and subsequently,
	 * we could not decode the block number correctly. We try to do
	 * at least somthing about this protocol design flaw. The
	 * following should work as long as the minimum number of digits
	 * was used to encode the block number (no leading zeroes).
	 */
	if( fsm->transfer.last >= 0 ){
		tmp = fsm->transfer.last;
		while( tmp ) {
			tmp /= 256;
			digits++;
		};
#if 0		
		tdu_printf(TDU_LOG_TMP,"%d digits required\n",digits);
#endif
	}

	/*
	 * strip first leading zero 
	 * (will be present if last block is block 0)
	 */
	if( *(tb->data) == 0 ) tb->data++;
#if 0
	tdu_printf(TDU_LOG_TMP,"%d digits present\n",tb->pn - tb->data);
#endif
	if( (tb->pn - tb->data) > digits){
		/*
		 * reason too long: assuming
		 * negative response to last block with reason code.
		 * Block number needs to be parsed by hand and checked.
		 */
		block=0;
		reason_code = tb->data + digits;
		while( tb->data < reason_code ) {
			block *= 0x100;
			block += *(tb->data);
			(tb->data)++;
		};
		if( block != fsm->transfer.last ){
			tdu_send_p_exception(fsm,TDU_RE_SYNTAX_ERROR);
			/* FIXME: does this need to be treated differently */
			transfer_rej_received( fsm, tb);
			return -1;
		}
	} else {
		block = tdu_parse_li(tb);
	}
	tdu_printf(TDU_LOG_ERR, " received neg ack for pkt=%d (last=%d)",
		   block, fsm->transfer.last);
	if( reason_code ) tdu_printf(TDU_LOG_ERR, ": %s",
				     tdu_str_reason(*reason_code));
	tdu_printf(TDU_LOG_ERR, "\n");
	if( ! reason_code ){
		/* The (possible last) block was refused, but we don't
		 *  support any recovery procedure (yet). */
		/* Is this the correct error code? */
		tdu_send_p_exception(fsm,TDU_RE_PRIMITIVE_NOT_HANDLED);
		/* FIXME: does this need to be treated differently */
		transfer_rej_received( fsm, tb);
	} else {
		/* 
		 * The submitted file was transmitted successfully,
		 * but finally not accepted by the receiver. We
		 * (the sender) close it normally
		 */
		gettimeofday( & fsm->transfer.tv_end, NULL );
		tdu_access_set_idle(fsm);
		fsm->stream->close(fsm->stream);
		tdu_transfer_end_notify(fsm,1);
	}
	return 0;
}


static int transfer_rej_received(struct tdu_fsm *fsm, struct tdu_buf * tb)
{
	tdu_printf(TDU_LOG_ERR, "t_u_execption_report (transfer reject)\n");
	tdu_transfer_abort(fsm);
	return 0;
}

/*
 * perform a t_write_request.
 */
int tdu_t_write_req( struct tdu_fsm *fsm, struct tdu_buf * tb )
{
	int  wlen, hlen;
	int  len  =  tb->tail - tb->md;
	unsigned char conf;

	conf = 0x08; /* expl. conf requested */
	if( ! fsm->transfer.pkt_cnt  ) conf |= 0x01; /* first block */
	if(   fsm->transfer.last >= 0 ) conf |= 0x02; /* last block  */

	tb -> data = tb -> md;
	tdu_stick_li(tb, fsm->transfer.pkt_cnt);
	*(--tb->data) = conf;
	tdu_stick_le(tb, tb->md - tb->data );
	*(--tb->data) = TDU_PI_EX_CONF_1ST_LAST_BLNO;
	tdu_stick_le(tb, tb->tail - tb->data );
	*(--tb->data) = TDU_CI_T_WRITE;
	tb->ci = tb->data;
	*(--tb->data) = 0; /* no Q-bit */
	hlen = tb->md   - tb->data;
	len =  tb->tail - tb->data;

	tdu_printf(TDU_LOG_WRI,"tdu_t_write_req: pkt=%d, ack'ed=%d, last=%d "
		   "writing: ",fsm->transfer.pkt_cnt, fsm->transfer.confirmed,
		   fsm->transfer.last);
	tdu_print_cmd(TDU_LOG_WRI,tb->ci,tb->tail,4);
	wlen = write( fsm->socket, tb->data, len ) - hlen;
	/* tdu_printf("tdu_t_write_req: %d bytes written\n", wlen); */
	if( wlen == (len - hlen) ) {
		fsm->transfer.pkt_cnt++;
		fsm->transfer.byte_cnt += wlen;
		tdu_start_timer(fsm);
		tdu_printf(TDU_LOG_HASH,"#");
	} else {
		perror("tdu_t_write_req:write()");
	}
	return wlen;
}

/*
 * perform a t_write_end_request.
 */
int tdu_t_write_end_req( struct tdu_fsm *fsm, struct tdu_buf * tb)
{
	fsm->transfer.last = fsm->transfer.pkt_cnt;
	tdu_printf(TDU_LOG_WRI,"tdu_t_write_end_req() last=%d\n",
		   fsm->transfer.last);
	return tdu_t_write_req( fsm, tb );
}


void tdu_transfer_abort(struct tdu_fsm * fsm)
{
	gettimeofday( & fsm->transfer.tv_end, NULL );
	tdu_access_set_idle( fsm );
	tdu_transfer_end_notify(fsm,1);
}

/* 
 * send a t_transfer_reject tdu to peer 
 */
static int tdu_send_transfer_reject( struct tdu_fsm *fsm, int reason,
				     unsigned char * other_reason )
{
	struct tdu_buf tb[1];

	tdu_init_tb( tb );
	tdu_add_reason( tb, 0, reason, other_reason );
	tdu_add_ci_header( tb, TDU_CI_T_TRANSFER_REJECT );
	return tdu_send_packet( tb, fsm );
}


int tdu_transfer_rej_req(struct tdu_fsm * fsm, int reason,
			  unsigned char * other_reason )
{
	tdu_send_transfer_reject( fsm, reason, other_reason );
	tdu_transfer_abort(fsm);
	return 0;
}
