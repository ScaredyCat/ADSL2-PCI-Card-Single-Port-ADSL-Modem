/* $Id: isac.h,v 1.1.4.1 2001/11/20 14:19:36 kai Exp $
 *
 * ISAC specific defines
 *
 * Author       Karsten Keil
 * Copyright    by Karsten Keil      <keil@isdn4linux.de>
 * 
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 */

/* All Registers original Siemens Spec  */
#ifdef CONFIG_HISAX_PEB3086
                                       /* Registers of ISAC             */
#define	ISAC_XFIFOD             	(0x00)
#define	ISAC_RFIFOD             	(0x00)
#define	ISAC_ISTAD              	(0x20)
                                       /* Bit definition of ISTAD */
#define	ISTAD_XDU          	0x4
#define	ISTAD_XMR          	0x8
#define	ISTAD_XPR          	0x10
#define	ISTAD_RFO          	0x20
#define	ISTAD_RPF          	0x40
#define	ISTAD_RME          	0x80

#define	ISAC_MASKD              	(0x20)
                                       /* Bit definition of MASKD */
#define	MASKD_XDU          	0x4
#define	MASKD_XMR          	0x8
#define	MASKD_XPR          	0x10
#define	MASKD_RFO          	0x20
#define	MASKD_RPF          	0x40
#define	MASKD_RME          	0x80

#define	ISAC_STARD              	(0x21)
                                       /* Bit definition of STARD */
#define	STARD_XACI         	0x2
#define	STARD_RACI         	0x8
#define	STARD_XFW          	0x40
#define	STARD_XDOV         	0x80

#define	ISAC_CMDRD              	(0x21)
                                       /* Bit definition of CMDRD */
#define	CMDRD_XRES         	0x1
#define	CMDRD_XME          	0x2
#define	CMDRD_XTF          	0x8
#define	CMDRD_STI          	0x10
#define	CMDRD_RRES         	0x40
#define	CMDRD_RMC          	0x80

#define	ISAC_MODED              	(0x22)
                                       /* Bit definition of MODED */
#define	MODED_RAC          	0x8

#define	ISAC_EXMD1              	(0x23)
                                       /* Bit definition of EXMD1 */
#define	EXMD1_ITF          	0x1
#define	EXMD1_RCRC         	0x4
#define	EXMD1_XCRC         	0x8
#define	EXMD1_SRA          	0x10
#define	EXMD1_XFBS         	0x80

#define	ISAC_TIMR1              	(0x24)

#define	ISAC_SAP1               	(0x25)
                                       /* Bit definition of SAP1 */
#define	SAP1_MHA           	0x1

#define	ISAC_SAP2               	(0x26)
                                       /* Bit definition of SAP2 */
#define	SAP2_MLA           	0x1

#define	ISAC_RBCLD              	(0x26)

#define	ISAC_RBCHD              	(0x27)
                                       /* Bit definition of RBCHD */
#define	RBCHD_OV           	0x10

#define	ISAC_TEI1               	(0x27)
                                       /* Bit definition of TEI1 */
#define	TEI1_EA1           	0x1

#define	ISAC_TEI2               	(0x28)
                                       /* Bit definition of TEI2 */
#define	TEI2_EA2           	0x1

#define	ISAC_RSTAD              	(0x28)
                                       /* Bit definition of RSTAD */
#define	RSTAD_TA           	0x1
#define	RSTAD_C_R          	0x2
#define	RSTAD_SA0          	0x4
#define	RSTAD_SA1          	0x8
#define	RSTAD_RAB          	0x10
#define	RSTAD_CRC          	0x20
#define	RSTAD_RDO          	0x40
#define	RSTAD_VFR          	0x80

#define	ISAC_TMD                	(0x29)
                                       /* Bit definition of TMD */
#define	TMD_TLP            	0x1

#define	ISAC_CIR0               	(0x2e)
                                       /* Bit definition of CIR0 */
