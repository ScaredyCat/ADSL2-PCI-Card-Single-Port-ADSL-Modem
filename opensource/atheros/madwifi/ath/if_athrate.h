/*-
 * Copyright (c) 2004 Sam Leffler, Errno Consulting
 * Copyright (c) 2004 Video54 Technologies, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
    without modification.
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
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/madwifi/ath/if_athrate.h#5 $
 */
#ifndef _ATH_RATECTRL_H_
#define _ATH_RATECTRL_H_

/*
 * Interface definitions for transmit rate control modules for the
 * Atheros driver.
 *
 * A rate control module is responsible for choosing the transmit rate
 * for each data frame.  Management+control frames are always sent at
 * a fixed rate.
 *
 * Only one module may be present at a time; the driver references
 * rate control interfaces by symbol name.  If multiple modules are
 * to be supported we'll need to switch to a registration-based scheme
 * as is currently done, for example, for authentication modules.
 *
 * An instance of the rate control module is attached to each device
 * at attach time and detached when the device is destroyed.  The module
 * may associate data with each device and each node (station).  Both
 * sets of storage are opaque except for the size of the per-node storage
 * which must be provided when the module is attached.
 *
 * The rate control module is notified for each state transition and
 * station association/reassociation.  Otherwise it is queried for a
 * rate for each outgoing frame and provided status from each transmitted
 * frame.  Any ancillary processing is the responsibility of the module
 * (e.g. if periodic processing is required then the module should setup
 * it's own timer).
 *
 * In addition to the transmit rate for each frame the module must also
 * indicate the number of attempts to make at the specified rate.  If this
 * number is != ATH_TXMAXTRY then an additional callback is made to setup
 * additional transmit state.  The rate control code is assumed to write
 * this additional data directly to the transmit descriptor.
 */
struct ath_softc;
struct ath_node;
struct ath_desc;
struct ieee80211vap;

struct ath_ratectrl {
	size_t	arc_space;	/* space required for per-node state */
	size_t	arc_vap_space;	/* space required for per-vap state */
};

#define ATH_RC_DS_FLAG       0x01
#define ATH_RC_CW40_FLAG     0x02
#define ATH_RC_SGI_FLAG      0x04
#define ATH_RC_HT_FLAG       0x08
#define ATH_RC_RTSCTS_FLAG   0x10

enum ath_rc_cwmode{
    ATH_RC_CW20_MODE,
    ATH_RC_CW40_MODE,     
};

#define ATH_RC_PROBE_ALLOWED    0x00000001
#define ATH_RC_MINRATE_LASTRATE 0x00000002

struct ath_rc_series {
    u_int8_t rix;	
	u_int8_t tries;	
    u_int8_t flags;
};

/*
 * Attach/detach a rate control module.
 */
struct ath_ratectrl *ath_rate_attach(struct ath_softc *);
void	ath_rate_detach(struct ath_ratectrl *);


/*
 * State storage handling.
 */
/*
 * Initialize per-node state already allocated for the specified
 * node; this space can be assumed initialized to zero.
 */
void	ath_rate_node_init(struct ath_softc *, struct ath_node *);
/*
 * Cleanup any per-node state prior to the node being reclaimed.
 */
void	ath_rate_node_cleanup(struct ath_softc *, struct ath_node *);
/*
 * Update rate control state on station associate/reassociate 
 * (when operating as an ap or for nodes discovered when operating
 * in ibss mode).
 */
void	ath_rate_newassoc(struct ath_softc *, struct ath_node *,
		int isNewAssociation, unsigned int capflag);
/*
 * Update/reset rate control state for 802.11 state transitions.
 * Important mostly as the analog to ath_rate_newassoc when operating
 * in station mode.
 */
void	ath_rate_newstate(struct ieee80211vap *, enum ieee80211_state);

/*
 * Transmit handling.
 */
/*
 * Return the transmit info for a data packet.  If multi-rate state
 * is to be setup then try0 should contain a value other than ATH_TXMATRY
 * and ath_rate_setupxtxdesc will be called after deciding if the frame
 * can be transmitted with multi-rate retry.
 */
void	ath_rate_findrate(struct ath_softc *sc, struct ath_node *an,
                  size_t frameLen, int numTries, int numRates, int stepDnInc,
                  unsigned int rcflag,struct ath_rc_series series[], int *isProbe
                  );
/*
 * Update rate control state for a packet associated with the
 * supplied transmit descriptor.  The routine is invoked both
 * for packets that were successfully sent and for those that
 * failed (consult the descriptor for details).
 */
void	ath_rate_tx_complete(struct ath_softc *, struct ath_node *,
		const struct ath_desc *, struct ath_rc_series series[], int nframes, int nbad);


void    ath_rate_stateupdate(struct ath_softc *sc, struct ath_node *an, 
                             enum ath_rc_cwmode cwmode);

#endif /* _ATH_RATECTRL_H_ */
