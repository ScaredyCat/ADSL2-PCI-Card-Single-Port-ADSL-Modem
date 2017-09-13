//060414:leejack Fixed closing vcc issue

/*
 * Nirav.
 * The actual br2684ctl application is divided into 2 application : br2684ctld, br2684ctl
 * IFX_BR2684DEAMON : This code belongs to the daemon. When the SIGHUP is received, it
 * reads the command from BR2684_CMDFILE and creates the socket interface and keeps the socket open
 * or closes the socket for already opened interface. Thus, for all the interface in the system 
 * only one br2684ctld needs to be run. While previously, as many br2684ctl as many no. of nas interface 
 * were required to be run. Thus, this approach saves RAM.
 * IFX_BR2684CLIENT : This code belongs to the client. This is just a dummy client, which
 * write the command passed to it in the BR2684_CMDFILE and exits.
 * */

//605161:fchang 2006/5/16 Include the same header files to be consistent

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <syslog.h>
#include <atm.h>
//605161:fchang.removed #include <linux/atmdev.h>
//605161:fchang.removed #include <linux/atmbr2684.h>

#include "../../../kernel/opensource/linux-2.4.31/include/linux/atmdev.h" //605161:fchang.added
#include "../../../kernel/opensource/linux-2.4.31/include/linux/atmbr2684.h" //605161:fchang.added


#include "ifx_common.h"
#if defined(IFX_BR2684DEAMON) || defined(IFX_BR2684CLIENT)
#include <fcntl.h>
#include <signal.h>
#endif


#define BR2684_FILE_PREFIX  "nas"

/* Written by Marcell GAL <cell@sch.bme.hu> to make use of the */
/* ioctls defined in the br2684... kernel patch */
/* Compile with cc -o br2684ctl br2684ctl.c -latm */

/*
   Modified feb 2001 by Stephen Aaskov (saa@lasat.com)
   - Added daemonization code
   - Added syslog

TODO: Delete interfaces after exit?
*/


#define LOG_NAME        "RFC1483/2684 bridge"
#define LOG_OPTION      LOG_PERROR
#define LOG_FACILITY    LOG_LOCAL0

#ifdef IFX_BR2684DEAMON
#define exit(x)		return(-(x))
#endif

#ifndef IFX_BR2684CLIENT /* [ IFX_BR2684CLIENT */

int lastsock, lastitf;
int unit_id;


#ifdef IFX_BR2684DEAMON
int fatal(const char *str, int i)
#else
void fatal(const char *str, int i)
#endif
{
	syslog (LOG_ERR,"Fatal: %s",str);
	exit(2);
};


void exitFunc(void)
{
	syslog (LOG_PID,"Daemon terminated\n");
}



int create_br(char *nstr, int payload)
{
	int num, err;

	if(lastsock<0) {
		lastsock = socket(PF_ATMPVC, SOCK_DGRAM, ATM_AAL5);
	}
	if (lastsock<0) {
		syslog(LOG_ERR, "socket creation failed: %s",strerror(errno));
	} else {
		/* create the device with ioctl: */
		num=atoi(nstr);
		if( num>=0 && num<1234567890){
			struct atm_newif_br2684 ni;
			ni.backend_num = ATM_BACKEND_BR2684;
			ni.media = BR2684_MEDIA_ETHERNET;
			ni.mtu = 1500;
			ni.payload = payload;	/* bridged or routed */

			sprintf(ni.ifname, "%s%d", BR2684_FILE_PREFIX,  num);
			err=ioctl (lastsock, ATM_NEWBACKENDIF, &ni);

			if (err == 0)
			{
				syslog(LOG_INFO, "Interface \"%s\" created sucessfully\n",ni.ifname);
				unit_id = num;
			}
			else
				syslog(LOG_INFO, "Interface \"%s\" could not be created, reason: %s\n",
						ni.ifname,strerror(errno));
			lastitf=num;    /* even if we didn't create, because existed, assign_vcc wil want to know it! */
		} else {
			syslog(LOG_ERR,"err: strange interface number %d", num );
		}
	}
	return 0;
}

