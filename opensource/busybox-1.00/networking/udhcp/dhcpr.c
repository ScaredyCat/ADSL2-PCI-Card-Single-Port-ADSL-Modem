/* dhcpr.c
 *
 * udhcp Relay
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
 * Nirav
 * IFX_INETD_ENHANCEMENT - These changes are added to make udhcpr run through inetd. 
 * When there is no request for udhcpr for pre-configured timeout period, udhcpr quits.
 * When the new request for udhcpr comes, inetd runs udhcpr and from then on udhcpr function as normal.
 */

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>

#include <getopt.h>

#include "dhcpd.h"
#include "dhcpr.h"
#include "options.h"
//#include "relaypacket.h"
#include "clientsocket.h"
#include "socket.h"
#include "signalpipe.h"
#include "script.h"
#include "common.h"
#if 0
#include "arpping.h"
#include "files.h"
#include "serverpacket.h"
#include "static_leases.h"
#endif

static int state;
//static unsigned long timeout;
//static unsigned long server_addr;
//static unsigned long requested_ip;
//static int packet_num;

#define LISTEN_NONE 0
#define LISTEN_KERNEL 1
#define LISTEN_RAW 2


#define SIZE_OF_INTF_NAME 10
#ifdef IFX_INETD_ENHANCEMENT
#define IFX_TIMEOUT	300
#endif

//#define INIT_SELECTING 0
static int listen_mode = LISTEN_KERNEL;

/* globals */
//struct dhcpOfferedAddr *leases;
struct relay_config_t relay_config;
unsigned short local_port = 0;
unsigned short remote_port = 0;

int add_agent_options = 1; //if non zero, add relay agent options.
int drop_agent_mismatches = 1; //if nonzero, drop ser replies that don't
			       //have matching circuit-id's
int max_hop_count = 10;   //maximum hop count

//static int fd = -1;

struct server_list{
	struct server_list *next;
	struct sockaddr_in to;
	int    index;
	int    fd;
	char interface[SIZE_OF_INTF_NAME];
	uint32_t primary_address;
	uint8_t arp[6];
} *servers;

struct interface_list{
	struct interface_list *next;
	struct interface_info intf;
} *interfaces;

/* What to do about packets we're asked to relay that
   already have a relay option: */
enum { forward_and_append,	/* Forward and append our own relay option. */
       forward_and_replace,	/* Forward, but replace theirs with ours. */
       forward_untouched,	/* Forward without changes. */
       discard } agent_relay_mode = forward_and_replace;

#ifndef IN_BUSYBOX
show_usage()
{
	printf(
			"Usage: udhcpr [OPTIONS]\n\n"
			"-i[intf] [..-i[intf] -i[intf] ] Interface to use\n"
			"-s[serv_ip:intf] [.. -s[serv_ip:intf] -s[serv_ip:intf] ] Server ip and the interface\n"
			);
	exit(0);
}
#else
#define show_usage bb_show_usage
extern void show_usage(void) __attribute__ ((noreturn));
#endif
#if 0
static void change_mode(int new_mode)
{
	DEBUG(LOG_INFO,"entering %s listen mode",
			new_mode ? (new_mode == 1? "kernel" : "raw"): "none");
	//if (fd > 0) close(fd);
	//fd = -1;
	listen_mode =new_mode;
}
#endif
static void relay_background(void)
{
	background(relay_config.pidfile);
	relay_config.foreground =1;
	relay_config.background_if_no_lease=0;
}

/* Find an interface that matches the circuit ID specified in the
   Relay Agent Information option.   If one is found, store it through
   the pointer given; otherwise, leave the existing pointer alone.

   We actually deviate somewhat from the current specification here:
   if the option buffer is corrupt, we suggest that the caller not
   respond to this packet.  If the circuit ID doesn't match any known
   interface, we suggest that the caller to drop the packet.  Only if
   we find a circuit ID that matches an existing interface do we tell
   the caller to go ahead and process the packet. */

