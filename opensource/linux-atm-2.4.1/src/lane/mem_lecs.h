/*
 *
 * Memory handling funcs
 *
 * $Id: mem_lecs.h,v 1.1.2.1 2001/09/03 18:41:05 paulsch Exp $
 *
 */

#ifndef MEM_LECS_H
#define MEM_LECS_H

/* System includes */
#include <sys/types.h>

void *mem_alloc(const char *unit, size_t nbytes);
void mem_free(const char *unit, const void *mem);

void mem_dump(void);
#endif /* MEM_LECS_H */
