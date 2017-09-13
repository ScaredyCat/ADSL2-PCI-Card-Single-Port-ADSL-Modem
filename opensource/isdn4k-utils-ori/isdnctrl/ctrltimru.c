/*
 * ISDN TimRu-Control
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "config.h"
#ifdef HAVE_TIMRU
#include <linux/isdn_timru.h>
#include <linux/isdn_budget.h>
#endif

#include "isdnctrl.h"
#include "ctrlconf.h"

#ifdef HAVE_TIMRU
/* TimRu - Erweiterung */
/*
06.06.97:cal:turnaround_rule(): bei nicht-umkehrbarer Regel mit 0 zurueckkehren
10.06.97:cal:hdl_timeout_rule(): memset(&timru, 0, ...)
12.06.97:cal:commandline-parsing kompatibel zur Hauptschleife, damit mehrere
	Kommandos hintereinander angegeben werden koennen; id und cmdptr werden
	an die betreffenden Funktionen uebergeben.
26.06.97:cal:neben "*" auch "any" als Wildcard-Angabe und neben "!" auch "not"
	als Negations-Angabe zulassen; bei "bringup" jetzt auch hup-timeout als
	initial-timeout einlesen.
09.06.98:cal:Mini-Glibc-Patch von Andreas Steffan <deas@uni-hamburg.de> eing.
*/

char	*timru_protfam_kt [] =	{ "ip", "ipx", "ppp", "*", "any", "\0" };
int	timru_protfam_nt [] =	{ ISDN_TIMRU_PROTFAM_IP,
				  ISDN_TIMRU_PROTFAM_IPX,
				  ISDN_TIMRU_PROTFAM_PPP,
				  ISDN_TIMRU_PROTFAM_WILDCARD,
				  ISDN_TIMRU_PROTFAM_WILDCARD,
				  -1 };

char	*timru_ip_kt [] =	{ "icmp", "tcp", "udp", "*", "any", "\0" };
int	timru_ip_nt [] =	{ ISDN_TIMRU_IP_ICMP,
				  ISDN_TIMRU_IP_TCP,
				  ISDN_TIMRU_IP_UDP,
				  ISDN_TIMRU_IP_WILDCARD,
				  ISDN_TIMRU_IP_WILDCARD,
				  -1 };

char	*timru_ppp_kt [] =	{ "ipcp", "ipxcp", "ccp", "lcp", "pap", "lqr", 
				  "chap", "*", "any", "\0" };
int	timru_ppp_nt [] =	{ ISDN_TIMRU_PPP_IPCP,
				  ISDN_TIMRU_PPP_IPXCP,
				  ISDN_TIMRU_PPP_CCP,
				  ISDN_TIMRU_PPP_LCP,
				  ISDN_TIMRU_PPP_PAP,
				  ISDN_TIMRU_PPP_LQR,
				  ISDN_TIMRU_PPP_CHAP,
				  ISDN_TIMRU_PPP_WILDCARD,
				  ISDN_TIMRU_PPP_WILDCARD,
				  -1 };


char	*timru_ruletype_kt [] =	{ "bringup", "keepup_in", "keepup_out", "\0" };
int	timru_ruletype_nt [] =	{ ISDN_TIMRU_BRINGUP,
				  ISDN_TIMRU_KEEPUP_IN,
				  ISDN_TIMRU_KEEPUP_OUT,
				  -1 };

#define	MIN_IP_PORT	0
#define	MAX_IP_PORT	0xffff

#define	MIN_ICMP_TYPE	0
#define	MAX_ICMP_TYPE	0xff

int	flush_timeout_rules(int, char *, int, int, int, char *[]);
int	output_rule_header(int, int);
int	output_timeout_rules(int, char *, int, int, char *[]);
int	output_timeout_rule(isdn_ioctl_timeout_rule *);
int	read_icmp_type(char *, __u8 *, __u8 *);
int	read_ip_addr(char *, struct in_addr *, struct in_addr *);
int	read_ip_port(char *, char *, __u16 *, __u16 *);
int	set_default_timeout(int, char *, int, int, char *[]);
int	turnaround_rule(isdn_ioctl_timeout_rule *);
int	write_icmp_type(char *, __u8, __u8);
int	write_ip_addr(char *, struct in_addr, struct in_addr);
int	write_ip_port(char *, __u16, __u16);


