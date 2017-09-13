#ifndef __RIP_READ_CONFIG_H__
#define __RIP_READ_CONFIG_H__
#include <zebra.h>

#include "if.h"
#include "command.h"
#include "prefix.h"
#include "table.h"
#include "thread.h"
#include "memory.h"
#include "log.h"
#include "stream.h"
#include "filter.h"
#include "sockunion.h"
#include "routemap.h"
#include "plist.h"
#include "distribute.h"
#include "md5-gnu.h"
#include "keychain.h"

#include "ripd/ripd.h"
#include "ripd/rip_debug.h"

#ifdef DEBUG_RIPD_STANDALONE
#include "zebra/debug.h"
#endif

#include "log.h"

extern struct interface *cmd_current_interface ;

int debug_rip_events(struct interface *ifp,int argc,char **argv);
int debug_rip_packet(struct interface *ifp,int argc,char **argv);
int debug_rip_packet_all(struct interface *ifp,int argc,char **argv);
int debug_rip_packet_direct(struct interface *ifp,int argc,char **argv);
int config_log_stdout(struct interface *ifp,int argc,char **argv);
int config_log_file(struct interface *ifp,int argc,char **argv);
int config_log_syslog(struct interface *ifp,int argc,char **argv);
int rip_network(struct interface *ifp,int argc,char **argv) ;
int rip_default_information_originate(struct interface *ifp,int argc,char **argv);
int rip_neighbor(struct interface *ifp,int argc,char **argv) ;
int rip_enable_network_add (struct prefix *p);
int rip_enable_if_add (char *ifname);
void rip_enable_apply_all ();
int rip_neighbor_add (struct prefix_ipv4 *p);
int rip_passive_interface_set (struct vty *vty, char *ifname);
int rip_passive_interface(struct interface *ifp,int argc,char **argv);
int rip_version(struct interface *ifp,int argc,char **argv);
int rip_default_metric(struct interface *ifp,int argc,char **argv);
int rip_timers( struct interface *ifp,int argc,char **argv );
int rip_route( struct interface *ifp,int argc,char **argv ) ;
int rip_distance(struct interface *ifp, int argc,char **argv );
int rip_distance_source(struct interface *ifp,int argc,char **argv ) ;
int rip_distance_source_access_list(struct interface *ifp,int argc,char **argv );
int distribute_list_all(struct interface *ifp,int argc,char **argv );
int distribute_list(struct interface *ifp,int argc,char **argv );
int districute_list_prefix_all(struct interface *ifp,int argc,char **argv );
int districute_list_prefix(struct interface *ifp,int argc,char **argv );
int rip_interface( struct interface *intf,int argc, char **argv);
int ip_rip_send_version( struct interface *ifp,int argc, char **argv);
int ip_rip_send_version_1(struct interface *ifp,int argc, char **argv);
int ip_rip_send_version_2(struct interface *ifp,int argc, char **argv);
int ip_rip_receive_version(struct interface *ifp,int argc, char **argv);
int ip_rip_receive_version_1(struct interface *ifp,int argc, char **argv );
int ip_rip_receive_version_2(struct interface *ifp,int argc, char **argv);
int ip_rip_authentication_mode(struct interface *ifp,int argc, char **argv);
int ip_rip_authentication_key_chain(struct interface *ifp,int argc, char **argv );
int ip_rip_authentication_string(struct interface *ifp,int argc, char **argv );
int rip_split_horizon(struct interface *ifp,int argc, char **argv);


	
	
void rip_event (enum rip_event event, int sock);
int rip_distance_set (struct vty *vty, char *distance_str, char *ip_str,char *access_list_str);
struct distribute * distribute_list_set (char *ifname, enum distribute_type type, char *alist_name);
struct distribute * distribute_list_prefix_set (char *ifname, enum distribute_type type,
			    char *plist_name);

void rip_read_config(char *config_file, char *config_current_dir, char *config_default_dir);
void rip_read_config_file(FILE *filep);

#endif
