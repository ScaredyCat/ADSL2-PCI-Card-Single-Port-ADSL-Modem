/******************************************************************************
**
** FILE NAME    : danube_ppe.c
** PROJECT      : Danube
** MODULES     	: ATM (ADSL)
**
** DATE         : 1 AUG 2005
** AUTHOR       : Xu Liang
** DESCRIPTION  : ATM Driver
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
**  1 AUG 2005  Xu Liang        Initiate Version
**  8 SEP 2006  Xu Liang        Add support for A11 (including fix for PCI)
** 15 SEP 2006  Xu Liang        Reconfigure QSB everytime the upper limit cell
**                              rate is set
**  2 OCT 2006  Xu Liang        Add config option and register function for
**                              set_cell_rate and adsl_led functions.
*******************************************************************************/

//static int g_tx_desc_num = 0;
//static struct sk_buff *my_sk_push[2048] = {0};
//static struct sk_buff *my_sk_pop[2048] = {0};
//static struct sk_buff **my_sk_push_pointer = my_sk_push, **my_sk_pop_pointer = my_sk_pop;



/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/config.h>
#include <linux/config.h>       /* retrieve the CONFIG_* macros */
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#   define MODVERSIONS
#endif

#if defined(MODVERSIONS) && !defined(__GENKSYMS__)
#    include <linux/modversions.h>
#endif

#ifndef EXPORT_SYMTAB
#  define EXPORT_SYMTAB         /* need this one 'cause we export symbols */
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/atmdev.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/irq.h>
#include <asm/io.h>
//#include <asm/delay.h>
#include <linux/errno.h>

/*
 *  Chip Specific Head File
 */
#include <asm/danube/irq.h>
#include <asm/danube/atm_mib.h>

#include <asm/danube/danube_cgu.h>
#include <asm/danube/danube_ppe.h>
#ifndef CONFIG_PCI
#include <asm/danube/danube_ppe_fw.h>
#endif

extern void amz_push_oam(unsigned char *);



/*
 * ####################################
 *   Parameters to Configure PPE
 * ####################################
 */
static int port_max_connection[2] = {7, 7};     /*  Maximum number of connections for ports (0-14)  */
static int port_cell_rate_up[2] = {3200, 3200}; /*  Maximum TX cell rate for ports                  */
//static int port_cell_rate_up[2] = {82547, 5000}; /*  Maximum TX cell rate for ports                  */

static int qsb_tau   = 1;                       /*  QSB cell delay variation due to concurrency     */
static int qsb_srvm  = 0x0F;                    /*  QSB scheduler burst length                      */
static int qsb_tstep = 4 ;                      /*  QSB time step, all legal values are 1, 2, 4     */

static int write_descriptor_delay  = 0x20;      /*  Write descriptor delay                          */

static int aal5_fill_pattern       = 0x007E;    /*  AAL5 padding byte ('~')                         */
static int aal5r_max_packet_size   = 0x0700;    /*  Max frame size for RX                           */
static int aal5r_min_packet_size   = 0x0000;    /*  Min frame size for RX                           */
static int aal5s_max_packet_size   = 0x0700;    /*  Max frame size for TX                           */
static int aal5s_min_packet_size   = 0x0000;    /*  Min frame size for TX                           */
static int aal5r_drop_error_packet = 1;         /*  Drop error packet in RX path                    */

static int dma_rx_descriptor_length = 48;       /*  Number of descriptors per DMA RX channel        */
//static int dma_rx_descriptor_length =  8;       /*  Number of descriptors per DMA RX channel        */
static int dma_tx_descriptor_length = 64;       /*  Number of descriptors per DMA TX channel        */
static int dma_rx_clp1_descriptor_threshold = 38;

MODULE_PARM(port_max_connection, "2-2i");
MODULE_PARM_DESC(port_max_connection, "Maximum atm connection for port (0-1)");
MODULE_PARM(port_cell_rate_up, "2-2i");
MODULE_PARM_DESC(port_cell_rate_up, "ATM port upstream rate in cells/s");

MODULE_PARM(qsb_tau,"i");
MODULE_PARM_DESC(qsb_tau, "Cell delay variation. Value must be > 0");
MODULE_PARM(qsb_srvm, "i");
MODULE_PARM_DESC(qsb_srvm, "Maximum burst size");
MODULE_PARM(qsb_tstep, "i");
MODULE_PARM_DESC(qsb_tstep, "n*32 cycles per sbs cycles n=1,2,4");

MODULE_PARM(write_descriptor_delay, "i" );
MODULE_PARM_DESC(write_descriptor_delay, "PPE core clock cycles between descriptor write and effectiveness in external RAM");

MODULE_PARM(a5_fill_pattern, "i");
MODULE_PARM_DESC(a5_fill_pattern, "Filling pattern (PAD) for AAL5 frames");
MODULE_PARM(aal5r_max_packet_size, "i");
MODULE_PARM_DESC(aal5r_max_packet_size, "Max packet size in byte for downstream AAL5 frames");
MODULE_PARM(aal5r_min_packet_size, "i");
MODULE_PARM_DESC(aal5r_min_packet_size, "Min packet size in byte for downstream AAL5 frames");
MODULE_PARM(aal5s_max_packet_size, "i");
MODULE_PARM_DESC(aal5s_max_packet_size, "Max packet size in byte for upstream AAL5 frames");
MODULE_PARM(aal5s_min_packet_size, "i");
MODULE_PARM_DESC(aal5s_min_packet_size, "Min packet size in byte for upstream AAL5 frames");
MODULE_PARM(aal5r_drop_error_packet, "i");
MODULE_PARM_DESC(aal5r_drop_error_packet, "Non-zero value to drop error packet for downstream");

MODULE_PARM(dma_rx_descriptor_length, "i");
MODULE_PARM_DESC(dma_rx_descriptor_length, "Number of descriptor assigned to DMA RX channel (>16)");
MODULE_PARM(dma_tx_descriptor_length, "i");
MODULE_PARM_DESC(dma_tx_descriptor_length, "Number of descriptor assigned to DMA TX channel (>16)");
MODULE_PARM(dma_rx_clp1_descriptor_threshold, "i");
MODULE_PARM_DESC(dma_rx_clp1_descriptor_threshold, "Descriptor threshold for cells with cell loss priority 1");


/*
 * ####################################
 *              Definition
 * ####################################
 */

#ifdef CONFIG_DANUBE_MEI
  #define CONFIG_IFX_ADSL_ATM_LED   // adsl atm traffic led support
#endif

#define ENABLE_TR067_LOOPBACK           0
#define ENABLE_TR067_LOOPBACK_PROC      0
#define ENABLE_TR067_LOOPBACK_DUMP      0

#define DISABLE_DANUBE_PPE_SET_CELL_RATE    0

#define LINUX_2_4_31                    1

#define DEBUG_ABANDON_RX                0

#define DEBUG_SHOW_COUNT                0

#define DEBUG_ON_AMAZON                 0

#define DEBUG_ON_VENUS                  0

#define DEBUG_DUMP_SKB                  0

#define DEBUG_QOS                       0

#define DEBUG_CONNECTION_THROUGHPUT     0

#define DISABLE_QSB                     0

#define DISABLE_VBR                     0

#define ENABLE_RX_QOS                   1

#define WAIT_FOR_TX_DESC                0

#define STATS_ON_VCC_BASIS              1

#define PPE_MAILBOX_IGU1_INT            INT_NUM_IM2_IRL24

#define QSB_IS_HALF_FPI2_CLOCK          0

#define ADDR_MSB_FIX                    ((u32)0x80000000)

#ifndef ATM_PDU_OVHD
  #define ATM_PDU_OVHD                  0
#endif

#if defined(DISABLE_QSB) && DISABLE_QSB && defined(DISABLE_VBR) && !DISABLE_VBR
  #undef DISABLE_VBR
  #define DISABLE_VBR                   1
#endif

#if (defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON) || (defined(DEBUG_ON_VENUS) && DEBUG_ON_VENUS)
  #undef dbg
  #define dbg(format, arg...)           printk(KERN_DEBUG __FILE__ ":" format "\n", ##arg)

  #define INLINE
#else
  #ifndef dbg
    #define dbg(format, arg...)         do {} while (0)
  #endif

  #define INLINE                        inline
#endif  //  (defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON) || (defined(DEBUG_ON_VENUS) && DEBUG_ON_VENUS)

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
  #include <asm/danube/danube_gptu.h>

  #define POLL_FREQ                     5
  #define DISPLAY_INTERVAL              2
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

#if defined(CONFIG_DANUBE_CHIP_A11) && defined(CONFIG_DANUBE_PPE_PCI_SOFTWARE_ARBITOR)
  #define USE_FIX_FOR_PCI_PPE           1
#endif

#if defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
  #include <asm/danube/danube_ppe_fw_fix_for_pci.h>

  #define PPE_MAILBOX_IGU0_INT          INT_NUM_IM2_IRL23

  extern void danube_disable_external_pci(void);
  extern void danube_enable_external_pci(void);
#elif defined(CONFIG_PCI)
  #include <asm/danube/danube_ppe_fw.h>
#endif  //  defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE

/*
 *  Bits Operation
 */
#define GET_BITS(x, msb, lsb)           (((x) & ((1 << ((msb) + 1)) - 1)) >> (lsb))
#define SET_BITS(x, msb, lsb, value)    (((x) & ~(((1 << ((msb) + 1)) - 1) ^ ((1 << (lsb)) - 1))) | (((value) & ((1 << (1 + (msb) - (lsb))) - 1)) << (lsb)))

/*
 *  EMA Settings
 */
#define EMA_CMD_BUF_LEN      0x0040
#define EMA_CMD_BASE_ADDR    (0x00001580 << 2)
#define EMA_DATA_BUF_LEN     0x0100
#define EMA_DATA_BASE_ADDR   (0x00001900 << 2)
#define EMA_WRITE_BURST      0x2
#define EMA_READ_BURST       0x2
//#define EMA_WRITE_BURST      0x0
//#define EMA_READ_BURST       0x0

/*
 *  ATM Port, QSB Queue, DMA RX/TX Channel Parameters
 */
#define ATM_PORT_NUMBER                 2
#define MAX_QUEUE_NUMBER                16
#define QSB_QUEUE_NUMBER_BASE           1
#define MAX_QUEUE_NUMBER_PER_PORT       (MAX_QUEUE_NUMBER - QSB_QUEUE_NUMBER_BASE)
#define MAX_CONNECTION_NUMBER           MAX_QUEUE_NUMBER
#define MAX_RX_DMA_CHANNEL_NUMBER       8
#define MAX_TX_DMA_CHANNEL_NUMBER       16
#define DMA_ALIGNMENT                   4

#define DEFAULT_RX_HUNT_BITTH           4

/*
 *  RX DMA Channel Allocation For Different QoS Level
 */
#define RX_DMA_CH_CBR                   0
#define RX_DMA_CH_VBR_RT                1
#define RX_DMA_CH_VBR_NRT               2
#define RX_DMA_CH_AVR                   3
#define RX_DMA_CH_UBR                   4
#define RX_DMA_CH_OAM                   5
#define RX_DMA_CH_TOTAL                 6

/*
 *  QSB Queue Scheduling and Shaping Definitions
 */
#define QSB_WFQ_NONUBR_MAX              0x3f00
#define QSB_WFQ_UBR_BYPASS              0x3fff
#define QSB_TP_TS_MAX                   65472
#define QSB_TAUS_MAX                    64512
#define QSB_GCR_MIN                     18

/*
 *  OAM Definitions
 */
#define OAM_RX_QUEUE_NUMBER             1
#define OAM_TX_QUEUE_NUMBER_PER_PORT    0
#define OAM_RX_DMA_CHANNEL_NUMBER       OAM_RX_QUEUE_NUMBER
#define OAM_HTU_ENTRY_NUMBER            3
#define OAM_F4_SEG_HTU_ENTRY            0
#define OAM_F4_TOT_HTU_ENTRY            1
#define OAM_F5_HTU_ENTRY                2
#define OAM_F4_CELL_ID                  0
#define OAM_F5_CELL_ID                  15

/*
 *  RX Frame Definitions
 */
#define MAX_RX_PACKET_ALIGN_BYTES       3
#define MAX_RX_PACKET_PADDING_BYTES     3
#define RX_INBAND_TRAILER_LENGTH        8
#define MAX_RX_FRAME_EXTRA_BYTES        (RX_INBAND_TRAILER_LENGTH + MAX_RX_PACKET_ALIGN_BYTES + MAX_RX_PACKET_PADDING_BYTES)

/*
 *  TX Frame Definitions
 */
#define MAX_TX_HEADER_ALIGN_BYTES       12
#define MAX_TX_PACKET_ALIGN_BYTES       3
#define MAX_TX_PACKET_PADDING_BYTES     3
#define TX_INBAND_HEADER_LENGTH         8
#define MAX_TX_FRAME_EXTRA_BYTES        (TX_INBAND_HEADER_LENGTH + MAX_TX_HEADER_ALIGN_BYTES + MAX_TX_PACKET_ALIGN_BYTES + MAX_TX_PACKET_PADDING_BYTES)

#define IDLE_CYCLE_NUMBER               30000

#define CELL_SIZE                       ATM_AAL0_SDU

#define WRX_DMA_CHANNEL_INTERRUPT_MODE  0x00
#define WRX_DMA_CHANNEL_POLLING_MODE    0x01
//#define WRX_DMA_CHANNEL_COUNTER_MODE    0x02
#define WRX_DMA_CHANNEL_COUNTER_MODE    WRX_DMA_CHANNEL_INTERRUPT_MODE

#define WRX_DMA_BUF_LEN_PER_DESCRIPTOR  0x00
#define WRX_DMA_BUF_LEN_PER_CHANNEL     0x01

/*  QSB */
#define QSB_RAMAC_RW_READ               0
#define QSB_RAMAC_RW_WRITE              1

#define QSB_RAMAC_TSEL_QPT              0x01
#define QSB_RAMAC_TSEL_SCT              0x02
#define QSB_RAMAC_TSEL_SPT              0x03
#define QSB_RAMAC_TSEL_VBR              0x08

#define QSB_RAMAC_LH_LOW                0
#define QSB_RAMAC_LH_HIGH               1

#define QSB_QPT_SET_MASK                0x0
#define QSB_QVPT_SET_MASK               0x0
#define QSB_SET_SCT_MASK                0x0
#define QSB_SET_SPT_MASK                0x0
#define QSB_SET_SPT_SBVALID_MASK        0x7FFFFFFF

#define QSB_SPT_SBV_VALID               (1 << 31)
#define QSB_SPT_PN_SET(value)           (((value) & 0x01) ? (1 << 16) : 0)
#define QSB_SPT_INTRATE_SET(value)      SET_BITS(0, 13, 0, value)

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
 *  Host-PPE Communication Data Address Mapping
 */
#define CFG_WRX_HTUTS                   PPM_INT_UNIT_ADDR(0x2400)   /*  WAN RX HTU Table Size, must be configured before enable PPE firmware.   */
#define CFG_WRX_QNUM                    PPM_INT_UNIT_ADDR(0x2401)   /*  WAN RX Queue Number */
#define CFG_WRX_DCHNUM                  PPM_INT_UNIT_ADDR(0x2402)   /*  WAN RX DMA Channel Number, no more than 8, must be configured before enable PPE firmware.   */
#define CFG_WTX_DCHNUM                  PPM_INT_UNIT_ADDR(0x2403)   /*  WAN TX DMA Channel Number, no more than 16, must be configured before enable PPE firmware.  */
#define CFG_WRDES_DELAY                 PPM_INT_UNIT_ADDR(0x2404)   /*  WAN Descriptor Write Delay, must be configured before enable PPE firmware.  */
#define WRX_DMACH_ON                    PPM_INT_UNIT_ADDR(0x2405)   /*  WAN RX DMA Channel Enable, must be configured before enable PPE firmware.   */
#define WTX_DMACH_ON                    PPM_INT_UNIT_ADDR(0x2406)   /*  WAN TX DMA Channel Enable, must be configured before enable PPE firmware.   */
#define WRX_HUNT_BITTH                  PPM_INT_UNIT_ADDR(0x2407)   /*  WAN RX HUNT Threshold, must be between 2 to 8.  */
#define WRX_QUEUE_CONFIG(i)             ((struct wrx_queue_config*)PPM_INT_UNIT_ADDR(0x2500 + (i) * 20))
#define WRX_DMA_CHANNEL_CONFIG(i)       ((struct wrx_dma_channel_config*)PPM_INT_UNIT_ADDR(0x2640 + (i) * 7))
#define WTX_PORT_CONFIG(i)              ((struct wtx_port_config*)PPM_INT_UNIT_ADDR(0x2440 + (i)))
#define WTX_QUEUE_CONFIG(i)             ((struct wtx_queue_config*)PPM_INT_UNIT_ADDR(0x2710 + (i) * 27))
#define WTX_DMA_CHANNEL_CONFIG(i)       ((struct wtx_dma_channel_config*)PPM_INT_UNIT_ADDR(0x2711 + (i) * 27))
#define WAN_MIB_TABLE                   ((struct wan_mib_table*)PPM_INT_UNIT_ADDR(0x2410))
#define HTU_ENTRY(i)                    ((struct htu_entry*)PPM_INT_UNIT_ADDR(0x2000 + (i)))
#define HTU_MASK(i)                     ((struct htu_mask*)PPM_INT_UNIT_ADDR(0x2020 + (i)))
#define HTU_RESULT(i)                   ((struct htu_result*)PPM_INT_UNIT_ADDR(0x2040 + (i)))

/*
 *  PP32 Debug Control Register
 */
#define PP32_DBG_CTRL                   PP32_DEBUG_REG_ADDR(0x0000)

#define DBG_CTRL_START_SET(value)       ((value) ? (1 << 0) : 0)
#define DBG_CTRL_STOP_SET(value)        ((value) ? (1 << 1) : 0)
#define DBG_CTRL_STEP_SET(value)        ((value) ? (1 << 2) : 0)

/*
 *  Code/Data Memory (CDM) Interface Configuration Register
 */
#define CDM_CFG                         PPE_REG_ADDR(0x0100)

#define CDM_CFG_RAM1                    GET_BITS(*CDM_CFG, 3, 2)
#define CDM_CFG_RAM0                    (*CDM_CFG & (1 << 1))

#define CDM_CFG_RAM1_SET(value)         SET_BITS(0, 3, 2, value)
#define CDM_CFG_RAM0_SET(value)         ((value) ? (1 << 1) : 0)

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
 *  QSB Internal Cell Delay Variation Register
 */
#define QSB_ICDV                        QSB_CONF_REG(0x0007)

#define QSB_ICDV_TAU                    GET_BITS(*QSB_ICDV, 5, 0)

#define QSB_ICDV_TAU_SET(value)         SET_BITS(0, 5, 0, value)

/*
 *  QSB Scheduler Burst Limit Register
 */
#define QSB_SBL                         QSB_CONF_REG(0x0009)

#define QSB_SBL_SBL                     GET_BITS(*QSB_SBL, 3, 0)

#define QSB_SBL_SBL_SET(value)          SET_BITS(0, 3, 0, value)

/*
 *  QSB Configuration Register
 */
#define QSB_CFG                         QSB_CONF_REG(0x000A)

#define QSB_CFG_TSTEPC                  GET_BITS(*QSB_CFG, 1, 0)

#define QSB_CFG_TSTEPC_SET(value)       SET_BITS(0, 1, 0, value)

/*
 *  QSB RAM Transfer Table Register
 */
#define QSB_RTM                         QSB_CONF_REG(0x000B)

#define QSB_RTM_DM                      (*QSB_RTM)

#define QSB_RTM_DM_SET(value)           ((value) & 0xFFFFFFFF)

/*
 *  QSB RAM Transfer Data Register
 */
#define QSB_RTD                         QSB_CONF_REG(0x000C)

#define QSB_RTD_TTV                     (*QSB_RTD)

#define QSB_RTD_TTV_SET(value)          ((value) & 0xFFFFFFFF)

/*
 *  QSB RAM Access Register
 */
#define QSB_RAMAC                       QSB_CONF_REG(0x000D)

#define QSB_RAMAC_RW                    (*QSB_RAMAC & (1 << 31))
#define QSB_RAMAC_TSEL                  GET_BITS(*QSB_RAMAC, 27, 24)
#define QSB_RAMAC_LH                    (*QSB_RAMAC & (1 << 16))
#define QSB_RAMAC_TESEL                 GET_BITS(*QSB_RAMAC, 9, 0)

#define QSB_RAMAC_RW_SET(value)         ((value) ? (1 << 31) : 0)
#define QSB_RAMAC_TSEL_SET(value)       SET_BITS(0, 27, 24, value)
#define QSB_RAMAC_LH_SET(value)         ((value) ? (1 << 16) : 0)
#define QSB_RAMAC_TESEL_SET(value)      SET_BITS(0, 9, 0, value)

/*
 *  Mailbox IGU0 Registers
 */
#define MBOX_IGU0_ISRS                  PPE_REG_ADDR(0x0200)
#define MBOX_IGU0_ISRC                  PPE_REG_ADDR(0x0201)
#define MBOX_IGU0_ISR                   PPE_REG_ADDR(0x0202)
#define MBOX_IGU0_IER                   PPE_REG_ADDR(0x0203)

#define MBOX_IGU0_ISRS_SET(n)           (1 << (n))
#define MBOX_IGU0_ISRC_CLEAR(n)         (1 << (n))
#define MBOX_IGU0_ISR_ISR(n)            (*MBOX_IGU0_ISR & (1 << (n)))
#define MBOX_IGU0_IER_EN(n)             (*MBOX_IGU0_IER & (1 << (n)))
#define MBOX_IGU0_IER_EN_SET(n)         (1 << (n))

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
 *  DREG Idle Counters
 */
#define DREG_AT_CELL0                   PPE_REG_ADDR(0x0D24)
#define DREG_AT_CELL1                   PPE_REG_ADDR(0x0D25)
#define DREG_AT_IDLE_CNT0               PPE_REG_ADDR(0x0D26)
#define DREG_AT_IDLE_CNT1               PPE_REG_ADDR(0x0D27)
#define DREG_AR_CELL0                   PPE_REG_ADDR(0x0D68)
#define DREG_AR_CELL1                   PPE_REG_ADDR(0x0D69)
#define DREG_AR_IDLE_CNT0               PPE_REG_ADDR(0x0D6A)
#define DREG_AR_IDLE_CNT1               PPE_REG_ADDR(0x0D6B)
#define DREG_AR_AIIDLE_CNT0             PPE_REG_ADDR(0x0D6C)
#define DREG_AR_AIIDLE_CNT1             PPE_REG_ADDR(0x0D6D)
#define DREG_AR_BE_CNT0                 PPE_REG_ADDR(0x0D6E)
#define DREG_AR_BE_CNT1                 PPE_REG_ADDR(0x0D6F)


/*
 * ####################################
 * Preparation of Debug on Amazon Chip
 * ####################################
 */

/*
 *  If try module on Amazon chip, prepare some tricks to prevent invalid memory write.
 */
#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    u32 g_pFakeRegisters[0xD000] = {0};

    #undef  DANUBE_PPE
    #define DANUBE_PPE                  ((u32)g_pFakeRegisters)

    int debug_thread_running = 0;
    int debug_rx_desc_start_point[8] = {0};
    int debug_tx_desc_start_point[16] = {0};

  #if 0
    static int debug_thread_proc(void *);
  #endif
    static int debug_ppe_sim(void);
    static INLINE void init_debug(void);
    static INLINE void clear_debug(void);
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON


/*
 * ####################################
 *  Preparation of STATA ON VCC BASIS
 * ####################################
 */
#if defined(STATS_ON_VCC_BASIS) && STATS_ON_VCC_BASIS
    #define UPDATE_VCC_STAT(conn, item, num)    do { ppe_dev.connection[conn].item += num; } while (0)
#else
    #define UPDATE_VCC_STAT(conn, item, num)
#endif  //  defined(STATS_ON_VCC_BASIS) && STATS_ON_VCC_BASIS


/*
 * ####################################
 *              Data Type
 * ####################################
 */

/*
 *  64-bit Data Type
 */
typedef struct {
    unsigned int    h: 32;
    unsigned int    l: 32;
} ppe_u64_t;

/*
 *  PPE ATM Cell Header
 */
#if defined(__BIG_ENDIAN)
    struct uni_cell_header {
        unsigned int        gfc     :4;
        unsigned int        vpi     :8;
        unsigned int        vci     :16;
        unsigned int        pti     :3;
        unsigned int        clp     :1;
    };
#else
    struct uni_cell_header {
        unsigned int        clp     :1;
        unsigned int        pti     :3;
        unsigned int        vci     :16;
        unsigned int        vpi     :8;
        unsigned int        gfc     :4;
    };
#endif  //  defined(__BIG_ENDIAN)

/*
 *  Inband Header and Trailer
 */
#if defined(__BIG_ENDIAN)
    struct rx_inband_trailer {
        /*  0 - 3h  */
        unsigned int        uu      :8;
        unsigned int        cpi     :8;
        unsigned int        stw_res1:4;
        unsigned int        stw_clp :1;
        unsigned int        stw_ec  :1;
        unsigned int        stw_uu  :1;
        unsigned int        stw_cpi :1;
        unsigned int        stw_ovz :1;
        unsigned int        stw_mfl :1;
        unsigned int        stw_usz :1;
        unsigned int        stw_crc :1;
        unsigned int        stw_il  :1;
        unsigned int        stw_ra  :1;
        unsigned int        stw_res2:2;
        /*  4 - 7h  */
        unsigned int        gfc     :4;
        unsigned int        vpi     :8;
        unsigned int        vci     :16;
        unsigned int        pti     :3;
        unsigned int        clp     :1;
    };

    struct tx_inband_header {
        /*  0 - 3h  */
        unsigned int        gfc     :4;
        unsigned int        vpi     :8;
        unsigned int        vci     :16;
        unsigned int        pti     :3;
        unsigned int        clp     :1;
        /*  4 - 7h  */
        unsigned int        uu      :8;
        unsigned int        cpi     :8;
        unsigned int        pad     :8;
        unsigned int        res1    :8;
    };
#else
    struct rx_inband_trailer {
        /*  0 - 3h  */
        unsigned int        stw_res2:2;
        unsigned int        stw_ra  :1;
        unsigned int        stw_il  :1;
        unsigned int        stw_crc :1;
        unsigned int        stw_usz :1;
        unsigned int        stw_mfl :1;
        unsigned int        stw_ovz :1;
        unsigned int        stw_cpi :1;
        unsigned int        stw_uu  :1;
        unsigned int        stw_ec  :1;
        unsigned int        stw_clp :1;
        unsigned int        stw_res1:4;
        unsigned int        cpi     :8;
        unsigned int        uu      :8;
        /*  4 - 7h  */
        unsigned int        clp     :1;
        unsigned int        pti     :3;
        unsigned int        vci     :16;
        unsigned int        vpi     :8;
        unsigned int        gfc     :4;
    };

    struct tx_inband_header {
        /*  0 - 3h  */
        unsigned int        clp     :1;
        unsigned int        pti     :3;
        unsigned int        vci     :16;
        unsigned int        vpi     :8;
        unsigned int        gfc     :4;
        /*  4 - 7h  */
        unsigned int        res1    :8;
        unsigned int        pad     :8;
        unsigned int        cpi     :8;
        unsigned int        uu      :8;
    };
#endif  //  defined(__BIG_ENDIAN)

/*
 *  MIB Table Maintained by Firmware
 */
struct wan_mib_table {
    u32                     res1;
    u32                     wrx_drophtu_cell;
    u32                     wrx_dropdes_pdu;
    u32                     wrx_correct_pdu;
    u32                     wrx_err_pdu;
    u32                     wrx_dropdes_cell;
    u32                     wrx_correct_cell;
    u32                     wrx_err_cell;
    u32                     wrx_total_byte;
    u32                     wtx_total_pdu;
    u32                     wtx_total_cell;
    u32                     wtx_total_byte;
};

/*
 *  Internal Structure of Device
 */
struct port {
    int                     connection_base;        /*  first connection ID (RX/TX queue ID)    */
    u32                     max_connections;        /*  maximum connection number               */
    u32                     connection_table;       /*  connection opened status, every bit     */
                                                    /*  stands for one connection   */

    u32                     tx_max_cell_rate;       /*  maximum cell rate                       */
    u32                     tx_current_cell_rate;   /*  currently used cell rate                */

#if !defined(ENABLE_RX_QOS) || !ENABLE_RX_QOS
    int                     rx_dma_channel_base;    /*  first RX DMA channel ID                 */
    u32                     rx_dma_channel_assigned;/*  totally RX DMA channels used            */
#endif  //  !defined(ENABLE_RX_QOS) || !ENABLE_RX_QOS

    int                     oam_tx_queue;           /*  first TX queue ID of OAM cell           */

    struct atm_dev          *dev;
};

struct connection {
    struct atm_vcc          *vcc;                   /*  opened VCC                              */
    struct timeval          access_time;            /*  time when last F4/F5 user cell arrives  */

    u32                     aal5_vcc_crc_err;       /*  number of packets with CRC error        */
    u32                     aal5_vcc_oversize_sdu;  /*  number of packets with oversize error   */

    int                     rx_dma_channel;         /*  RX DMA channel ID assigned              */

    int                     port;                   /*  to which port the connection belongs    */

#if defined(STATS_ON_VCC_BASIS) && STATS_ON_VCC_BASIS
    u32                     rx_pdu;
    u32                     rx_err_pdu;
    u32                     rx_sw_drop_pdu;

    u32                     tx_pdu;
    u32                     tx_err_pdu;
    u32                     tx_hw_drop_pdu;
    u32                     tx_sw_drop_pdu;
#endif  //  defined(STATS_ON_VCC_BASIS) && STATS_ON_VCC_BASIS
};

