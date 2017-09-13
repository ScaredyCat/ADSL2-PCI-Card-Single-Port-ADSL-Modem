/* $Id: eft_i4l.c,v 1.4 2000/06/08 10:04:24 keil Exp $ */
/*
 * isdn4linux implementation dependent functions
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <net/if.h>
#include <linux/x25.h>
#include <linux/isdn.h>
#include <linux/isdnif.h>
#include <errno.h>
#include "../config.h"
#include <eft.h>

/* 
 * Get the remote phone number corresponding to the connected socket.
 * isdn_no must be an array of at least 21 chacters
 */
int eft_get_peer_phone(unsigned char * isdn_no, int sk)
{
	int fd, len, i, is, ie, try_first_channel=0, ret;
	unsigned char buf[4001];
	char dev[EFT_DEV_NAME_LEN], *d;	

	isdn_net_ioctl_phone if_phone;

	isdn_no[0] = 0;

	fd = open("/dev/isdninfo",O_RDONLY);
	if ( fd < 0 ){
		perror("eft_get_peer_phone:open(/dev/isdninfo)");
		return -1;
	}

#ifdef IIOCNETGPN  
/*
 * This IOCTL is supported in recent i4l-cvs version and in kernel 2.3.6.
 * The peer number is really determined from the network interface
 */
	d = eft_get_device(dev, EFT_DEV_NAME_LEN, sk);
	if ( ! d ){
		fprintf(stderr,"eft_get_peer_phone: unable to figure out "
			"isdn device\n");
		return -1;
	}
	strncpy(if_phone.name,d,sizeof(if_phone.name));
	ret = ioctl(fd,IIOCNETGPN,&if_phone);
	if(ret == 0){
		strcpy(isdn_no,if_phone.phone);
		close(fd);
		return strlen(isdn_no);
	} else if( errno == EINVAL ) { 
		/* kernel does not support the ioctl above:
		 * use fall back method
		 */
	        fprintf(stderr, "eft_get_peer_phone: probably "
			"no ioctl(IIOCNETGPN), trying fall back method\n");
		try_first_channel=1;
	} else {
		perror("eft_get_peer_phone:ioctl(IIOCNETGPN)");
		close(fd);
		return -1;
	}
#else
#warning   You seem to include old kernel isdn header files. Thus, reliable
#warning   identification of peer isdn phone number will not work.
	try_first_channel=1;
#endif

if( ! try_first_channel ) return 0;

/*
 * Fallback method for kernels which don't support the appropriate ioctl.
 * Currently, this assumes that the connection is on the first
 * channel of the first driver. To enforce that the connection arrives
 * on the first channel of the first HL driver, you need to bind that channel
 * to the interface.
 */
	len = read(fd,buf,4000);
	if( len < 0 ) {
		perror("eft_get_peer_phone:read(/dev/isdninfo)");
		close(fd);
		return len;
	}
	close(fd);
	/* search for position after string "phone:" in isdnfinfo contents */
	for( i=0; i<4000; i++ ){
		if( (buf[i] == ':') && (buf[i-3] == 'o') ){
			is = i+2;
			break;
		} 
	};
	ie = 4000;
	for( i=is; i<4000; i++ ){
		if( buf[i] == ' ') {
			ie = i;
			break;
		} 
	}
	len = ie - is;
	if( len > 20 ) return len; 
	memcpy(isdn_no,buf+is,len);
	isdn_no[len]=0;
	return 0;
}

/*
 * parse the eft conf file for client msn
 */ 
static char * eft_get_client_msn()
{
#define MY_LINESIZE 162
	FILE * f;
	char * s, cmsn[]="EFT_CLIENT_MSN=";
	static char buf[MY_LINESIZE];
	int l;

	f = fopen(CONFIG_I4L_CONFDIR "/eft.conf","r");
	if( ! f ){
		perror("eft_get_client_msn:fopen "CONFIG_I4L_CONFDIR "/eft.conf");
		return NULL;
	}
	
	l = strlen(cmsn);
	while( (s=fgets(buf,MY_LINESIZE,f)) ){
		buf[MY_LINESIZE-1] = 0;
		if( strncmp(s,cmsn,l) == 0 ) {
			fclose(f);
			return s+l;
		}
	}
	fclose(f);
	return NULL;
}

