/*-
 * Copyright (c) 2005 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 */

/*
 * wlanconfig wlanX create wlandev wifiX
 *	wlanmode station | adhoc | ibss | ap | monitor [bssid | -bssid]
 * wlanconfig wlanX destroy
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>

#include <stdio.h>
#include <ctype.h>
#include <getopt.h>

/*
 * Linux uses __BIG_ENDIAN and __LITTLE_ENDIAN while BSD uses _foo
 * and an explicit _BYTE_ORDER.  Sorry, BSD got there first--define
 * things in the BSD way...
 */
#define	_LITTLE_ENDIAN	1234	/* LSB first: i386, vax */
#define	_BIG_ENDIAN	4321	/* MSB first: 68000, ibm, net */
#include <asm/byteorder.h>
#if defined(__LITTLE_ENDIAN)
#define	_BYTE_ORDER	_LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN)
#define	_BYTE_ORDER	_BIG_ENDIAN
#else
#error "Please fix asm/byteorder.h"
#endif

#include "net80211/ieee80211.h"
#include "net80211/ieee80211_crypto.h"
#include "net80211/ieee80211_ioctl.h"

/*
 * These are taken from ieee80211_node.h
 */

#define IEEE80211_NODE_TURBOP	0x0001		/* Turbo prime enable */
#define IEEE80211_NODE_COMP	0x0002		/* Compresssion enable */
#define IEEE80211_NODE_FF	0x0004          /* Fast Frame capable */
#define IEEE80211_NODE_XR	0x0008		/* Atheros WME enable */
#define IEEE80211_NODE_AR	0x0010		/* AR capable */
#define IEEE80211_NODE_BOOST	0x0080 

#define	streq(a,b)	(strncasecmp(a,b,sizeof(b)-1) == 0)

static void vap_create(struct ifreq *);
static void vap_destroy(const char *ifname);
static void list_stations(const char *ifname);
static void list_scan(const char *ifname);
static void list_channels(const char *ifname, int allchans);
static void list_keys(const char *ifname);
static void list_capabilities(const char *ifname);
static void list_wme(const char *ifname);
static void ieee80211_status(const char *ifname);

static void usage(void);
static int getopmode(const char *);
static int getflag(const char *);
static int get80211param(const char *ifname, int param, void * data, size_t len);
static int set80211priv(const char *ifname, int op, void *data, size_t len);
static int get80211priv(const char *ifname, int op, void *data, size_t len);

size_t strlcat(char *dst, const char *src, size_t siz);

int	verbose = 0;

int
main(int argc, char *argv[])
{
	const char *ifname, *cmd;

	if (argc < 3)
		usage();

	ifname = argv[1];
	cmd = argv[2];
	if (streq(cmd, "create")) {
		struct ieee80211_clone_params cp;
		struct ifreq ifr;

		memset(&ifr, 0, sizeof(ifr));

		memset(&cp, 0, sizeof(cp));
		strncpy(cp.icp_name, ifname, IFNAMSIZ);
		/* NB: station mode is the default */
		cp.icp_opmode = IEEE80211_M_STA;
		/* NB: default is to request a unique bssid/mac */
		cp.icp_flags = IEEE80211_CLONE_BSSID;

		while (argc > 3) {
			if (strcmp(argv[3], "wlanmode") == 0) {
				if (argc < 5)
					usage();
				cp.icp_opmode = (u_int16_t) getopmode(argv[4]);
				argc--, argv++;
			} else if (strcmp(argv[3], "wlandev") == 0) {
				if (argc < 5)
					usage();
				strncpy(ifr.ifr_name, argv[4], IFNAMSIZ);
				argc--, argv++;
			} else {
				int flag = getflag(argv[3]);
				if (flag < 0)
					cp.icp_flags &= ~(-flag);
				else
					cp.icp_flags |= flag;
			}
			argc--, argv++;
		}
		if (ifr.ifr_name[0] == '\0')
			errx(1, "no device specified with wlandev");
		ifr.ifr_data = (void *) &cp;
		vap_create(&ifr);
	} else if (streq(cmd, "destroy")) {
		vap_destroy(ifname);
	} else if (streq(cmd, "list")) {
		if (argc > 3) {
			const char *arg = argv[3];

			if (streq(arg, "sta"))
				list_stations(ifname);
			else if (streq(arg, "scan") || streq(arg, "ap"))
				list_scan(ifname);
			else if (streq(arg, "chan") || streq(arg, "freq"))
				list_channels(ifname, 1);
			else if (streq(arg, "active"))
				list_channels(ifname, 0);
			else if (streq(arg, "keys"))
				list_keys(ifname);
			else if (streq(arg, "caps"))
				list_capabilities(ifname);
			else if (streq(arg, "wme"))
				list_wme(ifname);
		} else				/* NB: for compatibility */
			list_stations(ifname);
	} else
		ieee80211_status(ifname);

	return 0;
}

