/* $Id: eiconctrl.c,v 1.22 2001/03/01 14:59:12 paul Exp $
 *
 * Eicon-ISDN driver for Linux. (Control-Utility)
 *
 * Copyright 1998      by Fritz Elfert (fritz@isdn4linux.de)
 * Copyright 1998-2000 by Armin Schindler (mac@melware.de)
 * Copyright 1999,2000 Cytronics & Melware (info@melware.de)
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
 * $Log: eiconctrl.c,v $
 * Revision 1.22  2001/03/01 14:59:12  paul
 * Various patches to fix errors when using the newest glibc,
 * replaced use of insecure tempnam() function
 * and to remove warnings etc.
 *
 * Revision 1.21  2000/12/02 21:39:42  armin
 * Update of load and log utility.
 * New firmware and NT mode for PRI card.
 *
 * Revision 1.20  2000/07/08 14:18:52  armin
 * Changes for devfs.
 *
 * Revision 1.19  2000/06/12 12:29:06  armin
 * removed compiler warnings.
 *
 * Revision 1.18  2000/06/08 20:56:42  armin
 * added checking for card id.
 *
 * Revision 1.17  2000/06/08 08:31:03  armin
 * corrected tei parameter option
 *
 * Revision 1.16  2000/06/07 21:08:35  armin
 * Fixed OAD, OSA and SPID parameter setting.
 *
 * Revision 1.15  2000/04/24 07:53:03  armin
 * Extended interface to divaload.
 *
 * Revision 1.14  2000/03/25 12:56:40  armin
 * First checkin of new version 2.0
 * - support for 4BRI, includes orig Eicon
 *   divautil files and firmware updated (only etsi).
 *
 * Revision 1.13  2000/02/12 14:01:32  armin
 * Version 1.2
 * Added write function to management interface.
 * Fixed too small log buffer for isdnlog.
 *
 * Revision 1.12  2000/01/26 18:35:05  armin
 * New version of control utility.
 * Added activate-function for isdnlog trace information.
 *
 * Revision 1.11  2000/01/24 19:57:37  armin
 * Added INSTALL and README file.
 * Some updates and new option for configure script.
 *
 * Revision 1.10  2000/01/12 07:05:09  armin
 * Fixed error on loading old S card.
 *
 * Revision 1.9  1999/11/21 12:41:25  armin
 * Added further check for future driver changes.
 *
 * Revision 1.8  1999/10/12 18:01:52  armin
 * Backward compatible to older driver versions.
 *
 * Revision 1.7  1999/09/06 08:03:24  fritz
 * Changed my mail-address.
 *
 * Revision 1.6  1999/08/18 20:20:45  armin
 * Added XLOG functions for all cards.
 *
 * Revision 1.5  1999/03/29 11:05:23  armin
 * Installing and Loading the old type Eicon ISA-cards.
 * New firmware in one tgz-file.
 * Commandline Management Interface.
 *
 * Revision 1.4  1999/03/02 11:35:57  armin
 * Change of email address.
 *
 * Revision 1.3  1999/02/25 22:35:14  armin
 * Did not compile with new version.
 *
 * Revision 1.2  1999/01/20 21:16:45  armin
 * Added some debugging features.
 *
 * Revision 1.1  1999/01/01 17:27:57  armin
 * First checkin of new created control utility for Eicon.Diehl driver.
 * diehlctrl is obsolete.
 *
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <signal.h>
#ifdef HAVE_NCURSES_H
#	include <ncurses.h>
#endif
#ifdef HAVE_CURSES_H
#	include <curses.h>
#endif
#ifdef HAVE_CURSES_NCURSES_H
#	include <curses/ncurses.h>
#endif
#ifdef HAVE_NCURSES_CURSES_H
#	include <ncurses/curses.h>
#endif


#include <linux/types.h>
#include <linux/isdn.h>
#include <eicon.h>


#define COMBIFILE "dspdload.bin"

#define MIN(a,b) ((a)>(b) ? (b) : (a))

#define EICON_CTRL_VERSION 2 

char *cmd;
int verbose = 0;
eicon_manifbuf *mb;

WINDOW *statwin;
WINDOW *headwin;
static int h_line;
static int stat_y;

#ifdef HAVE_NPCI 
/* old driver Release < 2.0 */

int num_directory_entries;
int usage_mask_size;
int download_count;
int directory_size;
int usage_bit;
int usage_byte;
__u32 table_count = 0;
unsigned int no_of_downloads = 0;
int total_bytes_in_download = 0;
__u32 download_pos;
t_dsp_download_desc p_download_table[35];
char *no_of_tables;

#else
/* new driver Release >= 2.0 */

extern char DrvID[];

extern int Divaload_main(int, char **); 
#include <divas.h>

#endif

int fd;
isdn_ioctl_struct ioctl_s;

char protoname[1024];
char Man_Path[160];
int  man_ent_count;

struct man_s {
	u_char type;
	u_char attribute;
	u_char status;
	u_char var_length;
	u_char path_length;
	char   Name[180];
	char   Var[180];
} man_ent[60];

char *spid_state[] =
{
  "Idle",
  "Up",
  "Disconnecting",
  "Connecting",
  "SPID Initializing",
  "SPID Initialized",
  "UNKNOWN STATE"
};


#if HAVE_XLOG || HAVE_TRACE
/*********** XLOG stuff **********/

#define byte __u8
#define word __u16
#define dword __u32

/* #define SWAP(a) ((((a) & 0xff) << 8) | (((a) &0xff00)>>8)) */
#define SWAP(a) (a)

/*********** XLOG stuff end **********/
#endif /* XLOG */

__u16 xlog(FILE * stream,void * buffer);

void eiconctrl_usage() {
  fprintf(stderr,"Eiconctrl Utility Version 2.3                      (c) 2000 Cytronics & Melware\n");
  fprintf(stderr,"usage: %s add <DriverID> <membase> <irq>              (add card)\n",cmd);
  fprintf(stderr,"   or: %s [-d <DriverID>] membase [membase-addr]      (get/set memaddr)\n",cmd);
  fprintf(stderr,"   or: %s [-d <DriverID>] irq   [irq-nr]              (get/set irq)\n",cmd);
  fprintf(stderr,"   or: %s [-d <DriverID>] [-v] load <protocol> [options]\n",cmd);
  fprintf(stderr,"   or: %s [-d <DriverID>] debug [<debug value>]\n",cmd);
  fprintf(stderr,"   or: %s [-d <DriverID>] manage [read|exec <path>]   (management-tool)\n",cmd);
#ifdef HAVE_TRACE
  fprintf(stderr,"   or: %s [-d <DriverID>] xlog [cont|<filename>]      (retrieve XLOG)\n",cmd);
  fprintf(stderr,"   or: %s [-d <DriverID>] isdnlog [on|off]            (D-Channel log)\n",cmd);
#else
#ifdef HAVE_XLOG
  fprintf(stderr,"   or: %s [-d <DriverID>] xlog [cont]                 (request XLOG)\n",cmd);
#endif
#endif
  fprintf(stderr,"load firmware:\n");
  fprintf(stderr," basics  : -d <DriverID> ID defined when eicon module was loaded/card added\n");
  fprintf(stderr,"         : -v            verbose\n");
  fprintf(stderr," options : -l[channel#]  channel allocation policy\n");
  fprintf(stderr,"         : -e            CRC4 Multiframe usage\n");
  fprintf(stderr,"         : -tTEI         use the fixed TEI\n");
  fprintf(stderr,"         : -h            no HSCX 30 mode (PRI only)\n");
  fprintf(stderr,"         : -n            NT2 mode\n");
  fprintf(stderr,"         : -p            leased line D channel\n");
  fprintf(stderr,"         : -s[0|1|2]     LAPD layer 2 session strategy\n");
  fprintf(stderr,"         : -o            allow disordered info elements\n");
  fprintf(stderr,"         : -z            switch to loopback mode\n");
#ifndef HAVE_NPCI
  fprintf(stderr,"Please use 'divaload' for DIVA Server options.\n");
#endif
  exit(-1);
}


#ifdef HAVE_NPCI
/*--------------------------------------------------------------
 * display_combifile_details()
 *
 * Displays the information in the combifile header
 * Arguments: Pointer to the combifile structure in memory
 * Returns:   The address of the begining of directory which is
 *            directly after the file description.
 *-------------------------------------------------------------*/

t_dsp_combifile_directory_entry *display_combifile_details(char *details)
{
        __u32 offset=0;
        t_dsp_combifile_header *file_header;
        char *description;
        t_dsp_combifile_directory_entry *return_ptr = NULL;

        file_header = (t_dsp_combifile_header *)details;

        printf("%s\n", file_header->format_identification);
#ifdef DEBUG 
        printf("\tFormat Version: 0x%.4x\n", file_header->format_version_bcd);
        printf("\tNumber of directory entries : %d\n", file_header->directory_entries);
        printf("\tDownload count: %d\n", file_header->download_count);
#endif

        description = (char *)file_header + (file_header->header_size);

        printf("%s\n", description);

        num_directory_entries = file_header->directory_entries;
        usage_mask_size = file_header->usage_mask_size;
        download_count = file_header->download_count;
        directory_size = file_header->directory_size;

        return_ptr = (t_dsp_combifile_directory_entry *) (unsigned long)file_header ;
        offset  += (file_header->header_size);
        offset  += (file_header->combifile_description_size);
        offset += (__u32)(unsigned long)return_ptr;

        return (t_dsp_combifile_directory_entry *)(unsigned long)offset;

}


