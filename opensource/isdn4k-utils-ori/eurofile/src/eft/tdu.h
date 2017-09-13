/* $Id: tdu.h,v 1.1 1999/06/30 17:18:41 he Exp $ */
/*
  A (rather incomplete) implementation of the T-protocol
  defined in ETS 300 075.

  Copyright 1997 by Henner Eisen

  This software may be copied or modified under the terms of the
  GNU General Public Licence (GPL), Version 2.

*/ 
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>

#define TDU_MAX_BUF_LEN 4097


/* TDU command identifiers according to ETS 300 075, table 4 */

#define	TDU_CI_T_ASSOCIATE		0x20
#define	TDU_CI_T_RELEASE		0x21
#define	TDU_CI_T_ABORT			0x38

#define	TDU_CI_T_ACCESS			0x22
#define	TDU_CI_T_END_ACCESS		0x23

#define	TDU_CI_T_DIRECTORY		0x24
#define	TDU_CI_T_LOAD			0x25
#define	TDU_CI_T_SAVE			0x26
#define	TDU_CI_T_RENAME			0x27
#define	TDU_CI_T_DELETE			0x28
#define	TDU_CI_T_TYPED_DATA		0x29
#define	TDU_CI_T_WRITE			0x2f

#define	TDU_CI_T_TRANSFER_REJECT	0x36
#define	TDU_CI_T_READ_RESTART		0x37
#define	TDU_CI_T_P_EXCEPTION		0x2e
#define	TDU_CI_T_RESPONSE_POSITIVE	0x32
#define	TDU_CI_T_RESPONSE_NEGATIVE	0x33

/* Dummy CI's, used for other events net resulting from T-protocol messages*/
#define	TDU_CI_RESPONSE_TIMEOUT		0x101
#define	TDU_CI_SOCK_READ_ERROR		0x102
#define	TDU_CI_TC_DISCONNECT		0x103 /* lower layer disconnect */
#define	TDU_CI_EMPTY			0x104 /* zero sized TDU */


/* error return codes */
#define	TDU_ERR_TIMEOUT		1

/* TDU parameter identifiers according to ETS 300 075, table 5 */

#define	TDU_PI_USER_DATA		0x40
#define	TDU_PI_CALLED_ADDRESS		0x41
#define	TDU_PI_CALLING_ADDRESS		0x42
#define	TDU_PI_RESULT_REASON		0x43
#define	TDU_PI_ROLE_FUNCTION		0x44
#define	TDU_PI_APPLICATION_NAME		0x45
#define	TDU_PI_APPL_RESPONSE_TIMEOUT	0x46
#define	TDU_PI_SIZE_RECOVERY_WINDOW	0x47
#define	TDU_PI_DESIGNATION		0x48
#define	TDU_PI_NEW_NAME			0x49
#define	TDU_PI_REQUEST_IDENT		0x4a
#define	TDU_PI_IDENTIFICATION		0x4b
#define	TDU_PI_EX_CONF_1ST_LAST_BLNO	0x4c
#define	TDU_PI_TRANSFER_MODE		0x4d
#define	TDU_PI_RECOVERY_POINT		0x4f
#define	TDU_PI_SERVICE_CLASS		0x51

/* Bit coded parameter values */

#define TDU_BIT_ROLE         (0x01 << 0)
#define TDU_BIT_READ_RESTART (0x01 << 1)
#define TDU_BIT_TYPED_DATA   (0x01 << 2)
#define TDU_BIT_DIRECTORY    (0x01 << 3)
#define TDU_BIT_DELETE       (0x01 << 4)
#define TDU_BIT_RENAME       (0x01 << 5)
#define TDU_BIT_SAVE         (0x01 << 6)
#define TDU_BIT_LOAD         (0x01 << 7)

#define TDU_BIT_REQ_ID       (0x01 << 0)
#define TDU_BIT_FIRST_BLOCK  (0x01 << 0)
#define TDU_BIT_LAST_BLOCK   (0x01 << 1)
#define TDU_BIT_EXPL_CONF    (0x01 << 3)

#define TDU_BIT_SYMM_SERVICE  (0x01 << 1)
#define TDU_BIT_BASIC_KERNEL (0x01 << 0)

#define TDU_BIT_TRANSFER_MODE (0x01 << 0)

#define TDU_MASK_RECOVERY (0x01 << 0)
#define TDU_MASK_SIZE (0x07 << 1)
#define TDU_MASK_WINDOW (0x07 << 5)



struct tdu_fsm; /* forward */ 
struct tdu_buf; /* forward */ 
struct tdu_user; /* forward */ 

