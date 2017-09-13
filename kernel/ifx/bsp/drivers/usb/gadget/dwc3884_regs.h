/*
 * Device controller for the Synopsys 3884-0 DWC USB 2.0 HS OTG Subsystem-AHB.
 * Specs and errata are available from http://www.synopsys.com.
 *
 * Synopsys Inc. supported the development of this driver.
 *  
 */

/*
 * Copyright (C) 2006 Christian Schroeder, EMSYS GmbH, Ilmenau, Germany
 * Copyright (C) 2006 EMSYS GmbH, ILmenau, Germany, GmbH (http://www.emsys.de) 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*---------------------------- register interface ----------------------------*/

/* USB registers: 0xBE101000 - 0xBE101FFF */

/* core global registers */
struct GADGET_NAME(__GADGET,_core_regs) {

  /* offset 0x000 */
  volatile u32 gotgctl;                     
  /* OTG control and status register */
#define GOTGCTL_DEV_HNP_EN_BIT          11
#define GOTGCTL_CON_ID_STS_BIT          16
#define GOTGCTL_B_SES_VLD_BIT           19

  volatile u32 gotgint;                     
  /* OTG interrupt register */
#define GOTGINT_SES_END_DET             2
#define GOTGINT_SES_REQ_SUS_STS_CHNG    8
#define GOTGINT_HST_NEG_SUC_STS_CHNG    9
#define GOTGINT_HST_NEG_DET             17
#define GOTGINT_ADEV_TOUT_CHG           18
#define GOTGINT_DBNCE_DONE              19

  volatile u32 gahbcfg;                     
  /* Core AHB configuration register */
#define GAHBCFG_GLBL_INTR_MSK_BIT       0
#define GAHBCFG_HBST_LEN_LBIT           1
#define GAHBCFG_HBST_LEN_HBIT           4
#define GAHBCFG_HBST_LEN_SINGLE         0
#define GAHBCFG_HBST_LEN_INCR           1
#define GAHBCFG_HBST_LEN_INCR4          3
#define GAHBCFG_HBST_LEN_INCR8          5
#define GAHBCFG_HBST_LEN_INCR16         7
#define GAHBCFG_DMA_EN_BIT              5

  volatile u32 gusbcfg;                     
  /* Core USB configuration register */
#define GUSBCFG_TOUT_CAL_LBIT           0
#define GUSBCFG_TOUT_CAL_HBIT           2
#define GUSBCFG_PHY_IF_BIT              3
#define GUSBCFG_ULPI_UTMI_SEL_BIT       4
#define GUSBCFG_PHY_SEL_BIT             6
#define GUSBCFG_DDR_SEL_BIT             7
#define GUSBCFG_SRP_CAP_BIT             8
#define GUSBCFG_HNP_CAP_BIT             9
#define GUSBCFG_TRD_TIM_LBIT            10
#define GUSBCFG_TRD_TIM_HBIT            13

  volatile u32 grstctl;                     
  /* Core reset register */
#define GRSTCTL_RX_FFLSH_BIT            4
#define GRSTCTL_TX_FFLSH_BIT            5
#define GRSTCTL_TX_FNUM_LBIT            6
#define GRSTCTL_TX_FNUM_HBIT            10
#define GRSTCTL_TX_FLSH_NPT             0
#define GRSTCTL_TX_FLSH_ALL             16

