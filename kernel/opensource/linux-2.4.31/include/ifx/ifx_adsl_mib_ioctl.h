/******************************************************************************
       Copyright (c) 2002, Infineon Technologies.  All rights reserved.

                               No Warranty
   Because the program is licensed free of charge, there is no warranty for
   the program, to the extent permitted by applicable law.  Except when
   otherwise stated in writing the copyright holders and/or other parties
   provide the program "as is" without warranty of any kind, either
   expressed or implied, including, but not limited to, the implied
   warranties of merchantability and fitness for a particular purpose. The
   entire risk as to the quality and performance of the program is with
   you.  should the program prove defective, you assume the cost of all
   necessary servicing, repair or correction.

   In no event unless required by applicable law or agreed to in writing
   will any copyright holder, or any other party who may modify and/or
   redistribute the program as permitted above, be liable to you for
   damages, including any general, special, incidental or consequential
   damages arising out of the use or inability to use the program
   (including but not limited to loss of data or data being rendered
   inaccurate or losses sustained by you or third parties or a failure of
   the program to operate with any other programs), even if such holder or
   other party has been advised of the possibility of such damages.
******************************************************************************/
#ifndef __IFX_ADSL_APP_IOCTL_H
#define __IFX_ADSL_APP_IOCTL_H

// #warning "__IFX_ADSL_APP_IOCTL_H"

/* Interface Name */
//#define INTERFACE_NAME <define the interface>

/* adslLineTable constants */
#define GET_ADSL_LINE_CODE		( 1 + IFX_ADSL_IOC_MIB_BASE)

/* adslAtucPhysTable constants */
#define GET_ADSL_ATUC_PHY		( 4 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturPhysTable constants */
#define GET_ADSL_ATUR_PHY		( 10 + IFX_ADSL_IOC_MIB_BASE)

/* adslAtucChanTable constants */
#define GET_ADSL_ATUC_CHAN_INFO 	( 15 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturChanTable constants */
#define GET_ADSL_ATUR_CHAN_INFO		( 18 + IFX_ADSL_IOC_MIB_BASE)

/* adslAtucPerfDataTable constants */
#define GET_ADSL_ATUC_PERF_DATA		( 21 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturPerfDataTable constants */
#define GET_ADSL_ATUR_PERF_DATA		( 40 + IFX_ADSL_IOC_MIB_BASE)

/* adslAtucIntervalTable constants */
#define GET_ADSL_ATUC_INTVL_INFO	( 60 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturIntervalTable constants */
#define GET_ADSL_ATUR_INTVL_INFO	( 65 + IFX_ADSL_IOC_MIB_BASE)

/* adslAtucChanPerfDataTable constants */
#define GET_ADSL_ATUC_CHAN_PERF_DATA	( 70 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturChanPerfDataTable constants */
#define GET_ADSL_ATUR_CHAN_PERF_DATA	( 90 + IFX_ADSL_IOC_MIB_BASE)

/* adslAtucChanIntervalTable constants */
#define GET_ADSL_ATUC_CHAN_INTVL_INFO	( 110 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturChanIntervalTable constants */
#define GET_ADSL_ATUR_CHAN_INTVL_INFO	( 115 + IFX_ADSL_IOC_MIB_BASE)

/* adslLineAlarmConfProfileTable constants */
#define GET_ADSL_ALRM_CONF_PROF		( 120 + IFX_ADSL_IOC_MIB_BASE)
#define SET_ADSL_ALRM_CONF_PROF		( 121 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturTrap constants */
#define ADSL_ATUR_TRAPS			( 135 + IFX_ADSL_IOC_MIB_BASE)

//////////////////  RFC-3440 //////////////

#ifdef IFX_ADSL_MIB_RFC3440
/* adslLineExtTable */
#define GET_ADSL_ATUC_LINE_EXT		( 201 + IFX_ADSL_IOC_MIB_BASE)
#define SET_ADSL_ATUC_LINE_EXT		( 203 + IFX_ADSL_IOC_MIB_BASE)

/* adslAtucPerfDateExtTable */
#define GET_ADSL_ATUC_PERF_DATA_EXT	( 205 + IFX_ADSL_IOC_MIB_BASE)

/* adslAtucIntervalExtTable */
#define GET_ADSL_ATUC_INTVL_EXT_INFO	( 221 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturPerfDataExtTable */
#define GET_ADSL_ATUR_PERF_DATA_EXT	( 225 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturIntervalExtTable */
#define GET_ADSL_ATUR_INTVL_EXT_INFO	( 233 + IFX_ADSL_IOC_MIB_BASE)

/* adslAlarmConfProfileExtTable */
#define GET_ADSL_ALRM_CONF_PROF_EXT	( 235 + IFX_ADSL_IOC_MIB_BASE)
#define SET_ADSL_ALRM_CONF_PROF_EXT	( 236 + IFX_ADSL_IOC_MIB_BASE)

/* adslAturExtTrap */
#define ADSL_ATUR_EXT_TRAPS		( 240 + IFX_ADSL_IOC_MIB_BASE)

#endif

/* The following constants are added to support the WEB related ADSL Statistics */

/* adslLineStatus constants */
#define GET_ADSL_LINE_STATUS		( 245 + IFX_ADSL_IOC_MIB_BASE)

/* adslLineRate constants */
#define GET_ADSL_LINE_RATE		( 250 + IFX_ADSL_IOC_MIB_BASE)

/* adslLineInformation constants */
#define GET_ADSL_LINE_INFO		( 255 + IFX_ADSL_IOC_MIB_BASE)

/* adslNearEndPerformanceStats constants */
#define GET_ADSL_NEAREND_STATS		( 270 + IFX_ADSL_IOC_MIB_BASE)

/* adslFarEndPerformanceStats constants */
#define GET_ADSL_FAREND_STATS		( 290 + IFX_ADSL_IOC_MIB_BASE)

/* Sub-carrier related parameters */
#define GET_ADSL_LINE_INIT_STATS	( 150 + IFX_ADSL_IOC_MIB_BASE)
#define GET_ADSL_POWER_SPECTRAL_DENSITY	( 151 + IFX_ADSL_IOC_MIB_BASE)

/**
 * \deprecated
 *	Not necessary anymore, as this is handled by the autoboot daemon in kernel!
 */
#define IFX_ADSL_MIB_LO_ATUC		( 295 + IFX_ADSL_IOC_MIB_BASE)
/**
 * \deprecated
 *	Not necessary anymore, as this is handled by the autoboot daemon in kernel!
 */
#define IFX_ADSL_MIB_LO_ATUR		( 296 + IFX_ADSL_IOC_MIB_BASE)

#define GET_ADSL_ATUC_SUBCARRIER_STATS	( 297 + IFX_ADSL_IOC_MIB_BASE)
#define GET_ADSL_ATUR_SUBCARRIER_STATS	( 298 + IFX_ADSL_IOC_MIB_BASE)



///////////////////////////////////////////////////////////
// makeCMV(Opcode, Group, Address, Index, Size, Data)

/* adslLineCode Flags */
#define LINE_CODE_FLAG			0x1	/* BIT 0th position */

/* adslAtucPhysTable Flags */
#define ATUC_PHY_SER_NUM_FLAG		0x1	/* BIT 0th position */

#define ATUC_PHY_VENDOR_ID_FLAG		0x2	/* BIT 1 */

#define ATUC_PHY_VER_NUM_FLAG		0x4	/* BIT 2 */

#define ATUC_CURR_STAT_FLAG		0x8	/* BIT 3 */

