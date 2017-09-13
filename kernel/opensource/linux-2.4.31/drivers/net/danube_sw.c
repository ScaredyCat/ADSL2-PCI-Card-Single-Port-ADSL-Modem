/******************************************************************************
**
** FILE NAME    : danube_sw.c
** PROJECT      : Danube
** MODULES     	: ETH Interface (MII0)
**
** DATE         : 11 AUG 2005
** AUTHOR       : Wu Qi Ming
** DESCRIPTION  : ETH Interface (MII0) Driver
** COPYRIGHT    : 	Copyright (c) 2006
**			Infineon Technologies AG
**			Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 11 AUG 2005  Wu Qi Ming      Initiate Version
** 23 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/



#include <linux/module.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/interrupt.h> /* mark_bh */
#include <linux/mii.h>
#include <asm/uaccess.h>
#include <linux/in.h>
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/tcp.h>         /* struct tcphdr */
#include <linux/skbuff.h>
#include <linux/in6.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/ethtool.h>
#include <asm/checksum.h>
#include <linux/init.h>
#include <asm/delay.h>


#define ENABLE_RX_DPLUS_PATH       0

#define ENABLE_DIRECT_BRIDGE            0
#if defined(ENABLE_DIRECT_BRIDGE) && ENABLE_DIRECT_BRIDGE
  struct net_device *switch_eth2_dev;
  extern struct net_device *eth2_switch_dev;
#endif

#define LOOPBACK_OVERHEAD (128 + MAX_HEADER + 16 + 16)

#undef LOOPBACK_TEST

#include <asm/danube/danube.h>
#include <asm/danube/danube_sw.h>
#include <asm/danube/danube_dma.h>

#undef DANUBE_SW_DUMP

#undef ENABLE_TRACE
#ifdef ENABLE_TRACE
#define TRACE(fmt,args...) printk("%s: " fmt, __FUNCTION__ , ##args)
#else
#define TRACE(fmt,args...)
#endif

#define DANUBE_SW_DMSG(fmt, args...) printk( KERN_ERR  "%s: " fmt,__FUNCTION__, ## args)

#define DANUBE_SW_INT_NO 1
#define ETHERNET_PACKET_DMA_BUFFER_SIZE 1536
#define MDIO_DELAY 1 /*mdio negotiation time*/


#define IFX_SUCCESS 1
#define IFX_ERROR   -1

#define MII_MODE 1
#define REV_MII_MODE 2
/***************************************** Functions  Declaration *********************************/
static int switch_init(struct net_device *dev);
void switch_tx_timeout (struct net_device *dev);
int switch_tx(struct sk_buff *skb, struct net_device *dev);
int switch_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);
struct net_device_stats *switch_stats(struct net_device *dev);
int switch_change_mtu(struct net_device *dev, int new_mtu);
int switch_set_mac_address(struct net_device *dev, void *p);
void switch_tx_timeout (struct net_device *dev);
static int probe_transceiver(struct net_device *dev);
void open_rx_dma(struct net_device *dev);
void close_rx_dma(struct net_device *dev);
void amazon_xon(struct net_device *dev);
int init_dma_device(_dma_device_info* dma_dev);
/***************************************** Global Data *******************************************/
static struct net_device switch_devs[2] = {
    { init: switch_init, },
    { init: switch_init, }
};


//060620:henryhsu modify for vlan
#if defined(CONFIG_IFX_NFEXT_SWITCH_PHYPORT) || defined(CONFIG_IFX_NFEXT_SWITCH_PHYPORT_MODULE)
	#define AMAZON_SW_VLAN_BRIDGING_SUPPORT
	int (*amazon_sw_phyport_rx)(struct net_device *dev, struct sk_buff *skb) = NULL;
	EXPORT_SYMBOL(amazon_sw_phyport_rx);
	struct net_device *amazon_sw_for_phyport = &(switch_devs[0]);
	EXPORT_SYMBOL(amazon_sw_for_phyport);
#endif


struct proc_dir_entry* g_danube_etop_dir;
static int timeout = 10*HZ;
static int rx_stopped=0;
static int g_transmit=0;
/*************************************************************************************************/

#ifdef DANUBE_SW_DUMP
/*
 * Brief:	dump skb data
 */
static inline void dump_skb(u32 len, char * data)
{
	int i;
	for(i=0;i<len;i++){
		printk("%2.2x ",(u8)(data[i]));
		if (i % 16 == 15)
			printk("\n");
	}
	printk("\n");
}
#endif