  volatile u32 gintsts;                     
  /* Core interrupt register */
#define GINTSTS_CUR_MOD_BIT             0
#define GINTSTS_MODE_MIS_BIT            1
#define GINTSTS_OTG_INT_BIT             2
#define GINTSTS_SOF_BIT                 3
#define GINTSTS_RX_FLV_BIT              4
#define GINTMSK_NPTXF_EMPTY_BIT         5
#define GINTSTS_GIN_NAK_EFF_BIT         6
#define GINTSTS_GOUT_NAK_EFF_BIT        7
#define GINTSTS_ULPICK_INT_BIT          8
#define GINTSTS_I2C_INT_BIT             9
#define GINTSTS_ERLY_SUSP_BIT           10
#define GINTSTS_USB_SUSP_BIT            11
#define GINTSTS_USB_RST_BIT             12
#define GINTSTS_ENUM_DONE_BIT           13
#define GINTSTS_ISO_OUT_DROP_BIT        14
#define GINTSTS_EOPF_BIT                15
#define GINTSTS_EP_MIS_BIT              17
#define GINTSTS_IEP_INT_BIT             18
#define GINTSTS_OEP_INT_BIT             19
#define GINTSTS_INCOMP_IOS_IN_BIT       20
#define GINTSTS_INCOMP_ISO_OUT_BIT      21
#define GINTSTS_FET_SUSP_BIT            22
#define GINTSTS_CONID_STS_CHNG_BIT      28
#define GINTSTS_DISCONN_INT_BIT         29
#define GINTSTS_SESS_REQ_INT_BIT        30
#define GINTSTS_WKUP_INT_BIT            31

  volatile u32 gintmsk;                     
  /* Core interrupt mask register */
#define GINTMSK_MODE_MIS_MSK_BIT        1
#define GINTMSK_OTG_INT_MSK_BIT         2
#define GINTMSK_SOF_MSK_BIT             3
#define GINTMSK_RX_FLV_MSK_BIT          4
#define GINTMSK_NPTXF_EMPTY_MSK_BIT     5
#define GINTMSK_GIN_NAK_EFF_MSK_BIT     6
#define GINTMSK_GOUT_NAK_EFF_MSK_BIT    7
#define GINTMSK_ULPICK_INT_MSK_BIT      8
#define GINTMSK_I2C_INT_MSK_BIT         9
#define GINTMSK_ERLY_SUSP_MSK_BIT       10
#define GINTMSK_USB_SUSP_MSK_BIT        11
#define GINTMSK_USB_RST_MSK_BIT         12
#define GINTMSK_ENUM_DONE_MSK_BIT       13
#define GINTMSK_ISO_OUT_DROP_MSK_BIT    14
#define GINTMSK_EOPF_MSK_BIT            15
#define GINTMSK_EP_MIS_MSK_BIT          17
#define GINTMSK_INEP_INT_MSK_BIT        18
#define GINTMSK_OEP_INT_MSK_BIT         19
#define GINTMSK_INCOMP_IOS_IN_MSK_BIT   20
#define GINTMSK_INCOMP_ISO_OUT_MSK_BIT  21
#define GINTMSK_FET_SUSP_MSK_BIT        22
#define GINTMSK_CONID_STS_CHNG_MSK_BIT  28
#define GINTMSK_DISCONN_INT_MSK_BIT     29
#define GINTMSK_SESS_REQ_INT_MSK_BIT    30
#define GINTMSK_WKUP_INT_MSK_BIT        31

  volatile u32 grxstsr;                     
  /* Status debug read register */
#define GRXSTSR_EPNUM_LBIT              0
#define GRXSTSR_EPNUM_HBIT              3
#define GRXSTSR_BCNT_LBIT               4
#define GRXSTSR_BCNT_HBIT               14
#define GRXSTSR_DPID_LBIT               15
#define GRXSTSR_DPID_HBIT               16
#define GRXSTSR_PKTSTS_LBIT             17
#define GRXSTSR_PKTSTS_HBIT             20

  volatile u32 grxstsp;                     
  /* Status debug  pop register */
#define GRXSTSP_EPNUM_LBIT              0
#define GRXSTSP_EPNUM_HBIT              3
#define GRXSTSP_BCNT_LBIT               4
#define GRXSTSP_BCNT_HBIT               14
#define GRXSTSP_DPID_LBIT               15
#define GRXSTSP_DPID_HBIT               16
#define GRXSTSP_PKTSTS_LBIT             17
#define GRXSTSP_PKTSTS_HBIT             20
#define GRXSTSP_FN_LBIT                 21
#define GRXSTSP_FN_HBIT                 24

  volatile u32 grxfsiz;                     
  /* Receive fifo size register */
#define GRXFSIZ_RX_FDEP_LBIT            0
#define GRXFSIZ_RX_FDEP_HBIT            15

