/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
//-----------------------------------------------------------------------
//Description:	
// 	Since the IKOS emulation is extremly slow, we can't wait until the 
// * whole system is up and then verify our drivers. Thus we decide to hack
// * the kernel so that it is traped in this function to do verification.
// * We borrow some parts from U-boot.
// * Test Cases see below

//-----------------------------------------------------------------------
//Author:	peng.liu@infineon.com
//Created:	12-April-2004
//-----------------------------------------------------------------------
/* History
 * Last changed on:
 * Last changed by: 
 */

#include <linux/config.h>

#ifdef IKOS_MINI_BOOT
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/module.h>

#include <linux/interrupt.h>
#include <asm/time.h>

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/danube/danube.h>
#include <asm/danube/irq.h>

#include <linux/in.h>
#include <linux/netdevice.h> 
#include <linux/etherdevice.h> 
#include <linux/ip.h>
#include <linux/tcp.h> 
#include <linux/skbuff.h>
#include <linux/in6.h>
#include <linux/atmdev.h>


#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/danube/danube_tpe.h>
#include <asm/danube/ifx_ssc.h>
#include <asm/danube/ifx_ssc_defines.h>
#include <asm/danube/danube_dma.h>
#include <asm/danube/danube_sw.h>
#include <asm/danube/danube_wdt.h>
#include <asm/danube/danube_mei.h>
#include <asm/danube/atm_defines.h>


#define CMD_MAX 64
#define CFG_MAXARGS 6

#define DANUBE_IKOS_EMSG(fmt,args...) printk("%s:" fmt, __FUNCTION__, ##args)

#define LB_MODE_DFE			0	/* loopback at DFE Aware */
#define LB_MODE_TPE			1	/* loopback at TPE */


#define SWITCH_PORT_LOOPBACK		0
#define SWITCH_TPE_LOOPBACK		1
#define SWITCH_TPE_AWARE_LOOPBACK	2
/******************* global data ************************************************/
static struct atm_vcc g_vcc[15];
static u32 g_swin_buffer[ATM_AAL0_SDU/4+1];
static volatile u32 g_tpe_pkt_cnt=0;
static volatile u32 g_sw_pkt_cnt=0;
static struct sk_buff *g_send_skb;
static struct sk_buff * g_sw_send_skb;
static u8 loopback_mode = SWITCH_PORT_LOOPBACK;

/*********  external function declaration ********************/
//kgdb_serial.c
extern char getDebugChar(void);
extern int putDebugChar(char c);
extern int serial_tstc (void);
//in danube_sw.c
extern struct net_device switch_devs[2];
extern int switch_init_module(void);
extern void  switch_cleanup(void);
extern int switch_init(struct net_device *dev);
extern int switch_open(struct net_device *dev);
extern int switch_tx(struct sk_buff *skb, struct net_device *dev);
extern int switch_release(struct net_device *dev);

#ifdef CONFIG_DANUBE_EEPROM
//ifx_ssc.c
extern int ifx_ssc_init(void);
extern int ifx_ssc_open(struct inode *inode, struct file * filp);
extern int ifx_ssc_close(struct inode *inode, struct file *filp);
extern void ifx_ssc_cleanup_module(void);
extern int ifx_ssc_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,unsigned long data);
extern ssize_t ifx_ssc_kwrite(int port, const char *kbuf, size_t len);
extern ssize_t ifx_ssc_kread(int port, char *kbuf, size_t len);

extern int danube_ssc_cs_low(u32 pin);
extern int danube_ssc_cs_high(u32 pin);
extern int danube_ssc_txrx(char * tx_buf, u32 tx_len, char * rx_buf, u32 rx_len);
extern int danube_ssc_tx(char * tx_buf, u32 tx_len);
extern int danube_ssc_rx(char * rx_buf, u32 rx_len); 

//danube_eeprom.c
extern ssize_t danube_eeprom_kread(char *buf, size_t len, u32 addr);
extern ssize_t danube_eeprom_kwrite(char *buf, size_t len, u32 addr);
#endif //CONFIG_DANUBE_EEPROM

//danube_tpe.c
extern int danube_atm_swin(u8 queue, void* cell);
extern u8* danube_atm_alloc_rx(int len, int* offset, void **opt);
extern int danube_atm_dma_tx(u8 vpi, u16 vci, u8 clp, u8 qid, struct sk_buff *skb);
extern struct sk_buff *danube_atm_alloc_tx(struct atm_vcc *vcc,unsigned int size);
extern void	danube_atm_cleanup(void);
extern int dma_may_send(int ch);
extern struct sk_buff * danube_atm_alloc_buffer(int len);
extern int set_htu_entry(u8 vpi, u8 vci, u8 qid, u8 idx);
#ifdef CONFIG_PCI
//pci.
extern int pcibios_init(void);
#endif 

#ifdef CONFIG_DANUBE_MEI
//danube_mei.c
extern MEI_ERROR meiForceRebootAdslModem(void);
extern u16 *Recent_indicator;
extern void danube_mei_cleanup_module(void);
extern int danube_mei_init_module(void);
extern void meiLongwordWrite(u32 ul_address, u32 ul_data);
extern void meiLongwordRead(u32 ul_address, u32 *pul_data);
#endif

#ifdef CONFIG_DANUBE_WDT
//danube_wdt.c
extern int danube_wdt_init_module(void);
extern void danube_wdt_cleanup_module(void);
extern int wdt_enable(int timeout);
extern int wdt_disable(void);
#endif //CONFIG_DANUBE_WDT

/*****  command type ****/
struct cmd_tbl_s{
	char * name; 					/* command name		*/
	int (*cmd)(struct cmd_tbl_s *, int, char *[]); 	/* command functionn	*/
	char * usage;					/* Usage message(short)	*/
	int	len;					/* minimum len		*/
};
typedef struct cmd_tbl_s cmd_tbl_t;

/*********  internal function declaration ********************/
static int do_help    (cmd_tbl_t *cmdtp, int argc, char *argv[]);
static int do_time( cmd_tbl_t *cmdtp, int argc, char *argv[]);
static int do_tpe_swie    (cmd_tbl_t *cmdtp, int argc, char *argv[]);
static int do_tpe_dma    (cmd_tbl_t *cmdtp, int argc, char *argv[]);
static int do_tpe_test   (cmd_tbl_t *cmdtp, int argc, char *argv[]);
static int do_sw_lp( cmd_tbl_t * cmdtp, int argc, char *argv[]);
static int do_fast_sw_lp( cmd_tbl_t * cmdtp, int argc, char *argv[]);
static int do_sw_tpe_lp ( cmd_tbl_t *cmdtp, int argc, char *argv[]);
#ifdef CONFIG_DANUBE_EEPROM
static int do_eeprom ( cmd_tbl_t *cmdtp, int argc, char *argv[]);
#endif
#ifdef CONFIG_PCI
static int do_pci ( cmd_tbl_t *cmdtp, int argc, char *argv[]);
#endif 
#ifdef CONFIG_DANUBE_WDT
static int do_wdt ( cmd_tbl_t *cmdtp, int argc, char *argv[]);
#endif
#ifdef CONFIG_DANUBE_MEI
static int do_mei ( cmd_tbl_t *cmdtp, int argc, char *argv[]);
static int do_tpe_oam    (cmd_tbl_t *cmdtp, int argc, char *argv[]);
static int do_tpe0_dfe    (cmd_tbl_t *cmdtp, int argc, char *argv[]);
static int do_tpe5_dfe    (cmd_tbl_t *cmdtp, int argc, char *argv[]);
#endif

static cmd_tbl_t cmd_tbl []={
#ifdef CONFIG_DANUBE_MEI
	{"tpe_oam",do_tpe_oam,"tpe_oam [vpi vci pti]: OAM cells loopback test", 5},
	{"tpe0_dfe",do_tpe0_dfe,"tpe0_dfe:AAL0 cells loopbak through AWARE DFE", 5},
	{"tpe5_dfe",do_tpe5_dfe,"tpe5_dfe:AAL5 cells loopbak through AWARE DFE", 5},
	{"tpe_test",do_tpe_test,"tpe_test type_of_error", 5},
#endif
#ifdef CONFIG_DANUBE_WDT
	{"wdt",do_wdt,"wdt [timeout]: Watch Dog Timer test", 3},
#endif
#ifdef CONFIG_PCI
	{"pci",do_pci,"pci: scan PCI bus", 3},
#endif
#ifdef CONFIG_DANUBE_EEPROM	
	{"eeprom",do_eeprom,"eeprom [start_addr] [test_size]: read/write Serial EEPROM ", 3},
#endif
	{"sw_tpe_lp",do_sw_tpe_lp,"sw_tpe_lp: loopback through switch, TPE and Aware DFE", 6},
	{"sw_lp",do_sw_lp,"sw_lp: switch loopback", 5},
	{"fast_sw_lp",do_fast_sw_lp,"fast_sw_lp: switch loopback not using driver", 5},
	{"tpe_swie",do_tpe_swie,"tpe_swie: AAL0 cells loopback through TPE", 5},
	{"tpe_dma",do_tpe_dma,"tpe_dma [LOOPBACK_OPTION PACKET_LEN PACKET_OFFSET PACKET_NUMBER]", 5},
	{"time", do_time,"time: print jiffies value",1},
	{"help",do_help, "h[elp]: this list", 1},
	{NULL, do_help, NULL,0},
};

/*********************  helpers **********************************************/
static inline void dump_data(u8 * data, int len)
{
	int i;
	for(i=0;i<len;i++){	
		printk("%2x ",data[i]);
		if (i % 16 == 15)
			printk("\n");
	}
	printk("\n");
}

#define IKOS_DUMP_SKB
/*
 * Brief:	dump skb data
 */
static inline void dump_skb(struct sk_buff * skb)
{
#ifdef IKOS_DUMP_SKB
	dump_data(skb->data, skb->len);
#endif
}
/*
 * Brief:	check skb data
 * Return:
 *		0
 *		-1 fails
 */
