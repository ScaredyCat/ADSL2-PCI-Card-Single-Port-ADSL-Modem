#ifndef __DANUBE_PPE_DEV_ADDR_H__2005_08_11__17_00__
#define __DANUBE_PPE_DEV_ADDR_H__2005_08_11__17_00__

/*
 *  PP32 Data Memory Mapped Registers
 */
#define PPE_BASE_ADDR                   (KSEG1 + 0x20106000)
#define PPE_REG_ADDR(x)                 ((volatile unsigned long*)(PPE_BASE_ADDR + ((0x4000 + x) << 2)))
/*
 *    PP32 Power Saving Register
 */
#define PP32_SLEEP                      PPE_REG_ADDR(0x0010)
/*
 *    Code/Data Memory (CDM) Register
 */
#define CDM_CFG                         PPE_REG_ADDR(0x0100)
/*
 *    Mailbox Registers
 */
#define MBOX_IGU0_ISRS                  PPE_REG_ADDR(0x0200)
#define MBOX_IGU0_ISRC                  PPE_REG_ADDR(0x0201)
#define MBOX_IGU0_ISR                   PPE_REG_ADDR(0x0202)
#define MBOX_IGU0_IER                   PPE_REG_ADDR(0x0203)
#define MBOX_IGU1_ISRS0                 PPE_REG_ADDR(0x0204)
#define MBOX_IGU1_ISRC0                 PPE_REG_ADDR(0x0205)
#define MBOX_IGU1_ISR0                  PPE_REG_ADDR(0x0206)
#define MBOX_IGU1_IER0                  PPE_REG_ADDR(0x0207)
#define MBOX_IGU1_ISRS1                 PPE_REG_ADDR(0x0208)
#define MBOX_IGU1_ISRC1                 PPE_REG_ADDR(0x0209)
#define MBOX_IGU1_ISR1                  PPE_REG_ADDR(0x020A)
#define MBOX_IGU1_IER1                  PPE_REG_ADDR(0x020B)
#define MBOX_IGU1_ISRS2                 PPE_REG_ADDR(0x020C)
#define MBOX_IGU1_ISRC2                 PPE_REG_ADDR(0x020D)
#define MBOX_IGU1_ISR2                  PPE_REG_ADDR(0x020E)
#define MBOX_IGU1_IER2                  PPE_REG_ADDR(0x020F)
#define MBOX_IGU2_ISRS                  PPE_REG_ADDR(0x0210)
#define MBOX_IGU2_ISRC                  PPE_REG_ADDR(0x0211)
#define MBOX_IGU2_ISR                   PPE_REG_ADDR(0x0212)
#define MBOX_IGU2_IER                   PPE_REG_ADDR(0x0213)
#define MBOX_IGU3_ISRS                  PPE_REG_ADDR(0x0214)
#define MBOX_IGU3_ISRC                  PPE_REG_ADDR(0x0215)
#define MBOX_IGU3_ISR                   PPE_REG_ADDR(0x0216)
#define MBOX_IGU3_IER                   PPE_REG_ADDR(0x0217)
#define MBOX_IGU4_ISRS                  PPE_REG_ADDR(0x0218)
#define MBOX_IGU4_ISRC                  PPE_REG_ADDR(0x0219)
#define MBOX_IGU4_ISR                   PPE_REG_ADDR(0x021A)
#define MBOX_IGU4_IER                   PPE_REG_ADDR(0x021B)
/*
 *    Shared Buffer (SB) Registers
 */
#define SB_MST_PRI0                     PPE_REG_ADDR(0x0300)
#define SB_MST_PRI1                     PPE_REG_ADDR(0x0301)
#define SB_MST_PRI2                     PPE_REG_ADDR(0x0302)
#define SB_MST_PRI3                     PPE_REG_ADDR(0x0303)
#define SB_MST_PRI4                     PPE_REG_ADDR(0x0304)
#define SB_MST_SEL                      PPE_REG_ADDR(0x0305)
/*
 *    RTHA Registers
 */
