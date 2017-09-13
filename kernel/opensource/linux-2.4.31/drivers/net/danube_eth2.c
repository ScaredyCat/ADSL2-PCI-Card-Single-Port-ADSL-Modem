/******************************************************************************
**
** FILE NAME    : danube_eth2.c
** PROJECT      : Danube
** MODULES     	: Second ETH Interface (MII1)
**
** DATE         : 28 NOV 2005
** AUTHOR       : Xu Liang
** DESCRIPTION  : Second ETH Interface (MII1) Driver
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
** 28 NOV 2005  Xu Liang        Initiate Version
** 23 AUG 2006  Xu Liang        Add feature for D1 support:
**                              1. DPLUS shared by both ETH0 and ETH2.
**                              2. TX descriptors are moved to share buffer.
** 23 OCT 2006  Xu Liang        Add GPL header.
** 18 DEC 2006  Xu Liang        Move dma_cache_inv after memory writing
** 11 JAN 2007  Xu Liang        Move eth2_dev.irq_handling_flag check after TX
**                              interrupt handler in case RX tasklet block TX
**                              interrupt in high speed injection and low speed
**                              egress case.
*******************************************************************************/



/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/atmdev.h>
#include <linux/init.h>
#include <linux/etherdevice.h>  /*  eth_type_trans  */
#include <linux/ethtool.h>      /*  ethtool_cmd     */
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <linux/errno.h>

/*
 *  Chip Specific Head File
 */
#include <asm/danube/irq.h>

#include <asm/danube/danube_cgu.h>
#include <asm/danube/danube_eth2.h>


/*
 * ####################################
 *   Parameters to Configure PPE
 * ####################################
 */
static int write_descriptor_delay  = 0x20;      /*  Write descriptor delay                          */

static int rx_max_packet_size   = 0x05EE;       /*  Max packet size for RX                          */
static int rx_min_packet_size   = 0x0040;       /*  Min packet size for RX                          */
static int tx_max_packet_size   = 0x05EE;       /*  Max packet size for TX                          */
static int tx_min_packet_size   = 0x0040;       /*  Min packet size for TX                          */

static int dma_rx_descriptor_length = 24;       /*  Number of descriptors per DMA RX channel        */
static int dma_tx_descriptor_length = 24;       /*  Number of descriptors per DMA TX channel        */

MODULE_PARM(write_descriptor_delay, "i" );
MODULE_PARM_DESC(write_descriptor_delay, "PPE core clock cycles between descriptor write and effectiveness in external RAM");

MODULE_PARM(rx_max_packet_size, "i");
MODULE_PARM_DESC(rx_max_packet_size, "Max packet size in byte for downstream ethernet frames");
MODULE_PARM(rx_min_packet_size, "i");
MODULE_PARM_DESC(rx_min_packet_size, "Min packet size in byte for downstream ethernet frames");
MODULE_PARM(tx_max_packet_size, "i");
MODULE_PARM_DESC(tx_max_packet_size, "Max packet size in byte for upstream ethernet frames");
MODULE_PARM(tx_min_packet_size, "i");
MODULE_PARM_DESC(tx_min_packet_size, "Min packet size in byte for upstream ethernet frames");

MODULE_PARM(dma_rx_descriptor_length, "i");
MODULE_PARM_DESC(dma_rx_descriptor_length, "Number of descriptor assigned to DMA RX channel (>16)");
MODULE_PARM(dma_tx_descriptor_length, "i");
MODULE_PARM_DESC(dma_tx_descriptor_length, "Number of descriptor assigned to DMA TX channel (>16)");


/*
 * ####################################
 *              Definition
 * ####################################
 */

#define ENABLE_TWINPATH_BOARD           0

#define LINUX_2_4_31                    1

#define ENABLE_TX_CLK_INVERSION         1

#define ENABLE_RX_DPLUS_PATH            0

#define ENABLE_SHAREBUFFER_TX_DESC      0

#define ENABLE_LOOPBACK_TESTING_RX2TX   0

#define ENABLE_LOOPBACK_TESTING_TX2RX   0

#define ENABLE_DIRECT_BRIDGE            0

#define ENABLE_HARDWARE_EMULATION       0

#define ENABLE_ETH2_DEBUG               0

#define ENABLE_ETH2_ASSERT              0

#define ENABLE_DEBUG_COUNTER            0

#define DEBUG_DUMP_RX_SKB_BEFORE        0   //  before function eth_type_trans

#define DEBUG_DUMP_RX_SKB_AFTER         0   //  after function eth_type_trans

#define DEBUG_DUMP_TX_SKB               0

#define DEBUG_DUMP_ETOP_REGISTER        0

#define DEBUG_WRITE_GPIO_REGISTER       1

#define ENABLE_PROBE_TRANSCEIVER        0

#define ENABLE_ETH2_HW_FLOWCONTROL      1

#if !defined(ENABLE_TWINPATH_BOARD) || !ENABLE_TWINPATH_BOARD
  #define MII_MODE_SETUP                MII_MODE
#else
  #define MII_MODE_SETUP                REV_MII_MODE

  #undef  ENABLE_MII1_TX_CLK_INVERSION
  #define ENABLE_MII1_TX_CLK_INVERSION  0
#endif

#define PPE_MAILBOX_IGU1_INT            INT_NUM_IM2_IRL24

#define MY_ETHADDR                      my_ethaddr

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
  #undef  ENABLE_SHAREBUFFER_TX_DESC
  #define ENABLE_SHAREBUFFER_TX_DESC    0

  #include <asm/danube/danube_eth2_fw.h>
#else
  #include <asm/danube/danube_dma.h>

  #if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    #include <asm/danube/danube_eth2_fw_with_dplus.h>
  #else
    #define SB_TX_DESC_BASE_ADDR        SB_RAM0_ADDR(0x0200)

    #include <asm/danube/danube_eth2_fw_with_dplus_sb.h>
  #endif
#endif

#if (defined(DEBUG_DUMP_RX_SKB_BEFORE) && DEBUG_DUMP_RX_SKB_BEFORE) || (defined(DEBUG_DUMP_RX_SKB_AFTER) && DEBUG_DUMP_RX_SKB_AFTER)
  #define DEBUG_DUMP_RX_SKB             1
#else
  #define DEBUG_DUMP_RX_SKB             0
#endif

#if defined(CONFIG_NET_HW_FLOWCONTROL) && (defined(ENABLE_ETH2_HW_FLOWCONTROL) && ENABLE_ETH2_HW_FLOWCONTROL)
  #define ETH2_HW_FLOWCONTROL           1
#else
  #define ETH2_HW_FLOWCONTROL           0
#endif

#if defined(ENABLE_ETH2_DEBUG) && ENABLE_ETH2_DEBUG
  #define ENABLE_DEBUG_PRINT            1
  #define DISABLE_INLINE                1
#else
  #define ENABLE_DEBUG_PRINT            0
  #define DISABLE_INLINE                0
#endif  //  defined(ENABLE_ETH2_DEBUG) && ENABLE_ETH2_DEBUG

#if !defined(DISABLE_INLINE) || !DISABLE_INLINE
  #define INLINE                        inline
#else
  #define INLINE
#endif  //  !defined(DISABLE_INLINE) || !DISABLE_INLINE

#if defined(ENABLE_DEBUG_PRINT) && ENABLE_DEBUG_PRINT
  #undef  dbg
  #define dbg(format, arg...)           printk(KERN_WARNING __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg)
#else
  #if !defined(dbg)
    #define dbg(format, arg...)         do {} while (0)
  #endif    //  !defined(dbg)
#endif  //  defined(ENABLE_DEBUG_PRINT) && ENABLE_DEBUG_PRINT