static inline int check_skb(struct atm_vcc *vcc, struct sk_buff *skb)
{
	int i;
	u8 * data;
	data = (u8 *) (skb->data);
	if (data[0]  != (u8) vcc->vpi) return -1;
	if (data[1]  != (u8) vcc->vci) return -1;
	for(i=2;i<skb->len;i++){
		if (data[i] != (u8) ((i-2)%0x100)) {
			return -1;
		}
	}
	return 0;
}

/*
 * Brief:	fill in skb data
 */
static inline void fill_skb(struct atm_vcc *vcc, struct sk_buff *skb, u32 length)
{
	int i;
	u8 * data;
	data = (u8 *) skb_put(skb, length);
	data[0]  = ((u8) vcc->vpi);
	data[1]  = ((u8) vcc->vci);
	for(i=2;i<skb->len;i++){
		data[i] = ((u8) ((i-2)%0x100));
	}
}

/*	Brief:	call back function for AAL5 loopback
 */
int tpe_aal5_lp(struct atm_vcc *vcc,struct sk_buff *skb, int err)
{
	g_tpe_pkt_cnt ++;
	if (err){
		return 0;
	}
	if (vcc == NULL || skb == NULL){
		DANUBE_IKOS_EMSG("invalid parameter \n");
		return 0;
	}
	
	if (check_skb(vcc,skb) !=0 ){
		dump_skb(skb);
	}

	dev_kfree_skb(skb);
	return 0;
}
/*	Brief:	call back function for AAL0 loopback
 */
int tpe_aal0_lp(struct atm_vcc *vcc,struct sk_buff *skb, int err)
{
	g_tpe_pkt_cnt ++;
	if (err){
		return 0;
	}
	if (vcc == NULL || skb == NULL){
		DANUBE_IKOS_EMSG("invalid parameter \n");
		return 0;
	}	
	dump_skb(skb);
	dev_kfree_skb(skb);
	return 0;
}

/*	Brief:	call back function for Switch & TPE loopback
 */
int sw_tpe_lp(struct atm_vcc *vcc,struct sk_buff *skb, int err)
{
	g_tpe_pkt_cnt ++;
	if (err){
		return 0;
	}
	DANUBE_IKOS_DMSG("sw_tpe_lp: received\n");
	if (vcc == NULL || skb == NULL){
		DANUBE_IKOS_EMSG("invalid parameter \n");
		if (skb != NULL) dev_kfree_skb(skb);
		return -EINVAL;
	}
	switch_tx(skb, &switch_devs[vcc->vci - 0x33]);	
	return 0;
}

static u32 g_sw_drop=0;
/* Brief:
 *	supports
 *		SWITCH_PORT_LOOPBACK:	loop back directly to the same port
 *		SWITCH_TPE_LOOPBACK:	loop back via TPE to the same port
 *		SWITCH_TPE_AWARE_LOOPBACK: loop back via TPE and Aware to the same port
 */
void danube_sw_loopback(struct sk_buff* skb)
{
#if 0
    struct ethhdr *eth;
    unsigned char temp[6];
#endif    
    u8 qid=0;
    unsigned int wm;
    g_sw_pkt_cnt++;
    
    if (skb == NULL) return;
#if 0
    //swap src and des mac address
    eth=(struct ethhdr*)(skb->data);
    memcpy(temp,eth->h_source,6);
    memcpy(eth->h_source,eth->h_dest,6);
    memcpy(eth->h_dest,temp,6);
    
    DANUBE_IKOS_DMSG("danube_sw_loopback: received\n");
#endif
    
    if (loopback_mode == SWITCH_PORT_LOOPBACK){
	if((skb->dev-switch_devs)==0){
		if (switch_tx(skb, &switch_devs[1]) != 0 ){
			g_sw_drop++;
		}
	}else{
		if (switch_tx(skb, &switch_devs[0]) != 0 ){
			g_sw_drop++;
		}	
	}
    }else if (loopback_mode == SWITCH_TPE_LOOPBACK || loopback_mode==SWITCH_TPE_AWARE_LOOPBACK){
    	struct sk_buff* atm_skb=NULL;
	struct switch_priv * sw_dev;
	struct dma_device_info * dma_dev;
	sw_dev = (struct switch_priv *)(skb->dev->priv);
	dma_dev = (sw_dev->dma_device);
	u32 port=(u32) (dma_dev->priv);
	atm_skb = danube_atm_alloc_buffer(skb->len);
	
	if (atm_skb == NULL){
		DANUBE_IKOS_DMSG("skb allocation fails\n");
		g_sw_drop++;
		goto danube_sw_loopback_atm_err_exit;
	}
	skb_reserve(atm_skb,8);
	memcpy(skb_put(atm_skb, skb->len), skb->data, skb->len);
	
	if (loopback_mode == SWITCH_TPE_LOOPBACK){
		qid = 0x11+port;
	}else{
		qid = 0x1+port;
	}
	if (dma_may_send(0) == 0) {
		DANUBE_IKOS_DMSG("DMA channel not free\n");
		g_sw_drop++;
		goto danube_sw_loopback_atm_err_exit;
	}
	
	wm = *( (volatile u32 *) (CBM_WMSTAT0_ADDR));
	if (  (wm & (1<<(port+1))) != 0){
		DANUBE_IKOS_DMSG("watermark hits\n");
		g_sw_drop++;
		goto danube_sw_loopback_atm_err_exit;	
	}
	
	if (danube_atm_dma_tx(g_vcc[port].vpi, g_vcc[port].vci, 0, qid, atm_skb)){
		DANUBE_IKOS_DMSG("send fails\n");
		g_sw_drop++;
		goto danube_sw_loopback_atm_err_exit;
	}

	dev_kfree_skb(skb);
	return;
	
danube_sw_loopback_atm_err_exit:
	if (atm_skb != NULL) dev_kfree_skb(atm_skb);		
	dev_kfree_skb(skb);

     }

} 
 
/* 	Brief:	tpe AAL0 simple test
 *	Description:
 *		Send one AAL0 packet (ATM cell) to TPE and read it back through SWIE
 */
int do_tpe_swie ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int i;
	char c;
	g_tpe_pkt_cnt = 0;
	
	//configure CBM queues
	//TX QID 1 and RX QID 17
 	g_vcc[0].vpi = 0x33;
  	g_vcc[0].vci = 0x33;
  	g_vcc[0].itf = 0;                        //port no. 0 or 1
  	g_vcc[0].qos.aal = ATM_AAL0;
  	g_vcc[0].qos.txtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.txtp.pcr = 0;
  	g_vcc[0].qos.txtp.max_pcr = 0;
  	g_vcc[0].qos.rxtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.rxtp.pcr = 0;
  	g_vcc[0].qos.rxtp.max_pcr = 0;
	init_waitqueue_head(&g_vcc[0].sleep);
  	if (danube_atm_open(&g_vcc[0],tpe_aal0_lp)) {
 		DANUBE_IKOS_EMSG("open vcc fails\n");	
		return -EFAULT;
 	}	
	DANUBE_IKOS_EMSG("TPE queue config ok\n");
	
	
	g_swin_buffer[0] = 0x3300330;
	for(i=1;i<ATM_AAL0_SDU/4;i++){
		g_swin_buffer[i] = ((4*i-3)<<24)|
				((4*i-2)<<16)|
				((4*i-1)<<8)|
				((4*i));
	}
	danube_atm_swin(0x11, g_swin_buffer);
	while(1){
		if (serial_tstc()){
			c = getDebugChar();
			if (c == 'q' || c == 'Q'){
				DANUBE_IKOS_EMSG("terminated by user\n");
				break;
			}
		}
		if (g_tpe_pkt_cnt >= 1) break;
	}
	//wait for all packets to be received
	danube_atm_close(&g_vcc[0]);
	return 0;
}


#define TPE_DMA_TEST_LEN	80
#define TPE_DMA_TEST_NO		10

/* 	Brief:	AAL5 test
 *	Description:
 *		Send AAL5 packet and read back through DMA=>AAL5=>CBM=>AALS=>DMA
 *	command: LOOPBACK_OPTION PACKET_LEN PACKET_OFFSET PACKET_NUMBER
 *	LOOPBACK_OPTION:	0 (default: DFE) 1 (TPE)
 *	PACKET_LEN:	packet length for testing  (default is from 80 ~ 1024)
 *	PACKET_OFFSET: no meaning 
 *	PACKET_NUMBER: no. of packet for testing (default is 10)
 */
int do_tpe_dma ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int i,idx;
	u8 lb_mode,qid;
	u32 packet_len=0;
	u32 packet_no=0;
	u32 wm=0;
	int ret=0;
	char c;

	int offset = 8;
	g_tpe_pkt_cnt = 0;
#ifdef CONFIG_DANUBE_MEI
	if (argc > 1 ){
		lb_mode = (u8) simple_strtoul(argv[1],NULL, 0);
		if (lb_mode != LB_MODE_DFE && lb_mode != LB_MODE_TPE){
			DANUBE_IKOS_EMSG("Unknown loopback mode ! Use DFE Loopback\n");
			lb_mode = LB_MODE_DFE;
		}
	}else{
		lb_mode =  LB_MODE_DFE;	
	}
	if (lb_mode == LB_MODE_DFE) danube_mei_init_module();
#else	
	lb_mode =  LB_MODE_TPE;