struct ppe_dev {
    struct connection       connection[MAX_CONNECTION_NUMBER];
    struct port             port[ATM_PORT_NUMBER];

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
    u32                     rx_bytes_cur[MAX_CONNECTION_NUMBER];
    u32                     rx_bytes_rate[MAX_CONNECTION_NUMBER];

    u32                     tx_bytes_cur[MAX_CONNECTION_NUMBER];
    u32                     tx_bytes_rate[MAX_CONNECTION_NUMBER];

    u32                     dreg_ar_cell_bak[2];
    u32                     dreg_ar_cell_cur[2];
    u32                     dreg_ar_cell_rate[2];

    u32                     dreg_ar_idle_bak[2];
    u32                     dreg_ar_idle_cur[2];
    u32                     dreg_ar_idle_rate[2];

    u32                     dreg_ar_be_bak[2];
    u32                     dreg_ar_be_cur[2];
    u32                     dreg_ar_be_rate[2];

    u32                     dreg_at_cell_bak[2];
    u32                     dreg_at_cell_cur[2];
    u32                     dreg_at_cell_rate[2];

    u32                     dreg_at_idle_bak[2];
    u32                     dreg_at_idle_cur[2];
    u32                     dreg_at_idle_rate[2];

    int                     timer;

    int                     heart_beat;
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

    struct aal5 {
        u8              padding_byte;               /*  padding byte pattern of AAL5 packet     */

        u32             rx_max_packet_size;         /*  max AAL5 packet length                  */
        u32             rx_min_packet_size;         /*  min AAL5 packet length                  */
        u32             rx_buffer_size;             /*  max memory allocated for a AAL5 packet  */
        u32             tx_max_packet_size;         /*  max AAL5 packet length                  */
        u32             tx_min_packet_size;         /*  min AAL5 packet length                  */
        u32             tx_buffer_size;             /*  max memory allocated for a AAL5 packet  */

        unsigned int    rx_drop_error_packet;       /*  1: drop error packet, 0: ignore errors  */
    }                       aal5;

    struct qsb {
        u32             tau;                        /*  cell delay variation due to concurrency */
        u32             tstepc;                     /*  shceduler burst length                  */
        u32             sbl;                        /*  time step                               */
    }                       qsb;

    struct dma {
        u32             rx_descriptor_number;       /*  number of RX descriptors                */
        u32             tx_descriptor_number;       /*  number of TX descriptors                */
        u32             rx_clp1_desc_threshold;     /*  threshold to drop cells with CLP1       */

        u32             write_descriptor_delay;     /*  delay on descriptor write path          */

        u32             rx_total_channel_used;      /*  total RX channel used                   */
        void            *rx_descriptor_addr;        /*  base address of memory allocated for    */
                                                    /*  RX descriptors                          */
        struct rx_descriptor
                        *rx_descriptor_base;        /*  base address of RX descriptors          */
        int             rx_desc_read_pos[MAX_RX_DMA_CHANNEL_NUMBER];    /*  first RX descriptor */
                                                                        /*  to be read          */
//        struct sk_buff  **rx_skb_pointers;          /*  base address of RX sk_buff pointers     */

#if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
        long            rx_weight[MAX_RX_DMA_CHANNEL_NUMBER];           /*  RX schedule weight  */
        long            rx_default_weight[MAX_RX_DMA_CHANNEL_NUMBER];   /*  default weight      */
#endif

        u32             tx_total_channel_used;      /*  total TX channel used                   */
        void            *tx_descriptor_addr;        /*  base address of memory allocated for    */
                                                    /*  TX descriptors                          */
        struct tx_descriptor
                        *tx_descriptor_base;        /*  base address of TX descriptors          */
        int             tx_desc_alloc_pos[MAX_TX_DMA_CHANNEL_NUMBER];   /*  first TX descriptor */
                                                                        /*  could be allocated  */
//        int             tx_desc_alloc_num[MAX_TX_DMA_CHANNEL_NUMBER];   /*  number of allocated */
//                                                                        /*  TX descriptors      */
        int             tx_desc_alloc_flag[MAX_TX_DMA_CHANNEL_NUMBER];  /*  at least one TX     */
                                                                        /*  descriptor is alloc */
//        int             tx_desc_send_pos[MAX_TX_DMA_CHANNEL_NUMBER];    /*  first TX descriptor */
//                                                                        /*  to be send          */
        int             tx_desc_release_pos[MAX_TX_DMA_CHANNEL_NUMBER]; /*  first TX descriptor */
                                                                        /*  to be released      */
        struct sk_buff  **tx_skb_pointers;          /*  base address of TX sk_buff pointers     */
    }                       dma;

    struct mib {
        ppe_u64_t       wrx_total_byte;             /*  bit-64 extention of MIB table member    */
        ppe_u64_t       wtx_total_byte;             /*  bit-64 extention of MIB talbe member    */

        u32             wrx_pdu;                    /*  successfully received AAL5 packet       */
        u32             wrx_drop_pdu;               /*  AAL5 packet dropped by driver on RX     */
        u32             wtx_err_pdu;                /*  error AAL5 packet                       */
        u32             wtx_drop_pdu;               /*  AAL5 packet dropped by driver on TX     */
    }                       mib;
    struct wan_mib_table    prev_mib;

    int                     oam_rx_queue;           /*  RX queue ID of OAM cell                 */
    int                     oam_rx_dma_channel;     /*  RX DMA channel ID of OAM cell           */
    int                     max_connections;        /*  total connections available             */

    struct semaphore        sem;                    /*  lock used by open/close function        */
};

/*
 *  Host-PPE Communication Data Structure
 */
#if defined(__BIG_ENDIAN)
    struct wrx_queue_config {
        /*  0h  */
        unsigned int    res2        :27;
        unsigned int    dmach       :4;
        unsigned int    errdp       :1;
        /*  1h  */
        unsigned int    oversize    :16;
        unsigned int    undersize   :16;
        /*  2h  */
        unsigned int    res1        :16;
        unsigned int    mfs         :16;
        /*  3h  */
        unsigned int    uumask      :8;
        unsigned int    cpimask     :8;
        unsigned int    uuexp       :8;
        unsigned int    cpiexp      :8;
    };

    struct wtx_port_config {
        unsigned int    res1        :27;
        unsigned int    qid         :4;
        unsigned int    qsben       :1;
    };

    struct wtx_queue_config {
        unsigned int    res1        :25;
        unsigned int    sbid        :1;
        unsigned int    res2        :3;
        unsigned int    type        :2;
        unsigned int    qsben       :1;
    };

    struct wrx_dma_channel_config {
        /*  0h  */
        unsigned int    res1        :1;
        unsigned int    mode        :2;
        unsigned int    rlcfg       :1;
        unsigned int    desba       :28;
        /*  1h  */
        unsigned int    chrl        :16;
        unsigned int    clp1th      :16;
        /*  2h  */
        unsigned int    deslen      :16;
        unsigned int    vlddes      :16;
    };

    struct wtx_dma_channel_config {
        /*  0h  */
        unsigned int    res2        :1;
        unsigned int    mode        :2;
        unsigned int    res3        :1;
        unsigned int    desba       :28;
        /*  1h  */
        unsigned int    res1        :32;
        /*  2h  */
        unsigned int    deslen      :16;
        unsigned int    vlddes      :16;
    };

    struct htu_entry {
        unsigned int    res1        :2;
        unsigned int    pid         :2;
        unsigned int    vpi         :8;
        unsigned int    vci         :16;
        unsigned int    pti         :3;
        unsigned int    vld         :1;
    };

    struct htu_mask {
        unsigned int    set         :2;
        unsigned int    pid_mask    :2;
        unsigned int    vpi_mask    :8;
        unsigned int    vci_mask    :16;
        unsigned int    pti_mask    :3;
        unsigned int    clear       :1;
    };

   struct htu_result {
        unsigned int    res1        :12;
        unsigned int    cellid      :4;
        unsigned int    res2        :5;
        unsigned int    type        :1;
        unsigned int    ven         :1;
        unsigned int    res3        :5;
        unsigned int    qid         :4;
    };

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
    struct wrx_queue_config {
        /*  0h  */
        unsigned int    errdp       :1;
        unsigned int    dmach       :4;
        unsigned int    res2        :27;
        /*  1h  */
        unsigned int    undersize   :16;
        unsigned int    oversize    :16;
        /*  2h  */
        unsigned int    mfs         :16;
        unsigned int    res1        :16;
        /*  3h  */
        unsigned int    cpiexp      :8;
        unsigned int    uuexp       :8;
        unsigned int    cpimask     :8;
        unsigned int    uumask      :8;
    };

    struct wtx_port_config {
        unsigned int    qsben       :1;
        unsigned int    qid         :4;
        unsigned int    res1        :27;
    };

    struct wtx_queue_config {
        unsigned int    qsben       :1;
        unsigned int    type        :2;
        unsigned int    res2        :3;
        unsigned int    sbid        :1;
        unsigned int    res1        :25;
    };

    struct wrx_dma_channel_config
    {
        /*  0h  */
        unsigned int    desba       :28;
        unsigned int    rlcfg       :1;
        unsigned int    mode        :2;
        unsigned int    res1        :1;
        /*  1h  */
        unsigned int    clp1th      :16;
        unsigned int    chrl        :16;
        /*  2h  */
        unsigned int    vlddes      :16;
        unsigned int    deslen      :16;
    };

    struct wtx_dma_channel_config {
        /*  0h  */
        unsigned int    desba       :28;
        unsigned int    res3        :1;
        unsigned int    mode        :2;
        unsigned int    res2        :1;
        /*  1h  */
        unsigned int    res1        :32;
        /*  2h  */
        unsigned int    vlddes      :16;
        unsigned int    deslen      :16;
    };

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
 *  QSB Queue Parameter Table Entry and Queue VBR Parameter Table Entry
 */
#if defined(__BIG_ENDIAN)
    union qsb_queue_parameter_table {
        struct {
            unsigned int    res1    :1;
            unsigned int    vbr     :1;
            unsigned int    wfqf    :14;
            unsigned int    tp      :16;
        }               bit;
        u32             dword;
    };

    union qsb_queue_vbr_parameter_table {
        struct {
            unsigned int    taus    :16;
            unsigned int    ts      :16;
        }               bit;
        u32             dword;
    };
#else
    union qsb_queue_parameter_table {
        struct {
            unsigned int    tp      :16;
            unsigned int    wfqf    :14;
            unsigned int    vbr     :1;
            unsigned int    res1    :1;
        }               bit;
        u32             dword;
    };

    union qsb_queue_vbr_parameter_table {
        struct {
            unsigned int    ts      :16;
            unsigned int    taus    :16;
        }               bit;
        u32             dword;
    };
#endif  //  defined(__BIG_ENDIAN)


/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  Network Operations
 */
static int ppe_ioctl(struct atm_dev *, unsigned int, void *);
static int ppe_open(struct atm_vcc *, short, int);
static void ppe_close(struct atm_vcc *);
static int ppe_send(struct atm_vcc *, struct sk_buff *);
static int ppe_send_oam(struct atm_vcc *, void *, int);
static int ppe_change_qos(struct atm_vcc *, struct atm_qos *, int);

/*
 *  ADSL LED
 */
static void adsl_led_flash(void);

/*
 *  64-bit operation used by MIB calculation
 */
static INLINE void u64_add_u32(ppe_u64_t opt1, u32 opt2,ppe_u64_t *ret);

/*
 *  buffer manage functions
 */
static INLINE struct sk_buff* alloc_skb_rx(void);
static INLINE struct sk_buff* alloc_skb_tx(unsigned int);
static INLINE void resize_skb_rx(struct sk_buff *, unsigned int, int);
#if defined(LINUX_2_4_31) && LINUX_2_4_31
  struct sk_buff* atm_alloc_tx(struct atm_vcc *, unsigned int);
#else
  static struct sk_buff* atm_alloc_tx(struct atm_vcc *, unsigned int);
#endif
static INLINE void atm_free_tx_skb_vcc(struct sk_buff *);
static INLINE int alloc_tx_connection(int);

/*
 *  mailbox handler and signal function
 */
static INLINE int mailbox_rx_irq_handler(unsigned int, unsigned int *);
static INLINE void mailbox_tx_irq_handler(unsigned int);
#if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
  static INLINE int check_desc_valid(unsigned int);
#endif
static void mailbox_irq_handler(int, void *, struct pt_regs *);
static INLINE void mailbox_signal(unsigned int, int);
#if 0
static INLINE void mailbox_signal_with_mask(u32);
#endif

/*
 *  QSB & HTU setting functions
 */
static void set_qsb(struct atm_vcc *, struct atm_qos *, unsigned int);
static INLINE void set_htu_entry(unsigned int, unsigned int, unsigned int, int);
static INLINE void clear_htu_entry(unsigned int);

/*
 *  look up for connection ID
 */
static INLINE int find_vpi(unsigned int);
static INLINE int find_vpivci(unsigned int, unsigned int);
#if 0
static INLINE int find_vpivciport(unsigned int, unsigned int, unsigned int);
#endif
static INLINE int find_vcc(struct atm_vcc *);

/*
 *  Init & clean-up functions
 */
static INLINE void check_parameters(void);
static INLINE int init_ppe_dev(void);
static INLINE void clear_share_buffer(void);
static INLINE void init_rx_tables(void);
static INLINE void init_tx_tables(void);
static INLINE void clear_ppe_dev(void);

/*
 *  PP32 specific init functions
 */
static INLINE void init_ema(void);
static INLINE void init_chip(void);
static INLINE int pp32_download_code(u32 *, unsigned int, u32 *, unsigned int);
static INLINE int pp32_specific_init(void *);
static INLINE int pp32_start(void);
static INLINE void pp32_stop(void);

/*
 *  QSB & HTU init functions
 */
static INLINE void qsb_global_set(void);
static INLINE void validate_oam_htu_entry(void);
static INLINE void invalidate_oam_htu_entry(void);

/*
 *  Proc File
 */
static INLINE void proc_file_create(void);
static INLINE void proc_file_delete(void);
static int proc_read_idle_counter(char *, char **, off_t, int, int *, void *);
static int proc_read_stats(char *, char **, off_t, int, int *, void *);
#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
  static int proc_read_rate(char *, char **, off_t, int, int *, void *);
#endif
#if defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
  static int proc_read_mbox0(char *, char **, off_t, int, int *, void *);
#endif
#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
  #if defined(ENABLE_TR067_LOOPBACK_PROC) && ENABLE_TR067_LOOPBACK_PROC
    static int proc_read_test(char *, char **, off_t, int, int *, void *);
    static int proc_write_test(struct file *, const char *, unsigned long, void *);
  #endif
#endif

/*
 *  profiling functions
 */
#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
static inline unsigned long get_count(void)
{
    return read_c0_count();
}
#endif

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
static void ppe_timer_callback(unsigned long);
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

#if defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
static void pci_fix_irq_handler(int, void *, struct pt_regs *);
#endif  //  defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE

/*
 *  Export Functions
 */
void ifx_atm_set_cell_rate(int, u32);



/*
 * ####################################
 *            Local Variable
 * ####################################
 */

/*  global variable to hold all driver datas    */
static struct ppe_dev ppe_dev;

static struct atmdev_ops ppe_atm_ops = {
    owner:      THIS_MODULE,
    open:       ppe_open,
    close:      ppe_close,
    ioctl:      ppe_ioctl,
    send:       ppe_send,
    send_oam:   ppe_send_oam,
    change_qos: ppe_change_qos,
};

#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
static u32 count_tx;
static u32 count_rx;
static u32 count_send;
static u32 count_receive;
static u32 interval_txrx = 0;
static u32 interval_sendreceive = 0;
static u32 interval_sendtx = 0;
static u32 interval_rxreceive = 0;

static u32 count_open;
static u32 count_close;
static u32 interval_total = 0;
#endif

static void (*adsl_led_flash_cb)(void);



/*
 * ####################################
 *           Global Variable
 * ####################################
 */

struct proc_dir_entry *g_ppe_proc_dir;


/*
 * ####################################
 *            Local Function
 * ####################################
 */

/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access PPE driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands.
 *    arg   --- unsigned long, pointer to a structure to pass in arguments or
 *              pass out return value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */
static int ppe_ioctl(struct atm_dev *dev, unsigned int cmd, void *arg)
{
    int ret = 0;
    atm_cell_ifEntry_t mib_cell;
    atm_aal5_ifEntry_t mib_aal5;
    atm_aal5_vcc_x_t mib_vcc;
    u32 value;
    int conn;

//    dbg("ppe_ioctl");

    if ( _IOC_TYPE(cmd) != PPE_ATM_IOC_MAGIC
        || _IOC_NR(cmd) >= PPE_ATM_IOC_MAXNR )
        return -ENOTTY;

    if ( _IOC_DIR(cmd) & _IOC_READ )
        ret = !access_ok(VERIFY_WRITE, arg, _IOC_SIZE(cmd));
    else if ( _IOC_DIR(cmd) & _IOC_WRITE )
        ret = !access_ok(VERIFY_READ, arg, _IOC_SIZE(cmd));
    if ( ret )
        return -EFAULT;

    switch ( cmd )
    {
    case PPE_ATM_MIB_CELL:  /*  cell level  MIB */
        /*  These MIB should be read at ARC side, now put zero only.    */
        mib_cell.ifHCInOctets_h = 0;
        mib_cell.ifHCInOctets_l = 0;
        mib_cell.ifHCOutOctets_h = 0;
        mib_cell.ifHCOutOctets_l = 0;
        mib_cell.ifInErrors = 0;
        mib_cell.ifInUnknownProtos = WAN_MIB_TABLE->wrx_drophtu_cell;
        mib_cell.ifOutErrors = 0;

        copy_to_user(arg, &mib_cell, sizeof(mib_cell));
        break;
    case PPE_ATM_MIB_AAL5:  /*  AAL5 MIB    */
        value = WAN_MIB_TABLE->wrx_total_byte;
        u64_add_u32(ppe_dev.mib.wrx_total_byte, value - ppe_dev.prev_mib.wrx_total_byte, &ppe_dev.mib.wrx_total_byte);
        ppe_dev.prev_mib.wrx_total_byte = value;
        mib_aal5.ifHCInOctets_h = ppe_dev.mib.wrx_total_byte.h;
        mib_aal5.ifHCInOctets_l = ppe_dev.mib.wrx_total_byte.l;

        value = WAN_MIB_TABLE->wtx_total_byte;
        u64_add_u32(ppe_dev.mib.wtx_total_byte, value - ppe_dev.prev_mib.wtx_total_byte, &ppe_dev.mib.wtx_total_byte);
        ppe_dev.prev_mib.wtx_total_byte = value;
        mib_aal5.ifHCOutOctets_h = ppe_dev.mib.wtx_total_byte.h;
        mib_aal5.ifHCOutOctets_l = ppe_dev.mib.wtx_total_byte.l;

        mib_aal5.ifInUcastPkts  = ppe_dev.mib.wrx_pdu;
        mib_aal5.ifOutUcastPkts = WAN_MIB_TABLE->wtx_total_pdu;
        mib_aal5.ifInErrors     = WAN_MIB_TABLE->wrx_err_pdu;
        mib_aal5.ifInDiscards   = WAN_MIB_TABLE->wrx_dropdes_pdu + ppe_dev.mib.wrx_drop_pdu;
        mib_aal5.ifOutErros     = ppe_dev.mib.wtx_err_pdu;
        mib_aal5.ifOutDiscards  = ppe_dev.mib.wtx_drop_pdu;

        copy_to_user(arg, &mib_aal5, sizeof(mib_aal5));
        break;
    case PPE_ATM_MIB_VCC:   /*  VCC related MIB */
        copy_from_user(&mib_vcc, arg, sizeof(mib_vcc));
        conn = find_vpivci(mib_vcc.vpi, mib_vcc.vci);
        if ( conn >= 0 )
        {
            mib_vcc.mib_vcc.aal5VccCrcErrors     = ppe_dev.connection[conn].aal5_vcc_crc_err;
            mib_vcc.mib_vcc.aal5VccOverSizedSDUs = ppe_dev.connection[conn].aal5_vcc_oversize_sdu;
            mib_vcc.mib_vcc.aal5VccSarTimeOuts   = 0;   /*  no timer support    */
            copy_to_user(arg, &mib_vcc, sizeof(mib_vcc));
        }
        else
            ret = -EINVAL;
    default:
        ret = -ENOTTY;
    }

    return ret;
}

//extern void llc_mux_atm_push(struct atm_vcc * p_atmvcc,struct sk_buff *p_skb);

static int ppe_open(struct atm_vcc *vcc, short vpi, int vci)
{
    int ret;
    struct port *port = &ppe_dev.port[(int)vcc->dev->dev_data];
    int conn;
    int f_enable_irq = 0;
    int i;

//    dbg("ppe_open");
//    dbg("vcc->push = 0x%08X, llc_mux_atm_push = 0x%08X", (u32)vcc->push, (u32)llc_mux_atm_push);

    if ( vcc->qos.aal != ATM_AAL5 && vcc->qos.aal != ATM_AAL0 )
        return -EPROTONOSUPPORT;

#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
    vcc->qos.aal = ATM_AAL0;
#endif

    down(&ppe_dev.sem);

    /*  check bandwidth */
    if ( (vcc->qos.txtp.traffic_class == ATM_CBR && vcc->qos.txtp.max_pcr > (port->tx_max_cell_rate - port->tx_current_cell_rate))
      || (vcc->qos.txtp.traffic_class == ATM_VBR_RT && vcc->qos.txtp.max_pcr > (port->tx_max_cell_rate - port->tx_current_cell_rate))
      || (vcc->qos.txtp.traffic_class == ATM_VBR_NRT && vcc->qos.txtp.pcr > (port->tx_max_cell_rate - port->tx_current_cell_rate))
      || (vcc->qos.txtp.traffic_class == ATM_UBR_PLUS && vcc->qos.txtp.min_pcr > (port->tx_max_cell_rate - port->tx_current_cell_rate)) )
    {
        ret = -EINVAL;
        goto PPE_OPEN_EXIT;
    }

//    printk("alloc vpi = %d, vci = %d\n", vpi, vci);

    /*  check existing vpi,vci  */
    conn = find_vpivci(vpi, vci);
    if ( conn >= 0 )
    {
        ret = -EADDRINUSE;
        goto PPE_OPEN_EXIT;
    }

    /*  check whether it need to enable irq */
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        if ( ppe_dev.port[i].max_connections != 0 && ppe_dev.port[i].connection_table != 0 )
            break;
    if ( i == ATM_PORT_NUMBER )
        f_enable_irq = 1;

    /*  allocate connection */
    for ( i = 0, conn = port->connection_base; i < port->max_connections; i++, conn++ )
        if ( !(port->connection_table & (1 << i)) )
        {
            port->connection_table |= 1 << i;
            ppe_dev.connection[conn].vcc = vcc;
//            printk("ppe_dev.connection[%d].vcc = 0x%08X\n", conn, (u32)vcc);
            break;
        }
    if ( i == port->max_connections )
    {
        ret = -EINVAL;
        goto PPE_OPEN_EXIT;
    }

#if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
    /*  assign DMA channel and setup weight value for RX QoS    */
    switch ( vcc->qos.rxtp.traffic_class )
    {
    case ATM_CBR:
        ppe_dev.connection[conn].rx_dma_channel = RX_DMA_CH_CBR;
        break;
    case ATM_VBR_RT:
        ppe_dev.connection[conn].rx_dma_channel = RX_DMA_CH_VBR_RT;
        ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_RT] += vcc->qos.rxtp.max_pcr;
        ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] += vcc->qos.rxtp.max_pcr;
        break;
    case ATM_VBR_NRT:
        ppe_dev.connection[conn].rx_dma_channel = RX_DMA_CH_VBR_NRT;
        ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_NRT] += vcc->qos.rxtp.pcr;
        ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] += vcc->qos.rxtp.pcr;
        break;
    case ATM_ABR:
        ppe_dev.connection[conn].rx_dma_channel = RX_DMA_CH_AVR;
        ppe_dev.dma.rx_default_weight[RX_DMA_CH_AVR] += vcc->qos.rxtp.min_pcr;
        ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] += vcc->qos.rxtp.min_pcr;
        break;
    case ATM_UBR_PLUS:
    default:
        ppe_dev.connection[conn].rx_dma_channel = RX_DMA_CH_UBR;
        break;
    }

    /*  update RX queue configuration table */
    WRX_QUEUE_CONFIG(conn)->dmach = ppe_dev.connection[conn].rx_dma_channel;

//    printk("ppe_open: QID %d, DMA %d\n", conn, WRX_QUEUE_CONFIG(conn)->dmach);

//    dbg("conn = %d, dmach = %d", conn, WRX_QUEUE_CONFIG(conn)->dmach);
#endif  //  defined(ENABLE_RX_QOS) && ENABLE_RX_QOS

    /*  reserve bandwidth   */
    switch ( vcc->qos.txtp.traffic_class )
    {
    case ATM_CBR:
    case ATM_VBR_RT:
        port->tx_current_cell_rate += vcc->qos.txtp.max_pcr;
        break;
    case ATM_VBR_NRT:
        port->tx_current_cell_rate += vcc->qos.txtp.pcr;
        break;
    case ATM_UBR_PLUS:
        port->tx_current_cell_rate += vcc->qos.txtp.min_pcr;
        break;
    }

    /*  set qsb */
    set_qsb(vcc, &vcc->qos, conn);

    /*  update atm_vcc structure    */
    vcc->itf = (int)vcc->dev->dev_data;
    vcc->vpi = vpi;
    vcc->vci = vci;
#if !LINUX_2_4_31
    vcc->alloc_tx = atm_alloc_tx;
#endif
    set_bit(ATM_VF_READY, &vcc->flags);

#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    /*  enable irq  */
//    printk("ppe_open: enable_irq\n");
    if ( f_enable_irq )
        enable_irq(PPE_MAILBOX_IGU1_INT);
#endif

    /*  enable mailbox  */
    *MBOX_IGU1_ISRC =  (1 << conn) | (1 << (conn + 16));
    *MBOX_IGU1_IER  |= (1 << conn) | (1 << (conn + 16));
    *MBOX_IGU3_ISRC =  (1 << conn) | (1 << (conn + 16));
    *MBOX_IGU3_IER  |= (1 << conn) | (1 << (conn + 16));

    /*  set htu entry   */
    set_htu_entry(vpi, vci, conn, vcc->qos.aal == ATM_AAL5 ? 1 : 0);

    ret = 0;

//    MOD_INC_USE_COUNT;

//    printk("ppe_open(%d.%d): conn = %d, ppe_dev.dma = %08X\n", vcc->vpi, vcc->vci, conn, (u32)&ppe_dev.dma.rx_descriptor_number);

#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
    interval_txrx = 0;
    interval_sendreceive = 0;
    interval_sendtx = 0;
    interval_rxreceive = 0;

    interval_total = 0;
#endif

PPE_OPEN_EXIT:
    up(&ppe_dev.sem);

//    dbg("open ATM itf = %d, vpi = %d, vci = %d, ret = %d", (int)vcc->dev->dev_data, (int)vpi, vci, ret);
#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
    count_open = get_count();
#endif
    return ret;
}

static void ppe_close(struct atm_vcc *vcc)
{
    int conn;
    struct port *port;
    struct connection *connection;
    int i;

//    dbg("ppe_close");

#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
    count_close = get_count();
    interval_total = count_close - count_open;
#endif

    if ( vcc == NULL )
        return;

    down(&ppe_dev.sem);

//    MOD_DEC_USE_COUNT;

    /*  get connection id   */
    conn = find_vcc(vcc);
    if ( conn < 0 )
    {
        printk("can't find vcc\n");
        goto PPE_CLOSE_EXIT;
    }
    connection = &ppe_dev.connection[conn];
    port = &ppe_dev.port[connection->port];

    /*  clear htu   */
    clear_htu_entry(conn);

    /*  release connection  */
    port->connection_table &= ~(1 << (conn - port->connection_base));
    connection->vcc = NULL;
    connection->access_time.tv_sec = 0;
    connection->access_time.tv_usec = 0;
    connection->aal5_vcc_crc_err = 0;
    connection->aal5_vcc_oversize_sdu = 0;

    /*  disable mailbox */
//    *MBOX_IGU1_IER  &= ~((1 << conn) | (1 << (conn + 16)));
//    printk("ppe_close(%d.%d): conn = %d\n", vcc->vpi, vcc->vci, conn);
//    *MBOX_IGU3_IER  &= ~((1 << conn) | (1 << (conn + 16)));

#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    /*  disable irq */
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        if ( ppe_dev.port[i].max_connections != 0 && ppe_dev.port[i].connection_table != 0 )
            break;
    if ( i == ATM_PORT_NUMBER )
    {
//        printk("ppe_close: disable_irq\n");
        disable_irq(PPE_MAILBOX_IGU1_INT);
    }
#endif

    /*  clear mailbox   */
    *MBOX_IGU1_ISRC =  (1 << conn) | (1 << (conn + 16));
//    *MBOX_IGU3_ISRC =  (1 << conn) | (1 << (conn + 16));

#if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
    /*  remove weight value from RX DMA channel */
    switch ( vcc->qos.rxtp.traffic_class )
    {
    case ATM_VBR_RT:
        ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_RT] -= vcc->qos.rxtp.max_pcr;
        if ( ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] > ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_RT] )
            ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] = ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_RT];
        break;
    case ATM_VBR_NRT:
        ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_NRT] -= vcc->qos.rxtp.pcr;
        if ( ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] > ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_NRT] )
            ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] = ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_NRT];
        break;
    case ATM_ABR:
        ppe_dev.dma.rx_default_weight[RX_DMA_CH_AVR] -= vcc->qos.rxtp.min_pcr;
        if ( ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] > ppe_dev.dma.rx_default_weight[RX_DMA_CH_AVR] )
            ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] = ppe_dev.dma.rx_default_weight[RX_DMA_CH_AVR];
        break;
    case ATM_CBR:
    case ATM_UBR_PLUS:
    default:
        break;
    }
