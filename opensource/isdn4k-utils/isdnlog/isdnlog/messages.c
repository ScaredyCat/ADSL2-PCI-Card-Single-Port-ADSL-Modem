/* $Id: messages.c,v 1.3 1998/11/24 20:51:41 akool Exp $
 *
 * ISDN accounting for isdn4linux. (Q.931-Messages)
 *
 * Copyright 1995, 1998 by Andreas Kool (akool@isdn4linux.de)
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
 */

#define _MESSAGES_C_

#include "isdnlog.h"


static char *MessageType[] = { /* According to Table 4-2/Q.931 */
  "\x01 ALERTING",
  "\x02 CALL PROCEEDING",
  "\x03 PROGRESS",
  "\x05 SETUP",
  "\x07 CONNECT",
  "\x0d SETUP ACKNOWLEDGE",
  "\x0f CONNECT ACKNOWLEDGE",
  "\x20 USER INFORMATION",
  "\x21 SUSPEND REJECT",
  "\x22 RESUME REJECT",
  "\x24 HOLD",
  "\x25 SUSPEND",
  "\x26 RESUME",
  "\x28 HOLD ACKNOWLEDGE",     /* Makeln Acknowledge */
  "\x2d SUSPEND ACKNOWLEDGE",
  "\x2e RESUME ACKNOWLEDGE",
  "\x30	HOLD REJECT",
  "\x31 RETRIEVE",
  "\x33 RETRIEVE ACKNOWLEDGE", /* Makeln Eesume Acknowledge */
  "\x37 RETRIEVE REJECT",
  "\x45 DISCONNECT",
  "\x46 RESTART",
  "\x4d RELEASE",
  "\x4e RESTART ACKNOWLEDGE",
  "\x5a RELEASE COMPLETE",
  "\x60 SEGMENT",
  "\x62 FACILITY",
  "\x64 REGISTER",
  "\x6e NOTIFY",
  "\x75 STATUS ENQUIRY",
  "\x79 CONGESTION CONTROL",
  "\x7b INFORMATION",
  "\x7d STATUS",
  NULL };

static char *MessageType1TR6[] = {
  "\x61 REGister INDication",
  "\x62 CANCel INDication",
  "\x63 FACility STAtus",
  "\x64 STAtus ACKnowledge",
  "\x65 STAtus REJect",
  "\x66 FACility INFormation",
  "\x67 INFormation ACKnowledge",
  "\x68 INFormation REJect",
  "\x75 CLOSE",
  "\x77 CLOse ACKnowledge",

  "\x00 ESCape",
  "\x01 ALERT",
  "\x02 CALL SENT",
  "\x07 CONNect",
  "\x0f CONNect ACKnowledge",
  "\x05 SETUP",
  "\x0d SETUP ACKnowledge",
  "\x26 RESume",
  "\x2e RESume ACKnowledge",
  "\x22 RESume REJect",
  "\x25 SUSPend",
  "\x2d SUSPend ACKnowledge",
  "\x21 SUSPend REJect",
  "\x20 USER INFO",
  "\x40 DETach",
  "\x45 DISConnect",
  "\x4d RELease",
  "\x5a RELease ACKnowledge",
  "\x6e CANCel ACKnowledge",
  "\x67 CANCel REJect",
  "\x69 CONgestion CONtrol",
  "\x60 FACility",
  "\x68 FACility ACKnowledge",
  "\x66 FACility CANcel",
  "\x64 FACility REGister",
  "\x65 FACility REJect",
  "\x6d INFOrmation",
  "\x6c REGister ACKnowledge",
  "\x6f REGister REJect",
  "\x63 STATus",
  NULL };


