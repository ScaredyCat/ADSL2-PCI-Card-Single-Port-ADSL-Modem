#ifndef CBCP_H
#define CBCP_H

typedef struct cbcp_state {
    int    us_unit;	/* Interface unit number */
    u_char us_id;		/* Current id */
	int us_type;
	int us_allowed;
} cbcp_state;

extern struct protent cbcp_protent;

extern cbcp_state cbcp[];

#define CBCP_MINLEN 4

#define CBCP_REQ    1
#define CBCP_RESP   2
#define CBCP_ACK    3

#define CB_CONF_NO     1
#define CB_CONF_USER   2
#define CB_CONF_ADMIN  3
#define CB_CONF_LIST   4
#endif
