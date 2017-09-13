/* $Id: eft_dir.c,v 1.3 1999/10/06 18:16:22 he Exp $ */
/*
  Copyright 1998 by Henner Eisen

    This code is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/ 

/* directory operations needed for eft servers supporting navigation */


#include <malloc.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <eft.h>
#include "eft_private.h"
#include <tdu_user.h>
#include "fileheader.h"

/*
 * check wether a dent refers a regular file 
 */
int eft_valid_dent_r(CONST_DIRENT struct dirent * dent)
{
	struct stat s[1];
	int old_errno = errno;

	if( stat(dent->d_name,s) ){
		errno = old_errno;
		return 0;
	}
	return( S_ISREG(s->st_mode) );
}

/*
 * check whether a dent refers a regular file and can be used as a transfer 
 * name 
 */
int eft_valid_dent_r_t(CONST_DIRENT struct dirent * dent)
{
	struct stat s[1];
	int old_errno = errno;

	if( ! eft_valid_tname(dent->d_name) ) return 0;
	if( stat(dent->d_name,s) ){
		errno = old_errno;
		return 0;
	}
	return( S_ISREG(s->st_mode) );
}

/*
 * check whether a dent refers a regular file and can be used as a transfer 
 * name keyword
 */
int eft_valid_dent_r_k(CONST_DIRENT struct dirent * dent)
{
	struct stat s[1];
	int old_errno = errno;

	if( ! eft_valid_tkey(dent->d_name) ) return 0;
	if( stat(dent->d_name,s) ){
		errno = old_errno;
		return 0;
	}
	return( S_ISREG(s->st_mode) );
}

/*
 * check whether a dent refers a symlink to a regular file
 * and can be used as a transfer name keyword
 */
int eft_valid_dent_s_k(CONST_DIRENT struct dirent * dent)
{
	struct stat s[1];
	int old_errno = errno;

	if( ! eft_valid_tkey(dent->d_name) ) return 0;
	if( lstat(dent->d_name,s) ){
		errno = old_errno;
		return 0;
	}
	if( ! S_ISLNK(s->st_mode) ) return 0;
	if( stat(dent->d_name,s) ){
		errno = old_errno;
		return 0;
	}
	return( S_ISREG(s->st_mode) );
}

/*
 * Check whether a dent refers a regular file that has been assigned
 * a transfer name in the database.
 */
int eft_mapped_dent(CONST_DIRENT struct dirent * dent)
{
	int old_errno = errno, ret=0;

	if( strncmp(EFT_METAFILE_PREFIX,dent->d_name,6) == 0 ) return ret;
	if( (! eft_file_is_hidden(dent->d_name)) && 
	    eft_valid_dent_r(dent) && 
	    (! eft_is_alias(dent->d_name)) ) ret=1;
	errno = old_errno;
	return ret;
}

/*
 * check whether a dent refers a symlink and can be used as a transfer 
 * name keyword and a valid DO$ file name.
 */
int eft_valid_dent_s_kd(CONST_DIRENT struct dirent * dent)
{
	if( ! eft_valid_dosname(dent->d_name) ) return 0;
        return eft_valid_dent_s_k(dent);
}

/*
 * check whether a dent refers a regular file but cannot be used as 
 * a valid transfer name keyword
 */
int eft_invalid_dent_r_k(CONST_DIRENT struct dirent * dent)
{
	struct stat s[1];
	int old_errno = errno;
	int ret;

	if( eft_valid_tkey(dent->d_name) ) return 0;
	if( eft_file_is_hidden(dent->d_name) ) return 0;

	if( stat(dent->d_name,s) ){
		errno = old_errno;
		return 0;
	}
	ret=S_ISREG(s->st_mode);
	return ret;
}

/*
 * check wether a dent refers a directory and can be used as filestore name
 */
const char * g_dir=".";

