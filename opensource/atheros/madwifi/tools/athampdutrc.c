/*-
 * Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
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
 *
 * $FreeBSD: src/tools/tools/ath/athstats.c,v 1.4 2003/12/07 21:40:52 sam Exp $
 */

/*
 * Simple Atheros-specific tool to inspect and monitor network traffic
 * statistics.
 *  athstats [-i interface] [interval]
 * (default interface is wifi0).  If interval is specified a rolling output
 * is displayed every interval seconds.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#include "ah_desc.h"
#include "if_athioctl.h"

static const char *
ampdu_pkt_type(int npkts)
{
    if (npkts == 0)
        return "BAR";
    else if (npkts == 1)
        return "UNAGGR";
    else
        return "AGGR";
}

static void
dump_ampdu_trc(FILE *fd, const struct ath_stats *stats)
{
    int i, ent;
    const struct ath_ampdu_trc_entry *trc;
    const char *pkttype;

    fprintf(fd, "AMDU trace\n");
    fprintf(fd, "%8s %8s %8s %8s %8s %16s\n",
                "type", "seqno", "npkts", "aggrlen", "BA seqno", "BA bitmap");
    for (i = 0; i < (NTRENTS - 1); i++) {
        ent = (stats->ast_ampdu_trc.tr_head + i) % NTRENTS;
        trc = &stats->ast_ampdu_trc.tr_ents[ent];
        pkttype = ampdu_pkt_type(trc->tre_npkts);

        fprintf(fd, "%8s %8d %8d %8d %8d %08X%08X\n",
                    pkttype,
                    trc->tre_seqst, trc->tre_npkts, trc->tre_aggrlen,
                    trc->tre_baseqst, trc->tre_bamap1, trc->tre_bamap0);
    }
}

int
main(int argc, char *argv[])
{
#ifdef __linux__
    const char *ifname = "wifi0";
#else
    const char *ifname = "ath0";
#endif
    int s;
    struct ifreq ifr;
    struct ath_stats stats;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        err(1, "socket");
    if (argc > 1 && strcmp(argv[1], "-i") == 0) {
        if (argc < 2) {
            fprintf(stderr, "%s: missing interface name for -i\n",
                argv[0]);
            exit(-1);
        }
        ifname = argv[2];
        argc -= 2, argv += 2;
    }
    strncpy(ifr.ifr_name, ifname, sizeof (ifr.ifr_name));

    ifr.ifr_data = (caddr_t) &stats;
    if (ioctl(s, SIOCGATHSTATS, &ifr) < 0)
        err(1, ifr.ifr_name);
    dump_ampdu_trc(stdout, &stats);
    return 0;
}
