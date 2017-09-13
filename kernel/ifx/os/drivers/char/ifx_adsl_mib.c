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
/*
Changed log:
0.99.02
1.Fixed getting wrong data of GAINpsds/ GAINpsus/ BITSpsds/ BITSpsus parameters.
2.Fixed getting wrong data of SNRpsxx and ATCATPxx paraeters.
3.Enabled some MIB APIs in loop diagnostics mode.

2.00.00 02/10/2006
   First version for the separation of DSL and MEI
*/

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif
#define IFX_ADSL_MIB_RFC3440
//#define IFX_ADSL_PORT_RTEMS
#if defined(IFX_ADSL_PORT_RTEMS)
#include "danube_mei_rtems.h"
#else

#include <ifx/ifx_adsl_linux.h>
#include <ifx/ifx_adsl_mib_cmv.h>
#endif

static char IFX_ADSL_MIB_VERSION[]="2.00.00";

#define IFX_ADSL_DEBUG
#ifdef IFX_ADSL_DEBUG
#define IFX_ADSL_DMSG(fmt, args...) printk( KERN_DEBUG  "%s: " fmt,__FUNCTION__, ## args)
#else
#define IFX_ADSL_DMSG(fmt, args...) do { } while(0)
#endif
#define IFX_ADSL_EMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)


#define CONV_8_8_TO_SCALED_SIGNED_SHORT(val) (((short)(val)) * 10 / 256)
//	((val) & 0x80) ? (val) = -1 * (((val) ^ 0xFF) + 1): (val)

#define CONV_3_13_TO_SCALED_SIGNED_SHORT(val) (((short)(val)) * 10 / 8192)

#define ROUND_8_8_TO_SIGNED_SHORT(val)  (((val) >> 8) + (((val) & 0x80)?-1:1) *((((val) & 0xFF) + 0x7F) / 0xFF))

#define CONV_32_32_BYTE_SWAP(val) { u8 tmp; tmp=val[0];val[0]=val[1];val[1]=tmp;tmp=val[2];val[2]=val[3];val[3]=tmp;}

static ifx_adsl_mib * current_intvl;
static struct list_head interval_list;
static ifx_adsl_mib * mei_mib;


static mib_previous_read mib_pread={0,0,0,0,0,0,0,0,0,0,0,0};
static mib_flags_pretime mib_pflagtime;// initialized when module loaded

static u32 ATUC_PERF_LOFS=0;
static u32 ATUC_PERF_LOSS=0;
static u32 ATUC_PERF_ESS=0;
static u32 ATUC_PERF_INITS=0;
static u32 ATUR_PERF_LOFS=0;
static u32 ATUR_PERF_LOSS=0;
static u32 ATUR_PERF_LPR=0;
static u32 ATUR_PERF_ESS=0;
static u32 ATUR_CHAN_RECV_BLK=0;
static u32 ATUR_CHAN_TX_BLK=0;
static u32 ATUR_CHAN_CORR_BLK=0;
static u32 ATUR_CHAN_UNCORR_BLK=0;
//RFC-3440
static u32 ATUC_PERF_STAT_FASTR=0;
static u32 ATUC_PERF_STAT_FAILED_FASTR=0;
static u32 ATUC_PERF_STAT_SESL=0;
static u32 ATUC_PERF_STAT_UASL=0;
static u32 ATUR_PERF_STAT_SESL=0;
static u32 ATUR_PERF_STAT_UASL=0;

