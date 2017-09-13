/* $Id: eft_names.c,v 1.3 1999/10/06 18:16:22 he Exp $ */
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

/* 
 * operations (check, map, ..) for transfer names, file names and fstore names
 */
/*
 * If this macro is #define'd a symlink corresponding to the transfer
 * name will be generated in the current directory. 
 */
#define EFT_ADD_LINK
#undef EFT_ADD_LINK

#include <malloc.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <fnmatch.h>
#include <errno.h>
#include <eft.h>

#include <tdu_user.h>

#include "eft_private.h"
/* #include "../config.h" */

static ino_t g_inode;
static unsigned char * g_fname;

static const char 
	hide_prefix[]   = EFT_METAFILE_PREFIX "_hide.",
	alias_prefix[]  = EFT_METAFILE_PREFIX "_alias.",
	tn_prefix[]     = EFT_METAFILE_PREFIX "_tn.",
	tn_dos_prefix[] = EFT_METAFILE_PREFIX "_td.",
	fn_prefix[]     = EFT_METAFILE_PREFIX "_fn.", 
	fn_dos_prefix[] = EFT_METAFILE_PREFIX "_fd.";

/*
 * free the namlist storage dynamically allocated by the scandir() function
 */
void eft_free_namelist(int ndirs, struct dirent **namelist)
{
	if( ndirs < 0 ) return;
	while( ndirs-- ) free(namelist[ndirs]);
	free(namelist);
}

/*
 * check whether a dirent case-insensitivly matches g_fname
 */
static int dent_casecmp(CONST_DIRENT struct dirent * dent)
{
	return( strcasecmp(dent->d_name,g_fname) == 0 );
}

void eft_fix_cases(unsigned char * fn)
{
	unsigned char * p, *dir, *bn, dot[2]=".";
	struct stat s[1];
	struct dirent **namelist;
	int ndirs;
	
	if( lstat(fn,s) == 0 ) return; /* matches without modification */
	/* if fn contains a '/' then it becomes very tricky. For now,
	 * we just convert the directory_part to lower case. As
	 * EUROFILE does not support creating directories, this 
	 * restriction is not as crucial as with file names
	 */
	p=fn;
	bn=fn;
	dir = dot;
	while( *p ){
		if( *(p++) == '/' ) bn = p; 
	}
	if( bn > fn ) dir = fn;

	g_fname = malloc( strlen(bn)+1 );
	if(! g_fname) return;
	strcpy(g_fname,bn);
	bn[0] = 0;
	p=dir;
#ifdef EFT_NAMES_DEBUG
	printf( "eft_fix_cases: dir=\"%s\", base=\"%s\"\n", dir, g_fname);
#endif
	while( *p ){
		*p = tolower((int) *(p++));
	};

	/* the basename is determined by finding a case-insensitive match
	 * in the directory.
	 *
	 * We really need to restrict matches to partical file classes,
	 * depending on wether this is used for directories, regular files,
	 * or database-symlink entries. However, this would become even more
	 * complex. And the proper action is fixing the buggy clients anyway.
	 */
	ndirs = scandir(dir, &namelist, dent_casecmp, alphasort);
	if( ndirs >=1 ){
#ifdef EFT_NAMES_DEBUG
		printf( "fixing: dir=\"%s\", base=\"%s\"\n",
			dir, namelist[0]->d_name);
#endif
		strcpy(bn,namelist[0]->d_name);
		eft_free_namelist(ndirs,namelist);
	} else {
#ifdef EFT_NAMES_DEBUG
		printf( "no case fix found\n");
#endif
		*bn = *g_fname;
	}
	free(g_fname);
}

/*
 * verify that key is a valid eft transfer name keyword
 */
int eft_valid_tkey(const char *key)
{
	int i, len=strlen(key);

	if ( len > 12 ) return 0;
	for( i=0; i<len; i++, key++ ){
		if( *key<0x21 || *key>0x7e || *key==0x28 || *key==0x29
		    || *key==0x2a || *key==0x2b || *key==0x2f ) return 0;
	}	
	return 1;
}

/*
 * verify that key is a valid dos file name as well as
 * a valid eft transfer name keyword.
 */
const char dos_invalid[]= "+/()*"     "\"\\:?<>|=,;";


