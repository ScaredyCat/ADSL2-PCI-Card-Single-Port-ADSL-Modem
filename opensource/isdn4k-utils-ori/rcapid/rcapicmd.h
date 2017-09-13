/*
 * ISDN4Linux Remote-CAPI Server
 *
 * Copyright 1998 by Christian A. Lademann (cal@zls.de)
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
 */

#ifndef __RCAPICMD_H__
#define __RCAPICMD_H__

/* REMOTE-CAPI commands */

#define RCAPI_REGISTER_REQ			CAPICMD(0xf2, 0xff)
#define RCAPI_REGISTER_CONF			CAPICMD(0xf3, 0xff)
#define RCAPI_GET_MANUFACTURER_REQ		CAPICMD(0xfa, 0xff)
#define RCAPI_GET_MANUFACTURER_CONF		CAPICMD(0xfb, 0xff)
#define RCAPI_GET_SERIAL_NUMBER_REQ		CAPICMD(0xfe, 0xff)
#define RCAPI_GET_SERIAL_NUMBER_CONF	        CAPICMD(0xff, 0xff)
#define RCAPI_GET_VERSION_REQ			CAPICMD(0xfc, 0xff)
#define RCAPI_GET_VERSION_CONF			CAPICMD(0xfd, 0xff)
#define RCAPI_GET_PROFILE_REQ			CAPICMD(0xe0, 0xff)
#define RCAPI_GET_PROFILE_CONF			CAPICMD(0xe1, 0xff)
#define RCAPI_AUTH_USER_REQ                     CAPICMD(0xff, 0x00)
#define RCAPI_AUTH_USER_CONF                    CAPICMD(0xff, 0x01)

#endif /* __RCAPICMD_H__ */
