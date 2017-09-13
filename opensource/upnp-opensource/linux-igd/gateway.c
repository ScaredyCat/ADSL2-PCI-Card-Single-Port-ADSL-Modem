/******************************************************************************
*  Copyright (c) 2002 Linux UPnP Internet Gateway Device Project              *    
*  All rights reserved.                                                       *
*                                                                             *   
*  This file is part of The Linux UPnP Internet Gateway Device (IGD).         *
*                                                                             *
*  The Linux UPnP IGD is free software; you can redistribute it and/or modify *
*  it under the terms of the GNU General Public License as published by       *
*  the Free Software Foundation; either version 2 of the License, or          *
*  (at your option) any later version.                                        *
*                                                                             *    
*  The Linux UPnP IGD is distributed in the hope that it will be useful,      *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*  GNU General Public License for more details.                               *
*                                                                             *   
*  You should have received a copy of the GNU General Public License          * 
*  along with Foobar; if not, write to the Free Software                      *
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  *
*                                                                             *  
*                                                                             *  
******************************************************************************/
// 
// Special thanks to Genmei Mori and his team for the work he done on the
// ipchains version of this code. .
//

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#include <syslog.h>
#include <signal.h>
#include "upnp/upnp.h"
#include "gateway.h"
#include "ipcon.h"
//#include "sample_util.h"
#include "portmap.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "pmlist.h"
#include "gate.h"
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <net/if.h>
//#include <sys/sockio.h>

#define RUNFILE "/var/run/upnpd.pid"
#define WANINFO "/var/run/upnpd/wanifo"
// The global GATE object
//Gate gate;
struct Gate gate;

extern char *GateServiceId[];
int wan_connected_status,term_signal=0;
unsigned long last_ip=0;
char *uuid_wanipconn="uuid:608007c2-cd7a-4242-80a2-ec2aab3d3369";
static void create_pidfile();

static void wan_state_change_handler(int signum)
{
	//syslog(LOG_INFO,"UPNPD:-----signum=%d--SIGUSR1=%d--SIGTERM=%d\n",signum,SIGUSR1,SIGTERM);
	if(signum == SIGTERM)
		term_signal=1;
	if(signum == SIGUSR1) //rebuild the upnp port_map list
		PortMapList_Rebuild(&(gate.m_list));
	return;
}	
// Callback Function wrapper.  This is needed because ISO forbids a pointer to a bound
// member function.  This corrects the issue.
int CallbackEventHandler(Upnp_EventType EventType, void *Event, void *Cookie)
{
//	return gate.GateDeviceCallbackEventHandler(EventType, Event, Cookie);
	return GateDeviceCallbackEventHandler(&gate, EventType, Event, Cookie);
}

int substr(char *docpath, char *infile, char *outfile, char *str_from, char *str_to);