int eft_valid_dent_d(CONST_DIRENT struct dirent * dent)
{
	struct stat s[1];
	char path[MAXPATHLEN+NAME_MAX+2];
	int old_errno = errno;

	/* FIXME: this stuff with global variables is not optimal */
	strcpy(path,g_dir);
	strcat(path,"/");
	strcat(path,dent->d_name);
	if( stat(path,s) ){
		errno = old_errno;
		return 0;
	}
	return( S_ISDIR(s->st_mode) && eft_valid_fstore_name(dent->d_name) );
}

static int eft_dir_is_hidden(char * dname, int strict_tree)
{
	
#ifdef EFT_DIR_DEBUG
	printf("eft_dir_is_hidden: checking %s\n",dname);
#endif
	if( 
		(strcmp( dname, eft_flat_dir_name) == 0) ||
		(strcmp( dname, ".") == 0)
		) return 1;
	/* Symlinks and some special dirents like ".." allow referring
	 * directories which are no real subdirectories of the directory
	 * contained in. This might results in directories with different
	 * filestore reference levels.
	 * It depends on interpretation of ETS 300 383 whether this is allowed.
	 * The choice whether to hide those directories is thus configurable.
	 */
	if( strict_tree ){
		struct stat s[1];
		int old_errno = errno;
		if(strcmp( dname, "..") == 0) return 1;
		if( lstat(dname,s) ){
			perror("dir_is_hidden:lstat()");
			errno = old_errno;
			return 1;
		}
		if( S_ISLNK(s->st_mode) ) return 1;
	}
	return 0;
}

/*
 * Compare transfer names, give nice names priority over ugly names.
 * The function assumes that it is only fed with valid transfer name key
 * words.
 *
 * The resulting ordering on existing symlinks is intended to select nice
 * transfer names for files whose name do not form a valid eft
 * transfer name keyword.
 */

static int trn_sort( const struct dirent * const *a,
		     const struct dirent * const *b)
{
	const char ugly[] = "?{}[];|#", *c;
	int ra, rb;
	
        /* Filenames containing ugly characters have probably been generated
	 * automatically by eft_mangle_name(). If somebody created a nicer
	 * name later, give that name priority.
	 */
	ra = rb = 0;
	for(c = (*a)->d_name; *c; c++){
		if( strchr(ugly,*c) ){
			ra=1;
			break;
		}
	}
	for(c = (*b)->d_name; *c; c++){
		if( strchr(ugly,*c) ){
			rb=1;
			break;
		}
	}
	if( rb && !ra ) return -1;
	if( ra && !rb ) return  1;

        /*
	 * Give file names forming a valid dos file name priority.
	 */
	ra = ! eft_valid_dosname((*a)->d_name);
	rb = ! eft_valid_dosname((*b)->d_name);
	if( rb && !ra ) return -1;
	if( ra && !rb ) return  1;
	    
        /*
	 * Give filenames not containg upper case characters priority over
	 * names containg upper cases. This is useful because certain clients
	 * don't treat cases appropriately.
	 */
	ra = rb = 0;
	for(c = (*a)->d_name; *c; c++){
		if( isupper(*c) ){
			ra=1;
			break;
		}
	}
	for(c = (*b)->d_name; *c; c++){
		if( isupper(*c) ){
			rb=1;
			break;
		}
	}
	if( rb && !ra ) return -1;
	if( ra && !rb ) return  1;
	    
	return alphasort(a,b);
}

/*
 * Update database containing the transfer names.
 *
 */