#endif  //  defined(ENABLE_RX_QOS) && ENABLE_RX_QOS

    /*  release bandwidth   */
    switch ( vcc->qos.txtp.traffic_class )
    {
    case ATM_CBR:
    case ATM_VBR_RT:
        port->tx_current_cell_rate -= vcc->qos.txtp.max_pcr;
        break;
    case ATM_VBR_NRT:
        port->tx_current_cell_rate -= vcc->qos.txtp.pcr;
        break;
    case ATM_UBR_PLUS:
        port->tx_current_cell_rate -= vcc->qos.txtp.min_pcr;
        break;
    }

    /*  idle for a while to let parallel operation finish   */
    for ( i = 0; i < IDLE_CYCLE_NUMBER; i++ );

PPE_CLOSE_EXIT:
    up(&ppe_dev.sem);

//    dbg("ppe_close finish");

#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
    printk("send_receive = %d, tx_rx = %d, send_tx = %d, rx_receive = %d\n", interval_sendreceive, interval_txrx, interval_sendtx, interval_rxreceive);
    printk("total = %d\n", interval_total);
#endif
//    printk("conn %d: rx_desc_read_pos = %d, tx_desc_alloc_pos = %d, tx_desc_alloc_num = %d\n", conn, ppe_dev.dma.rx_desc_read_pos[conn], ppe_dev.dma.tx_desc_alloc_pos[conn], ppe_dev.dma.tx_desc_alloc_num[conn]);
//    printk("conn %d: rx_desc_read_pos = %d, tx_desc_alloc_pos = %d, tx_desc_release_pos = %d\n", conn, ppe_dev.dma.rx_desc_read_pos[conn], ppe_dev.dma.tx_desc_alloc_pos[conn], ppe_dev.dma.tx_desc_release_pos[conn]);
}

static int ppe_send(struct atm_vcc *vcc, struct sk_buff *skb)
{
    int ret;
    int conn;
    int desc_base;
    register struct tx_descriptor reg_desc;
    struct tx_descriptor *desc;

#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
    count_send = get_count();
#endif

//    dbg("ppe_send");
//    printk("ppe_send\n");
//    printk("skb->users = %d\n", skb->users.counter);

    if ( vcc == NULL || skb == NULL )
        return -EINVAL;

//    down(&ppe_dev.sem);

    ATM_SKB(skb)->vcc = vcc;
    conn = find_vcc(vcc);
//    if ( conn != 1 )
//        printk("ppe_send: conn = %d\n", conn);
    if ( conn < 0 )
    {
        ret = -EINVAL;
        goto FIND_VCC_FAIL;
    }

//    dbg("find_vcc");

    /*  allocate descriptor */
#if !defined(WAIT_FOR_TX_DESC) || !WAIT_FOR_TX_DESC
    desc_base = alloc_tx_connection(conn);
    if ( desc_base < 0 )
    {
        ret = -EIO;
        goto ALLOC_TX_CONNECTION_FAIL;
    }
#else
    desc_base = alloc_tx_connection(conn);
    {
        int k = 0;

        while ( desc_base < 0 )
        {
            int i;

            if ( ++k == 1000 )
                printk("can't get TX desc\n");

            for ( i = 0; i < 2500; i++ );
            //udelay(1000);
            desc_base = alloc_tx_connection(conn);
        }
    }
#endif

//    dbg("alloc_tx_connection");

    desc = &ppe_dev.dma.tx_descriptor_base[desc_base];

    /*  load descriptor from memory */
    reg_desc = *desc;

//    dbg("desc = 0x%08X", (u32)desc);

    if ( vcc->qos.aal == ATM_AAL5 )
    {
        int byteoff;
        int datalen;
        struct tx_inband_header *header;

        datalen = skb->len;
        byteoff = (u32)skb->data & (DMA_ALIGNMENT - 1);
        if ( skb_headroom(skb) < byteoff + TX_INBAND_HEADER_LENGTH )
        {
            struct sk_buff *new_skb;

//            dbg("skb_headroom(skb) < byteoff + TX_INBAND_HEADER_LENGTH");
//            printk("skb_headroom(skb 0x%08X, skb->data 0x%08X) (%d) < byteoff (%d) + TX_INBAND_HEADER_LENGTH (%d)\n", (u32)skb, (u32)skb->data, skb_headroom(skb), byteoff, TX_INBAND_HEADER_LENGTH);

            new_skb = alloc_skb_tx(datalen);
            if ( new_skb == NULL )
            {
//                printk("alloc_skb_tx: fail\n");
                ret = -ENOMEM;
                goto ALLOC_SKB_TX_FAIL;
            }
            ATM_SKB(new_skb)->vcc = NULL;
            skb_put(new_skb, datalen);
            memcpy(new_skb->data, skb->data, datalen);
            atm_free_tx_skb_vcc(skb);
            skb = new_skb;
            byteoff = (u32)skb->data & (DMA_ALIGNMENT - 1);
        }
        else
        {
//            dbg("skb_headroom(skb) >= byteoff + TX_INBAND_HEADER_LENGTH");
        }
//        dbg("before skb_push, skb->data = 0x%08X", (u32)skb->data);
        skb_push(skb, byteoff + TX_INBAND_HEADER_LENGTH);
//        dbg("after skb_push, skb->data = 0x%08X", (u32)skb->data);

//        {
//            char *ptr = (char *)((u32)skb->data + byteoff + TX_INBAND_HEADER_LENGTH);
//            int ti;
//
//            printk(KERN_DEBUG "send: ");
//            for ( ti = 0; ti < datalen; ti++ )
//                if ( (unsigned char)ptr[ti] < 0x20 || (unsigned char)ptr[ti] > 0x7F )
//                    printk("%d", (u32)ptr[ti]);
//                else
//                    printk("%c", ptr[ti]);
//            printk("\n");
//        }

        header = (struct tx_inband_header *)(u32)skb->data;
//        dbg("header = 0x%08X", (u32)header);

        /*  setup inband trailer    */
        header->uu   = 0;
        header->cpi  = 0;
        header->pad  = ppe_dev.aal5.padding_byte;
        header->res1 = 0;

        /*  setup cell header   */
        header->clp  = (vcc->atm_options & ATM_ATMOPT_CLP) ? 1 : 0;
        header->pti  = ATM_PTI_US0;
        header->vci  = vcc->vci;
        header->vpi  = vcc->vpi;
        header->gfc  = 0;

        /*  setup descriptor    */
        reg_desc.dataptr = (u32)skb->data >> 2;
        reg_desc.datalen = datalen;
        reg_desc.byteoff = byteoff;
        reg_desc.iscell  = 0;

//        dbg("setup header, datalen = %d, byteoff = %d", reg_desc.datalen, reg_desc.byteoff);
//        printk("setup header, datalen = %d, byteoff = %d\n", reg_desc.datalen, reg_desc.byteoff);

        UPDATE_VCC_STAT(conn, tx_pdu, 1);

        if ( vcc->stats )
            atomic_inc(&vcc->stats->tx);
    }
    else
    {
        /*  if data pointer is not aligned, allocate new sk_buff    */
        if ( ((u32)skb->data & (DMA_ALIGNMENT - 1)) )
        {
            struct sk_buff *new_skb;

//            printk("skb->data not aligned\n");

            new_skb = alloc_skb_tx(skb->len);
            if ( new_skb == NULL )
            {
                ret = -ENOMEM;
                goto ALLOC_SKB_TX_FAIL;
            }
            ATM_SKB(new_skb)->vcc = NULL;
            skb_put(new_skb, skb->len);
            memcpy(new_skb->data, skb->data, skb->len);
            atm_free_tx_skb_vcc(skb);
            skb = new_skb;
        }

        reg_desc.dataptr = (u32)skb->data >> 2;
        reg_desc.datalen = skb->len;
        reg_desc.byteoff = 0;
        reg_desc.iscell  = 1;

        if ( vcc->stats )
            atomic_inc(&vcc->stats->tx);

#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
  #if defined(ENABLE_TR067_LOOPBACK_PROC) && ENABLE_TR067_LOOPBACK_PROC
    #if defined(ENABLE_TR067_LOOPBACK_DUMP) && ENABLE_TR067_LOOPBACK_DUMP
        {
            unsigned char *p = (unsigned char *)skb->data;
            int i, j;

            printk("ppe send:\n");
            for ( i = 0; i < 7; i++ )
            {
                printk("  %02X:", i * 8);
                for ( j = 0; j < 8; j++ )
                    printk(" %02X", *p++);
                printk("\n");
            }
        }
    #endif
  #endif
#endif
    }

    reg_desc.own = 1;
//    reg_desc.c = 0;
    reg_desc.c = 1;

//    dbg("update descriptor send pointer, desc = 0x%08X", (u32)desc);

    /*  update descriptor send pointer  */
    ppe_dev.dma.tx_skb_pointers[desc_base] = skb;
//    if ( ++ppe_dev.dma.tx_desc_send_pos[conn] == ppe_dev.dma.tx_descriptor_number )
//        ppe_dev.dma.tx_desc_send_pos[conn] = 0;
//    *my_sk_push_pointer++ = skb;

    /*  write discriptor to memory and write back cache */
    *desc = reg_desc;
    dma_cache_wback((unsigned long)skb->data, skb->len);

#if defined(DEBUG_DUMP_SKB) && DEBUG_DUMP_SKB
    {
        int i;

        printk("ppe_send\n");
        printk("  desc = 0x%08X,  0x%08X 0x%08X\n", (u32)desc, *(u32*)desc, *((u32*)desc + 1));
        printk("  desc->dataptr = 0x%08X, 0x%08X\n", (u32)desc->dataptr, (u32)skb->data);
        printk("  desc->datalen = %d, %d\n", (u32)desc->datalen, (u32)skb->len);
        printk("  desc->byteoff = %d\n", (u32)desc->byteoff);
        printk("  desc->iscell  = %d\n", (u32)desc->iscell);
//        printk("  mailbox_singal(%d, tx)\n", conn);

        printk("  skb->len = %d\n", skb->len);
        printk("  skb->data:\n");
        for ( i = 0; i < skb->len; i++ )
        {
            if ( (i & 0x0F) == 0 )
                printk("    %4d:", i);
            printk(" %02X", (u32)skb->data[i]);
            if ( (i & 0x0F) == 0x0F )
                printk("\n");
        }
        if ( (i & 0x0F) != 0 )
            printk("\n");
    }
#endif

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
    ppe_dev.tx_bytes_cur[conn] += reg_desc.datalen;
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

    mailbox_signal(conn, 1);

//    dbg("ppe_send: success");
//    up(&ppe_dev.sem);

#ifdef CONFIG_IFX_ADSL_ATM_LED
    adsl_led_flash(); //taicheng : for adsl data led
#endif

    return 0;

FIND_VCC_FAIL:
//    printk("FIND_VCC_FAIL\n");

//    up(&ppe_dev.sem);
    ppe_dev.mib.wtx_err_pdu++;
    atm_free_tx_skb_vcc(skb);

    return ret;

ALLOC_SKB_TX_FAIL:
//    printk("ALLOC_SKB_TX_FAIL\n");

//    up(&ppe_dev.sem);
    if ( vcc->qos.aal == ATM_AAL5 )
    {
        UPDATE_VCC_STAT(conn, tx_err_pdu, 1);
        ppe_dev.mib.wtx_err_pdu++;
    }
    if ( vcc->stats )
        atomic_inc(&vcc->stats->tx_err);
    atm_free_tx_skb_vcc(skb);

    return ret;

#if !defined(WAIT_FOR_TX_DESC) || !WAIT_FOR_TX_DESC
ALLOC_TX_CONNECTION_FAIL:
//    printk("ALLOC_TX_CONNECTION_FAIL\n");

//    up(&ppe_dev.sem);
    if ( vcc->qos.aal == ATM_AAL5 )
    {
        UPDATE_VCC_STAT(conn, tx_sw_drop_pdu, 1);
        ppe_dev.mib.wtx_drop_pdu++;
    }
    if ( vcc->stats )
        atomic_inc(&vcc->stats->tx_err);
    atm_free_tx_skb_vcc(skb);

    return ret;
#endif
}

static int ppe_send_oam(struct atm_vcc *vcc, void *cell, int flags)
{
    int conn;
    struct uni_cell_header *uni_cell_header = (struct uni_cell_header *)cell;
    int desc_base;
    struct sk_buff *skb;
    register struct tx_descriptor reg_desc;
    struct tx_descriptor *desc;

//    dbg("ppe_send_oam");

    if ( ((uni_cell_header->pti == ATM_PTI_SEGF5 || uni_cell_header->pti == ATM_PTI_E2EF5)
        && find_vpivci(uni_cell_header->vpi, uni_cell_header->vci) < 0)
        || ((uni_cell_header->vci == 0x03 || uni_cell_header->vci == 0x04)
        && find_vpi(uni_cell_header->vpi) < 0) )
        return -EINVAL;

//    down(&ppe_dev.sem);

#if OAM_TX_QUEUE_NUMBER_PER_PORT != 0
    /*  get queue ID of OAM TX queue, and the TX DMA channel ID is the same as queue ID */
    conn = ppe_dev.port[(int)vcc->dev->dev_data].oam_tx_queue;
#else
    /*  find queue ID   */
    conn = find_vcc(vcc);
    if ( conn < 0 )
    {
        printk("OAM not find queue\n");
//        up(&ppe_dev.sem);
        return -EINVAL;
    }
#endif  //  OAM_TX_QUEUE_NUMBER_PER_PORT != 0

    /*  allocate descriptor */
    desc_base = alloc_tx_connection(conn);
    if ( desc_base < 0 )
    {
        printk("OAM not alloc tx connection\n");
//        up(&ppe_dev.sem);
        return -EIO;
    }

    desc = &ppe_dev.dma.tx_descriptor_base[desc_base];

    /*  load descriptor from memory */
    reg_desc = *(struct tx_descriptor *)desc;

    /*  allocate sk_buff    */
    skb = alloc_skb_tx(CELL_SIZE);
    if ( skb == NULL )
    {
//        up(&ppe_dev.sem);
        return -ENOMEM;
    }
#if OAM_TX_QUEUE_NUMBER_PER_PORT != 0
    ATM_SKB(skb)->vcc = NULL;
#else
    ATM_SKB(skb)->vcc = vcc;
#endif  //  OAM_TX_QUEUE_NUMBER_PER_PORT != 0

    /*  copy data   */
    skb_put(skb, CELL_SIZE);
    memcpy(skb->data, cell, CELL_SIZE);

    /*  setup descriptor    */
    reg_desc.dataptr = (u32)skb->data >> 2;
    reg_desc.datalen = CELL_SIZE;
    reg_desc.byteoff = 0;
    reg_desc.iscell  = 1;
    reg_desc.own = 1;
//    reg_desc.c = 0;
    reg_desc.c = 1;

    /*  update descriptor send pointer  */
    ppe_dev.dma.tx_skb_pointers[desc_base] = skb;
//    if ( ++ppe_dev.dma.tx_desc_send_pos[conn] == ppe_dev.dma.tx_descriptor_number )
//        ppe_dev.dma.tx_desc_send_pos[conn] = 0;

    /*  write discriptor to memory and write back cache */
    *(struct tx_descriptor *)desc = reg_desc;
    dma_cache_wback((unsigned long)skb->data, skb->len);

#if defined(DEBUG_DUMP_SKB) && DEBUG_DUMP_SKB
    {
        int i;

        printk("ppe_send_oam\n");
        printk("  desc = 0x%08X,  0x%08X 0x%08X\n", (u32)desc, *(u32*)desc, *((u32*)desc + 1));
        printk("  desc->dataptr = 0x%08X, 0x%08X\n", (u32)desc->dataptr, (u32)skb->data);
        printk("  desc->datalen = %d, %d\n", (u32)desc->datalen, (u32)skb->len);
        printk("  desc->byteoff = %d\n", (u32)desc->byteoff);
        printk("  desc->iscell  = %d\n", (u32)desc->iscell);
//        printk("  mailbox_singal(%d, tx)\n", conn);

        printk("  skb->len = %d\n", skb->len);
        printk("  skb->data:\n");
        for ( i = 0; i < skb->len; i++ )
        {
            if ( (i & 0x0F) == 0 )
                printk("    %4d:", i);
            printk(" %02X", (u32)skb->data[i]);
            if ( (i & 0x0F) == 0x0F )
                printk("\n");
        }
        if ( (i & 0x0F) != 0 )
            printk("\n");
    }
#endif

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
    ppe_dev.tx_bytes_cur[conn] += reg_desc.datalen;
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

    /*  signal PPE  */
    mailbox_signal(conn, 1);

//    up(&ppe_dev.sem);

#ifdef CONFIG_IFX_ADSL_ATM_LED
    adsl_led_flash(); //taicheng : for adsl data led
#endif

    return 0;
}

static int ppe_change_qos(struct atm_vcc *vcc, struct atm_qos *qos, int flags)
{
    int conn;

//    dbg("ppe_change_qos");

    if ( vcc == NULL || qos == NULL )
        return -EINVAL;

    conn = find_vcc(vcc);
    if ( conn < 0 )
        return -EINVAL;

    set_qsb(vcc, qos, conn);

    return 0;
}

#ifdef CONFIG_IFX_ADSL_ATM_LED
static void adsl_led_flash(void) //tc.chen
{
	if ( adsl_led_flash_cb )
		adsl_led_flash_cb();
}
#endif

/*
 *  Description:
 *    Add a 32-bit value to 64-bit value, and put result in a 64-bit variable.
 *  Input:
 *    opt1 --- ppe_u64_t, first operand, a 64-bit unsigned integer value
 *    opt2 --- u32, second operand, a 32-bit unsigned integer value
 *    ret  --- ppe_u64_t, pointer to a variable to hold result
 *  Output:
 *    none
 */