#define	CIR0_BAS           	0x1
#define	CIR0_S_G           	0x2
#define	CIR0_CIC1          	0x4
#define	CIR0_CIC0          	0x8

#define	ISAC_CIX0               	(0x2e)
                                       /* Bit definition of CIX0 */
#define	CIX0_BAC           	0x1
#define	CIX0_TBA0          	0x2
#define	CIX0_TBA1          	0x4
#define	CIX0_TBA2          	0x8

#define	ISAC_CIR1               	(0x2f)
                                       /* Bit definition of CIR1 */
#define	CIR1_CI1E          	0x1
#define	CIR1_CICW          	0x2

#define	ISAC_CIX1               	(0x2f)
                                       /* Bit definition of CIX1 */
#define	CIX1_CI1E          	0x1
#define	CIX1_CICW          	0x2

#define	ISAC_TR_CONF0           	(0x30)
                                       /* Bit definition of TR_CONF0 */
#define	TR_CONF0_LLD       	0x1
#define	TR_CONF0_EXLP      	0x2
#define	TR_CONF0_L1SW      	0x4
#define	TR_CONF0_EN_FECV   	0x8
#define	TR_CONF0_EN_ICV    	0x20
#define	TR_CONF0_BUS       	0x40
#define	TR_CONF0_DIS_TR    	0x80

#define	ISAC_TR_CONF1           	(0x31)
                                       /* Bit definition of TR_CONF1 */
#define	TR_CONF1_EN_SFSC   	0x20
#define	TR_CONF1_RPLL_ADJ  	0x40

#define	ISAC_TR_CONF2           	(0x32)
                                       /* Bit definition of TR_CONF2 */
#define	TR_CONF2_PDS       	0x40
#define	TR_CONF2_DIS_TX    	0x80

#define	ISAC_TR_STA             	(0x33)
                                       /* Bit definition of TR_STA */
#define	TR_STA_LD          	0x1
#define	TR_STA_FSYN        	0x4
#define	TR_STA_FECV        	0x10

#define	ISAC_TR_CMD             	(0x34)
                                       /* Bit definition of TR_CMD */
#define	TR_CMD_LP_A        	0x2
#define	TR_CMD_PD          	0x4
#define	TR_CMD_TDDIS       	0x8
#define	TR_CMD_DPRIO       	0x10

#define	ISAC_SQRR1              	(0x35)
                                       /* Bit definition of SQRR1 */
#define	SQRR1_SQR4         	0x1
#define	SQRR1_SQR3         	0x2
#define	SQRR1_SQR2         	0x4
#define	SQRR1_SQR1         	0x8
#define	SQRR1_MFEN         	0x40
#define	SQRR1_MSYN         	0x80

#define	ISAC_SQXR1              	(0x35)
                                       /* Bit definition of SQXR1 */
#define	SQXR1_SQX4         	0x1
#define	SQXR1_SQX3         	0x2
#define	SQXR1_SQX2         	0x4
#define	SQXR1_SQX1         	0x8
#define	SQXR1_MFEN         	0x40

#define	ISAC_SQXR2              	(0x36)
                                       /* Bit definition of SQXR2 */
#define	SQXR2_SQX34        	0x1
#define	SQXR2_SQX33        	0x2
#define	SQXR2_SQX32        	0x4
#define	SQXR2_SQX31        	0x8
#define	SQXR2_SQX24        	0x10
#define	SQXR2_SQX23        	0x20
#define	SQXR2_SQX22        	0x40
#define	SQXR2_SQX21        	0x80

#define	ISAC_SQRR2              	(0x36)
                                       /* Bit definition of SQRR2 */
#define	SQRR2_SQR34        	0x1
#define	SQRR2_SQR33        	0x2
#define	SQRR2_SQR32        	0x4
#define	SQRR2_SQR31        	0x8
#define	SQRR2_SQR24        	0x10
#define	SQRR2_SQR23        	0x20
#define	SQRR2_SQR22        	0x40
#define	SQRR2_SQR21        	0x80