__u32 store_download(char *data, __u16 size, char *store)
{
        __u16 real_size;
        static int first = 1;
        static char* position;
        static char* initial;
        static __u32 addr;
        __u32 data_start_addr;
        __u32 align;

#ifdef DEBUG
        printf("Writing Data to memory block\n");
#endif
                if(first)
                {
                addr = 0xa03a0000 + (((sizeof(__u32)+sizeof(p_download_table)) + ~0xfffffffc) & 0xfffffffc);
                position = initial = (char *)store;
                first = 0;
                }

/*Starting address to where the data is written in the download_block*/
	data_start_addr = addr;

        real_size = size;

        align = ((addr + (real_size + ~0xfffffffc)) & 0xfffffffc) -
                        (addr + real_size);

        memcpy(position, data, real_size);

        position += real_size;
        addr += real_size;

        bzero(position, align);

        position += align;
        addr += align;

        total_bytes_in_download = position - initial;

#ifdef DEBUG
        printf("total_bytes written so far is 0x%x\n",total_bytes_in_download);
	printf("align value for this download is: %d\n",align);
#endif
        return (data_start_addr);
}



__u32 get_download(char *download_block, char *download_area)
{
        int n, i;
	char Text[100];
        char *usage_mask;
        char test_byte=0;
        __u32 length=0;
        __u32 addr;
    	unsigned int table_index;
        t_dsp_file_header *file_header;

        t_dsp_download_desc *p_download_desc;
        char *data;

	i=0;
        no_of_downloads = 0;
        test_byte = 0x01;
        test_byte <<= usage_bit;
        usage_mask = malloc(usage_mask_size);

        if(!usage_mask)
        {
                printf("Error allocating memory for usage mask");
                return 0;
        }
        bzero(usage_mask, usage_mask_size);

        for(n = 0; n < download_count; n++)
        {
                memcpy(usage_mask, download_area, usage_mask_size);
#ifdef DEBUG 
        printf("  \n");
                printf("Usage mask = 0x%.2x ", usage_mask[0]);
                if(usage_mask_size > 1)
                {
                        for(i=1; i<usage_mask_size; i++)
                        {
                                printf("0x%.2x ", usage_mask[i]);
                        }
                }
#endif
                download_area += usage_mask_size;

                file_header = (t_dsp_file_header *)download_area;
#ifdef DEBUG
                printf("%s %d\n", file_header->format_identification,n);
#endif

                if( test_byte & usage_mask[usage_byte] )
                {
                no_of_downloads++;
	        table_index = (no_of_downloads - 1);

#ifdef DEBUG
	            printf("*****DSP DOWNLOAD %d REQUIRED******\n", n);
        	    printf("download required count is now %d\n",no_of_downloads);
	            printf("   \n");
#endif


			/*This is the length of the memory to malloc */

                        length += ((__u32)((__u16)((file_header->header_size)
                                                - sizeof(t_dsp_file_header))))
                        + ((__u32)(file_header->download_description_size))
                        + ((__u32)(file_header->memory_block_table_size))
                        + ((__u32)(file_header->segment_table_size))
                        + ((__u32)(file_header->symbol_table_size))
                        + ((__u32)(file_header->total_data_size_dm))
                        + ((__u32)(file_header->total_data_size_pm));

                        if(download_block)
                        {
                        data = (char *)file_header;
                        data += ((__u32)(file_header->header_size));

                                p_download_desc = &(p_download_table[table_index]);
                                p_download_desc->download_id = file_header->download_id;
                                p_download_desc->download_flags = file_header->download_flags;
                                p_download_desc->required_processing_power = file_header->required_processing_power;
                                p_download_desc->interface_channel_count = file_header->interface_channel_count;
                                p_download_desc->excess_header_size = ((__u16)((file_header->header_size) - (__u16)sizeof(t_dsp_file_header)));
                                p_download_desc->memory_block_count = file_header->memory_block_count;
                                p_download_desc->segment_count = file_header->segment_count;
                                p_download_desc->symbol_count = file_header->symbol_count;
                                p_download_desc->data_block_count_dm = file_header->data_block_count_dm;
                                p_download_desc->data_block_count_pm = file_header->data_block_count_pm;

                                p_download_desc->p_excess_header_data = NULL;
                                if (((p_download_desc->excess_header_size) != 0))
                                {
#ifdef DEBUG
                        printf("1.store_download called from get_download\n");
#endif
                                        addr = store_download(data, p_download_desc->excess_header_size,
                                                                                        download_block);
                                        p_download_desc->p_excess_header_data = (char *)(unsigned long)addr;
                                        data += ((p_download_desc->excess_header_size));
                                }
                                p_download_desc->p_download_description = NULL;
                                if (((file_header->download_description_size) != 0))
                                {
#ifdef DEBUG
                        printf("2.store_download called from get_download\n");
#endif
                                        addr = store_download(data, file_header->download_description_size,
                                                                                        download_block);
				/* Showing details of DSP-Task */
			if (verbose) {
				strncpy(Text, data, file_header->download_description_size);
				Text[file_header->download_description_size] = 0;
				printf("\t%s\n", Text);
			}

                                        p_download_desc->p_download_description = (char *)(unsigned long)addr;
                                        data += ((file_header->download_description_size));
                                }
                                p_download_desc->p_memory_block_table = NULL;
                                if ((file_header->memory_block_table_size) != 0)
                                {
#ifdef DEBUG
                        printf("3.store_download called from get_download\n");
#endif
                                        addr = store_download(data, file_header->memory_block_table_size,
                                                                                        download_block);
                                        p_download_desc->p_memory_block_table = (t_dsp_memory_block_desc *)(unsigned long)addr;
                                        data += ((file_header->memory_block_table_size));
                                }
                                p_download_desc->p_segment_table = NULL;
                                if ((file_header->segment_table_size) != 0)
                                {
#ifdef DEBUG
                        printf("4.store_download called from get_download\n");
#endif
                                        addr = store_download(data, file_header->segment_table_size,
                                                                                        download_block);
                                        p_download_desc->p_segment_table = (t_dsp_segment_desc *)(unsigned long)addr;
                                        data += (file_header->segment_table_size);
                                }
                                p_download_desc->p_symbol_table = NULL;
                                if ((file_header->symbol_table_size) != 0)
                                {
#ifdef DEBUG
                        printf("5.store_download called from get_download\n");
#endif
                                        addr = store_download(data, file_header->symbol_table_size,
                                                                                        download_block);
                                        p_download_desc->p_symbol_table = (t_dsp_symbol_desc *)(unsigned long)addr;
                                        data += (file_header->symbol_table_size);
                                }
                                p_download_desc->p_data_blocks_dm = NULL;
                                if ((file_header->total_data_size_dm) != 0)
                                {
#ifdef DEBUG
                        printf("6.store_download called from get_download\n");
#endif
                                        addr = store_download(data, file_header->total_data_size_dm,
                                                                                        download_block);
                                        p_download_desc->p_data_blocks_dm = (__u16 *)(unsigned long)addr;
                                        data += (file_header->total_data_size_dm);
                                }
                                p_download_desc->p_data_blocks_pm = NULL;
                                if ((file_header->total_data_size_pm) != 0)
                                {
#ifdef DEBUG
                        printf("7.store_download called from get_download\n");
#endif
                                        addr = store_download(data, file_header->total_data_size_pm,
                                                                                        download_block);
                                        p_download_desc->p_data_blocks_pm = (__u16 *)(unsigned long)addr;
                                        data += (file_header->total_data_size_pm);
                                }
                        }
                }

                download_area += ((__u32)((__u16)((file_header->header_size))));
                download_area += ((__u32)((file_header->download_description_size)));
                download_area += ((__u32)((file_header->memory_block_table_size)));
                download_area += ((__u32)((file_header->segment_table_size)));
                download_area += ((__u32)((file_header->symbol_table_size)));
                download_area += ((__u32)((file_header->total_data_size_dm)));
                download_area += ((__u32)((file_header->total_data_size_pm)));


        }

	table_count=no_of_downloads;
	/**no_of_tables=table_count;*/
	bzero(no_of_tables,sizeof(__u32));
	memcpy(no_of_tables,&table_count,sizeof(__u32));

#ifdef DEBUG
    printf("***0x%x bytes of memory required for %d downloads***\n", length, no_of_downloads);
#endif
        free(usage_mask);
        return length;
}