int assign_vcc(char *astr, int encap, int payload, int bufsize, struct atm_qos qos)
{
	int err=0;
	struct sockaddr_atmpvc addr;
	int fd;
	struct atm_backend_br2684 be;

	memset(&addr, 0, sizeof(addr));
	err=text2atm(astr,(struct sockaddr *)(&addr), sizeof(addr), T2A_PVC);
	if (err!=0)
		syslog(LOG_ERR,"Could not parse ATM parameters (error=%d)\n",err);

#if 0
	addr.sap_family = AF_ATMPVC;
	addr.sap_addr.itf = itf;
	addr.sap_addr.vpi = 0;
	addr.sap_addr.vci = vci;
#endif
	syslog(LOG_INFO,"Communicating over ATM %d.%d.%d, encapsulation: %s\n", addr.sap_addr.itf,
			addr.sap_addr.vpi,
			addr.sap_addr.vci,
			encap?"VC mux":"LLC");

	if ((fd = socket(PF_ATMPVC, SOCK_DGRAM, ATM_AAL5)) < 0)
		syslog(LOG_ERR,"failed to create socket %d, reason: %s", errno,strerror(errno));

	if (qos.aal == 0) {
		qos.aal                     = ATM_AAL5;
		qos.txtp.traffic_class      = ATM_UBR;
		qos.txtp.max_sdu            = 1524;
		qos.txtp.pcr                = ATM_MAX_PCR;
		qos.rxtp = qos.txtp;
	}

	if ( (err=setsockopt(fd,SOL_SOCKET,SO_SNDBUF, &bufsize ,sizeof(bufsize))) )
		syslog(LOG_ERR,"setsockopt SO_SNDBUF: (%d) %s\n",err, strerror(err));

	if (setsockopt(fd, SOL_ATM, SO_ATMQOS, &qos, sizeof(qos)) < 0)
		syslog(LOG_ERR,"setsockopt SO_ATMQOS %d", errno);

	err = connect(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_atmpvc));

	if (err < 0)
		fatal("failed to connect on socket", err);
#ifdef IFX_BR2684DEAMON
	if(err < 0) {
		syslog(LOG_ERR,"br2684ctld : Connect Error!!!");
		return err;
	}
#endif

	/* attach the vcc to device: */

	be.backend_num = ATM_BACKEND_BR2684;
	be.ifspec.method = BR2684_FIND_BYIFNAME;
	sprintf(be.ifspec.spec.ifname, "%s%d", BR2684_FILE_PREFIX, lastitf);
	be.fcs_in = BR2684_FCSIN_NO;
	be.fcs_out = BR2684_FCSOUT_NO;
	be.fcs_auto = 0;
	be.encaps = encap ? BR2684_ENCAPS_VC : BR2684_ENCAPS_LLC;
	be.payload = payload;
	be.has_vpiid = 0;
	be.send_padding = 0;
	be.min_size = 0;
	err=ioctl (fd, ATM_SETBACKEND, &be);
	if (err == 0)
		syslog (LOG_INFO,"Interface configured");
	else {
		syslog (LOG_ERR,"Could not configure interface:%s",strerror(errno));
		exit(2);
	}
	return fd ;
}


#ifdef IFX_BR2684DEAMON
int usage(char *s)
#else
void usage(char *s)
#endif
{
	printf("usage: %s [-b] [-p 0|1] [-s sndbuf] [-q qos] [-c number] [-e 0|1] [-a [itf.]vpi.vci]\n", s);
	exit(1);
}

#endif /* ]IFX_BR2684CLIENT */

#if defined(IFX_BR2684DEAMON) || defined(IFX_BR2684CLIENT)
#define DEAMON_PIDFILE_NAME	"br2684ctld"
#define BR2684CTL_DEAMON	"/usr/sbin/br2684ctld"
#define BR2684_CMDFILE	"/tmp/br2684cmd"
#endif /* IFX_BR2684DEAMON || IFX_BR2684CLIENT */


#ifdef IFX_BR2684DEAMON /*[IFX_BR2684DEAMON */
#define PROGRAM_NAME	"br2684ctl"
#define CLOSEIF_OPTION	"-k"

struct br2684_sock {
	int ifnum;
	int sockfd;
	struct br2684_sock *next;
};

struct br2684_sock *brsock = NULL;

int br2684_main(int argc, char **argv);

void terminate(int num) 
{
	syslog(LOG_INFO,"br2684ctld : Received SIGTERM or no interface configured.");
	ifx_rm_pid_file(DEAMON_PIDFILE_NAME);
#ifdef exit
#undef exit
	exit(0);
#endif
}