#define	ISAC_SQXR3              	(0x37)
                                       /* Bit definition of SQXR3 */
#define	SQXR3_SQR54        	0x1
#define	SQXR3_SQR53        	0x2
#define	SQXR3_SQR52        	0x4
#define	SQXR3_SQR51        	0x8
#define	SQXR3_SQR44        	0x10
#define	SQXR3_SQR43        	0x20
#define	SQXR3_SQR42        	0x40
#define	SQXR3_SQR41        	0x80

#define	ISAC_SQRR3              	(0x37)
                                       /* Bit definition of SQRR3 */
#define	SQRR3_SQR54        	0x1
#define	SQRR3_SQR53        	0x2
#define	SQRR3_SQR52        	0x4
#define	SQRR3_SQR51        	0x8
#define	SQRR3_SQR44        	0x10
#define	SQRR3_SQR43        	0x20
#define	SQRR3_SQR42        	0x40
#define	SQRR3_SQR41        	0x80

#define	ISAC_ISTATR             	(0x38)
                                       /* Bit definition of ISTATR */
#define	ISTATR_SQW         	0x1
#define	ISTATR_SQC         	0x2
#define	ISTATR_RIC         	0x4
#define	ISTATR_LD          	0x8

#define	ISAC_MASKTR             	(0x39)
                                       /* Bit definition of MASKTR */
#define	MASKTR_SQW         	0x1
#define	MASKTR_SQC         	0x2
#define	MASKTR_RIC         	0x4
#define	MASKTR_LD          	0x8

#define	ISAC_TR_MODE            	(0x3a)
                                       /* Bit definition of TR_MODE */
#define	TR_MODE_MODE0      	0x1
#define	TR_MODE_MODE1      	0x2
#define	TR_MODE_MODE2      	0x4
#define	TR_MODE_DCH_INH    	0x8

#define	ISAC_ACFG1              	(0x3c)
                                       /* Bit definition of ACFG1 */
#define	ACFG1_OD0          	0x1
#define	ACFG1_OD1          	0x2
#define	ACFG1_OD2          	0x4
#define	ACFG1_OD3          	0x8
#define	ACFG1_OD4          	0x10
#define	ACFG1_OD5          	0x20
#define	ACFG1_OD6          	0x40
#define	ACFG1_OD7          	0x80

#define	ISAC_ACFG2              	(0x3d)
                                       /* Bit definition of ACFG2 */
#define	ACFG2_EL0          	0x1
#define	ACFG2_EL1          	0x2
#define	ACFG2_LED          	0x4
#define	ACFG2_ACL          	0x8
#define	ACFG2_A4SEL        	0x10
#define	ACFG2_FBS          	0x20
#define	ACFG2_A5SEL        	0x40
#define	ACFG2_A7SEL        	0x80

#define	ISAC_AOE                	(0x3e)
                                       /* Bit definition of AOE */
#define	AOE_OE0            	0x1
#define	AOE_OE1            	0x2
#define	AOE_OE2            	0x4
#define	AOE_OE3            	0x8
#define	AOE_OE4            	0x10
#define	AOE_OE5            	0x20
#define	AOE_OE6            	0x40
#define	AOE_OE7            	0x80

#define	ISAC_ARX                	(0x3f)
                                       /* Bit definition of ARX */
#define	ARX_AR0            	0x1
#define	ARX_AR1            	0x2
#define	ARX_AR2            	0x4
#define	ARX_AR3            	0x8
#define	ARX_AR4            	0x10
#define	ARX_AR5            	0x20
#define	ARX_AR6            	0x40
#define	ARX_AR7            	0x80

#define	ISAC_ATX                	(0x3f)
                                       /* Bit definition of ATX */
#define	ATX_AT0            	0x1
#define	ATX_AT1            	0x2
#define	ATX_AT2            	0x4
#define	ATX_AT3            	0x8
#define	ATX_AT4            	0x10
#define	ATX_AT5            	0x20
#define	ATX_AT6            	0x40
#define	ATX_AT7            	0x80

