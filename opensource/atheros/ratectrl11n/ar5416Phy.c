/*
 *  Copyright (c) 2000-2002 Atheros Communications, Inc., All Rights Reserved
 *
 */

#ident "$Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/ratectrl11n/ar5416Phy.c#16 $"

#ifdef __FreeBSD__
#include <dev/ath/ath_rate/atheros/ratectrl.h>
#else
#include "ratectrl11n.h"
#endif

#define SHORT_PRE 1
#define LONG_PRE 0

#define WLAN_PHY_TURBO          WLAN_RC_PHY_TURBO
#define WLAN_PHY_CCK            WLAN_RC_PHY_CCK
#define WLAN_PHY_OFDM           WLAN_RC_PHY_OFDM
#define WLAN_PHY_HT_20_SS       WLAN_RC_PHY_HT_20_SS
#define WLAN_PHY_HT_20_DS       WLAN_RC_PHY_HT_20_DS
#define WLAN_PHY_HT_20_DS_HGI   WLAN_RC_PHY_HT_20_DS_HGI
#define WLAN_PHY_HT_40_SS       WLAN_RC_PHY_HT_40_SS
#define WLAN_PHY_HT_40_DS       WLAN_RC_PHY_HT_40_DS
#define WLAN_PHY_HT_40_DS_HGI   WLAN_RC_PHY_HT_40_DS_HGI