void eft_update_db(const char * dname)
{
	int ndirs, i;
	struct dirent **namelist;

	/* Scan current working directory for symbolic links that are
	 * re-usable as transfer names for files names that do not
	 * form a valid transfer name keyword.
	 *
	 * Some symlinks are given preference, this is taken care of
	 * by a ordering the symlink in a certain manner.
	 */
	/* FIXME: For performance reasons, check whether updating
	 * is really necessary, i.e. by
	 * checking last modification time of the directory
	 */
#ifdef EFT_DIR_DEBUG
        printf("scanning dir %s for ordered list of tkey symlinks\n",dname);
#endif
	ndirs = scandir(dname, &namelist, eft_valid_dent_s_k,
			DIRENT_CMP_TYPE trn_sort );
	/* printf("%d entries\n",ndirs); */
	if (ndirs < 0) { 
		perror("eft_update_db:scandir dent_s_k");
		return;
	} else {
		for(i=0; i<ndirs; i++) {
			/* use symlink as tname, but don't force usage
			 * for files that already have registered a tname
			 */
			eft_use_as_tname(namelist[i]->d_name,0,0);
		}
		eft_free_namelist(ndirs,namelist);
	}
#ifdef EFT_DIR_DEBUG	
        printf("scanning dir %s for non-tkey regular files\n",dname);
#endif
	ndirs = scandir(dname, &namelist, eft_invalid_dent_r_k,	alphasort);
	/* printf("%d entries\n",ndirs); */
	if (ndirs < 0) { 
		perror("eft_update_db:scandir dent_r_k");
		return;
	} else {
		/* this will create a tname by mangeling, if no 
		 * tname exists yet */
		for(i=0; i<ndirs; i++) {
			char tnn[13];
			eft_make_tname(tnn, namelist[i]->d_name);
		}
		eft_free_namelist(ndirs,namelist);
	}
}

/*
 * Stores the directory contents of dname in a file and returns a file
 * descriptor to it. The file is formatted as required by the EFT
 * DIRETORY primitive.
 */
int eft_store_dir(struct eft * eft, const char * dname, 
		  const char * pattern, int extended)
{
	int ndirs, i, fd = -2, fd2=-2, err;
	struct dirent **namelist;
	struct stat stat_buf;
	unsigned char *t_name, *f_name,	tn[TDU_PLEN_DESIGNATION+1];
	int writable;

	/* If we have read, write, and exec permissions, then
	 * we will be able to store and access meta-information
	 * by means of additinal symlinks. In that case, we can
	 * create a database in the directory which maps between
	 * transfer names and file names.
	 */

	writable = ! access(dname, R_OK|W_OK|X_OK);
	if( writable || ! access(EFT_METAFILE_PREFIX "_use_map",F_OK) ){
		eft->flags &= ~EFT_FLAG_DETERM_TN;
	} else {
		eft->flags |= EFT_FLAG_DETERM_TN;
	}

	if( pattern ){
		/* FIXME: use ETS 300 075 pattern to select files */
	}

	if( eft->flags & EFT_FLAG_DETERM_TN ){
		ndirs = scandir(dname, &namelist, eft_valid_dent_r, alphasort);
	} else {
		if( writable ) eft_update_db(dname);
		ndirs = scandir(dname, &namelist, eft_mapped_dent, alphasort);
	}
	if (ndirs < 0) { 
		perror("eft_store_dir:scandir");
		return -1;
	} else {
		FILE * str;
		fd = eft_get_tmp(eft);
		fd2=dup(fd);
		tdu_printf(TDU_LOG_DBG,"fd=%d\n",fd);
		str = fdopen(fd2,"w+");
		if( ! str ){
			perror("eft_store_dir:fdopen()");
			fd = -1;
			goto free_namelist;
		}
		
		for(i=0; i<ndirs; i++) {
			err = stat(namelist[i]->d_name,&stat_buf);
			if(err) {
				perror("stat()");
				continue;
			}
			if(eft_file_is_hidden(namelist[i]->d_name))
				continue;
			t_name = eft_tn_by_fn(tn,namelist[i]->d_name,eft_get_flags(eft));
			f_name = namelist[i]->d_name;
			if ( f_name && t_name ){
				if( extended ) {
					eft_fh_fwrite(str,t_name,
						      f_name,
						      &stat_buf);
				} else {
					fprintf(str, "%s %ld\r\n", 
						t_name, stat_buf.st_size);
				}
			}
		}
		fclose(str);
		if( lseek(fd,0,SEEK_SET) < 0 ){
			perror("eft_store_dir:lseek()");
			fd = -1;
			goto free_namelist;
		}
	}
free_namelist:
	eft_free_namelist(ndirs,namelist);
	return fd;
}