static int probe_transceiver(struct net_device *dev)
{
   struct switch_priv* priv=(struct switch_priv*)dev->priv;
   int enet_offset=0;
   if(dev-switch_devs==0)/*The first interface*/
    {
      priv->mdio_phy_addr=PHY0_ADDR;
      *(ENET_MAC_DA0+enet_offset)=\
        (dev->dev_addr[0]<<24)|(dev->dev_addr[1]<<16)\
	|(dev->dev_addr[2]<<8)|(dev->dev_addr[3]);
      *(ENET_MAC_DA1+enet_offset)=(dev->dev_addr[4]<<24)|(dev->dev_addr[5]<<16);
      if(!(*(ENET_MAC_CFG+enet_offset)&LINK_MASK))
      {

        /*enable the mdio state machine and reset */
        *ETOP_MDIO_CFG|=(*ETOP_MDIO_CFG & ~(PHYA0_MASK|UMM0_MASK|SMRST_MASK))\
                       |PHY0_ADDR<<3|1<<1|1<<13;

	udelay(MDIO_DELAY);/*wait for some time?, fix me*/
	if(!(*(ENET_MAC_CFG+enet_offset)&LINK_MASK))
	return IFX_ERROR;
      }
    }
    else if(dev-switch_devs==1)
    {
      enet_offset=0x40<<2;
      *(ENET_MAC_DA0+enet_offset)=\
        (dev->dev_addr[0]<<24)|(dev->dev_addr[1]<<16)\
	|(dev->dev_addr[2]<<8)|(dev->dev_addr[3]);
      *(ENET_MAC_DA1+enet_offset)=(dev->dev_addr[4]<<24)|(dev->dev_addr[5]<<16);

      priv->mdio_phy_addr=PHY1_ADDR;
      if(!(*(ENET_MAC_CFG+enet_offset)&LINK_MASK))
      {
        /*enable the mdio state machine and reset */
        *ETOP_MDIO_CFG|=(*ETOP_MDIO_CFG & ~(PHYA1_MASK|UMM1_MASK|SMRST_MASK))\
                       |PHY1_ADDR<<8|1<<2|1<<13;

	udelay(MDIO_DELAY);/*wait for some time?, fix me*/
	if(!(*(ENET_MAC_CFG+enet_offset)&LINK_MASK))
	return IFX_ERROR;
      }
    }
   return IFX_SUCCESS;
}


static unsigned char my_ethaddr[MAX_ADDR_LEN];
/* need to get the ether addr from u-boot */
static int __init ethaddr_setup(char *line)
{
	char *ep;
	int i;

	memset(my_ethaddr, 0, MAX_ADDR_LEN);
	/* there should really be routines to do this stuff */
	for (i = 0; i < 6; i++)
	{
		my_ethaddr[i] = line ? simple_strtoul(line, &ep, 16) : 0;
		if (line)
			line = (*ep) ? ep+1 : ep;
	}
	DANUBE_SW_DMSG("mac address %2x-%2x-%2x-%2x-%2x-%2x \n"
		,my_ethaddr[0]
		,my_ethaddr[1]
		,my_ethaddr[2]
		,my_ethaddr[3]
		,my_ethaddr[4]
		,my_ethaddr[5]);
	return 0;
}
__setup("ethaddr=", ethaddr_setup);

/* Brief: open RX DMA  channels
 * Parameter: net_device
 */
void open_rx_dma(struct net_device *dev)
{
    struct switch_priv* priv=(struct switch_priv*)dev->priv;
    struct dma_device_info* dma_dev=priv->dma_device;
    int i;

    for(i=0;i<dma_dev->max_rx_chan_num;i++)
    	{
	  if((dma_dev->rx_chan[i])->control==DANUBE_DMA_CH_ON)
	  (dma_dev->rx_chan[i])->open(dma_dev->rx_chan[i]);
	}

}

/* Brief: close RX DMA  channels
 * Parameter: net_device
 */
void close_rx_dma(struct net_device *dev)
{

    struct switch_priv* priv=(struct switch_priv*)dev->priv;
    struct dma_device_info* dma_dev=priv->dma_device;
    int i;

    for(i=0;i<dma_dev->max_rx_chan_num;i++)
    	dma_dev->rx_chan[i]->close(dma_dev->rx_chan[i]);


}

#ifdef CONFIG_NET_HW_FLOWCONTROL
/* Brief: Enable reciving DMA channel */
void danube_xon(struct net_device *dev)
{
    unsigned long flag;
    local_irq_save(flag);
    TRACE("wakeup\n");
    open_rx_dma(dev);
    local_irq_restore(flag);
}
#endif //CONFIG_NET_HW_FLOWCONTROL



