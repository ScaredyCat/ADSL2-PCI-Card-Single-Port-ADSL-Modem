#ifdef VTY_REMOVE

#include "rip_read_config.h"

/***** COMMANDS CUPPORTED *****/

#if 0

enable/config commands:
	debug rip events
	debug rip packet
	debug rip packet (send/recv)
	**debug rip packet (send/recv) detail
    
    log stdout
	log file FILENAME
	log syslog

interface commands:
	interface IFNAME
	ip rip send version (1|2)
	ip rip send version 1 2
	ip rip send version 2 1
	ip rip receive version (1|2)
	ip rip receive version (1 2)
	ip rip receive version (2 1)
	ip rip authentication mode (md5|text)
	ip rip authentication key-chain LINE
	ip rip authentication string LINE
	ip split-horizon

rip commands:
	network <prefix/ifname>
	neighbor <prefix>
	default-information originate
	version <1|2>
	passive-interface IFNAME
	default-metric <1-16>
	timers basic <5-2147483647> <5-2147483647> <5-2147483647>
	route A.B.C.D/M
	distance <1-255>
	distance <1-255> A.B.C.D/M
	distance <1-255> A.B.C.D/M WORD  {WORD: accesslist name}
	
#endif


#ifdef DEBUG_RIPD_STANDALONE	
/****   ENABLE COMMANDS ***/
/* debug rip events */

/* debug rip events */
int debug_rip_events(struct interface *ifp,int argc,char **argv)
{
  rip_debug_event = RIP_DEBUG_EVENT;
  zebra_debug_kernel = ZEBRA_DEBUG_KERNEL;
  
  return CMD_SUCCESS;
}


/* debug rip packet */
int debug_rip_packet(struct interface *ifp,int argc,char **argv)
{
	if ( argc == 0 ) {
		debug_rip_packet_all(NULL,0,NULL);
	}else {
		debug_rip_packet_direct(NULL,argc,argv);
	}
	return CMD_SUCCESS;
}

/* debug rip packet */
int debug_rip_packet_all(struct interface *ifp,int argc,char **argv)
{
  rip_debug_packet = RIP_DEBUG_PACKET;
  rip_debug_packet |= RIP_DEBUG_SEND;
  rip_debug_packet |= RIP_DEBUG_RECV;
  return CMD_SUCCESS;
}

/* debug rip packet (recv|send) */
int debug_rip_packet_direct(struct interface *ifp,int argc,char **argv)
{
  rip_debug_packet |= RIP_DEBUG_PACKET;
  if (strncmp ("send", argv[0], strlen (argv[0])) == 0)
    rip_debug_packet |= RIP_DEBUG_SEND;
  if (strncmp ("recv", argv[0], strlen (argv[0])) == 0)
    rip_debug_packet |= RIP_DEBUG_RECV;
  rip_debug_packet &= ~RIP_DEBUG_DETAIL;
  return CMD_SUCCESS;
}
/* debug rip packet (recv|send) detail */
int debug_rip_packet_detail(struct interface *ifp,int argc,char **argv)
{
  rip_debug_packet |= RIP_DEBUG_PACKET;
  if (strncmp ("send", argv[0], strlen (argv[0])) == 0)
    rip_debug_packet |= RIP_DEBUG_SEND;
  if (strncmp ("recv", argv[0], strlen (argv[0])) == 0)
    rip_debug_packet |= RIP_DEBUG_RECV;
  rip_debug_packet |= RIP_DEBUG_DETAIL;
  return CMD_SUCCESS;
}

#endif

/* log stdout */
int config_log_stdout(struct interface *ifp,int argc,char **argv)
{
  zlog_set_flag (NULL, ZLOG_STDOUT);
  return CMD_SUCCESS;
}

/* log file FILENAME */

