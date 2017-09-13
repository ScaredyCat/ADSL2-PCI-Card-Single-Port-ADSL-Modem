/*
 * Copyright (c) 2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 */
 
/*
 * CWM (Channel Width Management) 
 */
 

#include "opt_ah.h"

#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/cache.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/if_arp.h>

#include <asm/uaccess.h>

#include "if_ethersubr.h"		/* for ETHER_IS_MULTICAST */
#include "if_media.h"
#include "if_llc.h"

#include <net80211/ieee80211_radiotap.h>
#include <net80211/ieee80211_var.h>

#ifdef USE_HEADERLEN_RESV
#include <net80211/if_llc.h>
#endif

#define	AR_DEBUG
#include "if_athrate.h"
#include "net80211/if_athproto.h"
#include "if_athvar.h"
#include "ah_desc.h"
#include "ah_devid.h"			/* XXX to identify IBM cards */

#ifdef ATH_PCI		/* PCI BUS */
#include "if_ath_pci.h"
#endif			/* PCI BUS */
#ifdef ATH_AHB		/* AHB BUS */
#include "if_ath_ahb.h"
#endif			/* AHB BUS */

#include "if_ath_cwm.h"			/* Channel Width Management */

/*
 *----------------------------------------------------------------------
 * Debug support
 *----------------------------------------------------------------------
 */

#define	AR_CWM_DEBUG
    
#ifdef AR_CWM_DEBUG
#define	DPRINTF(sc, _fmt, ...) do {					\
	if (sc->sc_debug & 0x02000000)					\
		printk(_fmt, __VA_ARGS__);				\
} while (0)
#else
#define	DPRINTF(sc, _fmt, ...)
#endif

/*
 *----------------------------------------------------------------------
 * local definitions/macros
 *----------------------------------------------------------------------
 */

#define ATH_CWM_MAC_DISABLE_REQUEUE         /* disable requeuing for MAC 40=>20 transition */
#define ATH_CWM_PHY_DISABLE_TRANSITIONS     /* disable PHY transitions */

#define ATH_CWM_TIMER_INTERVAL		1	/* timer interval (seconds) */
#define ATH_CWM_TIMER_EXTCHSENSING	10	/* ext channel sensing enable/disable (seconds) */

/* CWM States */
enum ath_cwm_state {
	ATH_CWM_STATE_EXT_CLEAR,
	ATH_CWM_STATE_EXT_BUSY,
	ATH_CWM_STATE_EXT_UNAVAILABLE,
	ATH_CWM_STATE_MAX
};

/* CWM Events */
enum ath_cwm_event {
	ATH_CWM_EVENT_TXTIMEOUT,	/* tx timeout interrupt */
	ATH_CWM_EVENT_EXTCHCLEAR,	/* ext channel sensing - clear */
	ATH_CWM_EVENT_EXTCHBUSY,	/* ext channel sensing - busy */
	ATH_CWM_EVENT_EXTCHSTOP,	/* ext channel sensing - stop */
	ATH_CWM_EVENT_EXTCHRESUME,	/* ext channel sensing - resume */
	ATH_CWM_EVENT_DESTCW20,		/* dest channel width changed to 20  */
	ATH_CWM_EVENT_DESTCW40,		/* dest channel width changed to 40  */
	ATH_CWM_EVENT_MAX,
};

/* State transition */
struct ath_cwm_statetransition {
	enum ath_cwm_state	ct_newstate;			/* Output: new state */
	void (*ct_action)(struct ath_softc *, void *arg);  	/* Output: action */
};

/* CWM resources/state */
struct ath_cwm {
	struct timer_list	ac_timer;		/* CWM timer - monitor the extension channel */
	enum ath_cwm_state	ac_timer_prevstate;	/* CWM timer - prev state of last timer call */
	u_int8_t		ac_timer_statetime; 	/* CWM timer - time (sec) elapsed in current state */
	u_int8_t		ac_vextch; 	    	/* DBG virtual ext channel sensing enabled? */
	u_int8_t		ac_vextchbusy;	    	/* DBG virtual ext channel state */
	u_int32_t		ac_extchbusyper;	/* Last measurement of ext ch busy (percent) */

	/* State Machine */
	u_int8_t		ac_running;		/* CWM running */
	enum ath_cwm_state	ac_state;		/* CWM state */
	HAL_HT_CWM		ac_hwstate;		/* CWM hardware state */

	/* Debug Information */
};

/*
 *----------------------------------------------------------------------
 * local function declarations
 *----------------------------------------------------------------------
 */
static void cwm_attach(struct ath_softc *sc);
static void cwm_inithwstate(struct ieee80211_cwm *icw, struct ath_cwm *acw);
static void cwm_init(struct ath_softc *sc);
static void cwm_start(struct ath_softc *sc);
static void cwm_stop(struct ath_softc *sc);
static void cwm_statetransition(struct ath_softc *sc, enum ath_cwm_event event, void *arg);
static void cwm_timer(unsigned long data);
static int  cwm_extchbusy(struct ath_softc *sc);

static void cwm_action_invalid(struct ath_softc *sc, void *arg);
static void cwm_action_mac40to20(struct ath_softc *sc, void *arg);
static void cwm_action_mac20to40(struct ath_softc *sc, void *arg);
#ifndef ATH_CWM_PHY_DISABLE_TRANSITIONS
static void cwm_action_phy40to20(struct ath_softc *sc, void *arg);
#endif
static void cwm_action_phy20to40(struct ath_softc *sc, void *arg);
static void cwm_action_requeue(struct ath_softc *sc, void *arg);

static void cwm_sendactionmgmt(struct ath_softc *sc);
#ifndef ATH_CWM_MAC_DISABLE_REQUEUE
static int  cwm_queryfcc(struct ath_softc *sc);
static void cwm_stoptxdma(struct ath_softc *sc);
static void cwm_resumetxdma(struct ath_softc *sc);
#endif
static void cwm_rate_updatenode(struct ieee80211_node *ni);
static void cwm_rate_updateallnodes(struct ath_softc *sc);
static void cwm_debuginfo(struct ath_softc *sc);