/*
 * This is somwhat ugly here. As scanndir only operates on dirents,
 * using stat() inside scandir callbacks only work for the current
 * working directory. This was an initial implementation mistake.
 *
 * The right fix is to use scandir only for selection upon dirent names
 * and to select further by means of stat() sensible ebvaluation function
 * later.
 * 
 * One hack is to use a global variable g_dir for passing
 * the directory to the scandir callbacks. The other one is to change 
 * prior to calling scandir.
 */

/*
 * Compute a filestore reference for the directory dir_path.
 * The filestore reference is stored as a null-terminated string
 * inside the array ref[] which must must be of size
 * EFT_MAX_FS_REF_LEN+1 or larger.
 *
 * Returns a pointer to the reference string on success or NULL on failure.
 */
static char * eft_get_fstore_ref(char *ref, const char * dir_path, int strict, const char * home_ref)
{
	char path[MAXPATHLEN], wd[MAXPATHLEN], *r, *p, *dir, *ret=NULL;
	struct dirent **namelist;
	int i, j, ndirs;

	if( ! realpath(dir_path,path) ) return NULL;
	if( *path != '/' ) return NULL;
	
	if( ! getcwd(wd,MAXPATHLEN) ){
		perror("eft_get_fstore_ref:getcwd");
		return NULL;
	}
	if( chdir(dir_path) ){
		perror("eft_get_fstore_ref:chdir");
		return NULL;
	}

	r=ref+EFT_MAX_FS_REF_LEN;
	*r=0;

	p=path;
	dir=path;
	while( *p != 0 ) p++;
	/*
	 * The core algorithm will fail if trailing '/' is present in path.
	 * As path is generated by realpath, this can only happen
	 * when path is "/".
	 */
	while( (r>(ref+1)) && (p>path) ){
		while( *p != '/' ) p--;
		*p=0;
		r -= 2;
		*r=0;
		if( p==path ){
			dir="/";
			/* special case, only happens when resolved path
			 * is "/" */
			if( *(p+1) == 0 ) {
				r[0] = 'Z';
				r[1] = 'Z';
				ret = r;
				/* this is not an error,
				 * however: */
				goto error;
			}
		}
		if( chdir(dir) ){
			perror("eft_get_fs_ref:chdir");
			goto error;
		};
#ifdef EFT_DIR_DEBUG
		fprintf("scanning dir \"%s\" for \"%s\"\n", dir,p+1);
#endif
		g_dir=".";
		ndirs = scandir(dir, &namelist, eft_valid_dent_d, alphasort);
		if (ndirs < 0){
			perror("eft_get_fstore_ref:scandir()");
			goto error;
		}
#ifdef EFT_DIR_DEBUG
		fprintf(stderr, "%d dirs found in \"%s\"\n", ndirs,dir);
#endif
		j=0;
		for(i=0; i<ndirs; i++){
			if(eft_dir_is_hidden(namelist[i]->d_name,strict)){
#ifdef EFT_DIR_DEBUG
			printf("d_name \"%s\" is hidden %d \n", namelist[i]->d_name, j);
#endif
				continue;
			}
#ifdef EFT_DIR_DEBUG
			printf("comparing d_name \"%s\" with \"%s\" %d \n", namelist[i]->d_name, p+1,j);
#endif
			if( (strcmp( namelist[i]->d_name, p+1) == 0) ){
#ifdef EFT_DIR_DEBUG
				printf("found entry \"%s\"\n", p+1);
#endif
				r[0] = 'A'+(j/26);
				r[1] = 'A'+(j%26);
				break;
			}
			j++;
		};
		eft_free_namelist(ndirs,namelist);
		if ( ! *r ){
#ifdef EFT_DIR_DEBUG
			printf("eft_get_fstore_name(): internal error"
				": \"%s\" not in \"%s\"\n", p+1,dir);
#endif
			goto error;
		}
#ifdef EFT_DIR_DEBUG
		printf("\"%s\" in \"%s\" has reference \"%s\"\n",p+1,dir,r);
#endif
	}

	if( (p == path) && (r >= ref) ){
		int len = strlen(home_ref);
		if(strncmp(home_ref,r,len) == 0){ 
			r += len;
		} else if ( r-2 >= ref ){
			r -= 2;
			r[0]= 'Z';
			r[1]= 'Z';
		}
		ret = r;
	}

error:
	if( chdir(wd) ){
		perror("failed to reset working direcrtory");
	}
	return ret;
}