static void
vap_create(struct ifreq *ifr)
{
	char oname[IFNAMSIZ];
	int s;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(1, "socket(SOCK_DRAGM)");
	strncpy(oname, ifr->ifr_name, IFNAMSIZ);
	if (ioctl(s, SIOC80211IFCREATE, ifr) < 0)
		err(1, "ioctl");
	/* NB: print name of clone device when generated */
	if (memcmp(oname, ifr->ifr_name, IFNAMSIZ) != 0)
		printf("%s\n", ifr->ifr_name);
}

static void
vap_destroy(const char *ifname)
{
	struct ifreq ifr;
	int s;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(1, "socket(SOCK_DRAGM)");
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(s, SIOC80211IFDESTROY, &ifr) < 0)
		err(1, "ioctl");
}

static void
usage(void)
{
	fprintf(stderr, "usage: wlanconfig wlanX create wlandev wifiX\n");
	fprintf(stderr, "            wlanmode [sta|adhoc|ap|monitor] [bssid | -bssid] [nosbeacon]\n");
	fprintf(stderr, "usage: wlanconfig wlanX destroy\n");
	exit(-1);
}

static int
getopmode(const char *s)
{
	if (streq(s, "sta"))
		return IEEE80211_M_STA;
	if (streq(s, "ibss") || streq(s, "adhoc"))
		return IEEE80211_M_IBSS;
	if (streq(s, "mon"))
		return IEEE80211_M_MONITOR;
	if (streq(s, "ap") || streq(s, "hostap"))
		return IEEE80211_M_HOSTAP;
	if (streq(s, "wds"))
		return IEEE80211_M_WDS;
	errx(1, "unknown operating mode %s", s);
	/*NOTREACHED*/
	return -1;
}

static int
getflag(const char  *s)
{
	const char *cp;
	int flag = 0;

	cp = (s[0] == '-' ? s+1 : s);
	if (strcmp(cp, "bssid") == 0)
		flag = IEEE80211_CLONE_BSSID;
	if (strcmp(cp, "nosbeacon") == 0)
		flag |= IEEE80211_NO_STABEACONS;
	if (flag == 0)
		errx(1, "unknown create option %s", s);
	return (s[0] == '-' ? -flag : flag);
}

/*
 * Convert IEEE channel number to MHz frequency.
 */
static u_int
ieee80211_ieee2mhz(u_int chan)
{
	if (chan == 14)
		return 2484;
	if (chan < 14)			/* 0-13 */
		return 2407 + chan*5;
	if (chan < 27)			/* 15-26 */
		return 2512 + ((chan-15)*20);
	return 5000 + (chan*5);
}

/*
 * Convert MHz frequency to IEEE channel number.
 */
static u_int
ieee80211_mhz2ieee(u_int freq)
{
	if (freq == 2484)
		return 14;
	if (freq < 2484)
		return (freq - 2407) / 5;
	if (freq < 5000)
		return 15 + ((freq - 2512) / 20);
	return (freq - 5000) / 5;
}

typedef u_int8_t uint8_t;

static int
getmaxrate(uint8_t rates[15], uint8_t nrates)
{
	int i, maxrate = -1;

	for (i = 0; i < nrates; i++) {
		int rate = rates[i] & IEEE80211_RATE_VAL;
		if (rate > maxrate)
			maxrate = rate;
	}
	return maxrate / 2;
}