/*
 *----------------------------------------------------------------------
 * static variables
 *----------------------------------------------------------------------
 */
static const char *ath_cwm_statename[ATH_CWM_STATE_MAX] = {
	"EXT CLEAR",		/* ATH_CWM_STATE_EXT_CLEAR */
	"EXT BUSY",		/* ATH_CWM_STATE_EXT_BUSY */
	"EXT UNAVAIL",		/* ATH_CWM_STATE_EXT_UNAVAILABLE */
};
   
static const char *ath_cwm_eventname[ATH_CWM_EVENT_MAX] = {
	"TXTIMEOUT", 		/* ATH_CWM_EVENT_TXTIMEOUT */
	"EXTCHCLEAR", 		/* ATH_CWM_EVENT_EXTCHCLEAR */
	"EXTCHBUSY", 		/* ATH_CWM_EVENT_EXTCHBUSY */
	"EXTCHSTOP", 		/* ATH_CWM_EVENT_EXTCHSTOP */
	"EXTCHRESUME", 		/* ATH_CWM_EVENT_EXTCHRESUME */
	"DESTCW20", 		/* ATH_CWM_EVENT_DESTCW20 */
	"DESTCW40", 		/* ATH_CWM_EVENT_DESTCW40 */
};

/*
 * State Transition Table 
 *
 * Input  (current state, event)
 * Output (new state, action)
 */
static const struct ath_cwm_statetransition ath_cwm_stt[ATH_CWM_STATE_MAX][ATH_CWM_EVENT_MAX] = {

	/* ATH_CWM_STATE_EXT_CLEAR */
	{
	/* ATH_CWM_EVENT_TXTIMEOUT	==> */ {ATH_CWM_STATE_EXT_BUSY,	 	cwm_action_mac40to20},
	/* ATH_CWM_EVENT_EXTCHCLEAR 	==> */ {ATH_CWM_STATE_EXT_CLEAR, 	NULL},
	/* ATH_CWM_EVENT_EXTCHBUSY 	==> */ {ATH_CWM_STATE_EXT_BUSY,  	cwm_action_mac40to20},
	/* ATH_CWM_EVENT_EXTCHSTOP 	==> */ {ATH_CWM_STATE_EXT_CLEAR, 	cwm_action_invalid},
	/* ATH_CWM_EVENT_EXTCHRESUME 	==> */ {ATH_CWM_STATE_EXT_CLEAR, 	cwm_action_invalid},
	/* ATH_CWM_EVENT_DESTCW20 	==> */ {ATH_CWM_STATE_EXT_CLEAR, 	cwm_action_requeue},
	/* ATH_CWM_EVENT_DESTCW40 	==> */ {ATH_CWM_STATE_EXT_CLEAR, 	NULL},
	},

	/* ATH_CWM_STATE_EXT_BUSY */
	{
	/* ATH_CWM_EVENT_TXTIMEOUT	==> */ {ATH_CWM_STATE_EXT_BUSY, 	NULL},
	/* ATH_CWM_EVENT_EXTCHCLEAR 	==> */ {ATH_CWM_STATE_EXT_CLEAR, 	cwm_action_mac20to40},
	/* ATH_CWM_EVENT_EXTCHBUSY 	==> */ {ATH_CWM_STATE_EXT_BUSY, 	NULL},
#ifdef ATH_CWM_PHY_DISABLE_TRANSITIONS
	/* ATH_CWM_EVENT_EXTCHSTOP  	==> */ {ATH_CWM_STATE_EXT_BUSY, 	NULL},
#else
	/* ATH_CWM_EVENT_EXTCHSTOP  	==> */ {ATH_CWM_STATE_EXT_UNAVAILABLE, 	cwm_action_phy40to20},
#endif    
	/* ATH_CWM_EVENT_EXTCHRESUME 	==> */ {ATH_CWM_STATE_EXT_BUSY, 	cwm_action_invalid},
	/* ATH_CWM_EVENT_DESTCW20 	==> */ {ATH_CWM_STATE_EXT_BUSY, 	NULL},
	/* ATH_CWM_EVENT_DESTCW40 	==> */ {ATH_CWM_STATE_EXT_BUSY, 	NULL},
	},

	/* ATH_CWM_STATE_EXT_UNAVAILABLE */
	{
	/* ATH_CWM_EVENT_TXTIMEOUT	==> */ {ATH_CWM_STATE_EXT_UNAVAILABLE, 	NULL},
	/* ATH_CWM_EVENT_EXTCHCLEAR 	==> */ {ATH_CWM_STATE_EXT_UNAVAILABLE, 	cwm_action_invalid},
	/* ATH_CWM_EVENT_EXTCHBUSY 	==> */ {ATH_CWM_STATE_EXT_UNAVAILABLE, 	cwm_action_invalid},
	/* ATH_CWM_EVENT_EXTCHSTOP 	==> */ {ATH_CWM_STATE_EXT_UNAVAILABLE, 	cwm_action_invalid},
	/* ATH_CWM_EVENT_EXTCHRESUME 	==> */ {ATH_CWM_STATE_EXT_BUSY, 	cwm_action_phy20to40},
	/* ATH_CWM_EVENT_DESTCW20 	==> */ {ATH_CWM_STATE_EXT_UNAVAILABLE, 	NULL},
	/* ATH_CWM_EVENT_DESTCW40 	==> */ {ATH_CWM_STATE_EXT_BUSY, 	cwm_action_phy20to40},
	}
};

/*
 *----------------------------------------------------------------------
 * public functions
 *----------------------------------------------------------------------
 */
/*
 * Device Attach
 */