static char *InformationElement[] = {
  "\x00 Segmented message",
  "\x04 bearer service indication",                /* Bearer capability */
  "\x08 Cause",
  "\x0c Connected address (obsolete)",
  "\x0d Extended facility information element identifier",
  "\x10 Call identity",
  "\x14 Call state",
  "\x18 Channel identification",
  "\x19 Data link connection identifier",
  "\x1c Facility information element identifier",  /* Facility */
  "\x1e Progress indicator",
  "\x20 Network-specific facilities",
  "\x24 Terminal capabilities (obsolete)",
  "\x27 Notification indicator",
  "\x28 Display",
  "\x29 Date/Time",
  "\x2c Keypad facility",
  "\x34 Signal",
  "\x40 Information rate",
  "\x42 End-to-end transit delay",
  "\x43 Transit delay selection and indication",
  "\x44 Packet layer binary parameters",
  "\x45 Packet layer window size",
  "\x46 Packet size",
  "\x47 Closed user group",
  "\x4a Reverse charge indication",
  "\x4c COLP",
  "\x6c Calling party number",
  "\x6d Calling party subaddress",
  "\x70 Called party number",
  "\x71 Called party subaddress",
  "\x74 Redirecting number",
  "\x76 Redirection number",
  "\x78 Transit network selection",
  "\x79 Restart indicator",
  "\x7c Low layer compatibility",
  "\x7d High layer compatibility",
  "\x7e User-user",
  "\x7f Escape for extension",
  NULL };

static char *InformationElement1TR6[] = {
  "\x08 Cause",
  "\x0c Connecting Address",
  "\x10 Call IDentity",
  "\x18 Channel IDentity",
  "\x20 Network Specific Facility",
  "\x28 Display",
  "\x2c Keypad",
  "\x6c Origination Address",
  "\x70 Destination Address",
  "\x7e User Info",

  "\x01 Service Indicator",
  "\x02 Charging Information",
  "\x03 Date",
  "\x05 Facility Select",
  "\x06 Facility Status",
  "\x07 Status Called",
  "\x08 Additional Transmission Attributes",
  NULL };

static char *CauseValue[] = { /* According to Q.850 */
  "\x01 Unallocated (unassigned) number",
  "\x02 No route to specified transit network",
  "\x03 No route to destination",
  "\x04 Send special information tone",
  "\x05 Misdialled trunk prefix",
  "\x06 Channel unacceptable",
  "\x07 Channel awarded and being delivered in an established channel",
  "\x08 Preemption",
  "\x09 Preemption - circuit reserved for reuse",
  "\x10 Normal call clearing",
  "\x11 User busy",
  "\x12 No user responding",
  "\x13 No answer from user (user alerted)",
  "\x14 Subscriber absent",
  "\x15 Call rejected",
  "\x16 Number changed",
  "\x1a non-selected user clearing",
  "\x1b Destination out of order",
  "\x1c Invalid number format (address incomplete)",
  "\x1d Facility rejected",
  "\x1e Response to Status enquiry",
  "\x1f Normal, unspecified",
  "\x22 No circuit/channel available",
  "\x26 Network out of order",
  "\x27 Permanent frame mode connection out-of-service",
  "\x28 Permanent frame mode connection operational",
  "\x29 Temporary failure",
  "\x2a Switching equipment congestion",
  "\x2b Access information discarded",
  "\x2c Requested circuit/channel not available",
  "\x2e Precedence call blocked",
  "\x2f Resource unavailable, unspecified",
  "\x31 Quality of service unavailable",
  "\x32 Requested facility not subscribed",
  "\x35 Outgoing calls barred within CUG",
  "\x37 Incoming calls barred within CUG",
  "\x39 Bearer capability not authorized",
  "\x3a Bearer capability not presently available",
  "\x3e Inconsistency in designated outgoing access information and subscriber class",
  "\x3f Service or option not available, unspecified",
  "\x41 Bearer capability not implemented",
  "\x42 Channel type not implemented",
  "\x43 Requested facility not implemented",
  "\x44 Only restricted digital information bearer capability is available",
  "\x45 angeforderte Eigenschaft nicht implementiert",
  "\x46 Nur beschraenkte digitale Information ueber Uebermittlungsfunktion ist verfuegbar",
  "\x4f Service or option not implemented",
  "\x51 Invalid call reference value",
  "\x52 Identified channel does not exist",
  "\x53 A suspended call exists, but this call identity does not",
  "\x54 Call identity in use",
  "\x55 No call suspended",
  "\x56 Call having the requested call identity has been cleared",
  "\x57 User not member of CUG",
  "\x58 Incompatible destination",
  "\x5a Non-existent CUG",
  "\x5b Invalid transit network selection",
  "\x5f Invalid message, unspecified",
  "\x60 Mandatory information element is missing",
  "\x61 Message type non-existent or not implemented",
  "\x62 Message not compatible with call state or message type non-existent or not implemented",
  "\x63 Information element/parameter non-existent or not implemented",
  "\x64 Invalid information element contents",
  "\x65 Message not compatible with call state",
  "\x66 Recovery on timer expiry",
  "\x67 Parameter non-existent or not implemented - passed on",
  "\x6e Message with unrecognized parameter discarded",
  "\x6f Protocol error, unspecified",
  "\x7f Interworking, unspecified",
  NULL };

