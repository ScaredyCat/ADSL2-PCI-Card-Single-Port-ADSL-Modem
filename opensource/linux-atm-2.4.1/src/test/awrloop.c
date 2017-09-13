
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <atm.h>

#if 0
#define DPRINTF(format,args...) printf(format,##args)
#else
#define DPRINTF(format,args...)
#endif


#define BUFF_LEN 50000
#define DUMP_COL 15

int packet_lost=0,packet_sent=0,packet_recv=0,packet_error=0,dump=0,show_sent_status=0;
int min_packet_data_len=1,max_packet_data_len=100,num_of_packet=1;
char send_buff[BUFF_LEN];
char recv_buff[BUFF_LEN];

fd_set set;
struct timeval timeout;

void sigint(int);
void sighup(int);

void print_results(int final);

static int abort_flag=0;
static int dump_flag=0;
static int g_itf=0;
static int g_vpi=0;
static int g_vci=0;

static void usage(const char *name)
{
    fprintf(stderr,"usage: %s itf.vpi.vci [Min_Data_len] [Max_Data_Len] [DontClose] [Num_of_Packet] [Dump (0|1|2)] [Show_Sent_Status (0|1)] \n",name);
	fprintf(stderr," Default values \n Min_Data_len : 1 \n Max_Data_Len : 100 \n Num_of_Packet : 1\n Dump : 0");
	fprintf(stderr,"\n ( Note : Dump 0 => No dump , 1 => Dump only if packet error , 2 => Dump even if no error )\n");

    exit(1);
}


void show_dump(char *rcv_buff,int recv_len,char *snd_buff,int send_len)
{
  int ix=0,iy=0,recv_data_index=0,send_data_index=0,recv_done=0,send_done=0,rows=0;
  char temp[255];
	
  rows = send_len / DUMP_COL ;
  printf("\n\t--------------------------- Packet Dump -----------------------------\n");
 
  for(iy=0;iy <= rows;iy++)
	{
	 	if(iy == 0)
			{
		
				sprintf(temp,"\n %30s %6d %8s %30s %6d\n","Recv data len ",recv_len,"|","Send data len ",send_len);
				printf("%s",temp);
			}
		for(ix=0; ix < (DUMP_COL * 2 + 1); ix++)
		{
			if (ix < DUMP_COL )
			{	
				if(recv_data_index < recv_len)
				{
					printf(" %x",rcv_buff[recv_data_index]);
					recv_data_index++;
				}
				else
				{
				   printf("   ");
					recv_done =1 ;
				}
			}
			if (ix > DUMP_COL ) 
			{
				if(send_data_index < send_len)
				{
					printf(" %x",snd_buff[send_data_index]);
					send_data_index++;
				}
				else
				{
				   printf("   ");
				   send_done =1 ;
				}
	
			}	
			
			if(ix == DUMP_COL)
				printf(" | ");
		}
		
		if (recv_done && send_done)
			break;
		else
			printf("\n");
	}
	printf("\n\t ---------------------------------------------------------------------- \n");
}

int awr(int sock,int data_len,char data)
{
    int send_len=0,recv_len=0,index2=0;

    memset(recv_buff,'\0',BUFF_LEN);
    memset(send_buff,'\0',BUFF_LEN);
    memset(send_buff,data,data_len);
  
    send_len = write(sock,send_buff,strlen(send_buff));
    DPRINTF("\n write data len is %d",send_len);
    if (send_len < 0) printf(" (%s)",strerror(errno));
    packet_sent++;
	if (show_sent_status)
	    printf("\n Packet sent no. %d  ( Data length %d ) ",packet_sent,data_len);

 	FD_ZERO(&set);
 	FD_SET(sock,&set);

	timeout.tv_sec = 10;
    timeout.tv_usec = 100000;
                
    select(sock+1,&set,NULL,NULL,&timeout);
                
    if (FD_ISSET(sock,&set)) 
	{
	   recv_len = read(sock,recv_buff,BUFF_LEN);
           DPRINTF("\n recv data length %d",recv_len);
		
		if (recv_len < 0) 
		{
			printf(" (%s)",strerror(errno));
			printf("\n Neeraj : ERROR : break from read  ");
		    return 1;
		}
		if (recv_len != send_len)
		{
			printf("\n Error : Send length ( %d ) is diferent from Rcv length ( %d ) ",send_len,recv_len);
		}
        if (memcmp(send_buff,recv_buff,recv_len) == 0)
			{
				packet_recv++;
				if (dump == 2)
				show_dump(recv_buff,recv_len,send_buff,send_len);
			}
		else
			{
			  	packet_error++;
				if(dump)
					  show_dump(recv_buff,recv_len,send_buff,send_len);
            }

		DPRINTF("\n Packet Rcv Data \n ");
		for(index2  =0;index2 <= recv_len; index2++)
			DPRINTF(" %c ",recv_buff[index2]);
	
       }
      else
	   {
         packet_lost++;
		 printf("\n Pkts lost : [%d] of [%d] ",packet_lost,packet_sent);
       }

}