int danube_switch_open(struct net_device *dev)
{
    struct switch_priv* priv=(struct switch_priv*)dev->priv;

    MOD_INC_USE_COUNT;
#if 0
    if(!probe_transceiver(dev))
      {
       TRACE("%s cannot work because of hardware problem\n",dev->name);
       MOD_DEC_USE_COUNT;
       return -1;
      }
    TRACE("%s\n",dev->name);
#endif
    open_rx_dma(dev); //TX is opened once there is an outgoing packet

#ifdef CONFIG_NET_HW_FLOWCONTROL
    if ( (priv->fc_bit = netdev_register_fc(dev, danube_xon)) == 0){
    	TRACE("Hardware Flow Control register fails\n");
    }
#endif //CONFIG_NET_HW_FLOWCONTROL
    netif_start_queue(dev);

    return OK;
}


int switch_release(struct net_device *dev)
{

   struct switch_priv* priv=(struct switch_priv*)dev->priv;
   struct dma_device_info* dma_dev=priv->dma_device;

   //dma_device_release(dma_dev);
    close_rx_dma(dev);

#ifdef CONFIG_NET_HW_FLOWCONTROL
   if (priv->fc_bit){
   	 netdev_unregister_fc(priv->fc_bit);
   }
#endif //CONFIG_NET_HW_FLOWCONTROL
   netif_stop_queue(dev);
   MOD_DEC_USE_COUNT;

    return OK;
}

int switch_proc_read(char *buf, char **start, off_t offset,
                         int count, int *eof, void *data)
{
   *eof=1;
   return (int)offset+4;
}



int switch_rx(struct net_device *dev, int len,struct sk_buff* skb)
{
    int ret=0;
    struct switch_priv *priv = (struct switch_priv *) dev->priv;
#ifdef CONFIG_NET_HW_FLOWCONTROL
    int mit_sel=0;
#endif

//060620:henryhsu modify for vlan
#if defined(CONFIG_IFX_NFEXT_SWITCH_PHYPORT) || defined(CONFIG_IFX_NFEXT_SWITCH_PHYPORT_MODULE)
    if (((dev-switch_devs) == 0) && amazon_sw_phyport_rx)
    {

	if (amazon_sw_phyport_rx(dev, skb) < 0)
        {   priv->stats.rx_errors++;
            return;
        }
    }
    else
#endif
    {
        skb->dev = dev;
#if !defined(ENABLE_DIRECT_BRIDGE) || !ENABLE_DIRECT_BRIDGE
        skb->protocol = eth_type_trans(skb, dev);
#endif
    }

#ifdef CONFIG_NET_HW_FLOWCONTROL
  #if defined(ENABLE_DIRECT_BRIDGE) && ENABLE_DIRECT_BRIDGE
    switch_eth2_dev->hard_start_xmit(skb, switch_eth2_dev);
    mit_sel = NET_RX_SUCCESS;
  #else
    mit_sel = netif_rx(skb);
  #endif
    switch(mit_sel){
    	case NET_RX_SUCCESS:
        case NET_RX_CN_LOW:
        case NET_RX_CN_MOD:
        	ret=len;
		break;
        case NET_RX_CN_HIGH:
		break;
	case NET_RX_DROP:
		if ( (priv->fc_bit) && ( ! test_and_set_bit(priv->fc_bit, &netdev_fc_xoff))){
			close_rx_dma(dev);
			TRACE("RX STOPPED!\n");
			rx_stopped=1;

		}
		ret=0;
		break;
    }
#else
  #if defined(ENABLE_DIRECT_BRIDGE) && ENABLE_DIRECT_BRIDGE
    switch_eth2_dev->hard_start_xmit(skb, switch_eth2_dev);
  #else
    netif_rx(skb);
#endif
#endif

    priv->stats.rx_packets++;
    priv->stats.rx_bytes+=len;
    //printk("packet %d\n",priv->stats.rx_packets);
    return ret;
}