int 
ath_cwm_attach(struct ath_softc *sc)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct net_device 	*dev 	= ic->ic_dev;
	struct ieee80211_cwm 	*icw	= &ic->ic_cwm;
	struct ath_cwm 		*acw;

	DPRINTF(sc, "%s\n", __func__);

	acw = kmalloc(sizeof(struct ath_cwm), GFP_KERNEL);
	if (acw == NULL) {
		printk("%s: no memory for cwm attach\n", __func__);
		return ENOMEM;
	}

	memset(acw, 0, sizeof(struct ath_cwm));
	sc->sc_cwm = acw;

	/* ieee80211 Layer - Default Configuration */
	icw->cw_mode 		= IEEE80211_CWM_MODE20;
	icw->cw_extoffset 	= 0;
	icw->cw_extprotmode	= IEEE80211_CWM_EXTPROTNONE;
	icw->cw_extprotspacing 	= IEEE80211_CWM_EXTPROTSPACING25;
	icw->cw_enable      	= 1;
	icw->cw_extbusythreshold = ATH_CWM_EXTCH_BUSY_THRESHOLD;

	/* Initialize state machine */
	cwm_attach(sc);
				       
	/* Allocate resources */
	init_timer(&acw->ac_timer);
	acw->ac_timer.function = cwm_timer;
	acw->ac_timer.data = (unsigned long) dev;

	return 0;
}

/*
 * Device Detach
 */
void
ath_cwm_detach(struct ath_softc *sc)
{
	struct ath_cwm 		*acw	= sc->sc_cwm;

	DPRINTF(sc, "%s\n", __func__);
	
	if (acw == NULL) {
		printk("%s: error - acw NULL. Possible attach failure\n", __func__);
		return;
	}

	/* Cleanup resources */
	del_timer(&acw->ac_timer);

	/* free CWM memory */
	kfree(acw);
	sc->sc_cwm = NULL;
}

/*
 * Device Init
 */
void
ath_cwm_init(struct ath_softc *sc)
{
	struct ath_cwm 		*acw	= sc->sc_cwm;

	DPRINTF(sc, "%s\n", __func__);
	
        if (acw == NULL) {
		printk("%s: error - acw NULL. Possible attach failure\n", __func__);
		return;
	}

	cwm_init(sc);
}

/*
 * Device Stop
 */
void
ath_cwm_stop(struct ath_softc *sc)
{
	struct ath_cwm 		*acw	= sc->sc_cwm;

	DPRINTF(sc, "%s\n", __func__);
	
        if (acw == NULL) {
		printk("%s: error - acw NULL. Possible attach failure\n", __func__);
		return;
	}

	cwm_stop(sc);
}
/*
 * New State (Virtual AP)
 */
void
ath_cwm_newstate(struct ieee80211vap *vap, enum ieee80211_state nstate)
{
	struct ieee80211com 	*ic  = vap->iv_ic;
	struct net_device   	*dev = ic->ic_dev;
	struct ath_softc    	*sc  = dev->priv;
	struct ath_cwm 		*acw = sc->sc_cwm;
	struct ieee80211_cwm 	*icw = &ic->ic_cwm;

        if (acw == NULL) {
		printk("%s: error - acw NULL. Possible attach failure\n", __func__);
		return;
	}

	DPRINTF(sc, "%s: vap%d: %s -> %s\n", __func__, vap->iv_unit,
		ieee80211_state_name[vap->iv_state],
		ieee80211_state_name[nstate]);

	switch (nstate) {
	case IEEE80211_S_INIT:		/* default state */
		switch (vap->iv_opmode) {
		case IEEE80211_M_HOSTAP:
		case IEEE80211_M_STA:
			if (ic->ic_nrunning == 0) {
				cwm_stop(sc);
				cwm_init(sc);
			}
			break;
		default:
			break;
		}
		break;
	case IEEE80211_S_SCAN:    	/* scanning */
		switch (vap->iv_opmode) {
		case IEEE80211_M_HOSTAP:
		case IEEE80211_M_STA:
			/* 20 MHz mode when scanning */
			acw->ac_hwstate.ht_macmode = HAL_HT_MACMODE_20;
			acw->ac_hwstate.ht_phymode = HAL_HT_PHYMODE_20;
			acw->ac_hwstate.ht_extoff  = 0;
			
			/* stop CWM state machine */
			cwm_stop(sc);
			break;
		default:
			break;
		}
		break;
	case IEEE80211_S_JOIN:		/* try to join */
		/* channel information is now valid */
		if ((icw->cw_mode == IEEE80211_CWM_MODE2040) ||
		    (icw->cw_mode == IEEE80211_CWM_MODE40)) {
			if (ath_cwm_ht40allowed(sc)) {
				/* set channel width to 40 MHz. 
				 * extension offset is assumed to be set correctly
				 * by config/auto channel select
				 */
				icw->cw_width = IEEE80211_CWM_WIDTH40;
			} else {
				DPRINTF(sc, "%s: Regulatory does not allow 40MHz.\n", __func__);
				icw->cw_width = IEEE80211_CWM_WIDTH20;
				icw->cw_extoffset = 0;
			}
			/* init hardware state */
			cwm_inithwstate(icw, acw);
		}
		break;
	case IEEE80211_S_AUTH:    	/* try to authenticate */
		break;
	case IEEE80211_S_ASSOC:    	/* try to assoc */
		break;
	case IEEE80211_S_RUN:      	/* associated */
		switch (vap->iv_opmode) {
		case IEEE80211_M_HOSTAP:
		case IEEE80211_M_STA:
			cwm_start(sc);
			break;
		default:
			break;
		}
		break;
	default:
		DPRINTF(sc, "%s: unrecognized state %d\n", __func__, nstate);
		break;
	}
}

/*
 * Atheros layer ioctl.
 *
 * Used to debug CWM state machine by triggering events
 *
 */
