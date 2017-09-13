
/*
 *
 * Copyright (C) Eicon Technology Corporation, 2000.
 *
 * This source file is supplied for the exclusive use with Eicon
 * Technology Corporation's range of DIVA Server Adapters.
 *
 * Eicon File Revision :    1.1  
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
 * Data Structure for down loading configuration details to the protocol
 * driver controlling the Diehl Diva card.
 *
 */
#if !defined(_LOAD_H)
#define _LOAD_H

#define MAX_ADDR           23
#define MAX_SPID           32
#define ETDP_TEI           0x01
#define ETDP_NT2           0x02
#define ETDP_PERMANENT     0x04
#define ETDP_LAYER2        0x08
#define ETDP_NOORDERCHECK  0x10
#define ETDP_PORT1         0x20
#define ETDP_PORT2         0x40


typedef struct 
{
    word configured;

    byte tei;
    byte nt2;
    byte permanent;
    byte stablel2;
    byte noordercheck;
    byte oad[2][MAX_ADDR];
    byte osa[2][MAX_ADDR];
    byte spid[2][MAX_SPID];

}parameter_t;


#endif /*_LOAD_H*/