int switch_hw_receive(struct net_device* dev,struct dma_device_info* dma_dev)
{
    u8* buf=NULL;
    int len=0;
    //void* opt=NULL;
    struct sk_buff *skb=NULL;
    struct switch_priv *priv = (struct switch_priv *) dev->priv;

    len=dma_device_read(dma_dev,&buf,(void**)&skb);

    //TRACE("DMA rx len:%d\n",len);

    if (len >= 0x600){
 	TRACE("packet too large %d\n",len);
	goto switch_hw_receive_err_exit;
    }

    /* remove CRC */
    len -= 4;
    if (skb == NULL ){
    	TRACE("cannot restore pointer\n");
    	goto switch_hw_receive_err_exit;
    }
    if (len > (skb->end -skb->tail)){
    	TRACE("BUG, len:%d end:%p tail:%p\n", (len+4), skb->end, skb->tail);
	goto switch_hw_receive_err_exit;
    }
    skb_put(skb,len);
    skb->dev = dev;
        if (buf){
#ifdef DANUBE_SW_DUMP
    	printk("rx:");
	dump_skb(len, (char *)buf);
#endif
    }

    len=switch_rx(dev,len,skb);
    return OK;
switch_hw_receive_err_exit:
    if (buf){
#ifdef DANUBE_SW_DUMP
    	dump_skb(len, (char *)buf);
#endif
    }
    if(len==0)
    {
    if (skb) dev_kfree_skb_any(skb);
    priv->stats.rx_errors++;
    priv->stats.rx_dropped++;
    return -EIO;
    }
    else
    return len;

}

int switch_hw_tx(char *buf, int len, struct net_device *dev)
{
    int ret=0;
    struct switch_priv *priv=dev->priv;
    struct dma_device_info* dma_dev=priv->dma_device;
    ret=dma_device_write(dma_dev,buf,len, priv->skb);
#if 0
    if(rx_stopped)
    {
      danube_xon(dma_dev);
      rx_stopped=0;
    }
#endif
    return ret;
}


int select_tx_chan(struct sk_buff *skb, struct net_device *dev)
{
     int chan=0;
/*TODO: select the channel based on some criteria*/

     return chan;
}



int switch_tx(struct sk_buff *skb, struct net_device *dev)
{

    int len;
    char *data;
    struct switch_priv *priv=dev->priv;
    struct dma_device_info* dma_dev=priv->dma_device;
    len = skb->len < ETH_ZLEN ? ETH_ZLEN : skb->len;
    data = skb->data;
    priv->skb = skb;
    dev->trans_start = jiffies;
    dma_dev->current_tx_chan=select_tx_chan(skb, dev);/*select the tx channel*/
    //printk("switch tx\n");
    g_transmit++;
    if ( switch_hw_tx(data, len, dev)!= len){
        dev_kfree_skb_any(skb);

        priv->stats.tx_errors++;
	priv->stats.tx_dropped++;

    }
    else{
      priv->stats.tx_packets++;
      priv->stats.tx_bytes+=len;
     }
    return OK;
}




void switch_tx_timeout (struct net_device *dev)
{
    int i;
    struct switch_priv* priv=(struct switch_priv*)dev->priv;

    TRACE("%s tx_status==%d\n",dev->name,priv->tx_status);
    //TODO:must restart the TX channels
    priv->stats.tx_errors++;
    for(i=0;i<priv->dma_device->max_tx_chan_num;i++)
          {
            priv->dma_device->tx_chan[i]->disable_irq(priv->dma_device->tx_chan[i]);
          }
    netif_wake_queue(dev);
    return;
}




int dma_intr_handler(struct dma_device_info* dma_dev,int status)
{
     struct net_device* dev;
     int i;
//  struct switch_priv* priv;

     dev=switch_devs + (u32) dma_dev->priv;
     //TRACE("status:%d \n", status);
     switch(status)
     {
       case RCV_INT:
          //TRACE("switch receive chan=%d\n",dma_dev->current_rx_chan);
          switch_hw_receive(dev,dma_dev);
          break;
       case TX_BUF_FULL_INT:
          TRACE("tx buffer full\n");
          netif_stop_queue(dev);
	  for(i=0;i<dma_dev->max_tx_chan_num;i++)
          {
             if((dma_dev->tx_chan[i])->control==DANUBE_DMA_CH_ON)
         	    dma_dev->tx_chan[i]->enable_irq(dma_dev->tx_chan[i]);
          }
          break;
       case TRANSMIT_CPT_INT:
          TRACE("tx buffer released\n");
	  for(i=0;i<dma_dev->max_tx_chan_num;i++)
          {
            dma_dev->tx_chan[i]->disable_irq(dma_dev->tx_chan[i]);
          }
          netif_wake_queue(dev);
          break;
     }
     return OK;
}

/* reserve 2 bytes in front of data pointer*/
u8* etop_dma_buffer_alloc(int len, int* byte_offset,void** opt)
{
    u8* buffer=NULL;
    struct sk_buff *skb=NULL;
    skb = dev_alloc_skb(ETHERNET_PACKET_DMA_BUFFER_SIZE);
    if (skb == NULL) {
    	return NULL;
    }
    buffer=(u8*)(skb->data);
    skb_reserve(skb, 2);
    //TRACE("%8p\n",skb->data);
    *(int*)opt=(int)skb;
    *byte_offset=2;
    return buffer;
}

