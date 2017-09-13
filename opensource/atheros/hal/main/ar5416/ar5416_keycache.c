/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_keycache.c#1 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_internal.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416desc.h"

/*
 * Note: The key cache hardware requires that each double-word
 * pair be written in even/odd order (since the destination is
 * a 64-bit register).  Don't reorder the writes in this code
 * w/o considering this!
 */
#define	KEY_XOR			0xaa

#define	IS_MIC_ENABLED(ah) \
	(AH5416(ah)->ah_staId1Defaults & AR_STA_ID1_CRPT_MIC_ENABLE)

/*
 * Return the size of the hardware key cache.
 */
u_int32_t
ar5416GetKeyCacheSize(struct ath_hal *ah)
{
	return AH_PRIVATE(ah)->ah_caps.halKeyCacheSize;
}

/*
 * Return true if the specific key cache entry is valid.
 */
HAL_BOOL
ar5416IsKeyCacheEntryValid(struct ath_hal *ah, u_int16_t entry)
{
	if (entry < AH_PRIVATE(ah)->ah_caps.halKeyCacheSize) {
		u_int32_t val = OS_REG_READ(ah, AR_KEYTABLE_MAC1(entry));
		if (val & AR_KEYTABLE_VALID)
			return AH_TRUE;
	}
	return AH_FALSE;
}

/*
 * Clear the specified key cache entry and any associated MIC entry.
 */
HAL_BOOL
ar5416ResetKeyCacheEntry(struct ath_hal *ah, u_int16_t entry)
{
	u_int32_t keyType;

	if (entry >= AH_PRIVATE(ah)->ah_caps.halKeyCacheSize) {
		HALDEBUG(ah, "%s: entry %u out of range\n", __func__, entry);
		return AH_FALSE;
	}
	keyType = OS_REG_READ(ah, AR_KEYTABLE_TYPE(entry));

	/* XXX why not clear key type/valid bit first? */
	OS_REG_WRITE(ah, AR_KEYTABLE_KEY0(entry), 0);
	OS_REG_WRITE(ah, AR_KEYTABLE_KEY1(entry), 0);
	OS_REG_WRITE(ah, AR_KEYTABLE_KEY2(entry), 0);
	OS_REG_WRITE(ah, AR_KEYTABLE_KEY3(entry), 0);
	OS_REG_WRITE(ah, AR_KEYTABLE_KEY4(entry), 0);
	OS_REG_WRITE(ah, AR_KEYTABLE_TYPE(entry), AR_KEYTABLE_TYPE_CLR);
	OS_REG_WRITE(ah, AR_KEYTABLE_MAC0(entry), 0);
	OS_REG_WRITE(ah, AR_KEYTABLE_MAC1(entry), 0);
	if (keyType == AR_KEYTABLE_TYPE_TKIP && IS_MIC_ENABLED(ah)) {
		u_int16_t micentry = entry+64;	/* MIC goes at slot+64 */

		HALASSERT(micentry < AH_PRIVATE(ah)->ah_caps.halKeyCacheSize);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY0(micentry), 0);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY1(micentry), 0);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY2(micentry), 0);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY3(micentry), 0);
		/* NB: key type and MAC are known to be ok */
	}
	return AH_TRUE;
}

/*
 * Sets the mac part of the specified key cache entry (and any
 * associated MIC entry) and mark them valid.
 */
HAL_BOOL
ar5416SetKeyCacheEntryMac(struct ath_hal *ah, u_int16_t entry, const u_int8_t *mac)
{
	u_int32_t macHi, macLo;

	if (entry >= AH_PRIVATE(ah)->ah_caps.halKeyCacheSize) {
		HALDEBUG(ah, "%s: entry %u out of range\n", __func__, entry);
		return AH_FALSE;
	}
	/*
	 * Set MAC address -- shifted right by 1.  MacLo is
	 * the 4 MSBs, and MacHi is the 2 LSBs.
	 */
	if (mac != AH_NULL) {
		macHi = (mac[5] << 8) | mac[4];
		macLo = (mac[3] << 24)| (mac[2] << 16)
		      | (mac[1] << 8) | mac[0];
		macLo >>= 1;
		macLo |= (macHi & 1) << 31;	/* carry */
		macHi >>= 1;
	} else {
		macLo = macHi = 0;
	}
	OS_REG_WRITE(ah, AR_KEYTABLE_MAC0(entry), macLo);
	OS_REG_WRITE(ah, AR_KEYTABLE_MAC1(entry), macHi | AR_KEYTABLE_VALID);
	return AH_TRUE;
}

/*
 * Sets the contents of the specified key cache entry
 * and any associated MIC entry.
 */