static adslChanPrevTxRate PrevTxRate={0,0};
static adslPhysCurrStatus CurrStatus={0,0};
static ChanType chantype={0,0};
static adslLineAlarmConfProfileEntry AlarmConfProfile={"No Name\0",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
static adslFarEndPerfStats FarendStatsData;
struct timeval FarendData_acquire_time={0};
static adslInitStats AdslInitStatsData;

#ifdef IFX_ADSL_MIB_RFC3440
static adslLineAlarmConfProfileExtEntry AlarmConfProfileExt={"No Name\0",0,0,0,0,0,0};
#endif

static wait_queue_head_t wait_queue_mibdaemon;

#if defined(__LINUX__)
static struct completion mei_mib_thread_exit;

static int phy_mei_net_init(struct net_device * dev);
static int interleave_mei_net_init(struct net_device * dev);
static int fast_mei_net_init(struct net_device * dev);
static struct net_device_stats * phy_mei_net_get_stats(struct net_device * dev);
static struct net_device_stats * interleave_mei_net_get_stats(struct net_device * dev);
static struct net_device_stats * fast_mei_net_get_stats(struct net_device * dev);

typedef struct mei_priv{
        struct net_device_stats stats;
}mei_priv;

static struct net_device phy_mei_net = { init: phy_mei_net_init, name: "MEI_PHY"};
static struct net_device interleave_mei_net = { init: interleave_mei_net_init, name: "MEI_INTL"};
static struct net_device fast_mei_net = { init: fast_mei_net_init, name: "MEI_FAST"};


/////////////////               net device                              ///////////////////////////////////////////////////
static int phy_mei_net_init(struct net_device * dev)
{
	dev->get_stats = phy_mei_net_get_stats;
	dev->ip_ptr = NULL;
	dev->type = 94;
	dev->flags=IFF_UP;
	dev->priv = kmalloc(sizeof(struct mei_priv), GFP_KERNEL);
	if(dev->priv == NULL)
		return -ENOMEM;
	memset(dev->priv, 0, sizeof(struct mei_priv));
	return 0;
}

static int interleave_mei_net_init(struct net_device * dev)
{
	dev->get_stats = interleave_mei_net_get_stats;
	dev->ip_ptr = NULL;
	dev->type = 124;
	dev->flags=IFF_UP;
	dev->priv = kmalloc(sizeof(struct mei_priv), GFP_KERNEL);
	if(dev->priv == NULL)
		return -ENOMEM;
	memset(dev->priv, 0, sizeof(struct mei_priv));
	return 0;
}

static int fast_mei_net_init(struct net_device * dev)
{
	dev->get_stats = fast_mei_net_get_stats;
	dev->ip_ptr = NULL;
	dev->type = 125;
	dev->flags=IFF_UP;
	dev->priv = kmalloc(sizeof(struct mei_priv), GFP_KERNEL);
	if(dev->priv == NULL)
		return -ENOMEM;
	memset(dev->priv, 0, sizeof(struct mei_priv));
	return 0;
}

static struct net_device_stats * phy_mei_net_get_stats(struct net_device * dev)
{
	struct mei_priv * priv;
	priv = (struct mei_priv *)dev->priv;
	(priv->stats).rx_packets = ATUR_CHAN_RECV_BLK;
	(priv->stats).tx_packets = ATUR_CHAN_TX_BLK;
	(priv->stats).rx_errors = ATUR_CHAN_CORR_BLK + ATUR_CHAN_UNCORR_BLK;
	(priv->stats).rx_dropped = ATUR_CHAN_UNCORR_BLK;

	return &(priv->stats);
}

static struct net_device_stats * interleave_mei_net_get_stats(struct net_device * dev)
{
	struct mei_priv * priv;
	priv = (struct mei_priv *)dev->priv;
	(priv->stats).rx_packets = ATUR_CHAN_RECV_BLK;
	(priv->stats).tx_packets = ATUR_CHAN_TX_BLK;
	(priv->stats).rx_errors = ATUR_CHAN_CORR_BLK + ATUR_CHAN_UNCORR_BLK;
	(priv->stats).rx_dropped = ATUR_CHAN_UNCORR_BLK;

	return &(priv->stats);
}

static struct net_device_stats * fast_mei_net_get_stats(struct net_device * dev)
{
	struct mei_priv * priv;
	priv = (struct mei_priv *)dev->priv;
	(priv->stats).rx_packets = ATUR_CHAN_RECV_BLK;
	(priv->stats).tx_packets = ATUR_CHAN_TX_BLK;
	(priv->stats).rx_errors = ATUR_CHAN_CORR_BLK + ATUR_CHAN_UNCORR_BLK;
	(priv->stats).rx_dropped = ATUR_CHAN_UNCORR_BLK;

	return &(priv->stats);
}
#endif //defined (__LINUX__)

int ifx_adsl_mib_shutdown = 0;
int mei_mib_ioctl(MEI_inode_t * ino, MEI_file_t * fil, unsigned int command, unsigned long lon)
{
	ifx_adsl_mib * mib_ptr;
	ifx_adsl_mib * temp_intvl;
	structpts pts;
	u16 data[12];  //used in makeCMV, to pass in payload when CMV set, ignored when CMV read.

	int i,k;
	u16 trapsflag=0;
	struct timeval time_now;
	struct list_head * ptr;
	u32 j=0;
	u32 temp=0,temp2;
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 hdlc_cmd[2];
	u16 hdlc_rx_buffer[32];
	int hdlc_rx_len=0;
	int meierr;

	switch(command){
	case GET_ADSL_LINE_CODE:
		pts.adslLineTableEntry_pt = (adslLineTableEntry *)kmalloc(sizeof(adslLineTableEntry), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineTableEntry_pt, (char *)lon, sizeof(adslLineTableEntry));
		if(IS_FLAG_SET((&(pts.adslLineTableEntry_pt->flags)), LINE_CODE_FLAG)){
			pts.adslLineTableEntry_pt->adslLineCode = 2;
		}
		copy_to_user((char *)lon, (char *)pts.adslLineTableEntry_pt, sizeof(adslLineTableEntry));
		kfree(pts.adslLineTableEntry_pt);
		break;
#ifdef IFX_ADSL_MIB_RFC3440
	case GET_ADSL_ATUC_LINE_EXT:
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;
		pts.adslLineExtTableEntry_pt = (adslLineExtTableEntry *)kmalloc(sizeof(adslLineExtTableEntry), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineExtTableEntry_pt, (char *)lon, sizeof(adslLineExtTableEntry));
		if(IS_FLAG_SET((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_CAP_FLAG)){
			ATUC_LINE_TRANS_CAP_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 67 Index 0");
				CLR_FLAG((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_CAP_FLAG);
			}
			else{
				memcpy((&(pts.adslLineExtTableEntry_pt->adslLineTransAtucCap)), RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		if(IS_FLAG_SET((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_CONFIG_FLAG)){
			ATUC_LINE_TRANS_CONFIG_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 67 Index 0");
				CLR_FLAG((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_CONFIG_FLAG);
			}
			else{
				memcpy((&(pts.adslLineExtTableEntry_pt->adslLineTransAtucConfig)), RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		if(IS_FLAG_SET((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_ACTUAL_FLAG)){
			ATUC_LINE_TRANS_ACTUAL_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 2 Address 1 Index 0");
				CLR_FLAG((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_ACTUAL_FLAG);
			}
			else{
				memcpy((&(pts.adslLineExtTableEntry_pt->adslLineTransAtucActual)), RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		if(IS_FLAG_SET((&(pts.adslLineExtTableEntry_pt->flags)), LINE_GLITE_POWER_STATE_FLAG)){    // not supported currently
			CLR_FLAG((&(pts.adslLineExtTableEntry_pt->flags)), LINE_GLITE_POWER_STATE_FLAG);
		}
		copy_to_user((char *)lon, (char *)pts.adslLineExtTableEntry_pt, sizeof(adslLineExtTableEntry));
		kfree(pts.adslLineTableEntry_pt);
		MEI_MUTEX_UNLOCK(mei_sema);
		break;
#endif

#ifdef IFX_ADSL_MIB_RFC3440
	case SET_ADSL_ATUC_LINE_EXT:
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;
		pts.adslLineExtTableEntry_pt = (adslLineExtTableEntry *)kmalloc(sizeof(adslLineExtTableEntry), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineExtTableEntry_pt, (char *)lon, sizeof(adslLineExtTableEntry));

		//only adslLineTransAtucConfig can be set.
		CLR_FLAG((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_CAP_FLAG);
		if(IS_FLAG_SET((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_CONFIG_FLAG)){
			memcpy(data,(&(pts.adslLineExtTableEntry_pt->adslLineTransAtucConfig)), 2);
			ATUC_LINE_TRANS_CONFIG_FLAG_MAKECMV_WR;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 67 Index 0");
				CLR_FLAG((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_CONFIG_FLAG);
			}
		}
		CLR_FLAG((&(pts.adslLineExtTableEntry_pt->flags)), ATUC_LINE_TRANS_ACTUAL_FLAG);
		CLR_FLAG((&(pts.adslLineExtTableEntry_pt->flags)), LINE_GLITE_POWER_STATE_FLAG);

		copy_to_user((char *)lon, (char *)pts.adslLineExtTableEntry_pt, sizeof(adslLineExtTableEntry));
		kfree(pts.adslLineTableEntry_pt);
		MEI_MUTEX_UNLOCK(mei_sema);
		break;
#endif

	case GET_ADSL_ATUC_PHY:
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;

		pts.adslAtucPhysEntry_pt = (adslAtucPhysEntry *)kmalloc(sizeof(adslAtucPhysEntry), GFP_KERNEL);
		copy_from_user((char *)pts.adslAtucPhysEntry_pt, (char *)lon, sizeof(adslAtucPhysEntry));
		if(IS_FLAG_SET((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_PHY_SER_NUM_FLAG)){
			ATUC_PHY_SER_NUM_FLAG_MAKECMV1;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 57 Index 0");
				CLR_FLAG((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_PHY_SER_NUM_FLAG);
			}
			else{
				memcpy(pts.adslAtucPhysEntry_pt->serial_no, RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
			ATUC_PHY_SER_NUM_FLAG_MAKECMV2;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 57 Index 12");
				CLR_FLAG((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_PHY_SER_NUM_FLAG);
			}
			else{
				memcpy((pts.adslAtucPhysEntry_pt->serial_no+24), RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		if(IS_FLAG_SET((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_PHY_VENDOR_ID_FLAG)){
			ATUC_PHY_VENDOR_ID_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 64 Index 0");
				CLR_FLAG((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_PHY_VENDOR_ID_FLAG);
			}
			else{
				memcpy(&pts.adslAtucPhysEntry_pt->vendor_id.vendor_id, RxMessage+4, ((RxMessage[0]&0xf)*2));
				CONV_32_32_BYTE_SWAP(pts.adslAtucPhysEntry_pt->vendor_id.vendor_info.provider_id);
			}
		}
		if(IS_FLAG_SET((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_PHY_VER_NUM_FLAG)){
			ATUC_PHY_VER_NUM_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 58 Index 0");
				CLR_FLAG((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_PHY_VER_NUM_FLAG);
			}
			else{
				memcpy(pts.adslAtucPhysEntry_pt->version_no, RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		if(IS_FLAG_SET((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_CURR_STAT_FLAG)){
			pts.adslAtucPhysEntry_pt->status = CurrStatus.adslAtucCurrStatus;
		}
		if(IS_FLAG_SET((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_CURR_OUT_PWR_FLAG)){
			ATUC_CURR_OUT_PWR_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 68 Index 6");
				CLR_FLAG((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_CURR_OUT_PWR_FLAG);
			}
			else{
				pts.adslAtucPhysEntry_pt->outputPwr = RxMessage[4];
			}
		}
		if(IS_FLAG_SET((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_CURR_ATTR_FLAG)){
			ATUC_CURR_ATTR_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 68 Index 4");
				CLR_FLAG((&(pts.adslAtucPhysEntry_pt->flags)), ATUC_CURR_ATTR_FLAG);
			}
			else{
				memcpy((&(pts.adslAtucPhysEntry_pt->attainableRate)), RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		copy_to_user((char *)lon, (char *)pts.adslAtucPhysEntry_pt, sizeof(adslAtucPhysEntry));
		kfree(pts.adslAtucPhysEntry_pt);

		MEI_MUTEX_UNLOCK(mei_sema);
		break;
	case GET_ADSL_ATUR_PHY:
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;

		pts.adslAturPhysEntry_pt = (adslAturPhysEntry *)kmalloc(sizeof(adslAturPhysEntry), GFP_KERNEL);
		copy_from_user((char *)pts.adslAturPhysEntry_pt, (char *)lon, sizeof(adslAturPhysEntry));
		if(IS_FLAG_SET((&(pts.adslAturPhysEntry_pt->flags)), ATUR_PHY_SER_NUM_FLAG)){
			ATUR_PHY_SER_NUM_FLAG_MAKECMV1;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 62 Index 0");
				CLR_FLAG((&(pts.adslAturPhysEntry_pt->flags)), ATUR_PHY_SER_NUM_FLAG);
			}
			else{
				memcpy(pts.adslAturPhysEntry_pt->serial_no, RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
			ATUR_PHY_SER_NUM_FLAG_MAKECMV2;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 62 Index 12");
				CLR_FLAG((&(pts.adslAturPhysEntry_pt->flags)), ATUR_PHY_SER_NUM_FLAG);
			}
			else{
				memcpy((pts.adslAturPhysEntry_pt->serial_no+24), RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		if(IS_FLAG_SET((&(pts.adslAturPhysEntry_pt->flags)), ATUR_PHY_VENDOR_ID_FLAG)){
			ATUR_PHY_VENDOR_ID_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 65 Index 0");
				CLR_FLAG((&(pts.adslAturPhysEntry_pt->flags)), ATUR_PHY_VENDOR_ID_FLAG);
			}
			else{
				memcpy(&pts.adslAturPhysEntry_pt->vendor_id.vendor_id, RxMessage+4, ((RxMessage[0]&0xf)*2));
				CONV_32_32_BYTE_SWAP(pts.adslAturPhysEntry_pt->vendor_id.vendor_info.provider_id);
			}
		}
		if(IS_FLAG_SET((&(pts.adslAturPhysEntry_pt->flags)), ATUR_PHY_VER_NUM_FLAG)){
			ATUR_PHY_VER_NUM_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 61 Index 0");
				CLR_FLAG((&(pts.adslAturPhysEntry_pt->flags)), ATUR_PHY_VER_NUM_FLAG);
			}
			else{
				memcpy(pts.adslAturPhysEntry_pt->version_no, RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		if(IS_FLAG_SET((&(pts.adslAturPhysEntry_pt->flags)), ATUR_SNRMGN_FLAG)){
			ATUR_SNRMGN_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 68 Index 4");
				CLR_FLAG((&(pts.adslAturPhysEntry_pt->flags)), ATUR_SNRMGN_FLAG);
			}
			else{
				memcpy((&(pts.adslAturPhysEntry_pt->SnrMgn)), RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		if(IS_FLAG_SET((&(pts.adslAturPhysEntry_pt->flags)), ATUR_ATTN_FLAG)){
			ATUR_ATTN_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 68 Index 2");
				CLR_FLAG((&(pts.adslAturPhysEntry_pt->flags)), ATUR_ATTN_FLAG);
			}
			else{
				memcpy((&(pts.adslAturPhysEntry_pt->Attn)), RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		if(IS_FLAG_SET((&(pts.adslAturPhysEntry_pt->flags)), ATUR_CURR_STAT_FLAG)){
			pts.adslAturPhysEntry_pt->status = CurrStatus.adslAturCurrStatus;
		}
		if(IS_FLAG_SET((&(pts.adslAturPhysEntry_pt->flags)), ATUR_CURR_OUT_PWR_FLAG)){
			ATUR_CURR_OUT_PWR_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 69 Index 6");
				CLR_FLAG((&(pts.adslAturPhysEntry_pt->flags)), ATUR_CURR_OUT_PWR_FLAG);
			}
			else{
				//memcpy((&(pts.adslAturPhysEntry_pt->outputPwr)), RxMessage+4, ((RxMessage[0]&0xf)*2));
				pts.adslAturPhysEntry_pt->outputPwr = RxMessage[4];
			}
		}
		if(IS_FLAG_SET((&(pts.adslAturPhysEntry_pt->flags)), ATUR_CURR_ATTR_FLAG)){
			ATUR_CURR_ATTR_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 3 Address 69 Index 4");
				CLR_FLAG((&(pts.adslAturPhysEntry_pt->flags)), ATUR_CURR_ATTR_FLAG);
			}
			else{
				memcpy((&(pts.adslAturPhysEntry_pt->attainableRate)), RxMessage+4, ((RxMessage[0]&0xf)*2));
			}
		}
		copy_to_user((char *)lon, (char *)pts.adslAturPhysEntry_pt, sizeof(adslAturPhysEntry));
		kfree(pts.adslAturPhysEntry_pt);

		MEI_MUTEX_UNLOCK(mei_sema);
		break;
	case GET_ADSL_ATUC_CHAN_INFO:
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;

		pts.adslAtucChanInfo_pt = (adslAtucChanInfo *)kmalloc(sizeof(adslAtucChanInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslAtucChanInfo_pt, (char *)lon, sizeof(adslAtucChanInfo));
		if(IS_FLAG_SET((&(pts.adslAtucChanInfo_pt->flags)), ATUC_CHAN_INTLV_DELAY_FLAG)){
			if((chantype.interleave!=1) || (chantype.fast==1)){
				CLR_FLAG((&(pts.adslAtucChanInfo_pt->flags)), ATUC_CHAN_INTLV_DELAY_FLAG);
			}
			else{
				ATUC_CHAN_INTLV_DELAY_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 3 Index 1");
					CLR_FLAG((&(pts.adslAtucChanInfo_pt->flags)), ATUC_CHAN_INTLV_DELAY_FLAG);
				}
				else{
					memcpy((&(pts.adslAtucChanInfo_pt->interleaveDelay)), RxMessage+4, ((RxMessage[0]&0xf)*2));
				}
			}
		}
		if(IS_FLAG_SET((&(pts.adslAtucChanInfo_pt->flags)), ATUC_CHAN_CURR_TX_RATE_FLAG)){
			ATUC_CHAN_CURR_TX_RATE_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 1 Index 0");
				CLR_FLAG((&(pts.adslAtucChanInfo_pt->flags)), ATUC_CHAN_CURR_TX_RATE_FLAG);
			}
			else{
				pts.adslAtucChanInfo_pt->currTxRate = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
			}
		}
		if(IS_FLAG_SET((&(pts.adslAtucChanInfo_pt->flags)), ATUC_CHAN_PREV_TX_RATE_FLAG)){
			pts.adslAtucChanInfo_pt->prevTxRate = PrevTxRate.adslAtucChanPrevTxRate;
		}
		copy_to_user((char *)lon, (char *)pts.adslAtucChanInfo_pt, sizeof(adslAtucChanInfo));
		kfree(pts.adslAtucChanInfo_pt);

		MEI_MUTEX_UNLOCK(mei_sema);
		break;
	case GET_ADSL_ATUR_CHAN_INFO:
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;

		pts.adslAturChanInfo_pt = (adslAturChanInfo *)kmalloc(sizeof(adslAturChanInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslAturChanInfo_pt, (char *)lon, sizeof(adslAturChanInfo));
		if(IS_FLAG_SET((&(pts.adslAturChanInfo_pt->flags)), ATUR_CHAN_INTLV_DELAY_FLAG)){
			if((chantype.interleave!=1) || (chantype.fast==1)){
				CLR_FLAG((&(pts.adslAturChanInfo_pt->flags)), ATUR_CHAN_INTLV_DELAY_FLAG);
			}
			else{
				ATUR_CHAN_INTLV_DELAY_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 2 Index 1");
					CLR_FLAG((&(pts.adslAturChanInfo_pt->flags)), ATUR_CHAN_INTLV_DELAY_FLAG);
				}
				else{
					memcpy((&(pts.adslAturChanInfo_pt->interleaveDelay)), RxMessage+4, ((RxMessage[0]&0xf)*2));
				}
			}
		}
		if(IS_FLAG_SET((&(pts.adslAturChanInfo_pt->flags)), ATUR_CHAN_CURR_TX_RATE_FLAG)){
			ATUR_CHAN_CURR_TX_RATE_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 0 Index 0");
				CLR_FLAG((&(pts.adslAturChanInfo_pt->flags)), ATUR_CHAN_CURR_TX_RATE_FLAG);
			}
			else{
				pts.adslAturChanInfo_pt->currTxRate = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
			}
		}
		if(IS_FLAG_SET((&(pts.adslAturChanInfo_pt->flags)), ATUR_CHAN_PREV_TX_RATE_FLAG)){
			pts.adslAturChanInfo_pt->prevTxRate = PrevTxRate.adslAturChanPrevTxRate;
		}
		if(IS_FLAG_SET((&(pts.adslAturChanInfo_pt->flags)), ATUR_CHAN_CRC_BLK_LEN_FLAG)){
			// ? no CMV to update this
			CLR_FLAG((&(pts.adslAturChanInfo_pt->flags)), ATUR_CHAN_CRC_BLK_LEN_FLAG);
		}
		copy_to_user((char *)lon, (char *)pts.adslAturChanInfo_pt, sizeof(adslAturChanInfo));
		kfree(pts.adslAturChanInfo_pt);

		MEI_MUTEX_UNLOCK(mei_sema);
		break;
	case GET_ADSL_ATUC_PERF_DATA:
		pts.atucPerfDataEntry_pt = (atucPerfDataEntry *)kmalloc(sizeof(atucPerfDataEntry), GFP_KERNEL);
		copy_from_user((char *)pts.atucPerfDataEntry_pt, (char *)lon, sizeof(atucPerfDataEntry));
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_LOFS_FLAG)){
			pts.atucPerfDataEntry_pt->adslAtucPerfLofs=ATUC_PERF_LOFS;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_LOSS_FLAG)){
			pts.atucPerfDataEntry_pt->adslAtucPerfLoss=ATUC_PERF_LOSS;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_ESS_FLAG)){
			pts.atucPerfDataEntry_pt->adslAtucPerfESs=ATUC_PERF_ESS;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_INITS_FLAG)){
			pts.atucPerfDataEntry_pt->adslAtucPerfInits=ATUC_PERF_INITS;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_VALID_INTVLS_FLAG)){
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i++;
				if(i==96)
					break;
			}
			pts.atucPerfDataEntry_pt->adslAtucPerfValidIntervals=i;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_INVALID_INTVLS_FLAG)){
			pts.atucPerfDataEntry_pt->adslAtucPerfInvalidIntervals=0;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_15MIN_TIME_ELAPSED_FLAG)){
			do_gettimeofday(&time_now);
			pts.atucPerfDataEntry_pt->adslAtucPerfCurr15MinTimeElapsed=time_now.tv_sec - (current_intvl->start_time).tv_sec;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_15MIN_LOFS_FLAG)){
			pts.atucPerfDataEntry_pt->adslAtucPerfCurr15MinLofs=current_intvl->AtucPerfLof;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_15MIN_LOSS_FLAG)){
			pts.atucPerfDataEntry_pt->adslAtucPerfCurr15MinLoss=current_intvl->AtucPerfLos;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_15MIN_ESS_FLAG)){
			pts.atucPerfDataEntry_pt->adslAtucPerfCurr15MinESs=current_intvl->AtucPerfEs;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_15MIN_INIT_FLAG)){
			pts.atucPerfDataEntry_pt->adslAtucPerfCurr15MinInits=current_intvl->AtucPerfInit;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_1DAY_TIME_ELAPSED_FLAG)){
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i+=900;
			}
			do_gettimeofday(&time_now);
			i+=time_now.tv_sec - (current_intvl->start_time).tv_sec;
			if(i>=86400)
				pts.atucPerfDataEntry_pt->adslAtucPerfCurr1DayTimeElapsed=i-86400;
			else
				pts.atucPerfDataEntry_pt->adslAtucPerfCurr1DayTimeElapsed=i;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_1DAY_LOFS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfLof;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AtucPerfLof;
			pts.atucPerfDataEntry_pt->adslAtucPerfCurr1DayLofs=j;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_1DAY_LOSS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfLos;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AtucPerfLos;
			pts.atucPerfDataEntry_pt->adslAtucPerfCurr1DayLoss=j;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_1DAY_ESS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfEs;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AtucPerfEs;
			pts.atucPerfDataEntry_pt->adslAtucPerfCurr1DayESs=j;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_CURR_1DAY_INIT_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfInit;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AtucPerfInit;
			pts.atucPerfDataEntry_pt->adslAtucPerfCurr1DayInits=j;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_PREV_1DAY_MON_SEC_FLAG)){
			i=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				i++;
			}
			if(i>=96)
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayMoniSecs=86400;
			else
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayMoniSecs=0;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_PREV_1DAY_LOFS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfLof;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayLofs=j;
			else
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayLofs=0;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_PREV_1DAY_LOSS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfLos;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayLoss=j;
			else
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayLoss=0;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_PREV_1DAY_ESS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfEs;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayESs=j;
			else
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayESs=0;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataEntry_pt->flags)), ATUC_PERF_PREV_1DAY_INITS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfInit;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayInits=j;
			else
				pts.atucPerfDataEntry_pt->adslAtucPerfPrev1DayInits=0;
		}

		copy_to_user((char *)lon, (char *)pts.atucPerfDataEntry_pt, sizeof(atucPerfDataEntry));
		kfree(pts.atucPerfDataEntry_pt);
		break;
#ifdef IFX_ADSL_MIB_RFC3440
	case GET_ADSL_ATUC_PERF_DATA_EXT:	//??? CMV mapping not available
		pts.atucPerfDataExtEntry_pt = (atucPerfDataExtEntry *)kmalloc(sizeof(atucPerfDataExtEntry), GFP_KERNEL);
		copy_from_user((char *)pts.atucPerfDataExtEntry_pt, (char *)lon, sizeof(atucPerfDataExtEntry));
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_STAT_FASTR_FLAG)){
			pts.atucPerfDataExtEntry_pt->adslAtucPerfStatFastR=ATUC_PERF_STAT_FASTR;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_STAT_FAILED_FASTR_FLAG)){
			pts.atucPerfDataExtEntry_pt->adslAtucPerfStatFailedFastR=ATUC_PERF_STAT_FAILED_FASTR;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_STAT_SESL_FLAG)){
			pts.atucPerfDataExtEntry_pt->adslAtucPerfStatSesL=ATUC_PERF_STAT_SESL;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_STAT_UASL_FLAG)){
			pts.atucPerfDataExtEntry_pt->adslAtucPerfStatUasL=ATUC_PERF_STAT_UASL;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_CURR_15MIN_FASTR_FLAG)){
			pts.atucPerfDataExtEntry_pt->adslAtucPerfCurr15MinFastR=current_intvl->AtucPerfStatFastR;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_CURR_15MIN_FAILED_FASTR_FLAG)){
			pts.atucPerfDataExtEntry_pt->adslAtucPerfCurr15MinFailedFastR=current_intvl->AtucPerfStatFailedFastR;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_CURR_15MIN_SESL_FLAG)){
			pts.atucPerfDataExtEntry_pt->adslAtucPerfCurr15MinSesL=current_intvl->AtucPerfStatSesL;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_CURR_15MIN_UASL_FLAG)){
			pts.atucPerfDataExtEntry_pt->adslAtucPerfCurr15MinUasL=current_intvl->AtucPerfStatUasL;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_CURR_1DAY_FASTR_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfStatFastR;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AtucPerfStatFastR;
			pts.atucPerfDataExtEntry_pt->adslAtucPerfCurr1DayFastR=j;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_CURR_1DAY_FAILED_FASTR_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfStatFailedFastR;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AtucPerfStatFailedFastR;
			pts.atucPerfDataExtEntry_pt->adslAtucPerfCurr1DayFailedFastR=j;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_CURR_1DAY_SESL_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfStatSesL;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AtucPerfStatSesL;
			pts.atucPerfDataExtEntry_pt->adslAtucPerfCurr1DaySesL=j;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_CURR_1DAY_UASL_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfStatUasL;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AtucPerfStatUasL;
			pts.atucPerfDataExtEntry_pt->adslAtucPerfCurr1DayUasL=j;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_PREV_1DAY_FASTR_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfStatFastR;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.atucPerfDataExtEntry_pt->adslAtucPerfPrev1DayFastR=j;
			else
				pts.atucPerfDataExtEntry_pt->adslAtucPerfPrev1DayFastR=0;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_PREV_1DAY_FAILED_FASTR_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfStatFailedFastR;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.atucPerfDataExtEntry_pt->adslAtucPerfPrev1DayFailedFastR=j;
			else
				pts.atucPerfDataExtEntry_pt->adslAtucPerfPrev1DayFailedFastR=0;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_PREV_1DAY_SESL_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfStatSesL;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.atucPerfDataExtEntry_pt->adslAtucPerfPrev1DaySesL=j;
			else
				pts.atucPerfDataExtEntry_pt->adslAtucPerfPrev1DaySesL=0;
		}
		if(IS_FLAG_SET((&(pts.atucPerfDataExtEntry_pt->flags)), ATUC_PERF_PREV_1DAY_UASL_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AtucPerfStatUasL;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.atucPerfDataExtEntry_pt->adslAtucPerfPrev1DayUasL=j;
			else
				pts.atucPerfDataExtEntry_pt->adslAtucPerfPrev1DayUasL=0;
		}
		copy_to_user((char *)lon, (char *)pts.atucPerfDataExtEntry_pt, sizeof(atucPerfDataExtEntry));
		kfree(pts.atucPerfDataExtEntry_pt);
		break;
#endif
	case GET_ADSL_ATUR_PERF_DATA:
		pts.aturPerfDataEntry_pt = (aturPerfDataEntry *)kmalloc(sizeof(aturPerfDataEntry), GFP_KERNEL);
		copy_from_user((char *)pts.aturPerfDataEntry_pt, (char *)lon, sizeof(aturPerfDataEntry));
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_LOFS_FLAG)){
			pts.aturPerfDataEntry_pt->adslAturPerfLofs=ATUR_PERF_LOFS;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_LOSS_FLAG)){
			pts.aturPerfDataEntry_pt->adslAturPerfLoss=ATUR_PERF_LOSS;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_LPR_FLAG)){
			pts.aturPerfDataEntry_pt->adslAturPerfLprs=ATUR_PERF_LPR;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_ESS_FLAG)){
			pts.aturPerfDataEntry_pt->adslAturPerfESs=ATUR_PERF_ESS;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_VALID_INTVLS_FLAG)){
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i++;
				if(i==96)
					break;
			}
			pts.aturPerfDataEntry_pt->adslAturPerfValidIntervals=i;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_INVALID_INTVLS_FLAG)){
			pts.aturPerfDataEntry_pt->adslAturPerfInvalidIntervals=0;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_15MIN_TIME_ELAPSED_FLAG)){
			do_gettimeofday(&time_now);
			pts.aturPerfDataEntry_pt->adslAturPerfCurr15MinTimeElapsed=time_now.tv_sec - (current_intvl->start_time).tv_sec;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_15MIN_LOFS_FLAG)){
			pts.aturPerfDataEntry_pt->adslAturPerfCurr15MinLofs=current_intvl->AturPerfLof;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_15MIN_LOSS_FLAG)){
			pts.aturPerfDataEntry_pt->adslAturPerfCurr15MinLoss=current_intvl->AturPerfLos;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_15MIN_LPR_FLAG)){
			pts.aturPerfDataEntry_pt->adslAturPerfCurr15MinLprs=current_intvl->AturPerfLpr;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_15MIN_ESS_FLAG)){
			pts.aturPerfDataEntry_pt->adslAturPerfCurr15MinESs=current_intvl->AturPerfEs;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_1DAY_TIME_ELAPSED_FLAG)){
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i+=900;
			}
			do_gettimeofday(&time_now);
			i+=time_now.tv_sec - (current_intvl->start_time).tv_sec;
			if(i>=86400)
				pts.aturPerfDataEntry_pt->adslAturPerfCurr1DayTimeElapsed=i-86400;
			else
				pts.aturPerfDataEntry_pt->adslAturPerfCurr1DayTimeElapsed=i;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_1DAY_LOFS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfLof;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturPerfLof;
			pts.aturPerfDataEntry_pt->adslAturPerfCurr1DayLofs=j;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_1DAY_LOSS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfLos;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturPerfLos;
			pts.aturPerfDataEntry_pt->adslAturPerfCurr1DayLoss=j;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_1DAY_LPR_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfLpr;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturPerfLpr;
			pts.aturPerfDataEntry_pt->adslAturPerfCurr1DayLprs=j;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_CURR_1DAY_ESS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfEs;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturPerfEs;
			pts.aturPerfDataEntry_pt->adslAturPerfCurr1DayESs=j;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_PREV_1DAY_MON_SEC_FLAG)){
			i=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				i++;
			}
			if(i>=96)
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayMoniSecs=86400;
			else
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayMoniSecs=0;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_PREV_1DAY_LOFS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfLof;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayLofs=j;
			else
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayLofs=0;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_PREV_1DAY_LOSS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfLos;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayLoss=j;
			else
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayLoss=0;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_PREV_1DAY_LPR_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfLpr;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayLprs=j;
			else
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayLprs=0;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataEntry_pt->flags)), ATUR_PERF_PREV_1DAY_ESS_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfEs;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayESs=j;
			else
				pts.aturPerfDataEntry_pt->adslAturPerfPrev1DayESs=0;
		}

		copy_to_user((char *)lon, (char *)pts.aturPerfDataEntry_pt, sizeof(aturPerfDataEntry));
		kfree(pts.aturPerfDataEntry_pt);
		break;