static INLINE void u64_add_u32(ppe_u64_t opt1, u32 opt2,ppe_u64_t *ret)
{
    ret->l = opt1.l + opt2;
    if ( ret->l < opt1.l || ret->l < opt2 )
        ret->h++;
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
    skb = dev_alloc_skb(ppe_dev.aal5.rx_buffer_size + DMA_ALIGNMENT);
    if ( skb )
    {
        /*  must be burst length alignment  */
        if ( ((u32)skb->data & (DMA_ALIGNMENT - 1)) != 0 )
            skb_reserve(skb, ~((u32)skb->data + (DMA_ALIGNMENT - 1)) & (DMA_ALIGNMENT - 1));
        /*  put skb in reserved area "skb->data - 4"    */
        *((u32*)skb->data - 1) = (u32)skb;
        /*  invalidate cache    */
        dma_cache_inv((unsigned long)skb->head, (u32)skb->end - (u32)skb->head);
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

    /*  allocate memory including header and padding    */
    size += TX_INBAND_HEADER_LENGTH + MAX_TX_PACKET_ALIGN_BYTES + MAX_TX_PACKET_PADDING_BYTES;
    size &= ~(DMA_ALIGNMENT - 1);
    skb = dev_alloc_skb(size + DMA_ALIGNMENT);
    /*  must be burst length alignment  */
    if ( skb )
        skb_reserve(skb, (~((u32)skb->data + (DMA_ALIGNMENT - 1)) & (DMA_ALIGNMENT - 1)) + TX_INBAND_HEADER_LENGTH);
    return skb;
}

/*
 *  Description:
 *    In RX path, the sk_buff is allocated before length is confirmed, so that
 *    it takes maximum value as buffer size. As the data length is confirmed,
 *    some member of sk_buff need to be set to the actual value.
 *  Input:
 *    skb      --- struct sk_buff *, sk_buff need to be updated
 *    size     --- unsigned int, actual size of data
 *    is_cell  --- int, 0 means AAL5 packet, else means AAL0/OAM cell.
 *  Output:
 *    none
 */
static INLINE void resize_skb_rx(struct sk_buff *skb, unsigned int size, int is_cell)
{
    if ( (u32)skb < 0x80000000 )
    {
        int key = 0;
        printk("resize_skb_rx problem: skb = %08X, size = %d, is_cell = %d\n", (u32)skb, size, is_cell);
        while ( !key )
        {
        }
    }
//    size = SKB_DATA_ALIGN(size + 16 + DMA_ALIGNMENT + (is_cell ? 0 : MAX_RX_FRAME_EXTRA_BYTES));

    /*  do not modify 'truesize' and 'end', or crash kernel */

    /*  Load the data pointers. */
#if 0
    skb->data = (unsigned char*)(((u32)skb->head + 16 + 0x0F) & ~0x0F);
#else
    skb->data = (unsigned char*)(((u32)skb->head + 16 + (DMA_ALIGNMENT - 1)) & ~(DMA_ALIGNMENT - 1));
#endif
    skb->tail = skb->data;

    /*  put skb in reserved area "skb->data - 4"    */
//    *((u32*)skb->data - 1) = (u32)skb;

    /*  Set up other state  */
    skb->len = 0;
    skb->cloned = 0;
#if defined(CONFIG_IMQ) || defined (CONFIG_IMQ_MODULE)
    skb->imq_flags = 0;
    skb->nf_info = NULL;
#endif
    skb->data_len = 0;
}

/*
 *  Description:
 *    Allocate sk_buff for TX path, and this function will be hooked to higher
 *    layer protocol implementation.
 *  Input:
 *    vcc      --- struct atm_vcc *, structure of an opened connection
 *    size     --- unsigned int, size of the buffer
 *  Output:
 *    sk_buff* --- 0:    Failed
 *                 else: Pointer to sk_buff
 */
#if defined(LINUX_2_4_31) && LINUX_2_4_31
  struct sk_buff* atm_alloc_tx(struct atm_vcc *vcc, unsigned int size)
#else
  static struct sk_buff* atm_alloc_tx(struct atm_vcc *vcc, unsigned int size)
#endif
{
    int conn;
    struct sk_buff *skb;

    /*  oversize packet */
    if ( ((size + TX_INBAND_HEADER_LENGTH + MAX_TX_PACKET_ALIGN_BYTES + MAX_TX_PACKET_PADDING_BYTES) & ~(DMA_ALIGNMENT - 1))  > ppe_dev.aal5.tx_max_packet_size )
    {
        printk("atm_alloc_tx: oversize packet\n");
        return NULL;
    }
#if 1
    /*  send buffer overflow    */
  #if !LINUX_2_4_31
    if ( atomic_read(&vcc->tx_inuse) && !atm_may_send(vcc, size) )
  #else
    if ( atomic_read(&vcc->sk->wmem_alloc) && !atm_may_send(vcc, size) )
  #endif
    {
        printk("atm_alloc_tx: send buffer overflow\n");
        return NULL;
    }
#endif

#ifndef CONFIG_IFX_ADSL_CEOC
    conn = find_vcc(vcc);
    if ( conn < 0 )
    {
        printk("atm_alloc_tx: unknown VCC\n");
        return NULL;
    }
#endif

    skb = dev_alloc_skb(size);
    if ( skb == NULL )
    {
        printk("atm_alloc_tx: sk buffer is used up\n");
        return NULL;
    }

#if !LINUX_2_4_31
    atomic_add(skb->truesize + ATM_PDU_OVHD, &vcc->tx_inuse);
#else
    atomic_add(skb->truesize + ATM_PDU_OVHD, &vcc->sk->wmem_alloc);
#endif

    return skb;
}

/*
 *  Description:
 *    Release sk_buff, which is allocated for TX path.
 *  Input:
 *    skb --- struct sk_buff *, sk_buff to be released.
 *  Output:
 *    none
 */
static INLINE void atm_free_tx_skb_vcc(struct sk_buff *skb)
{
    struct atm_vcc* vcc;

    if ( (u32)skb <= 0x80000000 )
    {
        volatile int key = 0;
        printk("atm_free_tx_skb_vcc: skb = %08X\n", (u32)skb);
        for ( ; !key; );
    }

    vcc = ATM_SKB(skb)->vcc;

#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
    dev_kfree_skb_any(skb);
    return;
#endif
//    printk("skb->users = %d\n", skb->users);
    if ( vcc != NULL && vcc->pop != NULL )
    {
        if ( atomic_read(&skb->users) == 0 )
        {
            volatile int key = 0;
            printk("atm_free_tx_skb_vcc(vcc->pop): skb->users == 0, skb = %08X\n", (u32)skb);
            for ( ; !key; );
        }
        vcc->pop(vcc, skb);
    }
    else
    {
        if ( atomic_read(&skb->users) == 0 )
        {
            volatile int key = 0;
            printk("atm_free_tx_skb_vcc(dev_kfree_skb_any): skb->users == 0, skb = %08X\n", (u32)skb);
            for ( ; !key; );
        }
        dev_kfree_skb_any(skb);
    }
}

/*
 *  Description:
 *    Allocate a TX descriptor for an opened connection.
 *  Input:
 *    connection --- int, connection ID
 *  Output:
 *    int        --- negative value: descriptor is used up.
 *                   else:           index of descriptor relative to the first
 *                                   one of this connection ID.
 */
static INLINE int alloc_tx_connection(int connection)
{
    int sys_flag;
    int desc_base;

#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
    if ( ppe_dev.dma.tx_desc_alloc_pos[connection] == ppe_dev.dma.tx_desc_release_pos[connection] && ppe_dev.dma.tx_desc_alloc_flag[connection] )
        mailbox_tx_irq_handler(connection);
#endif

//    if ( ppe_dev.dma.tx_desc_alloc_num[connection] >= ppe_dev.dma.tx_descriptor_number )
//        return -1;
    if ( ppe_dev.dma.tx_desc_alloc_pos[connection] == ppe_dev.dma.tx_desc_release_pos[connection] && ppe_dev.dma.tx_desc_alloc_flag[connection] )
        return -1;

    /*  amend descriptor pointer and allocation number  */
    local_irq_save(sys_flag);
    desc_base = ppe_dev.dma.tx_descriptor_number * (connection - QSB_QUEUE_NUMBER_BASE) + ppe_dev.dma.tx_desc_alloc_pos[connection];
    if ( ++ppe_dev.dma.tx_desc_alloc_pos[connection] == ppe_dev.dma.tx_descriptor_number )
        ppe_dev.dma.tx_desc_alloc_pos[connection] = 0;
    ppe_dev.dma.tx_desc_alloc_flag[connection] = 1;
    local_irq_restore(sys_flag);


//    ppe_dev.dma.tx_desc_alloc_num[connection]++;

//    g_tx_desc_num++;
    return desc_base;
}

/*
 *  Description:
 *    Handle IRQ triggered by received packet (RX).
 *  Input:
 *    channel --- unsigned int, channel ID which triggered IRQ
 *    len     --- unsigned int, retrieve length of packet/cell processed, NULL
 *                means to ignore the length
 *  Output:
 *    int     --- 0:    Success
 *                else: Error Code (-EAGAIN, retry until owner flag set)
 */
static INLINE int mailbox_rx_irq_handler(unsigned int channel, unsigned int *len)
{
    int conn;
    int skb_base;
    register struct rx_descriptor reg_desc;
    struct rx_descriptor *desc;
    struct sk_buff *skb;
    struct atm_vcc *vcc;
    struct rx_inband_trailer *trailer;

//  dbg("mailbox_rx_irq_handler (ch %d)", channel);
//  printk("mailbox_rx_irq_handler (ch %d)\n", channel);

    /*  get sk_buff pointer and descriptor  */
    skb_base = ppe_dev.dma.rx_descriptor_number * channel + ppe_dev.dma.rx_desc_read_pos[channel];
//  printk("ppe_dev.dma.rx_desc_read_pos[%d] = %d, skb_base = %d\n", channel, ppe_dev.dma.rx_desc_read_pos[channel], skb_base);
//  if ( ++ppe_dev.dma.rx_desc_read_pos[channel] == ppe_dev.dma.rx_descriptor_number )
//      ppe_dev.dma.rx_desc_read_pos[channel] = 0;
//  skb = ppe_dev.dma.rx_skb_pointers[skb_base];

    desc = &ppe_dev.dma.rx_descriptor_base[skb_base];
//  dbg("desc = 0x%08X", (u32)desc);

    /*  load descriptor from memory */
    reg_desc = *desc;

#if 0
  #if defined(DEBUG_DUMP_SKB) && DEBUG_DUMP_SKB
    skb = *(struct sk_buff **)((((u32)reg_desc.dataptr << 2) | KSEG0) - 4);

    {
        int i;
        unsigned char *ch = (unsigned char *)(((u32)desc->dataptr << 2) | 0x80000000);
        int len = (desc->datalen + desc->byteoff + 8 + 3) & ~0x03;

        printk("RX IRQ\n");
        printk("  desc (0x%08X) = 0x%08X 0x%08X\n", (u32)desc, *(u32*)desc, *((u32*)desc + 1));
        printk("  skb (0x%08X)\n", (u32)skb);
        printk("  skb->len = %d\n", skb->len);
        printk("  skb->data = 0x%08X, desc->dataptr = 0x%08X\n", (u32)skb->data, (u32)desc->dataptr << 2);
        for ( i = 0; i < len; i++ )
        {
            if ( (i & 0x0F) == 0 )
                printk("    %4d:", i);
            printk(" %02X", (u32)ch[i]);
            if ( (i & 0x0F) == 0x0F )
                printk("\n");
        }
        if ( (i & 0x0F) != 0 )
            printk("\n");
    }
  #endif
#endif

    if ( reg_desc.own || !reg_desc.c )
        return -EAGAIN;

    if ( ++ppe_dev.dma.rx_desc_read_pos[channel] == ppe_dev.dma.rx_descriptor_number )
        ppe_dev.dma.rx_desc_read_pos[channel] = 0;

//    printk("RX updating descriptor succeeded: 0x%08X 0x%08X, qid %d\n", *(u32*)desc, *((u32*)desc + 1), desc->id);

    skb = *(struct sk_buff **)((((u32)reg_desc.dataptr << 2) | KSEG0) - 4);
    if ( (u32)skb <= 0x80000000 )
    {
        int key = 0;
        printk("skb problem: skb = %08X, system is panic!\n", (u32)skb);
        for ( ; !key; );
    }

    conn = reg_desc.id;

    if ( conn == ppe_dev.oam_rx_queue )
    {
//      dbg("OAM");
//      printk("OAM\n");

        /*  OAM */
        struct uni_cell_header *header = (struct uni_cell_header *)skb->data;

        if ( header->pti == ATM_PTI_SEGF5 || header->pti == ATM_PTI_E2EF5 )
            conn = find_vpivci(header->vpi, header->vci);
        else if ( header->vci == 0x03 || header->vci == 0x04 )
            conn = find_vpi(header->vpi);
        else
            conn = -1;

        if ( conn >= 0 && ppe_dev.connection[conn].vcc != NULL )
        {
            vcc = ppe_dev.connection[conn].vcc;
            ppe_dev.connection[conn].access_time = xtime;

#ifdef CONFIG_IFX_ADSL_ATM_LED
            adsl_led_flash(); //taicheng : for adsl data led
#endif

            if ( vcc->push_oam != NULL )
                vcc->push_oam(vcc, skb->data);
            else
                amz_push_oam(skb->data);
        }

        /*  don't need resize   */
    }
    else
    {
//      dbg("Not OAM");

        if ( len )
            *len = 0;

        if ( ppe_dev.connection[conn].vcc != NULL )
        {
            vcc = ppe_dev.connection[conn].vcc;

            if ( vcc->qos.aal == ATM_AAL5 )
            {
                if ( !reg_desc.err )
                {
//                  dbg("AAL5 packet");
//                  printk("AAL5 packet\n");

                    /*  AAL5 packet */
                    resize_skb_rx(skb, reg_desc.datalen + reg_desc.byteoff, 0);
                    skb_reserve(skb, reg_desc.byteoff);
                    skb_put(skb, reg_desc.datalen);

#if defined(DEBUG_DUMP_SKB) && DEBUG_DUMP_SKB
                    {
                        int i;
                        int len;

                        len = (skb->len + reg_desc.byteoff + 8 + 3) & 0xFFFFFFFC;
                        printk("RX data\n");
                        printk("  skb->len = %d\n", skb->len);
                        printk("  skb->data:\n");
                        for ( i = 0; i < len; i++ )
                        {
                            if ( (i & 0x0F) == 0 )
                                printk("    %4d:", i);
                            printk(" %02X", (u32)skb->data[i]);
                            if ( (i & 0x0F) == 0x0F )
                                printk("\n");
                        }
                        if ( (i & 0x0F) != 0 )
                            printk("\n");
                    }
#endif

//                  dbg("after resize");

//                  dbg("ATM_SKB(skb) = 0x%08X", (u32)ATM_SKB(skb));
                    if ( (u32)ATM_SKB(skb) <= 0x80000000 )
                    {
                        int key = 0;
                        printk("ATM_SKB(skb) problem: ATM_SKB(skb) = %08X, system is panic!\n", (u32)ATM_SKB(skb));
                        for ( ; !key; );
                    }
                    ATM_SKB(skb)->vcc = vcc;
                    ppe_dev.connection[conn].access_time = xtime;
//                  dbg("before atm_charge");
                    if ( atm_charge(vcc, skb->truesize) )
                    {
                        struct sk_buff *new_skb;
//                      dbg("atm_charge ok");

                        new_skb = alloc_skb_rx();
                        if ( new_skb )
                        {

                            UPDATE_VCC_STAT(conn, rx_pdu, 1);

//                            dbg("vcc = 0x%08X", (u32)vcc);
//                            dbg("vcc->stats = 0x%08X", (u32)vcc->stats);

                            ppe_dev.mib.wrx_pdu++;
                            if ( vcc->stats )
                                atomic_inc(&vcc->stats->rx);
//                          dbg("before vcc->push(0x%08X, 0x%08X)", (u32)vcc, (u32)skb);
//                          dbg("skb->head = 0x%08X", (u32)skb->head);
//                          dbg("skb->data = 0x%08X", (u32)skb->data);
//                          dbg("skb->tail = 0x%08X", (u32)skb->tail);
//                          dbg("skb->end = 0x%08X", (u32)skb->end);
//                          dbg("skb->len = %d", (u32)skb->len);
//                          dbg("skb->data_len = %d", (u32)skb->data_len);
//                          dbg("vcc->vpi = %d", (u32)vcc->vpi);
//                          dbg("vcc->vci = %d", (u32)vcc->vci);
//                          dbg("vcc->itf = %d", (u32)vcc->itf);
//                          {
//                              char *ptr = (char *)skb->data;
//                              int ti;
//
//                              printk(KERN_DEBUG "receive: ");
//                              for ( ti = 0; ti < skb->len; ti++ )
//                                  if ( (unsigned char)ptr[ti] < 0x20 || (unsigned char)ptr[ti] > 0x7F )
//                                      printk("%d", (u32)ptr[ti]);
//                                  else
//                                      printk("%c", ptr[ti]);
//                              printk("\n");
//                          }
#if 1
  #if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
                            ppe_dev.rx_bytes_cur[conn] += skb->len;
  #endif

  #ifdef CONFIG_IFX_ADSL_ATM_LED
                            adsl_led_flash(); //taicheng : for adsl data led
  #endif

  #if defined(DEBUG_ABANDON_RX) && DEBUG_ABANDON_RX
                            atm_return(vcc, skb->truesize);
                            dev_kfree_skb_any(skb);
  #else
                            vcc->push(vcc, skb);
  #endif
#else
                            {
                                struct k_atm_aal_stats stats = *vcc->stats;
                                int flag = 0;

  #if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
                                ppe_dev.rx_bytes_cur[conn] += skb->len;
  #endif

  #ifdef CONFIG_IFX_ADSL_ATM_LED
                                adsl_led_flash(); //taicheng : for adsl data led
  #endif
                                vcc->push(vcc, skb);
                                if ( vcc->stats->rx.counter != stats.rx.counter )
                                {
                                    dbg("vcc->stats->rx (diff) = %d", vcc->stats->rx.counter - stats.rx.counter);
                                    flag++;
                                }
                                if ( vcc->stats->rx_err.counter != stats.rx_err.counter )
                                {
                                    dbg("vcc->stats->rx_err (diff) = %d", vcc->stats->rx_err.counter - stats.rx_err.counter);
                                    flag++;
                                }
                                if ( vcc->stats->rx_drop.counter != stats.rx_drop.counter )
                                {
                                    dbg("vcc->stats->rx_drop (diff) = %d", vcc->stats->rx_drop.counter - stats.rx_drop.counter);
                                    flag++;
                                }
                                if ( vcc->stats->tx.counter != stats.tx.counter )
                                {
                                    dbg("vcc->stats->tx (diff) = %d", vcc->stats->tx.counter - stats.tx.counter);
                                    flag++;
                                }
                                if ( vcc->stats->tx_err.counter != stats.tx_err.counter )
                                {
                                    dbg("vcc->stats->tx_err (diff) = %d", vcc->stats->tx_err.counter - stats.tx_err.counter);
                                    flag++;
                                }
                                if ( !flag )
                                    dbg("vcc->stats not changed");
                            }
#endif
//                          dbg("after vcc->push(0x%08X, 0x%08X)", (u32)vcc, (u32)skb);
//                          printk("after vcc->push(0x%08X, 0x%08X)\n", (u32)vcc, (u32)skb);

//                          ppe_dev.dma.rx_skb_pointers[skb_base] = alloc_skb_rx();
//                          reg_desc.dataptr = (u32)ppe_dev.dma.rx_skb_pointers[skb_base]->data >> 2;
                            reg_desc.dataptr = (u32)new_skb->data >> 2;

                            if ( len )
                                *len = reg_desc.datalen;
                        }
                        else
                        {
                            /*  no sk buffer    */
                            UPDATE_VCC_STAT(conn, rx_sw_drop_pdu, 1);

                            ppe_dev.mib.wrx_drop_pdu++;
                            if ( vcc->stats )
                                atomic_inc(&vcc->stats->rx_drop);

                            resize_skb_rx(skb, ppe_dev.aal5.rx_buffer_size, 0);
                        }
                    }
                    else
                    {
//                      dbg("atm_charge fail");
//                      printk("atm_charge fail\n");

                        /*  no enough space */
                        UPDATE_VCC_STAT(conn, rx_sw_drop_pdu, 1);

                        ppe_dev.mib.wrx_drop_pdu++;
                        if ( vcc->stats )
                            atomic_inc(&vcc->stats->rx_drop);

                        resize_skb_rx(skb, ppe_dev.aal5.rx_buffer_size, 0);
                    }

//                  dbg("after process");
                }
                else
                {
//                  printk("reg_desc.err\n");

                    /*  drop packet/cell    */
                    if ( vcc->qos.aal == ATM_AAL5 )
                    {
                        UPDATE_VCC_STAT(conn, rx_err_pdu, 1);

                        trailer = (struct rx_inband_trailer *)((u32)skb->data + ((reg_desc.byteoff + reg_desc.datalen + DMA_ALIGNMENT - 1) & ~ (DMA_ALIGNMENT - 1)));
                        if ( trailer->stw_crc )
                            ppe_dev.connection[conn].aal5_vcc_crc_err++;
                        if ( trailer->stw_ovz )
                            ppe_dev.connection[conn].aal5_vcc_oversize_sdu++;
//                      ppe_dev.mib.wrx_drop_pdu++;
                    }
                    if ( vcc->stats )
                    {
//                      atomic_inc(&vcc->stats->rx_drop);
                        atomic_inc(&vcc->stats->rx_err);
                    }

                    /*  don't need resize   */
                }
            }
            else
            {
//              printk("AAL0 cell\n");

                /*  AAL0 cell   */
                resize_skb_rx(skb, CELL_SIZE, 1);
                skb_put(skb, CELL_SIZE);

                ATM_SKB(skb)->vcc = vcc;
                ppe_dev.connection[conn].access_time = xtime;
#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
                if ( 1 )
                {
//                  vcc->qos.aal = ATM_AAL0;
                    ppe_send(vcc, skb);
                }
                else
#endif
                if ( atm_charge(vcc, skb->truesize) )
                {
                    struct sk_buff *new_skb;

                    new_skb = alloc_skb_rx();
                    if ( new_skb )
                    {
#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
                        ppe_dev.rx_bytes_cur[conn] += skb->len;
#endif

                        if ( vcc->stats )
                            atomic_inc(&vcc->stats->rx);
#ifdef CONFIG_IFX_ADSL_ATM_LED
                        adsl_led_flash(); //taicheng : for adsl data led
#endif
                        vcc->push(vcc, skb);

//                      ppe_dev.dma.rx_skb_pointers[skb_base] = alloc_skb_rx();
//                      reg_desc.dataptr = (u32)ppe_dev.dma.rx_skb_pointers[skb_base]->data >> 2;
                        reg_desc.dataptr = (u32)new_skb->data >> 2;

                        if ( len )
                            *len = CELL_SIZE;
                    }
                    else
                    {
                        if ( vcc->stats )
                            atomic_inc(&vcc->stats->rx_drop);
                        resize_skb_rx(skb, ppe_dev.aal5.rx_buffer_size, 0);
                    }
                }
                else
                {
                    if ( vcc->stats )
                        atomic_inc(&vcc->stats->rx_drop);
                    resize_skb_rx(skb, ppe_dev.aal5.rx_buffer_size, 0);
                }
            }
        }
        else
        {
//          printk("ppe_dev.connection[%d].vcc == NULL\n", conn);

            ppe_dev.mib.wrx_drop_pdu++;

            /*  don't need resize   */
        }
    }

//  resize_skb_rx(ppe_dev.dma.rx_skb_pointers[skb_base], ppe_dev.aal5.rx_buffer_size, 0);

    reg_desc.byteoff = 0;
    reg_desc.datalen = ppe_dev.aal5.rx_buffer_size;
    reg_desc.own = 1;
    reg_desc.c   = 0;

    /*  write discriptor to memory  */
    *desc = reg_desc;

//  dbg("leave mailbox_rx_irq_handler");

#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
    count_receive = get_count();
    interval_rxreceive += count_receive - count_rx;
    interval_sendreceive += count_receive - count_send;
//  printk("send = %d, tx = %d, rx = %d, receive = %d\n", count_send, count_tx, count_rx, count_receive);
#endif

    return 0;
}

/*
 *  Description:
 *    Handle IRQ triggered by a released buffer of TX path.
 *  Input:
 *    conn --- unsigned int, channel ID which triggered IRQ. It's same as the
 *             connection ID.
 *  Output:
 *    none
 */
static INLINE void mailbox_tx_irq_handler(unsigned int conn)
{
#if 1
    if ( ppe_dev.dma.tx_desc_alloc_flag[conn] )
    {
        int desc_base;
        int *release_pos;
        struct sk_buff *skb;

        release_pos = &ppe_dev.dma.tx_desc_release_pos[conn];
        desc_base = ppe_dev.dma.tx_descriptor_number * (conn - QSB_QUEUE_NUMBER_BASE) + *release_pos;
        while ( !ppe_dev.dma.tx_descriptor_base[desc_base].own )
        {
            skb = ppe_dev.dma.tx_skb_pointers[desc_base];

            ppe_dev.dma.tx_descriptor_base[desc_base].own = 1;  //  pretend PP32 hold owner bit, so that won't be released more than once, so allocation process don't check this bit

            if ( ++*release_pos == ppe_dev.dma.tx_descriptor_number )
                *release_pos = 0;

            if ( *release_pos == ppe_dev.dma.tx_desc_alloc_pos[conn] )
            {
                ppe_dev.dma.tx_desc_alloc_flag[conn] = 0;

                atm_free_tx_skb_vcc(skb);
                break;
            }

            if ( *release_pos == 0 )
                desc_base = ppe_dev.dma.tx_descriptor_number * (conn - QSB_QUEUE_NUMBER_BASE);
            else
                desc_base++;

            atm_free_tx_skb_vcc(skb);
        }
    }
#elif 0
    int skb_base;
    struct sk_buff *skb;
    unsigned int tx_desc_num = WTX_DMA_CHANNEL_CONFIG(conn)->vlddes;    /*  shadow of valid descriptor number   */

//    printk("mailbox_tx_irq_handler (ch %d)\n", conn);

    for ( ; ppe_dev.dma.tx_desc_alloc_num[conn] > tx_desc_num; ppe_dev.dma.tx_desc_alloc_num[conn]-- )
    {
        /*  release sk_buff */
        skb_base = ppe_dev.dma.tx_descriptor_number * (conn - QSB_QUEUE_NUMBER_BASE) + (ppe_dev.dma.tx_desc_alloc_pos[conn] + ppe_dev.dma.tx_descriptor_number - ppe_dev.dma.tx_desc_alloc_num[conn]) % ppe_dev.dma.tx_descriptor_number;
        skb = ppe_dev.dma.tx_skb_pointers[skb_base];

        *my_sk_pop_pointer++ = skb;
        if ( (u32)skb <= 0x80000000 )
        {
#if 1
            struct sk_buff **p;
            int i;

            printk("push skb\n");
            for ( p = my_sk_push, i = 0; p != my_sk_push_pointer && i < sizeof(my_sk_push) / sizeof(*my_sk_push); p++, i++ )
            {
                printk(" %08X", (u32)*p);
                if ( i % 8 == 7 )
                    printk("\n");
            }
            if ( i % 8 != 7 )
                printk("\n");

            printk("pop skb\n");
            for ( p = my_sk_pop, i = 0; p != my_sk_pop_pointer && i < sizeof(my_sk_pop) / sizeof(*my_sk_pop); p++, i++ )
            {
                printk(" %08X", (u32)*p);
                if ( i % 8 == 7 )
                    printk("\n");
            }
            if ( i % 8 != 7 )
                printk("\n");

            printk("conn %d: skb_base = %d, tx_desc_alloc_pos = %d, tx_desc_alloc_num = %d, tx_skb_pointers = %08X\n",
                    conn, skb_base, ppe_dev.dma.tx_desc_alloc_pos[conn], ppe_dev.dma.tx_desc_alloc_num[conn], (u32)ppe_dev.dma.tx_skb_pointers + ppe_dev.dma.tx_descriptor_number * (conn - QSB_QUEUE_NUMBER_BASE));
#else
            break;
#endif
        }
        atm_free_tx_skb_vcc(skb);
//        g_tx_desc_num--;
    }
#if 0
    if ( g_tx_desc_num != tx_desc_num )
    {
        int key = 0;
        printk("g_tx_desc_num = %d, tx_desc_num = %d\n", g_tx_desc_num, tx_desc_num);
        for ( ; !key; );
    }
#endif

#else
    int skb_base;
    struct sk_buff *skb;

    if ( ppe_dev.dma.tx_desc_alloc_num[conn] != 0 )
    {
        /*  release sk_buff */
        skb_base = ppe_dev.dma.tx_descriptor_number * (conn - QSB_QUEUE_NUMBER_BASE) + (ppe_dev.dma.tx_desc_alloc_pos[conn] + ppe_dev.dma.tx_descriptor_number - ppe_dev.dma.tx_desc_alloc_num[conn]) % ppe_dev.dma.tx_descriptor_number;
        skb = ppe_dev.dma.tx_skb_pointers[skb_base];

        atm_free_tx_skb_vcc(skb);

        ppe_dev.dma.tx_desc_alloc_num[conn]--;
    }
#endif
}

#if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
/*
 *  Description:
 *    Check if the descriptor is updated.
 *  Input:
 *    channel --- unsigned int, channel ID which to check
 *  Output:
 *    int     --- 1: valid, 0: invalid
 */
static INLINE int check_desc_valid(unsigned int channel)
{
    int skb_base;
    struct rx_descriptor *desc;

    skb_base = ppe_dev.dma.rx_descriptor_number * channel + ppe_dev.dma.rx_desc_read_pos[channel];
    desc = &ppe_dev.dma.rx_descriptor_base[skb_base];
    return !desc->own && desc->c ? 1 : 0;
}
#endif

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
#if 1   /* DMA channel attached to QID is align with RX QoS parameter   */
    int channel_mask;   /*  DMA channel accordant IRQ bit mask  */
    int channel;
    unsigned int rx_irq_number[MAX_RX_DMA_CHANNEL_NUMBER] = {0};
    unsigned int total_rx_irq_number = 0;

//    dbg("mailbox_irq_handler");
//    printk("mailbox_irq_handler: 0x%08X\n", *MBOX_IGU1_ISR);

#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
    if ( (*MBOX_IGU1_ISR & 0xFFFF) )
    {
        count_rx = get_count();
        interval_txrx += count_rx - count_tx;
    }
#endif

    if ( !*MBOX_IGU1_ISR )
        return;

    channel_mask = 1;
    channel = 0;
    while ( channel < ppe_dev.dma.rx_total_channel_used )
    {
        if ( (*MBOX_IGU1_ISR & channel_mask) )
        {
            /*  RX  */
            /*  clear IRQ   */
            *MBOX_IGU1_ISRC = channel_mask;
//            printk("  RX: *MBOX_IGU1_ISR = 0x%08X\n", *MBOX_IGU1_ISR);
  #if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
            *MBOX_IGU1_ISR ^= channel_mask;
            *MBOX_IGU1_ISRC = 0;
  #endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

            /*  wait for mailbox cleared    */
            while ( (*MBOX_IGU3_ISR & channel_mask) );

            /*  shadow the number of valid descriptor   */
            rx_irq_number[channel] = WRX_DMA_CHANNEL_CONFIG(channel)->vlddes;

            total_rx_irq_number += rx_irq_number[channel];

//            dbg("total_rx_irq_number = %d", total_rx_irq_number);
//            printk("vlddes = %d, rx_irq_number[%d] = %d, total_rx_irq_number = %d\n", WRX_DMA_CHANNEL_CONFIG(channel)->vlddes, channel, rx_irq_number[channel], total_rx_irq_number);
        }

        channel_mask <<= 1;
        channel++;
    }

    channel_mask = 1 << (16 + QSB_QUEUE_NUMBER_BASE);
    channel = QSB_QUEUE_NUMBER_BASE;
    while ( channel - QSB_QUEUE_NUMBER_BASE < ppe_dev.dma.tx_total_channel_used )
    {
        if ( (*MBOX_IGU1_ISR & channel_mask) )
        {
//            if ( channel != 1 )
//            {
//                printk("TX irq error\n");
//                while ( 1 )
//                {
//                }
//            }
            /*  TX  */
            /*  clear IRQ   */
            *MBOX_IGU1_ISRC = channel_mask;
//            printk("  TX: *MBOX_IGU1_ISR = 0x%08X\n", *MBOX_IGU1_ISR);
  #if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
            *MBOX_IGU1_ISR ^= channel_mask;
            *MBOX_IGU1_ISRC = 0;
  #endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

            mailbox_tx_irq_handler(channel);
        }

        channel_mask <<= 1;
        channel++;
    }

  #if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
    channel = 0;
    while ( total_rx_irq_number )
    {
        switch ( channel )
        {
        case RX_DMA_CH_CBR:
        case RX_DMA_CH_OAM:
            /*  handle it as soon as possible   */
            while ( rx_irq_number[channel] != 0 && mailbox_rx_irq_handler(channel, NULL) == 0 )
            {
                rx_irq_number[channel]--;
                total_rx_irq_number--;
//                dbg("RX_DMA_CH_CBR, total_rx_irq_number = %d", total_rx_irq_number);
//                printk("RX_DMA_CH_CBR, total_rx_irq_number = %d, rx_irq_number[%d] = %d\n", total_rx_irq_number, channel, rx_irq_number[channel]);
                /*  signal firmware that descriptor is updated */
                mailbox_signal(channel, 0);
            }
//            if ( rx_irq_number[channel] != 0 )
//                dbg("RX_DMA_CH_CBR, rx_irq_number[channel] = %d", rx_irq_number[channel]);
            break;
        case RX_DMA_CH_VBR_RT:
            /*  WFQ */
            if ( rx_irq_number[RX_DMA_CH_VBR_RT] != 0
                && (rx_irq_number[RX_DMA_CH_VBR_NRT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_NRT) || ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] < ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT])
                && (rx_irq_number[RX_DMA_CH_AVR] == 0 || !check_desc_valid(RX_DMA_CH_AVR) || ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] < ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT])
            )
            {
                unsigned int len;

                if ( mailbox_rx_irq_handler(RX_DMA_CH_VBR_RT, &len) == 0 )
                {
                    rx_irq_number[RX_DMA_CH_VBR_RT]--;
                    total_rx_irq_number--;
//                    dbg("RX_DMA_CH_VBR_RT, total_rx_irq_number = %d", total_rx_irq_number);
//                    printk("RX_DMA_CH_VBR_RT, total_rx_irq_number = %d, rx_irq_number[%d] = %d\n", total_rx_irq_number, channel, rx_irq_number[channel]);
                    /*  signal firmware that descriptor is updated */
                    mailbox_signal(channel, 0);

                    len = (len + CELL_SIZE - 1) / CELL_SIZE;
                    if ( ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] <= len )
                        ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] = ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_RT] + ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] - len;
                }
            }
//            if ( rx_irq_number[channel] != 0 )
//            {
//                dbg("RX_DMA_CH_VBR_RT, rx_irq_number[channel] = %d, total_rx_irq_number = %d", rx_irq_number[channel], total_rx_irq_number);
//                rx_irq_number[channel] = 0;
//                total_rx_irq_number = 0;
//            }
            break;
        case RX_DMA_CH_VBR_NRT:
            /*  WFQ */
            if ( rx_irq_number[RX_DMA_CH_VBR_NRT] != 0
                && (rx_irq_number[RX_DMA_CH_VBR_RT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_RT) || ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] < ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT])
                && (rx_irq_number[RX_DMA_CH_AVR] == 0 || !check_desc_valid(RX_DMA_CH_AVR) || ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] < ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT])
            )
            {
                unsigned int len;

                if ( mailbox_rx_irq_handler(RX_DMA_CH_VBR_NRT, &len) == 0 )
                {
                    rx_irq_number[RX_DMA_CH_VBR_NRT]--;
                    total_rx_irq_number--;
//                    dbg("RX_DMA_CH_VBR_NRT, total_rx_irq_number = %d", total_rx_irq_number);
//                    printk("RX_DMA_CH_VBR_NRT, total_rx_irq_number = %d, rx_irq_number[%d] = %d\n", total_rx_irq_number, channel, rx_irq_number[channel]);
                    /*  signal firmware that descriptor is updated */
                    mailbox_signal(channel, 0);

                    len = (len + CELL_SIZE - 1) / CELL_SIZE;
                    if ( ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] <= len )
                        ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] = ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_NRT] + ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] - len;
                }
            }
//            if ( rx_irq_number[channel] != 0 )
//                dbg("RX_DMA_CH_VBR_NRT, rx_irq_number[channel] = %d", rx_irq_number[channel]);
            break;
        case RX_DMA_CH_AVR:
            /*  WFQ */
            if ( rx_irq_number[RX_DMA_CH_AVR] != 0
                && (rx_irq_number[RX_DMA_CH_VBR_RT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_RT) || ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] < ppe_dev.dma.rx_weight[RX_DMA_CH_AVR])
                && (rx_irq_number[RX_DMA_CH_VBR_NRT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_NRT) || ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] < ppe_dev.dma.rx_weight[RX_DMA_CH_AVR])
            )
            {
                unsigned int len;

                if ( mailbox_rx_irq_handler(RX_DMA_CH_AVR, &len) == 0 )
                {
                    rx_irq_number[RX_DMA_CH_AVR]--;
                    total_rx_irq_number--;
//                    dbg("RX_DMA_CH_AVR, total_rx_irq_number = %d", total_rx_irq_number);
//                    printk("RX_DMA_CH_AVR, total_rx_irq_number = %d, rx_irq_number[%d] = %d\n", total_rx_irq_number, channel, rx_irq_number[channel]);
                    /*  signal firmware that descriptor is updated */
                    mailbox_signal(channel, 0);

                    len = (len + CELL_SIZE - 1) / CELL_SIZE;
                    if ( ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] <= len )
                        ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] = ppe_dev.dma.rx_default_weight[RX_DMA_CH_AVR] + ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] - len;
                }
            }
//            if ( rx_irq_number[channel] != 0 )
//                dbg("RX_DMA_CH_AVR, rx_irq_number[channel] = %d", rx_irq_number[channel]);
            break;
        case RX_DMA_CH_UBR:
        default:
            /*  Handle it when all others are handled or others are not available to handle.    */
            if ( rx_irq_number[channel] != 0
                && (rx_irq_number[RX_DMA_CH_VBR_RT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_RT))
                && (rx_irq_number[RX_DMA_CH_VBR_NRT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_NRT))
                && (rx_irq_number[RX_DMA_CH_AVR] == 0 || !check_desc_valid(RX_DMA_CH_AVR)) )
                if ( mailbox_rx_irq_handler(channel, NULL) == 0 )
                {
                    rx_irq_number[channel]--;
                    total_rx_irq_number--;
//                    dbg("RX_DMA_CH_UBR, total_rx_irq_number = %d, rx_irq_number[%d] = %d", total_rx_irq_number, channel, rx_irq_number[channel]);
//                    printk("RX_DMA_CH_UBR, total_rx_irq_number = %d, rx_irq_number[%d] = %d\n", total_rx_irq_number, channel, rx_irq_number[channel]);
                    /*  signal firmware that descriptor is updated */
                    mailbox_signal(channel, 0);
                }
//            if ( rx_irq_number[channel] != 0 )
//            {
//                dbg("RX_DMA_CH_UBR, rx_irq_number[channel] = %d", rx_irq_number[channel]);
//                rx_irq_number[channel] = 0;
//                total_rx_irq_number = 0;
//            }
        }

        if ( ++channel == ppe_dev.dma.rx_total_channel_used )
            channel = 0;
    }
  #else
    channel = 0;
    while ( total_rx_irq_number )
    {
        while ( rx_irq_number[channel] != 0 && mailbox_rx_irq_handler(channel, NULL) == 0 )
        {
            rx_irq_number[channel]--;
            total_rx_irq_number--;
            /*  signal firmware that descriptor is updated */
            mailbox_signal(channel, 0);
        }

        if ( ++channel == ppe_dev.dma.rx_total_channel_used )
            channel = 0;
    }
  #endif  //  defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
