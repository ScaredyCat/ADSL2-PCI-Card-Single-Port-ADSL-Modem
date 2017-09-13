/******************************************************************************
**
** FILE NAME    : infineon_sdio_card.h
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

#define SDIO_CIS_PTR 0x09

typedef struct csd {
	uint32_t csd:2, rev6:6, taac:8, nsac:8, tran_speed:8;
	uint16_t ccc:12, read_bl_len:4;

	// 
	uint16_t read_bl_partial:1,
		write_blk_misalign:1,
		read_blk_misalign:1, dsr_imp:1, c_size:12;

	uint16_t vdd_r_curr_min:3,
		vdd_r_curr_max:3,
		vdd_w_curr_min:3,
		vdd_w_curr_max:3, c_size_mult:3, erase_blk_en:1;

	uint8_t sector_size;
	uint8_t wp_grp_size;

	//
	uint32_t wp_grp_enable:1,
		rev_mmc:2,
		r2w_factor:3,
		write_bl_len:4,
		write_bl_partial:1,
		rev5:5,
		file_format_grp:1,
		copy:1,
		perm_write_protect:1,
		tmp_write_protect:1, file_format:2, rev2:2, crc:7, rev1:1;

} sdio_csd_t;

#define SCR_SD_BUS_WIDTH_1BIT (1<<0)
#define SCR_SD_BUS_WIDTH_4BIT (1<<2)

typedef struct sd_scr {
	uint32_t reserved32;
	uint16_t reserved16;
	uint8_t sd_bus_width:4, sd_security:3, data_stat_after_erase:1;
	uint8_t sd_spec:4, scr_structure:4;
} sd_scr_t;

typedef struct sd_cid {
	uint8_t mid;
	uint16_t oid;
	uint8_t pnm[5];
	uint8_t prv;
	uint32_t psn;
#if 1
	uint16_t reserved:4, mdt:12;
#else
	uint16_t mdt;
#endif
	uint8_t crc:7, no_used:1;
} sd_cid_t;

typedef struct sdio_card {
	struct list_head list;
	void *priv;		/* pointer to private data */
	struct sd_controller *controller;
	uint32_t sdio_ocr;	// ocr register , the VDD voltage profile
	uint32_t mp_ocr;	// ocr register , the VDD voltage profile
	uint8_t state;		// the card current state
	uint16_t rca;
	struct sd_cis_tuple *cis;
	void *sd_driver;
	void *sdio_driver;
	int type;
	uint32_t vdd;
	uint32_t csd_raw[4];
	sd_cid_t cid;
	sdio_csd_t csd;
	uint32_t max_speed;
	sd_scr_t scr;
	uint8_t current_speed;
	uint8_t current_block_len_shift;
	uint8_t block_len_bits;
	uint8_t valid;
} sdio_card_t;

typedef struct sdio_card_driver {
	struct list_head list;
	 
char *name;
	int (*probe) (void);
	void (*remove) (sdio_card_t *);
	int (*insert) (sdio_card_t *);
	int (*eject) (sdio_card_t *);
	void (*sdio_irq) (void);
	int type;
	struct sd_cis_tuple *cis;
	uint32_t cid[4];
} sdio_card_driver_t;

int register_sd_card_driver (sdio_card_driver_t * driver);
void unregister_sd_card_driver (sdio_card_driver_t * driver);
