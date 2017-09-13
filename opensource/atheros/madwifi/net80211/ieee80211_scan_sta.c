/*-
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef EXPORT_SYMTAB
#define	EXPORT_SYMTAB
#endif

__FBSDID("$FreeBSD$");

/*
 * IEEE 802.11 station scanning support.
 */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/init.h>

#include "if_media.h"

#include <net80211/ieee80211_var.h>

/*
 * Parameters for managing cache entries:
 *
 * o a station with STA_FAILS_MAX failures is not considered
 *   when picking a candidate
 * o a station that hasn't had an update in STA_PURGE_SCANS
 *   (background) scans is discarded
 * o after STA_FAILS_AGE seconds we clear the failure count
 */
#define	STA_FAILS_MAX	2		/* assoc failures before ignored */
#define	STA_FAILS_AGE	(2*60)		/* time before clearing fails (secs) */
#define	STA_PURGE_SCANS	2		/* age for purging entries (scans) */

/* XXX tunable */
#define	STA_RSSI_MIN	8		/* min acceptable rssi */

#define RSSI_LPF_LEN	10
#define	RSSI_EP_MULTIPLIER	(1<<7)	/* pow2 to optimize out * and / */
#define RSSI_IN(x)		((x) * RSSI_EP_MULTIPLIER)
#define LPF_RSSI(x, y, len)	(((x) * ((len) - 1) + (y)) / (len))
#define RSSI_LPF(x, y) do {						\
    if ((y) >= -20)							\
    	x = LPF_RSSI((x), RSSI_IN((y)), RSSI_LPF_LEN);			\
} while (0)
#define	EP_RND(x, mul) \
	((((x)%(mul)) >= ((mul)/2)) ? howmany(x, mul) : (x)/(mul))
#define	RSSI_GET(x)	EP_RND(x, RSSI_EP_MULTIPLIER)

struct sta_entry {
	struct ieee80211_scan_entry base;
	TAILQ_ENTRY(sta_entry) se_list;
	LIST_ENTRY(sta_entry) se_hash;
	u_int8_t	se_fails;		/* failure to associate count */
	u_int8_t	se_seen;		/* seen during current scan */
	u_int8_t	se_notseen;		/* not seen in previous scans */
	u_int32_t	se_avgrssi;		/* LPF rssi state */
	unsigned long	se_lastupdate;		/* time of last update */
	unsigned long	se_lastfail;		/* time of last failure */
	unsigned long	se_lastassoc;		/* time of last association */
	u_int		se_scangen;		/* iterator scan gen# */
};

#define	STA_HASHSIZE	32
/* simple hash is enough for variation of macaddr */
#define	STA_HASH(addr)	\
	(((const u_int8_t *)(addr))[IEEE80211_ADDR_LEN - 1] % STA_HASHSIZE)

struct sta_table {
	spinlock_t	st_lock;		/* on scan table */
	TAILQ_HEAD(, sta_entry) st_entry;	/* all entries */
	ATH_LIST_HEAD(, sta_entry) st_hash[STA_HASHSIZE];
	spinlock_t	st_scanlock;		/* on st_scangen */
	u_int		st_scangen;		/* gen# for iterator */
	int		st_newscan;
};

static void sta_flush_table(struct sta_table *);
static int match_bss(struct ieee80211vap *,
	const struct ieee80211_scan_state *, const struct sta_entry *);

/*
 * Attach prior to any scanning work.
 */
static int
sta_attach(struct ieee80211_scan_state *ss)
{
	struct sta_table *st;

	_MOD_INC_USE(THIS_MODULE, return 0);

	MALLOC(st, struct sta_table *, sizeof(struct sta_table),
		M_80211_SCAN, M_NOWAIT | M_ZERO);
	if (st == NULL)
		return 0;
	spin_lock_init(&st->st_lock);
	spin_lock_init(&st->st_scanlock);
	TAILQ_INIT(&st->st_entry);
	ss->ss_priv = st;
	return 1;
}

/*
 * Cleanup any private state.
 */
static int
sta_detach(struct ieee80211_scan_state *ss)
{
	struct sta_table *st = ss->ss_priv;

	if (st != NULL) {
		sta_flush_table(st);
		FREE(st, M_80211_SCAN);
	}

	_MOD_DEC_USE(THIS_MODULE);
	return 1;
}

/*
 * Flush all per-scan state.
 */
static int
sta_flush(struct ieee80211_scan_state *ss)
{
	struct sta_table *st = ss->ss_priv;

	spin_lock_bh(&st->st_lock);
	sta_flush_table(st);
	spin_unlock_bh(&st->st_lock);
	ss->ss_last = 0;
	return 0;
}

/*
 * Flush all entries in the scan cache.
 */
static void
sta_flush_table(struct sta_table *st)
{
	struct sta_entry *se, *next;

	TAILQ_FOREACH_SAFE(se, &st->st_entry, se_list, next) {
		TAILQ_REMOVE(&st->st_entry, se, se_list);
		LIST_REMOVE(se, se_hash);
		FREE(se, M_80211_SCAN);
	}
}

static void
saveie(u_int8_t **iep, const u_int8_t *ie)
{

	if (ie == NULL)
		*iep = NULL;
	else
		ieee80211_saveie(iep, ie);
}

/*
 * Process a beacon or probe response frame; create an
 * entry in the scan cache or update any previous entry.
 */