#if defined(ENABLE_ETH2_ASSERT) && ENABLE_ETH2_ASSERT
  #define ETH2_ASSERT(cond, format, arg...)  \
                                        do { if ( !(cond) ) printk(KERN_ERR __FILE__ ":%d:%s: " format "\n", __LINE__, __FUNCTION__, ##arg); } while ( 0 )
#else
  #define ETH2_ASSERT(cond, format, arg...)  \
                                        do { } while ( 0 )
#endif  //  defined(ENABLE_ETH2_ASSERT) && ENABLE_ETH2_ASSERT

/*
 *  Eth Mode
 */
#define MII_MODE                        1
#define REV_MII_MODE                    2

/*
 *  Default Eth Hardware Configuration
 */
#define CDM_CFG_DEFAULT                 0x00000000
#define SB_MST_SEL_DEFAULT              0x00000003
#if 0
  #define ENETS1_CFG_DEFAULT            (0x00007037 | (RX_HEAD_MAC_ADDR_ALIGNMENT << 18) | (1 << 30))
#else
  #define ENETS1_CFG_DEFAULT            (0x00007037 | (RX_HEAD_MAC_ADDR_ALIGNMENT << 18))
#endif
#define ENETF1_CFG_DEFAULT              0x00007010
#define ENETS1_PGCNT_DEFAULT            0x00020000
#define ENETS1_PKTCNT_DEFAULT           0x00000200
#define ENETF1_PGCNT_DEFAULT            0x00020000
#define ENETF1_PKTCNT_DEFAULT           0x00000200
#define ENETS1_COS_CFG_DEFAULT          0x00000002  // This enables multiple DMA channels when packets with VLAN is injected. COS mapping is through ETOP_IG_VLAN_COS; It is already preconfigured.
#define ENETS1_DBA_DEFAULT              0x00000400
#define ENETS1_CBA_DEFAULT              0x00000AE0
#define ENETF1_DBA_DEFAULT              0x00001600
#define ENETF1_CBA_DEFAULT              0x00001800

/*
 *  Constant Definition
 */
#define ETH2_WATCHDOG_TIMEOUT           (10 * HZ)
#define ETOP_MDIO_PHY1_ADDR             1
#define ETOP_MDIO_DELAY                 1
#define IDLE_CYCLE_NUMBER               30000
#define RX_TOTAL_CHANNEL_USED           MAX_RX_DMA_CHANNEL_NUMBER
#define TX_TOTAL_CHANNEL_USED           MAX_TX_DMA_CHANNEL_NUMBER

/*
 *  Tasklet Parameters
 */
#if 0
  #define TASKLET_MAX_EMPTY_LOOP        3
#endif
#define TASKLET_MAX_RX_CHECK_LOOP       100

/*
 *  DMA RX/TX Channel Parameters
 */
#define MAX_RX_DMA_CHANNEL_NUMBER       4
#define MAX_TX_DMA_CHANNEL_NUMBER       4
#define DMA_ALIGNMENT                   4

/*
 *  Ethernet Frame Definitions
 */
#define ETH_MAC_HEADER_LENGTH           18
#define ETH_CRC_LENGTH                  4
#define ETH_MIN_FRAME_LENGTH            64
#define ETH_MAX_FRAME_LENGTH            1518

/*
 *  RX Frame Definitions
 */
#define RX_HEAD_MAC_ADDR_ALIGNMENT      2
#define RX_TAIL_CRC_LENGTH              ETH_CRC_LENGTH

/*
 *  TX Frame Definitions
 */
#define MAX_TX_PACKET_ALIGN_BYTES       3
#define MAX_TX_PACKET_PADDING_BYTES     3
#define MIN_TX_PACKET_LENGTH            (ETH_MIN_FRAME_LENGTH - ETH_CRC_LENGTH)

/*
 *  EMA Settings
 */
#define EMA_CMD_BUF_LEN      0x0040
#define EMA_CMD_BASE_ADDR    (0x00001580 << 2)
#define EMA_DATA_BUF_LEN     0x0100
#define EMA_DATA_BASE_ADDR   (0x00001900 << 2)
#define EMA_WRITE_BURST      0x2
#define EMA_READ_BURST       0x2

/*
 *  Bits Operation
 */
#define GET_BITS(x, msb, lsb)           (((x) & ((1 << ((msb) + 1)) - 1)) >> (lsb))
#define SET_BITS(x, msb, lsb, value)    (((x) & ~(((1 << ((msb) + 1)) - 1) ^ ((1 << (lsb)) - 1))) | (((value) & ((1 << (1 + (msb) - (lsb))) - 1)) << (lsb)))

/*
 *  FPI Configuration Bus Register and Memory Address Mapping
 */
#define DANUBE_PPE                      (KSEG1 + 0x1E180000)
#define PP32_DEBUG_REG_ADDR(x)          ((volatile u32*)(DANUBE_PPE + (((x) + 0x0000) << 2)))
#define PPM_INT_REG_ADDR(x)             ((volatile u32*)(DANUBE_PPE + (((x) + 0x0030) << 2)))
#define PP32_INTERNAL_RES_ADDR(x)       ((volatile u32*)(DANUBE_PPE + (((x) + 0x0040) << 2)))
#define PPE_CLOCK_CONTROL_ADDR(x)       ((volatile u32*)(DANUBE_PPE + (((x) + 0x0100) << 2)))
#define CDM_CODE_MEMORY_RAM0_ADDR(x)    ((volatile u32*)(DANUBE_PPE + (((x) + 0x1000) << 2)))
#define CDM_CODE_MEMORY_RAM1_ADDR(x)    ((volatile u32*)(DANUBE_PPE + (((x) + 0x2000) << 2)))
#define PPE_REG_ADDR(x)                 ((volatile u32*)(DANUBE_PPE + (((x) + 0x4000) << 2)))
#define PP32_DATA_MEMORY_RAM1_ADDR(x)   ((volatile u32*)(DANUBE_PPE + (((x) + 0x5000) << 2)))
#define PPM_INT_UNIT_ADDR(x)            ((volatile u32*)(DANUBE_PPE + (((x) + 0x6000) << 2)))
#define PPM_TIMER0_ADDR(x)              ((volatile u32*)(DANUBE_PPE + (((x) + 0x6100) << 2)))
#define PPM_TASK_IND_REG_ADDR(x)        ((volatile u32*)(DANUBE_PPE + (((x) + 0x6200) << 2)))
#define PPS_BRK_ADDR(x)                 ((volatile u32*)(DANUBE_PPE + (((x) + 0x6300) << 2)))
#define PPM_TIMER1_ADDR(x)              ((volatile u32*)(DANUBE_PPE + (((x) + 0x6400) << 2)))
#define SB_RAM0_ADDR(x)                 ((volatile u32*)(DANUBE_PPE + (((x) + 0x8000) << 2)))
#define SB_RAM1_ADDR(x)                 ((volatile u32*)(DANUBE_PPE + (((x) + 0x8400) << 2)))
#define SB_RAM2_ADDR(x)                 ((volatile u32*)(DANUBE_PPE + (((x) + 0x8C00) << 2)))
#define SB_RAM3_ADDR(x)                 ((volatile u32*)(DANUBE_PPE + (((x) + 0x9600) << 2)))
#define QSB_CONF_REG(x)                 ((volatile u32*)(DANUBE_PPE + (((x) + 0xC000) << 2)))

/*
 *  DWORD-Length of Memory Blocks
 */
#define PP32_DEBUG_REG_DWLEN            0x0030
#define PPM_INT_REG_DWLEN               0x0010
#define PP32_INTERNAL_RES_DWLEN         0x00C0
#define PPE_CLOCK_CONTROL_DWLEN         0x0F00
#define CDM_CODE_MEMORY_RAM0_DWLEN      0x1000
#define CDM_CODE_MEMORY_RAM1_DWLEN      0x0800
#define PPE_REG_DWLEN                   0x1000
#define PP32_DATA_MEMORY_RAM1_DWLEN     0x0800
#define PPM_INT_UNIT_DWLEN              0x0100
#define PPM_TIMER0_DWLEN                0x0100
#define PPM_TASK_IND_REG_DWLEN          0x0100
#define PPS_BRK_DWLEN                   0x0100
#define PPM_TIMER1_DWLEN                0x0100
#define SB_RAM0_DWLEN                   0x0400
#define SB_RAM1_DWLEN                   0x0800
#define SB_RAM2_DWLEN                   0x0A00
#define SB_RAM3_DWLEN                   0x0400
#define QSB_CONF_REG_DWLEN              0x0100

/*
 *  Share Buffer Registers
 */
#define SB_MST_SEL                      PPE_REG_ADDR(0x0304)

/*
 *    ETOP MDIO Registers
 */
#define ETOP_MDIO_CFG                   PPE_REG_ADDR(0x0600)
#define ETOP_MDIO_ACC                   PPE_REG_ADDR(0x0601)
#define ETOP_CFG                        PPE_REG_ADDR(0x0602)
#define ETOP_IG_VLAN_COS                PPE_REG_ADDR(0x0603)
#define ETOP_IG_DSCP_COSx(x)            PPE_REG_ADDR(0x0607 - ((x) & 0x03))
#define ETOP_IG_PLEN_CTRL0              PPE_REG_ADDR(0x0608)
//#define ETOP_IG_PLEN_CTRL1              PPE_REG_ADDR(0x0609)
#define ETOP_ISR                        PPE_REG_ADDR(0x060A)
#define ETOP_IER                        PPE_REG_ADDR(0x060B)
#define ETOP_VPID                       PPE_REG_ADDR(0x060C)
#define ENET_MAC_CFG(i)                 PPE_REG_ADDR(0x0610 + ((i) ? 0x40 : 0x00))
#define ENETS_DBA(i)                    PPE_REG_ADDR(0x0612 + ((i) ? 0x40 : 0x00))
#define ENETS_CBA(i)                    PPE_REG_ADDR(0x0613 + ((i) ? 0x40 : 0x00))
#define ENETS_CFG(i)                    PPE_REG_ADDR(0x0614 + ((i) ? 0x40 : 0x00))
#define ENETS_PGCNT(i)                  PPE_REG_ADDR(0x0615 + ((i) ? 0x40 : 0x00))
#define ENETS_PKTCNT(i)                 PPE_REG_ADDR(0x0616 + ((i) ? 0x40 : 0x00))
#define ENETS_BUF_CTRL(i)               PPE_REG_ADDR(0x0617 + ((i) ? 0x40 : 0x00))
#define ENETS_COS_CFG(i)                PPE_REG_ADDR(0x0618 + ((i) ? 0x40 : 0x00))
#define ENETS_IGDROP(i)                 PPE_REG_ADDR(0x0619 + ((i) ? 0x40 : 0x00))
#define ENETS_IGERR(i)                  PPE_REG_ADDR(0x061A + ((i) ? 0x40 : 0x00))
#define ENETS_MAC_DA0(i)                PPE_REG_ADDR(0x061B + ((i) ? 0x40 : 0x00))
#define ENETS_MAC_DA1(i)                PPE_REG_ADDR(0x061C + ((i) ? 0x40 : 0x00))
#define ENETF_DBA(i)                    PPE_REG_ADDR(0x0630 + ((i) ? 0x40 : 0x00))
#define ENETF_CBA(i)                    PPE_REG_ADDR(0x0631 + ((i) ? 0x40 : 0x00))
#define ENETF_CFG(i)                    PPE_REG_ADDR(0x0632 + ((i) ? 0x40 : 0x00))
#define ENETF_PGCNT(i)                  PPE_REG_ADDR(0x0633 + ((i) ? 0x40 : 0x00))
#define ENETF_PKTCNT(i)                 PPE_REG_ADDR(0x0634 + ((i) ? 0x40 : 0x00))
#define ENETF_HFCTRL(i)                 PPE_REG_ADDR(0x0635 + ((i) ? 0x40 : 0x00))
#define ENETF_TXCTRL(i)                 PPE_REG_ADDR(0x0636 + ((i) ? 0x40 : 0x00))
#define ENETF_VLCOS0(i)                 PPE_REG_ADDR(0x0638 + ((i) ? 0x40 : 0x00))
#define ENETF_VLCOS1(i)                 PPE_REG_ADDR(0x0639 + ((i) ? 0x40 : 0x00))
#define ENETF_VLCOS2(i)                 PPE_REG_ADDR(0x063A + ((i) ? 0x40 : 0x00))
#define ENETF_VLCOS3(i)                 PPE_REG_ADDR(0x063B + ((i) ? 0x40 : 0x00))
#define ENETF_EGCOL(i)                  PPE_REG_ADDR(0x063C + ((i) ? 0x40 : 0x00))
#define ENETF_EGDROP(i)                 PPE_REG_ADDR(0x063D + ((i) ? 0x40 : 0x00))
//#define ENET_MAC_DA0(i)                 PPE_REG_ADDR(0x063E + ((i) ? 0x40 : 0x00))
//#define ENET_MAC_DA1(i)                 PPE_REG_ADDR(0x063F + ((i) ? 0x40 : 0x00))

/*
 *  DPlus Registers
 */
#define DPLUS_RXCFG                     PPE_REG_ADDR(0x0712)
#define DPLUS_RXPGCNT                   PPE_REG_ADDR(0x0713)

/*
 *  EMA Registers
 */
#define EMA_CMDCFG                      PPE_REG_ADDR(0x0A00)
#define EMA_DATACFG                     PPE_REG_ADDR(0x0A01)
#define EMA_CMDCNT                      PPE_REG_ADDR(0x0A02)
#define EMA_DATACNT                     PPE_REG_ADDR(0x0A03)
#define EMA_ISR                         PPE_REG_ADDR(0x0A04)
#define EMA_IER                         PPE_REG_ADDR(0x0A05)
#define EMA_CFG                         PPE_REG_ADDR(0x0A06)
#define EMA_SUBID                       PPE_REG_ADDR(0x0A07)

/*
 *  Host-PPE Communication Data Address Mapping
 */
#define CFG_WAN_WRDES_DELAY             PPE_REG_ADDR(0x1104)
#define CFG_ERX_DMACH_ON                PPE_REG_ADDR(0x110D)
#define CFG_ETX_DMACH_ON                PPE_REG_ADDR(0x110E)
#define CFG_ETH_DMA_MODE                PPE_REG_ADDR(0x110F)
#define CFG_ETH_DMA_POLL_BASE           PPE_REG_ADDR(0x1110)
#define CFG_ETH_RXDMA_POLL_CNT          PPE_REG_ADDR(0x1111)
#define CFG_ETH_TXDMA_POLL_CNT          PPE_REG_ADDR(0x1112)
#define CFG_LOOPBACK                    PPE_REG_ADDR(0x1113)
#define ETH_MIB_TABLE                   ((struct eth_mib_table*)PPE_REG_ADDR(0x11B0))
#define ERX_DMA_CHANNEL_CONFIG(i)       ((struct erx_dma_channel_config*)PPE_REG_ADDR(0x1130 + (i) * 7))
#define ETX_DMA_CHANNEL_CONFIG(i)       ((struct etx_dma_channel_config*)PPE_REG_ADDR(0x1150 + (i) * 7))

/*
 *  PP32 Debug Control Register
 */
#define PP32_DBG_CTRL                   PP32_DEBUG_REG_ADDR(0x0000)

#define DBG_CTRL_START_SET(value)       ((value) ? (1 << 0) : 0)
#define DBG_CTRL_STOP_SET(value)        ((value) ? (1 << 1) : 0)
#define DBG_CTRL_STEP_SET(value)        ((value) ? (1 << 2) : 0)

/*
 *    Code/Data Memory (CDM) Interface Configuration Register
 */
#define CDM_CFG                         PPE_REG_ADDR(0x0100)

#define CDM_CFG_RAM1                    GET_BITS(*CDM_CFG, 3, 2)
#define CDM_CFG_RAM0                    (*CDM_CFG & (1 << 1))

#define CDM_CFG_RAM1_SET(value)         SET_BITS(0, 3, 2, value)
#define CDM_CFG_RAM0_SET(value)         ((value) ? (1 << 1) : 0)

/*
 *  ETOP MDIO Configuration Register
 */
#define ETOP_MDIO_CFG_SMRST(value)      ((value) ? (1 << 13) : 0)
#define ETOP_MDIO_CFG_PHYA(i, value)    ((i) ? SET_BITS(0, 12, 8, (value)) : SET_BITS(0, 7, 3, (value)))
#define ETOP_MDIO_CFG_UMM(i, value)     ((value) ? ((i) ? (1 << 2) : (1 << 1)) : 0)

#define ETOP_MDIO_CFG_MASK(i)           (ETOP_MDIO_CFG_SMRST(1) | ETOP_MDIO_CFG_PHYA(i, 0x1F) | ETOP_MDIO_CFG_UMM(i, 1))

/*
 *  ETOP Configuration Register
 */
#define ETOP_CFG_MII1_OFF(value)        ((value) ? (1 << 3) : 0)
#define ETOP_CFG_REV_MII1_ON(value)     ((value) ? (1 << 4) : 0)
#define ETOP_CFG_TURBO_MII1_ON(value)   ((value) ? (1 << 5) : 0)
#define ETOP_CFG_SEN1_ON(value)         ((value) ? (1 << 7) : 0)
#define ETOP_CFG_FEN1_ON(value)         ((value) ? (1 << 9) : 0)
#define ETOP_CFG_TCKINV1_ON(value)      ((value) ? (1 << 11) : 0)

#define ETOP_CFG_MII1_MASK              (ETOP_CFG_MII1_OFF(1) | ETOP_CFG_REV_MII1_ON(1) | ETOP_CFG_TURBO_MII1_ON(1) | ETOP_CFG_SEN1_ON(1) | ETOP_CFG_FEN1_ON(1))

/*
 *  ETOP IG VLAN Priority CoS Mapping
 */
#define ETOP_IG_VLAN_COS_Px_SET(org, x, value)   \
                                        SET_BITS((org), ((x) << 1) + 1, (x) << 1, (value))

/*
 *  ETOP IG DSCP CoS Mapping Register x
 */
#define ETOP_IG_DSCP_COS_SET(x, value)  do {        \
                                            *ETOP_IG_DSCP_COSx((x) >> 4) = SET_BITS(*ETOP_IG_DSCP_COSx((x) >> 4), (((x) & 0x0F) << 1) + 1, ((x) & 0x0F) << 1, (value)); \
                                        } while ( 0 )

/*
 * ETOP Ingress Packet Length Control 0
 */
#define ETOP_IG_PLEN_CTRL0_UNDER_SET(value)     \
                                        (((value) & 0x7F) << 16)
#define ETOP_IG_PLEN_CTRL0_OVER_SET(value)      \
                                        ((value) & 0x3FFF)


/*
 *  ENet MAC Configuration Register
 */
#define ENET_MAC_CFG_CRC(i)             (*ENET_MAC_CFG(i) & (0x01 << 11))
#define ENET_MAC_CFG_DUPLEX(i)          (*ENET_MAC_CFG(i) & (0x01 << 2))
#define ENET_MAC_CFG_SPEED(i)           (*ENET_MAC_CFG(i) & (0x01 << 1))
#define ENET_MAC_CFG_LINK(i)            (*ENET_MAC_CFG(i) & 0x01)

#define ENET_MAC_CFG_CRC_OFF(i)         do { *ENET_MAC_CFG(i) &= ~(1 << 11); } while (0)
#define ENET_MAC_CFG_CRC_ON(i)          do { *ENET_MAC_CFG(i) |= 1 << 11; } while (0)
#define ENET_MAC_CFG_DUPLEX_HALF(i)     do { *ENET_MAC_CFG(i) &= ~(1 << 2); } while (0)
#define ENET_MAC_CFG_DUPLEX_FULL(i)     do { *ENET_MAC_CFG(i) |= 1 << 2; } while (0)
#define ENET_MAC_CFG_SPEED_10M(i)       do { *ENET_MAC_CFG(i) &= ~(1 << 1); } while (0)
#define ENET_MAC_CFG_SPEED_100M(i)      do { *ENET_MAC_CFG(i) |= 1 << 1; } while (0)
#define ENET_MAC_CFG_LINK_NOT_OK(i)     do { *ENET_MAC_CFG(i) &= ~1; } while (0)
#define ENET_MAC_CFG_LINK_OK(i)         do { *ENET_MAC_CFG(i) |= 1; } while (0)

/*
 *  ENets Configuration Register
 */
#define ENETS_CFG_VL2_SET               (1 << 29)
#define ENETS_CFG_VL2_CLEAR             ~(1 << 29)
#define ENETS_CFG_FTUC_SET              (1 << 28)
#define ENETS_CFG_FTUC_CLEAR            ~(1 << 28)
#define ENETS_CFG_DPBC_SET              (1 << 27)
#define ENETS_CFG_DPBC_CLEAR            ~(1 << 27)
#define ENETS_CFG_DPMC_SET              (1 << 26)
#define ENETS_CFG_DPMC_CLEAR            ~(1 << 26)

/*
 *  ENets Classification Configuration Register
 */
#define ENETS_COS_CFG_VLAN_SET          (1 << 1)
#define ENETS_COS_CFG_VLAN_CLEAR        ~(1 << 1)
#define ENETS_COS_CFG_DSCP_SET          (1 << 0)
#define ENETS_COS_CFG_DSCP_CLEAR        ~(1 << 0)

/*
 *  Mailbox IGU1 Registers
 */
#define MBOX_IGU1_ISRS                  PPE_REG_ADDR(0x0204)
#define MBOX_IGU1_ISRC                  PPE_REG_ADDR(0x0205)
#define MBOX_IGU1_ISR                   PPE_REG_ADDR(0x0206)
#define MBOX_IGU1_IER                   PPE_REG_ADDR(0x0207)

#define MBOX_IGU1_ISRS_SET(n)           (1 << (n))
#define MBOX_IGU1_ISRC_CLEAR(n)         (1 << (n))
#define MBOX_IGU1_ISR_ISR(n)            (*MBOX_IGU1_ISR & (1 << (n)))
#define MBOX_IGU1_IER_EN(n)             (*MBOX_IGU1_IER & (1 << (n)))
#define MBOX_IGU1_IER_EN_SET(n)         (1 << (n))

/*
 *  Mailbox IGU3 Registers
 */
#define MBOX_IGU3_ISRS                  PPE_REG_ADDR(0x0214)
#define MBOX_IGU3_ISRC                  PPE_REG_ADDR(0x0215)
#define MBOX_IGU3_ISR                   PPE_REG_ADDR(0x0216)
#define MBOX_IGU3_IER                   PPE_REG_ADDR(0x0217)

#define MBOX_IGU3_ISRS_SET(n)           (1 << (n))
#define MBOX_IGU3_ISRC_CLEAR(n)         (1 << (n))
#define MBOX_IGU3_ISR_ISR(n)            (*MBOX_IGU3_ISR & (1 << (n)))
#define MBOX_IGU3_IER_EN(n)             (*MBOX_IGU3_IER & (1 << (n)))
#define MBOX_IGU3_IER_EN_SET(n)         (1 << (n))


/*
 * ####################################
 *  Preparation of Hardware Emulation
 * ####################################
 */

#if defined(ENABLE_HARDWARE_EMULATION) && ENABLE_HARDWARE_EMULATION
    u32 g_pFakeRegisters[0xD000] = {0};

  #undef  DANUBE_PPE
  #define DANUBE_PPE                    ((u32)g_pFakeRegisters)
#endif  //  defined(ENABLE_HARDWARE_EMULATION) && ENABLE_HARDWARE_EMULATION


/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*
 *  MIB Table Maintained by Firmware
 */
struct eth_mib_table {
    u32                     erx_dropdes_pdu;
    u32                     erx_pass_pdu;
    u32                     erx_pass_bytes;
    u32                     res1;
    u32                     etx_total_pdu;
    u32                     etx_total_bytes;
};

/*
 *  Host-PPE Communication Data Structure
 */
#if defined(__BIG_ENDIAN)
    struct erx_dma_channel_config {
        /*  0h  */
        unsigned int    res3        :1;
        unsigned int    res4        :2;
        unsigned int    res5        :1;
        unsigned int    desba       :28;
        /*  1h  */
        unsigned int    res1        :16;
        unsigned int    res2        :16;
        /*  2h  */
        unsigned int    deslen      :16;
        unsigned int    vlddes      :16;
    };

    struct etx_dma_channel_config {
        /*  0h  */
        unsigned int    res3        :1;
        unsigned int    res4        :2;
        unsigned int    res5        :1;
        unsigned int    desba       :28;
        /*  1h  */
        unsigned int    res1        :16;
        unsigned int    res2        :16;
        /*  2h  */
        unsigned int    deslen      :16;
        unsigned int    vlddes      :16;
    };
#else   //  defined(__BIG_ENDIAN)
    struct erx_dma_channel_config {
        /*  0h  */
        unsigned int    desba       :28;
        unsigned int    res5        :1;
        unsigned int    res4        :2;
        unsigned int    res3        :1;
        /*  1h  */
        unsigned int    res2        :16;
        unsigned int    res1        :16;
        /*  2h  */
        unsigned int    vlddes      :16;
        unsigned int    deslen      :16;
    };

    struct etx_dma_channel_config {
        /*  0h  */
        unsigned int    desba       :28;
        unsigned int    res5        :1;
        unsigned int    res4        :2;
        unsigned int    res3        :1;
        /*  1h  */
        unsigned int    res2        :16;
        unsigned int    res1        :16;
        /*  2h  */
        unsigned int    vlddes      :16;
        unsigned int    deslen      :16;
    };
#endif  //  defined(__BIG_ENDIAN)

/*
 *  Descirptor
 */
#if defined(__BIG_ENDIAN)
    struct rx_descriptor {
        /*  0 - 3h  */
        unsigned int    own         :1;
        unsigned int    c           :1;
        unsigned int    sop         :1;
        unsigned int    eop         :1;
        unsigned int    res1        :3;
        unsigned int    byteoff     :2;
        unsigned int    res2        :2;
        unsigned int    id          :4;
        unsigned int    err         :1;
        unsigned int    datalen     :16;
        /*  4 - 7h  */
        unsigned int    res3        :4;
        unsigned int    dataptr     :28;
    };

    struct tx_descriptor {
        /*  0 - 3h  */
        unsigned int    own         :1;
        unsigned int    c           :1;
        unsigned int    sop         :1;
        unsigned int    eop         :1;
        unsigned int    byteoff     :5;
        unsigned int    res1        :5;
        unsigned int    iscell      :1;
        unsigned int    clp         :1;
        unsigned int    datalen     :16;
        /*  4 - 7h  */
        unsigned int    res2        :4;
        unsigned int    dataptr     :28;
    };
#else
    struct rx_descriptor {
        /*  4 - 7h  */
        unsigned int    dataptr     :28;
        unsigned int    res3        :4;
        /*  0 - 3h  */
        unsigned int    datalen     :16;
        unsigned int    err         :1;
        unsigned int    id          :4;
        unsigned int    res2        :2;
        unsigned int    byteoff     :2;
        unsigned int    res1        :3;
        unsigned int    eop         :1;
        unsigned int    sop         :1;
        unsigned int    c           :1;
        unsigned int    own         :1;
    };

    struct tx_descriptor {
        /*  4 - 7h  */
        unsigned int    dataptr     :28;
        unsigned int    res2        :4;
        /*  0 - 3h  */
        unsigned int    datalen     :16;
        unsigned int    clp         :1;
        unsigned int    iscell      :1;
        unsigned int    res1        :5;
        unsigned int    byteoff     :5;
        unsigned int    eop         :1;
        unsigned int    sop         :1;
        unsigned int    c           :1;
        unsigned int    own         :1;
    };
#endif  //  defined(__BIG_ENDIAN)

/*
 *  Eth2 Structure
 */
struct eth2_dev {
    u32                 rx_buffer_size;             /*  max memory allocated for a RX packet    */

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    u32                 rx_descriptor_number;       /*  number of RX descriptors                */
#endif
    u32                 tx_descriptor_number;       /*  number of TX descriptors                */

    u32                 write_descriptor_delay;     /*  delay on descriptor write path          */

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    void                *rx_descriptor_addr;        /*  base address of memory allocated for    */
                                                    /*  RX descriptors                          */
    struct rx_descriptor
                        *rx_descriptor_base;        /*  base address of RX descriptors          */
    struct rx_descriptor
                        *rx_desc_ch_base[RX_TOTAL_CHANNEL_USED];
    int                 rx_desc_read_pos[RX_TOTAL_CHANNEL_USED];        /*  first RX descriptor */
                                                                        /*  to be read          */
#else
    struct dma_device_info
                        *rx_dma_device;
#endif

    void                *tx_descriptor_addr;        /*  base address of memory allocated for    */
                                                    /*  TX descriptors                          */
    struct tx_descriptor
                        *tx_descriptor_base;        /*  base address of TX descriptors          */
    int                 tx_desc_alloc_pos[TX_TOTAL_CHANNEL_USED];       /*  first TX descriptor */
                                                                        /*  could be allocated  */
    struct sk_buff      **tx_skb_pointers;          /*  base address of TX sk_buff pointers     */

    u32                 rx_drop_counter;            /*  number of dropped RX packet             */
    struct net_device_stats
                        stats;                      /*  net device stats                        */

#if defined(ETH2_HW_FLOWCONTROL) && ETH2_HW_FLOWCONTROL
    int                 fc_bit;                     /*  net wakeup callback                     */
#endif

    u32                 rx_irq;                     /*  RX channel IRQ enabled                  */
    u32                 tx_irq;                     /*  TX channel IRQ enabled                  */
    int                 irq_handling_flag;
#if 0
    int                 empty_loop;                 /*  tasklet empty loop counter              */
#endif

#if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
    u32                 rx_success;                 /*  number of packet succeeded to push up   */
    u32                 rx_fail;                    /*  number of packet failed in pushing up   */
    u32                 tx_success;                 /*  number of packet succeeded to transmit  */

    u32                 rx_driver_level_error;      /*  number of rx error at driver level      */
    u32                 rx_driver_level_drop;       /*  number of rx drop at driver level       */
    u32                 tx_driver_level_drop;       /*  number of tx drop at driver level       */

    u32                 rx_desc_update_wait_loop;
    u32                 tx_desc_update_wait_loop;
#endif  //  defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
};


/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  Network Operations
 */
static int eth2_init(struct net_device *);
static struct net_device_stats *eth2_get_stats(struct net_device *);
static int eth2_open(struct net_device *);
static int eth2_stop(struct net_device *);
static int eth2_hard_start_xmit(struct sk_buff *, struct net_device *);
static int eth2_set_mac_address(struct net_device *, void *);
static int eth2_ioctl(struct net_device *, struct ifreq *, int);
static int eth2_change_mtu(struct net_device *, int);
static void eth2_tx_timeout(struct net_device *);

/*
 *  Channel Determination
 */
static INLINE int pick_up_tx_channel(struct sk_buff *, struct net_device *);

/*
 *  Buffer manage functions
 */
static INLINE struct sk_buff* alloc_skb_rx(void);
static INLINE struct sk_buff* alloc_skb_tx(unsigned int);
#if 0
  #if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    static INLINE void reset_skb_rx(struct sk_buff *);
  #endif
#endif
static INLINE int alloc_tx_channel(int, int *);

/*
 *  Mailbox handler and signal function
 */
#if defined(ETH2_HW_FLOWCONTROL) && ETH2_HW_FLOWCONTROL
  static void eth2_xon(struct net_device *);
#endif
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
  static void do_eth2_tasklet(unsigned long);
  static INLINE int mailbox_rx_irq_handler(unsigned int);
#endif
static void mailbox_irq_handler(int, void *, struct pt_regs *);
static INLINE void mailbox_signal(unsigned int, int);

#if defined(ENABLE_RX_DPLUS_PATH) && ENABLE_RX_DPLUS_PATH
/*
 *  DPlus & DMA related functions
 */
  static u8* dma_buffer_alloc(int, int *, void **);
  static int dma_buffer_free(u8 *, void *);
  static int dma_int_handler(struct dma_device_info *, int);
  static INLINE int dma_rx_int_handler(struct dma_device_info *);
#endif

/*
 *  ioctl help functions
 */
static INLINE int ethtool_ioctl(struct net_device *, struct ifreq *);
static INLINE void set_vlan_cos(struct vlan_cos_req *);
static INLINE void set_dscp_cos(struct dscp_cos_req *);

/*
 *  Init & clean-up functions
 */
#if defined(ENABLE_PROBE_TRANSCEIVER) && ENABLE_PROBE_TRANSCEIVER
  static INLINE int probe_transceiver(struct net_device *);
#endif
static INLINE void check_parameters(void);
static INLINE int init_eth2_dev(void);
static INLINE void clear_share_buffer(void);
static INLINE void init_tables(void);
static INLINE void clear_eth2_dev(void);

/*
 *  PP32 specific init functions
 */
static INLINE void init_ema(void);
static INLINE void init_chip(int);
static INLINE int pp32_download_code(u32 *, unsigned int, u32 *, unsigned int);
static INLINE int pp32_specific_init(void *);
static INLINE int pp32_start(void);
static INLINE void pp32_stop(void);

/*
 *  Proc File
 */
static INLINE void proc_file_create(void);
static INLINE void proc_file_delete(void);
static int proc_read_stats(char *, char **, off_t, int, int *, void *);

/*
 *  Debug functions
 */
#if (defined(DEBUG_DUMP_RX_SKB) && DEBUG_DUMP_RX_SKB) || (defined(DEBUG_DUMP_TX_SKB) && DEBUG_DUMP_TX_SKB)
  static INLINE void dump_skb(struct sk_buff *, int);
#endif  //  (defined(DEBUG_DUMP_RX_SKB) && DEBUG_DUMP_RX_SKB) || (defined(DEBUG_DUMP_TX_SKB) && DEBUG_DUMP_TX_SKB)
#if defined(DEBUG_DUMP_ETOP_REGISTER) && DEBUG_DUMP_ETOP_REGISTER
  static INLINE void dump_etop0_reg(void);
  static INLINE void dump_etop1_reg(void);
  static INLINE void dump_etop_reg(void);
#endif  //  defined(DEBUG_DUMP_ETOP_REGISTER) && DEBUG_DUMP_ETOP_REGISTER

#if defined(ENABLE_DIRECT_BRIDGE) && ENABLE_DIRECT_BRIDGE
  extern int switch_tx(struct sk_buff *, struct net_device *);
#endif  //  defined(ENABLE_DIRECT_BRIDGE) && ENABLE_DIRECT_BRIDGE


/*
 * ####################################
 *            Local Variable
 * ####################################
 */

static struct net_device eth2_net_dev = {
    name:   "eth2",
    init:   eth2_init,
};

static struct eth2_dev eth2_dev;

static u8 my_ethaddr[MAX_ADDR_LEN] = {0x01, 0x02, 0x03, 0x04, 0xDB, 0x30, 0x00, 0x00};

static struct proc_dir_entry *g_eth2_proc_dir;

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
  static DECLARE_TASKLET(eth2_tasklet, do_eth2_tasklet, 0);
#endif

#if defined(ENABLE_DIRECT_BRIDGE) && ENABLE_DIRECT_BRIDGE
  struct net_device *eth2_switch_dev;
  extern struct net_device *switch_eth2_dev;
#endif  //  defined(ENABLE_DIRECT_BRIDGE) && ENABLE_DIRECT_BRIDGE


/*
 * ####################################
 *           Global Variable
 * ####################################
 */


/*
 * ####################################
 *            Local Function
 * ####################################
 */

static int eth2_init(struct net_device *dev)
{
    u64 retval;
    int i;

//    dbg("invoked");

    ether_setup(dev);   /*  assign some members */

    /*  hook network operations */
    dev->get_stats       = eth2_get_stats;
    dev->open            = eth2_open;
    dev->stop            = eth2_stop;
    dev->hard_start_xmit = eth2_hard_start_xmit;
    dev->set_mac_address = eth2_set_mac_address;
    dev->do_ioctl        = eth2_ioctl;
    dev->change_mtu      = eth2_change_mtu;
    dev->tx_timeout      = eth2_tx_timeout;
    dev->watchdog_timeo  = ETH2_WATCHDOG_TIMEOUT;
    dev->priv            = &eth2_dev;

    SET_MODULE_OWNER(dev);

    /*  read MAC address from the MAC table and put them into device    */
    retval = 0;
    for ( i = 0; i < 6; i++ )
        retval += MY_ETHADDR[i];
    if ( retval == 0 )
    {
        /*  ethaddr not set in u-boot   */
        dev->dev_addr[0] = 0x00;
		dev->dev_addr[1] = 0x20;
		dev->dev_addr[2] = 0xda;
		dev->dev_addr[3] = 0x86;
		dev->dev_addr[4] = 0x23;
		dev->dev_addr[5] = 0x74 + 1;    /*  eth2    */
	}
	else
	{
	    for ( i = 0; i < 6; i++ )
	        dev->dev_addr[i] = MY_ETHADDR[i];
	    dev->dev_addr[5] += 1;          /*  eth2    */
	}

	return 0;   //  Qi Ming set 1 here in Switch driver
}

static struct net_device_stats *eth2_get_stats(struct net_device *dev)
{
//    dbg("invoked");

    if ( dev->priv != &eth2_dev )
        return NULL;

    eth2_dev.rx_drop_counter  += *ENETS_IGDROP(1);

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    eth2_dev.stats.rx_packets  = ETH_MIB_TABLE->erx_pass_pdu;
    eth2_dev.stats.rx_bytes    = ETH_MIB_TABLE->erx_pass_bytes;
    eth2_dev.stats.rx_errors  += *ENETS_IGERR(1);
    eth2_dev.stats.rx_dropped  = ETH_MIB_TABLE->erx_dropdes_pdu + eth2_dev.rx_drop_counter;
#else
    eth2_dev.stats.rx_errors  += *ENETS_IGERR(1);
    eth2_dev.stats.rx_dropped += eth2_dev.rx_drop_counter;
#endif

    eth2_dev.stats.tx_packets  = ETH_MIB_TABLE->etx_total_pdu;
    eth2_dev.stats.tx_bytes    = ETH_MIB_TABLE->etx_total_bytes;
    eth2_dev.stats.tx_errors  += *ENETF_EGCOL(1);               /*  eth2_dev.stats.tx_errors is also updated in function eth2_hard_start_xmit   */
    eth2_dev.stats.tx_dropped += *ENETF_EGDROP(1);              /*  eth2_dev.stats.tx_dropped is updated in function eth2_hard_start_xmit       */

    return &eth2_dev.stats;
}

static int eth2_open(struct net_device *dev)
{
    dbg("invoked");

    MOD_INC_USE_COUNT;

#if defined(ENABLE_PROBE_TRANSCEIVER) && ENABLE_PROBE_TRANSCEIVER
    if ( !probe_transceiver(dev) )
    {
        printk("%s cannot work because of hardware problem\n", dev->name);
        MOD_DEC_USE_COUNT;
        return -1;
    }
#endif  //  defined(ENABLE_PROBE_TRANSCEIVER) && ENABLE_PROBE_TRANSCEIVER
    dbg(" %s", dev->name);

#if defined(ETH2_HW_FLOWCONTROL) && ETH2_HW_FLOWCONTROL
    if ( (eth2_dev.fc_bit = netdev_register_fc(dev, eth2_xon)) == 0 )
    {
        printk("Hardware Flow Control register fails\n");
    }
#endif  //  defined(ETH2_HW_FLOWCONTROL) && ETH2_HW_FLOWCONTROL

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    //  enable RX DMA
    *CFG_ERX_DMACH_ON = (1 << RX_TOTAL_CHANNEL_USED) - 1;
#else
    {
        int i;

        ETH2_ASSERT((u32)eth2_dev.rx_dma_device >= 0x80000000, "eth2_dev.rx_dma_device = 0x%08X", (u32)eth2_dev.rx_dma_device);
        for ( i = 0; i < eth2_dev.rx_dma_device->max_rx_chan_num; i++ )
        {
            ETH2_ASSERT((u32)eth2_dev.rx_dma_device->rx_chan[i] >= 0x80000000, "eth2_dev.rx_dma_device->rx_chan[%d] = 0x%08X", i, (u32)eth2_dev.rx_dma_device->rx_chan[i]);
            ETH2_ASSERT(eth2_dev.rx_dma_device->rx_chan[i]->control == DANUBE_DMA_CH_ON, "eth2_dev.rx_dma_device->rx_chan[i]->control = %d", eth2_dev.rx_dma_device->rx_chan[i]->control);

            if ( eth2_dev.rx_dma_device->rx_chan[i]->control == DANUBE_DMA_CH_ON )
            {
                ETH2_ASSERT((u32)eth2_dev.rx_dma_device->rx_chan[i]->open >= 0x80000000, "eth2_dev.rx_dma_device->rx_chan[%d]->open = 0x%08X", i, (u32)eth2_dev.rx_dma_device->rx_chan[i]->open);
                eth2_dev.rx_dma_device->rx_chan[i]->open(eth2_dev.rx_dma_device->rx_chan[i]);
            }
        }
    }
#endif

    netif_start_queue(dev);

    return 0;
}

static int eth2_stop(struct net_device *dev)
{
    dbg("invoked");

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    //  disable RX DMA
    *CFG_ERX_DMACH_ON = 0x00;
#else
    {
        int i;

        for ( i = 0; i < eth2_dev.rx_dma_device->max_rx_chan_num; i++ )
            eth2_dev.rx_dma_device->rx_chan[i]->close(eth2_dev.rx_dma_device->rx_chan[i]);
    }
#endif

#if defined(ETH2_HW_FLOWCONTROL) && ETH2_HW_FLOWCONTROL
    if ( eth2_dev.fc_bit )
        netdev_unregister_fc(eth2_dev.fc_bit);
#endif  //  defined(ETH2_HW_FLOWCONTROL) && ETH2_HW_FLOWCONTROL

    netif_stop_queue(dev);
    MOD_DEC_USE_COUNT;

    return 0;
}

static int eth2_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    int ret;
    int ch;
    int f_full;
    int desc_base;
#if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    register struct tx_descriptor reg_desc;
#else
    struct tx_descriptor reg_desc;
#endif
    struct tx_descriptor *desc;

#if defined(ENABLE_LOOPBACK_TESTING_TX2RX) && ENABLE_LOOPBACK_TESTING_TX2RX
    if ( dev )
    {
        unsigned char ipaddr[6];

  #if defined(DEBUG_DUMP_TX_SKB) && DEBUG_DUMP_TX_SKB
        dump_skb(skb, 0);
  #endif    //  defined(DEBUG_DUMP_TX_SKB) && DEBUG_DUMP_TX_SKB

        if ( *(unsigned long *)skb->data != 0xffffffff || *((unsigned short *)skb->data + 2) != 0xffff )
        {
            memcpy(ipaddr, skb->data, 6);
            memcpy(skb->data, (unsigned char *)skb->data + 6, 6);
            memcpy((unsigned char *)skb->data + 6, ipaddr, 6);
        }

  #if defined(DEBUG_DUMP_RX_SKB_BEFORE) && DEBUG_DUMP_RX_SKB_BEFORE
        dump_skb(skb, 1);
  #endif    //  defined(DEBUG_DUMP_RX_SKB_BEFORE) && DEBUG_DUMP_RX_SKB_BEFORE

        skb->dev = &eth2_net_dev;
        skb->protocol = eth_type_trans(skb, &eth2_net_dev);

  #if defined(DEBUG_DUMP_RX_SKB_AFTER) && DEBUG_DUMP_RX_SKB_AFTER
        dump_skb(skb, 1);
  #endif    //  defined(DEBUG_DUMP_RX_SKB_AFTER) && DEBUG_DUMP_RX_SKB_AFTER

        if ( netif_rx(skb) == NET_RX_DROP )
        {
  #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
                eth2_dev.rx_fail++;
  #endif    //  defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
        }
        else
        {
  #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
                eth2_dev.rx_success++;
  #endif    //  defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
        }

        eth2_net_dev.trans_start = jiffies;

  #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
        eth2_dev.tx_success++;
  #endif    //  defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER

        return 0;
    }
#endif  //  defined(ENABLE_LOOPBACK_TESTING_TX2RX) && ENABLE_LOOPBACK_TESTING_TX2RX

#if defined(ENABLE_LOOPBACK_TESTING_RX2TX) && ENABLE_LOOPBACK_TESTING_RX2TX
    if ( !dev )
        dev = &eth2_net_dev;
#endif

    /*  By default, channel 0 is used to transmit data. If want to take full  *
     *  advantage of 4 channels, please implement a strategy to select a      *
     *  channel for transmission.                                             */
    ch = pick_up_tx_channel(skb, dev);

    /*  allocate descriptor */
    desc_base = alloc_tx_channel(ch, &f_full);
    if ( desc_base < 0 )
    {
        u32 sys_flag;

        dev->trans_start = jiffies;
        netif_stop_queue(&eth2_net_dev);

        local_irq_save(sys_flag);
        eth2_dev.tx_irq |= 1 << ch;
        *MBOX_IGU1_ISRC = 1 << (ch + 16);
        *MBOX_IGU1_IER = (eth2_dev.tx_irq << 16) | eth2_dev.rx_irq;
        local_irq_restore(sys_flag);


#if 0
        ret = -EIO;
#else
        ret = 0;
#endif
        goto ALLOC_TX_CONNECTION_FAIL;
    }
    else if ( f_full )
    {
        u32 sys_flag;

        dev->trans_start = jiffies;
        netif_stop_queue(&eth2_net_dev);

        local_irq_save(sys_flag);
        eth2_dev.tx_irq |= 1 << ch;
        *MBOX_IGU1_ISRC = 1 << (ch + 16);
        *MBOX_IGU1_IER = (eth2_dev.tx_irq << 16) | eth2_dev.rx_irq;
        local_irq_restore(sys_flag);
    }

    if ( eth2_dev.tx_skb_pointers[desc_base] )
        dev_kfree_skb_any(eth2_dev.tx_skb_pointers[desc_base]);

    desc = &eth2_dev.tx_descriptor_base[desc_base];

    /*  load descriptor from memory */
    reg_desc = *desc;

    /*  if data pointer is not aligned, allocate new sk_buff    */
    if ( ((u32)skb->data & ~(DMA_ALIGNMENT - 1)) < (u32)skb->head )
    {
        struct sk_buff *new_skb;

        dbg("skb->data is not aligned");

        new_skb = alloc_skb_tx(skb->len);
        if ( new_skb == NULL )
        {
#if 0
            ret = -ENOMEM;
#else
            ret = 0;
#endif
            eth2_dev.tx_skb_pointers[desc_base] = NULL;
            goto ALLOC_SKB_TX_FAIL;
        }
        skb_put(new_skb, skb->len);
        memcpy(new_skb->data, skb->data, skb->len);
        dev_kfree_skb_any(skb);
        skb = new_skb;
    }

    /*  if packet length is less than 60, pad it to 60 bytes    */
//    if ( skb->len < MIN_TX_PACKET_LENGTH )
//        skb_put(skb, MIN_TX_PACKET_LENGTH - skb->len);
    ETH2_ASSERT(skb->len >= MIN_TX_PACKET_LENGTH, "packet length (%d) is less than 60 bytes\n", skb->len);

    reg_desc.dataptr = (u32)skb->data >> 2;
    reg_desc.datalen = skb->len < MIN_TX_PACKET_LENGTH ? MIN_TX_PACKET_LENGTH : skb->len;
    reg_desc.byteoff = (u32)skb->data & (DMA_ALIGNMENT - 1);
    reg_desc.own     = 1;
    reg_desc.c       = 1;

    /*  update descriptor send pointer  */
    eth2_dev.tx_skb_pointers[desc_base] = skb;

#if defined(DEBUG_DUMP_TX_SKB) && DEBUG_DUMP_TX_SKB
    dump_skb(skb, 0);
#endif  //  defined(DEBUG_DUMP_TX_SKB) && DEBUG_DUMP_TX_SKB

    /*  write discriptor to memory and write back cache */
#if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    *desc = reg_desc;
    dma_cache_wback((unsigned long)skb->data, skb->len);
#else
    dma_cache_wback((unsigned long)skb->data, skb->len);
    *((u32*)desc + 1) = *((u32*)(&reg_desc) + 1);
    *(u32*)desc = *(u32*)(&reg_desc);

  #if 0
    if ( reg_desc.dataptr == 0 )
        printk("reg_desc.dataptr == 0\n");
    if ( *((u32*)desc + 1) == 0 )
        printk("*((u32*)desc + 1) == 0\n");
  #endif
#endif

    /*  signal firmware */
    dev->trans_start = jiffies;
#if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    mailbox_signal(ch, 1);
#endif

#if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
    eth2_dev.tx_success++;
#endif  //  defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER

    return 0;

ALLOC_SKB_TX_FAIL:
    dbg("ALLOC_SKB_TX_FAIL");
ALLOC_TX_CONNECTION_FAIL:
    dev_kfree_skb_any(skb);
//    eth2_dev.stats.tx_errors++;
    eth2_dev.stats.tx_dropped++;
    return ret;
}