  volatile u32 gnptxfsiz;                   
  /* Non-periodic transmit fifo size register */
#define GNPTXFSIZ_NPTX_FST_ADDR_LBIT    0
#define GNPTXFSIZ_NPTX_FST_ADDR_HBIT    15
#define GNPTXFSIZ_NPTX_FDEP_LBIT        16
#define GNPTXFSIZ_NPTX_FDEP_HBIT        31

  volatile u32 gnptxsts;                    
  /* Non-periodic transmit fifo/queue status register */
#define GNPTXSTS_NPTX_FSPC_AVAIL_LBIT   0
#define GNPTXSTS_NPTX_FSPC_AVAIL_HBIT   15
#define GNPTXSTS_NPTX_QSPC_AVAIL_LBIT   16
#define GNPTXSTS_NPTX_QSPC_AVAIL_HBIT   23
#define GNPTXSTS_NPTX_QTOP_TERM_BIT     24
#define GNPTXSTS_NPTX_QTOP_TKN_LBIT     25
#define GNPTXSTS_NPTX_QTOP_TKN_HBIT     26
#define GNPTXSTS_NPTX_QTOP_INOUT        0
#define GNPTXSTS_NPTX_QTOP_ZERO         1
#define GNPTXSTS_NPTX_QTOP_PINGSPLIT    2
#define GNPTXSTS_NPTX_QTOP_HALT         3
#define GNPTXSTS_NPTX_QTOP_EPNUM_LBIT   27
#define GNPTXSTS_NPTX_QTOP_EPNUM_HBIT   30

  volatile u32 gi2cctl;                     
  /* I2C access register */
  volatile u32 gpvndctl;                    
  /* PHY vendor control register */
  volatile u32 ggpio;                       
  /* General purpose io register */
  volatile u32 guid;                        
  /* User id register */
#define GUID_USER_ID_LBIT               0
#define GUID_USER_ID_HBIT               31

  volatile u32 gsnpsid;                     
  /* Synopsys id register */
#define GSNPSID_SYNOPSYS_ID_LBIT        0
#define GSNPSID_SYNOPSYS_ID_HBIT        31

  volatile u32 ghwcfg1;                     
  /* User hw config1 register */
#define GHWCFG1_EP_DIR_BIDIR            0
#define GHWCFG1_EP_DIR_IN               1
#define GHWCFG1_EP_DIR_OUT              2

  volatile u32 ghwcfg2;                     
  /* User hw config2 register */
#define GHWCFG2_OTG_MODE_LBIT           0
#define GHWCFG2_OTG_MODE_HBIT           2
#define GHWCFG2_OTG_MODE_HNP_SRP_BOTH   0
#define GHWCFG2_OTG_MODE_SRP_BOTH       1
#define GHWCFG2_OTG_MODE_NON_OTG_BOTH   2
#define GHWCFG2_OTG_MODE_SRP_DEV        3
#define GHWCFG2_OTG_MODE_NON_OTG_DEV    4
#define GHWCFG2_OTG_MODE_SRP_HOST       5
#define GHWCFG2_OTG_MODE_NON_OTG_HOST   6
#define GHWCFG2_OTG_ARCH_LBIT           3
#define GHWCFG2_OTG_ARCH_HBIT           4
#define GHWCFG2_OTG_ARCH_SLAVE          0
#define GHWCFG2_OTG_ARCH_EXT_DMA        1
#define GHWCFG2_OTG_ARCH_INT_DMA        2
#define GHWCFG2_HS_PHY_TYPE_LBIT        6
#define GHWCFG2_HS_PHY_TYPE_HBIT        7
#define GHWCFG2_HS_PHY_TYPE_NONE        0
#define GHWCFG2_HS_PHY_TYPE_UTMI        1
#define GHWCFG2_HS_PHY_TYPE_ULPI        2
#define GHWCFG2_HS_PHY_TYPE_BOTH        3
#define GHWCFG2_FS_PHY_TYPE_LBIT        8
#define GHWCFG2_FS_PHY_TYPE_HBIT        9
#define GHWCFG2_FS_PHY_TYPE_NONE        0
#define GHWCFG2_FS_PHY_TYPE_DEDICATED   1
#define GHWCFG2_FS_PHY_TYPE_SHARE_UTMI  2
#define GHWCFG2_FS_PHY_TYPE_SHARE_ULPI  3
#define GHWCFG2_NUM_DEV_EPS_LBIT        10
#define GHWCFG2_NUM_DEV_EPS_HBIT        13
#define GHWCFG2_DYN_FIFO_SIZING_BIT     19
#define GHWCFG2_NPTX_QDEPTH_LBIT        22
#define GHWCFG2_NPTX_QDEPTH_HBIT        22
#define GHWCFG2_TKN_QDEPTH_LBIT         26
#define GHWCFG2_TKN_QDEPTH_HBIT         29