static int
sta_add(struct ieee80211_scan_state *ss, 
	const struct ieee80211_scanparams *sp,
	const struct ieee80211_frame *wh,
	int subtype, int rssi, int rstamp)
{
#define	ISPROBE(_st)	((_st) == IEEE80211_FC0_SUBTYPE_PROBE_RESP)
#define	PICK1ST(_ss) \
	((ss->ss_flags & (IEEE80211_SCAN_PICK1ST | IEEE80211_SCAN_GOTPICK)) == \
	IEEE80211_SCAN_PICK1ST)
	struct sta_table *st = ss->ss_priv;
	const u_int8_t *macaddr = wh->i_addr2;
	struct ieee80211vap *vap = ss->ss_vap;
	struct ieee80211com *ic = vap->iv_ic;
	struct sta_entry *se;
	struct ieee80211_scan_entry *ise;
	int hash;

	hash = STA_HASH(macaddr);

	spin_lock_bh(&st->st_lock);
	LIST_FOREACH(se, &st->st_hash[hash], se_hash)
		if (IEEE80211_ADDR_EQ(se->base.se_macaddr, macaddr))
			goto found;
	MALLOC(se, struct sta_entry *, sizeof(struct sta_entry),
		M_80211_SCAN, M_NOWAIT | M_ZERO);
	if (se == NULL) {
		spin_unlock_bh(&st->st_lock);
		return 0;
	}
	se->se_scangen = st->st_scangen-1;
	IEEE80211_ADDR_COPY(se->base.se_macaddr, macaddr);
	TAILQ_INSERT_TAIL(&st->st_entry, se, se_list);
	LIST_INSERT_HEAD(&st->st_hash[hash], se, se_hash);
found:
	ise = &se->base;
	/* XXX ap beaconing multiple ssid w/ same bssid */
	if (sp->ssid[1] != 0 &&
	    (ISPROBE(subtype) || ise->se_ssid[1] == 0))
		memcpy(ise->se_ssid, sp->ssid, 2+sp->ssid[1]);
	KASSERT(sp->rates[1] <= IEEE80211_RATE_MAXSIZE,
		("rate set too large: %u", sp->rates[1]));
	memcpy(ise->se_rates, sp->rates, 2+sp->rates[1]);
	if (sp->xrates != NULL) {
		/* XXX validate xrates[1] */
		KASSERT(sp->xrates[1] <= IEEE80211_RATE_MAXSIZE,
			("xrate set too large: %u", sp->xrates[1]));
		memcpy(ise->se_xrates, sp->xrates, 2+sp->xrates[1]);
	} else
		ise->se_xrates[1] = 0;
	IEEE80211_ADDR_COPY(ise->se_bssid, wh->i_addr3);
	/*
	 * Record rssi data using extended precision LPF filter.
	 */
	if (se->se_lastupdate == 0)		/* first sample */
		se->se_avgrssi = RSSI_IN(rssi);
	else					/* avg w/ previous samples */
		RSSI_LPF(se->se_avgrssi, rssi);
	se->base.se_rssi = RSSI_GET(se->se_avgrssi);
	ise->se_rstamp = rstamp;
	memcpy(ise->se_tstamp.data, sp->tstamp, sizeof(ise->se_tstamp));
	ise->se_intval = sp->bintval;
	ise->se_capinfo = sp->capinfo;
	ise->se_chan = ic->ic_curchan;
	ise->se_fhdwell = sp->fhdwell;
	ise->se_fhindex = sp->fhindex;
	ise->se_erp = sp->erp;
	ise->se_timoff = sp->timoff;
	if (sp->tim != NULL) {
		const struct ieee80211_tim_ie *tim =
		    (const struct ieee80211_tim_ie *) sp->tim;
		ise->se_dtimperiod = tim->tim_period;
	}
	saveie(&ise->se_wme_ie, sp->wme);
	saveie(&ise->se_wpa_ie, sp->wpa);
	saveie(&ise->se_htcap_ie, sp->htcap);
	saveie(&ise->se_htinfo_ie, sp->htinfo);
	saveie(&ise->se_ath_ie, sp->ath);

	/* clear failure count after STA_FAIL_AGE passes */
	if (se->se_fails && (jiffies - se->se_lastfail) > STA_FAILS_AGE*HZ) {
		se->se_fails = 0;
		IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_SCAN, macaddr,
		    "%s: fails %u", __func__, se->se_fails);
	}

	se->se_lastupdate = jiffies;		/* update time */
	se->se_seen = 1;
	se->se_notseen = 0;

	spin_unlock_bh(&st->st_lock);

	/*
	 * If looking for a quick choice and nothing's
	 * been found check here.
	 */
	if (PICK1ST(ss) && match_bss(vap, ss, se) == 0)
		ss->ss_flags |= IEEE80211_SCAN_GOTPICK;

	return 1;
#undef PICK1ST
#undef ISPROBE
}

static struct ieee80211_channel *
finddot11channel(struct ieee80211com *ic, int freq, u_int flags)
{
	struct ieee80211_channel *c;
	int j;

	for (j = 0; j < ic->ic_nchans; j++) {
		c = &ic->ic_channels[j];
		if ((c->ic_freq == freq) && ((c->ic_flags & flags) == flags))
			return c;
	}
	return NULL;
}

static struct ieee80211_channel *
find11gchannel(struct ieee80211com *ic, int freq)
{
	struct ieee80211_channel *c;
	int j;

	for (j = 0; j < ic->ic_nchans; j++) {
		c = &ic->ic_channels[j];
		if (c->ic_freq == freq && IEEE80211_IS_CHAN_ANYG(c))
			return c;
	}
	return NULL;
}

static const u_int chanflags[] = {
	IEEE80211_CHAN_B,	/* IEEE80211_MODE_AUTO */
	IEEE80211_CHAN_A,	/* IEEE80211_MODE_11A */
	IEEE80211_CHAN_B,	/* IEEE80211_MODE_11B */
	IEEE80211_CHAN_PUREG,	/* IEEE80211_MODE_11G */
	IEEE80211_CHAN_FHSS,	/* IEEE80211_MODE_FH */
	IEEE80211_CHAN_A,	/* IEEE80211_MODE_TURBO_A */ /* for turbo mode look for AP in normal channel */
	IEEE80211_CHAN_PUREG,	/* IEEE80211_MODE_TURBO_G */
	IEEE80211_CHAN_11NA,	/* IEEE80211_MODE_11NA */
	IEEE80211_CHAN_11NG,	/* IEEE80211_MODE_11NG */
	IEEE80211_CHAN_ST,	/* IEEE80211_MODE_TURBO_STATIC_A */
};

static void
add_channels(struct ieee80211com *ic,
	struct ieee80211_scan_state *ss,
	enum ieee80211_phymode mode, enum ieee80211_phymode desmode, 
	const u_int16_t freq[], int nfreq)
{
#define	N(a)	(sizeof(a) / sizeof(a[0]))
	struct ieee80211_channel *c, *cg, *cn;
	u_int modeflags;
	int i;

	KASSERT(mode < N(chanflags), ("Unexpected mode %u", mode));
	modeflags = chanflags[mode];
	for (i = 0; i < nfreq; i++) {
		c = ieee80211_find_channel(ic, freq[i], modeflags);
		if (c == NULL || isclr(ic->ic_chan_active, c->ic_ieee))
			continue;
		if (desmode == IEEE80211_MODE_AUTO) {
			switch(mode) {
				case IEEE80211_MODE_11B:
					if ((cn = finddot11channel(ic, c->ic_freq, IEEE80211_CHAN_11NG)) != NULL)
						c = cn;
					else if ((cg = find11gchannel(ic, c->ic_freq)) != NULL) 
						c = cg;
					break;
				case IEEE80211_MODE_TURBO_G:
				case IEEE80211_MODE_11G:
					if ((cn = finddot11channel(ic, c->ic_freq, IEEE80211_CHAN_11NG)) != NULL)
						c = cn;
					break;
				case IEEE80211_MODE_TURBO_A:
				case IEEE80211_MODE_11A:
					if ((cn = finddot11channel(ic, c->ic_freq, IEEE80211_CHAN_11NA)) != NULL)
						c = cn;
					break;
				default:
					break;
			}
		}
		if (ss->ss_last >= IEEE80211_SCAN_MAX)
			break;
		ss->ss_chans[ss->ss_last++] = c;
	}
#undef N
}