#define	ISAC_CDA10              	(0x40)
#define	ISAC_CDA11              	(0x41)
#define	ISAC_CDA20              	(0x42)
#define	ISAC_CDA21              	(0x43)
#define	ISAC_CDA_TSDP10         	(0x44)
                                       /* Bit definition of CDA_TSDP10 */
#define	CDA_TSDP10_DPS     	0x80

#define	ISAC_CDA_TSDP11         	(0x45)
                                       /* Bit definition of CDA_TSDP11 */
#define	CDA_TSDP11_DPS     	0x80

#define	ISAC_CDA_TSDP20         	(0x46)
                                       /* Bit definition of CDA_TSDP20 */
#define	CDA_TSDP20_DPS     	0x80

#define	ISAC_CDA_TSDP21         	(0x47)
                                       /* Bit definition of CDA_TSDP21 */
#define	CDA_TSDP21_DPS     	0x80

#define	ISAC_BCHA_TSDP_BC1      	(0x48)
                                       /* Bit definition of BCHA_TSDP_BC1 */
#define	BCHA_TSDP_BC1_DPS  	0x80

#define	ISAC_BCHA_TSDP_BC2      	(0x49)
                                       /* Bit definition of BCHA_TSDP_BC2 */
#define	BCHA_TSDP_BC2_DPS  	0x80

#define	ISAC_TR_TSDP_BC1        	(0x4c)
                                       /* Bit definition of TR_TSDP_BC1 */
#define	TR_TSDP_BC1_DPS    	0x80

#define	ISAC_TR_TSDP_BC2        	(0x4d)
                                       /* Bit definition of TR_TSDP_BC2 */
#define	TR_TSDP_BC2_DPS    	0x80

#define	ISAC_CDA1_CR            	(0x4e)
                                       /* Bit definition of CDA1_CR */
#define	CDA1_CR_SWAP       	0x1
#define	CDA1_CR_EN_O0      	0x2
#define	CDA1_CR_EN_O1      	0x4
#define	CDA1_CR_EN_I0      	0x8
#define	CDA1_CR_EN_I1      	0x10
#define	CDA1_CR_EN_TBM     	0x20

#define	ISAC_CDA2_CR            	(0x4f)
                                       /* Bit definition of CDA2_CR */
#define	CDA2_CR_SWAP       	0x1
#define	CDA2_CR_EN_O0      	0x2
#define	CDA2_CR_EN_O1      	0x4
#define	CDA2_CR_EN_I0      	0x8
#define	CDA2_CR_EN_I1      	0x10
#define	CDA2_CR_EN_TBM     	0x20

#define	ISAC_TR_CR              	(0x50)
                                       /* Bit definition of TR_CR */
#define	TR_CR_EN_B1X       	0x8
#define	TR_CR_EN_B2X       	0x10
#define	TR_CR_EN_B1R       	0x20
#define	TR_CR_EN_B2R       	0x40
#define	TR_CR_EN_D         	0x80

#define	ISAC_BCHA_CR            	(0x51)
                                       /* Bit definition of BCHA_CR */
#define	BCHA_CR_EN_B1      	0x8
#define	BCHA_CR_EN_B2      	0x10
#define	BCHA_CR_EN_D       	0x20
#define	BCHA_CR_DPS_D      	0x80

#define	ISAC_BCHB_CR            	(0x52)
                                       /* Bit definition of BCHB_CR */
#define	BCHB_CR_EN_B1      	0x8
#define	BCHB_CR_EN_B2      	0x10
#define	BCHB_CR_EN_D       	0x20
#define	BCHB_CR_DPS_D      	0x80

#define	ISAC_DCI_CR             	(0x53)
                                       /* Bit definition of DCI_CR */
#define	DCI_CR_D_EN_B1     	0x8
#define	DCI_CR_D_EN_B2     	0x10
#define	DCI_CR_D_EN_D      	0x20
#define	DCI_CR_EN_CI1      	0x40
#define	DCI_CR_DPS_CI1     	0x80