#define RFBI_CFG                        PPE_REG_ADDR(0x0400)
#define RBA_CFG0                        PPE_REG_ADDR(0x0404)
#define RBA_CFG1                        PPE_REG_ADDR(0x0405)
#define RCA_CFG0                        PPE_REG_ADDR(0x0408)
#define RCA_CFG1                        PPE_REG_ADDR(0x0409)
#define RDES_CFG0                       PPE_REG_ADDR(0x040C)
#define RDES_CFG1                       PPE_REG_ADDR(0x040D)
#define SFSM_STATE0                     PPE_REG_ADDR(0x0410)
#define SFSM_STATE1                     PPE_REG_ADDR(0x0411)
#define SFSM_DBA0                       PPE_REG_ADDR(0x0412)
#define SFSM_DBA1                       PPE_REG_ADDR(0x0413)
#define SFSM_CBA0                       PPE_REG_ADDR(0x0414)
#define SFSM_CBA1                       PPE_REG_ADDR(0x0415)
#define SFSM_CFG0                       PPE_REG_ADDR(0x0416)
#define SFSM_CFG1                       PPE_REG_ADDR(0x0417)
#define SFSM_PGCNT0                     PPE_REG_ADDR(0x041C)
#define SFSM_PGCNT1                     PPE_REG_ADDR(0x041D)
/*
 *    TTHA Registers
 */
#define FFSM_DBA0                       PPE_REG_ADDR(0x0508)
#define FFSM_DBA1                       PPE_REG_ADDR(0x0509)
#define FFSM_CFG0                       PPE_REG_ADDR(0x050A)
#define FFSM_CFG1                       PPE_REG_ADDR(0x050B)
#define FFSM_IDLE_HEAD_BC0              PPE_REG_ADDR(0x050E)
#define FFSM_IDLE_HEAD_BC1              PPE_REG_ADDR(0x050F)
#define FFSM_PGCNT0                     PPE_REG_ADDR(0x0514)
#define FFSM_PGCNT1                     PPE_REG_ADDR(0x0515)
/*
 *    ETOP MDIO Registers
 */
#define ETOP_MDIO_CFG                   PPE_REG_ADDR(0x0600)
#define ETOP_MDIO_ACC                   PPE_REG_ADDR(0x0601)
#define ETOP_CFG                        PPE_REG_ADDR(0x0602)
#define ETOP_IG_VLAN_COS                PPE_REG_ADDR(0x0603)
#define ETOP_IG_DSCP_COS3               PPE_REG_ADDR(0x0604)
#define ETOP_IG_DSCP_COS2               PPE_REG_ADDR(0x0605)
#define ETOP_IG_DSCP_COS1               PPE_REG_ADDR(0x0606)
#define ETOP_IG_DSCP_COS0               PPE_REG_ADDR(0x0607)
#define ETOP_IG_PLEN_CTRL0              PPE_REG_ADDR(0x0608)
#define ETOP_IG_PLEN_CTRL1              PPE_REG_ADDR(0x0609)
#define ETOP_ISR                        PPE_REG_ADDR(0x060A)
#define ETOP_IER                        PPE_REG_ADDR(0x060B)
#define ETOP_VPID                       PPE_REG_ADDR(0x060C)
#define ENET_MAC_CFG                    PPE_REG_ADDR(0x0610)
#define ENETS_DBA                       PPE_REG_ADDR(0x0612)
#define ENETS_CBA                       PPE_REG_ADDR(0x0613)
#define ENETS_CFG                       PPE_REG_ADDR(0x0614)
#define ENETS_PGCNT                     PPE_REG_ADDR(0x0615)
#define ENETS_PKTCNT                    PPE_REG_ADDR(0x0616)
#define ENETS_BUF_CTRL                  PPE_REG_ADDR(0x0617)
#define ENETS_COS_CFG                   PPE_REG_ADDR(0x0618)
#define ENETS_IGDROP                    PPE_REG_ADDR(0x0619)
#define ENETF_DBA                       PPE_REG_ADDR(0x0630)
#define ENETF_CBA                       PPE_REG_ADDR(0x0631)
#define ENETF_CFG                       PPE_REG_ADDR(0x0632)
#define ENETF_PGCNT                     PPE_REG_ADDR(0x0633)
#define ENETF_PKTCNT                    PPE_REG_ADDR(0x0634)
#define ENETF_HFCTRL                    PPE_REG_ADDR(0x0635)
#define ENETF_TXCTRL                    PPE_REG_ADDR(0x0636)
#define ENETF_VLCOS0                    PPE_REG_ADDR(0x0638)
#define ENETF_VLCOS1                    PPE_REG_ADDR(0x0639)
#define ENETF_VLCOS2                    PPE_REG_ADDR(0x063A)
#define ENETF_VLCOS3                    PPE_REG_ADDR(0x063B)
#define ENETF_EGERR                     PPE_REG_ADDR(0x063C)
#define ENETF_EGDROP                    PPE_REG_ADDR(0x063D)
#define ENET_MAC_DA0                    PPE_REG_ADDR(0x063E)
#define ENET_MAC_DA1                    PPE_REG_ADDR(0x063F)
/*
 *    DPLUS Registers
 */