int
ath_cwm_ioctl(struct ath_softc *sc, int cmd, caddr_t data)
{
	struct ieee80211com 	*ic  = &sc->sc_ic;
	struct ath_cwm 	   	*acw = sc->sc_cwm;
	struct ath_hal 		*ah  = sc->sc_ah;
	enum ath_cwm_event	event;
	u_int32_t		value;
	struct ath_cwmdbg 	*dc;
	struct ath_cwminfo 	*ci;
	HAL_HT_CWM 		ht;


	DPRINTF(sc, "%s\n", __func__);

        if (acw == NULL) {
		printk("%s: error - acw NULL. Possible attach failure\n", __func__);
		return -EINVAL;
	}

	switch (cmd) {
	case SIOCGATHCWMDBG:
		if (!acw->ac_running) {
			DPRINTF(sc, "%s: cwm fsm not running\n", __func__);
			return -EINVAL;
		}
		dc = (struct ath_cwmdbg *) data;
		switch (dc->dc_cmd) {
		case ATH_DBGCWM_CMD_EVENT:
			event = dc->dc_arg;
			if (event < ATH_CWM_EVENT_MAX) {
				DPRINTF(sc, "%s: event %s\n", __func__, ath_cwm_eventname[event]);
				cwm_statetransition(sc, event, NULL);
				return 0;
			} else {
				DPRINTF(sc, "%s: invalid event %d\n", __func__, event);
				return -EINVAL;
			}
			break;
		case ATH_DBGCWM_CMD_CTL:
			/* Owl 2.0 only */
			value = ath_hal_get11nRxClear(ah);
			if (dc->dc_arg) {
				value |= HAL_RX_CLEAR_CTL_LOW;
			} else {
				value &= ~HAL_RX_CLEAR_CTL_LOW;
			}
			ath_hal_set11nRxClear(ah, value);
			return 0;
			break;
		case ATH_DBGCWM_CMD_EXT:
			/* Owl 2.0 only */
			value = ath_hal_get11nRxClear(ah);
			if (dc->dc_arg) {
				value |= HAL_RX_CLEAR_EXT_LOW;
			} else {
				value &= ~HAL_RX_CLEAR_EXT_LOW;
			}
			ath_hal_set11nRxClear(ah, value);
			return 0;
			break;
		case ATH_DBGCWM_CMD_VEXT:
			acw->ac_vextch = 1;
			acw->ac_vextchbusy = dc->dc_arg;
			return 0;
			break;
		default:
			DPRINTF(sc, "%s: invalid SIOCGATHCWMDBG command %d\n", __func__, dc->dc_cmd);
			return -EINVAL;
			break;
		}
		break;
	case SIOCGATHCWMINFO:
		ci = (struct ath_cwminfo *) data;
		ci->ci_chwidth = ic->ic_cwm.cw_width;
		memset(&ht, 0, sizeof(ht));
		ath_cwm_gethwstate(sc, &ht);
		ci->ci_macmode = ht.ht_macmode;
		ci->ci_phymode = ht.ht_phymode;
		ci->ci_extbusyper = acw->ac_extchbusyper;
		return 0;
		break;
	default:
		DPRINTF(sc, "%s: invalid command %d\n", __func__, cmd);
		return -EINVAL;
		break;
	}
}

/*
 * Node has changed channel width 
 *
 */
void
ath_cwm_newchwidth(struct ieee80211_node *ni)
{
	struct ath_softc 	*sc 	= ni->ni_ic->ic_dev->priv;
	struct ath_cwm 		*acw	= sc->sc_cwm;
	enum ath_cwm_event	event;
	
	DPRINTF(sc, "%s: chwidth = %d\n", __func__, ni->ni_chwidth);

        if (acw == NULL) {
		printk("%s: error - acw NULL. Possible attach failure\n", __func__);
		return;
	}

	if (!acw->ac_running) {
	      return;
	}

	event = (ni->ni_chwidth == IEEE80211_CWM_WIDTH40) ?
		 ATH_CWM_EVENT_DESTCW40 : ATH_CWM_EVENT_DESTCW20;
	cwm_statetransition(sc, event, ni);

	/* update node's rate table */
	cwm_rate_updatenode(ni); 
}

 /*
  * get ext channel busy (percentage) 
  */
 u_int32_t
 ath_cwm_getextbusy(struct ath_softc *sc)
 {
	struct ath_cwm 	*acw = sc->sc_cwm;
 
	if (acw == NULL) {
		printk("%s: error - acw NULL. Possible attach failure\n", __func__);
		return 0;
	}
 
	return acw->ac_extchbusyper;
 }

/*
 * Tx timeout interrupt 
 *
 */
void
ath_cwm_txtimeout(struct ath_softc *sc)
{
	struct ath_cwm 		*acw	= sc->sc_cwm;
	
	DPRINTF(sc, "%s\n", __func__);

        if (acw == NULL) {
		printk("%s: error - acw NULL. Possible attach failure\n", __func__);
		return;
	}

	if (!acw->ac_running) {
	      return;
	}

	cwm_statetransition(sc, ATH_CWM_EVENT_TXTIMEOUT, NULL);
}

/*
 * get hw state 
 *
 */
void
ath_cwm_gethwstate(struct ath_softc *sc, HAL_HT_CWM *cwm)
{
	struct ath_cwm 		*acw	= sc->sc_cwm;

        if (acw == NULL) {
		printk("%s: error - acw NULL. Possible attach failure\n", __func__);
		return;
	}

	*cwm = acw->ac_hwstate;
}

/*
 * HT40 allowed?
 * - check device
 * - check regulatory (channel flags)
 */
int
ath_cwm_ht40allowed(struct ath_softc *sc) 
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;

	DPRINTF(sc, "%s: IC chfreq %d, chflags 0x%X\n", __func__, 
		ic->ic_curchan->ic_freq, ic->ic_curchan->ic_flags);
	DPRINTF(sc, "%s: SC chfreq %d, chflags 0x%X\n", __func__, 
		sc->sc_curchan.channel,  sc->sc_curchan.channelFlags);

	if ((ic->ic_htcap & IEEE80211_HTCAP_C_CHWIDTH40) &&
	    IEEE80211_IS_CHAN_11N_CTL_40_CAPABLE(ic->ic_curchan)) {
		return 1;
	} else {
		return 0;
	}   
}

/*
 *----------------------------------------------------------------------
 * local functions
 *----------------------------------------------------------------------
 */