int
read_ip_addr(char *s, struct in_addr *a, struct in_addr *m) {
	char		host [256],
			mask [16];
	int		a1, a2, a3, a4;
	struct hostent	*he;
	struct netent	*ne;


	if(!strcmp(s, "*") || !strcmp(s, "any")) {
		a->s_addr = (__u32)htonl(0);
		m->s_addr = (__u32)htonl(0);

		return(0);
	}

	if(strchr(s, '/')) {
		strcpy(host, strtok(s, "/"));
		strcpy(mask, strtok(NULL, "/"));
	} else {
		strcpy(host, s);
		strcpy(mask, "0");
	}

	if(! strcmp(host, "0"))
		a->s_addr = (__u32)htonl(0x00000000);
	else if(sscanf(host, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4)
		a->s_addr = (__u32)htonl((((a1&0xff)<<24)|((a2&0xff)<<16)|((a3&0xff)<<8)|(a4&0xff)));
	else if((ne = getnetbyname(s)) && ne->n_addrtype == AF_INET)
		a->s_addr = (__u32)htonl(ne->n_net);
	else if((he = gethostbyname(s)) && he->h_addrtype == AF_INET)
		memcpy(a, he->h_addr_list [0], sizeof(struct in_addr));
	else
		return(-1);

	if(sscanf(mask, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) == 4)
		m->s_addr = (__u32)htonl((((a1&0xff)<<24)|((a2&0xff)<<16)|((a3&0xff)<<8)|(a4&0xff)));
	else if(sscanf(mask, "%d", &a1) == 1) {
		int	i;

		m->s_addr = (__u32)htonl(0);

		for(i = a1; i > 0; i--)
			m->s_addr |= 1 << (32 - i);

		m->s_addr = (__u32)htonl(m->s_addr);
	} else
		return(-1);

	return(0);
}


int
write_ip_addr(char *s, struct in_addr a, struct in_addr m) {
	int	a1, a2, a3, a4, m1, m2, m3, m4,
		i, mbits, in_mask;


	if(a.s_addr == (__u32)ntohl(0))
		sprintf(s, "0");
	else {
		a1 = (ntohl(a.s_addr) >> 24) & 0xff;
		a2 = (ntohl(a.s_addr) >> 16) & 0xff;
		a3 = (ntohl(a.s_addr) >> 8) & 0xff;
		a4 = ntohl(a.s_addr) & 0xff;

		sprintf(s, "%d.%d.%d.%d", a1, a2, a3, a4);
	}

	mbits = 0;
	in_mask = 1;
	for(i = 31; i >= 0; i--) {
		if((((1 << i) & ntohl(m.s_addr)) != 0) == (in_mask != 0)) {
			if(in_mask)
				mbits++;
		} else {
			if(in_mask)
				in_mask = 0;
			else
				break;
		}
	}

	if(i < 0)
		sprintf(s, "%s/%d", s, mbits);
	else {
		m1 = (ntohl(m.s_addr) >> 24) & 0xff;
		m2 = (ntohl(m.s_addr) >> 16) & 0xff;
		m3 = (ntohl(m.s_addr) >> 8) & 0xff;
		m4 = ntohl(m.s_addr) & 0xff;

		sprintf(s, "%s/%d.%d.%d.%d", s, m1, m2, m3, m4);
	}

	return(0);
}


int
read_ip_port(char *s, char *proto, __u16 *from, __u16 *to) {
	int		f, t, n;
	char		*q;
	struct servent	*service;


	if(!strcmp(s, "*") || !strcmp(s, "any")) {
		f = MIN_IP_PORT;
		t = MAX_IP_PORT;
	} else {
		if(proto && (service = getservbyname(s, proto))) /* specific port by name */
			f = t = ntohs(service->s_port);
		else
			if((q = strchr(s, '-'))) {	/* port range */
				if(q - s == 0)	/* -to: empty from */
					f = MIN_IP_PORT;
				else {
					if(sscanf(s, "%i%n", &f, &n) <= 0 || n != q - s)
						return(-1);
				}
			
				if(! *(q + 1))	/* from-: empty to */
					t = MAX_IP_PORT;
				else {
					if(sscanf(q + 1, "%i%n", &t, &n) <= 0 || n != strlen(q + 1))
						return(-1);
				}
			} else {	/* specific port */
				if(sscanf(s, "%i%n", &f, &n) > 0 || n == strlen(s))
					t = f;
				else
					return(-1);
			}
	}

	if(f < MIN_IP_PORT || f > MAX_IP_PORT || t < MIN_IP_PORT || t > MAX_IP_PORT)
		return(-1);

	*from = (__u16)htons(f);
	*to = (__u16)htons(t);

	return(0);
}


int
write_ip_port(char *s, __u16 from, __u16 to) {
	if(ntohs(from) == MIN_IP_PORT && ntohs(to) == MAX_IP_PORT)
		sprintf(s, "*");
	else if(from == to)
		sprintf(s, "%d", ntohs(from));
	else
		sprintf(s, "%d-%d", ntohs(from), ntohs(to));

	return(0);
}


int
read_icmp_type(char *s, __u8 *from, __u8 *to) {
	int		f, t, n;
	char		*q;


	if(!strcmp(s, "*") || !strcmp(s, "any")) {
		f = MIN_ICMP_TYPE;
		t = MAX_ICMP_TYPE;
	} else {
		if((q = strchr(s, '-'))) {	/* port range */
			if(q - s == 0)	/* -to: empty from */
				f = MIN_ICMP_TYPE;
			else {
				if(sscanf(s, "%i%n", &f, &n) <= 0 || n != q - s)
					return(-1);
			}
			
			if(! *(q + 1))	/* from-: empty to */
				t = MAX_ICMP_TYPE;
			else {
				if(sscanf(q + 1, "%i%n", &t, &n) <= 0 || n != strlen(q + 1))
					return(-1);
			}
		} else {	/* specific port */
			if(sscanf(s, "%i%n", &f, &n) > 0 || n == strlen(s))
				t = f;
			else
				return(-1);
		}
	}

	if(f < MIN_ICMP_TYPE || f > MAX_ICMP_TYPE || t < MIN_ICMP_TYPE || t > MAX_ICMP_TYPE)
		return(-1);

	*from = (__u8)f;
	*to = (__u8)t;

	return(0);
}


int
write_icmp_type(char *s, __u8 from, __u8 to) {
	if(from == MIN_ICMP_TYPE && to == MAX_ICMP_TYPE)
		sprintf(s, "*");
	else if(from == to)
		sprintf(s, "%d", from);
	else
		sprintf(s, "%d-%d", from, to);

	return(0);
}


#define	SHIFT_ARGS(msg)	{ \
	if(++argp >= argc) { \
		fprintf(stderr, "rule-parser: out of arguments"); \
		if(strlen(msg)) \
			fprintf(stderr, " looking for %s", (msg)); \
		fprintf(stderr, "\n"); \
		return(-1); \
	} else { \
		++args_used; \
	} \
}