int find_interface_by_agent_option (struct dhcpMessage *packet, struct interface_list **out, uint8_t *buf, int len)
{
	int i = 0;
	u_int8_t *circuit_id = 0;
	unsigned circuit_id_len=0;
	struct interface_list *ip;

	while (i < len) {
		/* If the next agent option overflows the end of the
		   packet, the agent option buffer is corrupt. */
		if (i + 1 == len ||
		    i + buf [i + 1] + 2 > len) {
			return -1;
		}
		switch (buf [i]) {
			/* Remember where the circuit ID is... */
		      case RAI_CIRCUIT_ID:
			circuit_id = &buf [i + 2];
			circuit_id_len = buf [i + 1];
			i += circuit_id_len + 2;
			continue;

		      default:
			i += buf [i + 1] + 2;
			break;
		}
	}

	/* If there's no circuit ID, it's not really ours, tell the caller
	   it's no good. */
	if (!circuit_id) {
		DEBUG(LOG_ERR,"Error: No circuit id\n");
		return -1;
	}

	/* Scan the interface list looking for an interface whose
	   name matches the one specified in circuit_id. */

	for (ip = interfaces; ip; ip = ip -> next) {
		if (strlen(ip ->intf.interface) == circuit_id_len &&
		    !memcmp (ip ->intf.interface, circuit_id, strlen(ip->intf.interface)))
			break;
	}

	/* If we got a match, use it. */
	if (ip) {
		*out = ip;
		DEBUG(LOG_DEBUG,"Circuit id match.. returning\n");
		return 1;
	}

	/* If we didn't get a match, the circuit ID was bogus. */
	DEBUG(LOG_ERR,"Error: No match in circuit id\n");
	return -1;
}
//Strip any relay agent information options from the dhcp packet 
//option buffer. If there is a circuit id suboption, look up the
// outgoing interface based upon it
int strip_relay_agent_options(struct interface_info *in, 
				struct interface_list **out,
				struct dhcpMessage *packet,
				unsigned length)
{
	int is_dhcp = 0;
	uint8_t *op, *sp, *max;
	int good_agent_option = 0;
	int status;

	/* If we're not adding agent options to packets, we're not taking
	   them out either. */
	if (!add_agent_options)
		return length;

	/* If there's no cookie, it's a bootp packet, so we should just
	   forward it unchanged. */
	if(!(ntohl(packet->cookie) == DHCP_MAGIC))
	{
		return length;
	}
	max = ((u_int8_t *)packet) + sizeof(struct dhcpMessage);
	sp = op = (uint8_t *)&(packet -> options);

	while (op < max) {
		switch (*op) {
			/* Skip padding... */
		      case DHCP_PADDING:
			if (sp != op)
				*sp = *op;
			++op;
			++sp;
			continue;

			/* If we see a message type, it's a DHCP packet. */
		      case DHCP_MESSAGE_TYPE:
			is_dhcp = 1;
			goto skip;
			break;

			/* Quit immediately if we hit an End option. */
		      case DHCP_END:
			if (sp != op)
				*sp++ = *op++;
			goto out;

		      case DHCP_AGENT_OPTIONS:
			/* We shouldn't see a relay agent option in a
			   packet before we've seen the DHCP packet type,
			   but if we do, we have to leave it alone. */
			if (!is_dhcp)
				goto skip;

			status = find_interface_by_agent_option (packet,
								 out, op + 2,
								 op [1]);
			if (status == -1 && drop_agent_mismatches)
			{
				DEBUG(LOG_INFO," Error: Agent mismatch\n");
				return 0;
			}
			if (status)
				good_agent_option = 1;
			op += op [1] + 2;
			break;

		      skip:
			/* Skip over other options. */
		      default:
			if (sp != op)
				memcpy (sp, op, (unsigned)(op [1] + 2));
			sp += op [1] + 2;
			op += op [1] + 2;
			break;
		}
	}
      out:

	/* If it's not a DHCP packet, we're not supposed to touch it. */
	if (!is_dhcp)
		return length;

	/* If none of the agent options we found matched, or if we didn't
	   find any agent options, count this packet as not having any
	   matching agent options, and if we're relying on agent options
	   to determine the outgoing interface, drop the packet. */

	if (!good_agent_option) {
		if (drop_agent_mismatches)
			return 0;
	}

	/* Adjust the length... */
	if (sp != op) {
		length = sp - ((u_int8_t *)packet);

		/* Make sure the packet isn't short (this is unlikely,
                   but WTH) */
		if (length < BOOTP_MIN_LEN) {
			memset (sp, 0, BOOTP_MIN_LEN - length);
			length = BOOTP_MIN_LEN;
		}
	}
	return length;
}

