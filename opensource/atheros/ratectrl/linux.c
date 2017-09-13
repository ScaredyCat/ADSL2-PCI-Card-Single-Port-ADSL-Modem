/*-
 * Copyright (c) 2004 Video54 Technologies, Inc.
 * Copyright (c) 2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/ratectrl11n/linux.c#20 $
 */
#ifndef EXPORT_SYMTAB
#define	EXPORT_SYMTAB
#endif

/*
 * Atheros rate control algorithm (Linux-specific code)
 */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/cache.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>

#include <asm/uaccess.h>

#include <net80211/if_media.h>
#include <net80211/ieee80211_var.h>

#include "if_athvar.h"
#include "ah_desc.h"

#include "ratectrl11n.h"

#ifdef DEBUG_PKTLOG
struct ath_pktlog_rcfuncs *g_pktlog_rcfuncs = NULL;

EXPORT_SYMBOL(g_pktlog_rcfuncs);

#endif
/*
 * Attach to a device instance.  Setup the public definition
 * of how much per-node space we need and setup the private
 * phy tables that have rate control parameters.  These tables
 * are normally part of the Atheros hal but are not included
 * in our hal as the rate control data was not being used and
 * was considered proprietary (at the time).
 */
struct ath_ratectrl *
ath_rate_attach(struct ath_softc *sc)
{
	struct atheros_softc *asc;

	asc = kmalloc(sizeof(struct atheros_softc), GFP_ATOMIC);
	if (asc == NULL)
		return NULL;
	memset(asc, 0, sizeof(struct atheros_softc));
	asc->arc.arc_space = sizeof(struct atheros_node);
	asc->arc.arc_vap_space = sizeof(struct atheros_vap);
	/*
	 * Use the magic number to figure out the chip type.
	 * There's probably a better way to do this but for
	 * now this suffices.
	 *
	 * NB: We don't have a separate set of tables for the
	 *     5210; treat it like a 5211 since it has the same
	 *     tx descriptor format and (hopefully) sufficiently
	 *     similar operating characteristics to work ok.
	 */
	switch (sc->sc_ah->ah_magic) {
	default:		/* XXX 5210 */
	case 0xdeadbeef:
	case 0x19641014:	/* 5416 */
		ar5416AttachRateTables(asc);
		break;
			
	}
	return &asc->arc;
}
EXPORT_SYMBOL(ath_rate_attach);

void
ath_rate_detach(struct ath_ratectrl *rc)
{
	kfree(rc);
}
EXPORT_SYMBOL(ath_rate_detach);

/*
 * Initialize per-node rate control state.
 */
void
ath_rate_node_init(struct ath_softc *sc, struct ath_node *an)
{
	rcSibInit(sc, an);
}
EXPORT_SYMBOL(ath_rate_node_init);

/*
 * Cleanup per-node rate control state.
 */
void
ath_rate_node_cleanup(struct ath_softc *sc, struct ath_node *an)
{
	/* NB: nothing to do */
}
EXPORT_SYMBOL(ath_rate_node_cleanup);


/*
 * Return the next series 0 transmit rate and setup for a callback
 * to install the multi-rate transmit data if appropriate.  We cannot
 * install the multi-rate transmit data here because the caller is
 * going to initialize the tx descriptor and so would clobber whatever
 * we write. Note that we choose an arbitrary series 0 try count to
 * insure we get called back; this permits us to defer calculating
 * the actual number of tries until the callback at which time we
 * can just copy the pre-calculated series data.
 */