static const u_int16_t rcl1[] =		/* 8 FCC channel: 52, 56, 60, 64, 36, 40, 44, 48 */
{ 5260, 5280, 5300, 5320, 5180, 5200, 5220, 5240 };
static const u_int16_t rcl2[] =		/* 4 MKK channels: 34, 38, 42, 46 */
{ 5170, 5190, 5210, 5230 };
static const u_int16_t rcl3[] =		/* 2.4Ghz ch: 1,6,11,7,13 */
{ 2412, 2437, 2462, 2442, 2472 };
static const u_int16_t rcl4[] =		/* 5 FCC channel: 149, 153, 161, 165 */
{ 5745, 5765, 5785, 5805, 5825 };
static const u_int16_t rcl7[] =		/* 11 ETSI channel: 100,104,108,112,116,120,124,128,132,136,140 */
{ 5500, 5520, 5540, 5560, 5580, 5600, 5620, 5640, 5660, 5680, 5700 };
static const u_int16_t rcl8[] =		/* 2.4Ghz ch: 2,3,4,5,8,9,10,12 */
{ 2417, 2422, 2427, 2432, 2447, 2452, 2457, 2467 };
static const u_int16_t rcl9[] =		/* 2.4Ghz ch: 14 */
{ 2484 };
static const u_int16_t rcl10[] =	/* Added Korean channels 2312-2372 */
{ 2312, 2317, 2322, 2327, 2332, 2337, 2342, 2347, 2352, 2357, 2362, 2367, 2372 };
static const u_int16_t rcl11[] =	/* Added Japan channels in 4.9/5.0 spectrum */
{ 5040, 5060, 5080, 4920, 4940, 4960, 4980 };
#ifdef ATH_TURBO_SCAN
static const u_int16_t rcl5[] =		/* 3 static turbo channels */
{ 5210, 5250, 5290 };
static const u_int16_t rcl6[] =		/* 2 static turbo channels */
{ 5760, 5800 };
static const u_int16_t rcl6x[] =	/* 4 FCC3 turbo channels */
{ 5540, 5580, 5620, 5660 };
static const u_int16_t rcl12[] =	/* 2.4Ghz Turbo channel 6 */
{ 2437 };
static const u_int16_t rcl13[] =	/* dynamic Turbo channels */
{ 5200, 5240, 5280, 5765, 5805 };
#endif /* ATH_TURBO_SCAN */

struct scanlist {
	u_int16_t	mode;
	u_int16_t	count;
	const u_int16_t	*list;
};

#define	IEEE80211_MODE_TURBO_STATIC_A	IEEE80211_MODE_MAX
#define	X(a)	.count = sizeof(a)/sizeof(a[0]), .list = a

static const struct scanlist staScanTable[] = {
	{ IEEE80211_MODE_11B,   	X(rcl3) },
	{ IEEE80211_MODE_11A,   	X(rcl1) },
	{ IEEE80211_MODE_11A,   	X(rcl2) },
	{ IEEE80211_MODE_11B,   	X(rcl8) },
	{ IEEE80211_MODE_11B,   	X(rcl9) },
	{ IEEE80211_MODE_11A,   	X(rcl4) },
#ifdef ATH_TURBO_SCAN
	{ IEEE80211_MODE_TURBO_STATIC_A,	X(rcl5) },
	{ IEEE80211_MODE_TURBO_STATIC_A,	X(rcl6) },
	{ IEEE80211_MODE_TURBO_A,	X(rcl6x) },
	{ IEEE80211_MODE_TURBO_A,	X(rcl13) },
#endif /* ATH_TURBO_SCAN */
	{ IEEE80211_MODE_11A,		X(rcl7) },
	{ IEEE80211_MODE_11B,		X(rcl10) },
	{ IEEE80211_MODE_11A,		X(rcl11) },
#ifdef ATH_TURBO_SCAN
	{ IEEE80211_MODE_TURBO_G,	X(rcl12) },
#endif /* ATH_TURBO_SCAN */
	{ .list = NULL }
};

static int
checktable(const struct scanlist *scan, const struct ieee80211_channel *c)
{
	int i;

	for (; scan->list != NULL; scan++) {
		for (i = 0; i < scan->count; i++)
			if (scan->list[i] == c->ic_freq) 
				return 1;
	}
	return 0;
}

/*
 * Start a station-mode scan by populating the channel list.
 */
