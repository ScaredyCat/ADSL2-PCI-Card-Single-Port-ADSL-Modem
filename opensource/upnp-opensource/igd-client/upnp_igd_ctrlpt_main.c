///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000-2003 Intel Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "sample_util.h"
#include "upnp_igd_ctrlpt.h"
#include <string.h>

/* upnp_igd_ctrlpt_main.c */
void FreePortMapList(void);
void GetDevicePortMap(void);
void linux_print(const char *string);
int str2numsplit(char *buf, char c);
int str2arglist(char *buf, int *list, char c, int max);
int check_intport_exit(int intport);
int IgdExtrenalFileInit(void);
void IgdDebugCheck(void);


#define MAX_UPNP_ARGS		 5	
#define EXT_PORT_BASE 8000
#define ADD_PORT_TIMEOUT 3 //sec
#define _UPNPC_EXTFILE "/var/run/upnpc/upnpc.external"
#define _UPNPC_DEVMAP "/var/run/upnpc/upnpc.devmap"
#define _UPNPC_DBGFILE "/var/run/upnpc/upnpc.debug"


#define RUNFILE "/var/run/upnpc.pid"
char *DelPortPara[3]={"NewRemoteHost","NewExternalPort","NewProtocol"};
char *DelPortVal[3];
char *AddPortPara[8]={"NewRemoteHost","NewExternalPort","NewProtocol","NewInternalPort","NewInternalClient","NewEnabled","NewPortMappingDescription","NewLeaseDuration"};
char *AddPortVal[8];
char *tmpConstStr[3]={"","1","0"};
char *PortIndexPara[3]={"NewPortMappingIndex",NULL,NULL};
char *PortIndexVal[3]={NULL,NULL,NULL};
int sip_ext_port[MAX_PORT_MAPPING];
int sip_int_port[MAX_PORT_MAPPING];
char *term_ext_port[MAX_PORT_MAPPING];
char term_ext_proto[16];
int ext_port_cnt=0;
int opt_getmap=0;
int igd_debug_flag;
int DeviceNumOfPortCount=0;
int waittime_addport = 0;

extern struct IgdDevicePortMapInfo PortMapList[MAX_PORT_MAPPING+1];

