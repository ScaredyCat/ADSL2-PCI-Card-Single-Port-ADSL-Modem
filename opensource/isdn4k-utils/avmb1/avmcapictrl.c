/*
 * $Id: avmcapictrl.c,v 1.16 2001/03/01 14:59:11 paul Exp $
 * 
 * AVM-B1-ISDN driver for Linux. (Control-Utility)
 * 
 * Copyright 1996 by Carsten Paeth (calle@calle.in-berlin.de)
 * 
 * $Log: avmcapictrl.c,v $
 * Revision 1.16  2001/03/01 14:59:11  paul
 * Various patches to fix errors when using the newest glibc,
 * replaced use of insecure tempnam() function
 * and to remove warnings etc.
 *
 * Revision 1.15  2000/03/08 13:40:33  calle
 * check for /dev/isdn/capi20 if /dev/capi20 doesn't exist.
 *
 * Revision 1.14  2000/01/28 16:36:19  calle
 * generic addcard (call add_card function of named driver)
 *
 * Revision 1.13  1999/12/06 17:01:51  calle
 * more documentation and possible errors explained.
 *
 * Revision 1.12  1999/07/01 16:37:53  calle
 * New command with new driver
 *    avmcapictrl trace [contrnr] [off|short|on|full|shortnodate|nodata]
 * to make traces of capi messages per controller.
 *
 * Revision 1.11  1999/06/21 15:30:45  calle
 * extend error message if io port is out of range, now tell user that an
 * AVM B1 PCI card must be added by loading module b1pci.
 *
 * Revision 1.10  1998/07/15 15:08:20  calle
 * port and irq check changed.
 *
 * Revision 1.9  1998/02/27 15:42:00  calle
 * T1 running with slow link.
 *
 * Revision 1.8  1998/02/24 17:56:23  calle
 * changes for T1.
 *
 * Revision 1.7  1998/02/07 20:32:00  calle
 * update man page, remove old cardtype M1, add is done via avm_cs.o
 *
 * Revision 1.6  1998/02/07 20:09:00  calle
 * - added support for DN1/SPID1 DN2/SPID2 for 5ESS und NI1 protocols.
 * - allow debuging of patchvalues.
 * - optimize configure.in/configure
 *
 * Revision 1.5  1998/01/16 14:02:08  calle
 * patchvalues working now, leased lines and dchannel protocols like
 * CT1,VN3 und AUSTEL support okay, point to point also patchable.
 *
 * Revision 1.3  1997/12/07 20:02:22  calle
 * prepared support for cardtype and different protocols
 *
 * Revision 1.2  1997/03/20 00:18:57  luethje
 * inserted the line #include <errno.h> in avmb1/avmcapictrl.c and imon/imon.c,
 * some bugfixes, new structure in isdnlog/isdnrep/isdnrep.c.
 *
 * Revision 1.1  1997/03/04 22:46:32  calle
 * Added program to add and download firmware to AVM-B1 card
 *
 * Revision 2.2  1997/02/12 09:31:39  calle
 * more verbose error messages
 *
 * Revision 1.1  1997/01/31 10:32:20  calle
 * Initial revision
 *
 * 
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/isdn.h>
#include <linux/b1lli.h>
#include <linux/capi.h>
/* new ioctls */
#include <linux/kernelcapi.h>

static char capidevname[] = "/dev/capi20";
static char capidevnamenew[] = "/dev/isdn/capi20";

char *cmd;
char *ctrldev;
int arg_ofs;

int debugpatch = 0;

void usage(void)
{
	fprintf(stderr, "usage: %s add <portbase> <irq> [B1|T1 [<cardnr>]] (Add a new card)\n", cmd);
	fprintf(stderr, "   or: %s load <bootcode> [contrnr [protocol [P2P | DN1:SPID1 [DN2:SPID2]]]] (load firmware)\n", cmd);
	fprintf(stderr, "   or: %s reset [contrnr] (reset controller)\n", cmd);
	fprintf(stderr, "   or: %s remove [contrnr] (reset controller)\n", cmd);
#ifdef KCAPI_CMD_TRACE
	fprintf(stderr, "   or: %s trace [contrnr] [off|short|on|full|shortnodate|nodata]\n", cmd);
#endif
#ifdef KCAPI_CMD_ADDCARD
	fprintf(stderr, "   or: %s addcard <driver> <portbase> <irq> [ <membase> [ <cardnr> ] ]\n", cmd);
#endif
	exit(1);
}