static int
sta_start(struct ieee80211_scan_state *ss, struct ieee80211vap *vap)
{
#define	N(a)	(sizeof(a)/sizeof(a[0]))
	struct ieee80211com *ic = vap->iv_ic;
	struct sta_table *st = ss->ss_priv;
	const struct scanlist *scan;
	enum ieee80211_phymode mode;
	struct ieee80211_channel *c;
	int i;
	
	ss->ss_last = 0;
	/*
	 * Use the table of ordered channels to construct the list
	 * of channels for scanning.  Any channels in the ordered
	 * list not in the master list will be discarded.
	 */
	for (scan = staScanTable; scan->list != NULL; scan++) {
		mode = scan->mode;
		if (vap->iv_des_mode != IEEE80211_MODE_AUTO) {
			/*
			 * If a desired mode was specified, scan only 
			 * channels that satisfy that constraint.
			 */
			if (vap->iv_des_mode != mode) {
				/*
				 * The scan table marks 2.4Ghz channels as b
				 * so if the desired mode is 11g, then use
				 * the 11b channel list but upgrade the mode.
				 */
				switch(vap->iv_des_mode) {
					case IEEE80211_MODE_11G : 
						if (mode == IEEE80211_MODE_11B)
							mode = IEEE80211_MODE_11G;
						break;
					case IEEE80211_MODE_11NA : 
						if (mode == IEEE80211_MODE_11A)
							mode = IEEE80211_MODE_11NA;
						break;
					case IEEE80211_MODE_11NG : 
						if ((mode == IEEE80211_MODE_11G) ||
							(mode == IEEE80211_MODE_11B))
							mode = IEEE80211_MODE_11NG;
						break;
					default:
						break;
				}
			}
            if (vap->iv_des_mode != mode) 
                continue;
		}
		/* XR does not operate on turbo channels */
		if ((vap->iv_flags & IEEE80211_F_XR) &&
		    (mode == IEEE80211_MODE_TURBO_A ||
		     mode == IEEE80211_MODE_TURBO_G))
			continue;
		/*
		 * Add the list of the channels; any that are not
		 * in the master channel list will be discarded.
		 */
		add_channels(ic, ss, mode, vap->iv_des_mode, scan->list, scan->count);
	}

	/*
	 * Add the channels from the ic (from HAL) that are not present
	 * in the staScanTable.
	 */
	for (i = 0; i < ic->ic_nchans; i++) {
		c = &ic->ic_channels[i];
		/*
		 * scan dynamic turbo channels in normal mode.
		 */
		if (IEEE80211_IS_CHAN_DTURBO(c))
			continue;
		mode = ieee80211_chan2mode(c);
		if (vap->iv_des_mode != IEEE80211_MODE_AUTO) {
			/*
			 * If a desired mode was specified, scan only 
			 * channels that satisfy that constraint.
			 */
			if (vap->iv_des_mode != mode) 
				continue;
			
		} 
		if (!checktable(staScanTable, c))
			ss->ss_chans[ss->ss_last++] = c;
	}

	ss->ss_next = 0;
	/* XXX tunables */
	ss->ss_mindwell = msecs_to_jiffies(20);		/* 20ms */
	ss->ss_maxdwell = msecs_to_jiffies(200);	/* 200ms */

#ifdef IEEE80211_DEBUG
	if (ieee80211_msg_scan(vap)) {
		printf("%s: scan set ", vap->iv_dev->name);
		ieee80211_scan_dump_channels(ss);
		printf(" dwell min %ld max %ld\n",
			ss->ss_mindwell, ss->ss_maxdwell);
	}
#endif /* IEEE80211_DEBUG */

	st->st_newscan = 1;

	return 0;
#undef N
}

/*
 * Restart a bg scan.
 */
static int
sta_restart(struct ieee80211_scan_state *ss, struct ieee80211vap *vap)
{
	struct sta_table *st = ss->ss_priv;

	st->st_newscan = 1;
	return 0;
}

/*
 * Cancel an ongoing scan.
 */
static int
sta_cancel(struct ieee80211_scan_state *ss, struct ieee80211vap *vap)
{
	return 0;
}

static u_int8_t
maxrate(const struct ieee80211_scan_entry *se)
{
	u_int8_t max, r;
	int i;

	max = 0;
	for (i = 0; i < se->se_rates[1]; i++) {
		r = se->se_rates[2+i] & IEEE80211_RATE_VAL;
		if (r > max)
			max = r;
	}
	for (i = 0; i < se->se_xrates[1]; i++) {
		r = se->se_xrates[2+i] & IEEE80211_RATE_VAL;
		if (r > max)
			max = r;
	}
	return max;
}

/*
 * Compare the capabilities of two entries and decide which is
 * more desirable (return >0 if a is considered better).  Note
 * that we assume compatibility/usability has already been checked
 * so we don't need to (e.g. validate whether privacy is supported).
 * Used to select the best scan candidate for association in a BSS.
 */
static int
sta_compare(const struct sta_entry *a, const struct sta_entry *b)
{
	u_int8_t maxa, maxb;
	int weight;

	/* privacy support preferred */
	if ((a->base.se_capinfo & IEEE80211_CAPINFO_PRIVACY) &&
	    (b->base.se_capinfo & IEEE80211_CAPINFO_PRIVACY) == 0)
		return 1;
	if ((a->base.se_capinfo & IEEE80211_CAPINFO_PRIVACY) == 0 &&
	    (b->base.se_capinfo & IEEE80211_CAPINFO_PRIVACY))
		return -1;

	/* compare count of previous failures */
	weight = b->se_fails - a->se_fails;
	if (abs(weight) > 1)
		return weight;

	if (abs(b->base.se_rssi - a->base.se_rssi) < 5) {
		/* best/max rate preferred if signal level close enough XXX */
		maxa = maxrate(&a->base);
		maxb = maxrate(&b->base);
		if (maxa != maxb)
			return maxa - maxb;
		/* XXX use freq for channel preference */
		/* for now just prefer 5Ghz band to all other bands */
		if (IEEE80211_IS_CHAN_5GHZ(a->base.se_chan) &&
		   !IEEE80211_IS_CHAN_5GHZ(b->base.se_chan))
			return 1;
		if (!IEEE80211_IS_CHAN_5GHZ(a->base.se_chan) &&
		     IEEE80211_IS_CHAN_5GHZ(b->base.se_chan))
			return -1;
	}
	/* all things being equal, use signal level */
	return a->base.se_rssi - b->base.se_rssi;
}

/*
 * Check rate set suitability
 */
static int
check_rate(struct ieee80211vap *vap, const struct ieee80211_scan_entry *se)
{
#define	RV(v)	((v) & IEEE80211_RATE_VAL)
	struct ieee80211com *ic = vap->iv_ic;
	struct ieee80211_rateset *srs;
	struct ieee80211_rateset rrs;
	int i,j;

	if (IEEE80211_IS_CHAN_HALF(se->se_chan)) {
		srs = &ic->ic_sup_half_rates;
	} else if (IEEE80211_IS_CHAN_QUARTER(se->se_chan)) {
		srs = &ic->ic_sup_quarter_rates;
	} else {
		srs = &ic->ic_sup_rates[ieee80211_chan2mode(se->se_chan)];
	}
	memset(&rrs, 0, sizeof(rrs));
	rrs.rs_nrates = se->se_rates[1];
	memcpy(rrs.rs_rates, se->se_rates + 2, rrs.rs_nrates);
	if (se->se_xrates != NULL) {
		u_int8_t nxrates;
		/*
		 * Tack on 11g extended supported rate element.
		 */
		nxrates = se->se_xrates[1];
		if (rrs.rs_nrates + nxrates > IEEE80211_RATE_MAXSIZE) {
			nxrates = IEEE80211_RATE_MAXSIZE - rrs.rs_nrates;
		}
		memcpy(rrs.rs_rates + rrs.rs_nrates, se->se_xrates+2, nxrates);
		rrs.rs_nrates += nxrates;
	}
	if (!(vap->iv_fixed_rate.mode & IEEE80211_FIXED_RATE_MCS) &&
	    (vap->iv_fixed_rate.mode != IEEE80211_FIXED_RATE_NONE))
	{
		for (i = 0; i < rrs.rs_nrates; i++) {
			if ((rrs.rs_rates[i] & IEEE80211_RATE_VAL) == 
					(vap->iv_fixed_rate.series & IEEE80211_RATE_VAL)) {
				break;
			}
		}
		if (i == rrs.rs_nrates) {
			return 0;
		}
	}
	for (i = 0; i < rrs.rs_nrates; i++) {
		if (rrs.rs_rates[i] & IEEE80211_RATE_BASIC) {
			for (j = 0; j < srs->rs_nrates; j++) {
				if (RV(rrs.rs_rates[i]) == RV(srs->rs_rates[j])) {
					break;
				}
			}
			if (j == srs->rs_nrates) {
				return 0;
			}
		}
	}
	return 1;
#undef RV
}

