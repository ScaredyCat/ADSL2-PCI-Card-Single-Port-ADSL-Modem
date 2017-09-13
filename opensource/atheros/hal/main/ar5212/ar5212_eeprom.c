/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5212/ar5212_eeprom.c#1 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5212

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"
#ifdef AH_DEBUG
#include "ah_desc.h"			/* NB: for HAL_PHYERR* */
#endif
#include "ah_eeprom.h"

#include "ar5212/ar5212.h"
#include "ar5212/ar5212reg.h"
#include "ar5212/ar5212phy.h"
#ifdef AH_SUPPORT_AR5311
#include "ar5212/ar5311reg.h"
#endif

/*
 * Read 16 bits of data from offset into *data
 */
HAL_BOOL
ar5212EepromRead(struct ath_hal *ah, u_int off, u_int16_t *data)
{
	OS_REG_WRITE(ah, AR_EEPROM_ADDR, off);
	OS_REG_WRITE(ah, AR_EEPROM_CMD, AR_EEPROM_CMD_READ);

	if (!ath_hal_wait(ah, AR_EEPROM_STS,
	    AR_EEPROM_STS_READ_COMPLETE | AR_EEPROM_STS_READ_ERROR,
	    AR_EEPROM_STS_READ_COMPLETE)) {
		HALDEBUG(ah, "%s: read failed for entry 0x%x\n", __func__, off);
		return AH_FALSE;
	}
	*data = OS_REG_READ(ah, AR_EEPROM_DATA) & 0xffff;
	return AH_TRUE;
}

#ifdef AH_SUPPORT_WRITE_EEPROM
/*
 * Write 16 bits of data from data to the specified EEPROM offset.
 */
HAL_BOOL
ar5212EepromWrite(struct ath_hal *ah, u_int off, u_int16_t data)
{
	OS_REG_WRITE(ah, AR_EEPROM_ADDR, off);
	OS_REG_WRITE(ah, AR_EEPROM_DATA, data);
	OS_REG_WRITE(ah, AR_EEPROM_CMD, AR_EEPROM_CMD_WRITE);

	if (!ath_hal_wait(ah, AR_EEPROM_STS,
	    AR_EEPROM_STS_WRITE_COMPLETE | AR_EEPROM_STS_WRITE_ERROR,
	    AR_EEPROM_STS_WRITE_COMPLETE)) {
		HALDEBUG(ah, "%s: write failed for entry 0x%x, data 0x%x\n",
			__func__, off, data);
		return AH_FALSE;
	}
	return AH_TRUE;
}
#endif /* AH_SUPPORT_WRITE_EEPROM */

#endif /* AH_SUPPORT_AR5212 */
