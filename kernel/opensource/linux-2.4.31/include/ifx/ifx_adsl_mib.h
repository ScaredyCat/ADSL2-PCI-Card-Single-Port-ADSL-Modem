/*
 * ########################################################################
 *
 *  This program is free softwavre; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 *
 */
 
#ifndef         _IFX_ADSL_MIB_H
#define        	_IFX_ADSL_MIB_H

/////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__KERNEL__) || defined (IFX_ADSL_PORT_RTEMS)


#define MIB_INTERVAL		10000 	//msec

// Number of intervals
#define INTERVAL_NUM					192 //two days
typedef struct ifx_adsl_mib{
	struct list_head list;
	struct timeval start_time;	//start of current interval
	
	int AtucPerfLof;
	int AtucPerfLos;
	int AtucPerfEs;
	int AtucPerfInit;
	
	int AturPerfLof;
	int AturPerfLos;
	int AturPerfLpr;
	int AturPerfEs;
	
	int AturChanPerfRxBlk;
	int AturChanPerfTxBlk;
	int AturChanPerfCorrBlk;
	int AturChanPerfUncorrBlk;
	
	//RFC-3440
	int AtucPerfStatFastR;
	int AtucPerfStatFailedFastR;
	int AtucPerfStatSesL;
	int AtucPerfStatUasL;
	int AturPerfStatSesL;
	int AturPerfStatUasL;
}ifx_adsl_mib;

typedef struct adslChanPrevTxRate{
	u32 adslAtucChanPrevTxRate;
	u32 adslAturChanPrevTxRate;
}adslChanPrevTxRate;

typedef struct adslPhysCurrStatus{
	u32 adslAtucCurrStatus;
	u32 adslAturCurrStatus;
}adslPhysCurrStatus;

typedef struct ChanType{
	int interleave;
	int fast;
	int bearchannel0;
	int bearchannel1;
}ChanType;

typedef struct mib_previous_read{
	u16 ATUC_PERF_ESS;
	u16 ATUR_PERF_ESS;
	u32 ATUR_CHAN_RECV_BLK;
	u16 ATUR_CHAN_CORR_BLK_INTL;
	u16 ATUR_CHAN_CORR_BLK_FAST;
	u16 ATUR_CHAN_UNCORR_BLK_INTL;
	u16 ATUR_CHAN_UNCORR_BLK_FAST;
	u16 ATUC_PERF_STAT_FASTR;
	u16 ATUC_PERF_STAT_FAILED_FASTR;
	u16 ATUC_PERF_STAT_SESL;
	u16 ATUC_PERF_STAT_UASL;
	u16 ATUR_PERF_STAT_SESL;
}mib_previous_read;

typedef struct mib_flags_pretime{
	struct timeval ATUC_PERF_LOSS_PTIME;
	struct timeval ATUC_PERF_LOFS_PTIME;
	struct timeval ATUR_PERF_LOSS_PTIME;
	struct timeval ATUR_PERF_LOFS_PTIME;
	struct timeval ATUR_PERF_LPR_PTIME;
}mib_flags_pretime;

#endif

#endif //_IFX_ADSL_MIB_H