/*
 * CWM Attach
 *
 * - initialize state machine according to default settings
 * - called by ath_attach() 
 *
 */
static void
cwm_attach(struct ath_softc *sc)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct ieee80211_cwm 	*icw	= &ic->ic_cwm;
	struct ath_cwm 		*acw	= sc->sc_cwm;

	DPRINTF(sc, "%s\n", __func__);

	/* ieee80211 layer initialization */
	icw->cw_width = IEEE80211_CWM_WIDTH20;

	/* Atheros layer initialization */
	acw->ac_running 	= 0;
	acw->ac_timer_statetime = 0;
	acw->ac_timer_prevstate = ATH_CWM_STATE_EXT_CLEAR;
	acw->ac_vextch 		= 0;
	acw->ac_vextchbusy 	= 0;
	acw->ac_extchbusyper 	= 0;

	/* default state */
	acw->ac_state  		= ATH_CWM_STATE_EXT_CLEAR;
	cwm_inithwstate(icw, acw);
}

/*
 * CWM init hardware state based on current configuration
 *
 */
static void 
cwm_inithwstate(struct ieee80211_cwm *icw, struct ath_cwm *acw)
{
	/* mac and phy mode */
	switch (icw->cw_width) {
	case IEEE80211_CWM_WIDTH40:
		acw->ac_hwstate.ht_macmode = HAL_HT_MACMODE_2040;
		acw->ac_hwstate.ht_phymode = HAL_HT_PHYMODE_2040;
		/* extension channel offset */
		acw->ac_hwstate.ht_extoff = icw->cw_extoffset;
		break;

	case IEEE80211_CWM_WIDTH20:
	default:
		acw->ac_hwstate.ht_macmode = HAL_HT_MACMODE_20;
		acw->ac_hwstate.ht_extoff = icw->cw_extoffset;
		if (acw->ac_hwstate.ht_extoff == 0) {
		    acw->ac_hwstate.ht_phymode = HAL_HT_PHYMODE_20;
		} else {
		    acw->ac_hwstate.ht_phymode = HAL_HT_PHYMODE_2040;
		}
		break;
	}

	/* extension channel protection spacing */				   
	switch (icw->cw_extprotspacing) {
	case IEEE80211_CWM_EXTPROTSPACING20:
		acw->ac_hwstate.ht_extprotspacing = HAL_HT_EXTPROTSPACING_20;
		break;
	case IEEE80211_CWM_EXTPROTSPACING25:
	default:
		acw->ac_hwstate.ht_extprotspacing = HAL_HT_EXTPROTSPACING_25;
		break;
	}
}

/*
 * CWM Init
 *
 * - initialize state machine based on current configuration
 * - called by ath_init()
 *
 */
static void
cwm_init(struct ath_softc *sc)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct ieee80211_cwm 	*icw	= &ic->ic_cwm;

	DPRINTF(sc, "%s\n", __func__);

 	/* Validate configuration */
	switch (icw->cw_mode) {
	case IEEE80211_CWM_MODE20:
		if (icw->cw_extoffset != 0) {
			DPRINTF(sc, "%s: Invalid configuration. Forcing extoffset to 0\n", __func__);
			icw->cw_extoffset = 0;
		}
		icw->cw_width = IEEE80211_CWM_WIDTH20;
                icw->cw_enable = 0;
		/* device not capable of 40 MHz */
		ic->ic_htcap &= ~IEEE80211_HTCAP_C_CHWIDTH40;
		break;
	case IEEE80211_CWM_MODE40:
		if (icw->cw_extoffset == 0) {
			DPRINTF(sc, "%s: Invalid configuration. Forcing extoffset to 1\n", __func__);
			icw->cw_extoffset = 1;
		}
		icw->cw_width = IEEE80211_CWM_WIDTH40;
		icw->cw_enable = 0;
		break;
	case IEEE80211_CWM_MODE2040:
		if (icw->cw_extoffset == 0) {
			DPRINTF(sc, "%s: Invalid configuration. Forcing extoffset to 1\n", __func__);
			icw->cw_extoffset = 1;
		}
		icw->cw_width = IEEE80211_CWM_WIDTH40;
		break;
	default:
		DPRINTF(sc, "%s: Invalid cwm mode. Forcing to 20MHz only\n", __func__);
		icw->cw_mode = IEEE80211_CWM_MODE20;
		icw->cw_extoffset = 0;
		icw->cw_width = IEEE80211_CWM_WIDTH20;
		icw->cw_enable = 0;
		break;
	}

	/* Display configuration */
	DPRINTF(sc, "%s: cw_mode %d\n", __func__, icw->cw_mode);
	DPRINTF(sc, "%s: cw_extoffset %d\n", __func__, icw->cw_extoffset);
	DPRINTF(sc, "%s: cw_extprotmode %d\n", __func__, icw->cw_extprotmode);
	DPRINTF(sc, "%s: cw_extprotspacing %d\n", __func__, icw->cw_extprotspacing);
	DPRINTF(sc, "%s: cw_enable %d\n", __func__, icw->cw_enable);
}

/*
 * CWM Start
 */
static void
cwm_start(struct ath_softc *sc)
{
	struct ieee80211com 	*ic	= &sc->sc_ic;
	struct ieee80211_cwm 	*icw	= &ic->ic_cwm;
	struct ath_cwm 		*acw	= sc->sc_cwm;

	DPRINTF(sc, "%s\n", __func__);

	if (acw->ac_running) {
		return;
	}

	if (!icw->cw_enable) {
		DPRINTF(sc, "%s: CWM state machine disabled via configuration\n", __func__);
		return;
	}

	if(!ath_cwm_ht40allowed(sc)) {
		DPRINTF(sc, "%s: CWM state machine disabled because of regulatory restrictions\n", __func__);
		return;
	}

       	/* Start CWM */
	acw->ac_running = 1;
	cwm_debuginfo(sc);
	mod_timer(&acw->ac_timer, jiffies + (ATH_CWM_TIMER_INTERVAL * HZ));
}


