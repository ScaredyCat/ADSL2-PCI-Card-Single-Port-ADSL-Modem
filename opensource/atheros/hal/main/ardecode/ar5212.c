/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ardecode/ar5212.c#1 $
 */
#include <stdio.h>

#include <sys/types.h>

#include "ah_decode.h"

#include "ar5212/ar5212reg.h"
#include "ar5212/ar5212phy.h"

#define PWR_TABLE_SIZE	64

struct {
	u_int32_t	off;
	const char*	name;
	const char*	bits;
} regnames[] = {
	{ AR_CR,		"AR_CR" },
	{ AR_RXDP,		"AR_RXDP" },
	{ AR_CFG,		"AR_CFG" },
	{ AR_IER,		"AR_IER" },
	{ AR_TXCFG,		"AR_TXCFG" },
	{ AR_RXCFG,		"AR_RXCFG" },
	{ AR_MIBC,		"AR_MIBC" },
	{ AR_TOPS,		"AR_TOPS" },
	{ AR_RXNPTO,		"AR_RXNPTO" },
	{ AR_TXNPTO,		"AR_TXNPTO" },
	{ AR_RPGTO,		"AR_RPGTO" },
	{ AR_RPCNT,		"AR_RPCNT" },
	{ AR_MACMISC,		"AR_MACMISC" },
	{ AR_ISR,		"AR_ISR" },
	{ AR_ISR_S0,		"AR_ISR_S0" },
	{ AR_ISR_S1,		"AR_ISR_S1" },
	{ AR_ISR_S2,		"AR_ISR_S2" },
	{ AR_ISR_S3,		"AR_ISR_S3" },
	{ AR_ISR_S4,		"AR_ISR_S4" },
	{ AR_IMR,		"AR_IMR" },
	{ AR_IMR_S0,		"AR_IMR_S0" },
	{ AR_IMR_S1,		"AR_IMR_S1" },
	{ AR_IMR_S2,		"AR_IMR_S2" },
	{ AR_IMR_S3,		"AR_IMR_S3" },
	{ AR_IMR_S4,		"AR_IMR_S4" },
	{ AR_ISR_RAC,		"AR_ISR_RAC" },
	{ AR_ISR_S0_S,		"AR_ISR_S0_S" },
	{ AR_ISR_S1_S,		"AR_ISR_S1_S" },
	{ AR_ISR_S2_S,		"AR_ISR_S2_S" },
	{ AR_ISR_S3_S,		"AR_ISR_S3_S" },
	{ AR_ISR_S4_S,		"AR_ISR_S4_S" },
	{ AR_DCM_A,		"AR_DCM_A" },
	{ AR_DCM_D,		"AR_DCM_D" },
	{ AR_DCCFG,		"AR_DCCFG" },
	{ AR_CCFG,		"AR_CCFG" },
	{ AR_CCUCFG,		"AR_CCUCFG" },
	{ AR_CPC_0,		"AR_CPC_0" },
	{ AR_CPC_1,		"AR_CPC_1" },
	{ AR_CPC_2,		"AR_CPC_2" },
	{ AR_CPC_3,		"AR_CPC_3" },
	{ AR_CPCOVF,		"AR_CPCOVF" },
	{ AR_Q0_TXDP,		"AR_Q0_TXDP" },
	{ AR_Q1_TXDP,		"AR_Q1_TXDP" },
	{ AR_Q2_TXDP,		"AR_Q2_TXDP" },
	{ AR_Q3_TXDP,		"AR_Q3_TXDP" },
	{ AR_Q4_TXDP,		"AR_Q4_TXDP" },
	{ AR_Q5_TXDP,		"AR_Q5_TXDP" },
	{ AR_Q6_TXDP,		"AR_Q6_TXDP" },
	{ AR_Q7_TXDP,		"AR_Q7_TXDP" },
	{ AR_Q8_TXDP,		"AR_Q8_TXDP" },
	{ AR_Q9_TXDP,		"AR_Q9_TXDP" },
	{ AR_Q_TXE,		"AR_Q_TXE" },
	{ AR_Q_TXD,		"AR_Q_TXD" },
	{ AR_Q0_CBRCFG,		"AR_Q0_CBRCFG" },
	{ AR_Q1_CBRCFG,		"AR_Q1_CBRCFG" },
	{ AR_Q2_CBRCFG,		"AR_Q2_CBRCFG" },
	{ AR_Q3_CBRCFG,		"AR_Q3_CBRCFG" },
	{ AR_Q4_CBRCFG,		"AR_Q4_CBRCFG" },
	{ AR_Q5_CBRCFG,		"AR_Q5_CBRCFG" },
	{ AR_Q6_CBRCFG,		"AR_Q6_CBRCFG" },
	{ AR_Q7_CBRCFG,		"AR_Q7_CBRCFG" },
	{ AR_Q8_CBRCFG,		"AR_Q8_CBRCFG" },
	{ AR_Q9_CBRCFG,		"AR_Q9_CBRCFG" },
	{ AR_Q0_RDYTIMECFG,	"AR_Q0_RDYTIMECFG" },
	{ AR_Q1_RDYTIMECFG,	"AR_Q1_RDYTIMECFG" },
	{ AR_Q2_RDYTIMECFG,	"AR_Q2_RDYTIMECFG" },
	{ AR_Q3_RDYTIMECFG,	"AR_Q3_RDYTIMECFG" },
	{ AR_Q4_RDYTIMECFG,	"AR_Q4_RDYTIMECFG" },
	{ AR_Q5_RDYTIMECFG,	"AR_Q5_RDYTIMECFG" },
	{ AR_Q6_RDYTIMECFG,	"AR_Q6_RDYTIMECFG" },
	{ AR_Q7_RDYTIMECFG,	"AR_Q7_RDYTIMECFG" },
	{ AR_Q8_RDYTIMECFG,	"AR_Q8_RDYTIMECFG" },
	{ AR_Q9_RDYTIMECFG,	"AR_Q9_RDYTIMECFG" },
	{ AR_Q_ONESHOTARM_SC,	"AR_Q_ONESHOTARM_SC" },
	{ AR_Q_ONESHOTARM_CC,	"AR_Q_ONESHOTARM_CC" },
	{ AR_Q0_MISC,		"AR_Q0_MISC" },
	{ AR_Q1_MISC,		"AR_Q1_MISC" },
	{ AR_Q2_MISC,		"AR_Q2_MISC" },
	{ AR_Q3_MISC,		"AR_Q3_MISC" },
	{ AR_Q4_MISC,		"AR_Q4_MISC" },
	{ AR_Q5_MISC,		"AR_Q5_MISC" },
	{ AR_Q6_MISC,		"AR_Q6_MISC" },
	{ AR_Q7_MISC,		"AR_Q7_MISC" },
	{ AR_Q8_MISC,		"AR_Q8_MISC" },
	{ AR_Q9_MISC,		"AR_Q9_MISC" },
	{ AR_Q0_STS,		"AR_Q0_STS" },
	{ AR_Q1_STS,		"AR_Q1_STS" },
	{ AR_Q2_STS,		"AR_Q2_STS" },
	{ AR_Q3_STS,		"AR_Q3_STS" },
	{ AR_Q4_STS,		"AR_Q4_STS" },
	{ AR_Q5_STS,		"AR_Q5_STS" },
	{ AR_Q6_STS,		"AR_Q6_STS" },
	{ AR_Q7_STS,		"AR_Q7_STS" },
	{ AR_Q8_STS,		"AR_Q8_STS" },
	{ AR_Q9_STS,		"AR_Q9_STS" },
	{ AR_Q_RDYTIMESHDN,	"AR_Q_RDYTIMESHDN" },
	{ AR_Q_CBBS,		"AR_Q_CBBS" },
	{ AR_Q_CBBA,		"AR_Q_CBBA" },
	{ AR_Q_CBC,		"AR_Q_CBC" },
	{ AR_D0_QCUMASK,	"AR_D0_QCUMASK" },
	{ AR_D1_QCUMASK,	"AR_D1_QCUMASK" },
	{ AR_D2_QCUMASK,	"AR_D2_QCUMASK" },
	{ AR_D3_QCUMASK,	"AR_D3_QCUMASK" },
	{ AR_D4_QCUMASK,	"AR_D4_QCUMASK" },
	{ AR_D5_QCUMASK,	"AR_D5_QCUMASK" },
	{ AR_D6_QCUMASK,	"AR_D6_QCUMASK" },
	{ AR_D7_QCUMASK,	"AR_D7_QCUMASK" },
	{ AR_D8_QCUMASK,	"AR_D8_QCUMASK" },
	{ AR_D9_QCUMASK,	"AR_D9_QCUMASK" },
	{ AR_D0_LCL_IFS,	"AR_D0_LCL_IFS" },
	{ AR_D1_LCL_IFS,	"AR_D1_LCL_IFS" },
	{ AR_D2_LCL_IFS,	"AR_D2_LCL_IFS" },
	{ AR_D3_LCL_IFS,	"AR_D3_LCL_IFS" },
	{ AR_D4_LCL_IFS,	"AR_D4_LCL_IFS" },
	{ AR_D5_LCL_IFS,	"AR_D5_LCL_IFS" },
	{ AR_D6_LCL_IFS,	"AR_D6_LCL_IFS" },
	{ AR_D7_LCL_IFS,	"AR_D7_LCL_IFS" },
	{ AR_D8_LCL_IFS,	"AR_D8_LCL_IFS" },
	{ AR_D9_LCL_IFS,	"AR_D9_LCL_IFS" },
	{ AR_D0_RETRY_LIMIT,	"AR_D0_RETRY_LIMIT" },
	{ AR_D1_RETRY_LIMIT,	"AR_D1_RETRY_LIMIT" },
	{ AR_D2_RETRY_LIMIT,	"AR_D2_RETRY_LIMIT" },
	{ AR_D3_RETRY_LIMIT,	"AR_D3_RETRY_LIMIT" },
	{ AR_D4_RETRY_LIMIT,	"AR_D4_RETRY_LIMIT" },
	{ AR_D5_RETRY_LIMIT,	"AR_D5_RETRY_LIMIT" },
	{ AR_D6_RETRY_LIMIT,	"AR_D6_RETRY_LIMIT" },
	{ AR_D7_RETRY_LIMIT,	"AR_D7_RETRY_LIMIT" },
	{ AR_D8_RETRY_LIMIT,	"AR_D8_RETRY_LIMIT" },
	{ AR_D9_RETRY_LIMIT,	"AR_D9_RETRY_LIMIT" },
	{ AR_D0_CHNTIME,	"AR_D0_CHNTIME" },
	{ AR_D1_CHNTIME,	"AR_D1_CHNTIME" },
	{ AR_D2_CHNTIME,	"AR_D2_CHNTIME" },
	{ AR_D3_CHNTIME,	"AR_D3_CHNTIME" },
	{ AR_D4_CHNTIME,	"AR_D4_CHNTIME" },
	{ AR_D5_CHNTIME,	"AR_D5_CHNTIME" },
	{ AR_D6_CHNTIME,	"AR_D6_CHNTIME" },
	{ AR_D7_CHNTIME,	"AR_D7_CHNTIME" },
	{ AR_D8_CHNTIME,	"AR_D8_CHNTIME" },
	{ AR_D9_CHNTIME,	"AR_D9_CHNTIME" },
	{ AR_D0_MISC,		"AR_D0_MISC" },
	{ AR_D1_MISC,		"AR_D1_MISC" },
	{ AR_D2_MISC,		"AR_D2_MISC" },
	{ AR_D3_MISC,		"AR_D3_MISC" },
	{ AR_D4_MISC,		"AR_D4_MISC" },
	{ AR_D5_MISC,		"AR_D5_MISC" },
	{ AR_D6_MISC,		"AR_D6_MISC" },
	{ AR_D7_MISC,		"AR_D7_MISC" },
	{ AR_D8_MISC,		"AR_D8_MISC" },
	{ AR_D9_MISC,		"AR_D9_MISC" },
	{ AR_D0_SEQNUM,		"AR_D0_SEQNUM" },
	{ AR_D1_SEQNUM,		"AR_D1_SEQNUM" },
	{ AR_D2_SEQNUM,		"AR_D2_SEQNUM" },
	{ AR_D3_SEQNUM,		"AR_D3_SEQNUM" },
	{ AR_D4_SEQNUM,		"AR_D4_SEQNUM" },
	{ AR_D5_SEQNUM,		"AR_D5_SEQNUM" },
	{ AR_D6_SEQNUM,		"AR_D6_SEQNUM" },
	{ AR_D7_SEQNUM,		"AR_D7_SEQNUM" },
	{ AR_D8_SEQNUM,		"AR_D8_SEQNUM" },
	{ AR_D9_SEQNUM,		"AR_D9_SEQNUM" },
	{ AR_D_GBL_IFS_SIFS,	"AR_D_GBL_IFS_SIFS" },
	{ AR_D_GBL_IFS_SLOT,	"AR_D_GBL_IFS_SLOT" },
	{ AR_D_GBL_IFS_EIFS,	"AR_D_GBL_IFS_EIFS" },
	{ AR_D_GBL_IFS_MISC,	"AR_D_GBL_IFS_MISC" },
	{ AR_RC,		"AR_RC" },
	{ AR_SCR,		"AR_SCR" },
	{ AR_INTPEND,		"AR_INTPEND" },
	{ AR_SFR,		"AR_SFR" },
	{ AR_PCICFG,		"AR_PCICFG" },
	{ AR_GPIOCR,		"AR_GPIOCR" },
	{ AR_GPIODO,		"AR_GPIODO" },
	{ AR_GPIODI,		"AR_GPIODI" },
	{ AR_SREV,		"AR_SREV" },
	{ AR_TXEPOST,		"AR_TXEPOST" },
	{ AR_QSM,		"AR_QSM" },
	{ AR_EEPROM_ADDR,	"AR_EEPROM_ADDR" },
	{ AR_EEPROM_DATA,	"AR_EEPROM_DATA" },
	{ AR_EEPROM_CMD,	"AR_EEPROM_CMD" },
	{ AR_EEPROM_STS,	"AR_EEPROM_STS" },
	{ AR_EEPROM_CFG,	"AR_EEPROM_CFG" },
	{ AR_STA_ID0,		"AR_STA_ID0" },
	{ AR_STA_ID1,		"AR_STA_ID1" },
	{ AR_BSS_ID0,		"AR_BSS_ID0" },
	{ AR_BSS_ID1,		"AR_BSS_ID1" },
	{ AR_SLOT_TIME,		"AR_SLOT_TIME" },
	{ AR_TIME_OUT,		"AR_TIME_OUT" },
	{ AR_RSSI_THR,		"AR_RSSI_THR" },
	{ AR_USEC,		"AR_USEC" },
	{ AR_BEACON,		"AR_BEACON" },
	{ AR_CFP_PERIOD,	"AR_CFP_PERIOD" },
	{ AR_TIMER0,		"AR_TIMER0" },
	{ AR_TIMER1,		"AR_TIMER1" },
	{ AR_TIMER2,		"AR_TIMER2" },
	{ AR_TIMER3,		"AR_TIMER3" },
	{ AR_CFP_DUR,		"AR_CFP_DUR" },
	{ AR_RX_FILTER,		"AR_RX_FILTER" },
	{ AR_MCAST_FIL0,	"AR_MCAST_FIL0" },
	{ AR_MCAST_FIL1,	"AR_MCAST_FIL1" },
	{ AR_DIAG_SW,		"AR_DIAG_SW" },
	{ AR_TSF_L32,		"AR_TSF_L32" },
	{ AR_TSF_U32,		"AR_TSF_U32" },
	{ AR_TST_ADDAC,		"AR_TST_ADDAC" },
	{ AR_DEF_ANTENNA,	"AR_DEF_ANTENNA" },
	{ AR_LAST_TSTP,		"AR_LAST_TSTP" },
	{ AR_NAV,		"AR_NAV" },
	{ AR_RTS_OK,		"AR_RTS_OK" },
	{ AR_RTS_FAIL,		"AR_RTS_FAIL" },
	{ AR_ACK_FAIL,		"AR_ACK_FAIL" },
	{ AR_FCS_FAIL,		"AR_FCS_FAIL" },
	{ AR_BEACON_CNT,	"AR_BEACON_CNT" },
	{ AR_XRMODE,		"AR_XRMODE" },
	{ AR_XRDEL,		"AR_XRDEL" },
	{ AR_XRTO,		"AR_XRTO" },
	{ AR_XRCRP,		"AR_XRCRP" },
	{ AR_XRSTMP,		"AR_XRSTMP" },
	{ AR_SLEEP1,		"AR_SLEEP1" },
	{ AR_SLEEP2,		"AR_SLEEP2" },
	{ AR_SLEEP3,		"AR_SLEEP3" },
	{ AR_BSSMSKL,		"AR_BSSMSKL" },
	{ AR_BSSMSKU,		"AR_BSSMSKU" },
	{ AR_TPC,		"AR_TPC" },
	{ AR_TFCNT,		"AR_TFCNT" },
	{ AR_RFCNT,		"AR_RFCNT" },
	{ AR_RCCNT,		"AR_RCCNT" },
	{ AR_CCCNT,		"AR_CCCNT" },
	{ AR_PHY_ERR,		"AR_PHY_ERR" },
	{ AR_PHY_TURBO,		"AR_PHY_TURBO" },
	{ AR_PHY_TIMING3,	"AR_PHY_TIMING3" },
	{ AR_PHY_CHIP_ID,	"AR_PHY_CHIP_ID" },
	{ AR_PHY_ACTIVE,	"AR_PHY_ACTIVE" },
	{ AR_PHY_DESIRED_SZ,	"AR_PHY_DESIRED_SZ" },
	{ AR_PHY_FIND_SIG,	"AR_PHY_FIND_SIG" },
	{ AR_PHY_AGC_CTL1,	"AR_PHY_AGC_CTL1" },
	{ AR_PHY_AGC_CONTROL,	"AR_PHY_AGC_CONTROL" },
	{ AR_PHY_SFCORR_LOW,	"AR_PHY_SFCORR_LOW" },
	{ AR_PHY_SFCORR,	"AR_PHY_SFCORR" },
	{ AR_PHY_PLL_CTL,	"AR_PHY_PLL_CTL" },
	{ AR_PHY_RX_DELAY,	"AR_PHY_RX_DELAY" },
	{ AR_PHY_TIMING_CTRL4,	"AR_PHY_TIMING_CTRL4" },
	{ AR_PHY_TIMING5,	"AR_PHY_TIMING5" },
	{ AR_PHY_PAPD_PROBE,	"AR_PHY_PAPD_PROBE" },
	{ AR_PHY_POWER_TX_RATE1,"AR_PHY_POWER_TX_RATE1" },
	{ AR_PHY_POWER_TX_RATE2,"AR_PHY_POWER_TX_RATE2" },
	{ AR_PHY_POWER_TX_RATE_MAX,
				"AR_PHY_POWER_TX_RATE_MAX" },
	{ AR_PHY_FRAME_CTL,	"AR_PHY_FRAME_CTL" },
	{ AR_PHY_RADAR_0,	"AR_PHY_RADAR_0" },
	{ AR_PHY_IQCAL_RES_PWR_MEAS_I,
				"AR_PHY_IQCAL_RES_PWR_MEAS_I" },
	{ AR_PHY_IQCAL_RES_PWR_MEAS_Q,
				"AR_PHY_IQCAL_RES_PWR_MEAS_Q" },
	{ AR_PHY_IQCAL_RES_IQ_CORR_MEAS,
				"AR_PHY_IQCAL_RES_IQ_CORR_MEAS" },
	{ AR_PHY_CURRENT_RSSI,	"AR_PHY_CURRENT_RSSI" },
	{ AR_PHY_MODE,		"AR_PHY_MODE" },
	{ AR_PHY_CCK_TX_CTRL,	"AR_PHY_CCK_TX_CTRL" },
	{ AR_PHY_CCK_DETECT,	"AR_PHY_CCK_DETECT" },
	{ AR_PHY_POWER_TX_RATE3,"AR_PHY_POWER_TX_RATE3" },
	{ AR_PHY_POWER_TX_RATE4,"AR_PHY_POWER_TX_RATE4" },
};