  volatile u32 ghwcfg3;                     
  /* User hw config3 register */
#define GHWCFG3_XFER_SIZE_WIDTH_LBIT    0
#define GHWCFG3_XFER_SIZE_WIDTH_HBIT    3
#define GHWCFG3_PKT_SIZE_WIDTH_LBIT     4
#define GHWCFG3_PKT_SIZE_WIDTH_HBIT     6
#define GHWCFG3_OTG_EN_BIT              7
#define GHWCFG3_DFIFO_DEPTH_LBIT        16
#define GHWCFG3_DFIFO_DEPTH_HBIT        31

  volatile u32 ghwcfg4;                     
  /* User hw config4 register */
#define GHWCFG4_NUM_DEV_PERIO_EPS_LBIT  0
#define GHWCFG4_NUM_DEV_PERIO_EPS_HBIT  3
#define GHWCFG4_PHY_DATA_WIDTH_LBIT     14
#define GHWCFG4_PHY_DATA_WIDTH_HBIT     15
#define GHWCFG4_PHY_DATA_WIDTH_8        0
#define GHWCFG4_PHY_DATA_WIDTH_16       1
#define GHWCFG4_PHY_DATA_WIDTH_SEL      2
#define GHWCFG4_NUM_CTL_EPS_LBIT        16
#define GHWCFG4_NUM_CTL_EPS_HBIT        19

  volatile u32 _unused[44];                 
  /* not used */

  /* offset 0x104 */
  volatile u32 dptxfsiz[15];                
  /* Device periodic transmit fifo n size register */
#define DPTXFSIZ_DPTX_FST_ADDR_LBIT     0
#define DPTXFSIZ_DPTX_FST_ADDR_HBIT     15
#define DPTXFSIZ_DPTX_FSIZE_LBIT        16
#define DPTXFSIZ_DPTX_FSIZE_HBIT        31
} __attribute__ ((packed));


/* power and clock gating registers */
struct GADGET_NAME(__GADGET,_pwclk_regs) {

  /* offset 0xE00 */
  volatile u32 pcgcctl;                     
  /* Power and clock gating control register */
} __attribute__ ((packed));


/* device mode global registers */
struct GADGET_NAME(__GADGET,_usbd_regs) {

  /* offset 0x800 */
  volatile u32 dcfg;                        
  /* Device configuration register */
#define DCFG_DEV_SPD_LBIT               0
#define DCFG_DEV_SPD_HBIT               1
#define DCFG_DEV_SPD_HS                 0
#define DCFG_DEV_SPD_FS20               1
#define DCFG_DEV_SPD_LS                 2
#define DCFG_DEV_SPD_FS11               3
#define DCFG_NZ_STS_OUT_HSHK_BIT        2
#define DCFG_DEV_ADDR_LBIT              4
#define DCFG_DEV_ADDR_HBIT              10
#define DCFG_PER_FRINT_LBIT             11
#define DCFG_PER_FRINT_HBIT             12
#define DCFG_PER_FRINT_80               0
#define DCFG_PER_FRINT_85               1
#define DCFG_PER_FRINT_90               2
#define DCFG_PER_FRINT_95               3
#define DCFG_EP_MIS_CNT_LBIT            18
#define DCFG_EP_MIS_CNT_HBIT            22