#endif
	if (argc > 2){
		packet_len = (u32) simple_strtoul(argv[2],NULL, 0);	
	}
	if (packet_len==0) packet_len =TPE_DMA_TEST_LEN;
	
	if (argc > 4 ){
		packet_no = simple_strtoul(argv[4],NULL,0);	
	}
	if (packet_no == 0) packet_no = TPE_DMA_TEST_NO;
	
	//configure CBM queues
	//TX QID 1 and RX QID 17
	for(i=0;i<15;i++){
 		g_vcc[i].vpi = 0x33;
  		g_vcc[i].vci = 0x33+i;
  		g_vcc[i].itf = 0;                        //port no. 0 or 1
  		g_vcc[i].qos.aal = ATM_AAL5;
  		g_vcc[i].qos.txtp.traffic_class = ATM_UBR;
  		g_vcc[i].qos.txtp.pcr = 0;
  		g_vcc[i].qos.txtp.max_pcr = 0;
  		g_vcc[i].qos.rxtp.traffic_class = ATM_UBR;
  		g_vcc[i].qos.rxtp.pcr = 0;
  		g_vcc[i].qos.rxtp.max_pcr = 0;
		if ((ret=danube_atm_open(&g_vcc[i],tpe_aal5_lp))) {
 			DANUBE_IKOS_EMSG("open vcc fails itf.vpi.vci %d.%d.%d\n",g_vcc[i].itf,g_vcc[i].vpi,g_vcc[i].vci );	
			goto do_tpe_dma_err_exit;
 		}
	}
	DANUBE_IKOS_DMSG("TPE queue config ok\n");
	
	//prepare data to be sent out
	for(i=0;i <packet_no;i++){
		idx = i%15;
		
		extern int dma_may_send(int ch);
		while (dma_may_send(0) == 0);
		while (1){
			wm = *( (volatile u32 *) (CBM_WMSTAT0_ADDR));
			if (  (wm & (1<<(idx+1))) == 0){
				break;
			}
		}

		g_send_skb=danube_atm_alloc_buffer(packet_len);	
		if (g_send_skb == NULL){
			DANUBE_IKOS_EMSG("skb allocation fails\n");
			goto do_tpe_dma_err_exit;
		}
		//adjust the alignment requirement
		skb_reserve(g_send_skb,offset);
		fill_skb( &g_vcc[idx], g_send_skb, packet_len);
		
		if (lb_mode == LB_MODE_DFE){
			qid = idx+1;
		}else{
			qid = idx+ 1+ 16;
		}
		if (danube_atm_dma_tx(g_vcc[idx].vpi, g_vcc[idx].vci, 0, qid, g_send_skb)){
			DANUBE_IKOS_EMSG("send fails\n");
			goto do_tpe_dma_err_exit;
		}
	}
	//wait for all packets to be received
	while(1){
		if (serial_tstc()){
			c = getDebugChar();
			if (c == 'q' || c == 'Q'){
				DANUBE_IKOS_EMSG("terminated by user\n");
				break;
			}
		}
		if (g_tpe_pkt_cnt >= packet_no) break;
	}
	
do_tpe_dma_err_exit:
	for(i=0;i<15;i++){
		danube_atm_close(&g_vcc[i]);
	}
	DANUBE_IKOS_EMSG("packet sent: %d, received: %d \n", packet_no, g_tpe_pkt_cnt);

#ifdef CONFIG_DANUBE_MEI
	if (lb_mode == LB_MODE_DFE)
		danube_mei_cleanup_module();
#endif		
	return ret;
}

#ifdef CONFIG_DANUBE_MEI
/*
 * Brief: CRC32 calculation
 * Description: normal speed
 * len: give k+32 bits with additional 32 bits all zero
 * data: to be calculated, no 32 alignment requirements
 * if (len < 4) return 0
 */
unsigned int QUOTIENT=0x04c11db7;
static unsigned int crc32(unsigned char *data, int len)
{
    unsigned int        result;
    int                 i,j;
    unsigned char       octet;
    
    if (len < 4) return 0;

    result = *data++ << 24;
    result |= *data++ << 16;
    result |= *data++ << 8;
    result |= *data++;
    result = ~ result;
    len -=4;
    
    for (i=0; i<len; i++)
    {
        octet = *(data++);
        for (j=0; j<8; j++)
        {
            if (result & 0x80000000)
            {
                result = (result << 1) ^ QUOTIENT ^ (octet >> 7);
            }
            else
            {
                result = (result << 1) ^ (octet >> 7);
            }
            octet <<= 1;
        }
    }
    
    return ~result;             /* The complement of the remainder */
}

/*	Brief: inject ATM cell
 *	Description:
 *		create ATM header and inject with SWIE
 */
static void inject_atm_cell(u8 qid,char *data,u8 vpi, u16 vci,u8 pti,u8 clp)
{
	char atm_cell[52];
	*((u32 *)atm_cell) = (vpi<<20) | (vci<<4) | ((pti&0x7)<<1) | (clp&1);
	memcpy((atm_cell+4),data,48);
	dump_data(atm_cell, 52);
	danube_atm_swin(qid,atm_cell);
}

#define	ERR_CRC			(1<<1)
#define	ERR_INVALID_LEN		(1<<2)
#define	ERR_RECEIVE_AB		(1<<3)
#define	ERR_CPI_E		(1<<4)
#define	ERR_UU_E		(1<<5)
#define	ERR_CONGESTION		(1<<6)
#define	ERR_CLP			(1<<7)
#define	ERR_CID			(1<<8)

char g_aal5_packet_buffer[65536+256];

/*	Brief: segments to ATM cells and sends out
 *	Description:
 *		generate the AAL5 PDU with random data/increment data/all fix data
 *		with all kinds of erros and then do the Segmentation, inject to TPE
 *		through SWIE
 *		Segmentation:
 *			1. find aal5 packet size (48 bytes multiple and spaces for trailer)
 *			2. padding
 *			3. fill in CPCS_UU, CPI, packet_len, all zeros for CRC32
 *			4. calculate CRC
 *			5. cut to pieces of 48 bytes
 *	Parameters:
 *		qid:	to be inserted
 *		len: len of AAL5 PDU
 *		error_flag: error status
 *		bit	meaning
 *		1	CRC
 *		2	Invalid length
 *		3	Receive Abort (zero legnth)
 *		4	CPI error
 *		5	CPC_UU error
 *		6	Congestion
 *		7	CLP
 *	Return:
 *		0 ok
 */
static void inject_aal5_packet(u8 qid, u32 len,u8 vpi, u16 vci,u8 pti,u8 clp,u8 padding, unsigned int error_flag)
{
	u32 idx=0;
	u32 aal5_size=0;
	u32 crc;
	char * cell;
	int i=0;
	//construct ATM header
	if (error_flag & ERR_CONGESTION){
		pti |= 2;
	}
	if (error_flag & ERR_CLP){
		clp |= 1;
	}
	idx = (len / 48) + 1;
	if ((len % 48) > 40){
		idx ++;
	}
	aal5_size = idx * 48;
	memset(g_aal5_packet_buffer,0,aal5_size);
	//padding
	for(i=len;i<aal5_size-8;i++){
		g_aal5_packet_buffer[i] = padding;
	}
	//CPCS_UU
	if (error_flag & ERR_UU_E){
		g_aal5_packet_buffer[aal5_size-8] = 1;
	}else{
		g_aal5_packet_buffer[aal5_size-8] = 0;
	}
	//CPI
	if (error_flag & ERR_CPI_E){
		g_aal5_packet_buffer[aal5_size-7] = 1;
	}else{
		g_aal5_packet_buffer[aal5_size-7] = 0;
	}
	//Packet len
	if (error_flag & ERR_INVALID_LEN){
		g_aal5_packet_buffer[aal5_size-6] = 1;
		g_aal5_packet_buffer[aal5_size-5] = 1;
	}else if (error_flag & ERR_RECEIVE_AB){
		g_aal5_packet_buffer[aal5_size-6] = 0;
		g_aal5_packet_buffer[aal5_size-5] = 0;
	}else{
		g_aal5_packet_buffer[aal5_size-6] = (len>>8) & 0xff;
		g_aal5_packet_buffer[aal5_size-5] = (len) & 0xff;
	}
	
	//CRC
	g_aal5_packet_buffer[aal5_size-4] = 0;
	g_aal5_packet_buffer[aal5_size-3] = 0;
	g_aal5_packet_buffer[aal5_size-2] = 0;
	g_aal5_packet_buffer[aal5_size-1] = 0;
	
	if (!(error_flag & ERR_CRC)){
		crc = crc32(g_aal5_packet_buffer,aal5_size);
		g_aal5_packet_buffer[aal5_size-4] = (crc>>24) & 0xff;
		g_aal5_packet_buffer[aal5_size-3] = (crc>>16) & 0xff;
		g_aal5_packet_buffer[aal5_size-2] = (crc>>8) & 0xff;
		g_aal5_packet_buffer[aal5_size-1] = (crc) & 0xff;
	}
	
	
	cell = g_aal5_packet_buffer;
	for(i=0;i<idx;i++){
		if (i == idx-1){
			pti |= 1;
		}
		if (error_flag&ERR_CID){
			vci = 0x33+i;
		}
		inject_atm_cell(qid,cell,vpi,vci,pti,clp);
		cell += 48;
	}
	
}

/* 	Brief:	TPE error test
 *	Description:
 *		Send erroreous AAL5 packet and read back through DMA=>AAL5=>CBM=>AALS=>DMA
 *	command: 
 *	tpe_test: qid packet_len type_of_error
 *		type_of_error
 *		bit	meaning
 *		1	CRC
 *		2	Invalid length
 *		3	Receive Abort (zero legnth)
 *		4	CPI error
 *		5	CPC_UU error
 *		6	Congestion
 *		7	CLP
 */
int do_tpe_test ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int i;
	u8 lb_mode,qid=1;
	u32 packet_len=0;
	u32 packet_no=1;
	u32 error_flag=0;
	int ret=0;
	char c;

	g_tpe_pkt_cnt = 0;
	
	danube_mei_init_module();
	
	if (argc > 1 ){
		qid = (u8) simple_strtoul(argv[1],NULL, 0);
	}
	if (argc > 2){
		packet_len = (u32) simple_strtoul(argv[2],NULL, 0);	
	}
	if (packet_len==0) packet_len =TPE_DMA_TEST_LEN;
	
	if (argc > 3){
		error_flag = (u32) simple_strtoul(argv[3],NULL, 0);
		printk("error_flag: %x\n",error_flag);
	}
	if (argc > 4){	
	}
	//configure CBM queues
	//TX QID 1 and RX QID 17
	for(i=0;i<15;i++){
 		g_vcc[i].vpi = 0x33;
  		g_vcc[i].vci = 0x33+i;
  		g_vcc[i].itf = 0;                        //port no. 0 or 1
  		g_vcc[i].qos.aal = ATM_AAL5;
  		g_vcc[i].qos.txtp.traffic_class = ATM_UBR;
  		g_vcc[i].qos.txtp.pcr = 0;
  		g_vcc[i].qos.txtp.max_pcr = 0;
  		g_vcc[i].qos.rxtp.traffic_class = ATM_UBR;
  		g_vcc[i].qos.rxtp.pcr = 0;
  		g_vcc[i].qos.rxtp.max_pcr = 0;
		if ((ret=danube_atm_open(&g_vcc[i],tpe_aal5_lp))) {
 			DANUBE_IKOS_EMSG("open vcc fails itf.vpi.vci %d.%d.%d\n",g_vcc[i].itf,g_vcc[i].vpi,g_vcc[i].vci );	
			goto do_tpe_dma_err_exit;
 		}
	}
	DANUBE_IKOS_DMSG("TPE queue config ok\n");
	if (error_flag&ERR_CID){
		for(i=0;i<15;i++){
			set_htu_entry(g_vcc[i].vpi, g_vcc[i].vci, 1,i);
		}	
	}
	
	//prepare data to be sent out
	inject_aal5_packet(qid, packet_len,0x33,0x32+qid,0,0,0,error_flag);
	//wait for all packets to be received
	while(1){
		if (serial_tstc()){
			c = getDebugChar();
			break;
		}
		if (g_tpe_pkt_cnt >= packet_no) break;
	}
	DANUBE_IKOS_EMSG("packet sent: %d, received: %d \n", packet_no, g_tpe_pkt_cnt);