#define	UNSHIFT_ARGS(i) { \
	if(argp > (i)) { \
		argp -= (i); \
		args_used -= (i); \
	} \
}


#define	PARSER_ERROR(msg)	{ \
	fprintf(stderr, "rule-parser: parse-error"); \
	if(strlen(msg)) \
		fprintf(stderr, ": %s", (msg)); \
	fprintf(stderr, "\n"); \
	return(-1); \
}

#define max(a, b)	((a) > (b) ? (a) : (b))


int
hdl_timeout_rule(int fd, char *id, int cmd, int argc, char *argv []) {
	isdn_ioctl_timeout_rule	timru;
	int			ioctl_fcn,
				n_protfam, n_prot,
				ioret, n,
				other_type = -1,
				argp = 0,
				args_used = 0;
	char		k_protfam [16], k_prot [16];


	memset(&timru, 0, sizeof(isdn_ioctl_timeout_rule));

	/* cmd */
	switch(cmd) {
	case ADDRULE:
		ioctl_fcn = IIOCNETARU;
		timru.where = 1;
		break;

	case INSRULE:
		ioctl_fcn = IIOCNETARU;
		timru.where = 0;
		break;

	case DELRULE:
		ioctl_fcn = IIOCNETDRU;
		break;

	case DEFAULT:
		return(args_used + set_default_timeout(fd, id, argp, argc, argv));
		break;

	case SHOWRULES:
		return(args_used + output_timeout_rules(fd, id, argp, argc, argv));
		break;

	case FLUSHRULES:
		return(args_used + flush_timeout_rules(fd, id, 0, argp, argc, argv));
		break;

	case FLUSHALLRULES:
		return(args_used + flush_timeout_rules(fd, id, 1, argp, argc, argv));
		break;

	default:
		PARSER_ERROR("unknown command");
	}

	/* interface */
	strncpy(timru.name, id, sizeof(timru.name) - 1);

	/* rule-type */
	/* SHIFT_ARGS("rule-type"); */
	if(! strcmp(argv [argp], "bringup")) {
		timru.rule.type = ISDN_TIMRU_BRINGUP;
	} else if(! strcmp(argv [argp], "keepup")) {
		/* keepup-type */
		SHIFT_ARGS("keepup-type");
		if(! strcmp(argv [argp], "in"))
			timru.rule.type = ISDN_TIMRU_KEEPUP_IN;
		else if(! strcmp(argv [argp], "out"))
			timru.rule.type = ISDN_TIMRU_KEEPUP_OUT;
		else if(! strcmp(argv [argp], "both")) {
			timru.rule.type = ISDN_TIMRU_KEEPUP_IN;
			other_type = ISDN_TIMRU_KEEPUP_OUT;
		} else
			PARSER_ERROR("wrong keepup-type");
	} else
		PARSER_ERROR("wrong rule-type");

	/* hup-timeout */
	SHIFT_ARGS("hup-timeout");
	if(sscanf(argv [argp], "%i%n", &timru.rule.timeout, &n) <= 0 || n != strlen(argv [argp]))
		PARSER_ERROR("wrong timeout");

	/* neg */
	SHIFT_ARGS("rule-parameter");
	if(! strcmp(argv [argp], "!") || ! strcmp(argv [argp], "not")) {
		timru.rule.neg = 1;
		SHIFT_ARGS("rule-parameter");
	} else
		timru.rule.neg = 0;

	/* rule */
	if(strchr(argv [argp], '/')) {
		if(sscanf(argv [argp], "%[a-z0-9*]/%[a-z0-9*]", k_protfam, k_prot) != 2)
			PARSER_ERROR("wrong protocol specification");
	} else {
		if(! strcmp(argv [argp], "any")) {
			strcpy(k_protfam, "any");
			strcpy(k_prot, "any");
		} else
			PARSER_ERROR("wrong protocol specification");
	}

	if((n_protfam = key2num(k_protfam, timru_protfam_kt, timru_protfam_nt)) < 0)
		PARSER_ERROR("wrong protocol-family");

	timru.rule.protfam = n_protfam;

	switch(timru.rule.protfam) {
	case ISDN_TIMRU_PROTFAM_IP:
		if((n_prot = key2num(k_prot, timru_ip_kt, timru_ip_nt)) < 0)
			PARSER_ERROR("wrong ip-protocol");

		timru.rule.rule.ip.protocol = n_prot;

		/* src-addr */
		SHIFT_ARGS("source-address");
		if(read_ip_addr(argv [argp], &timru.rule.rule.ip.saddr, &timru.rule.rule.ip.smask) < 0)
			PARSER_ERROR("wrong source-address");

		switch(timru.rule.rule.ip.protocol) {
		case ISDN_TIMRU_IP_ICMP:
			/* port-range */
			SHIFT_ARGS("icmp-type(s)");
			if(read_icmp_type(argv [argp], &timru.rule.rule.ip.pt.type.from, &timru.rule.rule.ip.pt.type.to) < 0)
				PARSER_ERROR("wrong icmp-type(s)");
			break;

		case ISDN_TIMRU_IP_TCP:
		case ISDN_TIMRU_IP_UDP:
			/* service-range */
			SHIFT_ARGS("source port-number(s)");
			if(read_ip_port(argv [argp], k_prot, &timru.rule.rule.ip.pt.port.s_from, &timru.rule.rule.ip.pt.port.s_to) < 0)
				PARSER_ERROR("wrong source port-number(s)");
			break;
		}

		/* dst-addr */
		SHIFT_ARGS("destination-address");
		if(read_ip_addr(argv [argp], &timru.rule.rule.ip.daddr, &timru.rule.rule.ip.dmask) < 0)
			PARSER_ERROR("wrong destination-address");

		switch(timru.rule.rule.ip.protocol) {
		case ISDN_TIMRU_IP_TCP:
		case ISDN_TIMRU_IP_UDP:
			/* service-range */
			SHIFT_ARGS("destination port-number(s)");
			if(read_ip_port(argv [argp], k_prot, &timru.rule.rule.ip.pt.port.d_from, &timru.rule.rule.ip.pt.port.d_to) < 0)
				PARSER_ERROR("wrong destination port-number(s)");
			break;
		}

		break;
	
	case ISDN_TIMRU_PROTFAM_PPP:
		if((n_prot = key2num(k_prot, timru_ppp_kt, timru_ppp_nt)) < 0)
			PARSER_ERROR("wrong ppp-protocol");

		timru.rule.rule.ppp.protocol = n_prot;
		break;
	}

	if((ioret = ioctl(fd, ioctl_fcn, &timru)) < 0) {
		perror("ioctl failed.");
		return(ioret);
	}

	if(other_type >= 0) {
		if(turnaround_rule(&timru) < 0)
			return(-1);

		timru.rule.type = other_type;

		if((ioret = ioctl(fd, ioctl_fcn, &timru)) < 0) {
			perror("ioctl failed.");
			return(ioret);
		}
	}

	return(args_used);
}