eicon_codebuf *load_combifile(int card_type, u_char *protobuf, int *plen)
{
        int fd;
	int tmp[9];
	char combifilename[100];
        int count, j;
        int file_set_number = 0;
        struct stat file_info;
        char *combifile_start;
        char *usage_mask_ptr;
        char *download_block;
        t_dsp_combifile_directory_entry *directory;
        t_dsp_combifile_directory_entry *tmp_directory;
        __u32 download_size;
	eicon_codebuf *cb;

	sprintf(combifilename, "%s/%s", DATADIR, COMBIFILE);
        if ((fd = open(combifilename, O_RDONLY, 0)) == -1)
        {
                perror("Error opening Eicon combifile");
                return(0);
        }

        if (fstat(fd, &file_info))
        {
                perror("Error geting file details of Eicon combifile");
                close(fd);
                return(0);
        }

        if ( file_info.st_size <= 0 )
        {
                perror("Invalid file length in Eicon combifile");
                close(fd);
                return(0);
        }

        combifile_start = malloc(file_info.st_size);

        if(!combifile_start)
        {
                perror("Error allocating memory for Eicon combifile");
                close(fd);
                return(0);
        }

#ifdef DEBUG
                printf("File mapped to address 0x%x\n", (__u32)combifile_start);
#endif


        if((read(fd, combifile_start, file_info.st_size)) != file_info.st_size)
        {
                perror("Error reading Eicon combifile into memory");
                free(combifile_start);
                close(fd);
                return(0);
        }

        close(fd); /* We're done with the file */

        directory = display_combifile_details(combifile_start);

#ifdef DEBUG
        printf("Directory mapped to address 0x%x, offset = 0x%x\n", (__u32)directory,
                                                ((unsigned int)directory - (unsigned int)combifile_start));
#endif

        tmp_directory = directory;

        for(count = 0; count < num_directory_entries; count++)
        {
                if((tmp_directory->card_type_number) == card_type)
                {
#ifdef DEBUG
                        printf("Found entry in directory slot %d\n", count);
            		printf("Matched Card %d is %d. Fileset number is %d\n", count,
                       		(tmp_directory->card_type_number),
	                        (tmp_directory->file_set_number));

#endif
                        file_set_number = tmp_directory->file_set_number;
                        break;
                }
#ifdef DEBUG
                printf("Card %d is %d. Fileset number is %d\n", count,
                                                (tmp_directory->card_type_number),
                                                (tmp_directory->file_set_number));
#endif
                tmp_directory++; 
        }

        if(count == num_directory_entries)
        {
                printf("Card not found in directory\n");
                free(combifile_start);
                return(0);
        }

        usage_bit = file_set_number%8;
        usage_byte= file_set_number/8;

#ifdef DEBUG
        printf("Bit field is bit %d in byte %d of the usage mask\n",
                                usage_bit,
                                usage_byte);
#endif

        usage_mask_ptr = (char *)(directory);
        usage_mask_ptr += directory_size;

#ifdef DEBUG
        printf("First mask at address 0x%x, offset = 0x%x\n", (__u32)usage_mask_ptr,
                                                (__u32)(usage_mask_ptr - combifile_start));
#endif

        no_of_tables = malloc((sizeof(__u32)));
        download_size = get_download(NULL, usage_mask_ptr);

#ifdef DEBUG
        printf("Initial size of download_size is 0x%x\n",download_size);
#endif

        if(!download_size)
        {
                printf("Error getting details on DSP downloads\n");
                free(combifile_start);
                return(0);
        }

        download_block = malloc((download_size + (no_of_downloads * 100)));

#ifdef DEBUG
        printf("download_block size = (download_size + alignments) is: 0x%x\n",(download_size + (no_of_downloads*100)));
#endif

        if(!download_block)
        {
                printf("Error allocating memory for download\n");
                free(combifile_start);
                return(0);
        }

#ifdef DEBUG
        printf("Calling get_download to write into download_block\n");
#endif

        if(!(get_download(download_block, usage_mask_ptr)))
        {
                printf("Error getting data for DSP download\n");
                free(download_block);
                free(combifile_start);
	        free(no_of_tables);
                return(0);
        }

	tmp[0] = *plen;

	memcpy(protobuf + *plen, download_block, total_bytes_in_download);
	tmp[1] = total_bytes_in_download;
	*plen +=  total_bytes_in_download;

	memcpy(protobuf + *plen, no_of_tables, sizeof(table_count));
	tmp[2] = sizeof(table_count);
	*plen +=  sizeof(table_count);

	memcpy(protobuf + *plen, (char *)p_download_table, sizeof(p_download_table));
	tmp[3] = sizeof(p_download_table);
	*plen +=  sizeof(p_download_table);

	cb = malloc(sizeof(eicon_codebuf) + *plen);
	memset(cb, 0, sizeof(eicon_codebuf));
        memcpy(&cb->pci.code, protobuf, *plen);
    	for (j=0; j < 4; j++) {
		if (j==0) cb->pci.protocol_len = tmp[0];
		else cb->pci.dsp_code_len[j] = tmp[j];
	}
	cb->pci.dsp_code_num = 3;

 free(no_of_tables);
 free(download_block);
 free(combifile_start);
 return (cb);
}
#endif /* NPCI */

void beep2(void)
{
 beep();
 fflush(stdout);
 refresh();
}

int write_manage_element(char *m_dir, int request, char *bval, int vlen, int type)
{
	int len, ret, i, j;
	long lval;
	byte *buf;

	if (strlen(m_dir)) {
		if (m_dir[0] == '\\') m_dir++;
	}
	len = strlen(m_dir);
        mb->count = request;
        mb->pos = 0;
        mb->length[0] = len + vlen + 5;
        memset(&mb->data, 0, 690);

	if (len)
	        strncpy(&mb->data[5], m_dir, len);
        mb->data[4] = len;
        mb->data[3] = vlen;
	buf = &mb->data[5 + len];
	switch(type) {
		case 0x81:
			if (1 != sscanf(bval, "%ld", &lval))
				return(-1);
			for (i=0; i<vlen; i++) buf[i] = (byte)(lval >> (8*i));
			break;
		case 0x82:
			if (1 != sscanf(bval, "%lu", &lval))
				return(-1);
			for (i=0; i<vlen; i++) buf[i] = (byte)(lval >> (8*i));
			break;
		case 0x83:
			if (1 != sscanf(bval, "%lx", &lval))
				return(-1);
			for (i=0; i<vlen; i++) buf[i] = (byte)(lval >> (8*i));
			break;
		case 0x85:
			if (1 != sscanf(bval, "%lu", &lval))
				return(-1);
			if ((lval < 0) || (lval > 1))
				return(-1);
			for (i=0; i<(vlen-1); i++) buf[i] = 0;
			buf[i] = (byte)lval;
			break;
		case 0x87:
			for(i=0; i<vlen; i++) {
				buf[vlen-i-1] = 0;
				for(j=0; j<8; j++) {
					buf[vlen-i-1] |= (bval[i*9+j] - '0') << (7-j);
				}
			}
			break;
		default:
			return(-1);
	}
        mb->data[0] = type;
        ioctl_s.arg = (ulong)mb;
        ret = ioctl(fd, EICON_IOCTL_MANIF + IIOCDRVCTL, &ioctl_s);
	return(ret);
}