do_tpe_dma_err_exit:
	for(i=0;i<15;i++){
		danube_atm_close(&g_vcc[i]);
	}

	danube_mei_cleanup_module();
	return ret;
}
#endif //CONFIG_DANUBE_MEI
/* 	Brief:	Switch AAL5 loop back test
 *	Description:
 *		Receive packet from SW and and send to TPE as AAL5 packet
 *	The AAL5 packet is looped back and send back to Switch.
 */
int do_sw_tpe_lp ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	g_tpe_pkt_cnt = 0;
	g_sw_pkt_cnt=0;
	int ret=0;
	char c;
	u8 lb_mode;

#ifdef CONFIG_DANUBE_MEI
	if (argc > 1 ){
		lb_mode = (u8) simple_strtoul(argv[1],NULL, 0);
		if (lb_mode != LB_MODE_DFE && lb_mode != LB_MODE_TPE){
			DANUBE_IKOS_EMSG("Unknown loopback mode ! Use DFE Loopback\n");
			lb_mode = LB_MODE_DFE;
		}
	}else{
		lb_mode =  LB_MODE_DFE;	
	}
	if (lb_mode == LB_MODE_DFE) danube_mei_init_module();
#else	
	lb_mode =  LB_MODE_TPE;
#endif

	//configure CBM queues
	//TX QID 1 and RX QID 17 for port 0
	//TX QID 2 and RX QID 18 for port 1 
 	g_vcc[0].vpi = 0x33;
  	g_vcc[0].vci = 0x33;
  	g_vcc[0].itf = 0;                        //port no. 0 or 1
  	g_vcc[0].qos.aal = ATM_AAL5;
  	g_vcc[0].qos.txtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.txtp.pcr = 0;
  	g_vcc[0].qos.txtp.max_pcr = 0;
  	g_vcc[0].qos.rxtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.rxtp.pcr = 0;
  	g_vcc[0].qos.rxtp.max_pcr = 0;
  	if ((ret=danube_atm_open(&g_vcc[0],sw_tpe_lp))) {
 		DANUBE_IKOS_EMSG("open vcc fails\n");
		goto do_sw_tpe_exit;
 	}
	g_vcc[1].vpi = 0x33;
  	g_vcc[1].vci = 0x34;
  	g_vcc[1].itf = 0;                        //port no. 0 or 1
  	g_vcc[1].qos.aal = ATM_AAL5;
  	g_vcc[1].qos.txtp.traffic_class = ATM_UBR;
  	g_vcc[1].qos.txtp.pcr = 0;
  	g_vcc[1].qos.txtp.max_pcr = 0;
  	g_vcc[1].qos.rxtp.traffic_class = ATM_UBR;
  	g_vcc[1].qos.rxtp.pcr = 0;
  	g_vcc[1].qos.rxtp.max_pcr = 0;
  	if ((ret=danube_atm_open(&g_vcc[1],sw_tpe_lp))) {
 		DANUBE_IKOS_EMSG("open vcc fails\n");
		danube_atm_close(&g_vcc[0]);	
		goto do_sw_tpe_exit;
 	}

	DANUBE_IKOS_DMSG("TPE queue config ok\n");

	if ((ret=switch_init_module())){
		DANUBE_IKOS_EMSG("switch_init_module fails\n");
		goto do_sw_tpe_err_exit;
	}
	if ((ret=switch_open(&switch_devs[0]))){
		DANUBE_IKOS_EMSG("switch_open eth0 fails\n");
		switch_cleanup();
		goto do_sw_tpe_err_exit;
	}
	if ((ret=switch_open(&switch_devs[1]))){
		DANUBE_IKOS_EMSG("switch_open eth1 fails\n");
		switch_release(&switch_devs[0]);
		switch_cleanup();
		goto do_sw_tpe_err_exit;
	}
	DANUBE_IKOS_DMSG("switch config ok\n");
	if (lb_mode == LB_MODE_DFE) {
		loopback_mode = SWITCH_TPE_AWARE_LOOPBACK;
	}else{
		loopback_mode = SWITCH_TPE_LOOPBACK;
	}
	while(1){
		DANUBE_IKOS_EMSG("Reay to loop back packet through TPE\n");
		c = getDebugChar();
		if (c == 'q' || c == 'Q'){
			DANUBE_IKOS_EMSG("terminated by user\n");
			break;
		}
	};
	switch_release(&switch_devs[0]);
	switch_release(&switch_devs[1]);
	switch_cleanup();

do_sw_tpe_err_exit:
	danube_atm_close(&g_vcc[0]);
	danube_atm_close(&g_vcc[1]);
do_sw_tpe_exit:	
#ifdef CONFIG_DANUBE_MEI
	if (lb_mode == LB_MODE_DFE)
		danube_mei_cleanup_module();
#endif	
	return ret;

}

/* Brief:	switch simple loop back without driver
 * Parameter:
 * Return:	0 ok
 *		-EFAULT 
 */


#define SW1_ON
#define SW2_ON
#undef TPE1_ON
#undef TPE2_ON
#undef DPLUS_ON
#undef SW1_to_TPE1
#undef TPE1_to_SW1
#undef SW2_to_TPE2
#undef TPE2_to_SW2
#define SW1_to_SW2
#define SW2_to_SW1
                                                                                                       
#undef dma_swap

#define dma_offset0
#undef dma_offset1
#undef dma_offset2
#undef dma_offset3

#define DANUBE_DMA_CH0_ON               0x001
#define DANUBE_DMA_CH1_ON               0x002
#define DANUBE_DMA_CH2_ON               0x004
#define DANUBE_DMA_CH3_ON               0x008
#define DANUBE_DMA_CH4_ON               0x010
#define DANUBE_DMA_CH5_ON               0x020
#define DANUBE_DMA_CH6_ON               0x040
#define DANUBE_DMA_CH7_ON               0x080
#define DANUBE_DMA_CH8_ON               0x100
#define DANUBE_DMA_CH9_ON               0x200
#define DANUBE_DMA_CH10_ON              0x400
#define DANUBE_DMA_CH11_ON              0x800

#define NUM_RX_CHAN             7
#define NUM_TX_CHAN             5
#define NUM_RX_DESC             16
#define NUM_TX_DESC             16
#define MAX_PACKET_SIZE         2000

#define DES_START_ADDR          (0xA0800000)                                                                                                       

#define DANUBE_WRITE_REG(reg, value) *((volatile u32 *)(reg)) = (u32)(value)
#define DANUBE_READ_REG(reg, value)  value = (u32)*((volatile u32*)(reg))
typedef struct
{
        union
        {
                struct
                {
                        volatile u32 OWN                 :1;
                        volatile u32 C                   :1;
                        volatile u32 Sop                 :1;
                        volatile u32 Eop                 :1;
                        volatile u32 reserved            :3;
                        volatile u32 Byteoffset          :2;
                        volatile u32 reserve             :7;
                        volatile u32 DataLen             :16;
                }field;
                                                                                                       
                volatile u32 word;
        }status;
                                                                                                       
        volatile u32 DataPtr;
} danube_rx_descriptor_t;
                                                                                                       
typedef struct
{
        union
        {
                struct
                {
                        volatile u32 OWN                 :1;
                        volatile u32 C                   :1;
                        volatile u32 Sop                 :1;
                        volatile u32 Eop                 :1;
                        volatile u32 Byteoffset          :5;
                        volatile u32 reserved            :7;
                        volatile u32 DataLen             :16;
                }field;
                                                                                                       
                volatile u32 word;
        }status;
                                                                                                       
        volatile u32 DataPtr;
} danube_tx_descriptor_t;
static danube_rx_descriptor_t *rx_ring =(danube_rx_descriptor_t *)DES_START_ADDR;
static danube_tx_descriptor_t *tx_ring =(danube_tx_descriptor_t *)(DES_START_ADDR + (NUM_RX_DESC*NUM_RX_CHAN) * sizeof(danube_rx_descriptor_t));
static volatile u8  *netRxPackets =(u8*) (DES_START_ADDR + (NUM_RX_DESC*NUM_RX_CHAN) * sizeof(danube_rx_descriptor_t) + (NUM_TX_DESC*NUM_TX_CHAN) * sizeof(danube_tx_descriptor_t));
static volatile u8  *netTxPackets =(u8*) (DES_START_ADDR + (NUM_RX_DESC*NUM_RX_CHAN) * sizeof(danube_rx_descriptor_t) + (NUM_TX_DESC*NUM_TX_CHAN) * sizeof(danube_tx_descriptor_t)+ (NUM_RX_DESC * NUM_RX_CHAN) * MAX_PACKET_SIZE);
static volatile int * rx_num=(int *)(DES_START_ADDR + NUM_RX_DESC * sizeof(danube_rx_descriptor_t) + NUM_TX_DESC * sizeof(danube_tx_descriptor_t) + (NUM_RX_DESC+NUM_TX_DESC) * MAX_PACKET_SIZE);
static volatile int * tx_num=(int *)(DES_START_ADDR + NUM_RX_DESC * sizeof(danube_rx_descriptor_t) + NUM_TX_DESC * sizeof(danube_tx_descriptor_t) + (NUM_RX_DESC+NUM_TX_DESC) * MAX_PACKET_SIZE + sizeof(int));
                                                                                                       