static RATE_TABLE ar5416_11naRateTable = {

    41,  /* number of rates */
    {/*                                                             rate    short     dot11   ctrl  RssiAck  RssiAck */
     /*              valid                           Kbps   uKbps    Code   Preamble    Rate   Rate ValidMin DeltaMin*/ 
     /*    6 Mb */ {  TRUE_ALL,  WLAN_PHY_OFDM,          6000,   5400,  0x0b,    0x00,        12,   0,       2,       1,      0,      0,      0,      0},
     /*    9 Mb */ {  TRUE_ALL,  WLAN_PHY_OFDM,          9000,   7800,  0x0f,    0x00,        18,   0,       3,       1,      1,      1,      1,      1},
     /*   12 Mb */ {  TRUE,  WLAN_PHY_OFDM,         12000,  10000,  0x0a,    0x00,        24,   2,       4,       2,      2,      2,      2,      2},
     /*   18 Mb */ {  TRUE,  WLAN_PHY_OFDM,         18000,  13900,  0x0e,    0x00,        36,   2,       6,       2,      3,      3,      3,      3},
     /*   24 Mb */ {  TRUE,  WLAN_PHY_OFDM,         24000,  17300,  0x09,    0x00,        48,   4,      10,       3,      4,      4,      4,      4},
     /*   36 Mb */ {  TRUE,  WLAN_PHY_OFDM,         36000,  23000,  0x0d,    0x00,        72,   4,      14,       3,      5,      5,      5,      5},
     /*   48 Mb */ {  TRUE,  WLAN_PHY_OFDM,         48000,  27400,  0x08,    0x00,        96,   4,      19,       3,      6,      6,      6,      6},
     /*   54 Mb */ {  TRUE,  WLAN_PHY_OFDM,         54000,  29300,  0x0c,    0x00,       108,   4,      23,       3,      7,      7,      7,      7},
     /*  6.5 Mb */ {  FALSE,  WLAN_PHY_HT_20_SS,      6500,   6400,  0x80,    0x00,         0,   8,      23,       3,      8,     24,      8,     24},
     /*   13 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,     13000,  12700,  0x81,    0x00,         1,   8,      23,       3,      9,     25,      9,     25},
     /* 19.5 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,     19500,  18800,  0x82,    0x00,         2,   8,      23,       3,     10,     26,     10,     26},
     /*   26 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,     26000,  25000,  0x83,    0x00,         3,   8,      23,       3,     11,     27,     11,     27},
     /*   39 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,     39000,  36700,  0x84,    0x00,         4,   8,      23,       3,     12,     28,     12,     28},
     /*   52 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,     52000,  48100,  0x85,    0x00,         5,   8,      23,       3,     13,     29,     13,     29},
     /* 58.5 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,     58500,  53500,  0x86,    0x00,         6,   8,      23,       3,     14,     30,     14,     30},
     /*   65 Mb */ {  FALSE,  WLAN_PHY_HT_20_SS,     65000,  59000,  0x87,    0x00,         7,   8,      23,       3,     15,     31,     15,     31},
     /*   13 Mb */ {  FALSE,  WLAN_PHY_HT_20_DS,     13000,  12700,  0x88,    0x00,         8,   8,      23,       3,     16,     32,     16,     32},
     /*   26 Mb */ {  FALSE,  WLAN_PHY_HT_20_DS,     26000,  24800,  0x89,    0x00,         9,   8,      23,       3,     17,     33,     17,     33},
     /*   39 Mb */ {  FALSE,  WLAN_PHY_HT_20_DS,     39000,  36600,  0x8a,    0x00,        10,   8,      23,       3,     18,     34,     18,     34},
     /*   52 Mb */ {  FALSE,  WLAN_PHY_HT_20_DS,     52000,  48100,  0x8b,    0x00,        11,   8,      23,       3,     19,     35,     19,     35},
     /*   78 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_DS,     78000,  69500,  0x8c,    0x00,        12,   8,      23,       3,     20,     36,     20,     36},
     /*  104 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_DS,    104000,  89500,  0x8d,    0x00,        13,   8,      23,       3,     21,     37,     21,     37},
     /*  117 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_DS,    117000,  98900,  0x8e,    0x00,        14,   8,      23,       3,     22,     38,     22,     38},
     /*  130 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_DS,    130000, 108300,  0x8f,    0x00,        15,   8,      23,       3,     23,     39,     40,     40},
     /* 13.5 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,     13500,  13200,  0x80,    0x00,         0,   8,      23,       3,      8,     24,     24,     24},
     /* 27.0 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,     27500,  25900,  0x81,    0x00,         1,   8,      23,       3,      9,     25,     25,     25},
     /* 40.5 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,     40500,  38600,  0x82,    0x00,         2,   8,      23,       3,     10,     26,     26,     26},
     /*   54 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,     54000,  49800,  0x83,    0x00,         3,   8,      23,       3,     11,     27,     27,     27},
     /*   81 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,     81500,  72200,  0x84,    0x00,         4,   8,      23,       3,     12,     28,     28,     28},
     /*  108 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,    108000,  92900,  0x85,    0x00,         5,   8,      23,       3,     13,     29,     29,     29},
     /* 121.5Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,    121500, 102700,  0x86,    0x00,         6,   8,      23,       3,     14,     30,     30,     30},
     /*  135 Mb */ {  FALSE,  WLAN_PHY_HT_40_SS,    135000, 112000,  0x87,    0x00,         7,   8,      23,       3,     15,     31,     31,     31},
     /*   27 Mb */ {  FALSE,  WLAN_PHY_HT_40_DS,     27000,  25800,  0x88,    0x00,         8,   8,      23,       3,     16,     32,     32,     32},
     /*   54 Mb */ {  FALSE,  WLAN_PHY_HT_40_DS,     54000,  49800,  0x89,    0x00,         9,   8,      23,       3,     17,     33,     33,     33},
     /*   81 Mb */ {  FALSE,  WLAN_PHY_HT_40_DS,     81000,  71900,  0x8a,    0x00,        10,   8,      23,       3,     18,     34,     34,     34},
     /*  108 Mb */ {  FALSE,  WLAN_PHY_HT_40_DS,    108000,  92500,  0x8b,    0x00,        11,   8,      23,       3,     19,     35,     35,     35},
     /*  162 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS,    162000, 130300,  0x8c,    0x00,        12,   8,      23,       3,     20,     36,     36,     36},
     /*  216 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS,    216000, 162800,  0x8d,    0x00,        13,   8,      23,       3,     21,     37,     37,     37},
     /*  243 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS,    243000, 178200,  0x8e,    0x00,        14,   8,      23,       3,     22,     38,     38,     38},
     /*  270 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS,    270000, 192100,  0x8f,    0x00,        15,   8,      23,       3,     23,     39,     40,     40},
     /*  300 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS_HGI,300000, 207000,  0x8f,    0x00,        15,   8,      23,       3,     23,     39,     40,     40},
    },    
    50,  /* probe interval */    
    50,  /* rssi reduce interval */    
    WLAN_RC_HT_FLAG,  /* Phy rates allowed initially */
};

	/* TRUE_ALL - valid for 20/40/Legacy, TRUE - Legacy only, TRUE_20 - HT 20 only, TRUE_40 - HT 40 only */

static RATE_TABLE ar5416_11ngRateTable = {

    45,  /* number of rates - should match the no. of rows below */
    {/*                                                          short    dot11 ctrl  RssiAck  RssiAck*/ 
     /*              valid                      Kbps  uKbps  RC  Preamble Rate  Rate  ValidMin DeltaMin*/
     /*    1 Mb */ {  TRUE_ALL,WLAN_PHY_CCK,     1000, 900,  0x1b, 0x00,   2,    0,     0,     1,      0,    0,     0,     0},
     /*    2 Mb */ {  TRUE_ALL,WLAN_PHY_CCK,     2000, 1900, 0x1a, 0x04,   4,    1,     1,     1,      1,    1,     1,     1},
     /*  5.5 Mb */ {  TRUE_ALL,WLAN_PHY_CCK,     5500, 4900, 0x19, 0x04,   11,   2,     2,     2,      2,    2,     2,     2},
     /*   11 Mb */ {  TRUE_ALL,WLAN_PHY_CCK,     11000,8100, 0x18, 0x04,   22,   3,     3,     2,      3,    3,     3,     3},
     /*    6 Mb */ {  FALSE,   WLAN_PHY_OFDM,    6000, 5400, 0x0b, 0x00,   12,   4,     2,     1,      4,    4,     4,     4},
     /*    9 Mb */ {  FALSE,   WLAN_PHY_OFDM,    9000, 7800, 0x0f, 0x00,   18,   4,     3,     1,      5,    5,     5,     5},
     /*   12 Mb */ {  TRUE,    WLAN_PHY_OFDM,    12000,10100,0x0a, 0x00,   24,   6,     4,     1,      6,    6,     6,     6},
     /*   18 Mb */ {  TRUE,    WLAN_PHY_OFDM,    18000,14100,0x0e, 0x00,   36,   6,     6,     2,      7,    7,     7,     7},
     /*   24 Mb */ {  TRUE,    WLAN_PHY_OFDM,    24000,17700,0x09, 0x00,   48,   8,     10,    3,      8,    8,     8,     8},
     /*   36 Mb */ {  TRUE,    WLAN_PHY_OFDM,    36000,23700,0x0d, 0x00,   72,   8,     14,    3,      9,    9,     9,     9},
     /*   48 Mb */ {  TRUE,    WLAN_PHY_OFDM,    48000,27400,0x08, 0x00,   96,   8,     20,    3,     10,   10,    10,    10},
     /*   54 Mb */ {  TRUE,    WLAN_PHY_OFDM,    54000,30900,0x0c, 0x00,  108,   8,     23,    3,     11,   11,    11,    11},
     /*  6.5 Mb */ {  FALSE,    WLAN_PHY_HT_20_SS,6500, 6400, 0x80, 0x00,    0,   8,      2,    3,     12,   28,    12,    28},
     /*   13 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,13000,12700,0x81, 0x00,    1,   8,      4,    3,     13,   29,    13,    29},
     /* 19.5 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,19500,18800,0x82, 0x00,    2,   8,      6,    3,     14,   30,    14,    30},
     /*   26 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,26000,25000,0x83, 0x00,    3,   8,     10,    3,     15,   31,    15,    31},
     /*   39 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,39000,36700,0x84, 0x00,    4,   8,     14,    3,     16,   32,    16,    32},
     /*   52 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,52000,48100,0x85, 0x00,    5,   8,     20,    3,     17,   33,    17,    33},
     /* 58.5 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_SS,58500,53500,0x86, 0x00,    6,   8,     23,    3,     18,   34,    18,    34},
     /*   65 Mb */ {  FALSE,    WLAN_PHY_HT_20_SS,65000,59000,0x87, 0x00,    7,   8,     25,    3,     19,   35,    19,    35},
     /*   13 Mb */ {  FALSE,    WLAN_PHY_HT_20_DS,13000,12700,0x88, 0x00,    8,   8,      2,    3,     20,   36,    20,    36},
     /*   26 Mb */ {  FALSE,    WLAN_PHY_HT_20_DS,26000,24800,0x89, 0x00,    9,   8,      4,    3,     21,   37,    21,    37},
     /*   39 Mb */ {  FALSE,    WLAN_PHY_HT_20_DS,39000,36600,0x8a, 0x00,   10,   8,      6,    3,     22,   38,    22,    38},
     /*   52 Mb */ {  FALSE,    WLAN_PHY_HT_20_DS,52000,48100,0x8b, 0x00,   11,   8,      10,   3,     23,   39,    23,    39},
     /*   78 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_DS,78000,69500,0x8c, 0x00,   12,   8,      14,   3,     24,   40,    24,    40},
     /*  104 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_DS,104000,89500,0x8d,0x00,   13,   8,      20,   3,     25,   41,    25,    41},
     /*  117 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_DS,117000,98900,0x8e,0x00,   14,   8,      23,   3,     26,   42,    26,    43},
     /*  130 Mb */ {  TRUE_20,  WLAN_PHY_HT_20_DS,130000,108300,0x8f,0x00,  15,   8,      25,   3,     27,   43,    27,    44},
     /* 13.5 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,13500, 13200, 0x80,0x00,   0,   8,       2,   3,     12,   28,    28,    28},
     /* 27.0 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,27500, 25900, 0x81,0x00,   1,   8,       4,   3,     13,   29,    29,    29},
     /* 40.5 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,40500, 38600, 0x82,0x00,   2,   8,       6,   3,     14,   30,    30,    30},
     /*   54 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,54000, 49800, 0x83,0x00,   3,   8,      10,   3,     15,   31,    31,    31},
     /*   81 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,81500, 72200, 0x84,0x00,   4,   8,      14,   3,     16,   32,    32,    32},
     /*  108 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,108000,92900, 0x85,0x00,   5,   8,      20,   3,     17,   33,    33,    33},
     /* 121.5Mb */ {  TRUE_40,  WLAN_PHY_HT_40_SS,121500,102700,0x86,0x00,   6,   8,      23,   3,     18,   34,    34,    34},
     /*  135 Mb */ {  FALSE,    WLAN_PHY_HT_40_SS,135000,112000,0x87,0x00,   7,   8,      25,   3,     19,   35,    35,    35},
     /*   27 Mb */ {  FALSE,    WLAN_PHY_HT_40_DS,27000, 25800, 0x88,0x00,   8,   8,       2,   3,     20,   36,    36,    36},
     /*   54 Mb */ {  FALSE,    WLAN_PHY_HT_40_DS,54000, 49800, 0x89,0x00,   9,   8,       4,   3,     21,   37,    37,    37},
     /*   81 Mb */ {  FALSE,    WLAN_PHY_HT_40_DS,81000, 71900, 0x8a,0x00,  10,   8,       6,   3,     22,   38,    38,    38},
     /*  108 Mb */ {  FALSE,    WLAN_PHY_HT_40_DS,108000,92500, 0x8b,0x00,  11,   8,      10,   3,     23,   39,    39,    39},
     /*  162 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS,162000,130300,0x8c,0x00,  12,   8,      14,   3,     24,   40,    40,    40},
     /*  216 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS,216000,162800,0x8d,0x00,  13,   8,      20,   3,     25,   41,    41,    41},
     /*  243 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS,243000,178200,0x8e,0x00,  14,   8,      23,   3,     26,   42,    43,    42},
     /*  270 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS_HGI,270000,192100,0x8e,0x00,14, 8,      25,   3,     26,   43,    43,    43},
     /*  300 Mb */ {  TRUE_40,  WLAN_PHY_HT_40_DS_HGI,300000,207000, 0x8f,0x00,15,8,      25,   3,     27,   44,    44,    44},
    },    
    50,  /* probe interval */     
    50,  /* rssi reduce interval */    
    WLAN_RC_HT_FLAG,  /* Phy rates allowed initially */
};

static RATE_TABLE ar5416_11aRateTable = {
    8,  /* number of rates */
    {/*                                                             short            ctrl  RssiAck  RssiAck*/
     /*              valid                 Kbps  uKbps  rateCode Preamble  dot11Rate Rate ValidMin DeltaMin*/
     /*   6 Mb */ {  TRUE, WLAN_PHY_OFDM,  6000,  5400,   0x0b,    0x00, (0x80|12),   0,       2,       1,      0}, 
     /*   9 Mb */ {  TRUE, WLAN_PHY_OFDM,  9000,  7800,   0x0f,    0x00,        18,   0,       3,       1,      1}, 
     /*  12 Mb */ {  TRUE, WLAN_PHY_OFDM, 12000, 10000,   0x0a,    0x00, (0x80|24),   2,       4,       2,      2}, 
     /*  18 Mb */ {  TRUE, WLAN_PHY_OFDM, 18000, 13900,   0x0e,    0x00,        36,   2,       6,       2,      3}, 
     /*  24 Mb */ {  TRUE, WLAN_PHY_OFDM, 24000, 17300,   0x09,    0x00, (0x80|48),   4,      10,       3,      4}, 
     /*  36 Mb */ {  TRUE, WLAN_PHY_OFDM, 36000, 23000,   0x0d,    0x00,        72,   4,      14,       3,      5}, 
     /*  48 Mb */ {  TRUE, WLAN_PHY_OFDM, 48000, 27400,   0x08,    0x00,        96,   4,      19,       3,      6}, 
     /*  54 Mb */ {  TRUE, WLAN_PHY_OFDM, 54000, 29300,   0x0c,    0x00,       108,   4,      23,       3,      7}, 
    },    
    50,  /* probe interval */    
    50,  /* rssi reduce interval */
    0,   /* Phy rates allowed initially */
};

static RATE_TABLE ar5416_11aRateTable_Half = {
    8,  /* number of rates */
    {/*                                                             short            ctrl  RssiAck  RssiAck*/
     /*              valid                 Kbps  uKbps  rateCode Preamble  dot11Rate Rate ValidMin DeltaMin*/
     /*   6 Mb */ {  TRUE, WLAN_PHY_OFDM,   3000,  2700,   0x0b,    0x00,  (0x80|6),   0,       2,       1,      0},
     /*   9 Mb */ {  TRUE, WLAN_PHY_OFDM,   4500,  3900,   0x0f,    0x00,         9,   0,       3,       1,      1},
     /*  12 Mb */ {  TRUE, WLAN_PHY_OFDM,   6000,  5000,   0x0a,    0x00, (0x80|12),   2,       4,       2,      2},
     /*  18 Mb */ {  TRUE, WLAN_PHY_OFDM,   9000,  6950,   0x0e,    0x00,        18,   2,       6,       2,      3},
     /*  24 Mb */ {  TRUE, WLAN_PHY_OFDM,  12000,  8650,   0x09,    0x00, (0x80|24),   4,      10,       3,      4},
     /*  36 Mb */ {  TRUE, WLAN_PHY_OFDM,  18000, 11500,   0x0d,    0x00,        36,   4,      14,       3,      5},
     /*  48 Mb */ {  TRUE, WLAN_PHY_OFDM,  24000, 13700,   0x08,    0x00,        48,   4,      19,       3,      6},
     /*  54 Mb */ {  TRUE, WLAN_PHY_OFDM,  27000, 14650,   0x0c,    0x00,        54,   4,      23,       3,      7},
    },    
    50,  /* probe interval */    
    50,  /* rssi reduce interval */
    0,   /* Phy rates allowed initially */
};

static RATE_TABLE ar5416_11aRateTable_Quarter = {
    8,  /* number of rates */
    {/*                                                             short            ctrl  RssiAck  RssiAck*/
     /*              valid                 Kbps  uKbps  rateCode Preamble  dot11Rate Rate ValidMin DeltaMin*/
     /*   6 Mb */ {  TRUE, WLAN_PHY_OFDM,   1500,  1350,   0x0b,    0x00,  (0x80|3),   0,       2,       1,      0},
     /*   9 Mb */ {  TRUE, WLAN_PHY_OFDM,   2250,  1950,   0x0f,    0x00,         4,   0,       3,       1,      1},
     /*  12 Mb */ {  TRUE, WLAN_PHY_OFDM,   3000,  2500,   0x0a,    0x00,  (0x80|6),   2,       4,       2,      2},
     /*  18 Mb */ {  TRUE, WLAN_PHY_OFDM,   4500,  3475,   0x0e,    0x00,         9,   2,       6,       2,      3},
     /*  24 Mb */ {  TRUE, WLAN_PHY_OFDM,   6000,  4325,   0x09,    0x00, (0x80|12),   4,      10,       3,      4},
     /*  36 Mb */ {  TRUE, WLAN_PHY_OFDM,   9000,  5750,   0x0d,    0x00,        18,   4,      14,       3,      5},
     /*  48 Mb */ {  TRUE, WLAN_PHY_OFDM,  12000,  6850,   0x08,    0x00,        24,   4,      19,       3,      6},
     /*  54 Mb */ {  TRUE, WLAN_PHY_OFDM,  13500,  7325,   0x0c,    0x00,        27,   4,      23,       3,      7},
    },    
    50,  /* probe interval */    
    50,  /* rssi reduce interval */
    0,   /* Phy rates allowed initially */
};

static RATE_TABLE ar5416_TurboRateTable = {
    8,  /* number of rates */
    {/*                                                              short            ctrl  RssiAck  RssiAck*/
     /*              valid                  Kbps  uKbps  rateCode Preamble  dot11Rate Rate ValidMin DeltaMin*/
     /*   6 Mb */ {  TRUE, WLAN_PHY_TURBO,  6000,  5400,   0x0b,    0x00, (0x80|12),   0,       2,       1,      0},
     /*   9 Mb */ {  TRUE, WLAN_PHY_TURBO,  9000,  7800,   0x0f,    0x00,        18,   0,       4,       1,      1},
     /*  12 Mb */ {  TRUE, WLAN_PHY_TURBO, 12000, 10000,   0x0a,    0x00, (0x80|24),   2,       7,       2,      2},
     /*  18 Mb */ {  TRUE, WLAN_PHY_TURBO, 18000, 13900,   0x0e,    0x00,        36,   2,       9,       2,      3},
     /*  24 Mb */ {  TRUE, WLAN_PHY_TURBO, 24000, 17300,   0x09,    0x00, (0x80|48),   4,      14,       3,      4},
     /*  36 Mb */ {  TRUE, WLAN_PHY_TURBO, 36000, 23000,   0x0d,    0x00,        72,   4,      17,       3,      5},
     /*  48 Mb */ {  TRUE, WLAN_PHY_TURBO, 48000, 27400,   0x08,    0x00,        96,   4,      22,       3,      6},
     /*  54 Mb */ {  TRUE, WLAN_PHY_TURBO, 54000, 29300,   0x0c,    0x00,       108,   4,      26,       3,      7},
    },    
    50, /* probe interval */    
    50,  /* rssi reduce interval */
    0,   /* Phy rates allowed initially */
};

/* Venice TODO: roundUpRate() is broken when the rate table does not represent rates
 * in increasing order  e.g.  5.5, 11, 6, 9.    
 * An average rate of 6 Mbps will currently map to 11 Mbps. 
 */
static RATE_TABLE ar5416_11gRateTable = {
    12,  /* number of rates */
    {/*                                                              short            ctrl  RssiAck  RssiAck*/
     /*              valid                  Kbps  uKbps  rateCode Preamble  dot11Rate Rate ValidMin DeltaMin*/
     /*   1 Mb */ {  TRUE,  WLAN_PHY_CCK,   1000,   900,  0x1b,    0x00,         2,   0,       0,       1,      0},
     /*   2 Mb */ {  TRUE,  WLAN_PHY_CCK,   2000,  1900,  0x1a,    0x04,         4,   1,       1,       1,      1},
     /* 5.5 Mb */ {  TRUE,  WLAN_PHY_CCK,   5500,  4900,  0x19,    0x04,        11,   2,       2,       2,      2},
     /*  11 Mb */ {  TRUE,  WLAN_PHY_CCK,  11000,  8100,  0x18,    0x04,        22,   3,       3,       2,      3},
     /*   6 Mb */ {  TRUE,  WLAN_PHY_OFDM, 6000,   5400,  0x0b,    0x00,        12,   4,       2,       1,      4},
     /*   9 Mb */ {  TRUE,  WLAN_PHY_OFDM, 9000,   7800,  0x0f,    0x00,        18,   4,       3,       1,      5},
     /*  12 Mb */ {  TRUE,  WLAN_PHY_OFDM, 12000, 10000,  0x0a,    0x00,        24,   6,       4,       1,      6},
     /*  18 Mb */ {  TRUE,  WLAN_PHY_OFDM, 18000, 13900,  0x0e,    0x00,        36,   6,       6,       2,      7},
     /*  24 Mb */ {  TRUE,  WLAN_PHY_OFDM, 24000, 17300,  0x09,    0x00,        48,   8,      10,       3,      8},
     /*  36 Mb */ {  TRUE,  WLAN_PHY_OFDM, 36000, 23000,  0x0d,    0x00,        72,   8,      14,       3,      9},
     /*  48 Mb */ {  TRUE,  WLAN_PHY_OFDM, 48000, 27400,  0x08,    0x00,        96,   8,      19,       3,     10},
     /*  54 Mb */ {  TRUE,  WLAN_PHY_OFDM, 54000, 29300,  0x0c,    0x00,       108,   8,      23,       3,     11},
    },    
    50,  /* probe interval */    
    50,  /* rssi reduce interval */    
    0,   /* Phy rates allowed initially */    
};

static RATE_TABLE ar5416_11bRateTable = {
    4,  /* number of rates */
    {/*                                                             short            ctrl  RssiAck  RssiAck*/
     /*              valid                 Kbps  uKbps  rateCode Preamble  dot11Rate Rate ValidMin DeltaMin*/
     /*   1 Mb */ {  TRUE,  WLAN_PHY_CCK,  1000,  900,  0x1b,    0x00, (0x80| 2),   0,       0,       1,      0},
     /*   2 Mb */ {  TRUE,  WLAN_PHY_CCK,  2000, 1800,  0x1a,    0x04, (0x80| 4),   1,       1,       1,      1},
     /* 5.5 Mb */ {  TRUE,  WLAN_PHY_CCK,  5500, 4300,  0x19,    0x04, (0x80|11),   1,       2,       2,      2},
     /*  11 Mb */ {  TRUE,  WLAN_PHY_CCK, 11000, 7100,  0x18,    0x04, (0x80|22),   1,       4,     100,      3},
    },    
    100, /* probe interval */    
    100, /* rssi reduce interval */    
    0,   /* Phy rates allowed initially */
};

void
ar5416SetupRateTables(void)
{
    atheros_setuptable(&ar5416_11bRateTable);
    atheros_setuptable(&ar5416_11aRateTable);
    atheros_setuptable(&ar5416_11gRateTable);
    atheros_setuptable(&ar5416_TurboRateTable);
    atheros_setuptable(&ar5416_11ngRateTable);
    atheros_setuptable(&ar5416_11naRateTable);
}

void
ar5416AttachRateTables(struct atheros_softc *sc)
{
    /*
     * Attach device specific rate tables; for ar5212.
     * 11a static turbo and 11g static turbo share the same table.
     * Dynamic turbo uses combined rate table.
     */
    sc->hwRateTable[WIRELESS_MODE_11b]   = &ar5416_11bRateTable;
    sc->hwRateTable[WIRELESS_MODE_11a]   = &ar5416_11aRateTable;
    sc->hwRateTable[WIRELESS_MODE_11g]   = &ar5416_11gRateTable;
    sc->hwRateTable[WIRELESS_MODE_108a]  = &ar5416_TurboRateTable;
    sc->hwRateTable[WIRELESS_MODE_108g]  = &ar5416_TurboRateTable;
    sc->hwRateTable[WIRELESS_MODE_11NG]  = &ar5416_11ngRateTable;
    sc->hwRateTable[WIRELESS_MODE_11NA]  = &ar5416_11naRateTable;
}


void
ar5416SetQuarterRateTable(struct atheros_softc *sc)
{
    sc->hwRateTable[WIRELESS_MODE_11a] = &ar5416_11aRateTable_Quarter;
    return;
}

void
ar5416SetHalfRateTable(struct atheros_softc *sc)
{
    sc->hwRateTable[WIRELESS_MODE_11a] = &ar5416_11aRateTable_Half;
    return;
}

void
ar5416SetFullRateTable(struct atheros_softc *sc)
{
    sc->hwRateTable[WIRELESS_MODE_11a]   = &ar5416_11aRateTable;
    return;
}
                 