int eft_valid_dosname(const char *key)
{
	int i, len=strlen(key), dot=0;

	if ( len > 12 ) return 0;
	for( i=0; i<len; i++ ){
		if( (key[i] < 0x21) || (key[i] > 0x7e) ||
		    strchr(dos_invalid,key[i]) ) return 0;
	};

	if( *key == '.' ) return 0;

	for( i=0; i<len; i++, key++ ){
		if( key[i] == '.' ){
			if( dot ){
				return 0; /* too many dots */
			} 
			dot = 1;
			if( i < (len-4) ) return 0; /* too left */
		}
	}		       
	return 1;
}

int eft_valid_tname(const char * name)
{
	const char * slash[9];
	char key[13];
	int is=1, i, len;
	
	slash[0] = name-1;
	i=0;
	while( name[i] ){
		if( name[i] == '/' ){
			slash[is] = name+i;
			is++;
		}
		if( (i>=70) || (is>8)  ) return 0;
		i++;
	}
	slash[is] = name+i;
	for( i=0; i<is; i++ ){
		len = (slash[i+1] - slash[i]) - 1;
		if( len > 12 ) return 0;
		strncpy(key,slash[i]+1,len);
		key[len]=0;
		if( ! eft_valid_tkey(key) ) return 0;
	}
	return 1;
}

int eft_acceptable_tname(const char * name)
{
	if( 0 ) {
		return eft_valid_tname(name);
	} else {
		return fnmatch("*" EFT_METAFILE_PREFIX "*", name, 0) 
			== FNM_NOMATCH;
	} 
}

/*
 * exclude certain files from downloading
 */
int eft_file_is_tabu( unsigned char * name ){
	/* 
	 * for security reasons, refuse to transfer core files as they might
	 * unintentionally contain sensitive information. the same holds for
	 * passwd
	 */
	char *tabu_list[]={ "*core", "*passwd", NULL }, **f=tabu_list;

	while( *f != NULL ){
		if( fnmatch(*f,name,0) != FNM_NOMATCH ) return 1;
		f++;
	}
	return 0;
}

/*
 * verify that fstore is a valid eft filestore name.
 */
int eft_valid_fstore_name(const unsigned char *fstore)
{
	int len=strlen(fstore);

	if ( len > EFT_MAX_FSTORE_LEN ) return 0;

	while( *fstore != 0 ){
		if( *fstore < 0x20 || *fstore > 0x7f ||
		    *fstore == '\n' || *fstore == '\r' ) return 0;
		fstore++;
	};
	return 1;
}

/*
 * check whether the inode number reffered by dent is equal to g_inode
 * (g_inode is a global variable)
 */
static int dent_has_g_inode(CONST_DIRENT struct dirent * dent)
{
	struct stat s[1];
	int old_errno = errno;

	if( lstat(dent->d_name,s) ){
		errno = old_errno;
		return 0;
	}
	return( s->st_ino == g_inode );
}

char * eft_lookup_fname(char *, const char *, int case_fix, int dos);

/*
 * Map an EUROFile transfer name to a local filename according to
 * our local conventions.
 * See comments in eft_tn_by_fn for supported mapping techniques.
 */	
unsigned char * eft_fn_by_tn(unsigned char * fn, unsigned char * tn,
			     long flags)
{
  /* 
   * Algorithmic method: transfer names longer than 12 characters are
   * modified by inserting '/' characters every 12th charater. Remove those
   * extra '/'-characters to obtain the original file name.
   */

	int ndirs, case_fix = flags & EFT_FLAG_CASEFIX_TN;
	int dos = flags & EFT_FLAG_DOS_TN;
	struct dirent **namelist;
	unsigned char * p=fn;
	
	if( flags & EFT_FLAG_DETERM_TN ){
		if( strncasecmp(tn,"/LONG/",6) == 0 ) {
			tn += 6;
			while(*tn){
				if( *tn != '/' ) *(p++) = *tn;
				tn++;
			};
			*p=0;
		} else if( strncasecmp(tn,"//INODE/",8) == 0 ) {
			tn += 8;
			g_inode = atoi(tn);
			*fn=0;
			ndirs = scandir(".", &namelist, dent_has_g_inode, alphasort);
			if( ndirs >=1 )	strcpy(fn,namelist[0]->d_name);
			eft_free_namelist(ndirs,namelist);
			case_fix = 0;
		} else {
			strcpy(fn,tn);
		}
		/* 
		 * fix to handle broken clients with case-insensitive
		 * or inconsistent transfer name encoding.
		 */
		if( case_fix ){
			eft_fix_cases(fn);
		}
		return fn;
	} else {
		char * ret;
		
		/* 
		 * look up file name in database when transfer name is valid,
		 * fall back to literally interpreting transfer name as
		 * filename if not successful
		 */
		if( eft_valid_tkey(tn) ){
			ret=eft_lookup_fname(fn, tn, case_fix, dos);
			if( ret ){
				return ret;
			} else {
				ret=strncpy(fn,tn,13);
			} 
		} else {
			ret=strncpy(fn,tn,MAXPATHLEN);
			fn[MAXPATHLEN-1]=0;
		}
		if( case_fix ){
			eft_fix_cases(fn);
#ifdef EFT_NAMES_DEBUG
			printf("fixed cases, file name now %s\n",tn);
#endif
		}
		return fn;
	}
}