int etop_dma_buffer_free(u8* dataptr,void* opt)
{
   struct sk_buff *skb=NULL;
   if(opt==NULL){
   	kfree(dataptr);
   }else {
        skb=(struct sk_buff*)opt;

	dev_kfree_skb_any(skb);


   }
   return OK;
}


static int set_mac(struct net_device *dev,u16 speed,u8 duplex,u8 autoneg)
{
   int ret;
   int dev_num=dev-switch_devs;
   u32 enet_offset=dev_num*(0x40<<2);
   if(autoneg==AUTONEG_ENABLE)
   {
      /*set property and start autonegotiation*/
      /*have to set mdio advertisement register and restart autonegotiation*/
      /*which is a very rare case, put it to future development if necessary.*/
      ret=IFX_SUCCESS;
   }
   else /*autoneg==AUTONEG_DISABLE or -1*/
   {
     /*set property without autonegotiation*/
     if(dev_num==0)
     {
       *ETOP_MDIO_CFG&=~UMM0_MASK;
     }
     else
     {
       *ETOP_MDIO_CFG&=~UMM0_MASK;
     }
       /*set speed*/
       if(speed==SPEED_10)
       *(ENET_MAC_CFG+enet_offset)&=~SPEED_MASK;
       else if(speed==SPEED_100)
       *(ENET_MAC_CFG+enet_offset)|=SPEED_MASK;

       /*set duplex*/
       if(duplex==DUPLEX_HALF)
       *(ENET_MAC_CFG+enet_offset)&=~DUPLEX_MASK;
       else if(duplex)
       *(ENET_MAC_CFG+enet_offset)|=DUPLEX_MASK;

       *(ENET_MAC_CFG+enet_offset)|=LINK_MASK;
       ret=IFX_SUCCESS;
   }
   return ret;
}


static int
switch_ethtool_ioctl(struct net_device *dev, struct ifreq *ifr)
{
        int ret=IFX_SUCCESS;
	struct switch_priv* priv=(struct switch_priv*)dev->priv;

	struct ethtool_cmd ecmd;
        /*calculate the enet offset*/
	u32 enet_offset;
	if(dev-switch_devs==0) enet_offset=0;
	else enet_offset=0x40<<2;

	if (copy_from_user(&ecmd, ifr->ifr_data, sizeof (ecmd)))
		return -EFAULT;

	switch (ecmd.cmd) {
		case ETHTOOL_GSET:/*get hardware information*/
		{
			memset((void *) &ecmd, 0, sizeof (ecmd));
			ecmd.supported =
			  SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII |
			  SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
			  SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full;
			ecmd.port = PORT_MII;
			ecmd.transceiver = XCVR_EXTERNAL;
			ecmd.phy_address = priv->mdio_phy_addr;
			if(*(ENET_MAC_CFG+enet_offset)&SPEED_MASK)
			{
			   priv->current_speed=SPEED_100;

			}
			else
			{
			   priv->current_speed=SPEED_10;

			}
			ecmd.speed =priv->current_speed;

			if(*(ENET_MAC_CFG+enet_offset)&DUPLEX_MASK)
			{
			   priv->full_duplex=DUPLEX_FULL;

			}
			else
			{
			   priv->full_duplex=DUPLEX_HALF;

			}
			ecmd.duplex = priv->full_duplex;

			if(enet_offset==0)
			{
			   if(*ETOP_MDIO_CFG&UMM0_MASK)
			   {
			    ecmd.autoneg = AUTONEG_ENABLE;
			    ecmd.advertising |=
				  ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
				  ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full;
			   }
			   else
			   {
			      ecmd.autoneg = AUTONEG_DISABLE;
                              ecmd.advertising &=
				  ~(ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
				  ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full);
			   }
			}
			else
			{
			   if(*ETOP_MDIO_CFG&UMM1_MASK)
			   {
			    ecmd.autoneg = AUTONEG_ENABLE;
			    ecmd.advertising |=
				  ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
				  ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full;
			   }
			   else
			   {
			     ecmd.autoneg = AUTONEG_DISABLE;
			     ecmd.advertising &=
				  ~(ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
				  ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full);
			   }
			}


			if (copy_to_user(ifr->ifr_data, &ecmd, sizeof (ecmd)))
				return -EFAULT;
			ret=IFX_SUCCESS;

		}

		break;

		case ETHTOOL_SSET:/*force the speed and duplex mode*/
		{
			if (!capable(CAP_NET_ADMIN)) {
				return -EPERM;
			}
			ret=set_mac(dev,ecmd.speed,ecmd.duplex,ecmd.autoneg);
		}
		break;

		case ETHTOOL_GDRVINFO:/*get driver information*/
		{
			struct ethtool_drvinfo info;
			memset((void *) &info, 0, sizeof (info));
			strncpy(info.driver, "DANUBE ETOP DRIVER", sizeof(info.driver) - 1);
			strncpy(info.fw_version, "0.0.1", sizeof(info.fw_version) - 1);
			strncpy(info.bus_info, "N/A", sizeof(info.bus_info) - 1);
			info.regdump_len = 0;
			info.eedump_len = 0;
			info.testinfo_len = 0;
			if (copy_to_user(ifr->ifr_data, &info, sizeof (info)))
				return -EFAULT;
			ret=IFX_SUCCESS;
		}
		break;
		case ETHTOOL_NWAY_RST:/*restart auto negotiation*/
			if(enet_offset==0)
			*ETOP_MDIO_CFG|=UMM0_MASK|SMRST_MASK;
			else
			*ETOP_MDIO_CFG|=UMM1_MASK|SMRST_MASK;
			ret=IFX_SUCCESS;
		break;
		default:
			return -EOPNOTSUPP;
		break;

	}


	return ret;
}