int get_manage_element(char *m_dir, int request)
{
	int i,j,o,k,tmp;
	int len, vlen = 0;
	long unsigned duint;
	u_char buf[100];

	if (strlen(m_dir)) {
		if (m_dir[0] == '\\') m_dir++;
	}
	len = strlen(m_dir);
        mb->count = request;
        mb->pos = 0;
        mb->length[0] = len + 5;
        memset(&mb->data, 0, 690);

	if (len)
	        strncpy(&mb->data[5], m_dir, len);
        mb->data[4] = len;

        ioctl_s.arg = (ulong)mb;
        if (ioctl(fd, EICON_IOCTL_MANIF + IIOCDRVCTL, &ioctl_s) < 0) {
		return(-1);
        }
	if (request == 0x04) return 0;

	mb->pos = 0;
	man_ent_count = mb->count;
        for (i = 0; i < mb->count; i++) {
		man_ent[i].type = mb->data[mb->pos++];
		man_ent[i].attribute = mb->data[mb->pos++];
		man_ent[i].status = mb->data[mb->pos++];
		man_ent[i].var_length = mb->data[mb->pos++];
		man_ent[i].path_length = mb->data[mb->pos++];

		memcpy(man_ent[i].Name, &mb->data[mb->pos] + len, man_ent[i].path_length - len);
		man_ent[i].Name[man_ent[i].path_length - len] = 0;
		if (man_ent[i].Name[0] == '\\') strcpy(man_ent[i].Name, man_ent[i].Name + 1);
		mb->pos += man_ent[i].path_length;

		if (man_ent[i].type &0x80) 
			vlen = man_ent[i].var_length;
		else
			vlen = mb->length[i] - man_ent[i].path_length - 5; 

		memcpy(man_ent[i].Var, &mb->data[mb->pos], vlen); 
		man_ent[i].Var[vlen] = 0;
		mb->pos += (mb->length[i]  - man_ent[i].path_length - 5);
		o = 0;
		if (vlen) {
			switch(man_ent[i].type) {
				case 0x04:
					if (man_ent[i].Var[0])
					{
						j=0;
						do
						{
							buf[o++] = man_ent[i].Var[++j];
							buf[o] = 0;
						}
						while (j < man_ent[i].Var[0]);
						strcpy(man_ent[i].Var, buf);
					}
					break;
				case 0x05:
					if (man_ent[i].Var[0])
					{
						j=0;
						do
						{
							o+=sprintf(&buf[o],"%02x ",(__u8)man_ent[i].Var[++j]);
						}
						while (j < man_ent[i].Var[0]);
						strcpy(man_ent[i].Var, buf);
					}
					break;
				case 0x81:
					duint = man_ent[i].Var[vlen-1]&0x80 ? -1 : 0; 
					for (j=0; j<vlen; j++) ((__u8 *)&duint)[j] = man_ent[i].Var[j];
					sprintf(man_ent[i].Var,"%ld",(long)duint);
					break;
				case 0x82:
					for (j=0,duint=0; j<vlen; j++) 
						duint += ((unsigned char)man_ent[i].Var[j]) << (8 * j);
					sprintf(man_ent[i].Var,"%lu", duint);
					break;
				case 0x83:
					for (j=0,duint=0; j<vlen; j++)
						duint += ((unsigned char)man_ent[i].Var[j]) << (8 * j);
					sprintf(man_ent[i].Var,"%lx", duint);
					break;
				case 0x84:
					for (j=0; j<vlen; j++) o+=sprintf(&buf[o], "%02x", man_ent[i].Var[j]);
					strcpy(man_ent[i].Var, buf);
					break;
				case 0x85:
					for (j=0,duint=0; j<vlen; j++)
						duint += ((unsigned char) man_ent[i].Var[j]) << (8 * j);
					if (duint) sprintf(man_ent[i].Var,"TRUE");
					else sprintf(man_ent[i].Var,"FALSE");
					break;
				case 0x86:
					for (j=0; j<vlen; j++)
					{
						if (j) o+=sprintf(&buf[o], ".");
						o+=sprintf(&buf[o], "%03i", man_ent[i].Var[j]);
					}
					strcpy(man_ent[i].Var, buf);
					break;
				case 0x87:
					for (j=0,duint=0; j<vlen; j++)
						duint += ((unsigned char) man_ent[i].Var[j]) << (8 * j);
					for (j=0; j<vlen; j++)
					{
						if (j) o+=sprintf(&buf[o], " ");
						tmp = (__u8)(duint >> (vlen-j-1)*8);
						for (k=0; k<8; k++)
						{
							o+=sprintf(&buf[o], "%d", (tmp >> (7-k)) &0x1);
						}
					}
					strcpy(man_ent[i].Var, buf);
					break;
				case 0x88:
					j = MIN(sizeof(spid_state)/sizeof(__u8 *)-1, man_ent[i].Var[0]);
					sprintf(man_ent[i].Var, "%s", spid_state[j]);
					break;
			}
		}
		else 
		if (man_ent[i].type == 0x02) strcpy(man_ent[i].Var, "COMMAND");
	}
  return(0);
}

void eicon_manage_head(void)
{
	int ctype;
        mvwaddstr(headwin, 0,0,"Management for Eicon DIVA Server cards                      Cytronics & Melware");
        if ((ctype = ioctl(fd, EICON_IOCTL_GETTYPE + IIOCDRVCTL, &ioctl_s)) < 0) {
                return;
        }
        switch (ctype) {
                case EICON_CTYPE_MAESTRAP:
                        mvwaddstr(headwin, 1,0,"Adapter-type is Diva Server PRI/PCI");
                        break;
                case EICON_CTYPE_MAESTRAQ:
                        mvwaddstr(headwin, 1,0,"Adapter-type is Diva Server 4BRI/PCI");
                        break;
                case EICON_CTYPE_MAESTRA:
                        mvwaddstr(headwin, 1,0,"Adapter-type is Diva Server BRI/PCI");
                        break;
                default:
                        mvwaddstr(headwin, 1,0,"Adapter-type is unknown");
                        return;
        }
}

void eicon_manage_init_ncurses(void)
{
        initscr();
        noecho();
        nonl();
        refresh();
        cbreak();
        keypad(stdscr,TRUE);
	curs_set(0);
	statwin = newpad(50,80);
	headwin = newpad(5,80);
        start_color();
	
}

void show_man_entries(void)
{
	int i;
	char MLine[80];
	char AttSt[7];

        for(i = 0; i < man_ent_count; i++) {
                if (man_ent[i].type == 0x01) {
                        sprintf(AttSt, "<DIR>");
                } else {
                        sprintf(AttSt,"%c%c%c%c%c",
                                (man_ent[i].attribute &0x01) ? 'w' : '-',
                                (man_ent[i].attribute &0x02) ? 'e' : '-',
                                (man_ent[i].status &0x01) ? 'l' : '-',
                                (man_ent[i].status &0x02) ? 'e' : '-',
                                (man_ent[i].status &0x04) ? 'p' : '-');
                }
                sprintf(MLine, "%-17s %s %s\n",
                        man_ent[i].Name,
                        AttSt,
                        man_ent[i].Var);
                printf(MLine);
        }
}

void eicon_manage_draw(void)
{
	int i;
	int max_line = 0;
	char MLine[80];
	char AttSt[7];

	mvwaddstr(headwin, 2, 0, "                                                                                ");
        mvwaddstr(headwin, 3,0,"Name              Flags Variable");
        mvwaddstr(headwin, 4,0,"-------------------------------------------------------------------------------");

        max_line =  man_ent_count;
        for(i = 0; i < max_line; i++) {
                if (man_ent[i].type == 0x01) {
                        sprintf(AttSt, "<DIR>");
                } else {
                        sprintf(AttSt,"%c%c%c%c%c",
                                (man_ent[i].attribute &0x01) ? 'w' : '-',
                                (man_ent[i].attribute &0x02) ? 'e' : '-',
                                (man_ent[i].status &0x01) ? 'l' : '-',
                                (man_ent[i].status &0x02) ? 'e' : '-',
                                (man_ent[i].status &0x04) ? 'p' : '-');
                }
                sprintf(MLine, "%-17s %s %-56s",
                        man_ent[i].Name,
                        AttSt,
                        man_ent[i].Var);
                if (i == h_line) wattron(statwin, A_REVERSE);
                mvwaddstr(statwin, i, 0, MLine);
                wattroff(statwin, A_REVERSE);
        }
        for(i = max_line; i < 50; i++) {
		mvwaddstr(statwin, i, 0, "                                                                                ");
	}
        prefresh(statwin, stat_y, 0, 5, 0, LINES-4, COLS);
        mvwaddstr(headwin, 2,0,"Directory : ");
        waddstr(headwin, Man_Path);
        prefresh(headwin, 0, 0, 0, 0, 5, COLS);
	refresh();
}

void do_manage_resize(int dummy) {
        endwin();
	eicon_manage_init_ncurses();
	eicon_manage_head();
	eicon_manage_draw();
	eicon_manage_draw();
	refresh();
	signal(SIGWINCH, do_manage_resize);
}

void rmws(char *text)
{
	int c;
	for (c=strlen(text)-1;c>=0;c--)
	{
		if (text[c]!=' ') return;
		text[c]='\0';
	}
	return;
}

int l_edit(char *buevar, char *uevar, int length)
{
	int ca,yq,xq,x,y;
	int ilen=0;
	int bilen=0;
	getyx(statwin,yq,xq);
	for(ca = 0; ca < length; ca++) uevar[ca]=32;
	uevar[ca]='\0';
	strcpy(uevar,buevar);
	ilen=strlen(buevar);
	uevar[ilen]=32;
	uevar[length]='\0';
	waddstr(statwin,uevar);
	wmove(statwin,yq,xq);
	laby01:
        prefresh(statwin, stat_y, 0, 5, 0, LINES-4, COLS);
	ca=getch();
	switch(ca)
	{
		case KEY_LEFT:
			if (bilen<=0) goto laby02;
			getyx(statwin,y,x);
			x--;
			bilen--;
			wmove(statwin,y,x);
			goto laby01;
		case KEY_RIGHT:
			if (bilen>=length) goto laby02;
			getyx(statwin,y,x);
			x++;
			bilen++;
			wmove(statwin,y,x);
			goto laby01;
		case KEY_BACKSPACE:
		case 8:
			if (bilen<1) goto laby02;
			getyx(statwin,y,x);
			x--; bilen--;
			for (ca=bilen;ca<(length-1);ca++) uevar[ca]=uevar[ca+1];
			uevar[length-1]=32;
			mvwaddstr(statwin,yq,xq,uevar);
			wmove(statwin,y,x);
			goto laby01;
		case 10:
		case 13:
			rmws(uevar);
			return(1);
		case 27:
			rmws(uevar);
			return(2);
	}
	if ((ca > 31) && (ca < 123))
	{
		if (length <= bilen) goto laby02;
		uevar[bilen]=ca;
		bilen++;
		getyx(statwin,y,x);
		x++;
		mvwaddstr(statwin,yq,xq,uevar);
		wmove(statwin,y,x);
		goto laby01;
	}
	laby02:
	beep2();
	goto laby01;
}