#define DP_ERROR	-1
#define DP_NONE		0
#define DP_DSS1		1
#define DP_D64S		2
#define DP_D64S2	3
#define DP_D64SD	4
#define DP_DS01		5
#define DP_DS02		6
#define DP_CT1		7
#define DP_VN3		8
#define DP_AUSTEL	9
#define DP_5ESS		10 	/* need SPID,SPID2,DN,DN2 */
#define DP_NI1		11 	/* need SPID,SPID2,DN,DN2 */
#define DP_DSS1MOBIL	12
#define DP_1TR6MOBIL	13
#define DP_GSM		14
#define DP_1TR6		15
#define DP_T1		16

static struct pmap {
  char *name;
  int   protocol;
} pmap[] = {
  { "DSS1", DP_DSS1 },
  { "D64S", DP_D64S },
  { "D64S2", DP_D64S2 },
  { "D64SD", DP_D64SD },
  { "DS01", DP_DS01  },
  { "DS02", DP_DS02 },
  { "CT1", DP_CT1 },
  { "VN3", DP_VN3 },
  { "AUSTEL", DP_AUSTEL },
  { "5ESS", DP_5ESS },
  { "NI1", DP_NI1 },
  { "T1", DP_T1 },
#if 0
  { "DSS1MOBIL", DP_DSS1MOBIL },
  { "1TR6MOBIL", DP_1TR6MOBIL },
  { "GSM", DP_GSM },
  { "1TR6", DP_1TR6 },
#endif
  { 0 },
};

static void show_protocols()
{
   struct pmap *p;
   int pos = 0;
   for (p=pmap; p->name; p++) {
      int len = strlen(p->name);
      if (pos + len + 2 > 80) {
	 fprintf(stderr,",\n");
	 pos = 0;
      }
      if (pos == 0) {
         fprintf(stderr, "        %s", p->name);
         pos += 8 + len;
      } else {
         fprintf(stderr, ",%s", p->name);
	 pos += 1 + len;
      }
   }
   if (pos)
	fprintf(stderr, "\n");
}

static int dchan_protocol(char *pname)
{
   struct pmap *p;
   for (p=pmap; p->name; p++) {
      if (strcasecmp(pname, p->name) == 0)
	 return p->protocol;
   }
   return DP_ERROR;
}

static char patcharea[2048];
static int  patchlen = 0;

static void addpatchvalue(char *name, char *value, int len)
{
   int nlen = strlen(name);
   if (patchlen + nlen + len + 2 >= sizeof(patcharea)) {
      fprintf(stderr, "%s: can't add patchvalue %s\n" , cmd, name);
      exit(3);
   }
   patcharea[patchlen++] = ':';
   memcpy(&patcharea[patchlen], name, nlen);
   patchlen += nlen;
   patcharea[patchlen++] = 0;
   memcpy(&patcharea[patchlen], value, len);
   patchlen += len;
   patcharea[patchlen++] = 0;
   patcharea[patchlen+1] = 0;
}