/*
 * Check rate set suitability
 */
static int
check_ht_rate(struct ieee80211vap *vap, const struct ieee80211_scan_entry *se)
{
#define	RV(v)	((v) & IEEE80211_RATE_VAL)
	struct ieee80211com *ic = vap->iv_ic;
	struct ieee80211_rateset *srs;
	struct ieee80211_rateset rrs;
	struct ieee80211_ie_htcap_cmn *htcap;
	struct ieee80211_ie_htinfo_cmn *htinfo;
	int i,j,k;

	k = 0;
	srs = &ic->ic_sup_ht_rates[ieee80211_chan2mode(se->se_chan)];
	memset(&rrs, 0, sizeof(rrs));
	if (se->se_htcap_ie && se->se_htinfo_ie) {
		htcap  = (struct ieee80211_ie_htcap_cmn *)se->se_htcap_ie;
		htinfo = (struct ieee80211_ie_htinfo_cmn *)se->se_htinfo_ie;

		for (i=0; i < IEEE80211_HT_RATE_SIZE; i++) {
			if (htcap->hc_mcsset[i/8] & (1<<(i%8))) {
				rrs.rs_rates[k++] = i | ((htinfo->hi_basicmcsset[i/8] & (1<<(i%8))) ? 
							 IEEE80211_RATE_BASIC : 0);
			}
			if (k == IEEE80211_RATE_MAXSIZE) {
				break;
			}
		}
	}
	rrs.rs_nrates = k;
	if ((vap->iv_fixed_rate.mode == IEEE80211_FIXED_RATE_MCS) &&
	    (vap->iv_fixed_rate.mode != IEEE80211_FIXED_RATE_NONE))
	{
		for (i = 0; i < rrs.rs_nrates; i++) {
			if ((rrs.rs_rates[i] & IEEE80211_RATE_VAL) == 
			    (vap->iv_fixed_rate.series & IEEE80211_RATE_VAL)) {
				break;
			}
		}
		if (i == rrs.rs_nrates) {
			return 0;
		}
	}
	for (i = 0; i < rrs.rs_nrates; i++) {
		if (rrs.rs_rates[i] & IEEE80211_RATE_BASIC) {
			for (j = 0; j < srs->rs_nrates; j++) {
				if (RV(rrs.rs_rates[i]) == RV(srs->rs_rates[j])) {
					break;
				}
			}
			if (j == srs->rs_nrates) {
				return 0;
			}
		}
	}
	return 1;
#undef RV
}

static int
match_ssid(const u_int8_t *ie,
	int nssid, const struct ieee80211_scan_ssid ssids[])
{
	int i;

	for (i = 0; i < nssid; i++) {
		if (ie[1] == ssids[i].len &&
		     memcmp(ie+2, ssids[i].ssid, ie[1]) == 0)
			return 1;
	}
	return 0;
}

/*
 * Test a scan candidate for suitability/compatibility.
 */
static int
match_bss(struct ieee80211vap *vap,
	const struct ieee80211_scan_state *ss, const struct sta_entry *se0)
{
	struct ieee80211com *ic = vap->iv_ic;
	const struct ieee80211_scan_entry *se = &se0->base;
        int fail;

	fail = 0;
	if (isclr(ic->ic_chan_active, ieee80211_chan2ieee(ic, se->se_chan)))
		fail |= 0x01;
	/*
	 * NB: normally the desired mode is used to construct
	 * the channel list, but it's possible for the scan
	 * cache to include entries for stations outside this
	 * list so we check the desired mode here to weed them
	 * out.
	 */
	if (vap->iv_des_mode != IEEE80211_MODE_AUTO &&
	    (!(se->se_chan->ic_flags & IEEE80211_CHAN_ALLTURBO) &
	    chanflags[vap->iv_des_mode]))
		fail |= 0x01;
	if (vap->iv_opmode == IEEE80211_M_IBSS) {
		if ((se->se_capinfo & IEEE80211_CAPINFO_IBSS) == 0)
			fail |= 0x02;
	} else {
		if ((se->se_capinfo & IEEE80211_CAPINFO_ESS) == 0)
			fail |= 0x02;
	}
	if (vap->iv_flags & IEEE80211_F_PRIVACY) {
		if ((se->se_capinfo & IEEE80211_CAPINFO_PRIVACY) == 0)
			fail |= 0x04;
	} else {
		/* XXX does this mean privacy is supported or required? */
		if (se->se_capinfo & IEEE80211_CAPINFO_PRIVACY)
			fail |= 0x04;
	}
	if (!check_rate(vap, se))
		fail |= 0x08;
	if (!check_ht_rate(vap, se))
		fail |= 0x08;
	if (ss->ss_nssid != 0 &&
	    !match_ssid(se->se_ssid, ss->ss_nssid, ss->ss_ssid))
		fail |= 0x10;
	if ((vap->iv_flags & IEEE80211_F_DESBSSID) &&
	    !IEEE80211_ADDR_EQ(vap->iv_des_bssid, se->se_bssid))
		fail |= 0x20;
	if (se0->se_fails >= STA_FAILS_MAX)
		fail |= 0x40;
	if (se0->se_notseen >= STA_PURGE_SCANS)
		fail |= 0x80;
	if (se->se_rssi < STA_RSSI_MIN)
		fail |= 0x100;
#ifdef IEEE80211_DEBUG
	if (ieee80211_msg(vap, IEEE80211_MSG_SCAN | IEEE80211_MSG_ROAM)) {
		printf(" %c %s",
		    fail & 0x40 ? '=' : fail & 0x80 ? '^' : fail ? '-' : '+',
		    ether_sprintf(se->se_macaddr));
		printf(" %s%c", ether_sprintf(se->se_bssid),
		    fail & 0x20 ? '!' : ' ');
		printf(" %3d%c", ieee80211_chan2ieee(ic, se->se_chan),
			fail & 0x01 ? '!' : ' ');
		printf(" %+4d%c", se->se_rssi, fail & 0x100 ? '!' : ' ');
		printf(" M%c", fail & 0x08 ? '!' : ' ');
		printf(" %4s%c",
		    (se->se_capinfo & IEEE80211_CAPINFO_ESS) ? "ess" :
		    (se->se_capinfo & IEEE80211_CAPINFO_IBSS) ? "ibss" :
		    "????",
		    fail & 0x02 ? '!' : ' ');
		printf(" %3s%c ",
		    (se->se_capinfo & IEEE80211_CAPINFO_PRIVACY) ?
		    "wep" : "no",
		    fail & 0x04 ? '!' : ' ');
		ieee80211_print_essid(se->se_ssid+2, se->se_ssid[1]);
		printf("%s\n", fail & 0x10 ? "!" : "");
	}
#endif
	return fail;
}

