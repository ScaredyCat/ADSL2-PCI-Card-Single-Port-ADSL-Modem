/* $Id: c20msg.c,v 1.2 1998/10/23 12:50:44 fritz Exp $
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
 * $Log: c20msg.c,v $
 * Revision 1.2  1998/10/23 12:50:44  fritz
 * Added RCS keywords and GPL notice.
 *
 */
char *Decode_Info (unsigned int Info) {
	switch (Info) {
		/* informative values (corresponding message was processed) */
		case 0x0001:
			return "NCPI not supported by current protocol, NCPI ignored";
		case 0x0002:
			return "Flags not supported by current protocol, flags ignored";
		case 0x0003:
			return "Alert already sent by another application";
			/* error information concerning CAPI_REGISTER */
		case 0x1001:
			return "Too many applications";
		case 0x1002:
			return "Logical block size to small, must be at least 128 Bytes";
		case 0x1003:
			return "Buffer exceeds 64 kByte";
		case 0x1004:
			return "Message buffer size too small, must be at least 1024 Bytes";
		case 0x1005:
			return "Max. number of logical connections not supported";
		case 0x1006:
			return "Reserved";
		case 0x1007:
			return "The message could not be accepted because of an internal busy condition";
		case 0x1008:
			return "OS resource error (no memory ?)";
		case 0x1009:
			return "CAPI not installed";
		case 0x100A:
			return "Controller does not support external equipment";
		case 0x100B:
			return "Controller does only support external equipment";
			/* error information concerning message exchange functions */
		case 0x1101:
			return "Illegal application number";
		case 0x1102:
			return "Illegal command or subcommand or message length less than 12 bytes";
		case 0x1103:
			return "The message could not be accepted because of a queue full condition !! The error code does not imply that CAPI cannot receive messages directed to another controller, PLCI or NCCI";
		case 0x1104:
			return "Queue is empty";
		case 0x1105:
			return "Queue overflow, a message was lost !! This indicates a configuration error. The only recovery from this error is to perform a CAPI_RELEASE";
		case 0x1106:
			return "Unknown notification parameter";
		case 0x1107:
			return "The Message could not be accepted because of an internal busy condition";
		case 0x1108:
			return "OS Resource error (no memory ?)";
		case 0x1109:
			return "CAPI not installed";
		case 0x110A:
			return "Controller does not support external equipment";
		case 0x110B:
			return "Controller does only support external equipment";
			/* error information concerning resource / coding problems */
		case 0x2001:
			return "Message not supported in current state";
		case 0x2002:
			return "Illegal Controller / PLCI / NCCI";
		case 0x2003:
			return "Out of PLCIs";
		case 0x2004:
			return "Out of NCCIs";
		case 0x2005:
			return "Out of LISTEN requests";
		case 0x2006:
			return "Out of FAX resources (protocol T.30)";
		case 0x2007:
			return "Illegal message parameter coding";
			/* error information concerning requested services */
		case 0x3001:
			return "B1 protocol not supported";
		case 0x3002:
			return "B2 protocol not supported";
		case 0x3003:
			return "B3 protocol not supported";
		case 0x3004:
			return "B1 protocol parameter not supported";
		case 0x3005:
			return "B2 protocol parameter not supported";
		case 0x3006:
			return "B3 protocol parameter not supported";
		case 0x3007:
			return "B protocol combination not supported";
		case 0x3008:
			return "NCPI not supported";
		case 0x3009:
			return "CIP Value unknown";
		case 0x300A:
			return "Flags not supported (reserved bits)";
		case 0x300B:
			return "Facility not supported";
		case 0x300C:
			return "Data length not supported by current protocol";
		case 0x300D:
			return "Reset procedure not supported by current protocol";
			/* informations about the clearing of a physical connection */
		case 0x3301:
			return "Protocol error layer 1 (broken line or B-channel removed by signalling protocol)";
		case 0x3302:
			return "Protocol error layer 2";
		case 0x3303:
			return "Protocol error layer 3";
		case 0x3304:
			return "Another application got that call";
			/* T.30 specific reasons */
		case 0x3311:
			return "Connecting not successful (remote station is no FAX G3 machine)";
		case 0x3312:
			return "Connecting not successful (training error)";
		case 0x3313:
			return "Disconnected before transfer (remote station does not support transfer mode, e.g. resolution)";
		case 0x3314:
			return "Disconnected during transfer (remote abort)";
		case 0x3315:
			return "Disconnected during transfer (remote procedure error, e.g. unsuccessful repetition of T.30 commands)";
		case 0x3316:
			return "Disconnected during transfer (local tx data underrun)";
		case 0x3317:
			return "Disconnected during transfer (local rx data overflow)";
		case 0x3318:
			return "Disconnected during transfer (local abort)";
		case 0x3319:
			return "Illegal parameter coding (e.g. SFF coding error)";
			/* disconnect causes from the network according to ETS 300 102-1/Q.931 */
		case 0x3481:
			return "Unallocated (unassigned) number";
		case 0x3482:
			return "No route to specified transit network";
		case 0x3483:
			return "No route to destination";
		case 0x3486:
			return "Channel unacceptable";
		case 0x3487:
			return "Call awarded and being delivered in an established channel";
		case 0x3490:
			return "Normal call clearing";
		case 0x3491:
			return "User busy";
		case 0x3492:
			return "No user responding";
		case 0x3493:
			return "No answer from user (user alerted)";
		case 0x3495:
			return "Call rejected";
		case 0x3496:
			return "Number changed";
		case 0x349A:
			return "Non-selected user clearing";
		case 0x349B:
			return "Destination out of order";
		case 0x349C:
			return "Invalid number format";
		case 0x349D:
			return "Facility rejected";
		case 0x349E:
			return "Response to STATUS ENQUIRY";
		case 0x349F:
			return "Normal, unspecified";
		case 0x34A2:
			return "No circuit / channel available";
		case 0x34A6:
			return "Network out of order";
		case 0x34A9:
			return "Temporary failure";
		case 0x34AA:
			return "Switching equipment congestion";
		case 0x34AB:
			return "Access information discarded";
		case 0x34AC:
			return "Requested circuit / channel not available";
		case 0x34AF:
			return "Resources unavailable, unspecified";
		case 0x34B1:
			return "Quality of service unavailable";
		case 0x34B2:
			return "Requested facility not subscribed";
		case 0x34B9:
			return "Bearer capability not authorized";
		case 0x34BA:
			return "Bearer capability not presently available";
		case 0x34BF:
			return "Service or option not available, unspecified";
		case 0x34C1:
			return "Bearer capability not implemented";
		case 0x34C2:
			return "Channel type not implemented";
		case 0x34C5:
			return "Requested facility not implemented";
		case 0x34C6:
			return "Only restricted digital information bearer capability is available";
		case 0x34CF:
			return "Service or option not implemented, unspecified";
		case 0x34D1:
			return "Invalid call reference value";
		case 0x34D2:
			return "Identified channel does not exist";
		case 0x34D3:
			return "A suspended call exists, but this call identity does not";
		case 0x34D4:
			return "Call identity in use";
		case 0x34D5:
			return "No call suspended";
		case 0x34D6:
			return "Call having the requested call identity has been cleared";
		case 0x34D8:
			return "Incompatible destination";
		case 0x34DB:
			return "Invalid transit network selection";
		case 0x34DF:
			return "Invalid message, unspecified";
		case 0x34E0:
			return "Mandatory information element is missing";
		case 0x34E1:
			return "Message type non-existent or not implemented";
		case 0x34E2:
			return "Message not compatible with call state or message type non-existent or not implemented";
		case 0x34E3:
			return "Information element non-existent or not implemented";
		case 0x34E4:
			return "Invalid information element contents";
		case 0x34E5:
			return "Message not compatible with call state";
		case 0x34E6:
			return "Recovery on timer expiry";
		case 0x34EF:
			return "Protocol error, unspecified";
		case 0x34FF:
			return "Interworking, unspecified";
		default:
			return "No additional information";
	}
}

