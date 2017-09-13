#ifndef G723_H
#define G723_H

#include <stdlib.h>

typedef short int	Word16;
typedef long  int   Word32;

#ifdef  __cplusplus
extern "C" {
#endif

/* initial encoder */
void g723_initial_encoder(void);

/* initial decoder */
void g723_initial_decoder(void);

/* g723.1 encoder  */
/* rate: 0/1 for 5.3k/6.3k */
int g723_encoder(Word16 *databuf,char *bin,int rate);

/* g723.1 decoder */
void g723_decoder(Word16 *databuf,char *bin);

#ifdef  __cplusplus
}
#endif

#endif /* G723_H */