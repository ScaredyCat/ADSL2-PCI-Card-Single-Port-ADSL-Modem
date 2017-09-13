/*
 * Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5312/ar5312.h#2 $
 */
#ifndef _ATH_AR5312_H_
#define _ATH_AR5312_H_

#include "ar5212/ar5212.h"

#if AH_SUPPORT_2316
#define GETWMACNUM(_ah) (((((u_int32_t)((_ah)->ah_sh))==AR531X_WLAN0) || (((u_int32_t)((_ah)->ah_sh))==AR5315_WLAN0)) ?(0):((((u_int32_t)((_ah)->ah_sh))==AR531X_WLAN1)?(1):(-1)))
#else
#define GETWMACNUM(_ah) ((((u_int32_t)((_ah)->ah_sh))==AR531X_WLAN0) ?(0):((((u_int32_t)((_ah)->ah_sh))==AR531X_WLAN1)?(1):(-1)))
#endif

/*
 * This is board-specific data that is stored in a "fixed" location in flash.
 */
struct ar531x_boarddata {
	u_int32_t magic;             /* board data is valid */
#define AR531X_BD_MAGIC 0x35333131   /* "5311", for all 531x platforms */
	u_int16_t cksum;             /* checksum (starting with BD_REV 2) */
	u_int16_t rev;               /* revision of this struct */
#define BD_REV  4
	char   boardName[64];        /* Name of board */
	u_int16_t major;             /* Board major number */
	u_int16_t minor;             /* Board minor number */
	u_int32_t config;            /* Board configuration */
#define BD_ENET0        0x00000001   /* ENET0 is stuffed */
#define BD_ENET1        0x00000002   /* ENET1 is stuffed */
#define BD_UART1        0x00000004   /* UART1 is stuffed */
#define BD_UART0        0x00000008   /* UART0 is stuffed (dma) */
#define BD_RSTFACTORY   0x00000010   /* Reset factory defaults stuffed */
#define BD_SYSLED       0x00000020   /* System LED stuffed */
#define BD_EXTUARTCLK   0x00000040   /* External UART clock */
#define BD_CPUFREQ      0x00000080   /* cpu freq is valid in nvram */
#define BD_SYSFREQ      0x00000100   /* sys freq is set in nvram */
#define BD_WLAN0        0x00000200   /* Enable WLAN0 */
#define BD_MEMCAP       0x00000400   /* CAP SDRAM @ memCap for testing */
#define BD_DISWATCHDOG  0x00000800   /* disable system watchdog */
#define BD_WLAN1        0x00001000   /* Enable WLAN1 (ar5212) */
#define BD_ISCASPER     0x00002000   /* FLAG for AR2312 */
#define BD_WLAN0_2G_EN  0x00004000   /* FLAG for radio0_2G */
#define BD_WLAN0_5G_EN  0x00008000   /* FLAG for radio0_2G */
#define BD_WLAN1_2G_EN  0x00020000   /* FLAG for radio0_2G */
#define BD_WLAN1_5G_EN  0x00040000   /* FLAG for radio0_2G */
	u_int16_t resetConfigGpio;   /* Reset factory GPIO pin */
	u_int16_t sysLedGpio;        /* System LED GPIO pin */
	
	u_int32_t cpuFreq;           /* CPU core frequency in Hz */
	u_int32_t sysFreq;           /* System frequency in Hz */
	u_int32_t cntFreq;           /* Calculated C0_COUNT frequency */
	
	u_int8_t  wlan0Mac[6];
	u_int8_t  enet0Mac[6];
	u_int8_t  enet1Mac[6];
	
	u_int16_t pciId;             /* Pseudo PCIID for common code */
	u_int16_t memCap;            /* cap bank1 in MB */
	
	/* version 3 */
	u_int8_t  wlan1Mac[6];       /* (ar5212) */
};

extern	struct ath_hal * ar5312Attach(u_int16_t devid, HAL_SOFTC sc,
				      HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status);
extern  HAL_BOOL ar5312IsInterruptPending(struct ath_hal *ah);

extern	HAL_BOOL ar5312GpioCfgOutput(struct ath_hal *, u_int32_t gpio);
extern	HAL_BOOL ar5312GpioCfgInput(struct ath_hal *, u_int32_t gpio);
extern	HAL_BOOL ar5312GpioSet(struct ath_hal *, u_int32_t gpio, u_int32_t val);
extern	u_int32_t ar5312GpioGet(struct ath_hal *ah, u_int32_t gpio);
extern	void ar5312GpioSetIntr(struct ath_hal *ah, u_int, u_int32_t ilevel);
extern  void ar5312SetLedState(struct ath_hal *ah, HAL_LED_STATE state);
extern  HAL_BOOL ar5312DetectCardPresent(struct ath_hal *ah);
extern  void ar5312SetupClock(struct ath_hal *ah, HAL_OPMODE opmode);
extern  void ar5312RestoreClock(struct ath_hal *ah, HAL_OPMODE opmode);
extern  void ar5312DumpState(struct ath_hal *ah);
extern  HAL_BOOL ar5312Reset(struct ath_hal *ah, HAL_OPMODE opmode,
              HAL_CHANNEL *chan, HAL_BOOL bChannelChange, HAL_STATUS *status);
extern  HAL_BOOL ar5312ChipReset(struct ath_hal *ah, HAL_CHANNEL *chan);
extern  HAL_BOOL ar5312SetPowerMode(struct ath_hal *ah, HAL_POWER_MODE mode,
                                    int setChip);
extern  HAL_BOOL ar5312PhyDisable(struct ath_hal *ah);
extern  HAL_BOOL ar5312Disable(struct ath_hal *ah);
extern  HAL_BOOL ar5312MacReset(struct ath_hal *ah, unsigned int RCMask);
extern  u_int32_t ar5312GetPowerMode(struct ath_hal *ah);
extern  HAL_BOOL ar5312GetPowerStatus(struct ath_hal *ah);

/* BSP functions */
extern  HAL_BOOL ar5312SetupFlash(void);
extern	HAL_BOOL ar5312EepromRead(struct ath_hal *, u_int off, u_int16_t *data);
extern	HAL_BOOL ar5312EepromWrite(struct ath_hal *, u_int off, u_int16_t data);
extern  HAL_BOOL ar5312GetMacAddr(struct ath_hal *ah);

#endif	/* _ATH_AR3212_H_ */
