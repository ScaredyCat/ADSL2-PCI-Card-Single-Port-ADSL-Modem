#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "atrc.h"

void
atrc_print(atrc_t *atrc)
{
    int             i, head, tail;
    uint32_t        tsh, tsl;
    atrc_entry_t    *etrc;


    printf("atrc info\n");
    printf("inited %d\n", atrc->inited);
    printf("stopped %d\n", atrc->stopped);
    printf("nfiles %d\n", atrc->nfiles);
    printf("head %d tail %d\n", atrc->head, atrc->tail);

    for (i = 0; i < atrc->nfiles; i++) {
        printf("file[%d] : %s\n", i, atrc->files[i]);
    }

    head = atrc->head;
    tail = atrc->tail;
    etrc = &atrc->atrc[head];
    tsh = etrc->tsh;
    tsl = etrc->tsl;

    printf("Trace start at %x%08x\n", etrc->tsh, etrc->tsl);
    printf("%32s:%5s:%10s/%8s: %03s:%03s:%03s:%03s\n",
            "File", "Line", "Value(hex)", "dec", "sec", "ms", "us", "ns");
    
    while (head != tail) {
        uint32_t    d, d_s, d_ms, d_us, d_ns;
        etrc = &atrc->atrc[head];
        d = etrc->tsl - tsl;
        d *= 2; d /= 3;
        d_ns = d % 1000;
        d_us = d % 1000000; d_us /= 1000;
        d_ms = d % 1000000000; d_ms /= 1000000;
        d_s  = d /= 1000000000;
        d_s += (etrc->tsh - tsh);
        printf("%s:%5d:%03d:%03d:%03d:%03d -- %u/%x\n",
                basename(atrc->files[etrc->file]),
                etrc->line,
                d_s, d_ms, d_us, d_ns,
                etrc->value,
                etrc->value);
        tsh = etrc->tsh;
        tsl = etrc->tsl;
        head ++;
        head %= (ATRC_MAX - 1);
    }
}

main()
{
    int     fd;
    atrc_t  atrc;
    ssize_t sz;

    fd = open("/dev/atrc0", O_RDONLY);
    if (fd < 0) {
        perror("open(2)");
        exit(1);
    }
    sz = read(fd, &atrc, sizeof(atrc_t));
    if (sz != sizeof(atrc_t)) {
        perror("read(2)");
        exit(1);
    }
    atrc_print(&atrc);
    close(fd);
}

