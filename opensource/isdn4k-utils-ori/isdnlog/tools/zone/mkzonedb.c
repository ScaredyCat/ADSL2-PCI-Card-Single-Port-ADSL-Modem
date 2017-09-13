/*
 * Make zone datafile
 *
 * Copyright 1999 by Leopold Toetsch <lt@toetsch.at>
 *
 * SYNOPSIS
 * mkzonedb -r Zonefile -d database [-f] [-v] [-V] [-o Oz] [-l L]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

static char progversion[] = "1.25";

/*
 * Changes:
 *
 * 1.25 2001.06.06 lt fixed sparc
 *
 * 1.23 1999.10.12 lt moved /CC/code handling to destination
 *
 * 1.11 1999.07.08 lt added support for NL
 *
 *      in NL areacode may be shorter than actual areacodenumber
 *      in this case \tLEN is appended to text
 *
 */

#define STANDALONE

#define _MKZONEDB_C_

#include <limits.h>

#include <stdlib.h>
#include <stdio.h>
#ifdef __GLIBC__
# define __USE_GNU  /* for declaration of basename() */
#endif
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#if !defined(__GLIBC__) && !defined(basename)
extern const char *basename (const char *name);
#endif

#include "config.h"
#include "common.h"
#include "upack.h"


void usage(char *argv[]) {
	fprintf(stderr, "%s: -r RedZonefile -d Database [ -v ] [ -V ] [ -o Localzone ] [ -l Len ]\n",
		basename(argv[0]));
	exit(EXIT_FAILURE);
}

static int (*zones)[3];
static int *numbers;
static bool verbose=false;
static bool stdoutisatty=true;  /* assume the worst :-) */
static int table[256];
static int tablelen, keylen, keydigs, maxnum;
static int n, nn;
static int ortszone=1;
static int numlen;

static void read_rzfile(char *rf) {
	int i;
	char line[BUFSIZ], *p, *op;
	FILE *fp;
	int from,to,z;

        if (strcmp(rf, "-")) {  /* use stdin? */
            if ((fp=fopen(rf, "r")) == 0) {
		fprintf(stderr, "Coudn't read '%s'\n", rf);
		exit(EXIT_FAILURE);
            }
	}
        else {
            fp = stdin;
        }
	maxnum = numlen==5 ? 40000 : 10000;
	if ((numbers = calloc(maxnum+1, sizeof(int))) == 0) {
		fprintf(stderr, "Out of mem\n");
		exit(EXIT_FAILURE);
	}
	n=i=keylen=keydigs=0;
	if (verbose)
		printf("Reading %s\n", rf);

	while (fgets(line, BUFSIZ, fp)) {
		if (verbose && stdoutisatty && (n % 1000) == 0) {
			printf("%d\r", n);
			fflush(stdout);
		}
		if (!*line)
			break;
		if (strlen(line)>40)
			fprintf(stderr, "Possible junk in line %d", n);
		from = strtoul(line, &p, 10);
		if (p-line > keydigs)
			keydigs=p-line;
		p++;
		op = p;
		to = strtoul(p, &p, 10);
		if (p-op > keydigs)
			keydigs=p-op;
		p++;
		z = strtoul(p, &p, 10);
		if (z > 127) {
			fprintf(stderr, "Something is wrong with this file (line %d)\n", n);
			exit(EXIT_FAILURE);
		}
		if ((zones = realloc(zones, (n+1)*3*sizeof(int))) == 0) {
			fprintf(stderr, "Out of mem\n");
			exit(EXIT_FAILURE);
		}
		zones[n][0]=from;
		zones[n][1]=to;
		zones[n][2]=z;
		if (from > keylen)
			keylen=from;
		if (to > keylen)
			keylen=to;
		if(to>maxnum) {
			maxnum=to;
			if ((numbers = realloc(numbers,(maxnum+1)*sizeof(int))) == 0) {
				fprintf(stderr, "Out of mem\n");
				exit(EXIT_FAILURE);
			}
		}
		numbers[to]++;
		n++;
	}
        if (fp != stdin) {
            fclose(fp);
        }
	free(rf);
	maxnum=keylen<maxnum?keylen:maxnum;
}

/* get the 256 top used nums in table */
static void make_table() {
	int i, j, k;
	tablelen = 0;
	if (verbose)
		printf("%d\nSorting\n", n);
	nn = maxnum;
	if (keylen < nn)
		nn = keylen;
	for (j=0; j<256; j++) {
		int max = 0;
		k = -1;
		for (i=0; i<=nn; i++) {
			if (numbers[i] > max) {
				k = i;
				max = numbers[i];
			}
		}
		if (k == -1) {
			nn = j;
			break;
		}
		numbers[k]=0;
		table[j] = k;
		if (k > tablelen)
			tablelen = k;
	}
	free(numbers);
	if (nn > 256)
		nn = 256;
}