void eicon_management(void)
{
	int Key;
	int i;
	h_line = 0;
	stat_y = 0;

	signal(SIGWINCH, do_manage_resize);

	eicon_manage_init_ncurses();
	eicon_manage_head();
	eicon_manage_draw();
	redraw1:
	eicon_manage_draw();
	Keyboard:
	Key = getch();
	switch(Key) {
		case 27:
		case 'q':
		case 'Q':
		        move(22,0);
        		refresh();
	        	endwin();

                        close(fd); 
                        exit(0);
			break;
		case KEY_UP:
			if (h_line) {
				h_line--;
				if (stat_y > h_line) stat_y--;
			}
			goto redraw1;
		case KEY_DOWN:
			if (h_line < man_ent_count - 1) {
				h_line++;
				if ((stat_y + LINES - 9) < h_line) stat_y++;
			}
			goto redraw1;
		case KEY_LEFT:
			if ((strcmp(Man_Path,"\\")) && (strlen(Man_Path) > 1)) {
				for(i=strlen(Man_Path); i >= 0; i--) {
					if (Man_Path[i] == '\\') {
						Man_Path[i] = 0;
						break;
					}
				}
				if (strlen(Man_Path) == 0) strcpy(Man_Path,"\\");
				if (get_manage_element(Man_Path, 0x02) < 0) {
					clear();
		       	                mvaddstr(0,0, "Error ioctl Management-interface");
					refresh();
               			        return;
				}
				h_line = 0;
				stat_y = 0;
				goto redraw1;
			}
			beep2();
			goto Keyboard;
		case 10:
		case 13:
		case KEY_RIGHT:
			if (man_ent[h_line].type == 0x01) { /* Directory */
				if (Man_Path[strlen(Man_Path)-1] != '\\') strcat(Man_Path, "\\");
				strcat(Man_Path, man_ent[h_line].Name);
				if (get_manage_element(Man_Path, 0x02) < 0) {
					clear();
		       	                mvaddstr(0,0, "Error ioctl Management-interface");
					refresh();
               			        return;
				}
				h_line = 0;
				stat_y = 0;
				goto redraw1;
			}
			if (man_ent[h_line].type == 0x02) { /* Executable function */
				i = strlen(Man_Path);
				if (Man_Path[strlen(Man_Path)-1] != '\\') strcat(Man_Path, "\\");
				strcat(Man_Path, man_ent[h_line].Name);
				if (get_manage_element(Man_Path, 0x04) < 0) {
					clear();
		       	                mvaddstr(0,0, "Error ioctl Management-interface");
					refresh();
               			        return;
				}
				Man_Path[i] = 0;
				if (get_manage_element(Man_Path, 0x02) < 0) {
					clear();
		       	                mvaddstr(0,0, "Error ioctl Management-interface");
					refresh();
               			        return;
				}
				goto redraw1;

			}
                        if (man_ent[h_line].type == 0x06) { /* Trace Event */
                                int tcmd = 0x05;
                                i = strlen(Man_Path);
                                if (Man_Path[strlen(Man_Path)-1] != '\\') strcat(Man_Path, "\\");
                                strcat(Man_Path, man_ent[h_line].Name);
                                if (man_ent[h_line].status & 0x02)                                                                                                tcmd = 0x06;
                                if (get_manage_element(Man_Path, tcmd) < 0) {
                                        clear();
                                        mvaddstr(0,0, "Error ioctl Management-interface");
                                        refresh();
                                        return;
                                }
                                Man_Path[i] = 0;
                                if (get_manage_element(Man_Path, 0x02) < 0) {
                                        clear();
                                        mvaddstr(0,0, "Error ioctl Management-interface");
                                        refresh();
                                        return;
                                }
                                goto redraw1;

                        }
			if (man_ent[h_line].attribute & 0x01) { /* Writetable function */
				unsigned char eline[200];
				int ltmp;
                		sprintf(eline, "%-17s ", man_ent[h_line].Name);
                		wattron(statwin, A_BLINK|A_REVERSE);
                		mvwaddstr(statwin, h_line, 0, eline);
                		wattroff(statwin, A_BLINK|A_REVERSE);
				curs_set(1);
				wmove(statwin,h_line,24);
                		wattron(statwin, A_REVERSE);
				ltmp = l_edit(man_ent[h_line].Var, eline, 40);
				if ((strlen(eline) < 1) || (ltmp != 1)) {
                			wattroff(statwin, A_REVERSE);
					curs_set(0);
					goto redraw1;
				}
                		wattroff(statwin, A_REVERSE);
				curs_set(0);
				i = strlen(Man_Path);
				if (Man_Path[strlen(Man_Path)-1] != '\\') strcat(Man_Path, "\\");
				strcat(Man_Path, man_ent[h_line].Name);
				if (write_manage_element(Man_Path, 0x03, eline,
					man_ent[h_line].var_length, man_ent[h_line].type) < 0) {
					beep2();
				}
				Man_Path[i] = 0;
				if (get_manage_element(Man_Path, 0x02) < 0) {
					clear();
		       	                mvaddstr(0,0, "Error ioctl Management-interface");
					refresh();
               			        return;
				}
				goto redraw1;

			}
			beep2();
			goto Keyboard;
		case 'r':
			if (get_manage_element(Man_Path, 0x02) < 0) {
				clear();
	       	                mvaddstr(0,0, "Error ioctl Management-interface");
				refresh();
       			        return;
			}
			goto redraw1;
		default:
			beep2();
			goto Keyboard;
	}

}

void filter_slash(char *s)
{
	int i;
	for (i=0; i < strlen(s); i++)
		if (s[i] == '/') s[i] = '\\';
}

void load_startup_code(char *startupcode, char *fileext)
{
	FILE *code;
	u_char bootbuf[0x1000];
	char filename[100];
	int tmp;
	eicon_codebuf *cb;

	sprintf(filename, "%s/%s", DATADIR, startupcode);

	if (!(code = fopen(filename,"r"))) {
		perror(filename);
		exit(-1);
	}
	if ((tmp = fread(bootbuf, 1, sizeof(bootbuf), code))<1) {
		fprintf(stderr, "Read error on %s\n", filename);
		exit(-1);
	}
	fclose(code);
	cb = malloc(sizeof(eicon_codebuf) + tmp);
	memset(cb, 0, sizeof(eicon_codebuf));
	memcpy(&cb->isa.code, bootbuf, tmp);
	cb->isa.bootstrap_len = tmp;
	cb->isa.boot_opt = EICON_ISA_BOOT_NORMAL;
	printf("Loading Startup Code (%s %d bytes)...\n", startupcode, tmp);
	ioctl_s.arg = (ulong)cb;
	if (ioctl(fd, EICON_IOCTL_LOADBOOT + IIOCDRVCTL, &ioctl_s) < 0) {
		perror("ioctl LOADBOOT");
		exit(-1);
	}
	if ((tmp = ioctl(fd, EICON_IOCTL_GETTYPE + IIOCDRVCTL, &ioctl_s)) < 0) {
		perror("ioctl GETTYPE");
		exit(-1);
	}
	switch (tmp) {
		case EICON_CTYPE_S:
			strcpy(fileext,".bin");
			printf("Adapter-type is Eicon-S\n");
			break;
		case EICON_CTYPE_SX:
			strcpy(fileext,".sx");
			printf("Adapter-type is Eicon-SX\n");
			break;
		case EICON_CTYPE_SCOM:
			strcpy(fileext,".sy");
			printf("Adapter-type is Eicon-SCOM\n");
			break;
		case EICON_CTYPE_QUADRO:
			strcpy(fileext,".sq");
			printf("Adapter-type is Eicon-QUADRO\n");
			break;
		case EICON_CTYPE_S2M:
			strcpy(fileext,".p");
			printf("Adapter-type is Eicon-S2M\n");
			break;
		default:
			fprintf(stderr, "Unknown Adapter type %d for ISA-load\n", tmp);
			exit(-1);
	}
}