#define	ISAC_MON_CR             	(0x54)
                                       /* Bit definition of MON_CR */
#define	MON_CR_EN_MON      	0x40
#define	MON_CR_DPS         	0x80

#define	ISAC_SDS1_CR            	(0x55)
                                       /* Bit definition of SDS1_CR */
#define	SDS1_CR_ENS_TSS3   	0x20
#define	SDS1_CR_ENS_TSS1   	0x40
#define	SDS1_CR_ENS_TSS    	0x80

#define	ISAC_SDS2_CR            	(0x56)
                                       /* Bit definition of SDS2_CR */
#define	SDS2_CR_ENS_TSS3   	0x20
#define	SDS2_CR_ENS_TSS1   	0x40
#define	SDS2_CR_ENS_TSS    	0x80

#define	ISAC_IOM_CR             	(0x57)
                                       /* Bit definition of IOM_CR */
#define	IOM_CR_DIS_IOM     	0x1
#define	IOM_CR_DIS_OD      	0x2
#define	IOM_CR_CLKM        	0x4
#define	IOM_CR_EN_BCL      	0x8
#define	IOM_CR_TIC_DIS     	0x10
#define	IOM_CR_CI_CS       	0x20
#define	IOM_CR_DIS_AW      	0x40
#define	IOM_CR_SPU         	0x80

#define	ISAC_ASTI               	(0x58)
                                       /* Bit definition of ASTI */
#define	ASTI_ACK10         	0x1
#define	ASTI_ACK11         	0x2
#define	ASTI_ACK20         	0x4
#define	ASTI_ACK21         	0x8

#define	ISAC_STI                	(0x58)
                                       /* Bit definition of STI */
#define	STI_STI10          	0x1
#define	STI_STI11          	0x2
#define	STI_STI20          	0x4
#define	STI_STI21          	0x8
#define	STI_STOV10         	0x10
#define	STI_STOV11         	0x20
#define	STI_STOV20         	0x40
#define	STI_STOV21         	0x80

#define	ISAC_MSTI               	(0x59)
                                       /* Bit definition of MSTI */
#define	MSTI_STI10         	0x1
#define	MSTI_STI11         	0x2
#define	MSTI_STI20         	0x4
#define	MSTI_STI21         	0x8
#define	MSTI_STOV10        	0x10
#define	MSTI_STOV11        	0x20
#define	MSTI_STOV20        	0x40
#define	MSTI_STOV21        	0x80

#define	ISAC_SDS_CONF           	(0x5a)
                                       /* Bit definition of SDS_CONF */
#define	SDS_CONF_SDS1_BCL  	0x1
#define	SDS_CONF_SDS2_BCL  	0x2

#define	ISAC_MCDA               	(0x5b)

#define	ISAC_MOX                	(0x5c)
#define	ISAC_MOR                	(0x5c)
#define	ISAC_MOSR               	(0x5d)
                                       /* Bit definition of MOSR */
#define	MOSR_MAB           	0x10
#define	MOSR_MDA           	0x20
#define	MOSR_MER           	0x40
#define	MOSR_MDR           	0x80

#define	ISAC_MOCR               	(0x5e)
                                       /* Bit definition of MOCR */
#define	MOCR_MXC           	0x10
#define	MOCR_MIE           	0x20
#define	MOCR_MRC           	0x40
#define	MOCR_MRE           	0x80

#define	ISAC_MCONF              	(0x5f)
                                       /* Bit definition of MCONF */
#define	MCONF_TOUT         	0x1

#define	ISAC_MSTA               	(0x5f)
                                       /* Bit definition of MSTA */
#define	MSTA_TOUT          	0x1
#define	MSTA_MAC           	0x4

#define	ISAC_ISTA               	(0x60)
                                       /* Bit definition of ISTA */