static volatile int * rx_avail_num=(int *)(DES_START_ADDR + NUM_RX_DESC * sizeof(danube_rx_descriptor_t) + NUM_TX_DESC * sizeof(danube_tx_descriptor_t) + (NUM_RX_DESC+NUM_TX_DESC) * MAX_PACKET_SIZE + (sizeof(int)*2));
static volatile int * tx_avail_num=(int *)(DES_START_ADDR + NUM_RX_DESC * sizeof(danube_rx_descriptor_t) + NUM_TX_DESC * sizeof(danube_tx_descriptor_t) + (NUM_RX_DESC+NUM_TX_DESC) * MAX_PACKET_SIZE + (sizeof(int)*3));
static void danube_dma_init(void)
{
	/* Reset DMA
         */
        DANUBE_WRITE_REG(DANUBE_DMA_CH_RST, 0xfff);
                                                                                                       
        /* Clear Interrupt Status Register
         */
        DANUBE_WRITE_REG(DANUBE_DMA_CH0_ISR, 0xFFFFFFFF);
        DANUBE_WRITE_REG(DANUBE_DMA_CH7_ISR, 0xFFFFFFFF);
                                                                                                       
        /* Disable All Interrupts
         */
        DANUBE_WRITE_REG(DANUBE_DMA_CH0_MSK, 0xFFFFFFFF);
        DANUBE_WRITE_REG(DANUBE_DMA_CH7_MSK, 0xFFFFFFFF);
}

static void DANUBE_SW_chip_init(void)
{
        u32 temp;
        DANUBE_READ_REG(0xb0105400,temp);
        DANUBE_WRITE_REG(DANUBE_SW_UN_DEST, 0x1ff);
        DANUBE_WRITE_REG(DANUBE_SW_COS_CTL, 0xf);
        DANUBE_WRITE_REG(DANUBE_SW_ARL_CTL, 0xC90);
        DANUBE_WRITE_REG(DANUBE_SW_P2_PCTL, 0x401);
#ifdef CRC_GEN
        DANUBE_WRITE_REG(DANUBE_SW_P2_CTL, 0x6);
#else
        DANUBE_WRITE_REG(DANUBE_SW_P2_CTL, 0x4);
#endif
	DANUBE_SW_REG32(DANUBE_CGU_PLL0SR) = (DANUBE_SW_REG32(DANUBE_CGU_PLL0SR)) | 0x58000000;
	//clock for PHY
        DANUBE_SW_REG32(DANUBE_CGU_IFCCR) = (DANUBE_SW_REG32(DANUBE_CGU_IFCCR))| 0x80000004;
        //enable power for PHY
        DANUBE_SW_REG32(DANUBE_PMU_PWDCR) = (DANUBE_SW_REG32(DANUBE_PMU_PWDCR))| DANUBE_PMU_PWDCR_EPHY;
        //set reverse MII, enable MDIO statemachine
        DANUBE_SW_REG32(DANUBE_SW_MDIO_CFG) = 0x800027bf;
	while(1){
		if (((DANUBE_SW_REG32(DANUBE_SW_MDIO_CFG)) & 0x80000000) == 0) break;
	}

        //DANUBE_WRITE_REG(DANUBE_SW_MDIO_CFG,0x0550);  //Phy_addr_p1\2     -------------------
        DANUBE_WRITE_REG(DANUBE_SW_EPHY, 0xff);
        DANUBE_WRITE_REG(DANUBE_SW_PS_CTL, 0x3);
        return;
}
static void danube_fast_lb_dma_setup(void)
{
	int i;
	int j;
	*tx_num=0;
	*rx_num=0;
        *tx_avail_num=0;
        *rx_avail_num=0;
        u32 temp;
        

	DANUBE_WRITE_REG(DANUBE_DMA_Desc_BA, (u32)rx_ring); 
	
	DANUBE_WRITE_REG(DANUBE_DMA_CH0_DES_LEN, NUM_RX_DESC);  
	DANUBE_WRITE_REG(DANUBE_DMA_CH1_DES_LEN, NUM_RX_DESC);  
	DANUBE_WRITE_REG(DANUBE_DMA_CH2_DES_LEN, NUM_RX_DESC);  
	DANUBE_WRITE_REG(DANUBE_DMA_CH3_DES_LEN, NUM_RX_DESC);  
	DANUBE_WRITE_REG(DANUBE_DMA_CH4_DES_LEN, NUM_RX_DESC);  
	DANUBE_WRITE_REG(DANUBE_DMA_CH5_DES_LEN, NUM_RX_DESC);  
	DANUBE_WRITE_REG(DANUBE_DMA_CH6_DES_LEN, NUM_RX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH7_DES_LEN, NUM_TX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH8_DES_LEN, NUM_TX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH9_DES_LEN, NUM_TX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH10_DES_LEN, NUM_TX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH11_DES_LEN, NUM_TX_DESC);

	DANUBE_WRITE_REG(DANUBE_DMA_CH1_DES_OFST, NUM_RX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH2_DES_OFST, NUM_RX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH3_DES_OFST, NUM_RX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH4_DES_OFST, NUM_RX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH5_DES_OFST, NUM_RX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH6_DES_OFST, NUM_RX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH7_DES_OFST, NUM_RX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH8_DES_OFST, NUM_TX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH9_DES_OFST, NUM_TX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH10_DES_OFST, NUM_TX_DESC);
	DANUBE_WRITE_REG(DANUBE_DMA_CH11_DES_OFST, NUM_TX_DESC);

	DANUBE_WRITE_REG(DANUBE_DMA_SW_BL, 0x00000000); //two 32 bit burst
	DANUBE_WRITE_REG(DANUBE_DMA_POLLING_REG, 0x80000010);
#ifdef dma_swap
	DANUBE_WRITE_REG(DANUBE_DMA_ECON_REG, DANUBE_DMA_SW_EN_SWAP);
#endif

     for (j =0; j <NUM_RX_CHAN; j++) 
     {
	for(i=0;i < NUM_RX_DESC; i++)
	{
		danube_rx_descriptor_t * rx_desc = rx_ring+(i+(j*NUM_RX_DESC));
		rx_desc->status.word=0;	
		rx_desc->status.field.OWN=1;
	        rx_desc->status.field.Sop=0;
	        rx_desc->status.field.Eop=0;
        	rx_desc->status.field.C=0;
#ifdef dma_offset0
		rx_desc->status.field.Byteoffset=0;
#endif
#ifdef dma_offset1
		rx_desc->status.field.Byteoffset=1;
#endif
#ifdef dma_offset2
		rx_desc->status.field.Byteoffset=2;
#endif
#ifdef dma_offset3
		rx_desc->status.field.Byteoffset=3;
#endif
		rx_desc->status.field.DataLen=MAX_PACKET_SIZE;   /* 1536  */	
// reserved 8 bytes of space for tpe header
		rx_desc->DataPtr=(u32)(netRxPackets+(i+(j*NUM_RX_DESC))*MAX_PACKET_SIZE);
#ifdef SW1_to_TPE1
              if (j==0) {
		rx_desc->DataPtr=(u32)(netRxPackets+(i+(j*NUM_RX_DESC))*MAX_PACKET_SIZE + 8);
              };
#else
              if (j==4) {
		rx_desc->DataPtr=(u32)(netRxPackets+(i+(j*NUM_RX_DESC))*MAX_PACKET_SIZE + 8);
              };
#endif

#ifdef SW2_to_TPE2
              if (j==2) {
		rx_desc->DataPtr=(u32)(netRxPackets+(i+(j*NUM_RX_DESC))*MAX_PACKET_SIZE + 8);
              };
#else
              if (j==5) {
		rx_desc->DataPtr=(u32)(netRxPackets+(i+(j*NUM_RX_DESC))*MAX_PACKET_SIZE + 8);
              };
#endif
	}
     }
     for(j=0;j<NUM_TX_CHAN; j++)
     {
	for(i=0;i < NUM_TX_DESC; i++)
	{
		danube_tx_descriptor_t * tx_desc = tx_ring+(i+(j*NUM_TX_DESC));
		tx_desc->status.word=0;
		tx_desc->status.field.OWN=0;
	        tx_desc->status.field.Sop=0;
	        tx_desc->status.field.Eop=0;
        	tx_desc->status.field.C=0;
#ifdef dma_offset0
		tx_desc->status.field.Byteoffset=0;
#endif
#ifdef dma_offset1
		tx_desc->status.field.Byteoffset=1;
#endif
#ifdef dma_offset2
		tx_desc->status.field.Byteoffset=2;
#endif
#ifdef dma_offset3
		tx_desc->status.field.Byteoffset=3;
#endif
               if (j==0) {
#ifdef SW1_to_TPE1
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(4*NUM_RX_DESC))*MAX_PACKET_SIZE);
#else
  #ifdef SW1_to_SW2
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(2*NUM_RX_DESC))*MAX_PACKET_SIZE);
  #else
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(0*NUM_RX_DESC))*MAX_PACKET_SIZE);
  #endif
#endif
               } else if (j==1) {  // skip switch RX channel 1
#ifdef SW2_to_TPE2
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(4*NUM_RX_DESC)+(1*NUM_TX_DESC))*MAX_PACKET_SIZE);
#else
  #ifdef SW2_to_SW1
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(0*NUM_RX_DESC)+(0*NUM_TX_DESC))*MAX_PACKET_SIZE);
  #else
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(2*NUM_RX_DESC)+(0*NUM_TX_DESC))*MAX_PACKET_SIZE);
  #endif