#define ATUC_CURR_OUT_PWR_FLAG		0x10	/* BIT 4 */

#define ATUC_CURR_ATTR_FLAG		0x20	/* BIT 5 */


/* adslAturPhysTable	Flags */
#define ATUR_PHY_SER_NUM_FLAG		0x1	/* BIT 0th position */

#define ATUR_PHY_VENDOR_ID_FLAG		0x2	/* BIT 1 */

#define ATUR_PHY_VER_NUM_FLAG		0x4	/* BIT 2 */

#define ATUR_SNRMGN_FLAG		0x8

#define ATUR_ATTN_FLAG			0x10

#define ATUR_CURR_STAT_FLAG		0x20	/* BIT 3 */

#define ATUR_CURR_OUT_PWR_FLAG		0x40	/* BIT 4 */

#define ATUR_CURR_ATTR_FLAG		0x80	/* BIT 5 */

/* adslAtucChanTable Flags */
#define ATUC_CHAN_INTLV_DELAY_FLAG	0x1	/* BIT 0th position */

#define ATUC_CHAN_CURR_TX_RATE_FLAG	0x2	/* BIT 1 */

#define ATUC_CHAN_PREV_TX_RATE_FLAG	0x4	/* BIT 2 */

/* adslAturChanTable Flags */
#define ATUR_CHAN_INTLV_DELAY_FLAG	0x1	/* BIT 0th position */

#define ATUR_CHAN_CURR_TX_RATE_FLAG	0x2	/* BIT 1 */

#define ATUR_CHAN_PREV_TX_RATE_FLAG	0x4	/* BIT 2 */

#define ATUR_CHAN_CRC_BLK_LEN_FLAG	0x8	/* BIT 3 */

/* adslAtucPerfDataTable Flags */
#define ATUC_PERF_LOFS_FLAG		0x1	/* BIT 0th position */
#define ATUC_PERF_LOSS_FLAG		0x2	/* BIT 1 */
#define ATUC_PERF_ESS_FLAG		0x4	/* BIT 2 */
#define ATUC_PERF_INITS_FLAG	0x8	/* BIT 3 */
#define ATUC_PERF_VALID_INTVLS_FLAG	0x10 /* BIT 4 */
#define ATUC_PERF_INVALID_INTVLS_FLAG	0x20 /* BIT 5 */
#define ATUC_PERF_CURR_15MIN_TIME_ELAPSED_FLAG	0x40 /* BIT 6 */
#define ATUC_PERF_CURR_15MIN_LOFS_FLAG	 	0x80 	 /* BIT 7 */
#define ATUC_PERF_CURR_15MIN_LOSS_FLAG		0x100 /* BIT 8 */
#define ATUC_PERF_CURR_15MIN_ESS_FLAG		0x200	/* BIT 9 */
#define ATUC_PERF_CURR_15MIN_INIT_FLAG		0x400 /* BIT 10 */
#define ATUC_PERF_CURR_1DAY_TIME_ELAPSED_FLAG 0x800 /* BIT 11 */
#define ATUC_PERF_CURR_1DAY_LOFS_FLAG		0x1000 /* BIT 12 */
#define ATUC_PERF_CURR_1DAY_LOSS_FLAG		0x2000 /* BIT 13 */
#define ATUC_PERF_CURR_1DAY_ESS_FLAG		0x4000 /* BIT 14 */
#define ATUC_PERF_CURR_1DAY_INIT_FLAG		0x8000 /* BIT 15 */
#define ATUC_PERF_PREV_1DAY_MON_SEC_FLAG	0x10000 /* BIT 16 */
#define ATUC_PERF_PREV_1DAY_LOFS_FLAG		0x20000 /* BIT 17 */
#define ATUC_PERF_PREV_1DAY_LOSS_FLAG		0x40000 /* BIT 18 */
#define ATUC_PERF_PREV_1DAY_ESS_FLAG		0x80000 /* BIT 19 */
#define ATUC_PERF_PREV_1DAY_INITS_FLAG		0x100000 /* BIT 20 */

/* adslAturPerfDataTable Flags */
#define ATUR_PERF_LOFS_FLAG		0x1	/* BIT 0th position */
#define ATUR_PERF_LOSS_FLAG		0x2	/* BIT 1 */
#define ATUR_PERF_LPR_FLAG		0x4	/* BIT 2 */
#define ATUR_PERF_ESS_FLAG		0x8	/* BIT 3 */
#define ATUR_PERF_VALID_INTVLS_FLAG	0x10 /* BIT 4 */
#define ATUR_PERF_INVALID_INTVLS_FLAG	0x20 /* BIT 5 */
#define ATUR_PERF_CURR_15MIN_TIME_ELAPSED_FLAG	0x40 /* BIT 6 */
#define ATUR_PERF_CURR_15MIN_LOFS_FLAG	 	0x80 	 /* BIT 7 */
#define ATUR_PERF_CURR_15MIN_LOSS_FLAG		0x100 /* BIT 8 */
#define ATUR_PERF_CURR_15MIN_LPR_FLAG		0x200 /* BIT 9 */
#define ATUR_PERF_CURR_15MIN_ESS_FLAG		0x400	/* BIT 10 */
#define ATUR_PERF_CURR_1DAY_TIME_ELAPSED_FLAG 	0x800 /* BIT 11 */
#define ATUR_PERF_CURR_1DAY_LOFS_FLAG		0x1000 /* BIT 12 */
#define ATUR_PERF_CURR_1DAY_LOSS_FLAG		0x2000 /* BIT 13 */
#define ATUR_PERF_CURR_1DAY_LPR_FLAG		0x4000 /* BIT 14 */
#define ATUR_PERF_CURR_1DAY_ESS_FLAG		0x8000 /* BIT 15 */
#define ATUR_PERF_PREV_1DAY_MON_SEC_FLAG	0x10000 /* BIT 16 */
#define ATUR_PERF_PREV_1DAY_LOFS_FLAG		0x20000 /* BIT 17 */
#define ATUR_PERF_PREV_1DAY_LOSS_FLAG		0x40000 /* BIT 18 */
#define ATUR_PERF_PREV_1DAY_LPR_FLAG		0x80000 /* BIT 19 */
#define ATUR_PERF_PREV_1DAY_ESS_FLAG		0x100000 /* BIT 20 */

/* adslAtucIntervalTable Flags */
#define ATUC_INTVL_LOF_FLAG		0x1	/* BIT 0th position */
#define ATUC_INTVL_LOS_FLAG		0x2 	/* BIT 1 */
#define ATUC_INTVL_ESS_FLAG		0x4	/* BIT 2 */
#define ATUC_INTVL_INIT_FLAG		0x8   /* BIT 3 */
#define ATUC_INTVL_VALID_DATA_FLAG 	0x10 /* BIT 4 */

/* adslAturIntervalTable Flags */
#define ATUR_INTVL_LOF_FLAG		0x1	/* BIT 0th position */
#define ATUR_INTVL_LOS_FLAG		0x2 	/* BIT 1 */
#define ATUR_INTVL_LPR_FLAG		0x4 	/* BIT 2 */
#define ATUR_INTVL_ESS_FLAG		0x8	/* BIT 3 */
#define ATUR_INTVL_VALID_DATA_FLAG 	0x10 /* BIT 4 */