#ifdef IFX_ADSL_MIB_RFC3440
	case GET_ADSL_ATUR_PERF_DATA_EXT:
		pts.aturPerfDataExtEntry_pt = (aturPerfDataExtEntry *)kmalloc(sizeof(aturPerfDataExtEntry), GFP_KERNEL);
		copy_from_user((char *)pts.aturPerfDataExtEntry_pt, (char *)lon, sizeof(aturPerfDataExtEntry));
		if(IS_FLAG_SET((&(pts.aturPerfDataExtEntry_pt->flags)), ATUR_PERF_STAT_SESL_FLAG)){
			pts.aturPerfDataExtEntry_pt->adslAturPerfStatSesL=ATUR_PERF_STAT_SESL;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataExtEntry_pt->flags)), ATUR_PERF_STAT_UASL_FLAG)){
			pts.aturPerfDataExtEntry_pt->adslAturPerfStatUasL=ATUR_PERF_STAT_UASL;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataExtEntry_pt->flags)), ATUR_PERF_CURR_15MIN_SESL_FLAG)){
			pts.aturPerfDataExtEntry_pt->adslAturPerfCurr15MinSesL=current_intvl->AturPerfStatSesL;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataExtEntry_pt->flags)), ATUR_PERF_CURR_15MIN_UASL_FLAG)){
			pts.aturPerfDataExtEntry_pt->adslAturPerfCurr15MinUasL=current_intvl->AturPerfStatUasL;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataExtEntry_pt->flags)), ATUR_PERF_CURR_1DAY_SESL_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfStatSesL;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturPerfStatSesL;
			pts.aturPerfDataExtEntry_pt->adslAturPerfCurr1DaySesL=j;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataExtEntry_pt->flags)), ATUR_PERF_CURR_1DAY_UASL_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfStatUasL;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturPerfStatUasL;
			pts.aturPerfDataExtEntry_pt->adslAturPerfCurr1DayUasL=j;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataExtEntry_pt->flags)), ATUR_PERF_PREV_1DAY_SESL_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfStatSesL;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturPerfDataExtEntry_pt->adslAturPerfPrev1DaySesL=j;
			else
				pts.aturPerfDataExtEntry_pt->adslAturPerfPrev1DaySesL=0;
		}
		if(IS_FLAG_SET((&(pts.aturPerfDataExtEntry_pt->flags)), ATUR_PERF_PREV_1DAY_UASL_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturPerfStatUasL;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturPerfDataExtEntry_pt->adslAturPerfPrev1DayUasL=j;
			else
				pts.aturPerfDataExtEntry_pt->adslAturPerfPrev1DayUasL=0;
		}
		copy_to_user((char *)lon, (char *)pts.aturPerfDataExtEntry_pt, sizeof(aturPerfDataExtEntry));
		kfree(pts.aturPerfDataExtEntry_pt);
		break;
#endif
	case GET_ADSL_ATUC_INTVL_INFO:
		pts.adslAtucIntvlInfo_pt = (adslAtucIntvlInfo *)kmalloc(sizeof(adslAtucIntvlInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslAtucIntvlInfo_pt, (char *)lon, sizeof(adslAtucIntvlInfo));

		if(pts.adslAtucIntvlInfo_pt->IntervalNumber <1){
			pts.adslAtucIntvlInfo_pt->intervalLOF = ATUC_PERF_LOFS;
			pts.adslAtucIntvlInfo_pt->intervalLOS = ATUC_PERF_LOSS;
			pts.adslAtucIntvlInfo_pt->intervalES = ATUC_PERF_ESS;
			pts.adslAtucIntvlInfo_pt->intervalInits = ATUC_PERF_INITS;
			pts.adslAtucIntvlInfo_pt->intervalValidData = 1;
		}
		else{
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i++;
				if(i==pts.adslAtucIntvlInfo_pt->IntervalNumber){
					temp_intvl = list_entry(ptr, ifx_adsl_mib, list);
					pts.adslAtucIntvlInfo_pt->intervalLOF = temp_intvl->AtucPerfLof;
					pts.adslAtucIntvlInfo_pt->intervalLOS = temp_intvl->AtucPerfLos;
					pts.adslAtucIntvlInfo_pt->intervalES = temp_intvl->AtucPerfEs;
					pts.adslAtucIntvlInfo_pt->intervalInits = temp_intvl->AtucPerfInit;
					pts.adslAtucIntvlInfo_pt->intervalValidData = 1;
					break;
				}
			}
			if(ptr==&interval_list){
				pts.adslAtucIntvlInfo_pt->intervalValidData = 0;
				pts.adslAtucIntvlInfo_pt->flags = 0;
				pts.adslAtucIntvlInfo_pt->intervalLOF = 0;
				pts.adslAtucIntvlInfo_pt->intervalLOS = 0;
				pts.adslAtucIntvlInfo_pt->intervalES = 0;
				pts.adslAtucIntvlInfo_pt->intervalInits = 0;
			}
		}

		copy_to_user((char *)lon, (char *)pts.adslAtucIntvlInfo_pt, sizeof(adslAtucIntvlInfo));
		kfree(pts.adslAtucIntvlInfo_pt);
		break;
#ifdef IFX_ADSL_MIB_RFC3440
	case GET_ADSL_ATUC_INTVL_EXT_INFO:
		pts.adslAtucInvtlExtInfo_pt = (adslAtucInvtlExtInfo *)kmalloc(sizeof(adslAtucInvtlExtInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslAtucInvtlExtInfo_pt, (char *)lon, sizeof(adslAtucInvtlExtInfo));
		if(pts.adslAtucInvtlExtInfo_pt->IntervalNumber <1){
			pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalFastR = ATUC_PERF_STAT_FASTR;
			pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalFailedFastR = ATUC_PERF_STAT_FAILED_FASTR;
			pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalSesL = ATUC_PERF_STAT_SESL;
			pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalUasL = ATUC_PERF_STAT_UASL;
		}
		else{
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i++;
				if(i==pts.adslAtucInvtlExtInfo_pt->IntervalNumber){
					temp_intvl = list_entry(ptr, ifx_adsl_mib, list);
					pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalFastR = temp_intvl->AtucPerfStatFastR;
					pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalFailedFastR = temp_intvl->AtucPerfStatFailedFastR;
					pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalSesL = temp_intvl->AtucPerfStatSesL;
					pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalUasL = temp_intvl->AtucPerfStatUasL;
					break;
				}
			}
			if(ptr==&interval_list){
				pts.adslAtucInvtlExtInfo_pt->flags = 0;
				pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalFastR = 0;
				pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalFailedFastR = 0;
				pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalSesL = 0;
				pts.adslAtucInvtlExtInfo_pt->adslAtucIntervalUasL = 0;
			}
		}
		copy_to_user((char *)lon, (char *)pts.adslAtucInvtlExtInfo_pt, sizeof(adslAtucInvtlExtInfo));
		kfree(pts.adslAtucInvtlExtInfo_pt);
		break;
#endif
	case GET_ADSL_ATUR_INTVL_INFO:
		pts.adslAturIntvlInfo_pt = (adslAturIntvlInfo *)kmalloc(sizeof(adslAturIntvlInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslAturIntvlInfo_pt, (char *)lon, sizeof(adslAturIntvlInfo));

		if(pts.adslAturIntvlInfo_pt->IntervalNumber <1){
			pts.adslAturIntvlInfo_pt->intervalLOF = ATUR_PERF_LOFS;
			pts.adslAturIntvlInfo_pt->intervalLOS = ATUR_PERF_LOSS;
			pts.adslAturIntvlInfo_pt->intervalES = ATUR_PERF_ESS;
			pts.adslAturIntvlInfo_pt->intervalLPR = ATUR_PERF_LPR;
			pts.adslAturIntvlInfo_pt->intervalValidData = 1;
		}
		else{
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i++;
				if(i==pts.adslAturIntvlInfo_pt->IntervalNumber){
					temp_intvl = list_entry(ptr, ifx_adsl_mib, list);
					pts.adslAturIntvlInfo_pt->intervalLOF = temp_intvl->AturPerfLof;
					pts.adslAturIntvlInfo_pt->intervalLOS = temp_intvl->AturPerfLos;
					pts.adslAturIntvlInfo_pt->intervalES = temp_intvl->AturPerfEs;
					pts.adslAturIntvlInfo_pt->intervalLPR = temp_intvl->AturPerfLpr;
					pts.adslAturIntvlInfo_pt->intervalValidData = 1;
					break;
				}
			}
			if(ptr==&interval_list){
				pts.adslAturIntvlInfo_pt->intervalValidData = 0;
				pts.adslAturIntvlInfo_pt->flags = 0;
				pts.adslAturIntvlInfo_pt->intervalLOF = 0;
				pts.adslAturIntvlInfo_pt->intervalLOS = 0;
				pts.adslAturIntvlInfo_pt->intervalES = 0;
				pts.adslAturIntvlInfo_pt->intervalLPR = 0;
			}
		}

		copy_to_user((char *)lon, (char *)pts.adslAturIntvlInfo_pt, sizeof(adslAturIntvlInfo));
		kfree(pts.adslAturIntvlInfo_pt);
		break;
#ifdef IFX_ADSL_MIB_RFC3440
	case GET_ADSL_ATUR_INTVL_EXT_INFO:
		pts.adslAturInvtlExtInfo_pt = (adslAturInvtlExtInfo *)kmalloc(sizeof(adslAturInvtlExtInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslAturInvtlExtInfo_pt, (char *)lon, sizeof(adslAturInvtlExtInfo));

		if(pts.adslAturInvtlExtInfo_pt->IntervalNumber <1){
			pts.adslAturInvtlExtInfo_pt->adslAturIntervalSesL = ATUR_PERF_STAT_SESL;
			pts.adslAturInvtlExtInfo_pt->adslAturIntervalUasL = ATUR_PERF_STAT_UASL;
		}
		else{
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i++;
				if(i==pts.adslAturInvtlExtInfo_pt->IntervalNumber){
					temp_intvl = list_entry(ptr, ifx_adsl_mib, list);
					pts.adslAturInvtlExtInfo_pt->adslAturIntervalSesL = temp_intvl->AturPerfStatSesL;
					pts.adslAturInvtlExtInfo_pt->adslAturIntervalUasL = temp_intvl->AturPerfStatUasL;
					break;
				}
			}
			if(ptr==&interval_list){
				pts.adslAturInvtlExtInfo_pt->flags = 0;
				pts.adslAturInvtlExtInfo_pt->adslAturIntervalSesL = 0;
				pts.adslAturInvtlExtInfo_pt->adslAturIntervalUasL = 0;
			}
		}

		copy_to_user((char *)lon, (char *)pts.adslAturInvtlExtInfo_pt, sizeof(adslAturInvtlExtInfo));
		kfree(pts.adslAturInvtlExtInfo_pt);
		break;
#endif
	case GET_ADSL_ATUC_CHAN_PERF_DATA:
		pts.atucChannelPerfDataEntry_pt = (atucChannelPerfDataEntry *)kmalloc(sizeof(atucChannelPerfDataEntry), GFP_KERNEL);
		copy_from_user((char *)pts.atucChannelPerfDataEntry_pt, (char *)lon, sizeof(atucChannelPerfDataEntry));

		pts.atucChannelPerfDataEntry_pt->flags = 0;

		copy_to_user((char *)lon, (char *)pts.atucChannelPerfDataEntry_pt, sizeof(atucChannelPerfDataEntry));
		kfree(pts.atucChannelPerfDataEntry_pt);
		break;
	case GET_ADSL_ATUR_CHAN_PERF_DATA:
		pts.aturChannelPerfDataEntry_pt = (aturChannelPerfDataEntry *)kmalloc(sizeof(aturChannelPerfDataEntry), GFP_KERNEL);
		copy_from_user((char *)pts.aturChannelPerfDataEntry_pt, (char *)lon, sizeof(aturChannelPerfDataEntry));
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_RECV_BLK_FLAG)){
			pts.aturChannelPerfDataEntry_pt->adslAturChanReceivedBlks=ATUR_CHAN_RECV_BLK;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_TX_BLK_FLAG)){
			pts.aturChannelPerfDataEntry_pt->adslAturChanTransmittedBlks=ATUR_CHAN_TX_BLK;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_CORR_BLK_FLAG)){
			pts.aturChannelPerfDataEntry_pt->adslAturChanCorrectedBlks=ATUR_CHAN_CORR_BLK;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_UNCORR_BLK_FLAG)){
			pts.aturChannelPerfDataEntry_pt->adslAturChanUncorrectBlks=ATUR_CHAN_UNCORR_BLK;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_VALID_INTVL_FLAG)){
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i++;
				if(i==96)
					break;
			}
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfValidIntervals=i;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_INVALID_INTVL_FLAG)){
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfInvalidIntervals=0;
		}
 		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_15MIN_TIME_ELAPSED_FLAG)){
			do_gettimeofday(&time_now);
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr15MinTimeElapsed=time_now.tv_sec - (current_intvl->start_time).tv_sec;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_15MIN_RECV_BLK_FLAG)){
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr15MinReceivedBlks=current_intvl->AturChanPerfRxBlk;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_15MIN_TX_BLK_FLAG)){
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr15MinTransmittedBlks=current_intvl->AturChanPerfTxBlk;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_15MIN_CORR_BLK_FLAG)){
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr15MinCorrectedBlks=current_intvl->AturChanPerfCorrBlk;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_15MIN_UNCORR_BLK_FLAG)){
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr15MinUncorrectBlks=current_intvl->AturChanPerfUncorrBlk;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_1DAY_TIME_ELAPSED_FLAG)){
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i+=900;
			}
			do_gettimeofday(&time_now);
			i+=time_now.tv_sec - (current_intvl->start_time).tv_sec;
			if(i>=86400)
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr1DayTimeElapsed=i-86400;
			else
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr1DayTimeElapsed=i;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_1DAY_RECV_BLK_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturChanPerfRxBlk;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturChanPerfRxBlk;
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr1DayReceivedBlks=j;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_1DAY_TX_BLK_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturChanPerfTxBlk;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturChanPerfTxBlk;
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr1DayTransmittedBlks=j;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_1DAY_CORR_BLK_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturChanPerfCorrBlk;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturChanPerfCorrBlk;
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr1DayCorrectedBlks=j;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_CURR_1DAY_UNCORR_BLK_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturChanPerfUncorrBlk;
				i++;
				if(i==96)
					j=0;
			}
			j+=current_intvl->AturChanPerfUncorrBlk;
			pts.aturChannelPerfDataEntry_pt->adslAturChanPerfCurr1DayUncorrectBlks=j;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_PREV_1DAY_MONI_SEC_FLAG)){
			i=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				i++;
			}
			if(i>=96)
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayMoniSecs=86400;
			else
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayMoniSecs=0;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_PREV_1DAY_RECV_BLK_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturChanPerfRxBlk;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayReceivedBlks=j;
			else
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayReceivedBlks=0;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_PREV_1DAY_TRANS_BLK_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturChanPerfTxBlk;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayTransmittedBlks=j;
			else
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayTransmittedBlks=0;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_PREV_1DAY_CORR_BLK_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturChanPerfCorrBlk;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayCorrectedBlks=j;
			else
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayCorrectedBlks=0;
		}
		if(IS_FLAG_SET((&(pts.aturChannelPerfDataEntry_pt->flags)), ATUR_CHAN_PERF_PREV_1DAY_UNCORR_BLK_FLAG)){
			i=0;
			j=0;
			for(ptr=interval_list.next; ptr!=&(current_intvl->list); ptr=ptr->next){
				mib_ptr = list_entry(ptr, ifx_adsl_mib, list);
				j+=mib_ptr->AturChanPerfUncorrBlk;
				i++;
				if(i==96)
					break;
			}
			if(i==96)
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayUncorrectBlks=j;
			else
				pts.aturChannelPerfDataEntry_pt->adslAturChanPerfPrev1DayUncorrectBlks=0;
		}

		copy_to_user((char *)lon, (char *)pts.aturChannelPerfDataEntry_pt, sizeof(aturChannelPerfDataEntry));
		kfree(pts.aturChannelPerfDataEntry_pt);
		break;
	case GET_ADSL_ATUC_CHAN_INTVL_INFO:
		pts.adslAtucChanIntvlInfo_pt = (adslAtucChanIntvlInfo *)kmalloc(sizeof(adslAtucChanIntvlInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslAtucChanIntvlInfo_pt, (char *)lon, sizeof(adslAtucChanIntvlInfo));

			pts.adslAtucChanIntvlInfo_pt->flags = 0;

		copy_to_user((char *)lon, (char *)pts.adslAtucChanIntvlInfo_pt, sizeof(adslAtucChanIntvlInfo));
		kfree(pts.adslAtucChanIntvlInfo_pt);
		break;
	case GET_ADSL_ATUR_CHAN_INTVL_INFO:
		pts.adslAturChanIntvlInfo_pt = (adslAturChanIntvlInfo *)kmalloc(sizeof(adslAturChanIntvlInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslAturChanIntvlInfo_pt, (char *)lon, sizeof(adslAturChanIntvlInfo));

		if(pts.adslAturChanIntvlInfo_pt->IntervalNumber <1){
			pts.adslAturChanIntvlInfo_pt->chanIntervalRecvdBlks = ATUR_CHAN_RECV_BLK;
			pts.adslAturChanIntvlInfo_pt->chanIntervalXmitBlks = ATUR_CHAN_TX_BLK;
			pts.adslAturChanIntvlInfo_pt->chanIntervalCorrectedBlks = ATUR_CHAN_CORR_BLK;
			pts.adslAturChanIntvlInfo_pt->chanIntervalUncorrectBlks = ATUR_CHAN_UNCORR_BLK;
			pts.adslAturChanIntvlInfo_pt->intervalValidData = 1;
		}
		else{
			i=0;
			for(ptr=(current_intvl->list).prev; ptr!=&interval_list; ptr=ptr->prev){
				i++;
				if(i==pts.adslAturChanIntvlInfo_pt->IntervalNumber){
					temp_intvl = list_entry(ptr, ifx_adsl_mib, list);
					pts.adslAturChanIntvlInfo_pt->chanIntervalRecvdBlks = temp_intvl->AturChanPerfRxBlk;
					pts.adslAturChanIntvlInfo_pt->chanIntervalXmitBlks = temp_intvl->AturChanPerfTxBlk;
					pts.adslAturChanIntvlInfo_pt->chanIntervalCorrectedBlks = temp_intvl->AturChanPerfCorrBlk;
					pts.adslAturChanIntvlInfo_pt->chanIntervalUncorrectBlks = temp_intvl->AturChanPerfUncorrBlk;
					pts.adslAturChanIntvlInfo_pt->intervalValidData = 1;
					break;
				}
			}
			if(ptr==&interval_list){
				pts.adslAturChanIntvlInfo_pt->intervalValidData = 0;
				pts.adslAturChanIntvlInfo_pt->flags = 0;
			}
		}

		copy_to_user((char *)lon, (char *)pts.adslAturChanIntvlInfo_pt, sizeof(adslAturChanIntvlInfo));
		kfree(pts.adslAturChanIntvlInfo_pt);
		break;
	case GET_ADSL_ALRM_CONF_PROF:
		pts.adslLineAlarmConfProfileEntry_pt = (adslLineAlarmConfProfileEntry *)kmalloc(sizeof(adslLineAlarmConfProfileEntry), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineAlarmConfProfileEntry_pt, (char *)lon, sizeof(adslLineAlarmConfProfileEntry));

		strncpy(pts.adslLineAlarmConfProfileEntry_pt->adslLineAlarmConfProfileName, AlarmConfProfile.adslLineAlarmConfProfileName, 32);
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_15MIN_LOFS_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAtucThresh15MinLofs=AlarmConfProfile.adslAtucThresh15MinLofs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_15MIN_LOSS_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAtucThresh15MinLoss=AlarmConfProfile.adslAtucThresh15MinLoss;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_15MIN_ESS_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAtucThresh15MinESs=AlarmConfProfile.adslAtucThresh15MinESs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_FAST_RATEUP_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAtucThreshFastRateUp=AlarmConfProfile.adslAtucThreshFastRateUp;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_INTERLEAVE_RATEUP_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAtucThreshInterleaveRateUp=AlarmConfProfile.adslAtucThreshInterleaveRateUp;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_FAST_RATEDOWN_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAtucThreshFastRateDown=AlarmConfProfile.adslAtucThreshFastRateDown;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_INTERLEAVE_RATEDOWN_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAtucThreshInterleaveRateDown=AlarmConfProfile.adslAtucThreshInterleaveRateDown;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_INIT_FAILURE_TRAP_ENABLE_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAtucInitFailureTrapEnable=AlarmConfProfile.adslAtucInitFailureTrapEnable;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_15MIN_LOFS_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAturThresh15MinLofs=AlarmConfProfile.adslAturThresh15MinLofs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_15MIN_LOSS_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAturThresh15MinLoss=AlarmConfProfile.adslAturThresh15MinLoss;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_15MIN_LPRS_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAturThresh15MinLprs=AlarmConfProfile.adslAturThresh15MinLprs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_15MIN_ESS_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAturThresh15MinESs=AlarmConfProfile.adslAturThresh15MinESs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_FAST_RATEUP_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAturThreshFastRateUp=AlarmConfProfile.adslAturThreshFastRateUp;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_INTERLEAVE_RATEUP_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAturThreshInterleaveRateUp=AlarmConfProfile.adslAturThreshInterleaveRateUp;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_FAST_RATEDOWN_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAturThreshFastRateDown=AlarmConfProfile.adslAturThreshFastRateDown;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_INTERLEAVE_RATEDOWN_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslAturThreshInterleaveRateDown=AlarmConfProfile.adslAturThreshInterleaveRateDown;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), LINE_ALARM_CONF_PROFILE_ROWSTATUS_FLAG)){
			pts.adslLineAlarmConfProfileEntry_pt->adslLineAlarmConfProfileRowStatus=AlarmConfProfile.adslLineAlarmConfProfileRowStatus;
		}
		copy_to_user((char *)lon, (char *)pts.adslLineAlarmConfProfileEntry_pt, sizeof(adslLineAlarmConfProfileEntry));
		kfree(pts.adslLineAlarmConfProfileEntry_pt);
		break;
