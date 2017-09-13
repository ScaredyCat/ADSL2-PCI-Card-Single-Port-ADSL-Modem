
/*
 *
 * Copyright (C) Eicon Technology Corporation, 2000.
 *
 * This source file is supplied for the exclusive use with Eicon
 * Technology Corporation's range of DIVA Server Adapters.
 *
 * Eicon File Revision :    1.7  
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


/* Includes */

#include <stdio.h>
#include <unistd.h>
#include "sys.h" /* platform specific stuff */
#include "dsp_defs.h" /* combifile structures and definitions */
#include "cardtype.h" /* containd defines for card type ordinals */
#include "divas.h" /* defines for IOCTLs */
#include "constant.h"

#include "linux.h" /* Linux specific download stuff */
#include <sys/ioctl.h>
#include  <sys/types.h> /* for file info */
#include  <sys/stat.h> 
#include  <fcntl.h> 

#include <stdlib.h> /* For dynamic memory allocation */
#include <memory.h> /* For memcpy stuff */

#include <errno.h> /* For error checking stuff */

#include "linuxcfg.h"

/* Definitions */
/*
static char dsp_combifile_format_identification[DSP_COMBIFILE_FORMAT_IDENTIFICATION_SIZE] =
{
  'E', 'i', 'c', 'o', 'n', '.', 'D', 'i',
  'e', 'h', 'l', ' ', 'D', 'S', 'P', ' ',
  'D', 'o', 'w', 'n', 'l', 'o', 'a', 'd',
  ' ', 'C', 'o', 'm', 'b', 'i', 'f', 'i',
  'l', 'e', '\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0'
};

static char dsp_file_format_identification[DSP_FILE_FORMAT_IDENTIFICATION_SIZE] =
{
  'E', 'i', 'c', 'o', 'n', '.', 'D', 'i',
  'e', 'h', 'l', ' ', 'D', 'S', 'P', ' ',
  'D', 'o', 'w', 'n', 'l', 'o', 'a', 'd',
  '\0','F', 'i', 'l', 'e','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0'
};
*/

/* #define COMBIFILE DATADIR "/dspdload.bin" */

/*These files will contain the binaries wriiten to card*/
#ifdef DEBUG
#define DSPFILE   DATADIR "/debug_dsp"
#define TABLEFILE DATADIR "/debug_tablefile"
#endif

/* External references */

extern int num_directory_entries;
extern int usage_mask_size;
extern int download_count;
extern int directory_size;

extern int usage_bit;
extern int usage_byte;

extern int card_id;
static int first_load = TRUE;


int set_download_pos(int card_type, word wFeatures);

/* Forward references */
t_dsp_combifile_directory_entry *display_combifile_details(char *details);
dword get_download(char *download_block, char *usage_mask_ptr);
dword store_download(char *data, word size, char *store);
int set_alignment_mask(int card_type);
int download(char *block, dword size, int code);

extern char* selected_protocol_code_directory;
extern int selected_bri_code_version;

/*--------------------------------------------------------------------------
 * load_combifile() function
 * 
 * opens combifile, reads into buffer and calls fn to display info.
 * It then parses the directory for the fileset required for the
 * specified card. It the calls a function to determine the amount of memory 
 * required for the download and calls it again to get the download required.
 * Finally it downloads the data from the combi file and a table of addresses.
 *
 * Parameters: cardtype = Card ordinal as specified in cardtype.h
 *--------------------------------------------------------------------------*/