/* adslAtucChanPerfDataTable Flags */
#define ATUC_CHAN_RECV_BLK_FLAG	0x01	/* BIT 0th position */
#define ATUC_CHAN_TX_BLK_FLAG	0x02	/* BIT 1 */
#define ATUC_CHAN_CORR_BLK_FLAG	0x04	/* BIT 2 */
#define ATUC_CHAN_UNCORR_BLK_FLAG 0x08	/* BIT 3 */
#define ATUC_CHAN_PERF_VALID_INTVL_FLAG 0x10 /* BIT 4 */
#define ATUC_CHAN_PERF_INVALID_INTVL_FLAG 0x20 /* BIT 5 */
#define ATUC_CHAN_PERF_CURR_15MIN_TIME_ELAPSED_FLAG 0x40 /* BIT 6 */
#define ATUC_CHAN_PERF_CURR_15MIN_RECV_BLK_FLAG	0x80 /* BIT 7 */
#define ATUC_CHAN_PERF_CURR_15MIN_TX_BLK_FLAG 0x100 /* BIT 8 */
#define ATUC_CHAN_PERF_CURR_15MIN_CORR_BLK_FLAG 0x200 /* BIT 9 */
#define ATUC_CHAN_PERF_CURR_15MIN_UNCORR_BLK_FLAG 0x400 /* BIT 10 */
#define ATUC_CHAN_PERF_CURR_1DAY_TIME_ELAPSED_FLAG 0x800 /* BIT 11*/
#define ATUC_CHAN_PERF_CURR_1DAY_RECV_BLK_FLAG 0x1000 /* BIT 12 */
#define ATUC_CHAN_PERF_CURR_1DAY_TX_BLK_FLAG 0x2000 /* BIT 13 */
#define ATUC_CHAN_PERF_CURR_1DAY_CORR_BLK_FLAG 0x4000 /* BIT 14 */
#define ATUC_CHAN_PERF_CURR_1DAY_UNCORR_BLK_FLAG 0x8000 /* BIT 15 */
#define ATUC_CHAN_PERF_PREV_1DAY_MONI_SEC_FLAG 0x10000 /* BIT 16 */
#define ATUC_CHAN_PERF_PREV_1DAY_RECV_BLK_FLAG 0x20000 /* BIT 17 */
#define ATUC_CHAN_PERF_PREV_1DAY_TX_BLK_FLAG 0x40000 /* BIT 18 */
#define ATUC_CHAN_PERF_PREV_1DAY_CORR_BLK_FLAG 0x80000 /* BIT 19 */
#define ATUC_CHAN_PERF_PREV_1DAY_UNCORR_BLK_FLAG 0x100000 /* BIT 20 */


/* adslAturChanPerfDataTable Flags */
#define ATUR_CHAN_RECV_BLK_FLAG   0x01 	/* BIT 0th position */
#define ATUR_CHAN_TX_BLK_FLAG     0x02 	/* BIT 1 */
#define ATUR_CHAN_CORR_BLK_FLAG   0x04 	/* BIT 2 */
#define ATUR_CHAN_UNCORR_BLK_FLAG 0x08		/* BIT 3 */
#define ATUR_CHAN_PERF_VALID_INTVL_FLAG   0x10 	/* BIT 4 */
#define ATUR_CHAN_PERF_INVALID_INTVL_FLAG 0x20 	/* BIT 5 */
#define ATUR_CHAN_PERF_CURR_15MIN_TIME_ELAPSED_FLAG 0x40 /* BIT 6 */
#define ATUR_CHAN_PERF_CURR_15MIN_RECV_BLK_FLAG    0x80   /* BIT 7 */
#define ATUR_CHAN_PERF_CURR_15MIN_TX_BLK_FLAG      0x100 /* BIT 8 */
#define ATUR_CHAN_PERF_CURR_15MIN_CORR_BLK_FLAG    0x200 /* BIT 9 */
#define ATUR_CHAN_PERF_CURR_15MIN_UNCORR_BLK_FLAG  0x400 /* BIT 10 */
#define ATUR_CHAN_PERF_CURR_1DAY_TIME_ELAPSED_FLAG 0x800 /* BIT 11 */
#define ATUR_CHAN_PERF_CURR_1DAY_RECV_BLK_FLAG     0x1000 /* BIT 12 */
#define ATUR_CHAN_PERF_CURR_1DAY_TX_BLK_FLAG       0x2000 /* BIT 13 */
#define ATUR_CHAN_PERF_CURR_1DAY_CORR_BLK_FLAG     0x4000 /* BIT 14 */
#define ATUR_CHAN_PERF_CURR_1DAY_UNCORR_BLK_FLAG   0x8000 /* BIT 15 */
#define ATUR_CHAN_PERF_PREV_1DAY_MONI_SEC_FLAG     0x10000 /* BIT 16 */
#define ATUR_CHAN_PERF_PREV_1DAY_RECV_BLK_FLAG     0x20000 /* BIT 17 */
#define ATUR_CHAN_PERF_PREV_1DAY_TRANS_BLK_FLAG    0x40000 /* BIT 18 */
#define ATUR_CHAN_PERF_PREV_1DAY_CORR_BLK_FLAG     0x80000 /* BIT 19 */
#define ATUR_CHAN_PERF_PREV_1DAY_UNCORR_BLK_FLAG   0x100000 /* BIT 20 */

/* adslAtucChanIntervalTable Flags */
#define ATUC_CHAN_INTVL_NUM_FLAG   	     	0x1 	/* BIT 0th position */
#define ATUC_CHAN_INTVL_RECV_BLK_FLAG  		0x2 	/* BIT 1 */
#define ATUC_CHAN_INTVL_TX_BLK_FLAG  		0x4	/* BIT 2 */
#define ATUC_CHAN_INTVL_CORR_BLK_FLAG   	0x8 	/* BIT 3 */
#define ATUC_CHAN_INTVL_UNCORR_BLK_FLAG   	0x10 	/* BIT 4 */
#define ATUC_CHAN_INTVL_VALID_DATA_FLAG 	0x20 	/* BIT 5 */

/* adslAturChanIntervalTable Flags */
#define ATUR_CHAN_INTVL_NUM_FLAG   	     	0x1 	/* BIT 0th Position */
#define ATUR_CHAN_INTVL_RECV_BLK_FLAG  		0x2 	/* BIT 1 */
#define ATUR_CHAN_INTVL_TX_BLK_FLAG  		0x4	/* BIT 2 */
#define ATUR_CHAN_INTVL_CORR_BLK_FLAG   	0x8 	/* BIT 3 */
#define ATUR_CHAN_INTVL_UNCORR_BLK_FLAG   	0x10 	/* BIT 4 */
#define ATUR_CHAN_INTVL_VALID_DATA_FLAG 	0x20 	/* BIT 5 */