/* 
 * Configure isdn network devices for outgoing eft connection
 */
static int sync_pipe_r=-1;
static int sync_pipe_w=-1;
int eft_get_x25route(struct sockaddr_x25 * x25addr,
		     struct x25_route_struct *x25_route, char * isdn_no)
{
        int s=-1, ifd=-1, filedes[2], err;
	char * addr = "", if_name[255]="eftpout", *msn;

	isdn_net_ioctl_phone phone;
        isdn_net_ioctl_cfg cfg;
	struct ifreq ifr;

	/* determine own msn to be used by eft client */
	msn = eft_get_client_msn();
	if(!msn){
		fprintf(stderr,
			"eft_get_x25route: could not determine own msn\n");
		return 1;
	} else {
		char *m = msn;
		while(*m) {
			if ((*m < '0') || (*m >'9')) {
				*m = 0;
				break;
			}
			m++;
		}
		if (!strlen(msn))
			strcpy(msn, "0");
	}

	/*
	 * By convention, the symbolic isdn address "localhost" is
	 * mapped to x25 address "1" and we assume that an external setup
	 * script has already configured a corresponding interface and an
	 * x25 route.
	 */
	if( strcmp(isdn_no, "localhost") == 0 ) {
		strcpy(x25addr->sx25_addr.x25_addr, "1");
		/* don't set proper device name such that this magic route is
		   not accessible from caller */
		x25_route->device[0] = 0;
		return 0;
	}

	/* pipe for later telling the waiting parent to release the route */ 
	if(pipe(filedes)){
		perror("eft_get_x25route():pipe()");
		return -1;
	}
	sync_pipe_r=filedes[0];
	sync_pipe_w=filedes[1];

        if ((s = socket(PF_X25, SOCK_SEQPACKET, 0)) < 0 ) {
                perror("eft_get_x25route: socket");
                return 1;
        }

	memset(&cfg,0,sizeof(cfg));
	strcpy(cfg.name, if_name);

	if( (ifd=open("/dev/isdnctrl",O_RDWR)) < 0){
		perror("open isdnctrl");
		err = -1;
		goto error;
	};