  volatile u32 dctl;                        
  /* Device control register */
#define DCTL_RMT_WKUP_SIG_BIT           0
#define DCTL_SFT_DISCON_BIT             1
#define DCTL_GNPIN_NAK_STS_BIT          2
#define DCTL_GOUT_NAK_STS_BIT           3
#define DCTL_SGNPIN_NAK_BIT             7
#define DCTL_CGNPIN_NAK_BIT             8
#define DCTL_SGOUT_NAK_BIT              9
#define DCTL_CGOUT_NAK_BIT              10

  volatile u32 dsts;                        
  /* Device status register */
#define DSTS_SUSP_STS_BIT               0
#define DSTS_ENUM_SPD_LBIT              1
#define DSTS_ENUM_SPD_HBIT              2
#define DSTS_ENUM_SPD_HS                0
#define DSTS_ENUM_SPD_FS20              1
#define DSTS_ENUM_SPD_LS                2
#define DSTS_ENUM_SPD_FS11              3
#define DSTS_ERRTIC_ERR_BIT             3
#define DSTS_SOF_FN_LBIT                8
#define DSTS_SOF_FN_HBIT                21

  volatile u32 _unused;                     
  /* not used */

  volatile u32 diepmsk;                     
  /* Device in endpoint common interrupt mask register */
#define DIEPMSK_XFER_COMPL_MSK          0
#define DIEPMSK_EP_DISBLD_MSK           1
#define DIEPMSK_AHB_ERR_MSK             2
#define DIEPMSK_TIMEOUT_MSK             3
#define DIEPMSK_INTKN_TXF_EMP_MSK       4
#define DIEPMSK_INTKN_EP_MIS_MSK        5
#define DIEPMSK_INEP_NAK_EFF_MSK        6

  volatile u32 doepmsk;                     
  /* Device out endpoint common interrupt mask register */
#define DOEPMSK_XFER_COMPL_MSK          0
#define DOEPMSK_EP_DISBLD_MSK           1
#define DOEPMSK_AHB_ERR_MSK             2
#define DOEPMSK_SETUP_MSK               3
#define DOEPMSK_OUTTKN_EP_DIS_MSK       4

  volatile u32 daint;                       
  /* Device all endpoints interrupt register */
#define DAINT_IN_EP0_INT_BIT            0
#define DAINT_IN_EP1_INT_BIT            1
#define DAINT_IN_EP2_INT_BIT            2
#define DAINT_IN_EP3_INT_BIT            3
#define DAINT_IN_EP4_INT_BIT            4
#define DAINT_IN_EP5_INT_BIT            5
#define DAINT_IN_EP6_INT_BIT            6
#define DAINT_IN_EP7_INT_BIT            7
#define DAINT_IN_EP8_INT_BIT            8
#define DAINT_IN_EP9_INT_BIT            9
#define DAINT_IN_EP10_INT_BIT           10
#define DAINT_IN_EP11_INT_BIT           11
#define DAINT_IN_EP12_INT_BIT           12
#define DAINT_IN_EP13_INT_BIT           13
#define DAINT_IN_EP14_INT_BIT           14
#define DAINT_IN_EP15_INT_BIT           15
#define DAINT_OUT_EP0_INT_BIT           16
#define DAINT_OUT_EP1_INT_BIT           17
#define DAINT_OUT_EP2_INT_BIT           18
#define DAINT_OUT_EP3_INT_BIT           19
#define DAINT_OUT_EP4_INT_BIT           20
#define DAINT_OUT_EP5_INT_BIT           21
#define DAINT_OUT_EP6_INT_BIT           22
#define DAINT_OUT_EP7_INT_BIT           23
#define DAINT_OUT_EP8_INT_BIT           24
#define DAINT_OUT_EP9_INT_BIT           25
#define DAINT_OUT_EP10_INT_BIT          26
#define DAINT_OUT_EP11_INT_BIT          27
#define DAINT_OUT_EP12_INT_BIT          28
#define DAINT_OUT_EP13_INT_BIT          29
#define DAINT_OUT_EP14_INT_BIT          30
#define DAINT_OUT_EP15_INT_BIT          31