static char *CauseValue1TR6[] = {
  "\x00 Normal Call Clearing",
  "\x01 Invalid Call Reference",
  "\x03 Bearer Service Not Implemented",
  "\x07 Caller Identity unknown",
  "\x08 Caller Identity in Use",
  "\x09 No Channels available",
  "\x0a No Channels available",
  "\x10 Facility Not Implemented",
  "\x11 Facility Not Subscribed",
  "\x20 Outgoing calls barred",
  "\x21 User Access Busy",
  "\x22 Negative GBG",
  "\x23 Unknown GBG",
  "\x25 No SPV known",
  "\x35 Destination not obtainable",
  "\x38 Number changed",
  "\x39 Out Of Order",
  "\x3a No User Response",
  "\x3b User Busy",
  "\x3d Incoming Barred",
  "\x3e Call Rejected",
  "\x58 Invalid destination address",
  "\x59 Network Congestion",
  "\x5a Remote User initiated",
  "\x70 Local Procedure Error",
  "\x71 Remote Procedure Error",
  "\x72 Remote User Suspend",
  "\x73 Remote User Resumed",
  "\x7f User Info Discarded",
  NULL };


static char *Service1TR6[] = {
  "\x01 Fernsprechen",
  "\x02 Telefax Gruppe 2/3, Modem-DFUE",
  "\x03 X.21-Terminaladapter",
  "\x04 Telefax Gruppe 4 (digital)",
  "\x05 Bildschirmtext (64 kBit/s)",
  "\x07 Datenuebertragung (64 kBit/s oder V.110)",
  "\x08 X.25-Terminaladapter",
  "\x09 Teletex (64 kBit/s)",
  "\x0a Text-Fax (ASCII-Text und Grafiken gemischt)",
  "\x0d Fernwirken, Telemetrie, Alarmierung",
  "\x0e Fernzeichnen",
  "\x0f Bildschirmtext (zukuenftiger Standard)",
  "\x10 Bildtelefon",
  NULL };


char *qmsg(int type, int version, int val)
{
  register char **p, *what;
  static   char  *nix = "";


  switch (type) {
    case TYPE_MESSAGE : p = (version == VERSION_1TR6) ? MessageType1TR6 : MessageType;
    	 	      	what = "Message type";
    	 	      	break;

    case TYPE_ELEMENT : p = (version == VERSION_1TR6) ? InformationElement1TR6 : InformationElement;
    	 	      	what = "Information Element";
    	 	      	break;

    case TYPE_CAUSE   : if (val == -1)
    			  return(nix);

    	 	      	p = (version == VERSION_1TR6) ?  CauseValue1TR6 : CauseValue;
    	 	      	what = "Cause";
    	 	      	break;

    case TYPE_SERVICE : p = Service1TR6;
    	 	      	what = "Service";
                        break;

              default : p = (char **)NULL;
              	      	what = "";
                        break;
  } /* switch */

  while (*p && **p != val)
    p++;

  if (*p && (**p == val))
    return(*p + 2);
  else {
    if (++retnum == MAXRET)
      retnum = 0;

    sprintf(retstr[retnum], "UNKNOWN %s 0x%02x", what, val);
    return(retstr[retnum]);
  } /* else */
} /* qmsg */
