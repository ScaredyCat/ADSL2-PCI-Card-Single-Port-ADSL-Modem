/*
 * $Id: capiinit.c,v 1.9 2002/05/23 12:52:36 calle Exp $
 *
 * $Log: capiinit.c,v $
 * Revision 1.9  2002/05/23 12:52:36  calle
 * - Uaah. Bugfix for c2 patchvalues.
 *
 * Revision 1.8  2001/04/18 10:21:42  calle
 * support for "AVM ISDN Controller C2" added.
 *
 * Revision 1.7  2000/08/31 08:23:35  calle
 * - produce an error message, if a driver could not be loaded.
 *
 * Revision 1.6  2000/07/24 14:15:10  calle
 * Bugfix: pci controllers were always by initialized first in multi
 *         controller environment.
 *
 * Revision 1.5  2000/07/24 08:38:04  calle
 * - Bugfix: devfs mount was never detected, because an extra / in path.
 *
 * Revision 1.4  2000/06/30 14:08:45  calle
 * - creat /dev/capi if not exist, and mount capifs if available and devfs
 *   not availabe or not mounted on /dev.
 * - better error messages
 *
 * Revision 1.3  2000/06/29 15:17:21  calle
 * Mount capifs on /dev/capi if available.
 *
 * Revision 1.2  2000/05/18 15:20:18  calle
 * Umount capifs on "stop".
 *
 * Revision 1.1  2000/03/17 16:19:43  calle
 * New command capiinit, this will replace avmcapictrl in the future, if
 * called as "capiinit start" ist will load all modules, add all cards
 * configured in "/etc/capi.conf" and load the firmware to all active
 * cards. When called as "capiinit stop", it will deinit all cards and
 * remove all modules. Sample config in capiinit/capi.conf.
 *
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <linux/isdn.h>
#include <linux/b1lli.h>
#include <linux/capi.h>
#include <linux/kernelcapi.h>
#include <getopt.h>

#define MODPROBE	"/sbin/modprobe"
static char capidevnameold[] = "/dev/capi20";
static char capidevnamenew[] = "/dev/isdn/capi20";
static char *capidevname = capidevnameold;

static char *firmwarepath[] = {
	"/usr/lib/isdn",
	"/lib/isdn",
	0
};

/* ---------------- global variables --------------------------------- */

static int capifd = -1;

static int patchdebug = 0;	/* -d || -debug */
char *configfilename = "/etc/capi.conf";

/* ---------------- utils -------------------------------------------- */

#define STRUCTALLOC(type)	(type *)malloc(sizeof(type))

static FILE *fopen_with_errmsg(const char *path, const char *mode)
{
	FILE *fp;
	if ((fp = fopen(path, mode)) == NULL) {
		int serrno = errno;
		fprintf(stderr, "ERROR: fopen(%s,%s) failed - %s (%d)\n",
			path, mode, strerror(serrno), serrno);
	}
	return fp;
}

static char *skip_whitespace(char *s)
{
   while (*s && isspace(*s)) s++;
   return s;
}

static char *skip_nonwhitespace(char *s)
{
   while (*s && !isspace(*s)) s++;
   return s;
}

/* ---------------- load module -------------------------------------- */

static int load_module(char *module)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s %s", MODPROBE, module);
	return system(buf);
}

static int unload_module(char *module)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s -r %s", MODPROBE, module);
	return system(buf);
}

/* ---------------- /proc/capi/controller ---------------------------- */

/*
1 b1pciv4    detected b1pciv4-e400     - - 0xe400 19 0xea003000 r4
2 c4         detected c4-d800          - - 0xd800 17 0xea001000
3 c4         detected c4-d800          - - 0xd800 17 0xea001000
4 c4         detected c4-d800          - - 0xd800 17 0xea001000
5 c4         detected c4-d800          - - 0xd800 17 0xea001000
*/

#define CARD_FREE	0
#define CARD_DETECTED	1
#define CARD_LOADING	2
#define CARD_RUNNING	3

struct cardstatemap {
	char *name;
	int state;
} cardstatemap[] = {
{ "free",	CARD_FREE },
{ "detected",	CARD_DETECTED },
{ "loading",	CARD_LOADING },
{ "running",	CARD_RUNNING },
{ 0 }
};

static char *cardstate2str(int state)
{
	struct cardstatemap *p;
	for (p = cardstatemap; p->name; p++) {
		if (state == p->state)
			return p->name;
	}
	return cardstatemap[0].name;
}

static int str2cardstate(char *s)
{
	struct cardstatemap *p;
	for (p = cardstatemap; p->name; p++) {
		if (strcmp(p->name, s) == 0)
			return p->state;
	}
	return cardstatemap[0].state;
}

struct contrprocinfo {
	struct contrprocinfo *next;
	int                   contr;
	char                  driver[32];
	int                   state;
	char                  name[32];
	char                  driverinfo[32];
};

static void free_contrprocinfo(struct contrprocinfo **pp)
{
	struct contrprocinfo *p;
	while (*pp) {
		p = *pp;
		*pp = p->next;
		free(p);
	}
}

static struct contrprocinfo *load_contrprocinfo(int *lastcontrp)
{
	static char *fn = "/proc/capi/controller";
	struct contrprocinfo *list, *p, **pp;
	char buf[4096];
	FILE *fp;
	int line = 0;