void load_combifile(int card_type, word wFeatures, int bri_card)
{
	int fd;
	int count;
	int file_set_number=0;
	struct stat file_info;
	char *combifile_start;
	char *usage_mask_ptr;
	char *download_block;
	t_dsp_combifile_directory_entry *directory;
	t_dsp_combifile_directory_entry *tmp_directory;
	dword download_size;
	char COMBIFILE[1024];

	strcpy (COMBIFILE, selected_protocol_code_directory);
	strcat (COMBIFILE, "dspdload.bin");
	if ((bri_card == 1) && (selected_bri_code_version)) {
		strcpy (COMBIFILE, selected_protocol_code_directory);
		strcat (COMBIFILE, "dspdload.s6");
	}

	//printf ("I: DSP FILE:<%s>\n", COMBIFILE);
  
#ifdef DEBUG 
	int dsp_fd;
    int table_fd;
#endif
	if(!set_alignment_mask(card_type))
	{
		return;
	}

	if(!set_download_pos(card_type, wFeatures))
	{
		return;
	}

	if ((fd = open(COMBIFILE, O_RDONLY, 0)) == -1)
	{
		perror("Error opening Eicon combifile");
		return;
	}

	if (fstat(fd, &file_info))
	{
		perror("Error geting file details of Eicon combifile");
		close(fd);
		return;
	}

	if ( file_info.st_size <= 0 )
	{
		perror("Invalid file length in Eicon combifile");
		close(fd);
		return;
	}

	combifile_start = malloc(file_info.st_size);

	if(!combifile_start)
	{
		perror("Error allocating memory for Eicon combifile");
		close(fd);
		return;
	}

#ifdef DEBUG
		printf("File mapped to address 0x%x\n", combifile_start);
#endif

	if((read(fd, combifile_start, file_info.st_size)) != file_info.st_size)
	{
		perror("Error reading Eicon combifile into memory");
		free(combifile_start);
		close(fd);
		return;
	}

	close(fd); /* We're done with the file */

	directory = display_combifile_details(combifile_start);

#ifdef DEBUG
	printf("Directory mapped to address 0x%x, offset = 0x%x\n", directory,
						((unsigned int)directory - (unsigned int)combifile_start));
#endif

	tmp_directory = directory;

	for(count = 0; count < num_directory_entries; count++)
	{
		if(BYTE_SWAP_WORD(tmp_directory->card_type_number) == card_type)
		{
#ifdef DEBUG
			printf("Found entry in directory slot %d\n", count);
		printf("File set number is %d\n", 
						BYTE_SWAP_WORD(tmp_directory->file_set_number));
            printf("Matched Card %d is %d. Fileset number is %d\n", count,
                        BYTE_SWAP_WORD(tmp_directory->card_type_number),
                        BYTE_SWAP_WORD(tmp_directory->file_set_number));

#endif
			file_set_number = BYTE_SWAP_WORD(tmp_directory->file_set_number);
			break;
		}
#ifdef DEBUG
		printf("Card %d is %d. Fileset number is %d\n", count,
						BYTE_SWAP_WORD(tmp_directory->card_type_number),
						BYTE_SWAP_WORD(tmp_directory->file_set_number));
#endif
		tmp_directory++;
	}

	if(count == num_directory_entries)
	{
		printf("Card not found in directory\n");
		free(combifile_start);
		return;
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
	printf("First mask at address 0x%x, offset = 0x%x\n", usage_mask_ptr,
						(usage_mask_ptr - combifile_start));
#endif
    
	no_of_tables = malloc((sizeof(dword)));
	download_size = get_download(NULL, usage_mask_ptr);

#ifdef DEBUG
	printf("Initial size of download_size is 0x%x\n",download_size);
#endif

	if(!download_size)
	{
		printf("Error getting details on DSP downloads\n");
		free(combifile_start);
		return;
	}

	/* Allocate the amount of space to hold the details from the 
	 * combifile plus an additional amount to allow for alignment
	 * on dword boundary. (Max. shift is 3 bytes for each download)
	 */
     
    
	download_block = malloc((download_size + (no_of_downloads * 100)));

#ifdef DEBUG
	printf("download_block size = (download_size + alignments) is: 0x%x\n",(download_size + (no_of_downloads * 100)));
#endif


	if(!download_block)
	{
		printf("Error allocating memory for download\n");
		free(combifile_start);
		return;
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
		return;
	}

#ifdef DEBUG
    printf("Downloading data using IOCTLs to Card\n");
#endif

    
#ifdef DEBUG

	if ((dsp_fd = open(DSPFILE, O_RDWR)) == -1)
	{
		perror("Error opening Eicon dsp_file");
		return;
	}

	if ((!(write(dsp_fd,download_block,total_bytes_in_download))))
	{
		perror("Error writing to Eicon dsp_file");
        return;
    }

    close(dsp_fd);

#endif


	if(!(download(download_block, total_bytes_in_download, DIA_DSP_CODE)))
	{
		printf("Error downloading Combifile details\n");
		free(download_block);
		free(combifile_start);
        free(no_of_tables);
		return;
	}
     

    	 
	if(!(download(no_of_tables,sizeof(table_count), DIA_DLOAD_CNT)))
	{
		printf("Error downloading number of downloads to load\n");
		free(download_block);
		free(combifile_start);
		free(no_of_tables);
		return;
	}
	
#ifdef DEBUG
	
	if ((table_fd = open(TABLEFILE, O_RDWR)) == -1)
	{
		perror("Error opening Eicon table_file");
		return;
	}
	
	if ((!(write(table_fd,(char *)p_download_table,sizeof(p_download_table)))))
	{
		perror("Error writing to Eicon table_file");
        return;
    }

     close(table_fd);

#endif

	if(!(download((char *)p_download_table,sizeof(p_download_table), DIA_TABLE_CODE)))
	{
		printf("Error downloading Combifile details\n");
		free(download_block);
		free(combifile_start);
        free(no_of_tables);
		return;
	}
	
    


	free(download_block);
	free(combifile_start);
    free(no_of_tables);
	return;

}


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
	dword offset=0;
	t_dsp_combifile_header *file_header;
	char *description;
	t_dsp_combifile_directory_entry *return_ptr = NULL;

	file_header = (t_dsp_combifile_header *)details;

#ifdef DEBUG
	printf("%s\n", file_header->format_identification);
	printf("\tFormat Version: 0x%.4x\n", 
						BYTE_SWAP_WORD(file_header->format_version_bcd));
	printf("\tNumber of directory entries : %d\n", 
						BYTE_SWAP_WORD(file_header->directory_entries));
	printf("\tDownload count: %d\n", BYTE_SWAP_WORD(file_header->download_count));
#endif

	description = (char *)file_header + BYTE_SWAP_WORD((file_header->header_size));

	printf("%s\n", description);

	num_directory_entries = BYTE_SWAP_WORD(file_header->directory_entries);
	usage_mask_size = BYTE_SWAP_WORD(file_header->usage_mask_size);
	download_count = BYTE_SWAP_WORD(file_header->download_count);
	directory_size = BYTE_SWAP_WORD(file_header->directory_size);

	return_ptr = (t_dsp_combifile_directory_entry *) (unsigned long)file_header ;
	offset 	+= (BYTE_SWAP_WORD((file_header->header_size)));
	offset	+= (BYTE_SWAP_WORD((file_header->combifile_description_size)));
	offset += (dword)return_ptr;

	return (t_dsp_combifile_directory_entry *)offset;
	
}