int add_relay_agent_options(struct interface_info *ip,
				struct dhcpMessage *packet,
				unsigned length,
				uint32_t giaddr)
{
	int is_dhcp = 0 /*, agent_options_present = 0*/;
	uint8_t *op, *sp, *max, *end_pad = 0;

	//If we are not adding agent options to packet, we can skip*/
	if(!add_agent_options)
		return length;

	//If there is no cookie, it's a bootp packet, so we should
	//forward it unchanged
	if(!(ntohl(packet->cookie) == DHCP_MAGIC))
	{
		return length;
	}

	DEBUG(LOG_INFO,"Add Relay Agent options\n");
	//max = ((uint8_t *)packet)+ length;
	max = ((uint8_t *)packet)+ sizeof(struct dhcpMessage);
	sp = op = (uint8_t *)(&(packet -> options));
#if 1
	while(op<max)
	{
		switch(*op)
		{
			/* Skip padding*/
			case DHCP_PADDING:
				end_pad = sp;
				if(sp != op)
					*sp = *op;
				++op;
				++sp;
				continue;
				//if we see a message type, it's a DHCP packet
			case DHCP_MESSAGE_TYPE:
				is_dhcp =1;
				goto skip;
				break;

				//Quit immediately if we hit an End option */
			case DHCP_END:
				goto out;

			case DHCP_AGENT_OPTIONS:
				//we shouldn't see a relay agent option in a 
				//packet before we've seen the DHCP packet type,
				if(!is_dhcp)
					goto skip;
				end_pad = 0;

				/* There's already a Relay Agent Information option
				   in this packet.   How embarrassing.   Decide what
				   to do based on the mode the user specified. */

				switch (agent_relay_mode) {
					case forward_and_append:
						goto skip;
					case forward_untouched:
						return length;
					case discard:
						return 0;
					case forward_and_replace:
					default:
						break;
				}
				/* Skip over the agent option and start copying
				   if we aren't copying already. */
				op += op [1] + 2;
				break;
			skip:
				/* Skip over other options. */
			default:
				end_pad = 0;
				if (sp != op)
					memcpy (sp, op, (unsigned)(op [1] + 2));
				sp += op [1] + 2;
				op += op [1] + 2;
				break;


		}//end switch
	}//end while
	
	out:

	/* If it's not a DHCP packet, we're not supposed to touch it. */
	if (!is_dhcp)
		return length;

	/* If the packet was padded out, we can store the agent option
	   at the beginning of the padding. */

	if (end_pad)
		sp = end_pad;

	/* Remember where the end of the packet was after parsing
	   it. */
	op = sp;

	/* XXX Is there room? */

	/* Okay, cons up *our* Relay Agent Information option. */
	*sp++ = DHCP_AGENT_OPTIONS;
	*sp++ = 0;	/* Dunno... */

	/* Copy in the circuit id... */
	*sp++ = RAI_CIRCUIT_ID;
	/* Sanity check.   Had better not every happen. */
	//if (ip -> circuit_id_len > 255 || ip -> circuit_id_len < 1)
	//	log_fatal ("completely bogus circuit id length %d on %s\n",
	//			ip -> circuit_id_len, ip -> name);
	*sp++ = strlen(ip->interface);
	memcpy (sp, ip->interface, strlen(ip->interface));
	sp += strlen(ip->interface);
#if 0
	/* Copy in remote ID... */
	if (ip -> remote_id) {
		*sp++ = RAI_REMOTE_ID;
		if (ip -> remote_id_len > 255 || ip -> remote_id_len < 1)
			log_fatal ("bogus remote id length %d on %s\n",
					ip -> circuit_id_len, ip -> name);
		*sp++ = ip -> remote_id_len;
		memcpy (sp, ip -> remote_id, ip -> remote_id_len);
		sp += ip -> remote_id_len;
	}
#endif
	/* Relay option's total length shouldn't ever get to be more than
	   257 bytes. */
	if (sp - op > 257)
		DEBUG(LOG_DEBUG,"total agent option length exceeds 257 (%ld) on %s\n",
				(long)(sp - op), ip->interface);

	/* Calculate length of RAI option. */
	op [1] = sp - op - 2;

	/* Deposit an END token. */
	*sp++ = DHCP_END;

	/* Recalculate total packet length. */
	length = sp - ((u_int8_t *)packet);

	/* Make sure the packet isn't short (this is unlikely, but WTH) */
	if (length < BOOTP_MIN_LEN) {
		memset (sp, 0, BOOTP_MIN_LEN - length);
		length = BOOTP_MIN_LEN;
	}

	return length;
#endif
}
				

