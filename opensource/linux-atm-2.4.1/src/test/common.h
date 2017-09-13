#define UDP_RCV_PORT 1500
#define UDP_SEND_PORT 1501


char rcv_ip_addr[100] = "127.0.0.1";
char send_ip_addr[100] = "127.0.0.1";
int rcv_port = UDP_RCV_PORT , send_port = UDP_SEND_PORT;
struct sockaddr_in serv_addr,cli_addr;

