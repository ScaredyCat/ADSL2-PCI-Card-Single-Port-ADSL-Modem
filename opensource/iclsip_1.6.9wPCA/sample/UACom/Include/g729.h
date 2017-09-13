
#ifndef G729_H
#define G729_H

#include <stdlib.h>

#ifdef  __cplusplus
extern "C" {
#endif

void g729_init_encoder(void);
void g729_init_decoder(void);
void g729_encoder(INT16 * ,unsigned char *);
void g729_decoder(unsigned char *,INT16 *);

void g729a_init_encoder(void);
void g729a_init_decoder(void);
void g729a_encoder(INT16 * ,unsigned char *);
void g729a_decoder(unsigned char *,INT16 *);

#ifdef UACOM_USE_G729A

#define g729_init_encoder g729a_init_encoder
#define g729_init_decoder g729a_init_decoder
#define g729_encoder g729a_encoder
#define g729_decoder g729a_decoder

#endif

#ifdef  __cplusplus
}
#endif

#endif /* G729_H */