#define DPLUS_TXDB                      PPE_REG_ADDR(0x0700)
#define DPLUS_TXCB                      PPE_REG_ADDR(0x0701)
#define DPLUS_TXCFG                     PPE_REG_ADDR(0x0702)
#define DPLUS_TXPGCNT                   PPE_REG_ADDR(0x0703)
#define DPLUS_RXDB                      PPE_REG_ADDR(0x0710)
#define DPLUS_RXCB                      PPE_REG_ADDR(0x0711)
#define DPLUS_RXCFG                     PPE_REG_ADDR(0x0712)
#define DPLUS_RXPGCNT                   PPE_REG_ADDR(0x0713)
/*
 *    BMC Registers
 */
#define BMC_CMD3                        PPE_REG_ADDR(0x0800)
#define BMC_CMD2                        PPE_REG_ADDR(0x0801)
#define BMC_CMD1                        PPE_REG_ADDR(0x0802)
#define BMC_CMD0                        PPE_REG_ADDR(0x0803)
#define BMC_CFG0                        PPE_REG_ADDR(0x0804)
#define BMC_CFG1                        PPE_REG_ADDR(0x0805)
#define BMC_POLY0                       PPE_REG_ADDR(0x0806)
#define BMC_POLY1                       PPE_REG_ADDR(0x0807)
#define BMC_CRC0                        PPE_REG_ADDR(0x0808)
#define BMC_CRC1                        PPE_REG_ADDR(0x0809)
/*
 *    SLL Registers
 */
#define SLL_CMD1                        PPE_REG_ADDR(0x0900)
#define SLL_CMD0                        PPE_REG_ADDR(0x0901)
#define SLL_KEY0                        PPE_REG_ADDR(0x0910)
#define SLL_KEY1                        PPE_REG_ADDR(0x0911)
#define SLL_KEY2                        PPE_REG_ADDR(0x0912)
#define SLL_KEY3                        PPE_REG_ADDR(0x0913)
#define SLL_KEY4                        PPE_REG_ADDR(0x0914)
#define SLL_KEY5                        PPE_REG_ADDR(0x0915)
#define SLL_RESULT                      PPE_REG_ADDR(0x0920)
/*
 *    EMA Registers
 */
#define EMA_CMD2                        PPE_REG_ADDR(0x0A00)
#define EMA_CMD1                        PPE_REG_ADDR(0x0A01)
#define EMA_CMD0                        PPE_REG_ADDR(0x0A02)
#define EMA_ISR                         PPE_REG_ADDR(0x0A04)
#define EMA_IER                         PPE_REG_ADDR(0x0A05)
#define EMA_CFG                         PPE_REG_ADDR(0x0A06)
/*
 *    UTPS Registers
 */
#define UTP_TXCA0                       PPE_REG_ADDR(0x0B00)
#define UTP_TXNA0                       PPE_REG_ADDR(0x0B01)
#define UTP_TXCA1                       PPE_REG_ADDR(0x0B02)
#define UTP_TXNA1                       PPE_REG_ADDR(0x0B03)
#define UTP_RXCA0                       PPE_REG_ADDR(0x0B10)
#define UTP_RXNA0                       PPE_REG_ADDR(0x0B11)
#define UTP_RXCA1                       PPE_REG_ADDR(0x0B12)
#define UTP_RXNA1                       PPE_REG_ADDR(0x0B13)
#define UTP_CFG                         PPE_REG_ADDR(0x0B20)
#define UTP_ISR                         PPE_REG_ADDR(0x0B30)
#define UTP_IER                         PPE_REG_ADDR(0x0B31)
/*
 *    QSB Registers
 */