int set_vlan_cos(_vlan_cos_req* vlan_cos_req)
{
     u32 pri=vlan_cos_req->pri;
     u32 cos_value=vlan_cos_req->cos_value;
     u32 value;
     value=*ETOP_IG_VLAN_COS&~(3<<(pri*2));
     *ETOP_IG_VLAN_COS=value|(cos_value<<(pri*2));
     return IFX_SUCCESS;/*cannot fail*/
}


int set_dscp_cos(_dscp_cos_req* dscp_cos_req)
{
    u32 dscp=dscp_cos_req->dscp;
    u32 cos_value=dscp_cos_req->cos_value;
    u32 value;
    value=*(ETOP_IG_DSCP_COS0-(dscp/16*4))&~(3<<dscp%16*2);
    *(ETOP_IG_DSCP_COS0-(dscp/16*4))|=cos_value<<dscp%16*2;
    return IFX_SUCCESS;
}


int switch_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{

     int result=IFX_SUCCESS;
     int enet_offset=(dev-switch_devs)*(0x40<<2);
     _vlan_cos_req etop_vlan_cos_req;
     _dscp_cos_req etop_dscp_cos_req;
     switch(cmd){
                case SIOCETHTOOL:
			switch_ethtool_ioctl(dev,ifr);
                        break;
		case SET_VLAN_COS:
		        copy_from_user(&etop_vlan_cos_req,\
			 (_vlan_cos_req*)ifr->ifr_data,sizeof(_vlan_cos_req));
			set_vlan_cos(&etop_vlan_cos_req);
			break;

		case SET_DSCP_COS:
		        copy_from_user(&etop_dscp_cos_req,\
			 (_vlan_cos_req*)ifr->ifr_data,sizeof(_dscp_cos_req));
			set_dscp_cos(&etop_dscp_cos_req);
			break;
		case ENABLE_VLAN_CLASSIFICATION:
		     *(ENETS_COS_CFG)|=VLAN_MASK;
		case DISABLE_VLAN_CLASSIFICATION:
	             *(ENETS_COS_CFG)&=~VLAN_MASK;
		case ENABLE_DSCP_CLASSIFICATION:
		     *(ENETS_COS_CFG)|=DSCP_MASK;
		case DISABLE_DSCP_CLASSIFICATION:
		     *(ENETS_COS_CFG)&=~DSCP_MASK;
		case VLAN_CLASS_FIRST:
		     *(ENETS_CFG+enet_offset)&=~FTUC_MASK;
		case VLAN_CLASS_SECOND:
		     *(ENETS_CFG+enet_offset)|=VL2_MASK;
		case PASS_UNICAST_PACKETS:
		     *(ENETS_CFG+enet_offset)&=~FTUC_MASK;
		case FILTER_UNICAST_PACKETS:
		     *(ENETS_CFG+enet_offset)|=FTUC_MASK;
		case KEEP_BROADCAST_PACKETS:
		     *(ENETS_CFG+enet_offset)&=~DPBC_MASK;
		case DROP_BROADCAST_PACKETS:
		     *(ENETS_CFG+enet_offset)|=DPBC_MASK;
		case KEEP_MULTICAST_PACKETS:
		     *(ENETS_CFG+enet_offset)&=~DPMC_MASK;
		case DROP_MULTICAST_PACKETS:
		     *(ENETS_CFG+enet_offset)|=DPMC_MASK;
		}

          return result;
}



