/* $Id: eft.h,v 1.3 1999/10/05 21:23:22 he Exp $ */
/*
 * Interface for accessing eurofile service primitives.
 * Eurofile for Linux is implemented as a library. Programmers
 * are encouraged to use that library for writing more user friendldy
 * Eurofile client and server front ends.
 */

struct eft;


extern int eft_connect (struct eft *, unsigned char *);
extern int eft_disconnect (struct eft *);
#ifdef FD_SET
struct timeval;
extern int eft_select( struct eft *, int, fd_set *,
		       fd_set *, fd_set *, struct timeval *);
#endif
extern struct eft * eft_make_instance();
extern int eft_attach_socket(struct eft *, int);
extern int eft_get_socket(struct eft *);
extern int eft_is_up(struct eft *);

extern int eft_load_fd(struct eft *, int, unsigned char *, int);
extern int eft_load(struct eft *, unsigned char *, unsigned char *);

extern int eft_dir_fd (struct eft *, int, unsigned char *, int );
extern int eft_xdir_txt (struct eft *, unsigned char * );
extern int eft_cd (struct eft *, unsigned char *);
extern int eft_getcwd (struct eft *, unsigned char *);
extern int eft_mkdir (struct eft *, unsigned char *);
extern int eft_list_fd(struct eft *, int, int);
extern int eft_slist_fd(struct eft *, int, int);


extern int eft_save_fd(struct eft *, unsigned char *, int, int);
extern int eft_save(struct eft *, unsigned char *, unsigned char *);

extern int eft_delete (struct eft *, unsigned char *);
extern int eft_rename(struct eft *, unsigned char *, unsigned char *);

extern void eft_prompt(char *);

extern char * eft_re_descr(int);

#define EFT_RE_ID_REJECTED		0x21
#define EFT_RE_DISK_FULL		0x22
#define EFT_RE_FILE_ACCESS_IMPOSSIBLE	0x23
#define EFT_RE_RESERVED			0x24
#define EFT_RE_USER_INTERRUPT		0x25
#define EFT_RE_USER_ABORT		0x26
#define EFT_RE_NO_EXTENDED_FMT		0x27
#define EFT_RE_NO_LOG_ACCESS		0x28
#define EFT_RE_CMPR_FMT_NOT_SUPPORTED	0x29
#define EFT_RE_WRONG_FCS		0x2a
#define EFT_RE_CMPR_CODING_ERROR	0x2b

#define EFT_MAX_FSTORE_LEN		221
#define EFT_MAX_FS_REF_LEN		32
#define EFT_MAX_FS_LEVELS		16

extern int eft_server_mainloop(struct eft*);
extern int eft_accept_user(struct eft*);

extern int eft_release(struct eft*);

extern int eft_msg(struct eft *, unsigned char *);
extern struct tdu_user * eft_get_user(struct eft *);
extern int eft_remote_has_navigation(struct eft *);
extern void eft_set_home(struct eft * eft);
/* extern void eft_set_case_fix(struct eft *, int);
extern int eft_need_case_fix(struct eft *);*/
extern void eft_fix_cases(unsigned char *);
/* extern void eft_set_slash_fix(struct eft *, int);
extern int eft_need_slash_fix(struct eft *); */
struct sockaddr_x25;
struct x25_route_struct;
extern int eft_get_x25route(struct sockaddr_x25 *, struct x25_route_struct *, char *isdn_no);
extern int eft_release_route(struct x25_route_struct *);
extern int eft_signal_release_route();
extern int eft_wait_release_route();
extern void eft_dl_disconnect(unsigned char *);
extern int eft_release_device(unsigned char *);

extern long eft_get_flags(struct eft *);
extern void eft_set_flags(struct eft *, long);
extern void eft_set_xferlog(struct eft *, int);
extern int eft_printable_called_addr(struct eft *, unsigned char *);
extern int eft_printable_assoc_udata(struct eft *, unsigned char *);

extern void eft_set_auth(struct eft *, 
			 int (*check_user)(struct eft *, char *,char *,char *),
			 int (*setup_user)(struct eft *),
			 int (*cleanup_user)(struct eft *)
);
extern int eft_set_address(struct eft *, char * address);


#define EFT_FLAG_ANONYMOUS  (1 << 0)
#define EFT_FLAG_GUEST      (1 << 1)
#define EFT_FLAG_DOS_TN     (1 << 2)   /* apply DOS transfer names contraints */
#define EFT_FLAG_CASEFIX_TN (1 << 3)   /* case insensitive transfer names */
#define EFT_FLAG_CASEFIX_FS (1 << 4)   /* case insensitive fstore names */
#define EFT_FLAG_SLASHFIX   (1 << 5)   /* fix clients replacing '\' by '/' */
#define EFT_FLAG_DETERM_TN  (1 << 6)   /* generate tr names deterministically*/
#define EFT_FLAG_STRICT_TREE (1 << 7)  /* don't allow directories aliases */

extern const char * eft_flat_dir_name;
extern char * eft_get_device(char * dev, int len, int sock_fd);
extern char * eft_map_to_user;
extern const char * eft_str_reason(int);
extern const char * eft_str_other_reason(int);

/* FIXME: where can we obtain the system limit for this */ 
#define EFT_DEV_NAME_LEN 15	