/* adslLineAlarmConfProfileTable Flags */
#define ATUC_THRESH_15MIN_LOFS_FLAG   		0x01   /* BIT 0th position */
#define ATUC_THRESH_15MIN_LOSS_FLAG   		0x02   /* BIT 1 */
#define ATUC_THRESH_15MIN_ESS_FLAG      	0x04   /* BIT 2 */
#define ATUC_THRESH_FAST_RATEUP_FLAG     	0x08   /* BIT 3 */
#define ATUC_THRESH_INTERLEAVE_RATEUP_FLAG	0x10   /* BIT 4 */
#define ATUC_THRESH_FAST_RATEDOWN_FLAG		0x20	 /* BIT 5 */
#define ATUC_THRESH_INTERLEAVE_RATEDOWN_FLAG   	0x40	/* BIT 6 */
#define ATUC_INIT_FAILURE_TRAP_ENABLE_FLAG	0x80 	/* BIT 7 */
#define ATUR_THRESH_15MIN_LOFS_FLAG   		0x100  	/* BIT 8 */
#define ATUR_THRESH_15MIN_LOSS_FLAG   		0x200  	/* BIT 9 */
#define ATUR_THRESH_15MIN_LPRS_FLAG    		0x400  	/* BIT 10 */
#define ATUR_THRESH_15MIN_ESS_FLAG      	0x800   	/* BIT 11 */
#define ATUR_THRESH_FAST_RATEUP_FLAG     	0x1000  	/* BIT 12 */
#define ATUR_THRESH_INTERLEAVE_RATEUP_FLAG	0x2000  	/* BIT 13 */
#define ATUR_THRESH_FAST_RATEDOWN_FLAG     	0x4000 	/* BIT 14 */
#define ATUR_THRESH_INTERLEAVE_RATEDOWN_FLAG	0x8000   	/* BIT 15 */
#define LINE_ALARM_CONF_PROFILE_ROWSTATUS_FLAG  0x10000   	/* BIT 16 */


/* adslAturTraps Flags */
#define ATUC_PERF_LOFS_THRESH_FLAG   	     	0x1 	/* BIT 0th position */
#define ATUC_PERF_LOSS_THRESH_FLAG  	 	0x2 	/* BIT 1 */
#define ATUC_PERF_ESS_THRESH_FLAG  	 	0x4 	/* BIT 2 */
#define ATUC_RATE_CHANGE_FLAG	  	 	0x8 	/* BIT 3 */
#define ATUR_PERF_LOFS_THRESH_FLAG   	     	0x10 	/* BIT 4 */
#define ATUR_PERF_LOSS_THRESH_FLAG  	 	0x20 	/* BIT 5 */
#define ATUR_PERF_LPRS_THRESH_FLAG  	 	0x40 	/* BIT 6 */
#define ATUR_PERF_ESS_THRESH_FLAG  	 	0x80 	/* BIT 7 */
#define ATUR_RATE_CHANGE_FLAG	  	 	0x100	/* BIT 8 */

//RFC- 3440 FLAG DEFINITIONS

#ifdef IFX_ADSL_MIB_RFC3440
/* adslLineExtTable flags */
#define ATUC_LINE_TRANS_CAP_FLAG	 	0x1		/* BIT 0th position */
#define ATUC_LINE_TRANS_CONFIG_FLAG	 	0x2		/* BIT 1 */
#define ATUC_LINE_TRANS_ACTUAL_FLAG	 	0x4		/* BIT 2 */
#define LINE_GLITE_POWER_STATE_FLAG 	 	0x8		/* BIT 3 */

/* adslAtucPerfDataExtTable flags */
#define ATUC_PERF_STAT_FASTR_FLAG	   0x1 /* BIT 0th position */
#define ATUC_PERF_STAT_FAILED_FASTR_FLAG 0x2 /* BIT 1 */
#define ATUC_PERF_STAT_SESL_FLAG 	   0X4	/* BIT 2 */
#define ATUC_PERF_STAT_UASL_FLAG		   0X8	/* BIT 3 */
#define ATUC_PERF_CURR_15MIN_FASTR_FLAG	   0X10	/* BIT 4 */
#define ATUC_PERF_CURR_15MIN_FAILED_FASTR_FLAG 0X20	/* BIT 5 */
#define ATUC_PERF_CURR_15MIN_SESL_FLAG	         0X40	/* BIT 6 */
#define ATUC_PERF_CURR_15MIN_UASL_FLAG		    0X80	/* BIT 7 */
#define ATUC_PERF_CURR_1DAY_FASTR_FLAG		    0X100	/* BIT 8 */
#define ATUC_PERF_CURR_1DAY_FAILED_FASTR_FLAG	0X200	/* BIT 9 */
#define ATUC_PERF_CURR_1DAY_SESL_FLAG			0X400	/* BIT 10 */
#define ATUC_PERF_CURR_1DAY_UASL_FLAG			0X800	/* BIT 11 */
#define ATUC_PERF_PREV_1DAY_FASTR_FLAG		     0X1000 /* BIT 12 */
#define ATUC_PERF_PREV_1DAY_FAILED_FASTR_FLAG	0X2000 /* BIT 13 */
#define ATUC_PERF_PREV_1DAY_SESL_FLAG			0X4000 /* BIT 14 */
#define ATUC_PERF_PREV_1DAY_UASL_FLAG			0X8000 /* BIT 15 */

/* adslAturPerfDataExtTable */
#define ATUR_PERF_STAT_SESL_FLAG		0X1 /* BIT 0th position */
#define ATUR_PERF_STAT_UASL_FLAG		0X2 /* BIT 1 */
#define ATUR_PERF_CURR_15MIN_SESL_FLAG		0X4 /* BIT 2 */
#define ATUR_PERF_CURR_15MIN_UASL_FLAG		0X8 /* BIT 3 */
#define ATUR_PERF_CURR_1DAY_SESL_FLAG		0X10 /* BIT 4 */
#define ATUR_PERF_CURR_1DAY_UASL_FLAG		0X20 /* BIT 5 */
#define ATUR_PERF_PREV_1DAY_SESL_FLAG		0X40 /* BIT 6 */
#define ATUR_PERF_PREV_1DAY_UASL_FLAG		0X80 /* BIT 7 */

/* adslAutcIntervalExtTable flags */
#define ATUC_INTERVAL_FASTR_FLAG		0x1 /* Bit 0 */
#define ATUC_INTERVAL_FAILED_FASTR_FLAG		0x2 /* Bit 1 */
#define ATUC_INTERVAL_SESL_FLAG			0x4 /* Bit 2 */
#define ATUC_INTERVAL_UASL_FLAG			0x8 /* Bit 3 */

/* adslAturIntervalExtTable */
#define ATUR_INTERVAL_SESL_FLAG		0X1 /* BIT 0th position */
#define ATUR_INTERVAL_UASL_FLAG		0X2 /* BIT 1 */

/* adslAlarmConfProfileExtTable */
#define ATUC_THRESH_15MIN_FAILED_FASTR_FLAG 0X1/* BIT 0th position */
#define ATUC_THRESH_15MIN_SESL_FLAG		 0X2 /* BIT 1 */
#define ATUC_THRESH_15MIN_UASL_FLAG		 0X4 /* BIT 2 */
#define ATUR_THRESH_15MIN_SESL_FLAG		 0X8 /* BIT 3 */
#define ATUR_THRESH_15MIN_UASL_FLAG		 0X10 /* BIT 4 */

/* adslAturExtTraps */
#define ATUC_15MIN_FAILED_FASTR_TRAP_FLAG 	0X1 /* BIT 0th position */
#define ATUC_15MIN_SESL_TRAP_FLAG		 0X2 /* BIT 1 */
#define ATUC_15MIN_UASL_TRAP_FLAG		 0X4 /* BIT 2 */
#define ATUR_15MIN_SESL_TRAP_FLAG		 0X8 /* BIT 3 */
#define ATUR_15MIN_UASL_TRAP_FLAG		 0X10 /* BIT 4 */

#endif

/* adslLineStatus Flags */
#define LINE_STAT_MODEM_STATUS_FLAG   	 0x1 /* BIT 0th position */
#define LINE_STAT_MODE_SEL_FLAG  	 0x2 /* BIT 1 */
#define LINE_STAT_TRELLCOD_ENABLE_FLAG 0x4 /* BIT 2 */
#define LINE_STAT_LATENCY_FLAG	  	 0x8 /* BIT 3 */