int main (int argc, char** argv)
{
	char *desc_doc_name=NULL, *conf_dir_path=NULL;
	char lan_ip_address[16];
	char desc_doc_url[200];
	char if_buf[16];
	int sig;
	sigset_t sigs_to_catch;
	struct sigaction sa;
	int port;
	int ret,loop=0;
	char *address;
	struct sockaddr_in sin;
  int sock;
  Upnp_Document PropSet=NULL;
//	pid_t pid,sid;

	// Log startup of daemon
	syslog(LOG_INFO, "UPnP Internet Gateway Device start up!");
//	syslog(LOG_INFO, "Special Thanks for Intel's Open Source SDK and original author Genmei Mori's work.");
	
	if (argc != 3)
	{
		printf("Usage: upnpd <external ifname> <internal ifname>\n");
		printf("Example: upnpd ppp0 eth0\n");
		printf("Example: upnpd eth1 eth0\n");
		exit(0);
	}

	if (geteuid()) {
		fprintf(stderr, "upnpd must be run as root\n");
		exit(EXIT_FAILURE);
	}
#if 0
	pid = fork();
        if (pid < 0)
        {
                perror("Error forking a new process.");
                exit(EXIT_FAILURE);
        }
        if (pid > 0)
                exit(EXIT_SUCCESS);

        if ((sid = setsid()) < 0)
        {
                perror("Error running setsid");
                exit(EXIT_FAILURE);
        }
#endif
        if ((chdir("/")) < 0)
        {
                perror("Error setting root directory");
                exit(EXIT_FAILURE);
        }

        umask(0);

        //close (STDOUT_FILENO);
        //close (STDERR_FILENO);
	wan_connected_status = 0;
	
	/* Zero out the gate structure */
	memset(&gate, 0, sizeof(struct Gate));

//	gate.m_ipcon = new IPCon(argv[2]);
	gate.m_ipcon = (struct IPCon *)malloc(sizeof(struct IPCon));
	IPCon_Construct(gate.m_ipcon, argv[2]);

//	address = gate.m_ipcon->IPCon_GetIpAddrStr();
	address = IPCon_GetIpAddrStr(gate.m_ipcon);
	strcpy(lan_ip_address, address);
	if (address) free(address);
	IPCon_Destruct(gate.m_ipcon);
	free(gate.m_ipcon);
	
	port = INIT_PORT;
	desc_doc_name=INIT_DESC_DOC;
	conf_dir_path=INIT_CONF_DIR;

	sprintf(desc_doc_url, "http://%s:%d/%s.xml", lan_ip_address, port,desc_doc_name);
   	//syslog(LOG_DEBUG, "Intializing UPnP with desc_doc_url=%s\n",desc_doc_url);
	//syslog(LOG_DEBUG, "ipaddress=%s port=%d\n", lan_ip_address, port);
	//syslog(LOG_DEBUG, "conf_dir_path=%s\n", conf_dir_path);
        substr(conf_dir_path, "gatedesc.skl", "gatedesc.skl1", "!ADDR!",lan_ip_address);
        
#ifdef _VENDERID 
				substr(conf_dir_path, "gatedesc.skl1", "gatedesc.skl2", "!VENDER!",_VENDERID);
#else
      	substr(conf_dir_path, "gatedesc.skl1", "gatedesc.skl2", "!VENDER!","VENDOR");
#endif 
#ifdef _PRODUCTNAME
			 substr(conf_dir_path, "gatedesc.skl2", "gatedesc.skl1", "!PRODUCTNAME!",_PRODUCTNAME);	
#else
			 substr(conf_dir_path, "gatedesc.skl2", "gatedesc.skl1", "!PRODUCTNAME!","VoIP ATA");	
#endif
#ifdef _WEBURL
				substr(conf_dir_path, "gatedesc.skl1", "gatedesc.xml", "!WEBURL!",_WEBURL);
#else
				substr(conf_dir_path, "gatedesc.skl1", "gatedesc.xml", "!WEBURL!","www.company.com");
#endif 

	if ((ret = UpnpInit(lan_ip_address, port)) != UPNP_E_SUCCESS)
	{
		syslog(LOG_ERR, "Error with UpnpInit -- %d\n", ret);
		UpnpFinish();
		exit(1);
	}
	syslog(LOG_DEBUG, "UPnP Initialization Completed");
	
	//syslog(LOG_DEBUG, "Setting webserver root directory -- %s\n",conf_dir_path);
	if ((ret = UpnpSetWebServerRootDir(conf_dir_path)) != UPNP_E_SUCCESS)
	{
		syslog(LOG_ERR, "Error setting webserver root directory -- %s: %d\n",
			       	conf_dir_path, ret);
		UpnpFinish();
		exit(1);
	}

//	gate.m_ipcon = new IPCon(argv[1]);
	gate.m_ipcon = (struct IPCon *) malloc(sizeof(struct IPCon));
	IPCon_Construct(gate.m_ipcon, argv[1]);

	//syslog(LOG_DEBUG, "Registering the root device\n");
	if ((ret = UpnpRegisterRootDevice(desc_doc_url, CallbackEventHandler,
				&gate.device_handle, &gate.device_handle)) != UPNP_E_SUCCESS)
	{
		syslog(LOG_ERR, "Error registering the rootdevice : %d\n", ret);
		UpnpFinish();
		exit(1);
	}
	else
	{
		//syslog(LOG_DEBUG, "RootDevice Registered\n");
		//syslog(LOG_DEBUG, "Initializing State Table\n");
//		gate.GateDeviceStateTableInit(desc_doc_url);
		GateDeviceStateTableInit(&gate, desc_doc_url);
		//syslog(LOG_DEBUG, "State Table Initialized\n");
    /* yfchou ++ send bye-bye when init */
		upnp_send_byebye(desc_doc_url);
		upnp_send_byebye(desc_doc_url);		
		/* yfchou -- send bye-bye when init */
		
		if ((ret = UpnpSendAdvertisement(gate.device_handle, 1800))
			!= UPNP_E_SUCCESS)
		{
//			syslog(LOG_ERR, "Error sending advertisements : %d\n", ret);
			UpnpFinish();
			exit(1);
		}

//		syslog(LOG_DEBUG, "Advertisements Sent\n");
	}
	create_pidfile();
	gate.startup_time = time(NULL);
	
	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGINT);
	sigaddset(&sigs_to_catch, SIGTERM);
	sigaddset( &sigs_to_catch, SIGUSR1 );
  sigaddset( &sigs_to_catch, SIGUSR2 );
  
	sa.sa_mask = sigs_to_catch;
  sa.sa_flags = 0;
  sa.sa_handler = wan_state_change_handler;
  sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	
	while(!term_signal)
	{
			PortMapList_ScanLease(&(gate.m_list));
			sock = socket(AF_INET, SOCK_STREAM, 0);
			get_if_name(if_buf);
			if(get_if_addr(sock, &if_buf[0], &sin) == 0)
    	{
    			if(!wan_connected_status)
    			{
    				wan_connected_status = 1;
    				syslog(LOG_INFO, "UPNPD: wan status change to Connected");
    				PropSet= UpnpCreatePropertySet(1,"ConnectionStatus", "Connected");
    				ret = UpnpNotifyExt(gate.device_handle, uuid_wanipconn ,GateServiceId[GATE_SERVICE_CONNECT],PropSet);
    				//syslog(LOG_INFO, "UPNPD:ret=%d",ret);
    				UpnpDocument_free(PropSet);
    				
    				IPCon_Destruct(gate.m_ipcon);
						free(gate.m_ipcon);
						gate.m_ipcon = (struct IPCon *)malloc(sizeof(struct IPCon));
						IPCon_Construct(gate.m_ipcon, if_buf);
    			}	
    			if(last_ip!=sin.sin_addr.s_addr)
    			{
    				PropSet= UpnpCreatePropertySet(1,"ExternalIPAddress", inet_ntoa(sin.sin_addr));
    				ret = UpnpNotifyExt(gate.device_handle, uuid_wanipconn ,GateServiceId[GATE_SERVICE_CONNECT],PropSet);
    				//syslog(LOG_INFO, "UPNPD:ret=%d",ret);
    				UpnpDocument_free(PropSet);
    				last_ip=sin.sin_addr.s_addr;
    			}	
  		}	
  		else
  		{
  				if(wan_connected_status)
  				{
  					wan_connected_status = 0;	
  					syslog(LOG_INFO, "UPNPD: wan status change to Disconnected");
  					PropSet= UpnpCreatePropertySet(1,"ConnectionStatus", "Disconnected");
    				ret = UpnpNotifyExt(gate.device_handle, uuid_wanipconn ,GateServiceId[GATE_SERVICE_CONNECT],PropSet);
    				//syslog(LOG_INFO, "UPNPD:ret=%d",ret);
    				UpnpDocument_free(PropSet);
  				}	
  			
  		}	
  		close(sock);
			sleep(1);
	}	
	
	syslog(LOG_INFO, "UPNPD: Shutting down on signal ...\n");
	
	UpnpUnRegisterRootDevice(gate.device_handle);
	UpnpFinish();

	/* Delete the port we opened */
	PortMapList_Destruct(&(gate.m_list));
	/* Clean up the Gate object */
	GateDeviceDestruct(&gate);

	exit(0);

}	