static void
sta_update_notseen(struct sta_table *st)
{
	struct sta_entry *se;

	spin_lock_bh(&st->st_lock);
	TAILQ_FOREACH(se, &st->st_entry, se_list) {
		/*
		 * If seen the reset and don't bump the count;
		 * otherwise bump the ``not seen'' count.  Note
		 * that this insures that stations for which we
		 * see frames while not scanning but not during
		 * this scan will not be penalized.
		 */
		if (se->se_seen)
			se->se_seen = 0;
		else
			se->se_notseen++;
	}
	spin_unlock_bh(&st->st_lock);
}

static void
sta_dec_fails(struct sta_table *st)
{
	struct sta_entry *se;

	spin_lock_bh(&st->st_lock);
	TAILQ_FOREACH(se, &st->st_entry, se_list)
		if (se->se_fails)
			se->se_fails--;
	spin_unlock_bh(&st->st_lock);
}

static struct sta_entry *
select_bss(struct ieee80211_scan_state *ss, struct ieee80211vap *vap)
{
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *se, *selbs = NULL;

	IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN | IEEE80211_MSG_ROAM, " %s\n",
	    "macaddr          bssid         chan  rssi  rate flag  wep  essid");
	spin_lock_bh(&st->st_lock);
	TAILQ_FOREACH(se, &st->st_entry, se_list) {
		if (match_bss(vap, ss, se) == 0) {
			if (selbs == NULL)
				selbs = se;
			else if (sta_compare(se, selbs) > 0)
				selbs = se;
		}
	}
	spin_unlock_bh(&st->st_lock);

	return selbs;
}

/*
 * Pick an ap or ibss network to join or find a channel
 * to use to start an ibss network.
 */
static int
sta_pick_bss(struct ieee80211_scan_state *ss, struct ieee80211vap *vap)
{
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *selbs;

	KASSERT(vap->iv_opmode == IEEE80211_M_STA,
		("wrong mode %u", vap->iv_opmode));

	if (st->st_newscan) {
		sta_update_notseen(st);
		st->st_newscan = 0;
	}
	if (ss->ss_flags & IEEE80211_SCAN_NOPICK) {
		/*
		 * Manual/background scan, don't select+join the
		 * bss, just return.  The scanning framework will
		 * handle notification that this has completed.
		 */
		ss->ss_flags &= ~IEEE80211_SCAN_NOPICK;
		return 1;
	}
	/*
	 * Automatic sequencing; look for a candidate and
	 * if found join the network.
	 */
	/* NB: unlocked read should be ok */
	if (TAILQ_FIRST(&st->st_entry) == NULL) {
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN,
			"%s: no scan candidate\n", __func__);
notfound:
		/*
		 * If nothing suitable was found decrement
		 * the failure counts so entries will be
		 * reconsidered the next time around.  We
		 * really want to do this only for sta's
		 * where we've previously had some success.
		 */
		sta_dec_fails(st);
		st->st_newscan = 1;
		return 0;			/* restart scan */
	}
	selbs = select_bss(ss, vap);
	if (selbs == NULL || !ieee80211_sta_join(vap, &selbs->base))
		goto notfound;
	return 1;				/* terminate scan */
}

/*
 * Lookup an entry in the scan cache.  We assume we're
 * called from the bottom half or such that we don't need
 * to block the bottom half so that it's safe to return
 * a reference to an entry w/o holding the lock on the table.
 */
static struct sta_entry *
sta_lookup(struct sta_table *st, const u_int8_t macaddr[IEEE80211_ADDR_LEN])
{
	struct sta_entry *se;
	int hash = STA_HASH(macaddr);

	spin_lock(&st->st_lock);
	LIST_FOREACH(se, &st->st_hash[hash], se_hash)
		if (IEEE80211_ADDR_EQ(se->base.se_macaddr, macaddr))
			break;
	spin_unlock(&st->st_lock);

	return se;		/* NB: unlocked */
}

#ifdef notyet
static void
sta_roam_check(struct ieee80211_scan_state *ss, struct ieee80211vap *vap)
{
	struct ieee80211_node *ni = vap->iv_bss;
	struct ieee80211com *ic = vap->iv_ic;
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *se, *selbs;
	u_int8_t roamRate, curRate;
	int8_t roamRssi, curRssi;

	se = sta_lookup(st, ni->ni_macaddr);
	if (se == NULL) {
		/* XXX something is wrong */
		return;
	}

	if (IEEE80211_IS_CHAN_B(ic->ic_bsschan)) {
		roamRate = vap->iv_roam.rate11bOnly;
		roamRssi = vap->iv_roam.rssi11bOnly;
	} else {
		roamRate = vap->iv_roam.rate11a;
		roamRssi = vap->iv_roam.rssi11a;
	}
	/* NB: the most up to date rssi is in the node, not the scan cache */
	curRssi = ic->ic_node_getrssi(ni);
	curRate = ni->ni_rates.rs_rates[ni->ni_txrate] & IEEE80211_RATE_VAL;
	IEEE80211_DPRINTF(vap, IEEE80211_MSG_ROAM,
		"%s: currssi %d currate %u roamrssi %d roamrate %u\n",
		__func__, curRssi, curRate, roamRssi, roamRate);
	if ((vap->iv_flags & IEEE80211_F_BGSCAN) &&
	    time_after(jiffies, ic->ic_lastscan + vap->iv_scanvalid)) {
		/*
		 * Scan cache contents is too old; check about updating it.
		 */
		if (curRate < roamRate || curRssi < roamRssi) {
			/*
			 * Thresholds exceeded, force a scan now so we
			 * have current state to make a decision with.
			 */
			ieee80211_bg_scan(vap);
		} else if (time_after(jiffies,
				ic->ic_lastdata + vap->iv_bgscanidle)) {
			/*
			 * We're not in need of a new ap, but idle;
			 * kick off a bg scan to replenish the cache.
			 */
			ieee80211_bg_scan(vap);
		}
	} else {
		/*
		 * Scan cache contents are warm enough to use;
		 * check if a new ap should be used and switch.
		 * XXX deauth current ap
		 */
		if (curRate < roamRate || curRssi < roamRssi) {
			se->base.se_rssi = curRssi;
			selbs = select_bss(ss, vap);
			if (selbs != NULL && selbs != se)
				ieee80211_sta_join(vap, &selbs->base);
		}
	}
}
#endif