/* adslLineRate Flags */
#define LINE_RATE_DATA_RATEDS_FLAG   	0x1 /* BIT 0th position */




#define LINE_RATE_DATA_RATEUS_FLAG  	0x2 /* BIT 1 */




#define LINE_RATE_ATTNDRDS_FLAG  	0x4 /* BIT 2 */

#define LINE_RATE_ATTNDRUS_FLAG	  	0x8 /* BIT 3 */

/* adslLineInformation Flags */
#define LINE_INFO_INTLV_DEPTHDS_FLAG	0x1 /* BIT 0th position */
#define LINE_INFO_INTLV_DEPTHUS_FLAG	0x2 /* BIT 1 */
#define LINE_INFO_LATNDS_FLAG		0x4 /* BIT 2 */
#define LINE_INFO_LATNUS_FLAG	  	0x8 /* BIT 3 */
#define LINE_INFO_SATNDS_FLAG   	 	0x10 /* BIT 4 */
#define LINE_INFO_SATNUS_FLAG  	 	0x20 /* BIT 5 */
#define LINE_INFO_SNRMNDS_FLAG  	 	0x40 /* BIT 6 */
#define LINE_INFO_SNRMNUS_FLAG  	 	0x80 /* BIT 7 */
#define LINE_INFO_ACATPDS_FLAG	  	0x100 /* BIT 8 */
#define LINE_INFO_ACATPUS_FLAG	  	0x200 /* BIT 9 */

/* adslNearEndPerformanceStats Flags */
#define NEAREND_PERF_SUPERFRAME_FLAG	0x1 /* BIT 0th position */
#define NEAREND_PERF_LOS_FLAG		0x2 /* BIT 1 */
#define NEAREND_PERF_LOF_FLAG		0x4 /* BIT 2 */
#define NEAREND_PERF_LPR_FLAG		0x8 /* BIT 3 */
#define NEAREND_PERF_NCD_FLAG		0x10 /* BIT 4 */
#define NEAREND_PERF_LCD_FLAG		0x20 /* BIT 5 */
#define NEAREND_PERF_CRC_FLAG		0x40 /* BIT 6 */
#define NEAREND_PERF_RSCORR_FLAG	0x80 /* BIT 7 */
#define NEAREND_PERF_FECS_FLAG		0x100 /* BIT 8 */
#define NEAREND_PERF_ES_FLAG		0x200 /* BIT 9 */
#define NEAREND_PERF_SES_FLAG		0x400 /* BIT 10 */
#define NEAREND_PERF_LOSS_FLAG		0x800 /* BIT 11 */
#define NEAREND_PERF_UAS_FLAG		0x1000 /* BIT 12 */
#define NEAREND_PERF_HECERR_FLAG		0x2000 /* BIT 13 */

/* adslFarEndPerformanceStats Flags */
#define FAREND_PERF_LOS_FLAG	0x1 /* BIT 0th position */
#define FAREND_PERF_LOF_FLAG	0x2 /* BIT 1 */
#define FAREND_PERF_LPR_FLAG	0x4 /* BIT 2 */
#define FAREND_PERF_NCD_FLAG	0x8 /* BIT 3 */
#define FAREND_PERF_LCD_FLAG	0x10 /* BIT 4 */
#define FAREND_PERF_CRC_FLAG	0x20 /* BIT 5 */
#define FAREND_PERF_RSCORR_FLAG	0x40 /* BIT 6 */
#define FAREND_PERF_FECS_FLAG	0x80 /* BIT 7 */
#define FAREND_PERF_ES_FLAG	0x100 /* BIT 8 */
#define FAREND_PERF_SES_FLAG	0x200 /* BIT 9 */
#define FAREND_PERF_LOSS_FLAG	0x400 /* BIT 10 */
#define FAREND_PERF_UAS_FLAG	0x800 /* BIT 11 */
#define FAREND_PERF_HECERR_FLAG	0x1000 /* BIT 12 */
// 603221:tc.chen end
/* TR-69 related additional parameters - defines */
/* Defines for  struct adslATURSubcarrierInfo */
#define	NEAREND_HLINSC	0x1
#define	NEAREND_HLINPS	0x2
#define	NEAREND_HLOGMT	0x4
#define NEAREND_HLOGPS	0x8
#define NEAREND_QLNMT	0x10
#define	NEAREND_QLNPS	0x20
#define	NEAREND_SNRMT	0x40
#define	NEAREND_SNRPS	0x80
#define	NEAREND_BITPS	0x100
#define	NEAREND_GAINPS	0x200

/* Defines for  struct adslATUCSubcarrierInfo */
#define	 FAREND_HLINSC	0x1

/* As per the feedback from Knut on 21/08/2006, the cmv command of HLINSC should be INFO 70 2 */
#define	 FAREND_HLINPS	0x2
#define	 FAREND_HLOGMT	0x4
#define  FAREND_HLOGPS	0x8
#define  FAREND_QLNMT	0x10
#define	 FAREND_QLNPS	0x20
#define	 FAREND_SNRMT	0x40
#define	 FAREND_SNRPS	0x80
#define	 FAREND_BITPS	0x100
#define	 FAREND_GAINPS	0x200


// GET_ADSL_POWER_SPECTRAL_DENSITY

/////////////////////////////////////////////////Macro Definitions ? FLAG Setting & Testing

#define SET_FLAG(flags, flag_val)   ((*flags) = ((*flags) | flag_val))
//	-- This macro sets the flags with the flag_val. Here flags is passed as a pointer

#define IS_FLAG_SET(flags, test_flag)	(((*flags) & (test_flag)) == (test_flag)? test_flag:0)
// 	-- This macro verifies whether test_flag has been set in flags. Here flags is passed as a pointer


#define CLR_FLAG(flags, flag_bit)	((*flags) = (*flags) & (~flag_bit))
//	-- This macro resets the specified flag_bit in the flags. Here flags is passed as a pointer


////////////////////////////////////////////////DATA STRUCTURES ORGANIZATION

#define u32 unsigned int
#define u16 unsigned short
#define s16 short
#define u8 unsigned char

/** A type for handling boolean issues. */
typedef enum {
   /** false */
   DSL_FALSE = 0,
   /** true */
   DSL_TRUE = 1
} DSL_boolean_t;


/*
   Here are the data structures used for accessing mib parameters. The ioctl
   call includes the third parameter as a void pointer. This parameter has to
   be type-casted in the driver code to the corresponding structure depending
   upon the command type. For Ex: consider the ioctl used to get the
   adslLineCode type, ioctl(fd,GET_ADSL_LINE_CODE,void *struct_adslLineTableEntry).
   In the driver code we check on the type of the command,
   i.e GET_ADSL_LINE_CODE and type-cast the void pointer to
   struct adslLineTableEntry type.
*/

typedef u32 AdslPerfTimeElapsed;
typedef u32 AdslPerfPrevDayCount;
typedef u32 PerfCurrentCount;
typedef u32 PerfIntervalCount;
typedef u32 AdslPerfCurrDayCount;


//ioctl(int fd, GET_ADSL_LINE_CODE, void *struct_adslLineTableEntry)

typedef struct adslLineTableEntry {
	int ifIndex;
	int adslLineCode;
	u8 flags;
} adslLineTableEntry;

