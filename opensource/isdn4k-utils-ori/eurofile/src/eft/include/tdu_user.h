
/*
  A (rather incomplete) implementation of the T-protocol
  defined in ETS 300 075. 

  This file containing stuff which the user of the T-protocol needs to
  be aware of.

  Copyright 1997 by Henner Eisen

  This software may be copied or modified under the terms of the
  GNU General Public Licence (GPL), Version 2, or any later version.

*/ 

#define LIMIT(val,lim) ( (val) <= (lim) ? val : lim ) 

#define E4L_VERSION "0.0.11"
extern const char e4l_version[];

/*
 * Identifiers in result/reason code parameters.
 * According to ETS 300 075 7.2.1 and 7.2.2
 */
#define TDU_RE_CALLED_ADDR_INCOR_P	0x30
#define TDU_RE_CALLED_ADDR_INCOR_U	0x40
#define TDU_RE_CALLING_ADDR_INCOR_P	0x31
#define TDU_RE_CALLING_ADDR_INCOR_U	0x41
#define TDU_RE_ROLE_REFUSED_P		0x32
#define TDU_RE_ROLE_REFUSED_U		0x42
#define TDU_RE_INSUFF_PRIMITIVES	0x43
#define TDU_RE_APPL_NAME_UNKNOWN	0x44
#define TDU_RE_SVC_CLASS_REFUSED_P	0x35
#define TDU_RE_SVC_CLASS_REFUSED_U	0x45
#define TDU_RE_ERRON_RECOV_POINT	0x46
#define TDU_RE_ERRON_DESIGN		0x47
#define TDU_RE_NO_ANSWER		0x48
#define TDU_RE_UNKNOWN_FILE		0x49
#define TDU_RE_FILE_EXISTS		0x4a
#define TDU_RE_ERRON_FILE		0x4b
#define TDU_RE_ERRON_NEW_NAME		0x4c
#define TDU_RE_NEW_NAME_IN_USE		0x5d
#define TDU_RE_WRONG_ID			0x50
#define TDU_RE_ERRON_UDATA		0x60
#define TDU_RE_SVC_UNKNOWN		0x61
#define TDU_RE_GROUP_FORBIDDEN		0x62
#define TDU_RE_OTHER_REASON		0x6f
#define TDU_RE_REPEATED_ERROR		0x70
#define TDU_RE_DELAY_EXPIRED		0x71
#define TDU_RE_UNKNOWN_MESSAGE		0x72
#define TDU_RE_SYNTAX_ERROR		0x73
#define TDU_RE_LOWER_LAYER_ERROR	0x74
#define TDU_RE_PROTOCOL_CONFLICT	0x75
#define TDU_RE_PRIMITIVE_NOT_HANDLED	0x76

/* Mask for classifying/controlling log file output */

#define TDU_LOG_IN	0x01 << 1	/* incoming frame */
#define TDU_LOG_IER	0x01 << 2	/* incoming frame indicating error */
#define TDU_LOG_OUT	0x01 << 3	/* outgoing frame */
#define TDU_LOG_OER	0x01 << 4	/* outgoing frame indicating error */
#define TDU_LOG_WRI	0x01 << 5	/* t_write frame */
#define TDU_LOG_REW	0x01 << 6	/* pos response to t_write frame */
#define TDU_LOG_TRC	0x01 << 7	/* function call trace */
#define TDU_LOG_WAIT	0x01 << 8	/* possibly blocking operations */
#define TDU_LOG_LOG	0x01 << 9	/* other informational log meessage */
#define TDU_LOG_DBG	0x01 << 10	/* misc other debug messages */
#define TDU_LOG_ERR	0x01 << 11	/* error message to be printed */
#define TDU_LOG_WARN	0x01 << 12	/* warning message to be printed */
#define TDU_LOG_FH	0x01 << 13	/* file header contents printed */
#define TDU_LOG_HASH	0x01 << 14	/* hash mark printing for each packet*/
#define TDU_LOG_TMP	0x01 << 15	/* for temorarily inserted debugging
					 * output messages */
#define TDU_LOG_ISDNLOG	0x01 << 16	/* used to switch on isdn4linux 
					 * /dev/isdnlog logging */
/* 
 * These can be used by the application which uses the T-protocol (i.e. eftd)
 * for defining their private logging facility.
 */
#define TDU_LOG_AP1	0x01 << 17	/* application level 1 log messages */
#define TDU_LOG_AP2 	0x01 << 18	/* application level 2 log messages */
#define TDU_LOG_AP3 	0x01 << 19	/* application level 3 log messages */


#define TDU_PLEN_ADDR 254
#define TDU_PLEN_APPLNAME 16
#define TDU_PLEN_IDENT 31
#define TDU_PLEN_UDATA 254
#define TDU_PLEN_DESIGNATION 254
#define TDU_PLEN_NEW_NAME 254
#define TDU_PLEN_OTHER_REASON 62

#define TDU_ROLE_MASTER 0x01
#define TDU_ROLE_SLAVE  0x00





struct tdu_stream;
struct tdu_buf;

