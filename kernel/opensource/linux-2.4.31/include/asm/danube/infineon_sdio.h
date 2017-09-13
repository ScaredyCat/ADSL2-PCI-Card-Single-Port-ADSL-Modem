/******************************************************************************
**
** FILE NAME    : infineon_sdio.h
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

#define RCV_INT          1
#define TX_BUF_FULL_INT  2
#define TRANSMIT_CPT_INT 4

#undef SDIODEBUG
#ifdef SDIO_DEBUG
#define SDIODBGMSG(fmt, args ...) printk(KERN_DEBUG "sdio:" fmt, ##args)
#define SDIOERRMSG(fmt, args ...) printk(KERN_ERR "sdio:" fmt, ##args)
#else
#define SDIODBGMSG(fmt, args ...)
#define SDIOERRMSG(fmt, args ...) printk(KERN_ERR "sdio:" fmt, ##args)
#endif

#define MAX_RETRY		10
#define MAX_DATA_WAIT_RETRY	500

#define MMC_CARD_FOUND		4
#define COMBO_CARD_FOUND	3
#define SDIO_CARD_FOUND		2
#define MP_CARD_FOUND		1
#define FOUND_NONE		0
#define OK 			0
#define ERROR_NOMEM		-1	// out of memory
#define ERROR_CRC_ERROR		-2
#define ERROR_TIMEOUT		-3
#define ERROR_WRONG_CARD_STATE 	-4
#define ERROR_WRONG_RESPONSE_TYPE -5
#define ERROR_WRONG_RESPONSE_CMD  -6
#define ERROR_DATA_ERROR	-7

struct sd_cmd {
	uint32_t op_code;
	uint32_t args;
	uint32_t response_type;
	uint32_t response[4];
	int error;
};

// cis tuple
#define SD_CISTPL_NULL 	0x00
#define SD_CISTPL_CHECHSUM	0x10
#define SD_CISTPL_VERS_1	0x15
#define SD_CISTPL_ALTSTR	0x16
#define SD_CISTPL_MANFID	0x20
#define SD_CISTPL_FUNCID	0x21
#define SD_CISTPL_FUNCE	0x22
#define SD_CISTPL_SDIO_STD	0x91
#define SD_CISTPL_SDIO_EXT	0x92
#define SD_CISTPL_END	0xFF

struct sd_cis_tuple {
	uint8_t code;
	struct sd_cis_tuple *next;
	uint8_t *data;
	uint8_t size;
};

#define SDIO_TYPE	1
#define SD_TYPE		2

#define SD_READ_BLOCK	1
#define SDIO_READ_CSA	2
#define SD_WRITE_BLOCK 	3
#define SDIO_WRITE_CSA	4

typedef struct block_data {
	uint8_t *data;
	struct block_data *next;
} block_data_t;

typedef struct {
	int type;		// read single block/write single block/read multi block/write multi block/
	uint8_t block_size_shift;
	int nBlocks;
	uint32_t addr;
	uint8_t func_no;
	block_data_t *blocks;
	block_data_t *blocks_header;
	uint32_t request_size;
	int error;
} sd_block_request_t;

#define SD_CLK_400K	400000

#define SD_BUS_1BITS	1
#define SD_BUS_4BITS	4

int ifx_sdio_block_transfer (sdio_card_t * card,
			     sd_block_request_t * request);