void
ath_rate_findrate(struct ath_softc *sc, 
				  struct ath_node *an,
				  size_t frameLen, 
				  int numTries, 
				  int numRates, 
				  int stepDnInc,
				  unsigned int rcflag,
				  struct ath_rc_series series[], 
				  int *isProbe
				  )
{
	struct ieee80211vap  *vap = an->an_node.ni_vap;
	//struct ieee80211com *ic = &sc->sc_ic;
	struct atheros_node *oan = ATH_NODE_ATHEROS(an);

	*isProbe = 0;
	if (!numRates || !numTries) {
		return;
	}

	if (vap->iv_fixed_rate.mode == IEEE80211_FIXED_RATE_NONE) {
		rcRateFind(sc, an, numTries, numRates, stepDnInc, rcflag, series, isProbe);

	} else {
		   int idx;

		   for (idx = 0; idx < 4; idx++) {
		       unsigned int mcs;

		       	series[idx].tries = 
					IEEE80211_RATE_IDX_ENTRY(vap->iv_fixed_rate.retries, idx);
			mcs = IEEE80211_RATE_IDX_ENTRY(vap->iv_fixed_rate.series, idx);
		

			if (idx == 3 && (mcs & 0xf0) == 0x70)
				mcs = (mcs & ~0xf0)|0x80;

			if (!(mcs & 0x80))
				series[idx].flags = 0;
			else
		            series[idx].flags = ((oan->htcap & WLAN_RC_DS_FLAG) ? ATH_RC_DS_FLAG : 0) |
                                ((oan->htcap & WLAN_RC_40_FLAG) ? ATH_RC_CW40_FLAG : 0) |
                                ((oan->htcap & WLAN_RC_SGI_FLAG)? ATH_RC_SGI_FLAG : 0);

				//series[idx].flags = (ic->ic_htflags & IEEE80211_HTF_SHORTGI? ATH_RC_SGI_FLAG: 0)
				//	    |(ic->ic_cwm.cw_mode & IEEE80211_CWM_MODE40? ATH_RC_CW40_FLAG: 0);

			series[idx].rix = sc->sc_rixmap[mcs];
		  }
	}
}
EXPORT_SYMBOL(ath_rate_findrate);


#include <ar5212/ar5212desc.h>

#define	MS(_v, _f)	(((_v) & _f) >> _f##_S)

/*
 * Process a tx descriptor for a completed transmit (success or failure).
 */
void
ath_rate_tx_complete(struct ath_softc *sc,
	struct ath_node *an, const struct ath_desc *ds,  struct ath_rc_series rcs[], 
	int nframes, int nbad)
{
	int finalTSIdx = ds->ds_txstat.ts_rate;
	int tx_status = 0;

	if (an->an_node.ni_vap->iv_fixed_rate.mode != IEEE80211_FIXED_RATE_NONE ||
		ds->ds_txstat.ts_status & HAL_TXERR_FILT) {
		return;
	}

#ifdef ATH_CHAINMASK_SELECT
	if (ds->ds_txstat.ts_rssi > 0) {
		ATH_RSSI_LPF(sc->sc_chainmask_sel.tx_avgrssi, ds->ds_txstat.ts_rssi);
	}
#endif

	if ((ds->ds_txstat.ts_status & HAL_TXERR_XRETRY) ||
		 (ds->ds_txstat.ts_status & HAL_TXERR_FIFO) || 
		 (ds->ds_txstat.ts_flags & HAL_TX_DATA_UNDERRUN) ||
		 (ds->ds_txstat.ts_flags & HAL_TX_DELIM_UNDERRUN)) {
		tx_status = 1;
	}

	rcUpdate(sc, an, ds->ds_txstat.ts_rssi , ds->ds_txstat.ts_antenna, finalTSIdx,
			tx_status, rcs, nframes , nbad, ds->ds_txstat.ts_longretry);
}
EXPORT_SYMBOL(ath_rate_tx_complete);

/*
 * Update rate-control state on station associate/reassociate.
 */
void
ath_rate_newassoc(struct ath_softc *sc, struct ath_node *an, int isnew, 
		  unsigned int capflag)
{
	struct ieee80211vap  *vap = an->an_node.ni_vap;


	if (isnew) {
		struct atheros_node *oan = ATH_NODE_ATHEROS(an);

		/* FIX ME:XXXX Looks like this not used at all.	*/
		oan->htcap = ((capflag & ATH_RC_DS_FLAG) ? WLAN_RC_DS_FLAG : 0) |
				 ((capflag & ATH_RC_SGI_FLAG) ? WLAN_RC_SGI_FLAG : 0) | 
				 ((capflag & ATH_RC_HT_FLAG)  ? WLAN_RC_HT_FLAG : 0) |
				 ((capflag & ATH_RC_CW40_FLAG) ? WLAN_RC_40_FLAG : 0);
	
		rcSibUpdate(sc, an, oan->htcap, 0);
		/* 
		 * Set an initial tx rate for the net80211 layer.
		 * Even though noone uses it, it wants to validate
		 * the setting before entering RUN state so if there
		 * was a pervious setting from a different node it
		 * may be invalid.
		 * Since we are using multirate we set one of the
		 * rate in multirate. 
		 */

		if (vap->iv_fixed_rate.mode != IEEE80211_FIXED_RATE_NONE)  {
			int mcs;	
			mcs = IEEE80211_RATE_IDX_ENTRY(vap->iv_fixed_rate.series, 0);
			an->an_node.ni_txrate = oan->rixMap[mcs] | IEEE80211_RATE_MCS;
		} else {
			an->an_node.ni_txrate  = 0;
		}
			an->an_node.ni_txrate  = 0;
	}
}
EXPORT_SYMBOL(ath_rate_newassoc);