	if (lastcontrp)
		*lastcontrp = 0;
	list = 0;
	if ((fp = fopen_with_errmsg(fn, "r")) == NULL)
		return 0;
	while (fgets(buf,sizeof(buf),fp)) {
		char *s, *tmp, *target;
		line++;
		if ((p = STRUCTALLOC(struct contrprocinfo)) == 0) {
			fprintf(stderr, "ERROR: %s:%d: malloc failed\n", fn, line);
			goto error;
		}
		memset(p, 0, sizeof(*p));
		s = buf;
		/* contr */
		tmp = s;
		p->contr = strtol(s, &tmp, 10);
		if (tmp == s) goto parseerror;
		/* driver */
		target = skip_whitespace(tmp);
		s = skip_nonwhitespace(target);
		if (!*s) goto parseerror;
		*s++ = 0;
		snprintf(p->driver, sizeof(p->driver), "%s", target);
		/* state */
		target = skip_whitespace(s);
		s = skip_nonwhitespace(target);
		if (!*s) goto parseerror;
		*s++ = 0;
		p->state = str2cardstate(target);
		/* name */
		target = skip_whitespace(s);
		s = skip_nonwhitespace(target);
		if (!*s) goto parseerror;
		*s++ = 0;
		snprintf(p->name, sizeof(p->name), "%s", target);
		/* driverinfo */
		target = skip_whitespace(s);
		while (*s && *s != '\n') s++;
		if (*s) *s = 0;
		snprintf(p->driverinfo, sizeof(p->driverinfo), "%s", target);

		for (pp = &list; *pp; pp = &(*pp)->next) ;
		*pp = p;
		if (lastcontrp && *lastcontrp < p->contr)
			*lastcontrp = p->contr;
	}
	fclose(fp);
	return list;
parseerror:
	fprintf(stderr, "ERROR: %s:%d: parse error\n", fn, line);
error:
	fclose(fp);
	free_contrprocinfo(&list);
	return 0;
}

static struct contrprocinfo *
find_contrprocinfo(struct contrprocinfo *cpinfo, int contr)
{
	struct contrprocinfo *p;
	for (p = cpinfo; p; p = p->next)
		if (p->contr == contr)
			break;
	return p;
}

static void show_contrprocinfo(struct contrprocinfo *cpinfo)
{
	struct contrprocinfo *p;
	for (p = cpinfo; p; p = p->next) {
		printf("%d %-10s %-8s %-16s %s\n", 
			p->contr, p->driver, cardstate2str(p->state),
			p->name, p->driverinfo);
	}
}

/* ---------------- /proc/capi/driver -------------------------------- */

/*
b1pci                            0  1.20 
b1pciv4                          1 0.0
c4                               4  1.4 
*/