void FreePortMapList(void)
{
	struct IgdDevicePortMapInfo *plist;
	
	for(plist = &PortMapList[0];plist->valid;plist++)
	{
		if(plist->rm_host)
			free(plist->rm_host);
		if(plist->ext_port)
			free(plist->ext_port);
		if(plist->ext_proto)
			free(plist->ext_proto);
		if(plist->int_port)
			free(plist->int_port);
		if(plist->client_ip)
			free(plist->client_ip);
		if(plist->desc)
			free(plist->desc);
	}				
}
static int find_noused_port(int *last_port, int start_port)
{
		int i,find=0;
		struct IgdDevicePortMapInfo *plist;
	
		for(i=*last_port;i<start_port+MAX_PORT_MAPPING;i++)
		{
			for(plist = &PortMapList[0];plist->valid;plist++)
			{
				if((i==atoi(plist->ext_port)))
				{
					find=1;
					break;
				}	
			}
			if(find)
			{
				find = 0;	
				continue;
			}	
			else
			{
				*last_port=i;
				break;
			}	
		}
		return find;
}	
static void AddPortMappingEntry(int NoPort, int extport_auto, char *extport, char *proto, 
												int *intport, char *clientip, char* desc)
{
	int i,j,find,bound,last_port;
	struct IgdDevicePortMapInfo *plist;
	char dupstr[128];
	struct timeval timeval;
	int time_stamp;
	
	last_port = atoi(extport);
	memset(&AddPortVal[0],0,sizeof(AddPortVal));
	for(i=0;i<NoPort;i++)
	/* step 1 search all the list for match */
	{
		find=0;
		bound=0;
		for(plist = &PortMapList[0];plist->valid;plist++)
		{
			if((last_port==atoi(plist->ext_port))&&(intport[i]==atoi(plist->int_port))
						&&(strcmp(proto,plist->ext_proto)==0)&&(strcmp(clientip,plist->client_ip)==0))
			{
					SampleUtil_Print("upnpc find match port and reserve it--ext=%d,int=%d--\n",last_port,intport[i]);
					find=1;
					sip_ext_port[ext_port_cnt]=last_port;
					ext_port_cnt++;
					goto find_match;
			}
		}
		for(plist = &PortMapList[0];plist->valid;plist++)
		{	
			if((intport[i]==atoi(plist->int_port))
					&&(strcmp(proto,plist->ext_proto)==0)&&(strcmp(clientip,plist->client_ip)==0))
			{
				SampleUtil_Print("upnpc:May can overwite it--ext=%s--int=%d",plist->ext_port,intport[i]);
				find=1;
				if(plist->ext_port)
				{
					if(!extport_auto)
					{
						DelPortVal[0]=tmpConstStr[0];
						DelPortVal[1]=plist->ext_port;
						DelPortVal[2]=plist->ext_proto;
						IgdCtrlPointSendAction(2,1,"DeletePortMapping",DelPortPara,DelPortVal,3);
						AddPortVal[0]=tmpConstStr[0];
						if(extport_auto)
							find_noused_port(&last_port,atoi(extport));
						sprintf(dupstr,"%d",last_port);
						AddPortVal[1]=strdup(dupstr);
						AddPortVal[2]=proto;
						sprintf(dupstr,"%d",intport[i]);
						AddPortVal[3]=strdup(dupstr);
						AddPortVal[4]=clientip;
						AddPortVal[5]=tmpConstStr[1];
						AddPortVal[6]=desc;
						AddPortVal[7]=tmpConstStr[2];
						waittime_addport=1;
						IgdCtrlPointSendAction(2,1,"AddPortMapping",AddPortPara,AddPortVal,8);
						gettimeofday(&timeval, NULL);
						time_stamp = timeval.tv_sec;
						while(waittime_addport)
						{
							gettimeofday(&timeval, NULL);
							if(timeval.tv_sec-time_stamp>ADD_PORT_TIMEOUT)
							{
								waittime_addport=0;
								break;
							}	
						}	
						sip_ext_port[ext_port_cnt]=atoi(AddPortVal[1]);
						if(AddPortVal[1])
							free(AddPortVal[1]);
						if(AddPortVal[3])
							free(AddPortVal[3]);
					}
					else /* reserve the extport */
					{
						sip_ext_port[ext_port_cnt]=	last_port =atoi(plist->ext_port);
					}	
					ext_port_cnt++;
				}		
				goto find_match;
			}			
		}	
find_match:		
		if(find) //need no add port
		{
			last_port++;
			continue;
		}	
		else /* step 2 search no used external port */
		{
			find=0;
			if(bound<MAX_PORT_MAPPING)
			{
				if(!extport_auto) // external port range specify by ATA
				{	
					//Now I have no idea to do such case...
					for(plist = &PortMapList[0];plist->valid;plist++)
					{
							if((last_port==atoi(plist->ext_port)))
							{
								find=1;
								sip_ext_port[ext_port_cnt]=0;
								ext_port_cnt++;
								break;
							}
					}
				}
				else
				{
					find = find_noused_port(&last_port,atoi(extport));
				}		
				// if not	found add the port; otherwise give up
				if(!find)
				{
					AddPortVal[0]=tmpConstStr[0];
					sprintf(dupstr,"%d",last_port);
					AddPortVal[1]=strdup(dupstr);
					AddPortVal[2]=proto;
					sprintf(dupstr,"%d",intport[i]);
					AddPortVal[3]=strdup(dupstr);
					AddPortVal[4]=clientip;
					AddPortVal[5]=tmpConstStr[1];
					AddPortVal[6]=desc;
					AddPortVal[7]=tmpConstStr[2];
					waittime_addport=1;
					IgdCtrlPointSendAction(2,1,"AddPortMapping",AddPortPara,AddPortVal,8);
					gettimeofday(&timeval, NULL);
					time_stamp = timeval.tv_sec;
					while(waittime_addport)
					{
						gettimeofday(&timeval, NULL);
						if(timeval.tv_sec-time_stamp>ADD_PORT_TIMEOUT)
						{
							waittime_addport=0;
							break;
						}	
					}	
					sip_ext_port[ext_port_cnt]=atoi(AddPortVal[1]);
					ext_port_cnt++;
					if(AddPortVal[1])
							free(AddPortVal[1]);
					if(AddPortVal[3])
							free(AddPortVal[3]);		
				}			
			}	
			else ///* step 3 if no port can used port, use the exist port */
			{
				// UPNP device should not be overwrite, so give up !!!	
				SampleUtil_Print( "upnpc: give up the add port request!\n");
			}
			last_port++;
		}
	}	
	//step 4 free the port list
	FreePortMapList();												 
}
void GetDevicePortMap(void)
{
	struct IgdDevicePortMapInfo *plist;
	FILE	*fptr;
	char *tmp_rh="";
	char *tmp_des="";
	
	if ( (fptr=fopen(_UPNPC_DEVMAP,"w")) == NULL)
	{
  	 printf(stderr,"can't open upnpc.devmap !!!\n");
  	 return;
  }
  IgdExtrenalIPInfo(fptr,1);
	for(plist = &PortMapList[0];plist->valid;plist++)
	{
		if(plist->rm_host)
					tmp_rh = plist->rm_host;
		if(plist->desc)
					tmp_des = plist->desc;			
		fprintf(fptr, "RH=%s;EP=%s;IC=%s;IP=%s;PROT=%s;DUR=%s;DES=%s\n",
								tmp_rh,plist->ext_port,plist->client_ip,
								plist->int_port,plist->ext_proto,plist->lease_time
								,tmp_des);
	}	
	fclose(fptr);	
	FreePortMapList();
}	
static void delete_handler(int signum)
{
	char line1[256],line2[256];
	FILE	*fptr;
	int i;
	
	if ( (fptr=fopen(_UPNPC_EXTFILE,"r")) == NULL)
	{
  		printf(stderr,"can't open upnpc.external !!!\n");
  		goto t_err;
  }
  fscanf(fptr,"EXT_IP=%s\n",line1);
  fscanf(fptr,"EXT_PORT=%s\n",line2);
  fclose(fptr);	
  //printf("%s\n%s\n",line1,line2);
  if((str2arglist(line2, term_ext_port, ':', ext_port_cnt) != ext_port_cnt))
	{
		printf("upnpc:the delete port number not match\n");
		exit(0);
	}	
	for(i=0;(i<ext_port_cnt)&&(i<MAX_PORT_MAPPING);i++)
	{
  	DelPortVal[0]=tmpConstStr[0];
		DelPortVal[1]=term_ext_port[i];
		DelPortVal[2]=term_ext_proto;
		IgdCtrlPointSendAction(2,1,"DeletePortMapping",DelPortPara,DelPortVal,3);
	}	
t_err:  
	return;
	//exit(0);
}	