#define QSB_RELOG                       PPE_REG_ADDR(0x0C00)
#define QSB_EMIT0                       PPE_REG_ADDR(0x0C01)
#define QSB_EMIT1                       PPE_REG_ADDR(0x0C02)
#define QSB_ICDV                        PPE_REG_ADDR(0x0C07)
#define QSB_SBL                         PPE_REG_ADDR(0x0C09)
#define QSB_CFG                         PPE_REG_ADDR(0x0C0A)
#define QSB_RTM                         PPE_REG_ADDR(0x0C0B)
#define QSB_RTD                         PPE_REG_ADDR(0x0C0C)
#define QSB_RAMAC                       PPE_REG_ADDR(0x0C0D)
#define QSB_ISTAT                       PPE_REG_ADDR(0x0C0E)
#define QSB_IMR                         PPE_REG_ADDR(0x0C0F)
#define QSB_SRC                         PPE_REG_ADDR(0x0C10)
/*
 *    DSP User Registers
 */
#define DREG_A_VERSION                  PPE_REG_ADDR(0x0D00)
#define DREG_A_CFG                      PPE_REG_ADDR(0x0D01)
#define DREG_AT_CTRL                    PPE_REG_ADDR(0x0D02)
#define DREG_AR_CTRL                    PPE_REG_ADDR(0x0D08)
#define DREG_A_UTPCFG                   PPE_REG_ADDR(0x0D0E)
#define DREG_A_STATUS                   PPE_REG_ADDR(0x0D0F)
#define DREG_AT_CELL0                   PPE_REG_ADDR(0x0D24)
#define DREG_AT_CELL1                   PPE_REG_ADDR(0x0D25)
#define DREG_AT_IDLE_CNT0               PPE_REG_ADDR(0x0D26)
#define DREG_AT_IDLE_CNT1               PPE_REG_ADDR(0x0D27)
#define DREG_AT_IDLE0                   PPE_REG_ADDR(0x0D28)
#define DREG_AT_IDLE1                   PPE_REG_ADDR(0x0D29)
#define DREG_AR_CFG0                    PPE_REG_ADDR(0x0D60)
#define DREG_AR_CFG1                    PPE_REG_ADDR(0x0D61)
#define DREG_AR_ATM_STAT0               PPE_REG_ADDR(0x0D66)
#define DREG_AR_ATM_STAT1               PPE_REG_ADDR(0x0D67)
#define DREG_AR_CELL0                   PPE_REG_ADDR(0x0D68)
#define DREG_AR_CELL1                   PPE_REG_ADDR(0x0D69)
#define DREG_AR_IDLE_CNT0               PPE_REG_ADDR(0x0D6A)
#define DREG_AR_IDLE_CNT1               PPE_REG_ADDR(0x0D6B)
#define DREG_AR_AIIDLE_CNT0             PPE_REG_ADDR(0x0D6C)
#define DREG_AR_AIIDLE_CNT1             PPE_REG_ADDR(0x0D6D)
#define DREG_AR_BE_CNT0                 PPE_REG_ADDR(0x0D6E)
#define DREG_AR_BE_CNT1                 PPE_REG_ADDR(0x0D6F)
#define DREG_AR_HEC_CNT0                PPE_REG_ADDR(0x0D70)
#define DREG_AR_HEC_CNT1                PPE_REG_ADDR(0x0D71)
#define DREG_AR_CD_CNT0                 PPE_REG_ADDR(0x0D72)
#define DREG_AR_CD_CNT1                 PPE_REG_ADDR(0x0D73)
#define DREG_AR_IDLE0                   PPE_REG_ADDR(0x0D74)
#define DREG_AR_IDLE1                   PPE_REG_ADDR(0x0D75)
#define DREG_AR_DELIN0                  PPE_REG_ADDR(0x0D76)
#define DREG_AR_DELIN1                  PPE_REG_ADDR(0x0D77)
#define DREG_RESV0                      PPE_REG_ADDR(0x0D78)
#define DREG_RESV1                      PPE_REG_ADDR(0x0D79)
#define DREG_RX_MIB_CMD0                PPE_REG_ADDR(0x0D80)
#define DREG_RX_MIB_CMD1                PPE_REG_ADDR(0x0D81)
#define DREG_AR_OVDROP_CNT0             PPE_REG_ADDR(0x0D98)
#define DREG_AR_OVDROP_CNT1             PPE_REG_ADDR(0x0D99)

#endif //  __DANUBE_PPE_DEV_ADDR_H__2005_08_11__17_00__