/*
 * CWM Stop
 */
static void
cwm_stop(struct ath_softc *sc)
{
	struct ath_cwm 		*acw	= sc->sc_cwm;

	DPRINTF(sc, "%s\n", __func__);

	if (acw->ac_running) {
		acw->ac_running = 0;
		del_timer(&acw->ac_timer);
	}
}


/*
 * CWM State Transition
 *
 * Assumptions: always called from tasklet context
 *		Code currently not SMP safe. 
 * 
 */
static void
cwm_statetransition(struct ath_softc *sc, enum ath_cwm_event event, void *arg)
{
	struct ath_cwm 				*acw = sc->sc_cwm;
	struct ieee80211com 			*ic = &sc->sc_ic;
	const struct ath_cwm_statetransition 	*st;
	static const struct ath_cwm_statetransition apst = {ATH_CWM_STATE_EXT_UNAVAILABLE,
						            NULL}; /* special case for AP */

	KASSERT(acw->ac_running, ("%s: event received but cwm fsm not running\n", __func__));
	if(!acw->ac_running) {
		return;
	}

        KASSERT(event < ATH_CWM_EVENT_MAX, ("%s: invalid event %d\n", __func__, event));

	/* Input:  current state, event 
	 * Output: new state, action    
	 */
	st =  &ath_cwm_stt[acw->ac_state][event];

	/* A little hacky, but avoids having a separate state transition table for the AP */
	if ((ic->ic_opmode == IEEE80211_M_HOSTAP) && (acw->ac_state == ATH_CWM_STATE_EXT_UNAVAILABLE) 
	    && (event == ATH_CWM_EVENT_DESTCW40)) {
		st = &apst;
	}

	DPRINTF(sc, "%s: Event %s. State %s => %s\n", __func__,
	       ath_cwm_eventname[event], ath_cwm_statename[acw->ac_state], ath_cwm_statename[st->ct_newstate]);

	/* update state */
	acw->ac_state = st->ct_newstate;

	/* associated action (if any) */
	if (st->ct_action != NULL) {
		st->ct_action(sc, arg);
	} else {
		DPRINTF(sc, "%s: no associated action\n", __func__);
	}
}



/*
 * CWM Timer
 * 
 * - monitor the extension channel 
 * - generate CWM events based on extension channel activity
 *
 */
static void
cwm_timer(unsigned long data)
{
	struct net_device	*dev 	= (struct net_device *)data;
	struct ath_softc    	*sc 	= dev->priv;
	struct ath_cwm 		*acw	= sc->sc_cwm;
	int			extchbusy = 0, persistentstate = 0;

#if 0
	DPRINTF(sc, "%s\n", __func__);
#endif

	if(!acw->ac_running) {
		return;
	}

	/* monitor extension channel */
	if (acw->ac_state != ATH_CWM_STATE_EXT_UNAVAILABLE) {
		extchbusy = cwm_extchbusy(sc);
	}

	/* check for same state for a period of time */
	if (acw->ac_state == acw->ac_timer_prevstate) {
		acw->ac_timer_statetime++;
		if (acw->ac_timer_statetime == ATH_CWM_TIMER_EXTCHSENSING) {
			persistentstate = 1;
		}
	} else {
		/* reset counter */
		acw->ac_timer_statetime = 0;
		acw->ac_timer_prevstate = acw->ac_state;
	}

        switch (acw->ac_state) {
	case ATH_CWM_STATE_EXT_CLEAR:
		if (extchbusy) {
			cwm_statetransition(sc, ATH_CWM_EVENT_EXTCHBUSY, NULL);
		}
		break;
	case ATH_CWM_STATE_EXT_BUSY:
		if (extchbusy) {
			if (persistentstate) {
				cwm_statetransition(sc, ATH_CWM_EVENT_EXTCHSTOP, NULL);
			}
		} else {
			cwm_statetransition(sc, ATH_CWM_EVENT_EXTCHCLEAR, NULL);
		}
		break;
	case ATH_CWM_STATE_EXT_UNAVAILABLE:
		if (persistentstate) {
			cwm_statetransition(sc, ATH_CWM_EVENT_EXTCHRESUME, NULL);
		}
		break;
	default:
		DPRINTF(sc, "%s: invalid state, %d\n", __func__, acw->ac_state);
		break;

	}
	/* schedule timer */
	mod_timer(&acw->ac_timer, jiffies + (ATH_CWM_TIMER_INTERVAL * HZ));
}

/*
 * CWM Extension Channel Busy Check
 * 
 * - return 	1 if extension channel busy,
 *		0 otherwise
 */
static int
cwm_extchbusy(struct ath_softc *sc)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct ieee80211_cwm 	*icw	= &ic->ic_cwm;
	struct ath_cwm 		*acw	= sc->sc_cwm;
	int 			busy 	= 0;

	/* debugging - virtual extension channel sensing */
	if (acw->ac_vextch) {
		return acw->ac_vextchbusy;
	}

	/* Extension Channel busy (0-100%) */
	acw->ac_extchbusyper = ath_hal_get11nextbusy(sc->sc_ah);
	if (acw->ac_extchbusyper > icw->cw_extbusythreshold) {
		busy = 1;
	}

	return busy;
}

/*
 * Actions
 */

/*
 * Action: Invalid state transition
 */
static void
cwm_action_invalid(struct ath_softc *sc, void *arg)
{
	DPRINTF(sc, "%s\n", __func__);
}

/*
 * Action: switch MAC from 40 to 20
 */
static void
cwm_action_mac40to20(struct ath_softc *sc, void *arg)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct ieee80211_cwm 	*icw	= &ic->ic_cwm;
	struct ath_cwm 		*acw	= sc->sc_cwm;
#ifndef ATH_CWM_MAC_DISABLE_REQUEUE
	int			ac;
	struct ath_txq      	*txq;
#endif

	DPRINTF(sc, "%s\n", __func__);

	sc->sc_stats.ast_cwm_mac++;