#else
    int channel_mask;   /*  DMA channel accordant IRQ bit mask  */
    int channel;
    int is_rx;          /*  0-7 RX, 16-31 TX    */
    unsigned int rx_irq_number[MAX_RX_DMA_CHANNEL_NUMBER] = {0};
    unsigned int total_rx_irq_number = 0;

    if ( irq != PPE_MAILBOX_IGU1_INT )
        return;

    channel_mask = 1;
    channel = 0;
    is_rx = 1;
    while ( *MBOX_IGU1_ISR || total_rx_irq_number != 0 )
    {
        if ( (*MBOX_IGU1_ISR & channel_mask) )
        {
            /*  clear IRQ   */
            *MBOX_IGU1_ISRC = channel_mask;
  #if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
            *MBOX_IGU1_ISR ^= channel_mask;
            *MBOX_IGU1_ISRC = 0;
  #endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

            if ( is_rx )
            {
                /*  increase IRQ number of corresponding channel    */
                rx_irq_number[channel]++;
                total_rx_irq_number++;
            }
            else
                /*  TX Indicator    */
                mailbox_tx_irq_handler(channel);
        }

        /*  RX Indicator    */
  #if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
        if ( is_rx )
        {
            switch ( channel )
            {
            case RX_DMA_CH_CBR:
            case RX_DMA_CH_OAM:
                /*  handle it as soon as possible   */
                while ( rx_irq_number[channel] != 0 && mailbox_rx_irq_handler(channel, NULL) == 0 )
                {
                    rx_irq_number[channel]--;
                    total_rx_irq_number--;
                    /*  signal firmware that descriptor is updated */
                    mailbox_signal_with_mask(channel_mask);
                }
                break;
            case RX_DMA_CH_VBR_RT:
                /*  WFQ */
                if ( rx_irq_number[RX_DMA_CH_VBR_RT] != 0
                    && (rx_irq_number[RX_DMA_CH_VBR_NRT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_NRT) || ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] < ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT])
                    && (rx_irq_number[RX_DMA_CH_AVR] == 0 || !check_desc_valid(RX_DMA_CH_AVR) || ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] < ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT])
                )
                {
                    unsigned int len;

                    if ( mailbox_rx_irq_handler(RX_DMA_CH_VBR_RT, &len) == 0 )
                    {
                        rx_irq_number[RX_DMA_CH_VBR_RT]--;
                        total_rx_irq_number--;
                        /*  signal firmware that descriptor is updated */
                        mailbox_signal_with_mask(channel_mask);

                        len = (len + CELL_SIZE - 1) / CELL_SIZE;
                        if ( ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] <= len )
                            ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] = ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_RT] + ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] - len;
                    }
                }
                break;
            case RX_DMA_CH_VBR_NRT:
                /*  WFQ */
                if ( rx_irq_number[RX_DMA_CH_VBR_NRT] != 0
                    && (rx_irq_number[RX_DMA_CH_VBR_RT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_RT) || ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] < ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT])
                    && (rx_irq_number[RX_DMA_CH_AVR] == 0 || !check_desc_valid(RX_DMA_CH_AVR) || ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] < ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT])
                )
                {
                    unsigned int len;

                    if ( mailbox_rx_irq_handler(RX_DMA_CH_VBR_NRT, &len) == 0 )
                    {
                        rx_irq_number[RX_DMA_CH_VBR_NRT]--;
                        total_rx_irq_number--;
                        /*  signal firmware that descriptor is updated */
                        mailbox_signal_with_mask(channel_mask);

                        len = (len + CELL_SIZE - 1) / CELL_SIZE;
                        if ( ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] <= len )
                            ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] = ppe_dev.dma.rx_default_weight[RX_DMA_CH_VBR_NRT] + ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] - len;
                    }
                }
                break;
            case RX_DMA_CH_AVR:
                /*  WFQ */
                if ( rx_irq_number[RX_DMA_CH_AVR] != 0
                    && (rx_irq_number[RX_DMA_CH_VBR_RT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_RT) || ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_RT] < ppe_dev.dma.rx_weight[RX_DMA_CH_AVR])
                    && (rx_irq_number[RX_DMA_CH_VBR_NRT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_NRT) || ppe_dev.dma.rx_weight[RX_DMA_CH_VBR_NRT] < ppe_dev.dma.rx_weight[RX_DMA_CH_AVR])
                )
                {
                    unsigned int len;

                    if ( mailbox_rx_irq_handler(RX_DMA_CH_AVR, &len) == 0 )
                    {
                        rx_irq_number[RX_DMA_CH_AVR]--;
                        total_rx_irq_number--;
                        /*  signal firmware that descriptor is updated */
                        mailbox_signal_with_mask(channel_mask);

                        len = (len + CELL_SIZE - 1) / CELL_SIZE;
                        if ( ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] <= len )
                            ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] = ppe_dev.dma.rx_default_weight[RX_DMA_CH_AVR] + ppe_dev.dma.rx_weight[RX_DMA_CH_AVR] - len;
                    }
                }
                break;
            case RX_DMA_CH_UBR:
            default:
                /*  Handle it when all others are handled or others are not available to handle.    */
                if ( rx_irq_number[channel] != 0
                    && (rx_irq_number[RX_DMA_CH_VBR_RT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_RT))
                    && (rx_irq_number[RX_DMA_CH_VBR_NRT] == 0 || !check_desc_valid(RX_DMA_CH_VBR_NRT))
                    && (rx_irq_number[RX_DMA_CH_AVR] == 0 || !check_desc_valid(RX_DMA_CH_AVR)) )
                    if ( mailbox_rx_irq_handler(channel, NULL) == 0 )
                    {
                        rx_irq_number[channel]--;
                        total_rx_irq_number--;
                        /*  signal firmware that descriptor is updated */
                        mailbox_signal_with_mask(channel_mask);
                    }
            }
        }
  #else
        if ( is_rx && rx_irq_number[channel] != 0 && mailbox_rx_irq_handler(channel, NULL) == 0 )
        {
            rx_irq_number[channel]--;
            total_rx_irq_number--;
            /*  signal firmware that descriptor is updated */
            mailbox_signal_with_mask(channel_mask);
        }
  #endif  //  defined(ENABLE_RX_QOS) && ENABLE_RX_QOS

        if ( channel_mask == (1 << (ppe_dev.dma.rx_total_channel_used - 1)) )
        {
            /*  go to TX part   */
            channel_mask = 1 << (16 + QSB_QUEUE_NUMBER_BASE);
            channel = QSB_QUEUE_NUMBER_BASE;
            is_rx = 0;
        }
        else if ( channel_mask == (1 << (16 + QSB_QUEUE_NUMBER_BASE + ppe_dev.dma.tx_total_channel_used - 1)) )
        {
            /*  go to RX part   */
            channel_mask = 1;
            channel = 0;
            is_rx = 1;
        }
        else
        {
            channel_mask <<= 1;
            channel++;
        }
    }
#endif
}

/*
 *  Description:
 *    Signal PPE firmware a TX packet ready or RX descriptor updated.
 *  Input:
 *    channel --- unsigned int, connection ID
 *    is_tx   --- int, 0 means RX, else means TX
 *  Output:
 *    none
 */
static INLINE void mailbox_signal(unsigned int channel, int is_tx)
{
    if ( is_tx )
    {
        while ( MBOX_IGU3_ISR_ISR(channel + 16) );
        *MBOX_IGU3_ISRS = MBOX_IGU3_ISRS_SET(channel + 16);

//        dbg("mailbox_signal: tx (ch %d)", channel);
//        printk("mailbox_signal: tx (ch %d)\n", channel);
#if defined(DEBUG_SHOW_COUNT) && DEBUG_SHOW_COUNT
        count_tx = get_count();
        interval_sendtx += count_tx - count_send;
#endif
    }
    else
    {
//        dbg("mailbox_signal: rx (ch %d)", channel);
//        printk("mailbox_signal: rx (ch %d)\n", channel);

        while ( MBOX_IGU3_ISR_ISR(channel) );
        *MBOX_IGU3_ISRS = MBOX_IGU3_ISRS_SET(channel);
    }

#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    /*  clear ISRS and set ISR register */
    if ( is_tx )
    {
        *MBOX_IGU3_ISRS = 0;
        *MBOX_IGU3_ISR |= (1 << (channel + 16));
    }
    else
    {
        *MBOX_IGU3_ISRS = 0;
        *MBOX_IGU3_ISR |= (1 << channel);
    }

//    dbg("before: debug_ppe_sim");
    debug_ppe_sim();
//    dbg("after: debug_ppe_sim");
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
}

#if 0
/*
 *  Description:
 *    Signal PPE firmware a TX packet ready or RX descriptor updated.
 *  Input:
 *    mask --- u32, 1 bit mask to signal corresponding DMA channel
 *  Output:
 *    none
 */
static INLINE void mailbox_signal_with_mask(u32 channel_mask)
{
    while ( (*MBOX_IGU3_ISR & channel_mask) );
    *MBOX_IGU3_ISRS = channel_mask;

#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    /*  clear ISRS and set ISR register */
    *MBOX_IGU3_ISRS = 0;
    *MBOX_IGU3_ISR |= channel_mask;

    debug_ppe_sim();
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
}
#endif

/*
 *  Description:
 *    Setup QSB queue.
 *  Input:
 *    vcc        --- struct atm_vcc *, structure of an opened connection
 *    qos        --- struct atm_qos *, QoS parameter of the connection
 *    connection --- unsigned int, QSB queue ID, which is same as connection ID
 *  Output:
 *    none
 */
static void set_qsb(struct atm_vcc *vcc, struct atm_qos *qos, unsigned int connection)
{
#if !defined(DISABLE_QSB) || !DISABLE_QSB

  #if defined(QSB_IS_HALF_FPI2_CLOCK) && QSB_IS_HALF_FPI2_CLOCK
    u32 qsb_clk = cgu_get_fpi_bus_clock(2) >> 1;    /*  Half of FPI configuration 2 (slow FPI bus) */
  #else
    u32 qsb_clk = cgu_get_fpi_bus_clock(2);         /*  FPI configuration 2 (slow FPI bus) */
  #endif  //  defined(QSB_IS_HALF_FPI2_CLOCK) && QSB_IS_HALF_FPI2_CLOCK
    union qsb_queue_parameter_table qsb_queue_parameter_table = {{0}};
    union qsb_queue_vbr_parameter_table qsb_queue_vbr_parameter_table = {{0}};
    u32 tmp;

  #if defined(DEBUG_QOS) && DEBUG_QOS
    {
        static char *str_traffic_class[9] = {
            "ATM_NONE",
            "ATM_UBR",
            "ATM_CBR",
            "ATM_VBR",
            "ATM_ABR",
            "ATM_ANYCLASS",
            "ATM_VBR_RT",
            "ATM_UBR_PLUS",
            "ATM_MAX_PCR"
        };

        unsigned char traffic_class = qos->txtp.traffic_class;
        int max_pcr = qos->txtp.max_pcr;
        int pcr = qos->txtp.pcr;
        int min_pcr = qos->txtp.min_pcr;
    #if !defined(DISABLE_VBR) || !DISABLE_VBR
        int scr = qos->txtp.scr;
        int mbs = qos->txtp.mbs;
        int cdv = qos->txtp.cdv;
    #endif  //  !defined(DISABLE_VBR) || !DISABLE_VBR

        printk("TX QoS\n");

        printk("  traffic class : ");
        if ( traffic_class == (unsigned char)ATM_MAX_PCR )
            printk("ATM_MAX_PCR\n");
        else if ( traffic_class > ATM_UBR_PLUS )
            printk("Unknown Class\n");
        else
            printk("%s\n", str_traffic_class[traffic_class]);

        printk("  max pcr       : %d\n", max_pcr);
        printk("  desired pcr   : %d\n", pcr);
        printk("  min pcr       : %d\n", min_pcr);

    #if !defined(DISABLE_VBR) || !DISABLE_VBR
        printk("  sustained rate: %d\n", scr);
        printk("  max burst size: %d\n", mbs);
        printk("  cell delay var: %d\n", cdv);
    #endif  //  !defined(DISABLE_VBR) || !DISABLE_VBR
    }
  #endif    //  defined(DEBUG_QOS) && DEBUG_QOS

    /*
     *  Peak Cell Rate (PCR) Limiter
     */
    if ( qos->txtp.max_pcr == 0 )
        qsb_queue_parameter_table.bit.tp = 0;   /*  disable PCR limiter */
    else
    {
        /*  peak cell rate would be slightly lower than requested [maximum_rate / pcr = (qsb_clock / 8) * (time_step / 4) / pcr] */
        tmp = ((qsb_clk * ppe_dev.qsb.tstepc) >> 5) / qos->txtp.max_pcr + 1;
        /*  check if overflow takes place   */
        qsb_queue_parameter_table.bit.tp = tmp > QSB_TP_TS_MAX ? QSB_TP_TS_MAX : tmp;
    }

    /*
     *  Weighted Fair Queueing Factor (WFQF)
     */
    switch ( qos->txtp.traffic_class )
    {
    case ATM_CBR:
    case ATM_VBR_RT:
        /*  real time queue gets weighted fair queueing bypass  */
        qsb_queue_parameter_table.bit.wfqf = 0;
        break;
    case ATM_VBR_NRT:
    case ATM_UBR_PLUS:
        /*  WFQF calculation here is based on virtual cell rates, to reduce granularity for high rates  */
        /*  WFQF is maximum cell rate / garenteed cell rate                                             */
        /*  wfqf = qsb_minimum_cell_rate * QSB_WFQ_NONUBR_MAX / requested_minimum_peak_cell_rate        */
        if ( qos->txtp.min_pcr == 0 )
            qsb_queue_parameter_table.bit.wfqf = QSB_WFQ_NONUBR_MAX;
        else
        {
            tmp = QSB_GCR_MIN * QSB_WFQ_NONUBR_MAX / qos->txtp.min_pcr;
            if ( tmp == 0 )
                qsb_queue_parameter_table.bit.wfqf = 1;
            else if ( tmp > QSB_WFQ_NONUBR_MAX )
                qsb_queue_parameter_table.bit.wfqf = QSB_WFQ_NONUBR_MAX;
            else
                qsb_queue_parameter_table.bit.wfqf = tmp;
        }
        break;
    default:
    case ATM_UBR:
        qsb_queue_parameter_table.bit.wfqf = QSB_WFQ_UBR_BYPASS;
    }

    /*
     *  Sustained Cell Rate (SCR) Leaky Bucket Shaper VBR.0/VBR.1
     */
    if ( qos->txtp.traffic_class == ATM_VBR_RT || qos->txtp.traffic_class == ATM_VBR_NRT )
    {
  #if defined(DISABLE_VBR) && DISABLE_VBR
        /*  disable shaper  */
        qsb_queue_vbr_parameter_table.bit.taus = 0;
        qsb_queue_vbr_parameter_table.bit.ts = 0;
  #else
        if ( qos->txtp.scr == 0 )
        {
            /*  disable shaper  */
            qsb_queue_vbr_parameter_table.bit.taus = 0;
            qsb_queue_vbr_parameter_table.bit.ts = 0;
        }
        else
        {
            /*  Cell Loss Priority  (CLP)   */
            if ( (vcc->atm_options & ATM_ATMOPT_CLP) )
                /*  CLP1    */
                qsb_queue_parameter_table.bit.vbr = 1;
            else
                /*  CLP0    */
                qsb_queue_parameter_table.bit.vbr = 0;
            /*  Rate Shaper Parameter (TS) and Burst Tolerance Parameter for SCR (tauS) */
            tmp = ((qsb_clk * ppe_dev.qsb.tstepc) >> 5) / qos->txtp.scr + 1;
            qsb_queue_vbr_parameter_table.bit.ts = tmp > QSB_TP_TS_MAX ? QSB_TP_TS_MAX : tmp;
            tmp = (qos->txtp.mbs - 1) * (qsb_queue_vbr_parameter_table.bit.ts - qsb_queue_parameter_table.bit.tp) / 64;
            if ( tmp == 0 )
                qsb_queue_vbr_parameter_table.bit.taus = 1;
            else if ( tmp > QSB_TAUS_MAX )
                qsb_queue_vbr_parameter_table.bit.taus = QSB_TAUS_MAX;
            else
                qsb_queue_vbr_parameter_table.bit.taus = tmp;
        }
  #endif
    }
    else
    {
        qsb_queue_vbr_parameter_table.bit.taus = 0;
        qsb_queue_vbr_parameter_table.bit.ts = 0;
    }

    /*  Queue Parameter Table (QPT) */
    *QSB_RTM   = QSB_RTM_DM_SET(QSB_QPT_SET_MASK);
    *QSB_RTD   = QSB_RTD_TTV_SET(qsb_queue_parameter_table.dword);
    *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_QPT) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(connection);
  #if defined(DEBUG_QOS) && DEBUG_QOS
    printk("QPT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
  #endif
    /*  Queue VBR Paramter Table (QVPT) */
    *QSB_RTM   = QSB_RTM_DM_SET(QSB_QVPT_SET_MASK);
    *QSB_RTD   = QSB_RTD_TTV_SET(qsb_queue_vbr_parameter_table.dword);
    *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_VBR) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(connection);
  #if defined(DEBUG_QOS) && DEBUG_QOS
    printk("QVPT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
  #endif

  #if defined(DEBUG_QOS) && DEBUG_QOS
    printk("set_qsb\n");
    printk("  qsb_clk = %lu\n", (unsigned long)qsb_clk);
    printk("  qsb_queue_parameter_table.bit.tp       = %d\n", (int)qsb_queue_parameter_table.bit.tp);
    printk("  qsb_queue_parameter_table.bit.wfqf     = %d (0x%08X)\n", (int)qsb_queue_parameter_table.bit.wfqf, (int)qsb_queue_parameter_table.bit.wfqf);
    printk("  qsb_queue_parameter_table.bit.vbr      = %d\n", (int)qsb_queue_parameter_table.bit.vbr);
    printk("  qsb_queue_parameter_table.dword        = 0x%08X\n", (int)qsb_queue_parameter_table.dword);
    printk("  qsb_queue_vbr_parameter_table.bit.ts   = %d\n", (int)qsb_queue_vbr_parameter_table.bit.ts);
    printk("  qsb_queue_vbr_parameter_table.bit.taus = %d\n", (int)qsb_queue_vbr_parameter_table.bit.taus);
    printk("  qsb_queue_vbr_parameter_table.dword    = 0x%08X\n", (int)qsb_queue_vbr_parameter_table.dword);
  #endif

#endif  //  !defined(DISABLE_QSB) || !DISABLE_QSB
}

/*
 *  Description:
 *    Add one entry to HTU table.
 *  Input:
 *    vpi        --- unsigned int, virtual path ID
 *    vci        --- unsigned int, virtual channel ID
 *    connection --- unsigned int, connection ID
 *    aal5       --- int, 0 means AAL0, else means AAL5
 *  Output:
 *    none
 */
static INLINE void set_htu_entry(unsigned int vpi, unsigned int vci, unsigned int connection, int aal5)
{
    struct htu_entry htu_entry = {  res1:       0x00,
                                    pid:        ppe_dev.connection[connection].port & 0x01,
                                    vpi:        vpi,
                                    vci:        vci,
                                    pti:        0x00,
                                    vld:        0x01};

    struct htu_mask htu_mask = {    set:        0x03,
                                    pid_mask:   0x02,
                                    vpi_mask:   0x00,
                                    vci_mask:   0x0000,
                                    pti_mask:   0x03,   //  0xx, user data
                                    clear:      0x00};

    struct htu_result htu_result = {res1:       0x00,
                                    cellid:     connection,
                                    res2:       0x00,
                                    type:       aal5 ? 0x00 : 0x01,
                                    ven:        0x01,
                                    res3:       0x00,
                                    qid:        connection};

#if 0
    htu_entry.vld     = 1;
    htu_mask.pid_mask = 0x03;
    htu_mask.vpi_mask = 0xFF;
    htu_mask.vci_mask = 0xFFFF;
    htu_mask.pti_mask = 0x7;
    htu_result.cellid = 1;
    htu_result.type   = 0;
    htu_result.ven    = 0;
    htu_result.qid    = connection;
#endif
    *HTU_RESULT(connection - QSB_QUEUE_NUMBER_BASE + OAM_HTU_ENTRY_NUMBER) = htu_result;
    *HTU_MASK(connection - QSB_QUEUE_NUMBER_BASE + OAM_HTU_ENTRY_NUMBER)   = htu_mask;
    *HTU_ENTRY(connection - QSB_QUEUE_NUMBER_BASE + OAM_HTU_ENTRY_NUMBER)  = htu_entry;

//    printk("Config HTU (%d) for QID %d\n", connection - QSB_QUEUE_NUMBER_BASE + OAM_HTU_ENTRY_NUMBER, connection);
//    printk("  HTU_RESULT = 0x%08X\n", *(u32*)HTU_RESULT(connection - QSB_QUEUE_NUMBER_BASE + OAM_HTU_ENTRY_NUMBER));
//    printk("  HTU_MASK   = 0x%08X\n", *(u32*)HTU_MASK(connection - QSB_QUEUE_NUMBER_BASE + OAM_HTU_ENTRY_NUMBER));
//    printk("  HTU_ENTRY  = 0x%08X\n", *(u32*)HTU_ENTRY(connection - QSB_QUEUE_NUMBER_BASE + OAM_HTU_ENTRY_NUMBER));
}

/*
 *  Description:
 *    Remove one entry from HTU table.
 *  Input:
 *    connection --- unsigned int, connection ID
 *  Output:
 *    none
 */
static INLINE void clear_htu_entry(unsigned int connection)
{
    HTU_ENTRY(connection - QSB_QUEUE_NUMBER_BASE + OAM_HTU_ENTRY_NUMBER)->vld = 0;
}

/*
 *  Description:
 *    Loop up for connection ID with virtual path ID.
 *  Input:
 *    vpi --- unsigned int, virtual path ID
 *  Output:
 *    int --- negative value: failed
 *            else          : connection ID
 */
static INLINE int find_vpi(unsigned int vpi)
{
    int i, j;
    struct connection *connection = ppe_dev.connection;
    struct port *port;
    int base;

    port = ppe_dev.port;
    for ( i = 0; i < ATM_PORT_NUMBER; i++, port++ )
    {
        base = port->connection_base;
        for ( j = 0; j < port->max_connections; j++, base++ )
            if ( (port->connection_table & (1 << j))
                && connection[base].vcc != NULL
                && vpi == connection[base].vcc->vpi )
                return base;
    }
    return -1;
}

/*
 *  Description:
 *    Loop up for connection ID with virtual path ID and virtual channel ID.
 *  Input:
 *    vpi --- unsigned int, virtual path ID
 *    vci --- unsigned int, virtual channel ID
 *  Output:
 *    int --- negative value: failed
 *            else          : connection ID
 */
static INLINE int find_vpivci(unsigned int vpi, unsigned int vci)
{
    int i, j;
    struct connection *connection = ppe_dev.connection;
    struct port *port;
    int base;

    port = ppe_dev.port;
    for ( i = 0; i < ATM_PORT_NUMBER; i++, port++ )
    {
        base = port->connection_base;
        for ( j = 0; j < port->max_connections; j++, base++ )
            if ( (port->connection_table & (1 << j))
                && connection[base].vcc != NULL
                && vpi == connection[base].vcc->vpi
                && vci == connection[base].vcc->vci )
                return base;
    }
    return -1;
}

#if 0
/*
 *  Description:
 *    Loop up for connection ID with virtual path ID, virtual channel ID and port ID.
 *  Input:
 *    vpi  --- unsigned int, virtual path ID
 *    vci  --- unsigned int, virtual channel ID
 *    port --- unsigned int, port ID
 *  Output:
 *    int  --- negative value: failed
 *            else          : connection ID
 */
static INLINE int find_vpivciport(unsigned int vpi, unsigned int vci, unsigned int port)
{
    int i;
    struct connection *connection = ppe_dev.connection;
    int max_connections = ppe_dev.port[port].max_connections;
    u32 occupation_table = ppe_dev.port[port].connection_table;
    int base = ppe_dev.port[port].connection_base;

    for ( i = 0; i < max_connections; i++ )
        if ( (occupation_table & (1 << i))
            && connection[base].vcc != NULL
            && vpi == connection[base].vcc->vpi
            && vci == connection[base].vcc->vci )
            return base;
    return -1;
}
#endif

/*
 *  Description:
 *    Loop up for connection ID with atm_vcc structure.
 *  Input:
 *    vcc --- struct atm_vcc *, atm_vcc structure of opened connection
 *  Output:
 *    int --- negative value: failed
 *            else          : connection ID
 */
static INLINE int find_vcc(struct atm_vcc *vcc)
{
    int i;
    struct connection *connection = ppe_dev.connection;
    int max_connections = ppe_dev.port[(int)vcc->dev->dev_data].max_connections;
    u32 occupation_table = ppe_dev.port[(int)vcc->dev->dev_data].connection_table;
    int base = ppe_dev.port[(int)vcc->dev->dev_data].connection_base;

    for ( i = 0; i < max_connections; i++, base++ )
        if ( (occupation_table & (1 << i))
            && connection[base].vcc == vcc )
            return base;
    return -1;
}

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
    int i;
    int enabled_port_number;
    int unassigned_queue_number;
    int assigned_queue_number;

    enabled_port_number = 0;
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        if ( port_max_connection[i] < 1 )
            port_max_connection[i] = 0;
        else
            enabled_port_number++;
    /*  If the max connection number of a port is not 0, the port is enabled  */
    /*  and at lease two connection ID must be reserved for this port. One of */
    /*  them is used as OAM TX path.                                          */
    unassigned_queue_number = MAX_QUEUE_NUMBER - QSB_QUEUE_NUMBER_BASE;
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        if ( port_max_connection[i] > 0 )
        {
            enabled_port_number--;
            assigned_queue_number = unassigned_queue_number - enabled_port_number * (1 + OAM_TX_QUEUE_NUMBER_PER_PORT) - OAM_TX_QUEUE_NUMBER_PER_PORT;
            if ( assigned_queue_number > MAX_QUEUE_NUMBER_PER_PORT - OAM_TX_QUEUE_NUMBER_PER_PORT )
                assigned_queue_number = MAX_QUEUE_NUMBER_PER_PORT - OAM_TX_QUEUE_NUMBER_PER_PORT;
            if ( port_max_connection[i] > assigned_queue_number )
            {
                port_max_connection[i] = assigned_queue_number;
                unassigned_queue_number -= assigned_queue_number;
            }
            else
                unassigned_queue_number -= port_max_connection[i];
        }

    /*  Please refer to Amazon spec 15.4 for setting these values.  */
    if ( qsb_tau < 1 )
        qsb_tau = 1;
    if ( qsb_tstep < 1 )
        qsb_tstep = 1;
    else if ( qsb_tstep > 4 )
        qsb_tstep = 4;
    else if ( qsb_tstep == 3 )
        qsb_tstep = 2;

    /*  There is a delay between PPE write descriptor and descriptor is       */
    /*  really stored in memory. Host also has this delay when writing        */
    /*  descriptor. So PPE will use this value to determine if the write      */
    /*  operation makes effect.                                               */
    if ( write_descriptor_delay < 0 )
        write_descriptor_delay = 0;

    if ( aal5_fill_pattern < 0 )
        aal5_fill_pattern = 0;
    else
        aal5_fill_pattern &= 0xFF;

    /*  Because of the limitation of length field in descriptors, the packet  */
    /*  size could not be larger than 64K minus overhead size.                */
    if ( aal5r_max_packet_size < 0 )
        aal5r_max_packet_size = 0;
    else if ( aal5r_max_packet_size >= 65536 - MAX_RX_FRAME_EXTRA_BYTES )
        aal5r_max_packet_size = 65536 - MAX_RX_FRAME_EXTRA_BYTES;
    if ( aal5r_min_packet_size < 0 )
        aal5r_min_packet_size = 0;
    else if ( aal5r_min_packet_size > aal5r_max_packet_size )
        aal5r_min_packet_size = aal5r_max_packet_size;
    if ( aal5s_max_packet_size < 0 )
        aal5s_max_packet_size = 0;
    else if ( aal5s_max_packet_size >= 65536 - MAX_TX_FRAME_EXTRA_BYTES )
        aal5s_max_packet_size = 65536 - MAX_TX_FRAME_EXTRA_BYTES;
    if ( aal5s_min_packet_size < 0 )
        aal5s_min_packet_size = 0;
    else if ( aal5s_min_packet_size > aal5s_max_packet_size )
        aal5s_min_packet_size = aal5s_max_packet_size;

#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
    dma_rx_descriptor_length = dma_tx_descriptor_length;
#endif

    if ( dma_rx_descriptor_length < 2 )
        dma_rx_descriptor_length = 2;
    if ( dma_tx_descriptor_length < 2 )
        dma_tx_descriptor_length = 2;
    if ( dma_rx_clp1_descriptor_threshold < 0 )
        dma_rx_clp1_descriptor_threshold = 0;
    else if ( dma_rx_clp1_descriptor_threshold > dma_rx_descriptor_length )
        dma_rx_clp1_descriptor_threshold = dma_rx_descriptor_length;
}

/*
 *  Description:
 *    Setup variable ppe_dev and allocate memory.
 *  Input:
 *    none
 *  Output:
 *    int --- 0:    Success
 *            else: Error Code
 */
