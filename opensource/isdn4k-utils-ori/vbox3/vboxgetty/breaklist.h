
#ifndef _VBOX_BREAKLIST_H
#define _VBOX_BREAKLIST_H 1

/** Defines **************************************************************/

#define VBOXBREAK_MAX_ENTRIES	64

/** Variables ************************************************************/

extern unsigned char *breaklist[VBOXBREAK_MAX_ENTRIES];

/** Prototypes ***********************************************************/

extern void				 breaklist_init(void);
extern void				 breaklist_clear(void);
extern unsigned char *breaklist_add(unsigned char *);
extern int				 breaklist_del(unsigned char *);
extern void				 breaklist_dump(void);

#endif /* _VBOX_BREAKLIST_H */