  volatile u32 daintmsk;                    
  /* Device all endpoints interrupt mask register */
#define DAINTMSK_IN_EP0_MSK_BIT         0
#define DAINTMSK_IN_EP1_MSK_BIT         1
#define DAINTMSK_IN_EP2_MSK_BIT         2
#define DAINTMSK_IN_EP3_MSK_BIT         3
#define DAINTMSK_IN_EP4_MSK_BIT         4
#define DAINTMSK_IN_EP5_MSK_BIT         5
#define DAINTMSK_IN_EP6_MSK_BIT         6
#define DAINTMSK_IN_EP7_MSK_BIT         7
#define DAINTMSK_IN_EP8_MSK_BIT         8
#define DAINTMSK_IN_EP9_MSK_BIT         9
#define DAINTMSK_IN_EP10_MSK_BIT        10
#define DAINTMSK_IN_EP11_MSK_BIT        11
#define DAINTMSK_IN_EP12_MSK_BIT        12
#define DAINTMSK_IN_EP13_MSK_BIT        13
#define DAINTMSK_IN_EP14_MSK_BIT        14
#define DAINTMSK_IN_EP15_MSK_BIT        15
#define DAINTMSK_OUT_EP0_MSK_BIT        16
#define DAINTMSK_OUT_EP1_MSK_BIT        17
#define DAINTMSK_OUT_EP2_MSK_BIT        18
#define DAINTMSK_OUT_EP3_MSK_BIT        19
#define DAINTMSK_OUT_EP4_MSK_BIT        20
#define DAINTMSK_OUT_EP5_MSK_BIT        21
#define DAINTMSK_OUT_EP6_MSK_BIT        22
#define DAINTMSK_OUT_EP7_MSK_BIT        23
#define DAINTMSK_OUT_EP8_MSK_BIT        24
#define DAINTMSK_OUT_EP9_MSK_BIT        25
#define DAINTMSK_OUT_EP10_MSK_BIT       26
#define DAINTMSK_OUT_EP11_MSK_BIT       27
#define DAINTMSK_OUT_EP12_MSK_BIT       28
#define DAINTMSK_OUT_EP13_MSK_BIT       29
#define DAINTMSK_OUT_EP14_MSK_BIT       30
#define DAINTMSK_OUT_EP15_MSK_BIT       31

  volatile u32 dtknqr1;                     
  /* Device in token sequence learning queue read 1 register */
#define DTKNQR1_TKN0_LBIT		8
#define DTKNQR1_TKN0_HBIT		11

  volatile u32 dtknqr2;                     
  /* Device in token sequence learning queue read 2 register */
  volatile u32 dvbusdis;                    
  /* Device vbus discharge time register */
  volatile u32 dvbuspulse;                  
  /* Device vbus pulsing time register */
  volatile u32 dtknqr3;                     
  /* Device in token sequence learning queue read 3 register */
  volatile u32 dtknqr4;                     
  /* Device in token sequence learning queue read 4 register */
} __attribute__ ((packed));

/* device logical in endpoint n specific registers */
struct GADGET_NAME(__GADGET,_ep_in_regs) {