static struct net_device_stats *danube_get_stats(struct net_device *dev)
{
	return (struct net_device_stats *)dev->priv;
}

int etop_register_proc_read(char *buf, char **start, off_t offset,
                         int count, int *eof, void *data)
{
   int len=0;;

   len+=sprintf(buf+len,"etop register\n");
   len+=sprintf(buf+len,"%d packets from protocol stack received!\n",g_transmit);
   /*TODO: add the register value here, if the user does not want to use the mem application*/

   return len;
}


int etop_dscp_cos_proc_read(char *buf, char **start, off_t offset,
                         int count, int *eof, void *data)
{
   int len=0;;
   int i;
   int j;
   len+=sprintf(buf+len,"DSCP COS MAP:\n");
   for(j=0;j<4;j++)
   {
     for(i=0;i<16;i++)
     {
      len+=sprintf(buf+len,"DSCP%d %d\n", i+j*16, (u32)((*(ETOP_IG_DSCP_COS0-j*4)>>(i*2))&0x3));
     }
   }
   return len;
}

int etop_vlan_cos_proc_read(char *buf, char **start, off_t offset,
                         int count, int *eof, void *data)
{
   int len=0;;
   int i;
   len+=sprintf(buf+len,"VLAN COS MAP\n");
   for(i=0;i<8;i++)
   {
     len+=sprintf(buf+len,"Pri %d %d\n", i, (u32)((*ETOP_IG_VLAN_COS>>(i*2))&0x3));
   }
   return len;
}


struct net_device_stats *switch_stats(struct net_device *dev)
{
    struct switch_priv *priv = (struct switch_priv *) dev->priv;

    return &priv->stats;
}



static int switch_init(struct net_device *dev)
{
    int result;
    u64 retval=0;
    int i;

    struct switch_priv *priv;
    ether_setup(dev); /* assign some of the fields */

    TRACE("%s up\n",dev->name);

    dev->open            = danube_switch_open;
    dev->stop            = switch_release;
    dev->hard_start_xmit = switch_tx;
    dev->do_ioctl        = switch_ioctl;
    dev->get_stats       = danube_get_stats;
    //dev->change_mtu      = switch_change_mtu;
    //dev->set_mac_address =switch_set_mac_address;
    dev->tx_timeout      =switch_tx_timeout;
    dev->watchdog_timeo  =timeout;
    //dev->flags           |= IFF_NOARP|IFF_PROMISC;



    dev->priv = kmalloc(sizeof(struct switch_priv), GFP_KERNEL);
    if (dev->priv == NULL)
        return -ENOMEM;
    memset(dev->priv, 0, sizeof(struct switch_priv));
    priv=dev->priv;

    /*initialize the dma device*/
    priv->dma_device=dma_device_reserve("PPE");
    if(!priv->dma_device) return IFX_ERROR;
    priv->dma_device->buffer_alloc=&etop_dma_buffer_alloc;
    priv->dma_device->buffer_free=&etop_dma_buffer_free;
    priv->dma_device->intr_handler=&dma_intr_handler;
#if defined(ENABLE_RX_DPLUS_PATH) && ENABLE_RX_DPLUS_PATH
    priv->dma_device->max_rx_chan_num=2;/*turn on all the receive channels*/
#else
    priv->dma_device->max_rx_chan_num=4;/*turn on all the receive channels*/
#endif

    for(i=0;i<priv->dma_device->max_rx_chan_num;i++)
    {
      priv->dma_device->rx_chan[i]->packet_size=ETHERNET_PACKET_DMA_BUFFER_SIZE;
      priv->dma_device->rx_chan[i]->control=DANUBE_DMA_CH_ON;
    }

    for(i=0;i<priv->dma_device->max_tx_chan_num;i++)
    {
      if(i==0) priv->dma_device->tx_chan[i]->control=DANUBE_DMA_CH_ON;
      else  priv->dma_device->tx_chan[i]->control=DANUBE_DMA_CH_OFF;
    }

    result=dma_device_register(priv->dma_device);

	/*read the mac address from the mac table and put them into the mac table.*/
  	for (i = 0; i < 6; i++){
		retval +=my_ethaddr[i];
	}
	/* ethaddr not set in u-boot ? */
	if (retval == 0){
		TRACE("use default MAC address\n");
		dev->dev_addr[0] = 0x00;
		dev->dev_addr[1] = 0x20;
		dev->dev_addr[2] = 0xda;
		dev->dev_addr[3] = 0x86;
		dev->dev_addr[4] = 0x23;
		dev->dev_addr[5] = 0x74 + (unsigned char)(dev-switch_devs);
	}else{
		for (i = 0; i < 6; i++){
			dev->dev_addr[i] = my_ethaddr[i];
		}
		dev->dev_addr[5] += + (unsigned char)(dev-switch_devs);
	}
#ifdef AMAZON_SW_DMSG_MSG
	retval = 0;
	for (i = 0; i < 6; i++){
		retval = (retval<<8) + dev->dev_addr[i];
		printk(" %2x ", dev->dev_addr[i]);
	}
	//add_mac_table_entry(retval);
#endif

    return 0;
}


