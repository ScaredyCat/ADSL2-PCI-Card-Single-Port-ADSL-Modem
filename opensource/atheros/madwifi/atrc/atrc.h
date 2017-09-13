/*
 * atrc.h
 */

#ifndef _ATRC_H
#define _ATRC_H

#define ATRC_MAX        (64 * 1024)
#define ATRC_NFILES     32
#define ATRC_MAXNLEN    256

typedef struct {
    uint32_t    file    : 8;
    uint32_t    line    : 24;
    uint32_t    tsh;
    uint32_t    tsl;
    uint32_t    value;
} atrc_entry_t;

typedef struct {
    int     inited;
    int     asserted;
    int     trcleft;
    int     stopped;
    int     head;
    int     tail;
    int     nfiles;
    char    files[ATRC_NFILES][ATRC_MAXNLEN];
    atrc_entry_t atrc[ATRC_MAX];
} atrc_t;

/*
 * public
 */
#define atrc_decl()     static int __atrc_file_no; \
                        static int __atrc_inited = 0
#define atrc_file()     __atrc_file_no = __atrc_file(__FILE__)
#define atrc_init()     if (!__atrc_inited++) atrc_file()
#define atrc_ent(_v)    __atrc((uint32_t)_v, __atrc_file_no, __LINE__)
#define atrcl_ent(_v)   __atrcl((uint32_t)_v, __atrc_file_no, __LINE__)

#define assert(_cond)   \
    if (!(_cond)) {     \
        __atrc_assert(# _cond, __atrc_file_no, __LINE__);   \
    }

/*
 * protected
 */
int  __atrc_file(char *file);
void __atrc(uint32_t value, int file, int line);
void __atrcl(uint32_t value, int file, int line);
void __atrc_assert(char *cond, int file, int line);

#endif /* _ATRC_H  */