int main(int argc,char **argv)
{

    struct sockaddr_atmpvc addr;
    struct atm_qos qos;
    int s,index1=0;
    char data = 'A'; 

    signal(SIGINT, sigint);
    signal(SIGHUP, sighup);

    if (argc < 2) usage(argv[0]);
    
    if (argc >= 3)
	{
	    min_packet_data_len = atoi(argv[2]);
		DPRINTF("\n Min Data lenght of each packet is %d",min_packet_data_len);
	}

	if (argc >= 4)
	{
		max_packet_data_len = atoi(argv[3]);
		DPRINTF("\n Max Data length of each packet is %d",max_packet_data_len);
	    if (max_packet_data_len > BUFF_LEN-1)
		{
          printf("Error : Max packet length should be less than %d",BUFF_LEN );
          exit(1);
		}
	}

   if(min_packet_data_len > max_packet_data_len)
	{
		 printf("Error : Min packet length should be less than %d",max_packet_data_len );
         exit(1);
    }

	if (argc >= 5)
	{
		num_of_packet = atoi(argv[4]);
		DPRINTF("\n Number of packet to send is %d",num_of_packet);
	}
    
    if(argc >= 6)
	{
		dump = atoi(argv[5]);
	}

	if(argc >= 7)
	{
		show_sent_status = atoi(argv[6]);
	}


	if ((s = socket(PF_ATMPVC,SOCK_DGRAM,0)) < 0) {
	perror("socket");
	return 1;
    }

    memset(&addr,0,sizeof(addr));
     
    if (text2atm(argv[1],(struct sockaddr *) &addr,sizeof(addr),T2A_PVC) < 0)
	usage(argv[0]);

    g_itf = addr.sap_addr.itf;
    g_vpi = addr.sap_addr.vpi;
    g_vci = addr.sap_addr.vci;


    memset(&qos,0,sizeof(qos));
    qos.aal = ATM_AAL5;
    qos.txtp.traffic_class = ATM_UBR;
    qos.txtp.max_sdu = BUFF_LEN ;

    qos.rxtp.traffic_class = ATM_UBR;
    qos.rxtp.max_sdu = BUFF_LEN;

    if (setsockopt(s,SOL_ATM,SO_ATMQOS,&qos,sizeof(qos)) < 0) {
	perror("setsockopt SO_ATMQOS");
	return 1;
    }

	if (bind(s,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
	perror("bind");
	return 1;
    }
    
   printf("\n");
   
    
	do
	{
    	for(index1=1;index1<=num_of_packet;index1++)
			{
			 awr(s,min_packet_data_len,data);
			 if (abort_flag == 1)
				 goto lbl_exit;
			 if (dump_flag == 1) {
				 dump_flag=0;
				 print_results(0);
			 }
			 data++ ;
			//printf("\n data %d and rem %d",data, data % 75);
			 if( (data % 85) < 65)
				data = 'A';
             }

       min_packet_data_len++;
	}while(min_packet_data_len <= max_packet_data_len);
    
lbl_exit:
	print_results(1);	
    return 0;
}


void print_results(int final)
{
   if (final) {
   printf("\n ==> awrloop TPE Test Result Summary - itf.vpi.vci = %d.%d.%d\n",
		  g_itf, g_vpi, g_vci); 
   } else {
   printf("\n ==> awrloop TPE Test Result Running  - itf.vpi.vci = %d.%d.%d\n",
		  g_itf, g_vpi, g_vci); 

   }
   printf("\n Pkts [Tx:%d] [Rx:%d] [Lost:%d] [Err:%d]\n",packet_sent,
		   packet_recv, packet_lost, packet_error);
}

void sigint(int signo)
{
	abort_flag = 1;
}

void sighup(int signo)
{
	dump_flag = 1;
}