void config_br2684(int num)
{
	int cmd_fd = -1;
	char strcmd[256];
	char **argv = NULL;
	int argc = 0;
	char *ptemp = NULL;
	int sockfd = -1;
	int i = 0;

	syslog(LOG_INFO,"br2684ctld HUP received");
	cmd_fd = open(BR2684_CMDFILE,O_RDONLY);
	if(cmd_fd < 0) {
		syslog(LOG_INFO,"br2684ctld : Could not open %s file!!!",BR2684_CMDFILE);	
		return ;
	}
	memset(strcmd,0x00,sizeof(strcmd));
	if(read(cmd_fd,strcmd,sizeof(strcmd)) <= 0) {
		syslog(LOG_INFO,"br2684ctld : No data read from file %s!!!",BR2684_CMDFILE);
		close(cmd_fd);
		return ;
	}
	close(cmd_fd);

	//060414:leejack	
	unlink(BR2684_CMDFILE);

	if(strstr(strcmd,CLOSEIF_OPTION)) {
		int intfnum = 0;
		struct br2684_sock *ptrcur = NULL;
		struct br2684_sock *ptrpre = NULL;
		ptemp = strstr(strcmd,CLOSEIF_OPTION);
		ptemp += strlen(CLOSEIF_OPTION);
		intfnum = atoi(ptemp);
		ptrcur = brsock;
		ptrpre = brsock;
		while(ptrcur) {
			if(ptrcur->ifnum == intfnum)
				break;
			ptrpre = ptrcur;	
			ptrcur = ptrcur->next;
		}
		
		if(ptrcur) {
			syslog(LOG_INFO,"br2684ctld : Closing socket for nas%d",ptrcur->ifnum);
			close(ptrcur->sockfd);
			ptrpre->next = ptrcur->next;
			if(ptrcur == brsock)
				brsock = ptrcur->next;
			free(ptrcur);
		} else {
			syslog(LOG_INFO,"br2684ctld : Could not find socket for nas%d",intfnum);
		}
		if(brsock == NULL)
			terminate(SIGTERM);
		return;
	}

	argc++;
	argv = (char **)calloc(1,sizeof(char **));
	argv[argc - 1] = (char *)calloc(1, strlen(PROGRAM_NAME) + 1);
	strncpy(argv[argc - 1],PROGRAM_NAME,strlen(PROGRAM_NAME));

	ptemp = strtok(strcmd," ");
	while(ptemp) {
		argc++;
		argv = (char **)realloc(argv,argc * sizeof(char **));
		argv[argc - 1] = (char *)calloc(1,strlen(ptemp) + 1);
		strncpy(argv[argc - 1],ptemp,strlen(ptemp));
		ptemp = strtok(NULL," ");
	}
	sockfd = br2684_main(argc,argv);
	if(sockfd > 0) {
		struct br2684_sock *ptr = NULL;
		ptr = (struct br2684_sock *)calloc(1,sizeof(*ptr));
		ptr->ifnum = lastitf;
		ptr->sockfd = sockfd;
		ptr->next = brsock;
		brsock = ptr;
		syslog(LOG_INFO,"br2684ctld : Interface nas%d with sockfd %d",lastitf,sockfd);
	} 

	for(i = 0; i < argc; i++)
		if(argv[i])
			free(argv[i]);
	free(argv);
	argc = 0;
	argv = NULL;
	if(brsock == NULL)
		terminate(SIGTERM);
	return ;
}

int main(int argc, char **argv)
{
	struct sigaction sa;

	daemon(0, 0);
	ifx_create_pid_file(DEAMON_PIDFILE_NAME);

	syslog(LOG_INFO,"br2684ctld started");
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = config_br2684;
	sigaction(SIGHUP, &sa, NULL);
	sa.sa_handler = terminate;
	sigaction(SIGTERM, &sa, NULL);

	while (1) sleep(10);    /* to keep the sockets... */
	return 0;
}
#endif /* ]IFX_BR2684DEAMON */

