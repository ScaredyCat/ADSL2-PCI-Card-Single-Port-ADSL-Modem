#ifndef __DANUBE_MEI_MIB_CMV_H
#define __DANUBE_MEI_MIB_CMV_H

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

#define ATUC_PHY_SER_NUM_FLAG_MAKECMV1	makeCMV(H2D_CMV_READ, INFO, 57, 0, 12, data,TxMessage)
#define ATUC_PHY_SER_NUM_FLAG_MAKECMV2	makeCMV(H2D_CMV_READ, INFO, 57, 12, 4, data,TxMessage)
#define ATUC_PHY_VENDOR_ID_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 64, 0, 4, data,TxMessage)
#define ATUC_PHY_VER_NUM_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 58, 0, 8, data,TxMessage)

#if 0 /* TD20060915 */
#define ATUC_CURR_OUT_PWR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 68, 5, 1, data,TxMessage)
#define ATUC_CURR_ATTR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 69, 0, 2, data,TxMessage)
#define ATUC_CHAN_INTLV_DELAY_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, RATE, 3, 1, 1, data,TxMessage)
#else
#define ATUC_CURR_OUT_PWR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 68, 6, 1, data,TxMessage)
#define ATUC_CURR_ATTR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 68, 4, 2, data,TxMessage)
#define ATUC_CHAN_INTLV_DELAY_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 92, 1, 1, data,TxMessage)
#endif

#define ATUC_CHAN_CURR_TX_RATE_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, RATE, 1, 0, 2, data,TxMessage)
#define ATUR_PHY_SER_NUM_FLAG_MAKECMV1	makeCMV(H2D_CMV_READ, INFO, 62, 0, 12, data,TxMessage)
#define ATUR_PHY_SER_NUM_FLAG_MAKECMV2	makeCMV(H2D_CMV_READ, INFO, 62, 12, 4, data,TxMessage)
#define ATUR_PHY_VENDOR_ID_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 65, 0, 4, data,TxMessage)
#define ATUR_PHY_VER_NUM_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 61, 0, 8, data,TxMessage)

#if 0 /* [ Ritesh. Use PLAM 45 0 for 0.1dB resolution rather than INFO 68 3 */
#define ATUR_SNRMGN_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 68, 4, 1, data,TxMessage)
#else
#define ATUR_SNRMGN_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, PLAM_SNRMargin_0_1db, 0, 1, data, TxMessage)
#endif

#define ATUR_ATTN_FLAG_MAKECMV		makeCMV(H2D_CMV_READ, INFO, 68, 2, 1, data,TxMessage)

#if 0 /* TD20060915 */
#define ATUR_CURR_OUT_PWR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 69, 5, 1, data,TxMessage)
#define ATUR_CURR_ATTR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 68, 0, 2, data,TxMessage)
#define ATUR_CHAN_INTLV_DELAY_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, RATE, 2, 1, 1, data,TxMessage)
#else
#define ATUR_CURR_OUT_PWR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 69, 6, 1, data,TxMessage)
#define ATUR_CURR_ATTR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 69, 4, 2, data,TxMessage)
#define ATUR_CHAN_INTLV_DELAY_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 93, 1, 1, data,TxMessage)
#endif