#ifdef IFX_ADSL_MIB_RFC3440
typedef struct adslLineExtTableEntry {
	int ifIndex;
	u16 adslLineTransAtucCap;
	u16 adslLineTransAtucConfig;
	u16 adslLineTransAtucActual;
	int adslLineGlitePowerState;
	u32 flags;
}adslLineExtTableEntry;
#endif
//ioctl(int fd, GET_ADSL_ATUC_PHY, void  *struct_adslAtucPhysEntry)
#ifndef u_char
#define u_char u8
#endif

typedef struct adslVendorId {
	u16	country_code;
	u_char	provider_id[4];  /* Ascii characters */
	u_char	revision_info[2];
}adslVendorId;

typedef struct adslAtucPhysEntry {
	int ifIndex;
	char serial_no[32];
	union {
		char vendor_id[16];
		adslVendorId vendor_info;
	} vendor_id;
	char version_no[16];
	u32 status;
	int outputPwr;
	u32 attainableRate;
	u8 flags;
} adslAtucPhysEntry;


//ioctl(int fd, GET_ADSL_ATUR_PHY, void  *struct_adslAturPhysEntry)

typedef struct adslAturPhysEntry {
	int ifIndex;
	char serial_no[32];
	union {
	char vendor_id[16];
		adslVendorId vendor_info;
	} vendor_id;
	char version_no[16];
	int SnrMgn;
	u32 Attn;
	u32 status;
	int outputPwr;
	u32 attainableRate;
	u8 flags;
} adslAturPhysEntry;


//ioctl(int fd, GET_ADSL_ATUC_CHAN_INFO, void *struct_adslAtucChanInfo)

typedef struct adslAtucChanInfo {
	int ifIndex;
 	u32 interleaveDelay;
	u32 currTxRate;
	u32 prevTxRate;
	u8 flags;
} adslAtucChanInfo;


//ioctl(int fd, GET_ADSL_ATUR_CHAN_INFO, void *struct_adslAturChanInfo)

typedef struct adslAturChanInfo {
	int ifIndex;
 	u32 interleaveDelay;
 	u32 currTxRate;
 	u32 prevTxRate;
 	u32 crcBlkLen;
 	u8 flags;
} adslAturChanInfo;


//ioctl(int fd, GET_ADSL_ATUC_PERF_DATA,  void *struct_atucPerfDataEntry)

typedef struct atucPerfDataEntry
{
   int			ifIndex;
   u32 			adslAtucPerfLofs;
   u32 			adslAtucPerfLoss;
   u32 			adslAtucPerfESs;
   u32 			adslAtucPerfInits;
   int         		adslAtucPerfValidIntervals;
   int         		adslAtucPerfInvalidIntervals;
   AdslPerfTimeElapsed 	adslAtucPerfCurr15MinTimeElapsed;
   PerfCurrentCount 	adslAtucPerfCurr15MinLofs;
   PerfCurrentCount 	adslAtucPerfCurr15MinLoss;
   PerfCurrentCount 	adslAtucPerfCurr15MinESs;
   PerfCurrentCount 	adslAtucPerfCurr15MinInits;
   AdslPerfTimeElapsed 	adslAtucPerfCurr1DayTimeElapsed;
   AdslPerfCurrDayCount adslAtucPerfCurr1DayLofs;
   AdslPerfCurrDayCount adslAtucPerfCurr1DayLoss;
   AdslPerfCurrDayCount adslAtucPerfCurr1DayESs;
   AdslPerfCurrDayCount adslAtucPerfCurr1DayInits;
   int         		adslAtucPerfPrev1DayMoniSecs;
   AdslPerfPrevDayCount adslAtucPerfPrev1DayLofs;
   AdslPerfPrevDayCount adslAtucPerfPrev1DayLoss;
   AdslPerfPrevDayCount adslAtucPerfPrev1DayESs;
   AdslPerfPrevDayCount adslAtucPerfPrev1DayInits;
   u32			flags;
} atucPerfDataEntry;

#ifdef IFX_ADSL_MIB_RFC3440
typedef struct atucPerfDataExtEntry
 {
  int ifIndex;
  u32 adslAtucPerfStatFastR;
  u32 adslAtucPerfStatFailedFastR;
  u32 adslAtucPerfStatSesL;
  u32 adslAtucPerfStatUasL;
  u32 adslAtucPerfCurr15MinFastR;
  u32 adslAtucPerfCurr15MinFailedFastR;
  u32 adslAtucPerfCurr15MinSesL;
  u32 adslAtucPerfCurr15MinUasL;
  u32 adslAtucPerfCurr1DayFastR;
  u32 adslAtucPerfCurr1DayFailedFastR;
  u32 adslAtucPerfCurr1DaySesL;
  u32 adslAtucPerfCurr1DayUasL;
  u32 adslAtucPerfPrev1DayFastR;
  u32 adslAtucPerfPrev1DayFailedFastR;
  u32 adslAtucPerfPrev1DaySesL;
  u32 adslAtucPerfPrev1DayUasL;
  u32	flags;
} atucPerfDataExtEntry;

#endif
//ioctl(int fd, GET_ADSL_ATUR_PERF_DATA, void *struct_aturPerfDataEntry)

typedef struct aturPerfDataEntry
{
   int			ifIndex;
   u32 			adslAturPerfLofs;
   u32 			adslAturPerfLoss;
   u32 			adslAturPerfLprs;
   u32 			adslAturPerfESs;
   int         		adslAturPerfValidIntervals;
   int         		adslAturPerfInvalidIntervals;
   AdslPerfTimeElapsed 	adslAturPerfCurr15MinTimeElapsed;
   PerfCurrentCount 	adslAturPerfCurr15MinLofs;
   PerfCurrentCount 	adslAturPerfCurr15MinLoss;
   PerfCurrentCount 	adslAturPerfCurr15MinLprs;
   PerfCurrentCount 	adslAturPerfCurr15MinESs;
   AdslPerfTimeElapsed 	adslAturPerfCurr1DayTimeElapsed;
   AdslPerfCurrDayCount adslAturPerfCurr1DayLofs;
   AdslPerfCurrDayCount adslAturPerfCurr1DayLoss;
   AdslPerfCurrDayCount adslAturPerfCurr1DayLprs;
   AdslPerfCurrDayCount adslAturPerfCurr1DayESs;
   int         		adslAturPerfPrev1DayMoniSecs;
   AdslPerfPrevDayCount adslAturPerfPrev1DayLofs;
   AdslPerfPrevDayCount adslAturPerfPrev1DayLoss;
   AdslPerfPrevDayCount adslAturPerfPrev1DayLprs;
   AdslPerfPrevDayCount adslAturPerfPrev1DayESs;
   u32			flags;
} aturPerfDataEntry;

#ifdef IFX_ADSL_MIB_RFC3440
typedef struct aturPerfDataExtEntry
 {
  int ifIndex;
  u32 adslAturPerfStatSesL;
  u32 adslAturPerfStatUasL;
  u32 adslAturPerfCurr15MinSesL;
  u32 adslAturPerfCurr15MinUasL;
  u32 adslAturPerfCurr1DaySesL;
  u32 adslAturPerfCurr1DayUasL;
  u32 adslAturPerfPrev1DaySesL;
  u32 adslAturPerfPrev1DayUasL;
  u32	flags;
} aturPerfDataExtEntry;
#endif
//ioctl(int fd, GET_ADSL_ATUC_INTVL_INFO, void *struct_adslAtucInvtInfo)

typedef struct adslAtucIntvlInfo {
	int ifIndex;
        int IntervalNumber;
 	PerfIntervalCount intervalLOF;
 	PerfIntervalCount intervalLOS;
  	PerfIntervalCount intervalES;
 	PerfIntervalCount intervalInits;
	int intervalValidData;
 	u8 flags;
} adslAtucIntvlInfo;

