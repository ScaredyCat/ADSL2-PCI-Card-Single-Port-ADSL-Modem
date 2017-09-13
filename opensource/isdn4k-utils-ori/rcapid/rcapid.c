/*
 * ISDN4Linux Remote-CAPI Server
 *
 * Copyright 1998 by Christian A. Lademann (cal@zls.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include	<sys/time.h>
#include	<sys/types.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<ctype.h>
#include        <string.h>
#include	"capicmd.h"
#include	"rcapicmd.h"
#include	"capi20.h"

#define	RCAPID_VERSION_INFO	"CAPI 2.0 / RCAPID by Christian A. Lademann"

#define	MAX_CAPIMSGLEN	3072 + 128 + 10 // 64 * 1024 does not seem to be necessary -- KG

#define LOGFILE_PATH_TEMPL	"/tmp/rcapid.log"
char	logfile_path [64];
FILE	*logfile = NULL;
int		loglevel = 0;

#define log(level,args...)	(void)(((level) <= loglevel) && \
						((logfile || (logfile = fopen(logfile_path, "a+"))) && \
						fprintf(logfile, "%d: ", getpid()) && \
						fprintf(logfile, ## args) && \
						fflush(logfile)))
char	logbuf [10240], *lptr;


typedef unsigned char	byte;
typedef unsigned short	word;
typedef unsigned int	dword;

typedef struct {
	int		len;
	char	*data;
}	structure;


struct capi_message {
	structure	raw;
	word		TotalLength,
				ApplID;
	byte		Command,
				Subcommand;
	word		MessageNumber;
	structure	Parameters;
};


word	remote_ApplID, local_ApplID;
int		capi_fd = -1;


#define	SIZE_TABMNR2DHDL	16
struct {
	int		used;
	word	MessageNumber;
	word	DataHandle;
}	tabMnr2Dhdl [SIZE_TABMNR2DHDL];

int	tabMnr2Dhdl_initd = 0;


int
setDataHandle(word mnr, word dhdl) {
	int	i;

	if(! tabMnr2Dhdl_initd) {
		for(i = 0; i < SIZE_TABMNR2DHDL; i++)
			tabMnr2Dhdl [i] .used = 0;

		tabMnr2Dhdl_initd = 1;
	}

	for(i = 0; i < SIZE_TABMNR2DHDL; i++)
		if(! tabMnr2Dhdl [i] .used) {
			tabMnr2Dhdl [i] .used = 1;
			tabMnr2Dhdl [i] .MessageNumber = mnr;
			tabMnr2Dhdl [i] .DataHandle = dhdl;

			log(9, "set mnr/dhdl (0x%04x, 0x%04x)\n", mnr, dhdl);

			return(0);
		}

	return(-1);
}


int
getDataHandle(word mnr, word *dhdl) {
	int	i;

	if(! tabMnr2Dhdl_initd)
		return(-1);

	for(i = 0; i < SIZE_TABMNR2DHDL; i++)
		if(tabMnr2Dhdl [i] .used && tabMnr2Dhdl [i] .MessageNumber == mnr) {
			*dhdl = tabMnr2Dhdl [i] .DataHandle;
			tabMnr2Dhdl [i] .used = 0;

			log(9, "get mnr/dhdl (0x%04x, 0x%04x)\n", mnr, *dhdl);
			return(0);
		}

	return(1);
}


void
log_hd(int level, char *buf, int len) {
	char			hline [50], aline [17], *p, *q;
	unsigned char	c;
	int				i;


	if(level > loglevel)
		return;

	memset(hline, 0, sizeof(hline));
	memset(aline, 0, sizeof(aline));

	p = hline;
	q = aline;

	for(i = 0; i < len; i++) {
		c = (unsigned char)buf [i];

		p += sprintf(p, "%02x ", c);
		if(i % 8 == 0)
			p += sprintf(p, " ");

		q += sprintf(q, "%c", (isprint(c) ? c : '.'));

		if(i == len - 1 || (i + 1) % 16 == 0) {
			log(level, "    %-50.50s%s\n", hline, aline);

			p = hline;
			q = aline;
		}
	}
}


byte
get_byte(char **p) {
	*p += 1;

	return((unsigned char)*(*p - 1));
}


word
get_word(char **p) {
	return(get_byte(p) | (get_byte(p) << 8));
}


word
get_netword(char **p) {
	return((get_byte(p) << 8) | get_byte(p));
}


dword
get_dword(char **p) {
	return(get_word(p) | (get_word(p) << 16));
}


char *
put_byte(char **p, byte val) {
	**p = val;
	*p += 1;
	return(*p);
}


char *
put_word(char **p, word val) {
	put_byte(p, val & 0xff);
	put_byte(p, (val & 0xff00) >> 8);
	return(*p);
}


char *
put_netword(char **p, word val) {
	put_byte(p, (val & 0xff00) >> 8);
	put_byte(p, val & 0xff);
	return(*p);
}


char *
put_dword(char **p, dword val) {
	put_word(p, val & 0xffff);
	put_word(p, (val & 0xffff0000) >> 16);
	return(*p);
}


char *
put_struct(char **p, int len, char *val) {
	if(len < 255)
		put_byte(p, len);
	else {
		put_byte(p, 0xff);
		put_word(p, len);
	}

	if(len) {
		memcpy(*p, val, len);
		*p += len;
	}

	return(*p);
}


int
getMessageFromBuffer(struct capi_message *msg, char *buf, int len) {
	char	*p = buf;

	msg->raw.len = len;
	msg->raw.data = buf;
	msg->TotalLength = get_word(&p);
	msg->ApplID = get_word(&p);
	msg->Command = get_byte(&p);
	msg->Subcommand = get_byte(&p);
	msg->MessageNumber = get_word(&p);
	if(p - buf < len) {
		msg->Parameters.len = p - buf + 1;
		msg->Parameters.data = p;
	} else {
		msg->Parameters.len = 0;
		msg->Parameters.data = NULL;
	}

	log(9, "gotMessageFromBuffer: cmd=(%02x/%02x)\n", msg->Command, msg->Subcommand);

	return(0);
}


int
rcv_message(struct capi_message *msg) {
	static char	buf [MAX_CAPIMSGLEN],
				*p;
	int			len, rlen;


	if(read(0, buf, 2) == 2) {
		p = buf;
		len = (int)get_netword(&p);

		log(5, "rcapi-message: len=%d (%2x%2x)\n", len, buf [0], buf [1]);
		len -= 2;
		if((rlen = read(0, buf, len)) != len) {
			log(5, "short message received! (received=%d, expected=%d)\n", rlen, len);
			/* exit(1); */
			return(-1);
		}

		log_hd(6, buf, len);

		getMessageFromBuffer(msg, buf, rlen);

		return(0);
	}

	return(-1);
}


