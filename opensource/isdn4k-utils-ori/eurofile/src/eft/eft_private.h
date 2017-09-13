/* $Id: eft_private.h,v 1.2 1999/10/05 21:23:21 he Exp $ */
#include <sys/param.h>  /* MAXPATHLEN */

/* file names beginning with this prefix are
 * used to store meta information used by the eftp4linux implementaion
 */
#define EFT_METAFILE_PREFIX ".++eft"

struct eft{
	struct tdu_stream * ts;
	struct tdu_fsm  * fsm;
	struct tdu_user  * usr;
	int tmp_fd;
	int xferlog;
	long flags;
	void * user_profile;
	int (*check_user)(struct eft *, char *, char *, char *);
	int (*setup_user)(struct eft *);
	int (*cleanup_user)(struct eft *);
	struct fileheader_par * fh;
	/* for preventing buffer overflows, we need MAXPATHLEN > 
	   EFT_MAX_FSTORE_LEN, which is true on linux*/
	unsigned char cwd[MAXPATHLEN+1];
	unsigned char home[MAXPATHLEN+1];
	unsigned char home_ref[EFT_MAX_FS_REF_LEN+1];
	unsigned char fn[MAXPATHLEN+1]; /* file name beeing trasferred */
	unsigned char * isdn_no /* of peer */;
	unsigned char * user_name /* login name of remote user */;
	char * address;
};

struct tdu_param;

extern void eft_print_time( struct tdu_fsm * );
extern int eft_msg2stdout(struct tdu_user *, struct tdu_param*);
extern int eft_store_dir(struct eft *, const char *, const char *, int);
extern int eft_store_slist(struct eft *, const char * dname, const char * prefix);
extern int eft_store_cwd(struct eft *);
extern int eft_valid_tkey(const char *);
extern int eft_valid_dosname(const char *);
extern int eft_valid_tname(const char *);
extern int eft_acceptable_tname(const char *);
extern int eft_valid_fstore_name(const unsigned char *);
extern int eft_make_tmp();
extern int eft_get_tmp(struct eft *);
extern int eft_get_string_fd(struct eft *, unsigned char *);
extern int eft_cd_close(struct tdu_stream *);
extern unsigned char * eft_stream_get_home(struct tdu_stream *);
extern void eft_stream_set_names(struct eft *, unsigned char *,
			  unsigned char *, int);
struct stat;
extern void eft_stream_set_stat(struct eft *, struct stat *);
extern void eft_stream_init_fd(struct eft *, int, int);
#if 0
 extern void  eft_stream_set_length(struct eft *, int);
#endif
struct dirent;
extern void eft_free_namelist(int ndirs, struct dirent **namelist);
extern unsigned char * eft_tn_by_fn(unsigned char * tn, unsigned char * fn, long flags);
extern unsigned char * eft_fn_by_tn(unsigned char * fn, unsigned char * tn, long flags);
extern int eft_file_is_tabu(unsigned char * fn);
extern int eft_get_peer_phone(unsigned char * isdn_no, int sk);
extern void eft_add_peer_phone(struct eft *, unsigned char * str);
extern unsigned char * eft_peer_phone(struct eft *);
extern int eft_file_is_hidden(const char *);
extern char * eft_make_tname(char *, const char *);
extern char * eft_make_tname(char *, const char *);
extern const char * eft_use_as_tname(const char *, int, int);
extern const char * eft_signature;
extern const char * eft_flat_dir_name;
extern int eft_is_alias(const char *);

#define DIRENT_CMP_TYPE (int (*)(const void*,const void *)) 
#define CONST_DIRENT

/* glibc1 has different definition of scandir */
#ifdef __GNU_LIBRARY__
#ifndef __GLIBC__

#undef CONST_DIRENT
#define CONST_DIRENT const 
#undef DIRENT_CMP_TYPE
#define DIRENT_CMP_TYPE

#endif
#endif

