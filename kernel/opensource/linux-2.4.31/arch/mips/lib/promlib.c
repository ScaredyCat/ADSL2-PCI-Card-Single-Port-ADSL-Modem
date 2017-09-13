#include <stdarg.h>
#include <linux/kernel.h>

extern void prom_putchar(char);
#if 0
void prom_printf(char *fmt, ...)
{
	va_list args;
	char ppbuf[1024];
	char *bptr;

	va_start(args, fmt);
	vsprintf(ppbuf, fmt, args);

	bptr = ppbuf;

	while (*bptr != 0) {
		if (*bptr == '\n')
			prom_putchar('\r');

		prom_putchar(*bptr++);
	}
	va_end(args);
}
char buf[1024];

void prom_printf(const char * fmt, ...)
{
        va_list args;
        int l;
        char *p, *buf_end;

        /* Low level, brute force, not SMP safe... */
        va_start(args, fmt);
        l = vsprintf(buf, fmt, args); /* hopefully i < sizeof(buf) */
        va_end(args);
        buf_end = buf + l;

        for (p = buf; p < buf_end; p++) {
                /* Wait for FIFO to empty */
                while (((*AMAZON_ASC_FSTAT) >> 8) != 0x00) ;
                /* Crude cr/nl handling is better than none */
                if(*p == '\n') *AMAZON_ASC_TBUF=('\r');
                *AMAZON_ASC_TBUF=(*p);
        }

}
#endif
