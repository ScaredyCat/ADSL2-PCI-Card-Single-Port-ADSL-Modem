/*
 * Values for Layer-2-protocol-selection
 */
#define ISDN_PROTO_L2_X75I   0   /* X75/LAPB with I-Frames            */
#define ISDN_PROTO_L2_X75UI  1   /* X75/LAPB with UI-Frames           */
#define ISDN_PROTO_L2_X75BUI 2   /* X75/LAPB with UI-Frames           */
#define ISDN_PROTO_L2_HDLC   3   /* HDLC                              */
#define ISDN_PROTO_L2_TRANS  4   /* Transparent (Voice)               */
#define ISDN_PROTO_L2_X25DTE 5   /* X25/LAPB DTE mode                 */
#define ISDN_PROTO_L2_X25DCE 6   /* X25/LAPB DCE mode                 */
#define ISDN_PROTO_L2_V11096 7   /* V.110 bitrate adaption 9600 Baud  */
#define ISDN_PROTO_L2_V11019 8   /* V.110 bitrate adaption 19200 Baud */
#define ISDN_PROTO_L2_V11038 9   /* V.110 bitrate adaption 38400 Baud */
#define ISDN_PROTO_L2_MODEM  10  /* Analog Modem on Board */
#define ISDN_PROTO_L2_FAX    11  /* Fax Group 2/3         */
#define ISDN_PROTO_L2_HDLC_56K 12   /* HDLC 56k                          */
#define ISDN_PROTO_L2_MAX    15  /* Max. 16 Protocols                 */

/*
 * Values for Layer-3-protocol-selection
 */
#define ISDN_PROTO_L3_TRANS	0	/* Transparent */
#define ISDN_PROTO_L3_TRANSDSP	1	/* Transparent with DSP */
#define ISDN_PROTO_L3_FCLASS2	2	/* Fax Group 2/3 CLASS 2 */
#define ISDN_PROTO_L3_FCLASS1	3	/* Fax Group 2/3 CLASS 1 */
#define ISDN_PROTO_L3_MAX	7	/* Max. 8 Protocols */