int
turnaround_rule(isdn_ioctl_timeout_rule *timru) {
	isdn_ioctl_timeout_rule	timbuf;

	memcpy((char *)&timbuf, (char *)timru, sizeof(isdn_ioctl_timeout_rule));
	switch(timru->rule.protfam) {
	case ISDN_TIMRU_PROTFAM_IP:
		memcpy((char *)&timru->rule.rule.ip.saddr, (char *)&timbuf.rule.rule.ip.daddr, sizeof(timbuf.rule.rule.ip.daddr));
		memcpy((char *)&timru->rule.rule.ip.smask, (char *)&timbuf.rule.rule.ip.dmask, sizeof(timbuf.rule.rule.ip.dmask));
		memcpy((char *)&timru->rule.rule.ip.daddr, (char *)&timbuf.rule.rule.ip.saddr, sizeof(timbuf.rule.rule.ip.saddr));
		memcpy((char *)&timru->rule.rule.ip.dmask, (char *)&timbuf.rule.rule.ip.smask, sizeof(timbuf.rule.rule.ip.smask));

		switch(timru->rule.rule.ip.protocol) {
		case ISDN_TIMRU_IP_TCP:
		case ISDN_TIMRU_IP_UDP:
			timru->rule.rule.ip.pt.port.s_from = timbuf.rule.rule.ip.pt.port.d_from;
			timru->rule.rule.ip.pt.port.s_to = timbuf.rule.rule.ip.pt.port.d_to;
			timru->rule.rule.ip.pt.port.d_from = timbuf.rule.rule.ip.pt.port.s_from;
			timru->rule.rule.ip.pt.port.d_to = timbuf.rule.rule.ip.pt.port.s_to;
			break;
		}
		break;
	}
	return(0);
}


