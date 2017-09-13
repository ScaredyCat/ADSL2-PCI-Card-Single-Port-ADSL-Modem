/******************************************************************************
**
** FILE NAME    : infineon_sdio_controller.h
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

typedef struct sd_controller {
	struct list_head list;
	char *name;
	int (*send_cmd) (struct sd_controller * sd_dev, struct sd_cmd * cmd);
	int (*send_data) (struct sd_controller * dev, struct sdio_card * card,
			  sd_block_request_t * request, uint32_t timeout);
	int (*read_data_pre) (struct sd_controller * dev,
			      struct sdio_card * card,
			      sd_block_request_t * request);
	int (*read_data) (struct sd_controller * dev, struct sdio_card * card,
			  sd_block_request_t * request, uint32_t timeout);
	int (*set_ops) (int type, uint32_t data);
	void (*mask_ack_irq) (void);

	uint8_t sdio_irq_state;
	uint32_t VDD;		// the VDD voltage profile of the controller
	//uint32_t ocr;
	uint32_t current_freq;	//
	uint8_t busy;
	struct sd_cmd *cmd;
	sdio_card_t *card_transfer_state;
	sdio_card_t cards;
	uint32_t current_speed;
	uint8_t current_bus_width;
	void *priv;
	struct semaphore sd_data_sema;
	struct semaphore sd_cmd_sema;
	uint8_t blklen;
} sdio_controller_t;

#define SD_SET_VDD	 1
#define SD_SET_FREQENCY  2
#define SD_SET_BUS_WIDTH 3
#define SD_SET_BLOCK_LEN 4

int sd_core_sdio_int (struct sd_controller *dev);
int register_sd_controller (sdio_controller_t * controller);
void unregister_sd_controller (sdio_controller_t * controller);