int config_log_file(struct interface *ifp,int argc,char **argv)
{
  int ret;
  char *cwd;
  char *fullpath;

  /* Path detection. */
  if (! IS_DIRECTORY_SEP (*argv[0]))
    {
      cwd = getcwd (NULL, MAXPATHLEN);
      fullpath = XMALLOC (MTYPE_TMP,
			  strlen (cwd) + strlen (argv[0]) + 2);
      sprintf (fullpath, "%s/%s", cwd, argv[0]);
    }
  else
    fullpath = argv[0];

  ret = zlog_set_file (NULL, ZLOG_FILE, fullpath);

  if (!ret)
    {
      //vty_out (vty, "can't open logfile %s\n", argv[0]);
      return CMD_WARNING;
    }

  return CMD_SUCCESS;
}

/*  log syslog */

int config_log_syslog(struct interface *ifp,int argc,char **argv)
{
  zlog_set_flag (NULL, ZLOG_SYSLOG);
  zlog_default->facility = LOG_DAEMON;
  return CMD_SUCCESS;
}



/****  RIP_COMMANDS *****/
/* "network (A.B.C.D/M|WORD)" */

int rip_network(struct interface *ifp,int argc,char **argv) {
   int ret;
   struct prefix_ipv4 p;

   ret = str2prefix_ipv4 (argv[0], &p);

   if (ret)
     ret = rip_enable_network_add ((struct prefix *) &p);
   else
     ret = rip_enable_if_add (argv[0]);

   if (ret < 0){
      return CMD_WARNING;
   }

   rip_enable_apply_all ();
   return CMD_SUCCESS;

}

/* default-information originate */
int rip_default_information_originate(struct interface *ifp,int argc,char **argv)
{
  struct prefix_ipv4 p;

  if (! rip->default_information)
    {
      memset (&p, 0, sizeof (struct prefix_ipv4));
      p.family = AF_INET;

      rip->default_information = 1;
  
      rip_redistribute_add (ZEBRA_ROUTE_RIP, RIP_ROUTE_STATIC, &p, 0, NULL);
    }

  return CMD_SUCCESS;
}