/*
 * Store the contents of the directory dname in a file and return a file
 * descriptor to it. The file is formatted as required by the eft S-LIST
 * service. prefix is the navigation filestore reference prefix to be
 * prepended to all locally generated navigation filestore reference. 
 * if prefix is NULL, reference string corresponding to dname is used.
 */ 
int eft_store_slist(struct eft * eft, const char * dname
		    , const char * prefix)
{
	int ndirs, i, j, fd = -2, fd2=-2;
	int strict = eft->flags & EFT_FLAG_STRICT_TREE;
	struct dirent **namelist;
        char* path, ref_storage[EFT_MAX_FS_REF_LEN+1], *wd="";
	char resolved_path[MAXPATHLEN+1];

	if( ! prefix ){
		prefix = eft_get_fstore_ref(ref_storage, dname, strict, eft->home_ref);
		if( !prefix ) prefix="__DIR_ERROR_";
		if(dname){
			wd = realpath(dname, resolved_path);
			if(wd && strcmp(wd,"/")) strcat(wd,"/");
		}
	}
	g_dir=dname;
	ndirs = scandir(dname, &namelist, eft_valid_dent_d, alphasort);

	if (ndirs < 0) { 
		perror("eft_store_slist:scandir");
	} else {
		FILE * str;
		fd = eft_get_tmp(eft);
		fd2=dup(fd);
		tdu_printf(TDU_LOG_DBG,"fd=%d\n",fd);
		str = fdopen(fd2,"w+");
		if( ! str ){
			perror("eft_store_slist:fdopen()");
			fd = -1;
			goto free_namelist;
		}
		for(i=0,j=0; i<ndirs; i++){
			if(eft_dir_is_hidden(namelist[i]->d_name,strict)){
				continue;
			} else {
				path = namelist[i]->d_name;
			}
			if( (path==NULL) || (!eft_valid_fstore_name(path)) ){
				/* namelist contains valid fstore names only*/
				path = namelist[i]->d_name;
			}
#ifdef EFT_DIR_DEBUG
			printf( "%s%c%c %s\r\n", prefix,
				'A'+(j/26), 'A'+(j%26), path);
#endif
			fprintf(str, "%s%c%c %s%s\r\n", prefix,
				'A'+(j/26), 'A'+(j%26), wd, path);
			j++;
		}
		fclose(str);
		if( lseek(fd,0,SEEK_SET) < 0 ){
			perror("eft_store_slist:lseek()");
			fd = -1;
		}
	}
free_namelist:
	eft_free_namelist(ndirs,namelist);
	return fd;
}

/*
 * sets the 'home' directory (that's the one which becomes the
 * working directory after login or when navigation is reset) to
 * the current working directory.
 */
void eft_set_home(struct eft * eft)
{
	char buf[EFT_MAX_FSTORE_LEN+1], *dir, *prefix,
		pref_buf[EFT_MAX_FS_REF_LEN+1];

	dir = getcwd(buf,EFT_MAX_FSTORE_LEN);
	if( dir ){
		buf[EFT_MAX_FSTORE_LEN]=0;
		strcpy(eft->home,dir);
	} else {
		strcpy(eft->home,"/");
	}
	prefix = eft_get_fstore_ref(pref_buf, ".",
				    eft->flags & EFT_FLAG_STRICT_TREE,"");
	if( prefix ){
		strcpy(eft->home_ref,prefix);
	} else {
		eft->home_ref[0]=0;
	}
	/* tdu_printf(TDU_LOG_TMP,"prefix is '%s' '%s', home=%s\n", prefix, eft->home_ref,eft->home); */
}

