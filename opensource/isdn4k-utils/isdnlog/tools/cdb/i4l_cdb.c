/*
 * i4l_cdb ... constant database for isdntools
 *
 * s. also zone/common.h for the macros
 *
 * This is a gdbm replacement with only the functions used in isdnlog
 * database files are either written (by mkzonedb or dest/Makefile)
 * or read.
 * Read and write access can't be mixed, also writing must be done in
 * one step.
 *
 * Copyright 2000 by Leopold Toetsch <lt@toetsch.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * History
 *
 * 26.07.2000 V0.10 lt stable ;-)
 *
 */
 
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include "i4l_cdb.h"


_DB i4l_cdb_open(char *name, int wr) {
    _DB d = 0;
    int fd;
    fd = open(name, wr ? O_RDWR|O_CREAT|O_TRUNC : O_RDONLY, 0644);
    if (fd >= 0) {
	d = calloc(sizeof(struct _db_t), 1);
	if (d == NULL) {
	    close(fd);
	    return NULL;
	}    
	d->cdb = calloc(sizeof(struct cdb), 1);
	if (d->cdb == NULL) {
	    close(fd);
	    free(d);
	    return NULL;
	}
	if (wr) {
	    d->cdb_make = calloc(sizeof(struct cdb_make), 1);
	    d->writing = 1;
	    if (d->cdb_make == NULL) {
err:	    
		close(fd);
		free(d->cdb);
		free(d);
		return NULL;
	    }
	    if (cdb_make_start(d->cdb_make, fd) == -1) {
		free(d->cdb_make);
		goto err;
	    }
	}
	else 
	    cdb_init(d->cdb, fd);
    }
    return d;	
}

void i4l_cdb_close(_DB d) {
    if (!d)
	return;
    if (d->cdb_make) {
	cdb_make_finish(d->cdb_make);
	free(d->cdb_make);
    }	
    cdb_free(d->cdb);
    close(d->cdb->fd);
    free(d->cdb);
    free(d);
}

int i4l_cdb_store(_DB d, datum key, datum value) {
    if (!d)
	return -1;
    if(cdb_make_add(d->cdb_make, key.dptr, key.dsize, value.dptr, value.dsize) == -1)
	return -1;
    return 0;	
}

datum i4l_cdb_fetch(_DB d, datum key) {
    int r;
    uint32 pos, len;
    static unsigned char buf[1024];
    datum value;
    
    value.dptr = NULL;
    value.dsize = 0;
    if (!d)
	return value;
    
    if(d->writing)
	return value;
    cdb_findstart(d->cdb);
    r = cdb_findnext(d->cdb, key.dptr, key.dsize);
    if (r <= 0)
	return value;
    pos = cdb_datapos(d->cdb);
    len = cdb_datalen(d->cdb);

#ifdef WE_DONT_MODIFY_RESULT_OF_CDBREAD
    /* we return a pointer to mapped region if we can */
    /* nice idea - forget it */
    if (d->cdb->map) {
	if ((pos > d->cdb->size) || (d->cdb->size - pos < len))
              return value;
	value.dptr = d->cdb->map + pos;
	value.dsize = len;
    }
    else 
#endif
    {
    /* we return result in a static buffer */
        r = sizeof buf;
        if (len > r)
    	    return value;
	if (r > len) r = len;
	if (cdb_read(d->cdb,buf,r,pos) == -1)
	    return value;
	value.dptr = buf;
	value.dsize = len;
    }
    return value;    	
}

int i4l_cdb_exists(_DB d, datum key) {
    int r;
    
    if (!d)
	return 0;
    if(d->writing)
	return 0;
    cdb_findstart(d->cdb);
    r = cdb_findnext(d->cdb, key.dptr, key.dsize);
    return (r <= 0) ? 0 : 1;
}