/*-----------------------------------------------------------------------
 * get_download ()
 *
 * Loops for each download in the combifile, reading the usage mask to
 * determine if this DSP code is required for the current file set.
 * If a memory address is specified for download_area, the code is stored
 * there.
 * Arguments:ptr to download_block to store code,ptr to first usage_mask in 
 *           combifile which has been read into memory     
 * Returns:  length of download required
 *-----------------------------------------------------------------------*/


dword get_download(char *download_block, char *download_area)
{
	int n;
	char *usage_mask;
	char test_byte=0;
	dword length=0;
	dword addr;
    unsigned int table_index;
	t_dsp_file_header *file_header;
	t_dsp_download_desc *p_download_desc;
	char *data;

#ifdef DEBUG
	int i;
#endif

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
     

/*This is the lenth of the memory to malloc */ 

			length += ((dword)((word)(BYTE_SWAP_WORD(file_header->header_size) 
						- sizeof(t_dsp_file_header))))
			+ ((dword)BYTE_SWAP_WORD((file_header->download_description_size)))
			+ ((dword)BYTE_SWAP_WORD((file_header->memory_block_table_size)))
			+ ((dword)BYTE_SWAP_WORD((file_header->segment_table_size)))
			+ ((dword)BYTE_SWAP_WORD((file_header->symbol_table_size)))
			+ ((dword)BYTE_SWAP_WORD((file_header->total_data_size_dm)))
			+ ((dword)BYTE_SWAP_WORD((file_header->total_data_size_pm)));


			if(download_block)
			{
			
			data = (char *)file_header;
			data += ((dword)(BYTE_SWAP_WORD(file_header->header_size))); 
			
				p_download_desc = &(p_download_table[table_index]);
				p_download_desc->download_id = file_header->download_id;
				p_download_desc->download_flags = file_header->download_flags;
				p_download_desc->required_processing_power = file_header->required_processing_power;
				p_download_desc->interface_channel_count = file_header->interface_channel_count;
				p_download_desc->excess_header_size = BYTE_SWAP_WORD((word)(BYTE_SWAP_WORD(file_header->header_size) - (word)sizeof(t_dsp_file_header)));
				p_download_desc->memory_block_count = file_header->memory_block_count;
				p_download_desc->segment_count = file_header->segment_count;
				p_download_desc->symbol_count = file_header->symbol_count;
				p_download_desc->data_block_count_dm = file_header->data_block_count_dm;
				p_download_desc->data_block_count_pm = file_header->data_block_count_pm;

				p_download_desc->p_excess_header_data = NULL;
				if ((BYTE_SWAP_WORD (p_download_desc->excess_header_size) != 0))
				{
#ifdef DEBUG
			printf("1.store_download called from get_download\n");
			
#endif
					addr = store_download(data, p_download_desc->excess_header_size,
											download_block);
					p_download_desc->p_excess_header_data = (byte *)addr;
					data += (BYTE_SWAP_WORD(p_download_desc->excess_header_size)); 
				}
				p_download_desc->p_download_description = NULL;
				if ((BYTE_SWAP_WORD(file_header->download_description_size) != 0))
				{
#ifdef DEBUG
			printf("2.store_download called from get_download\n");
#endif
					addr = store_download(data, file_header->download_description_size,
											download_block);
					p_download_desc->p_download_description = (char *)addr;
					data += (BYTE_SWAP_WORD(file_header->download_description_size)); 
				}
				p_download_desc->p_memory_block_table = NULL;
				if ((BYTE_SWAP_WORD(file_header->memory_block_table_size) != 0))
				{
#ifdef DEBUG
			printf("3.store_download called from get_download\n");
#endif
					addr = store_download(data, file_header->memory_block_table_size,
											download_block);
					p_download_desc->p_memory_block_table = (t_dsp_memory_block_desc *)addr;
					data += (BYTE_SWAP_WORD(file_header->memory_block_table_size)); 
				}
				p_download_desc->p_segment_table = NULL;
				if ((BYTE_SWAP_WORD(file_header->segment_table_size) != 0))
				{
#ifdef DEBUG
			printf("4.store_download called from get_download\n");
#endif
					addr = store_download(data, file_header->segment_table_size,
											download_block);
					p_download_desc->p_segment_table = (t_dsp_segment_desc *)addr;
					data += (BYTE_SWAP_WORD(file_header->segment_table_size)); 
				}
				p_download_desc->p_symbol_table = NULL;
				if ((BYTE_SWAP_WORD(file_header->symbol_table_size) != 0))
				{
#ifdef DEBUG
			printf("5.store_download called from get_download\n");
#endif
					addr = store_download(data, file_header->symbol_table_size,
											download_block);
					p_download_desc->p_symbol_table = (t_dsp_symbol_desc *)addr;
					data += (BYTE_SWAP_WORD(file_header->symbol_table_size)); 
				}
				p_download_desc->p_data_blocks_dm = NULL;
				if ((BYTE_SWAP_WORD(file_header->total_data_size_dm) != 0))
				{
#ifdef DEBUG
			printf("6.store_download called from get_download\n");
#endif
					addr = store_download(data, file_header->total_data_size_dm,
											download_block);
					p_download_desc->p_data_blocks_dm = (word *)addr;
					data += (BYTE_SWAP_WORD(file_header->total_data_size_dm)); 
				}
				p_download_desc->p_data_blocks_pm = NULL;
				if ((BYTE_SWAP_WORD(file_header->total_data_size_pm) != 0))
				{
#ifdef DEBUG
			printf("7.store_download called from get_download\n");
#endif
					addr = store_download(data, file_header->total_data_size_pm,
											download_block);
					p_download_desc->p_data_blocks_pm = (word *)addr;
					data += (BYTE_SWAP_WORD(file_header->total_data_size_pm)); 
				}

			}

		}

		download_area += ((dword)((word)(BYTE_SWAP_WORD(file_header->header_size)))); 
	 	download_area +=((dword)BYTE_SWAP_WORD((file_header->download_description_size)));
		download_area += ((dword)BYTE_SWAP_WORD((file_header->memory_block_table_size)));
		download_area += ((dword)BYTE_SWAP_WORD((file_header->segment_table_size)));
		download_area += ((dword)BYTE_SWAP_WORD((file_header->symbol_table_size)));
		download_area += ((dword)BYTE_SWAP_WORD((file_header->total_data_size_dm)));
		download_area += ((dword)BYTE_SWAP_WORD((file_header->total_data_size_pm)));


	}
	
	table_count=BYTE_SWAP_DWORD(no_of_downloads);
    /**no_of_tables=table_count;*/ 
    bzero(no_of_tables,sizeof(dword));
    memcpy(no_of_tables,&table_count,sizeof(dword));
	