static const char *
getcaps(int capinfo)
{
	static char capstring[32];
	char *cp = capstring;

	if (capinfo & IEEE80211_CAPINFO_ESS)
		*cp++ = 'E';
	if (capinfo & IEEE80211_CAPINFO_IBSS)
		*cp++ = 'I';
	if (capinfo & IEEE80211_CAPINFO_CF_POLLABLE)
		*cp++ = 'c';
	if (capinfo & IEEE80211_CAPINFO_CF_POLLREQ)
		*cp++ = 'C';
	if (capinfo & IEEE80211_CAPINFO_PRIVACY)
		*cp++ = 'P';
	if (capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE)
		*cp++ = 'S';
	if (capinfo & IEEE80211_CAPINFO_PBCC)
		*cp++ = 'B';
	if (capinfo & IEEE80211_CAPINFO_CHNL_AGILITY)
		*cp++ = 'A';
	if (capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME)
		*cp++ = 's';
	if (capinfo & IEEE80211_CAPINFO_RSN)
		*cp++ = 'R';
	if (capinfo & IEEE80211_CAPINFO_DSSSOFDM)
		*cp++ = 'D';
	*cp = '\0';
	return capstring;
}

static const char *
getathcaps(int capinfo)
{
	static char capstring[32];
	char *cp = capstring;

	if (capinfo & IEEE80211_NODE_TURBOP)
		*cp++ = 'D';
	if (capinfo & IEEE80211_NODE_COMP)
		*cp++ = 'C';
	if (capinfo & IEEE80211_NODE_FF)
		*cp++ = 'F';
	if (capinfo & IEEE80211_NODE_XR)
		*cp++ = 'X';
	if (capinfo & IEEE80211_NODE_AR)
		*cp++ = 'A';
	if (capinfo & IEEE80211_NODE_BOOST)
		*cp++ = 'T';
	*cp = '\0';
	return capstring;
}

static const char *
gethtcaps(int capinfo)
{
	static char capstring[32];
	char *cp = capstring;

	if (capinfo & IEEE80211_HTCAP_C_ADVCODING)
		*cp++ = 'A';
	if (capinfo & IEEE80211_HTCAP_C_CHWIDTH40)
		*cp++ = 'W';
	if ((capinfo & IEEE80211_HTCAP_C_MIMOPWRSAVE_MASK) == 
             IEEE80211_HTCAP_C_MIMOPWRSAVE_ALL)
		*cp++ = 'P';
	if ((capinfo & IEEE80211_HTCAP_C_MIMOPWRSAVE_MASK) == 
             IEEE80211_HTCAP_C_MIMOPWRSAVE_DYNAMIC)
		*cp++ = 'R';
	if (capinfo & IEEE80211_HTCAP_C_GREENFIELD)
		*cp++ = 'G';
	if (capinfo & IEEE80211_HTCAP_C_SHORTGI40)
		*cp++ = 'S';
	if (capinfo & IEEE80211_HTCAP_C_DELAYEDBLKACK)
		*cp++ = 'D';
	if (capinfo & IEEE80211_HTCAP_C_MAXAMSDUSIZE)
		*cp++ = 'M';
	*cp = '\0';
	return capstring;
}

static void
printie(const char* tag, const uint8_t *ie, size_t ielen, int maxlen)
{
	printf("%s", tag);
	if (verbose) {
		maxlen -= strlen(tag)+2;
		if (2*ielen > maxlen)
			maxlen--;
		printf("<");
		for (; ielen > 0; ie++, ielen--) {
			if (maxlen-- <= 0)
				break;
			printf("%02x", *ie);
		}
		if (ielen != 0)
			printf("-");
		printf(">");
	}
}

/*
 * Copy the ssid string contents into buf, truncating to fit.  If the
 * ssid is entirely printable then just copy intact.  Otherwise convert
 * to hexadecimal.  If the result is truncated then replace the last
 * three characters with "...".
 */
static size_t
copy_essid(char buf[], size_t bufsize, const u_int8_t *essid, size_t essid_len)
{
	const u_int8_t *p; 
	size_t maxlen;
	int i;

	if (essid_len > bufsize)
		maxlen = bufsize;
	else
		maxlen = essid_len;
	/* determine printable or not */
	for (i = 0, p = essid; i < maxlen; i++, p++) {
		if (*p < ' ' || *p > 0x7e)
			break;
	}
	if (i != maxlen) {		/* not printable, print as hex */
		if (bufsize < 3)
			return 0;
#if 0
		strlcpy(buf, "0x", bufsize);
#else
		strncpy(buf, "0x", bufsize);
#endif
		bufsize -= 2;
		p = essid;
		for (i = 0; i < maxlen && bufsize >= 2; i++) {
			sprintf(&buf[2+2*i], "%02x", *p++);
			bufsize -= 2;
		}
		maxlen = 2+2*i;
	} else {			/* printable, truncate as needed */
		memcpy(buf, essid, maxlen);
	}
	if (maxlen != essid_len)
		memcpy(buf+maxlen-3, "...", 3);
	return maxlen;
}

