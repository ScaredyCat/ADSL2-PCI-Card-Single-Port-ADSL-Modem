/******************************************************************************
**
** FILE NAME    : danube_sdio_controller_registers.h
** PROJECT      : Danube
** MODULES      : SDIO
**
** DATE         : 1 Jan 2006
** AUTHOR       : TC Chen
** DESCRIPTION  : SDIO Driver
** COPYRIGHT    :       Copyright (c) 2006
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Version $Date      $Author     $Comment
*******************************************************************************/

//registers address space
#define MMCI_ADDR_SPACE 	0xBE102000

// registers address
// MCI Registers
#define MCI_PWR			(MMCI_ADDR_SPACE + 0x0000)
#define MCI_CLK			(MMCI_ADDR_SPACE + 0x0004)
#define MCI_ARG			(MMCI_ADDR_SPACE + 0x0008)
#define MCI_CMD			(MMCI_ADDR_SPACE + 0x000C)
#define MCI_REPCMD		(MMCI_ADDR_SPACE + 0x0010)
#define MCI_REP0		(MMCI_ADDR_SPACE + 0x0014)
#define MCI_REP1		(MMCI_ADDR_SPACE + 0x0018)
#define MCI_REP2		(MMCI_ADDR_SPACE + 0x001C)
#define MCI_REP3		(MMCI_ADDR_SPACE + 0x0020)
#define MCI_DTIM		(MMCI_ADDR_SPACE + 0x0024)
#define MCI_DLGTH		(MMCI_ADDR_SPACE + 0x0028)
#define MCI_DCTRL		(MMCI_ADDR_SPACE + 0x002C)
#define MCI_DCNT		(MMCI_ADDR_SPACE + 0x0030)
#define MCI_STAT		(MMCI_ADDR_SPACE + 0x0034)
#define MCI_CL			(MMCI_ADDR_SPACE + 0x0038)
#define MCI_IM0			(MMCI_ADDR_SPACE + 0x003C)
#define MCI_IM1			(MMCI_ADDR_SPACE + 0x0040)
#define MCI_SDMCS		(MMCI_ADDR_SPACE + 0x0044)
#define MCI_FC			(MMCI_ADDR_SPACE + 0x0048)

#define MCI_DF0			(MMCI_ADDR_SPACE + 0x0080)
#define MCI_DF1			(MMCI_ADDR_SPACE + 0x0084)
#define MCI_DF2			(MMCI_ADDR_SPACE + 0x0088)
#define MCI_DF3			(MMCI_ADDR_SPACE + 0x008C)
#define MCI_DF4			(MMCI_ADDR_SPACE + 0x0090)
#define MCI_DF5			(MMCI_ADDR_SPACE + 0x0094)
#define MCI_DF6			(MMCI_ADDR_SPACE + 0x0098)
#define MCI_DF7			(MMCI_ADDR_SPACE + 0x009C)
#define MCI_DF8			(MMCI_ADDR_SPACE + 0x00A0)
#define MCI_DF9			(MMCI_ADDR_SPACE + 0x00A4)
#define MCI_DF10		(MMCI_ADDR_SPACE + 0x00A8)
#define MCI_DF11		(MMCI_ADDR_SPACE + 0x00AC)
#define MCI_DF12		(MMCI_ADDR_SPACE + 0x00B0)
#define MCI_DF13		(MMCI_ADDR_SPACE + 0x00B4)
#define MCI_DF14		(MMCI_ADDR_SPACE + 0x00B8)
#define MCI_DF15		(MMCI_ADDR_SPACE + 0x00BC)

#define MCI_PID0		(MMCI_ADDR_SPACE + 0x0FE0)
#define MCI_PID1		(MMCI_ADDR_SPACE + 0x0FE4)
#define MCI_PID2		(MMCI_ADDR_SPACE + 0x0FE8)
#define MCI_PID3		(MMCI_ADDR_SPACE + 0x0FEC)
#define MCI_PCID0		(MMCI_ADDR_SPACE + 0x0FF0)
#define MCI_PCID1		(MMCI_ADDR_SPACE + 0x0FF4)
#define MCI_PCID2		(MMCI_ADDR_SPACE + 0x0FF8)
#define MCI_PCID3		(MMCI_ADDR_SPACE + 0x0FFC)

// SDIO Registers
#define SDIO_CLC		(MMCI_ADDR_SPACE + 0x1000)
#define SDIO_ID			(MMCI_ADDR_SPACE + 0x1004)
#define SDIO_CTRL		(MMCI_ADDR_SPACE + 0x1008)

#define SDIO_RIS		(MMCI_ADDR_SPACE + 0x100C)	// need to check
#define SDIO_IMC		(MMCI_ADDR_SPACE + 0x1010)	//need to check
#define SDIO_MIS		(MMCI_ADDR_SPACE + 0x1014)
#define SDIO_ICR		(MMCI_ADDR_SPACE + 0x1018)
#define SDIO_ISR		(MMCI_ADDR_SPACE + 0x101C)
#define SDIO_DMACON		(MMCI_ADDR_SPACE + 0x1020)

#define SDIO_RSVD_ADDR0		(MMCI_ADDR_SPACE + 0x10F0)

/////////////////////////////////////
// MCI_PWR REGISTER
#define MCI_PWR_OFF		0x0
#define MCI_PWR_UP		0x2
#define MCI_PWR_ON		0x3
#define MCI_PWR_VDD_20		0x0
#define MCI_PWR_VDD_21		0x4
#define MCI_PWR_VDD_22		0x8
#define MCI_PWR_VDD_23		0x12
#define MCI_PWR_VDD_24		0x16
#define MCI_PWR_VDD_25		0x20
#define MCI_PWR_VDD_26		0x24
#define MCI_PWR_VDD_27		0x28
#define MCI_PWR_VDD_28		0x32
#define MCI_PWR_VDD_29		0x36
#define MCI_PWR_VDD_30		0x40
#define MCI_PWR_VDD_31		0x44
#define MCI_PWR_VDD_32		0x48
#define MCI_PWR_VDD_33		0x52
#define MCI_PWR_VDD_34		0x56
#define MCI_PWR_VDD_35		0x60
#define MCI_PWR_OD		BIT6
#define MCI_PWR_ROD		BIT7