int set_configuration(avmb1_t4file *t4config, int protocol, int p2p,
		      char *dn1, char *spid1, char *dn2, char *spid2)
{
   if (protocol != DP_T1) {
      addpatchvalue("WATCHDOG", "1", 1);
      addpatchvalue("AutoFrame", "\001", 1);
   } else {
      addpatchvalue("WATCHDOG", "0", 1);
   }
   switch (protocol) {
      case DP_T1: 
      case DP_NONE: 
	 break;
      case DP_DSS1: 
	 break;
      case DP_D64S: 
      case DP_D64S2: 
      case DP_D64SD: 
	 p2p = 0;
         addpatchvalue("FV2", "2", 1);
         addpatchvalue("TEI", "\000", 1);
	 break;
      case DP_DS01: 
      case DP_DS02: 
	 p2p = 0;
         addpatchvalue("FV2", "1", 1);
         addpatchvalue("TEI", "\000", 1);
	 break;
      case DP_CT1: 
         addpatchvalue("PROTOCOL", "\001", 1);
	 break;
      case DP_VN3: 
         addpatchvalue("PROTOCOL", "\002", 1);
	 break;
      case DP_AUSTEL: 
         addpatchvalue("PROTOCOL", "\004", 1);
	 break;
      case DP_NI1: 
	 p2p = 0;
         addpatchvalue("PROTOCOL", "\003", 1);
	 if (dn1 && spid1) {
            addpatchvalue("DN", dn1, strlen(dn1));
            addpatchvalue("SPID", spid1, strlen(spid1));
	 }
	 if (dn2 && spid2) {
            addpatchvalue("DN2", dn2, strlen(dn2));
            addpatchvalue("SPID2", spid2, strlen(spid2));
	 }
	 break;
      case DP_5ESS: 
	 p2p = 0;
         addpatchvalue("PROTOCOL", "\005", 1);
	 if (dn1 && spid1) {
            addpatchvalue("DN", dn1, strlen(dn1));
            addpatchvalue("SPID", spid1, strlen(spid1));
	 }
	 if (dn2 && spid2) {
            addpatchvalue("DN2", dn2, strlen(dn2));
            addpatchvalue("SPID2", spid2, strlen(spid2));
	 }
	 break;
      case DP_DSS1MOBIL: 
         addpatchvalue("PatchMobileMode", "0", 1);
	 break;
      case DP_1TR6MOBIL: 
         addpatchvalue("PatchMobileMode", "0", 1);
	 break;
      case DP_GSM: 
	 break;
      case DP_1TR6: 
	 break;
      default: 
	 return -1;
   }
   if (p2p) {
      addpatchvalue("P2P", "\001", 1);
      addpatchvalue("TEI", "\000", 1);
   }
   t4config->len = patchlen+1;
   t4config->data = patcharea;
   if (debugpatch) {
      FILE *fp = fopen("/tmp/b1.pvals", "w");
      if (fp) {
	 fwrite(t4config->data, t4config->len, 1, fp);
	 fclose(fp);
	 fprintf(stderr, "avmcapictrl: patchvalues written to /tmp/b1.pvals\n");
      }
   }
   return 0;
}

int validports[] =
{0x150, 0x250, 0x300, 0x340, 0};
int validirqs[] = { 3, 4, 5, 6, 7, 9, 10, 11, 12, 15, 0};
int validhemairqs[] = { 3, 5, 7, 9, 10, 11, 12, 15, 0};