int
output_timeout_rules(int fd, char *id, int firstarg, int argc, char *argv []) {
	isdn_ioctl_timeout_rule	timru;
	int			i, j, k,
				header_printed;


	/* interface */
	strncpy(timru.name, id, sizeof(timru.name) - 1);

	printf("Timeout rules for interface %s:\n", timru.name);

	for(i = 0; i < ISDN_TIMRU_NUM_CHECK; i++) {
		timru.where = -1;
		timru.type = i;

		if(ioctl(fd, IIOCNETGRU, &timru) == 0) {
			switch(i) {
			case ISDN_TIMRU_BRINGUP:
				printf("Default bringup policy: ");
				if(timru.defval == 0)
					printf("false\n");
				else
					printf("true, initial timeout is %d sec.\n", timru.defval);
				break;

			case ISDN_TIMRU_KEEPUP_IN:
				printf("Default huptimeout for incoming packets: %d sec.\n", timru.defval);
				break;

			case ISDN_TIMRU_KEEPUP_OUT:
				printf("Default huptimeout for outgoing packets: %d sec.\n", timru.defval);
				printf("\nCurrent huptimeout: %d sec.\n", timru.index);
				printf("Time until hangup: %d sec.\n", max(0, timru.index - timru.protfam));
				break;
			}
		}
	}

	for(i = 0; i < ISDN_TIMRU_NUM_CHECK; i++) {
		header_printed = 0;
		for(j = 0; j < ISDN_TIMRU_NUM_PROTFAM; j++) {
			k = 0;
			header_printed = 0;
			while(1) {
				timru.type = i;
				timru.protfam = j;
				timru.index = k;
				timru.where = 0;

				if(ioctl(fd, IIOCNETGRU, &timru) == 0) {
					if(! header_printed) {
						output_rule_header(i, j);
						header_printed = 1;
					}
					output_timeout_rule(&timru);
				} else
					break;

				k++;
			}
		}
	}

	return(0);
}


int
output_timeout_rule(isdn_ioctl_timeout_rule *timru) {
	char	str [80];

	/* rule-nr */
	printf("%d-%d-%-d ", timru->type, timru->protfam, timru->index);

	/* rule-type */
	printf("%s ", num2key(timru->rule.type, timru_ruletype_kt, timru_ruletype_nt));
	
	/* timeout */
	if(timru->rule.type != ISDN_TIMRU_BRINGUP)
		printf("%d ", timru->rule.timeout);

	/* neg */
	printf("%s", (timru->rule.neg ? "!" : ""));

	/* protocol */
	printf("%s", num2key(timru->rule.protfam, timru_protfam_kt, timru_protfam_nt));
	switch(timru->rule.protfam) {
	case ISDN_TIMRU_PROTFAM_IP:
		/* protocol */
		printf("/%s ", num2key(timru->rule.rule.ip.protocol, timru_ip_kt, timru_ip_nt));

		/* src addr */
		write_ip_addr(str, timru->rule.rule.ip.saddr, timru->rule.rule.ip.smask);
		printf("%s ", str);

		/* src port/type */
		strcpy(str, "\0");
		switch(timru->rule.rule.ip.protocol) {
		case ISDN_TIMRU_IP_ICMP:
			write_icmp_type(str, timru->rule.rule.ip.pt.type.from, timru->rule.rule.ip.pt.type.to);
			printf("%s ", str);
			break;
		
		case ISDN_TIMRU_IP_TCP:
		case ISDN_TIMRU_IP_UDP:
			write_ip_port(str, timru->rule.rule.ip.pt.port.s_from, timru->rule.rule.ip.pt.port.s_to);
			printf("%s ", str);
			break;
		}

		/* dst addr */
		write_ip_addr(str, timru->rule.rule.ip.daddr, timru->rule.rule.ip.dmask);
		printf("%s ", str);

		/* dst port */
		strcpy(str, "\0");
		switch(timru->rule.rule.ip.protocol) {
		case ISDN_TIMRU_IP_TCP:
		case ISDN_TIMRU_IP_UDP:
			write_ip_port(str, timru->rule.rule.ip.pt.port.d_from, timru->rule.rule.ip.pt.port.d_to);
			printf("%s ", str);
			break;
		}
		break;

	case ISDN_TIMRU_PROTFAM_PPP:
		printf("/%s", num2key(timru->rule.rule.ppp.protocol, timru_ppp_kt, timru_ppp_nt));
		break;

	default:
		printf(" ");
		break;
	}

	printf("\n");

	return(0);
}


