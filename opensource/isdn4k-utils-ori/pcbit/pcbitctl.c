/*
 * Copyright (C) 1996 Universidade de Lisboa
 * 
 * Writen by Pedro Roque Marques (roque@di.fc.ul.pt)
 *
 * This software may be used and distributed according to the terms of 
 * the GNU Public License, incorporated herein by reference.
 */

/*        
 *        PCBIT-D firmware loader
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <getopt.h>

#include <linux/isdn.h>
#include <pcbit.h>

#define MAXLINEHEX 80
#define NUM_LIN 32
#define MAXSUPERLINE 3000

static void usage(char *);
static int loadfw(int, char *, int);
static int writefw(int, char*);
static int bitd_read(unsigned char* buf, int *len, int nlines, FILE* fp);
static int issue_ioctl(int board, int cmd);

extern int convhexbin (char *filename, unsigned char *buf, int size);



int main(int argc, char *argv[])
{
	int option_index = 0;
	char fwfile[FILENAME_MAX];
	int force = 0;
	int board = 0;
	char c, choice = 0;                     /* {ping,stop,load} */
	static struct option long_options[] =
	{
		{"ping",  0, 0, 'p'},
		{"stop",  0, 0, 's'},
		{"force", 0, 0, 'f'},
		{"load",  1, 0, 'l'},
		{"board", 1, 0, 'b'},
		{0, 0, 0, 0}
	};

	while (1) {
		c = getopt_long(argc, argv, "psfl:b:", long_options, 
				&option_index);
		if ( c == -1 )
			break;

		switch(c) {
		case 'p':
		case 's':
			choice = c;
			break;
		case 'l':
			choice = c;
			strcpy(fwfile, optarg);
			break;
		case 'b':
			board = atoi(optarg);
			break;
		case 'f':
			force = 1;
			break;
		default:
			fprintf(stderr, "unkown option %c\n", c);
			break;
		}
		if (choice)
			break;
	}

	switch (choice) {
	case 's':
		issue_ioctl(board, PCBIT_STOP);
		break;
	case 'p':
		issue_ioctl(board, PCBIT_PING188);
		break;
	case  'l':
		return loadfw(board, fwfile, force);
		break;
	default:
		usage(argv[0]);
		return 1;
	}

	return 0;
}

static void usage(char *name){
	fprintf(stderr, "usage:" 
		"\t%s ping\n"
		"\t%s stop\n"
		"\t%s load [-f] <firmware>\n", 
		name, name, name);
}


static int getrdp_byte(int fd, int board, ushort addr, unsigned short * value)
{
	isdn_ioctl_struct io_cmd;
	struct pcbit_ioctl * pcbit_cmd;

	strcpy(io_cmd.drvid, "pcbitX");
	io_cmd.drvid[5] = '0' + board;

	pcbit_cmd = (struct pcbit_ioctl *) &io_cmd.arg;

	pcbit_cmd->info.rdp_byte.addr = addr;
	pcbit_cmd->info.rdp_byte.value = 0x0000;

	if (ioctl(fd,  PCBIT_GETBYTE, &io_cmd) == -1)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

	*value = pcbit_cmd->info.rdp_byte.value;
	return 0;
}

static int setrdp_byte(int fd, int board, ushort addr, unsigned char value)
{
	isdn_ioctl_struct io_cmd;
	struct pcbit_ioctl * pcbit_cmd;

	strcpy(io_cmd.drvid, "pcbitX");
	io_cmd.drvid[5] = '0' + board;

	pcbit_cmd = (struct pcbit_ioctl *) &io_cmd.arg;

	pcbit_cmd->info.rdp_byte.addr = addr;
	pcbit_cmd->info.rdp_byte.value = value;

	if (ioctl(fd,  PCBIT_SETBYTE, &io_cmd) == -1)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}
	return 0;
}

static int testrdp(int fd, int board)
{
	int retv;
	unsigned short value;

	retv = setrdp_byte(fd, board, 0x0000, 0x55);

	if (!retv)
		return retv;

	value = 0x00;

	retv = getrdp_byte(fd, board, 0x0000, &value);

	if (!retv)
		return retv;

	if (value != 0x55)
	{
		fprintf(stderr, "test_board: values don't match\n");
		return -1;
	}

	retv = setrdp_byte(fd, board, 0x0000, 0xaa);
	if (!retv)
		return retv;

	value = 0x00;
	retv = getrdp_byte(fd, board, 0x0000, &value);
	if (!retv)
		return retv;

	if (value != 0xaa)
	{
		fprintf(stderr, "test_board: values don't match\n");
		return -1;
	}

	return 0;
}