static int checkportandirq(int cardtype, int port, int irq)
{
	int i;
	if (cardtype == AVM_CARDTYPE_T1) {
		if ((port & 0xf) != 0) {
			fprintf(stderr, "%s: illegal port %d\n", cmd, port);
			fprintf(stderr, "%s: 3 low bits had to be zero\n", cmd);
			return -1;
		}
		if ((port & 0x30) == 0x30) {
			fprintf(stderr, "%s: illegal port %d\n", cmd, port);
			fprintf(stderr, "%s: bit 4 and 5 can not be one together\n", cmd);
			return -1;
		}
		for (i = 0; validhemairqs[i] && irq != validhemairqs[i]; i++);
		if (!validhemairqs[i]) {
			fprintf(stderr, "%s: illegal irq %d\n", cmd, irq);
			fprintf(stderr, "%s: try one of %d", cmd, validhemairqs[0]);
			for (i = 1; validhemairqs[i]; i++)
				fprintf(stderr, ", %d", validhemairqs[i]);
			fprintf(stderr, "\n");
			return -1;
		}
	} else {
		for (i = 0; validports[i] && port != validports[i]; i++);
		if (!validports[i]) {
			fprintf(stderr, "%s: illegal io-addr 0x%x\n", cmd, port);
			fprintf(stderr, "%s: try one of 0x%x", cmd, validports[0]);
			for (i = 1; validports[i]; i++)
				fprintf(stderr, ", 0x%x", validports[i]);
			fprintf(stderr, "\n");
			fprintf(stderr, "%s: to install a B1 PCI card load module b1pci.o\n", cmd);
			return -1;
		}
		for (i = 0; validirqs[i] && irq != validirqs[i]; i++);
		if (!validirqs[i]) {
			fprintf(stderr, "%s: illegal irq %d\n", cmd, irq);
			fprintf(stderr, "%s: try one of %d", cmd, validirqs[0]);
			for (i = 1; validirqs[i]; i++)
				fprintf(stderr, ", %d", validirqs[i]);
			fprintf(stderr, "\n");
			return -1;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int fd;
	int ac;
	capi_manufacturer_cmd ioctl_s;
	avmb1_extcarddef newcard;
	avmb1_loadandconfigdef ldef;
	avmb1_resetdef rdef;
        avmb1_getdef gdef;
	int newdriver;
	char *dn1 = 0;
	char *spid1 = 0;
	char *dn2 = 0;
	char *spid2 = 0;

	cmd = strrchr(argv[0], '/');
	cmd = (cmd == NULL) ? argv[0] : ++cmd;
	if (argc > 1) {
		arg_ofs = 1;
	} else
		usage();
	ac = argc - (arg_ofs - 1);
	if ((fd = open(capidevname, O_RDWR)) < 0 && errno == ENOENT)
  	   fd = open(capidevnamenew, O_RDWR);
	if (fd < 0) {
		switch (errno) {
		   case ENOENT:
		      perror("Device file /dev/capi20 and /dev/isdn/capi20 missing, use instdev");
		      exit(2);
		   case ENODEV:
		      perror("device capi20 not registered");
		      fprintf(stderr, "look in /proc/devices.\n");
		      fprintf(stderr, "maybe the devicefiles are installed with a wrong majornumber,\n");
		      fprintf(stderr, "or you linux kernel version only supports 64 char device (check /usr/include/linux/major.h)\n");
		      exit(2);
		}
		perror("/dev/capi20");
		exit(-1);
	}
	gdef.contr = 0;
	ioctl_s.cmd = AVMB1_GET_CARDINFO;
	ioctl_s.data = &gdef;
	newdriver = 0;

	if ((ioctl(fd, CAPI_MANUFACTURER_CMD, &ioctl_s)) < 0) {
	   if (errno != EINVAL)
	      newdriver = 1;
	} else {
	   newdriver = 1;
	}

	if (!strcasecmp(argv[arg_ofs], "add")) {
	        int port, irq, cardtype, cardnr = 0;
		if (ac >= 4) {
			sscanf(argv[arg_ofs + 1], "%i", &port);
			sscanf(argv[arg_ofs + 2], "%i", &irq);
			if (argv[arg_ofs + 3]) {
			   if (strcasecmp(argv[arg_ofs + 3],"B1") == 0) {
	                      cardtype = AVM_CARDTYPE_B1;
			   } else if (strcasecmp(argv[arg_ofs + 3],"T1") == 0) {
	                      cardtype = AVM_CARDTYPE_T1;
			      if (argv[arg_ofs + 4])
			         sscanf(argv[arg_ofs + 4], "%i", &cardnr);
			   } else {
				fprintf(stderr, "%s: illegal cardtype \"%s\"\n", cmd, argv[arg_ofs + 3]);
				fprintf(stderr, "%s: try one of B1,T1", cmd);
				exit(-1);
			   }
			} else {
	                   cardtype = AVM_CARDTYPE_B1;
			}
			if (checkportandirq(cardtype, port, irq) != 0)
				exit(1);
			newcard.port = port;
			newcard.irq = irq;
			newcard.cardtype = cardtype;
			newcard.cardnr = cardnr;
			if (!newdriver && cardtype != AVM_CARDTYPE_B1) {
			   fprintf(stderr, "%s: only B1 supported by kernel driver, sorry\n", cmd);
			   exit(1);
			}
			if (newdriver)
			   ioctl_s.cmd = AVMB1_ADDCARD_WITH_TYPE;
			else ioctl_s.cmd = AVMB1_ADDCARD;
			ioctl_s.data = &newcard;
			if ((ioctl(fd, CAPI_MANUFACTURER_CMD, &ioctl_s)) < 0) {
				perror("ioctl ADDCARD");
				fprintf(stderr, "%s: please also look at the kernel message, call command dmesg(8)\n", cmd);
				exit(-1);
			}
			close(fd);
			return 0;
		}
	}
	if (   strcasecmp(argv[arg_ofs], "load") == 0
	    || strcasecmp(argv[arg_ofs], "test") == 0 ) {
		struct stat st;
		int codefd;
		int contr = 1;
		int protocol = 0;
		int p2p = 0;

	        if (strcasecmp(argv[arg_ofs], "test") == 0)
		   debugpatch = 1;

		if (ac == 2) {
		   usage();
		   exit(1);
		}

		if (ac > 3)
			contr = atoi(argv[arg_ofs + 2]);

		if (ac > 4) {
			if (!newdriver) {
			   fprintf(stderr, "%s: need newer kernel driver to set protocol\n",
						cmd);
			   exit(1);
			}
			protocol = dchan_protocol(argv[arg_ofs + 3]);
			if (protocol < 0) {
				fprintf(stderr,"invalid protocol \"%s\"\n",
						argv[arg_ofs + 3]);
				show_protocols();
				exit(1);
			}
		}
		if (ac > 5) {
		   if (strcasecmp(argv[arg_ofs + 4], "P2P") == 0) {
		      p2p = 1;
		   } else {
		     if (protocol !=  DP_5ESS && protocol != DP_NI1) {
		        fprintf(stderr,"parameter should be P2P not \"%s\"\n",
				      argv[arg_ofs + 4]);
		        exit(1);
		     }
		     dn1 = argv[arg_ofs + 4];
		     spid1 = strchr(dn1, ':');
		     if (spid1 == 0) {
		        fprintf(stderr,"DN1 and SPID1 should be spearated by ':s': %s\n",
				      argv[arg_ofs + 4]);
		        exit(1);
			
		     }
		     *spid1++ = 0;
		     if (ac > 6) {
			dn2 = argv[arg_ofs + 5];
			spid2 = strchr(dn2, ':');
			if (spid2 == 0) {
			   fprintf(stderr,"DN2 and SPID2 should be spearated by ':s': %s\n",
				      argv[arg_ofs + 5]);
			   exit(1);
			
			}
			*spid2++ = 0;
		     }
		   }
		}

		if (stat(argv[arg_ofs + 1], &st)) {
			perror(argv[arg_ofs + 1]);
			exit(2);
		}
		if (!(codefd = open(argv[arg_ofs + 1], O_RDONLY))) {
			perror(argv[arg_ofs + 1]);
			exit(2);
		}
		ldef.contr = contr;
		ldef.t4file.len = st.st_size;
		ldef.t4file.data = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, codefd, 0);
		if (ldef.t4file.data == (unsigned char *) -1) {
			perror("mmap");
			exit(2);
		}

		ldef.t4config.len = 0;
		ldef.t4config.data = 0;
		if (protocol || p2p || (dn1 && spid1) || (dn2 && spid2)) {
		   set_configuration(&ldef.t4config, protocol, p2p,
					dn1, spid1, dn2, spid2);
		   if (debugpatch) 
		      exit(0);
		} else if (debugpatch) {
		   fprintf(stderr,"avmcapictrl: no patchvalues needed\n");
		}
		printf("Loading Bootcode %s ... ", argv[arg_ofs + 1]);
		fflush(stdout);
	        if (newdriver)
		   ioctl_s.cmd = AVMB1_LOAD_AND_CONFIG;
		else ioctl_s.cmd = AVMB1_LOAD;
		ioctl_s.data = &ldef;
		if ((ioctl(fd, CAPI_MANUFACTURER_CMD, &ioctl_s)) < 0) {
			perror("\nioctl LOAD");
			fprintf(stderr, "%s: please also look at the kernel message, call command dmesg(8)\n", cmd);
			exit(2);
		}
		munmap(ldef.t4file.data, ldef.t4file.len);
		close(codefd);
		close(fd);
		printf("done\n");
		return 0;
	}
	if (!strcasecmp(argv[arg_ofs], "reset")) {
		int contr = 1;

		if (ac > 2)
			contr = atoi(argv[arg_ofs + 1]);

		rdef.contr = contr;
		ioctl_s.cmd = AVMB1_RESETCARD;
		ioctl_s.data = &rdef;
		if ((ioctl(fd, CAPI_MANUFACTURER_CMD, &ioctl_s)) < 0) {
			fprintf(stderr, "%s: please also look at the kernel message, call command dmesg(8)\n", cmd);
			perror("\nioctl RESETCARD");
			exit(2);
		}
		close(fd);
		return 0;
	}
	if (   !strcasecmp(argv[arg_ofs], "remove")
            || !strcasecmp(argv[arg_ofs], "delete")
            || !strcasecmp(argv[arg_ofs], "del")) {
		int contr = 1;

		if (ac > 2)
			contr = atoi(argv[arg_ofs + 1]);

		rdef.contr = contr;
		ioctl_s.cmd = AVMB1_REMOVECARD;
		ioctl_s.data = &rdef;
		if ((ioctl(fd, CAPI_MANUFACTURER_CMD, &ioctl_s)) < 0) {
			fprintf(stderr, "%s: please also look at the kernel message, call command dmesg(8)\n", cmd);
			perror("\nioctl REMOVECARD");
			exit(2);
		}
		close(fd);
		return 0;
	}
#ifdef KCAPI_CMD_TRACE
	if (!strcasecmp(argv[arg_ofs], "trace")) {
		kcapi_flagdef fdef;
		char *s = 0;
		fdef.contr = 1;
		fdef.flag = 0;

		if (ac > 2) {
			s = argv[arg_ofs + 1];
			if (isdigit(*s)) {
			   fdef.contr = atoi(argv[arg_ofs + 1]);
			   s = argv[arg_ofs + 2];
			}
		}
		if (s) {
			if (isdigit(*s)) {
				fdef.flag = atoi(s);
			} else if (strcasecmp(s, "off") == 0) {
				fdef.flag = KCAPI_TRACE_OFF;
			} else if (strcasecmp(s, "short") == 0) {
				fdef.flag = KCAPI_TRACE_SHORT;
			} else if (strcasecmp(s, "on") == 0) {
				fdef.flag = KCAPI_TRACE_FULL;
			} else if (strcasecmp(s, "full") == 0) {
				fdef.flag = KCAPI_TRACE_FULL;
			} else if (strcasecmp(s, "shortnodata") == 0) {
				fdef.flag = KCAPI_TRACE_SHORT_NO_DATA;
			} else if (strcasecmp(s, "nodata") == 0) {
				fdef.flag = KCAPI_TRACE_FULL_NO_DATA;
			} else {
				usage();
				exit(1);
			}
		}

		ioctl_s.cmd = KCAPI_CMD_TRACE;
		ioctl_s.data = &fdef;
		if ((ioctl(fd, CAPI_MANUFACTURER_CMD, &ioctl_s)) < 0) {
			perror("\nioctl TRACE");
			exit(2);
		}
		close(fd);
		if (fdef.flag != KCAPI_TRACE_OFF)
		    printf("%s: trace switched on, look at the kernel messages, check dmesg(8)\n", cmd);
	        else printf("%s: trace switched off\n", cmd);
		return 0;
	}
#endif
#ifdef KCAPI_CMD_ADDCARD
	if (!strcasecmp(argv[arg_ofs], "addcard")) {
		kcapi_carddef carddef;
		char *s = 0;
	        int port;
		int irq;
		int cardnr = 0;
		unsigned long membase = 0;

                memset(&carddef, 0, sizeof(carddef));

		if (ac >= 5) {
			s = argv[arg_ofs + 1];
			if (strlen(s) > sizeof(carddef.driver)) {
				fprintf(stderr, "%s: driver name > %lu\n",
						cmd, (unsigned long)sizeof(carddef.driver));
				exit(1);
			}
			strncpy(carddef.driver, s, sizeof(carddef.driver));

			if (sscanf(argv[arg_ofs + 2], "%i", &port) != 1) {
				fprintf(stderr, "%s: invalid port \"%s\"\n",
						cmd, argv[arg_ofs + 2]);
				exit(1);
			}
			carddef.port = port;

			if (sscanf(argv[arg_ofs + 3], "%i", &irq) != 1) {
				fprintf(stderr, "%s: invalid irq \"%s\"\n",
						cmd, argv[arg_ofs + 3]);
				exit(1);
			}
			carddef.irq = irq;

			if (argv[arg_ofs + 4]) {
			   if (sscanf(argv[arg_ofs + 4], "%li", &membase) != 1) {
			   	fprintf(stderr, "%s: invalid membase \"%s\"\n",
						cmd, argv[arg_ofs + 4]);
				exit(1);
			   }
			   carddef.membase = membase;

			   if (argv[arg_ofs + 5]) {
			      if (sscanf(argv[arg_ofs + 5], "%i", &cardnr) != 1) {
			   	fprintf(stderr, "%s: invalid cardnr \"%s\"\n",
						cmd, argv[arg_ofs + 5]);
				exit(1);
			      }
			      carddef.cardnr = cardnr;
			   }
			}
		} else {
			fprintf(stderr, "%s: missing arguments\n", cmd);
			usage();
			exit(1);
		}

		ioctl_s.cmd = KCAPI_CMD_ADDCARD;
		ioctl_s.data = &carddef;
		if ((ioctl(fd, CAPI_MANUFACTURER_CMD, &ioctl_s)) < 0) {
			perror("\nioctl ADDCARD");
			exit(2);
		}
		close(fd);
		return 0;
	}
#endif
	usage();
	return 0;
}