#define	ISTA_ICD           	0x1
#define	ISTA_MOS           	0x2
#define	ISTA_TRAN          	0x4
#define	ISTA_AUX           	0x8
#define	ISTA_CIC           	0x10
#define	ISTA_ST            	0x20
#define	ISTA_ICB           	0x80

#define	ISAC_MASK               	(0x60)
                                       /* Bit definition of MASK */
#define	MASK_ICD           	0x1
#define	MASK_MOS           	0x2
#define	MASK_TRAN          	0x4
#define	MASK_AUX           	0x8
#define	MASK_CIC           	0x10
#define	MASK_ST            	0x20
#define	MASK_ICB           	0x80

#define	ISAC_AUXI               	(0x61)
                                       /* Bit definition of AUXI */
#define	AUXI_INT1          	0x1
#define	AUXI_INT2          	0x2
#define	AUXI_TIN1          	0x4
#define	AUXI_TIN2          	0x8
#define	AUXI_WOV           	0x10
#define	AUXI_EAW           	0x20

#define	ISAC_AUXM               	(0x61)
                                       /* Bit definition of AUXM */
#define	AUXM_INT1          	0x1
#define	AUXM_INT2          	0x2
#define	AUXM_TIN1          	0x4
#define	AUXM_TIN2          	0x8
#define	AUXM_WOV           	0x10
#define	AUXM_EAW           	0x20

#define	ISAC_MODE1              	(0x62)
                                       /* Bit definition of MODE1 */
#define	MODE1_RSS1         	0x1
#define	MODE1_RSS2         	0x2
#define	MODE1_CFS          	0x4
#define	MODE1_WTC2         	0x8
#define	MODE1_WTC1         	0x10

#define	ISAC_MODE2              	(0x63)
                                       /* Bit definition of MODE2 */
#define	MODE2_PPSDX        	0x1
#define	MODE2_INT_POL      	0x8

#define	ISAC_ID                 	(0x64)

#define	ISAC_SRES               	(0x64)
                                       /* Bit definition of SRES */
#define	SRES_RES_RSTO      	0x1
#define	SRES_RES_TR        	0x2
#define	SRES_RES_IOM       	0x4
#define	SRES_RES_DCH       	0x8
#define	SRES_RES_MON       	0x10
#define	SRES_RES_BCH      	0x40
#define	SRES_RES_CI        	0x80

#define	ISAC_TIMR2              	(0x65)
                                       /* Bit definition of TIMR2 */
#define	TIMR2_TMD          	0x80

#define	ISAC_MASKB             	(0x70)
                                       /* Bit definition of MASKB1 */
#define	MASKB1_XDU         	0x4
#define	MASKB1_XPR         	0x10
#define	MASKB1_RFO         	0x20
#define	MASKB1_RPF         	0x40
#define	MASKB1_RME         	0x80

#define	ISAC_ISTAB             	(0x70)
                                       /* Bit definition of ISTAB1 */
#define	ISTAB1_XDU         	0x4
#define	ISTAB1_XPR         	0x10
#define	ISTAB1_RFO         	0x20
#define	ISTAB1_RPF         	0x40
#define	ISTAB1_RME         	0x80

#define	ISAC_CMDRB             	(0x71)
                                       /* Bit definition of CMDRB1 */
#define	CMDRB1_XRES        	0x1
#define	CMDRB1_XME         	0x2
#define	CMDRB1_XTF         	0x8
#define	CMDRB1_RRES        	0x40
#define	CMDRB1_RMC         	0x80

#define	ISAC_STARB             	(0x71)
                                       /* Bit definition of STARB1 */
#define	STARB1_XACI        	0x2
#define	STARB1_RACI        	0x8
#define	STARB1_XFW         	0x40
#define	STARB1_XDOV        	0x80

#define	ISAC_MODEB             	(0x72)
                                       /* Bit definition of MODEB1 */
#define	MODEB1_RAC         	0x8
#define	MODEB1_MDS0        	0x20
#define	MODEB1_MDS1        	0x40
#define	MODEB1_MDS2        	0x80

