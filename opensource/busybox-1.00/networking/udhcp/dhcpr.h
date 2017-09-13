/* dhcpd.h */
#ifndef _DHCPR_H
#define _DHCPR_H

#include <netinet/ip.h>
#include <netinet/udp.h>

#include "libbb_udhcp.h"
#include "leases.h"
#include "version.h"



struct relay_config_t{
	int    port;
	char    foreground;
//	char   interface[10];
//	char   server[16]; /* Server IP list in network order */
	uint8_t *clientid;
//	uint8_t arp[6];
//	int ifindex;
	char *pidfile;
	char quit_after_lease;
	char abort_if_no_lease;
	char background_if_no_lease;
	
};

struct interface_info{
	char interface[10];
	uint8_t arp[6];
	int ifindex;
	//char primary_address[16];
	uint32_t primary_address;
	int fd; //raw socket fd number
};
#define INIT_SELECTING 	0
#define REQUESTING     	1
#define BOUND		2
#define RENEWING	3
#define REBINDING	4
#define INIT_REBOOT	5
#define RENEW_REQUESTED	6
#define RELEASED	7
#endif