/*
 * stores the contents of the current working directory in a file
 * and returns an open file descriptor to that file.
 */
int eft_store_cwd(struct eft * eft)
{
	char buf[EFT_MAX_FSTORE_LEN+1], *dir;
	int fd;

	dir = getcwd(buf,EFT_MAX_FSTORE_LEN);
	if( dir ){
		buf[EFT_MAX_FSTORE_LEN] = 0;
		fd = eft_get_string_fd(eft,dir);
	}
	return fd;
}


unsigned char * eft_stream_get_home( struct tdu_stream * ts )
{
	struct eft * eft = tdu_stream_get_user(ts)->priv;

	return eft->home; 
}

/*
 * This is to be used as the tdu_stream's close method when
 * a EUROSFT92/NAVIGATION/SELECT file is received. This takes care of reading
 * the contents of that transmitted file and changing the corresponding working
 * directory
 */
int eft_cd_close( struct tdu_stream * ts )
{
	/* FIXME: better error code ?*/
	int len, err = TDU_RE_UNKNOWN_FILE;
	unsigned char * c;
	char buf_st[EFT_MAX_FSTORE_LEN+1], buf_st2[EFT_MAX_FSTORE_LEN+1],
		*buf = buf_st, wd[MAXPATHLEN];
	struct eft * eft = tdu_stream_get_user(ts)->priv;

	if( lseek(ts->fd,0,SEEK_SET) < 0 ){
		perror("eft_cd_close:lseek()");
		return err;
	} else {
		len = read(ts->fd,buf,EFT_MAX_FSTORE_LEN);
		if( len < 0 ) { 
			perror("eft_cd_close:read()");
			goto close;
		}
		buf[len]=0;
		tdu_printf(TDU_LOG_AP2,"CHD: SELECT filestore=\"%s\"\n", buf);
		/* FIXME: better error handling / more different error codes*/
		if( strcmp(buf,"EUROSFT92/NAVIGATION/RESET") == 0 ) { 
			buf = eft_stream_get_home(ts);
		} else if( eft_get_flags(eft) & EFT_FLAG_CASEFIX_FS ){
			if( eft_get_flags(eft) & EFT_FLAG_SLASHFIX) strcpy(buf_st2, buf_st);
			eft_fix_cases(buf);
		}
		if( chdir(buf) ) {
			tdu_printf(TDU_LOG_ERR,"eft_cd_close: couldn't cd to \"%s\":%s\n",
				buf, strerror(errno) );
			if( errno == ENOENT && (eft_get_flags(eft)&EFT_FLAG_SLASHFIX)){
				/* Some broken clients (AVM) mess up '/'
				 * with '\'. Try again with '\' substituted
				 * by '/'.
				 */
				tdu_printf(TDU_LOG_LOG, "trying again with backslashes substituted: ");
				if( eft_get_flags(eft)&EFT_FLAG_SLASHFIX) buf = buf_st2;
				for(c=buf; *c != 0; c++){
					if( *c == '\\') *c = '/';
				};
				if( eft_get_flags(eft)&EFT_FLAG_CASEFIX_FS ) eft_fix_cases(buf);
				if( chdir(buf) ) {
					tdu_printf(TDU_LOG_ERR,"eft_cd_close: even couldn't cd to \"%s\":%s\n",
						   buf, strerror(errno) );
					goto close;
				} else {
					err = 0;
				}
			} else {
				goto close;
			}
		} else {
			err = 0;
		}
	}
close:
	c = getcwd(wd, MAXPATHLEN);
	if(c) {
		tdu_printf(TDU_LOG_AP2,"CHD: END    cwd=\"%s\"\n", c);
	} else {
		tdu_printf(TDU_LOG_ERR,"eft_cd_close: getcwd(): %s\n", strerror(errno));
	}
	tdu_fd_close(ts);
	return err;
}
