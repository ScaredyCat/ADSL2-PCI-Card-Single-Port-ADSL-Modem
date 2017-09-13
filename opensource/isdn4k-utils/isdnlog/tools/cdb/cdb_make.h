/* Public domain. */

#ifndef CDB_MAKE_H
#define CDB_MAKE_H
#include "freecdbmake.h"
/* struct for makeing */
 struct  cdb_make{
    int fd;
    struct cdbmake cdbm;
    uint32 pos;
    };
    
#define cdb_datapos(c) ((c)->dpos)
#define cdb_datalen(c) ((c)->dlen)

extern int cdb_make_start(struct cdb_make *,int);
extern int cdb_make_add(struct cdb_make *,char *,unsigned int,char *,unsigned int);
extern int cdb_make_finish(struct cdb_make *);

#endif