static INLINE int init_ppe_dev(void)
{
    int i, j;
    int rx_desc, tx_desc;
    int conn;
    int oam_tx_queue;
#if !defined(ENABLE_RX_QOS) || !ENABLE_RX_QOS
    int rx_dma_channel_base;
    int rx_dma_channel_assigned;
#endif  //  !defined(ENABLE_RX_QOS) || !ENABLE_RX_QOS

    struct rx_descriptor rx_descriptor = {  own:    1,
                                            c:      0,
                                            sop:    1,
                                            eop:    1,
                                            res1:   0,
                                            byteoff:0,
                                            res2:   0,
                                            id:     0,
                                            err:    0,
                                            datalen:0,
                                            res3:   0,
                                            dataptr:0};

    struct tx_descriptor tx_descriptor = {  own:    1,  //  pretend it's hold by PP32
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

    memset(&ppe_dev, 0, sizeof(ppe_dev));

    /*
     *  Setup AAL5 members, buffer size must be larger than max packet size plus overhead.
     */
    ppe_dev.aal5.padding_byte         = (u8)aal5_fill_pattern;
    ppe_dev.aal5.rx_max_packet_size   = (u32)aal5r_max_packet_size;
    ppe_dev.aal5.rx_min_packet_size   = (u32)aal5r_min_packet_size;
    ppe_dev.aal5.rx_buffer_size       = ((u32)(aal5r_max_packet_size > CELL_SIZE ? aal5r_max_packet_size + MAX_RX_FRAME_EXTRA_BYTES : CELL_SIZE + MAX_RX_FRAME_EXTRA_BYTES) + DMA_ALIGNMENT - 1) & ~(DMA_ALIGNMENT - 1);
    ppe_dev.aal5.tx_max_packet_size   = (u32)aal5s_max_packet_size;
    ppe_dev.aal5.tx_min_packet_size   = (u32)aal5s_min_packet_size;
    ppe_dev.aal5.tx_buffer_size       = ((u32)(aal5s_max_packet_size > CELL_SIZE ? aal5s_max_packet_size + MAX_TX_FRAME_EXTRA_BYTES : CELL_SIZE + MAX_TX_FRAME_EXTRA_BYTES) + DMA_ALIGNMENT - 1) & ~(DMA_ALIGNMENT - 1);
    ppe_dev.aal5.rx_drop_error_packet = aal5r_drop_error_packet ? 1 : 0;

    /*
     *  Setup QSB members, please refer to Amazon spec 15.4 to get the value calculation formula.
     */
    ppe_dev.qsb.tau     = (u32)qsb_tau;
    ppe_dev.qsb.tstepc  = (u32)qsb_tstep;
    ppe_dev.qsb.sbl     = (u32)qsb_srvm;

    /*
     *  Setup port, connection, other members.
     */
    conn = 0;
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
    {
        /*  first connection ID of port */
        ppe_dev.port[i].connection_base  = conn + QSB_QUEUE_NUMBER_BASE;
        /*  max number of connections of port   */
        ppe_dev.port[i].max_connections  = (u32)port_max_connection[i];
        /*  max cell rate the port has  */
        ppe_dev.port[i].tx_max_cell_rate = (u32)port_cell_rate_up[i];

        /*  link connection ID to port ID   */
        for ( j = port_max_connection[i] - 1; j >= 0; j-- )
            ppe_dev.connection[conn++ + QSB_QUEUE_NUMBER_BASE].port = i;
    }
    /*  total connection numbers of all ports   */
    ppe_dev.max_connections = conn;
    /*  OAM RX queue ID, which is the first available connection ID after */
    /*  connections assigned to ports.                                    */
    ppe_dev.oam_rx_queue = conn + QSB_QUEUE_NUMBER_BASE;

#if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
    oam_tx_queue = conn;
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        if ( port_max_connection[i] != 0 )
        {
            ppe_dev.port[i].oam_tx_queue = oam_tx_queue + QSB_QUEUE_NUMBER_BASE;

            for ( j = 0; j < OAM_TX_QUEUE_NUMBER_PER_PORT; j++ )
                /*  Since connection ID is one to one mapped to RX/TX queue ID, the connection  */
                /*  structure must be reserved for OAM RX/TX queues, and member "port" is set   */
                /*  according to port to which OAM TX queue is connected.                       */
                ppe_dev.connection[oam_tx_queue++ + QSB_QUEUE_NUMBER_BASE].port = i;
        }
    /*  DMA RX channel assigned to OAM RX queue */
    ppe_dev.oam_rx_dma_channel = RX_DMA_CH_OAM;
    /*  DMA RX channel will be assigned dynamically when VCC is open.   */
#else   //  defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
    rx_dma_channel_base = 0;
    oam_tx_queue = conn;
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        if ( port_max_connection[i] != 0 )
        {
            /*  Calculate the number of DMA RX channels could be assigned to port.  */
            rx_dma_channel_assigned = i == ATM_PORT_NUMBER - 1
                                      ? (MAX_RX_DMA_CHANNEL_NUMBER - OAM_RX_DMA_CHANNEL_NUMBER) - rx_dma_channel_base
                                      : (ppe_dev.port[i].max_connections * (MAX_RX_DMA_CHANNEL_NUMBER - OAM_RX_DMA_CHANNEL_NUMBER) + ppe_dev.max_connections / 2) / ppe_dev.max_connections;
            /*  Amend the number, which could be zero.  */
            if ( rx_dma_channel_assigned == 0 )
                rx_dma_channel_assigned = 1;
            /*  Calculate the first DMA RX channel ID could be assigned to port.    */
            if ( rx_dma_channel_base + rx_dma_channel_assigned > MAX_RX_DMA_CHANNEL_NUMBER - OAM_RX_DMA_CHANNEL_NUMBER )
                rx_dma_channel_base = MAX_RX_DMA_CHANNEL_NUMBER - OAM_RX_DMA_CHANNEL_NUMBER - rx_dma_channel_assigned;

            /*  first DMA RX channel ID */
            ppe_dev.port[i].rx_dma_channel_base     = rx_dma_channel_base;
            /*  number of DMA RX channels assigned to this port */
            ppe_dev.port[i].rx_dma_channel_assigned = rx_dma_channel_assigned;
            /*  OAM TX queue ID, which must be assigned after connections assigned to ports */
            ppe_dev.port[i].oam_tx_queue            = oam_tx_queue + QSB_QUEUE_NUMBER_BASE;

            rx_dma_channel_base += rx_dma_channel_assigned;

            for ( j = 0; j < OAM_TX_QUEUE_NUMBER_PER_PORT; j++ )
                /*  Since connection ID is one to one mapped to RX/TX queue ID, the connection  */
                /*  structure must be reserved for OAM RX/TX queues, and member "port" is set   */
                /*  according to port to which OAM TX queue is connected.                       */
                ppe_dev.connection[oam_tx_queue++ + QSB_QUEUE_NUMBER_BASE].port = i;
        }
    /*  DMA RX channel assigned to OAM RX queue */
    ppe_dev.oam_rx_dma_channel = rx_dma_channel_base;

    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
       for ( j = 0; j < port_max_connection[i]; j++ )
            /*  Assign DMA RX channel to RX queues. One channel could be assigned to more than one queue.   */
            ppe_dev.connection[ppe_dev.port[i].connection_base + j].rx_dma_channel = ppe_dev.port[i].rx_dma_channel_base + j % ppe_dev.port[i].rx_dma_channel_assigned;
#endif  //  defined(ENABLE_RX_QOS) && ENABLE_RX_QOS

    /*  initialize semaphore used by open and close */
    sema_init(&ppe_dev.sem, 1);

    /*
     *  dma
     */
    /*  descriptor number of RX DMA channel */
    ppe_dev.dma.rx_descriptor_number         = dma_rx_descriptor_length;
    /*  descriptor number of TX DMA channel */
    ppe_dev.dma.tx_descriptor_number         = dma_tx_descriptor_length;
    /*  If used descriptors are more than this value, cell with CLP1 is dropped.    */
    ppe_dev.dma.rx_clp1_desc_threshold = dma_rx_clp1_descriptor_threshold;

    /*  delay on descriptor write path  */
    ppe_dev.dma.write_descriptor_delay       = write_descriptor_delay;

    /*  total DMA RX channel used   */
#if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
    ppe_dev.dma.rx_total_channel_used = RX_DMA_CH_TOTAL;
#else
    ppe_dev.dma.rx_total_channel_used = rx_dma_channel_base + OAM_RX_DMA_CHANNEL_NUMBER;
#endif  //  defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
    /*  total DMA TX channel used (exclude channel reserved by QSB) */
    ppe_dev.dma.tx_total_channel_used = oam_tx_queue;

    /*  allocate memory for RX descriptors  */
    ppe_dev.dma.rx_descriptor_addr = kmalloc(ppe_dev.dma.rx_total_channel_used * ppe_dev.dma.rx_descriptor_number * sizeof(struct rx_descriptor) + 4, GFP_KERNEL | GFP_DMA);
    if ( !ppe_dev.dma.rx_descriptor_addr )
        goto RX_DESCRIPTOR_BASE_ALLOCATE_FAIL;
    /*  do alignment (DWORD)    */
    ppe_dev.dma.rx_descriptor_base = (struct rx_descriptor *)(((u32)ppe_dev.dma.rx_descriptor_addr + 0x03) & ~0x03);
    ppe_dev.dma.rx_descriptor_base = (struct rx_descriptor *)((u32)ppe_dev.dma.rx_descriptor_base | KSEG1);    //  no cache
    /*  allocate pointers to RX sk_buff */
//    ppe_dev.dma.rx_skb_pointers = kmalloc(ppe_dev.dma.rx_total_channel_used * ppe_dev.dma.rx_descriptor_number * sizeof(struct sk_buff *), GFP_KERNEL);
//    if ( !ppe_dev.dma.rx_skb_pointers )
//        goto RX_SKB_POINTER_ALLOCATE_FAIL;

    /*  allocate memory for TX descriptors  */
    ppe_dev.dma.tx_descriptor_addr = kmalloc(ppe_dev.dma.tx_total_channel_used * ppe_dev.dma.tx_descriptor_number * sizeof(struct tx_descriptor) + 4, GFP_KERNEL | GFP_DMA);
    if ( !ppe_dev.dma.tx_descriptor_addr )
        goto TX_DESCRIPTOR_BASE_ALLOCATE_FAIL;
    /*  do alignment (DWORD)    */
    ppe_dev.dma.tx_descriptor_base = (struct tx_descriptor *)(((u32)ppe_dev.dma.tx_descriptor_addr + 0x03) & ~0x03);
    ppe_dev.dma.tx_descriptor_base = (struct tx_descriptor *)((u32)ppe_dev.dma.tx_descriptor_base | KSEG1);    //  no cache
    /*  allocate pointers to TX sk_buff */
    ppe_dev.dma.tx_skb_pointers = kmalloc(ppe_dev.dma.tx_total_channel_used * ppe_dev.dma.tx_descriptor_number * sizeof(struct sk_buff *), GFP_KERNEL);
    if ( !ppe_dev.dma.tx_skb_pointers )
        goto TX_SKB_POINTER_ALLOCATE_FAIL;
    memset(ppe_dev.dma.tx_skb_pointers, 0, ppe_dev.dma.tx_total_channel_used * ppe_dev.dma.tx_descriptor_number * sizeof(struct sk_buff *));

    /*  Allocate RX sk_buff and fill up RX descriptors. */
    rx_descriptor.datalen = ppe_dev.aal5.rx_buffer_size;
    for ( rx_desc = ppe_dev.dma.rx_total_channel_used * ppe_dev.dma.rx_descriptor_number - 1; rx_desc >= 0; rx_desc-- )
    {
        struct sk_buff *skb;

//        ppe_dev.dma.rx_skb_pointers[rx_desc] = alloc_skb_rx();
//        if ( !ppe_dev.dma.rx_skb_pointers[rx_desc] )
//            goto ALLOC_SKB_RX_FAIL;
//        rx_descriptor.dataptr = (u32)ppe_dev.dma.rx_skb_pointers[rx_desc]->data >> 2;
        skb = alloc_skb_rx();
        if ( skb == NULL )
            panic("sk buffer is used up\n");
        rx_descriptor.dataptr = (u32)skb->data >> 2;
        ppe_dev.dma.rx_descriptor_base[rx_desc] = rx_descriptor;

#if 0
        if ( rx_descriptor.dataptr == 0x000C4804 )
        {
            printk("rx_desc = %d\n", rx_desc);
            printk("desc (0x%08X) = 0x%08X 0x%08X\n", (u32)&ppe_dev.dma.rx_descriptor_base[rx_desc], *(u32*)&ppe_dev.dma.rx_descriptor_base[rx_desc], *((u32*)&ppe_dev.dma.rx_descriptor_base[rx_desc] + 1));
            printk("skb (0x%08X)\n", (u32)ppe_dev.dma.rx_skb_pointers[rx_desc]);
            printk("skb->data = 0x%08X, desc->dataptr = 0x%08X\n", (u32)ppe_dev.dma.rx_skb_pointers[rx_desc]->data, (u32)ppe_dev.dma.rx_descriptor_base[rx_desc].dataptr << 2);
        }

        if ( rx_desc == 192 )
        {
            printk("rx_desc = %d\n", rx_desc);
            printk("desc (0x%08X) = 0x%08X 0x%08X\n", (u32)&ppe_dev.dma.rx_descriptor_base[rx_desc], *(u32*)&ppe_dev.dma.rx_descriptor_base[rx_desc], *((u32*)&ppe_dev.dma.rx_descriptor_base[rx_desc] + 1));
            printk("skb (0x%08X)\n", (u32)ppe_dev.dma.rx_skb_pointers[rx_desc]);
            printk("skb->data = 0x%08X, desc->dataptr = 0x%08X\n", (u32)ppe_dev.dma.rx_skb_pointers[rx_desc]->data, (u32)ppe_dev.dma.rx_descriptor_base[rx_desc].dataptr << 2);
        }
#endif
    }

    /*  Fill up TX descriptors. */
    tx_descriptor.datalen = ppe_dev.aal5.tx_buffer_size;
    for ( tx_desc = ppe_dev.dma.tx_total_channel_used * ppe_dev.dma.tx_descriptor_number - 1; tx_desc >= 0; tx_desc-- )
        ppe_dev.dma.tx_descriptor_base[tx_desc] = tx_descriptor;

//    dbg("init_tx_tables succeed");

#if 0
    {
        int i;

        printk("ppe_dev\n");
        printk("  connection\n");
        for ( i = 0; i < MAX_CONNECTION_NUMBER; i++ )
        {
            printk("  [%d]\n", i);
            printk("    rx dma ch: %d\n", ppe_dev.connection[i].rx_dma_channel);
            printk("    port:      %d\n", ppe_dev.connection[i].port);
        }
        printk("  port\n");
        for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        {
            printk("  [%d]\n", i);
            printk("    conn base:     %d\n", ppe_dev.port[i].connection_base);
            printk("    max conn:      %d\n", ppe_dev.port[i].max_connections);
            printk("    conn table:    %d\n", ppe_dev.port[i].connection_table);
            printk("    max cell rate: %d\n", ppe_dev.port[i].tx_max_cell_rate);
            printk("    cur cell rate: %d\n", ppe_dev.port[i].tx_current_cell_rate);
  #if !defined(ENABLE_RX_QOS) || !ENABLE_RX_QOS
            printk("    rx dma ch bas: %d\n", ppe_dev.port[i].rx_dma_channel_base);
            printk("    rx dma ch ass: %d\n", ppe_dev.port[i].rx_dma_channel_assigned);
  #endif
            printk("    oam tx queue:  %d\n", ppe_dev.port[i].oam_tx_queue);
            printk("    dev pointer:   0x%08X\n", (u32)ppe_dev.port[i].dev);
        }
        printk("  aal5\n");
        printk("    padding: %02X\n", ppe_dev.aal5.padding_byte);
        printk("    rx max:  %d\n", ppe_dev.aal5.rx_max_packet_size);
        printk("    rx min:  %d\n", ppe_dev.aal5.rx_min_packet_size);
        printk("    rx buf:  %d\n", ppe_dev.aal5.rx_buffer_size);
        printk("    tx max:  %d\n", ppe_dev.aal5.tx_max_packet_size);
        printk("    tx min:  %d\n", ppe_dev.aal5.tx_min_packet_size);
        printk("    tx buf:  %d\n", ppe_dev.aal5.tx_buffer_size);
        printk("    rx drop: %d\n", ppe_dev.aal5.rx_drop_error_packet);
        printk("  qsb\n");
        printk("    tau:    %d\n", ppe_dev.qsb.tau);
        printk("    tstepc: %d\n", ppe_dev.qsb.tstepc);
        printk("    sbl:    %d\n", ppe_dev.qsb.sbl);
        printk("  dma\n");
        printk("    rx desc num:   %d\n", ppe_dev.dma.rx_descriptor_number);
        printk("    tx desc num:   %d\n", ppe_dev.dma.tx_descriptor_number);
        printk("    rx desc clp1:  %d\n", ppe_dev.dma.rx_clp1_desc_threshold);
        printk("    wr desc delay: %d\n", ppe_dev.dma.write_descriptor_delay);
        printk("    rx total ch:   %d\n", ppe_dev.dma.rx_total_channel_used);
        printk("    rx desc addr:  0x%08X\n", (u32)ppe_dev.dma.rx_descriptor_addr);
        printk("    rx desc base:  0x%08X\n", (u32)ppe_dev.dma.rx_descriptor_base);
//        printk("    rx skb base:   0x%08X\n", (u32)ppe_dev.dma.rx_skb_pointers);
        printk("    tx total ch:   %d\n", ppe_dev.dma.tx_total_channel_used);
        printk("    tx desc addr:  0x%08X\n", (u32)ppe_dev.dma.tx_descriptor_addr);
        printk("    tx desc base:  0x%08X\n", (u32)ppe_dev.dma.tx_descriptor_base);
        printk("    tx skb base:   0x%08X\n", (u32)ppe_dev.dma.tx_skb_pointers);
        printk("  oam rx queue:  %d\n", ppe_dev.oam_rx_queue);
        printk("  oam rx dma ch: %d\n", ppe_dev.oam_rx_dma_channel);
        printk("  max conn:      %d\n", ppe_dev.max_connections);
    }
#endif
    return 0;

//ALLOC_SKB_RX_FAIL:
//    for ( rx_desc++; rx_desc < ppe_dev.dma.rx_total_channel_used * ppe_dev.dma.rx_descriptor_number; rx_desc++ )
//        dev_kfree_skb_any(ppe_dev.dma.rx_skb_pointers[rx_desc]);
//    kfree(ppe_dev.dma.tx_skb_pointers);
TX_SKB_POINTER_ALLOCATE_FAIL:
    kfree(ppe_dev.dma.tx_descriptor_addr);
TX_DESCRIPTOR_BASE_ALLOCATE_FAIL:
//    kfree(ppe_dev.dma.rx_skb_pointers);
//RX_SKB_POINTER_ALLOCATE_FAIL:
    kfree(ppe_dev.dma.rx_descriptor_addr);
RX_DESCRIPTOR_BASE_ALLOCATE_FAIL:
//    dbg("init_tx_tables fail");
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
 *    Setup RX relative registers and tables, including HTU table. All
 *    parameters are taken from ppe_dev.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void init_rx_tables(void)
{
    int i, j;
    struct wrx_queue_config wrx_queue_config = {0};
    struct wrx_dma_channel_config wrx_dma_channel_config = {0};
    struct htu_entry htu_entry = {0};
    struct htu_result htu_result = {0};

    struct htu_mask htu_mask = {    set:        0x03,
                                    pid_mask:   0x00,
                                    vpi_mask:   0x00,
                                    vci_mask:   0x00,
                                    pti_mask:   0x00,
                                    clear:      0x00};

    /*
     *  General Registers
     */
    *CFG_WRX_HTUTS  = ppe_dev.max_connections + OAM_HTU_ENTRY_NUMBER;
    *CFG_WRX_QNUM   = ppe_dev.max_connections + OAM_RX_QUEUE_NUMBER + QSB_QUEUE_NUMBER_BASE;
    *CFG_WRX_DCHNUM = ppe_dev.dma.rx_total_channel_used;
    *WRX_DMACH_ON   = (1 << ppe_dev.dma.rx_total_channel_used) - 1;
    *WRX_HUNT_BITTH = DEFAULT_RX_HUNT_BITTH;

    /*
     *  WRX Queue Configuration Table
     */
    wrx_queue_config.uumask    = 0;
    wrx_queue_config.cpimask   = 0;
    wrx_queue_config.uuexp     = 0;
    wrx_queue_config.cpiexp    = 0;
    wrx_queue_config.mfs       = ppe_dev.aal5.rx_max_packet_size;   // rx_buffer_size
    wrx_queue_config.oversize  = ppe_dev.aal5.rx_max_packet_size;
    wrx_queue_config.undersize = ppe_dev.aal5.rx_min_packet_size;
    wrx_queue_config.errdp     = ppe_dev.aal5.rx_drop_error_packet;
    for ( i = 0; i < QSB_QUEUE_NUMBER_BASE; i++ )
        *WRX_QUEUE_CONFIG(i) = wrx_queue_config;
    for ( j = 0; j < ppe_dev.max_connections; j++ )
    {
#if !defined(ENABLE_RX_QOS) || !ENABLE_RX_QOS
        /*  If RX QoS is disabled, the DMA channel must be fixed.   */
        wrx_queue_config.dmach = ppe_dev.connection[i].rx_dma_channel;
#endif  //  !defined(ENABLE_RX_QOS) || !ENABLE_RX_QOS
        *WRX_QUEUE_CONFIG(i++) = wrx_queue_config;
    }
    /*  OAM RX Queue    */
    for ( j = 0; j < OAM_RX_DMA_CHANNEL_NUMBER; j++ )
    {
#if defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
        wrx_queue_config.dmach = RX_DMA_CH_OAM;
#else
        wrx_queue_config.dmach = ppe_dev.oam_rx_dma_channel + j;
#endif  //  defined(ENABLE_RX_QOS) && ENABLE_RX_QOS
        *WRX_QUEUE_CONFIG(i++) = wrx_queue_config;
    }

    /*
     *  WRX DMA Channel Configuration Table
     */
    wrx_dma_channel_config.deslen = ppe_dev.dma.rx_descriptor_number;
//    wrx_dma_channel_config.chrl   = ppe_dev.aal5.rx_buffer_size;
    wrx_dma_channel_config.chrl   = 0;
    wrx_dma_channel_config.clp1th = ppe_dev.dma.rx_clp1_desc_threshold;
    wrx_dma_channel_config.mode   = WRX_DMA_CHANNEL_COUNTER_MODE;
    wrx_dma_channel_config.rlcfg  = WRX_DMA_BUF_LEN_PER_DESCRIPTOR;
    for ( i = 0; i < ppe_dev.dma.rx_total_channel_used; i++ )
    {
        wrx_dma_channel_config.desba = (((u32)ppe_dev.dma.rx_descriptor_base >> 2) & 0x0FFFFFFF) + ppe_dev.dma.rx_descriptor_number * i * (sizeof(struct rx_descriptor) >> 2);
        *WRX_DMA_CHANNEL_CONFIG(i) = wrx_dma_channel_config;

//        dbg("i = %d, desba = 0x%08X", i, (u32)wrx_dma_channel_config.desba << 2);
//        dbg("i = %d, desba = 0x%08X", i, (u32)(wrx_dma_channel_config.desba << 2));
    }

    /*
     *  HTU Tables
     */
    for ( i = 0; i < ppe_dev.max_connections; i++ )
    {
        htu_result.qid = (unsigned int)i;

        *HTU_ENTRY(i + OAM_HTU_ENTRY_NUMBER)  = htu_entry;
        *HTU_MASK(i + OAM_HTU_ENTRY_NUMBER)   = htu_mask;
        *HTU_RESULT(i + OAM_HTU_ENTRY_NUMBER) = htu_result;
    }
    /*  OAM HTU Entry   */
    htu_entry.vci     = 0x03;
    htu_mask.pid_mask = 0x03;
    htu_mask.vpi_mask = 0xFF;
    htu_mask.vci_mask = 0x0000;
    htu_mask.pti_mask = 0x07;
    htu_result.cellid = ppe_dev.oam_rx_queue;
    htu_result.type   = 1;
    htu_result.ven    = 1;
    htu_result.qid    = ppe_dev.oam_rx_queue;
    *HTU_RESULT(OAM_F4_SEG_HTU_ENTRY) = htu_result;
    *HTU_MASK(OAM_F4_SEG_HTU_ENTRY)   = htu_mask;
    *HTU_ENTRY(OAM_F4_SEG_HTU_ENTRY)  = htu_entry;
    htu_entry.vci     = 0x04;
    htu_result.cellid = ppe_dev.oam_rx_queue;
    htu_result.type   = 1;
    htu_result.ven    = 1;
    htu_result.qid    = ppe_dev.oam_rx_queue;
    *HTU_RESULT(OAM_F4_TOT_HTU_ENTRY) = htu_result;
    *HTU_MASK(OAM_F4_TOT_HTU_ENTRY)   = htu_mask;
    *HTU_ENTRY(OAM_F4_TOT_HTU_ENTRY)  = htu_entry;
    htu_entry.vci     = 0x00;
    htu_entry.pti     = 0x04;
    htu_mask.vci_mask = 0xFFFF;
    htu_mask.pti_mask = 0x01;
    htu_result.cellid = ppe_dev.oam_rx_queue;
    htu_result.type   = 1;
    htu_result.ven    = 1;
    htu_result.qid    = ppe_dev.oam_rx_queue;
    *HTU_RESULT(OAM_F5_HTU_ENTRY) = htu_result;
    *HTU_MASK(OAM_F5_HTU_ENTRY)   = htu_mask;
    *HTU_ENTRY(OAM_F5_HTU_ENTRY)  = htu_entry;
}

/*
 *  Description:
 *    Setup TX relative registers and tables. All parameters are taken from
 *    ppe_dev.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void init_tx_tables(void)
{
    int i, j;
    struct wtx_queue_config wtx_queue_config = {0};
    struct wtx_dma_channel_config wtx_dma_channel_config = {0};

    struct wtx_port_config wtx_port_config = {  res1:   0,
                                                qid:    0,
                                                qsben:  1};

    /*
     *  General Registers
     */
    *CFG_WTX_DCHNUM     = ppe_dev.dma.tx_total_channel_used + QSB_QUEUE_NUMBER_BASE;
    *WTX_DMACH_ON       = ((1 << (ppe_dev.dma.tx_total_channel_used + QSB_QUEUE_NUMBER_BASE)) - 1) ^ ((1 << QSB_QUEUE_NUMBER_BASE) - 1);
    *CFG_WRDES_DELAY    = ppe_dev.dma.write_descriptor_delay;

    /*
     *  WTX Port Configuration Table
     */
#if !defined(DISABLE_QSB) || !DISABLE_QSB
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        *WTX_PORT_CONFIG(i) = wtx_port_config;
#else
    wtx_port_config.qsben = 0;
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
    {
        wtx_port_config.qid = ppe_dev.port[i].connection_base;
        *WTX_PORT_CONFIG(i) = wtx_port_config;

//        printk("port %d: qid = %d, qsb disabled\n", i, wtx_port_config.qid);
    }
#endif

    /*
     *  WTX Queue Configuration Table
     */
    wtx_queue_config.res1  = 0;
    wtx_queue_config.res2  = 0;
//    wtx_queue_config.type  = 0x03;
    wtx_queue_config.type  = 0x0;
#if !defined(DISABLE_QSB) || !DISABLE_QSB
    wtx_queue_config.qsben = 1;
#else
    wtx_queue_config.qsben = 0;
#endif
    wtx_queue_config.sbid  = 0;
    for ( i = 0; i < QSB_QUEUE_NUMBER_BASE; i++ )
        *WTX_QUEUE_CONFIG(i) = wtx_queue_config;
    for ( j = 0; j < ppe_dev.max_connections; j++ )
    {
        wtx_queue_config.sbid = ppe_dev.connection[i].port & 0x01;  /*  assign QSB to TX queue  */
        *WTX_QUEUE_CONFIG(i) = wtx_queue_config;
        i++;
    }
    /*  OAM TX Queue    */
//    wtx_queue_config.type = 0x01;
    wtx_queue_config.type  = 0x00;
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
    {
        wtx_queue_config.sbid = i & 0x01;
        for ( j = 0; j < OAM_TX_QUEUE_NUMBER_PER_PORT; j++ )
            *WTX_QUEUE_CONFIG(ppe_dev.port[i].oam_tx_queue + j) = wtx_queue_config;
    }

    /*
     *  WTX DMA Channel Configuration Table
     */
    wtx_dma_channel_config.mode   = WRX_DMA_CHANNEL_COUNTER_MODE;
    wtx_dma_channel_config.deslen = 0;
    wtx_dma_channel_config.desba = 0;
    for ( i = 0; i < QSB_QUEUE_NUMBER_BASE; i++ )
        *WTX_DMA_CHANNEL_CONFIG(i) = wtx_dma_channel_config;
    /*  normal connection and OAM channel   */
    wtx_dma_channel_config.deslen = ppe_dev.dma.tx_descriptor_number;
    for ( j = 0; j < ppe_dev.dma.tx_total_channel_used; j++ )
    {
        wtx_dma_channel_config.desba = (((u32)ppe_dev.dma.tx_descriptor_base >> 2) & 0x0FFFFFFF) + ppe_dev.dma.tx_descriptor_number * j * (sizeof(struct tx_descriptor) >> 2);
        *WTX_DMA_CHANNEL_CONFIG(i++) = wtx_dma_channel_config;

//        dbg("i = %d, desba = 0x%08X", i - 1, (u32)wtx_dma_channel_config.desba << 2);
//        dbg("i = %d, desba = 0x%08X", i - 1, (u32)(wtx_dma_channel_config.desba << 2));
    }
}

/*
 *  Description:
 *    Clean-up ppe_dev and release memory.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void clear_ppe_dev(void)
{
    int i;

    /*
     *  free memory allocated for RX/TX descriptors and RX sk_buff
     */
    for ( i = 0; i < ppe_dev.dma.tx_total_channel_used; i++ )
    {
        int conn = i + QSB_QUEUE_NUMBER_BASE;
        int desc_base;
        struct sk_buff *skb;

        while ( ppe_dev.dma.tx_desc_release_pos[conn] != ppe_dev.dma.tx_desc_alloc_pos[conn] )
        {
            desc_base = ppe_dev.dma.tx_descriptor_number * (conn - QSB_QUEUE_NUMBER_BASE) + ppe_dev.dma.tx_desc_release_pos[conn];
            if ( !ppe_dev.dma.tx_descriptor_base[desc_base].own )
            {
                skb = ppe_dev.dma.tx_skb_pointers[desc_base];
                atm_free_tx_skb_vcc(skb);

                ppe_dev.dma.tx_descriptor_base[desc_base].own = 1;  //  pretend PP32 hold owner bit, so that won't be released more than once, so allocation process don't check this bit
            }
            if ( ++ppe_dev.dma.tx_desc_release_pos[conn] == ppe_dev.dma.tx_descriptor_number )
                ppe_dev.dma.tx_desc_release_pos[conn] = 0;
        }
//        while ( ppe_dev.dma.tx_desc_alloc_num[conn] > 0 )
//        {
//            --ppe_dev.dma.tx_desc_alloc_num[conn];
//            if ( ppe_dev.dma.tx_desc_alloc_pos[conn]-- == 0 )
//                ppe_dev.dma.tx_desc_alloc_pos[conn] = ppe_dev.dma.tx_descriptor_number - 1;
//            desc_base = ppe_dev.dma.tx_desc_alloc_pos[conn];
//            skb = ppe_dev.dma.tx_skb_pointers[desc_base];
//            atm_free_tx_skb_vcc(skb);
//        }
    }

//    for ( i = ppe_dev.dma.rx_total_channel_used * ppe_dev.dma.rx_descriptor_number - 1; i >= 0; i-- )
//        dev_kfree_skb_any(ppe_dev.dma.rx_skb_pointers[i]);
    for ( i = ppe_dev.dma.rx_total_channel_used * ppe_dev.dma.rx_descriptor_number - 1; i >= 0; i-- )
        dev_kfree_skb_any(*(struct sk_buff **)(((ppe_dev.dma.rx_descriptor_base[i].dataptr << 2) | KSEG0) - 4));

    kfree(ppe_dev.dma.tx_skb_pointers);
    kfree(ppe_dev.dma.tx_descriptor_addr);
//    kfree(ppe_dev.dma.rx_skb_pointers);
    kfree(ppe_dev.dma.rx_descriptor_addr);
}