void
ar5212print(FILE *fd, u_int recnum, struct athregrec *r)
{
#define	N(a)	(sizeof (a) / sizeof (a[0]))
	char buf[64];
	const char* bits;
	int i;

	for (i = 0; i < N(regnames); i++)
		if (regnames[i].off == r->reg)
			break;
	fprintf(fd, "\n%05u: ", recnum);
	if (i < N(regnames)) {
		snprintf(buf, sizeof (buf), "%s (0x%x)",
			regnames[i].name, r->reg);
		bits = regnames[i].bits;
	} else if (AR_KEYTABLE(0) <= r->reg && r->reg < AR_KEYTABLE(128)) {
		snprintf(buf, sizeof (buf), "AR_KEYTABLE%u(%u) (0x%x)",
			((r->reg - AR_KEYTABLE_0) >> 2) & 7,
			(r->reg - AR_KEYTABLE_0) >> 5, r->reg);
		bits = NULL;
	} else if (AR_PHY_PCDAC_TX_POWER(0) <= r->reg && r->reg < AR_PHY_PCDAC_TX_POWER(PWR_TABLE_SIZE/2)) {
		snprintf(buf, sizeof (buf), "AR_PHY_PCDAC_TX_POWER(%u) (0x%x)",
			(r->reg - AR_PHY_PCDAC_TX_POWER_0) >> 2, r->reg);
		bits = NULL;
	} else if (AR_RATE_DURATION(0) <= r->reg && r->reg < AR_RATE_DURATION(32)) {
		snprintf(buf, sizeof (buf), "AR_RATE_DURATION(0x%x) (0x%x)",
			(r->reg - AR_RATE_DURATION_0) >> 2, r->reg);
		bits = NULL;
	} else if (AR_PHY_BASE <= r->reg) {
		snprintf(buf, sizeof (buf), "AR_PHY(%u) (0x%x)",
			(r->reg - AR_PHY_BASE) >> 2, r->reg);
		bits = NULL;
	} else {
		snprintf(buf, sizeof (buf), "0x%x", r->reg);
		bits = NULL;
	}
	fprintf(fd, "%-30s %s 0x%x", buf, r->op ? "<=" : "=>", r->val);
	if (bits) {
		const char *p = bits;
		int tmp, n;

		for (tmp = 0, p++; *p;) {
			n = *p++;
			if (r->val & (1 << (n - 1))) {
				putc(tmp ? ',' : '<', fd);
				for (; (n = *p) > ' '; ++p)
					putc(n, fd);
				tmp = 1;
			} else
				for (; *p > ' '; ++p)
					continue;
		}
		if (tmp)
			putc('>', fd);
	}
#undef N
}