int
output_rule_header(int rule_type, int protfam) {
	printf("\n");
	switch(rule_type) {
	case ISDN_TIMRU_BRINGUP:	printf("Bringup-rules for outgoing "); break;
	case ISDN_TIMRU_KEEPUP_IN:	printf("Keepup-rules for incoming "); break;
	case ISDN_TIMRU_KEEPUP_OUT:	printf("Keepup-rules for outgoing "); break;
	}
	printf("%s-packets:\n", num2key(protfam, timru_protfam_kt, timru_protfam_nt));

	return(0);
}


int
flush_timeout_rules(int fd, char *id, int all, int firstarg, int argc, char *argv []) {
	isdn_ioctl_timeout_rule	timru;
	int			i, j, first, last,
				argp = firstarg,
				args_used = 0;
	char			name [9];


	/* interface */
	strncpy(name, id, sizeof(name) - 1);

	if(all) {
		first = 0;
		last = ISDN_TIMRU_NUM_CHECK - 1;
	} else {
		/* rule-type */
		SHIFT_ARGS("rule-type");
		if(! strcmp(argv [argp], "bringup")) {
			first = last = ISDN_TIMRU_BRINGUP;
		} else if(! strcmp(argv [argp], "keepup")) {
			/* keepup-type */
			SHIFT_ARGS("keepup-type");
			if(! strcmp(argv [argp], "in"))
				first = last = ISDN_TIMRU_KEEPUP_IN;
			else if(! strcmp(argv [argp], "out"))
				first = last = ISDN_TIMRU_KEEPUP_OUT;
			else if(! strcmp(argv [argp], "both")) {
				first = ISDN_TIMRU_KEEPUP_IN;
				last = ISDN_TIMRU_KEEPUP_OUT;
			} else
				PARSER_ERROR("wrong keepup-type");
		} else
			PARSER_ERROR("wrong rule-type");
	}

	for(i = first; i <= last; i++) {
		for(j = 0; j < ISDN_TIMRU_NUM_PROTFAM; j++) {
			while(1) {
				strcpy(timru.name, name);
				timru.where = 0;
				timru.type = i;
				timru.protfam = j;
				timru.index = 0;

				if(ioctl(fd, IIOCNETGRU, &timru) < 0)
					break;

				if(ioctl(fd, IIOCNETDRU, &timru) < 0)
					break;
			}
		}
	}

	return(args_used);
}


int
set_default_timeout(int fd, char *id, int firstarg, int argc, char *argv []) {
	isdn_ioctl_timeout_rule	timru;
	int			n, ret, other_type = -1,
				argp = firstarg,
				args_used = 0;


	/* set defaults */
	timru.where = -1;

	/* interface */
	strncpy(timru.name, id, sizeof(timru.name) - 1);

	/* rule-type */
	SHIFT_ARGS("rule-type");
	if(! strcmp(argv [argp], "bringup")) {
		timru.type = ISDN_TIMRU_BRINGUP;
	} else if(! strcmp(argv [argp], "keepup")) {
		/* keepup-type */
		SHIFT_ARGS("keepup-type");
		if(! strcmp(argv [argp], "in"))
			timru.type = ISDN_TIMRU_KEEPUP_IN;
		else if(! strcmp(argv [argp], "out"))
			timru.type = ISDN_TIMRU_KEEPUP_OUT;
		else if(! strcmp(argv [argp], "both")) {
			timru.type = ISDN_TIMRU_KEEPUP_IN;
			other_type = ISDN_TIMRU_KEEPUP_OUT;
		} else
			PARSER_ERROR("wrong keepup-type");
	} else
		PARSER_ERROR("wrong rule-type");

	/* default value */
	SHIFT_ARGS("default-value");
	if(sscanf(argv [argp], "%i%n", &timru.defval, &n) <= 0 || n != strlen(argv [argp]))
		PARSER_ERROR("wrong default-value");

	if((ret = ioctl(fd, IIOCNETARU, &timru)) < 0)
		return(ret);

	if(other_type >= 0) {
		timru.type = other_type;
		if((ret = ioctl(fd, IIOCNETARU, &timru)) < 0)
			return(ret);
	}

	return(args_used);
}