/*
 * Age entries in the scan cache.
 * XXX also do roaming since it's convenient
 */
static void
sta_age(struct ieee80211_scan_state *ss)
{
#ifdef notyet
	struct ieee80211vap *vap = ss->ss_vap;
#endif
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *se, *next;

	spin_lock(&st->st_lock);
	TAILQ_FOREACH_SAFE(se, &st->st_entry, se_list, next) {
		if (se->se_notseen > STA_PURGE_SCANS) {
			TAILQ_REMOVE(&st->st_entry, se, se_list);
			LIST_REMOVE(se, se_hash);
			FREE(se, M_80211_SCAN);
		}
	}
	spin_unlock(&st->st_lock);
#ifdef notyet
	/*
	 * If rate control is enabled check periodically to see if
	 * we should roam from our current connection to one that
	 * might be better.  This only applies when we're operating
	 * in sta mode and automatic roaming is set.
	 * XXX defer if busy
	 * XXX repeater station
	 */
	KASSERT(vap->iv_opmode == IEEE80211_M_STA,
		("wrong mode %u", vap->iv_opmode));
	if (vap->iv_ic->ic_roaming == IEEE80211_ROAMING_AUTO &&
	    vap->iv_fixed_rate.mode == IEEE80211_FIXED_RATE_NONE &&
	    vap->iv_state >= IEEE80211_S_RUN)
		/* XXX vap is implicit */
		sta_roam_check(ss, vap);
#endif
}

/*
 * Iterate over the entries in the scan cache, invoking
 * the callback function on each one.
 */
static void
sta_iterate(struct ieee80211_scan_state *ss, 
	ieee80211_scan_iter_func *f, void *arg)
{
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *se;
	u_int gen;

	spin_lock_bh(&st->st_scanlock);
	gen = st->st_scangen++;
restart:
	spin_lock(&st->st_lock);
	TAILQ_FOREACH(se, &st->st_entry, se_list) {
		if (se->se_scangen != gen) {
			se->se_scangen = gen;
			/* update public state */
			se->base.se_age = jiffies - se->se_lastupdate;
			spin_unlock(&st->st_lock);
			(*f)(arg, &se->base);
			goto restart;
		}
	}
	spin_unlock(&st->st_lock);

	spin_unlock_bh(&st->st_scanlock);
}

static void
sta_assoc_fail(struct ieee80211_scan_state *ss,
	const u_int8_t macaddr[IEEE80211_ADDR_LEN], int reason)
{
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *se;

	se = sta_lookup(st, macaddr);
	if (se != NULL) {
		se->se_fails++;
		se->se_lastfail = jiffies;
		IEEE80211_NOTE_MAC(ss->ss_vap, IEEE80211_MSG_SCAN,
		    macaddr, "%s: reason %u fails %u",
		    __func__, reason, se->se_fails);
	}
}

static void
sta_assoc_success(struct ieee80211_scan_state *ss,
	const u_int8_t macaddr[IEEE80211_ADDR_LEN])
{
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *se;

	se = sta_lookup(st, macaddr);
	if (se != NULL) {
#if 0
		se->se_fails = 0;
		IEEE80211_NOTE_MAC(ss->ss_vap, IEEE80211_MSG_SCAN,
		    macaddr, "%s: fails %u",
		    __func__, se->se_fails);
#endif
		se->se_lastassoc = jiffies;
	}
}

static const struct ieee80211_scanner sta_default = {
	.scan_name		= "default",
	.scan_attach		= sta_attach,
	.scan_detach		= sta_detach,
	.scan_start		= sta_start,
	.scan_restart		= sta_restart,
	.scan_cancel		= sta_cancel,
	.scan_end		= sta_pick_bss,
	.scan_flush		= sta_flush,
	.scan_add		= sta_add,
	.scan_age		= sta_age,
	.scan_iterate		= sta_iterate,
	.scan_assoc_fail	= sta_assoc_fail,
	.scan_assoc_success	= sta_assoc_success,
};

/*
 * Adhoc mode-specific support.
 */

static const u_int16_t adhocWorld[] =		/* 36, 40, 44, 48 */
{ 5180, 5200, 5220, 5240 };
static const u_int16_t adhocFcc3[] =		/* 36, 40, 44, 48 145, 149, 153, 157, 161, 165 */
{ 5180, 5200, 5220, 5240, 5725, 5745, 5765, 5785, 5805, 5825 };
static const u_int16_t adhocMkk[] =		/* 34, 38, 42, 46 */
{ 5170, 5190, 5210, 5230 };
static const u_int16_t adhoc11b[] =		/* 10, 11 */
{ 2457, 2462 };

static const struct scanlist adhocScanTable[] = {
	{ IEEE80211_MODE_11B,   	X(adhoc11b) },
	{ IEEE80211_MODE_11A,   	X(adhocWorld) },
	{ IEEE80211_MODE_11A,   	X(adhocFcc3) },
	{ IEEE80211_MODE_11B,   	X(adhocMkk) },
	{ .list = NULL }
};
#undef X

/*
 * Start an adhoc-mode scan by populating the channel list.
 */