char * eft_lookup_tname(char *, const char *);

/*
 * Map a directory entry fn to an EUROFile transfer name according to
 * our local conventions.
 */	
unsigned char * eft_tn_by_fn(unsigned char * tn, unsigned char * fn, long flags)
{
  /* 
   * Old algoritmic method: file names longer than 12 characters are
   * modified by inserting '/' characters every 12th charater. If this
   * does not generate a valid transfer name, a transfer name based on
   * the file's inode number is generated. From such transfer
   * names, a human eye can easily derive the corresponding file name
   * (which is useful, if the clients does not support extended directory
   * requests).
   *
   * Although this always generates transfer names fulfilling ETS 300 383
   * and ETS 300 075, lots of interworking problems have been observed
   * because clients don't handle transfer names with '/' characters
   * appropriately.
   */
	unsigned char * bn=fn;
	struct stat s[1];
	long inode;
	int len=0, slen=0, coding=0;
	int old_errno = errno;
	char * ret;

	if( flags & EFT_FLAG_DETERM_TN ){
		if( lstat(fn,s) ){
			errno = old_errno;
			return 0;
		}
		inode = s->st_ino;
		
		/* only the basename is used for deriving a transfer name */
		while( *fn ){
			if( *(fn++) == '/' ){
				bn = fn;
			}
		};
		
		fn = bn;
		while(*fn){
			len++;
			if( *fn<0x21 || *fn>0x7e || *fn==0x28 || *fn==0x29
			    || *fn==0x2a || *fn==0x2b || *fn==0x2f ) {
				coding = 1;
				break;
			}
			fn++;
		}
		if( coding || len > 52) {
			/* no chance of simple name mapping because characters
			 * out of range or name to long */
			sprintf(tn, "//INODE/%ld", inode);
		} else if ( len <=12 ) {
			strcpy(tn,bn);
		} else {
			/* the name contains only characters valid for transfer name
			   key words, but exceeds the keyword size of 12 bytes.
			   We just insert '/' characters after every 12 bytes in order
			   to formally fulfill the silly ETS 300 075 (chap 7.4.4)
			   restriction */
			strcpy(tn, "/LONG/");
			fn = tn+6;
			while( *bn ){
				if( slen == 12 ){
					*(fn++) = '/';
					slen = 0;
				}
				*(fn++) = *(bn++);
				slen++;
			}
			*fn = 0;
		}
		return tn;
	} else {
/* 
 * New mangeling method: file names which form a valid transfer name keyword
 * are used unmodified as transfer names. However, certain magic files are
 * marked as hidden in a database, no transfer name is generated for them.
 * For other file names, a transfer name is looked up in a database.
 * If no transfer name is found, a transfer name keyword is created using
 * a mangeling algorithm and inserted in the database. If the file system
 * containing the current directory supports symbolic links and is writable
 * by the user, an additional symlink corresponding to the transfer name /
 * file name pair is created.
 */
		if( eft_file_is_hidden(fn) ){
			/* printf("fn %s is hidden\n",fn); */
			return NULL;
		}
		
		if( eft_valid_tkey(fn) ){
			/* printf("fn is a valid tkey\n"); */
			strncpy(tn,fn,13);
			return tn;
		}
		
		/* printf("fn %s is not a valid tkey\n",fn); */
		ret = eft_make_tname(tn,fn);
		
		return ret;
	}
}

/*
 * Now some functions to access the file-name / transfer-name database.
 *
 * The database is currently implemented by means of dummy dirent's
 * in the current working directory.
 *
 * FIXME: the latter currently won't work on filesystem which don't support
 * symlinks. Not very restrictive when used on Linux or BSD unless
 * you've mounted foreign file systems and want to export these via
 * Eurofile. Maybe a dbm based map is more appropriate for this.
 *
 * It also requires write permissions to the current working directory
 * in order to create the corresponding symlinks. This is not very
 * restrictive because without write permissions the eft user cannot change
 * the directory contents. Thus, dynamically changing the file name / transfer
 * name map is not very important as long as a map has been created
 * previously by a user with write access.
 */

