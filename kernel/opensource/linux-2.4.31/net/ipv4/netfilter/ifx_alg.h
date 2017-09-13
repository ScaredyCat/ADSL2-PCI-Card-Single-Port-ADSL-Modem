/*  The declarations here have to be in a header file, because
 *  they need to be known both to the kernel module
 *  (in chardev.c) and the process calling ioctl (ioctl.c)
 */
#include <linux/ioctl.h>

#define RTSP_CONN_DEVICE_NAME "rtsp_conn_dev"
#define RTSP_DEVICE_NAME "rtsp_dev"

enum AlgControlProtocol
{
	IP_PROTO_TCP=1,
	IP_PROTO_UDP,
	IP_PROTO_TCP_UDP
};

struct rtsp_params
{
	int rtsp_port;
	enum AlgControlProtocol ip_proto;
};

struct rtsp_conn_registration_data
{
	struct list_head list;
	int proto;
	int port;
	struct ip_conntrack_helper *conntrack_helper;

};

struct rtsp_registration_data
{
	struct list_head list;
	int proto;
	int port;
	struct ip_nat_helper *helper;

};

/* The major device number. We can't rely on dynamic 
 * registration any more, because ioctls need to know 
 * it. */
#define MAJOR_NUM_RTSP 		100
#define MAJOR_NUM_RTSP_CONN 	231
#define IOCTL_DEREGISTER_RTSP_PORT _IOR(MAJOR_NUM_RTSP, 3, struct rtsp_params*)
#define IOCTL_DEREGISTER_RTSP_PORT_CONN _IOR(MAJOR_NUM_RTSP_CONN, 3, struct rtsp_params*)



/* Get the n'th byte of the message */
//#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)
#define IOCTL_REGISTER_RTSP_PORT _IOWR(MAJOR_NUM_RTSP, 2, struct rtsp_params*)
#define IOCTL_REGISTER_RTSP_PORT_CONN _IOWR(MAJOR_NUM_RTSP_CONN, 2, struct rtsp_params*)

 /* The IOCTL is used for both input and output. It 
  * receives from the user a number, n, and returns 
  * Message[n]. */


/* The name of the device file */
#define DEVICE_FILE_NAME_RTSP		"/dev/rtsp_dev"
#define DEVICE_FILE_NAME_RTSP_CONN 	"/dev/rtsp_conn_dev"