#ifdef IFX_ADSL_MIB_RFC3440
typedef struct adslAtucInvtlExtInfo
 {
  int ifIndex;
  int IntervalNumber;
  u32 adslAtucIntervalFastR;
  u32 adslAtucIntervalFailedFastR;
  u32 adslAtucIntervalSesL;
  u32 adslAtucIntervalUasL;
  u32	flags;
} adslAtucInvtlExtInfo;
#endif
//ioctl(int fd, GET_ADSL_ATUR_INTVL_INFO, void *struct_adslAturInvtlInfo)

typedef struct adslAturIntvlInfo {
	int ifIndex;
        int IntervalNumber;
 	PerfIntervalCount intervalLOF;
 	PerfIntervalCount intervalLOS;
 	PerfIntervalCount intervalLPR;
  	PerfIntervalCount intervalES;
 	int intervalValidData;
 	u8 flags;
} adslAturIntvlInfo;

#ifdef IFX_ADSL_MIB_RFC3440
typedef struct adslAturInvtlExtInfo
 {
  int ifIndex;
  int IntervalNumber;
  u32 adslAturIntervalSesL;
  u32 adslAturIntervalUasL;
  u32	flags;
} adslAturInvtlExtInfo;
#endif
//ioctl(int fd, GET_ADSL_ATUC_CHAN_PERF_DATA,  void *struct_atucChannelPerfDataEntry)

typedef struct atucChannelPerfDataEntry
{
   int			ifIndex;
   u32 			adslAtucChanReceivedBlks;
   u32 			adslAtucChanTransmittedBlks;
   u32 			adslAtucChanCorrectedBlks;
   u32 			adslAtucChanUncorrectBlks;
   int         		adslAtucChanPerfValidIntervals;
   int         		adslAtucChanPerfInvalidIntervals;
   AdslPerfTimeElapsed 	adslAtucChanPerfCurr15MinTimeElapsed;
   PerfCurrentCount 	adslAtucChanPerfCurr15MinReceivedBlks;
   PerfCurrentCount 	adslAtucChanPerfCurr15MinTransmittedBlks;
   PerfCurrentCount 	adslAtucChanPerfCurr15MinCorrectedBlks;
   PerfCurrentCount 	adslAtucChanPerfCurr15MinUncorrectBlks;
   AdslPerfTimeElapsed  adslAtucChanPerfCurr1DayTimeElapsed;
   AdslPerfCurrDayCount adslAtucChanPerfCurr1DayReceivedBlks;
   AdslPerfCurrDayCount adslAtucChanPerfCurr1DayTransmittedBlks;
   AdslPerfCurrDayCount adslAtucChanPerfCurr1DayCorrectedBlks;
   AdslPerfCurrDayCount adslAtucChanPerfCurr1DayUncorrectBlks;
   int                  adslAtucChanPerfPrev1DayMoniSecs;
   AdslPerfPrevDayCount adslAtucChanPerfPrev1DayReceivedBlks;
   AdslPerfPrevDayCount adslAtucChanPerfPrev1DayTransmittedBlks;
   AdslPerfPrevDayCount adslAtucChanPerfPrev1DayCorrectedBlks;
   AdslPerfPrevDayCount adslAtucChanPerfPrev1DayUncorrectBlks;
   u32			flags;
}atucChannelPerfDataEntry;


//ioctl(int fd, GET_ADSL_ATUR_CHAN_PERF_DATA,  void *struct_aturChannelPerfDataEntry)

typedef struct aturChannelPerfDataEntry
{
   int			ifIndex;
   u32 			adslAturChanReceivedBlks;
   u32 			adslAturChanTransmittedBlks;
   u32 			adslAturChanCorrectedBlks;
   u32 			adslAturChanUncorrectBlks;
   int         		adslAturChanPerfValidIntervals;
   int         		adslAturChanPerfInvalidIntervals;
   AdslPerfTimeElapsed 	adslAturChanPerfCurr15MinTimeElapsed;
   PerfCurrentCount 	adslAturChanPerfCurr15MinReceivedBlks;
   PerfCurrentCount 	adslAturChanPerfCurr15MinTransmittedBlks;
   PerfCurrentCount 	adslAturChanPerfCurr15MinCorrectedBlks;
   PerfCurrentCount 	adslAturChanPerfCurr15MinUncorrectBlks;
   AdslPerfTimeElapsed  adslAturChanPerfCurr1DayTimeElapsed;
   AdslPerfCurrDayCount adslAturChanPerfCurr1DayReceivedBlks;
   AdslPerfCurrDayCount adslAturChanPerfCurr1DayTransmittedBlks;
   AdslPerfCurrDayCount adslAturChanPerfCurr1DayCorrectedBlks;
   AdslPerfCurrDayCount adslAturChanPerfCurr1DayUncorrectBlks;
   int                  adslAturChanPerfPrev1DayMoniSecs;
   AdslPerfPrevDayCount adslAturChanPerfPrev1DayReceivedBlks;
   AdslPerfPrevDayCount adslAturChanPerfPrev1DayTransmittedBlks;
   AdslPerfPrevDayCount adslAturChanPerfPrev1DayCorrectedBlks;
   AdslPerfPrevDayCount adslAturChanPerfPrev1DayUncorrectBlks;
   u32			flags;
} aturChannelPerfDataEntry;


//ioctl(int fd, GET_ADSL_ATUC_CHAN_INTVL_INFO, void *struct_adslAtucChanIntvlInfo)

typedef struct adslAtucChanIntvlInfo {
	int ifIndex;
        int IntervalNumber;
 	PerfIntervalCount chanIntervalRecvdBlks;
 	PerfIntervalCount chanIntervalXmitBlks;
  	PerfIntervalCount chanIntervalCorrectedBlks;
 	PerfIntervalCount chanIntervalUncorrectBlks;
 	int intervalValidData;
 	u8 flags;
} adslAtucChanIntvlInfo;


//ioctl(int fd, GET_ADSL_ATUR_CHAN_INTVL_INFO, void *struct_adslAturChanIntvlInfo)

typedef struct adslAturChanIntvlInfo {
	int ifIndex;
        int IntervalNumber;
 	PerfIntervalCount chanIntervalRecvdBlks;
 	PerfIntervalCount chanIntervalXmitBlks;
  	PerfIntervalCount chanIntervalCorrectedBlks;
 	PerfIntervalCount chanIntervalUncorrectBlks;
 	int intervalValidData;
	u8 flags;
} adslAturChanIntvlInfo;


//ioctl(int fd, GET_ADSL_ALRM_CONF_PROF,  void *struct_adslLineAlarmConfProfileEntry)
//ioctl(int fd, SET_ADSL_ALRM_CONF_PROF,  void *struct_adslLineAlarmConfProfileEntry)

typedef struct  adslLineAlarmConfProfileEntry
 {
  unsigned char adslLineAlarmConfProfileName[32];
    int 	adslAtucThresh15MinLofs;
    int 	adslAtucThresh15MinLoss;
    int 	adslAtucThresh15MinESs;
    u32 	adslAtucThreshFastRateUp;
    u32 	adslAtucThreshInterleaveRateUp;
    u32 	adslAtucThreshFastRateDown;
    u32 	adslAtucThreshInterleaveRateDown;
    int 	adslAtucInitFailureTrapEnable;
    int 	adslAturThresh15MinLofs;
    int 	adslAturThresh15MinLoss;
    int 	adslAturThresh15MinLprs;
    int 	adslAturThresh15MinESs;
    u32 	adslAturThreshFastRateUp;
    u32 	adslAturThreshInterleaveRateUp;
    u32 	adslAturThreshFastRateDown;
    u32 	adslAturThreshInterleaveRateDown;
    int 	adslLineAlarmConfProfileRowStatus;
    u32	flags;
} adslLineAlarmConfProfileEntry;