static int
adhoc_start(struct ieee80211_scan_state *ss, struct ieee80211vap *vap)
{
#define	N(a)	(sizeof(a)/sizeof(a[0]))
	struct ieee80211com *ic = vap->iv_ic;
	struct sta_table *st = ss->ss_priv;
	const struct scanlist *scan;
	enum ieee80211_phymode mode;
	
	ss->ss_last = 0;
	/*
	 * Use the table of ordered channels to construct the list
	 * of channels for scanning.  Any channels in the ordered
	 * list not in the master list will be discarded.
	 */
	for (scan = adhocScanTable; scan->list != NULL; scan++) {
		mode = scan->mode;
		if (vap->iv_des_mode != IEEE80211_MODE_AUTO) {
			/*
			 * If a desired mode was specified, scan only 
			 * channels that satisfy that constraint.
			 */
			if (vap->iv_des_mode != mode) {
				switch(vap->iv_des_mode) {
					case IEEE80211_MODE_11G : 
						if (mode == IEEE80211_MODE_11B)
							mode = IEEE80211_MODE_11G;	/* upgrade */
						break;
					case IEEE80211_MODE_11NA : 
						if (mode == IEEE80211_MODE_11A)
							mode = IEEE80211_MODE_11NA;	/* upgrade */
						break;
					case IEEE80211_MODE_11NG : 
						if ((mode == IEEE80211_MODE_11G) ||
							(mode == IEEE80211_MODE_11B))
							mode = IEEE80211_MODE_11NG;	/* upgrade */
						break;
					default:
						break;
				}
			}
		}
		/* XR does not operate on turbo channels */
		if ((vap->iv_flags & IEEE80211_F_XR) &&
		    (mode == IEEE80211_MODE_TURBO_A ||
		     mode == IEEE80211_MODE_TURBO_G))
			continue;
		/*
		 * Add the list of the channels; any that are not
		 * in the master channel list will be discarded.
		 */
		add_channels(ic, ss, mode, vap->iv_des_mode, scan->list, scan->count);
	}
	ss->ss_next = 0;
	/* XXX tunables */
	ss->ss_mindwell = msecs_to_jiffies(200);	/* 200ms */
	ss->ss_maxdwell = msecs_to_jiffies(200);	/* 200ms */

#ifdef IEEE80211_DEBUG
	if (ieee80211_msg_scan(vap)) {
		printf("%s: scan set ", vap->iv_dev->name);
		ieee80211_scan_dump_channels(ss);
		printf(" dwell min %ld max %ld\n",
			ss->ss_mindwell, ss->ss_maxdwell);
	}
#endif /* IEEE80211_DEBUG */

	st->st_newscan = 1;

	return 0;
#undef N
}

/*
 * Select a channel to start an adhoc network on.
 * The channel list was populated with appropriate
 * channels so select one that looks least occupied.
 * XXX need regulatory domain constraints
 */
static struct ieee80211_channel *
adhoc_pick_channel(struct ieee80211_scan_state *ss)
{
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *se;
	struct ieee80211_channel *c, *bestchan;
	int i, bestrssi, maxrssi;

	bestchan = NULL;
	bestrssi = -1;

	spin_lock_bh(&st->st_lock);
	for (i = 0; i < ss->ss_last; i++) {
		c = ss->ss_chans[i];
		maxrssi = 0;
		TAILQ_FOREACH(se, &st->st_entry, se_list) {
			if (se->base.se_chan != c)
				continue;
			if (se->base.se_rssi > maxrssi)
				maxrssi = se->base.se_rssi;
		}
		if (bestchan == NULL || maxrssi < bestrssi)
			bestchan = c;
	}
	spin_unlock_bh(&st->st_lock);

	return bestchan;
}

/*
 * Pick an ibss network to join or find a channel
 * to use to start an ibss network.
 */
static int
adhoc_pick_bss(struct ieee80211_scan_state *ss, struct ieee80211vap *vap)
{
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *selbs;
	struct ieee80211_channel *chan;

	KASSERT(vap->iv_opmode == IEEE80211_M_IBSS ||
		vap->iv_opmode == IEEE80211_M_AHDEMO,
		("wrong opmode %u", vap->iv_opmode));

	if (st->st_newscan) {
		sta_update_notseen(st);
		st->st_newscan = 0;
	}
	if (ss->ss_flags & IEEE80211_SCAN_NOPICK) {
		/*
		 * Manual/background scan, don't select+join the
		 * bss, just return.  The scanning framework will
		 * handle notification that this has completed.
		 */
		ss->ss_flags &= ~IEEE80211_SCAN_NOPICK;
		return 1;
	}
	/*
	 * Automatic sequencing; look for a candidate and
	 * if found join the network.
	 */
	/* NB: unlocked read should be ok */
	if (TAILQ_FIRST(&st->st_entry) == NULL) {
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN,
			"%s: no scan candidate\n", __func__);
notfound:
		if (vap->iv_des_nssid) {
			/*
			 * No existing adhoc network to join and we have
			 * an ssid; start one up.  If no channel was
			 * specified, try to select a channel.
			 */
			if (vap->iv_des_chan == IEEE80211_CHAN_ANYC)
				chan = adhoc_pick_channel(ss);
			else
				chan = vap->iv_des_chan;
			if (chan != NULL) {
				ieee80211_create_ibss(vap, chan);
				return 1;
			}
		}
		/*
		 * If nothing suitable was found decrement
		 * the failure counts so entries will be
		 * reconsidered the next time around.  We
		 * really want to do this only for sta's
		 * where we've previously had some success.
		 */
		sta_dec_fails(st);
		st->st_newscan = 1;
		return 0;			/* restart scan */
	}
	selbs = select_bss(ss, vap);
	if (selbs == NULL || !ieee80211_sta_join(vap, &selbs->base))
		goto notfound;
	return 1;				/* terminate scan */
}

/*
 * Age entries in the scan cache.
 */
static void
adhoc_age(struct ieee80211_scan_state *ss)
{
	struct sta_table *st = ss->ss_priv;
	struct sta_entry *se, *next;

	spin_lock(&st->st_lock);
	TAILQ_FOREACH_SAFE(se, &st->st_entry, se_list, next) {
		if (se->se_notseen > STA_PURGE_SCANS) {
			TAILQ_REMOVE(&st->st_entry, se, se_list);
			LIST_REMOVE(se, se_hash);
			FREE(se, M_80211_SCAN);
		}
	}
	spin_unlock(&st->st_lock);
}

static const struct ieee80211_scanner adhoc_default = {
	.scan_name		= "default",
	.scan_attach		= sta_attach,
	.scan_detach		= sta_detach,
	.scan_start		= adhoc_start,
	.scan_restart		= sta_restart,
	.scan_cancel		= sta_cancel,
	.scan_end		= adhoc_pick_bss,
	.scan_flush		= sta_flush,
	.scan_add		= sta_add,
	.scan_age		= adhoc_age,
	.scan_iterate		= sta_iterate,
	.scan_assoc_fail	= sta_assoc_fail,
	.scan_assoc_success	= sta_assoc_success,
};

/*
 * Module glue.
 */

MODULE_AUTHOR("Errno Consulting, Sam Leffler");
MODULE_DESCRIPTION("802.11 wireless support: default station scanner");
#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif

static int __init
init_scanner_sta(void)
{
	ieee80211_scanner_register(IEEE80211_M_STA, &sta_default);
	ieee80211_scanner_register(IEEE80211_M_IBSS, &adhoc_default);
	ieee80211_scanner_register(IEEE80211_M_AHDEMO, &adhoc_default);
	return 0;
}
module_init(init_scanner_sta);

static void __exit
exit_scanner_sta(void)
{
	ieee80211_scanner_unregister_all(&sta_default);
	ieee80211_scanner_unregister_all(&adhoc_default);
}
module_exit(exit_scanner_sta);