	if( ioctl(ifd, IIOCNETAIF, if_name) ){
		perror("addif");
		err = -1;
	} else {
		if ( ioctl(ifd, IIOCNETGCF, &cfg) ){
			perror("get_cfg");
			err = -1;
			goto error_delif;
		}
		strncpy(cfg.eaz, msn, sizeof(cfg.eaz));
		cfg.eaz[sizeof(cfg.eaz)-1] = 0;
		cfg.l2_proto = ISDN_PROTO_L2_X75I;
		cfg.dialmax = 1;
		cfg.secure = 1;
		cfg.onhtime = 200;
		cfg.p_encap = ISDN_NET_ENCAP_X25IFACE;
#ifdef ISDN_NET_DM_AUTO
		cfg.dialmode = ISDN_NET_DM_AUTO;
#endif
		if ( ioctl(ifd, IIOCNETSCF, &cfg) ){
			perror("set_cfg");
			err = -1;
			ioctl(ifd, IIOCNETDIF, if_name);
			goto error_delif;
		}
		
		strcpy(phone.name, if_name);
		phone.outgoing = 1;
		strncpy(phone.phone, isdn_no, ISDN_MSNLEN);
		phone.phone[ISDN_MSNLEN-1] = 0;
		if( ioctl(ifd, IIOCNETANM, &phone) ){
			perror("add_phone");
			err = -1;
			goto error_delif;
		}

		/* ifconfig up */
		strcpy(ifr.ifr_name, if_name);
		if (ioctl(s, SIOCGIFFLAGS, &ifr) ) {
			perror("SIOCAGIFFLAGS");
			err = -1;
			goto error_delif;
		} 
		ifr.ifr_flags |= IFF_UP;
		if (ioctl(s, SIOCGIFFLAGS, &ifr) ) {
			perror("SIOCAGIFFLAGS");
			err = -1;
			goto error_delif;
		} else {
			ifr.ifr_flags |= IFF_UP;
			if (ioctl(s, SIOCSIFFLAGS, &ifr) ) {
				perror("SIOCASIFFLAGS");
				err = -1;
				goto error_ifdown;
			}
			strcpy(x25_route->address.x25_addr, addr);
			x25_route->sigdigits = strlen(addr);
			strcpy(x25_route->device, if_name);
			printf( "adding route %s, sig=%d\n",addr,x25_route->sigdigits);
			if (ioctl(s, SIOCADDRT, x25_route) ) {
				perror("SIOCADDRT");
				err = -1;
				goto error_ifdown;
			}
			strcpy(x25addr->sx25_addr.x25_addr, addr);
			err = 0;
		error_ifdown:
			if( err ) {
				ifr.ifr_flags &= ~IFF_UP;
				ioctl(s, SIOCSIFFLAGS, &ifr);
			}
		}
	error_delif:
		if(err) ioctl(ifd, IIOCNETDIF, if_name);
	}
error:
	close(ifd);
	close(s);
	return err;
}

/* 
 * wait for request from child to release x25 route
 */
int eft_wait_release_route()
{
	char dummy[1];

	close(sync_pipe_w);
	sync_pipe_w = -1;
	/* printf("waiting for release route\n"); */
	read(sync_pipe_r,dummy,1);
	/* printf("end waiting\n"); */
	return 0;
}

/* 
 * release isdn network devices
 */
int eft_signal_release_route()
{
	/* signal release route request to possible parent process */
	/* printf("signalling release route\n"); */
	close(sync_pipe_r);
	sync_pipe_r = -1;
	close(sync_pipe_w);
	sync_pipe_w = -1;
	return 0;
}

int eft_release_route(struct x25_route_struct * x25_route)
{
	int s;

	/* try closing the route ourselves */
        if ((s = socket(PF_X25, SOCK_SEQPACKET, 0)) < 0) {
                perror("eft_release_route: socket");
                return 1;
        }

	/* printf("releasing route\n"); */
        if (ioctl(s, SIOCDELRT, x25_route) == -1) {
                perror("SIOCDELRT");
                close(s);
		return -1;
        }
	/* printf("route released\n"); */
	close(s);
	return 0;
}

/* 
 * hang up physical connection of isdn network interface.
 */
void eft_dl_disconnect(unsigned char * if_name)
{
	int ifd;

	if( (ifd=open("/dev/isdnctrl",O_RDWR)) < 0){
		perror("open isdnctrl");
		goto error;
	};
	if( ioctl(ifd, IIOCNETHUP, if_name) < 0 ){
		perror("hangup");
		goto error;
	}
error:
	close(ifd);
}
/* 
 * release (possibly remove) isdn network interface.
 */
int eft_release_device(unsigned char * if_name)
{
	int err=0, s=-1, ifd=-1;
	struct ifreq ifr;

        if ((s = socket(PF_X25, SOCK_SEQPACKET, 0)) < 0) {
                perror("eft_release_device: socket");
                goto error_delif;
        }

	/* ifconfig down */
	ifr.ifr_flags = 0;
	strcpy(ifr.ifr_name, if_name);
	if (ioctl(s, SIOCGIFFLAGS, &ifr) ) {
		perror("SIOCGIFFLAGS");
		err = -1;
		goto error_delif;
	} 
	ifr.ifr_flags &= ~IFF_UP;
	if (ioctl(s, SIOCSIFFLAGS, &ifr) ) {
		perror("SIOCSIFFLAGS");
		err = -1;
		goto error_delif;
	}

error_delif:
	if( (ifd=open("/dev/isdnctrl",O_RDWR)) < 0){
		perror("open isdnctrl");
		err = -1;
		goto error;
	};
	if( ioctl(ifd, IIOCNETDIF, if_name) ){
		perror("delif");
		err = -1;
	}
	ioctl(ifd, IIOCNETDIF, if_name);

error:
	close(ifd);
	close(s);
	return err;
}