/* unalligned little endian access */     
#define LE_READ_4(p)					\
	((u_int32_t)					\
	 ((((const u_int8_t *)(p))[0]      ) |		\
	  (((const u_int8_t *)(p))[1] <<  8) |		\
	  (((const u_int8_t *)(p))[2] << 16) |		\
	  (((const u_int8_t *)(p))[3] << 24)))

static int __inline
iswpaoui(const u_int8_t *frm)
{
	return frm[1] > 3 && LE_READ_4(frm+2) == ((WPA_OUI_TYPE<<24)|WPA_OUI);
}

static int __inline
iswmeoui(const u_int8_t *frm)
{
	return frm[1] > 3 && LE_READ_4(frm+2) == ((WME_OUI_TYPE<<24)|WME_OUI);
}

static int __inline
isatherosoui(const u_int8_t *frm)
{
	return frm[1] > 3 && LE_READ_4(frm+2) == ((ATH_OUI_TYPE<<24)|ATH_OUI);
}

static void
printies(const u_int8_t *vp, int ielen, int maxcols)
{
	while (ielen > 0) {
		switch (vp[0]) {
		case IEEE80211_ELEMID_VENDOR:
			if (iswpaoui(vp))
				printie(" WPA", vp, 2+vp[1], maxcols);
			else if (iswmeoui(vp))
				printie(" WME", vp, 2+vp[1], maxcols);
			else if (isatherosoui(vp))
				printie(" ATH", vp, 2+vp[1], maxcols);
			else
				printie(" VEN", vp, 2+vp[1], maxcols);
			break;
		case IEEE80211_ELEMID_RSN:
			printie(" RSN", vp, 2+vp[1], maxcols);
			break;
		default:
			printie(" ???", vp, 2+vp[1], maxcols);
			break;
		}
		ielen -= 2+vp[1];
		vp += 2+vp[1];
	}
}

static const char *
ieee80211_ntoa(const uint8_t mac[IEEE80211_ADDR_LEN])
{
	static char a[18];
	int i;

	i = snprintf(a, sizeof(a), "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return (i < 17 ? NULL : a);
}

static void
list_stations(const char *ifname)
{
	uint8_t buf[24*1024];
	struct iwreq iwr;
	uint8_t *cp;
	int s, len;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(1, "socket(SOCK_DRAGM)");

	(void) memset(&iwr, 0, sizeof(iwr));
	(void) strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));
	iwr.u.data.pointer = (void *) buf;
	iwr.u.data.length = sizeof(buf);
	if (ioctl(s, IEEE80211_IOCTL_STA_INFO, &iwr) < 0)
		errx(1, "unable to get station information");
	len = iwr.u.data.length;
	if (len < sizeof(struct ieee80211req_sta_info))
		return;

	printf("%-17.17s %4s %4s %4s %4s %4s %6s %6s %4s %5s %3s %8s %6s\n"
		, "ADDR"
		, "AID"
		, "CHAN"
		, "RATE"
		, "RSSI"
		, "IDLE"
		, "TXSEQ"
		, "RXSEQ"
		, "CAPS"
	        , "ACAPS"
		, "ERP"
		, "STATE"
	        , "HTCAPS"
	);
	cp = buf;
	do {
		struct ieee80211req_sta_info *si;
		uint8_t *vp;

		si = (struct ieee80211req_sta_info *) cp;
		vp = (u_int8_t *)(si+1);
		printf("%s %4u %4d %3dM %4d %4d %6d %6d %-4.4s %-5.5s %3x %8x %-6.6s"
			, ieee80211_ntoa(si->isi_macaddr)
			, IEEE80211_AID(si->isi_associd)
			, ieee80211_mhz2ieee(si->isi_freq)
			, (si->isi_rates[si->isi_txrate] & IEEE80211_RATE_VAL)/2
			, si->isi_rssi
			, si->isi_inact
			, si->isi_txseqs[0]
			, si->isi_rxseqs[0]
		        , getcaps(si->isi_capinfo)
		        , getathcaps(si->isi_athflags)
			, si->isi_erp
			, si->isi_state
		        , gethtcaps(si->isi_htcap)
		);
		printies(vp, si->isi_ie_len, 24);
		printf("\n");
		cp += si->isi_len, len -= si->isi_len;
	} while (len >= sizeof(struct ieee80211req_sta_info));
}