int main(int argc, char **argv) {
	int tmp;
	int ac;
	int arg_ofs=1;
	int card_id;

	cmd = argv[0];
	if (argc > 1) {
		if (!strcmp(argv[arg_ofs], "-h"))
			eiconctrl_usage();
		if (!strcmp(argv[arg_ofs], "--help"))
			eiconctrl_usage();
		
		if (!strcmp(argv[arg_ofs], "-d")) {
			arg_ofs++;
			if (arg_ofs >= argc)
				eiconctrl_usage();
			strcpy(ioctl_s.drvid, argv[arg_ofs++]);
			if (arg_ofs >= argc)
				eiconctrl_usage();
			if (!strcmp(argv[arg_ofs], "-v")) {
				arg_ofs++;
				verbose = 1;
			}
		} else {
			ioctl_s.drvid[0] = '\0';
			if (!strcmp(argv[arg_ofs], "-v")) {
				arg_ofs++;
				verbose = 1;
			}
		}
	} else
		eiconctrl_usage();

#ifndef HAVE_NPCI
	strcpy(DrvID, ioctl_s.drvid);
#endif

	ac = argc - (arg_ofs - 1);
	if (arg_ofs >= argc)
		eiconctrl_usage();
	fd = open("/dev/isdn/isdnctrl",O_RDWR | O_NONBLOCK);
	if (fd < 0)
		fd = open("/dev/isdnctrl",O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("/dev/isdnctrl");
		exit(-1);
	}

	if ((tmp = ioctl(fd, EICON_IOCTL_GETVER + IIOCDRVCTL, &ioctl_s)) < 0) {
		fprintf(stderr, "Driver ID %s not found or\n", ioctl_s.drvid);
		fprintf(stderr, "Eicon kernel driver is too old !\n"); 
		exit(-1);
	}
	if (tmp < EICON_CTRL_VERSION) {
		fprintf(stderr, "Eicon kernel driver is older than eiconctrl !\n"); 
		fprintf(stderr, "Please update !\n");
	}
	if (tmp > EICON_CTRL_VERSION) {
		fprintf(stderr, "Eicon kernel driver is newer than eiconctrl !\n"); 
		fprintf(stderr, "Please update !\n");
	}

	if (!strcmp(argv[arg_ofs], "add")) {
		eicon_cdef *cdef;
		if (ac != 5) 
			eiconctrl_usage();
		cdef = malloc(sizeof(eicon_cdef));
		strcpy(cdef->id, argv[arg_ofs + 1]);
		if (strlen(cdef->id) < 1)
			eiconctrl_usage();
		if (sscanf(argv[arg_ofs + 2], "%i", &cdef->membase) !=1 )
			eiconctrl_usage();
		if (sscanf(argv[arg_ofs + 3], "%i", &cdef->irq) !=1 )
			eiconctrl_usage();
		ioctl_s.arg = (ulong)cdef;
		if (ioctl(fd, EICON_IOCTL_ADDCARD + IIOCDRVCTL, &ioctl_s) < 0) {
			perror("ioctl ADDCARD");
			exit(-1);
		}
		printf("Card added.\n");
		close(fd);
		return 0;
	}
	if (!strcmp(argv[arg_ofs], "membase")) {
		if (ac == 3) {
			if (sscanf(argv[arg_ofs + 1], "%i", &tmp) !=1 )
				eiconctrl_usage();
			ioctl_s.arg = tmp;
			if (ioctl(fd, EICON_IOCTL_SETMMIO + IIOCDRVCTL, &ioctl_s) < 0) {
				perror("ioctl SETMMIO");
				exit(-1);
			}
		}
		if ((tmp = ioctl(fd, EICON_IOCTL_GETMMIO + IIOCDRVCTL, &ioctl_s)) < 0) {
			perror("ioctl GETMMIO");
			exit(-1);
		}
		printf("Shared memory at 0x%x\n", tmp);
		close(fd);
		return 0;
	}
	if (!strcmp(argv[arg_ofs], "irq")) {
		if (ac == 3) {
			if (sscanf(argv[arg_ofs + 1], "%i", &tmp) != 1)
				eiconctrl_usage();
			ioctl_s.arg = tmp;
			if (ioctl(fd, EICON_IOCTL_SETIRQ + IIOCDRVCTL, &ioctl_s) < 0) {
				perror("ioctl SETIRQ");
				exit(-1);
			}
		}
		if ((tmp = ioctl(fd, EICON_IOCTL_GETIRQ + IIOCDRVCTL, &ioctl_s)) < 0) {
			perror("ioctl GETIRQ");
			exit(-1);
		}
		printf("Irq is %d\n", tmp);
		close(fd);
		return 0;
	}

#ifndef HAVE_NPCI
        if (!strcmp(argv[arg_ofs], "divaload")) {
		int ret;
		int i, dlen;
		int lcard;
		int have, haves;
		char *newargv[35];
		int  newarg;

		labload:
		ret = 0;
		i = arg_ofs + 1;
		have = 0;
		haves = 0;

		newarg = 0;
		newargv[0] = malloc(50);
		strcpy(newargv[0], "eiconctrl.divaload");
		newarg++;

		card_id = -1;
		ioctl_s.arg = (ulong) &card_id;
		if ((ioctl(fd, EICON_IOCTL_GETTYPE + IIOCDRVCTL, &ioctl_s)) < 1) {
			perror("ioctl GETTYPE");
			exit(-1);
		}

		if (card_id > 0) {
			lcard = card_id - 1;
			printf("Card-ID = %d\n", card_id);
		} else {
			lcard = 0;
			dlen = strlen(ioctl_s.drvid);
			if (dlen) {
				if (isdigit(ioctl_s.drvid[dlen - 1])) {
					lcard = ioctl_s.drvid[dlen - 1] - '0';
				}
			}
		}

		while(argv[i]) {
			if ((!(strncmp(argv[i], "-t", 2))) && (strlen(argv[i]) > 2)){
				newargv[newarg] = malloc(4);
				strcpy(newargv[newarg], "-t");
				newarg++;
				newargv[newarg] = malloc(strlen(argv[i]) + 1);
				strcpy(newargv[newarg], argv[i] + 2);
			} else
			if ((!(strncmp(argv[i], "-l", 2))) && (strlen(argv[i]) > 2)){
				newargv[newarg] = malloc(4);
				strcpy(newargv[newarg], "-l");
				newarg++;
				newargv[newarg] = malloc(strlen(argv[i]) + 1);
				strcpy(newargv[newarg], argv[i] + 2);
			} else
			if ((!(strncmp(argv[i], "-s", 2))) && (strlen(argv[i]) > 2)){
				newargv[newarg] = malloc(4);
				strcpy(newargv[newarg], "-s");
				newarg++;
				newargv[newarg] = malloc(strlen(argv[i]) + 1);
				strcpy(newargv[newarg], argv[i] + 2);
			} else
			{
				newargv[newarg] = malloc(strlen(argv[i]) + 1);
				strcpy(newargv[newarg], argv[i]);
			}
			if ((!(strcmp(argv[i],"-c"))) || (!(strcmp(argv[i],"-all")))) {
				have = 1;
			}
			if (!(strcmp(argv[i],"-f"))) {
				haves = 1;
			}
			i++;
			newarg++;
		}

		if (!have) {
			newargv[newarg] = malloc(4);
			strcpy(newargv[newarg], "-c");
			newarg++;
			newargv[newarg] = malloc(4);
			sprintf(newargv[newarg], "%d", lcard + 1);
			newarg++;
		}
		if (!haves) {
			newargv[newarg] = malloc(4);
			strcpy(newargv[newarg], "-f");
			newarg++;
			newargv[newarg] = malloc(strlen(protoname) + 1);
			strcpy(newargv[newarg], protoname);
			newarg++;
		}
		newargv[newarg] = NULL;
		printf("Using Eicon's divaload...\n");
		ret = Divaload_main(newarg, newargv);
		return ret;
	}
#endif

        if ((!strcmp(argv[arg_ofs], "load")) || (!strcmp(argv[arg_ofs], "loadpci"))) {
                FILE *code;
		int isabus = 0;
                int plen = 0;
		int ctype = 0;
		int card_type = 0;
		int tmp, i;
		int tei = 255;
		char fileext[5];
                char filename[1024];
                u_char protobuf[0x100000];
                eicon_codebuf *cb = NULL;

		card_id = -1;

		if (argc <= (arg_ofs + 1))
                       	strcpy(protoname,"etsi");
		else {
			if (argv[arg_ofs + 1][0] == '-')
                       		strcpy(protoname,"etsi");
			else	
	                        strcpy(protoname,argv[++arg_ofs]);
		}
		
		if ((ctype = ioctl(fd, EICON_IOCTL_GETTYPE + IIOCDRVCTL, &ioctl_s)) < 1) {
			perror("ioctl GETTYPE");
			exit(-1);
		}
		switch (ctype) {
			case EICON_CTYPE_MAESTRAP:
				printf("Adapter-type is Diva Server PRI/PCI\n");
				card_type = 23;
#ifdef HAVE_NPCI
				strcpy(fileext, ".pm");
				tei = 1;
				break;
#else
				goto labload;
#endif
			case EICON_CTYPE_MAESTRA:
				printf("Adapter-type is Diva Server BRI/PCI\n");
				card_type = 21;
#ifdef HAVE_NPCI
				strcpy(fileext, ".sm");
				tei = 0;
				break;
#else
				goto labload;
#endif
#ifndef HAVE_NPCI
			case EICON_CTYPE_MAESTRAQ:
				printf("Adapter-type is Diva Server 4BRI/PCI\n");
				goto labload;
#endif
			case EICON_CTYPE_S:
			case EICON_CTYPE_SX:
			case EICON_CTYPE_SCOM:
			case EICON_CTYPE_QUADRO:
			case EICON_CTYPE_ISABRI:
				isabus = 1;
				tei = 0;
				load_startup_code("dnload.bin", fileext);
				break;
			case EICON_CTYPE_S2M:
			case EICON_CTYPE_ISAPRI:
				isabus = 1;
				tei = 1;
				load_startup_code("prload.bin", fileext);
				break;
			default:
				fprintf(stderr, "Adapter type %d not supported\n", ctype);
                          	exit(-1);
		}

	  	sprintf(filename, "%s/te_%s%s", DATADIR, protoname, fileext);
               	if (!(code = fopen(filename,"r"))) {
                               	perror(filename);
                               	exit(-1);
               	}
		printf("Protocol File : %s ", filename);
		if ((tmp = fread(protobuf, 1, sizeof(protobuf), code))<1) {
			fclose(code);
			fprintf(stderr, "Read error on %s\n", filename);
       	                exit(-1);
		}
               	fclose(code);
               	printf("(%d bytes)\n", tmp);
		plen += tmp;
			
		if (verbose) {
			if (isabus) {
				printf("Protocol: %s\n", &protobuf[4]);
				plen = (plen % 256)?((plen/256)+1)*256:plen;
			} else {
				strncpy(filename, &protobuf[0x80], 100);
				for (i=0; filename[i] && filename[i]!='\r' && filename[i]!='\n'; i++);
				filename[i] = 0;
				printf("%s\n", filename);
			}
		}

		if (isabus) {
			if(!(cb = malloc(sizeof(eicon_codebuf) + plen ))) {
        	                fprintf(stderr, "Out of Memory\n");
       	        	        exit(-1);
			}
			memset(cb, 0, sizeof(eicon_codebuf));
			memcpy(&cb->isa.code, protobuf, plen);
			cb->isa.firmware_len = plen;
		} else {
#ifdef HAVE_NPCI
			if (!(cb = load_combifile(card_type, protobuf, &plen))) {
        	                fprintf(stderr, "Error loading Combifile\n");
       	        	        exit(-1);
			}
#endif
		}

		if (isabus) {
			cb->isa.tei = tei;
			cb->isa.nt2 = 0;
			cb->isa.WatchDog = 0;
			cb->isa.Permanent = 0;
			cb->isa.XInterface = 0;
			cb->isa.StableL2 = 0;
			cb->isa.NoOrderCheck = 0;
			cb->isa.HandsetType = 0;
			cb->isa.LowChannel = 0;
			cb->isa.ProtVersion = 0;
			cb->isa.Crc4 = 0;
			cb->isa.Loopback = 0;
		} else {
#ifdef HAVE_NPCI
			cb->pci.tei = tei;
			cb->pci.nt2 = 0;
			cb->pci.WatchDog = 0;
			cb->pci.Permanent = 0;
			cb->pci.XInterface = 0;
			cb->pci.StableL2 = 0;
			cb->pci.NoOrderCheck = 0;
			cb->pci.HandsetType = 0;
			cb->pci.LowChannel = 0;
			cb->pci.ProtVersion = 0;
			cb->pci.Crc4 = 0;
			cb->pci.Loopback = 0;
			cb->pci.NoHscx30Mode = 0;
#endif /* NPCI */
		}

		/* parse extented options */
		while(ac > (arg_ofs + 1)) {
			arg_ofs++;
			if (!strncmp(argv[arg_ofs], "-l", 2)) {
				if (isabus) {
					cb->isa.LowChannel = atoi(argv[arg_ofs] + 2);
					if (!cb->isa.LowChannel) cb->isa.LowChannel = 1;
				} else {
#ifdef HAVE_NPCI
					cb->pci.LowChannel = atoi(argv[arg_ofs] + 2);
					if (!cb->pci.LowChannel) cb->pci.LowChannel = 1;
#endif /* NPCI */
				}
                                continue;
                        }
                        if (!strncmp(argv[arg_ofs], "-t", 2)) {
				if (isabus) {
                                	cb->isa.tei = atoi(argv[arg_ofs] + 2);
				} else {
#ifdef HAVE_NPCI
                                	cb->pci.tei = atoi(argv[arg_ofs] + 2);
#endif
				}
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-z")) {
				if (isabus) 
	                                cb->isa.Loopback = 1;
#ifdef HAVE_NPCI
				else
	                                cb->pci.Loopback = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-p")) {
				if (isabus) 
                                	cb->isa.Permanent = 1;
#ifdef HAVE_NPCI
				else
                                	cb->pci.Permanent = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-w")) {
				if (isabus) 
                        	        cb->isa.WatchDog = 1;
#ifdef HAVE_NPCI
				else
                        	        cb->pci.WatchDog = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-e")) {
				if (isabus) 
                	                cb->isa.Crc4 = 1;
#ifdef HAVE_NPCI
				else
                	                cb->pci.Crc4 = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-e1")) {
				if (isabus) 
        	                        cb->isa.Crc4 = 1;
#ifdef HAVE_NPCI
				else
        	                        cb->pci.Crc4 = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-e2")) {
				if (isabus) 
	                                cb->isa.Crc4 = 2;
#ifdef HAVE_NPCI
				else
	                                cb->pci.Crc4 = 2;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-n")) {
				if (isabus) 
                                	cb->isa.nt2 = 1;
#ifdef HAVE_NPCI
				else
                                	cb->pci.nt2 = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-p")) {
				if (isabus) 
                        	        cb->isa.Permanent = 1;
#ifdef HAVE_NPCI
				else
                        	        cb->pci.Permanent = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-h")) {
#ifdef HAVE_NPCI
                                if (!isabus) cb->pci.NoHscx30Mode = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-o")) {
				if (isabus) 
                	                cb->isa.NoOrderCheck = 1;
#ifdef HAVE_NPCI
				else
                	                cb->pci.NoOrderCheck = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-s")) {
				if (isabus) 
        	                        cb->isa.StableL2 = 1;
#ifdef HAVE_NPCI
				else
        	                        cb->pci.StableL2 = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-s1")) {
				if (isabus) 
	                                cb->isa.StableL2 = 1;
#ifdef HAVE_NPCI
				else
	                                cb->pci.StableL2 = 1;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-s2")) {
				if (isabus) 
                                	cb->isa.StableL2 = 2;
#ifdef HAVE_NPCI
				else
                                	cb->pci.StableL2 = 2;
#endif
                                continue;
                        }
                        if (!strcmp(argv[arg_ofs], "-s0")) {
				if (isabus) 
                                	cb->isa.StableL2 = 0;
#ifdef HAVE_NPCI
				else
                                	cb->pci.StableL2 = 0;
#endif
                                continue;
                        }
		}
		printf("Downloading Code (%d bytes)...\n", plen);
		ioctl_s.arg = (ulong)cb;
		tmp = (isabus) ? EICON_IOCTL_LOADISA : EICON_IOCTL_LOADPCI;
		if (ioctl(fd, tmp + IIOCDRVCTL, &ioctl_s) < 0) {
			printf("\nError, possibly updated driver, try re-compile utility !\n");
			perror("ioctl LOAD");
			exit(-1);
		}
		printf("completed.\n");
		close(fd);
		return 0;
        }

        if (!strcmp(argv[arg_ofs], "debug")) {
		if (argc <= (arg_ofs + 1))
			ioctl_s.arg = 1;
		else	
			ioctl_s.arg = atol(argv[arg_ofs + 1]);
		if (ioctl(fd, EICON_IOCTL_DEBUGVAR + IIOCDRVCTL, &ioctl_s) < 0) {
			perror("ioctl DEBUG VALUE");
			exit(-1);
		}
		return 0;
	}

        if (!strcmp(argv[arg_ofs], "freeit")) {
		if (ioctl(fd, EICON_IOCTL_FREEIT + IIOCDRVCTL, &ioctl_s) < 0) {
			perror("ioctl FREEIT");
			exit(-1);
		}
		return 0;
	}

#ifdef HAVE_TRACE
	if (!strcmp(argv[arg_ofs], "xlog")) {
		int dfd = fd;
		int cont = 0;
		char file[300];
		unsigned char buffer[1000];
		unsigned char inbuffer[1000];
		int pos = 0, ret = 1;
		unsigned char byte = 0;
		char *p, *q;
		unsigned long val, sec;

		if (argc > (++arg_ofs)) {
			if (!strcmp(argv[arg_ofs], "cont"))
				cont = 1;
			else {
				strcpy(file, argv[arg_ofs]);
				cont = 2;
				dfd = open(file, O_RDWR);
				if (dfd < 0) {
					fprintf(stderr, "File not found.\n");
					exit(-1);
				}
			}
		}
		mb = malloc(sizeof(eicon_manifbuf));
		strcpy (Man_Path, "\\Trace\\Log Buffer");
		if (cont < 2)
		get_manage_element(Man_Path, 0x05);
		while(1) {
			memset(buffer, 0, sizeof(buffer));
			memset(inbuffer, 0, sizeof(inbuffer));
			fflush(stdout);
			while((byte != 13) && (byte != 10) && (pos < 998) && (ret > 0)) {
				if ((ret = read(dfd, &byte, 1)) == 1) {
					inbuffer[pos++] = byte;
				}
			}
			byte = 0;
			pos = 0;
			if ((strlen(inbuffer) > 10) && (strncmp(inbuffer, "XLOG: ", 6) == 0)) {
				p = inbuffer + 6;
				val = strtol(p, &q, 16);
				sec=val/1000;
				printf("%5ld:%04ld:%03ld - ",
					(long)sec/3600,
					(long)sec%3600,
					(long)val%1000 );
				p = q;
				val = strtol(p, &q, 16);
				p = q;
				val = strtol(p, &q, 16);
				(unsigned short) *buffer = (unsigned short) val;
				pos = 2;
				while ((p != q) && (*q != 0)) {
					p = q;
					val = strtol(p, &q, 16);
					buffer[pos++] = val;
				}
				pos = 0;
				xlog(stdout, buffer);
			}
			if ((ret == 0) && (cont == 2))
				break;
			if ((ret < 0) && (cont != 1))
				break;
			if ((ret < 0) && (cont == 1))
				usleep(10000);
			ret = 1;
		}
		if (cont < 2)
			get_manage_element(Man_Path, 0x06);
		close(fd);
		return 0;
	}
#else
#ifdef HAVE_XLOG
	if (!strcmp(argv[arg_ofs], "xlog")) {
		int cont = 0;
		int ii;
		int ret_val;
		int end = 0;
		__u32 sec, msec;
		mi_pc_maint_t *pcm;
		xlogreq_t xlogreq;
		xlog_entry_t xlog_entry;
		xlog_entry_t swap_entry;
		__u8 *xlog_byte, *swap_byte;

		xlog_byte = (__u8 *)&xlog_entry;
		swap_byte = (__u8 *)&swap_entry;

		if ((argc > (++arg_ofs)) && (!strcmp(argv[arg_ofs], "cont")))
			cont = 1;
		printf("Getting log%s...\n", ((cont) ? ", CTRL-C to quit" : ""));

		pcm = &xlogreq.pcm;

		while(1)
		{
			fflush(stdout);

			memset(&xlogreq, 0, sizeof(xlogreq_t));
			xlogreq.command = 1;
			ioctl_s.arg = (ulong)&xlogreq;
			if ((ret_val = ioctl(fd, EICON_IOCTL_GETXLOG + IIOCDRVCTL, &ioctl_s)) < 0) {
				perror("ioctl XLOG");
                		close(fd); 
				exit(-1);
			}

			*(MIPS_BUFFER *)&xlog_entry = pcm->data;
			msec = (((__u32)xlog_entry.timeh)<<16)+xlog_entry.timel;
			sec=msec/1000;
			switch (ret_val)
			{
				case XLOG_OK:
					for(ii = 0; ii < MIPS_BUFFER_SZ; ii += 2)
					{
						swap_byte[ii] = xlog_byte[ii+1];
						swap_byte[ii+1] = xlog_byte[ii];
					}

					switch(xlog_entry.code)
					{
						case 1:
							msec = (((__u32)SWAP(xlog_entry.timeh))<<16)+SWAP(xlog_entry.timel);
							sec=msec/1000;
							printf("%5ld:%04ld:%03ld - ",
								(long)sec/3600,
								(long)sec%3600,
								(long)msec%1000 );
							printf("%s\n", xlog_entry.buffer );
							break;

						case 2:
							msec = (((__u32)SWAP(xlog_entry.timeh))<<16)+SWAP(xlog_entry.timel);
							sec=msec/1000;
							printf("%5ld:%04ld:%03ld - ",
								(long)sec/3600,
								(long)sec%3600,
								(long)msec%1000 );
							xlog(stdout, xlog_entry.buffer);
							break;

						default:
							printf("\n unknown code %d\n", SWAP(xlog_entry.code));
							end++;
							break;
					}

					break;
				case XLOG_ERR_DONE:
					if(cont) {
						sleep(2);
					} else {
						end++;
					}
					break;

				case XLOG_ERR_CARD_STATE:
					printf("Card in wrong state for tracing\n");
					end++;
					break;

				case XLOG_ERR_CMD:
					printf("Command error doing ioctl\n");
					end++;
					break;

				case XLOG_ERR_TIMEOUT:
					if(cont) {
						sleep(2);
					} else {
						printf("Xlog timeout\n");
						end++;
					}
					break;

				case XLOG_ERR_UNKNOWN:
					printf("Unknown error during ioctl\n");
					end++;
					break;

				 default:
					printf("Returned (%d)\n", ret_val);
					perror("Error doing ioctl");
					end++;
					break;
			}
			if( end )
				break;
		}
                close(fd); 
		return 0;
	}
#endif /* XLOG */
#endif /* TRACE */

#ifdef HAVE_TRACE
	if (!strcmp(argv[arg_ofs], "isdnlog")) {
		int tcmd = 0x05;
		int dval = 513;
		int ctype = -1;
		char LogLength[10];
		char EventEnable[50];

		strcpy(LogLength, "80");
		strcpy(EventEnable, "00000000 00000001");

		if ((ctype = ioctl(fd, EICON_IOCTL_GETTYPE + IIOCDRVCTL, &ioctl_s)) < 1) {
			perror("ioctl GETTYPE");
			exit(-1);
		}
		switch (ctype) {
			case EICON_CTYPE_MAESTRAP:
			case EICON_CTYPE_MAESTRAQ:
			case EICON_CTYPE_MAESTRA:
				break;
			default:
				fprintf(stderr, "Adapter type %d does not supported this.\n", ctype);
				exit(-1);
		}
		mb = malloc(sizeof(eicon_manifbuf));

		if (argc > (++arg_ofs)) {
		if (!strcmp(argv[arg_ofs++], "off")) {
				tcmd = 0x06;
				dval = 1;
				strcpy(LogLength, "30");
				strcpy(EventEnable, "00000000 11111111");
			}
		}
		ioctl_s.arg = dval;
		if (ioctl(fd, EICON_IOCTL_DEBUGVAR + IIOCDRVCTL, &ioctl_s) < 0) {
			perror("Error changing debug value.");
			exit(-1);
		}
		strcpy (Man_Path, "\\Trace\\Log Buffer");
		if (get_manage_element(Man_Path, tcmd) < 0) {
			fprintf(stderr, "Error or already in that state.\n");
			exit(-1);
		}
		strcpy (Man_Path, "\\Trace\\Max Log Length");
		if (write_manage_element(Man_Path, 0x03, LogLength, 2, 0x82) < 0) {
			fprintf(stderr, "Error changing Log Length.\n");
			exit(-1);
		}
		strcpy (Man_Path, "\\Trace\\Event Enable");
		if (write_manage_element(Man_Path, 0x03, EventEnable, 2, 0x87) < 0) {
			fprintf(stderr, "Error changing Event Enable.\n");
			exit(-1);
		}
		close(fd);
		return 0;
	}
#endif /* TRACE */

        if (!strcmp(argv[arg_ofs], "manage")) {
		mb = malloc(sizeof(eicon_manifbuf));

		if (argc > (++arg_ofs)) {
			if (!strcmp(argv[arg_ofs], "read")) {
				if (argc <= (arg_ofs + 1)) {
					fprintf(stderr, "Path to be read is missing\n");
					exit(-1);
				}
				strcpy(Man_Path, argv[arg_ofs + 1]);
				filter_slash(Man_Path);
				if (get_manage_element(Man_Path, 0x02) < 0) {
		       	                fprintf(stderr, "Error ioctl Management-interface\n");
               			        exit(-1);
				}
				show_man_entries();
				close(fd);
				return 0;
			}
			if (!strcmp(argv[arg_ofs], "exec")) {
				if (argc <= (arg_ofs + 1)) {
					fprintf(stderr, "Path to be executed is missing\n");
					exit(-1);
				}
				strcpy(Man_Path, argv[arg_ofs + 1]);
				filter_slash(Man_Path);
				if (get_manage_element(Man_Path, 0x04) < 0) {
		       	                fprintf(stderr, "Error ioctl Management-interface\n");
               			        exit(-1);
				}
				close(fd);
				return 0;
			}
			fprintf(stderr, "Unknown command for Management-interface\n");
			exit(-1);
		}

		strcpy (Man_Path, "\\");

		if (get_manage_element(Man_Path, 0x02) < 0) {
       	                fprintf(stderr, "Error ioctl Management-interface\n");
               	        exit(-1);
		}

		eicon_management();
		move(22,0);
		refresh();
		endwin();

                close(fd); 
		exit(0);
	}

	fprintf(stderr, "unknown command\n");
	exit(-1);
}
