
#ifndef __AUTOCHSELECT_H__
#define __AUTOCHSELECT_H__

#define RSSI_TO_DBM_OFFSET 120 // RSSI-115 = dBm


typedef struct {
	ULONG dirtyness[MAX_NUM_OF_CHANNELS+1];
	ULONG max_rssi[MAX_NUM_OF_CHANNELS+1];
	ULONG total_rssi[MAX_NUM_OF_CHANNELS+1];
	UINT32 FalseCCA[MAX_NUM_OF_CHANNELS+1];
} CHANNELINFO, *PCHANNELINFO;

typedef struct {
	UCHAR Bssid[MAC_ADDR_LEN];
	UCHAR SsidLen;
	CHAR Ssid[MAX_LEN_OF_SSID];
	UCHAR Channel;
	UCHAR CentralChannel;	//Store the wide-band central channel for 40MHz.  .used in 40MHz AP. Or this is the same as Channel.
	UCHAR Rssi;
} BSSENTRY, *PBSSENTRY;

typedef struct {
	UCHAR BssNr;
	BSSENTRY BssEntry[MAX_LEN_OF_BSS_TABLE];	
} BSSINFO, *PBSSINFO;

#endif // __AUTOCHSELECT_H__ //