int issue_ioctl(int board, int cmd)
{
	int fd;
	isdn_ioctl_struct io_cmd;
	struct pcbit_ioctl * pcbit_cmd;
  

	if ( (fd = open("/dev/isdnctrl", O_RDWR)) == -1) {	
		fprintf(stderr, "error opening /dev/isdnctl: %s\n", 
			strerror(errno));
		return errno;
	}

	strcpy(io_cmd.drvid, "pcbitX");
	io_cmd.drvid[5] = '0' + board;


	pcbit_cmd = (struct pcbit_ioctl *) &io_cmd.arg;
  
	if (ioctl(fd,  cmd, &io_cmd) < 0) {	
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

	return 0;
}


static int loadfw(int board, char *fname, int force)
{
	int fd;
	int attempt, retv, i, j;
	unsigned short value;
	isdn_ioctl_struct io_cmd;
	struct pcbit_ioctl * pcbit_cmd;
	unsigned char stpd1[1024], stpd2[1024], zerobuf[1024];
	unsigned char execstr[7] = {
		0x01,
		0x04,
		0x00,
		0x00,
		0x00,
		0x00,
		0x12
	};
	
	/* 
	 * 1. get_status 
	 * 2. load stpd.1 
	 * 3. load stpd.2 
	 * 4. test 
	 * 5. firmware load 
	 * 6. execute 
	 * 7. set_protocol_running 
	 */


	strcpy(io_cmd.drvid, "pcbitX");
	io_cmd.drvid[5] = '0' + board;

	pcbit_cmd = (struct pcbit_ioctl *) &io_cmd.arg;

	if ( (fd = open("/dev/isdnctrl", O_RDWR)) == -1) {    
		fprintf(stderr, "error opening /dev/isdnctl: %s\n", 
			strerror(errno));
		return errno;
	}
  
	if (ioctl(fd,  PCBIT_GETSTAT, &io_cmd) < 0) {	
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

	if (pcbit_cmd->info.l2_status == L2_RUNNING) {
		if (force) {
			if (ioctl(fd,  PCBIT_STOP, &io_cmd) < 0) {	
			  fprintf(stderr, "ioctl failed: %s\n", 
					strerror(errno));
				return errno;
			}			
		}
		else {
			fprintf(stderr, "PCBIT-D driver is up and running. no action nedded\n");
			return 0;
		}
	}

#ifdef DEBUG
	fprintf(stderr, "1. Status check OK\n");
#endif
	/*
	 * load stpd.{1,2}
	 */


	memset(stpd1, 0, 1024);
	memset(stpd2, 0, 1024);

	if ( convhexbin("stpd.1", stpd1, 1024) < 0 )
	{
		fprintf(stderr, "error reading stpd.1\n");
		return -1;
	}

	if ( convhexbin("stpd.2", stpd2, 1024) < 0 )
	{
		fprintf(stderr, "error reading stpd.2\n");
		return -1;
	}

#ifdef DEBUG
	fprintf(stderr, "2. stpd.{1,2} loaded \n");
#endif

	if (ioctl(fd,  PCBIT_STRLOAD, &io_cmd) < 0)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

	if (ioctl(fd,  PCBIT_WATCH188, &io_cmd) < 0) {	
	  fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
	  return errno;

	}

	if (ioctl(fd,  PCBIT_LWMODE, &io_cmd) < 0)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

	memset(zerobuf, 0, 1024);
	testrdp(fd, board);


#ifdef DEBUG
	fprintf(stderr, "3.a linear write mode\n");
#endif

	write(fd, zerobuf, 1024);

	sleep(1);

	if (testrdp(fd, board))
	{
		fprintf(stderr, "error: testrdp\n");
		return -1;
	}

	write(fd, stpd1, 1024);

	sleep(1);

	for (j=0; j<2; j++) {

		for (i=0; i<60; i++)
		{
			getrdp_byte(fd, board, 0x03fd, &value);
			if (value == 0x55)
				break;

			if (write(fd, stpd1, 1021) < 0)
			{
				printf("error in write\n");
				return -1;
			}

			usleep(200000);
			/*
			  setrdp_byte(fd, board, 0x03fd, 0x00);
			  setrdp_byte(fd, board, 0x03ff, 0x00);
			  */
		}

		if (i == 60)
		{
			printf("error in stpd.1 load\n");
			return -1;
		}

		for (i=0; i<180; i++)
		{
			getrdp_byte(fd, board, 0x03ff, &value);
			if (value == 0x55)
				break;
			usleep(20000);
		}

		if (i == 180)
		{
			printf("error in stpd.1 load - 2 val \n");
			return -1;
		}
  
		getrdp_byte(fd, board, 0x03fe, &value);

		if (value == 0x1f)
		{
			printf("1f\n");
			break;
		}
		setrdp_byte(fd, board, 0x03fe, 0x00);
		setrdp_byte(fd, board, 0x03ff, 0x00);
	}

#ifdef DEBUG
	fprintf(stderr, "3.c stpd.1 writen\n");
#endif

	memcpy(stpd2 + 0x3e0, stpd1 + 0x3e0, 0x10);
	if (write(fd, stpd2, 1023) < 0)
	{
		fprintf(stderr, "write failed: %s\n", strerror(errno));
		return errno;
	}

	value = 0x00;
	attempt = 0;

	while(1)
	{
		if ((retv = getrdp_byte(fd, board, 0x03ff, &value)))
		{
			fprintf(stderr, "read_byte: error\n");
			return retv;
		}

		if (value == 0x55)
			break;

		setrdp_byte(fd, board, 0x03ff, 0x00);

		if (attempt==120)
		{
			fprintf(stderr, "Communication error\n");
			return -1;
		}
		else
			attempt++;

		usleep(50);
	}

#ifdef DEBUG
	fprintf(stderr, "3.d stpd.2 writen\n");
#endif

	if (ioctl(fd,  PCBIT_FWMODE, &io_cmd) == -1)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}
#ifdef DEBUG
	fprintf(stderr, "4.a load mode\n");
#endif
  
	if (writefw(fd, fname))
		return -1;

	/* execute */

	if (write(fd, execstr, 7) < 0)
	{
		fprintf(stderr, "write failed: %s\n", strerror(errno));
		return errno;
	}

#ifdef DEBUG
	fprintf(stderr, "4.b execute\n");
#endif

	/*
	sleep(10);

	if (ioctl(fd,  PCBIT_STRLOAD, &io_cmd) == -1)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

	if (ioctl(fd,  PCBIT_APION, &io_cmd) == -1)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

	if (ioctl(fd,  PCBIT_PING188, &io_cmd) == -1)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

	sleep(5);
	*/

	if (ioctl(fd,  PCBIT_ENDLOAD, &io_cmd) == -1)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

#ifdef DEBUG
	fprintf(stderr, "4.c load ok\n");
#endif

	sleep(2);

	if (ioctl(fd,  PCBIT_RUNNING, &io_cmd) == -1)
	{
		fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
		return errno;
	}

#ifdef DEBUG
	fprintf(stderr, "5. running\n");
#endif
	return 0;
}

static int writefw(int fd, char* fname)
{
  FILE *fp;
  char shdr[80];
  char buf[MAXSUPERLINE];
  int i, len;
  
  if ((fp=fopen(fname, "rb")) == NULL) 
    {
      fprintf(stderr, "error in fopen %s:%s\n", fname, strerror(errno));
      return -1;
    }

  fgets (shdr, 80, fp);
  fprintf(stderr, "Loading %s \n", shdr);

  len = MAXSUPERLINE;
  while((i=bitd_read (buf + 3, &len, 80, fp))) {

    buf[0]=0;

    *((short *)&buf[1])=(short)(len);

    if (write(fd, buf, len + 3) < 0)
      {
	fprintf(stderr, "load firmware - error in write:%s\n",strerror(errno));
	return errno;
      }
    fprintf(stderr, ".");
    fflush(stderr);
  }
  
  fprintf(stderr, "\n load complete\n");
  fclose(fp);
  fprintf(stderr, "Firmware loaded\n");
  return 0;
}



/*
 *  Discard 1st char (':') and convert the rest to bin
 */


static int bitd_read(unsigned char* buf, int *len, int nlines, FILE* fp)
{
  char line[80];
  int i, j;
  unsigned int val;
  int aux = 0;

  *len = 1;

  for (i = 0; i < nlines; i++) {
    if (fgets(line, 80, fp) == NULL)            
      break;

    aux += strlen(line) - 1;

    line[strlen(line) - 1] = 0;

    for (j=1; line[j]; j+=2) {  

      sscanf(line + j, "%02x", &val);

      buf[(*len)++] = val;
    }
  }


  buf[0] = i;
  
  return aux;
}








