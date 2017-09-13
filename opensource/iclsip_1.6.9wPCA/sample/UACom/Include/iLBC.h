#ifndef iLBC_H
#define iLBC_H

#include "iLBC_define.h"

#define ILBCNOOFWORDS_MAX   (NO_OF_BYTES_30MS/2)

#ifdef  __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------*
 *  Encoder interface function 
 *----------------------------------------------------------------*/

short /* (o) Number of bytes encoded */
iLBC_encode( iLBC_Enc_Inst_t *iLBCenc_inst,  /* (i/o) Encoder instance */ 
	short *encoded_data,		/* (o) The encoded bytes */
	short *data			/* (i) The signal block to encode*/
);

/*----------------------------------------------------------------*
*  Decoder interface function 
*---------------------------------------------------------------*/

short /* (o) Number of decoded samples */
iLBC_decode( iLBC_Dec_Inst_t *iLBCdec_inst,  /* (i/o) Decoder instance */
	short *decoded_data,		/* (o) Decoded signal block*/
	short *encoded_data,		/* (i) Encoded bytes */
	short mode                      /* (i) 0=PL, 1=Normal */
);

short /* (o) Number of bytes encoded */
iLBC_initEncoder(
       iLBC_Enc_Inst_t *iLBCenc_inst,  /* (i/o) Encoder instance */
       int mode			       /* (i) frame size mode */
);
   
short /* (o) Number of decoded samples */
iLBC_initDecoder(
       iLBC_Dec_Inst_t *iLBCdec_inst,  /* (i/o) Decoder instance */
       int mode,                       /* (i) frame size mode */
       int use_enhancer                /* (i) 1 to use enhancer 0 to run without enhancer */
);

#ifdef  __cplusplus
}
#endif

#endif iLBC_H