#define ATUR_CHAN_CURR_TX_RATE_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, RATE, 0, 0, 2, data,TxMessage)
#define ATUC_PERF_LO_FLAG_MAKECMV		makeCMV(H2D_CMV_READ, PLAM, 0, 0, 1, data,TxMessage)
#define ATUC_PERF_ESS_FLAG_MAKECMV		makeCMV(H2D_CMV_READ, PLAM, 7, 0, 1, data,TxMessage)
#define ATUR_PERF_LO_FLAG_MAKECMV		makeCMV(H2D_CMV_READ, PLAM, 1, 0, 1, data,TxMessage)
#define ATUR_PERF_ESS_FLAG_MAKECMV		makeCMV(H2D_CMV_READ, PLAM, 33, 0, 1, data,TxMessage)
#define ATUR_CHAN_RECV_BLK_FLAG_MAKECMV_LSW		makeCMV(H2D_CMV_READ, PLAM, 20, 0, 1, data,TxMessage)
#define ATUR_CHAN_RECV_BLK_FLAG_MAKECMV_MSW		makeCMV(H2D_CMV_READ, PLAM, 21, 0, 1, data,TxMessage)
#define ATUR_CHAN_TX_BLK_FLAG_MAKECMV_LSW		makeCMV(H2D_CMV_READ, PLAM, 20, 0, 1, data,TxMessage)
#define ATUR_CHAN_TX_BLK_FLAG_MAKECMV_MSW		makeCMV(H2D_CMV_READ, PLAM, 21, 0, 1, data,TxMessage)
#define ATUR_CHAN_CORR_BLK_FLAG_MAKECMV_INTL		makeCMV(H2D_CMV_READ, PLAM, 3, 0, 1, data,TxMessage)
#define ATUR_CHAN_CORR_BLK_FLAG_MAKECMV_FAST		makeCMV(H2D_CMV_READ, PLAM, 3, 1, 1, data,TxMessage)
#define ATUR_CHAN_UNCORR_BLK_FLAG_MAKECMV_INTL		makeCMV(H2D_CMV_READ, PLAM, 2, 0, 1, data,TxMessage)
#define ATUR_CHAN_UNCORR_BLK_FLAG_MAKECMV_FAST		makeCMV(H2D_CMV_READ, PLAM, 2, 1, 1, data,TxMessage)
#define ATUC_LINE_TRANS_CAP_FLAG_MAKECMV	makeCMV(H2D_CMV_READ,INFO, 67, 0, 1, data,TxMessage)
#define ATUC_LINE_TRANS_CONFIG_FLAG_MAKECMV	makeCMV(H2D_CMV_READ,INFO, 67, 0, 1, data,TxMessage)
#define ATUC_LINE_TRANS_CONFIG_FLAG_MAKECMV_WR	makeCMV(H2D_CMV_WRITE,INFO, 67, 0, 1, data,TxMessage)
#define ATUC_LINE_TRANS_ACTUAL_FLAG_MAKECMV	makeCMV(H2D_CMV_READ,STAT, 1, 0, 1, data,TxMessage)
#define LINE_GLITE_POWER_STATE_FLAG_MAKECMV	makeCMV(H2D_CMV_READ,STAT, 0, 0, 1, data,TxMessage)
#define ATUC_PERF_STAT_FASTR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, STAT, 0, 0, 1, data, TxMessage)
#define ATUC_PERF_STAT_FAILED_FASTR_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, STAT, 0, 0, 1, data, TxMessage)
#define ATUC_PERF_STAT_SESL_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 8, 0, 1, data, TxMessage)
#define ATUC_PERF_STAT_UASL_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 10, 0, 1, data, TxMessage)
#define ATUR_PERF_STAT_SESL_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 34, 0, 1, data, TxMessage)
#define ATUR_PERF_STAT_UASL_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 36, 0, 1, data, TxMessage)
#define LINE_STAT_MODEM_STATUS_FLAG_MAKECMV makeCMV(H2D_CMV_READ, STAT, 0, 0, 1, data, TxMessage)
#define LINE_STAT_MODE_SEL_FLAG_MAKECMV makeCMV(H2D_CMV_READ, STAT, 1, 0, 1, data, TxMessage)
#define LINE_STAT_TRELLCOD_ENABLE_FLAG_MAKECMV makeCMV(H2D_CMV_READ, OPTN, 2, 0, 1, data, TxMessage)
#define LINE_STAT_LATENCY_FLAG_MAKECMV makeCMV(H2D_CMV_READ, STAT, 12, 0, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL1_LP0_MAKECMV makeCMV(H2D_CMV_READ, RATE, 1, 0, 2, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL1_LP1_MAKECMV makeCMV(H2D_CMV_READ, RATE, 1, 2, 2, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_RP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 12, 0, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_MP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 13, 0, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_LP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 14, 0, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_TP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 15, 0, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_KP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 17, 0, 2, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_RP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 12, 1, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_MP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 13, 1, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_LP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 14, 1, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_TP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 15, 1, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEDS_FLAG_ADSL2_KP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 17, 2, 2, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL1_LP0_MAKECMV makeCMV(H2D_CMV_READ, RATE, 0, 0, 2, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL1_LP1_MAKECMV makeCMV(H2D_CMV_READ, RATE, 0, 2, 2, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_RP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 23, 0, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_MP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 24, 0, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_LP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 25, 0, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_TP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 26, 0, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_KP_LP0_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 28, 0, 2, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_RP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 23, 1, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_MP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 24, 1, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_LP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 25, 1, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_TP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 26, 1, 1, data, TxMessage)
#define LINE_RATE_DATA_RATEUS_FLAG_ADSL2_KP_LP1_MAKECMV makeCMV(H2D_CMV_READ, CNFG, 28, 2, 2, data, TxMessage)
#define LINE_RATE_ATTNDRDS_FLAG_MAKECMV makeCMV(H2D_CMV_READ, INFO, 68, 4, 2, data, TxMessage)
#define LINE_RATE_ATTNDRUS_FLAG_MAKECMV makeCMV(H2D_CMV_READ, INFO, 69, 4, 2, data, TxMessage)
#define LINE_INFO_INTLV_DEPTHDS_FLAG_LP0_MAKECMV	makeCMV(H2D_CMV_READ, CNFG, 27, 0, 1, data, TxMessage)
#define LINE_INFO_INTLV_DEPTHDS_FLAG_LP1_MAKECMV	makeCMV(H2D_CMV_READ, CNFG, 27, 1, 1, data, TxMessage)
#define LINE_INFO_INTLV_DEPTHUS_FLAG_LP0_MAKECMV	makeCMV(H2D_CMV_READ, CNFG, 16, 0, 1, data, TxMessage)
#define LINE_INFO_INTLV_DEPTHUS_FLAG_LP1_MAKECMV	makeCMV(H2D_CMV_READ, CNFG, 16, 1, 1, data, TxMessage)
#define LINE_INFO_LATNDS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 68, 1, 1, data, TxMessage)
#define LINE_INFO_LATNUS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 69, 1, 1, data, TxMessage)
#define LINE_INFO_SATNDS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 68, 2, 1, data, TxMessage)
#define LINE_INFO_SATNUS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 69, 2, 1, data, TxMessage)
#define LINE_INFO_SNRMNDS_FLAG_ADSL1_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 68, 3, 1, data, TxMessage)
#define LINE_INFO_SNRMNDS_FLAG_ADSL2_MAKECMV	makeCMV(H2D_CMV_READ, RATE, 3, 0, 1, data, TxMessage)
#define LINE_INFO_SNRMNDS_FLAG_ADSL2PLUS_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 46, 0, 1, data, TxMessage)
#define LINE_INFO_SNRMNUS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 69, 3, 1, data, TxMessage)
#define LINE_INFO_ACATPDS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 68, 6, 1, data, TxMessage)
#define LINE_INFO_ACATPUS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 69, 6, 1, data, TxMessage)
#define NEAREND_PERF_SUPERFRAME_FLAG_LSW_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 20, 0, 1, data, TxMessage)
#define NEAREND_PERF_SUPERFRAME_FLAG_MSW_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 21, 0, 1, data, TxMessage)
#define NEAREND_PERF_LOS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 0, 0, 1, data, TxMessage)
#define NEAREND_PERF_CRC_FLAG_LP0_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 2, 0, 1, data, TxMessage)
#define NEAREND_PERF_CRC_FLAG_LP1_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 2, 1, 1, data, TxMessage)
#define NEAREND_PERF_RSCORR_FLAG_LP0_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 3, 0, 1, data, TxMessage)
#define NEAREND_PERF_RSCORR_FLAG_LP1_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 3, 1, 1, data, TxMessage)
#define NEAREND_PERF_FECS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 6, 0, 1, data, TxMessage)
#define NEAREND_PERF_ES_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 7, 0, 1, data, TxMessage)
#define NEAREND_PERF_SES_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 8, 0, 1, data, TxMessage)
#define NEAREND_PERF_LOSS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 9, 0, 1, data, TxMessage)
#define NEAREND_PERF_UAS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 10, 0, 1, data, TxMessage)
#define NEAREND_PERF_HECERR_FLAG_BC0_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 11, 0, 2, data, TxMessage)
#define NEAREND_PERF_HECERR_FLAG_BC1_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 11, 2, 2, data, TxMessage)
#define FAREND_PERF_LOS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 1, 0, 1, data, TxMessage)
#define FAREND_PERF_CRC_FLAG_LP0_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 24, 0, 1, data, TxMessage)
#define FAREND_PERF_CRC_FLAG_LP1_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 24, 1, 1, data, TxMessage)
#define FAREND_PERF_RSCORR_FLAG_LP0_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 28, 0, 1, data, TxMessage)
#define FAREND_PERF_RSCORR_FLAG_LP1_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 28, 1, 1, data, TxMessage)
#define FAREND_PERF_FECS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 32, 0, 1, data, TxMessage)
#define FAREND_PERF_ES_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 33, 0, 1, data, TxMessage)
#define FAREND_PERF_SES_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 34, 0, 1, data, TxMessage)
#define FAREND_PERF_LOSS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 35, 0, 1, data, TxMessage)
#define FAREND_PERF_UAS_FLAG_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 36, 0, 1, data, TxMessage)
#define FAREND_PERF_HECERR_FLAG_BC0_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 37, 0, 2, data, TxMessage)
#define FAREND_PERF_HECERR_FLAG_BC1_MAKECMV	makeCMV(H2D_CMV_READ, PLAM, 37, 2, 2, data, TxMessage)
#define NEAREND_HLINSC_MAKECMV(mode)		makeCMV(mode, INFO, 71, 2, 1, data, TxMessage)
#define NEAREND_HLINPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 73, idx, size, data, TxMessage)
#define NEAREND_HLOGMT_MAKECMV(mode)		makeCMV(mode, INFO, 80, 0, 1, data, TxMessage)
#define NEAREND_HLOGPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 75, idx, size, data, TxMessage)
#define NEAREND_QLNMT_MAKECMV(mode)		makeCMV(mode, INFO, 80, 1, 1, data, TxMessage)
#define NEAREND_QLNPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 77, idx, size, data, TxMessage)
#define NEAREND_SNRMT_MAKECMV(mode)		makeCMV(mode, INFO, 80, 2, 1, data, TxMessage)
#define NEAREND_SNRPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 78, idx, size, data, TxMessage)
#define NEAREND_BITPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 22, idx, size, data, TxMessage)
#define NEAREND_GAINPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 24, idx, size, data, TxMessage)
#define  FAREND_HLINSC_MAKECMV(mode)		makeCMV(mode, INFO, 70, 2, 1, data, TxMessage)
#define  FAREND_HLINPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 72, idx, size, data, TxMessage)
#define  FAREND_HLOGMT_MAKECMV(mode)		makeCMV(mode, INFO, 79, 0, 1, data, TxMessage)
#define  FAREND_HLOGPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 74, idx, size, data, TxMessage)
#define  FAREND_QLNMT_MAKECMV(mode)		makeCMV(mode, INFO, 79, 1, 1, data, TxMessage)
#define  FAREND_QLNPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 76, idx, size, data, TxMessage)
#define  FAREND_SNRMT_MAKECMV(mode)		makeCMV(mode, INFO, 79, 2, 1, data, TxMessage)
#define  FAREND_SNRPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 11, idx, size, data, TxMessage)
#define  FAREND_SNRPS_DIAG_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 10, idx, size, data, TxMessage)
#define  FAREND_BITPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 23, idx, size, data, TxMessage)
#define  FAREND_GAINPS_MAKECMV(mode,idx,size)	makeCMV(mode, INFO, 25, idx, size, data, TxMessage)
#define NOMPSD_US_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 102, 0, 1, data, TxMessage)
#define NOMPSD_DS_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 102, 1, 1, data, TxMessage)
#define PCB_US_MAKECMV		makeCMV(H2D_CMV_READ, INFO, 102, 6, 1, data, TxMessage)
#define PCB_DS_MAKECMV		makeCMV(H2D_CMV_READ, INFO, 102, 7, 1, data, TxMessage)
#define	RMSGI_US_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 102, 10, 1, data, TxMessage)
#define	RMSGI_DS_MAKECMV	makeCMV(H2D_CMV_READ, INFO, 102, 11, 1, data, TxMessage)

#endif /* __DANUBE_MEI_MIB_CMV_H */

