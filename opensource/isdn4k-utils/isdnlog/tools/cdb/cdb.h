#ifndef _CDB_H_
#define _CDB_H_

#include "freecdb.h"

struct cdb {
    int fd;
    uint32 dpos;
    uint32 dlen;
    } ;
    
void cdb_init(struct cdb *cdb, int fd);
void cdb_free(struct cdb *cdb);
int cdb_read(struct cdb *cdb, char *buf,unsigned int len, uint32 pos);
void cdb_findstart(struct cdb *cdb);
int cdb_findnext(struct cdb *cdb ,char *key, unsigned int len);

#endif
