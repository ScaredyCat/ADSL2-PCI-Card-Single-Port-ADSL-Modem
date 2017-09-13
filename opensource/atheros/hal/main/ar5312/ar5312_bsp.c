/*
 * Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5312/ar5312_bsp.c#1 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5312

#include "ah.h"
#include "ah_internal.h"

#include "ar5312/ar5312.h"
#include "ar5312/ar5312reg.h"
#include "ar5212/ar5212desc.h"

struct ar531x_boarddata *ar5312_boardConfig=AH_NULL;
char *radioConfig=AH_NULL;

int
ar5312_get_board_config(void)
{
	int dataFound;
	char *bd_config;
	
	/*
	 * Find start of Board Configuration data, using heuristics:
	 * Search back from the (aliased) end of flash by 0x1000 bytes
	 * at a time until we find the string "5311", which marks the
	 * start of Board Configuration.  Give up if we've searched
	 * more than 500KB.
	 */
	dataFound = 0;
	for (bd_config = (char *)0xbffff000;
	     bd_config > (char *)0xbff80000;
	     bd_config -= 0x1000) {
		if ( *(int *)bd_config == AR531X_BD_MAGIC) {
			dataFound = 1;
			break;
		}
	}
	
	if (!dataFound) {
		ath_hal_printf(AH_NULL,
			       "Could not find Board Configuration Data\n");
		bd_config = AH_NULL;
	}
	ar5312_boardConfig = (struct ar531x_boarddata *) bd_config;
	return(dataFound);
}

int
ar5312_get_radio_config(void)
{
	int dataFound;
	char *radio_config;
	
	/* 
	 * Now find the start of Radio Configuration data, using heuristics:
	 * Search forward from Board Configuration data by 0x1000 bytes
	 * at a time until we find non-0xffffffff.
	 */
	dataFound = 0;
	for (radio_config = ((char *) ar5312_boardConfig) + 0x1000;
	     radio_config < (char *)0xbffff000;
	     radio_config += 0x1000) {
		if (*(int *)radio_config != 0xffffffff) {
			dataFound = 1;
			break;
		}
	}

	if (!dataFound) {
	    dataFound = 0;
	    for (radio_config = ((char *) ar5312_boardConfig) + 0xf8;
		 radio_config < (char *)0xbffff0f8;
		 radio_config += 0x1000) {
		if (*(int *)radio_config != 0xffffffff) {
		    dataFound = 1;
		    break;
		}
	    }
	}

	if (!dataFound) {
		ath_hal_printf(AH_NULL,
			       "Could not find Radio Configuration data\n");
		radio_config = AH_NULL;
	}
	radioConfig = radio_config;
	return(dataFound);
}

HAL_BOOL
ar5312SetupFlash(void)
{
	if (ar5312_get_board_config()) {
		if (ar5312_get_radio_config()) {
			return(AH_TRUE);
		}
	}
	return(AH_FALSE);
}	

HAL_BOOL
ar5312GetMacAddr(struct ath_hal *ah)
{
        int wlanNum;
        struct ath_hal_5212 *ahp= AH5212(ah);
        char *macAddr;

        if ((wlanNum = GETWMACNUM(ah)) == -1) {
                return (AH_FALSE);
        }
	if (ar5312_boardConfig == AH_NULL) {
		return(AH_FALSE);
	}
	switch(wlanNum) {
	case 0:
		macAddr = ar5312_boardConfig->wlan0Mac;
		break;
	case 1:
		macAddr = ar5312_boardConfig->wlan1Mac;
		break;
	default:
		ath_hal_printf(ah, "Invalid WLAN wmac index (%d)\n",
			       wlanNum);
		return(AH_FALSE);
	}
	OS_MEMCPY(&(ahp->ah_macaddr[0]),macAddr,6);
	return(AH_TRUE);
}

#endif /* AH_SUPPORT_AR5312 */