void relay(struct dhcpMessage *packet, int fd)
{

	struct interface_list *out, *in;
	struct interface_info *ip=NULL;
	unsigned length=0;
	struct server_list *sp = (struct server_list *)0;

	//Find the interface that corresponds to the giaddr in the packet
	if(packet->giaddr)
	{
		for(out = interfaces; out; out = out->next)
		{
			if(out->intf.primary_address == packet->giaddr)
				break;
		}
	}
	else
	{
		out = (struct interface_list *)0;
	}

	//find the interface through which we received this packet
	for(in= interfaces; in; in =in->next)
	{
		if(in->intf.fd == fd)
			ip = &(in->intf);
	}
	for(sp= servers; sp; sp=sp->next)
	{
		if(sp->fd == fd)
		{
			DEBUG(LOG_DEBUG,"Coming from server, %d, %d\n", sp->fd, fd);
		}
	}

	/* If its a bootreply, forward it to the client */
	if(packet->op == BOOTREPLY)
	{
		DEBUG(LOG_INFO,"Bootreply message\n");
#if 1
		if(!(packet->flags & htons(BOOTP_BROADCAST)) /*&&
							       can_unicast_without_arp(out)*/)
		{
			DEBUG(LOG_DEBUG,"Sending unicast\n");
		}
		else
		{

			DEBUG(LOG_DEBUG,"Sending broadcast\n");
			//Need to implement this feature. Currently not
			//required
		}
		//Wipe out the agent relay options and if possible figure
		//out which interface to use based on the contents of the 
		//option that we put on the request to which the server is
		//replying
		if(!(length = strip_relay_agent_options(ip, &out, packet, length)))
			return;

		if(!out)
		{
			struct in_addr a;
			a.s_addr = packet->giaddr;
			DEBUG(LOG_DEBUG,"packet to bogus giaddr %s.\n", inet_ntoa(a));
			return;
		}
		//if(kernel_packet(packet, packet->giaddr, SERVER_PORT, 
		if(raw_packet(packet, packet->giaddr, SERVER_PORT, packet->yiaddr, CLIENT_PORT, /*sp->arp*/packet->chaddr, out->intf.ifindex) < 0 )
		{
			DEBUG(LOG_ERR,"Error: failed to send the message\n");
		}

		return;
#endif
	}

	//If giaddr matches on of our addresses, ignore the packet -
	// we just sent it..
	if(out)
		return;


	//Add relay agent options if indicated. If something goes wrong
	//drop the packet.
	if(!(length = add_relay_agent_options(ip, packet, length, ip->primary_address)))
		return;


//	DEBUG(LOG_INFO,"Entering relay\n");

	//If giaddr is not already set, set it so the server can figure out
	//what net its from and so that we can later forward the response to 
	//the correct net. If its already set the response will be sent 
	//directly to the relay agent the set giaddr, so we won't see it.

	if(!packet->giaddr)
		packet->giaddr = ip->primary_address;
	if(packet->hops< max_hop_count)
		packet ->hops = packet ->hops +1;
	else
		return;

	//Otherwise, it's a BOOTREQUEST, so forward it to all the servers

	DEBUG(LOG_DEBUG,"Bootrequest message \n");
	for(sp= servers; sp; sp=sp->next)
	{
		/*uint8_t mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};*/
		//Sending it in a broadcast mac, because I do not know how 
		//to obtain the mac from the ARP table from user space
		if(kernel_packet(packet, sp->primary_address, SERVER_PORT, sp->to.sin_addr.s_addr, SERVER_PORT)< 0)
			DEBUG(LOG_INFO,"failed to send the message\n");
#if 0		
		if(raw_packet(packet, sp->primary_address, 
				SERVER_PORT, sp->to.sin_addr.s_addr, 
				SERVER_PORT, mac, sp->index) < 0 )
			DEBUG(LOG_INFO,"failed to send the message\n");
#endif
	}

	DEBUG(LOG_DEBUG,"Returning from relay\n");
}

