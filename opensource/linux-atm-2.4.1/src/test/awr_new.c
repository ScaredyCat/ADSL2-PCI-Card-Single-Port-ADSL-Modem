
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/socket.h>
#include <sys/time.h>
#include <time.h>
#include <atm.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <common.h>

#if 0
#define DPRINTF(format,args...) printf(format,##args)
#else
#define DPRINTF(format,args...)
#endif


#define BUFF_LEN 50000
#define DUMP_COL 15
#define DATA_LEN 10000





char send_buff[BUFF_LEN],recv_buff[BUFF_LEN];

struct timeval timeout;
fd_set set;


static void usage(const char *name)
{
    fprintf(stderr,"usage: %s itf.vpi.vci [RCV_IP_ADDR] [RCV_PORT] [SEND_IP_ADDR] [SEND_PORT] \n",name);
	fprintf(stderr," Default values \n RCV_IP_ADDR : 127.0.0.1 \n SEND_IP_ADDR : 127.0.0.1 \n RCV_PORT : 1500 \n SEND_PORT : 1501\n");
    exit(1);
}





//int awr(int sock,int data_len,char data)
int awr(int sock,int data_len,char *data)
{
    int send_len=0,recv_len=0;

    memset(recv_buff,'\0',BUFF_LEN);
    memset(send_buff,'\0',BUFF_LEN);
   // memset(send_buff,data,data_len);
		strcpy(send_buff,data);
  
    send_len = write(sock,send_buff,data_len);
    DPRINTF("\n write data len is %d",send_len);
    if (send_len < 0) printf(" (%s)",strerror(errno));
    

 	FD_ZERO(&set);
 	FD_SET(sock,&set);

	timeout.tv_sec = 0;
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
	}
	else
	{
    printf("\n\n Atm Recv Time out........ \n\n");
	}
//   printf("\n data received after atm loopback is %s \n",recv_buff); 
  // send_to_udp(recv_buff,recv_len);

return 0;

}

int create_atm_socket(char *vpi_vci)
{


	struct sockaddr_atmpvc addr;
  struct atm_qos qos;
  int s = -1;
  
	if ((s = socket(PF_ATMPVC,SOCK_DGRAM,0)) < 0) 
	{
			perror("socket");
			return -1;
    }

    memset(&addr,0,sizeof(addr));
     
    if (text2atm(vpi_vci,(struct sockaddr *) &addr,sizeof(addr),T2A_PVC) < 0)
		{
      printf("\n Error : text2atm fails "); 
			return -1;
		}
			


    memset(&qos,0,sizeof(qos));
    qos.aal = ATM_AAL5;
    qos.txtp.traffic_class = ATM_UBR;
    qos.txtp.max_sdu = BUFF_LEN ;

    qos.rxtp.traffic_class = ATM_UBR;
    qos.rxtp.max_sdu = BUFF_LEN;

    if (setsockopt(s,SOL_ATM,SO_ATMQOS,&qos,sizeof(qos)) < 0) {
	perror("setsockopt SO_ATMQOS");
	return -1;
    }

	if (bind(s,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
	perror("bind");
	return -1;
    }

	return s;
    
}

int main(int argc,char **argv)
{
	int addr_len = 0,data_len=0;
  int atm_sock = -1,udp_rcv_sock = -1;
  
  char msg[DATA_LEN];

  if (argc < 2) usage(argv[0]);

  if (argc >= 3)
	{
	    strcpy(rcv_ip_addr,argv[2]);
			DPRINTF("\n Rcv IP address is %s",rcv_ip_addr);
	}

	if (argc >= 4)
	{
		rcv_port = atoi(argv[3]);
		DPRINTF("\n Rcv Port is %d",rcv_port);
	}

  if (argc >= 5)
	{
		strcpy(send_ip_addr,argv[4]);
		DPRINTF("\n Send IP address is %s",send_ip_addr);
	}
    
  if(argc >= 6)
	{
		send_port = atoi(argv[5]);
		DPRINTF("\n Send Port is %d \n",send_port);
	}

 
	atm_sock = create_atm_socket(argv[1]);
  if (atm_sock < 0)
  {
    printf("\n Error  : Create atm socket Fails \n");
    exit(1);
	}

	

  //udp_rcv_sock = create_udp_rcv_socket();
	//if (udp_rcv_sock < 0)
  //{
    //printf("\n Error  : Create UDP rcv socket Fails \n");
    //exit(1);
	//}

	 
  memset(msg,'\0',DATA_LEN);
  addr_len = sizeof(cli_addr);
  data_len = recvfrom(udp_rcv_sock,msg,DATA_LEN,0, (struct sockaddr *)&cli_addr,&addr_len);
  if (data_len < 0)
	{
		printf("ERROR : Data recive Fail");
    exit(1);
	}
  
  //printf("Data recv is %s and data len is %d",msg,data_len);
	 awr(atm_sock,data_len,msg);
  
	close(udp_rcv_sock);
  printf("\n udp socket is closed........\n ");
 
  close(atm_sock);
  printf("\n atm socket is closed.........\n");
  
 // printf("\n");
  sleep(10);
  return 0;
}
