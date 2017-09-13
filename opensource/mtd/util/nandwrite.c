/*
 *  nandwrite.c
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 *   		  2003 Thomas Gleixner (tglx@linutronix.de)
 *
 * $Id: nandwrite.c,v 1.11 2004/05/06 06:47:13 gleixner Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Overview:
 *   This utility writes a binary image directly to a NAND flash
 *   chip or NAND chips contained in DoC devices. This is the
 *   "inverse operation" of nanddump.
 *
 * tglx: Major rewrite to handle bad blocks, write data with or without ECC
 *	 write oob data only on request
 *
 * Bug/ToDo:
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>

#include <asm/types.h>
#include "mtd/mtd-user.h"

#define PROGRAM "nandwrite"
#define VERSION "1.2"

/*
 * Buffer array used for writing data
 */
unsigned char writebuf[512];
unsigned char oobbuf[16];

// oob layouts to pass into the kernel as default
struct nand_oobinfo none_oobinfo = { 
	.useecc = MTD_NANDECC_OFF,
};

struct nand_oobinfo jffs2_oobinfo = { 
	.useecc = MTD_NANDECC_PLACE,
	.eccbytes = 6,
	.eccpos = { 0, 1, 2, 3, 6, 7 }
};

struct nand_oobinfo yaffs_oobinfo = { 
	.useecc = MTD_NANDECC_PLACE,
	.eccbytes = 6,
	.eccpos = { 8, 9, 10, 13, 14, 15}
};

struct nand_oobinfo autoplace_oobinfo = {
	.useecc = MTD_NANDECC_AUTOPLACE
};

void display_help (void)
{
	printf("Usage: nandwrite [OPTION] MTD_DEVICE INPUTFILE\n"
	       "Writes to the specified MTD device.\n"
	       "\n"
	       "  -a, --autoplace  	Use auto oob layout\n"
	       "  -j, --jffs2  	 	force jffs2 oob layout\n"
	       "  -y, --yaffs  	 	force yaffs oob layout\n"
	       "  -n, --noecc		write without ecc\n"
	       "  -o, --oob    	 	image copntains oob data\n"
	       "  -s addr, --start=addr set start address (default is 0)\n"
	       "  -q, --quiet    	don't display progress messages\n"
	       "      --help     	display this help and exit\n"
	       "      --version  	output version information and exit\n");
	exit(0);
}

void display_version (void)
{
	printf(PROGRAM " " VERSION "\n"
	       "\n"
	       "Copyright (C) 2003 Thomas Gleixner \n"
	       "\n"
	       PROGRAM " comes with NO WARRANTY\n"
	       "to the extent permitted by law.\n"
	       "\n"
	       "You may redistribute copies of " PROGRAM "\n"
	       "under the terms of the GNU General Public Licence.\n"
	       "See the file `COPYING' for more information.\n");
	exit(0);
}

char 	*mtd_device, *img;
int 	mtdoffset = 0;
int 	quiet = 0;
int	writeoob = 0;
int	autoplace = 0;
int	forcejffs2 = 0;
int	forceyaffs = 0;
int	noecc = 0;

void process_options (int argc, char *argv[])
{
	int error = 0;

	for (;;) {
		int option_index = 0;
		static const char *short_options = "os:ajynq";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"oob", no_argument, 0, 'o'},
			{"start", required_argument, 0, 's'},
			{"autoplace", no_argument, 0, 'a'},
			{"jffs2", no_argument, 0, 'j'},
			{"yaffs", no_argument, 0, 'y'},
			{"noecc", no_argument, 0, 'n'},
			{"quiet", no_argument, 0, 'q'},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				    long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
		case 0:
			switch (option_index) {
			case 0:
				display_help();
				break;
			case 1:
				display_version();
				break;
			}
			break;
		case 'q':
			quiet = 1;
			break;
		case 'a':
			autoplace = 1;
			break;
		case 'j':
			forcejffs2 = 1;
			break;
		case 'y':
			forceyaffs = 1;
			break;
		case 'n':
			noecc = 1;
			break;
		case 'o':
			writeoob = 1;
			break;
		case 's':
			mtdoffset = atoi (optarg);
			break;
		case '?':
			error = 1;
			break;
		}
	}
	
	if ((argc - optind) != 2 || error) 
		display_help ();
	
	mtd_device = argv[optind++];
	img = argv[optind];
}

/*
 * Main program
 */