#ifdef DEBUG
	printf("***0x%x bytes of memory required for %d downloads***\n", length, no_of_downloads);
    printf("BYTE_SWAP_DWORD table_count is:%d\n",table_count);
    printf("      \n");
#endif


	free(usage_mask);

        first_load = TRUE; 
       
	return length;

}


/*----------------------------------------------------------------------
 * store_download()
 *
 * Stores the size bytes of DSP code from data into dynamicaly
 * allocated memory block.
 * Arguments: pointer to data, length, pointer to begining of mem block
 * Returns the address to be put in download table
 *----------------------------------------------------------------------*/

dword store_download(char *data, word size, char *store)
{
	word real_size;
	static char* position;
	static char* initial;
	static dword addr;
	dword data_start_addr;
	dword align;

#ifdef DEBUG
	printf("Writing Data to memory block\n");
#endif

		
		if(first_load)
		{
		addr = download_pos;
		position = initial = (char *)store;
   
   		first_load = FALSE;
		}
  	
/*Starting address to where the data is written in the download_block*/    
    data_start_addr = addr;

	real_size = BYTE_SWAP_WORD(size);

	align = ((addr + (real_size + ~ALIGNMENT_MASK)) & ALIGNMENT_MASK) - 
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

/* We return (data_start_addr) because we want to point to start of data
   However for next write to download_block the starting postion addr is saved*/

	  return BYTE_SWAP_DWORD(data_start_addr);
	/*return BYTE_SWAP_DWORD(addr);*/
}