/* unalligned little endian access */     
#define LE_READ_4(p)					\
	((u_int32_t)					\
	 ((((const u_int8_t *)(p))[0]      ) |		\
	  (((const u_int8_t *)(p))[1] <<  8) |		\
	  (((const u_int8_t *)(p))[2] << 16) |		\
	  (((const u_int8_t *)(p))[3] << 24)))

static void
list_scan(const char *ifname)
{
	uint8_t buf[24*1024];
	struct iwreq iwr;
	char ssid[14];
	uint8_t *cp;
	int len;

	len = get80211priv(ifname, IEEE80211_IOCTL_SCAN_RESULTS,
			    buf, sizeof(buf));
	if (len == -1)
		errx(1, "unable to get scan results");
	if (len < sizeof(struct ieee80211req_scan_result))
		return;

	printf("%-14.14s  %-17.17s  %4s %4s  %-5s %3s %4s\n"
		, "SSID"
		, "BSSID"
		, "CHAN"
		, "RATE"
		, "S:N"
		, "INT"
		, "CAPS"
	);
	cp = buf;
	do {
		struct ieee80211req_scan_result *sr;
		uint8_t *vp;

		sr = (struct ieee80211req_scan_result *) cp;
		vp = (u_int8_t *)(sr+1);
		printf("%-14.*s  %s  %3d  %3dM %2d:%-2d  %3d %-4.4s"
			, copy_essid(ssid, sizeof(ssid), vp, sr->isr_ssid_len)
				, ssid
			, ieee80211_ntoa(sr->isr_bssid)
			, ieee80211_mhz2ieee(sr->isr_freq)
			, getmaxrate(sr->isr_rates, sr->isr_nrates)
			, (int8_t) sr->isr_rssi, sr->isr_noise
			, sr->isr_intval
			, getcaps(sr->isr_capinfo)
		);
		printies(vp + sr->isr_ssid_len, sr->isr_ie_len, 24);;
		printf("\n");
		cp += sr->isr_len, len -= sr->isr_len;
	} while (len >= sizeof(struct ieee80211req_scan_result));
}

static void
print_chaninfo(const struct ieee80211_channel *c)
{
#define	IEEE80211_IS_CHAN_PASSIVE(_c) \
	(((_c)->ic_flags & IEEE80211_CHAN_PASSIVE))
	char buf[14];

	buf[0] = '\0';
	if (IEEE80211_IS_CHAN_FHSS(c))
		strlcat(buf, " FHSS", sizeof(buf));
	if (IEEE80211_IS_CHAN_A(c))
		strlcat(buf, " 11a", sizeof(buf));
    if (IEEE80211_IS_CHAN_11NA(c))
        strlcat(buf, " 11na", sizeof(buf));
    if (IEEE80211_IS_CHAN_11NG(c))
        strlcat(buf, " 11ng", sizeof(buf));
	/* XXX 11g schizophrenia */
	if (IEEE80211_IS_CHAN_G(c) ||
	    IEEE80211_IS_CHAN_PUREG(c))
		strlcat(buf, " 11g", sizeof(buf));
	else if (IEEE80211_IS_CHAN_B(c))
		strlcat(buf, " 11b", sizeof(buf));
	if (IEEE80211_IS_CHAN_TURBO(c))
		strlcat(buf, " Turbo", sizeof(buf));
    if(IEEE80211_IS_CHAN_11N_CTL_CAPABLE(c))
        strlcat(buf, " C", sizeof(buf));
    if(IEEE80211_IS_CHAN_11N_CTL_U_CAPABLE(c))
        strlcat(buf, " CU", sizeof(buf));
    if(IEEE80211_IS_CHAN_11N_CTL_L_CAPABLE(c))
        strlcat(buf, " CL", sizeof(buf));
	printf("Channel %3u : %u%c Mhz%-14.14s",
		ieee80211_mhz2ieee(c->ic_freq), c->ic_freq,
		IEEE80211_IS_CHAN_PASSIVE(c) ? '*' : ' ', buf);
#undef IEEE80211_IS_CHAN_PASSIVE
}