int substr(char *docpath, char *infile, char *outfile, char *str_from, char *str_to)
{
	FILE *fpi, *fpo;
	char pathi[256], patho[256];
	char buffi[4096], buffo[4096];
	int len_buff, len_from, len_to;
	int i, j;

	sprintf(pathi, "%s%s", docpath, infile);
	if ((fpi = fopen(pathi,"r")) == NULL) 
	{
		printf("input file can not open\n");
		return (-1);
	}

	sprintf(patho, "%s%s", docpath, outfile);
	if ((fpo = fopen(patho,"w")) == NULL) {
		printf("output file can not open\n");
		fclose(fpi);
		return (-1);
	}

	len_from = strlen(str_from);
	len_to   = strlen(str_to);

	while (fgets(buffi, 4096, fpi) != NULL) 
	{
		len_buff = strlen(buffi);
		for (i=0, j=0; i <= len_buff-len_from; i++, j++) 
		{
			if (strncmp(buffi+i, str_from, len_from)==0) 
			{
				strcpy (buffo+j, str_to);
				i += len_from - 1;
				j += len_to - 1;
			} else
			*(buffo + j) = *(buffi + i);
		}
		strcpy(buffo + j, buffi + i);
		fputs(buffo, fpo);
	}

	fclose(fpo);
	fclose(fpi);
	return (0);
}