static void write_db(char * df) {
	_DB db;
	datum key, value;
	UL ul, kul;
	US us, kus;
	int ofrom;
	int vlen;
	US count;
	char *val, *p;
	int i, j;
	char version[80];

	if (verbose)
		printf("Writing\n");
	if((db=OPEN(df,WRITE)) == 0) {
		fprintf(stderr, "Can't create '%s' - %s\n", df, GET_ERR);
		exit(EXIT_FAILURE);
	}
	/* tablelen .. len of table entries */
	/* keylen = keysize */
	if (maxnum > keylen)
		keylen=maxnum;
	keylen = keylen > 0xffff ? 4 : 2;
	tablelen = tablelen > 0xffff ? 4 : tablelen > 0xff ? 2 : 1;

	/* write version & table */
	key.dptr = "vErSiO";
	key.dsize = 7;
	/* version of zone.c must be not smaller than dataversion */
	sprintf(version,"V1.25 K%c C%c N%d T%d O%d L%d",
		keylen==2?'S':'L',tablelen==1?'C':tablelen==2?'S':'L',
		nn,n, ortszone, numlen?numlen:keydigs);
	value.dptr = version;
	value.dsize = strlen(version)+1;
	if(STORE(db, key, value)) {
		fprintf(stderr, "Error storing version key - %s\n", GET_ERR);
		exit(EXIT_FAILURE);
	}

	if ((p = val = calloc(nn, tablelen)) == 0) {
		fprintf(stderr, "Out of mem\n");
		exit(EXIT_FAILURE);
	}
	key.dptr = "_tAbLe\0";
	key.dsize = 7;
	for (i=0; i<nn; i++)
		if(tablelen==1)
			*p++ = (UC)table[i];
		else if(tablelen == 2) {
		  	// *((US*)p)++ = (US)table[i];
			tools_pack16(p, (US)table[i]);
		  	p += 2;
		}
		else {
			// *((UL*)p)++ = (UL)table[i];
			tools_pack32(p, (UL)table[i]);
		  	p += 4;
		}
	value.dptr = val;
	value.dsize = nn*tablelen;
	if(STORE(db, key, value)) {
		fprintf(stderr, "Error storing table key - %s\n", GET_ERR);
		exit(EXIT_FAILURE);
	}
	free(val);

	/* and write data */
	val = malloc(2); /* size of count */
	vlen = 2;
	ofrom = -1;
	count = 0;
	for (i=0; i<n; i++) {
		bool found = false;
		UC uc;
		unsigned char buf[4];

		if (verbose && stdoutisatty && (i % 1000) == 0) {
			printf("%d\r", i);
			fflush(stdout);
		}
		if (ofrom != -1 && ofrom != zones[i][0]) {
			// *((US*)val) = count;
			tools_pack16(val, count);
			value.dptr = val;
			value.dsize = vlen;
			if(STORE(db, key, value)) {
				fprintf(stderr, "Error storing key '%d' - %s\n",ofrom,GET_ERR);
				exit(EXIT_FAILURE);
			}
			free(value.dptr); /* the realloced val */
		}
		if (ofrom != zones[i][0]) {
			count = 0;
			val = malloc(2); /* size */
			vlen = 2;
			/* set up key */
			ofrom = zones[i][0];
			if (keylen == 4) {
				kul = (UL)ofrom;
				tools_pack32(buf, kul);
				key.dptr = buf;
			}
			else {
				kus = (US)ofrom;
				tools_pack16(buf, kus);
				key.dptr = buf;
			}
			key.dsize = keylen;
		}
		count++;
		for (j=0; j<nn; j++)
			if(table[j] == zones[i][1]) {
				found = true;
				val = realloc(val, vlen+2);
				uc = (UC)zones[i][2];
				val[vlen++] = uc;
				uc = (UC)j;
				val[vlen++] = uc;
				break;
			}
		if (!found) {
			val = realloc(val, vlen+1+keylen);
			zones[i][2] |= 128;
			uc = (UC)zones[i][2];
			val[vlen++] = uc;
			if(keylen == 2) {
				us = (US)zones[i][1];
				// *((US*)(&val[vlen])) = us;
				tools_pack16(&val[vlen], us);
			}
			else {
				ul = (UL)zones[i][1];
				// *((UL*)(&val[vlen])) = ul;
				tools_pack32(&val[vlen], ul);
			}
			vlen+=keylen;
		}
	}
	if(verbose)
		printf("%d\n", i);
	/* write last */
    	// *((US*)val) = count;
	tools_pack16(val, count);

	value.dptr = val;
	value.dsize = vlen;
	STORE(db, key, value);
	free(value.dptr);
	CLOSE(db);
	free(zones);
	free(df);
}

int main (int argc, char *argv[])
{
	char *df=0;
	char *rf=0;
	int c;

	if (argc < 2)
		usage(argv);
	while ( (c=getopt(argc, argv, "vVr:d:o:l:")) != EOF) {
		switch (c) {
			case 'v' : verbose = true; break;
			case 'V' : printf("%s: V%s Db=%s\n",
				basename(argv[0]), progversion, dbv); exit(1);
			case 'd' : df = strdup(optarg); break;
			case 'r' : rf = strdup(optarg); break;
			case 'o' : ortszone = atoi(optarg); break;
			case 'l' : numlen = atoi(optarg); break;
		}
	}
        if (verbose)
            stdoutisatty = isatty(fileno(stdout));

	read_rzfile(rf);
	make_table();
	write_db(df);

	/* Uff this got longer as I thought,
	   C is a real low level language -
	   now it's clear, why I prefer Perl
	*/
	return(EXIT_SUCCESS);
}
