
/*
 *
 * Copyright (C) Eicon Technology Corporation, 2000.
 *
 * This source file is supplied for the exclusive use with Eicon
 * Technology Corporation's range of DIVA Server Adapters.
 *
 * Eicon File Revision :    1.8  
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY OF ANY KIND WHATSOEVER INCLUDING ANY 
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


/*
 * Source file for Unix Diva card library
 */

#include    <stdlib.h>
#include    <stdio.h>
#include	<string.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include	<sys/ioctl.h>
#include    <fcntl.h>
#include    <unistd.h>
#include    <errno.h>
/*#include <ioctls.h>*/
#include    "sys.h"
#include    "pc_maint.h"
#include    "divas.h"
#include    "load.h"
#include    "diva.h"
#include    "diload.h"
#include 	"cardtype.h"

#include    "linuxcfg.h"

int card_id;

extern byte new_cards[];
extern byte cards[];
extern dword selected_protocol_is_dmlt;
extern char* selected_protocol_code_directory;
extern int selected_bri_code_version;

/*int sprintf(char *,char *,...);*/
void *malloc(size_t);
/*
 * load the specified buffer containing DSP code onto the Diva card
 */

void load_combifile(int card_type, word wFeatures, int bri);

int DivaALoad (char *dsp_name,
							 dia_config_t *options,
							dia_card_t *card,
							char *msg , int adapter_instance) {
	int card_type = -1;
    int         loadfd;         /* Divas file pointer */
    int         fd;             /* file pointer */
    struct stat file_info;      /* info about file */
    dia_load_t   load;           /* information to load */
	char		*microcode_dir = selected_protocol_code_directory;
	char		filename[100];
	dia_start_t start;
	int rc;
    char	*pFeatureString;
    word	wFeatures = 0;

	card_id = load.card_id = options->card_id;
	start.card_id = options->card_id;

    /* open the Divas device */
    if ((loadfd = open(DIVAS_DEVICE_DFS,  O_RDONLY, 0)) < 0)
    if ((loadfd = open(DIVAS_DEVICE,  O_RDONLY, 0)) == -1)
    {
        sprintf(msg,DIVAS_DEVICE);
        return(ERR_ETDD_OPEN);
    }

	card = NULL;

    /* if we have command line options configured */
    if( card )
    {
        /* pass them down to the driver. */
        if((rc = ioctl(loadfd, DIA_IOCTL_INIT, card)) == -1)
        {
            fprintf(stderr, "%s INIT Error %d\n", DIVAS_DEVICE, rc);
            (void)close(loadfd);
            return(ERR_ETDD_IOCTL);
        }
    }
    card_type = cards[options->card_id + 1];
#if 0
    switch (card_type) {
			case 0: printf ("I: PRI Card\n"); break;
			case 1: printf ("I: BRI Card\n"); break;
			case 2: printf ("I: 4BRI Card\n"); break;
			default:
				printf ("I: unknown CARD\n");
		}
#endif


	if ((cards[options->card_id + 1] == 2) &&
			(adapter_instance == 4)) { // Only on first 4BRI virtual adapter
		strcpy(filename, microcode_dir);
		strcat(filename, "/ds4bri.bit");

		if ((fd = open(filename, O_RDONLY, 0)) == -1) {
			sprintf(msg,dsp_name);
			(void)close(loadfd);
			return(ERR_ETDD_DSP);
		}

		if (fstat(fd, &file_info)) {
			sprintf(msg,dsp_name);
			(void)close(loadfd);
			(void)close(fd);
			return(ERR_ETDD_ACCESS);
		}

		if (file_info.st_size <= 0) {
			sprintf(msg,"file error (%s)",dsp_name);
			(void)close(loadfd);
			(void)close(fd);
			return(ERR_ETDD_READ);
		}

		if (!(load.code = malloc(file_info.st_size))) {
			(void)close(loadfd);
			(void)close(fd);
			return(ERR_ETDD_NOMEM);
		}

		if (read(fd, load.code, file_info.st_size) != file_info.st_size) {
			sprintf(msg,"format error, %s", dsp_name);
			(void)close(loadfd);
			(void)close(fd);
			return(ERR_ETDD_READ);
		}

		load.length = file_info.st_size;
		load.code_type=DIA_FPGA_CODE;


		if ((ioctl(loadfd, DIA_IOCTL_LOAD, &load)) == -1) {
			fprintf(stderr, "%s LOAD\n", DIVAS_DEVICE);
			(void)close(loadfd);
			(void)close(fd);
			return(ERR_ETDD_IOCTL);
		}
		free(load.code); 
		close(fd); 
	}

	/*
		Download protocol code now
		*/

	if (selected_protocol_is_dmlt & (1 << card_type)) {
		char* p = strstr (dsp_name, ".");
		strcpy(filename, microcode_dir);
		strcat(filename, "te_dmlt");
		strcat(filename, p);
		if ((fd = open(filename, O_RDONLY, 0)) >= 0) {
			close (fd);
		} else {
			strcpy(filename, microcode_dir);
			strcat(filename, dsp_name);
		}
	} else {
		strcpy(filename, microcode_dir);
		strcat(filename, dsp_name);
	}

/*
	Alternative protocol code for BRI ?
	*/
	if (card_type == 1) {
		switch (selected_bri_code_version) {
			case 6:
				filename[strlen(filename)-1]='6';
				break;
			default:
		}
	}

	// printf ("load %s\n", filename);

	/* open this DSP binary for reading */
	if ((fd = open(filename, O_RDONLY, 0)) == -1) {
		sprintf(msg,dsp_name);
		(void)close(loadfd);
		return(ERR_ETDD_DSP);
	}

	if (fstat(fd, &file_info)) {
		sprintf(msg,dsp_name);
		(void)close(loadfd);
		(void)close(fd);
		return(ERR_ETDD_ACCESS);
	}

	if ( file_info.st_size <= 0 ) {
		sprintf(msg,"file error (%s)",dsp_name);
		(void)close(loadfd);
		(void)close(fd);
		return(ERR_ETDD_READ);
	}

	/* allocate a buffer and read contents of file into buffer */
	if (!(load.code = malloc(file_info.st_size))) {
		(void)close(loadfd);
		(void)close(fd);
		return(ERR_ETDD_NOMEM);
	}

	if (read(fd, load.code, file_info.st_size) != file_info.st_size) {
		sprintf(msg,"format error, %s", dsp_name);
		(void)close(loadfd);
		(void)close(fd);
		return(ERR_ETDD_READ);
	}

	pFeatureString = (char *) strstr(&load.code[0x80], "[F#");
	pFeatureString += 3;

	wFeatures = strtol(pFeatureString,NULL, 16);

	load.length = file_info.st_size;
	load.code_type=DIA_CPU_CODE;

	/* assign buffers and length and pass ioctl to /dev/Divas */
	if((ioctl(loadfd, DIA_IOCTL_LOAD, &load)) == -1) {
		fprintf(stderr, "%s LOAD\n", DIVAS_DEVICE);
		(void)close(loadfd);
		(void)close(fd);
		return(ERR_ETDD_IOCTL);
	}
	free(load.code); 


	(void) close(fd);
	(void) close(loadfd);

	if (cards[options->card_id + 1] == 0) {
		load_combifile(CARDTYPE_DIVASRV_P_9M_PCI, wFeatures, 0);
	} else if (cards[options->card_id + 1] == 1) {
		load_combifile(CARDTYPE_MAESTRA_PCI, wFeatures, 1);
	} else if ((cards[options->card_id + 1] == 2) && (adapter_instance == 1)) {
				/*
					Only on last 4BRI virtual adapter
					*/
			load_combifile(CARDTYPE_DIVASRV_Q_8M_PCI, wFeatures, 0);
	}

    /* open the Divas device */
    if ((loadfd = open(DIVAS_DEVICE_DFS, O_RDONLY, 0)) < 0)
	if ((loadfd = open(DIVAS_DEVICE, O_RDONLY, 0)) == -1) {
		sprintf(msg,DIVAS_DEVICE);
		return(ERR_ETDD_OPEN);
	}
   
	if ((ioctl(loadfd, DIA_IOCTL_CONFIG, options)) == -1) {
		fprintf(stderr, "%s CONFIG\n", DIVAS_DEVICE);
		(void)close(loadfd);
		return(ERR_ETDD_IOCTL);
	}

    /* Inform /dev/Divas that loading is finished */
	
	if (adapter_instance == 1) { // only do this once
		if((ioctl(loadfd, DIA_IOCTL_START, &start)) == -1) {
			fprintf(stderr, "%s START\n", DIVAS_DEVICE);
			(void)close(loadfd);
			return(ERR_ETDD_IOCTL);
		}
	}//if (adapter_instance == 1)

	(void)close(loadfd);

	return(ERR_ETDD_OK);
}