char *
defs_timru(char *id) {
	static char	r [1024];
	char	*p = r;

	p += sprintf(p, "addrule %s keepup in 0 ppp/lcp\n", id);

	return(r);
}


/* Budget-Erweiterung */
/*
??.07.97:cal:DAY: (60 * HOUR) --> (24 * HOUR)
04.11.97:cal:div. Formatierungen
*/

int output_budgets(int, char *, int, int, int, char *[]);
int read_budget_time(char *, time_t *);
char *write_budget_time(time_t);
char *output_time(time_t *);

#define	MINUTE	60
#define	HOUR	(60 * MINUTE)
#define	DAY	(24 * HOUR)
#define	WEEK	(7 * DAY)
#define	MONTH	(30 * DAY)
#define	YEAR	(365 * DAY)


int
hdl_budget(int fd, char *id, int cmd, int argc, char *argv []) {
	isdn_ioctl_budget	budget;
	int			ioret, n,
				argp = 0,
				args_used = 0;
	time_t		t;


	memset(&budget, 0, sizeof(isdn_ioctl_budget));

	/* cmd */
	switch(cmd) {
	case SHOWBUDGETS:
	case SAVEBUDGETS:
		return(args_used + output_budgets(fd, id, cmd, argp, argc, argv));
		break;

	case BUDGET:
		/* interface */
		strncpy(budget.name, id, sizeof(budget.name) - 1);

		/* budget-type */
		SHIFT_ARGS("rule-type");
		if(! strcmp(argv [argp], "dial"))
			budget.budget = ISDN_BUDGET_DIAL;
		else if(! strcmp(argv [argp], "charge"))
			budget.budget = ISDN_BUDGET_CHARGE;
		else if(! strcmp(argv [argp], "online"))
			budget.budget = ISDN_BUDGET_ONLINE;
		else
			PARSER_ERROR("wrong budget-type");

		budget.command = ISDN_BUDGET_GET_BUDGET;
		if((ioret = ioctl(fd, IIOCNETBUD, &budget)) < 0) {
			perror("ioctl failed.");
			return(ioret);
		}

		/* amount */
		SHIFT_ARGS("amount");
		if(!strcmp(argv [argp], "off")) {
			budget.amount = -1;
			budget.period = (time_t)0;
		} else {
			/* amount */
			if(read_budget_time(argv [argp], &t) < 0)
				PARSER_ERROR("wrong amount");
			budget.amount = t;

			/* period */
			SHIFT_ARGS("period");
			if(read_budget_time(argv [argp], &budget.period) < 0)
				PARSER_ERROR("wrong period");

			budget.used = 0;
			budget.period_started = (time_t)0;
		}

		budget.command = ISDN_BUDGET_SET_BUDGET;
		if((ioret = ioctl(fd, IIOCNETBUD, &budget)) < 0) {
			perror("ioctl failed.");
			return(ioret);
		}

		return(args_used);
		break;

	case RESTOREBUDGETS:
		/* interface */
		strncpy(budget.name, id, sizeof(budget.name) - 1);

		while(argp + 1 < argc) {
			SHIFT_ARGS("saved-budget");
			if(sscanf(argv [argp], "%d:%d:%ld:%d:%ld%n",
				&budget.budget, &budget.amount, (long *)&budget.period,
				&budget.used, (long *)&budget.period_started, &n) != 5 ||
				n != strlen(argv [argp])) {

				UNSHIFT_ARGS(1);
				break;
			} else {
				budget.command = ISDN_BUDGET_SET_BUDGET;
				if((ioret = ioctl(fd, IIOCNETBUD, &budget)) < 0) {
					perror("ioctl failed.");
					return(ioret);
				}
			}
		}

		return(args_used);
		break;

	default:
		PARSER_ERROR("unknown command");
	}
}


int
read_budget_time(char *str, time_t *period) {
	char	*s = str, base [10], comma [2];
	int	e, n, amount;
	time_t	total = 0;

	while(*s) {
		memset(base, 0, sizeof(base));

		if((e = sscanf(s, "%i%n%[a-zA-Z]%n", &amount, &n, base, &n)) > 0) {
			s += n;

			if(*s && sscanf(s, "%[,]%n", comma, &n) > 0)
				s += n;

			if(e == 1 || !strcmp(base, "s") || !strcmp(base, "sec"))
				total += amount;
			else if(!strcmp(base, "m") || !strcmp(base, "min"))
				total += (amount * MINUTE);
			else if(!strcmp(base, "h") || !strcmp(base, "hour"))
				total += (amount * HOUR);
			else if(!strcmp(base, "d") || !strcmp(base, "day"))
				total += (amount * DAY);
			else if(!strcmp(base, "w") || !strcmp(base, "week"))
				total += (amount * WEEK);
			else if(!strcmp(base, "M") || !strcmp(base, "month"))
				total += (amount * MONTH);
			else if(!strcmp(base, "y") || !strcmp(base, "year"))
				total += (amount * YEAR);
			else
				return(-1);
		} else
			return(-1);
	}

	*period = total;
				
	return(0);
}


