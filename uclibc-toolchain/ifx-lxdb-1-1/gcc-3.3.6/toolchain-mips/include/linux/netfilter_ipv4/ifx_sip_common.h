/* ============================================================================
 * Copyright (C) 2003[- 2004]  Infineon Technologies AG.
 *
 * All rights reserved.
 * ============================================================================
 *
 * ============================================================================
 *
 * This document contains proprietary information belonging to Infineon 
 * Technologies AG. Passing on and copying of this document, and communication
 * of its contents is not permitted without prior written authorisation.
 * 
 * ============================================================================
 */


/* ===========================================================================
 *
 * File Name: (From version control system)
 * Author : Atanu Mondal
 * Date: 
 *
 * ===========================================================================
 *
 * Project: <project/component name>
 * Block: <block/module name>
 *
 * ===========================================================================
 * Contents: This file contains the common definitions required by the
 * 	     conntrack and the nat helper files.
 * 
 * ===========================================================================
 * References: <List of design documents covering this file.>
 */



#ifndef __IFX_SIP_COMMON_H__
#define __IFX_SIP_COMMON_H__

#include <linux/netfilter_ipv4/ip_conntrack_sip.h>
#include <linux/ioctl.h>

#if 1
struct sip_params
{
	int sip_port;
	enum SipControlProtocol proto;
};
#endif



/* The major device number. We can't rely on dynamic 
 * registration any more, because ioctls need to know 
 * it. */
#define MAJOR_NUM 				233
#define SIP_CONN_MAJOR_NUM 			234


/* Set the message of the device driver */
#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, char *)
/* _IOR means that we're creating an ioctl command 
 * number for passing information from a user process
 * to the kernel module. 
 *
 * The first arguments, MAJOR_NUM, is the major device 
 * number we're using.
 *
 * The second argument is the number of the command 
 * (there could be several with different meanings).
 *
 * The third argument is the type we want to get from 
 * the process to the kernel.
 */

/* Deregister the port and the protocol */
#define IOCTL_DEREGISTER_PORT _IOWR(MAJOR_NUM, 3, struct sip_params*)
#define IOCTL_DEREGISTER_CONN_PORT _IOWR(SIP_CONN_MAJOR_NUM,3, struct sip_params*)

 /* This IOCTL is used for output, to get the message 
  * of the device driver. However, we still need the 
  * buffer to place the message in to be input, 
  * as it is allocated by the process.
  */


/* Register the port and protocol*/
#define IOCTL_REGISTER_PORT _IOWR(MAJOR_NUM, 2, struct sip_params*)
#define IOCTL_REGISTER_CONN_PORT _IOWR(SIP_CONN_MAJOR_NUM, 2, struct sip_params*)
 /* The IOCTL is used for both input and output. It 
  * receives from the user a number, n, and returns 
  * Message[n]. */


/* The name of the device file */
#define DEVICE_FILE_NAME "sip_dev"
#define DEVICE_CONN_FILE_NAME "sip_conn_dev"
#define DEVICE_CONN_NAME "sip_conn_dev"

messagetokentype messagetoken[]=
{
	{CALLID,{"Call-ID:","i:","****"}},			//0
	{CONTACT,{"Contact:", "m:", "****"}},			//1
	{FROM,{"From:", "f:","****"}},				//2
	{CONTENT_LENGTH,{"Content-Length:", "l:","****"}},	//3
	{CONTENT_TYPE,{"Content-Type:","c:","****"}},		//4
	{SUBJECT,{"Subject:","s:","****"}},			//5
	{TO,{"To:","t:","****"}},				//6
	{VIA,{"Via:","v:","****"}},				//7
	{RECORD_ROUTE,{"Record-Route:","****"}},		//8
	{ROUTE,{"Route:","****"}},				//9
	{REQUEST_URI,{"INVITE","ACK","OPTIONS","BYE","CANCEL","REGISTER","****"}},
	{ENDOFTOKEN,{"****"}}
};




#endif /* __IFX_SIP_COMMON_H__ */
