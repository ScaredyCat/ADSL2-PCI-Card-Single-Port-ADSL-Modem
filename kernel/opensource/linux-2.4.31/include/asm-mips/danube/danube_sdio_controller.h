/******************************************************************************
**
** FILE NAME    : danube_sdio_controller.h
** PROJECT      : Danube
** MODULES      : SDIO
**
** DATE         : 1 Jan 2006
** AUTHOR       : TC Chen
** DESCRIPTION  : DANUBE SDIO Driver
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
#define DEVICE_NAME "DANUBE_SDIO"

typedef struct {
	struct dma_device_info *dma_device;
	uint32_t mclk_speed;
} danube_sd_controller_priv_t;

#define DANUBE_SDIO_SEND_CMD 		1
#define DANUBE_SDIO_SEND_DATA 		2
#define DANUBE_SDIO_READ_DATA 		3
#define DANUBE_SDIO_SET_OPS_WBUS	4
#define DANUBE_SDIO_SET_OPS_FREQUENCY	5
#define DANUBE_SDIO_GET_OPS_WBUS	6
#define DANUBE_SDIO_GET_OPS_FREQUENCY	7

typedef struct {
	int block_length;	//0~11:1 Byte ~ 2048 Bytes
	int data_length;	// total data size
	struct sd_cmd cmd;
	char data[2048];
} danube_sdio_ioctl_block_request;