/*Modify by Henry , Add a flag to change bridge PDU / routed PDU.*/
#ifdef IFX_BR2684DEAMON
int br2684_main(int argc, char **argv)
#else
int main (int argc, char **argv)
#endif
{
#ifdef IFX_BR2684CLIENT
	char strcmd[256];
	int cmd_fd = -1;
	int deamon_pid = -1;
	int i;

	memset(strcmd,0x00,sizeof(strcmd));
	for(i = 1; i < argc; i++) {
		strcat(strcmd,argv[i]);
		strcat(strcmd," ");
	}
	strcmd[strlen(strcmd) - 1] = '\0';
	cmd_fd = open(BR2684_CMDFILE,O_WRONLY | O_CREAT);
	if(cmd_fd < 0) {
		perror("br2684ctl ");
		syslog(LOG_ERR,"Could not open cmd file %s in write mode!!!",BR2684_CMDFILE);
		return -1;
	}
	write(cmd_fd,strcmd,strlen(strcmd));
	close(cmd_fd);
	
	deamon_pid = ifx_get_process_pid(DEAMON_PIDFILE_NAME);
	if(deamon_pid < 1) {
		/* deamon is not running */
		system(BR2684CTL_DEAMON);
		sleep(2);
		deamon_pid = ifx_get_process_pid(DEAMON_PIDFILE_NAME);
	}

	if(deamon_pid < 1) {
		syslog(LOG_ERR,"Could not get pid of %s!!!",BR2684CTL_DEAMON);
	} else {
		syslog(LOG_INFO,"br2684_ctl : Sending SIGHUP to %d",deamon_pid);
		kill(deamon_pid,SIGHUP);
	}

	return 0;
#else
	int c, background=0, encap=0, sndbuf=8192;
	int payload=1;

	struct atm_qos reqqos;
	lastsock=-1;
	lastitf=0;
	char itf_name[20];
	int fd = -1;

	/* st qos to 0 */
	memset(&reqqos, 0, sizeof(reqqos));

	//openlog (LOG_NAME,LOG_OPTION,LOG_FACILITY);
	optind = 0;
	if (argc>1)
		while ((c = getopt(argc, argv,"a:bc:e:s:p:q:?h")) !=EOF)
			switch (c) {
				case 'b':
					background=1;
					break;
				case 'p':	/* payload type: routed (0) or bridged (1) */
					payload = atoi(optarg);
					break;
				case 'c':
					create_br(optarg, payload);
					break;
				case 'e':
					encap=(atoi(optarg));
					if(encap<0){
						syslog (LOG_ERR, "invalid encapsulation: %s:\n",optarg);
						encap=0;
					}
					break;
				case 's':
					sndbuf=(atoi(optarg));
					if(sndbuf<0){
						syslog(LOG_ERR, "Invalid sndbuf: %s, using size of 8192 instead\n",optarg);
						sndbuf=8192;
					}
					break;
				case 'q':
					printf ("optarg : %s",optarg);
					if (text2qos(optarg,&reqqos,0)) fprintf(stderr,"QOS parameter invalid\n");
					break;
				case 'a':
					fd = assign_vcc(optarg, encap, payload,sndbuf, reqqos);
					break;
				case '?':
				case 'h':
				default:
					usage(argv[0]);
			}
	else
		usage(argv[0]);

	if (argc != optind) usage(argv[0]);

	if(lastsock>=0) close(lastsock);

#ifdef IFX_BR2684DEAMON
	return fd;
#else
	if (background) {
		pid_t pid;

		pid=fork();
		if (pid < 0) {
			fprintf(stderr,"Error detaching\n");
			exit(2);
		} else if (pid)
			exit(0); // This is the parent

		// Become a process group and session group leader
		if (setsid()<0) {
			fprintf (stderr,"Could not set process group\n");
			exit(2);
		}

		// Fork again to let process group leader exit
		pid = fork();
		if (pid < 0) {
			fprintf(stderr,"Error detaching during second fork\n");
			exit(2);
		} else if (pid)
			exit(0); // This is the parent

		// Now we're ready for buisness
		chdir("/");            // Don't keep directories in use
		close(0); close(1); close(2);  // Close stdin, -out and -error
		/*
		   Note that this implementation does not keep an open
		   stdout/err.
		   If we need them they can be opened now
		   */

	}

	sprintf(itf_name,"%s%d",BR2684_FILE_PREFIX, unit_id);
	ifx_create_pid_file(itf_name);

	syslog (LOG_INFO, "RFC 1483/2684 bridge daemon started\n");
	atexit (exitFunc);

	while (1) sleep(30);    /* to keep the sockets... */
	return 0;
#endif /* IFX_BR2684DEAMON */
#endif /* IFX_BR2684CLIENT */
}


