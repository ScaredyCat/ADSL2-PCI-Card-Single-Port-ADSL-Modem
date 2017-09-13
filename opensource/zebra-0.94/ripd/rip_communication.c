#include <stdio.h>

#include <zebra.h>

#include "vector.h"
#include "vty.h"
#include "command.h"
#include "prefix.h"
#include "table.h"
#include "stream.h"
#include "linklist.h"
#include "log.h"
#include "if.h"
#include "ripd.h"
#include "ripd/rip_debug.h"

/* XXX: check if this is needed and why */
/* Default rtm_table for all clients */
int rtm_table_default = 0;

void rip_enable_apply (struct interface *);
void rip_passive_interface_apply (struct interface *);
int rip_if_down(struct interface *ifp);

int
zebra_check_addr (struct prefix *p)
{
  if (p->family == AF_INET)
     {
        u_int32_t addr;
        addr = p->u.prefix4.s_addr;
        addr = ntohl (addr);
        if (IPV4_NET127 (addr) || IN_CLASSD (addr))
        return 0;
     }
  return 1;
}


/* Interface down information. */
void
zebra_interface_down_update (struct interface *ifp)
{
	#ifdef DEBUG_RIPD_STANDALONE
	 if (IS_RIP_DEBUG_EVENT)
	    zlog_info("RIPDS:interface is DOWN now :%s ",ifp->name);
	#endif
	 /* Lookup this by interface index. */
    if( !if_lookup_by_name (ifp->name) ) {
       return;
	}

    rip_if_down(ifp);
}

/* Interface up information. */
void
zebra_interface_up_update (struct interface *ifp)
{
  	#ifdef DEBUG_RIPD_STANDALONE
	 if (IS_RIP_DEBUG_EVENT)
  		zlog_info("RIPDS:interface %s is UP now ",ifp->name);

	#endif

  
  if ( !if_lookup_by_name (ifp->name) ) {

     /* If such interface does not exist, indicate an error */
     return;
  }

  rip_enable_apply (ifp);
 
  /* Check for a passive interface */
  rip_passive_interface_apply (ifp);

  /* Apply distribute list to the all interface. */
  rip_distribute_update_interface (ifp);

}


/* Interface information update. */
void
zebra_interface_add_update (struct interface *ifp)
{
	struct interface *ifpnew;
	
    
	#ifdef DEBUG_RIPD_STANDALONE
	 if (IS_RIP_DEBUG_EVENT)
		zlog_info("RIPDS:Interface %s is Added ",ifp->name);
	#endif

    /* If such interface does not exist, make new one. */
    if ( !if_lookup_by_name(ifp->name) ) {
          ifpnew = if_create ();
	      strncpy (ifpnew->name, ifp->name, IFNAMSIZ);
          memcpy(ifpnew,ifp,sizeof(*ifp));
		
		  rip_enable_apply (ifp);
    	  /* Apply distribute list to the all interface. */
   	 	  rip_distribute_update_interface (ifp);
		  return ;
	}

	rip_enable_apply (ifp);
    /* Apply distribute list to the all interface. */
    rip_distribute_update_interface (ifp);

}

void
zebra_interface_delete_update (struct interface *ifp)
{
   	#ifdef DEBUG_RIPD_STANDALONE
	 if (IS_RIP_DEBUG_EVENT)
    	zlog_info("RIPDS:Interface %s is Deleted ",ifp->name);
	#endif

	if ( !if_lookup_by_name (ifp->name) ) {
     	/* If such interface does not exist, indicate an error */
     	return;
    }

    if (if_is_up (ifp)) {
        rip_if_down(ifp);
    } 


}

/* Interface address addition. */
void
zebra_interface_address_add_update (struct interface *ifp,
                    struct connected *ifc)
{
    struct prefix *p;

	#ifdef DEBUG_RIPD_STANDALONE
	 if (IS_RIP_DEBUG_EVENT)
		zlog_info("RIPDS:Interface address is added for if:%s ",ifp->name);
	#endif

	if ( !if_lookup_by_index(ifp->ifindex) ) {
     	/* If such interface does not exist, indicate an error */
     	return ;
    }

	if (ifc == NULL) {
        return ;
	}
  	/* Add connected address to the interface. */
    listnode_add (ifp->connected, ifc);

    p = ifc->address;
    if (p->family == AF_INET) {
      /* Check is this interface is RIP enabled or not.*/
      rip_enable_apply (ifc->ifp);
	  #ifdef HAVE_SNMP
      rip_ifaddr_add (ifc->ifp, ifc);
      #endif /* HAVE_SNMP */
	}

}

/* Interface address deletion. */
void
zebra_interface_address_delete_update (struct interface *ifp,
                       struct connected *ifc)
{
    struct prefix *p;
    struct connected *temp;

	if ( !if_lookup_by_index(ifp->ifindex) ) {
     	/* If such interface does not exist, indicate an error */
     	return;
    }

	p = ifc->address;

	temp = connected_delete_by_prefix (ifp, p);

   	#ifdef DEBUG_RIPD_STANDALONE
	 if (IS_RIP_DEBUG_EVENT)
		zlog_info("RIPDS:Interface address is Deleted for if:%s ",ifp->name);
	#endif
 
	if (temp) {
       if (p->family == AF_INET) {
	      /* Check if this interface is RIP enabled or not.*/
	      rip_enable_apply (ifc->ifp);
	   }
	   #ifdef HAVE_SNMP
	   rip_ifaddr_delete (ifc->ifp, ifc);
       #endif /* HAVE_SNMP */

	   /*
		*  chandrav ppp crash bug fix
		*  Commented the below line of code to fix problem 
		*  of ripd crashing upon creation of ppp interface 
	    */
	   #if 0
       connected_free (ifc);
	   #endif

    }

}


int
rib_add_ipv4 (int type, int flags, struct prefix_ipv4 *p,
          struct in_addr *gate, unsigned int ifindex, u_int32_t vrf_id,
		            u_int32_t metric, u_char distance)
{ 
	#ifdef DEBUG_RIPD_STANDALONE
	 if (IS_RIP_DEBUG_EVENT)
		zlog_info("RIPDS:rib_add_ipv4 is called");
	#endif

    /* Make it sure prefixlen is applied to the prefix. */
    apply_mask_ipv4 (p);

	rip_redistribute_add (type,RIP_ROUTE_REDISTRIBUTE, p, ifindex, gate);
	return 0;
}


int
rib_delete_ipv4 (int type, int flags, struct prefix_ipv4 *p,
         struct in_addr *gate, unsigned int ifindex, u_int32_t vrf_id)
{
	#ifdef DEBUG_RIPD_STANDALONE
	 if (IS_RIP_DEBUG_EVENT)
		zlog_info("RIPDS:rib_delete_ipv4 is called ");
	#endif

    /* Apply mask. */
    apply_mask_ipv4 (p);

    rip_redistribute_delete (type,RIP_ROUTE_REDISTRIBUTE, p, ifindex);
	return 0;
}

#ifdef HAVE_IPV6
int
rib_add_ipv6 (int type, int flags, struct prefix_ipv6 *p,
          struct in_addr *gate, unsigned int ifindex, u_int32_t vrf_id,
		            u_int32_t metric, u_char distance)
{ 
	return 0;
}


int
rib_delete_ipv6 (int type, int flags, struct prefix_ipv6 *p,
         struct in_addr *gate, unsigned int ifindex, u_int32_t vrf_id)
{
	return 0;
}
#endif