int main(int argc, char **argv)
{
	int cnt, fd, ifd, imglen, pagelen, blockstart = -1;
	struct mtd_info_user meminfo;
	struct mtd_oob_buf oob;

	process_options(argc, argv);

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		perror("open flash");
		exit(1);
	}

	/* Fill in MTD device capability structure */  
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit(1);
	}

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 16 && meminfo.oobblock == 512) &&
	    !(meminfo.oobsize == 8 && meminfo.oobblock == 256) && 
	    !(meminfo.oobsize == 64 && meminfo.oobblock == 2048)) {
		fprintf(stderr, "Unknown flash (not normal NAND)\n");
		close(fd);
		exit(1);
	}
	
	// write without ecc ?
	if (noecc) {
		if (ioctl (fd, MEMSETOOBSEL, &none_oobinfo) != 0) {
			perror ("MEMSETOOBSEL");
			close (fd);
			exit (1);
		} 
	}

	// autoplace ECC ?
	if (autoplace) {
		if (ioctl (fd, MEMSETOOBSEL, &autoplace_oobinfo) != 0) {
			perror ("MEMSETOOBSEL");
			close (fd);
			exit (1);
		} 
	}

	// force oob layout for jffs2 or yaffs ?
	if (forcejffs2 || forceyaffs) {
		struct nand_oobinfo *oobsel = forcejffs2 ? &jffs2_oobinfo : &yaffs_oobinfo;
		
		if (forceyaffs && meminfo.oobsize == 8) {
    			if (forceyaffs) {
				fprintf (stderr, "YAFSS cannot operate on 256 Byte page size");
				close (fd);
				exit (1);
			}	
			/* Adjust number of ecc bytes */	
			jffs2_oobinfo.eccbytes = 3;	
		}
		
		if (ioctl (fd, MEMSETOOBSEL, oobsel) != 0) {
			perror ("MEMSETOOBSEL");
			close (fd);
			exit (1);
		} 
	}

	oob.length = meminfo.oobsize;
	oob.ptr = oobbuf;

	/* Open the input file */
	if ((ifd = open(img, O_RDONLY)) == -1) {
		perror("open input file");
		close(fd);
		close(ifd);
		exit(1);
	}

	// get image length
   	imglen = lseek(ifd, 0, SEEK_END);
	lseek (ifd, 0, SEEK_SET);

	pagelen = meminfo.oobblock + ((writeoob == 1) ? meminfo.oobsize : 0);
	
	// Check, if file is pagealigned
	if ( (imglen % pagelen) != 0) {
		perror ("Input file is not page aligned");
		close (fd);
		close (ifd);
		exit (1);
	}
	
	// Check, if length fits into device
	if ( ((imglen / pagelen) * meminfo.oobblock) > (meminfo.size - mtdoffset)) {
		perror ("Input file does not fit into device");
		close (fd);
		close (ifd);
		exit (1);
	}
	
	/* Get data from input and write to the device */
	while(mtdoffset < meminfo.size) {
		// new eraseblock , check for bad block
		if (blockstart != (mtdoffset & (~meminfo.erasesize + 1))) {
			blockstart = mtdoffset & (~meminfo.erasesize + 1);
			oob.start = blockstart;
			if (!quiet)
				fprintf (stdout, "Writing data to block %x\n", blockstart);
			if (ioctl(fd, MEMREADOOB, &oob) != 0) {
				perror("ioctl(MEMREADOOB)");
				close (ifd);
				close (fd);
				exit (1);
			}
			if (oobbuf[5] != 0xff) {
				if (!quiet)
					fprintf (stderr, "Bad block at %x, will be skipped\n", blockstart);
				mtdoffset = blockstart + meminfo.erasesize;
				continue;					
			}
		}
	
		/* Read Page Data from input file */
		if ((cnt = read(ifd, writebuf, meminfo.oobblock)) != meminfo.oobblock) {
			if (cnt == 0)	// EOF
				break;
			perror ("File I/O error on input file");
			close (fd);
			close (ifd);
			exit (1);						
		}	
		
		if (writeoob) {
			/* Read OOB data from input file, exit on failure */
			if ((cnt = read(ifd, oobbuf, meminfo.oobsize)) != meminfo.oobsize) {
				perror ("File I/O error on input file");
				close (fd);
				close (ifd);
				exit (1);						
			}
			/* Write OOB data first, as ecc will be placed in there*/
			oob.start = mtdoffset;
			if (ioctl(fd, MEMWRITEOOB, &oob) != 0) {
				perror ("ioctl(MEMWRITEOOB)");
				close (fd);
				close (ifd);
				exit (1);
			}
			imglen -= meminfo.oobsize;
		}
		
		/* Write out the Page data */
		if (pwrite(fd, writebuf, meminfo.oobblock, mtdoffset) != meminfo.oobblock) {
			perror ("pwrite");
			close (fd);
			close (ifd);
			exit (1);
		}
		imglen -= meminfo.oobblock;
		mtdoffset += meminfo.oobblock;
	}

	/* Close the output file and MTD device */
	close(fd);
	close(ifd);

	if (imglen > 0) {
		perror ("Data did not fit into device, due to bad blocks\n");
		exit (1);
	}

	/* Return happy */
	return 0;
}