static INLINE void init_ema(void)
{
    *EMA_CMDCFG  = (EMA_CMD_BUF_LEN << 16) | (EMA_CMD_BASE_ADDR >> 2);
    *EMA_DATACFG = (EMA_DATA_BUF_LEN << 16) | (EMA_DATA_BASE_ADDR >> 2);
    *EMA_IER     = 0x000000FF;
	*EMA_CFG     = EMA_READ_BURST | (EMA_WRITE_BURST << 2);
}

static INLINE void init_chip(void)
{
#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    /*  enable PPE module in PMU    */
    *(unsigned long *)0xBF10201C &= ~((1 << 15) | (1 << 13) | (1 << 9));
#endif

    /*  init EMA    */
    init_ema();

    /*  enable mailbox  */
    *MBOX_IGU1_ISRC = 0xFFFFFFFF;
//    *MBOX_IGU1_IER  = 0xFFFFFFFF;
    *MBOX_IGU1_IER  = 0x00000000;
    *MBOX_IGU3_ISRC = 0xFFFFFFFF;
//    *MBOX_IGU3_IER  = 0xFFFFFFFF;
    *MBOX_IGU3_IER  = 0x00000000;

#if defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
    *MBOX_IGU0_ISRC = 0x00;
    *MBOX_IGU0_IER  = 0x03;
#endif  //  defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
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

/*
 *  Description:
 *    Setup QSB.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void qsb_global_set(void)
{
#if !defined(DISABLE_QSB) || !DISABLE_QSB

    int i, j;
  #if defined(QSB_IS_HALF_FPI2_CLOCK) && QSB_IS_HALF_FPI2_CLOCK
    u32 qsb_clk = cgu_get_fpi_bus_clock(2) >> 1;    /*  Half of FPI configuration 2 (slow FPI bus) */
  #else
    u32 qsb_clk = cgu_get_fpi_bus_clock(2);         /*  FPI configuration 2 (slow FPI bus)  */
  #endif  //  defined(QSB_IS_HALF_FPI2_CLOCK) && QSB_IS_HALF_FPI2_CLOCK
    u32 tmp1, tmp2, tmp3;
    union qsb_queue_parameter_table qsb_queue_parameter_table = {{0}};
    union qsb_queue_vbr_parameter_table qsb_queue_vbr_parameter_table = {{0}};
    int qsb_qid;

    *QSB_ICDV = QSB_ICDV_TAU_SET(ppe_dev.qsb.tau);
    *QSB_SBL  = QSB_SBL_SBL_SET(ppe_dev.qsb.sbl);
    *QSB_CFG  = QSB_CFG_TSTEPC_SET(ppe_dev.qsb.tstepc >> 1);
  #if defined(DEBUG_QOS) && DEBUG_QOS
    printk("QSB_ICDV (%08X) = %d (%d), QSB_SBL (%08X) = %d (%d), QSB_CFG (%08X) = %d (%d)\n", (u32)QSB_ICDV, *QSB_ICDV, QSB_ICDV_TAU_SET(ppe_dev.qsb.tau), (u32)QSB_SBL, *QSB_SBL, QSB_SBL_SBL_SET(ppe_dev.qsb.sbl), (u32)QSB_CFG, *QSB_CFG, QSB_CFG_TSTEPC_SET(ppe_dev.qsb.tstepc >> 1));
  #endif

    /*
     *  set SCT and SPT per port
     */
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        if ( ppe_dev.port[i].max_connections != 0 && ppe_dev.port[i].tx_max_cell_rate != 0 )
        {
            tmp1 = ((qsb_clk * ppe_dev.qsb.tstepc) >> 1) / ppe_dev.port[i].tx_max_cell_rate;
            tmp2 = tmp1 >> 6;                   /*  integer value of Tsb    */
            tmp3 = (tmp1 & ((1 << 6) - 1)) + 1; /*  fractional part of Tsb  */
            /*  carry over to integer part (?)  */
            if ( tmp3 == (1 << 6) )
            {
                tmp3 = 0;
                tmp2++;
            }
            if ( tmp2 == 0 )
                tmp2 = tmp3 = 1;
            /*  1. set mask                                 */
            /*  2. write value to data transfer register    */
            /*  3. start the tranfer                        */
            /*  SCT (FracRate)  */
            *QSB_RTM   = QSB_RTM_DM_SET(QSB_SET_SCT_MASK);
            *QSB_RTD   = QSB_RTD_TTV_SET(tmp3);
            *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_SCT) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(i & 0x01);
  #if defined(DEBUG_QOS) && DEBUG_QOS
            printk("SCT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
  #endif
            /*  SPT (SBV + PN + IntRage)    */
            *QSB_RTM   = QSB_RTM_DM_SET(QSB_SET_SPT_MASK);
            *QSB_RTD   = QSB_RTD_TTV_SET(QSB_SPT_SBV_VALID | QSB_SPT_PN_SET(i & 0x01) | QSB_SPT_INTRATE_SET(tmp2));
            *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_SPT) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(i & 0x01);
  #if defined(DEBUG_QOS) && DEBUG_QOS
            printk("SPT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
  #endif
        }

    /*
     *  set OAM TX queue
     */
    for ( i = 0; i < ATM_PORT_NUMBER; i++ )
        if ( ppe_dev.port[i].max_connections != 0 )
            for ( j = 0; j < OAM_TX_QUEUE_NUMBER_PER_PORT; j++ )
            {
                qsb_qid = ppe_dev.port[i].oam_tx_queue + j;

                /*  disable PCR limiter */
                qsb_queue_parameter_table.bit.tp = 0;
                /*  set WFQ as real time queue  */
                qsb_queue_parameter_table.bit.wfqf = 0;
                /*  disable leaky bucket shaper */
                qsb_queue_vbr_parameter_table.bit.taus = 0;
                qsb_queue_vbr_parameter_table.bit.ts = 0;

                /*  Queue Parameter Table (QPT) */
                *QSB_RTM   = QSB_RTM_DM_SET(QSB_QPT_SET_MASK);
                *QSB_RTD   = QSB_RTD_TTV_SET(qsb_queue_parameter_table.dword);
                *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_QPT) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(qsb_qid);
  #if defined(DEBUG_QOS) && DEBUG_QOS
                printk("QPT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
  #endif
                /*  Queue VBR Paramter Table (QVPT) */
                *QSB_RTM   = QSB_RTM_DM_SET(QSB_QVPT_SET_MASK);
                *QSB_RTD   = QSB_RTD_TTV_SET(qsb_queue_vbr_parameter_table.dword);
                *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_VBR) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(qsb_qid);
  #if defined(DEBUG_QOS) && DEBUG_QOS
                printk("QVPT: QSB_RTM (%08X) = 0x%08X, QSB_RTD (%08X) = 0x%08X, QSB_RAMAC (%08X) = 0x%08X\n", (u32)QSB_RTM, *QSB_RTM, (u32)QSB_RTD, *QSB_RTD, (u32)QSB_RAMAC, *QSB_RAMAC);
  #endif
            }

#endif  //  !defined(DISABLE_QSB) || !DISABLE_QSB
}

/*
 *  Description:
 *    Add HTU entries to capture OAM cell.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void validate_oam_htu_entry(void)
{
    HTU_ENTRY(OAM_F4_SEG_HTU_ENTRY)->vld = 1;
    HTU_ENTRY(OAM_F4_TOT_HTU_ENTRY)->vld = 1;
    HTU_ENTRY(OAM_F5_HTU_ENTRY)->vld = 1;
}

/*
 *  Description:
 *    Remove HTU entries which are used to capture OAM cell.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void invalidate_oam_htu_entry(void)
{
    register int i;

    HTU_ENTRY(OAM_F4_SEG_HTU_ENTRY)->vld = 0;
    HTU_ENTRY(OAM_F4_TOT_HTU_ENTRY)->vld = 0;
    HTU_ENTRY(OAM_F5_HTU_ENTRY)->vld = 0;
    /*  idle for a while to finish running HTU search   */
    for ( i = 0; i < IDLE_CYCLE_NUMBER; i++ );
}

static INLINE void proc_file_create(void)
{
    g_ppe_proc_dir = proc_mkdir("ppe", NULL);

	create_proc_read_entry("idle_counter",
                           0,
                           g_ppe_proc_dir,
                           proc_read_idle_counter,
                           NULL);

    create_proc_read_entry("stats",
                           0,
                           g_ppe_proc_dir,
                           proc_read_stats,
                           NULL);

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
    create_proc_read_entry("rate",
                           0,
                           g_ppe_proc_dir,
                           proc_read_rate,
                           NULL);
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

#if defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
    create_proc_read_entry("mbox0",
                           0,
                           g_ppe_proc_dir,
                           proc_read_mbox0,
                           NULL);
#endif  //  defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE

#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
  #if defined(ENABLE_TR067_LOOPBACK_PROC) && ENABLE_TR067_LOOPBACK_PROC
    struct proc_dir_entry *res;

    res = create_proc_read_entry("test",
                           0,
                           g_ppe_proc_dir,
                           proc_read_test,
                           NULL);
    if ( res )
        res->write_proc = proc_write_test;
  #endif
#endif
}

static INLINE void proc_file_delete(void)
{
    remove_proc_entry("idle_counter",
                      g_ppe_proc_dir);

    remove_proc_entry("stats",
                      g_ppe_proc_dir);

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
    remove_proc_entry("rate",
                      g_ppe_proc_dir);
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

#if defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
    remove_proc_entry("mbox0",
                      g_ppe_proc_dir);
#endif  //  defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE

#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
  #if defined(ENABLE_TR067_LOOPBACK_PROC) && ENABLE_TR067_LOOPBACK_PROC
    remove_proc_entry("test",
                      g_ppe_proc_dir);
  #endif
#endif

    remove_proc_entry("ppe", NULL);
}

static int proc_read_idle_counter(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    MOD_INC_USE_COUNT;

    len += sprintf(page + off,       "Channel 0\n");
    len += sprintf(page + off + len, "  TX\n");
    len += sprintf(page + off + len, "    DREG_AT_CELL0       = %d\n", *DREG_AT_CELL0 & 0xFFFF);
    len += sprintf(page + off + len, "    DREG_AT_IDLE_CNT0   = %d\n", *DREG_AT_IDLE_CNT0 & 0xFFFF);
    len += sprintf(page + off + len, "  RX\n");
    len += sprintf(page + off + len, "    DREG_AR_CELL0       = %d\n", *DREG_AR_CELL0 & 0xFFFF);
    len += sprintf(page + off + len, "    DREG_AR_IDLE_CNT0   = %d\n", *DREG_AR_IDLE_CNT0 & 0xFFFF);
    len += sprintf(page + off + len, "    DREG_AR_AIIDLE_CNT0 = %d\n", *DREG_AR_AIIDLE_CNT0 & 0xFFFF);
    len += sprintf(page + off + len, "    DREG_AR_BE_CNT0     = %d\n", *DREG_AR_BE_CNT0 & 0xFFFF);
    len += sprintf(page + off + len, "Channel 1\n");
    len += sprintf(page + off + len, "  TX\n");
    len += sprintf(page + off + len, "    DREG_AT_CELL1       = %d\n", *DREG_AT_CELL1 & 0xFFFF);
    len += sprintf(page + off + len, "    DREG_AT_IDLE_CNT1   = %d\n", *DREG_AT_IDLE_CNT1 & 0xFFFF);
    len += sprintf(page + off + len, "  RX\n");
    len += sprintf(page + off + len, "    DREG_AR_CELL1       = %d\n", *DREG_AR_CELL1 & 0xFFFF);
    len += sprintf(page + off + len, "    DREG_AR_IDLE_CNT1   = %d\n", *DREG_AR_IDLE_CNT1 & 0xFFFF);
    len += sprintf(page + off + len, "    DREG_AR_AIIDLE_CNT1 = %d\n", *DREG_AR_AIIDLE_CNT1 & 0xFFFF);
    len += sprintf(page + off + len, "    DREG_AR_BE_CNT1     = %d\n", *DREG_AR_BE_CNT1 & 0xFFFF);

    MOD_DEC_USE_COUNT;

    return len;
}