static void
list_channels(const char *ifname, int allchans)
{
	struct ieee80211req_chaninfo chans;
	struct ieee80211req_chaninfo achans;
	const struct ieee80211_channel *c;
	int i, half;

	if (get80211priv(ifname, IEEE80211_IOCTL_GETCHANINFO, &chans, sizeof(chans)) < 0)
		errx(1, "unable to get channel information");
	if (!allchans) {
		struct ieee80211req_chanlist active;

		if (get80211priv(ifname, IEEE80211_IOCTL_GETCHANLIST, &active, sizeof(active)) < 0)
			errx(1, "unable to get active channel list");
		memset(&achans, 0, sizeof(achans));
		for (i = 0; i < chans.ic_nchans; i++) {
			c = &chans.ic_chans[i];
			if (isset(active.ic_channels, ieee80211_mhz2ieee(c->ic_freq)) || allchans)
				achans.ic_chans[achans.ic_nchans++] = *c;
		}
	} else
		achans = chans;
	half = achans.ic_nchans / 2;
	if (achans.ic_nchans % 2)
		half++;
	for (i = 0; i < achans.ic_nchans / 2; i++) {
		print_chaninfo(&achans.ic_chans[i]);
		print_chaninfo(&achans.ic_chans[half+i]);
		printf("\n");
	}
	if (achans.ic_nchans % 2) {
		print_chaninfo(&achans.ic_chans[i]);
		printf("\n");
	}
}

static void
list_keys(const char *ifname)
{
}

#define	IEEE80211_C_BITS \
"\020\1WEP\2TKIP\3AES\4AES_CCM\6CKIP\7FF\10TURBOP\11IBSS\12PMGT\13HOSTAP\14AHDEMO" \
"\15SWRETRY\16TXPMGT\17SHSLOT\20SHPREAMBLE\21MONITOR\22TKIPMIC\30WPA1" \
"\31WPA2\32BURST\33WME"

/*
 * Print a value a la the %b format of the kernel's printf
 */