// MCI_CLK
#define MCI_CLK_EN		BIT8
#define MCI_CLK_PWR		BIT9
#define MCI_CLK_BY		BIT10
#define MCI_CLK_WD		BIT11

// MCI_CMD
#define MCI_CMD_EN		BIT10
#define MCI_CMD_PEN		BIT9
#define MCI_CMD_INT		BIT8
#define MCI_CMD_LRSP		BIT7
#define MCI_CMD_RSP		BIT6

#define MCI_CMD_NO_RSP		0
#define MCI_CMD_SHORT_RSP	(MCI_CMD_RSP)
#define MCI_CMD_LONG_RSP	(MCI_CMD_RSP | MCI_CMD_LRSP)

// MCI_DCTRL
#define MCI_DCTRL_DMA		BIT3
#define MCI_DCTRL_M		BIT2
#define MCI_DCTRL_DIR		BIT1
#define MCI_DCTRL_EN		BIT0

//MCI_STAT
#define MCI_STAT_RXDA		BIT21
#define MCI_STAT_TXDA		BIT20
#define MCI_STAT_RXFE		BIT19
#define MCI_STAT_TXFE		BIT18
#define MCI_STAT_RXFF		BIT17
#define MCI_STAT_TXFF		BIT16
#define MCI_STAT_RXHF		BIT15
#define MCI_STAT_TXHF		BIT14
#define MCI_STAT_RXA		BIT13
#define MCI_STAT_TXA		BIT12
#define MCI_STAT_CMDA		BIT11
#define MCI_STAT_DBE		BIT10
#define MCI_STAT_SBE		BIT9
#define MCI_STAT_DE		BIT8
#define MCI_STAT_CS		BIT7
#define MCI_STAT_CRE		BIT6
#define MCI_STAT_RO		BIT5
#define MCI_STAT_TU		BIT4
#define MCI_STAT_DTO		BIT3
#define MCI_STAT_CTO		BIT2
#define MCI_STAT_DCF		BIT1
#define MCI_STAT_CCF		BIT0

//MCI_CL
#define MCI_CL_DBEC		BIT10
#define MCI_CL_SBEC		BIT9
#define MCI_CL_DEC		BIT8
#define MCI_CL_CSC		BIT7
#define MCI_CL_CREC		BIT6
#define MCI_CL_ROC		BIT5
#define MCI_CL_TUC		BIT4
#define MCI_CL_DTOC		BIT3
#define MCI_CL_CTOC		BIT2
#define MCI_CL_DCFC		BIT1
#define MCI_CL_CCFC		BIT0

//MCI_IM
#define MCI_IM_RXDA		BIT21
#define MCI_IM_TXDA		BIT20
#define MCI_IM_RXFE		BIT19
#define MCI_IM_TXFE		BIT18
#define MCI_IM_RXFF		BIT17
#define MCI_IM_TXFF		BIT16
#define MCI_IM_RXHF		BIT15
#define MCI_IM_TXHF		BIT14
#define MCI_IM_RXA		BIT13
#define MCI_IM_TXA		BIT12
#define MCI_IM_CMDA		BIT11
#define MCI_IM_DBE		BIT10
#define MCI_IM_SBE		BIT9
#define MCI_IM_DE		BIT8
#define MCI_IM_CS		BIT7
#define MCI_IM_CRE		BIT6
#define MCI_IM_RO		BIT5
#define MCI_IM_TU		BIT4
#define MCI_IM_DTO		BIT3
#define MCI_IM_CTO		BIT2
#define MCI_IM_DCF		BIT1
#define MCI_IM_CCF		BIT0

// SDIO_CLC
#define SDIO_CLC_RMC_BYPASS	BIT8
#define SDIO_CLC_FSOE		BIT5
#define SDIO_CLC_SBWE		BIT4
#define SDIO_CLC_EDIS		BIT3
#define SDIO_CLC_SPEN		BIT2
#define SDIO_CLC_DISS		BIT1
#define SDIO_CLC_DISR		BIT0

// SDIO_CTRL
#define SDIO_CTRL_RDWT		BIT4
#define SDIO_CTRL_SDIOEN	BIT0

//SDIO_IMC
#define SDIO_IMC_SDIO		BIT2
#define SDIO_IMC_INTR1		BIT1
#define SDIO_IMC_INTR0		BIT0

//SDIO_RIS
#define SDIO_RIS_SDIO		BIT2
#define SDIO_RIS_INTR1		BIT1
#define SDIO_RIS_INTR0		BIT0

//SDIO_MIS
#define SDIO_MIS_SDIO		BIT2
#define SDIO_MIS_INTR1		BIT1
#define SDIO_MIS_INTR0		BIT0

// SDIO_ICR
#define SDIO_ICR_SDIO		BIT2
#define SDIO_ICR_INTR1		BIT1
#define SDIO_ICR_INTR0		BIT0

//SDIO_ISR
#define SDIO_ISR_SDIO		BIT2
#define SDIO_ISR_INTR1		BIT1
#define SDIO_ISR_INTR0		BIT0

// SDIO_DMACON
#define SDIO_DMACON_TXON	BIT1
#define SDIO_DMACON_RXON	BIT0
#define SDIO_DMA_CLASS		BIT2