void
linux_print( const char *string )
{
    puts( string );
}
int str2numsplit(char *buf, char c)
{
	char *ptr;
	int cnt=1,i;
	
	ptr = buf;
	for(i=0;i<strlen(buf);i++)
	{
		if(buf[i]==c)
			cnt++;
	}	
	return cnt;	
}	
int str2arglist(char *buf, int *list, char c, int max)
{
	char *idx = buf;
	int j=0;
	
	list[j++] = buf;
	while(*idx && j<max) {
		if(*idx == c || *idx == '\n') {
			*idx = 0;
			list[j++] = idx+1;
		}
		idx++;	
	}
	if(j==1 && !(*buf)) // No args
		j = 0;
		
	return j;
}
int check_intport_exit(int intport)
{
	int i;
	
	for(i=0;i<MAX_PORT_MAPPING;i++)
	{
		if(sip_int_port[i]==intport)
			return -1;
	}	
	return 0;
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
int IgdExtrenalFileInit(void)
{
		FILE	*fptr;
		if(!opt_getmap)
		{
			if ( (fptr=fopen(_UPNPC_EXTFILE,"w")) == NULL)
			{
    		printf(stderr,"can't open upnpc.external !!!\n");
    		return -1;
    	}
    	fprintf(fptr,"EXT_IP=0\n");
    	fprintf(fptr,"EXT_PORT=0\n");
    	fprintf(fptr,"INT_PORT=0\n");
    }
    else
    {
    	if ( (fptr=fopen(_UPNPC_DEVMAP,"w")) == NULL)
			{
    		printf(stderr,"can't open upnpc.devmap !!!\n");
    		return -1;
    	}
    }		
    
    fclose(fptr);
    return 0;	
}
void IgdDebugCheck()
{
	FILE	*fptr;
	char  *buff[10];
	
	if ( (fptr=fopen(_UPNPC_DBGFILE,"r+")) == NULL)
	{
			igd_debug_flag = 0;
			return;
	}	
	fgets(buff, 10, fptr);
	if(atoi(buff)==1)
		igd_debug_flag = 1;
	else
		igd_debug_flag = 0;
}	

int
main( int argc,
      char **argv )
{
    int rc;
    FILE	*fptr;
    int sig;
    sigset_t sigs_to_catch;
    struct sigaction sa;
    fd_set rfds;
    int max_sock,signo,retval;
    char *arglists[MAX_UPNP_ARGS],*intport_lists[MAX_UPNP_ARGS],*extport[2],*intport[2];
    int i,NumPortAdd,add1,add2;
    char dupstr[128],client_addr[16];
    struct IgdDevicePortMapInfo *portlist;
    int extport_auto,split_num,slip_tmp;
    char *ip_address = NULL;
    struct timeval timeval;
		int time_stamp;

		if (argc != 3)
		{
			printf("Usage: upnp_igd_ctrlpt <ifname> <numberofrules> <extport:extproto:intport:intclient:descrip>\n");
			printf("     	 The value of numberofrules can't be zero or greater than 8 \n");
			printf("Example: upnp_igd_ctrlpt eth0 1111:TCP:2222:192.168.1.100:SIP\n");
			exit(0);
		}
		if(strcmp(argv[2],"getmap")==0)
		{
			opt_getmap = 1;
			goto igd_ctrl_start;
		}	
		if((str2arglist(argv[2], arglists, ':', 5) != 5))
		{
			printf("upnpc:the add port rule fails\n");
			exit(0);
		}	
		if(strcmp(arglists[1],"TCP")!=0&&strcmp(arglists[1],"UDP")!=0)
		{
			printf("upnpc:the proto should UDP/TCP\n");
			exit(0);
		}	
		sprintf(term_ext_proto,"%s",arglists[1]);
		add1 = str2arglist(arglists[0], extport, '-', 2);
		if(add1==2)
		{
				add1=atoi(extport[1])-atoi(extport[0])+1;
				if(add1<0)
				{
					printf("upnpc: external port range fail\n"); 
					exit(0);
				}		
		}
		else if(add1>2)
		{
				printf("upnpc: external port range format fail\n"); 
				exit(0);
		}	
///////////////////////////////////////////////////////////////////
		split_num = str2numsplit(arglists[2],',');
		str2arglist(arglists[2],intport_lists,',',split_num);	
		add2=0;
		memset(sip_int_port,0,sizeof(sip_int_port));
		for(i=0;i<split_num;i++)
		{
			slip_tmp=str2arglist(intport_lists[i], intport, '-', 2);
			if(slip_tmp==1)
			{
				if(check_intport_exit(atoi(intport[0])) < 0)
				{
					printf("upnpc: internal port rang confilct!\n"); 
						exit(0);
				}	
				sip_int_port[add2++]=atoi(intport[0]);
				if(add2>MAX_PORT_MAPPING)
				{
						printf("upnpc: internal port out of rang [%d]\n",MAX_PORT_MAPPING); 
						exit(0);
				}	
			}	
			else if(slip_tmp==2)
			{
				int j;
				for(j=0;j<(atoi(intport[1])-atoi(intport[0])+1);j++)
				{
					if(check_intport_exit(atoi(intport[0])+j) < 0)
					{
						printf("upnpc: internal port rang confilct!\n"); 
						exit(0);
					}	
					sip_int_port[add2++]=atoi(intport[0])+j;
					if(add2>MAX_PORT_MAPPING)
					{
						printf("upnpc: internal port out of rang [%d]\n",MAX_PORT_MAPPING); 
						exit(0);
					}	
				}	
			}	
			else
			{
				printf("upnpc: internal port range format fail\n"); 
				exit(0);
			}		
			
		}			
///////////////////////////////////////////////////////////////////			
		if(add1!=add2)
		{
			if(atoi(extport[0])&&add1>1)
			{
					printf("upnpc: internal and external port range not match\n"); 
					exit(0);
			}	
		}
		NumPortAdd = add2;
		//printf("NumPortAdd=%d\n",NumPortAdd);
		if(NumPortAdd>MAX_PORT_MAPPING)
		{
				printf("upnpc: port range should be less than %d\n",MAX_PORT_MAPPING); 
				exit(0);
		}	
igd_ctrl_start:

		if(IgdExtrenalFileInit()!=0)
    		exit(0);		
    		
    IgdDebugCheck();
    		
    rc = IgdCtrlPointStart( linux_print, NULL ,argv[1]);
    if( rc != IGD_SUCCESS ) {
        SampleUtil_Print( "Error starting UPnP IGD Control Point" );
        exit( rc );
    }
    ip_address=IgdGetIpAddrStr(argv[1]);
    if(opt_getmap)
    		strcpy(client_addr,ip_address);
    else
    {
    		if(atoi(arglists[3])== 0)
    			strcpy(client_addr,ip_address);
    		else
    			strcpy(client_addr,arglists[3]);
    }		
    if(ip_address)
    		free(ip_address);
    create_pidfile();
    
    /*
       Catch Ctrl-C and properly shutdown 
     */
    sigemptyset( &sigs_to_catch );
    sigaddset( &sigs_to_catch, SIGINT );
    sigaddset( &sigs_to_catch, SIGUSR2 );
    //if want delete the port, call SIGUSR2
    sa.sa_mask = sigs_to_catch;
    sa.sa_flags = 0;
    sa.sa_handler = delete_handler;
		sigaction(SIGUSR2, &sa, NULL);
    //sigwait( &sigs_to_catch, &sig );
    upnpc_sp_setup();
    while(1)
    {
    	max_sock = upnpc_sp_fd_set(&rfds);
    	retval = select(max_sock + 1, &rfds, NULL, NULL,NULL);
    	if(retval<=0)
    			continue;
    	signo = upnpc_sp_read(&rfds);
    	switch(signo)
    	{
    		case IGDC_ADD_DEVICE:
    				IgdCtrlPointSendAction(2,1,"GetConnectionTypeInfo","","",0);    
						IgdCtrlPointSendAction(2,1,"GetNATRSIPStatus","","",0); 
						//IgdCtrlPointGetVar(2,1,"ConnectionStatus");  
						//IgdCtrlPointGetVar(2,1,"ExternalIPAddress");
						IgdCtrlPointSendAction(2,1,"GetExternalIPAddress","","",0);
						IgdCtrlPointSendAction(2,1,"GetStatusInfo","","",0);
    				break;
    		case IGDC_SOAP_PORT_MAPPING:
    				if(opt_getmap)
    				{
							GetDevicePortMap(); 
    					goto upnp_ctrl_exit;
    				}	
						if(!atoi(extport[0]))
						{
    					sprintf(dupstr,"%d",EXT_PORT_BASE);
    					extport_auto = 1;
    				}	
    				else
    				{
    					sprintf(dupstr,"%s",extport[0]);
    					extport_auto = 0;
    				}	
    				memset(sip_ext_port,0,sizeof(sip_ext_port));
    				ext_port_cnt=0;
    				AddPortMappingEntry(NumPortAdd, extport_auto,dupstr,arglists[1],sip_int_port,&client_addr[0],arglists[4]);
    				if ( (fptr=fopen(_UPNPC_EXTFILE,"w")) == NULL)
						{
    							printf(stderr,"can't open upnpc.external !!!\n");
        					//break;
        					goto upnp_ctrl_exit;
    				}
    				IgdExtrenalIPInfo(fptr,1);
    				fprintf(fptr,"EXT_PORT=");
    				//No external port available!
    				if(ext_port_cnt==0)
    				{
    					fprintf(fptr,"BAD");
    				}
    				else
    				{
    					for(i=0;i<ext_port_cnt;i++)
    					{
    						fprintf(fptr,"%d",sip_ext_port[i]);
    						if(i<ext_port_cnt-1)
    							fprintf(fptr,":");
    						if(i>MAX_PORT_MAPPING)
    							break;	
    					}
    				}		
    				fprintf(fptr,"\n");
    				fprintf(fptr,"INT_PORT=");
    				for(i=0;i<ext_port_cnt;i++)
    				{
    					fprintf(fptr,"%d",sip_int_port[i]);
    					if(i<ext_port_cnt-1)
    						fprintf(fptr,":");
    					if(i>MAX_PORT_MAPPING)
    						break;	
    				}
    				fprintf(fptr,"\n");
    				fclose(fptr);
    				goto upnp_ctrl_exit;
    				break;
    		case IGDC_GET_DISCONN_NOTIFY:
    				//do some thing....
    				if ( (fptr=fopen(_UPNPC_EXTFILE,"w")) == NULL)
						{
    							printf(stderr,"can't open upnpc.external !!!\n");
        					break;
    				}
    				IgdExtrenalIPInfo(fptr,0);
    				fprintf(fptr,"EXT_PORT=0\n");
    				fclose(fptr);	
    				break;
    		case IGDC_GET_CONN_NOTIFY:
    				IgdCtrlPointGetVar(2,1,"ExternalIPAddress");
    				IgdGetDeviceCanAddPort();
    				break;
    		case IGDC_GET_EXTERNALIP:		
    				//IgdExtrenalIPInfo(1);
    				break;
    		case IGDC_GET_PORT_MAPPING_NUBER:
    				//printf("IGDC_GET_PORT_MAPPING_NUBER\n");
						sprintf(dupstr,"%d",DeviceNumOfPortCount);
    				PortIndexVal[0] = strdup(dupstr);
    				//get_port_star_timer=1;
    				IgdCtrlPointSendAction(2,1,"GetGenericPortMappingEntry",PortIndexPara,PortIndexVal,1);
    				if(PortIndexVal[0])	
    						free(PortIndexVal[0]);
    				break;		
    		case IGDC_SOAP_PORT_MAPPING_FORCE:
    				if(!DeviceNumOfPortCount) //The Device have no entries now
    				{
    						if(opt_getmap)
    						{
    							goto upnp_ctrl_exit;
    						}	
    						memset(sip_ext_port,0,sizeof(sip_ext_port));
    						ext_port_cnt=0;
    						AddPortVal[0]=tmpConstStr[0];
								AddPortVal[2]=arglists[1];
								AddPortVal[4]=&client_addr[0];
								AddPortVal[5]=tmpConstStr[1];
								AddPortVal[6]=arglists[4];
								AddPortVal[7]=tmpConstStr[2];
								for(i=0;i<NumPortAdd;i++)
								{
									if(!atoi(extport[0])&& add1==1)
										sprintf(dupstr,"%d",EXT_PORT_BASE+i);
									else
										sprintf(dupstr,"%d",atoi(extport[0])+i);	
									AddPortVal[1]=strdup(dupstr);
									sprintf(dupstr,"%d",sip_int_port[i]);
									AddPortVal[3]=strdup(dupstr);
									waittime_addport = 1;
									IgdCtrlPointSendAction(2,1,"AddPortMapping",AddPortPara,AddPortVal,8);
									gettimeofday(&timeval, NULL);
									time_stamp = timeval.tv_sec;
									while(waittime_addport)
									{
										gettimeofday(&timeval, NULL);
										if(timeval.tv_sec-time_stamp>ADD_PORT_TIMEOUT)
										{
											waittime_addport=0;
											break;
										}	
									}	
									sip_ext_port[ext_port_cnt]=atoi(AddPortVal[1]);
									ext_port_cnt++;
									if(AddPortVal[1])
										free(AddPortVal[1]);
									if(AddPortVal[3])
										free(AddPortVal[3]);	
								}	
								isleep(2); 
								if ( (fptr=fopen(_UPNPC_EXTFILE,"w")) == NULL)
								{
    							printf(stderr,"can't open upnpc.external !!!\n");
        					//break;
        					goto upnp_ctrl_exit;
    						}
    						IgdExtrenalIPInfo(fptr,1);
    						fprintf(fptr,"EXT_PORT=");
    						for(i=0;i<ext_port_cnt;i++)
    						{
    							fprintf(fptr,"%d",sip_ext_port[i]);
    							if(i<ext_port_cnt-1)
    								fprintf(fptr,":");
    							if(i>MAX_PORT_MAPPING)
    							break;	
    						}	
    						fprintf(fptr,"\n");
    						fprintf(fptr,"INT_PORT=");
    						for(i=0;i<ext_port_cnt;i++)
    						{
    							fprintf(fptr,"%d",sip_int_port[i]);
    							if(i<ext_port_cnt-1)
    								fprintf(fptr,":");
    							if(i>MAX_PORT_MAPPING)
    							break;	
    						}
    						fprintf(fptr,"\n");
    						fclose(fptr);	
    						goto upnp_ctrl_exit;
    				}
    				break;	
    		case IGDC_SHUT_DOWN:
    				goto upnp_ctrl_exit;			
    		case IGDC_DEL_DEVICE:		
    		default:
    				break;
    	}	
  	}
upnp_ctrl_exit:  	
    SampleUtil_Print( "Shutting down on signal %d...\n", sig );
    rc = IgdCtrlPointStop(  );
    exit( rc );
}
