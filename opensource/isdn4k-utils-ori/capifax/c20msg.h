/* $Id: c20msg.h,v 1.2 1998/10/23 12:50:45 fritz Exp $
 *
 * This stuff is based heavily on AVM's CAPI-adk for linux.
 *
 * This program is free software; you can redistribute it and/or modify          * it under the terms of the GNU General Public License as published by          * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *                                                                               * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Decode_Info: Returns a string with an error description
 * Note: infos with values of 0x00xx are only warnings and the corresponding
 * messages have been processed.
 * The description for all info values but 0x34xx is taken from the CAPI 2.0
 * specification february 1994.
 * The description for the 0x34xx values is taken from ETS 300 102-1/Q.931
 *
 * $Log: c20msg.h,v $
 * Revision 1.2  1998/10/23 12:50:45  fritz
 * Added RCS keywords and GPL notice.
 *
 */
#ifndef _c20msg_h_
#define _c20msg_h_

char *Decode_Info(unsigned int Info);

/*
 * Decode_Command: Returns a textstring with the CAPI-commandname
 */
char *Decode_Command(unsigned char Command);

/*
 * Decode_Sub: Returns a textstring with the CAPI-subcommandname
 */
char *Decode_Sub (unsigned char Sub);

#endif /* _c20msg_h_ */