#ifdef ATH_CWM_MAC_DISABLE_REQUEUE
	/* set channel width */
	icw->cw_width = IEEE80211_CWM_WIDTH20;

	/* set MAC to 20 MHz */
	acw->ac_hwstate.ht_macmode = HAL_HT_MACMODE_20;
	ath_hal_set11nmac2040(sc->sc_ah, HAL_HT_MACMODE_20);

	/* notify rate control of new mode (select new rate table) */
	cwm_rate_updateallnodes(sc);
#else
	/* stop MAC */
	cwm_stoptxdma(sc);

	/*
	 * first complete all packets in h/w queue
	 * mark incomplete packets as sw filtered
	 */
	for (ac = WME_AC_BE; ac <= WME_AC_VO; ac++) {
		txq = sc->sc_ac2q[ac];
		owl_tx_processq(sc, txq, OWL_TXQ_FILTERED);
	}

	/* set channel width */
	icw->cw_width = IEEE80211_CWM_WIDTH20;

	/* set MAC to 20 MHz */
	acw->ac_hwstate.ht_macmode = HAL_HT_MACMODE_20;
	ath_hal_set11nmac2040(sc->sc_ah, HAL_HT_MACMODE_20);

	/* notify rate control of new mode (select new rate table) */
	cwm_rate_updateallnodes(sc);

	/* 
	 * Re-queue/re-aggregate packets as 20 MHz 
	 */
	for (ac = WME_AC_BE; ac <= WME_AC_VO; ac++) {
		txq = sc->sc_ac2q[ac];
		owl_tx_requeue(sc, txq);
	}

	/* Resume MAC */
	cwm_resumetxdma(sc);
#endif //ATH_CWM_MAC_DISABLE_REQUEUE

	/* all virtual APs - send ch width action management frame */
	cwm_sendactionmgmt(sc);
   
}

/*
 * Action: switch MAC from 20 to 40
 */
static void
cwm_action_mac20to40(struct ath_softc *sc, void *arg)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct ieee80211_cwm 	*icw	= &ic->ic_cwm;
	struct ath_cwm 		*acw	= sc->sc_cwm;

	DPRINTF(sc, "%s\n", __func__);

	sc->sc_stats.ast_cwm_mac++;

	/* No need to requeue existing frames on hardware queue
	 * This avoids stopping the hardware queue and re-queuing 
	 */

	/* set channel width */
	icw->cw_width = IEEE80211_CWM_WIDTH40;

	/* set MAC to 40 MHz */
	acw->ac_hwstate.ht_macmode = HAL_HT_MACMODE_2040;
	ath_hal_set11nmac2040(sc->sc_ah, HAL_HT_MACMODE_2040);

	/* notify rate control of new mode (select new rate table) */
	cwm_rate_updateallnodes(sc);

	/* all virtual APs - send ch width action management frame */
	cwm_sendactionmgmt(sc);
}

#ifndef ATH_CWM_PHY_DISABLE_TRANSITIONS
/*
 * Action: switch PHY from 40 to 20 
 */
static void
cwm_action_phy40to20(struct ath_softc *sc, void *arg)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct ath_cwm 		*acw	= sc->sc_cwm;

	DPRINTF(sc, "%s\n", __func__);

	sc->sc_stats.ast_cwm_phy++;

	/* PHY 20 MHz */
	acw->ac_hwstate.ht_phymode = HAL_HT_PHYMODE_20;
	acw->ac_hwstate.ht_extoff  = 0;

	/* fast channel change logic used to quickly switch phy 20/40 mode */
	ath_chan_set(sc, ic->ic_curchan);

	/* all virtual APs - re-send ch width action management frame */
	cwm_sendactionmgmt(sc);
}
#endif /* #ifndef ATH_CWM_PHY_DISABLE_TRANSITIONS */

/*
 * Action: switch PHY from 20 to 40 
 */
static void
cwm_action_phy20to40(struct ath_softc *sc, void *arg)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct ath_cwm 		*acw	= sc->sc_cwm;
	struct ieee80211_cwm 	*icw = &ic->ic_cwm;

	DPRINTF(sc, "%s\n", __func__);


	sc->sc_stats.ast_cwm_phy++;

	/* PHY 20/40 MHz */
	acw->ac_hwstate.ht_phymode = HAL_HT_PHYMODE_2040;
	acw->ac_hwstate.ht_extoff  = icw->cw_extoffset;

	/* fast channel change logic used to quickly switch phy 20/40 mode */
	ath_chan_set(sc, ic->ic_curchan);
}
       
/*
 * Action: filter destination and requeue
 */
static void
cwm_action_requeue(struct ath_softc *sc, void *arg)
{
	struct ieee80211_node 	*ni = (struct ieee80211_node *) arg;
#ifndef ATH_CWM_MAC_DISABLE_REQUEUE
	int 		      	ac;
	struct ath_txq      	*txq;
	int			requeue[WME_AC_VO + 1];
#endif

	DPRINTF(sc, "%s\n", __func__);
	KASSERT(ni != NULL, ("%s: error node is null\n", __func__));

	sc->sc_stats.ast_cwm_requeue++;

	/* destination node channel width changed
	 * requeue tx frames with updated node's channel width setting
	 */

#ifdef ATH_CWM_MAC_DISABLE_REQUEUE
	/* update node's rate table */
	cwm_rate_updatenode(ni);
#else
	/* requeueing only needed if destination node has frames
	 * on _any_ hardware queue 
	 */
	if (ATH_NODE_HWQ_ACTIVE(ni)) {
		/* stop hw tx */
		cwm_stoptxdma(sc);

		/*
		 * first complete all packets in h/w queue
		 * mark incomplete packets as sw filtered
		 */
		for (ac = WME_AC_BE; ac <= WME_AC_VO; ac++) {
			/* check if destination node has frames on _this_ hardware queue */
			if (ATH_NODE_AC_ACTIVE(ni, ac)) {
				txq = sc->sc_ac2q[ac];
				owl_tx_processq(sc, txq, OWL_TXQ_FILTERED);
				requeue[ac] = 1;
			} else {
				requeue[ac] = 0;
			}
		}

		/* update node's rate table */
		cwm_rate_updatenode(ni);

		/* requeue frames */
		for (ac = WME_AC_BE; ac <= WME_AC_VO; ac++) {
			/* check if destination node has frames on _this_ hardware queue */
			if (requeue[ac]) {
				txq = sc->sc_ac2q[ac];
				owl_tx_requeue(sc, txq);
			}
		}
		/* restart hw tx */
		cwm_resumetxdma(sc);
	} else {
		/* update node's rate table */
		cwm_rate_updatenode(ni);
	}
#endif  //ATH_CWM_MAC_DISABLE_REQUEUE
}