static int danube_sw_chip_init(int mode)
{
   // enable PPE
/*
	*DANUBE_PMU_PWDCR &=~((1<<DANUBE_PMU_ETOP_SHIFT)|(1<<DANUBE_PMU_PPE_SHIFT)|\
	                      (1<<DANUBE_PMU_ENET0_SHIFT)|(1<<DANUBE_PMU_ENET1_SHIFT));
*/
     *DANUBE_PMU_PWDCR = *DANUBE_PMU_PWDCR & 0xFFFFEFDF;
    // turn on port0, set to rmii and turn off port1.
	if(mode==REV_MII_MODE)
		{
		 *DANUBE_PPE32_ETOP_CFG = (*DANUBE_PPE32_ETOP_CFG & 0xfffffffc) | 0x0000000a;
                }
	else if (mode == MII_MODE)
	        {
		*DANUBE_PPE32_ETOP_CFG = (*DANUBE_PPE32_ETOP_CFG & 0xfffffffc) | 0x00000008;
		//*DANUBE_PPE32_ETOP_MDIO_CFG |=(*DANUBE_PPE32_ETOP_MDIO_CFG & ~(PHYA0_MASK|UMM0_MASK|SMRST_MASK))|1<<3|1<<1|1<<13;
			*DANUBE_PPE32_ETOP_MDIO_CFG = 0x20e;
                }
	*DANUBE_PPE32_ETOP_IG_PLEN_CTRL = 0x4005ee; // set packetlen.
        *ENET_MAC_CFG|=1<<11;/*enable the crc*/

}
/* Initialize the rest of the LOOPBACK device. */
int __init switch_init_module(void)
{

        int i=0,result,device_present = 0;
        struct net_device *dev;
        g_danube_etop_dir=proc_mkdir("danube_etop",NULL);

#if defined(ENABLE_DIRECT_BRIDGE) && ENABLE_DIRECT_BRIDGE
    eth2_switch_dev = &switch_devs[0];
#endif

	create_proc_read_entry("register",
                            0,
                            g_danube_etop_dir,
                            etop_register_proc_read,
                            NULL);
        create_proc_read_entry("DSCP_COS",
                            0,
                            g_danube_etop_dir,
                            etop_dscp_cos_proc_read,
                            NULL);
        create_proc_read_entry("VLAN_COS",
                            0,
                            g_danube_etop_dir,
                            etop_vlan_cos_proc_read,
                            NULL);

	/*
	 *	Fill in the generic fields of the device structure.
	 */

        for(i=0;i<DANUBE_SW_INT_NO;i++)
        {
	 dev=switch_devs+i;
	 strcpy(dev->name,"eth%d");
         SET_MODULE_OWNER(dev);
	 if ( (result = register_netdev(dev)) )
          TRACE("error %i registering device \"%s\"\n",
                   result, dev->name);
          else device_present++;
        }
        printk("danube MAC driver loaded!\n");
//#if defined(CONFIG_SWITCH_ADM6996I)  
        danube_sw_chip_init(REV_MII_MODE);
//#else
// 				danube_sw_chip_init(MII_MODE);
//#endif        
	return(0);
};

static void __exit switch_cleanup(void)
{
    int i;

    struct switch_priv *priv;
    for(i=0;i<DANUBE_SW_INT_NO;i++)
    {

     priv=switch_devs[i].priv;
     if(priv->dma_device)
      {
       dma_device_unregister(priv->dma_device);
       dma_device_release(priv->dma_device);
       kfree(priv->dma_device);
       TRACE("dma_device null now!\n");
      }
     kfree(switch_devs[i].priv);

     unregister_netdev(switch_devs+i);
    }

    return;
}

module_init(switch_init_module);
module_exit(switch_cleanup);