HAL_BOOL
ar5416SetKeyCacheEntry(struct ath_hal *ah, u_int16_t entry,
                       const HAL_KEYVAL *k, const u_int8_t *mac,
                       int xorKey)
{
	const HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;
	u_int32_t key0, key1, key2, key3, key4;
	u_int32_t keyType;
	u_int32_t xorMask = xorKey ?
		(KEY_XOR << 24 | KEY_XOR << 16 | KEY_XOR << 8 | KEY_XOR) : 0;

	if (entry >= pCap->halKeyCacheSize) {
		HALDEBUG(ah, "%s: entry %u out of range\n", __func__, entry);
		return AH_FALSE;
	}
	switch (k->kv_type) {
	case HAL_CIPHER_AES_OCB:
		keyType = AR_KEYTABLE_TYPE_AES;
		break;
	case HAL_CIPHER_AES_CCM:
		if (!pCap->halCipherAesCcmSupport) {
			HALDEBUG(ah, "%s: AES-CCM not supported by "
				"mac rev 0x%x\n",
				__func__, AH_PRIVATE(ah)->ah_macRev);
			return AH_FALSE;
		}
		keyType = AR_KEYTABLE_TYPE_CCM;
		break;
	case HAL_CIPHER_TKIP:
		keyType = AR_KEYTABLE_TYPE_TKIP;
		if (IS_MIC_ENABLED(ah) && entry+64 >= pCap->halKeyCacheSize) {
			HALDEBUG(ah, "%s: entry %u inappropriate for TKIP\n",
				__func__, entry);
			return AH_FALSE;
		}
		break;
	case HAL_CIPHER_WEP:
		if (k->kv_len < 40 / NBBY) {
			HALDEBUG(ah, "%s: WEP key length %u too small\n",
				__func__, k->kv_len);
			return AH_FALSE;
		}
		if (k->kv_len <= 40 / NBBY)
			keyType = AR_KEYTABLE_TYPE_40;
		else if (k->kv_len <= 104 / NBBY)
			keyType = AR_KEYTABLE_TYPE_104;
		else
			keyType = AR_KEYTABLE_TYPE_128;
		break;
	case HAL_CIPHER_CLR:
		keyType = AR_KEYTABLE_TYPE_CLR;
		break;
	default:
		HALDEBUG(ah, "%s: cipher %u not supported\n",
			__func__, k->kv_type);
		return AH_FALSE;
	}

	key0 = LE_READ_4(k->kv_val+0) ^ xorMask;
	key1 = (LE_READ_2(k->kv_val+4) ^ xorMask) & 0xffff;
	key2 = LE_READ_4(k->kv_val+6) ^ xorMask;
	key3 = (LE_READ_2(k->kv_val+10) ^ xorMask) & 0xffff;
	key4 = LE_READ_4(k->kv_val+12) ^ xorMask;
	if (k->kv_len <= 104 / NBBY)
		key4 &= 0xff;

	/*
	 * Note: key cache hardware requires that each double-word
	 * pair be written in even/odd order (since the destination is
	 * a 64-bit register).  Don't reorder these writes w/o
	 * considering this!
	 */
	if (keyType == AR_KEYTABLE_TYPE_TKIP && IS_MIC_ENABLED(ah)) {
		u_int16_t micentry = entry+64;	/* MIC goes at slot+64 */
		u_int32_t mic0, mic2;

		/*
		 * Invalidate the encrypt/decrypt key until the MIC
		 * key is installed so pending rx frames will fail
		 * with decrypt errors rather than a MIC error.
		 */
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY0(entry), ~key0);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY1(entry), ~key1);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY2(entry), key2);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY3(entry), key3);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY4(entry), key4);
		OS_REG_WRITE(ah, AR_KEYTABLE_TYPE(entry), keyType);
		(void) ar5416SetKeyCacheEntryMac(ah, entry, mac);

		mic0 = LE_READ_4(k->kv_mic+0);
		mic2 = LE_READ_4(k->kv_mic+4);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY0(micentry), mic0);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY1(micentry), 0);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY2(micentry), mic2);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY3(micentry), 0);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY4(micentry), 0);
		OS_REG_WRITE(ah, AR_KEYTABLE_TYPE(micentry),
			AR_KEYTABLE_TYPE_CLR);
		/* NB: MIC key is not marked valid and has no MAC address */
		OS_REG_WRITE(ah, AR_KEYTABLE_MAC0(micentry), 0);
		OS_REG_WRITE(ah, AR_KEYTABLE_MAC1(micentry), 0);

		/* correct intentionally corrupted key */
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY0(entry), key0);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY1(entry), key1);
	} else {
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY0(entry), key0);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY1(entry), key1);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY2(entry), key2);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY3(entry), key3);
		OS_REG_WRITE(ah, AR_KEYTABLE_KEY4(entry), key4);
		OS_REG_WRITE(ah, AR_KEYTABLE_TYPE(entry), keyType);

		(void) ar5416SetKeyCacheEntryMac(ah, entry, mac);
	}
	return AH_TRUE;
}
#endif /* AH_SUPPORT_AR5416 */