struct tdu_assoc_param {
	unsigned char called_addr[TDU_PLEN_ADDR+1];
	int called_len;
	unsigned char calling_addr[TDU_PLEN_ADDR+1];
	int calling_len;
	unsigned char appl_name[TDU_PLEN_APPLNAME+1];
	int appl_len;
	unsigned int resp_timeout;
	unsigned char symm_service;
	unsigned char basic_kernel;
	int expl_conf;
	unsigned char ident[TDU_PLEN_IDENT+1];
	int ident_len;
	int req_ident;
	unsigned char udata[TDU_PLEN_UDATA+1];
	int udata_len;
};

struct tdu_access_param {
	unsigned int role;
	unsigned int functions;
	unsigned int transfer_size;
	unsigned int window;
	unsigned int recovery;
	unsigned int transfer_mode;
	unsigned char udata[TDU_PLEN_UDATA+1];
	int udata_len;
};


struct tdu_udata_param {
	unsigned char udata[TDU_PLEN_UDATA+1];
	int udata_len;
};


struct tdu_transfer_param {
	unsigned char udata[TDU_PLEN_UDATA+1];
	int udata_len;
	unsigned char designation[TDU_PLEN_DESIGNATION+1];
	int designation_len;
	int recovery_point;
	unsigned char new_name[TDU_PLEN_NEW_NAME+1];
	int new_len;
};

typedef	union {
	struct tdu_assoc_param * assoc;
	struct tdu_access_param * access;
	struct tdu_udata_param * udata;
	struct tdu_transfer_param * transfer;
	struct tdu_stream * stream;
} tdu_par_union;

struct tdu_param {
	int reason;
	unsigned char other_reason[TDU_PLEN_OTHER_REASON+1];
	int reason_len;
	tdu_par_union res;
	tdu_par_union par;
};

/* 
 * This object provides callbacks for t-protocol indications received from
 * the peer.
 */
struct tdu_user{
	struct tdu_fsm * fsm;
	int (*t_associate)(struct tdu_user *, struct tdu_param *);
	int (*t_access)(struct tdu_user *, struct tdu_param *);
	int (*t_end_access)(struct tdu_user *, struct tdu_param *);
	int (*t_release)(struct tdu_user *, struct tdu_param *);
	int (*t_abort)(struct tdu_user *, struct tdu_param *);
	int (*t_dir)(struct tdu_user *, struct tdu_param *);
	int (*t_load)(struct tdu_user *, struct tdu_param *);
	int (*t_save)(struct tdu_user *, struct tdu_param *);
	int (*t_rename)(struct tdu_user *, struct tdu_param *);
	int (*t_delete)(struct tdu_user *, struct tdu_param *);
	int (*t_typed_data)(struct tdu_user *, struct tdu_param *);
	void (*transfer_end)(struct tdu_user *, int);
	int read_restart;
	void *priv;
};

struct tdu_header{
	int (*parse) (struct tdu_stream *, struct tdu_buf *, int);
	int (*read) (struct tdu_stream *, unsigned char *, int);
	int len;
	void * data;
};

/* 
 * This object processes mass transfer related t-protocol indications
 * received via the T-protocol's T-Write command
 */
struct tdu_stream {
	int fd;
	struct tdu_fsm * fsm;
	int (*t_write)     ( struct tdu_stream*, struct tdu_buf *);
	int (*t_write_end) ( struct tdu_stream*, struct tdu_buf *);
	int (*t_read_restart) ( struct tdu_stream* );
	int (*abort)       ( struct tdu_stream* );
	int (*close)       ( struct tdu_stream* ); /* signals end of transfer*/
 	int (*read)        ( struct tdu_stream*, struct tdu_buf * );

	struct tdu_header hdr;
};

extern unsigned int tdu_stderr_mask, tdu_logfile_mask;
extern int tdu_dispatch_packet(struct tdu_fsm *);
extern void tdu_init_slave(struct tdu_fsm *fsm);
extern int tdu_typed_data_req(struct tdu_fsm *, unsigned char *);

extern struct tdu_stream* tdu_user_get_stream(   struct tdu_user*);
extern struct tdu_user* tdu_stream_get_user(   struct tdu_stream*);
extern struct tdu_stream* tdu_user_attach_stream(struct tdu_user*,
						 struct tdu_stream *);
extern int tdu_std_abort (struct tdu_stream *);
extern int tdu_no_close (struct tdu_stream *);

extern int tdu_fd_read  (struct tdu_stream *, struct tdu_buf *);
extern int tdu_fd_close (struct tdu_stream *);
extern int tdu_fd_t_write (struct tdu_stream *, struct tdu_buf *);
extern int tdu_fd_t_write_end (struct tdu_stream *, struct tdu_buf *);


extern void tdu_set_default_access_param( struct tdu_access_param *,
					  struct tdu_user*,
					  struct tdu_stream * );

extern void tdu_user_set_initiator(struct tdu_user *, int);
extern void tdu_stream_init_fd(struct tdu_stream *, int fd, int close);
extern int tdu_printf( int, char *, ... );
extern void tdu_open_isdnlog(char *);
extern void tdu_isdnlog();
extern int tdu_log_prefix(const char *, const char*);
extern int tdu_open_log(const char * fname);
extern int tdu_mk_printable(unsigned char *, const unsigned char *, int len);