/*
 * Check wether a file is marked as hidden. This does not necessarily mean
 * that the file is inaccessible by means of eft. It only indicates that
 * such files won't be presented in response to a t_dir indication.
 */
int eft_file_is_hidden(const char * fn)
{
	char *c, buf[MAXPATHLEN];
	int hlen = strlen(hide_prefix);
	struct stat s[1];
	int old_errno = errno;
#ifdef EFT_NAMES_DEBUG
	printf("checking for existence of %s\n",buf);
#endif	
	if( !strncmp(fn,EFT_METAFILE_PREFIX,6) ){return 1;}

        strcpy(buf,hide_prefix);
	buf[MAXPATHLEN-1] = 0;
	c = strncpy(buf+hlen,fn,MAXPATHLEN-hlen);
	/* this will hide dangling symlinks */
	if( c && !buf[MAXPATHLEN-1] && !lstat(buf,s) ){
		errno = old_errno;
		return 1;
	}
	errno = old_errno;
	return 0;
}

/*
 * hide a file name for eft
 */
int eft_hide_file(char * fn)
{
	char buf[MAXPATHLEN], *c;
	int hlen = strlen(hide_prefix);

	strcpy(buf,hide_prefix);
	buf[MAXPATHLEN-1]=0;
	c=strncpy(buf+hlen,fn,MAXPATHLEN-hlen);
	if( c && !c[MAXPATHLEN-hlen-1] ){
		/* apparrently, linux does not accept empty symlinks */
		if(symlink(".",buf)){
			perror("hide:symlink()");
			return -1;
		} else {
			return 0;
		}
	}
	return -1;
}
/*
 * Look up a general (key,value) pair in the database. Currently, the database
 * is implemented by means of the symlink facility of the underlaying
 * filesystem. The key is a symlink pathname, the value is the contents of
 * the symlink. To support storing multiple maps in the same directory
 * it is possible to prepend a prefix based on the map name to the symlink
 * name.
 */
char * eft_lookup_entry(char *value, const char *key, const char *map, int maxlen, int case_fix)
{
	char path[MAXPATHLEN], *c;
	int len, mlen=strlen(map);
	int old_errno = errno;

	strcpy(path,map);
	path[MAXPATHLEN-1] = 0;
	c=strncpy(path+mlen,key,MAXPATHLEN-mlen);
	/* printf("lookup_entry by readlink %s.\n",path); */
	if( c && !c[MAXPATHLEN-mlen-1] ){
		value[maxlen-1]=0;
		if( case_fix ) eft_fix_cases(c);
		len = readlink(path,value,maxlen);
		if(len<0 && errno != ENOENT){
			perror("lookup entry:readlink");
			errno = old_errno;
		}
		if( (len<0) || (value[maxlen-1]!=0) ){
			/* printf("entry %s not found, len=%d\n",path,len); */
			return NULL;
		} else {
			value[len]=0;
			/* printf("found value %s \n",value); */
			return value;
		}
	}
	/* printf("invalid path\n"); */
	return NULL;
}

/*
 * Look up the transfer name of a file in the database.
 */
char * eft_lookup_tname(char *tn, const char *fn)
{
	return eft_lookup_entry(tn, fn, fn_prefix, 13, 0);
}

/*
 * Look up the file name corresponding to a transfer name in the database.
 * It is assumed that the transfer name tn has already been verified to
 * be a valid ETS 300 075/383 transfer name keyword.
 */
char * eft_lookup_fname(char *fn, const char *tn, int case_fix, int dos)
{
	char * ret;
	/* XXX use different prefixes for DOS-constraint mapping*/

#ifdef EFT_ADD_LINK
        ret = eft_lookup_entry(fn, tn, tn_prefix, MAXPATHLEN, case_fix);
        if(ret){
                return eft_lookup_entry(fn, tn, "", MAXPATHLEN, case_fix);
        } else {
                return NULL;
        }
#else
	ret = eft_lookup_entry(fn, tn, tn_prefix, MAXPATHLEN, case_fix);
	if(ret>0){
		return ret;
	} else {
		return eft_lookup_entry(fn, tn, "", MAXPATHLEN, case_fix);
	}
#endif
}

/*
 * Add a (key,value) pair to the database;
 */