static int eth2_set_mac_address(struct net_device *dev, void *p)
{
    struct sockaddr *addr = (struct sockaddr *)p;

    dbg("invoked");

    printk("%s: change MAC from %02X:%02X:%02X:%02X:%02X:%02X to %02X:%02X:%02X:%02X:%02X:%02X\n", dev->name,
        dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2], dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5],
        addr->sa_data[0], addr->sa_data[1], addr->sa_data[2], addr->sa_data[3], addr->sa_data[4], addr->sa_data[5]);

    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

    return 0;
}

static int eth2_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    dbg("invoked");

    switch ( cmd )
    {
    case SIOCETHTOOL:
        return ethtool_ioctl(dev, ifr);
    case SET_VLAN_COS:
        {
            struct vlan_cos_req vlan_cos_req;

            if ( copy_from_user(&vlan_cos_req, ifr->ifr_data, sizeof(struct vlan_cos_req)) )
                return -EFAULT;
            set_vlan_cos(&vlan_cos_req);
        }
        break;
    case SET_DSCP_COS:
        {
            struct dscp_cos_req dscp_cos_req;

            if ( copy_from_user(&dscp_cos_req, ifr->ifr_data, sizeof(struct dscp_cos_req)) )
                return -EFAULT;
            set_dscp_cos(&dscp_cos_req);
        }
        break;
    case ENABLE_VLAN_CLASSIFICATION:
        *ENETS_COS_CFG(1) |= ENETS_COS_CFG_VLAN_SET;    break;
    case DISABLE_VLAN_CLASSIFICATION:
        *ENETS_COS_CFG(1) &= ENETS_COS_CFG_VLAN_CLEAR;  break;
    case ENABLE_DSCP_CLASSIFICATION:
        *ENETS_COS_CFG(1) |= ENETS_COS_CFG_DSCP_SET;    break;
    case DISABLE_DSCP_CLASSIFICATION:
        *ENETS_COS_CFG(1) &= ENETS_COS_CFG_DSCP_CLEAR;  break;
    case VLAN_CLASS_FIRST:
        *ENETS_CFG(1) &= ENETS_CFG_FTUC_CLEAR;          break;
    case VLAN_CLASS_SECOND:
        *ENETS_CFG(1) |= ENETS_CFG_VL2_SET;             break;
    case PASS_UNICAST_PACKETS:
        *ENETS_CFG(1) &= ENETS_CFG_FTUC_CLEAR;          break;
    case FILTER_UNICAST_PACKETS:
        *ENETS_CFG(1) |= ENETS_CFG_FTUC_SET;            break;
    case KEEP_BROADCAST_PACKETS:
        *ENETS_CFG(1) &= ENETS_CFG_DPBC_CLEAR;          break;
    case DROP_BROADCAST_PACKETS:
        *ENETS_CFG(1) |= ENETS_CFG_DPBC_SET;            break;
    case KEEP_MULTICAST_PACKETS:
         *ENETS_CFG(1) &= ENETS_CFG_DPMC_CLEAR;         break;
    case DROP_MULTICAST_PACKETS:
        *ENETS_CFG(1) |= ENETS_CFG_DPMC_SET;
    default:
        return -EOPNOTSUPP;
    }

    return 0;
}