int
snd_message(struct capi_message *orig, int cmd, structure *params, structure *data) {
	static char	buf [MAX_CAPIMSGLEN],
				*p;
	int			len, rlen, slen;


	len = 8;
	if(params)
		len += params->len;

	rlen = len + 2;
	if(data)
		rlen += data->len;

	p = buf;
	put_netword(&p, rlen);

	put_word(&p, len);
	put_word(&p, orig->ApplID);
	put_byte(&p, (cmd >> 8) & 0xff);
	put_byte(&p, cmd & 0xff);
	put_word(&p, orig->MessageNumber);

	if(params) {
		memcpy(p, params->data, params->len);
		p += params->len;
	}

	if(data) {
		memcpy(p, data->data, data->len);
		p += data->len;
	}

	log(5, "send rcapi-message: %d bytes\n", rlen);
	log_hd(6, buf, rlen);

	if((slen = write(1, buf, rlen)) < rlen) {
		log(5, "sending interrupted (%d of %d bytes).\n", slen, rlen);
		return(-1);
	}

	return(0);
}


int
hdl_RCAPI_REGISTER_REQ(struct capi_message *msg) {
	CAPI_REGISTER_ERROR	err;
	char	*p;
	dword	Buffer;
	word	messageBufferSize,
			maxLogicalConnections,
			maxBDataBlocks,
			maxBDataLen;
	byte	capiVersion;
	int	capierr = 0;

	structure	retstruct;
	char		retstr [8];


	if(! (p = msg->Parameters.data)) {
		log(5, "RCAPI_REGISTER_REQ: parameters missing.\n");
		return(-1);
	}

	Buffer = get_dword(&p);
	messageBufferSize = get_word(&p);
	maxLogicalConnections = get_word(&p);
	maxBDataBlocks = get_word(&p);
	maxBDataLen = get_word(&p);
	capiVersion = get_byte(&p);

	log(5, "RCAPI_REGISTER_REQ\n");
	log(5, "\tBuffer:                0x%x\n", Buffer);
	log(5, "\tmessageBufferSize:     %d\n", messageBufferSize);
	log(5, "\tmaxLogicalConnections: %d\n", maxLogicalConnections);
	log(5, "\tmaxBDataBlocks:        %d\n", maxBDataBlocks);
	log(5, "\tmaxBDataLen:           %d\n", maxBDataLen);
	log(5, "\tcapiVersion:           %d\n", capiVersion);
	
	
	p = retstr;
	if(capiVersion == 2) {
		if((err = CAPI20_REGISTER(maxLogicalConnections, maxBDataBlocks, maxBDataLen, &capi_fd)) < 0) {
			capierr = err;
			log(5, "registration not successful\n");
		} else {
			local_ApplID = capi_fd;
			log(5, "registration successful: appl_id = %d\n", capi_fd);
			capi_fd = capi20_fileno(capi_fd);
			log(5, "                         capi_fd = %d\n", capi_fd);
		}
	} else
		capierr = 0x0003;

	put_word(&p, capierr);

	retstruct.len = sizeof(word);
	retstruct.data = retstr;

	return(snd_message(msg, RCAPI_REGISTER_CONF, &retstruct, NULL));
}


