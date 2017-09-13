/******************************************************************************
**
** FILE NAME    : infineon_sdio_cmds.h
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

#define BIT0		(1<<0)
#define BIT1		(1<<1)
#define BIT2		(1<<2)
#define BIT3		(1<<3)
#define BIT4		(1<<4)
#define BIT5		(1<<5)
#define BIT6		(1<<6)
#define BIT7		(1<<7)
#define BIT8		(1<<8)
#define BIT9		(1<<9)
#define BIT10		(1<<10)
#define BIT11		(1<<11)
#define BIT12		(1<<12)
#define BIT13		(1<<13)
#define BIT14		(1<<14)
#define BIT15		(1<<15)
#define BIT16		(1<<16)
#define BIT17		(1<<17)
#define BIT18		(1<<18)
#define BIT19		(1<<19)
#define BIT20		(1<<20)
#define BIT21		(1<<21)
#define BIT22		(1<<22)
#define BIT23		(1<<23)
#define BIT24		(1<<24)
#define BIT25		(1<<25)
#define BIT26		(1<<26)
#define BIT27		(1<<27)
#define BIT28		(1<<28)
#define BIT29		(1<<29)
#define BIT30		(1<<30)
#define BIT31		(1<<31)

// card status mask
#define SD_CS_AKE_SEQ_ERROR		BIT3
#define SD_CS_APP_CMD			BIT5
#define SD_CS_READY_FOR_DATA		BIT8
#define SD_CS_CURRETN_STATE		0x1E00
#define SD_CS_ERASE_RESET		BIT13
#define SD_CS_CARD_ECC_DISABLED		BIT14
#define SD_CS_WP_ERASE_SKIP		BIT15
#define SD_CS_CID_CSD_OVERWRITE		BIT16
#define	SD_CS_ERROR			BIT19
#define	SD_CS_CC_ERROR			BIT20
#define SD_CS_CARD_ECC_FAILED		BIT21
#define SD_CS_ILLEGAL_COMMAND		BIT22
#define SD_CS_COM_CRC_ERROR		BIT23
#define SD_CS_LOCK_UNLOCK_FAILED	BIT24
#define SD_CS_CARD_IS_LOCKED		BIT25
#define SD_CS_WP_VIOLATION		BIT26
#define SD_CS_ERASE_PARAM		BIT27
#define SD_CS_ERASE_SEQ_ERROR		BIT28
#define SD_CS_BLOCK_LEN_ERROR		BIT29
#define SD_CS_ADDRESS_ERROR		BIT30
#define SD_CS_OUT_OF_RANGE		BIT31

// SDIO status
#define SDIO_FUNCTION_NUMBER	BIT1
#define SDIO_OUT_OF_RANGE	BIT0

// SD card state
#define	SD_CS_STATE_IDLE	0
#define	SD_CS_STATE_READY	1
#define	SD_CS_STATE_IDENT	2
#define	SD_CS_STATE_STBY	3
#define	SD_CS_STATE_TRAN	4
#define	SD_CS_STATE_DATA	5
#define	SD_CS_STATE_RCV		6
#define	SD_CS_STATE_PRG		7
#define	SD_CS_STATE_DIS		8
#define SD_CS_STATE_MASK	0xF
#define SD_CS_STATE_SHIFT	9

// OCR Register
#define SD_OCR_VDD_20_21	BIT8
#define SD_OCR_VDD_21_22	BIT9
#define SD_OCR_VDD_22_23	BIT10
#define SD_OCR_VDD_23_24	BIT11
#define SD_OCR_VDD_24_25	BIT12
#define SD_OCR_VDD_25_26	BIT13
#define SD_OCR_VDD_26_27	BIT14
#define SD_OCR_VDD_27_28	BIT15
#define SD_OCR_VDD_28_29	BIT16
#define SD_OCR_VDD_29_30	BIT17
#define SD_OCR_VDD_30_31	BIT18
#define SD_OCR_VDD_31_32	BIT19
#define SD_OCR_VDD_32_33	BIT20
#define SD_OCR_VDD_33_34	BIT21
#define SD_OCR_VDD_34_35	BIT22
#define SD_OCR_VDD_35_36	BIT23
#define SD_OCR_CARD_BUSY	BIT31

#define SD_CMD0		0
#define SD_CMD1		1
#define SD_CMD2		2
#define SD_CMD3		3
#define SD_CMD4		4
#define SD_CMD5		5
#define SD_CMD6		6
#define SD_CMD7		7
#define SD_CMD8		8
#define SD_CMD9		9
#define SD_CMD10	10
#define SD_CMD11	11
#define SD_CMD12	12
#define SD_CMD13	13
#define SD_CMD14	14
#define SD_CMD15	15
#define SD_CMD16	16
#define SD_CMD17	17
#define SD_CMD18	18
#define SD_CMD19	19
#define SD_CMD20	20
#define SD_CMD21	21
#define SD_CMD22	22
#define SD_CMD23	23
#define SD_CMD24	24
#define SD_CMD25	25
#define SD_CMD26	26
#define SD_CMD27	27
#define SD_CMD28	28
#define SD_CMD29	29
#define SD_CMD30	30
#define SD_CMD31	31
#define SD_CMD32	32
#define SD_CMD33	33
#define SD_CMD34	34
#define SD_CMD35	35
#define SD_CMD36	36
#define SD_CMD37	37
#define SD_CMD38	38
#define SD_CMD39	39
#define SD_CMD40	40
#define SD_CMD41	41
#define SD_CMD42	42
#define SD_CMD43	43
#define SD_CMD44	44
#define SD_CMD45	45
#define SD_CMD46	46
#define SD_CMD47	47
#define SD_CMD48	48
#define SD_CMD49	49
#define SD_CMD50	50
#define SD_CMD51	51
#define SD_CMD52	52
#define SD_CMD53	53
#define SD_CMD54	54
#define SD_CMD55	55
#define SD_CMD56	56
#define SD_CMD57	57
#define SD_CMD58	58
#define SD_CMD59	59
#define SD_CMD60	60
#define SD_CMD61	61
#define SD_CMD62	62
#define SD_CMD63	63

#define SD_ACMD6	6
#define SD_ACMD13	13
#define SD_ACMD17	17
#define SD_ACMD18	18
#define SD_ACMD19	19
#define SD_ACMD20	20
#define SD_ACMD21	21
#define SD_ACMD22	22
#define SD_ACMD23	23
#define SD_ACMD24	24
#define SD_ACMD25	25
#define SD_ACMD26	26
#define SD_ACMD38	38
#define SD_ACMD39	39
#define SD_ACMD40	40
#define SD_ACMD41	41
#define SD_ACMD42	42
#define SD_ACMD43	43
#define SD_ACMD49	49
#define SD_ACMD51	51

// Basic Commands (class 0)                                                     // type  argument                               resp
#define SD_CMD_GO_IDLE_STATE		SD_CMD0	// bc                                           X
#define SD_CMD_SEND_OP_COND		SD_CMD1	//
#define SD_CMD_ALL_SEND_CID		SD_CMD2	// bcr                                          R2
#define SD_CMD_SEND_RELATIVE_ADDR	SD_CMD3	// bcr                                          R6
#define SD_CMD_SET_DSR			SD_CMD4	// bc   [31:16] DSR                             X
#define SD_CMD_IO_SEND_OP_COND		SD_CMD5	//
#define SD_CMD_SELECT_CARD		SD_CMD7	// ac   [31:16] RCA                             R1b
#define SD_CMD_SEND_CSD			SD_CMD9	// ac   [31:16] RCA                             R2
#define SD_CMD_SEND_CID			SD_CMD10	// ac   [31:16] RCA                             R2
#define SD_CMD_STOP_TRANSMISSION	SD_CMD12	// ac                                           R1b
#define SD_CMD_SEND_STATUS		SD_CMD13	// ac   [31:16] RCA                             R1
#define SD_CMD_GO_INACTIVE_STATE	SD_CMD15	// ac   [31:16] RCA                             X
#define SD_CMD_SET_BLOCKLEN		SD_CMD16	// ac   [31:0]  Block length                    R1

// Block oriented read Commands (class 2)
#define SD_CMD_READ_SINGLE_BLOCK	SD_CMD17	// adtc [31:0]  Data address                    R1
#define SD_CMD_READ_MULTIPLE_BLOCK	SD_CMD18	// adtc [31:0]  Data address                    R1

// Block oritend write Commands (class 4)
#define SD_CMD_WRITE_BLOCK		SD_CMD24	// adtc [31:0]  Data address                    R1
#define SD_CMD_WRITE_MULTIPLE_BLOCK	SD_CMD25	// adtc [31:0]  Data address                    R1
#define SD_CMD_PROGRAM_CSD		SD_CMD27	// adtc                                         R1

// Block oritend write protection Commands (class 6)
#define SD_CMD_SET_WRITE_PROT		SD_CMD28	// ac   [31:0]  Data address                    R1b
#define SD_CMD_CLR_WRITE_PROT		SD_CMD29	// ac   [31:0]  Data address                    R1b
#define SD_CMD_SEND_WRITE_PROT		SD_CMD30	// adtc [31:0]  Write protect data address      R1

// Erase Commands (class 5)
#define SD_CMD_ERASE_WR_BLK_START	SD_CMD32	// ac   [31:0]  Data address                    R1
#define SD_CMD_ERASE_WR_BLK_END		SD_CMD33	// ac   [31:0]  Data address                    R1
#define SD_CMD_ERASE			SD_CMD38	// ac                                           R1b

// Lock card (class 7)
#define SD_CMD_LOCK_UNLOCK		SD_CMD42	// adtc                                         R1
#define SD_CMD_IO_RW_REDIRECT		SD_CMD52	// bc                                           X
#define SD_CMD_IO_RW_EXTENDED		SD_CMD53	//

// Application specific commands (class 8)
#define SD_CMD_APP_CMD			SD_CMD55	// ac   [31:16] RCA                             R1
#define SD_CMD_GEN_CMD			SD_CMD56	// adtc [0]     RD/WR                           R1
#define SD_CMD_READ_OCR			SD_CMD58	//
#define SD_CMD_CRC_ON_OFF		SD_CMD59	//
#define SD_CMD_SET_BUS_WIDTH		SD_ACMD6	// ac   [1:0]   Bus width                       R1
#define SD_CMD_SD_STATUS		SD_ACMD13	// adtc                                         R1
#define SD_CMD_SEND_NUM_WR_BLOCKS	SD_ACMD22	// adtc                                         R1
#define SD_CMD_SET_WR_BLK_ERASE_COUNT	SD_ACMD23	// ac   [22:0]  Number of blocks                R1
#define SD_CMD_SD_APP_OP_COND		SD_ACMD41	// bcr  [31:0]  OCR width busy                  R3
#define SD_CMD_SET_CLR_CARD_DETECT	SD_ACMD42	// ac   [0]     set_cd                          R1
#define SD_CMD_SEND_SCR			SD_ACMD51	// adtc                                         R1

// Switch function commands (class 10)
#define SD_CMD_SWITCH_FUNC		SD_CMD6	// adtc [31] Mode [30:24] 0 [23:20] group 6     R1

#define SDIO_IO_RW_DIRECT_WRITE BIT31

#define SD_RSP_NONE	0
#define SD_RSP_R1	1
#define SD_RSP_R1b	2
#define SD_RSP_R2	3
#define SD_RSP_R3	4
#define SD_RSP_R4	5
#define SD_RSP_R5	6
#define SD_RSP_R6	7

#define R1_SD_CS_ERROR	(SD_CS_OUT_OF_RANGE | SD_CS_ADDRESS_ERROR | SD_CS_BLOCK_LEN_ERROR | \
 SD_CS_ERASE_SEQ_ERROR | SD_CS_ERASE_PARAM | SD_CS_WP_VIOLATION | SD_CS_LOCK_UNLOCK_FAILED | \
 SD_CS_COM_CRC_ERROR | SD_CS_ILLEGAL_COMMAND | SD_CS_CARD_ECC_FAILED | SD_CS_CC_ERROR | \
 SD_CS_ERROR | SD_CS_CID_CSD_OVERWRITE | SD_CS_AKE_SEQ_ERROR)

#define	R5_SD_CS_ERROR	((((SD_CS_COM_CRC_ERROR>>16) | SD_CS_ILLEGAL_COMMAND >> 16) | (SD_CS_ERROR >> 16) | SDIO_FUNCTION_NUMBER | SDIO_OUT_OF_RANGE) << 8)

#define R6_SD_CS_ERROR	((SD_CS_COM_CRC_ERROR >> 8) | (SD_CS_ILLEGAL_COMMAND>>8) | (SD_CS_ERROR>>6) | SD_CS_AKE_SEQ_ERROR)

#define SD_MEMRDY   	0x80000000
#define SD_MP_PRESENT 	0x8000000
#define SDIO_IORDY   	0x80000000