/*-------------------------------------------------------------------------
 * set_alignment_mask()
 *
 * Sets the alignment mask for the specified card type. This
 * allows us to allign the data in the download block correctly
 * Arguments: A int representing the card type
 * Returns TRUE if the card is supported, otherwise returns FALSE.
 *------------------------------------------------------------------------*/
int set_alignment_mask(int card_type)
{
	int ret;
	
	switch(card_type)
	{

		case CARDTYPE_MAESTRA_PCI:
        case CARDTYPE_DIVASRV_P_9M_PCI:
		case CARDTYPE_DIVASRV_Q_8M_PCI:
			ALIGNMENT_MASK = ALIGNMENT_MASK_MAESTRA;
			ret = TRUE;
			break;

		default:
			printf("Card not supported\n");
			ret = FALSE;
			break;
	}

	return ret;
}


/*-----------------------------------------------------------------------
 * set_download_position()
 * 
 * Sets the address to where the DSP code goes on the card
 * Arguments: Card number
 * Returns TRUE if card is supported
 *----------------------------------------------------------------------*/

int set_download_pos(int card_type, word wFeatures)
{
	int ret;
	
	switch(card_type)
	{

		case CARDTYPE_MAESTRA_PCI:
			if (wFeatures &  0x8)
			{
				download_pos = V90D_DSP_CODE_BASE;
			}
			else
			{
				download_pos = ORG_DSP_CODE_BASE;
			}

			download_pos += (((sizeof(dword) +
				sizeof(p_download_table)) + 3) 
				& 0xFFFFFFFC);
			ret = TRUE;
			break;

		case CARDTYPE_DIVASRV_P_9M_PCI:
		download_pos = MP_DSP_CODE_BASE	+ (((sizeof(dword) 
												+ sizeof(p_download_table)) 
												+ ~ALIGNMENT_MASK_MAESTRA) 
												& ALIGNMENT_MASK_MAESTRA);

            ret = TRUE;
            break;
												 

		case CARDTYPE_DIVASRV_Q_8M_PCI:
		if (wFeatures &  PROTCAP_V90D)
		{
			download_pos = MQ_V90D_DSP_CODE_BASE;
		}
		else
		{
			download_pos = MQ_ORG_DSP_CODE_BASE;
		}
		download_pos += (((sizeof(dword) +
			sizeof(p_download_table)) + 3) 
			& 0xFFFFFFFC);
		ret = TRUE;
		break;

		default:
			printf("Card not supported\n");
			ret = FALSE;
			break;
	}

	return ret;
}


/*-----------------------------------------------------------------------------
 * download()
 *
 * Does an IOCTL to download size bytes from block with code 
 * type set to code
 *
 * Arguments: Pointer to data, length of data , code type
 * Returns TRUE on success, other wise returns FALSE
 *----------------------------------------------------------------------------*/

int download(char *block, dword size, int code)
{
	dia_load_t load;
	int fd;

#ifdef DEBUG
	printf("Downloading to Card 0x%x bytes\n", size);
#endif

    load.card_id = card_id;

    /* open the Divas device */
	if ((fd = open(DIVAS_DEVICE_DFS, O_RDONLY, 0)) < 0)
	if ((fd = open(DIVAS_DEVICE, O_RDONLY, 0)) == -1)
	{
		perror("Error opening DIVA Server device");
		return(FALSE);
	}

	load.code_type = code;

	load.code = (unsigned char *)block;

	load.length = size;

    if((ioctl(fd, DIA_IOCTL_LOAD, &load)) == -1)
    {
		perror("IOCTL error on DIVA Servers");
        (void)close(fd);
        return FALSE;
    }

	(void)close(fd);

	return TRUE;
}
