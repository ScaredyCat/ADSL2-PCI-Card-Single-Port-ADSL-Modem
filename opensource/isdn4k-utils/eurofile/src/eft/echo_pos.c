/* $Id: echo_pos.c,v 1.1 1999/06/30 17:18:10 he Exp $ */
#include <tdu_user.h>
#include "tdu.h"

/*
 * Encapsulate parameters of a received tdu inside a t_response_positive
 * tdu and echo it back to the originator. This is only used for testing
 * and developement purpose for faking certain answers if there is a lack
 * of a properly implemented event response.
 */
int tdu_echo_pos( struct tdu_fsm *fsm, int event, struct tdu_buf *tb)
{
                tdu_printf( -1, "tdu_echo_pos: event=%d\n",event);
		tb->pn = tb->data;

		/* the data part of a t_write tdu won't be echoed, only the
		   block number for faking achnowledgment */
                if( event == TDU_CI_T_WRITE )
                        tb->tail = tb->data + 2 + *(tb->data+1);

                tdu_add_reason(tb,event,0,0);
                tdu_add_ci_header(tb,TDU_CI_T_RESPONSE_POSITIVE);

                tdu_send_packet(tb,fsm);
		return 0;
}