#define	ISAC_EXMB              	(0x73)
                                       /* Bit definition of EXMB1 */
#define	EXMB1_ITF          	0x1
#define	EXMB1_RCRC         	0x4
#define	EXMB1_XCRC         	0x8
#define	EXMB1_SRA          	0x10
#define	EXMB1_XFBS         	0x80

#define	ISAC_RAH1             	(0x75)
                                       /* Bit definition of RAH1B1 */
#define	RAH1B1_MHA         	0x1

#define	ISAC_RBCLB             	(0x76)

#define	ISAC_RAH2           	(0x76)
                                       /* Bit definition of RAH2B1 */
#define	RAH2B1_MLA         	0x1

#define	ISAC_RAL1             	(0x77)

#define	ISAC_RBCHB             	(0x77)
                                       /* Bit definition of RBCHB1 */
#define	RBCHB1_OV          	0x10

#define	ISAC_RSTAB             	(0x78)
                                       /* Bit definition of RSTAB1 */
#define	RSTAB1_LA          	0x1
#define	RSTAB1_C_R         	0x2
#define	RSTAB1_HA0         	0x4
#define	RSTAB1_HA1         	0x8
#define	RSTAB1_RAB         	0x10
#define	RSTAB1_CRC         	0x20
#define	RSTAB1_RDO         	0x40
#define	RSTAB1_VFR         	0x80

#define	ISAC_RAL2             	(0x78)

#define	ISAC_TMB               	(0x79)
                                       /* Bit definition of TMB1 */
#define	TMB1_TLP           	0x1

#define	ISAC_XFIFOB            	(0x7a)
#define	ISAC_RFIFOB            	(0x7a)
#else
#define ISAC_MASK 0x20
#define ISAC_ISTA 0x20
#define ISAC_STAR 0x21
#define ISAC_CMDR 0x21
#define ISAC_EXIR 0x24
#define ISAC_ADF2 0x39
#define ISAC_SPCR 0x30
#define ISAC_ADF1 0x38
#define ISAC_CIR0 0x31
#define ISAC_CIX0 0x31
#define ISAC_CIR1 0x33
#define ISAC_CIX1 0x33
#define ISAC_STCR 0x37
#define ISAC_MODE 0x22
#define ISAC_RSTA 0x27
#define ISAC_RBCL 0x25
#define ISAC_RBCH 0x2A
#define ISAC_TIMR 0x23
#define ISAC_SQXR 0x3b
#define ISAC_MOSR 0x3a
#define ISAC_MOCR 0x3a
#define ISAC_MOR0 0x32
#define ISAC_MOX0 0x32
#define ISAC_MOR1 0x34
#define ISAC_MOX1 0x34

#define ISAC_RBCH_XAC 0x80
#endif
#define ISAC_CMD_TIM	0x0
#define ISAC_CMD_RS	0x1
#define ISAC_CMD_SCZ	0x4
#define ISAC_CMD_SSZ	0x2
#define ISAC_CMD_AR8	0x8
#define ISAC_CMD_AR10	0x9
#define ISAC_CMD_ARL	0xA
#define ISAC_CMD_DUI	0xF

#define ISAC_IND_RS	0x1
#define ISAC_IND_PU	0x7
#define ISAC_IND_DR	0x0
#define ISAC_IND_SD	0x2
#define ISAC_IND_DIS	0x3
#define ISAC_IND_EI	0x6
#define ISAC_IND_RSY	0x4
#define ISAC_IND_ARD	0x8
#define ISAC_IND_TI	0xA
#define ISAC_IND_ATI	0xB
#define ISAC_IND_AI8	0xC
#define ISAC_IND_AI10	0xD
#define ISAC_IND_DID	0xF

extern void ISACVersion(struct IsdnCardState *cs, char *s);
extern void initisac(struct IsdnCardState *cs);
extern void isac_interrupt(struct IsdnCardState *cs, u_char val);
extern void clear_pending_isac_ints(struct IsdnCardState *cs);