/*
 * Return the name of the network interface which is used by 
 * a connected X.25 socket.
 *
 * This is a hack as it uses /proc file system contents like kernel internal
 * addresses of inode structures and might fail for future linux kernel
 * version.
 */
char * eft_get_device(char * dev, int len, int sock_fd)
{
#define PROC_X25_MAXLEN 180	
	char path_buf[MAXPATHLEN+1], link_buf[MAXPATHLEN+1], *p, *q,
		proc_buf[PROC_X25_MAXLEN+1], *t, *inode, *d;
	FILE *fx25;
	static char dummy[]="dummy";
	int i, inod_col, dev_col;

	/* first, the socket's inode number is extracted from the
	 * socket file descriptors /proc/self/fd/# symbolic link contents 
	 */
	snprintf(path_buf,MAXPATHLEN+1,"/proc/self/fd/%d",sock_fd);
	len = readlink(path_buf,link_buf,MAXPATHLEN+1);
	if( len < 0 ){
		perror("eft_get_device():readlink failed");
		return NULL;
	}
	p=link_buf;
	while( (p!=0) && ( (*p < '0') || (*p > '9') ) ) ++p;
	inode=q=p;
	while( (*q >= '0') && (*q <= '9')  ) ++q;
	*q=0;
	
	/* fprintf(stderr,"socket %d has inode %s\n", sock_fd,inode); */
	
	/* next, the position of the inode column in /proc/net/x25
	 * is determined. 
	 */
	fx25 = fopen("/proc/net/x25","r");
	if( ! fx25 ) {
		perror("eft_get_device: open /proc/net/x25 failed");
		return NULL;
	}
	q = fgets(proc_buf,PROC_X25_MAXLEN+1,fx25);
	if(!q){
		fprintf(stderr, "/proc/net/x25 fgets error\n");
		return NULL;
	}
	t = proc_buf;
	i=0;
	while(1){
		/* the termination critereon for this loop silently assumes
		 * that "inode" is the last column header.
		 */
		q = strsep(&t," \t\n\r");
		if(!q){
			fprintf(stderr, "/proc/net/x25 strsep error\n");
			return NULL;
		}
#if 0
		fprintf(stderr,"parameter in column %d is \"%s\"\n",i,q);
#endif
		if( strcmp("dev",q)   == 0 )  dev_col = i;
		if( strcmp("inode",q) == 0 ){
			inod_col = i;
			break;
		}
		if( *q ) i++;
	}
	
	/* last, the line with the socket's inode is located
	 * in /proc/net/x25. The device name is extracted from that line
	 */ 
	while( (q=fgets(proc_buf,PROC_X25_MAXLEN+1,fx25)) ){
		t = proc_buf;
		i=0;
		d=dummy;
		while( (q=strsep(&t," \t\n\r")) ){
			if( i == dev_col )  d = q;
			if( i == inod_col ){
#if 0
				fprintf(stderr,"line %d: dev %s, inode %s\n",
					i, d, q); 
#endif
				if( strcmp(inode,q) == 0 ){
					strncpy(dev,d,len);
					dev[len-1]=0;
					return dev;
				}
				break;
			}
			if( *q ) i++;
		};
	};
	fprintf(stderr, "/proc/net/x25 socket inode %s not found\n",inode);
	return dummy;
}