  /* offset 0x900 + n*0x20 (0 <= n <= 15) */
  volatile u32 diepctl;                     
  /* Device in endpoint n control register */
#define DIEPCTL_MPS_LBIT                0
#define DIEPCTL_MPS_HBIT_N              10
#define DIEPCTL_MPS_HBIT_0              10               /* for ep0 2 bit instead of 11 */
#define DIEPCTL_MPS_EP0_LS              3                /* for ep0, 8 */
#define DIEPCTL_MPS_EP0_HSFS            0                /* for ep0, 64 */
#define DIEPCTL_NEXT_EP_LBIT            11
#define DIEPCTL_NEXT_EP_HBIT            14
#define DIEPCTL_USB_ACT_EP_BIT          15               /* for ep0 always 1'b1 */
#define DIEPCTL_DPID_BIT                16               /* for ep0 reserved */
#define DIEPCTL_NAK_STS_BIT             17
#define DIEPCTL_EP_TYPE_LBIT            18               /* for ep0 always 2'h0 */
#define DIEPCTL_EP_TYPE_HBIT            19
#define DIEPCTL_EP_TYPE_CTRL            0
#define DIEPCTL_EP_TYPE_ISO             1
#define DIEPCTL_EP_TYPE_BULK            2
#define DIEPCTL_EP_TYPE_INT             3
#define DIEPCTL_STALL_BIT               21
#define DIEPCTL_TX_FNUM_LBIT            22               /* for ep0 always 4'h0 */
#define DIEPCTL_TX_FNUM_HBIT            25
#define DIEPCTL_CNAK_BIT                26
#define DIEPCTL_SNAK_BIT                27
#define DIEPCTL_SET_D0PID_BIT           28               /* for ep0 reserved */
#define DIEPCTL_SET_D1PID_BIT           29               /* for ep0 reserved */
#define DIEPCTL_EP_DIS_BIT              30
#define DIEPCTL_EP_ENA_BIT              31

  volatile u32 _unused0;                    
  /* not used */

  volatile u32 diepint;                     
  /* Device in endpoint n interrupt register */
#define DIEPINT_XFER_COMPL_BIT          0
#define DIEPINT_EP_DISBLD_BIT           1
#define DIEPINT_AHB_ERR_BIT             2
#define DIEPINT_TIMEOUT_BIT             3
#define DIEPINT_IN_TKN_TXF_EMP_BIT      4
#define DIEPINT_IN_TKN_EP_MIS_BIT       5
#define DIEPINT_IN_EP_NAK_EFF_BIT       6                /* for periodic ep only*/

  volatile u32 _unused1;                    
  /* not used */

  volatile u32 dieptsiz;                    
  /* Device in endpoint n transfer size register */
#define DIEPTSIZ_XFER_SIZE_LBIT         0
#define DIEPTSIZ_XFER_SIZE_HBIT_0       6
#define DIEPTSIZ_XFER_SIZE_HBIT_N       18
#define DIEPTSIZ_PKT_CNT_LBIT           19
#define DIEPTSIZ_PKT_CNT_HBIT_0         19               /* for ep0 1 instead of 10 bit */
#define DIEPTSIZ_PKT_CNT_HBIT_N         28
#define DIEPTSIZ_MC_LBIT                29
#define DIEPTSIZ_MC_HBIT                30

  volatile u32 diepdma;                     
  /* Device in endpoint n dma address rgister */
#define DIEPDMA_DMA_ADDR_LBIT           0
#define DIEPDMA_DMA_ADDR_HBIT           31

  volatile u32 _unused2;                    
  /* not used */
  volatile u32 _unused3;                    
  /* not used */
} __attribute__ ((packed));


/* device logical out endpoint n specific registers */
struct GADGET_NAME(__GADGET,_ep_out_regs) {
  /* offset 0xb00 + n*0x20 (0 <= n <= 15) */
  volatile u32 doepctl;                     
  /* Device out endpoint n control register */
#define DOEPCTL_MPS_LBIT                0
#define DOEPCTL_MPS_HBIT_0              1                /* for ep0 2 bit instead of 10 */
#define DOEPCTL_MPS_HBIT_N              10
#define DOEPCTL_MPS_EP0_LS              3                /* for ep0, 8 */
#define DOEPCTL_MPS_EP0_HSFS            0                /* for ep0, 64 */
#define DOEPCTL_USB_ACT_EP_BIT          15               /* for ep0 always 1'b1 */
#define DOEPCTL_DPID_BIT                16               /* for ep0 reserved */
#define DOEPCTL_NAK_STS_BIT             17
#define DOEPCTL_EP_TYPE_LBIT            18               /* for ep0 always 2'h0*/
#define DOEPCTL_EP_TYPE_HBIT            19
#define DOEPCTL_EP_TYPE_CTRL            0
#define DOEPCTL_EP_TYPE_ISO             1
#define DOEPCTL_EP_TYPE_BULK            2
#define DOEPCTL_EP_TYPE_INT             3
#define DOEPCTL_STALL_BIT               21
#define DOEPCTL_CNAK_BIT                26
#define DOEPCTL_SNAK_BIT                27
#define DOEPCTL_SET_D0PID_BIT           28               /* for ep0 reserved */
#define DOEPCTL_SET_D1PID_BIT           29               /* for ep0 reserved */
#define DOEPCTL_EP_DIS_BIT              30               /* for ep0 always 1'b1 */
#define DOEPCTL_EP_ENA_BIT              31