const char * eft_insert_entry(const char *value, const char *key, const char *map)
{
	const char *c;
	char path[MAXPATHLEN];
	int mlen=strlen(map);

	strcpy(path,map);
	path[MAXPATHLEN-1]=0;
	c=strncpy(path+mlen,key,MAXPATHLEN-mlen);
	if( c && !c[MAXPATHLEN-mlen-1] && symlink(value,path) ) return NULL;

	return value;
}

/*
 * remove (key,value) pair from the database;
 */
const char * eft_remove_entry(const char *value, const char *key,
			      const char *map)
{
	const char *c;
	char path[MAXPATHLEN];
	int mlen=strlen(map);

	strcpy(path,map);
	path[MAXPATHLEN-1]=0;
	c=strncpy(path+mlen,key,MAXPATHLEN-mlen);
	if( c && !c[MAXPATHLEN-mlen-1] && unlink(path) ) return NULL;

	return key;
}

/*
 * add a transfer name / file name pair to the database.
 */
const char * eft_insert_tname(const char *tn, const char *fn)
{
	const char * ret;

	eft_remove_entry(tn, fn, fn_prefix);
	ret = eft_insert_entry(tn, fn, fn_prefix);

	eft_remove_entry(fn, tn, tn_prefix);
	ret = eft_insert_entry(fn, tn, tn_prefix);

	return tn;
}

/*
 * Add a link corresponding to a transfer name for a file to the database.
 */
const char * eft_insert_link(const char *fn, const char *tn)
{
	eft_remove_entry(fn,tn,"");
	return eft_insert_entry(fn, tn, "");
}

/*
 * Mangle a file name into a unique transfer name keyword.
 *
 * eft transfer name keywords may consist of up to 12 IA5 (==ascii)
 * characters in the range from 0x21 to 0x7e, 
 * except '+', '/', '(', ')', and '*'.
 *
 * During previous interoperability tests it turned out that some
 * Eurofile implementation cause problems when transfer names are not
 * valid dos file names. Therefore, an additional restriction will
 * be supported on demand.
 *
 * The principle is as follows:
 * First the file name is truncated to 12 bytes. Then the file name is
 * searched for unallowed character. Those character are replaced by
 * characters unlikly to be used by human beeings.
 * If this generated a file name that already exists, then charcters are
 * changed. It is tried to avoid changing any charcters that still carry
 * information. Thus first characters at the bad positions are changed. If
 * this is not sufficient to create a unique transfer name, characters are
 * appended to the name if the name is shorter that 12 bytes.
 * As a last resource, other characters in the file name (starting from the
 * last) are replaced.
 */
char * eft_mangle_name(char *tn, const char *fn, int dos)
{
	const char invalid[] =    "+/()*";
        const char replace[] =    "?{}[];|";
        const char dos_replace[] = "#{}[]";
	const char *inv=invalid, *repl=replace;
	char *ld;
	int i, l, bad_pos[13], bad, good, tail, base, e, d,
		di[12], maxlen=12;
	/* XXX needs to be parameter */
	int case_fix=0;

	if( eft_valid_tkey(fn) ){
		strcpy(tn,fn);
		return tn;
	}
	
	strncpy(tn,fn,maxlen);
	tn[maxlen]=0;
	l=strlen(tn);
	for(i=l; i<=maxlen; i++) tn[i]=0;

	if(dos) {
		inv=dos_invalid;
		repl=dos_replace;
		/* special constraints for '.' */
		if(tn[0] == '.') tn[0]='?';
		ld = strrchr(tn,'.');
		printf(" dos %s\n",tn);
		if(!ld){
			for(i=l; i<8; i++) tn[i]='?';
			memmove(tn+9,tn+8,3);
			printf(" dos %s\n",tn);
			ld = tn+8;
			*ld='.';
			if(ld[1]==0) ld[1]='?';
			printf(" dos %s\n",tn);
		} else {
			i = (ld>tn+8) ? (tn+strlen(tn)-ld) : 4;
			printf(" dos i=%d\n",i);
			memmove(tn+8,ld,i);
			printf(" dos %s\n",tn);
			for(i=0; i<8; i++){
				if((tn[i]==0) || (tn[i]=='.'))  tn[i] = '?';
			}
			printf(" dos %s\n",tn);
		}
	}
	
	i=0;
	bad=0;
	good=12;
	tail=strlen(tn);

	while(tn[i]){
		if( (tn[i] < 0x21) || (tn[i] > 0x7e) || strchr(inv,tn[i]) ){
			bad_pos[bad] = i;
			bad++;
		} else {
			good--;
			bad_pos[good] = i;
		}
		i++;
	}

	for(i=bad; i<good; i++){
		bad_pos[i] = tail;
		tail++;
	}

	for(i=0;i<bad;i++) tn[ bad_pos[i] ] = repl[0];

	for(i=0;i<12;i++) di[i]=0;
	/* for(i=0;i<12;i++) printf(" %d", bad_pos[i]); printf("\n"); */
	
	if(dos){
		for(i=0;i<12;i++){
			if(bad_pos[i]==8){
				/* DOS dot needs to be changed last */
				bad_pos[i]  = bad_pos[11];
				bad_pos[11] = 8;
				break;
			}
		}
	}

	d=0;
	i=0;
	e=0;
	base=strlen(repl);
	while(1){
		/* printf("next %s\n",tn); */
		if( dos ){
			/* FIXME: need case insensitive match here ? */
		} else {
#if 0
			if( lstat(tn,s) && (errno==ENOENT) ) return tn;
#else
			char fn_tmp[MAXPATHLEN+1];
			if( ! eft_lookup_fname(fn_tmp, tn, case_fix,dos))return tn;
#endif
		}
		while( d >= (base-1) ){
			d=0;
			tn[ bad_pos[e] ] = repl[d];
			di[e]=d;
			e++;
			if(e >= maxlen) return NULL;
			d=di[e];
		};
		d++;
		di[e]=d;
		tn[ bad_pos[e] ] = repl[d];
		e=0;
	}
	return NULL;
}