static int eth2_change_mtu(struct net_device *dev, int new_mtu)
{
    dbg("not implemented");

    /*  not implemented */
    return 0;
}

static void eth2_tx_timeout(struct net_device *dev)
{
    u32 sys_flag;

    dbg("invoked");

    ETH2_ASSERT(dev == &eth2_net_dev, "dev != &eth2_net_dev");

    /*  must restart TX channel (pending)   */

    /*  disable TX irq, release skb when sending new packet */
    local_irq_save(sys_flag);
    eth2_dev.tx_irq = 0;
    *MBOX_IGU1_IER = eth2_dev.rx_irq;
    local_irq_restore(sys_flag);

    /*  wake up TX queue    */
    netif_wake_queue(dev);

    return;
}

/*
 *  Description:
 *    Implement strategy to pick up TX DMA channel to transmit packet. Not
 *    implemented yet.
 *  Input:
 *    skb --- struct sk_buff *, packet waiting to be transmitted.
 *    dev --- struct net_device *, device used to transmit packet.
 *  Output:
 *    int --- 0:    Success
 *            else: Error Code
 */
static INLINE int pick_up_tx_channel(struct sk_buff *skb, struct net_device *dev)
{
#if 0
    static int count = 0;

    return count++ % TX_TOTAL_CHANNEL_USED;
#else
    return 3;
#endif
}

/*
 *  Description:
 *    Allocate a sk_buff for RX path using. The size is maximum packet size
 *    plus maximum overhead size.
 *  Input:
 *    none
 *  Output:
 *    sk_buff* --- 0:    Failed
 *                 else: Pointer to sk_buff
 */
static INLINE struct sk_buff* alloc_skb_rx(void)
{
    struct sk_buff *skb;

    /*  allocate memroy including trailer and padding   */
    skb = dev_alloc_skb(eth2_dev.rx_buffer_size + RX_HEAD_MAC_ADDR_ALIGNMENT + DMA_ALIGNMENT);
    if ( skb )
    {
        /*  must be burst length alignment and reserve two more bytes for MAC address alignment  */
        if ( ((u32)skb->data & (DMA_ALIGNMENT - 1)) != 0 )
            skb_reserve(skb, ~((u32)skb->data + (DMA_ALIGNMENT - 1)) & (DMA_ALIGNMENT - 1));
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
        /*  put skb in reserved area "skb->data - 4"    */
        *((u32*)skb->data - 1) = (u32)skb;
        /*  invalidate cache    */
        dma_cache_inv((unsigned long)skb->head, (u32)skb->end - (u32)skb->head);
        /*  put skb in reserved area "skb->data - 4"    */
        // *((u32*)skb->data - 1) = (u32)skb;
#endif
    }
    return skb;
}

/*
 *  Description:
 *    Allocate a sk_buff for TX path using.
 *  Input:
 *    size     --- unsigned int, size of the buffer
 *  Output:
 *    sk_buff* --- 0:    Failed
 *                 else: Pointer to sk_buff
 */
static INLINE struct sk_buff* alloc_skb_tx(unsigned int size)
{
    struct sk_buff *skb;

    /*  allocate memory including padding   */
    size = (size + DMA_ALIGNMENT - 1) & ~(DMA_ALIGNMENT - 1);
    skb = dev_alloc_skb(size + DMA_ALIGNMENT);
    /*  must be burst length alignment  */
    if ( skb && ((u32)skb->data & (DMA_ALIGNMENT - 1)) != 0 )
        skb_reserve(skb, ~((u32)skb->data + (DMA_ALIGNMENT - 1)) & (DMA_ALIGNMENT - 1));
    return skb;
}

#if 0
  #if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
/*
 *  Description:
 *    In RX path, the sk_buff is allocated before length is confirmed, so that
 *    it takes maximum value as buffer size. As the data length is confirmed,
 *    reset sk_buff and set the new data length.
 *  Input:
 *    skb      --- struct sk_buff *, sk_buff need to be updated
 *  Output:
 *    none
 */
static INLINE void reset_skb_rx(struct sk_buff *skb)
{
    /*  do not modify 'truesize' and 'end', or crash kernel */

    /*  Load the data pointers. */
    skb->data = (unsigned char*)(((u32)skb->head + 16 + (DMA_ALIGNMENT - 1)) & ~(DMA_ALIGNMENT - 1));
    skb->tail = skb->data;

    /*  put skb in reserved area "skb->data - 6"    */
//    *((u32*)((u32)skb->data - RX_HEAD_MAC_ADDR_ALIGNMENT) - 1) = (u32)skb;

    /*  Set up other state  */
    skb->len = 0;
    skb->cloned = 0;
  #if defined(CONFIG_IMQ) || defined (CONFIG_IMQ_MODULE)
    skb->imq_flags = 0;
    skb->nf_info = NULL;
  #endif
    skb->data_len = 0;
}
  #endif
#endif

/*
 *  Description:
 *    Allocate a TX descriptor for DMA channel.
 *  Input:
 *    ch     --- int, connection ID
 *    f_full --- int *, a pointer to get descriptor full flag
 *               1: full, 0: not full
 *  Output:
 *    int    --- negative value: descriptor is used up.
 *               else:           index of descriptor relative to the first one
 *                               of this channel.
 */
static INLINE int alloc_tx_channel(int ch, int *f_full)
{
    u32 sys_flag;
    int desc_base;

    local_irq_save(sys_flag);

    desc_base = eth2_dev.tx_descriptor_number * ch + eth2_dev.tx_desc_alloc_pos[ch];
    if ( !eth2_dev.tx_descriptor_base[desc_base].own )  //  hold by MIPS
    {
        if ( ++eth2_dev.tx_desc_alloc_pos[ch] == eth2_dev.tx_descriptor_number )
            eth2_dev.tx_desc_alloc_pos[ch] = 0;

        ETH2_ASSERT(f_full, "pointer \"f_full\" must be valid!");
        if ( eth2_dev.tx_descriptor_base[eth2_dev.tx_descriptor_number * ch + eth2_dev.tx_desc_alloc_pos[ch]].own ) //  hold by PP32
            *f_full = 1;
        else
            *f_full = 0;
    }
    else
        desc_base = -1;

    local_irq_restore(sys_flag);

    return desc_base;
}

#if defined(ETH2_HW_FLOWCONTROL) && ETH2_HW_FLOWCONTROL
static void eth2_xon(struct net_device *dev)
{
    clear_bit(eth2_dev.fc_bit, &netdev_fc_xoff);
    if ( netif_running(dev) )
    {
  #if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
        *CFG_ERX_DMACH_ON = (1 << RX_TOTAL_CHANNEL_USED) - 1;
  #else
        int i;

        for ( i = 0; i < eth2_dev.rx_dma_device->max_rx_chan_num; i++ )
            if ( eth2_dev.rx_dma_device->rx_chan[i]->control == DANUBE_DMA_CH_ON )
                eth2_dev.rx_dma_device->rx_chan[i]->open(eth2_dev.rx_dma_device->rx_chan[i]);
  #endif
    }
}
#endif

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
static void do_eth2_tasklet(unsigned long arg)
{
  #if 0
    if ( !(*MBOX_IGU1_ISR & ((1 << RX_TOTAL_CHANNEL_USED) - 1)) )
    {
        if ( ++eth2_dev.empty_loop >= TASKLET_MAX_EMPTY_LOOP )
        {
            u32 sys_flag;

            eth2_dev.irq_handling_flag = 0;

            local_irq_save(sys_flag);
            eth2_dev.rx_irq = (1 << RX_TOTAL_CHANNEL_USED) - 1; //  enable RX interrupt
            *MBOX_IGU1_IER = (eth2_dev.tx_irq << 16) | eth2_dev.rx_irq;
            local_irq_restore(sys_flag);

            return;
        }
    }
    else
    {
        int rx_check_loop_counter = 0;
        u32 f_quit = 0;
        u32 channel_mask = 1;
        int channel = 0;
        u32 vlddes;

        while ( 1 )
        {
            if ( (*MBOX_IGU1_ISR & channel_mask) )
            {
                *MBOX_IGU1_ISRC = channel_mask;
                vlddes = ERX_DMA_CHANNEL_CONFIG(channel)->vlddes;

                while ( vlddes )
                {
                    if ( mailbox_rx_irq_handler(channel) == 0 )
                    {
                        /*  signal firmware that descriptor is updated  */
                        mailbox_signal(channel, 0);

                        vlddes--;
                        rx_check_loop_counter++;

                        f_quit = 0;
                    }
                    else
                    {
                        dbg("mailbox_rx_irq_handler(%d) = -EAGAIN", channel);

                        *MBOX_IGU1_ISRS = channel_mask;

                        f_quit |= channel_mask;

                        break;
                    }
                }
            }
            else
                f_quit |= channel_mask;

            if ( rx_check_loop_counter > TASKLET_MAX_RX_CHECK_LOOP || f_quit == (1 << RX_TOTAL_CHANNEL_USED) - 1 )
                break;

            /*  update channel, channel_mask    */
            if ( ++channel == RX_TOTAL_CHANNEL_USED )
            {
                channel_mask = 1;
                channel = 0;
            }
            else
                channel_mask <<= 1;
        }
    }

    tasklet_schedule(&eth2_tasklet);
 #else
    u32 sys_flag;
    int rx_check_loop_counter;
    u32 channel_mask;
    int channel;

    rx_check_loop_counter = 0;
    channel = RX_TOTAL_CHANNEL_USED - 1;
    channel_mask = 0;
    do
    {
        if ( ++channel == RX_TOTAL_CHANNEL_USED )
        {
            channel_mask = 1;
            channel = 0;
        }
        else
            channel_mask <<= 1;

        if ( !(*MBOX_IGU1_ISR & channel_mask) )
            continue;

        while ( mailbox_rx_irq_handler(channel) == 0 )
        {
            /*  signal firmware that descriptor is updated  */
            mailbox_signal(channel, 0);

            if ( ++rx_check_loop_counter >= TASKLET_MAX_RX_CHECK_LOOP )
            {
                tasklet_schedule(&eth2_tasklet);
                return;
            }
        }

        *MBOX_IGU1_ISRC = channel_mask;
    } while ( (*MBOX_IGU1_ISR & ((1 << RX_TOTAL_CHANNEL_USED) - 1)) );

    eth2_dev.irq_handling_flag = 0;

    local_irq_save(sys_flag);
    eth2_dev.rx_irq = (1 << RX_TOTAL_CHANNEL_USED) - 1; //  enable RX interrupt
    *MBOX_IGU1_IER = eth2_dev.tx_irq ? ((eth2_dev.tx_irq << 16) | eth2_dev.rx_irq) : eth2_dev.rx_irq;
    local_irq_restore(sys_flag);
  #endif
}
#endif

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
/*
 *  Description:
 *    Handle IRQ triggered by received packet (RX).
 *  Input:
 *    channel --- unsigned int, channel ID which triggered IRQ
 *  Output:
 *    int     --- 0:    Success
 *                else: Error Code (-EAGAIN, retry until owner flag set)
 */
