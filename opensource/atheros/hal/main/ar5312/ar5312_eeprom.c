/*
 * Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 */
#include "opt_ah.h"


#ifdef AH_SUPPORT_AR5312

#include "ah.h"
#include "ah_internal.h"

#include "ar5312/ar5312.h"
#include "ar5312/ar5312reg.h"
#include "ar5212/ar5212desc.h"

extern struct ar531x_boarddata *ar5312_boardConfig;
extern char *radioConfig;

/*
 * Read 16 bits of data from offset into *data
 */

HAL_BOOL
ar5312EepromRead(struct ath_hal *ah, u_int off, u_int16_t *dataIn)
{
        int wlanNum,i,offset;
	char *eepromAddr = radioConfig, *data=AH_NULL;
	
        if ((wlanNum = GETWMACNUM(ah)) == -1) {
                return (AH_FALSE);
        }
	data = (u_int8_t *) dataIn;
	for (i=0,offset=2*off; i<2; i++,offset++) {
		data[i] = eepromAddr[offset];
	}
        return(AH_TRUE);
}

#ifdef AH_SUPPORT_WRITE_EEPROM
/*
 * Write 16 bits of data from data to the specified EEPROM offset.
 */
HAL_BOOL
ar5312EepromWrite(struct ath_hal *ah, u_int off, u_int16_t data)
{
        int wlanNum;

        if ((wlanNum = GETWMACNUM(ah)) == -1) {
                return (AH_FALSE);
        }
	ath_hal_printf(ah,"ar5312EepromWrite not implemented. off=0x%8.8x\n", 
		       2*off);
        return(AH_TRUE);
}
#endif /* AH_SUPPORT_WRITE_EEPROM */

#endif /* AH_SUPPORT_AR5312 */