static void
rate_cb(void *arg, struct ieee80211_node *ni)
{
	struct ath_softc *sc = arg;
	u_int32_t capflag = 0;
	struct ieee80211com *ic = &sc->sc_ic;

	if ((ni->ni_chwidth == IEEE80211_CWM_WIDTH40) && 
		(ic->ic_cwm.cw_width == IEEE80211_CWM_WIDTH40))
	{
		capflag |=  ATH_RC_CW40_FLAG;
	}
	if (ni->ni_htcap & IEEE80211_HTCAP_C_SHORTGI40) {
		capflag |=  ATH_RC_SGI_FLAG;
	}
	if (ni->ni_flags & IEEE80211_NODE_HT) {
		capflag |=  ATH_RC_HT_FLAG | ATH_RC_DS_FLAG;
	}
	ath_rate_newassoc(sc, ATH_NODE(ni), 1, capflag); 
}

/*
 * Update rate-control state on a device state change.  When
 * operating as a station this includes associate/reassociate
 * with an AP.  Otherwise this gets called, for example, when
 * the we transition to run state when operating as an AP.
 */

void
ath_rate_newstate(struct ieee80211vap *vap, enum ieee80211_state state)
{
	struct ieee80211com *ic = vap->iv_ic;
	struct ath_softc *sc = ic->ic_dev->priv;
	struct atheros_softc *asc = (struct atheros_softc *) sc->sc_rc;

	switch (sc->sc_ah->ah_magic) {
	case 0xdeadbeef:
	case 0x19641014:	/* 5416 */
		/* For half and quarter rate channles use different 
		 * rate tables 
		 */
		if (IEEE80211_IS_CHAN_HALF(ic->ic_curchan)) {
			ar5416SetHalfRateTable(asc);
		} else if (IEEE80211_IS_CHAN_QUARTER(ic->ic_curchan)) {
			ar5416SetQuarterRateTable(asc);
		} else { /* full rate */
			ar5416SetFullRateTable(asc);
		}
		break;
	default:		/* XXX 5210 */
		break;
	}

	if (state == IEEE80211_S_RUN) {
		u_int32_t capflag = 0;
		if (vap->iv_opmode != IEEE80211_M_STA) {
			/*
			 * Sync rates for associated stations and neighbors.
			 */
			ieee80211_iterate_nodes(&ic->ic_sta, rate_cb, sc);

			if (ic->ic_cwm.cw_width == IEEE80211_CWM_WIDTH40) {
				capflag |= ATH_RC_CW40_FLAG;
			}
		} else {
			if ((vap->iv_bss->ni_chwidth == IEEE80211_CWM_WIDTH40) && 
				(ic->ic_cwm.cw_width == IEEE80211_CWM_WIDTH40)) 
			{
				capflag |= ATH_RC_CW40_FLAG;
			}
		}
		if ((vap->iv_bss->ni_htcap & IEEE80211_HTCAP_C_SHORTGI40)) {
			capflag |= ATH_RC_SGI_FLAG;
		}
		if (IEEE80211_IS_CHAN_11N(ic->ic_bsschan)) {
			capflag |= (ATH_RC_HT_FLAG | ATH_RC_DS_FLAG);
		}

		ath_rate_newassoc(sc, ATH_NODE(vap->iv_bss), 1, capflag);
	}
}
EXPORT_SYMBOL(ath_rate_newstate);

void
atheros_setuptable(RATE_TABLE *rt)
{
}

/*
 * Module glue.
 */
static	char *dev_info = "ath_rate_atheros";

MODULE_AUTHOR("Video54 Technologies, Inc.");
MODULE_DESCRIPTION("Rate control support for Atheros devices");
#ifdef MODULE_LICENSE
MODULE_LICENSE("Proprietary");
#endif

static int __init
init_ath_rate_atheros(void)
{
	/* XXX version is a guess; where should it come from? */
	printk(KERN_INFO "%s: Version 2.0.1\n"
		"Copyright (c) 2001-2004 Atheros Communications, Inc, "
		"All Rights Reserved\n", dev_info);
	ar5416SetupRateTables();
	return 0;
}
module_init(init_ath_rate_atheros);

static void __exit
exit_ath_rate_atheros(void)
{
	printk(KERN_INFO "%s: driver unloaded\n", dev_info);
}
module_exit(exit_ath_rate_atheros);