int
hdl_RCAPI_GET_SERIAL_NUMBER_REQ(struct capi_message *msg) {
	char	*p;

	structure	retstruct;
	char		retval [80];
	char		serial_number [64];


	log(5, "RCAPI_GET_SERIAL_NUMBER_REQ\n");
	
	CAPI20_GET_SERIAL_NUMBER(0, serial_number);
	memset(serial_number + strlen(serial_number), 0, sizeof(serial_number) - strlen(serial_number));

	p = retval;
	put_struct(&p, 64, serial_number);

	retstruct.len = p - retval;
	retstruct.data = (char *)&retval;

	return(snd_message(msg, RCAPI_GET_SERIAL_NUMBER_CONF, &retstruct, NULL));
}


int
hdl_RCAPI_GET_MANUFACTURER_REQ(struct capi_message *msg) {
	char	*p;

	structure	retstruct;
	char		retval [80];
	char		manufacturer [64] = {0,};


	log(5, "RCAPI_GET_MANUFACTURER_REQ\n");

	CAPI20_GET_MANUFACTURER(0, manufacturer);

	p = retval;
	put_struct(&p, 64, manufacturer);

	retstruct.len = p - retval;
	retstruct.data = (char *)&retval;

	return(snd_message(msg, RCAPI_GET_MANUFACTURER_CONF, &retstruct, NULL));
}


int
hdl_RCAPI_GET_VERSION_REQ(struct capi_message *msg) {
	char	*p;

	structure	retstruct;
	char		retval [80];
	char		info [64];


	log(5, "RCAPI_GET_VERSION_REQ\n");
	
	memset(info, 0, sizeof(info));
	sprintf(info, "%s", RCAPID_VERSION_INFO);
	memset(info + strlen(info), 0, sizeof(info) - strlen(info));

	p = retval;
	put_byte(&p, 2);
	put_byte(&p, 0);
	put_byte(&p, 0);
	put_byte(&p, 0);
	put_struct(&p, 64, info);

	retstruct.len = p - retval;
	retstruct.data = (char *)&retval;

	return(snd_message(msg, RCAPI_GET_VERSION_CONF, &retstruct, NULL));
}


int
hdl_RCAPI_GET_PROFILE_REQ(struct capi_message *msg) {
	word	CtrlNr;
	char	*p;

	structure	retstruct;
	char		retval [80];


	log(5, "RCAPI_GET_PROFILE_REQ\n");
	
	if(! (p = msg->Parameters.data)) {
		log(5, "RCAPI_GET_PROFILE_REG: parameters missing.\n");
		return(-1);
	}

	CtrlNr = get_dword(&p);
	log(5, "\tCtrlNr: %d\n", CtrlNr);

	memset(retval, 0, sizeof(retval));
	*(unsigned short *)retval = CAPI20_GET_PROFILE(CtrlNr, retval+2); 
	retstruct.len = 66;
	retstruct.data = retval;

	return(snd_message(msg, RCAPI_GET_PROFILE_CONF, &retstruct, NULL));
}


int
hdl_RCAPI_AUTH_USER_REQ(struct capi_message *msg) 
{
	word	w1,w2,b1;
	char	*p;
	
	structure	retstruct;
	char		retval [80];

	
	log(5, "RCAPI_AUTH_USER_REQ\n");
	
	if(! (p = msg->Parameters.data)) {
		log(5, "RCAPI_AUTH_USER_REQ: parameters missing.\n");
		return(-1);
	}

	
	w1 = get_word(&p);
	w2 = get_word(&p);
	b1 = get_byte(&p);
	log(5, "w1 0x%4x w2 0x%4x b1 0x%2x", w1,w2,b1);

	p = retval;
#if 0
	put_word(&p, w1);
	put_word(&p, w2);
	put_byte(&p, b1);
	put_byte(&p, 0);
#endif
	put_word(&p, 0);
	put_word(&p, 0x19);
	put_word(&p, 0);
	
	retstruct.len = p - retval;
	retstruct.data = (char *)&retval;

	return(snd_message(msg, RCAPI_AUTH_USER_CONF, &retstruct, NULL));
}

