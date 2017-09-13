
/*
 *
 * Copyright (C) Eicon Technology Corporation, 2000.
 *
 * This source file is supplied for the exclusive use with Eicon
 * Technology Corporation's range of DIVA Server Adapters.
 *
 * Eicon File Revision :    1.2  
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


#if !defined(_LINUX_H_)
#define _LINUX_H_
/* Macro for byte swaping */

#define BYTE_SWAP_WORD(a) a
#define BYTE_SWAP_DWORD(a) a

/* Globals */

int num_directory_entries = 0;
int usage_mask_size = 0;
int download_count = 0;
int directory_size = 0;
int ALIGNMENT_MASK;

int usage_bit = 0;
int usage_byte = 0;

unsigned int no_of_downloads = 0;
int total_bytes_in_download = 0;
dword  table_count=0;
char *no_of_tables;

t_dsp_download_desc p_download_table[DSP_MAX_DOWNLOAD_COUNT];

dword download_pos;

#endif /* _LINUX_H_ */
