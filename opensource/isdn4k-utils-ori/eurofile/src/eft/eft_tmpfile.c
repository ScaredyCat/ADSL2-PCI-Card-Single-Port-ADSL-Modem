/* $Id: eft_tmpfile.c,v 1.2 2001/03/01 14:59:12 paul Exp $ */
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
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <eft.h>
#include "eft_private.h"
#include <malloc.h>

int eft_make_tmp(void)
{
	char tmp[sizeof("/tmp/eftXXXXXX\0")];
	int fd;
	struct stat stat_ln, stat_fd;
		
	strcpy(tmp, "/tmp/eftXXXXXX");
	if( (fd = mkstemp(tmp)) < 0 ){
		perror("eft_make_tmp:mkstemp");
		return -1;
	}
	
	/* paranoia check to protect from symlink attacks */

	/* glibc2 seems to leave some members of struct stat
	 * un-initialzed which caused the check to report false alerts.
	 */
	memset(&stat_ln, 0, sizeof(stat_ln));
	memset(&stat_fd, 0, sizeof(stat_fd));
	if( lstat(tmp, &stat_ln) || fstat(fd,&stat_fd) 
	    || memcmp(&stat_ln, &stat_fd, sizeof(stat_ln) )){
		fprintf(stderr,"eft_make_tmp(): symlink attack for \"%s\" defended\n",tmp);
		close(fd);
		return -1;
	}
	
	if( unlink(tmp) ) perror("unlink()");
	return fd;
}

/*
 * Return a file discriptor to an open (temporary) file. The number of
 * temorary files available like this is limited (current to 1!). Thus
 * the file must be closed before this isc alled again.
 */
int eft_get_tmp(struct eft * eft)
{
	int fd;
	
	fd = dup(eft->tmp_fd);
	if( fd < 0 ) { 
		perror("eft_get_tmp: dup()");
	} else if( lseek(fd, 0, SEEK_SET) < 0 ){
		perror("eft_get_tmp: lseek()");
		fd = -1;
	} else if( ftruncate(fd,0) < 0 ){
		perror("eft_get_tmp: truncate()");
		fd = -1;
	}
		
	return fd;
}
/*
 * returns a file descriptor to a file that contains a string
 */
int eft_get_string_fd(struct eft * eft, unsigned char * str)
{
	int fd;

	fd = eft_get_tmp(eft);

	if( fd < 0 ){
		perror("cannot open tmp file");
		return fd;
	}
	write(fd, str, strlen(str));
	lseek(fd, 0, SEEK_SET);
	return fd;
}