static INLINE int mailbox_rx_irq_handler(unsigned int channel)
{
//    u32 sys_flag;
//    int skb_base;
    struct sk_buff *skb;
    register struct rx_descriptor reg_desc;
    struct rx_descriptor *desc;

//    local_irq_save(sys_flag);

    /*  get sk_buff pointer and descriptor  */
//    skb_base = eth2_dev.rx_descriptor_number * channel + eth2_dev.rx_desc_read_pos[channel];

    /*  load descriptor from memory */
//    desc = &eth2_dev.rx_descriptor_base[skb_base];
    desc = eth2_dev.rx_desc_ch_base[channel] + eth2_dev.rx_desc_read_pos[channel];
    reg_desc = *desc;

    /*  if PP32 hold descriptor or show not completed   */
    if ( reg_desc.own || !reg_desc.c )
    {
//        local_irq_restore(sys_flag);
        return -EAGAIN;
    }

    /*  update read position    */
    if ( ++eth2_dev.rx_desc_read_pos[channel] == eth2_dev.rx_descriptor_number )
        eth2_dev.rx_desc_read_pos[channel] = 0;

//    local_irq_restore(sys_flag);

    /*  get skb address */
    skb = *(struct sk_buff **)((((u32)reg_desc.dataptr << 2) | KSEG0) - 4);
    ETH2_ASSERT((u32)skb >= 0x80000000, "mailbox_rx_irq_handler: skb = 0x%08X", (u32)skb);

   if ( !reg_desc.err )
    {
        struct sk_buff *new_skb;

        new_skb = alloc_skb_rx();
        if ( new_skb )
        {
//            reset_skb_rx(skb);
            skb_reserve(skb, reg_desc.byteoff);
            skb_put(skb, reg_desc.datalen - RX_TAIL_CRC_LENGTH);

  #if defined(DEBUG_DUMP_RX_SKB_BEFORE) && DEBUG_DUMP_RX_SKB_BEFORE
            dump_skb(skb, 1);
  #endif

            ETH2_ASSERT(skb->len >= 60, "mailbox_rx_irq_handler: skb->len = %d, reg_desc.datalen = %d", skb->len, reg_desc.datalen);

            /*  parse protocol header   */
            skb->dev = &eth2_net_dev;
  #if (!defined(ENABLE_LOOPBACK_TESTING_RX2TX) || !ENABLE_LOOPBACK_TESTING_RX2TX) && (!defined(ENABLE_DIRECT_BRIDGE) || !ENABLE_DIRECT_BRIDGE)
            skb->protocol = eth_type_trans(skb, &eth2_net_dev);
  #endif

  #if defined(DEBUG_DUMP_RX_SKB_AFTER) && DEBUG_DUMP_RX_SKB_AFTER
            dump_skb(skb, 1);
  #endif

  #if defined(ETH2_HW_FLOWCONTROL) && ETH2_HW_FLOWCONTROL
    #if (!defined(ENABLE_LOOPBACK_TESTING_RX2TX) || !ENABLE_LOOPBACK_TESTING_RX2TX) && (!defined(ENABLE_DIRECT_BRIDGE) || !ENABLE_DIRECT_BRIDGE)
            if ( netif_rx(skb) == NET_RX_DROP )
    #else
      #if defined(ENABLE_LOOPBACK_TESTING_RX2TX) && ENABLE_LOOPBACK_TESTING_RX2TX
            eth2_hard_start_xmit(skb, NULL);
      #else
            switch_tx(skb, eth2_switch_dev);
      #endif
            if ( 0 )
    #endif
            {
                if ( eth2_dev.fc_bit && !test_and_set_bit(eth2_dev.fc_bit, &netdev_fc_xoff) )
                    *CFG_ERX_DMACH_ON = 0;

    #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
                eth2_dev.rx_fail++;
    #endif
            }
            else
            {
    #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
                eth2_dev.rx_success++;
    #endif
            }
  #else
    #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
      #if (!defined(ENABLE_LOOPBACK_TESTING_RX2TX) || !ENABLE_LOOPBACK_TESTING_RX2TX) && (!defined(ENABLE_DIRECT_BRIDGE) || !ENABLE_DIRECT_BRIDGE)
            if ( netif_rx(skb) != NET_RX_DROP )
      #else
        #if defined(ENABLE_LOOPBACK_TESTING_RX2TX) && ENABLE_LOOPBACK_TESTING_RX2TX
            eth2_hard_start_xmit(skb, NULL);
        #else
            switch_tx(skb, eth2_switch_dev);
        #endif
            if ( 1 )
      #endif
                eth2_dev.rx_success++;
            else
                eth2_dev.rx_fail++;
    #else
      #if (!defined(ENABLE_LOOPBACK_TESTING_RX2TX) || !ENABLE_LOOPBACK_TESTING_RX2TX) && (!defined(ENABLE_DIRECT_BRIDGE) || !ENABLE_DIRECT_BRIDGE)
            netif_rx(skb)
      #else
        #if defined(ENABLE_LOOPBACK_TESTING_RX2TX) && ENABLE_LOOPBACK_TESTING_RX2TX
            eth2_hard_start_xmit(skb, NULL);
        #else
            switch_tx(skb, eth2_switch_dev);
        #endif
      #endif
    #endif    //  defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
  #endif

            /*  update descriptor with new sk_buff  */
            reg_desc.dataptr = (u32)new_skb->data >> 2;
            reg_desc.byteoff = RX_HEAD_MAC_ADDR_ALIGNMENT;
        }
        else
        {
            dbg("null sk_buff");

            /*  no sk buffer    */
//            eth2_dev.stats.rx_errors++;
            eth2_dev.rx_drop_counter++;
  #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
            eth2_dev.rx_driver_level_drop++;
  #endif
        }
    }
    else
    {
        dbg("rx_error");
        eth2_dev.stats.rx_errors++;
//        eth2_dev.rx_drop_counter++;
  #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
        eth2_dev.rx_driver_level_error++;
  #endif
    }

    /*  update descriptor   */
    reg_desc.datalen = eth2_dev.rx_buffer_size;
    reg_desc.own = 1;
    reg_desc.c   = 0;

    /*  write descriptor to memory  */
    *desc = reg_desc;

    return 0;
}
#endif  //  !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH

/*
 *  Description:
 *    Handle IRQ of mailbox and despatch to relative handler.
 *  Input:
 *    irq    --- int, IRQ number
 *    dev_id --- void *, argument passed when registering IRQ handler
 *    regs   --- struct pt_regs *, registers' value before jumping into handler
 *  Output:
 *    none
 */
static void mailbox_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
#if 0   //  moved after TX handling in case block TX interrupt
    if ( eth2_dev.irq_handling_flag++ )
        return;
#endif

#if 1
    if ( !*MBOX_IGU1_ISR )
    {
        //eth2_dev.irq_handling_flag = 0;
        return;
    }
#endif

    /*  TX  */
    if ( eth2_dev.tx_irq )
    {
#if 0
        u32 channel_mask1 = 1;
        u32 channel_mask2 = 1 << 16;
        int channel = 0;
        u32 f_set = 0;

        while ( channel < TX_TOTAL_CHANNEL_USED )
        {
            if ( (eth2_dev.tx_irq & channel_mask1) && (*MBOX_IGU1_ISR & channel_mask2) )
            {
                /*  disable TX irq, release skb when sending new packet */
                eth2_dev.tx_irq &= ~channel_mask1;
                f_set |= channel_mask2; //  write register later
            }

            channel_mask1 <<= 1;
            channel_mask2 <<= 1;
            channel++;
        }

        if ( f_set )
        {
            *MBOX_IGU1_IER = (eth2_dev.tx_irq << 16) | eth2_dev.rx_irq;
            *MBOX_IGU1_ISRC = f_set;

  #if defined(ENABLE_HARDWARE_EMULATION) && ENABLE_HARDWARE_EMULATION
            *MBOX_IGU1_ISR ^= f_set;
            *MBOX_IGU1_ISRC = 0;
  #endif  //  defined(ENABLE_HARDWARE_EMULATION) && ENABLE_HARDWARE_EMULATION

            netif_wake_queue(&eth2_net_dev);
        }
#else
        u32 f_set;

        f_set = eth2_dev.tx_irq & (*MBOX_IGU1_ISR >> 16);
        if ( f_set )
        {
            eth2_dev.tx_irq &= ~f_set;
            *MBOX_IGU1_IER = (eth2_dev.tx_irq << 16) | eth2_dev.rx_irq;
//            *MBOX_IGU1_ISRC = (f_set << 16);

  #if defined(ENABLE_HARDWARE_EMULATION) && ENABLE_HARDWARE_EMULATION
            *MBOX_IGU1_ISR ^= f_set;
            *MBOX_IGU1_ISRC = 0;
  #endif  //  defined(ENABLE_HARDWARE_EMULATION) && ENABLE_HARDWARE_EMULATION

            netif_wake_queue(&eth2_net_dev);
        }
#endif
    }

    //  in case interrupt during tasklet running
    if ( eth2_dev.irq_handling_flag++ )
        return;

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    /*  RX  */
    if ( eth2_dev.rx_irq )
    {
  #if 0
        eth2_dev.empty_loop = 0;
  #endif
        eth2_dev.rx_irq = 0;
        *MBOX_IGU1_IER = eth2_dev.tx_irq << 16; //  disable RX irq
        tasklet_schedule(&eth2_tasklet);

        return;
    }
#endif

    eth2_dev.irq_handling_flag = 0;
}

/*
 *  Description:
 *    Signal PPE firmware a TX packet ready or RX descriptor updated.
 *  Input:
 *    connection --- unsigned int, connection ID
 *    is_tx      --- int, 0 means RX, else means TX
 *  Output:
 *    none
 */
static INLINE void mailbox_signal(unsigned int connection, int is_tx)
{
    if ( is_tx )
    {
#if !defined(ENABLE_DEBUG_COUNTER) || !ENABLE_DEBUG_COUNTER
        while ( MBOX_IGU3_ISR_ISR(connection + 16) );
#else
        while ( MBOX_IGU3_ISR_ISR(connection + 16) )
            eth2_dev.tx_desc_update_wait_loop++;
#endif
        *MBOX_IGU3_ISRS = MBOX_IGU3_ISRS_SET(connection + 16);
    }
    else
    {
#if !defined(ENABLE_DEBUG_COUNTER) || !ENABLE_DEBUG_COUNTER
        while ( MBOX_IGU3_ISR_ISR(connection) );
#else
        while ( MBOX_IGU3_ISR_ISR(connection) )
            eth2_dev.rx_desc_update_wait_loop++;
#endif
        *MBOX_IGU3_ISRS = MBOX_IGU3_ISRS_SET(connection);
    }

#if defined(ENABLE_HARDWARE_EMULATION) && ENABLE_HARDWARE_EMULATION
    /*  clear ISRS and set ISR register */
    if ( is_tx )
    {
        *MBOX_IGU3_ISRS = 0;
        *MBOX_IGU3_ISR |= (1 << (connection + 16));
    }
    else
    {
        *MBOX_IGU3_ISRS = 0;
        *MBOX_IGU3_ISR |= (1 << connection);
    }

//    debug_ppe_sim();
#endif  //  defined(ENABLE_HARDWARE_EMULATION) && ENABLE_HARDWARE_EMULATION
}

#if defined(ENABLE_RX_DPLUS_PATH) && ENABLE_RX_DPLUS_PATH
static u8* dma_buffer_alloc(int len, int *byte_offset, void **opt)
{
    u8 *buf;
    struct sk_buff *skb;

    skb = alloc_skb_rx();
    if ( !skb )
        return NULL;

    buf = (u8 *)skb->data;
    skb_reserve(skb, RX_HEAD_MAC_ADDR_ALIGNMENT);
    *(u32 *)opt = (u32)skb;
    *byte_offset = RX_HEAD_MAC_ADDR_ALIGNMENT;
    return buf;
}

static int dma_buffer_free(u8 *dataptr, void *opt)
{
    if ( !opt )
        kfree(dataptr);
    else
        dev_kfree_skb_any((struct sk_buff *)opt);

    return 0;
}

static int dma_int_handler(struct dma_device_info *dma_dev, int status)
{
    int ret = 0;

    switch ( status )
    {
    case RCV_INT:
        ret = dma_rx_int_handler(dma_dev);
        break;
    case TX_BUF_FULL_INT:
    case TRANSMIT_CPT_INT:
        dbg("TX uses EMA path, how come there is a TX event from DMA?");
        break;
    default:
        dbg("unkown DMA interrupt event - %d", status);
        break;
    }

    return ret;
}

static INLINE int dma_rx_int_handler(struct dma_device_info *dma_dev)
{
    struct sk_buff *skb = NULL;
    u8 *buf = NULL;
    int len;

    len = dma_device_read(dma_dev, &buf, (void **)&skb);

    if ( (u32)skb < 0x80000000 )
    {
        dbg("can not restore skb pointer");
        goto DMA_RX_INT_HANDLER_ERR;
    }

    if ( len > ETH_MAX_FRAME_LENGTH || len > (u32)skb->end - (u32)skb->data )
    {
        dbg("packet is too large");
        goto DMA_RX_INT_HANDLER_ERR;
    }

    if ( len < 64 )
    {
        dbg("packet is too small");
        goto DMA_RX_INT_HANDLER_ERR;
    }

    skb_put(skb, len - RX_TAIL_CRC_LENGTH);

  #if defined(DEBUG_DUMP_RX_SKB_BEFORE) && DEBUG_DUMP_RX_SKB_BEFORE
    dump_skb(skb, 1);
  #endif

    ETH2_ASSERT(skb->len >= 60, "dma_rx_int_handler: skb->len = %d, len = %d", skb->len, len);

    skb->dev = &eth2_net_dev;
  #if (!defined(ENABLE_LOOPBACK_TESTING_RX2TX) || !ENABLE_LOOPBACK_TESTING_RX2TX) && (!defined(ENABLE_DIRECT_BRIDGE) || !ENABLE_DIRECT_BRIDGE)
    skb->protocol = eth_type_trans(skb, &eth2_net_dev);
  #endif

  #if defined(DEBUG_DUMP_RX_SKB_AFTER) && DEBUG_DUMP_RX_SKB_AFTER
    dump_skb(skb, 1);
  #endif

  #if (!defined(ENABLE_LOOPBACK_TESTING_RX2TX) || !ENABLE_LOOPBACK_TESTING_RX2TX) && (!defined(ENABLE_DIRECT_BRIDGE) || !ENABLE_DIRECT_BRIDGE)
    if ( netif_rx(skb) == NET_RX_DROP )
  #else
    #if defined(ENABLE_LOOPBACK_TESTING_RX2TX) && ENABLE_LOOPBACK_TESTING_RX2TX
        eth2_hard_start_xmit(skb, NULL);
    #else
        switch_tx(skb, eth2_switch_dev);
    #endif
    if ( 0 )
  #endif
    {
  #if defined(ETH2_HW_FLOWCONTROL) && ETH2_HW_FLOWCONTROL
        if ( eth2_dev.fc_bit && !test_and_set_bit(eth2_dev.fc_bit, &netdev_fc_xoff) )
        {
            int i;

            for ( i = 0; i < eth2_dev.rx_dma_device->max_rx_chan_num; i++ )
                eth2_dev.rx_dma_device->rx_chan[i]->close(eth2_dev.rx_dma_device->rx_chan[i]);
        }
  #endif

  #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
        eth2_dev.rx_fail++;
  #endif
    }
    else
    {
        eth2_dev.stats.rx_packets++;
        eth2_dev.stats.rx_bytes += len - RX_TAIL_CRC_LENGTH;
  #if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
        eth2_dev.rx_success++;
  #endif
    }

    return 0;

DMA_RX_INT_HANDLER_ERR:
    if ( skb )
        dev_kfree_skb_any(skb);
    eth2_dev.stats.rx_errors++;
//    eth2_dev.stats.rx_dropped++;

    return 0;
}
#endif  //  defined(ENABLE_RX_DPLUS_PATH) && ENABLE_RX_DPLUS_PATH

/*
 *  Description:
 *    Handle ioctl command SIOCETHTOOL.
 *  Input:
 *    dev --- struct net_device *, device responsing to the command.
 *    ifr --- struct ifreq *, interface request structure to pass parameters
 *            or result.
 *  Output:
 *    int --- 0:    Success
 *            else: Error Code (-EFAULT, -EOPNOTSUPP)
 */
static INLINE int ethtool_ioctl(struct net_device *dev, struct ifreq *ifr)
{
    struct ethtool_cmd cmd;

    if ( copy_from_user(&cmd, ifr->ifr_data, sizeof(cmd)) )
        return -EFAULT;

    switch ( cmd.cmd )
    {
    case ETHTOOL_GSET:      /*  get hardware information        */
        {
            memset(&cmd, 0, sizeof(cmd));

            cmd.supported   = SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII |
                              SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
                              SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full;
            cmd.port        = PORT_MII;
            cmd.transceiver = XCVR_EXTERNAL;
            cmd.phy_address = 1;
            cmd.speed       = ENET_MAC_CFG_SPEED(1) ? SPEED_100 : SPEED_10;
            cmd.duplex      = ENET_MAC_CFG_DUPLEX(1) ? DUPLEX_FULL : DUPLEX_HALF;

            if ( (*ETOP_MDIO_CFG & ETOP_MDIO_CFG_UMM(1, 1)) )
            {
                /*  auto negotiate  */
                cmd.autoneg = AUTONEG_ENABLE;
                cmd.advertising |= ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
                                   ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full;
            }
            else
            {
                cmd.autoneg = AUTONEG_DISABLE;
                cmd.advertising &= ~(ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
                                     ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full);
            }

            if ( copy_to_user(ifr->ifr_data, &cmd, sizeof(cmd)) )
                return -EFAULT;
        }
        break;
    case ETHTOOL_SSET:      /*  force the speed and duplex mode */
        {
            if ( !capable(CAP_NET_ADMIN) )
                return -EPERM;

            if ( cmd.autoneg == AUTONEG_ENABLE )
            {
                /*  set property and start autonegotiation                                  */
                /*  have to set mdio advertisement register and restart autonegotiation     */
                /*  which is a very rare case, put it to future development if necessary.   */
            }
            else
            {
                /*  set property without autonegotiation    */
                *ETOP_MDIO_CFG &= ~ETOP_MDIO_CFG_UMM(1, 1);

                /*  set speed   */
                if ( cmd.speed == SPEED_10 )
                    ENET_MAC_CFG_SPEED_10M(1);
                else if ( cmd.speed == SPEED_100 )
                    ENET_MAC_CFG_SPEED_100M(1);

                /*  set duplex  */
                if ( cmd.duplex == DUPLEX_HALF )
                    ENET_MAC_CFG_DUPLEX_HALF(1);
                else if ( cmd.duplex == DUPLEX_FULL )
                    ENET_MAC_CFG_DUPLEX_FULL(1);

                ENET_MAC_CFG_LINK_OK(1);
            }
        }
        break;
    case ETHTOOL_GDRVINFO:  /*  get driver information          */
        {
            struct ethtool_drvinfo info;

            memset(&info, 0, sizeof(info));
            strncpy(info.driver, "Danube Eth2 Driver", sizeof(info.driver) - 1);
            strncpy(info.fw_version, "0.0.1", sizeof(info.fw_version) - 1);
            strncpy(info.bus_info, "N/A", sizeof(info.bus_info) - 1);
            info.regdump_len = 0;
            info.eedump_len = 0;
            info.testinfo_len = 0;
            if ( copy_to_user(ifr->ifr_data, &info, sizeof(info)) )
                return -EFAULT;
        }
        break;
    case ETHTOOL_NWAY_RST:  /*  restart auto negotiation        */
        *ETOP_MDIO_CFG |= ETOP_MDIO_CFG_SMRST(1) | ETOP_MDIO_CFG_UMM(1, 1);
        break;
    default:
        return -EOPNOTSUPP;
    }

    return 0;
}

