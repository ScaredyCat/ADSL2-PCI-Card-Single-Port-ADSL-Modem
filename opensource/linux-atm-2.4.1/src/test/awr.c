
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <atm.h>


#include <netinet/in.h>
#include <arpa/inet.h>

#if 0
#define DPRINTF(format,args...) printf(format,##args)
#else
#define DPRINTF(format,args...)
#endif


#define BUFF_LEN 50000
#define DUMP_COL 15

#define UDP_RCV_PORT 1500
#define UDP_SEND_PORT 1501
#define DATA_LEN 10000

struct sockaddr_in serv_addr,cli_addr,send_serv_addr;
char send_buff[BUFF_LEN],recv_buff[BUFF_LEN];
char rcv_ip_addr[100] = "127.0.0.1";
char send_ip_addr[100] = "127.0.0.1";
int rcv_port = UDP_RCV_PORT , send_port = UDP_SEND_PORT;
int send_serv_addr_len = 0;
fd_set set;
struct timeval timeout;
int atm_sock = -1,udp_rcv_sock = -1,udp_send_sock = -1;


static void usage(const char *name)
{
    fprintf(stderr,"usage: %s itf.vpi.vci [RCV_IP_ADDR] [RCV_PORT] [SEND_IP_ADDR] [SEND_PORT] \n",name);
	fprintf(stderr," Default values \n RCV_IP_ADDR : 127.0.0.1 \n SEND_IP_ADDR : 127.0.0.1 \n RCV_PORT : 1500 \n SEND_PORT : 1501\n");
    exit(1);
}


int send_to_udp(char *msg,int msg_len)
{
  int data_len =0 ;

   data_len = sendto(udp_send_sock,msg,msg_len,0, (struct sockaddr *)&send_serv_addr,send_serv_addr_len);
  if (data_len < 0)
	{
		DPRINTF("ERROR : Data send Fail data_len return is %d",data_len);
    //exit(1);
	}
  
 
 return data_len;

}


//int awr(int sock,int data_len,char data)
int awr(int sock,int data_len,char *data)
{
    int send_len=0,recv_len=0;

    memset(recv_buff,'\0',BUFF_LEN);
    memset(send_buff,'\0',BUFF_LEN);
    memcpy(send_buff,data,data_len);
//		strcpy(send_buff,data);
  
    send_len = write(sock,send_buff,data_len);
    DPRINTF("\n write data len is %d",send_len);
    if (send_len < 0) DPRINTF(" (%s)",strerror(errno));
    

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
			DPRINTF(" (%s)",strerror(errno));
			DPRINTF("\n ERROR : break from read  ");
		    return -1;
		}
		if (recv_len != send_len)
		{
			DPRINTF("\n Error : Send length ( %d ) is diferent from Rcv length ( %d ) ",send_len,recv_len);
		}
	}
	else
	{
    DPRINTF("\n\n Atm Recv Time out........ \n\n");
	}
	
return   send_to_udp(recv_buff,recv_len);


}


int create_udp_rcv_socket()
{
	int s = -1,ret =0 ;
	
	s = socket(PF_INET,SOCK_DGRAM,0);
	if (s < 0)
	{
  	DPRINTF("Error in creating UDP socket ");
		return -1;
	}

  serv_addr.sin_family  = AF_INET;
	//serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
  serv_addr.sin_addr.s_addr = inet_addr(rcv_ip_addr);
	serv_addr.sin_port = htons(rcv_port);

  ret = bind(s,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
  if (ret < 0)
    {
 			DPRINTF("Error : Bind Fails");
			return -1;
		}

	 return s;
}


int create_udp_send_socket()
{
  int sock = 0;
	sock = socket(PF_INET,SOCK_DGRAM,0);
	if (sock < 0)
	{
  	DPRINTF("Error in creating UDP send socket ");
	  exit(1);
	}

  send_serv_addr.sin_family  = AF_INET;
	//send_serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
	send_serv_addr.sin_addr.s_addr = inet_addr(send_ip_addr);
	send_serv_addr.sin_port = htons(send_port);
  send_serv_addr_len = sizeof(send_serv_addr);

	return sock;

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
      DPRINTF("\n Error : text2atm fails "); 
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
	int addr_len = 0,data_len=100;
    
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
    DPRINTF("\n Error  : Create atm socket Fails \n");
    exit(1);
	}


  udp_rcv_sock = create_udp_rcv_socket();
  if (udp_rcv_sock < 0)
  {
    DPRINTF("\n Error  : Create UDP rcv socket Fails \n");
    exit(1);
	}

udp_send_sock = create_udp_send_socket();
	if (udp_send_sock < 0)
	{
		DPRINTF("\n Error  : Create UDP send socket Fails \n");
    exit(1);

	}
	 
  memset(msg,'\0',DATA_LEN);
  addr_len = sizeof(cli_addr);
    
	while(data_len > 0)
	{	

		data_len = recvfrom(udp_rcv_sock,msg,DATA_LEN,0, (struct sockaddr *)&cli_addr,&addr_len);
	  if (data_len < 0)
		{
			DPRINTF("ERROR : Data recive Fail");
	    exit(1);
		}

	 	awr(atm_sock,data_len,msg);
	}
  
	close(udp_rcv_sock);
  close(udp_send_sock) ;
  close(atm_sock);
  
 // sleep(10);
  return 0;
}