#endif
               } else if (j==2) {  // skip switch RX channel 3
#ifdef TPE1_to_SW1
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(0*NUM_RX_DESC))*MAX_PACKET_SIZE);
#else
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(4*NUM_RX_DESC)+(0*NUM_TX_DESC))*MAX_PACKET_SIZE);
#endif
               } else if (j==3) {
#ifdef TPE2_to_SW2
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(2*NUM_RX_DESC)+(0*NUM_TX_DESC))*MAX_PACKET_SIZE);
#else
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(4*NUM_RX_DESC)+(1*NUM_TX_DESC))*MAX_PACKET_SIZE);
#endif
               } else if (j==4) {
		  tx_desc->DataPtr=(u32)(netRxPackets+(i+(NUM_RX_DESC*2)+(4*NUM_TX_DESC))*MAX_PACKET_SIZE);
               } 
	}
     }
		/* turn on DMA rx & tx channel
		 */
        DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
#ifdef SW1_ON
        temp = temp | DANUBE_DMA_CH0_ON; // | DANUBE_DMA_CH4_ON;
#endif

#ifdef SW2_ON
        temp = temp | DANUBE_DMA_CH2_ON; // | DANUBE_DMA_CH5_ON;
#endif

#ifdef TPE1_ON
        temp = temp | DANUBE_DMA_CH6_ON; // | DANUBE_DMA_CH8_ON;
#endif

#ifdef TPE2_ON
        temp = temp | DANUBE_DMA_CH7_ON; // | DANUBE_DMA_CH9_ON;
#endif

#ifdef DPLUS_ON
        temp = temp | DANUBE_DMA_CH10_ON; // | DANUBE_DMA_CH11_ON;
#endif
	DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
	
	DANUBE_WRITE_REG(DANUBE_SW_PS_CTL,0x7);

	return;
}


int loopback(int rx_base, int tx_base, int offset)
{
        danube_rx_descriptor_t * rx_desc = rx_ring + rx_base + offset;
        danube_tx_descriptor_t * tx_desc = tx_ring + tx_base + offset;
                if ((rx_desc->status.field.C ==1)) {
   #ifdef dma_offset0
                   tx_desc->status.word = (0xB0000000 | rx_desc->status.field.DataLen);
   #endif
   #ifdef dma_offset1
                   tx_desc->status.word = (0xB0800000 | rx_desc->status.field.DataLen);
   #endif
   #ifdef dma_offset2
                   tx_desc->status.word = (0xB1000000 | rx_desc->status.field.DataLen);
   #endif
   #ifdef dma_offset3
                   tx_desc->status.word = (0xB1800000 | rx_desc->status.field.DataLen);
   #endif
	           offset=(offset + 1) & 0x0000000F;
//	           offset=(offset + 1) % NUM_RX_DESC;
                }
    return offset;
};

int release (int rx_base, int tx_base, int offset)
{
        danube_rx_descriptor_t * rx_desc = rx_ring + rx_base + offset;
        danube_tx_descriptor_t * tx_desc = tx_ring + tx_base + offset;

                if (tx_desc->status.field.C==1)  {
   #ifdef dma_offset0
                   rx_desc->status.word = (0x80000000 | MAX_PACKET_SIZE) ;
   #endif
   #ifdef dma_offset1
                   rx_desc->status.word = (0x80800000 | MAX_PACKET_SIZE) ;
   #endif
   #ifdef dma_offset2
                   rx_desc->status.word = (0x81000000 | MAX_PACKET_SIZE) ;
   #endif
   #ifdef dma_offset3
                   rx_desc->status.word = (0x81800000 | MAX_PACKET_SIZE) ;
   #endif
	            offset=(offset + 1)& 0x0000000F;
//	           offset=(offset + 1) % NUM_RX_DESC;
                }
    return offset;
};