#ifdef IFX_ADSL_MIB_RFC3440
	case GET_ADSL_ALRM_CONF_PROF_EXT:
		pts.adslLineAlarmConfProfileExtEntry_pt = (adslLineAlarmConfProfileExtEntry *)kmalloc(sizeof(adslLineAlarmConfProfileExtEntry), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineAlarmConfProfileExtEntry_pt, (char *)lon, sizeof(adslLineAlarmConfProfileExtEntry));
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUC_THRESH_15MIN_FAILED_FASTR_FLAG)){
			pts.adslLineAlarmConfProfileExtEntry_pt->adslAtucThreshold15MinFailedFastR=AlarmConfProfileExt.adslAtucThreshold15MinFailedFastR;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUC_THRESH_15MIN_SESL_FLAG)){
			pts.adslLineAlarmConfProfileExtEntry_pt->adslAtucThreshold15MinSesL=AlarmConfProfileExt.adslAtucThreshold15MinSesL;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUC_THRESH_15MIN_UASL_FLAG)){
			pts.adslLineAlarmConfProfileExtEntry_pt->adslAtucThreshold15MinUasL=AlarmConfProfileExt.adslAtucThreshold15MinUasL;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUR_THRESH_15MIN_SESL_FLAG)){
			pts.adslLineAlarmConfProfileExtEntry_pt->adslAturThreshold15MinSesL=AlarmConfProfileExt.adslAturThreshold15MinSesL;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUR_THRESH_15MIN_UASL_FLAG)){
			pts.adslLineAlarmConfProfileExtEntry_pt->adslAturThreshold15MinUasL=AlarmConfProfileExt.adslAturThreshold15MinUasL;
		}
		copy_to_user((char *)lon, (char *)pts.adslLineAlarmConfProfileExtEntry_pt, sizeof(adslLineAlarmConfProfileExtEntry));
		kfree(pts.adslLineAlarmConfProfileExtEntry_pt);
		break;
#endif
	case SET_ADSL_ALRM_CONF_PROF:
		pts.adslLineAlarmConfProfileEntry_pt = (adslLineAlarmConfProfileEntry *)kmalloc(sizeof(adslLineAlarmConfProfileEntry), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineAlarmConfProfileEntry_pt, (char *)lon, sizeof(adslLineAlarmConfProfileEntry));

		strncpy(AlarmConfProfile.adslLineAlarmConfProfileName, pts.adslLineAlarmConfProfileEntry_pt->adslLineAlarmConfProfileName, 32);
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_15MIN_LOFS_FLAG)){
			AlarmConfProfile.adslAtucThresh15MinLofs=pts.adslLineAlarmConfProfileEntry_pt->adslAtucThresh15MinLofs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_15MIN_LOSS_FLAG)){
			AlarmConfProfile.adslAtucThresh15MinLoss=pts.adslLineAlarmConfProfileEntry_pt->adslAtucThresh15MinLoss;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_15MIN_ESS_FLAG)){
			AlarmConfProfile.adslAtucThresh15MinESs=pts.adslLineAlarmConfProfileEntry_pt->adslAtucThresh15MinESs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_FAST_RATEUP_FLAG)){
			AlarmConfProfile.adslAtucThreshFastRateUp=pts.adslLineAlarmConfProfileEntry_pt->adslAtucThreshFastRateUp;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_INTERLEAVE_RATEUP_FLAG)){
			AlarmConfProfile.adslAtucThreshInterleaveRateUp=pts.adslLineAlarmConfProfileEntry_pt->adslAtucThreshInterleaveRateUp;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_FAST_RATEDOWN_FLAG)){
			AlarmConfProfile.adslAtucThreshFastRateDown=pts.adslLineAlarmConfProfileEntry_pt->adslAtucThreshFastRateDown;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_THRESH_INTERLEAVE_RATEDOWN_FLAG)){
			AlarmConfProfile.adslAtucThreshInterleaveRateDown=pts.adslLineAlarmConfProfileEntry_pt->adslAtucThreshInterleaveRateDown;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUC_INIT_FAILURE_TRAP_ENABLE_FLAG)){
			AlarmConfProfile.adslAtucInitFailureTrapEnable=pts.adslLineAlarmConfProfileEntry_pt->adslAtucInitFailureTrapEnable;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_15MIN_LOFS_FLAG)){
			AlarmConfProfile.adslAturThresh15MinLofs=pts.adslLineAlarmConfProfileEntry_pt->adslAturThresh15MinLofs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_15MIN_LOSS_FLAG)){
			AlarmConfProfile.adslAturThresh15MinLoss=pts.adslLineAlarmConfProfileEntry_pt->adslAturThresh15MinLoss;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_15MIN_LPRS_FLAG)){
			AlarmConfProfile.adslAturThresh15MinLprs=pts.adslLineAlarmConfProfileEntry_pt->adslAturThresh15MinLprs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_15MIN_ESS_FLAG)){
			AlarmConfProfile.adslAturThresh15MinESs=pts.adslLineAlarmConfProfileEntry_pt->adslAturThresh15MinESs;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_FAST_RATEUP_FLAG)){
			AlarmConfProfile.adslAturThreshFastRateUp=pts.adslLineAlarmConfProfileEntry_pt->adslAturThreshFastRateUp;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_INTERLEAVE_RATEUP_FLAG)){
			AlarmConfProfile.adslAturThreshInterleaveRateUp=pts.adslLineAlarmConfProfileEntry_pt->adslAturThreshInterleaveRateUp;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_FAST_RATEDOWN_FLAG)){
			AlarmConfProfile.adslAturThreshFastRateDown=pts.adslLineAlarmConfProfileEntry_pt->adslAturThreshFastRateDown;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), ATUR_THRESH_INTERLEAVE_RATEDOWN_FLAG)){
			AlarmConfProfile.adslAturThreshInterleaveRateDown=pts.adslLineAlarmConfProfileEntry_pt->adslAturThreshInterleaveRateDown;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileEntry_pt->flags)), LINE_ALARM_CONF_PROFILE_ROWSTATUS_FLAG)){
			AlarmConfProfile.adslLineAlarmConfProfileRowStatus=pts.adslLineAlarmConfProfileEntry_pt->adslLineAlarmConfProfileRowStatus;
		}
		copy_to_user((char *)lon, (char *)pts.adslLineAlarmConfProfileEntry_pt, sizeof(adslLineAlarmConfProfileEntry));
		kfree(pts.adslLineAlarmConfProfileEntry_pt);
		break;

#ifdef IFX_ADSL_MIB_RFC3440
	case SET_ADSL_ALRM_CONF_PROF_EXT:
		pts.adslLineAlarmConfProfileExtEntry_pt = (adslLineAlarmConfProfileExtEntry *)kmalloc(sizeof(adslLineAlarmConfProfileExtEntry), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineAlarmConfProfileExtEntry_pt, (char *)lon, sizeof(adslLineAlarmConfProfileExtEntry));
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUC_THRESH_15MIN_FAILED_FASTR_FLAG)){
			AlarmConfProfileExt.adslAtucThreshold15MinFailedFastR=pts.adslLineAlarmConfProfileExtEntry_pt->adslAtucThreshold15MinFailedFastR;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUC_THRESH_15MIN_SESL_FLAG)){
			AlarmConfProfileExt.adslAtucThreshold15MinSesL=pts.adslLineAlarmConfProfileExtEntry_pt->adslAtucThreshold15MinSesL;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUC_THRESH_15MIN_UASL_FLAG)){
			AlarmConfProfileExt.adslAtucThreshold15MinUasL=pts.adslLineAlarmConfProfileExtEntry_pt->adslAtucThreshold15MinUasL;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUR_THRESH_15MIN_SESL_FLAG)){
			AlarmConfProfileExt.adslAturThreshold15MinSesL=pts.adslLineAlarmConfProfileExtEntry_pt->adslAturThreshold15MinSesL;
		}
		if(IS_FLAG_SET((&(pts.adslLineAlarmConfProfileExtEntry_pt->flags)), ATUR_THRESH_15MIN_UASL_FLAG)){
			AlarmConfProfileExt.adslAturThreshold15MinUasL=pts.adslLineAlarmConfProfileExtEntry_pt->adslAturThreshold15MinUasL;
		}
		copy_to_user((char *)lon, (char *)pts.adslLineAlarmConfProfileExtEntry_pt, sizeof(adslLineAlarmConfProfileExtEntry));
		kfree(pts.adslLineAlarmConfProfileExtEntry_pt);
		break;
#endif

	case ADSL_ATUR_TRAPS:
		if(MEI_MUTEX_LOCK(mei_sema))
               		return -ERESTARTSYS;

		trapsflag=0;
		if(AlarmConfProfile.adslAtucThresh15MinLofs!=0 && current_intvl->AtucPerfLof>=AlarmConfProfile.adslAtucThresh15MinLofs)
			trapsflag|=ATUC_PERF_LOFS_THRESH_FLAG;
		if(AlarmConfProfile.adslAtucThresh15MinLoss!=0 && current_intvl->AtucPerfLos>=AlarmConfProfile.adslAtucThresh15MinLoss)
			trapsflag|=ATUC_PERF_LOSS_THRESH_FLAG;
		if(AlarmConfProfile.adslAtucThresh15MinESs!=0 && current_intvl->AtucPerfEs>=AlarmConfProfile.adslAtucThresh15MinESs)
			trapsflag|=ATUC_PERF_ESS_THRESH_FLAG;
		if(chantype.fast==1){
			if(AlarmConfProfile.adslAtucThreshFastRateUp!=0 || AlarmConfProfile.adslAtucThreshFastRateDown!=0){
				ATUC_CHAN_CURR_TX_RATE_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 1 Index 0");
				}
				else{
					temp = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
					if((AlarmConfProfile.adslAtucThreshFastRateUp!=0) && (temp>=PrevTxRate.adslAtucChanPrevTxRate+AlarmConfProfile.adslAtucThreshFastRateUp)){
						trapsflag|=ATUC_RATE_CHANGE_FLAG;
						PrevTxRate.adslAtucChanPrevTxRate = temp;
					}
					if((AlarmConfProfile.adslAtucThreshFastRateDown!=0) && (temp<=PrevTxRate.adslAtucChanPrevTxRate-AlarmConfProfile.adslAtucThreshFastRateDown)){
						trapsflag|=ATUC_RATE_CHANGE_FLAG;
						PrevTxRate.adslAtucChanPrevTxRate = temp;
					}
				}
			}
		}
		if(chantype.interleave==1){
			if(AlarmConfProfile.adslAtucThreshInterleaveRateUp!=0 || AlarmConfProfile.adslAtucThreshInterleaveRateDown!=0){
				ATUC_CHAN_CURR_TX_RATE_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 1 Index 0");
				}
				else{
					temp = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
					if((AlarmConfProfile.adslAtucThreshInterleaveRateUp!=0) && (temp>=PrevTxRate.adslAtucChanPrevTxRate+AlarmConfProfile.adslAtucThreshInterleaveRateUp)){
						trapsflag|=ATUC_RATE_CHANGE_FLAG;
						PrevTxRate.adslAtucChanPrevTxRate = temp;
					}
					if((AlarmConfProfile.adslAtucThreshInterleaveRateDown!=0) && (temp<=PrevTxRate.adslAtucChanPrevTxRate-AlarmConfProfile.adslAtucThreshInterleaveRateDown)){
						trapsflag|=ATUC_RATE_CHANGE_FLAG;
						PrevTxRate.adslAtucChanPrevTxRate = temp;
					}
				}
			}
		}
		if(AlarmConfProfile.adslAturThresh15MinLofs!=0 && current_intvl->AturPerfLof>=AlarmConfProfile.adslAturThresh15MinLofs)
			trapsflag|=ATUR_PERF_LOFS_THRESH_FLAG;
		if(AlarmConfProfile.adslAturThresh15MinLoss!=0 && current_intvl->AturPerfLos>=AlarmConfProfile.adslAturThresh15MinLoss)
			trapsflag|=ATUR_PERF_LOSS_THRESH_FLAG;
		if(AlarmConfProfile.adslAturThresh15MinLprs!=0 && current_intvl->AturPerfLpr>=AlarmConfProfile.adslAturThresh15MinLprs)
			trapsflag|=ATUR_PERF_LPRS_THRESH_FLAG;
		if(AlarmConfProfile.adslAturThresh15MinESs!=0 && current_intvl->AturPerfEs>=AlarmConfProfile.adslAturThresh15MinESs)
			trapsflag|=ATUR_PERF_ESS_THRESH_FLAG;
		if(chantype.fast==1){
			if(AlarmConfProfile.adslAturThreshFastRateUp!=0 || AlarmConfProfile.adslAturThreshFastRateDown!=0){
				ATUR_CHAN_CURR_TX_RATE_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 0 Index 0");
				}
				else{
					temp = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
					if((AlarmConfProfile.adslAturThreshFastRateUp!=0) && (temp>=PrevTxRate.adslAturChanPrevTxRate+AlarmConfProfile.adslAturThreshFastRateUp)){
						trapsflag|=ATUR_RATE_CHANGE_FLAG;
						PrevTxRate.adslAturChanPrevTxRate = temp;
					}
					if((AlarmConfProfile.adslAturThreshFastRateDown!=0) && (temp<=PrevTxRate.adslAturChanPrevTxRate-AlarmConfProfile.adslAturThreshFastRateDown)){
						trapsflag|=ATUR_RATE_CHANGE_FLAG;
						PrevTxRate.adslAturChanPrevTxRate = temp;
					}
				}
			}
		}
		if(chantype.interleave==1){
			if(AlarmConfProfile.adslAturThreshInterleaveRateUp!=0 || AlarmConfProfile.adslAturThreshInterleaveRateDown!=0){
				ATUR_CHAN_CURR_TX_RATE_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 0 Index 0");
				}
				else{
					temp = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
					if((AlarmConfProfile.adslAturThreshInterleaveRateUp!=0) && (temp>=PrevTxRate.adslAturChanPrevTxRate+AlarmConfProfile.adslAturThreshInterleaveRateUp)){
						trapsflag|=ATUR_RATE_CHANGE_FLAG;
						PrevTxRate.adslAturChanPrevTxRate = temp;
					}
					if((AlarmConfProfile.adslAturThreshInterleaveRateDown!=0) && (temp<=PrevTxRate.adslAturChanPrevTxRate-AlarmConfProfile.adslAturThreshInterleaveRateDown)){
						trapsflag|=ATUR_RATE_CHANGE_FLAG;
						PrevTxRate.adslAturChanPrevTxRate = temp;
					}
				}
			}
		}
		copy_to_user((char *)lon, (char *)(&trapsflag), 2);

		MEI_MUTEX_UNLOCK(mei_sema);
		break;

#ifdef IFX_ADSL_MIB_RFC3440
	case ADSL_ATUR_EXT_TRAPS:
		trapsflag=0;
		if(AlarmConfProfileExt.adslAtucThreshold15MinFailedFastR!=0 && current_intvl->AtucPerfStatFailedFastR>=AlarmConfProfileExt.adslAtucThreshold15MinFailedFastR)
			trapsflag|=ATUC_15MIN_FAILED_FASTR_TRAP_FLAG;
		if(AlarmConfProfileExt.adslAtucThreshold15MinSesL!=0 && current_intvl->AtucPerfStatSesL>=AlarmConfProfileExt.adslAtucThreshold15MinSesL)
			trapsflag|=ATUC_15MIN_SESL_TRAP_FLAG;
		if(AlarmConfProfileExt.adslAtucThreshold15MinUasL!=0 && current_intvl->AtucPerfStatUasL>=AlarmConfProfileExt.adslAtucThreshold15MinUasL)
			trapsflag|=ATUC_15MIN_UASL_TRAP_FLAG;
		if(AlarmConfProfileExt.adslAturThreshold15MinSesL!=0 && current_intvl->AturPerfStatSesL>=AlarmConfProfileExt.adslAturThreshold15MinSesL)
			trapsflag|=ATUR_15MIN_SESL_TRAP_FLAG;
		if(AlarmConfProfileExt.adslAturThreshold15MinUasL!=0 && current_intvl->AturPerfStatUasL>=AlarmConfProfileExt.adslAturThreshold15MinUasL)
			trapsflag|=ATUR_15MIN_UASL_TRAP_FLAG;
		copy_to_user((char *)lon, (char *)(&trapsflag), 2);
		break;