static int driver_loaded(char *driver)
{
	static char *fn = "/proc/capi/driver";
	char buf[4096];
	FILE *fp;

	if (strcmp(driver, "b1pciv4") == 0)
		driver = "b1pci";

	if ((fp = fopen_with_errmsg(fn, "r")) == NULL)
		return 0;
	while (fgets(buf,sizeof(buf),fp)) {
		char *s = buf;
		while (*s && !isspace(*s)) s++;
		*s = 0;
		if (strcmp(buf, driver) == 0) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

static int load_driver(char *driver)
{
	if (strcmp(driver, "b1pciv4") == 0)
		driver = "b1pci";
	if (strcmp(driver, "c2") == 0)
		driver = "c4";
	return load_module(driver);
}

static int unload_driver(char *driver)
{
	if (strcmp(driver, "b1pciv4") == 0)
		driver = "b1pci";
	if (strcmp(driver, "c2") == 0)
		driver = "c4";
	return unload_module(driver);
}

/* ---------------- /proc/filesystems -------------------------------- */

static int filesystem_available(char *fstype)
{
	static char *fn = "/proc/filesystems";
	char buf[4096];
	FILE *fp;

	if ((fp = fopen_with_errmsg(fn, "r")) == NULL)
		return 0;
	while (fgets(buf,sizeof(buf),fp)) {
		char *t, *s;
		buf[strlen(buf)-1] = 0;
		t = skip_whitespace(buf);
		s = skip_nonwhitespace(t);
		if (*s) {
			t = skip_whitespace(s);
			s = skip_nonwhitespace(t);
		}
		if (strcmp(t, fstype) == 0) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

static int load_filesystem(char *fstype)
{
	return load_module(fstype);
}

static int unload_filesystem(char *fstype)
{
	return unload_module(fstype);
}

/* ---------------- /proc/mounts ------------------------------------- */

/*
/dev/root / ext2 rw 0 0
proc /proc proc rw 0 0
/dev/hda1 /boot ext2 rw 0 0
/dev/hda7 /src ext2 rw 0 0
devpts /dev/pts devpts rw 0 0
*/

static char *mounted(char *fstype)
{
	static char *fn = "/proc/mounts";
	static char mpret[PATH_MAX];
	char buf[4096];
	FILE *fp;
	char *mp,*ftype;

	if ((fp = fopen_with_errmsg(fn, "r")) == NULL)
		return 0;
	while (fgets(buf,sizeof(buf),fp)) {
		char *t, *s;
		buf[strlen(buf)-1] = 0;
		s = skip_whitespace(buf);
		t = skip_nonwhitespace(s);
		mp = skip_whitespace(t);
		t = skip_nonwhitespace(mp);
		if (!*t) 
			continue;
		*t++ = 0;
		strncpy(mpret, mp, sizeof(mpret)-1);
		mpret[sizeof(mpret)-1] = 0;
		ftype = skip_whitespace(t);
		t = skip_nonwhitespace(ftype);
		if (!*t)
			continue;
		*t++ = 0;
		if (strcmp(ftype, fstype) == 0) {
			fclose(fp);
			return mpret;
		}
	}
	fclose(fp);
	return 0;
}

/* ---------------- /etc/capi.conf ----------------------------------- */

/*
# card		file	proto	io	irq	mem	cardnr	options
b1isa		b1.t4	DSS1	0x150	7	-	-	P2P
*/

#define DP_ERROR	-1
#define DP_NONE		0
#define DP_1TR6		0
#define DP_DSS1		0
#define DP_CT1		1
#define DP_VN3		2
#define DP_NI1		3 	/* need SPID,SPID2,DN,DN2 */
#define DP_AUSTEL	4
#define DP_5ESS		5 	/* need SPID,SPID2,DN,DN2 */

static struct pmap {
  char *name;
  int   protocol;
} pmap[] = {
  { "-", DP_NONE },
  { "none", DP_NONE },
  { "DSS1", DP_DSS1 },
  { "1TR6", DP_1TR6 },
  { "CT1", DP_CT1 },
  { "VN3", DP_VN3 },
  { "AUSTEL", DP_AUSTEL },
  { "5ESS", DP_5ESS },
  { "NI1", DP_NI1 },
  { 0 },
};

static int dchan_protocol(char *pname)
{
   struct pmap *p;
   for (p=pmap; p->name; p++) {
      if (strcasecmp(pname, p->name) == 0)
	 return p->protocol;
   }
   return DP_ERROR;
}

struct patchinfo {
	int protocol;
	int p2p;
	char *dn1;
	char *spid1;
	char *dn2;
	char *spid2;
};

struct capicard {
	struct capicard  *next;
	int               found;
	int               line;
	char             *driver;
	char             *firmware;
	char             *protoname;
	int		  proto;
	int		  ioaddr;
	int		  irq;
	unsigned long	  memaddr;
	int               cardnr;
	char             *optionstring;
	struct patchinfo  patchinfo;
};

static void free_config(struct capicard **pp)
{
	struct capicard *p;
	while (*pp) {
		p = *pp;
		*pp = p->next;
		if (p->driver) free(p->driver);
		if (p->firmware) free(p->firmware);
		if (p->protoname) free(p->protoname);
		if (p->optionstring) free(p->optionstring);
		if (p->patchinfo.dn1) free(p->patchinfo.dn1);
		if (p->patchinfo.spid1) free(p->patchinfo.spid1);
		if (p->patchinfo.dn2) free(p->patchinfo.dn2);
		if (p->patchinfo.spid2) free(p->patchinfo.spid2);
		free(p);
	}
}

static int parse_cardoptions(char *opts, struct patchinfo *infop)
{
	char buf[4096];
	char *s, *option, *p;
	char save;

	strncpy(buf, opts, sizeof(buf)-1);
	buf[sizeof(buf)-1] = 0;

	s = buf;
	while (*s) {
		s = skip_whitespace(s);
		option = s;
		s = skip_nonwhitespace(option);
		save = *s;
		*s = 0;
		if (*option == 0 || *option == '#')
			break;
		if (strcasecmp(option, "p2p") == 0) {
			infop->p2p = 1;
			*s = save;
			if (*s) s++;
			continue;
		}
		if ((p = strchr(option, ':')) != 0) {	/* DN:SPID */
			char *dn, *spid;
			*p = 0;
			dn = strdup(option);
			spid = strdup(p+1);
			*p = ':';
			if (infop->dn1 == 0) {
				infop->dn1 = dn;
				infop->spid1 = spid;
			} else {
				if (infop->dn2) free(infop->dn2);
				if (infop->spid2) free(infop->spid2);
				infop->dn2 = dn;
				infop->spid2 = spid;
			}
			*s = save;
			if (*s) s++;
			continue;
		}

		fprintf(stderr, "ERROR: cardoptions: unknown option \"%s\"\n", option);
		*s = save;
		return -1;
	}
	return 0;
}

struct capicard *load_config(char *fn)
{
	struct capicard *list, *p, **pp;
	char buf[4096];
	char *s, *t, *tmp;
	FILE *fp;
	int line = 0;

	list = 0;
	if ((fp = fopen_with_errmsg(fn, "r")) == NULL)
		return 0;
	while (fgets(buf,sizeof(buf),fp)) {
		line++;

		buf[strlen(buf)-1] = 0;
		if ((p = STRUCTALLOC(struct capicard)) == 0) {
			fprintf(stderr, "ERROR: %s:%d: malloc failed\n", fn, line);
			goto error;
		}
		memset(p, 0, sizeof(*p));
		p->line = line;
		s = skip_whitespace(buf);
		if (*s == 0 || *s == '#')
			continue;

		/* driver */
		t = s;
		s = skip_nonwhitespace(t);
		if (*s) *s++ = 0;
		p->driver = strdup(t);
		if (!p->driver) goto nomem;
		s = skip_whitespace(s);

		/* firmware */
		t = s;
		s = skip_nonwhitespace(t);
		if (*s) *s++ = 0;
		p->firmware = strdup(t);
		if (!p->firmware) goto nomem;
		s = skip_whitespace(s);

		/* proto */
		t = s;
		s = skip_nonwhitespace(t);
		if (*s) *s++ = 0;
		p->protoname = strdup(t);
		if (!p->protoname) goto nomem;
		p->proto = dchan_protocol(t);
		s = skip_whitespace(s);

		/* ioaddr */
		t = s;
		s = skip_nonwhitespace(t);
		if (*s) *s++ = 0;
		if (strcmp(t, "-") == 0 || strcmp(t, "none") == 0) {
		   	p->ioaddr = 0;
		} else {
	 		tmp = t;
			p->ioaddr = strtol(t, &tmp, 0);
			if (*tmp) {
				fprintf(stderr, "ERROR: %s:%d: illegal ioaddr \"%s\"\n",
					fn, line, t);
				goto error;
			}
		}
		s = skip_whitespace(s);

		/* irq */
		t = s;
		s = skip_nonwhitespace(t);
		if (*s) *s++ = 0;
		if (strcmp(t, "-") == 0 || strcmp(t, "none") == 0) {
		   	p->irq = 0;
		} else {
			tmp = t;
			p->irq = strtol(t, &tmp, 0);
			if (*tmp) {
				fprintf(stderr, "ERROR: %s:%d: illegal irq \"%s\"\n",
					fn, line, t);
				goto error;
			}
		}
		s = skip_whitespace(s);

		/* memaddr */
		t = s;
		s = skip_nonwhitespace(t);
		if (*s) *s++ = 0;
		if (strcmp(t, "-") == 0 || strcmp(t, "none") == 0) {
		   	p->memaddr = 0;
		} else {
			tmp = t;
			p->memaddr = strtoul(t, &tmp, 0);
			if (*tmp) {
				fprintf(stderr, "ERROR: %s:%d: illegal memaddr \"%s\"\n",
					fn, line, t);
				goto error;
			}
		}
		s = skip_whitespace(s);

		/* cardnr */
		t = s;
		s = skip_nonwhitespace(t);
		if (*s) *s++ = 0;
		if (strcmp(t, "-") == 0 || strcmp(t, "none") == 0) {
		   	p->cardnr = 0;
		} else {
			tmp = t;
			p->cardnr = strtol(t, &tmp, 0);
			if (*tmp) {
				fprintf(stderr, "ERROR: %s:%d: illegal cardnr \"%s\"\n",
					fn, line, t);
				goto error;
			}
		}
		s = skip_whitespace(s);

		/* options */
		p->optionstring = strdup(s);
		if (!p->optionstring) goto nomem;

		if (parse_cardoptions(p->optionstring, &p->patchinfo) < 0) {
			fprintf(stderr, "ERROR: %s:%d: illegal options \"%s\"\n",
					fn, line, p->optionstring);
			goto error;
		}

		/* append it to the list */
		for (pp = &list; *pp; pp = &(*pp)->next) ;
		*pp = p;
	}
	fclose(fp);
	return list;
nomem:
	fprintf(stderr, "ERROR: %s:%d: no memory\n", fn, line);
error:
	fclose(fp);
	free_config(&list);
	return 0;
}

static void mark_unfound(struct capicard *cards) 
{
	struct capicard *card = cards;
	for (card = cards; card; card = card->next)
		card->found = 0;
}

static struct capicard *find_config(struct capicard *cards, char *driver)
{
	struct capicard *card;

	for (card = cards; card; card = card->next) {
		if (card->found)
			continue;
		if (strcmp(card->driver,driver) == 0)
			break;
	}
	if (card) {
		card->found = 1;
		return card;
	}

	if (strcmp(driver, "b1pci") == 0) {
		for (card = cards; card; card = card->next) {
			if (card->found)
				continue;
			if (strcmp(card->driver, "b1pciv4") == 0)
				break;
		}
	} else if (strcmp(driver, "b1pciv4") == 0) {
		for (card = cards; card; card = card->next) {
			if (card->found)
				continue;
			if (strcmp(card->driver, "b1pci") == 0)
				break;
		}
	} else if (strcmp(driver, "c4") == 0) {
		for (card = cards; card; card = card->next) {
			if (card->found)
				continue;
			if (strcmp(card->driver, "c2") == 0)
				break;
		}
	} else if (strcmp(driver, "c2") == 0) {
		for (card = cards; card; card = card->next) {
			if (card->found)
				continue;
			if (strcmp(card->driver, "c4") == 0)
				break;
		}
	}

	if (card) card->found = 1;
	return card;
}

static void show_confighead(void)
{
	printf("%s\t%-12s\t%s", "driver", "firmware", "proto");
	printf("\tio");
	printf("\tirq");
	printf("\tmem");
	printf("\tcardnr");
	printf("\toptions");
	printf("\n");
}

static void show_configone(struct capicard *p)
{
	printf("%s\t%-12s\t%s", p->driver, p->firmware, p->protoname);
	if (p->ioaddr) printf("\t0x%x", p->ioaddr);
	else printf("\t-");
	if (p->irq) printf("\t%d", p->irq);
	else printf("\t-");
	if (p->memaddr) printf("\t0x%lx", p->memaddr);
	else printf("\t-");
	if (p->cardnr) printf("\t%d", p->cardnr);
	else printf("\t-");
	printf("\t%s", p->optionstring);
	printf("\n");
}

static void show_config(struct capicard *cards)
{
	struct capicard *p;
	show_confighead();
	for (p = cards; p; p = p->next)
		show_configone(p);
}

/* ---------------- add card ----------------------------------------- */

static int add_card(struct capicard *card)
{
	capi_manufacturer_cmd ioctl_s;
	kcapi_carddef carddef;

	memset(&carddef, 0, sizeof(carddef));
	strncpy(carddef.driver, card->driver, sizeof(carddef.driver)-1);
	carddef.port = card->ioaddr;
	carddef.irq = card->irq;
	carddef.irq = card->irq;
	carddef.membase = card->memaddr;
	carddef.cardnr = card->cardnr;
	ioctl_s.cmd = KCAPI_CMD_ADDCARD;
	ioctl_s.data = &carddef;
	if (ioctl(capifd, CAPI_MANUFACTURER_CMD, &ioctl_s) >= 0)
		return 0;
	fprintf(stderr, "ERROR: add_card(%s) failed - %s (%d)\n",
				card->driver, strerror(errno), errno);
	fprintf(stderr, "\n!! CHECK THE KERNEL MESSAGES BEFORE SENDING MAIL !!\n\n");
	return -1;
}

/* ---------------- patchvalues for AVM active cards ----------------- */

static char patcharea[8192];
static int  patchlen = 0;

static char *getpatchdata(void)
{
	return patcharea;
}

static int getpatchlen(void)
{
	return patchlen;
}

static void clearpatcharea(void)
{
	patchlen = 0;
}

static void addpatchvalue(char *name, char *value, int len)
{
   int nlen = strlen(name);
   if (patchlen + nlen + len + 2 >= sizeof(patcharea)) {
      fprintf(stderr, "ERROR: addpatchvalue: patcharea overflow (%s)\n" , name);
      return;
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

static void addpatchstring(char *name, char *value)
{
	addpatchvalue(name, value, strlen(value));
}

static void addpatchbyte(char *name, unsigned char value)
{
	char buf[16];
	sprintf(buf, "\\x%02x", value);
	addpatchvalue(name, buf, strlen(buf));
}

static void addpatchvalues(struct patchinfo *p)
{
	int p2p = p->p2p;
	addpatchstring("WATCHDOG", "1");
	addpatchbyte("AutoFrame", 1);
   	switch (p->protocol) {
      		case DP_CT1: 
         		addpatchbyte("PROTOCOL", 1);
	 		break;
      		case DP_VN3: 
         		addpatchbyte("PROTOCOL", 2);
	 		break;
      		case DP_NI1: 
	 		p2p = 0;
         		addpatchbyte("PROTOCOL", 3);
	 		if (p->dn1 && p->spid1) {
            			addpatchstring("DN", p->dn1);
            			addpatchstring("SPID", p->spid1);
	 		}
	 		if (p->dn2 && p->spid2) {
            			addpatchstring("DN2", p->dn2);
            			addpatchstring("SPID2", p->spid2);
	 		}
	 		break;
      		case DP_AUSTEL: 
         		addpatchbyte("PROTOCOL", 4);
	 		break;
      		case DP_5ESS: 
	 		p2p = 0;
         		addpatchbyte("PROTOCOL", 5);
	 		if (p->dn1 && p->spid1) {
            			addpatchstring("DN", p->dn1);
            			addpatchstring("SPID", p->spid1);
	 		}
	 		if (p->dn2 && p->spid2) {
            			addpatchstring("DN2", p->dn2);
            			addpatchstring("SPID2", p->spid2);
	 		}
	 		break;
   	}
   	if (p2p) {
		addpatchbyte("P2P", 1);
		addpatchbyte("TEI", 0);
   	}
}

static void savepatcharea(char *dir, int contr)
{
	char fn[PATH_MAX];
	int fd;
	snprintf(fn, sizeof(fn), "%s/capicontr%d.pvals", dir, contr);
	if (access(fn, 0)) unlink(fn);
	if ((fd = open(fn, O_WRONLY|O_CREAT, 0600)) < 0) {
		fprintf(stderr, "creation of %s failed - %s (%d)\n",
			fn, strerror(errno), errno);
		return;
	}
	if (write(fd, patcharea, patchlen) != patchlen) {
		fprintf(stderr, "write to %s failed - %s (%d)\n",
			fn, strerror(errno), errno);
		close(fd);
		return;
	}
	close(fd);
	printf("controller %d: patchvalues written to %s\n",
			contr, fn);
}

/* ---------------- firmware load ------------------------------------ */

static char *locate_firmware(struct capicard *card)
{
	static char fn[PATH_MAX];
	char **path;

	if (card->firmware[0] == '/') {
		snprintf(fn, sizeof(fn), "%s", card->firmware);
		return fn;
	}
	for (path = firmwarepath; *path; path++) {
		snprintf(fn, sizeof(fn), "%s/%s", path[0], card->firmware);
		if (access(fn, R_OK) == 0)
			return fn;
	}
	return 0;
}

static char signalinfo[PATH_MAX+64];

static void sigdummy(int sig)
{
	fprintf(stderr, "ERROR: %s\n", signalinfo);
}

static struct capicard *load_firmware(int contr, struct capicard *card)
{
	capi_manufacturer_cmd ioctl_s;
	avmb1_loadandconfigdef ldef;
	struct capicard *next = card->next;
	char *fn, *type;
	struct stat st;
	int codefd;

	clearpatcharea();
	if (strcmp(card->driver, "c4") == 0) {
		struct capicard *cp;
		int i;
		for (i=0,cp=card; i < 4; i++) {
			addpatchbyte("CtlrNr", i);
			addpatchvalues(&cp->patchinfo);
			if (cp->next && strcmp(cp->next->driver, "c4") == 0)
				cp = cp->next;
		}
		next = cp->next;
	} else if (strcmp(card->driver, "c2") == 0) {
		struct capicard *cp;
		int i;
		for (i=0,cp=card; i < 2; i++) {
			addpatchbyte("CtlrNr", i);
			addpatchvalues(&cp->patchinfo);
			if (cp->next && strcmp(cp->next->driver, "c2") == 0)
				cp = cp->next;
		}
		next = cp->next;
	} else {
		addpatchvalues(&card->patchinfo);
	}

	if ((fn = locate_firmware(card)) == 0) {
		fprintf(stderr, "ERROR: controller %d: firmware file \"%s\" not found\n",
				contr, card->firmware);
		return next;
	}
	if (stat(fn, &st)) {
		fprintf(stderr, "ERROR: controller %d: stat failed for firmware file %s - %s (%d)\n",
				contr, fn, strerror(errno), errno);
		return next;
	}
	switch (st.st_mode & S_IFMT) {
		case S_IFREG:  type = 0; break;
		case S_IFSOCK: type = "socket"; break;
		case S_IFLNK:  type = "symbolic link"; break;
		case S_IFBLK:  type = "block device"; break;
		case S_IFDIR:  type = "directory"; break;
		case S_IFCHR:  type = "character device"; break;
		case S_IFIFO:  type = "named pipe"; break;
		default:       type = "unknown file type"; break;
	}
	if (type != 0) {
		fprintf(stderr, "ERROR: controller %d: firmware file \"%s\" is a %s\n",
				contr, fn, type);
		return next;
	}

	if ((codefd = open(fn, O_RDONLY)) < 0) {
		fprintf(stderr, "ERROR: controller %d: failed to open firmware file \"%s\" - %s (%d)\n",
				contr, fn, strerror(errno), errno);
		return next;
	}

	ldef.contr = contr;
	ldef.t4file.len = st.st_size;
	ldef.t4file.data = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, codefd, 0);
	if (ldef.t4file.data == (unsigned char *) -1) {
		fprintf(stderr, "ERROR: controller %d: mmap of firmware failed - %s (%d)\n",
				contr, strerror(errno), errno);
		return next;
	}

	ldef.t4config.len = getpatchlen();
	ldef.t4config.data = getpatchdata();

	if (patchdebug)
		savepatcharea("/tmp", contr);

	ioctl_s.cmd = AVMB1_LOAD_AND_CONFIG;
	ioctl_s.data = &ldef;
	snprintf(signalinfo, sizeof(signalinfo), 
		"controller %d: timeout while loading \"%s\"", contr, fn);
	signal(SIGALRM, sigdummy);
	alarm(10);
	if (ioctl(capifd, CAPI_MANUFACTURER_CMD, &ioctl_s) < 0) {
		if (errno != EINTR) {
			fprintf(stderr, "ERROR: controller %d: firmware load failed - %s (%d)\n", contr, strerror(errno), errno);
			fprintf(stderr, "\n!! CHECK THE KERNEL MESSAGES BEFORE SENDING MAIL !!\n\n");
		}
		alarm(0);
		signal(SIGALRM, SIG_DFL);
		return next;
	}
	alarm(0);
	signal(SIGALRM, SIG_DFL);
	munmap(ldef.t4file.data, ldef.t4file.len);
	close(codefd);
	return next;
}

int reset_controller(int contr)
{
	capi_manufacturer_cmd ioctl_s;
	avmb1_resetdef rdef;

	rdef.contr = contr;
	ioctl_s.cmd = AVMB1_RESETCARD;
	ioctl_s.data = &rdef;
	if (ioctl(capifd, CAPI_MANUFACTURER_CMD, &ioctl_s) < 0)
		return -1;
	return 0;
}

int remove_controller(int contr)
{
	capi_manufacturer_cmd ioctl_s;
	avmb1_resetdef rdef;

	rdef.contr = contr;
	ioctl_s.cmd = AVMB1_REMOVECARD;
	ioctl_s.data = &rdef;
	if (ioctl(capifd, CAPI_MANUFACTURER_CMD, &ioctl_s) < 0)
		return -1;
	return 0;
}

/* ---------------- prechecks ---------------------------------------- */

static int check_superuser(void)
{
	if (getuid() != 0) {
		fprintf(stderr, "ERROR: you must be super user\n");
		return -1;
	}
	return 0;
}

static int check_procfs(void)
{
	if (access("/proc/self", 0) < 0) {
		fprintf(stderr, "ERROR: proc filesystem not mounted\n");
		return -1;
	}
	return 0;
}

static int check_for_kernelcapi(void)
{
	if (access("/proc/capi/users", 0) == 0)
		return 0;
	load_module("kernelcapi");
	if (access("/proc/capi/users", 0) == 0)
		return 0;
	fprintf(stderr, "ERROR: cannot load module kernelcapi\n");
	return -1;
}

static int check_for_capi(void)
{
	if (access("/proc/capi/capi20", 0) == 0)
		return 0;
	load_module("capi");
	if (access("/proc/capi/capi20", 0) == 0)
		return 0;
	fprintf(stderr, "ERROR: cannot load module capi20\n");
	return -1;
}

static int check_for_devcapi(void)
{
	int fd;
	capidevname = capidevnameold;
	if ((fd = open(capidevname, O_RDWR)) < 0 && errno == ENOENT) {
	      capidevname = capidevnamenew;
	      fd = open(capidevname, O_RDWR);
	}
	if (fd < 0) {
		fprintf(stderr, "ERROR: cannot open %s nor %s - %s (%d)\n",
			capidevnameold, capidevnamenew, strerror(errno), errno);
		return -1;
	}
	close(fd);
	return 0;
}

static int check_for_capifs(void)
{
	if (filesystem_available("capifs"))
		return 0;
	load_filesystem("capifs");
	if (filesystem_available("capifs")) 
		return 0;
	if (filesystem_available("devfs"))
		return 0;
	load_filesystem("capifs");
	if (filesystem_available("capifs")) 
		return 0;
	fprintf(stderr, "WARNING: filesystem capifs not available\n");
	return -1;
}

static int checkdir(char *dir)
{
	struct stat st;
	if (stat(dir, &st) < 0)
		return -1;
	if (S_ISDIR(st.st_mode))
		return 0;
	return -1;
}

static int check_for_capifs_mounted(void)
{
	char *mp;
	if (filesystem_available("devfs")) {
		if ((mp = mounted("devfs")) != 0 && strcmp(mp, "/dev") == 0)
			return 0;
	}
	if (filesystem_available("capifs")) {
		if ((mp = mounted("capifs")) != 0 && strcmp(mp, "/dev/capi") == 0)
			return 0;
		if (checkdir("/dev/capi") < 0) {
			unlink("/dev/capi");
			if (mkdir("/dev/capi", 0755) < 0) {
				fprintf(stderr, "ERROR: mkdir(/dev/capi) failed - %s (%d)\n",
					strerror(errno), errno);
				return -1;
			}
		}
		system("mount -t capifs -omode=0666 capifs /dev/capi");
		if ((mp = mounted("capifs")) != 0 && strcmp(mp, "/dev/capi") == 0)
			return 0;
		fprintf(stderr, "ERROR: cound't mount capifs on /dev/capi\n");
		return -1;
	}
	return -1;
}
static int prestartcheck(void)
{
	if (check_superuser() < 0) return -1;
	if (check_procfs() < 0) return -1;
	if (check_for_kernelcapi() < 0) return -1;
	if (check_for_capi() < 0) return -1;
	if (check_for_devcapi() < 0) return -1;
	if (check_for_capifs() < 0) return 0; /* only warning */
	if (check_for_capifs_mounted() < 0) return -1;
	return 0;
}

static int prestopcheck(void)
{
	if (check_superuser() < 0) return -1;
	if (check_procfs() < 0) return -1;
	if (check_for_kernelcapi() < 0) return -1;
	if (check_for_capi() < 0) return -1;
	if (check_for_devcapi() < 0) return -1;
	return 0;
}

/* ------------------------------------------------------------------- */

static int card_exists(const char * driver, int ioaddr)
{
	static char buf[64];
	struct contrprocinfo *cp, *cpinfo;

	snprintf (buf, sizeof (buf), "%s-%x", driver, ioaddr);
	buf[sizeof (buf) - 1] = (char) 0;
	for (cp = cpinfo = load_contrprocinfo(0); cp; cp = cp->next) {
		if (strcmp (cp->name, buf) == 0)
			break;
	}
	free_contrprocinfo (&cpinfo);
	return cp != 0;
}


int main_start(void)
{
	struct capicard *cards, *card;
	struct contrprocinfo *cpinfo, *p;
	int contr, lastcontr;
	int ret = 0;

	if (prestartcheck() < 0)
		return -1;

	/* could not fail, tested by check_for_capi() */
	capifd = open(capidevname, O_RDWR);

	cards = load_config(configfilename);
	for (card = cards; card; card = card->next) {
		if (!driver_loaded(card->driver))
			load_driver(card->driver);
                if (!driver_loaded(card->driver)) {
			fprintf(stderr,"ERROR: failed to load driver %s\n",
                                       card->driver);
			continue;
		}
		if (card->ioaddr && !card_exists(card->driver, card->ioaddr)) 
			add_card(card);
	}

	mark_unfound(cards); 
	cpinfo = load_contrprocinfo(&lastcontr);
	for (contr = 1; contr <= lastcontr; contr++) {
		struct capicard *thiscard;
		cpinfo = load_contrprocinfo(0);
		p = find_contrprocinfo(cpinfo, contr);
		thiscard = find_config(cards, p->driver);
		if (p->state ==	CARD_LOADING)
			reset_controller(contr);
		if (p->state == CARD_DETECTED) {
			if (thiscard) {
				card = load_firmware(contr, thiscard);
			} else {
				fprintf(stderr,"ERROR: missing config entry for controller %d driver %s name %s\n",
					p->contr, p->driver, p->name);
			}
		}
		free_contrprocinfo(&cpinfo);
	}
	mark_unfound(cards); 
	cpinfo = load_contrprocinfo(&lastcontr);
	for (contr = 1; contr <= lastcontr; contr++) {
		struct capicard *thiscard;
		cpinfo = load_contrprocinfo(0);
		p = find_contrprocinfo(cpinfo, contr);
		thiscard = find_config(cards, p->driver);
		if (p->state == CARD_DETECTED && thiscard) {
			fprintf(stderr,"ERROR: failed to load firmware for controller %d driver %s name %s\n",
					p->contr, p->driver, p->name);
			ret = 3;
		}
		free_contrprocinfo(&cpinfo);
	}

	cpinfo = load_contrprocinfo(0);
	show_contrprocinfo(cpinfo);

	free_contrprocinfo(&cpinfo);
	free_config(&cards);
	close(capifd);

	return ret;
}

/* ------------------------------------------------------------------- */

int main_stop(void)
{
	struct capicard *cards, *card;
	struct contrprocinfo *cpinfo, *p;
	int contr, lastcontr;
	char *mp;

	if (prestopcheck() < 0)
		return -1;

	/* could not fail, tested by check_for_capi() */
	capifd = open(capidevname, O_RDWR);

	cpinfo = load_contrprocinfo(&lastcontr);
	for (contr = lastcontr; contr > 0; contr--) {
		cpinfo = load_contrprocinfo(0);
		p = find_contrprocinfo(cpinfo, contr);
		if (p && p->state == CARD_RUNNING)
			reset_controller(contr);
		free_contrprocinfo(&cpinfo);
	}
	cpinfo = load_contrprocinfo(&lastcontr);
	for (contr = lastcontr; contr > 0; contr--) {
		cpinfo = load_contrprocinfo(0);
		p = find_contrprocinfo(cpinfo, contr);
		if (p)
			remove_controller(contr);
		free_contrprocinfo(&cpinfo);
	}

	cpinfo = load_contrprocinfo(0);
	show_contrprocinfo(cpinfo);
	free_contrprocinfo(&cpinfo);
	close(capifd);

	cards = load_config(configfilename);
	for (card = cards; card; card = card->next) {
		if (driver_loaded(card->driver))
			unload_driver(card->driver);
	}
	for (card = cards; card; card = card->next) {
		if (driver_loaded(card->driver))
			unload_driver(card->driver);
	}

	unload_module("capi");
	unload_module("capidrv");
	unload_module("kernelcapi");
	unload_module("capiutil");
	if ((mp = mounted("capifs")) != 0 && strcmp(mp, "/dev/capi") == 0)
		system("umount /dev/capi");
	if (filesystem_available("capifs"))
		unload_filesystem("capifs");
	return 0;
}

int main_show(void)
{
	struct capicard *cards;
	cards = load_config(configfilename);
	if (cards == 0) {
		fprintf(stderr,"ERROR: no cards configured in %s\n",
				configfilename);
		return -1;
	}
	show_config(cards);
	return 0;
}

/* ------------------------------------------------------------------- */

static void usage(void)
{
    fprintf(stderr, "Usage: capiinit [OPTION]\n");
    fprintf(stderr, "   or: capiinit [OPTION] start\n");
    fprintf(stderr, "   or: capiinit [OPTION] stop\n");
    fprintf(stderr, "   or: capiinit [OPTION] show\n");
    fprintf(stderr, "Setup or unsetup CAPI2.0 Controllers\n");
    fprintf(stderr, "   -c, --config filename  (default %s)\n", configfilename);
    fprintf(stderr, "   -d, --debug            save patchvalues for debugging\n");
}

int main(int ac, char *av[])
{
         int c;

         for (;;) {
		int option_index = 0;
		static struct option long_options[] = {
			{"config", 1, 0, 'c'},
			{"debug", 1, 0, 'd'},
			{0, 0, 0, 0}
		};

		c = getopt_long (ac, av, "c:d",
				long_options, &option_index);
             	if (c == -1)
            		break;

		switch (c) {
			case 0:
 				printf ("option %s",
					long_options[option_index].name);
 				if (optarg)
   					printf (" with arg %s", optarg);
 				printf ("\n");
 				break;

			case 'c':
				configfilename = optarg;
				break;
			case 'd':
 				patchdebug = 1;
 				break;
			case '?':
				usage();
				return 1;
		}
	}

 	if (optind == ac) {
		return main_start();
	} else if (optind+1 == ac) {
		if (strcmp(av[optind], "start") == 0)
			return main_start();
		else if (strcmp(av[optind], "stop") == 0)
			return main_stop();
		else if (strcmp(av[optind], "show") == 0)
			return main_show();
	}
	usage();
	return 1;
}