/*
 * create a transfer name for a file and insert it in the database
 */
char * eft_make_tname(char *tn, const char *fn)
{
	char * ret;
	const char * rtn;
	/* XXX needs parameter */
	int dos=0;

	ret = eft_lookup_tname(tn,fn);
	if(ret) return ret;

	ret = eft_mangle_name(tn,fn,dos);
	if(ret){
		rtn = eft_insert_tname(tn,fn);
#ifdef EFT_ADD_LINK
		rtn = eft_insert_link(fn,tn);
#endif
	} else {
		printf("coudn't find a mangeled name for %s\n",fn);
	}
	return ret;
}

/*
 * Mark a symlink as alias (to be displayed as a transfer name only, not as
 * file name)
 */
const char * eft_mark_alias(const char * fn)
{
	return eft_insert_entry(fn, fn, alias_prefix);
}

int eft_is_alias(const char * fn)
{
	char dummy[MAXPATHLEN];
	return eft_lookup_entry(dummy,fn,alias_prefix,MAXPATHLEN,0) != NULL;
}
/*
 * Register an already existing symbolic link in the database to be
 * used as transfer name for the file it points to.
 */
const char * eft_use_as_tname(const char *tn, int force, int dos)
{
	char fn[MAXPATHLEN+1], tn_tmp[13];
	int len, i;
	struct stat s[1];

	len = readlink(tn,fn,MAXPATHLEN);
	if( (len<0) ){
		perror("use_as_tname:readlink");
		return NULL;
	}
		
	if( len>TDU_PLEN_DESIGNATION ) return NULL;
	
	fn[len]=0;
	/*
	 * we don't allow cross directory links because that would break
	 * the directory based database lookup
	 */
	for(i=0; i<len; i++){
		if(fn[i] == '/') return NULL;
	}
	if( stat(fn,s) ) return NULL;
	if( ! (S_ISREG(s->st_mode)) ) return NULL;

	if( (! force) && eft_lookup_tname(tn_tmp,fn) ) return NULL;

	eft_mark_alias(tn);
	return eft_insert_tname(tn,fn);
}


#if 0
int main(int argc, char **argv)
{
	char * res, tn[13], fn[MAXPATHLEN];

	if( argc!= 3){
		printf("usage %s -tf NAME\n",argv[0]);
	} else if(argv[1][1]=='t'){
		res = eft_fn_by_tn(fn,argv[2],flags);
		if(res){
			printf("transfer name %s looks up to %s\n",
			       argv[2],res);
		} else {
			printf("no file name found for transfer name %s\n",
			       argv[2]);
		}
	} else {
		res = eft_tn_by_fn(tn,argv[2],flags);
		if(res){
			printf("file %s has transfer name %s\n",
			       argv[2],res);
		} else {
			printf("no transfer name found for file %s\n",argv[2]);
		}
	}
	return 0;
}
#endif