#endif

	case IFX_ADSL_MIB_LO_ATUC:
		do_gettimeofday(&time_now);
		if(lon&0x1){
			if((time_now.tv_sec-(mib_pflagtime.ATUC_PERF_LOSS_PTIME).tv_sec)>2){
				current_intvl->AtucPerfLos++;
				ATUC_PERF_LOSS++;
				CurrStatus.adslAtucCurrStatus = 2;
			}
			(mib_pflagtime.ATUC_PERF_LOSS_PTIME).tv_sec = time_now.tv_sec;
		}
		if(lon&0x2){
			if((time_now.tv_sec-(mib_pflagtime.ATUC_PERF_LOFS_PTIME).tv_sec)>2){
				current_intvl->AtucPerfLof++;
				ATUC_PERF_LOFS++;
				CurrStatus.adslAtucCurrStatus = 1;
			}
			(mib_pflagtime.ATUC_PERF_LOFS_PTIME).tv_sec = time_now.tv_sec;
		}
		if(!(lon&0x3))
			CurrStatus.adslAtucCurrStatus = 0;
		break;
	case IFX_ADSL_MIB_LO_ATUR:
		do_gettimeofday(&time_now);
		if(lon&0x1){
			if((time_now.tv_sec-(mib_pflagtime.ATUR_PERF_LOSS_PTIME).tv_sec)>2){
				current_intvl->AturPerfLos++;
				ATUR_PERF_LOSS++;
				CurrStatus.adslAturCurrStatus = 2;
			}
			(mib_pflagtime.ATUR_PERF_LOSS_PTIME).tv_sec = time_now.tv_sec;
		}
		if(lon&0x2){
			if((time_now.tv_sec-(mib_pflagtime.ATUR_PERF_LOFS_PTIME).tv_sec)>2){
				current_intvl->AturPerfLof++;
				ATUR_PERF_LOFS++;
				CurrStatus.adslAturCurrStatus = 1;
			}
			(mib_pflagtime.ATUR_PERF_LOFS_PTIME).tv_sec = time_now.tv_sec;
		}
		if(lon&0x4){
			if((time_now.tv_sec-(mib_pflagtime.ATUR_PERF_LPR_PTIME).tv_sec)>2){
				current_intvl->AturPerfLpr++;
				ATUR_PERF_LPR++;
				CurrStatus.adslAturCurrStatus = 3;
			}
			(mib_pflagtime.ATUR_PERF_LPR_PTIME).tv_sec = time_now.tv_sec;
		}
		if(!(lon&0x7))
			CurrStatus.adslAturCurrStatus = 0;
		break;
	case GET_ADSL_LINE_STATUS:
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;

		pts.adslLineStatusInfo_pt = (adslLineStatusInfo *)kmalloc(sizeof(adslLineStatusInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineStatusInfo_pt, (char *)lon, sizeof(adslLineStatusInfo));

		if(IS_FLAG_SET((&(pts.adslLineStatusInfo_pt->flags)), LINE_STAT_MODEM_STATUS_FLAG)){
			LINE_STAT_MODEM_STATUS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group STAT Address 0 Index 0");
#endif
				pts.adslLineStatusInfo_pt->adslModemStatus = 0;
			}
			else{
				pts.adslLineStatusInfo_pt->adslModemStatus = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineStatusInfo_pt->flags)), LINE_STAT_MODE_SEL_FLAG)){
			LINE_STAT_MODE_SEL_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group STAT Address 1 Index 0");
#endif
				pts.adslLineStatusInfo_pt->adslModeSelected = 0;
			}
			else{
				pts.adslLineStatusInfo_pt->adslModeSelected = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineStatusInfo_pt->flags)), LINE_STAT_TRELLCOD_ENABLE_FLAG)){
			LINE_STAT_TRELLCOD_ENABLE_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group OPTN Address 2 Index 0");
#endif
				pts.adslLineStatusInfo_pt->adslTrellisCodeEnable = 0;
			}
			else{

				pts.adslLineStatusInfo_pt->adslTrellisCodeEnable = ((RxMessage[4]>>13)&0x1)==0x1?0:1;
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineStatusInfo_pt->flags)), LINE_STAT_LATENCY_FLAG)){
			LINE_STAT_LATENCY_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group STAT Address 12 Index 0");
#endif
				pts.adslLineStatusInfo_pt->adslLatency = 0;
			}
			else{
				pts.adslLineStatusInfo_pt->adslLatency = RxMessage[4];
			}
		}

		copy_to_user((char *)lon, (char *)pts.adslLineStatusInfo_pt, sizeof(adslLineStatusInfo));
		kfree(pts.adslLineStatusInfo_pt);

		MEI_MUTEX_UNLOCK(mei_sema);
		break;


	case GET_ADSL_LINE_RATE:
		if (showtime!=1 && loop_diagnostics_completed!=1)
			return -ERESTARTSYS;
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;

		pts.adslLineRateInfo_pt = (adslLineRateInfo *)kmalloc(sizeof(adslLineRateInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineRateInfo_pt, (char *)lon, sizeof(adslLineRateInfo));

		if(IS_FLAG_SET((&(pts.adslLineRateInfo_pt->flags)), LINE_RATE_DATA_RATEDS_FLAG)){
			if (adsl_mode <=8 && adsl_mode_extend==0) // adsl mode
			{
				if (chantype.interleave)
					LINE_RATE_DATA_RATEDS_FLAG_ADSL1_LP0_MAKECMV;
				else
					LINE_RATE_DATA_RATEDS_FLAG_ADSL1_LP1_MAKECMV;

				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group RATE Address 1 Index 0");
#endif
					pts.adslLineRateInfo_pt->adslDataRateds = 0;
				}
				else{
					pts.adslLineRateInfo_pt->adslDataRateds = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
				}
			}else  // adsl 2/2+
			{
				unsigned long Mp,Lp,Tp,Rp,Kp,Bpn,DataRate,DataRate_remain;
				Mp=Lp=Tp=Rp=Kp=Bpn=DataRate=DataRate_remain=0;
				//// up stream data rate

				if (chantype.interleave)
				{
					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_LP_LP0_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 25 Index 0");
#endif
						Lp = 0;
					}else
						Lp=RxMessage[4];

					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_RP_LP0_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 23 Index 0");
#endif
						Rp = 0;
					}else
						Rp=RxMessage[4];

					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_MP_LP0_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 24 Index 0");
#endif
						Mp = 0;
					}else
						Mp=RxMessage[4];

					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_TP_LP0_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 26 Index 0");
#endif
						Tp = 0;
					}else
						Tp=RxMessage[4];

					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_KP_LP0_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 28 Index 0");
#endif
						Kp = 0;
					}else
					{
						Kp=RxMessage[4]+ RxMessage[5]+1;
						Bpn=RxMessage[4]+ RxMessage[5];
					}
				}else
				{
					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_LP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 25 Index 1");
#endif
						Lp = 0;
					}else
						Lp=RxMessage[4];

					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_RP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 23 Index 1");
#endif
						Rp = 0;
					}else
						Rp=RxMessage[4];

					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_MP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 24 Index 1");
#endif
						Mp = 0;
					}else
						Mp=RxMessage[4];

					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_TP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 26 Index 1");
#endif
						Tp = 0;
					}else
						Tp=RxMessage[4];

					LINE_RATE_DATA_RATEUS_FLAG_ADSL2_KP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 28 Index 2");
#endif
						Kp = 0;
					}else
					{
						Kp=RxMessage[4]+ RxMessage[5]+1;
						Bpn=RxMessage[4]+ RxMessage[5];
					}
				}
				    if (Tp==0 || Kp== 0 || Mp == 0 || Rp == 0)
			            {
			              	DataRate = 0;
			              	meierr = -ERESTARTSYS;
			            }else
			            {
				DataRate=((Tp*(Bpn+1)-1)*Mp*Lp*4)/(Tp*(Kp*Mp+Rp));
				//DataRate_remain=((((Tp*(Bpn+1)-1)*Mp*Lp*4)%(Tp*(Kp*Mp+Rp)))*1000)/(Tp*(Kp*Mp+Rp));
				    }
				//pts.adslLineRateInfo_pt->adslDataRateds = DataRate * 1000 + DataRate_remain;
				pts.adslLineRateInfo_pt->adslDataRateds = DataRate;
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineRateInfo_pt->flags)), LINE_RATE_DATA_RATEUS_FLAG)){
			if (adsl_mode <=8 && adsl_mode_extend==0) // adsl mode
			{
				if (chantype.interleave)
					LINE_RATE_DATA_RATEUS_FLAG_ADSL1_LP0_MAKECMV;
				else
					LINE_RATE_DATA_RATEUS_FLAG_ADSL1_LP1_MAKECMV;

				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group RATE Address 0 Index 0");
#endif
					pts.adslLineRateInfo_pt->adslDataRateus = 0;
				}
				else{
					pts.adslLineRateInfo_pt->adslDataRateus = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
				}
			}else  // adsl 2/2+
			{
				unsigned long Mp,Lp,Tp,Rp,Kp,Bpn,DataRate,DataRate_remain;
				Mp=Lp=Tp=Rp=Kp=Bpn=DataRate=DataRate_remain=0;
				//// down stream data rate

		 		if (chantype.interleave)
				{
					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_LP_LP0_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 14 Index 0");
#endif
						Lp = 0;
					}else
						Lp=RxMessage[4];

					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_RP_LP0_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 12 Index 0");
#endif
						Rp = 0;
					}else
						Rp=RxMessage[4];

					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_MP_LP0_MAKECMV;
						if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 13 Index 0");
#endif
						Mp = 0;
					}else
						Mp=RxMessage[4];

					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_TP_LP0_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 15 Index 0");
#endif
						Tp = 0;
					}else
						Tp=RxMessage[4];

					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_KP_LP0_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 17 Index 0");
#endif
						Kp = 0;
					}else
					{
						Kp=RxMessage[4]+ RxMessage[5]+1;
						Bpn=RxMessage[4]+ RxMessage[5];
					}
				}else
				{
					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_LP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 14 Index 1");
#endif
						Lp = 0;
					}else
						Lp=RxMessage[4];

					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_RP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 12 Index 1");
#endif
						Rp = 0;
					}else
						Rp=RxMessage[4];

					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_MP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 13 Index 1");
#endif
						Mp = 0;
					}else
						Mp=RxMessage[4];

					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_TP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 15 Index 1");
#endif
						Tp = 0;
					}else
						Tp=RxMessage[4];

					LINE_RATE_DATA_RATEDS_FLAG_ADSL2_KP_LP1_MAKECMV;
					if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
						printk("\n\nCMV fail, Group CNFG Address 17 Index 2");
#endif
						Kp = 0;
					}else
					{
						Kp=RxMessage[4]+ RxMessage[5]+1;
						Bpn=RxMessage[4]+ RxMessage[5];
					}
				}
				    if (Tp==0 || Kp== 0 || Mp == 0 || Rp == 0)
			            {
			              	DataRate = 0;
			              	meierr = -ERESTARTSYS;
			            }else
			            {
				DataRate=((Tp*(Bpn+1)-1)*Mp*Lp*4)/(Tp*(Kp*Mp+Rp));
				//DataRate_remain=((((Tp*(Bpn+1)-1)*Mp*Lp*4)%(Tp*(Kp*Mp+Rp)))*1000)/(Tp*(Kp*Mp+Rp));
				    }
				//pts.adslLineRateInfo_pt->adslDataRateus = DataRate * 1000 + DataRate_remain;
				pts.adslLineRateInfo_pt->adslDataRateus = DataRate;
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineRateInfo_pt->flags)), LINE_RATE_ATTNDRDS_FLAG)){
			LINE_RATE_ATTNDRDS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group INFO Address 68 Index 4");
#endif
				pts.adslLineRateInfo_pt->adslATTNDRds = 0;
			}
			else{
				pts.adslLineRateInfo_pt->adslATTNDRds = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineRateInfo_pt->flags)), LINE_RATE_ATTNDRUS_FLAG)){
			if (adsl_mode <=8 && adsl_mode_extend==0) // adsl mode
			{
				LINE_RATE_ATTNDRUS_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 69 Index 4");
#endif
					pts.adslLineRateInfo_pt->adslATTNDRus = 0;
				}
				else{
					pts.adslLineRateInfo_pt->adslATTNDRus = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
				}
			}else
			{
				hdlc_cmd[0]=0x0181;
				hdlc_cmd[1]=0x24;
				MEI_MUTEX_UNLOCK(mei_sema);
				if (loop_diagnostics_completed==1)
				    return -ERESTARTSYS;
				if (IFX_ADSL_SendHdlc((char *)&hdlc_cmd[0],4)!= -EBUSY)
				{
					MEI_WAIT(1);
					hdlc_rx_len=0;
					hdlc_rx_len = IFX_ADSL_ReadHdlc((char *)&hdlc_rx_buffer,32*2);
					if (hdlc_rx_len <=0)
					{
						meierr = -ERESTARTSYS;
						goto GET_ADSL_LINE_RATE_END;
					}
					pts.adslLineRateInfo_pt->adslATTNDRus = (u32)le16_to_cpu(hdlc_rx_buffer[1])<<16 | (u32)le16_to_cpu(hdlc_rx_buffer[2]);
				}
				if(MEI_MUTEX_LOCK(mei_sema))
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_RATE_END;
				}
			}
		}
		copy_to_user((char *)lon, (char *)pts.adslLineRateInfo_pt, sizeof(adslLineRateInfo));
		MEI_MUTEX_UNLOCK(mei_sema);

GET_ADSL_LINE_RATE_END:
		kfree(pts.adslLineRateInfo_pt);
		break;

	case GET_ADSL_LINE_INFO:
		if (showtime!=1 && loop_diagnostics_completed!=1)
			return -ERESTARTSYS;
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;

		pts.adslLineInfo_pt = (adslLineInfo *)kmalloc(sizeof(adslLineInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslLineInfo_pt, (char *)lon, sizeof(adslLineInfo));

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_INTLV_DEPTHDS_FLAG)){
			if (chantype.interleave)
				LINE_INFO_INTLV_DEPTHDS_FLAG_LP0_MAKECMV;
			else
				LINE_INFO_INTLV_DEPTHDS_FLAG_LP1_MAKECMV;

			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group CNFG Address 27 Index 0");
#endif
				pts.adslLineInfo_pt->adslInterleaveDepthds = 0;
			}
			else{
				pts.adslLineInfo_pt->adslInterleaveDepthds = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_INTLV_DEPTHUS_FLAG)){
			if (chantype.interleave)
				LINE_INFO_INTLV_DEPTHUS_FLAG_LP0_MAKECMV;
			else
				LINE_INFO_INTLV_DEPTHUS_FLAG_LP1_MAKECMV;

			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group CNFG Address 16 Index 0");
#endif
				pts.adslLineInfo_pt->adslInterleaveDepthus = 0;
			}
			else{
				pts.adslLineInfo_pt->adslInterleaveDepthus = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_LATNDS_FLAG)){
			LINE_INFO_LATNDS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group INFO Address 68 Index 1");
#endif
				pts.adslLineInfo_pt->adslLATNds = 0;
			}
			else{
				pts.adslLineInfo_pt->adslLATNds = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_LATNUS_FLAG)){
			if (adsl_mode <=8 && adsl_mode_extend==0) // adsl mode
			{
				LINE_INFO_LATNUS_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 69 Index 1");
#endif
					pts.adslLineInfo_pt->adslLATNus = 0;
				}
				else{
					pts.adslLineInfo_pt->adslLATNus = RxMessage[4];
				}
			}else
			{
				hdlc_cmd[0]=0x0181;
				hdlc_cmd[1]=0x21;
				MEI_MUTEX_UNLOCK(mei_sema);
				if (loop_diagnostics_completed==1)
				    return -ERESTARTSYS;
				if (IFX_ADSL_SendHdlc((char *)&hdlc_cmd[0],4)!= -EBUSY)
				{
					MEI_WAIT(1);
					hdlc_rx_len=0;
					hdlc_rx_len = IFX_ADSL_ReadHdlc((char *)&hdlc_rx_buffer,32*2);
					if (hdlc_rx_len <=0)
					{
						meierr = -ERESTARTSYS;
						goto GET_ADSL_LINE_INFO_END;
					}
					pts.adslLineInfo_pt->adslLATNus = le16_to_cpu(hdlc_rx_buffer[1]);
				}else
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
				if(MEI_MUTEX_LOCK(mei_sema))
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_SATNDS_FLAG)){
			LINE_INFO_SATNDS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group INFO Address 68 Index 2");
#endif
				pts.adslLineInfo_pt->adslSATNds = 0;
			}
			else{
				pts.adslLineInfo_pt->adslSATNds = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_SATNUS_FLAG)){
			if (adsl_mode <=8 && adsl_mode_extend==0) // adsl mode
			{
				LINE_INFO_SATNUS_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 69 Index 2");
#endif
					pts.adslLineInfo_pt->adslSATNus = 0;
				}
				else{
					pts.adslLineInfo_pt->adslSATNus = RxMessage[4];
				}
			}else
			{
				hdlc_cmd[0]=0x0181;
				hdlc_cmd[1]=0x22;
				MEI_MUTEX_UNLOCK(mei_sema);
				if (loop_diagnostics_completed==1)
				    return -ERESTARTSYS;
				if (IFX_ADSL_SendHdlc((char *)&hdlc_cmd[0],4)!= -EBUSY)
				{
					MEI_WAIT(1);
					hdlc_rx_len=0;
					hdlc_rx_len = IFX_ADSL_ReadHdlc((char *)&hdlc_rx_buffer,32*2);
					if (hdlc_rx_len <=0)
					{
						meierr = -ERESTARTSYS;
						goto GET_ADSL_LINE_INFO_END;
					}
					pts.adslLineInfo_pt->adslSATNus = le16_to_cpu(hdlc_rx_buffer[1]);
				}else
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
				if(MEI_MUTEX_LOCK(mei_sema))
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_SNRMNDS_FLAG)){
			if (adsl_mode <=8 && adsl_mode_extend==0) // adsl mode
			{
				LINE_INFO_SNRMNDS_FLAG_ADSL1_MAKECMV;
			}
			else if ((adsl_mode == 0x4000) || (adsl_mode == 0x8000) || adsl_mode_extend > 0)
			{
				LINE_INFO_SNRMNDS_FLAG_ADSL2PLUS_MAKECMV;
			}
			else
			{
				LINE_INFO_SNRMNDS_FLAG_ADSL2_MAKECMV;
			}
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group INFO Address 68 Index 3");
#endif
				pts.adslLineInfo_pt->adslSNRMds = 0;
			}
			else{
				if (adsl_mode>8 || adsl_mode_extend>0)
				{
					int SNRMds,SNRMds_remain;
					SNRMds=RxMessage[4];
					SNRMds_remain=((SNRMds&0xff)*1000)/256;
					SNRMds=(SNRMds>>8)&0xff;
					if ((SNRMds_remain%100)>=50) SNRMds_remain=(SNRMds_remain/100)+1;
					else  SNRMds_remain=(SNRMds_remain/100);
					pts.adslLineInfo_pt->adslSNRMds = SNRMds*10 + SNRMds_remain;
				}else
				{
					pts.adslLineInfo_pt->adslSNRMds = RxMessage[4];
				}
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_SNRMNUS_FLAG)){
			if (adsl_mode <=8 && adsl_mode_extend == 0)
			{
				LINE_INFO_SNRMNUS_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 69 Index 3");
#endif
					pts.adslLineInfo_pt->adslSNRMus = 0;
				}
				else{
					pts.adslLineInfo_pt->adslSNRMus = RxMessage[4];
				}
			}else
			{
				hdlc_cmd[0]=0x0181;
				hdlc_cmd[1]=0x23;
				MEI_MUTEX_UNLOCK(mei_sema);
				if (loop_diagnostics_completed==1)
				    return -ERESTARTSYS;
				if (IFX_ADSL_SendHdlc((char *)&hdlc_cmd[0],4)!= -EBUSY)
				{
					MEI_WAIT(1);
					hdlc_rx_len=0;
					hdlc_rx_len = IFX_ADSL_ReadHdlc((char *)&hdlc_rx_buffer,32*2);
					if (hdlc_rx_len <=0)
					{
						meierr = -ERESTARTSYS;
						goto GET_ADSL_LINE_INFO_END;
					}
					pts.adslLineInfo_pt->adslSNRMus = le16_to_cpu(hdlc_rx_buffer[1]);
				}else
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
				if(MEI_MUTEX_LOCK(mei_sema))
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
			}
		}

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_ACATPDS_FLAG)){
#if 0
			if (adsl_mode <=8 && adsl_mode_extend == 0)
#endif
			{
				LINE_INFO_ACATPDS_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 68 Index 6");
#endif
					pts.adslLineInfo_pt->adslACATPds = 0;
				}
				else{
					pts.adslLineInfo_pt->adslACATPds = RxMessage[4];
				}
			}
#if 0
			else
			{
				hdlc_cmd[0]=0x0181;
				hdlc_cmd[1]=0x25;
				MEI_MUTEX_UNLOCK(mei_sema);
				if (IFX_ADSL_SendHdlc((char *)&hdlc_cmd[0],4)!= -EBUSY)
				{
					MEI_WAIT(1);
					hdlc_rx_len=0;
					hdlc_rx_len = IFX_ADSL_ReadHdlc((char *)&hdlc_rx_buffer,32*2);
					if (hdlc_rx_len <=0)
					{
						meierr = -ERESTARTSYS;
						goto GET_ADSL_LINE_INFO_END;
					}
					pts.adslLineInfo_pt->adslACATPds = le16_to_cpu(hdlc_rx_buffer[1]);
				}else
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
				if(MEI_MUTEX_LOCK(mei_sema))
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
			}