void
printb(const char *s, unsigned v, const char *bits)
{
	int i, any = 0;
	char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (bits) {
		putchar('<');
		while ((i = *bits++) != '\0') {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

static void
list_capabilities(const char *ifname)
{
	u_int32_t caps;

	if (get80211param(ifname, IEEE80211_PARAM_DRIVER_CAPS, &caps, sizeof(caps)) < 0)
		errx(1, "unable to get driver capabilities");
	printb(ifname, caps, IEEE80211_C_BITS);
	putchar('\n');
}

static void
list_wme(const char *ifname)
{
#define	GETPARAM() \
	(get80211priv(ifname, IEEE80211_IOCTL_GETWMMPARAMS, param, sizeof(param)) != -1)
	static const char *acnames[] = { "AC_BE", "AC_BK", "AC_VI", "AC_VO" };
	int param[3];
	int ac;

	param[2] = 0;		/* channel params */
	for (ac = WME_AC_BE; ac <= WME_AC_VO; ac++) {
again:
		if (param[2] != 0)
			printf("\t%s", "     ");
		else
			printf("\t%s", acnames[ac]);

		param[1] = ac;

		/* show WME BSS parameters */
		param[0] = IEEE80211_WMMPARAMS_CWMIN;
		if (GETPARAM())
			printf(" cwmin %2u", param[0]);
		param[0] = IEEE80211_WMMPARAMS_CWMAX;
		if (GETPARAM())
			printf(" cwmax %2u", param[0]);
		param[0] = IEEE80211_WMMPARAMS_AIFS;
		if (GETPARAM())
			printf(" aifs %2u", param[0]);
		param[0] = IEEE80211_WMMPARAMS_TXOPLIMIT;
		if (GETPARAM())
			printf(" txopLimit %3u", param[0]);
		param[0] = IEEE80211_WMMPARAMS_ACM;
		if (GETPARAM()) {
			if (param[0])
				printf(" acm");
			else if (verbose)
				printf(" -acm");
		}
		/* !BSS only */
		if (param[2] == 0) {
			param[0] = IEEE80211_WMMPARAMS_NOACKPOLICY;
			if (GETPARAM()) {
				if (param[0])
					printf(" -ack");
				else if (verbose)
					printf(" ack");
			}
		}
		printf("\n");
		if (param[2] == 0) {
			param[2] = 1;		/* bss params */
			goto again;
		} else
			param[2] = 0;
	}
}

static void
ieee80211_status(const char *ifname)
{
	/* XXX fill in */
}

static int
getsocket(void)
{
	static int s = -1;

	if (s < 0) {
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0)
			err(1, "socket(SOCK_DRAGM)");
	}
	return s;
}

static int
get80211param(const char *ifname, int param, void *data, size_t len)
{
	struct iwreq iwr;

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
	iwr.u.mode = param;

	if (ioctl(getsocket(), IEEE80211_IOCTL_GETPARAM, &iwr) < 0) {
		perror("ioctl[IEEE80211_IOCTL_GETPARAM]");
		return -1;
	}
	if (len < IFNAMSIZ) {
		/*
		 * Argument data fits inline; put it there.
		 */
		memcpy(data, iwr.u.name, len);
	}
	return iwr.u.data.length;
}

static int
do80211priv(struct iwreq *iwr, const char *ifname, int op, void *data, size_t len)
{
#define	N(a)	(sizeof(a)/sizeof(a[0]))

	memset(iwr, 0, sizeof(iwr));
	strncpy(iwr->ifr_name, ifname, IFNAMSIZ);
	if (len < IFNAMSIZ) {
		/*
		 * Argument data fits inline; put it there.
		 */
		memcpy(iwr->u.name, data, len);
	} else {
		/*
		 * Argument data too big for inline transfer; setup a
		 * parameter block instead; the kernel will transfer
		 * the data for the driver.
		 */
		iwr->u.data.pointer = data;
		iwr->u.data.length = len;
	}

	if (ioctl(getsocket(), op, iwr) < 0) {
		static const char *opnames[] = {
			"ioctl[IEEE80211_IOCTL_SETPARAM]",
			"ioctl[IEEE80211_IOCTL_GETPARAM]",
			"ioctl[IEEE80211_IOCTL_SETKEY]",
			"ioctl[SIOCIWFIRSTPRIV+3]",
			"ioctl[IEEE80211_IOCTL_DELKEY]",
			"ioctl[SIOCIWFIRSTPRIV+5]",
			"ioctl[IEEE80211_IOCTL_SETMLME]",
			"ioctl[SIOCIWFIRSTPRIV+7]",
			"ioctl[IEEE80211_IOCTL_SETOPTIE]",
			"ioctl[IEEE80211_IOCTL_GETOPTIE]",
			"ioctl[IEEE80211_IOCTL_ADDMAC]",
			"ioctl[SIOCIWFIRSTPRIV+11]",
			"ioctl[IEEE80211_IOCTL_DELMAC]",
			"ioctl[SIOCIWFIRSTPRIV+13]",
			"ioctl[IEEE80211_IOCTL_CHANLIST]",
			"ioctl[SIOCIWFIRSTPRIV+15]",
			"ioctl[IEEE80211_IOCTL_GETRSN]",
			"ioctl[SIOCIWFIRSTPRIV+17]",
			"ioctl[IEEE80211_IOCTL_GETKEY]",
		};
		op -= SIOCIWFIRSTPRIV;
		if (0 <= op && op < N(opnames))
			perror(opnames[op]);
		else
			perror("ioctl[unknown???]");
		return -1;
	}
	return 0;
#undef N
}

static int
set80211priv(const char *ifname, int op, void *data, size_t len)
{
	struct iwreq iwr;

	return do80211priv(&iwr, ifname, op, data, len);
}

static int
get80211priv(const char *ifname, int op, void *data, size_t len)
{
	struct iwreq iwr;

	if (do80211priv(&iwr, ifname, op, data, len) < 0)
		return -1;
	if (len < IFNAMSIZ)
		memcpy(data, iwr.u.name, len);
	return iwr.u.data.length;
}

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}