char *
write_budget_time(time_t period) {
	static char	str [64];
	time_t		rest = period;


	memset(str, 0, sizeof(str));

	if(rest >= YEAR) {
		sprintf(str, "%ldy", rest / YEAR);
		rest %= YEAR;
	}

	if(rest >= MONTH) {
		sprintf(str, "%s%ldM", str, rest / MONTH);
		rest %= MONTH;
	}

	if(rest >= WEEK) {
		sprintf(str, "%s%ldw", str, rest / WEEK);
		rest %= WEEK;
	}

	if(rest >= DAY) {
		sprintf(str, "%s%ldd", str, rest / DAY);
		rest %= DAY;
	}

	if(rest >= HOUR) {
		sprintf(str, "%s%ldh", str, rest / HOUR);
		rest %= HOUR;
	}

	if(rest >= MINUTE) {
		sprintf(str, "%s%ldm", str, rest / MINUTE);
		rest %= MINUTE;
	}

	if(rest > 0 || period == 0)
		sprintf(str, "%s%lds", str, rest);

	return(str);
}


char *
output_time(time_t *t) {
	static char	str [64];
	struct tm	*u;


	memset(str, 0, sizeof(str));

	if((u = localtime(t)))
		sprintf(str, "%02d.%02d.%04d, %02d:%02d:%02d",
			u->tm_mday, u->tm_mon + 1, u->tm_year + 1900,
			u->tm_hour, u->tm_min, u->tm_sec);

	return(str);
}


int
output_budgets(int fd, char *id, int cmd, int firstarg, int argc, char *argv []) {
	isdn_ioctl_budget	budget;
	int			i;


	/* interface */
	strncpy(budget.name, id, sizeof(budget.name) - 1);

	switch(cmd) {
	case SHOWBUDGETS:
		printf("Budgets for interface %s:\n", budget.name);

		printf("\n");
		printf("TYPE    AMOUNT     PERIOD     USED       SINCE\n");

		for(i = 0; i < ISDN_BUDGET_NUM_BUDGET; i++) {
			budget.command = ISDN_BUDGET_GET_BUDGET;
			budget.budget = i;

			if(ioctl(fd, IIOCNETBUD, &budget) == 0) {
				switch(i) {
				case ISDN_BUDGET_DIAL:
					printf("%-6.6s  ", "dial");
					if(budget.amount < 0)
						printf("off\n");
					else
						printf("%-10d %-10.10s %-10d %s\n",
							budget.amount, write_budget_time(budget.period), budget.used,
							output_time(&budget.period_started));
					break;

				case ISDN_BUDGET_CHARGE:
					printf("%-6.6s  ", "charge");
					if(budget.amount < 0)
						printf("off\n");
					else
						printf("%-10d %-10.10s %-10d %s\n",
							budget.amount, write_budget_time(budget.period), budget.used,
							output_time(&budget.period_started));
					break;

				case ISDN_BUDGET_ONLINE:
					printf("%-6.6s  ", "online");
					if(budget.amount < 0)
						printf("off\n");
					else {
						printf("%-10.10s ", write_budget_time(budget.amount));
						printf("%-10.10s ", write_budget_time(budget.period));
						printf("%-10.10s %s\n", write_budget_time(budget.used),
							output_time(&budget.period_started));
					}
					break;
				}
			} else {
				perror("ioctl failed");
			}
		}
		break;

	case SAVEBUDGETS:
		for(i = 0; i < ISDN_BUDGET_NUM_BUDGET; i++) {
			budget.command = ISDN_BUDGET_GET_BUDGET;
			budget.budget = i;

			if(ioctl(fd, IIOCNETBUD, &budget) == 0) {
				if(i > 0)
					printf(" ");

				printf("%d:%d:%ld:%d:%ld",
					i, budget.amount, budget.period,
					budget.used, budget.period_started);
			} else {
				perror("ioctl failed");
			}
		}
		printf("\n");
	}

	return(0);
}


char *
defs_budget(char *id) {
	static char	r [1024];
	char	*p = r;

	p += sprintf(p, "budget %s dial 10 1min\n", id);
	p += sprintf(p, "budget %s charge 100 1day\n", id);
	p += sprintf(p, "budget %s online 8hour 1day\n", id);

	return(r);
}
#endif