/*
 *  Description:
 *    Specify ETOP ingress VLAN priority's class of service mapping.
 *  Input:
 *    req --- struct vlan_cos_req *, pass parameters such as priority and class
 *            of service mapping.
 *  Output:
 *    none
 */
static INLINE void set_vlan_cos(struct vlan_cos_req *req)
{
     *ETOP_IG_VLAN_COS = ETOP_IG_VLAN_COS_Px_SET(*ETOP_IG_VLAN_COS, req->pri, req->cos_value);
}

/*
 *  Description:
 *    Specify ETOP ingress VLAN differential service control protocol's class of
 *    service mapping.
 *  Input:
 *    req --- struct dscp_cos_req *, pass parameters such as differential
 *            service control protocol and class of service mapping.
 *  Output:
 *    none
 */
static INLINE void set_dscp_cos(struct dscp_cos_req *req)
{
    ETOP_IG_DSCP_COS_SET(req->dscp, req->cos_value);
}

#if defined(ENABLE_PROBE_TRANSCEIVER) && ENABLE_PROBE_TRANSCEIVER
/*
 *  Description:
 *    Setup ethernet hardware in init process.
 *  Input:
 *    dev --- struct net_device *, device to be setup.
 *  Output:
 *    int --- 0:    Success
 *            else: Error Code (-EIO, link is not OK)
 */
static INLINE int probe_transceiver(struct net_device *dev)
{
    *ENETS_MAC_DA0(1) = (dev->dev_addr[0] << 24) | (dev->dev_addr[1] << 16) | (dev->dev_addr[2] << 8) | dev->dev_addr[3];
    *ENETS_MAC_DA1(1) = (dev->dev_addr[4] << 24) | (dev->dev_addr[3] << 16);

    if ( !ENET_MAC_CFG_LINK(1) )
    {
        *ETOP_MDIO_CFG = (*ETOP_MDIO_CFG & ~ETOP_MDIO_CFG_MASK(1))
                         | ETOP_MDIO_CFG_SMRST(1)
                         | ETOP_MDIO_CFG_PHYA(1, ETOP_MDIO_PHY1_ADDR)
                         | ETOP_MDIO_CFG_UMM(1, 1);

        udelay(ETOP_MDIO_DELAY);

        if ( !ENET_MAC_CFG_LINK(1) )
            return -EIO;
    }

    return 0;
}
#endif

/*
 *  Description:
 *    Check parameters passed by command "insmod" and amend them.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void check_parameters(void)
{
    /*  There is a delay between PPE write descriptor and descriptor is       */
    /*  really stored in memory. Host also has this delay when writing        */
    /*  descriptor. So PPE will use this value to determine if the write      */
    /*  operation makes effect.                                               */
    if ( write_descriptor_delay < 0 )
        write_descriptor_delay = 0;

    /*  Because of the limitation of length field in descriptors, the packet  */
    /*  size could not be larger than 64K minus overhead size.                */
    if ( rx_max_packet_size < ETH_MIN_FRAME_LENGTH )
        rx_max_packet_size = ETH_MIN_FRAME_LENGTH;
    else if ( rx_max_packet_size > ETH_MAX_FRAME_LENGTH )
        rx_max_packet_size = ETH_MAX_FRAME_LENGTH;
    if ( rx_min_packet_size < ETH_MIN_FRAME_LENGTH )
        rx_min_packet_size = ETH_MIN_FRAME_LENGTH;
    else if ( rx_min_packet_size > rx_max_packet_size )
        rx_min_packet_size = rx_max_packet_size;
    if ( tx_max_packet_size < ETH_MIN_FRAME_LENGTH )
        tx_max_packet_size = ETH_MIN_FRAME_LENGTH;
    else if ( tx_max_packet_size > ETH_MAX_FRAME_LENGTH )
        tx_max_packet_size = ETH_MAX_FRAME_LENGTH;
    if ( tx_min_packet_size < ETH_MIN_FRAME_LENGTH )
        tx_min_packet_size = ETH_MIN_FRAME_LENGTH;
    else if ( tx_min_packet_size > tx_max_packet_size )
        tx_min_packet_size = tx_max_packet_size;

    if ( dma_rx_descriptor_length < 2 )
        dma_rx_descriptor_length = 2;
#if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    if ( dma_tx_descriptor_length < 2 )
        dma_tx_descriptor_length = 2;
#else
    dma_tx_descriptor_length = 32;
#endif
}

/*
 *  Description:
 *    Setup variable eth2_dev and allocate memory.
 *  Input:
 *    none
 *  Output:
 *    int --- 0:    Success
 *            else: Error Code
 */
static INLINE int init_eth2_dev(void)
{
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    int rx_desc;
    struct rx_descriptor rx_descriptor = {  own:    1,
                                            c:      0,
                                            sop:    1,
                                            eop:    1,
                                            res1:   0,
                                            byteoff:RX_HEAD_MAC_ADDR_ALIGNMENT,
                                            res2:   0,
                                            id:     0,
                                            err:    0,
                                            datalen:0,
                                            res3:   0,
                                            dataptr:0};
    int i;
#else
    int i;
#endif

#if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    int tx_desc;
    struct tx_descriptor tx_descriptor = {  own:    0,  //  it's hold by MIPS
                                            c:      0,
                                            sop:    1,
                                            eop:    1,
                                            byteoff:0,
                                            res1:   0,
                                            iscell: 0,
                                            clp:    0,
                                            datalen:0,
                                            res2:   0,
                                            dataptr:0};
#endif

    memset(&eth2_dev, 0, sizeof(eth2_dev));

    eth2_dev.rx_buffer_size = (rx_max_packet_size + DMA_ALIGNMENT - 1) & ~(DMA_ALIGNMENT - 1);

    /*
     *  dma
     */
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    /*  descriptor number of RX DMA channel */
    eth2_dev.rx_descriptor_number   = dma_rx_descriptor_length;
#endif
    /*  descriptor number of TX DMA channel */
    eth2_dev.tx_descriptor_number   = dma_tx_descriptor_length;

    /*  delay on descriptor write path  */
    eth2_dev.write_descriptor_delay = write_descriptor_delay;

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    /*  allocate memory for RX descriptors  */
    eth2_dev.rx_descriptor_addr = kmalloc(RX_TOTAL_CHANNEL_USED * eth2_dev.rx_descriptor_number * sizeof(struct rx_descriptor) + DMA_ALIGNMENT, GFP_KERNEL | GFP_DMA);
    if ( !eth2_dev.rx_descriptor_addr )
        goto RX_DESCRIPTOR_BASE_ALLOCATE_FAIL;
    /*  do alignment (DWORD)    */
    eth2_dev.rx_descriptor_base = (struct rx_descriptor *)(((u32)eth2_dev.rx_descriptor_addr + (DMA_ALIGNMENT - 1)) & ~(DMA_ALIGNMENT - 1));
    eth2_dev.rx_descriptor_base = (struct rx_descriptor *)((u32)eth2_dev.rx_descriptor_base | KSEG1);   //  no cache
    for ( i = 0; i < RX_TOTAL_CHANNEL_USED; i++ )
        eth2_dev.rx_desc_ch_base[i] = eth2_dev.rx_descriptor_base + eth2_dev.rx_descriptor_number * i;
#else
    eth2_dev.rx_dma_device = dma_device_reserve("ETH2");
    if ( !eth2_dev.rx_dma_device )
        goto RX_DMA_DEVICE_RESERVE_FAIL;
    eth2_dev.rx_dma_device->buffer_alloc    = dma_buffer_alloc;
    eth2_dev.rx_dma_device->buffer_free     = dma_buffer_free;
    eth2_dev.rx_dma_device->intr_handler    = dma_int_handler;
    eth2_dev.rx_dma_device->max_rx_chan_num = 2;

    for ( i = 0; i < eth2_dev.rx_dma_device->max_rx_chan_num; i++ )
    {
        eth2_dev.rx_dma_device->rx_chan[i]->packet_size = eth2_dev.rx_buffer_size;
        eth2_dev.rx_dma_device->rx_chan[i]->control     = DANUBE_DMA_CH_ON;
    }

    for ( i = 0; i < eth2_dev.rx_dma_device->max_tx_chan_num; i++ )
    {
        eth2_dev.rx_dma_device->tx_chan[i]->control     = DANUBE_DMA_CH_OFF;
    }

    dma_device_register(eth2_dev.rx_dma_device);
#endif

#if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    /*  allocate memory for TX descriptors  */
    eth2_dev.tx_descriptor_addr = kmalloc(TX_TOTAL_CHANNEL_USED * eth2_dev.tx_descriptor_number * sizeof(struct tx_descriptor) + DMA_ALIGNMENT, GFP_KERNEL | GFP_DMA);
    if ( !eth2_dev.tx_descriptor_addr )
       goto TX_DESCRIPTOR_BASE_ALLOCATE_FAIL;
    /*  do alignment (DWORD)    */
    eth2_dev.tx_descriptor_base = (struct tx_descriptor *)(((u32)eth2_dev.tx_descriptor_addr + (DMA_ALIGNMENT - 1)) & ~(DMA_ALIGNMENT - 1));
    eth2_dev.tx_descriptor_base = (struct tx_descriptor *)((u32)eth2_dev.tx_descriptor_base | KSEG1);   //  no cache
#else
    eth2_dev.tx_descriptor_base = (struct tx_descriptor *)SB_TX_DESC_BASE_ADDR;
#endif
    /*  allocate pointers to TX sk_buff */
    eth2_dev.tx_skb_pointers = kmalloc(TX_TOTAL_CHANNEL_USED * eth2_dev.tx_descriptor_number * sizeof(struct sk_buff *), GFP_KERNEL);
    if ( !eth2_dev.tx_skb_pointers )
        goto TX_SKB_POINTER_ALLOCATE_FAIL;
    memset(eth2_dev.tx_skb_pointers, 0, TX_TOTAL_CHANNEL_USED * eth2_dev.tx_descriptor_number * sizeof(struct sk_buff *));

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    /*  Allocate RX sk_buff and fill up RX descriptors. */
    rx_descriptor.datalen = eth2_dev.rx_buffer_size;
    for ( rx_desc = RX_TOTAL_CHANNEL_USED * eth2_dev.rx_descriptor_number - 1; rx_desc >= 0; rx_desc-- )
    {
        struct sk_buff *skb;

        skb = alloc_skb_rx();
        if ( skb == NULL )
            panic("sk buffer is used up\n");
        rx_descriptor.dataptr = (u32)skb->data >> 2;
        eth2_dev.rx_descriptor_base[rx_desc] = rx_descriptor;
    }
#endif

#if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    /*  Fill up TX descriptors. */
    for ( tx_desc = TX_TOTAL_CHANNEL_USED * eth2_dev.tx_descriptor_number - 1; tx_desc >= 0; tx_desc-- )
        eth2_dev.tx_descriptor_base[tx_desc] = tx_descriptor;
#endif

#if defined(ENABLE_RX_DPLUS_PATH) && ENABLE_RX_DPLUS_PATH
    ETH2_ASSERT((u32)eth2_dev.rx_dma_device >= 0x80000000, "eth2_dev.rx_dma_device = 0x%08X", (u32)eth2_dev.rx_dma_device);
#endif

    return 0;

TX_SKB_POINTER_ALLOCATE_FAIL:
#if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    kfree(eth2_dev.tx_descriptor_addr);
TX_DESCRIPTOR_BASE_ALLOCATE_FAIL:
#endif
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    kfree(eth2_dev.rx_descriptor_addr);
RX_DESCRIPTOR_BASE_ALLOCATE_FAIL:
#else
    dma_device_unregister(eth2_dev.rx_dma_device);
    dma_device_release(eth2_dev.rx_dma_device);
RX_DMA_DEVICE_RESERVE_FAIL:
#endif
    return -ENOMEM;
}

/*
 *  Description:
 *    Fill up share buffer with 0.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void clear_share_buffer(void)
{
    volatile u32 *p = SB_RAM0_ADDR(0);
    unsigned int i;

    /*  write all zeros only    */
    for ( i = 0; i < SB_RAM0_DWLEN + SB_RAM1_DWLEN + SB_RAM2_DWLEN + SB_RAM3_DWLEN; i++ )
        *p++ = 0;
}

/*
 *  Description:
 *    Setup RX/TX relative registers and tables, including HTU table. All
 *    parameters are taken from eth2_dev.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void init_tables(void)
{
    int i;
    volatile u32 *p;
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    struct erx_dma_channel_config rx_config = {0};
#endif
    struct etx_dma_channel_config tx_config = {0};
#if defined(ENABLE_SHAREBUFFER_TX_DESC) && ENABLE_SHAREBUFFER_TX_DESC
    struct tx_descriptor tx_descriptor = {  own:    0,  //  it's hold by MIPS
                                            c:      0,
                                            sop:    1,
                                            eop:    1,
                                            byteoff:0,
                                            res1:   0,
                                            iscell: 0,
                                            clp:    0,
                                            datalen:0,
                                            res2:   0,
                                            dataptr:0};
#endif

    /*
     *  CDM Block 1
     */
    *CDM_CFG = CDM_CFG_RAM1_SET(0x00) | CDM_CFG_RAM0_SET(0x00); //  CDM block 1 must be data memory and mapped to 0x5000 (dword addr)
    p = PP32_DATA_MEMORY_RAM1_ADDR(0);                          //  Clear CDM block 1
    for ( i = 0; i < PP32_DATA_MEMORY_RAM1_DWLEN; i++ )
        *p++ = 0;

    /*
     *  General Registers
     */
    *CFG_WAN_WRDES_DELAY    = eth2_dev.write_descriptor_delay;
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    *CFG_ERX_DMACH_ON       = (1 << RX_TOTAL_CHANNEL_USED) - 1;
#else
    *CFG_ERX_DMACH_ON       = 0;
#endif
    *CFG_ETX_DMACH_ON       = (1 << TX_TOTAL_CHANNEL_USED) - 1;
    *CFG_ETH_DMA_MODE       = 0x01;     //  1: counter mode, 0: polling mode
    *CFG_ETH_DMA_POLL_BASE  = 0x2000;
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    *CFG_ETH_RXDMA_POLL_CNT = 0x02;
#else
    *CFG_ETH_RXDMA_POLL_CNT = 0;
#endif
    *CFG_ETH_TXDMA_POLL_CNT = 0x02;
    *CFG_LOOPBACK           = 0x00;

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    /*
     *  WRX DMA Channel Configuration Table
     */
    rx_config.deslen = eth2_dev.rx_descriptor_number;
    for ( i = 0; i < RX_TOTAL_CHANNEL_USED; i++ )
    {
        rx_config.desba = (((u32)eth2_dev.rx_descriptor_base >> 2) & 0x0FFFFFFF) + eth2_dev.rx_descriptor_number * i * (sizeof(struct  rx_descriptor) >> 2);
        *ERX_DMA_CHANNEL_CONFIG(i) = rx_config;
    }
#endif

    /*
     *  WTX DMA Channel Configuration Table
     */
    tx_config.deslen = eth2_dev.tx_descriptor_number;
    for ( i = 0; i < TX_TOTAL_CHANNEL_USED; i++ )
    {
        tx_config.desba = (((u32)eth2_dev.tx_descriptor_base >> 2) & 0x0FFFFFFF) + eth2_dev.tx_descriptor_number * i * (sizeof(struct  tx_descriptor) >> 2);
        *ETX_DMA_CHANNEL_CONFIG(i) = tx_config;
    }

#if defined(ENABLE_SHAREBUFFER_TX_DESC) && ENABLE_SHAREBUFFER_TX_DESC
    for ( i = TX_TOTAL_CHANNEL_USED * eth2_dev.tx_descriptor_number - 1; i >= 0; i-- )
        eth2_dev.tx_descriptor_base[i] = tx_descriptor;
#endif
}

/*
 *  Description:
 *    Clean-up eth2_dev and release memory.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void clear_eth2_dev(void)
{
    int desc_base;
    int i, j;

    /*
     *  free memory allocated for RX/TX descriptors and RX sk_buff
     */
    desc_base = 0;
    for ( i = 0; i < TX_TOTAL_CHANNEL_USED; i++ )
        for ( j = 0; j < eth2_dev.tx_descriptor_number; j++ )
        {
            if ( eth2_dev.tx_skb_pointers[desc_base] )
                dev_kfree_skb_any(eth2_dev.tx_skb_pointers[desc_base]);
            desc_base++;
        }

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    for ( i = RX_TOTAL_CHANNEL_USED * eth2_dev.rx_descriptor_number - 1; i >= 0; i-- )
        dev_kfree_skb_any(*(struct sk_buff **)(((eth2_dev.rx_descriptor_base[i].dataptr << 2) | KSEG0) - 4));
#else
    dma_device_unregister(eth2_dev.rx_dma_device);
    dma_device_release(eth2_dev.rx_dma_device);
#endif

    kfree(eth2_dev.tx_skb_pointers);