/*  "neighbor A.B.C.D" */
int rip_neighbor(struct interface *ifp,int argc,char **argv) {
  int ret;
  struct prefix_ipv4 p;

  ret = str2prefix_ipv4 (argv[0], &p);

  if (ret <= 0)
    {
      //vty_out (vty, "Please specify address by A.B.C.D%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  rip_neighbor_add (&p);
  
  return CMD_SUCCESS;

}

/* "passive-interface IFNAME" */
int rip_passive_interface(struct interface *ifp,int argc,char **argv) {
  return rip_passive_interface_set (NULL,argv[0] );
}


/* "version <1-2>" */
int rip_version(struct interface *ifp,int argc,char **argv) {
  int version;

  version = atoi (argv[0]);
  if (version != RIPv1 && version != RIPv2)
    {
      //vty_out (vty, "invalid rip version %d%s", version,
	  //     VTY_NEWLINE);
      return CMD_WARNING;
    }
  rip->version = version;

  return CMD_SUCCESS;

}

/* "default-metric <1-16>" */

int rip_default_metric(struct interface *ifp,int argc,char **argv)
{
  if (rip)
    {
      rip->default_metric = atoi (argv[0]);
      /* rip_update_default_metric (); */
    }
  return CMD_SUCCESS;
}

/* "timers basic <5-2147483647> <5-2147483647> <5-2147483647>" */
int rip_timers( struct interface *ifp,int argc,char **argv )
{
  unsigned long update;
  unsigned long timeout;
  unsigned long garbage;
  char *endptr = NULL;
  unsigned long RIP_TIMER_MAX = 2147483647;
  unsigned long RIP_TIMER_MIN = 5;

  update = strtoul (argv[0], &endptr, 10);
  if (update > RIP_TIMER_MAX || update < RIP_TIMER_MIN || *endptr != '\0')  
    {
      //vty_out (vty, "update timer value error%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  
  timeout = strtoul (argv[1], &endptr, 10);
  if (timeout > RIP_TIMER_MAX || timeout < RIP_TIMER_MIN || *endptr != '\0') 
    {
      //vty_out (vty, "timeout timer value error%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  
  garbage = strtoul (argv[2], &endptr, 10);
  if (garbage > RIP_TIMER_MAX || garbage < RIP_TIMER_MIN || *endptr != '\0') 
    {
      //vty_out (vty, "garbage timer value error%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  /* Set each timer value. */
  rip->update_time = update;
  rip->timeout_time = timeout;
  rip->garbage_time = garbage;

  /* Reset update timer thread. */
  rip_event (RIP_UPDATE_EVENT, 0);

  return CMD_SUCCESS;
}

/* "route A.B.C.D/M" */
int rip_route( struct interface *ifp,int argc,char **argv ) 
{
  int ret;
  struct prefix_ipv4 p;
  struct route_node *node;

  ret = str2prefix_ipv4 (argv[0], &p);
  if (ret < 0)
    {
      //vty_out (vty, "Malformed address%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  apply_mask_ipv4 (&p);

  /* For router rip configuration. */
  node = route_node_get (rip->route, (struct prefix *) &p);

  if (node->info)
    {
      //vty_out (vty, "There is already same static route.%s", VTY_NEWLINE);
      route_unlock_node (node);
      return CMD_WARNING;
    }

  node->info = "static";

  rip_redistribute_add (ZEBRA_ROUTE_RIP, RIP_ROUTE_STATIC, &p, 0, NULL);

  return CMD_SUCCESS;
}

/* "distance <1-255>" */
int rip_distance(struct interface *ifp, int argc,char **argv )
{
  rip->distance = atoi (argv[0]);
  return CMD_SUCCESS;
}

/*"distance <1-255> A.B.C.D/M" */

int rip_distance_source(struct interface *ifp,int argc,char **argv ) 
{
  return rip_distance_set (NULL, argv[0], argv[1], NULL);
  
}

/*"distance <1-255> A.B.C.D/M WORD" */
int rip_distance_source_access_list(struct interface *ifp,int argc,char **argv )
{
  return rip_distance_set (NULL,argv[0], argv[1], argv[2] );
}


/****  CONFIG COMMANDS *******/

/*** Distribute-list commands */
/* distribute-list WORD (in|out)" */
int distribute_list_all(struct interface *ifp,int argc,char **argv )
{
  enum distribute_type type;
  struct distribute *dist;

  /* Check of distribute list type. */
  if (strncmp (argv[1], "i", 1) == 0)
    type = DISTRIBUTE_IN;
  else if (strncmp (argv[1], "o", 1) == 0)
    type = DISTRIBUTE_OUT;
  else
    {
      //vty_out (vty, "distribute list direction must be [in|out]%s",
	  //     VTY_NEWLINE);
      return CMD_WARNING;
    }

  /* Get interface name corresponding distribute list. */
  dist = distribute_list_set (NULL, type, argv[0]);

  return CMD_SUCCESS;
}

/*  distribute-list WORD (in|out) WORD",
 */
int distribute_list(struct interface *ifp,int argc,char **argv )
{
  enum distribute_type type;
  struct distribute *dist;

  /* Check of distribute list type. */
  if (strncmp (argv[1], "i", 1) == 0)
    type = DISTRIBUTE_IN;
  else if (strncmp (argv[1], "o", 1) == 0)
    type = DISTRIBUTE_OUT;
  else
    {
      //vty_out (vty, "distribute list direction must be [in|out]%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  /* Get interface name corresponding distribute list. */
  dist = distribute_list_set (argv[2], type,argv[0]);

  return CMD_SUCCESS;
}       
/* distribute-list prefix WORD (in|out)", */

int districute_list_prefix_all(struct interface *ifp,int argc,char **argv )
{
  enum distribute_type type;
  struct distribute *dist;

  /* Check of distribute list type. */
  if (strncmp (argv[1], "i", 1) == 0)
    type = DISTRIBUTE_IN;
  else if (strncmp (argv[1], "o", 1) == 0)
    type = DISTRIBUTE_OUT;
  else
    {
      //vty_out (vty, "distribute list direction must be [in|out]%s", 
	  //     VTY_NEWLINE);
      return CMD_WARNING;
    }

  /* Get interface name corresponding distribute list. */
  dist = distribute_list_prefix_set (NULL, type, argv[0]);

  return CMD_SUCCESS;
}       

/** distribute-list prefix WORD (in|out) WORD", **/

int districute_list_prefix(struct interface *ifp,int argc,char **argv )
{
  enum distribute_type type;
  struct distribute *dist;

  /* Check of distribute list type. */
  if (strncmp (argv[1], "i", 1) == 0)
    type = DISTRIBUTE_IN;
  else if (strncmp (argv[1], "o", 1) == 0)
    type = DISTRIBUTE_OUT;
  else
    {
      //vty_out (vty, "distribute list direction must be [in|out]%s", 
	  //     VTY_NEWLINE);
      return CMD_WARNING;
    }

  /* Get interface name corresponding distribute list. */
  dist = distribute_list_prefix_set (argv[2], type, argv[0]);

  return CMD_SUCCESS;
}       


/* INTERFACE COMMANDS */

/* "interface IFNAME" */
int rip_interface( struct interface *intf,int argc, char **argv)
{
  struct interface *ifp;

  ifp = if_lookup_by_name (argv[0]);

  if (ifp == NULL)
    {
      ifp = if_create ();
	  #ifdef RIPD_MEMORY_FIX
	  if ( !ifp ) {
  		return CMD_WARNING;
	  }
	  #endif

      strncpy (ifp->name, argv[0], INTERFACE_NAMSIZ);
	}
  //intf = ifp;
  return CMD_SUCCESS;
}

/** Before calling any of the below methods,
 * please call
 *   ifp = if_lookup_by_name (intf);
 *     if (!ifp ) {
 *	  return CMD_WARNING;
 *    }
 *	 This will return the ifp structure;
 *	 pass this ifp to any/all of the below calls
 */
 
/*"ip rip send version (1|2)",
  */

int ip_rip_send_version( struct interface *ifp,int argc, char **argv)
{
  struct rip_interface *ri;

  ri = ifp->info;

  /* Version 1. */
  if (atoi (argv[0]) == 1)
    {
      ri->ri_send = RI_RIP_VERSION_1;
      return CMD_SUCCESS;
    }
  if (atoi (argv[0]) == 2)
    {
      ri->ri_send = RI_RIP_VERSION_2;
      return CMD_SUCCESS;
    }
  return CMD_WARNING;
}

/*"ip rip send version 1 2",
*/
int ip_rip_send_version_1(struct interface *ifp,int argc, char **argv)
{
  struct rip_interface *ri;

  ri = ifp->info;

  /* Version 1 and 2. */
  ri->ri_send = RI_RIP_VERSION_1_AND_2;
  return CMD_SUCCESS;
}
/* "ip rip send version 2 1",
*/
int ip_rip_send_version_2(struct interface *ifp,int argc, char **argv)
{
  struct rip_interface *ri;

  ri = ifp->info;

  /* Version 1 and 2. */
  ri->ri_send = RI_RIP_VERSION_1_AND_2;
  return CMD_SUCCESS;
}

/* "ip rip receive version (1|2)" */
int ip_rip_receive_version(struct interface *ifp,int argc, char **argv)
{
  struct rip_interface *ri;

  ri = ifp->info;

  /* Version 1. */
  if (atoi (argv[0]) == 1)
    {
      ri->ri_receive = RI_RIP_VERSION_1;
      return CMD_SUCCESS;
    }
  if (atoi (argv[0]) == 2)
    {
      ri->ri_receive = RI_RIP_VERSION_2;
      return CMD_SUCCESS;
    }
  return CMD_WARNING;
}

/**ip rip receive version 1 2",*/
int ip_rip_receive_version_1(struct interface *ifp,int argc, char **argv )
{
  struct rip_interface *ri;

  ri = ifp->info;

  /* Version 1 and 2. */
  ri->ri_receive = RI_RIP_VERSION_1_AND_2;
  return CMD_SUCCESS;
}

/** "ip rip receive version 2 1", **/
int ip_rip_receive_version_2(struct interface *ifp,int argc, char **argv)
{
  struct rip_interface *ri;
  
  ri = ifp->info;

  /* Version 1 and 2. */
  ri->ri_receive = RI_RIP_VERSION_1_AND_2;
  return CMD_SUCCESS;
}

/** "ip rip authentication mode (md5|text)" **/

int ip_rip_authentication_mode(struct interface *ifp,int argc, char **argv)
{
  struct rip_interface *ri;
  
  ri = ifp->info;

  if (strncmp ("md5", argv[0], strlen (argv[0])) == 0)
    ri->auth_type = RIP_AUTH_MD5;
  else if (strncmp ("text", argv[0], strlen (argv[0])) == 0)
    ri->auth_type = RIP_AUTH_SIMPLE_PASSWORD;
  else
    {
      //vty_out (vty, "mode should be md5 or text%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  return CMD_SUCCESS;
}

/**"ip rip authentication key-chain LINE", */
int ip_rip_authentication_key_chain(struct interface *ifp,int argc, char **argv )
{
  struct rip_interface *ri;

  ri = ifp->info;

  if (ri->auth_str)
    {
	  #ifndef VTY_REMOVE 
	  vty_out (vty, "%% authentication string configuration exists%s",
	       VTY_NEWLINE);
      #endif
      return CMD_WARNING;
    }

  if (ri->key_chain)
    free (ri->key_chain);

  ri->key_chain = strdup (argv[0]);

  return CMD_SUCCESS;
}


/** "ip rip authentication string LINE" **/
int ip_rip_authentication_string(struct interface *ifp,int argc, char **argv )
{
  struct rip_interface *ri;

  ri = ifp->info;

  if (strlen (argv[0]) > 16)
    { 
	  #ifndef VTY_REMOVE	
      vty_out (vty, "%% RIPv2 authentication string must be shorter than 16%s",
	       VTY_NEWLINE);
	  #endif
      return CMD_WARNING;
    }

  if (ri->key_chain)
    {
      //vty_out (vty, "%% key-chain configuration exists%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  if (ri->auth_str)
    free (ri->auth_str);

  ri->auth_str = strdup (argv[0]);

  return CMD_SUCCESS;
}
/*ip split-horizon*/
int rip_split_horizon(struct interface *ifp,int argc, char **argv)
{
  struct rip_interface *ri;

  ri = ifp->info;

  ri->split_horizon = 1;
  return CMD_SUCCESS;
}

FILE *
use_backup_config (char *fullpath)
{
  char *fullpath_sav, *fullpath_tmp;
  FILE *ret = NULL;
  struct stat buf;
  int tmp, sav;
  int c;
  char buffer[512];
  
  fullpath_sav = malloc (strlen (fullpath) + strlen (CONF_BACKUP_EXT) + 1);
  strcpy (fullpath_sav, fullpath);
  strcat (fullpath_sav, CONF_BACKUP_EXT);
  if (stat (fullpath_sav, &buf) == -1)
    {
      free (fullpath_sav);
      return NULL;
    }

  fullpath_tmp = malloc (strlen (fullpath) + 8);
  sprintf (fullpath_tmp, "%s.XXXXXX", fullpath);
  
  /* Open file to configuration write. */
  tmp = mkstemp (fullpath_tmp);
  if (tmp < 0)
    {
      free (fullpath_sav);
      free (fullpath_tmp);
      return NULL;
    }

  sav = open (fullpath_sav, O_RDONLY);
  if (sav < 0)
    {
      free (fullpath_sav);
      free (fullpath_tmp);
      unlink (fullpath_tmp);
      return NULL;
    }
  
  while((c = read (sav, buffer, 512)) > 0)
    write (tmp, buffer, c);
  
  close (sav);
  close (tmp);
  
  if (link (fullpath_tmp, fullpath) == 0)
    ret = fopen (fullpath, "r");

  unlink (fullpath_tmp);
  
  free (fullpath_sav);
  free (fullpath_tmp);
  return fopen (fullpath, "r");
}

void rip_read_config(char *config_file, char *config_current_dir, char *config_default_dir) 
{

  char *cwd;
  FILE *confp = NULL;
  char *fullpath;
  
  /* If -f flag specified. */
  if (config_file != NULL)
    {
      if (! IS_DIRECTORY_SEP (config_file[0]))
	{
	  cwd = getcwd (NULL, MAXPATHLEN);
	  fullpath = XMALLOC (MTYPE_TMP, 
			      strlen (cwd) + strlen (config_file) + 2);
	  sprintf (fullpath, "%s/%s", cwd, config_file);
	}
      else
	fullpath = config_file;

      confp = fopen (fullpath, "r");

      if (confp == NULL)
	{
	  confp = use_backup_config (fullpath);
	  if (confp)
	    fprintf (stderr, "WARNING: using backup configuration file!\n");
	  else
	    {
	      fprintf (stderr, "can't open configuration file [%s]\n", 
		       config_file);
	      exit(1);
	    }
	}
    }
  else
    {
      /* Relative path configuration file open. */
      if (config_current_dir)
	{
	  confp = fopen (config_current_dir, "r");
	  if (confp == NULL)
	    {
	      confp = use_backup_config (config_current_dir);
	      if (confp)
		fprintf (stderr, "WARNING: using backup configuration file!\n");
	    }
	}

      /* If there is no relative path exists, open system default file. */
      if (confp == NULL)
	{

	  confp = fopen (config_default_dir, "r");
	  if (confp == NULL)
	    {
	      confp = use_backup_config (config_default_dir);
	      if (confp)
		{
		  fprintf (stderr, "WARNING: using backup configuration file!\n");
		  fullpath = config_default_dir;
		}
	      else
		{
		  fprintf (stderr, "can't open configuration file [%s]\n",
			   config_default_dir);
		  exit (1);
		}
	    }      
	  else
	    fullpath = config_default_dir;
	}
      else
	{
	  /* Rleative path configuration file. */
	  cwd = getcwd (NULL, MAXPATHLEN);
	  fullpath = XMALLOC (MTYPE_TMP, 
			      strlen (cwd) + strlen (config_current_dir) + 2);
	  sprintf (fullpath, "%s/%s", cwd, config_current_dir);
	}  
    }  

	rip_read_config_file(confp);
	fclose (confp);

}

#if 0 //used for testing purpose --chandrav
int rip_read_config_test( char *config_file, char *config_current_dir, char *config_default_dir ) 
{
struct interface *ifp;

char *argv1[CMD_ARGC_MAX];
char *argv2[CMD_ARGC_MAX];
char *argv3[CMD_ARGC_MAX];
char *argv5[CMD_ARGC_MAX];
char *argv6[CMD_ARGC_MAX];

argv1[0]= malloc(100);
argv2[0]= malloc(100);
argv3[0]= malloc(100);
argv5[0]= malloc(100);
argv6[0]= malloc(100);


strcpy(argv1[0],"eth1");
strcpy(argv5[0],"lo");
//strcpy(argv6[0],"172.20.22.78/24");
strcpy(argv6[0],"eth1");

strcpy(argv2[0],"2");
strcpy(argv3[0],"20.18.18.18");

printf("before calling rip_interface");
//----interface eth0
rip_interface(NULL,1,argv1);
    //  --- ip rip send version 2
	//  --- ip rip receive version 2  
printf("called rip_interface");

    ifp = if_lookup_by_name ("eth1");
    if (!ifp ) {
 	  return CMD_WARNING;
    }
	ip_rip_send_version(ifp,0,argv2);
    ip_rip_receive_version(ifp,0,argv2);

#ifdef DEBUG_RIPD_STANDALONE
// ---- debug rip packet,events
debug_rip_events(NULL,0,NULL);
debug_rip_packet(NULL,0,NULL);
#endif

//---- log stdout
config_log_stdout(NULL,0,NULL);



//--- interface lo
rip_interface(NULL,1,argv5);
    //  --- ip rip send version 2
	//  --- ip rip receive version 2  
    ifp = if_lookup_by_name ("lo");
    if (!ifp ) {
 	  return CMD_WARNING;
    }
	ip_rip_send_version(ifp,0,argv2);
    ip_rip_receive_version(ifp,0,argv2);
	
	
//----  network eth0
rip_network(NULL,0,argv6);
// -- after router rip command ---
rip_version(NULL,0,argv2);
rip_neighbor(NULL,0,argv3);
return 0;
}
#endif //#if 0


#endif /* VTY_REMOVE */