int do_fast_sw_lp( cmd_tbl_t * cmdtp, int argc, char *argv[])
{
	int	length  = 0;
	volatile u32 * ptr;
	int 	swap;
        u32 	temp;
        void (*jump_start)(void)= NULL;
#ifdef SW1_ON
        int  sw1_tx_on = 0;
        int  sw1_rx_base = 0;
        int  sw1_tx_base = 0;
        int    local_sw1_num = 0;
        int    local_sw1_avail_num = 0;
#endif

#ifdef SW2_ON
        int  sw2_tx_on = 0;
        int sw2_rx_base = (NUM_RX_DESC*2);
        int sw2_tx_base = (NUM_TX_DESC*1);
        int    local_sw2_num = 0;
        int    local_sw2_avail_num = 0;
#endif

#ifdef TPE1_ON
        int  tpe1_tx_on = 0;
        int tpe1_rx_base = (NUM_RX_DESC*4);
        int tpe1_tx_base = (NUM_TX_DESC*2);
        int    local_tpe1_num = 0;
        int    local_tpe1_avail_num = 0;
#endif

#ifdef TPE2_ON
        int tpe2_tx_on = 0;
        int tpe2_rx_base = (NUM_RX_DESC*5);
        int tpe2_tx_base = (NUM_TX_DESC*3);
        int    local_tpe2_num = 0;
        int    local_tpe2_avail_num = 0;
#endif

#ifdef DPLUS_ON
        int dplus_tx_on = 0;
        int dplus_rx_base = (NUM_RX_DESC*6);
        int dplus_tx_base = (NUM_TX_DESC*4);
        int    local_dplus_num = 0;
        int    local_dplus_avail_num = 0;
#endif

	danube_dma_init();
	DANUBE_SW_chip_init();
	danube_fast_lb_dma_setup();
	
    while(1) {
#ifdef SW1_ON
 #ifdef SW1_to_TPE1
        local_sw1_num = loopback(sw1_rx_base,tpe1_tx_base,local_sw1_num);
        if (local_sw1_num!=local_sw1_avail_num) {
           if (tpe1_tx_on == 0 ) {
               tpe1_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH8_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
           local_sw1_avail_num = release(sw1_rx_base,tpe1_tx_base,local_sw1_avail_num);
        };
 #else
  #ifdef SW1_to_SW2
        local_sw1_num = loopback(sw1_rx_base,sw2_tx_base,local_sw1_num);
        if (local_sw1_num!=local_sw1_avail_num) {
           if (sw2_tx_on == 0 ) {
               sw2_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH5_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
           local_sw1_avail_num = release(sw1_rx_base,sw2_tx_base,local_sw1_avail_num);
        };
  #else
        local_sw1_num = loopback(sw1_rx_base,sw1_tx_base,local_sw1_num);
        if (local_sw1_num!=local_sw1_avail_num) {
           if (sw1_tx_on == 0 ) {
               sw1_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH4_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
           local_sw1_avail_num = release(sw1_rx_base,sw1_tx_base,local_sw1_avail_num);
        };
  #endif
 #endif
#endif

#ifdef SW2_ON
 #ifdef SW2_to_TPE2
        local_sw2_num = loopback(sw2_rx_base,tpe2_tx_base,local_sw2_num);
        if (local_sw2_num!=local_sw2_avail_num) {
           if (tpe2_tx_on == 0 ) {
               tpe2_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH9_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
            local_sw2_avail_num = release(sw2_rx_base,tpe2_tx_base,local_sw2_avail_num);
        };
 #else
  #ifdef SW2_to_SW1
        local_sw2_num = loopback(sw2_rx_base,sw1_tx_base,local_sw2_num);
        if (local_sw2_num!=local_sw2_avail_num) {
           if (sw1_tx_on == 0 ) {
               sw1_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH4_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
            local_sw2_avail_num = release(sw2_rx_base,sw1_tx_base,local_sw2_avail_num);
        };
  #else
        local_sw2_num = loopback(sw2_rx_base,sw2_tx_base,local_sw2_num);
        if (local_sw2_num!=local_sw2_avail_num) {
           if (sw2_tx_on == 0 ) {
               sw2_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH5_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
            local_sw2_avail_num = release(sw2_rx_base,sw2_tx_base,local_sw2_avail_num);
        };
  #endif
 #endif
#endif
#ifdef TPE1_ON
 #ifdef TPE1_to_SW1
        local_tpe1_num = loopback(tpe1_rx_base,sw1_tx_base,local_tpe1_num);
        if (local_tpe1_num!=local_tpe1_avail_num) {
           if (sw1_tx_on == 0 ) {
               sw1_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH5_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
            local_tpe1_avail_num = release(tpe1_rx_base,tpe1_tx_base,local_tpe1_avail_num);
        };
 #else
        local_tpe1_num = loopback(tpe1_rx_base,tpe1_tx_base,local_tpe1_num);
        if (local_tpe1_num!=local_tpe1_avail_num) {
           if (tpe1_tx_on == 0 ) {
               tpe1_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH8_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
            local_tpe1_avail_num = release(tpe1_rx_base,tpe1_tx_base,local_tpe1_avail_num);
        };
 #endif
#endif
#ifdef TPE2_ON
 #ifdef TPE2_to_SW2
        local_tpe2_num = loopback(tpe2_rx_base,sw2_tx_base,local_tpe2_num);
        if (local_tpe2_num!=local_tpe2_avail_num) {
           if (sw2_tx_on == 0 ) {
               sw2_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH6_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
            local_tpe2_avail_num = release(tpe2_rx_base,sw2_tx_base,local_tpe2_avail_num);
        };
 #else
        local_tpe2_num = loopback(tpe2_rx_base,tpe2_tx_base,local_tpe2_num);
        if (local_tpe2_num!=local_tpe2_avail_num) {
           if (tpe2_tx_on == 0 ) {
               tpe2_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH8_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
            local_tpe2_avail_num = release(tpe2_rx_base,tpe2_tx_base,local_tpe2_avail_num);
        };
 #endif
#endif
#ifdef DPLUS_ON
        local_dplus_num = loopback(dplus_rx_base,dplus_tx_base,local_dplus_num);
        if (local_dplus_num!=local_dplus_avail_num) {
           if (dplus_tx_on == 0 ) {
               dplus_tx_on = 1;
               DANUBE_READ_REG(DANUBE_DMA_CH_ON, temp);
               temp = temp | DANUBE_DMA_CH11_ON;
               DANUBE_WRITE_REG(DANUBE_DMA_CH_ON, temp);
            };
            local_dplus_avail_num = release(dplus_rx_base,dplus_tx_base,local_dplus_avail_num);
        };
#endif
    }
    return 0;
}

/* Brief:	switch simple loop back
 * Parameter:
 * Return:	0 ok
 *		-EFAULT 
 */
int do_sw_lp( cmd_tbl_t * cmdtp, int argc, char *argv[])
{
	u8 len = 80;
	int i;
	char c;
	int ret=0;
	g_sw_pkt_cnt=0;
	g_sw_drop=0;
	if ((ret=switch_init_module())){
		DANUBE_IKOS_EMSG("switch_init_module fails\n");
		goto do_sw_lp_exit;
	}
	if ((ret=switch_open(&switch_devs[0]))){
		DANUBE_IKOS_EMSG("switch_open eth0 fails\n");
		switch_cleanup();
		goto do_sw_lp_exit;
	}
	if ((ret=switch_open(&switch_devs[1]))){
		DANUBE_IKOS_EMSG("switch_open eth1 fails\n");
		switch_release(&switch_devs[0]);
		switch_cleanup();
		goto do_sw_lp_exit;
	}
	DANUBE_IKOS_DMSG("switch config ok\n");
	loopback_mode = SWITCH_PORT_LOOPBACK;
	while(1){
		DANUBE_IKOS_EMSG("Press 0 or 1 to send packet to respective port\n");
		DANUBE_IKOS_EMSG("Ready to loop back packet from port 0 and port1\n");
		c = getDebugChar();
		if (c == 'q' || c == 'Q'){
			DANUBE_IKOS_EMSG("terminated by user\n");
			break;
		}else if (c == '0' || c == '1'){
			g_sw_send_skb = dev_alloc_skb(len);
			if (g_sw_send_skb == NULL){
				DANUBE_IKOS_EMSG("no memory for skb\n");
				ret = -ENOMEM;
				goto do_sw_lp_exit;
			}
			for(i=0;i<len;i++){
				g_sw_send_skb->data[i] = i;
			}
			skb_put(g_sw_send_skb,len);
			switch_tx(g_sw_send_skb, &switch_devs[(c-'0')]);
		} 
	};

	switch_release(&switch_devs[0]);
	switch_release(&switch_devs[1]);
	switch_cleanup();
do_sw_lp_exit:
	return ret;
}

#ifdef CONFIG_DANUBE_EEPROM
/* Brief:	SSC test
 * Parameter:
 * Return:	0 ok
 *		-EFAULT 
 * Description:
 *	1. reset EEPROM status
 *	2. read status
 *	3. write a random data
 *	4. read back
 *	5. check
 * Format:	ssc [start_addr] [test_size] [read_only]
 */

#define EEPROM_TEST_SIZE	51
#define EEPROM_TEST_START_ADDR	0x0
#define EEPROM_SIZE		512
static int do_eeprom ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	unsigned char *ssc_tx_buf=NULL;
	unsigned char *ssc_rx_buf=NULL;
	int i=0, ret=0;
	u32 eff_size;
	u32 data;
	u32 test_size=0;
	u32 test_start_addr=EEPROM_TEST_START_ADDR;
	u32 readonly=0;
	char cmd;
	if (argc > 1){
		test_start_addr = simple_strtoul(argv[1],NULL, 0);	
		if (test_start_addr > EEPROM_SIZE) {
			DANUBE_IKOS_EMSG("start_addr not valid %x, (should less than %x) \n",test_start_addr,EEPROM_SIZE);
			ret = -EINVAL;
			goto do_eeprom_init_exit;
		}
	}
	
	if (argc > 2){
		test_size = simple_strtoul(argv[2],NULL, 0);	
		if ((test_size+test_start_addr) > EEPROM_SIZE) {
			DANUBE_IKOS_EMSG("test_size not valid %x, (should less than %x) \n",test_size,(EEPROM_SIZE-test_start_addr));
			ret = -EINVAL;
			goto do_eeprom_init_exit;
		}
	}
	if (test_size==0) test_size = EEPROM_TEST_SIZE;
	
	if (argc > 3){
		readonly = simple_strtoul(argv[3],NULL, 0);	
	}
	DANUBE_IKOS_DMSG("start_address=%d, size=%d\n",test_start_addr,test_size);
		
	eff_size = ((test_size+3) & (~3));
	ssc_tx_buf = (char*) kmalloc(sizeof(char) * eff_size, GFP_KERNEL);
	ssc_rx_buf = (char*) kmalloc(sizeof(char) * eff_size, GFP_KERNEL);
	
	if (ssc_tx_buf == NULL || ssc_rx_buf == NULL){
		DANUBE_IKOS_EMSG("no memory\n");
		ret=-ENOMEM;
		goto do_eeprom_init_exit;
	}

	if ((ret=ifx_ssc_init())){
		DANUBE_IKOS_EMSG("ifx_ssc_init fails\n");
		goto do_eeprom_init_exit;
	}
	DANUBE_IKOS_DMSG("ifx_ssc_init: ok\n");
	
	for(i=0;i<test_size;i++){
		ssc_tx_buf[i] = i;
		ssc_rx_buf[i] = 0;
	}
	
	if ( readonly == 0){
		if ((ret=danube_eeprom_kwrite(ssc_tx_buf,test_size,test_start_addr))){
			DANUBE_IKOS_DMSG("eeprom write fails\n");
			goto do_eeprom_open_exit;
		}
	}
	if ((ret=danube_eeprom_kread(ssc_rx_buf,test_size,test_start_addr))){
			DANUBE_IKOS_DMSG("eeprom write fails\n");
			goto do_eeprom_open_exit;
	}
	
	for(i=0;i<test_size;i++){
/*	
		if (ssc_tx_buf[i] != ssc_rx_buf[i]){
			DANUBE_IKOS_EMSG("DATA dismatch at %4x write[%2x] read[%2x]\n", 
				(i+EEPROM_TEST_START_ADDR),
				ssc_tx_buf[i],
				ssc_rx_buf[i]);
			ret = -1;
		}
*/
		printk("%2x ",ssc_rx_buf[i]);
		if (i % 16 == 15 )printk ("\n");
	}
	printk("\n");

do_eeprom_open_exit:
	ifx_ssc_cleanup_module();
do_eeprom_init_exit:
	if (ssc_tx_buf)	{
		kfree(ssc_tx_buf);
	}
	if (ssc_rx_buf)	{
		kfree(ssc_rx_buf);
	}		
	if (ret){
		DANUBE_IKOS_EMSG("erro code %d\n",ret);
	}
		DANUBE_IKOS_EMSG("***************************************************\n");
	if (ret){
		DANUBE_IKOS_EMSG("**** SSC EEPROM TEST with size %3d FAILS !!!  *****\n",test_size);
	}else{
		DANUBE_IKOS_EMSG("**** SSC EEPROM TEST with size %3d PASS    ********\n",test_size);
	}
		DANUBE_IKOS_EMSG("***************************************************\n");
	
	return 0;
}

#endif //CONFIG_DANUBE_EEPROM

#ifdef CONFIG_PCI
/* Brief:	pci
 */
static int do_pci ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	pcibios_init();
	return 0;
}
#endif //CONFIG_PCI

#ifdef CONFIG_DANUBE_WDT
#define DEF_TIMEOUT 30

/* Brief:	enable WDT and wait for the reset
 * Format: wdt [timeout]
 */
int do_wdt ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	char c;
	u32 timeout=DEF_TIMEOUT;
	
	if (argc > 1){
		timeout = simple_strtoul(argv[1],NULL, 0);	
	}
	if (timeout == 0) timeout = DEF_TIMEOUT;
	
	danube_wdt_init_module();
	
	wdt_enable((int)timeout);
	DANUBE_IKOS_EMSG("WDT enabled. Press c to renew WDT\n");
	
	while(1){
		if (serial_tstc()){
			c = getDebugChar();
			if (c == 'c' || c == 'C'){
				wdt_enable(timeout);
			}else if (c == 'q' || c == 'Q'){
				wdt_disable();
				break;
			}
		}
		DANUBE_IKOS_EMSG("Current timer value=%8x\n",(DANUBE_WDT_REG32(DANUBE_WDT_SR)>>16));
	};
	
	danube_wdt_cleanup_module();
	return 0;
}
#endif //CONFIG_DANUBE_WDT

#ifdef CONFIG_DANUBE_MEI
/* Brief:	MEI CMV test
 */
int do_mei ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	danube_mei_init_module();

	u32 temp;
	char c;
	DANUBE_IKOS_EMSG("Press q to quit\n");
	while(1){
		c = getDebugChar();
		if (c == 'q' || c == 'Q'){
			break;
		}
		meiLongwordWrite(MEI_XFR_ADDR, 0x1041c);
        	meiLongwordRead(MEI_DATA_XFR, &temp);
		DANUBE_IKOS_EMSG("Heart Beat %8x \n", temp);
	};

	danube_mei_cleanup_module();
	return 0;
}
/* 	Brief:	tpe AAL0 simple test
 *	Description:
 *		Send one AAL0 packet (ATM cell) to TPE and read it back through SWIE
 */
int do_tpe_oam ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int i;
	char c;
	u8 vpi=0x33;
	u16 vci=0x33;
	u8 pti=0;

	
	if (argc > 1 ){
		 vpi = (u8) simple_strtoul(argv[1],NULL, 0);
	}
	if (argc > 2 ){
		 vci = (u16)simple_strtoul(argv[2],NULL, 0);
	}
	if (argc > 3 ){
		 pti = (u8) simple_strtoul(argv[3],NULL, 0);
	}
	danube_mei_init_module();
	//configure CBM queues
	//TX QID 1 and RX QID 17
 	g_vcc[0].vpi = vpi;
  	g_vcc[0].vci = vci;
  	g_vcc[0].itf = 0;                        //port no. 0 or 1
  	g_vcc[0].qos.aal = ATM_AAL0;
  	g_vcc[0].qos.txtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.txtp.pcr = 0;
  	g_vcc[0].qos.txtp.max_pcr = 0;
  	g_vcc[0].qos.rxtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.rxtp.pcr = 0;
  	g_vcc[0].qos.rxtp.max_pcr = 0;
	init_waitqueue_head(&g_vcc[0].sleep);
  	if (danube_atm_open(&g_vcc[0],tpe_aal0_lp)) {
 		DANUBE_IKOS_EMSG("open vcc fails\n");	
		return -EFAULT;
 	}	
	DANUBE_IKOS_DMSG("TPE queue config ok\n");
	
	DANUBE_IKOS_DMSG("send oam to vpi:%d vci:%d pti:%d\n", vpi, vci, pti);
	g_swin_buffer[0] = (vpi<<20) + (vci<<4) + (pti<<1);	
	for(i=1;i<ATM_AAL0_SDU/4;i++){
		g_swin_buffer[i] = ((4*i-3)<<24)|
				((4*i-2)<<16)|
				((4*i-1)<<8)|
				((4*i));
	}
	danube_atm_swin(0x1, g_swin_buffer);
	while(1){
		if (serial_tstc()){
			c = getDebugChar();
			if (c == 'q' || c == 'Q'){
				DANUBE_IKOS_EMSG("terminated by user\n");
				break;
			}
		}
	}
	//wait for all packets to be received
	danube_atm_close(&g_vcc[0]);
	danube_mei_cleanup_module();
	return 0;
}

/* 	Brief:	tpe AAL0 simple test through AWARE DFE
 *	Description:
 *		Send one AAL0 packet (ATM cell) to TPE and read it back through SWIE
 */
int do_tpe0_dfe ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int i;
	char c;
	g_tpe_pkt_cnt = 0;
	
	//*((volatile u32 *)(PWR_RSTRQ)) = 0;

	danube_mei_init_module();
	unsigned long j = jiffies + 2;
	while (jiffies < j);
	//configure CBM queues
	//TX QID 1 and RX QID 17
 	g_vcc[0].vpi = 0x33;
  	g_vcc[0].vci = 0x33;
  	g_vcc[0].itf = 0;                        //port no. 0 or 1
  	g_vcc[0].qos.aal = ATM_AAL0;
  	g_vcc[0].qos.txtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.txtp.pcr = 0;
  	g_vcc[0].qos.txtp.max_pcr = 0;
  	g_vcc[0].qos.rxtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.rxtp.pcr = 0;
  	g_vcc[0].qos.rxtp.max_pcr = 0;
	init_waitqueue_head(&g_vcc[0].sleep);
  	if (danube_atm_open(&g_vcc[0],tpe_aal5_lp)) {
 		DANUBE_IKOS_EMSG("open vcc fails\n");	
		return -EFAULT;
 	}	
	DANUBE_IKOS_EMSG("TPE queue config ok\n");
	
	
	g_swin_buffer[0] = 0x3300330;
	for(i=1;i<ATM_AAL0_SDU/4;i++){
		g_swin_buffer[i] = ((4*i-3)<<24)|
				((4*i-2)<<16)|
				((4*i-1)<<8)|
				((4*i));
	}
	danube_atm_swin(0x1, g_swin_buffer);
	while(1){
		if (serial_tstc()){
			c = getDebugChar();
			if (c == 'q' || c == 'Q'){
				DANUBE_IKOS_EMSG("terminated by user\n");
				break;
			}
		}
		if (g_tpe_pkt_cnt >= 1) break;
	}
	//wait for all packets to be received
	danube_atm_close(&g_vcc[0]);
	danube_mei_cleanup_module();
	return 0;
}
/* 	Brief:	tpe AAL5 simple test through AWARE DFE
 *	Description:
 *		Send one AAL0 packet (ATM cell) to TPE and read it back through SWIE
 */
int do_tpe5_dfe ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	g_tpe_pkt_cnt = 0;

	danube_mei_init_module();
	unsigned long j = jiffies + 2;
	while (jiffies < j);
	
	int i;
	u8 * data;
	u32 packet_len = 0;
	int offset = 8;
	g_tpe_pkt_cnt = 0;
	int ret=0;
	char c;
	if (argc > 1){
		packet_len = (u8) simple_strtoul(argv[1],NULL, 0);	
	}
	if (argc > 2 ){
		offset = simple_strtoul(argv[2],NULL,0);
	}
	if (packet_len==0) packet_len =TPE_DMA_TEST_LEN;
	
	//configure CBM queues
	//TX QID 1 and RX QID 17
 	g_vcc[0].vpi = 0x33;
  	g_vcc[0].vci = 0x33;
  	g_vcc[0].itf = 0;                        //port no. 0 or 1
  	g_vcc[0].qos.aal = ATM_AAL5;
  	g_vcc[0].qos.txtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.txtp.pcr = 0;
  	g_vcc[0].qos.txtp.max_pcr = 0;
  	g_vcc[0].qos.rxtp.traffic_class = ATM_UBR;
  	g_vcc[0].qos.rxtp.pcr = 0;
  	g_vcc[0].qos.rxtp.max_pcr = 0;
  	if ((ret=danube_atm_open(&g_vcc[0],tpe_aal5_lp))) {
 		DANUBE_IKOS_EMSG("open vcc fails\n");	
		goto do_tpe5_dfe_err_exit;
 	}
	DANUBE_IKOS_DMSG("TPE queue config ok\n");
	
	//prepare data to be sent out
	danube_atm_alloc_rx(packet_len,&i,(void **)&g_send_skb);
	if (g_send_skb == NULL){
		DANUBE_IKOS_EMSG("skb allocation fails\n");
		goto do_tpe5_dfe_err_exit;
	}
	//reserve for header
	skb_reserve(g_send_skb,offset);
	data = (u8 *) skb_put(g_send_skb,packet_len);
	for (i=0;i<packet_len;i++){
		data[i] = i;
	}
	if (danube_atm_dma_tx(g_vcc[0].vpi, g_vcc[0].vci, 0, 0x1, g_send_skb)){
		DANUBE_IKOS_EMSG("send fails\n");
		goto do_tpe5_dfe_err_exit;
	}
	//wait for all packets to be received
	while(1){
		if (serial_tstc()){
			c = getDebugChar();
			if (c == 'q' || c == 'Q'){
				DANUBE_IKOS_EMSG("terminated by user\n");
				break;
			}
		}
		if (g_tpe_pkt_cnt >= 1) break;
	}
do_tpe5_dfe_err_exit:
	danube_atm_close(&g_vcc[0]);

	danube_mei_cleanup_module();
	return ret;
}



#endif //CONFIG_DANUBE_MEI
/**************************************************************************/
/* Brief:	help
 */
static int do_help ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int i=0;
	printk("test cases:\n");
	while ( cmd_tbl[i].name ){
		printk("\t%s\n", cmd_tbl[i].usage);
		i++;
	}
	return 0;
}
/***************************************************************************
 * Brief:	 find command table entry for a command
 */
cmd_tbl_t *find_cmd(const char *cmd)
{
	cmd_tbl_t *cmdtp;
	/* Search command table - Use linear search - it's a small table */
	for (cmdtp = &cmd_tbl[0]; cmdtp->name; cmdtp++) {
		if (strncmp (cmd, cmdtp->name, cmdtp->len) == 0){
			return cmdtp;
		}
	}
	return NULL;	/* not found */
}
/****************************************************************************
 * Brief: 	parse command line to args
 */
int parse_line (char *line, char *argv[])
{
	int nargs = 0;
	
#ifdef DEBUG_PARSER
	DANUBE_IKOS_DMSG ("parse_line: \"%s\"\n", line);
#endif

	while (nargs < CFG_MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
#ifdef DEBUG_PARSER
		DANUBE_IKOS_DMSG ("parse_line: nargs=%d\n", nargs);
#endif
			return (nargs);
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
#ifdef DEBUG_PARSER
		DANUBE_IKOS_DMSG ("parse_line: nargs=%d\n", nargs);
#endif
			return (nargs);
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	printk ("** Too many args (max. %d) **\n", CFG_MAXARGS);

#ifdef DEBUG_PARSER
	DANUBE_IKOS_DMSG ("parse_line: nargs=%d\n", nargs);
#endif
	return (nargs);
}
/* ********************************************************************/
/* Brief:	process a line of commond
 * Parameter:	cmd_line, current command line
 * Return:	0 on successful
 * 		1 quit command
 *		-1 error
 */
int process_command(char * cmd_line)
{
	cmd_tbl_t *cmdtp;
	char *argv[CFG_MAXARGS + 1];	/* NULL terminated	*/
	int argc;
	
	//ignore comments
	if (*cmd_line == '#'|| strlen(cmd_line) == 0) {
		return 0;
	}

	/* Extract arguments */
	argc = parse_line (cmd_line, argv);

	/* Look up command in command table */
	if ((cmdtp = find_cmd(argv[0])) == NULL) {
		printk ("Unknown command '%s' - try 'help'\n", argv[0]);
		return -1;	/* give up after bad command */
	}
	
	/* OK - call function to do the command */
	return (cmdtp->cmd) (cmdtp,argc, argv);
}

/* Brief:	read a line from console (null terminated)
 * Paramter:	buf, hold the return string
 *		limit, size of the buffer (including the null char)
 * Return:	number of bytes read, 0 means error
	
 */
static int read_line(char * buf, int limit) {
	int len=0;
	char c;
	while(1){
		c = getDebugChar();
		if (c == '\r' || c == '\n') {
			printk("\n");
			break;
		}
		//echo back
		putDebugChar(c);
		buf[len++] = c;
		if (len == limit - 1) break;
	}
	buf[len] = '\0';	
	return len;
}

/**************************************************************************/
/* Brief:	help
 */
extern unsigned long volatile jiffies;
static int do_time( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	printk("%16ul\n", (unsigned int) jiffies);
	return 0;
}

void __ikos(void){
	char line_buffer[CMD_MAX+1];
	printk("IKOS verification...\n");
	while (1){
		printk(">");
		if (read_line(line_buffer, CMD_MAX) != 0){
			//process the command
			process_command(line_buffer);
		}				
	}
}

/* Brief: testcase entry
 * Parameter: no
 * Return: never
 * Description:
 * 	Since the IKOS emulation is extremly slow, we can't wait until the 
 * whole system is up and then verify our drivers. Thus we decide to hack
 * the kernel so that it is traped in this function to do verification.
 * We borrow some parts from U-boot.
 */
void ikos(void){	
	if (danube_atm_create() == NULL){
		DANUBE_IKOS_EMSG("TPE init fails\n");
	}else{
		DANUBE_IKOS_EMSG("TPE init ok\n");	
	}
	__ikos();
	danube_atm_cleanup();
}
#endif //IKOS_MINI_BOOT