#if !defined(ENABLE_SHAREBUFFER_TX_DESC) || !ENABLE_SHAREBUFFER_TX_DESC
    kfree(eth2_dev.tx_descriptor_addr);
#endif
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    kfree(eth2_dev.rx_descriptor_addr);
#endif
}

static INLINE void init_ema(void)
{
    *EMA_CMDCFG  = (EMA_CMD_BUF_LEN << 16) | (EMA_CMD_BASE_ADDR >> 2);
    *EMA_DATACFG = (EMA_DATA_BUF_LEN << 16) | (EMA_DATA_BASE_ADDR >> 2);
    *EMA_IER     = 0x000000FF;
	*EMA_CFG     = EMA_READ_BURST | (EMA_WRITE_BURST << 2);
}

static INLINE void init_chip(int mode)
{
#if !defined(ENABLE_HARDWARE_EMULATION) || !ENABLE_HARDWARE_EMULATION
    //  Enable PPE in PMU
    *(unsigned long *)0xBF10201C &= ~((1 << 13) | (1 << 15));

  #if defined(DEBUG_WRITE_GPIO_REGISTER) && DEBUG_WRITE_GPIO_REGISTER
    *(unsigned long *)0xBE100B1C = (*(unsigned long *)0xBE100B1C & 0x00009859) | 0x00006786;
    *(unsigned long *)0xBE100B20 = (*(unsigned long *)0xBE100B20 & 0x00009859) | 0x000067a6;
    *(unsigned long *)0xBE100B4C = (*(unsigned long *)0xBE100B4C & 0x00000fa7) | 0x0000e058;
	*(unsigned long *)0xBE100B50 = (*(unsigned long *)0xBE100B50 & 0x00000fa7) | 0x0000f058;

	if ( mode != REV_MII_MODE )
	{
	    *(unsigned long *)0xBE100B18 = (*(unsigned long *)0xBE100B18 & 0x00009859) | 0x00006020;
		*(unsigned long *)0xBE100B48 = (*(unsigned long *)0xBE100B48 & 0x00000fa7) | 0x00007000;
		*(unsigned long *)0xBE100B24 = *(unsigned long *)0xBE100B24 | 0x00006020;
		*(unsigned long *)0xBE100B54 = *(unsigned long *)0xBE100B54 | 0x00007000;
		dbg("MII_MODE");
	}
	else
	{
	    *(unsigned long *)0xBE100B18 = (*(unsigned long *)0xBE100B18 & 0x00009859) | 0x00000786;
		*(unsigned long *)0xBE100B48 = (*(unsigned long *)0xBE100B48 & 0x00000fa7) | 0x00008058;
		*(unsigned long *)0xBE100B24 = *(unsigned long *)0xBE100B24 | 0x00000786;
		*(unsigned long *)0xBE100B54 = *(unsigned long *)0xBE100B54 | 0x00000058;
		dbg("REV_MII_MODE");
	}

    dbg("DANUBE_GPIO_P0_ALTSEL0(0xBE100B1C) = 0x%08X", *(unsigned int *)0xBE100B1C);
	dbg("DANUBE_GPIO_P0_ALTSEL1(0xBE100B20) = 0x%08X", *(unsigned int *)0xBE100B20);
	dbg("DANUBE_GPIO_P1_ALTSEL0(0xBE100B4C) = 0x%08X", *(unsigned int *)0xBE100B4C);
	dbg("DANUBE_GPIO_P1_ALTSEL1(0xBE100B50) = 0x%08X", *(unsigned int *)0xBE100B50);
	dbg("DANUBE_GPIO_P0_DIR(0xBE100B18)     = 0x%08X", *(unsigned int *)0xBE100B18);
	dbg("DANUBE_GPIO_P1_DIR(0xBE100B48)     = 0x%08X", *(unsigned int *)0xBE100B48);
	dbg("DANUBE_GPIO_P0_OD(0xBE100B24)      = 0x%08X", *(unsigned int *)0xBE100B24);
	dbg("DANUBE_GPIO_P1_OD(0xBE100B54)      = 0x%08X", *(unsigned int *)0xBE100B54);
  #else
    #error Must configure GPIO
  #endif    //  defined(DEBUG_WRITE_GPIO_REGISTER) && DEBUG_WRITE_GPIO_REGISTER
#endif  //  ENABLE_HARDWARE_EMULATION

#if defined(ENABLE_RX_DPLUS_PATH) && ENABLE_RX_DPLUS_PATH
    if ( 1 )
    {
        int i;
        u32 etop_cfg;
        u32 mac_cfg;
        u32 ig_plen_ctrl0;
        u32 mdio_cfg;

        etop_cfg      = *ETOP_CFG;
        mac_cfg       = *ENET_MAC_CFG(0);
        ig_plen_ctrl0 = *ETOP_IG_PLEN_CTRL0;
        mdio_cfg      = *ETOP_MDIO_CFG;
        *((volatile u32 *)(0xBF203000 + 0x0010)) |= (1 << 8);
        for ( i = 0; i < 0x1000; i++ );
        *ETOP_MDIO_CFG      = mdio_cfg;
        *ETOP_IG_PLEN_CTRL0 = ig_plen_ctrl0;
        *ENET_MAC_CFG(0)    = mac_cfg;
        *ETOP_CFG           = (etop_cfg | 0x01) & ~((1 << 6) | (1 << 8));
    }
#endif

    //  Stop fetching and sending
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    *ETOP_CFG = (*ETOP_CFG | ETOP_CFG_MII1_OFF(1)) & ~(ETOP_CFG_SEN1_ON(1) | ETOP_CFG_FEN1_ON(1));
#else
    *ETOP_CFG = (*ETOP_CFG | ETOP_CFG_MII1_OFF(1) | 0x01) & ~(ETOP_CFG_SEN1_ON(1) | ETOP_CFG_FEN1_ON(1) | (1 << 6) | (1 << 8)); //  Stop both eth0 and eth1
#endif

    //  Set min/max packet size
//    *ETOP_IG_PLEN_CTRL0 = ETOP_IG_PLEN_CTRL0_UNDER_SET(rx_min_packet_size) | ETOP_IG_PLEN_CTRL0_OVER_SET(rx_max_packet_size - ETH_MAC_HEADER_LENGTH);

#if 0   //  Qi Ming has disabled MDIO already in switch driver
    //  Disable MDIO
    ETOP_MDIO_CFG = 0x00;

    //  Enable CRC generation
    ENET_MAC_CFG_CRC_ON(1);

    //  Full Duplex
    ENET_MAC_CFG_DUPLEX_FULL(1);

    //  100M
    ENET_MAC_CFG_SPEED_100M(1);
#else
    *ENET_MAC_CFG(1) = 0x080F;  //  CRC, Egress Pause Frame, Full Duplex, 100M, Link On
//    *ENET_MAC_CFG(1) = 0x0807;  //  CRC, Full Duplex, 100M, Link On
#endif

    //  Configure 2nd ETH device
    *ENETS_DBA(1)     = ENETS1_DBA_DEFAULT;
    *ENETS_CBA(1)     = ENETS1_CBA_DEFAULT;
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    *ENETS_CFG(1)     = ENETS1_CFG_DEFAULT;
#else
    *ENETS_CFG(1)     = 0x506E;
//    *ENETS_PGCNT(0)   = (*ENETS_PGCNT(0) & ~0x0006000) | 0x00020000;
//    *ENETS_PKTCNT(0)  = (*ENETS_PKTCNT(0) & ~0x0600) | 0x0200;  //  PP32 is decrement source
    *ENETS_PGCNT(0)   = 0x00020000;
    *ENETS_PKTCNT(0)  = 0x0200;
#endif
    *ENETS_PGCNT(1)   = ENETS1_PGCNT_DEFAULT;
    *ENETS_PKTCNT(1)  = ENETS1_PKTCNT_DEFAULT;
    *ENETS_COS_CFG(1) = ENETS1_COS_CFG_DEFAULT;
    *ENETF_DBA(1)     = ENETF1_DBA_DEFAULT;
    *ENETF_CBA(1)     = ENETF1_CBA_DEFAULT;
#if 0
    *ENETF_CFG(1)     = ENETF1_CFG_DEFAULT;
#endif
    *ENETF_PGCNT(1)   = ENETF1_PGCNT_DEFAULT;
    *ENETF_PKTCNT(1)  = ENETF1_PKTCNT_DEFAULT;
#if defined(ENABLE_RX_DPLUS_PATH) && ENABLE_RX_DPLUS_PATH
//    *DPLUS_RXCFG      = *DPLUS_RXCFG & ~(1 << 15);  //  0x506E, disable DPLUS
//    *DPLUS_RXPGCNT    = (*DPLUS_RXPGCNT & ~0x00060000) | 0x00040000;    //  PP32 is increment source
    *DPLUS_RXCFG      = 0x506E;
    *DPLUS_RXPGCNT    = 0x00040000;
#endif

    //  Configure share buffer master selection
    *SB_MST_SEL |= 0x03;

    //  Config ETOP
#if defined(ENABLE_TX_CLK_INVERSION) && ENABLE_TX_CLK_INVERSION
    *ETOP_CFG |= ETOP_CFG_TCKINV1_ON(1);
#else
    *ETOP_CFG &= ~ETOP_CFG_TCKINV1_ON(1);
#endif  //  defined(ENABLE_TX_CLK_INVERSION) && ENABLE_TX_CLK_INVERSION
    switch ( mode )
    {
    default:
    case MII_MODE:
        *ETOP_CFG = (*ETOP_CFG & ~ETOP_CFG_MII1_MASK) | ETOP_CFG_MII1_OFF(0) | ETOP_CFG_REV_MII1_ON(0) | ETOP_CFG_TURBO_MII1_ON(0) | ETOP_CFG_SEN1_ON(1) | ETOP_CFG_FEN1_ON(1);
        break;
    case REV_MII_MODE:
        *ETOP_CFG = (*ETOP_CFG & ~ETOP_CFG_MII1_MASK) | ETOP_CFG_MII1_OFF(0) | ETOP_CFG_REV_MII1_ON(1) | ETOP_CFG_TURBO_MII1_ON(0) | ETOP_CFG_SEN1_ON(1) | ETOP_CFG_FEN1_ON(1);
    }
#if defined(ENABLE_RX_DPLUS_PATH) && ENABLE_RX_DPLUS_PATH
    *ETOP_CFG = (*ETOP_CFG & ~0x01) | (1 << 6) | (1 << 8);  //  Start eth0
#endif

    //  Init EMA
    init_ema();

    //  Enable mailbox
#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    *MBOX_IGU1_ISRC = 0xFFFFFFFF;
    *MBOX_IGU1_IER  = (1 << RX_TOTAL_CHANNEL_USED) - 1; //  enable RX interrupt only
    *MBOX_IGU3_ISRC = 0xFFFFFFFF;
    *MBOX_IGU3_IER  = ((1 << RX_TOTAL_CHANNEL_USED) - 1) | (((1 << TX_TOTAL_CHANNEL_USED) - 1) << 16);
#else
    *MBOX_IGU1_ISRC = 0xFFFFFFFF;
    *MBOX_IGU1_IER  = 0x00000000;   //  Don't need to enable RX interrupt, DMA driver handle RX path.
    *MBOX_IGU3_ISRC = 0xFFFFFFFF;
    *MBOX_IGU3_IER  = ((1 << TX_TOTAL_CHANNEL_USED) - 1) << 16; //  Never notice PPE firmware with RX event.
#endif

    dbg("ENETS_DBA(0)     = 0x%08X, ENETS_DBA(1)     = 0x%08X", *ENETS_DBA(0),    *ENETS_DBA(1));
    dbg("ENETS_CBA(0)     = 0x%08X, ENETS_CBA(1)     = 0x%08X", *ENETS_CBA(0),    *ENETS_CBA(1));
    dbg("ENETS_CFG(0)     = 0x%08X, ENETS_CFG(1)     = 0x%08X", *ENETS_CFG(0),    *ENETS_CFG(1));
    dbg("ENETS_PGCNT(0)   = 0x%08X, ENETS_PGCNT(1)   = 0x%08X", *ENETS_PGCNT(0),  *ENETS_PGCNT(1));
    dbg("ENETS_PKTCNT(0)  = 0x%08X, ENETS_PKTCNT(1)  = 0x%08X", *ENETS_PKTCNT(0), *ENETS_PKTCNT(1));
    dbg("ENETS_COS_CFG(0) = 0x%08X, ENETS_COS_CFG(1) = 0x%08X", *ENETS_COS_CFG(0),*ENETS_COS_CFG(1));
    dbg("ENETF_DBA(0)     = 0x%08X, ENETF_DBA(1)     = 0x%08X", *ENETF_DBA(0),    *ENETF_DBA(1));
    dbg("ENETF_CBA(0)     = 0x%08X, ENETF_CBA(1)     = 0x%08X", *ENETF_CBA(0),    *ENETF_CBA(1));
    dbg("ENETF_PGCNT(0)   = 0x%08X, ENETF_PGCNT(1)   = 0x%08X", *ENETF_PGCNT(0),  *ENETF_PGCNT(1));
    dbg("ENETF_PKTCNT(0)  = 0x%08X, ENETF_PKTCNT(1)  = 0x%08X", *ENETF_PKTCNT(0), *ENETF_PKTCNT(1));

    dbg("SB_MST_SEL = 0x%08X", *SB_MST_SEL);

    dbg("ETOP_CFG = 0x%08X", *ETOP_CFG);

    dbg("ETOP_IG_PLEN_CTRL0 = 0x%08X", *ETOP_IG_PLEN_CTRL0);

    dbg("ENET_MAC_CFG(0) = 0x%08X, ENET_MAC_CFG(1) = 0x%08X", *ENET_MAC_CFG(0), *ENET_MAC_CFG(1));
}

/*
 *  Description:
 *    Download PPE firmware binary code.
 *  Input:
 *    src       --- u32 *, binary code buffer
 *    dword_len --- unsigned int, binary code length in DWORD (32-bit)
 *  Output:
 *    int       --- 0:    Success
 *                  else: Error Code
 */
static INLINE int pp32_download_code(u32 *code_src, unsigned int code_dword_len, u32 *data_src, unsigned int data_dword_len)
{
    u32 reg_old_value;
    volatile u32 *dest;

    if ( code_src == 0 || ((unsigned long)code_src & 0x03) != 0
        || data_src == 0 || ((unsigned long)data_src & 0x03) )
        return -EINVAL;

    /*  save the old value of CDM_CFG and set PPE code memory to FPI bus access mode    */
    reg_old_value = *CDM_CFG;
    if ( code_dword_len <= 4096 )
        *CDM_CFG = CDM_CFG_RAM1_SET(0x00) | CDM_CFG_RAM0_SET(0x00);
    else
        *CDM_CFG = CDM_CFG_RAM1_SET(0x01) | CDM_CFG_RAM0_SET(0x00);

    /*  copy code   */
    dest = CDM_CODE_MEMORY_RAM0_ADDR(0);
    while ( code_dword_len-- > 0 )
        *dest++ = *code_src++;

    /*  copy data   */
    dest = PP32_DATA_MEMORY_RAM1_ADDR(0);
    while ( data_dword_len-- > 0 )
        *dest++ = *data_src++;

    /*  restore old configuration   */
//    *CDM_CFG = reg_old_value;

    return 0;
}

/*
 *  Description:
 *    Do PP32 specific initialization.
 *  Input:
 *    data --- void *, specific parameter passed in.
 *  Output:
 *    int  --- 0:    Success
 *             else: Error Code
 */
static INLINE int pp32_specific_init(void *data)
{
    return 0;
}

/*
 *  Description:
 *    Initialize and start up PP32.
 *  Input:
 *    none
 *  Output:
 *    int  --- 0:    Success
 *             else: Error Code
 */
static INLINE int pp32_start(void)
{
    int ret;
    register int i;

    /*  download firmware   */
    ret = pp32_download_code(firmware_binary_code, sizeof(firmware_binary_code) / sizeof(*firmware_binary_code), firmware_binary_data, sizeof(firmware_binary_data) / sizeof(*firmware_binary_data));
   if ( ret )
        return ret;

    /*  firmware specific initialization    */
    ret = pp32_specific_init(NULL);
    if ( ret )
        return ret;

    /*  run PP32    */
    *PP32_DBG_CTRL = DBG_CTRL_START_SET(1);
    /*  idle for a while to let PP32 init itself    */
    for ( i = 0; i < IDLE_CYCLE_NUMBER; i++ );

    return 0;
}

/*
 *  Description:
 *    Halt PP32.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void pp32_stop(void)
{
    /*  halt PP32   */
    *PP32_DBG_CTRL = DBG_CTRL_STOP_SET(1);
}

static INLINE void proc_file_create(void)
{
    g_eth2_proc_dir = proc_mkdir("eth2", NULL);

    create_proc_read_entry("stats",
                           0,
                           g_eth2_proc_dir,
                           proc_read_stats,
                           NULL);
}

static INLINE void proc_file_delete(void)
{
    remove_proc_entry("stats",
                      g_eth2_proc_dir);

    remove_proc_entry("eth2", NULL);
}