int
hdl_raw(struct capi_message *msg) {
	char	*p;
	word	dhdl;

	if(capi_fd < 0)
		return(-1);

	p = msg->raw.data + 2;
	put_word(&p, local_ApplID);

	log(5, "CAPICMD = %x\n", (int)CAPICMD(msg->Command, msg->Subcommand));
	switch(CAPICMD(msg->Command, msg->Subcommand)) {
	case CAPI_DATA_B3_RESP:
		if(getDataHandle(msg->MessageNumber, &dhdl) == 0) {
			p = msg->raw.data + 12;
			put_word(&p, dhdl);
		}
	}

	log(5, "forward raw message: %d bytes, capi_fd = %d\n", msg->raw.len, capi_fd);
	log_hd(6, msg->raw.data, msg->raw.len);
	
	write(capi_fd, msg->raw.data, msg->raw.len);
	return(0);
}


int
hdl_message(struct capi_message *msg) {
	log(5, "message: Tl=%d, AI=%x, C=%x, Sc=%x, Mn=%d\n",
		msg->TotalLength,
		msg->ApplID,
		msg->Command,
		msg->Subcommand,
		msg->MessageNumber);

	remote_ApplID = msg->ApplID;

	switch(CAPICMD(msg->Command, msg->Subcommand)) {
	case RCAPI_REGISTER_REQ:
		return(hdl_RCAPI_REGISTER_REQ(msg));
		break;

	case RCAPI_GET_SERIAL_NUMBER_REQ:
		return(hdl_RCAPI_GET_SERIAL_NUMBER_REQ(msg));
		break;

	case RCAPI_GET_MANUFACTURER_REQ:
		return(hdl_RCAPI_GET_MANUFACTURER_REQ(msg));
		break;

	case RCAPI_GET_VERSION_REQ:
		return(hdl_RCAPI_GET_VERSION_REQ(msg));
		break;

	case RCAPI_GET_PROFILE_REQ:
		return(hdl_RCAPI_GET_PROFILE_REQ(msg));
		break;

	case RCAPI_AUTH_USER_REQ:
	        return(hdl_RCAPI_AUTH_USER_REQ(msg));
		break;

	default:
		msg->ApplID = local_ApplID;
		return(hdl_raw(msg));
		break;
	}
}


int main(int argc, char *argv []) {
	struct capi_message	msg;
	extern int	optind;
	extern char	*optarg;
	char		c;
	//	int		i;

	loglevel = 0;

	while((c = getopt(argc, argv, "l:")) != EOF)
		switch(c) {
		case 'l':	loglevel = atoi(optarg); break;
		}


	sprintf(logfile_path, "/tmp/rcapid.log");
	log(5, "rcapid started (PID=%d)\n", getpid());

	while(! feof(stdin)) {
		fd_set	ifd, efd;
		int				max_fd, s;


		FD_ZERO(&ifd);
		FD_ZERO(&efd);
		FD_SET(0, &ifd);
		FD_SET(0, &efd);
		max_fd = 1;
		if(capi_fd >= 0) {
			FD_SET(capi_fd, &ifd);
			max_fd = capi_fd + 1;
		}

		s = select(max_fd, &ifd, NULL, &efd, NULL);
		if(s <= 0) {
			log(5, "exit: %s, %d\n", __FILE__, __LINE__);
			exit(1);
		}

		if(FD_ISSET(0, &ifd)) {
			if(rcv_message(&msg) == 0)
				hdl_message(&msg);
			else
				exit(1);
		} 

		if(capi_fd >= 0 && FD_ISSET(capi_fd, &ifd)) {
			char	buf [MAX_CAPIMSGLEN], *p;
			int		len, i;
			struct capi_message	msg;
			word	dhdl;

			if((len = read(capi_fd, buf + 2, sizeof(buf) - 2)) <= 0)
				continue;
			
			p = buf;
			len += 2;
			put_netword(&p, len);
			p = buf + 4;
			put_word(&p, remote_ApplID);

			log(5, "forward raw answer: %d bytes\n", len);
			log_hd(6, buf, len);

			i = write(1, buf, len);
			log(5, "written %d bytes\n", i);

			getMessageFromBuffer(&msg, buf + 2, len - 2);
			switch(CAPICMD(msg.Command, msg.Subcommand)) {
			case CAPI_DATA_B3_IND:
				p = msg.raw.data + 18;
				dhdl = get_word(&p);
				setDataHandle(msg.MessageNumber, dhdl);
				break;
			}
		}

		if(FD_ISSET(0, &efd)) {
			log(5, "exit: %s, %d\n", __FILE__, __LINE__);
			exit(0);
		}
	}
	return 0;
}
