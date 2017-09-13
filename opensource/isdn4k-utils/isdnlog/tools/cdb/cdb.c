/*
 * cdb ... constant database for isdntools
 *         highlevel interface similar to DJB's cdb 0.75
 *         but we use the free cdb-0.61 from debian
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
 * 09.08.2000 V0.10 lt stable ;-)
 *
 */
 
#include "cdb.h" 
#include "cdb_make.h" 
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>

/* init / exit - mostly dummy */
void cdb_init(struct cdb *cdb, int fd) {
    cdb->fd = fd;
}

void cdb_free(struct cdb *cdb) {
}

/* reading stuff */
void cdb_findstart(struct cdb *cdb) {
}

int cdb_read(struct cdb *cdb, char *buf,unsigned int len, uint32 pos) {
    /* actually we don't need pos, because the previous findnext
       postioned us correctly 
       if you want to have a real read - you know what to do */
    int ret = cdb_bread(cdb->fd, buf, len);
    return ret;
}

int cdb_findnext(struct cdb *cdb ,char *key, unsigned int len) {
    uint32 dlen;
    int ret = cdb_seek(cdb->fd, key, len, &dlen);
    if (ret == 1) {	/* found */
	cdb->dlen = dlen;
	cdb->dpos = lseek(cdb->fd, 0, SEEK_CUR);
    }
    return ret;
}

/* makeing stuff */
static char packbuf[8];

int cdb_make_start(struct cdb_make *c,int fd) {
    int i;
    char b = ' ';    
    
    cdbmake_init(&c->cdbm);
    c->fd = fd;
    for (i = 0;i < sizeof(c->cdbm.final);++i)
	if (write(fd, &b, 1) != 1)
	    return -1;
    c->pos = sizeof(c->cdbm.final);
    return 0;
}

static inline uint32 safeadd(u,v) uint32 u; uint32 v;
{
  u += v;
//  if (u < v) overflow(); we don't ;-)
  return u;
}
/* note:
   the allocated memory is never freed.
   but, this is called from shortrunning standalone programs
   so no problem.
*/   
int cdb_make_add(struct cdb_make *c,char *key,unsigned int keylen,char *data,unsigned int datalen) {
    uint32 h, pos;
    int i , ch;
    
    pos = c->pos;
    cdbmake_pack(packbuf,(uint32) keylen);
    cdbmake_pack(packbuf + 4,(uint32) datalen);
    if (write(c->fd,packbuf,8) != 8) 
	return -1;
    h = CDBMAKE_HASHSTART;
    for (i = 0;i < keylen;++i) {
      ch = key[i];
      h = cdbmake_hashadd(h,ch);
      if (write(c->fd, &key[i], 1) != 1)
          return -1;
    }
    for (i = 0;i < datalen;++i) {
      ch = data[i];
      if (write(c->fd, &data[i], 1) != 1)
          return -1;
    }
    if (!cdbmake_add(&c->cdbm,h,pos,malloc))
	return -1;
    pos = safeadd(pos,(uint32) 8);
    pos = safeadd(pos,(uint32) keylen);
    pos = safeadd(pos,(uint32) datalen);
    c->pos = pos;
    return 0;
}

int cdb_make_finish(struct cdb_make *c) {
    int i, u;
    uint32 len, pos;
    
    pos = c->pos;
    if (!cdbmake_split(&c->cdbm,malloc)) 
	return -1;

    for (i = 0;i < 256;++i) {
	len = cdbmake_throw(&c->cdbm,pos,i);
	for (u = 0;u < len;++u) {
	    cdbmake_pack(packbuf,c->cdbm.hash[u].h);
	    cdbmake_pack(packbuf + 4,c->cdbm.hash[u].p);
	    if (write(c->fd,packbuf,8) != 8) 
		return -1;
	    pos = safeadd(pos,(uint32) 8);
	}
    }
    lseek(c->fd, 0, SEEK_SET);	
    if (write(c->fd, c->cdbm.final,sizeof(c->cdbm.final)) < sizeof(c->cdbm.final))
	return -1 ;
    return 0;
}