/*
 * Decode_Command: Returns a textstring with the CAPI-commandname
 */
char *Decode_Command (unsigned char Command) {
	switch (Command) {
		case 0x01:
			return "ALERT";
		case 0x02:
			return "CONNECT";
		case 0x03:
			return "CONNECT_ACTIVE";
		case 0x04:
			return "DISCONNECT";
		case 0x05:
			return "LISTEN";
		case 0x08:
			return "INFO";
		case 0x41:
			return "SELECT_B_PROTOCOL";
		case 0x80:
			return "FACILITY";
		case 0x82:
			return "CONNECT_B3";
		case 0x83:
			return "CONNECT_B3_ACTIVE";
		case 0x84:
			return "DISCONNECT_B3";
		case 0x86:
			return "DATA_B3";
		case 0x87:
			return "RESET_B3";
		case 0x88:
			return "CONNECT_B3_T90_ACTIVE";
		case 0xff:
			return "MANUFACTURER";
	}
	return "Error: Command undefined in function Decode_Command";
}

/*
 * Decode_Sub: Returns a textstring with the CAPI-subcommandname
 */
char *Decode_Sub (unsigned char Sub) {
	switch (Sub) {
		case 0x80:
			/* Request */
			return "REQ";
		case 0x81:
			/* Confirmation */
			return "CONF";
		case 0x82:
			/* Indication */
			return "IND";
		case 0x83:
			/* Response */
			return "RESP";
	}
	return "Error: Subcommand undefined in function Decode_Sub";
}