static void
create_pidfile()
{
    FILE *pidfile;

    if ((pidfile = fopen(RUNFILE, "w")) != NULL) {
			fprintf(pidfile, "%d\n", getpid());
		(void) fclose(pidfile);
    } else {
			error("Failed to create pid file %s: %m", RUNFILE);
    }
}
int get_if_name(char *buf)
{
	 FILE *pidfile;

   if ((pidfile = fopen(WANINFO, "rb")) != NULL) {
   		fgets(buf, 1024, pidfile);
   		(void) fclose(pidfile);
   		if(strncmp(buf,"ppp1",4)==0)
   			buf[4]=0;
   		else if(strncmp(buf,"nas1",4)==0)	
   			buf[4]=0;
   		else
   			return -1;	
   		return 0;
   } else {
   		return -1;
   }
	
	
}	
int get_if_addr(int sock, char *name, struct sockaddr_in *sin)
{
  struct ifreq ifr;

  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, name);
  /* why does this need to be done twice? */
  if(ioctl(sock, SIOCGIFADDR, &ifr) < 0) 
  { 
    perror("ioctl(SIOCGIFADDR)"); 
    memset(sin, 0, sizeof(struct sockaddr_in));
    //dprintf((stderr, "%s: %s\n", name, "unknown interface"));
    return -1;
  }
  if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
  { 
    perror("ioctl(SIOCGIFADDR)"); 
    memset(sin, 0, sizeof(struct sockaddr_in));
    //dprintf((stderr, "%s: %s\n", name, "unknown interface"));
    return -1;
  }

  if(ifr.ifr_addr.sa_family == AF_INET)
  {
    memcpy(sin, &(ifr.ifr_addr), sizeof(struct sockaddr_in));
    //dprintf((stderr, "%s: %s\n", name, inet_ntoa(sin->sin_addr)));
    return 0;
  }
  else
  {
    memset(sin, 0, sizeof(struct sockaddr_in));
    //dprintf((stderr, "%s: %s\n", name, "could not resolve interface"));
    return -1;
  }
  return -1;
}