#endif
		}

		if(IS_FLAG_SET((&(pts.adslLineInfo_pt->flags)), LINE_INFO_ACATPUS_FLAG)){
			if (adsl_mode <=8 && adsl_mode_extend == 0)
			{
				LINE_INFO_ACATPUS_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 69 Index 6");
#endif
					pts.adslLineInfo_pt->adslACATPus = 0;
				}
				else{
					pts.adslLineInfo_pt->adslACATPus = RxMessage[4];
				}
			}else
			{
				hdlc_cmd[0]=0x0181;
				hdlc_cmd[1]=0x26;
				MEI_MUTEX_UNLOCK(mei_sema);
				if (loop_diagnostics_completed==1)
				    return -ERESTARTSYS;
				if (IFX_ADSL_SendHdlc((char *)&hdlc_cmd[0],4)!= -EBUSY)
				{
					MEI_WAIT(1);
					hdlc_rx_len=0;
					hdlc_rx_len = IFX_ADSL_ReadHdlc((char *)&hdlc_rx_buffer,32*2);
					if (hdlc_rx_len <=0)
					{
						meierr = -ERESTARTSYS;
						goto GET_ADSL_LINE_INFO_END;
					}
					pts.adslLineInfo_pt->adslACATPus = le16_to_cpu(hdlc_rx_buffer[1]);
				}else
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
				if(MEI_MUTEX_LOCK(mei_sema))
				{
					meierr = -ERESTARTSYS;
					goto GET_ADSL_LINE_INFO_END;
				}
			}
		}

		copy_to_user((char *)lon, (char *)pts.adslLineInfo_pt, sizeof(adslLineInfo));
		MEI_MUTEX_UNLOCK(mei_sema);

GET_ADSL_LINE_INFO_END:
		kfree(pts.adslLineInfo_pt);
		break;

	case GET_ADSL_NEAREND_STATS:
		if (showtime!=1 && loop_diagnostics_completed!=1)
			return -ERESTARTSYS;
		if(MEI_MUTEX_LOCK(mei_sema))
			return -ERESTARTSYS;

		pts.adslNearEndPerfStats_pt = (adslNearEndPerfStats *)kmalloc(sizeof(adslNearEndPerfStats), GFP_KERNEL);
		copy_from_user((char *)pts.adslNearEndPerfStats_pt, (char *)lon, sizeof(adslNearEndPerfStats));

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_SUPERFRAME_FLAG)){
			NEAREND_PERF_SUPERFRAME_FLAG_LSW_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 20 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslSuperFrames = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslSuperFrames = (u32)(RxMessage[4]);
			}
			NEAREND_PERF_SUPERFRAME_FLAG_MSW_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 21 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslSuperFrames = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslSuperFrames += (((u32)(RxMessage[4]))<<16);
			}
		}

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_LOS_FLAG) ||
		   IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_LOF_FLAG) ||
		   IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_LPR_FLAG) ||
		   IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_NCD_FLAG) ||
		   IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_LCD_FLAG) ){
			NEAREND_PERF_LOS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 0 Index 0");
#endif
				RxMessage[4] = 0;
			}
			if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_LOS_FLAG)){
				if( (RxMessage[4]&0x1) == 0x1)
					pts.adslNearEndPerfStats_pt->adslneLOS = 1;
				else
					pts.adslNearEndPerfStats_pt->adslneLOS = 0;
			}

			if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_LOF_FLAG)){
				if( (RxMessage[4]&0x2) == 0x2)
					pts.adslNearEndPerfStats_pt->adslneLOF = 1;
				else
					pts.adslNearEndPerfStats_pt->adslneLOF = 0;
			}

			if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_LPR_FLAG)){
				if( (RxMessage[4]&0x4) == 0x4)
					pts.adslNearEndPerfStats_pt->adslneLPR = 1;
				else
					pts.adslNearEndPerfStats_pt->adslneLPR = 0;
			}

			if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_NCD_FLAG)){
				pts.adslNearEndPerfStats_pt->adslneNCD = (RxMessage[4]>>4)&0x3;
			}

			if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_LCD_FLAG)){
				pts.adslNearEndPerfStats_pt->adslneLCD = (RxMessage[4]>>6)&0x3;
			}
		}

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_CRC_FLAG)){
			if (chantype.interleave)
				NEAREND_PERF_CRC_FLAG_LP0_MAKECMV;
			else
				NEAREND_PERF_CRC_FLAG_LP1_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 2 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslneCRC = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslneCRC = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
			}
		}

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_RSCORR_FLAG)){
			if (chantype.interleave)
				NEAREND_PERF_RSCORR_FLAG_LP0_MAKECMV;
			else
				NEAREND_PERF_RSCORR_FLAG_LP1_MAKECMV;

			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 3 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslneRSCorr = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslneRSCorr = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_FECS_FLAG)){
			NEAREND_PERF_FECS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 6 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslneFECS = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslneFECS = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_ES_FLAG)){
			NEAREND_PERF_ES_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 7 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslneES = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslneES = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_SES_FLAG)){
			NEAREND_PERF_SES_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 8 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslneSES = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslneSES = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_LOSS_FLAG)){
			NEAREND_PERF_LOSS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 9 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslneLOSS = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslneLOSS = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_UAS_FLAG)){
			NEAREND_PERF_UAS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 10 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslneUAS = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslneUAS = RxMessage[4];
			}
		}

		if(IS_FLAG_SET((&(pts.adslNearEndPerfStats_pt->flags)), NEAREND_PERF_HECERR_FLAG)){
			if (chantype.bearchannel0)
			{
				NEAREND_PERF_HECERR_FLAG_BC0_MAKECMV;
			}else if (chantype.bearchannel1)
			{
				NEAREND_PERF_HECERR_FLAG_BC1_MAKECMV;
			}
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 11 Index 0");
#endif
				pts.adslNearEndPerfStats_pt->adslneHECErrors = 0;
			}
			else{
				pts.adslNearEndPerfStats_pt->adslneHECErrors = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
			}
		}

		copy_to_user((char *)lon, (char *)pts.adslNearEndPerfStats_pt, sizeof(adslNearEndPerfStats));
		kfree(pts.adslNearEndPerfStats_pt);

		MEI_MUTEX_UNLOCK(mei_sema);
		break;

	case GET_ADSL_FAREND_STATS:

        	if (showtime!=1 && loop_diagnostics_completed!=1)
			return -ERESTARTSYS;

		if (adsl_mode>8 || adsl_mode_extend > 0)
		{
			do_gettimeofday(&time_now);
			if( FarendData_acquire_time.tv_sec==0 || time_now.tv_sec - FarendData_acquire_time.tv_sec>=1)
			{
				hdlc_cmd[0]=0x105;
				if (loop_diagnostics_completed==1)
					return -ERESTARTSYS;

				if (IFX_ADSL_SendHdlc((unsigned char *)&hdlc_cmd[0],2)!= -EBUSY)
				{
					MEI_WAIT(1);
					hdlc_rx_len=0;
					hdlc_rx_len = IFX_ADSL_ReadHdlc((char *)&hdlc_rx_buffer,32*2);
					if (hdlc_rx_len <=0)
					{
						return -ERESTARTSYS;
					}
					FarendStatsData.adslfeRSCorr = ((u32)le16_to_cpu(hdlc_rx_buffer[1]) << 16) + (u32)le16_to_cpu(hdlc_rx_buffer[2]);
					FarendStatsData.adslfeCRC = ((u32)le16_to_cpu(hdlc_rx_buffer[3]) << 16) + (u32)le16_to_cpu(hdlc_rx_buffer[4]);
					FarendStatsData.adslfeFECS = ((u32)le16_to_cpu(hdlc_rx_buffer[5]) << 16) + (u32)le16_to_cpu(hdlc_rx_buffer[6]);
					FarendStatsData.adslfeES = ((u32)le16_to_cpu(hdlc_rx_buffer[7]) << 16) + (u32)le16_to_cpu(hdlc_rx_buffer[8]);
					FarendStatsData.adslfeSES = ((u32)le16_to_cpu(hdlc_rx_buffer[9]) << 16) + (u32)le16_to_cpu(hdlc_rx_buffer[10]);
					FarendStatsData.adslfeLOSS = ((u32)le16_to_cpu(hdlc_rx_buffer[11]) << 16) + (u32)le16_to_cpu(hdlc_rx_buffer[12]);
					FarendStatsData.adslfeUAS = ((u32)le16_to_cpu(hdlc_rx_buffer[13]) << 16) + (u32)le16_to_cpu(hdlc_rx_buffer[14]);
					do_gettimeofday(&FarendData_acquire_time);
				}else
				{
					return -ERESTARTSYS;
				}
			}
		}

		if(MEI_MUTEX_LOCK(mei_sema))
        		return -ERESTARTSYS;
        	pts.adslFarEndPerfStats_pt = (adslFarEndPerfStats *)kmalloc(sizeof(adslFarEndPerfStats), GFP_KERNEL);
		copy_from_user((char *)pts.adslFarEndPerfStats_pt, (char *)lon, sizeof(adslFarEndPerfStats));
		if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_LOS_FLAG) ||
		   IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_LOF_FLAG) ||
		   IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_LPR_FLAG) ||
		   IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_NCD_FLAG) ||
		   IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_LCD_FLAG) ){
			FAREND_PERF_LOS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 1 Index 0");
#endif
				RxMessage[4] = 0;
			}
			if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_LOS_FLAG)){
				if((RxMessage[4]&0x1) == 0x1)
					pts.adslFarEndPerfStats_pt->adslfeLOS = 1;
				else
					pts.adslFarEndPerfStats_pt->adslfeLOS = 0;
			}

			if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_LOF_FLAG)){
				if((RxMessage[4]&0x2) == 0x2)
					pts.adslFarEndPerfStats_pt->adslfeLOF = 1;
				else
					pts.adslFarEndPerfStats_pt->adslfeLOF = 0;
			}

			if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_LPR_FLAG)){
				if((RxMessage[4]&0x4) == 0x4)
					pts.adslFarEndPerfStats_pt->adslfeLPR = 1;
				else
					pts.adslFarEndPerfStats_pt->adslfeLPR = 0;
			}

			if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_NCD_FLAG)){
				pts.adslFarEndPerfStats_pt->adslfeNCD = (RxMessage[4]>>4)&0x3;
			}

			if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_LCD_FLAG)){
				pts.adslFarEndPerfStats_pt->adslfeLCD = (RxMessage[4]>>6)&0x3;
			}
		}

		if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_CRC_FLAG)){
			if (adsl_mode<=8 && adsl_mode_extend == 0)
			{
				if (chantype.interleave)
				{
					FAREND_PERF_CRC_FLAG_LP0_MAKECMV;
				}
				else
				{
					FAREND_PERF_CRC_FLAG_LP1_MAKECMV;
				}
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 24 Index 0");
#endif
					pts.adslFarEndPerfStats_pt->adslfeCRC = 0;
				}
				else{
					pts.adslFarEndPerfStats_pt->adslfeCRC = RxMessage[4];
				}
			}else
			{
				pts.adslFarEndPerfStats_pt->adslfeCRC = FarendStatsData.adslfeCRC;
			}
		}

		if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_RSCORR_FLAG)){
			if (adsl_mode<=8 && adsl_mode_extend == 0)
			{
				if (chantype.interleave)
					FAREND_PERF_RSCORR_FLAG_LP0_MAKECMV;
				else
					FAREND_PERF_RSCORR_FLAG_LP1_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 28 Index 0");
#endif
					pts.adslFarEndPerfStats_pt->adslfeRSCorr = 0;
				}
				else{
					pts.adslFarEndPerfStats_pt->adslfeRSCorr = RxMessage[4];

				}
			}
			else
			{
				pts.adslFarEndPerfStats_pt->adslfeRSCorr = FarendStatsData.adslfeRSCorr;
			}
		}

		if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_FECS_FLAG)){
			if (adsl_mode<=8 && adsl_mode_extend == 0)
			{
				FAREND_PERF_FECS_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 32 Index 0");
#endif
					pts.adslFarEndPerfStats_pt->adslfeFECS = 0;
				}
				else{
					pts.adslFarEndPerfStats_pt->adslfeFECS = RxMessage[4];
				}
			}else {
				pts.adslFarEndPerfStats_pt->adslfeFECS = FarendStatsData.adslfeFECS;
			}
		}

		if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_ES_FLAG)){
			if (adsl_mode<=8 && adsl_mode_extend == 0)
			{
				FAREND_PERF_ES_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 33 Index 0");
#endif
					pts.adslFarEndPerfStats_pt->adslfeES = 0;
				}
				else{
					pts.adslFarEndPerfStats_pt->adslfeES = RxMessage[4];
				}
			}else
			{
				pts.adslFarEndPerfStats_pt->adslfeES = FarendStatsData.adslfeES;
			}
		}

		if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_SES_FLAG)){
			if (adsl_mode<=8 && adsl_mode_extend == 0)
			{
				FAREND_PERF_SES_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 34 Index 0");
#endif
					pts.adslFarEndPerfStats_pt->adslfeSES = 0;
				}
				else{
					pts.adslFarEndPerfStats_pt->adslfeSES = RxMessage[4];

				}
			}else
			{
				pts.adslFarEndPerfStats_pt->adslfeSES = FarendStatsData.adslfeSES;
			}
		}

		if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_LOSS_FLAG)){
			if (adsl_mode<=8 && adsl_mode_extend == 0)
			{
				FAREND_PERF_LOSS_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
					pts.adslFarEndPerfStats_pt->adslfeLOSS = 0;
				}
				else{
					pts.adslFarEndPerfStats_pt->adslfeLOSS = RxMessage[4];

				}
			}else
			{
				pts.adslFarEndPerfStats_pt->adslfeLOSS = FarendStatsData.adslfeLOSS;
			}
		}

		if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_UAS_FLAG)){
			if (adsl_mode<=8 && adsl_mode_extend == 0)
			{
				FAREND_PERF_UAS_FLAG_MAKECMV;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 36 Index 0");
#endif
					pts.adslFarEndPerfStats_pt->adslfeUAS = 0;
				}
				else{
					pts.adslFarEndPerfStats_pt->adslfeUAS = RxMessage[4];

				}
			}else
			{
				pts.adslFarEndPerfStats_pt->adslfeUAS = FarendStatsData.adslfeUAS;
			}
		}

		if(IS_FLAG_SET((&(pts.adslFarEndPerfStats_pt->flags)), FAREND_PERF_HECERR_FLAG)){
			if (chantype.bearchannel0)
			{
				FAREND_PERF_HECERR_FLAG_BC0_MAKECMV;
			}else if (chantype.bearchannel1)
			{
				FAREND_PERF_HECERR_FLAG_BC1_MAKECMV;
			}
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 37 Index 0");
#endif
				pts.adslFarEndPerfStats_pt->adslfeHECErrors = 0;
			}
			else{
				pts.adslFarEndPerfStats_pt->adslfeHECErrors = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
			}
		}

		copy_to_user((char *)lon, (char *)pts.adslFarEndPerfStats_pt, sizeof(adslFarEndPerfStats));
		kfree(pts.adslFarEndPerfStats_pt);

		MEI_MUTEX_UNLOCK(mei_sema);

		break;