typedef int (*tdu_handler) ( struct tdu_fsm *, int event, struct tdu_buf * );
typedef int (*tdu_tb_processor) ( struct tdu_fsm *, struct tdu_buf * );


struct tdu_stream; /* forward */ 

/* State machine for processing T-protocol events. A stacked state space
   is used. Each established regime has its own state. The stack of
   currently established regimes and the corresponding regime states form
   the state of the state machine. For each regime different states are
   represented by different event parser functions that perform the state
   dependent actions upon receiving regime specific events */

struct no_regime {
	tdu_handler handler;
	int initiator; /* are we the initiator of the physical connection?*/
};

struct association_regime {
	tdu_handler handler, 
		    master_handler,
		    slave_handler;
	tdu_tb_processor
		    idle_assoc_req_processor;
	
	time_t resp_timeout; /* peers application response timeout */ 
	struct tdu_assoc_param local, remote;
};

struct access_regime {
	tdu_handler handler;
	tdu_handler idle_handler; /* default handler for idle state (depends
				     on access regime's role). Should be set
				     to one of the follwing */
	tdu_handler idle_master_handler,
		    idle_slave_handler ;
	struct tdu_access_param local, remote;
};

struct transfer_regime {
	tdu_handler handler;
	int pkt_cnt, byte_cnt, confirmed, last, outbound;
        struct timeval tv_begin, tv_end;
};

struct tdu_fsm {
	unsigned long flags;

	tdu_handler *regime_handler;

	struct no_regime idle;
	struct association_regime assoc;
	struct access_regime access;
	struct transfer_regime transfer;

	struct tdu_user   *user;
	struct tdu_stream *stream;
	int socket;
	time_t expires;      /* application response timer expires at */ 
	int up;
	int wait;            /* waiting for a response to command. This is
				redundant (the authoritative state is implied
				by the currently active event handler) but
				provided for convenience */
};


/* buffer used to compose tdu's. Some similarities with Linux kernel's sk_buff
   are not by chance -:) */
struct tdu_buf {
	unsigned char * data;  /* first valid data byte when composing,
				  first unparsed data byte when parsing */
	unsigned char * tail;  /* points after last valid data byte */
	unsigned char * ci;    /* CI byte */
	unsigned char * p0;    /* posistion of first PI byte in buffer */
	unsigned char * pn;    /* posistion of next PI byte to be parsed */
	unsigned char * md;    /* mass transfer data pointer */
	int eof;               /* to mark end of file condition */
	unsigned char head[TDU_MAX_BUF_LEN];  /* head of data storage */
	unsigned char end[0];   /* end of buffer pointer */
};


/* properties */

struct tdu_descr {
	char*	descr;
	int	code,
		sym_only;
};

extern unsigned char * tdu_print_cmd( int, unsigned char *, unsigned char *, int );
extern unsigned char * tdu_print_par( int, unsigned char *, unsigned char *, int, int );
extern unsigned char * tdu_print_file_header( int, unsigned char *, unsigned char *, int );
extern void tdu_print_txt( int, unsigned char * , unsigned char * );
extern void tdu_print_hex( int, unsigned char * , unsigned char * );
extern void tdu_print_li( int, unsigned char * , unsigned char * );

extern int wait_for_response( struct tdu_fsm* );


extern int tdu_select( struct tdu_fsm *, int, fd_set *,
		       fd_set *, fd_set *, struct timeval *);
extern void tdu_init_master(struct tdu_fsm *);
extern int tdu_wait_for_idle(struct tdu_fsm *, struct timeval *);
extern int tdu_wait_for_end_access(struct tdu_fsm *, struct timeval *);
extern int tdu_wait_for_release(struct tdu_fsm *, struct timeval *);
extern int tdu_wait (struct tdu_fsm *, struct timeval *, int (*)(struct tdu_fsm *) );
extern int tdu_wait_for_not_associating(struct tdu_fsm *, struct timeval *);
extern int in_idle_assoc_regime_master( struct tdu_fsm *);

extern int tdu_access_loading (struct tdu_fsm *, int event, struct tdu_buf *);
extern int tdu_access_saving  (struct tdu_fsm *, int event, struct tdu_buf *);