static int proc_read_stats(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    int i, j;
    struct connection *connection;
    struct port *port;
    int base;

    MOD_INC_USE_COUNT;

    len += sprintf(page + off,       "ATM Stats:\n");

    connection = ppe_dev.connection;
    port = ppe_dev.port;
    for ( i = 0; i < ATM_PORT_NUMBER; i++, port++ )
    {
        base = port->connection_base;
        for ( j = 0; j < port->max_connections; j++, base++ )
            if ( (port->connection_table & (1 << j))
                && connection[base].vcc != NULL )
            {
                if ( connection[base].vcc->stats )
                {
                    struct k_atm_aal_stats *stats = connection[base].vcc->stats;

                    len += sprintf(page + off + len, "  VCC %d.%d.%d (stats)\n", i, connection[base].vcc->vpi, connection[base].vcc->vci);
                    len += sprintf(page + off + len, "    rx      = %d\n", stats->rx.counter);
                    len += sprintf(page + off + len, "    rx_err  = %d\n", stats->rx_err.counter);
                    len += sprintf(page + off + len, "    rx_drop = %d\n", stats->rx_drop.counter);
                    len += sprintf(page + off + len, "    tx      = %d\n", stats->tx.counter);
                    len += sprintf(page + off + len, "    tx_err  = %d\n", stats->tx_err.counter);
                }
                else
                    len += sprintf(page + off + len, "  VCC %d.%d.%d\n", i, connection[base].vcc->vpi, connection[base].vcc->vci);
            }
    }

    MOD_DEC_USE_COUNT;

    *eof = 1;

    return len;
}

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
static int proc_read_rate(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    int i;

    MOD_INC_USE_COUNT;

    len += sprintf(page + off,       "ATM Rate:\n");
    len += sprintf(page + off + len, "  Channel 0:\n");
    len += sprintf(page + off + len, "    RX Cell %d, bps %d\n", (ppe_dev.dreg_ar_cell_rate[0] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL, (ppe_dev.dreg_ar_cell_rate[0] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "    RX Idle %d, bps %d\n", (ppe_dev.dreg_ar_idle_rate[0] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL, (ppe_dev.dreg_ar_idle_rate[0] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "    RX Err  %d, bps %d\n", (ppe_dev.dreg_ar_be_rate[0] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL,   (ppe_dev.dreg_ar_be_rate[0] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "    TX Cell %d, bps %d\n", (ppe_dev.dreg_at_cell_rate[0] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL, (ppe_dev.dreg_at_cell_rate[0] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "    TX Idle %d, bps %d\n", (ppe_dev.dreg_at_idle_rate[0] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL, (ppe_dev.dreg_at_idle_rate[0] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "  Channel 1:\n");
    len += sprintf(page + off + len, "    RX Cell %d, bps %d\n", (ppe_dev.dreg_ar_cell_rate[1] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL, (ppe_dev.dreg_ar_cell_rate[1] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "    RX Idle %d, bps %d\n", (ppe_dev.dreg_ar_idle_rate[1] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL, (ppe_dev.dreg_ar_idle_rate[1] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "    RX Err  %d, bps %d\n", (ppe_dev.dreg_ar_be_rate[1] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL,   (ppe_dev.dreg_ar_be_rate[1] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "    TX Cell %d, bps %d\n", (ppe_dev.dreg_at_cell_rate[1] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL, (ppe_dev.dreg_at_cell_rate[1] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "    TX Idle %d, bps %d\n", (ppe_dev.dreg_at_idle_rate[1] + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL, (ppe_dev.dreg_at_idle_rate[1] * 48 * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
    len += sprintf(page + off + len, "Connection Rate:\n");
    for ( i = 0; i < MAX_CONNECTION_NUMBER; i++ )
        if ( ppe_dev.connection[i].vcc )
        {
            len += sprintf(page + off + len, "  Connection %d:\n", i);
            len += sprintf(page + off + len, "    RX bps %d\n", (ppe_dev.rx_bytes_rate[i] * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
            len += sprintf(page + off + len, "    TX bps %d\n", (ppe_dev.tx_bytes_rate[i] * 8 + DISPLAY_INTERVAL / 2) / DISPLAY_INTERVAL);
        }

    MOD_DEC_USE_COUNT;

    return len;
}
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

#if defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
static int proc_read_mbox0(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    MOD_INC_USE_COUNT;

    len += sprintf(page + off,       "MBox 0:\n");
    len += sprintf(page + off + len, "  MBOX_IGU0_ISRS = %08X:\n", *MBOX_IGU0_ISRS);
    len += sprintf(page + off + len, "  MBOX_IGU0_ISRC = %08X:\n", *MBOX_IGU0_ISRC);
    len += sprintf(page + off + len, "  MBOX_IGU0_ISR  = %08X:\n", *MBOX_IGU0_ISR);
    len += sprintf(page + off + len, "  MBOX_IGU0_IER  = %08X:\n", *MBOX_IGU0_IER);

    MOD_DEC_USE_COUNT;

    return len;
}
#endif  //  defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE

#if defined(ENABLE_TR067_LOOPBACK) && ENABLE_TR067_LOOPBACK
  #if defined(ENABLE_TR067_LOOPBACK_PROC) && ENABLE_TR067_LOOPBACK_PROC
static INLINE int stricmp(const char *p1, const char *p2)
{
    int c1, c2;

    while ( *p1 && *p2 )
    {
        c1 = *p1 >= 'A' && *p1 <= 'Z' ? *p1 + 'a' - 'A' : *p1;
        c2 = *p2 >= 'A' && *p2 <= 'Z' ? *p2 + 'a' - 'A' : *p2;
        if ( (c1 -= c2) )
            return c1;
        p1++;
        p2++;
    }

    return *p1 - *p2;
}

static int proc_read_test(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    MOD_INC_USE_COUNT;

    len += sprintf(page + off,       "test\n");

    MOD_DEC_USE_COUNT;

    return len;
}

static int proc_write_test(struct file *file, const char *buf, unsigned long count, void *data)
{
    char str[64];
    char *p;

    int len, rlen;

    MOD_INC_USE_COUNT;

    len = count < sizeof(str) ? count : sizeof(str) - 1;
    rlen = len - copy_from_user(str, buf, len);
    while ( rlen && str[rlen - 1] <= ' ' )
        rlen--;
    str[rlen] = 0;
    for ( p = str; *p && *p <= ' '; p++, rlen-- );
    if ( !*p )
    {
        MOD_DEC_USE_COUNT;
        return 0;
    }

    if ( stricmp(str, "send") == 0 )
    {
        struct atm_vcc *vcc;
        struct sk_buff *skb;
        struct uni_cell_header *uni_cell_header;
        unsigned char *p;
        int i, j;

        printk("send cell\n");
        vcc = ppe_dev.connection[ppe_dev.port[0].connection_base].vcc;
        printk("  vcc = %08X\n", (u32)vcc);

      #if 0
        for ( j = 0; j < ppe_dev.dma.tx_descriptor_number; j++ )
      #else
        for ( j = 0; j < 2; j++ )
      #endif
        {
            skb = alloc_skb_tx(128);
            printk("  skb = %08X\n", (u32)skb);

            skb_put(skb, ATM_AAL0_SDU);

            uni_cell_header = (struct uni_cell_header *)skb->data;
            uni_cell_header->gfc = 0;
            uni_cell_header->vpi = vcc->vpi;
            uni_cell_header->vci = vcc->vci;
            uni_cell_header->pti = ATM_PTI_US0;
            uni_cell_header->clp = 0;

            p = (unsigned char *)((u32)skb->data + 4);
            for ( i = 0; i < 56; i++ )
                *p++ = (unsigned char)((i + 1) / 10 * 16 + (i + 1) % 10);

            ppe_send(vcc, skb);
        }
    }

    MOD_DEC_USE_COUNT;

    return count;
}
  #endif
#endif

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
static void ppe_timer_callback(unsigned long arg)
{
    u32 dreg_ar_cell[2]     = {*DREG_AR_CELL0 & 0xFFFF, *DREG_AR_CELL1 & 0xFFFF};
    u32 dreg_ar_idle[2]     = {*DREG_AR_IDLE_CNT0 & 0xFFFF, *DREG_AR_IDLE_CNT1 & 0xFFFF};
    u32 dreg_ar_be[2]       = {*DREG_AR_BE_CNT0 & 0xFFFF, *DREG_AR_BE_CNT1 & 0xFFFF};
    u32 dreg_at_cell[2]     = {*DREG_AT_CELL0 & 0xFFFF, *DREG_AT_CELL1 & 0xFFFF};
    u32 dreg_at_idle[2]     = {*DREG_AT_IDLE_CNT0 & 0xFFFF, *DREG_AT_IDLE_CNT1 & 0xFFFF};

    if ( dreg_ar_cell[0] < ppe_dev.dreg_ar_cell_bak[0] )
        ppe_dev.dreg_ar_cell_bak[0] = 65536 + dreg_ar_cell[0] - ppe_dev.dreg_ar_cell_bak[0];
    else
        ppe_dev.dreg_ar_cell_bak[0] = dreg_ar_cell[0] - ppe_dev.dreg_ar_cell_bak[0];

    if ( dreg_ar_idle[0] < ppe_dev.dreg_ar_idle_bak[0] )
        ppe_dev.dreg_ar_idle_bak[0] = 65536 + dreg_ar_idle[0] - ppe_dev.dreg_ar_idle_bak[0];
    else
        ppe_dev.dreg_ar_idle_bak[0] = dreg_ar_idle[0] - ppe_dev.dreg_ar_idle_bak[0];

    if ( dreg_ar_be[0] < ppe_dev.dreg_ar_be_bak[0] )
        ppe_dev.dreg_ar_be_bak[0] = 65536 + dreg_ar_be[0] - ppe_dev.dreg_ar_be_bak[0];
    else
        ppe_dev.dreg_ar_be_bak[0] = dreg_ar_be[0] - ppe_dev.dreg_ar_be_bak[0];

    if ( dreg_at_cell[0] < ppe_dev.dreg_at_cell_bak[0] )
        ppe_dev.dreg_at_cell_bak[0] = 65536 + dreg_at_cell[0] - ppe_dev.dreg_at_cell_bak[0];
    else
        ppe_dev.dreg_at_cell_bak[0] = dreg_at_cell[0] - ppe_dev.dreg_at_cell_bak[0];

    if ( dreg_at_idle[0] < ppe_dev.dreg_at_idle_bak[0] )
        ppe_dev.dreg_at_idle_bak[0] = 65536 + dreg_at_idle[0] - ppe_dev.dreg_at_idle_bak[0];
    else
        ppe_dev.dreg_at_idle_bak[0] = dreg_at_idle[0] - ppe_dev.dreg_at_idle_bak[0];

    if ( dreg_ar_cell[1] < ppe_dev.dreg_ar_cell_bak[1] )
        ppe_dev.dreg_ar_cell_bak[1] = 65536 + dreg_ar_cell[1] - ppe_dev.dreg_ar_cell_bak[1];
    else
        ppe_dev.dreg_ar_cell_bak[1] = dreg_ar_cell[1] - ppe_dev.dreg_ar_cell_bak[1];

    if ( dreg_ar_idle[1] < ppe_dev.dreg_ar_idle_bak[1] )
        ppe_dev.dreg_ar_idle_bak[1] = 65536 + dreg_ar_idle[1] - ppe_dev.dreg_ar_idle_bak[1];
    else
        ppe_dev.dreg_ar_idle_bak[1] = dreg_ar_idle[1] - ppe_dev.dreg_ar_idle_bak[1];

    if ( dreg_ar_be[1] < ppe_dev.dreg_ar_be_bak[1] )
        ppe_dev.dreg_ar_be_bak[1] = 65536 + dreg_ar_be[1] - ppe_dev.dreg_ar_be_bak[1];
    else
        ppe_dev.dreg_ar_be_bak[1] = dreg_ar_be[1] - ppe_dev.dreg_ar_be_bak[1];

    if ( dreg_at_cell[1] < ppe_dev.dreg_at_cell_bak[1] )
        ppe_dev.dreg_at_cell_bak[1] = 65536 + dreg_at_cell[1] - ppe_dev.dreg_at_cell_bak[1];
    else
        ppe_dev.dreg_at_cell_bak[1] = dreg_at_cell[1] - ppe_dev.dreg_at_cell_bak[1];

    if ( dreg_at_idle[1] < ppe_dev.dreg_at_idle_bak[1] )
        ppe_dev.dreg_at_idle_bak[1] = 65536 + dreg_at_idle[1] - ppe_dev.dreg_at_idle_bak[1];
    else
        ppe_dev.dreg_at_idle_bak[1] = dreg_at_idle[1] - ppe_dev.dreg_at_idle_bak[1];

    ppe_dev.dreg_ar_cell_cur[0] += ppe_dev.dreg_ar_cell_bak[0];
    ppe_dev.dreg_ar_idle_cur[0] += ppe_dev.dreg_ar_idle_bak[0];
    ppe_dev.dreg_ar_be_cur[0]   += ppe_dev.dreg_ar_be_bak[0];
    ppe_dev.dreg_at_cell_cur[0] += ppe_dev.dreg_at_cell_bak[0];
    ppe_dev.dreg_at_idle_cur[0] += ppe_dev.dreg_at_idle_bak[0];

    ppe_dev.dreg_ar_cell_cur[1] += ppe_dev.dreg_ar_cell_bak[1];
    ppe_dev.dreg_ar_idle_cur[1] += ppe_dev.dreg_ar_idle_bak[1];
    ppe_dev.dreg_ar_be_cur[1]   += ppe_dev.dreg_ar_be_bak[1];
    ppe_dev.dreg_at_cell_cur[1] += ppe_dev.dreg_at_cell_bak[1];
    ppe_dev.dreg_at_idle_cur[1] += ppe_dev.dreg_at_idle_bak[1];

    ppe_dev.dreg_ar_cell_bak[0] = dreg_ar_cell[0];
    ppe_dev.dreg_ar_idle_bak[0] = dreg_ar_idle[0];
    ppe_dev.dreg_ar_be_bak[0]   = dreg_ar_be[0];
    ppe_dev.dreg_at_cell_bak[0] = dreg_at_cell[0];
    ppe_dev.dreg_at_idle_bak[0] = dreg_at_idle[0];

    ppe_dev.dreg_ar_cell_bak[1] = dreg_ar_cell[1];
    ppe_dev.dreg_ar_idle_bak[1] = dreg_ar_idle[1];
    ppe_dev.dreg_ar_be_bak[1]   = dreg_ar_be[1];
    ppe_dev.dreg_at_cell_bak[1] = dreg_at_cell[1];
    ppe_dev.dreg_at_idle_bak[1] = dreg_at_idle[1];

    if ( ++ppe_dev.heart_beat >= DISPLAY_INTERVAL * POLL_FREQ )
    {
        int i;

        for ( i = 0; i < 2; i++ )
        {
            ppe_dev.dreg_ar_cell_rate[i]    = ppe_dev.dreg_ar_cell_cur[i];
            ppe_dev.dreg_ar_idle_rate[i]    = ppe_dev.dreg_ar_idle_cur[i];
            ppe_dev.dreg_ar_be_rate[i]      = ppe_dev.dreg_ar_be_cur[i];
            ppe_dev.dreg_at_cell_rate[i]    = ppe_dev.dreg_at_cell_cur[i];
            ppe_dev.dreg_at_idle_rate[i]    = ppe_dev.dreg_at_idle_cur[i];

            ppe_dev.dreg_ar_cell_cur[i] = 0;
            ppe_dev.dreg_ar_idle_cur[i] = 0;
            ppe_dev.dreg_ar_be_cur[i]   = 0;
            ppe_dev.dreg_at_cell_cur[i] = 0;
            ppe_dev.dreg_at_idle_cur[i] = 0;
        }

        memcpy(ppe_dev.rx_bytes_rate, ppe_dev.rx_bytes_cur, sizeof(ppe_dev.rx_bytes_cur));
        memcpy(ppe_dev.tx_bytes_rate, ppe_dev.tx_bytes_cur, sizeof(ppe_dev.tx_bytes_cur));
        memset(ppe_dev.rx_bytes_cur, 0, sizeof(ppe_dev.rx_bytes_cur));
        memset(ppe_dev.tx_bytes_cur, 0, sizeof(ppe_dev.tx_bytes_cur));

        ppe_dev.heart_beat = 0;
    }
}
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

#if defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
static void pci_fix_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    if ( MBOX_IGU0_ISR_ISR(0) )
    {
        danube_enable_external_pci();
        *MBOX_IGU0_ISRC = MBOX_IGU0_ISRC_CLEAR(0);
    }

    if ( MBOX_IGU0_ISR_ISR(1) )
    {
        danube_disable_external_pci();
        *MBOX_IGU0_ISRC = MBOX_IGU0_ISRC_CLEAR(1);
    }
}
#endif  //  defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE

#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
#if 0
/*
 *  Description:
 *    Main loop to simulate firmware.
 *  Input:
 *    arg --- void *, parameter passed in when creating thread
 *  Output:
 *    int --- 0:    Success
 *            else: Error Code
 */
static int debug_thread_proc(void *arg)
{
    while ( debug_thread_running )
    {
//      schedule();
    }

    return 0;
}
#endif

static int debug_ppe_sim(void)
{
    int channel_mask;
    int channel;
    int is_rx;

//    dbg("debug_ppe_sim");

//    for ( channel = 0; channel < 10000; channel++ );

    channel_mask = 1;
    channel = 0;
    is_rx = 1;
    while ( *MBOX_IGU3_ISR )
    {
//        dbg("%s, channel = %d", is_rx ? "rx" : "tx", channel);

        if ( (*MBOX_IGU3_ISR & channel_mask) )
        {
//            dbg("channel %d interrupt", channel);

            /*  clear IRQ   */
            *MBOX_IGU3_ISR &= ~channel_mask;

            if ( is_rx )
            {
//                dbg("handle RX (ch %d)", channel);

                /*  RX  */
                WRX_DMA_CHANNEL_CONFIG(channel)->vlddes--;
            }
            else
            {
                /*  TX  */
                if ( WTX_DMA_CHANNEL_CONFIG(channel)->vlddes >= WTX_DMA_CHANNEL_CONFIG(channel & 0x0F)->deslen )
                {
                    /*  critical error, don't do anything   */
                    dbg("critical error, WTX_DMA_CHANNEL_CONFIG(channel)->vlddes >= WTX_DMA_CHANNEL_CONFIG(channel & 0x0F)->deslen");
                }
                else
                {
                    struct tx_descriptor *tx_desc;

                    struct tx_inband_header *headerptr;
                    unsigned char *dataptr;
                    unsigned int datalen;

                    int i;

//                    dbg("handle TX (ch %d)", channel);

                    /*  increase valid descriptor counter   */
                    WTX_DMA_CHANNEL_CONFIG(channel)->vlddes++;

                    /*  get descriptor address  */
//                    dbg("desba = 0x%08X, size = %d, start point = %d", ADDR_MSB_FIX + (u32)(WTX_DMA_CHANNEL_CONFIG(channel)->desba << 2), sizeof(struct tx_descriptor), (int)debug_tx_desc_start_point[channel]);
                    tx_desc = (struct tx_descriptor *)(ADDR_MSB_FIX + (WTX_DMA_CHANNEL_CONFIG(channel)->desba << 2) + sizeof(struct tx_descriptor) * debug_tx_desc_start_point[channel]);

                    /*  move descriptor read pointer to next one    */
                    if ( ++debug_tx_desc_start_point[channel] >= WTX_DMA_CHANNEL_CONFIG(channel)->deslen )
                        debug_tx_desc_start_point[channel] = 0;

#if 0
                    if ( WTX_QUEUE_CONFIG(channel)->type == 0x03 )
#else
                    if ( !tx_desc->iscell )
#endif
                    {
                        /*  AAL 5   */

//                        dbg("handle AAL5");

//                        dbg("tx_desc = 0x%08X", (u32)tx_desc);

                        /*  inband header address   */
                        headerptr = (struct tx_inband_header *)(ADDR_MSB_FIX + (tx_desc->dataptr << 2) + (tx_desc->byteoff >> 2));

//                        dbg("headerptr = 0x%08X, org = 0x%08X, byteoff = %d", (u32)headerptr, (u32)tx_desc->dataptr, tx_desc->byteoff);

                        /*  packet address  */
                        dataptr = (unsigned char *)(ADDR_MSB_FIX + (tx_desc->dataptr << 2) + tx_desc->byteoff + sizeof(struct tx_inband_header));

//                        dbg("headerptr = 0x%08X, dataptr = 0x%08X", (u32)headerptr, (u32)dataptr);

                        /*  length of packet    */
                        datalen = tx_desc->datalen;

                        /*  modify VCI  */
//                        headerptr->vci++;

//                        dbg("pti.vpi.vci = %d.%d.%d", headerptr->pti, headerptr->vpi, headerptr->vci);
                    }
                    else
                    {
                        /*  cell    */

                        /*  only first dword of structure tx_inband_header  */
                        headerptr = (struct tx_inband_header *)(ADDR_MSB_FIX + (tx_desc->dataptr << 2));

                        /*  cell content address    */
                        dataptr = (unsigned char *)(ADDR_MSB_FIX + (tx_desc->dataptr << 2) + 4);

                        /*  cell content length */
                        datalen = 48;
                    }   //  AAL5 or cell

                    /*  search in HTU   */
//                    dbg("start searching HTU");
                    for ( i = 0; (unsigned int)i < *CFG_WRX_HTUTS; i++ )
                        if ( HTU_ENTRY(i)->vld )
                        {
                            struct htu_entry entry;
                            unsigned long result;

                            entry.res1 = 0;
                            entry.pid  = WTX_QUEUE_CONFIG(channel)->sbid;
                            entry.vpi  = headerptr->vpi;
                            entry.vci  = headerptr->vci;
                            entry.pti  = headerptr->pti;
                            entry.vld  = 1;

                            result = *(unsigned long *)&entry & ~*(unsigned long *)HTU_MASK(i);
                            result ^= *(unsigned long *)HTU_ENTRY(i) & ~*(unsigned long *)HTU_MASK(i);

                            if ( !result )
                                /*  match   */
                                break;
                        }
//                    dbg("finish searching HTU");
                    if ( (unsigned int)i < *CFG_WRX_HTUTS )
                    {
                        /*  match and loop back */

                        int rx_qid;
                        int rx_channel;
                        struct rx_descriptor *rx_desc;
                        unsigned char *rx_dataptr;

//                        dbg("HTU match (qid %d, dma %d)", (int)(HTU_RESULT(i)->type == 0 ? HTU_RESULT(i)->qid : HTU_RESULT(i)->cellid), ppe_dev.connection[HTU_RESULT(i)->qid].rx_dma_channel);

                        /*  get qid */
                        rx_qid = (int)(HTU_RESULT(i)->type == 0 ? HTU_RESULT(i)->qid : HTU_RESULT(i)->cellid);
                        /*  refine to DMA channel ID    */
                        rx_channel = ppe_dev.connection[rx_qid].rx_dma_channel;

                        /*  get descriptor address  */
                        rx_desc = (struct rx_descriptor *)(ADDR_MSB_FIX + (WRX_DMA_CHANNEL_CONFIG(rx_channel)->desba << 2) + sizeof(struct rx_descriptor) * debug_rx_desc_start_point[rx_channel]);

//                        dbg("rx_desc = 0x%08X", (u32)rx_desc);

                        /*  check space available   */
                        if ( rx_desc->datalen < datalen )
                        {
                            /*  size error  */
                            dbg("debug_ppe_sim: rx size error");
                        }

                        /*  set some fields */
                        rx_desc->err = 0;
                        rx_desc->id = rx_qid;
                        rx_desc->byteoff = 0;
                        rx_desc->eop = 1;
                        rx_desc->sop = 1;
                        rx_desc->c = 1;
                        rx_desc->own = 0;

#if 0
                        if ( WTX_QUEUE_CONFIG(channel)->type == 0x03 )
#else
                        if ( !tx_desc->iscell )
#endif
                        {
                            /*  AAL 5   */

                            rx_dataptr = (unsigned char *)(ADDR_MSB_FIX + (rx_desc->dataptr << 2));
//                            dbg("rx_dataptr = 0x%08X, dataptr = 0x%08X, datalen = %d", (u32)rx_dataptr, (u32)dataptr, datalen);
                            for ( i = 0; i < datalen; i++ )
                                rx_dataptr[i] = dataptr[i];

//                            dbg("before handle header");

                            rx_dataptr += (datalen + 3) & ~0x03;
                            for ( i = 0; i < 8; i++ )
                                rx_dataptr[i] = ((char *)headerptr)[i];
                            rx_dataptr[2] = 0;
                            rx_dataptr[3] = 0;

                            rx_desc->datalen = datalen;

//                            dbg("after handle header");
                        }
                        else
                        {
                            /*  cell    */

                            rx_dataptr = (unsigned char *)(ADDR_MSB_FIX + (rx_desc->dataptr << 2));

                            for ( i = 0; i < 52; i++ )
                                rx_dataptr[i] = dataptr[i];

                            rx_desc->datalen = 52;
                        }   //  AAL5 or cell

                        /*  move descriptor read pointer to next one    */
                        if ( ++debug_rx_desc_start_point[rx_channel] >= WRX_DMA_CHANNEL_CONFIG(rx_channel)->deslen )
                            debug_rx_desc_start_point[rx_channel] = 0;

                        /*  check if counter of valid descriptor exceeds boundary   */
                        if ( WRX_DMA_CHANNEL_CONFIG(rx_channel)->vlddes >= WRX_DMA_CHANNEL_CONFIG(rx_channel)->deslen )
                        {
                            /*  critical error  */
                            dbg("critical error, WRX_DMA_CHANNEL_CONFIG(rx_channel)->vlddes >= WRX_DMA_CHANNEL_CONFIG(rx_channel)->deslen");
                        }

                        /*  increase valid descriptor counter   */
                        WRX_DMA_CHANNEL_CONFIG(rx_channel)->vlddes++;

                        /*  signal MIPS to handle incoming packet   */
                        *MBOX_IGU1_ISR |= 1 << rx_channel;
//                        dbg("rx_channel = %d", rx_channel);
                    }
                    else
                    {
                        /*  not match so that drop  */
                        dbg("HTU not match");
                    }

                    /*  decrease valid descriptor counter after processing  */
                    WTX_DMA_CHANNEL_CONFIG(channel)->vlddes--;

                    /*  update descriptor   */
                    tx_desc->own = 0;

                    /*  signal PPE driver to release the TX descriptor  */
                    *MBOX_IGU1_ISR |= channel_mask;
//                    dbg("TX channel_mask = 0x%08X", channel_mask);
//                    dbg("before mailbox_irq_handler");
                   mailbox_irq_handler(PPE_MAILBOX_IGU1_INT, (void *)0, (struct pt_regs *)0);
//                    dbg("after mailbox_irq_handler");
                }   //  critical error check
            }   //  RX or TX
        }

        if ( channel_mask == (1 << (ppe_dev.dma.rx_total_channel_used - 1)) )
        {
            /*  go to TX part   */
            channel_mask = 1 << (16 + QSB_QUEUE_NUMBER_BASE);
            channel = QSB_QUEUE_NUMBER_BASE;
            is_rx = 0;
        }
        else if ( channel_mask == (1 << (16 + QSB_QUEUE_NUMBER_BASE + ppe_dev.dma.tx_total_channel_used - 1)) )
        {
            /*  goto RX part    */
            channel_mask = 1;
            channel = 0;
            is_rx = 1;
        }
        else
        {
            channel_mask <<= 1;
            channel++;
        }
    }

    return 0;
}

/*
 *  Description:
 *    Start a thread to simulate firmware
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void init_debug(void)
{
//  debug_thread_running = 1;
//  kernel_thread(debug_thread_proc, NULL, CLONE_THREAD);
}

/*
 *  Description:
 *    Stop thread simulating firmware
 *  Input:
 *    none
 *  Output:
 *    none
 */
static INLINE void clear_debug(void)
{
    debug_thread_running = 0;
}
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON


/*
 * ####################################
 *           Global Function
 * ####################################
 */
void ifx_atm_set_cell_rate(int port, u32 cell_rate)
{
#if !defined(DISABLE_DANUBE_PPE_SET_CELL_RATE) || !DISABLE_DANUBE_PPE_SET_CELL_RATE
  #if !defined(DISABLE_QSB) || !DISABLE_QSB
    #if defined(QSB_IS_HALF_FPI2_CLOCK) && QSB_IS_HALF_FPI2_CLOCK
    u32 qsb_clk = cgu_get_fpi_bus_clock(2) >> 1;    /*  Half of FPI configuration 2 (slow FPI bus) */
    #else
    u32 qsb_clk = cgu_get_fpi_bus_clock(2);         /*  FPI configuration 2 (slow FPI bus)  */
    #endif  //  defined(QSB_IS_HALF_FPI2_CLOCK) && QSB_IS_HALF_FPI2_CLOCK
    u32 tmp1, tmp2, tmp3;

    if ( port < 0 || port > 1 )
        return;

    printk("Set PPE port %d max cell rate: %d\n", port, cell_rate);

    ppe_dev.port[port].tx_max_cell_rate = cell_rate;

    if ( ppe_dev.port[port].max_connections != 0 && ppe_dev.port[port].tx_max_cell_rate != 0 )
    {
        tmp1 = ((qsb_clk * ppe_dev.qsb.tstepc) >> 1) / ppe_dev.port[port].tx_max_cell_rate;
        tmp2 = tmp1 >> 6;                   /*  integer value of Tsb    */
        tmp3 = (tmp1 & ((1 << 6) - 1)) + 1; /*  fractional part of Tsb  */
        /*  carry over to integer part (?)  */
        if ( tmp3 == (1 << 6) )
        {
            tmp3 = 0;
            tmp2++;
        }
        if ( tmp2 == 0 )
            tmp2 = tmp3 = 1;
        /*  1. set mask                                 */
        /*  2. write value to data transfer register    */
        /*  3. start the tranfer                        */
        /* SCT (FracRate)  */
        *QSB_RTM   = QSB_RTM_DM_SET(QSB_SET_SCT_MASK);
        *QSB_RTD   = QSB_RTD_TTV_SET(tmp3);
        *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_SCT) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(port & 0x01);
    #if defined(DEBUG_QOS) && DEBUG_QOS
        printk("SCT: QSB_RTM = 0x%08X, QSB_RTD = 0x%08X, QSB_RAMAC = 0x%08X\n", *QSB_RTM, *QSB_RTD, *QSB_RAMAC);
    #endif
        /*  SPT (SBV + PN + IntRage)    */
        *QSB_RTM   = QSB_RTM_DM_SET(QSB_SET_SPT_MASK);
        *QSB_RTD   = QSB_RTD_TTV_SET(QSB_SPT_SBV_VALID | QSB_SPT_PN_SET(port & 0x01) | QSB_SPT_INTRATE_SET(tmp2));
        *QSB_RAMAC = QSB_RAMAC_RW_SET(QSB_RAMAC_RW_WRITE) | QSB_RAMAC_TSEL_SET(QSB_RAMAC_TSEL_SPT) | QSB_RAMAC_LH_SET(QSB_RAMAC_LH_LOW) | QSB_RAMAC_TESEL_SET(port & 0x01);
    #if defined(DEBUG_QOS) && DEBUG_QOS
        printk("SPT: QSB_RTM = 0x%08X, QSB_RTD = 0x%08X, QSB_RAMAC = 0x%08X\n", *QSB_RTM, *QSB_RTD, *QSB_RAMAC);
    #endif
    }

    //  reconfigure QSB for every PVC of this port
    {
        struct atm_vcc *vcc;
        struct port *p;
        int conn;
        int i;

        p = ppe_dev.port + port;
        if ( p->max_connections != 0 && p->connection_table != 0 )
            for ( i = 0, conn = p->connection_base; i < p->max_connections; i++, conn++ )
                if ( (p->connection_table & (1 << i)) )
                {
                    vcc = ppe_dev.connection[conn].vcc;

                    if ( vcc )
                        //  set QSB
                        set_qsb(vcc, &vcc->qos, conn);
                }
    }
  #endif
#endif
}

// tc.chen
/*
 *  Description:
 *    API for dsl led module to register a callback function
 *  Input:
 *    adsl_led_cb --- pointer to the callback function
 *  Output:
 *    int     --- 0: valid, -EIO: invalid
 */
int IFX_ATM_LED_Callback_Register(void (*adsl_led_cb)(void))
{
    int error =0;

    if ( adsl_led_cb && adsl_led_flash_cb == NULL )
        adsl_led_flash_cb = adsl_led_cb;
    else
        error = -EIO;

    return error;
}

// tc.chen
/*
 *  Description:
 *    API for dsl led module to unregister a callback function
 *  Input:
 *    adsl_led_cb --- pointer to the callback function
 *  Output:
 *    int     --- 0: valid, -EIO: invalid
 */
int IFX_ATM_LED_Callback_Unregister( void (*adsl_led_cb)(void))
{
    int error =0;

    if ( adsl_led_cb && adsl_led_flash_cb == adsl_led_cb )
        adsl_led_flash_cb = NULL;
    else
        error = -EIO;

    return error;
}



/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

//static INLINE void test_get_count(void)
//{
//    unsigned int count1, count2;
//    unsigned int compare1, compare2;

//    count1 = read_c0_count();
//    __asm__("sync");
//    compare1 = read_c0_compare();
//    __asm__("sync");
//    count2 = read_c0_count();
//    __asm__("sync");
//    compare2 = read_c0_compare();
//    printk("count1 = %d, count2 = %d, compare1 = %d, compare2 = %d\n", count1, count2, compare1, compare2);
//}

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
int __init danube_ppe_init(void)
{
    int ret;
    int port_num;

//    test_get_count();

    check_parameters();

    ret = init_ppe_dev();
    if ( ret )
        goto INIT_PPE_DEV_FAIL;

    clear_share_buffer();
    init_rx_tables();
    init_tx_tables();

#if 0
    {
        int i;

        printk("CFG_WRX_HTUTS   = %d\n", *CFG_WRX_HTUTS);
        printk("CFG_WRX_QNUM    = %d\n", *CFG_WRX_QNUM);
        printk("CFG_WRX_DCHNUM  = %d\n", *CFG_WRX_DCHNUM);
        printk("CFG_WTX_DCHNUM  = %d\n", *CFG_WTX_DCHNUM);
        printk("CFG_WRDES_DELAY = %d\n", *CFG_WRDES_DELAY);
        printk("WRX_DMACH_ON    = 0x%08X\n", *WRX_DMACH_ON);
        printk("WTX_DMACH_ON    = 0x%08X\n", *WTX_DMACH_ON);
        printk("WRX_HUNT_BITTH  = %d\n", *WRX_HUNT_BITTH);

        printk("WRX_QUEUE_CONFIG\n");
        for ( i = 0; i < 16; i++ )
            printk("%2d, 0x%08X,  0x%08X 0x%08X 0x%08X 0x%08X\n", i, (u32)WRX_QUEUE_CONFIG(i), *(u32*)WRX_QUEUE_CONFIG(i), *((u32*)WRX_QUEUE_CONFIG(i) + 1), *((u32*)WRX_QUEUE_CONFIG(i) + 2), *((u32*)WRX_QUEUE_CONFIG(i) + 3));

        printk("WRX_DMA_CHANNEL_CONFIG\n");
        for ( i = 0; i < 8; i++ )
            printk("%2d, 0x%08X,  0x%08X 0x%08X 0x%08X\n", i, (u32)WRX_DMA_CHANNEL_CONFIG(i), *(u32*)WRX_DMA_CHANNEL_CONFIG(i), *((u32*)WRX_DMA_CHANNEL_CONFIG(i) + 1), *((u32*)WRX_DMA_CHANNEL_CONFIG(i) + 2));

        printk("WTX_PORT_CONFIG\n");
        for ( i = 0; i < ATM_PORT_NUMBER; i++ )
            printk("%2d, 0x%08X,  0x%08X\n", i, (u32)WTX_PORT_CONFIG(i), *(u32*)WTX_PORT_CONFIG(i));

        printk("WTX_QUEUE_CONFIG\n");
        for ( i = 0; i < 16; i++ )
            printk("%2d, 0x%08X,  0x%08X\n", i, (u32)WTX_QUEUE_CONFIG(i), *(u32*)WTX_QUEUE_CONFIG(i));

        printk("WTX_DMA_CHANNEL_CONFIG\n");
        for ( i = 0; i < 16; i++ )
            printk("%2d, 0x%08X,  0x%08X 0x%08X 0x%08X\n", i, (u32)WTX_DMA_CHANNEL_CONFIG(i), *(u32*)WTX_DMA_CHANNEL_CONFIG(i), *((u32*)WTX_DMA_CHANNEL_CONFIG(i) + 1), *((u32*)WTX_DMA_CHANNEL_CONFIG(i) + 2));
    }
#endif

    /*  create devices  */
    for ( port_num = 0; port_num < ATM_PORT_NUMBER; port_num++ )
        if ( ppe_dev.port[port_num].max_connections != 0 )
        {
            ppe_dev.port[port_num].dev = atm_dev_register("danube_atm", &ppe_atm_ops, -1, 0UL);
            if ( !ppe_dev.port[port_num].dev )
            {
                ret = -EIO;
                goto ATM_DEV_REGISTER_FAIL;
            }
            else
            {
                ppe_dev.port[port_num].dev->ci_range.vpi_bits = 8;
                ppe_dev.port[port_num].dev->ci_range.vci_bits = 16;
                ppe_dev.port[port_num].dev->link_rate = ppe_dev.port[port_num].tx_max_cell_rate;
                ppe_dev.port[port_num].dev->dev_data = (void*)port_num;
            }
        }

#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    /*  register interrupt handler  */
    ret = request_irq(PPE_MAILBOX_IGU1_INT, mailbox_irq_handler, SA_INTERRUPT, "ppe_mailbox_isr", NULL);
    if ( ret )
    {
        if ( ret == -EBUSY )
            printk("ppe: IRQ may be occupied by ETH2 driver, please reconfig to disable it.\n");
        goto REQUEST_IRQ_PPE_MAILBOX_IGU1_INT_FAIL;
    }
    disable_irq(PPE_MAILBOX_IGU1_INT);
//    enable_irq(PPE_MAILBOX_IGU1_INT);

  #if defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
    ret = request_irq(PPE_MAILBOX_IGU0_INT, pci_fix_irq_handler, SA_INTERRUPT, "ppe_pci_fix_isr", NULL);
    if ( ret )
        printk("failed in registering mailbox 0 interrupt (pci fix)\n");
  #endif  //  defined(CONFIG_PCI) && defined(USE_FIX_FOR_PCI_PPE) && USE_FIX_FOR_PCI_PPE
#endif  //  !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON

    init_chip();

    ret = pp32_start();
    if ( ret )
        goto PP32_START_FAIL;

    qsb_global_set();
    validate_oam_htu_entry();

    /*  create proc file    */
    proc_file_create();

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
    ppe_dev.timer = set_timer(0, POLL_FREQ * 1000, 1, 0, TIMER_FLAG_CALLBACK_IN_IRQ, (unsigned long)ppe_timer_callback, 0);
    if ( ppe_dev.timer > 0 )
    {
        ppe_dev.dreg_ar_cell_bak[0] = *DREG_AR_CELL0 & 0xFFFF;
        ppe_dev.dreg_ar_idle_bak[0] = *DREG_AR_IDLE_CNT0 & 0xFFFF;
        ppe_dev.dreg_ar_be_bak[0]   = *DREG_AR_BE_CNT0 & 0xFFFF;
        ppe_dev.dreg_at_cell_bak[0] = *DREG_AT_CELL0 & 0xFFFF;
        ppe_dev.dreg_at_idle_bak[0] = *DREG_AT_IDLE_CNT0 & 0xFFFF;

        ppe_dev.dreg_ar_cell_bak[1] = *DREG_AR_CELL1 & 0xFFFF;
        ppe_dev.dreg_ar_idle_bak[1] = *DREG_AR_IDLE_CNT1 & 0xFFFF;
        ppe_dev.dreg_ar_be_bak[1]   = *DREG_AR_BE_CNT1 & 0xFFFF;
        ppe_dev.dreg_at_cell_bak[1] = *DREG_AT_CELL1 & 0xFFFF;
        ppe_dev.dreg_at_idle_bak[1] = *DREG_AT_IDLE_CNT1 & 0xFFFF;

        start_timer(ppe_dev.timer, 0);
    }
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    /*  test buffer manage functions    */
//    {
//        struct sk_buff *skb_rx, *skb_tx;
//
//        skb_rx = alloc_skb_rx();
//        dbg("skb_rx = 0x%08X", (u32)skb_rx);
//        if ( skb_rx )
//        {
//            dbg("skb_rx->truesize = %d", skb_rx->truesize);
//            dbg("skb_rx->head = 0x%08X", (u32)skb_rx->head);
//            dbg("skb_rx->data = 0x%08X", (u32)skb_rx->data);
//            dbg("skb_rx->tail = 0x%08X", (u32)skb_rx->tail);
//            dbg("skb_rx->end = 0x%08X", (u32)skb_rx->end);
//            dbg("skb_rx->len = %d", skb_rx->len);
//            dbg("skb_rx->data_len = %d", skb_rx->data_len);
//        }
//        dbg("");
//
//        resize_skb_rx(skb_rx, ppe_dev.aal5.rx_buffer_size, 0);
//        dbg("skb_rx = 0x%08X", (u32)skb_rx);
//        {
//            dbg("skb_rx->truesize = %d", skb_rx->truesize);
//            dbg("skb_rx->head = 0x%08X", (u32)skb_rx->head);
//            dbg("skb_rx->data = 0x%08X", (u32)skb_rx->data);
//            dbg("skb_rx->tail = 0x%08X", (u32)skb_rx->tail);
//            dbg("skb_rx->end = 0x%08X", (u32)skb_rx->end);
//            dbg("skb_rx->len = %d", skb_rx->len);
//            dbg("skb_rx->data_len = %d", skb_rx->data_len);
//        }
//        dbg("");
//
//        resize_skb_rx(skb_rx, 65, 1);
//        dbg("skb_rx = 0x%08X", (u32)skb_rx);
//        {
//            dbg("skb_rx->truesize = %d", skb_rx->truesize);
//            dbg("skb_rx->head = 0x%08X", (u32)skb_rx->head);
//            dbg("skb_rx->data = 0x%08X", (u32)skb_rx->data);
//            dbg("skb_rx->tail = 0x%08X", (u32)skb_rx->tail);
//            dbg("skb_rx->end = 0x%08X", (u32)skb_rx->end);
//            dbg("skb_rx->len = %d", skb_rx->len);
//            dbg("skb_rx->data_len = %d", skb_rx->data_len);
//        }
//        dbg("");
//
//        skb_tx = alloc_skb_tx(ppe_dev.aal5.rx_buffer_size);
//        dbg("skb_tx = 0x%08X", (u32)skb_tx);
//        if ( skb_tx )
//        {
//            dbg("skb_tx->truesize = %d", skb_tx->truesize);
//            dbg("skb_tx->head = 0x%08X", (u32)skb_tx->head);
//            dbg("skb_tx->data = 0x%08X", (u32)skb_tx->data);
//            dbg("skb_tx->tail = 0x%08X", (u32)skb_tx->tail);
//            dbg("skb_tx->end = 0x%08X", (u32)skb_tx->end);
//            dbg("skb_tx->len = %d", skb_tx->len);
//            dbg("skb_tx->data_len = %d", skb_tx->data_len);
//        }
//        dbg("");
//
//        dbg("before free skb");
//        dev_kfree_skb_any(skb_rx);
//        dbg("rx finish");
//        dev_kfree_skb_any(skb_tx);
//        dbg("after free skb");
//    }

    init_debug();
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

    printk("ppe: ATM init succeeded (firmware version 1.1.0.2.1.13)\n");
    return 0;

PP32_START_FAIL:

#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    free_irq(PPE_MAILBOX_IGU1_INT, NULL);
REQUEST_IRQ_PPE_MAILBOX_IGU1_INT_FAIL:
#endif  //  !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON

    for ( ; port_num > 0; )
    {
        port_num--;
        atm_dev_deregister(ppe_dev.port[port_num].dev);
    }
ATM_DEV_REGISTER_FAIL:
    clear_ppe_dev();
INIT_PPE_DEV_FAIL:
    printk("ppe: ATM init failed\n");
    return ret;
}

/*
 *  Description:
 *    Release memory, free IRQ, and deregister device.
 *  Input:
 *    none
 *  Output:
 *   none
 */
void __exit danube_ppe_exit(void)
{
    int port_num;

#if defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT
    if ( ppe_dev.timer > 0 )
    {
        stop_timer(ppe_dev.timer);
        free_timer(ppe_dev.timer);
    }
#endif  //  defined(DEBUG_CONNECTION_THROUGHPUT) && DEBUG_CONNECTION_THROUGHPUT

    proc_file_delete();

    invalidate_oam_htu_entry();

    pp32_stop();

#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    clear_debug();
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    free_irq(PPE_MAILBOX_IGU1_INT, NULL);
#endif  //  !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON

    for ( port_num = 0; port_num < ATM_PORT_NUMBER; port_num++ )
        if ( ppe_dev.port[port_num].max_connections != 0 )
            atm_dev_deregister(ppe_dev.port[port_num].dev);

    clear_ppe_dev();
}

//tc.chen
EXPORT_SYMBOL(ifx_atm_set_cell_rate);
EXPORT_SYMBOL(IFX_ATM_LED_Callback_Register);
EXPORT_SYMBOL(IFX_ATM_LED_Callback_Unregister);

module_init(danube_ppe_init);
module_exit(danube_ppe_exit);