// 603221:tc.chen end

	case GET_ADSL_ATUR_SUBCARRIER_STATS:
               	pts.adslATURSubcarrierInfo_pt = (adslATURSubcarrierInfo *)kmalloc(sizeof(adslATURSubcarrierInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslATURSubcarrierInfo_pt, (char *)lon, sizeof(adslATURSubcarrierInfo));

		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)), FAREND_HLINSC) ||
		   IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)), FAREND_HLINPS) ){

			if (loop_diagnostics_completed == 0)
			{
				MEI_WAIT_EVENT_TIMEOUT(wait_queue_loop_diagnostic,300*100);
				if (loop_diagnostics_completed==0)
				{
					return -ERESTARTSYS;
				}
			}
		}
		if(MEI_MUTEX_LOCK(mei_sema))
        		return -ERESTARTSYS;

		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_HLINSC)){
			FAREND_HLINSC_MAKECMV(H2D_CMV_READ);
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				pts.adslATURSubcarrierInfo_pt->HLINSCds = 0;
			}
			else{
				pts.adslATURSubcarrierInfo_pt->HLINSCds = RxMessage[4];
			}
		}
		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_HLINPS)){
			int index=0,size=12;
			//printk("FAREND_HLINPS\n");
			for (index=0;index<1024;index+=size)
			{
				if (index+size>=1024)
					size = 1024-index;
				FAREND_HLINPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				}
				else{
					memcpy(&pts.adslATURSubcarrierInfo_pt->HLINpsds[index],&RxMessage[4],size*2);
#if 0
					int msg_idx;
					for(msg_idx=0;msg_idx<size;msg_idx++)
						printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);
#endif
				}
			}
		}

		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_HLOGMT)){
			FAREND_HLOGMT_MAKECMV(H2D_CMV_READ);
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				pts.adslATURSubcarrierInfo_pt->HLOGMTds = 0;
			}
			else{
				pts.adslATURSubcarrierInfo_pt->HLOGMTds = RxMessage[4];

			}
		}

		/////////////////////////////////////////////////////////////////////////
		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_HLOGPS)){
			//printk("FAREND_HLOGPS\n");
			int index=0,size=12;
			for (index=0;index<256;index+=size)
			{
				if (index+size>=256)
					size = 256-index;

				FAREND_HLOGPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				}
				else{
					if (adsl_mode < 0x4000 && adsl_mode_extend==0)//adsl2 mode
					{
							memcpy(&pts.adslATURSubcarrierInfo_pt->HLOGpsds[index],&RxMessage[4],size*2);
					}else
					{
						int msg_idx=0;
						for (msg_idx=0;msg_idx<size;msg_idx++)
						{
								pts.adslATURSubcarrierInfo_pt->HLOGpsds[(index+msg_idx)*2+1] = RxMessage[4+msg_idx];
							//printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);
						}
					}
				}
			}
			if (adsl_mode >= 0x4000 || adsl_mode_extend >0)//adsl2+ mode
			{
					pts.adslATURSubcarrierInfo_pt->HLOGpsds[0] = pts.adslATURSubcarrierInfo_pt->HLOGpsds[1];
				for (index=1;index<256;index++)
				{
						pts.adslATURSubcarrierInfo_pt->HLOGpsds[index*2]   = (pts.adslATURSubcarrierInfo_pt->HLOGpsds[(index)*2-1] +  pts.adslATURSubcarrierInfo_pt->HLOGpsds[(index)*2+1] +1) >>1;
				}
			}
		}
		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_QLNMT)){
			FAREND_QLNMT_MAKECMV(H2D_CMV_READ);
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
					pts.adslATURSubcarrierInfo_pt->QLNMTds = 0;
			}
			else{
					pts.adslATURSubcarrierInfo_pt->QLNMTds = RxMessage[4];
			}
		}

		/////////////////////////////////////////////////////////////////////////
		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_QLNPS)){
			int index=0,size=12;
			//printk("FAREND_QLNPS\n");
			for (index=0;index<128;index+=size)
			{
				if (index+size>=128)
					size = 128-index;
				FAREND_QLNPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				}
				else{
					int msg_idx=0;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{
							//memcpy(&pts.adslATURSubcarrierInfo_pt->QLNpsds[index],&RxMessage[4],size*2);
						if (adsl_mode < 0x4000 && adsl_mode_extend==0)//adsl2 mode
						{
								pts.adslATURSubcarrierInfo_pt->QLNpsds[(index+msg_idx)*2] = (u16)(RxMessage[4+msg_idx]&0xFF);
								pts.adslATURSubcarrierInfo_pt->QLNpsds[(index+msg_idx)*2+1] = (u16)((RxMessage[4+msg_idx]>>8)&0xFF);
						}else
						{
								pts.adslATURSubcarrierInfo_pt->QLNpsds[(index+msg_idx)*4+1] = (u16)(RxMessage[4+msg_idx]&0xFF);
								pts.adslATURSubcarrierInfo_pt->QLNpsds[(index+msg_idx)*4+3] = (u16)((RxMessage[4+msg_idx]>>8)&0xFF);
							//printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);
						}
					}
				}
			}
			if (adsl_mode >= 0x4000 || adsl_mode_extend >0)//adsl2+ mode
			{
				pts.adslATURSubcarrierInfo_pt->QLNpsds[0] = pts.adslATURSubcarrierInfo_pt->QLNpsds[1];
				for (index=1;index<256;index++)
				{
					pts.adslATURSubcarrierInfo_pt->QLNpsds[index*2]   = (pts.adslATURSubcarrierInfo_pt->QLNpsds[(index)*2-1] +  pts.adslATURSubcarrierInfo_pt->QLNpsds[(index)*2+1]) >>1;
				}
			}
		}
		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_SNRMT)){
			FAREND_SNRMT_MAKECMV(H2D_CMV_READ);
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
					pts.adslATURSubcarrierInfo_pt->SNRMTds = 0;
			}
			else{
					pts.adslATURSubcarrierInfo_pt->SNRMTds = RxMessage[4];
			}
		}
		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_SNRPS)){
			int index=0,size=12;
			//printk("FAREND_SNRPS\n");
			for (index=0;index<512;index+=size)
			{
				if (index+size>=512)
					size = 512-index;
				if (loop_diagnostics_completed!=1)
				FAREND_SNRPS_MAKECMV(H2D_CMV_READ,index,size);
				else
					FAREND_SNRPS_DIAG_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				}
				else{
					//memcpy(&pts.adslATURSubcarrierInfo_pt->SNRpsds[index],&RxMessage[4],size*2);
					int msg_idx=0;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{
#if 1 /* Align units to G.997.1 - Ritesh */
						int val;
#if 0

						val = (u16)(RxMessage[4+msg_idx]&0xFF) / 2;
						val = ROUND_8_8_TO_SIGNED_SHORT(val);
						pts.adslATURSubcarrierInfo_pt->SNRpsds[index+msg_idx] = -32 + val;
#else
						val = CONV_8_8_TO_SCALED_SIGNED_SHORT(RxMessage[4+msg_idx]);
					        pts.adslATURSubcarrierInfo_pt->SNRpsds[index+msg_idx] = (val + 320 )*2/10;
#endif
#else

						pts.adslATURSubcarrierInfo_pt->SNRpsds[index+msg_idx] = (u16)(RxMessage[4+msg_idx]&0xFF);
#endif
						//printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);
					}

				}
			}
		}
		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_BITPS)){
			int index=0,size=12;
			//printk("FAREND_BITPS\n");
			for (index=0;index<256;index+=size)
			{
				if (index+size>=256)
					size = 256-index;
				FAREND_BITPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				}
				else{
					int msg_idx=0;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{
						pts.adslATURSubcarrierInfo_pt->BITpsds[(index+msg_idx)*2] = (u16)(RxMessage[4+msg_idx]&0xFF);
						pts.adslATURSubcarrierInfo_pt->BITpsds[(index+msg_idx)*2+1] = (u16)((RxMessage[4+msg_idx]>>8)&0xFF);
						//printk("index:%d ,cmv_result: %04X, %d\n",index+msg_idx,RxMessage[4+msg_idx],RxMessage[4+msg_idx]);
					}
				}
			}
		}
		if(IS_FLAG_SET((&(pts.adslATURSubcarrierInfo_pt->flags)),  FAREND_GAINPS)){
			int index=0,size=12;
			//printk("FAREND_GAINPS\n");
			for (index=0;index<512;index+=size)
			{
				if (index+size>=512 )
					size = 512 -index;
				FAREND_GAINPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				}
				else{
#if 1
					int msg_idx=0;
					short gainpsds = 0;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{
#if 0
						pts.adslATURSubcarrierInfo_pt->GAINpsds[(index+msg_idx)*2] = RxMessage[4+msg_idx]&0xFF;
						pts.adslATURSubcarrierInfo_pt->GAINpsds[(index+msg_idx)*2+1] = (RxMessage[4+msg_idx]>>8)&0xFF;
#else
						gainpsds = CONV_8_8_TO_SCALED_SIGNED_SHORT(RxMessage[4+msg_idx]);
						pts.adslATURSubcarrierInfo_pt->GAINpsds[(index+msg_idx)	* 2] = gainpsds;
#endif

					}
#else
					memcpy(&pts.adslATURSubcarrierInfo_pt->GAINpsds[index],&RxMessage[4],size*2);
#endif
#if 0
					int msg_idx=0;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{
						printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);

					}
#endif
				}
			}
		}
		copy_to_user((char *)lon, (char *)pts.adslATURSubcarrierInfo_pt, sizeof(adslATURSubcarrierInfo));
		kfree(pts.adslATURSubcarrierInfo_pt);

		MEI_MUTEX_UNLOCK(mei_sema);
		break;
	case GET_ADSL_ATUC_SUBCARRIER_STATS:
               	pts.adslATUCSubcarrierInfo_pt = (adslATUCSubcarrierInfo *)kmalloc(sizeof(adslATUCSubcarrierInfo), GFP_KERNEL);
		copy_from_user((char *)pts.adslATUCSubcarrierInfo_pt, (char *)lon, sizeof(adslATUCSubcarrierInfo));

		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)), NEAREND_HLINSC) ||
			IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)), NEAREND_HLINPS) ||
			IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)), NEAREND_HLOGMT) ||
			IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)), NEAREND_HLOGPS) ||
			IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)), NEAREND_QLNMT) ||
			IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)), NEAREND_QLNPS) ||
			IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)), NEAREND_SNRMT) ||
			IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)), NEAREND_SNRPS) ||
			IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)), NEAREND_HLINPS) ){

			if (loop_diagnostics_completed == 0)
			{
				MEI_WAIT_EVENT_TIMEOUT(wait_queue_loop_diagnostic,300*HZ);
				if (loop_diagnostics_completed==0)
				{
					return -ERESTARTSYS;
				}
			}
		}
		if(MEI_MUTEX_LOCK(mei_sema))
        		return -ERESTARTSYS;


		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_HLINSC)){
			NEAREND_HLINSC_MAKECMV(H2D_CMV_READ);
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group INFO Address 71 Index 2");
#endif
				pts.adslATUCSubcarrierInfo_pt->HLINSCus = 0;
			}
			else{
				pts.adslATUCSubcarrierInfo_pt->HLINSCus = RxMessage[4];
			}
		}
		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_HLINPS)){
			int index=0,size=12;
			//printk("NEAREND_HLINPS\n");
			for (index=0;index<128;index+=size)
			{
				if (index+size>=128)
					size = 128-index;
				NEAREND_HLINPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 73 Index 0");
#endif
				}
				else{
					memcpy(&pts.adslATUCSubcarrierInfo_pt->HLINpsus[index],&RxMessage[4],size*2);
#if 0
					int msg_idx;
					for (msg_idx=0;msg_idx<size;msg_idx++)
						printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);
#endif
				}
			}
		}

		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_HLOGMT)){
			NEAREND_HLOGMT_MAKECMV(H2D_CMV_READ);
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group INFO Address 80 Index 0");
#endif
				pts.adslATUCSubcarrierInfo_pt->HLOGMTus = 0;
			}
			else{
				pts.adslATUCSubcarrierInfo_pt->HLOGMTus = RxMessage[4];

			}
		}

		/////////////////////////////////////////////////////////////////////////
		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_HLOGPS)){
			int index=0,size=12;
			//printk("NEAREND_HLOGPS\n");
			for (index=0;index<64;index+=size)
			{
				if (index+size>=64)
					size = 64-index;
				NEAREND_HLOGPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 75 Index 0");
#endif
				}
				else{
#if 0
					if (adsl_mode <0x4000)//adsl /adsl2 mode
					{
#endif
						memcpy(&pts.adslATUCSubcarrierInfo_pt->HLOGpsus[index],&RxMessage[4],size*2);
#if 0
					}else
					{
						int msg_idx=0;
						for (msg_idx=0;msg_idx<size;msg_idx++)
						{
							//pts.adslATUCSubcarrierInfo_pt->HLOGpsus[(index+msg_idx)*2+1] = RxMessage[4+msg_idx];
							pts.adslATUCSubcarrierInfo_pt->HLOGpsus[(index+msg_idx)] = RxMessage[4+msg_idx];
						}
					}
#endif
				}
			}
#if 0
			if (adsl_mode >= 0x4000)//adsl2 mode
			{
				pts.adslATUCSubcarrierInfo_pt->HLOGpsus[0] = pts.adslATUCSubcarrierInfo_pt->HLOGpsus[1];
				for (index=1;index<64;index++)
				{
					pts.adslATUCSubcarrierInfo_pt->HLOGpsus[index*2]   = (pts.adslATUCSubcarrierInfo_pt->HLOGpsus[(index)*2-1] +  pts.adslATUCSubcarrierInfo_pt->HLOGpsus[(index)*2+1]) >>1;
				}
			}
#endif
		}
		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_QLNMT)){
			NEAREND_QLNMT_MAKECMV(H2D_CMV_READ);
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group INFO Address 80 Index 1");
#endif
				pts.adslATUCSubcarrierInfo_pt->QLNMTus = 0;
			}
			else{
				pts.adslATUCSubcarrierInfo_pt->QLNMTus = RxMessage[4];
			}
		}

		/////////////////////////////////////////////////////////////////////////
		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_QLNPS)){
			int index=0,size=12;
			//printk("NEAREND_QLNPS\n");
			for (index=0;index<32;index+=size)
			{
				if (index+size>=32)
					size = 32-index;
				NEAREND_QLNPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 77 Index 0");
#endif
				}
				else{
					int msg_idx=0;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{

#if 0
						//memcpy(&pts.adslATUCSubcarrierInfo_pt->QLNpsds[index],&RxMessage[4],size*2);
						if (adsl_mode == 0x200 || adsl_mode == 0x800 || adsl_mode ==0x2000  || adsl_mode ==0x4000 || (adsl_mode == 0 && (adsl_mode_extend == 0x4 || adsl_mode_extend == 0x2))//ADSL 2 Annex B(0x200)/J(0x800)/M(0x2000) //ADSL 2+ B,J,M
						if (adsl_mode < 0x4000 && adsl_mode_extend==0)//adsl2 mode
						{
							pts.adslATUCSubcarrierInfo_pt->QLNpsus[(index+msg_idx)*4+1] = (u16)(RxMessage[4+msg_idx]&0xFF);
							pts.adslATUCSubcarrierInfo_pt->QLNpsus[(index+msg_idx)*4+3] = (u16)((RxMessage[4+msg_idx]>>8)&0xFF);
						}else
#endif
						{
							pts.adslATUCSubcarrierInfo_pt->QLNpsus[(index+msg_idx)*2] = (u16)(RxMessage[4+msg_idx]&0xFF);
							pts.adslATUCSubcarrierInfo_pt->QLNpsus[(index+msg_idx)*2+1] = (u16)((RxMessage[4+msg_idx]>>8)&0xFF);
							//printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);
						}
					}
				}
			}
#if 0
			//if (adsl_mode <0x4000)//Annex I/J/L/M
			if (adsl_mode == 0x200 || adsl_mode == 0x800 || adsl_mode ==0x2000  || adsl_mode ==0x4000 || (adsl_mode == 0 && (adsl_mode_extend == 0x4 || adsl_mode_extend == 0x2))//ADSL 2 Annex B(0x200)/J(0x800)/M(0x2000) //ADSL 2+ B,J,M
			{
				pts.adslATUCSubcarrierInfo_pt->QLNpsus[0] = pts.adslATUCSubcarrierInfo_pt->QLNpsus[1];
				for (index=1;index<64;index++)
				{
					pts.adslATUCSubcarrierInfo_pt->QLNpsus[index*2]   = (pts.adslATUCSubcarrierInfo_pt->QLNpsus[(index)*2-1] +  pts.adslATUCSubcarrierInfo_pt->QLNpsus[(index)*2+1]) >>1;
				}
			}
#endif
		}
		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_SNRMT)){
			NEAREND_SNRMT_MAKECMV(H2D_CMV_READ);
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group INFO Address 80 Index 2");
#endif
				pts.adslATUCSubcarrierInfo_pt->SNRMTus = 0;
			}
			else{
				pts.adslATUCSubcarrierInfo_pt->SNRMTus = RxMessage[4];
			}
		}
		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_SNRPS)){
			int index=0,size=12;
			//printk("NEAREND_SNRPS\n");
			for (index=0;index<32;index+=size)
			{
				if (index+size>=32)
					size = 32-index;
				NEAREND_SNRPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 78 Index 0");
#endif
				}
				else{
					//memcpy(&pts.adslATUCSubcarrierInfo_pt->SNRpsus[index],&RxMessage[4],size*2);
					int msg_idx=0;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{
#if 0
#if 1 /* Align units to G.997.1 - Ritesh */
						int val;

						val = (u16)(RxMessage[4+msg_idx]&0xFF);
						val = ROUND_8_8_TO_SIGNED_SHORT(val);
						pts.adslATUCSubcarrierInfo_pt->SNRpsus[index+msg_idx] = -32 + val;
#else
						pts.adslATUCSubcarrierInfo_pt->SNRpsus[index+msg_idx] = (u16)(RxMessage[4+msg_idx]&0xFF);
#endif
#else
						pts.adslATUCSubcarrierInfo_pt->SNRpsus[(index+msg_idx)*2] = (u16)(RxMessage[4+msg_idx]&0xFF);
						pts.adslATUCSubcarrierInfo_pt->SNRpsus[(index+msg_idx)*2+1] = (u16)((RxMessage[4+msg_idx]>>8)&0xFF);
#endif
						//printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);
					}
				}
			}
		}
		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_BITPS)){
			int index=0,size=12;
			//printk("NEAREND_BITPS\n");
			for (index=0;index<32;index+=size)
			{
				if (index+size>=32)
					size = 32-index;
				NEAREND_BITPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 22 Index 0");
#endif
				}
				else{
					int msg_idx=0;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{
						pts.adslATUCSubcarrierInfo_pt->BITpsus[(index+msg_idx)*2] = (u16)(RxMessage[4+msg_idx]&0xFF);
						pts.adslATUCSubcarrierInfo_pt->BITpsus[(index+msg_idx)*2+1] = (u16)((RxMessage[4+msg_idx]>>8)&0xFF);
						//printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);
					}
				}
			}
		}
		if(IS_FLAG_SET((&(pts.adslATUCSubcarrierInfo_pt->flags)),  NEAREND_GAINPS)){
			int index=0,size=12;
			for (index=0;index<64;index+=size)
			{
				if (index+size>=64)
					size = 64-index;
				NEAREND_GAINPS_MAKECMV(H2D_CMV_READ,index,size);
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
					printk("\n\nCMV fail, Group INFO Address 24 Index 0");
#endif
				}
				else{
#if 1
					int msg_idx=0;
					short gainpsus = 0;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{
#if 0
						pts.adslATUCSubcarrierInfo_pt->GAINpsds[(index+msg_idx)*2] = RxMessage[4+msg_idx]&0xFF;
						pts.adslATUCSubcarrierInfo_pt->GAINpsds[(index+msg_idx)*2+1] = (RxMessage[4+msg_idx]>>8)&0xFF;
#else
#endif
#if 0
						gainpsus = CONV_8_8_TO_SCALED_SIGNED_SHORT(RxMessage[4+msg_idx]);
						pts.adslATUCSubcarrierInfo_pt->GAINpsus[(index+msg_idx)	* 2] = gainpsus;
#else
						gainpsus = CONV_3_13_TO_SCALED_SIGNED_SHORT(RxMessage[4+msg_idx]);
	 					pts.adslATUCSubcarrierInfo_pt->GAINpsus[(index+msg_idx)	] = gainpsus;
#endif

					}
#else
					memcpy(&pts.adslATUCSubcarrierInfo_pt->GAINpsus[index],&RxMessage[4],size*2);
#endif
#if 0
					int msg_idx;
					for (msg_idx=0;msg_idx<size;msg_idx++)
					{
						printk("index:%d ,cmv_result: %04X\n",index+msg_idx,RxMessage[4+msg_idx]);
					}
#endif
				}
			}
		}
		copy_to_user((char *)lon, (char *)pts.adslATUCSubcarrierInfo_pt, sizeof(adslATUCSubcarrierInfo));
		kfree(pts.adslATUCSubcarrierInfo_pt);
		MEI_MUTEX_UNLOCK(mei_sema);
		break;
	case GET_ADSL_LINE_INIT_STATS:
		copy_to_user((char *)lon, (char *)&AdslInitStatsData, sizeof(AdslInitStatsData));
		break;

	case GET_ADSL_POWER_SPECTRAL_DENSITY:
		if(MEI_MUTEX_LOCK(mei_sema))
                	return -ERESTARTSYS;
		i=0;
		pts.adslPowerSpectralDensity_pt = (adslPowerSpectralDensity *)kmalloc(sizeof(adslPowerSpectralDensity), GFP_KERNEL);
		memset((char *)pts.adslPowerSpectralDensity_pt, 0, sizeof(adslPowerSpectralDensity));

		//US
		NOMPSD_US_MAKECMV;
		if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
			printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
			i=-1;
		}
		else{
			j=RxMessage[4];
		}
		PCB_US_MAKECMV;
		if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
			printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				i=-1;
			}
			else{
				temp=RxMessage[4];
			}
			RMSGI_US_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				i=-1;
			}
			else{
				k=(int16_t)RxMessage[4];
			}
			if (i==0)
			{
				pts.adslPowerSpectralDensity_pt->ACTPSDus = ((int )(j*256 - temp*10*256 + k*10)) /256;
			}
			// DS
			i=0;
			j=temp=temp2=0;
			NOMPSD_DS_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				i=-1;
			}
			else{
				j=RxMessage[4];
			}
			PCB_DS_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				i=-1;
			}
			else{
				temp=RxMessage[4];
			}
			RMSGI_DS_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
#ifdef IFX_ADSL_DEBUG_ON
				printk("\n\nCMV fail, Group PLAM Address 35 Index 0");
#endif
				i=-1;
			}
			else{
				//temp2=RxMessage[4];
				k=(int16_t)RxMessage[4];
			}
			if (i==0)
			{
				pts.adslPowerSpectralDensity_pt->ACTPSDds = ((int )(j*256 - temp*10*256 + k*10)) /256;
			}
		copy_to_user((char *)lon, (char *)pts.adslPowerSpectralDensity_pt, sizeof(adslPowerSpectralDensity));
		kfree(pts.adslPowerSpectralDensity_pt);
		MEI_MUTEX_UNLOCK(mei_sema);
		break;
	}
	return MEI_SUCCESS;
}


int adsl_mib_poll(void *unused)
{
#if defined(__LINUX__)
	struct task_struct *tsk = current;
#endif
	int i=0;
	struct timeval time_now;
	struct timeval time_fini;
	u32 temp=0,temp2=0;
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));

	ifx_adsl_mib * mib_ptr;
	u16 * data=NULL;  //used in makeCMV, to pass in payload when CMV set, ignored when CMV read.
#if defined(__LINUX__)
	daemonize();
	reparent_to_init();
	strcpy(tsk->comm, "kmibpoll");
	sigfillset(&tsk->blocked);