extern int tdu_assoc_req(struct tdu_fsm *, struct tdu_assoc_param *);
extern int tdu_access_req(struct tdu_fsm *, struct tdu_access_param *);
extern int tdu_access_received(struct tdu_fsm *, struct tdu_buf *);
extern void tdu_assoc_allow_listen(struct tdu_fsm *);
extern int tdu_t_dir_req( struct tdu_fsm *, struct tdu_transfer_param *);
extern int tdu_t_load_req( struct tdu_fsm *, struct tdu_transfer_param *);
extern int tdu_t_save_req( struct tdu_fsm *, struct tdu_transfer_param *);
extern int tdu_generic_response_received (struct tdu_fsm *, struct tdu_buf *);

extern int tdu_t_write_req(struct tdu_fsm *, struct tdu_buf *);
extern int tdu_t_write_end_req(struct tdu_fsm *, struct tdu_buf *);
extern int tdu_transfer_rej_req(struct tdu_fsm *, int, unsigned char *);
extern int tdu_end_access_req(struct tdu_fsm *);
extern int tdu_end_access_received(struct tdu_fsm *);
extern int tdu_release_req(struct tdu_fsm *a);
extern int tdu_abort_req(struct tdu_fsm *, int, unsigned char *);

extern int tdu_abort(struct tdu_fsm *);

extern void tdu_init_tb( struct tdu_buf * );
extern void tdu_transfer_abort(struct tdu_fsm * );
extern void tdu_initial_set_idle(struct tdu_fsm * );
extern void tdu_assoc_set_master(struct tdu_fsm * );
extern void tdu_assoc_set_slave(struct tdu_fsm * );
extern void tdu_assoc_set_idle(struct tdu_fsm * );
extern void tdu_access_set_idle(struct tdu_fsm * );
extern void tdu_access_set_master(struct tdu_fsm * );
extern void tdu_access_set_slave(struct tdu_fsm * );
extern const char * tdu_cmd_descr( int );
extern const char * tdu_param_descr( int );
extern int tdu_tr_sending( struct tdu_fsm *, int, struct tdu_buf *);
extern int tdu_tr_receiving( struct tdu_fsm *, int, struct tdu_buf *);
extern char * tdu_des( struct tdu_descr *, int );
extern void tdu_trans_start(struct tdu_fsm *);

extern int tdu_assoc_master( struct tdu_fsm *, int, struct tdu_buf *);
extern int tdu_assoc_slave( struct tdu_fsm *, int, struct tdu_buf *);
extern int tdu_await_end_access( struct tdu_fsm *, int, struct tdu_buf *);
extern int tdu_send_response_pos( struct tdu_fsm *, int);
extern int tdu_send_response_neg( struct tdu_fsm *, int, int, unsigned char *);
extern int tdu_send_p_exception(struct tdu_fsm *, int);
extern void tdu_start_timer(struct tdu_fsm *);
extern void tdu_del_timer(struct tdu_fsm *);

extern int tdu_stick_li( struct tdu_buf *, unsigned long );
extern int tdu_stick_le( struct tdu_buf *, unsigned long );
extern int tdu_add_reason( struct tdu_buf *, int, int, unsigned char *);
extern int tdu_add_ci_header( struct tdu_buf *, int);
extern int tdu_send_packet( struct tdu_buf *, struct tdu_fsm * );
extern int tdu_recv_packet( struct tdu_buf *, struct tdu_fsm * );
extern void tdu_set_default_assoc_param( struct tdu_assoc_param * );
extern void tdu_prompt(char *);

extern FILE * tdu_logfile;
extern unsigned int tdu_stderr_dirty;

extern int tdu_parse_byte(struct tdu_buf *);
extern int tdu_get_next_pi(struct tdu_buf *);
extern long tdu_parse_le(struct tdu_buf * tb);
extern long tdu_parse_li(struct tdu_buf * tb);
extern long tdu_parse_string(struct tdu_buf *, unsigned char *, long);
extern int tdu_add_string_par(struct tdu_buf *, int, unsigned char *,
			      unsigned int);
extern int tdu_add_byte_par(struct tdu_buf *, int, unsigned char);
extern int tdu_add_int_par(struct tdu_buf *, int, unsigned long);
extern int tdu_add_pi_header(struct tdu_buf *, int);
extern int tdu_check_response( struct tdu_buf *, int, int);
extern int tdu_p_except_req( struct tdu_fsm *, int );

extern int tdu_before_regime( struct tdu_fsm *);
extern int tdu_aborted( struct tdu_fsm *, int, struct tdu_buf *);
extern int tdu_echo_pos( struct tdu_fsm *, int, struct tdu_buf *);


extern char * (*tdu_extended_reason)(int);
extern int tdu_typed_data_received(struct tdu_fsm *, struct tdu_buf *);
extern const char * tdu_str_reason(int);