  volatile u32 _unused0;                    
  /* not used */

  volatile u32 doepint;                     
  /* Device out endpoint n interrupt register */
#define DOEPINT_XFER_COMPL_BIT          0
#define DOEPINT_EP_DISBLD_BIT           1
#define DOEPINT_AHB_ERR_BIT             2
#define DOEPINT_SETUP_BIT               3                /* only for ep0 */
#define DOEPINT_OUTTKN_EP_DIS_BIT       4                /* only for ep0 */

  volatile u32 _unused1;                    
  /* not used */

  volatile u32 doeptsiz;                    
  /* Device out endpoint n transfer size register */
#define DOEPTSIZ_XFER_SIZE_LBIT         0
#define DOEPTSIZ_XFER_SIZE_HBIT_0       6                /* for ep0 6 instead of 19 bit */
#define DOEPTSIZ_XFER_SIZE_HBIT_N       18
#define DOEPTSIZ_PKT_CNT_LBIT           19
#define DOEPTSIZ_PKT_CNT_HBIT_0         19               /* for ep0 1 instead of 10 bit */
#define DOEPTSIZ_PKT_CNT_HBIT_N         28
#define DOEPTSIZ_SUP_CNT_LBIT           29               /* only for ep0 */
#define DOEPTSIZ_SUP_CNT_HBIT           30               /* only for ep0 */
#define DOEPTSIZ_SUP_CNT_DEFAULT        3                /* only for ep0 */

  volatile u32 doepdma;                     
  /* Device out endpoint n dma address rgister */
#define DOEPDMA_DMA_ADDR_LBIT           0
#define DOEPDMA_DMA_ADDR_HBIT           31

  volatile u32 _unused2;                    
  /* not used */
  volatile u32 _unused3;                    
  /* not used */
} __attribute__ ((packed));

/* RCU registers: 0xBF203000 - ? */

struct GADGET_NAME(__GADGET,_rcu_regs) {
  volatile u32 _unused0[6];
  /* not used */

  volatile u32 usbcfg;
  /* USB configure register */
#define USBCFG_PHY_SEL_BIT              12
#define USBCFG_HOST_DEVICE_BIT          11
#define USBCFG_HOST_END_BIT             10
#define USBCFG_DEV_END_BIT              9
#define USBCFG_ARBITER_PAUSE            8

  volatile u32 _unused1[3];
  /* not used */

  volatile u32 pcirdy;
  /* PCI boot ready register */
#define USBCFG_SET_PCI_RDY_BIT          0

} __attribute__ ((packed));

/* PMU registers: 0xBF102000 - ? */

struct GADGET_NAME(__GADGET,_pmu_regs) {
  volatile u32 _unused0[7];
  volatile u32 pwdcr;
} __attribute__ ((packed));

/* CGU registers: 0xBF103000 - ? */

struct GADGET_NAME(__GADGET,_cgu_regs) {
  volatile u32 _unused0[6];
  volatile u32 ifccr;
} __attribute__ ((packed));

/* GPIO registers: 0xBE100B00 - ? */

struct GADGET_NAME(__GADGET,_gpio_regs) {
  volatile u32 _unused0[4];
  volatile u32 p0_out;
  volatile u32 _unused1;
  volatile u32 p0_dir;
  volatile u32 p0_altsel0;
  volatile u32 p0_altsel1;
  volatile u32 p0_od;
  volatile u32 _unused2;
  volatile u32 p0_pudsel;
  volatile u32 p0_puden;
} __attribute__ ((packed));