static int proc_read_stats(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    MOD_INC_USE_COUNT;

#if 0
    {
        unsigned int drop = *ENETS_IGDROP(1);
        unsigned int err  = *ENETS_IGERR(1);
        len += sprintf(page + off + len, "drop = %u, err = %u\n", drop, err);
    }
#endif

#if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
    /*
     *  update counters (eth2_get_stats)
     */

    eth2_dev.rx_drop_counter  += *ENETS_IGDROP(1);

    eth2_dev.stats.rx_packets  = ETH_MIB_TABLE->erx_pass_pdu;
    eth2_dev.stats.rx_bytes    = ETH_MIB_TABLE->erx_pass_bytes;
    eth2_dev.stats.rx_errors  += *ENETS_IGERR(1);
    eth2_dev.stats.rx_dropped  = ETH_MIB_TABLE->erx_dropdes_pdu + eth2_dev.rx_drop_counter;

    eth2_dev.stats.tx_packets  = ETH_MIB_TABLE->etx_total_pdu;
    eth2_dev.stats.tx_bytes    = ETH_MIB_TABLE->etx_total_bytes;
    eth2_dev.stats.tx_errors  += *ENETF_EGCOL(1);               /*  eth2_dev.stats.tx_errors is also updated in function eth2_hard_start_xmit   */
    eth2_dev.stats.tx_dropped += *ENETF_EGDROP(1);              /*  eth2_dev.stats.tx_dropped is updated in function eth2_hard_start_xmit       */
#endif  //  defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER

    /*
     *  print counters
     */

    len += sprintf(page + off + len, "ETH2 Stats\n");

#if defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER
    len += sprintf(page + off + len, "  Total\n");
    len += sprintf(page + off + len, "    rx_success        = %u\n", eth2_dev.rx_success);
    len += sprintf(page + off + len, "    rx_fail           = %u\n", eth2_dev.rx_fail);
  #if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    len += sprintf(page + off + len, "    rx_desc_read_pos  = %u, %u, %u, %u\n", eth2_dev.rx_desc_read_pos[0], eth2_dev.rx_desc_read_pos[1], eth2_dev.rx_desc_read_pos[2], eth2_dev.rx_desc_read_pos[3]);
  #endif
    len += sprintf(page + off + len, "    tx_success        = %u\n", eth2_dev.tx_success);
    len += sprintf(page + off + len, "    tx_desc_alloc_pos = %u, %u, %u, %u\n", eth2_dev.tx_desc_alloc_pos[0], eth2_dev.tx_desc_alloc_pos[1], eth2_dev.tx_desc_alloc_pos[2], eth2_dev.tx_desc_alloc_pos[3]);
    len += sprintf(page + off + len, "  Driver\n");
    len += sprintf(page + off + len, "    rx_error          = %u\n", eth2_dev.rx_driver_level_error);
    len += sprintf(page + off + len, "    rx_drop           = %u\n", eth2_dev.rx_driver_level_drop);
    len += sprintf(page + off + len, "    tx_drop           = %u\n", eth2_dev.tx_driver_level_drop);
    len += sprintf(page + off + len, "  Firmware MIB\n");
    len += sprintf(page + off + len, "    erx_pass_pdu      = %u\n", ETH_MIB_TABLE->erx_pass_pdu);
    len += sprintf(page + off + len, "    erx_pass_bytes    = %u\n", ETH_MIB_TABLE->erx_pass_bytes);
    len += sprintf(page + off + len, "    erx_dropdes_pdu   = %u\n", ETH_MIB_TABLE->erx_dropdes_pdu);
    len += sprintf(page + off + len, "    etx_total_pdu     = %u\n", ETH_MIB_TABLE->etx_total_pdu);
    len += sprintf(page + off + len, "    etx_total_bytes   = %u\n", ETH_MIB_TABLE->etx_total_bytes);
    len += sprintf(page + off + len, "  Hardware\n");
    len += sprintf(page + off + len, "    ENETS_IGERR       = %lu\n", eth2_dev.stats.rx_errors - eth2_dev.rx_driver_level_error);
    len += sprintf(page + off + len, "    ENETS_IGDROP      = %u\n", eth2_dev.rx_drop_counter - eth2_dev.rx_driver_level_drop);
    len += sprintf(page + off + len, "    ENETS_PGCNT:UPAGE = %u\n", *ENETS_PGCNT(1) & 0xFF);
    len += sprintf(page + off + len, "    ENETF_EGCOL       = %lu\n", eth2_dev.stats.tx_errors);
    len += sprintf(page + off + len, "    ENETF_EGDROP      = %lu\n", eth2_dev.stats.tx_dropped - eth2_dev.tx_driver_level_drop);
    len += sprintf(page + off + len, "    ENETF_PGCNT:UPAGE = %u\n", *ENETF_PGCNT(1) & 0xFF);
    len += sprintf(page + off + len, "  EMA\n");
    len += sprintf(page + off + len, "    CMDBUF_VCNT       = %u\n", *EMA_CMDCNT & 0xFF);
    len += sprintf(page + off + len, "    DATABUF_UCNT      = %u\n", *EMA_DATACNT & 0x3FF);
    len += sprintf(page + off + len, "  Some Switches\n");
    len += sprintf(page + off + len, "    TX_CLK_INV        = %s\n", (*ETOP_CFG & ETOP_CFG_TCKINV1_ON(1)) ? "INV" : "NORM");
    len += sprintf(page + off + len, "    netif_Q_stopped   = %s\n", netif_queue_stopped(&eth2_net_dev) ? "stopped" : "running");
    len += sprintf(page + off + len, "    netif_running     = %s\n", netif_running(&eth2_net_dev) ? "running" : "stopped");
    len += sprintf(page + off + len, "    netdev_fc_xoff    = %s\n", (eth2_dev.fc_bit && test_bit(eth2_dev.fc_bit, &netdev_fc_xoff)) ? "off" : "on");
    len += sprintf(page + off + len, "    ERX_DMACH_ON/ETX_DMACH_ON\n");
    len += sprintf(page + off + len, "      ERX_DMACH_ON    = %04X\n", *CFG_ERX_DMACH_ON & 0xFFFF);
    len += sprintf(page + off + len, "      ETX_DMACH_ON    = %04X\n", *CFG_ETX_DMACH_ON & 0xFFFF);
    len += sprintf(page + off + len, "    IGU1_IER\n");
    len += sprintf(page + off + len, "      rx_irq = %04X, IER = %04X\n", eth2_dev.rx_irq & 0xFFFF, *MBOX_IGU1_IER & 0xFFFF);
    len += sprintf(page + off + len, "      tx_irq = %04X, IER = %04X\n", eth2_dev.tx_irq & 0xFFFF, *MBOX_IGU1_IER >> 16);
    len += sprintf(page + off + len, "  MDIO and ENET Configure\n");
    len += sprintf(page + off + len, "    ETOP_MDIO_CFG     = %08X\n", *ETOP_MDIO_CFG);
    len += sprintf(page + off + len, "    ETOP_CFG          = %08X\n", *ETOP_CFG);
    len += sprintf(page + off + len, "    ENET_MAC_CFG      = %08X\n", *ENET_MAC_CFG(1));
    len += sprintf(page + off + len, "  Mailbox Signal Wait Loop\n");
    len += sprintf(page + off + len, "    RX Wait Loop      = %d\n", eth2_dev.rx_desc_update_wait_loop);
    len += sprintf(page + off + len, "    TX Wait Loop      = %d\n", eth2_dev.tx_desc_update_wait_loop);
#endif  //  defined(ENABLE_DEBUG_COUNTER) && ENABLE_DEBUG_COUNTER

    MOD_DEC_USE_COUNT;

    return len;
}

#if (defined(DEBUG_DUMP_RX_SKB) && DEBUG_DUMP_RX_SKB) || (defined(DEBUG_DUMP_TX_SKB) && DEBUG_DUMP_TX_SKB)
static INLINE void dump_skb(struct sk_buff *skb, int is_rx)
{
    int i;

   printk(is_rx ? "RX path --- sk_buff\n" : "TX path --- sk_buff\n");
    printk("  skb->data = %08X, skb->tail = %08X, skb->len = %d\n", (u32)skb->data, (u32)skb->tail, (int)skb->len);
    for ( i = 1; i <= skb->len; i++ )
    {
        if ( i % 16 == 1 )
            printk("  %4d:", i - 1);
        printk(" %02X", (int)(*((char*)skb->data + i - 1) & 0xFF));
        if ( i % 16 == 0 )
            printk("\n");
    }
    if ( (i - 1) % 16 != 0 )
        printk("\n");
}
#endif

#if defined(DEBUG_DUMP_ETOP_REGISTER) && DEBUG_DUMP_ETOP_REGISTER
static INLINE void dump_etop0_reg()
{
    printk("ETOP0 Registers:\n");

    printk("  ETOP_CFG\n");
    printk("    MII0:    %s\n", (char*)(!(*ETOP_CFG & 0x0001) ? "ON" : "OFF"));
    printk("    Mode:    %s\n", (char*)(!(*ETOP_CFG & 0x0002) ? "MII" : "Rev MII"));
    printk("    Turbo:   %s\n", (char*)(!(*ETOP_CFG & 0x0004) ? "Normal" : "Turbo"));
    printk("    Ingress: %s\n", (char*)(!(*ETOP_CFG & 0x0040) ? "Disable" : "Enable"));
    printk("    Egress:  %s\n", (char*)(!(*ETOP_CFG & 0x0100) ? "Disable" : "Enable"));
    printk("    Clock:   %s\n", (char*)(!(*ETOP_CFG & 0x0400) ? "Normal" : "Inversed"));

    printk("  ENETS_CFG\n");
    printk("    HDLEN:   %d\n", (*ENETS_CFG(0) >> 18) & 0x7F);
    printk("    PNUM:    %d\n", *ENETS_CFG(0) & 0xFF);

    printk("  ENETS_PGCNT\n");
    printk("    DSRC:    %s\n", (char*)((*ENETS_PGCNT(0) & 0x040000) ? ((*ENETS_PGCNT(0) & 0x020000) ? "Cross" : "Local") : ((*ENETS_PGCNT(0) & 0x020000) ? "PP32" : "DPLUS")));
    printk("    UPAGE:   %d\n", *ENETS_PGCNT(0) & 0xFF);

    printk("  ENETS_PKTCNT\n");
    printk("    UPKT:    %d\n", *ENETS_PKTCNT(0) & 0xFF);
}

static INLINE void dump_etop1_reg()
{
    printk("ETOP1 Registers:\n");

    printk("  ETOP_CFG\n");
    printk("    MII0:     %s\n", (char*)(!(*ETOP_CFG & 0x0008) ? "ON" : "OFF"));
    printk("    Mode:     %s\n", (char*)(!(*ETOP_CFG & 0x0010) ? "MII" : "Rev MII"));
    printk("    Turbo:    %s\n", (char*)(!(*ETOP_CFG & 0x0020) ? "Normal" : "Turbo"));
    printk("    Ingress:  %s\n", (char*)(!(*ETOP_CFG & 0x0080) ? "Disable" : "Enable"));
    printk("    Egress:   %s\n", (char*)(!(*ETOP_CFG & 0x0200) ? "Disable" : "Enable"));
    printk("    Clock:    %s\n", (char*)(!(*ETOP_CFG & 0x0800) ? "Normal" : "Inversed"));

    printk("  ENETS_CFG\n");
    printk("    HDLEN:   %d\n", (*ENETS_CFG(1) >> 18) & 0x7F);
    printk("    PNUM:    %d\n", *ENETS_CFG(1) & 0xFF);

    printk("  ENETS_PGCNT\n");
    printk("    DSRC:    %s\n", (char*)((*ENETS_PGCNT(1) & 0x040000) ? ((*ENETS_PGCNT(1) & 0x020000) ? "Cross" : "Local") : ((*ENETS_PGCNT(1) & 0x020000) ? "PP32" : "DPLUS")));
    printk("    UPAGE:   %d\n", *ENETS_PGCNT(1) & 0xFF);

    printk("  ENETS_PKTCNT\n");
    printk("    UPKT:    %d\n", *ENETS_PKTCNT(1) & 0xFF);
}

static INLINE void dump_etop_reg()
{
    dump_etop0_reg();
    dump_etop1_reg();
}
#endif

#if defined(ENABLE_TWINPATH_BOARD) && ENABLE_TWINPATH_BOARD
extern int ifx_sw_vlan_add(int, int, int);
extern int ifx_sw_vlan_del(int, int);
static INLINE void board_init(void)
{
  #if 1

    int i, j;

    for(i = 0; i< 6; i++)
        for(j = 0; j<6; j++)
            ifx_sw_vlan_del(i, j);

    ifx_sw_vlan_add(1, 1, 0);
    ifx_sw_vlan_add(2, 2, 0);
    ifx_sw_vlan_add(4, 4, 0);
    ifx_sw_vlan_add(2, 1, 0);
    ifx_sw_vlan_add(4, 1, 0);
    ifx_sw_vlan_add(1, 2, 0);
    ifx_sw_vlan_add(2, 2, 0);
    ifx_sw_vlan_add(4, 2, 0);
    ifx_sw_vlan_add(1, 4, 0);
    ifx_sw_vlan_add(2, 4, 0);
    ifx_sw_vlan_add(4, 4, 0);

    ifx_sw_vlan_add(3, 3, 1);
    ifx_sw_vlan_add(5, 5, 1);
    ifx_sw_vlan_add(3, 5, 1);
    ifx_sw_vlan_add(5, 3, 1);

  #else

    int i, j;

    for(i = 0; i< 6; i++)
        for(j = 0; j<6; j++)
            ifx_sw_vlan_del(i, j);

    ifx_sw_vlan_add(1, 1);
    ifx_sw_vlan_add(2, 2);
    ifx_sw_vlan_add(4, 4);
    ifx_sw_vlan_add(2, 1);
    ifx_sw_vlan_add(4, 1);
    ifx_sw_vlan_add(1, 2);
    ifx_sw_vlan_add(2, 2);
    ifx_sw_vlan_add(4, 2);
    ifx_sw_vlan_add(1, 4);
    ifx_sw_vlan_add(2, 4);
    ifx_sw_vlan_add(4, 4);

    ifx_sw_vlan_add(3, 3);
    ifx_sw_vlan_add(5, 5);
    ifx_sw_vlan_add(3, 5);
    ifx_sw_vlan_add(5, 3);

  #endif
}
#endif



/*
 * ####################################
 *           Global Function
 * ####################################
 */


/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

/*
 *  Description:
 *    Initialize global variables, PP32, comunication structures, register IRQ
 *    and register device.
 *  Input:
 *    none
 *  Output:
 *    0    --- successful
 *    else --- failure, usually it is negative value of error code
 */
int __init danube_eth2_init(void)
{
    int ret;

    printk("Loading 2nd ETH driver... ");

    check_parameters();

    ret = init_eth2_dev();
    if ( ret )
        goto INIT_ETH2_DEV_FAIL;

    clear_share_buffer();

    init_tables();

#if !defined(ENABLE_RX_DPLUS_PATH) || !ENABLE_RX_DPLUS_PATH
    /*  Disable RX  */
//    *CFG_ERX_DMACH_ON = 0x00;
#endif

    /*  create device   */
    ret = register_netdev(&eth2_net_dev);
    if ( ret )
        goto REGISTER_NETDEV_FAIL;

#if !defined(ENABLE_HARDWARE_EMULATION) || !ENABLE_HARDWARE_EMULATION
    /*  register interrupt handler  */
    ret = request_irq(PPE_MAILBOX_IGU1_INT, mailbox_irq_handler, SA_INTERRUPT, "eth2_dma_isr", NULL);
    if ( ret )
        goto REQUEST_IRQ_PPE_MAILBOX_IGU1_INT_FAIL;
#endif  //  !defined(ENABLE_HARDWARE_EMULATION) || !ENABLE_HARDWARE_EMULATION

    init_chip(MII_MODE_SETUP);
    eth2_dev.rx_irq =  *MBOX_IGU1_IER & 0x0000FFFF;
    eth2_dev.tx_irq = (*MBOX_IGU1_IER & 0xFFFF0000) >> 16;

#if defined(ENABLE_TWINPATH_BOARD) && ENABLE_TWINPATH_BOARD
    board_init();
#endif

    ret = pp32_start();
    if ( ret )
        goto PP32_START_FAIL;

    /*  careful, PPE firmware may set some registers, recover them here */
    *MBOX_IGU1_IER = (eth2_dev.tx_irq << 16) | eth2_dev.rx_irq;

    /*  create proc file    */
    proc_file_create();

#if defined(ENABLE_DIRECT_BRIDGE) && ENABLE_DIRECT_BRIDGE
    switch_eth2_dev = &eth2_net_dev;
#endif

    printk("init succeeded\n");
    return 0;

PP32_START_FAIL:
#if !defined(ENABLE_HARDWARE_EMULATION) || !ENABLE_HARDWARE_EMULATION
    free_irq(PPE_MAILBOX_IGU1_INT, NULL);
REQUEST_IRQ_PPE_MAILBOX_IGU1_INT_FAIL:
#endif  //  !defined(ENABLE_HARDWARE_EMULATION) || !ENABLE_HARDWARE_EMULATION
    unregister_netdev(&eth2_net_dev);
REGISTER_NETDEV_FAIL:
    clear_eth2_dev();
INIT_ETH2_DEV_FAIL:
    printk(" init failed\n");
    return ret;
}

/*
 *  Description:
 *    Release memory, free IRQ, and deregister device.
 *  Input:
 *    none
 *  Output:
 *    none
 */
void __exit danube_eth2_exit(void)
{
    proc_file_delete();

    pp32_stop();

#if !defined(ENABLE_HARDWARE_EMULATION) || !ENABLE_HARDWARE_EMULATION
    free_irq(PPE_MAILBOX_IGU1_INT, NULL);
#endif  //  !defined(ENABLE_HARDWARE_EMULATION) || !ENABLE_HARDWARE_EMULATION

    unregister_netdev(&eth2_net_dev);

    clear_eth2_dev();
}

static int __init danube_ethaddr_setup(char *line)
{
    char *ep;
    int i;

    memset(MY_ETHADDR, 0, sizeof(MY_ETHADDR));
    for ( i = 0; i < 6; i++ )
    {
        MY_ETHADDR[i] = line ? simple_strtoul(line, &ep, 16) : 0;
        if ( line )
            line = *ep ? ep + 1 : ep;
    }
    dbg("2nd eth mac address %02X-%02X-%02X-%02X-%02X-%02X\n",
        MY_ETHADDR[0],
        MY_ETHADDR[1],
        MY_ETHADDR[2],
        MY_ETHADDR[3],
        MY_ETHADDR[4],
        MY_ETHADDR[5]);

    return 0;
}


module_init(danube_eth2_init);
module_exit(danube_eth2_exit);
__setup("ethaddr=", danube_ethaddr_setup);