int split_server_info(char* opt_arg, char* server_intf)
{
	int len;
	char c;
	char* tempptr=opt_arg;
	DEBUG(LOG_DEBUG,"split_server_info: optarg: [%s]\n",opt_arg);
	memset(server_intf, '\0',SIZE_OF_INTF_NAME);
	
	c= *tempptr;
	len = strlen(opt_arg);
	while(c != ':')
	{
		if(c == '\0')
			return 0;
		tempptr++;
		c = *tempptr;
		len--;
	}
	*tempptr= '\0';	
	strncpy(server_intf, tempptr+1, len);
	DEBUG(LOG_DEBUG,"split_server_info: serverintf: [%s]\n",server_intf);	
	
	return 1;
	
}

#ifdef COMBINED_BINARY
int udhcpr_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif

{
	int c;
	struct servent *ent;
	struct server_list *sp = (struct server_list *)0;
	struct interface_list *inf = (struct interface_list*)0;
	//int no_daemon = 0;
	//int quiet = 0;
	//char *s;
	int len;
	//unsigned long xid=0;
	//uint8_t *temp,*message;
	//unsigned long  t1=0, t2=0;
	//unsigned long start=0,lease=0;
	//struct in_addr temp_addr;

	struct timeval tv;
	int retval;
	fd_set rfds;
	long now;
	int max_fd=0;
	int sig;
	struct dhcpMessage packet;
	int fd;
#ifdef IFX_INETD_ENHANCEMENT
	char curInf[16];
#endif
	
	static const struct option arg_options[]={/*
		{"port",optional_argument,0,'p'},
		{"daemon",no_argument,0,'d'},*/
		{"interface",required_argument,0,'i'},
	/*	{"no_print",no_argument,0,'q'},*/
		{"server",required_argument,0,'s'},
#ifdef IFX_INETD_ENHANCEMENT
		{"Add",required_argument,0,'A'},
#endif
		{0,0,0,0}
	};

	servers=NULL;

	//memset(&relay_config, 0, sizeof(struct relay_config_t));
	/* get options */
	while (1) {
		int option_index = 0;
		//c = getopt_long(argc, argv, "p:d:s:i:q", arg_options, &option_index);
#ifdef IFX_INETD_ENHANCEMENT
		c = getopt_long(argc, argv, "s:i:A:q", arg_options, &option_index);
#else
		c = getopt_long(argc, argv, "s:i:q", arg_options, &option_index);
#endif
		if (c == -1) 
		{
			break;
		}

		switch (c) {
#if 0
		case 'p':
			DEBUG(LOG_INFO,"port: %s\n",optarg);
			relay_config.port = optarg;
					break;
		case 'd':sp = ((struct server_list*)malloc(sizeof *sp));
			if(!sp)
				DEBUG(LOG_INFO,"fatal: no memory for server\n");

			DEBUG(LOG_INFO,"foreground: yes\n");
			relay_config.foreground = 1;
			break;
#endif
		case 'i':
			DEBUG(LOG_INFO,"interface: %s\n",optarg);
			//relay_config.interface = optarg;
			//	strncpy(relay_config.interface, optarg, strlen(optarg));

			inf=((struct interface_list*)malloc(sizeof *inf));
			if(!inf)
				DEBUG(LOG_INFO,"fatal: no memory for interface\n");
			inf->next = interfaces;
			interfaces = inf;
			//strncpy(&(inf->intf.interface), optarg, strlen(optarg));
			strncpy(inf->intf.interface, optarg, strlen(optarg));

			break;
#if 0
		case 'w':
			DEBUG(LOG_INFO,"server interface\n");
#endif
			
		case 's':
			DEBUG(LOG_INFO,"server: %s\n",optarg);
			{
				struct hostent *he;
				struct in_addr ia, *iap =(struct in_addr*)0;
			 	char server_intf[SIZE_OF_INTF_NAME];
				if(split_server_info(optarg,server_intf))
				{
					
				}
				else
				{
					DEBUG(LOG_INFO,"Server: ip_addr:intf\n");
					return 1;
				}
				if(inet_aton(optarg, &ia))
				{
					iap = &ia;
				}
				else
				{
					he = gethostbyname(optarg);
					if(!he)
					{
						DEBUG(LOG_INFO,"%s: host unknown\n",optarg);
					}
					else
					{
						iap = ((struct in_addr *)he->h_addr_list[0]);
					}
				}
				if(iap)
				{
					DEBUG(LOG_INFO,"Putting in server list\n");
					sp = ((struct server_list*)malloc(sizeof *sp));
					if(!sp)
						DEBUG(LOG_INFO,"fatal: no memory for server\n");
					sp->next =servers;
					servers = sp;
					memcpy(&sp->to.sin_addr, iap, sizeof *iap);
					strncpy(sp->interface, server_intf, SIZE_OF_INTF_NAME);
					DEBUG(LOG_INFO,"Server Address: [%s]\n",inet_ntoa(sp->to.sin_addr));
				}

				//relay_config.server = optarg;
				//strncpy(relay_config.server, optarg, strlen(optarg));
			}
			break;

#ifdef IFX_INETD_ENHANCEMENT
		case 'A':
			strncpy(curInf,optarg,strlen(optarg));
			break;
#endif
#if 0
		case 'q':
			break;
#endif
		default:
			//DEBUG(LOG_INFO,"entering default\n");
			show_usage();
			break;
#if 0
			sp = ((struct server_list*)malloc(sizeof *sp));
			if(!sp)
				DEBUG(LOG_INFO,"fatal: no memory for server\n");
#endif

		}
	}

	if(!local_port)
	{
		ent = getservbyname("dhcps", "udp");
		if(!ent)
			local_port = htons(67);
		else
			local_port = ent->s_port;
		endservent();
	}
	remote_port = htons(ntohs(local_port)+1);

	//At least one server
	if(!sp)
	{
		show_usage();
	}

	for(sp = servers; sp; sp= sp->next)
	{
		sp->to.sin_port = local_port;
		sp->to.sin_family = AF_INET;
		if(read_interface(sp->interface, &(sp->index),&(sp->primary_address), sp->arp)<0)
		{
			DEBUG(LOG_ERR,"Error: Failing read_interface for server\n");		
			return 1; 
		}
		
	}

	DEBUG(LOG_INFO,"Local port: %d\n",ntohs(local_port));
	DEBUG(LOG_INFO,"Remote port: %d\n",ntohs(remote_port));
	
	for(inf = interfaces; inf; inf= inf->next)
	{
		struct in_addr a;
		DEBUG(LOG_DEBUG,"Interface : %p\n", inf);
		if(read_interface(inf->intf.interface, &(inf->intf.ifindex),
					&(inf->intf.primary_address),inf->intf.arp)<0)
		{
			return 1;
		}
		else
		{
			a.s_addr =inf->intf.primary_address;
			DEBUG(LOG_DEBUG," Address stored %s\n",inet_ntoa(a));
			DEBUG(LOG_DEBUG," Interface: %s\n",inf->intf.interface);
		}
		//check if a server is
	}


	
	//setup the signal pipe
	//udhcp_sp_setup();
 
	state = INIT_SELECTING;
	//change_mode(LISTEN_KERNEL);
	while(1)
	{
		//tv.tv_sec = timeout - uptime();
		tv.tv_sec = IFX_TIMEOUT;
		tv.tv_usec = 0;
		//open the socket for the 1st time only
		if(listen_mode != LISTEN_NONE && max_fd == 0)
		{
			for(inf = interfaces; inf; inf= inf->next)
			{	
				//if(listen_mode == LISTEN_KERNEL)
				//{
#ifdef IFX_INETD_ENHANCEMENT
				DEBUG(LOG_DEBUG,"Calling kernel socket\n");
				if(strcmp(inf->intf.interface,curInf) == 0) 
					fd = 0;
				else 
#endif
					fd = listen_socket(INADDR_ANY, SERVER_PORT, inf->intf.interface);
				//}
				//else
				//{
				//	DEBUG(LOG_INFO,"raw socket fd\n");
				//	fd = raw_socket(inf->intf.ifindex);
				//}

				if(fd<0)
				{
					DEBUG(LOG_ERR,"fatal: couldn't listen on sockets\n");
					return 0;
				}
				else
				{
					//Store the fd number in the interface, so we can identify
					//from which interface we received the packet
					inf->intf.fd =fd;
				}
/*
				if(max_fd == 0)
					max_fd = udhcp_sp_fd_set(&rfds, fd);
				else
					FD_SET(fd, &rfds);
*/
			}
#if 1
			for(sp = servers; sp; sp= sp->next)
			{
				//if(listen_mode == LISTEN_KERNEL)
				//{
					DEBUG(LOG_DEBUG,"Calling kernel socket\n");
					fd = listen_socket(INADDR_ANY, SERVER_PORT, sp->interface);
					//fd = listen_socket(sp->to.sin_addr.s_addr, SERVER_PORT, sp->interface);
				//}
				//else
				//{
				//	fd = raw_socket(sp->index);
				//}

				if(fd<0)
				{
					DEBUG(LOG_ERR,"fatal: couldn't listen on sockets\n");
					return 0;
				}
				else
				{
					//Store the fd number in the interface, so we can identify
					//from which interface we received the packet
					sp->fd =fd;
				}
//				FD_SET(fd, &rfds);

			}
		}
#endif
		max_fd = 0;
		//Set the fd in the fd_set everytime
		for(inf = interfaces; inf; inf= inf->next)
		{
			if(max_fd == 0)
			{
				DEBUG(LOG_DEBUG,"Calling interface udhcp_sp_fd_set\n");
				max_fd = udhcp_sp_fd_set(&rfds, inf->intf.fd);
			}
			else
			{
				DEBUG(LOG_DEBUG,"Calling interface fd_set\n");
				FD_SET(inf->intf.fd, &rfds);
			}
		}
		for(sp = servers; sp; sp= sp->next)
		{
			if(max_fd == 0)
			{
				DEBUG(LOG_DEBUG,"Calling serverinterface udhcp_sp_fd_set\n");
				max_fd = udhcp_sp_fd_set(&rfds, sp->fd);
			}
			else
			{
				DEBUG(LOG_DEBUG,"Calling serverinterface fd_set\n");
				FD_SET(sp->fd, &rfds);
			}
		}

		if(tv.tv_sec >0)
		{
			//DEBUG(LOG_INFO,"Waiting on select\n");
			retval = select(FD_SETSIZE, &rfds, NULL,  NULL, &tv);
			//DEBUG(LOG_INFO,"received\n");
		}
		else
			retval =0;


#ifdef IFX_INETD_ENHANCEMENT
		if(retval == 0) {
			syslog(LOG_INFO, "udhcpr Timed out. Exiting\n");
			exit(0);
		}
#endif
		now = uptime();
		if(retval == 0)
		{
			DEBUG(LOG_DEBUG,"retval 0\n");
			//change_mode(listen_mode);
		}
		else if(retval > 0 && listen_mode != LISTEN_NONE )
		{
			for(inf = interfaces; inf; inf= inf->next)
			{
				if(FD_ISSET(inf->intf.fd, &rfds))
				{
					fd = inf->intf.fd;

					DEBUG(LOG_DEBUG,"retval : received packet\n");
					//if (listen_mode == LISTEN_KERNEL)
						len = get_packet(&packet, fd);
					//else len = get_raw_relay_packet(&packet, fd);

					if (len == -1 && errno != EINTR) {
						DEBUG(LOG_ERR, "Error: error on read, %m");
						//change_mode(listen_mode); /* just close and reopen */
					}
					if (len < 0)
					{
						continue;
					}
					relay(&packet, fd);//process the packet
				}
			}

			for(sp = servers; sp; sp= sp->next)
			{
				if(FD_ISSET(sp->fd, &rfds))
				{
					fd = sp->fd;

					DEBUG(LOG_DEBUG,"retval : received packet\n");
					//if (listen_mode == LISTEN_KERNEL)
						len = get_packet(&packet, fd);
					//else len = get_raw_relay_packet(&packet, fd);

					if (len == -1 && errno != EINTR) {
						DEBUG(LOG_ERR, "Error: error on read, %m");
						//change_mode(listen_mode); /* just close and reopen */
					}
					if (len < 0)
					{
						continue;
					}
					relay(&packet, fd);//process the packet
				}
			}
			
		}
		else if (retval > 0 && (sig = udhcp_sp_read(&rfds)))
		{
			DEBUG(LOG_INFO,"retval : received signal\n");
		}
		else if(retval == -1 && errno == EINTR)
		{
			DEBUG(LOG_INFO,"retval : signal\n");
		}
		else
		{
			DEBUG(LOG_INFO,"retval: Error: %d\n",retval);
		}
	}

	return 0;
}