/*
 * Helper functions
 */

/*
 * Send ch width action management frame 
 */
static void
cwm_sendactionmgmt(struct ath_softc *sc)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct ieee80211vap 	*vap;
	struct ieee80211_node   *ni;
	struct ieee80211_action_mgt_args actionargs;

       /* all virtual APs - send ch width action management frame */
	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		struct net_device 	*dev 	= vap->iv_dev;

		if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
			/* create temporary node for broadcast */
			ni = ieee80211_dup_bss(vap, dev->broadcast);
		} else {
			ni = vap->iv_bss;
		}

		/* send channel width action frame */
		if (ni != NULL) {
			actionargs.category	= IEEE80211_ACTION_CAT_HT;
			actionargs.action	= IEEE80211_ACTION_HT_TXCHWIDTH;
			actionargs.arg1		= 0;
			actionargs.arg2		= 0;
			actionargs.arg3		= 0;
			IEEE80211_SEND_MGMT(ni, IEEE80211_FC0_SUBTYPE_ACTION, (int) &actionargs);
			if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
				/* temporary node - decrement reference count so that the node will be 
				 * automatically freed upon completion */
				ieee80211_free_node(ni);
			}
		}
	 }
}

#ifndef ATH_CWM_MAC_DISABLE_REQUEUE
/*
 * Fast Channel Change Allowed?
 */
 static int cwm_queryfcc(struct ath_softc *sc)
 {
	struct ieee80211com 	*ic 	= &sc->sc_ic;

	if (ic->ic_flags_ext & IEEE80211_FAST_CC) {
		return 1;
	} else {
		return 0;
	}
 }

/*
 * Stop tx DMA (fast).  Does not reset or channel change
 */
static void
cwm_stoptxdma(struct ath_softc *sc)
{
	struct ath_hal 		*ah 	= sc->sc_ah;
        
	/* disable interrupts */
	ath_hal_intrset(ah, 0);     	

        if (cwm_queryfcc(sc)) {
		/* fast tx abort */
		if (!ath_hal_aborttxdma(sc->sc_ah)) {
			printk("%s: unable to abort tx dma\n", __func__);
		}
	} else {
		int i;
		for (i = 0; i < HAL_NUM_TX_QUEUES; i++)
		    if (ATH_TXQ_SETUP(sc, i))
			ath_hal_stoptxdma(ah, sc->sc_txq[i].axq_qnum);
	}
}

/*
 * Resume tx DMA 
 */
static void
cwm_resumetxdma(struct ath_softc *sc)
{
	struct ath_hal 		*ah 	= sc->sc_ah;

        /* Re-enable interrupts */
        ath_hal_intrset(ah, sc->sc_imask);
}
#endif //#ifndef ATH_CWM_MAC_DISABLE_REQUEUE

/*
 * Update node's rate table 
 */
static void
cwm_rate_updatenode(struct ieee80211_node *ni) 
{
	struct ieee80211com 	*ic 	= ni->ni_ic;
	struct ath_softc 	*sc 	= ni->ni_ic->ic_dev->priv;
	u_int32_t 		capflag = 0;
        
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
 * Update all associated nodes and VAPs
 *
 * Called when local channel width changed.  e.g. if AP mode,
 * update all associated STAs when the AP's channel width changes.
 */
static void
cwm_rate_updateallnodes(struct ath_softc *sc)
{
	struct ieee80211com 	*ic 	= &sc->sc_ic;
	struct ieee80211vap 	*vap;

       /* all virtual APs - update destination nodes */
	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		KASSERT(vap->iv_state == IEEE80211_S_RUN , ("cwm_rate_updateallnodes: vap not in run state"));
		ath_rate_newstate(vap, vap->iv_state);
	 }
}

/*
 * Debug - Display CWM information 
 */

static void
cwm_debuginfo(struct ath_softc *sc)
{
	struct ath_cwm 		*acw	= sc->sc_cwm;

	DPRINTF(sc, "%s: ac_running %d\n",		__func__, acw->ac_running);
	DPRINTF(sc, "%s: ac_state %s\n", 		__func__, ath_cwm_statename[acw->ac_state]);
	DPRINTF(sc, "%s: ac_hwstate.ht_macmode %d\n",	__func__, acw->ac_hwstate.ht_macmode);
	DPRINTF(sc, "%s: ac_hwstate.ht_phymode %d\n",	__func__, acw->ac_hwstate.ht_phymode);
	DPRINTF(sc, "%s: ac_hwstate.ht_extprotspacing %d\n",__func__, acw->ac_hwstate.ht_extprotspacing);
	DPRINTF(sc, "%s: ac_hwstate.ht_extoff %d\n",	__func__, acw->ac_hwstate.ht_extoff);
	DPRINTF(sc, "%s: ac_vextch %d\n", 		__func__, acw->ac_vextch);
	DPRINTF(sc, "%s: ac_vextchbusy %d\n", 		__func__, acw->ac_vextchbusy);
	DPRINTF(sc, "%s: ac_timer_prevstate %s\n",	__func__, ath_cwm_statename[acw->ac_timer_prevstate]);
	DPRINTF(sc, "%s: ac_timer_statetime %d\n",	__func__, acw->ac_timer_statetime);
}