#ifdef IFX_ADSL_MIB_RFC3440
typedef struct adslLineAlarmConfProfileExtEntry
 {
  u8  adslLineAlarmConfProfileExtName[32];
  u32 adslAtucThreshold15MinFailedFastR;
  u32 adslAtucThreshold15MinSesL;
  u32 adslAtucThreshold15MinUasL;
  u32 adslAturThreshold15MinSesL;
  u32 adslAturThreshold15MinUasL;
  u32	flags;
} adslLineAlarmConfProfileExtEntry;
#endif
//TRAPS

/* The following Data Sturctures are added to support the WEB related parameters for ADSL Statistics */
typedef struct  adslLineStatus
 {
    int 	adslModemStatus;
    u32 	adslModeSelected;
    int 	adslAtucThresh15MinESs;
    int 	adslTrellisCodeEnable;
    int 	adslLatency;
    u8 flags;
 } adslLineStatusInfo;

typedef struct  adslLineRate
 {
    u32 	adslDataRateds;
    u32 	adslDataRateus;
    u32 	adslATTNDRds;
    u32 	adslATTNDRus;
    u8		flags;
 } adslLineRateInfo;

typedef struct  adslLineInfo
 {
    u32 	adslInterleaveDepthds;
    u32 	adslInterleaveDepthus;
    u32 	adslLATNds;
    u32 	adslLATNus;
    u32 	adslSATNds;
    u32 	adslSATNus;
    int	 	adslSNRMds;
    int	 	adslSNRMus;
    int	 	adslACATPds;
    int		adslACATPus;
    u32	flags;
 } adslLineInfo;

typedef struct  adslNearEndPerfStats
 {
    u32 	adslSuperFrames;
    u32 	adslneLOS;
    u32 	adslneLOF;
    u32 	adslneLPR;
    u32 	adslneNCD;
    u32 	adslneLCD;
    u32 	adslneCRC;
    u32		adslneRSCorr;
    u32		adslneFECS;
    u32		adslneES;
    u32		adslneSES;
    u32		adslneLOSS;
    u32		adslneUAS;
    u32		adslneHECErrors;
    u32		flags;
 } adslNearEndPerfStats;

typedef struct  adslFarEndPerfStats
 {
    u32 	adslfeLOS;
    u32 	adslfeLOF;
    u32 	adslfeLPR;
    u32 	adslfeNCD;
    u32 	adslfeLCD;
    u32 	adslfeCRC;
    u32		adslfeRSCorr;
    u32		adslfeFECS;
    u32		adslfeES;
    u32		adslfeSES;
    u32		adslfeLOSS;
    u32		adslfeUAS;
    u32		adslfeHECErrors;
    u32		flags;
 } adslFarEndPerfStats;

/* The number of tones (and hence indexes) is dependent on the ADSL mode - G.992.1, G.992.2, G.992.3, * G.992.4 and G.992.5 */
typedef struct adslATURSubcarrierInfo {
	int 	ifindex;
	u16	HLINSCds;
	u16	HLINpsds[1024];/* Even index = real part; Odd Index
				    = imaginary part for each tone */
	u16	HLOGMTds;
	u16	HLOGpsds[512];
	u16	QLNMTds;
	u16	QLNpsds[512];
	u16	SNRMTds;
	u16	SNRpsds[512];
	u16	BITpsds[512];
	s16	GAINpsds[512]; /* Signed value in 0.1dB units. i.e dB * 10.
				Needs to be converted into linear scale*/
	u16	flags;
}adslATURSubcarrierInfo;

typedef struct adslATUCSubcarrierInfo {
	int 	ifindex;
	u16	HLINSCus;
	u16	HLINpsus[128];/* Even index = real part; Odd Index
				    = imaginary part for each tone */
	u16	HLOGMTus;
	u16	HLOGpsus[64];
	u16	QLNMTus;
	u16	QLNpsus[64];
	u16	SNRMTus;
	u16 	SNRpsus[64];
	u16	BITpsus[64];
	s16	GAINpsus[64]; /* Signed value in 0.1dB units. i.e dB * 10.
				Needs to be converted into linear scale*/
	u16	flags;
}adslATUCSubcarrierInfo;

#ifndef u_int16
#define u_int16 u16
#endif

typedef struct adslInitStats {
	u_int16	FullInitializationCount;
	u_int16 FailedFullInitializationCount;
	u_int16 LINIT_Errors;
	u_int16	Init_Timeouts;
}adslInitStats;

typedef struct adslPowerSpectralDensity {
	int	ACTPSDds;
	int 	ACTPSDus;
}adslPowerSpectralDensity;

//ioctl(int fd, ADSL_ATUR_TRAPS, void  *uint16_flags)
typedef union structpts {
	adslLineTableEntry * adslLineTableEntry_pt;
	adslAtucPhysEntry * adslAtucPhysEntry_pt;
	adslAturPhysEntry * adslAturPhysEntry_pt;
	adslAtucChanInfo * adslAtucChanInfo_pt;
	adslAturChanInfo * adslAturChanInfo_pt;
	atucPerfDataEntry * atucPerfDataEntry_pt;
	aturPerfDataEntry * aturPerfDataEntry_pt;
	adslAtucIntvlInfo * adslAtucIntvlInfo_pt;
	adslAturIntvlInfo * adslAturIntvlInfo_pt;
	atucChannelPerfDataEntry * atucChannelPerfDataEntry_pt;
	aturChannelPerfDataEntry * aturChannelPerfDataEntry_pt;
	adslAtucChanIntvlInfo * adslAtucChanIntvlInfo_pt;
	adslAturChanIntvlInfo * adslAturChanIntvlInfo_pt;
	adslLineAlarmConfProfileEntry * adslLineAlarmConfProfileEntry_pt;
	// RFC 3440

    #ifdef IFX_ADSL_MIB_RFC3440
	adslLineExtTableEntry * adslLineExtTableEntry_pt;
	atucPerfDataExtEntry * atucPerfDataExtEntry_pt;
	adslAtucInvtlExtInfo * adslAtucInvtlExtInfo_pt;
	aturPerfDataExtEntry * aturPerfDataExtEntry_pt;
	adslAturInvtlExtInfo * adslAturInvtlExtInfo_pt;
	adslLineAlarmConfProfileExtEntry * adslLineAlarmConfProfileExtEntry_pt;
    #endif
    	adslLineStatusInfo	* adslLineStatusInfo_pt;
    	adslLineRateInfo	* adslLineRateInfo_pt;
    	adslLineInfo		* adslLineInfo_pt;
    	adslNearEndPerfStats	* adslNearEndPerfStats_pt;
    	adslFarEndPerfStats	* adslFarEndPerfStats_pt;
	adslATUCSubcarrierInfo  * adslATUCSubcarrierInfo_pt;
	adslATURSubcarrierInfo  * adslATURSubcarrierInfo_pt;
	adslPowerSpectralDensity * adslPowerSpectralDensity_pt;
}structpts;

#endif /* ] __IFX_ADSL_APP_IOCTL_H */
