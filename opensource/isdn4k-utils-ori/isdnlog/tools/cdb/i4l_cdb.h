/*
 GDB like interface for cdb
*/

#ifndef _I4L_CDB_H_
#define _I4L_CDB_H_

#include "cdb.h" 
#include "cdb_make.h"

typedef struct datum_t {
    unsigned char *dptr;
    size_t dsize;
} datum;

typedef struct _db_t {
    struct cdb *cdb;
    struct cdb_make *cdb_make;
    int writing;
} * _DB ;
  
_DB i4l_cdb_open(char *name, int wr);
void i4l_cdb_close(_DB d); 
int i4l_cdb_store(_DB d, datum key, datum value);
datum i4l_cdb_fetch(_DB d, datum key);
int i4l_cdb_exists(_DB d, datum key);

#endif