#endif
	IFX_ADSL_DMSG("Inside mib poll loop ...\n");
	i=0;
	while(ifx_adsl_mib_shutdown==0){
		if(i<MIB_INTERVAL)
		{
			MEI_WAIT_EVENT_TIMEOUT(wait_queue_mibdaemon, ((MIB_INTERVAL-i)/(1000/HZ)));
			if (ifx_adsl_mib_shutdown)
				break;
		}
		i=0;
		if(showtime==1){
			do_gettimeofday(&time_now);
			if(time_now.tv_sec - current_intvl->start_time.tv_sec>=900){
				if(current_intvl->list.next!=&interval_list){
					current_intvl = list_entry(current_intvl->list.next, ifx_adsl_mib, list);
					do_gettimeofday(&(current_intvl->start_time));
				}
				else{
					mib_ptr = list_entry(interval_list.next, ifx_adsl_mib, list);
					list_del(interval_list.next);
					memset(mib_ptr, 0, sizeof(ifx_adsl_mib));
					list_add_tail(&(mib_ptr->list), &interval_list);
					if(current_intvl->list.next==&interval_list)
					IFX_ADSL_EMSG("\n\nlink list error");
					current_intvl = list_entry(current_intvl->list.next, ifx_adsl_mib, list);
					do_gettimeofday(&(current_intvl->start_time));
				}
			}

			if(MEI_MUTEX_LOCK(mei_sema))
      				return -ERESTARTSYS;
			if(showtime!=1)
				goto mib_poll_end;
			ATUC_PERF_ESS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 7 Index 0");
			}
			else{
				temp = RxMessage[4]-mib_pread.ATUC_PERF_ESS;
				if(temp>=0){
					current_intvl->AtucPerfEs+=temp;
					ATUC_PERF_ESS+=temp;
					mib_pread.ATUC_PERF_ESS = RxMessage[4];
				}
				else{
					current_intvl->AtucPerfEs+=0xffff-mib_pread.ATUC_PERF_ESS+RxMessage[4];
					ATUC_PERF_ESS+=0xffff-mib_pread.ATUC_PERF_ESS+RxMessage[4];
					mib_pread.ATUC_PERF_ESS = RxMessage[4];
				}
			}

			if(showtime!=1)
				goto mib_poll_end;
			ATUR_PERF_ESS_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 33 Index 0");
			}
			else{
				temp = RxMessage[4]-mib_pread.ATUR_PERF_ESS;
				if(temp>=0){
					current_intvl->AturPerfEs+=temp;
					ATUR_PERF_ESS+=temp;
					mib_pread.ATUR_PERF_ESS = RxMessage[4];
				}
				else{
					current_intvl->AturPerfEs+=0xffff-mib_pread.ATUR_PERF_ESS+RxMessage[4];
					ATUR_PERF_ESS+=	0xffff-mib_pread.ATUR_PERF_ESS+RxMessage[4];
					mib_pread.ATUR_PERF_ESS=RxMessage[4];
				}
			}
			if(showtime!=1)
				goto mib_poll_end;
			// to update rx/tx blocks
			ATUR_CHAN_RECV_BLK_FLAG_MAKECMV_LSW;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 20 Index 0");
			}
			else{
				temp = RxMessage[4];
			}
			if(showtime!=1)
				goto mib_poll_end;
			ATUR_CHAN_RECV_BLK_FLAG_MAKECMV_MSW;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 21 Index 0");
			}
			else{
				temp2 = RxMessage[4];
			}
			if((temp + (temp2<<16) - mib_pread.ATUR_CHAN_RECV_BLK)>=0){
				current_intvl->AturChanPerfRxBlk+=temp + (temp2<<16) - mib_pread.ATUR_CHAN_RECV_BLK;
				ATUR_CHAN_RECV_BLK+=temp + (temp2<<16) - mib_pread.ATUR_CHAN_RECV_BLK;
				mib_pread.ATUR_CHAN_RECV_BLK = temp + (temp2<<16);
			}
			else{
				current_intvl->AturChanPerfRxBlk+=0xffffffff - mib_pread.ATUR_CHAN_RECV_BLK +(temp + (temp2<<16));
				ATUR_CHAN_RECV_BLK+=0xffffffff - mib_pread.ATUR_CHAN_RECV_BLK +(temp + (temp2<<16));
				mib_pread.ATUR_CHAN_RECV_BLK = temp + (temp2<<16);
			}
			current_intvl->AturChanPerfTxBlk = current_intvl->AturChanPerfRxBlk;
			ATUR_CHAN_TX_BLK = ATUR_CHAN_RECV_BLK;

			if(chantype.interleave == 1){
				if(showtime!=1)
					goto mib_poll_end;
				ATUR_CHAN_CORR_BLK_FLAG_MAKECMV_INTL;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 3 Index 0");
				}
				else{
					temp = RxMessage[4] - mib_pread.ATUR_CHAN_CORR_BLK_INTL;
					if(temp>=0){
						current_intvl->AturChanPerfCorrBlk+=temp;
						ATUR_CHAN_CORR_BLK+=temp;
						mib_pread.ATUR_CHAN_CORR_BLK_INTL = RxMessage[4];
					}
					else{
						current_intvl->AturChanPerfCorrBlk+=0xffff - mib_pread.ATUR_CHAN_CORR_BLK_INTL +RxMessage[4];
						ATUR_CHAN_CORR_BLK+=0xffff - mib_pread.ATUR_CHAN_CORR_BLK_INTL +RxMessage[4];
						mib_pread.ATUR_CHAN_CORR_BLK_INTL = RxMessage[4];
					}
				}
			}
			else if(chantype.fast == 1){
				if(showtime!=1)
					goto mib_poll_end;
				ATUR_CHAN_CORR_BLK_FLAG_MAKECMV_FAST;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 3 Index 1");
				}
				else{
					temp = RxMessage[4] - mib_pread.ATUR_CHAN_CORR_BLK_FAST;
					if(temp>=0){
						current_intvl->AturChanPerfCorrBlk+=temp;
						ATUR_CHAN_CORR_BLK+=temp;
						mib_pread.ATUR_CHAN_CORR_BLK_FAST = RxMessage[4];
					}
					else{
						current_intvl->AturChanPerfCorrBlk+=0xffff - mib_pread.ATUR_CHAN_CORR_BLK_FAST + RxMessage[4];
						ATUR_CHAN_CORR_BLK+=0xffff - mib_pread.ATUR_CHAN_CORR_BLK_FAST + RxMessage[4];
						mib_pread.ATUR_CHAN_CORR_BLK_FAST = RxMessage[4];
					}
				}
			}

			if(chantype.interleave == 1){
				if(showtime!=1)
					goto mib_poll_end;
				ATUR_CHAN_UNCORR_BLK_FLAG_MAKECMV_INTL;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 2 Index 0");
				}
				else{
					temp = RxMessage[4] - mib_pread.ATUR_CHAN_UNCORR_BLK_INTL;
					if(temp>=0){
						current_intvl->AturChanPerfUncorrBlk+=temp;
						ATUR_CHAN_UNCORR_BLK+=temp;
						mib_pread.ATUR_CHAN_UNCORR_BLK_INTL = RxMessage[4];
					}
					else{
						current_intvl->AturChanPerfUncorrBlk+=0xffff - mib_pread.ATUR_CHAN_UNCORR_BLK_INTL + RxMessage[4];
						ATUR_CHAN_UNCORR_BLK+=0xffff - mib_pread.ATUR_CHAN_UNCORR_BLK_INTL + RxMessage[4];
						mib_pread.ATUR_CHAN_UNCORR_BLK_INTL = RxMessage[4];
					}
				}
			}
			else if(chantype.fast == 1){
				if(showtime!=1)
				goto mib_poll_end;
				ATUR_CHAN_UNCORR_BLK_FLAG_MAKECMV_FAST;
				if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
					IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 2 Index 1");
				}
				else{
					temp = RxMessage[4] - mib_pread.ATUR_CHAN_UNCORR_BLK_FAST;
					if(temp>=0){
						current_intvl->AturChanPerfUncorrBlk+=temp;
						ATUR_CHAN_UNCORR_BLK+=temp;
						mib_pread.ATUR_CHAN_UNCORR_BLK_FAST = RxMessage[4];
					}
					else{
						current_intvl->AturChanPerfUncorrBlk+=0xffff - mib_pread.ATUR_CHAN_UNCORR_BLK_FAST + RxMessage[4];
						ATUR_CHAN_UNCORR_BLK+=0xffff - mib_pread.ATUR_CHAN_UNCORR_BLK_FAST + RxMessage[4];
						mib_pread.ATUR_CHAN_UNCORR_BLK_FAST = RxMessage[4];
					}
				}
			}

			//RFC-3440
#ifdef IFX_ADSL_MIB_RFC3440
			if(showtime!=1)
				goto mib_poll_end;
			ATUC_PERF_STAT_FASTR_FLAG_MAKECMV; //???
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 0 Address 0 Index 0");
			}
			else{
				temp = RxMessage[4] - mib_pread.ATUC_PERF_STAT_FASTR;
				if(temp>=0){
					current_intvl->AtucPerfStatFastR+=temp;
					ATUC_PERF_STAT_FASTR+=temp;
					mib_pread.ATUC_PERF_STAT_FASTR = RxMessage[4];
				}
				else{
					current_intvl->AtucPerfStatFastR+=0xffff - mib_pread.ATUC_PERF_STAT_FASTR + RxMessage[4];
					ATUC_PERF_STAT_FASTR+=0xffff - mib_pread.ATUC_PERF_STAT_FASTR + RxMessage[4];
					mib_pread.ATUC_PERF_STAT_FASTR = RxMessage[4];
				}
			}
			if(showtime!=1)
				goto mib_poll_end;
			ATUC_PERF_STAT_FAILED_FASTR_FLAG_MAKECMV; //???
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 0 Address 0 Index 0");
			}
			else{
				temp = RxMessage[4] - mib_pread.ATUC_PERF_STAT_FAILED_FASTR;
				if(temp>=0){
					current_intvl->AtucPerfStatFailedFastR+=temp;
					ATUC_PERF_STAT_FAILED_FASTR+=temp;
					mib_pread.ATUC_PERF_STAT_FAILED_FASTR = RxMessage[4];
				}
				else{
					current_intvl->AtucPerfStatFailedFastR+=0xffff - mib_pread.ATUC_PERF_STAT_FAILED_FASTR + RxMessage[4];
					ATUC_PERF_STAT_FAILED_FASTR+=0xffff - mib_pread.ATUC_PERF_STAT_FAILED_FASTR + RxMessage[4];
					mib_pread.ATUC_PERF_STAT_FAILED_FASTR = RxMessage[4];
				}
			}
			if(showtime!=1)
				goto mib_poll_end;
			ATUC_PERF_STAT_SESL_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 8 Index 0");
			}
			else{
				temp = RxMessage[4] - mib_pread.ATUC_PERF_STAT_SESL;
				if(temp>=0){
					current_intvl->AtucPerfStatSesL+=temp;
					ATUC_PERF_STAT_SESL+=temp;
					mib_pread.ATUC_PERF_STAT_SESL = RxMessage[4];
				}
				else{
					current_intvl->AtucPerfStatSesL+=0xffff - mib_pread.ATUC_PERF_STAT_SESL + RxMessage[4];
					ATUC_PERF_STAT_SESL+=0xffff - mib_pread.ATUC_PERF_STAT_SESL + RxMessage[4];
					mib_pread.ATUC_PERF_STAT_SESL = RxMessage[4];
				}
			}
			if(showtime!=1)
				goto mib_poll_end;
			ATUC_PERF_STAT_UASL_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 10 Index 0");
			}
			else{
				temp = RxMessage[4] - mib_pread.ATUC_PERF_STAT_UASL;
				if(temp>=0){
					current_intvl->AtucPerfStatUasL+=temp;
					ATUC_PERF_STAT_UASL+=temp;
					mib_pread.ATUC_PERF_STAT_UASL = RxMessage[4];
				}
				else{
					current_intvl->AtucPerfStatUasL+=0xffff - mib_pread.ATUC_PERF_STAT_UASL + RxMessage[4];
					ATUC_PERF_STAT_UASL+=0xffff - mib_pread.ATUC_PERF_STAT_UASL + RxMessage[4];
					mib_pread.ATUC_PERF_STAT_UASL = RxMessage[4];
				}
			}
			if(showtime!=1)
				goto mib_poll_end;
			ATUR_PERF_STAT_SESL_FLAG_MAKECMV;
			if(meiCMV(TxMessage, YES_REPLY, RxMessage)!=MEI_SUCCESS){
				IFX_ADSL_EMSG("\n\nCMV fail, Group 7 Address 34 Index 0");
			}
			else{
				temp = RxMessage[4] - mib_pread.ATUR_PERF_STAT_SESL;
				if(temp>=0){
					current_intvl->AtucPerfStatUasL+=temp;
					ATUC_PERF_STAT_UASL+=temp;
					mib_pread.ATUR_PERF_STAT_SESL = RxMessage[4];
				}
				else{
					current_intvl->AtucPerfStatUasL+=0xffff - mib_pread.ATUR_PERF_STAT_SESL + RxMessage[4];
					ATUC_PERF_STAT_UASL+=0xffff - mib_pread.ATUR_PERF_STAT_SESL + RxMessage[4];
					mib_pread.ATUR_PERF_STAT_SESL = RxMessage[4];
				}
			}

#endif
mib_poll_end:
			MEI_MUTEX_UNLOCK(mei_sema);
			do_gettimeofday(&time_fini);
			i = ((int)((time_fini.tv_sec-time_now.tv_sec)*1000)) + ((int)((time_fini.tv_usec-time_now.tv_usec)/1000))  ; //msec
		}//showtime==1
	} //end while
#if defined(__LINUX__)
	complete_and_exit(&mei_mib_thread_exit,0); //wake up mibdaemon
#endif
	return 0;
}

#if defined(__LINUX__)
int mib_poll_init(void)
{
	IFX_ADSL_DMSG("Starting mib_poll...\n");
	ifx_adsl_mib_shutdown = 0;
	init_completion(&mei_mib_thread_exit);
	kernel_thread(adsl_mib_poll, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGNAL);
	return 0;
}
#else
extern int mib_poll_init(void);
#endif

int mei_mib_adsl_link_up(void)
{
	u16 data[12];  //used in makeCMV, to pass in payload when CMV set, ignored when CMV read.
	u16 TxMessage[MSG_LENGTH],RxMessage[MSG_LENGTH];

	// decide what channel, then register
	makeCMV(H2D_CMV_READ, STAT, 12, 0, 1, data,TxMessage);
	if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
		IFX_ADSL_EMSG("\n\nCMV fail, Group 2 Address 12 Index 0");
	}
	else{
		if((RxMessage[4]&0x1)==1){
#if defined(__LINUX__)
			if(register_netdev(&interleave_mei_net)!=0){
				IFX_ADSL_EMSG("\n\n Register interleave Device Failed.");
			}
			else
#endif
			{
				chantype.interleave = 1;
				chantype.fast= 0;
				IFX_ADSL_DMSG("\n channel is interleave");
			}
		}
		else if((RxMessage[4]&0x2)==2){
#if defined(__LINUX__)
			if(register_netdev(&fast_mei_net)!=0){
				IFX_ADSL_EMSG("\n\n Register fast Device Failed.");
			}
			else
#endif
			{
				chantype.fast = 1;
				chantype.interleave = 0;
				IFX_ADSL_DMSG("\n channel is fast");
			}
		}
		else{
			IFX_ADSL_EMSG("\nunknown channel type, 0x%8x", RxMessage[4]);
		}
		if ( (RxMessage[4]&0x100) == 0x100)
		{
			chantype.bearchannel0 = 1;
		}else 	if ( (RxMessage[4]&0x200) == 0x200)
		{
			chantype.bearchannel1 = 1;
		}
	}

	// read adsl mode
	IFX_ADSL_ReadAdslMode();

	// update previous channel tx rate
	ATUC_CHAN_CURR_TX_RATE_FLAG_MAKECMV;
	if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
		IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 1 Index 0");
		PrevTxRate.adslAtucChanPrevTxRate = 0;
	}
	else{
		PrevTxRate.adslAtucChanPrevTxRate = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
	}
	ATUR_CHAN_CURR_TX_RATE_FLAG_MAKECMV;
	if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
		IFX_ADSL_EMSG("\n\nCMV fail, Group 6 Address 0 Index 0");
		PrevTxRate.adslAturChanPrevTxRate = 0;
	}
	else{
		PrevTxRate.adslAturChanPrevTxRate = (u32)(RxMessage[4]) + (((u32)(RxMessage[5]))<<16);
	}
	return 0;
}

int mei_mib_adsl_link_down(void)
{
	u16 RxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));
	u16 TxMessage[MSG_LENGTH]__attribute__ ((aligned(4)));

	PrevTxRate.adslAtucChanPrevTxRate=0;
	PrevTxRate.adslAturChanPrevTxRate=0;
	CurrStatus.adslAtucCurrStatus=0;
	CurrStatus.adslAturCurrStatus=0;
#if defined(__LINUX__)
	if(chantype.interleave==1){
		kfree(interleave_mei_net.priv);
		unregister_netdev(&interleave_mei_net);
	}
	else if(chantype.fast==1){
		kfree(fast_mei_net.priv);
		unregister_netdev(&fast_mei_net);
	}
#endif
	chantype.interleave=0;
	chantype.fast=0;
	chantype.bearchannel0 = 0;
	chantype.bearchannel1 = 0;
	adsl_mode = 0;

	makeCMV(H2D_CMV_READ, STAT, 0, 0, 1, NULL,TxMessage); //get status
	if(meiCMV(TxMessage, YES_REPLY,RxMessage)!=MEI_SUCCESS){
		AdslInitStatsData.FullInitializationCount++;
		AdslInitStatsData.FailedFullInitializationCount++;
		AdslInitStatsData.LINIT_Errors++;
	}else
	{
		if ( RxMessage[4]!=0x1)
		{
			AdslInitStatsData.FullInitializationCount++;
			if ( RxMessage[4] != 0x7)
			{
				AdslInitStatsData.LINIT_Errors++;
				AdslInitStatsData.FailedFullInitializationCount++;
			}
		}
	}
	return 0;
}

int ifx_adsl_mib_init(void)
{
	int i=0;

        printk("Infineon ADSL MIB Version:%s\n", IFX_ADSL_MIB_VERSION);
	MEI_INIT_WAKELIST("MIBQ",wait_queue_mibdaemon);		// for mib daemon
  	memset(&mib_pflagtime, 0, (sizeof(mib_flags_pretime)));
 	// initialize link list for intervals
	mei_mib = (ifx_adsl_mib *)kmalloc((sizeof(ifx_adsl_mib)*INTERVAL_NUM), GFP_KERNEL);
	if(mei_mib == NULL){
		return -1;
	}
	memset(mei_mib, 0, (sizeof(ifx_adsl_mib)*INTERVAL_NUM));

  	memset(&AdslInitStatsData, 0, sizeof(AdslInitStatsData));

  	INIT_LIST_HEAD(&interval_list);
	for(i=0;i<INTERVAL_NUM;i++)
		list_add_tail(&(mei_mib[i].list), &interval_list);
	current_intvl = list_entry(interval_list.next, ifx_adsl_mib, list);
	do_gettimeofday(&(current_intvl->start_time));
#if defined(__LINUX__)
  	if(register_netdev(&phy_mei_net)!=0){
          IFX_ADSL_EMSG("\n\n Register phy Device Failed.");
          return -1;
  	}
#endif
	mib_poll_init();
  return 0;
}

void ifx_adsl_mib_cleanup(void)
{
	ifx_adsl_mib_shutdown = 1;
	mei_mib_adsl_link_down();
	MEI_WAKEUP_EVENT(wait_queue_mibdaemon); //wake up mibdaemon
#if defined(__LINUX__)
	wait_for_completion(&mei_mib_thread_exit); //wait mibdemaon exit
	kfree(phy_mei_net.priv);
        unregister_netdev(&phy_mei_net);
#endif
        kfree(mei_mib);
        return;
}

