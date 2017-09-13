/*
** $Id: libvboxmodem.c,v 1.5 1998/09/18 15:08:58 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termio.h>

#include "libvboxmodem.h"

/** Prototypes ***********************************************************/

static void vboxmodem_sane_mode(TIO *, int);
static void vboxmodem_raw_mode(TIO *);
static void vboxmodem_speed(TIO *);
static void vboxmodem_flowcontrol(TIO *);
static void	vboxmodem_check_nocarrier(struct vboxmodem *, unsigned char);

/** Variables ************************************************************/

static unsigned char lastmodemerrmsg[255 + 1];

/*************************************************************************/
/** vboxmodem_open():	This function initialize the modem structure,	**/
/**							opens the device and does some needed setup.		**/
/*************************************************************************/
/** => vbm					Pointer to the modem structure						**/
/** => devicename			Name of the device to open								**/
/** <=						0 on success or -1 on error							**/
/*************************************************************************/

int vboxmodem_open(struct vboxmodem *vbm, unsigned char *devicename)
{
	TIO devicetio;

	vbm->fd					= -1;
	vbm->devicename		= strdup(devicename);
	vbm->input				= malloc(VBOXMODEM_BUFFER_SIZE + 1);
	vbm->inputpos			= 0;
	vbm->inputlen			= 0;	
	vbm->nocarrier			= 0;
	vbm->nocarrierpos		= 0;
	vbm->nocarriertxt		= strdup("NO CARRIER");

	if ((!vbm->devicename) || (!vbm->input) || (!vbm->nocarriertxt))
	{
		set_modem_error("Not enough memory to allocate modem structure");
		vboxmodem_close(vbm);

		return(-1);
	}

		/* Open, setup and initialize the modem device. Also set	*/
		/* speed and handshake.												*/

	if ((vbm->fd = open(vbm->devicename, O_RDWR|O_NDELAY)) == -1)
	{
		set_modem_error("Can't open modem device");
		vboxmodem_close(vbm);

		return(-1);
	}

	if (fcntl(vbm->fd, F_SETFL, O_RDWR) == -1)
	{
		set_modem_error("Can't setup modem device");
		vboxmodem_close(vbm);

		return(-1);
	}

	vboxmodem_sane_mode(&devicetio, 1);
	vboxmodem_speed(&devicetio);
	vboxmodem_raw_mode(&devicetio);
	vboxmodem_flowcontrol(&devicetio);

	if (vboxmodem_set_termio(vbm, &devicetio) == -1)
	{
		set_modem_error("Can't setup modem device settings");
		vboxmodem_close(vbm);
		
		return(-1);
	}

	strcpy(lastmodemerrmsg, "");
	return(0);
}

/*************************************************************************/
/** vboxmodem_close():	Frees all resources from vboxmodem_open().		**/
/*************************************************************************/
/** => vbm					Pointer to the initialized modem structure		**/
/*************************************************************************/

int vboxmodem_close(struct vboxmodem *vbm)
{
	if (vbm->devicename	) free(vbm->devicename);
	if (vbm->input			) free(vbm->input);
	if (vbm->nocarriertxt) free(vbm->nocarriertxt);

	vbm->devicename	= NULL;
	vbm->input			= NULL;
	vbm->nocarriertxt	= NULL;

	if (vbm->fd != -1)
	{
		if (close(vbm->fd) == -1)
		{
			set_modem_error("Can't close modem device!");
			
			return(-1);
		}
		else vbm->fd = -1;
	}

	return(0);
}

/*************************************************************************/
/** vboxmodem_error():	Returns a pointer to the last error message.		**/
/*************************************************************************/
/** <=						Pointer to the message									**/
/*************************************************************************/

unsigned char *vboxmodem_error(void)
{
	return(&lastmodemerrmsg[0]);
}

/*************************************************************************/
/** vboxmodem_raw_read():	Reads raw bytes from modem.						**/
/*************************************************************************/
/** => vbm						Pointer to the modem structure					**/
/** => line						Pointer to the read buffer							**/
/** => len						Length of the read buffer							**/
/** <=							Number of bytes in the read buffer				**/
/*************************************************************************/

int vboxmodem_raw_read(struct vboxmodem *vbm, unsigned char *line, int len)
{
	int use = 0;
	int pos = 0;

	if (len > VBOXMODEM_BUFFER_SIZE) len = VBOXMODEM_BUFFER_SIZE;

	if (vbm->inputlen >= len)
	{
		memcpy(line, &vbm->input[vbm->inputpos], len);

		vbm->inputlen -= len;
		vbm->inputpos += len;

		return(len);
	}

	if (vbm->inputlen > 0)
	{
		memcpy(line, &vbm->input[vbm->inputpos], vbm->inputlen);
	}
	
	len -= vbm->inputlen;
	use += vbm->inputlen;
	
	vbm->inputpos = 0;
	vbm->inputlen = 0;

	if ((vbm->inputlen = read(vbm->fd, vbm->input, VBOXMODEM_BUFFER_SIZE)) < 0)
	{
		vbm->inputpos = 0;
		vbm->inputlen = 0;

		return(use);
	}

	for (pos = 0; pos < vbm->inputlen; pos++)
	{
		vboxmodem_check_nocarrier(vbm, vbm->input[pos]);
	}

	if (vbm->inputlen < len) len = vbm->inputlen;

	memcpy(&line[use], &vbm->input[vbm->inputpos], len);

	vbm->inputlen -= len;
	vbm->inputpos += len;

	return(use + len);
}

/*************************************************************************/
/** vboxmodem_raw_write():	Sends a buffer to the modem.						**/
/*************************************************************************/
/** => vmb						Pointer to modem structure							**/
/** => output					Pointer to datas										**/
/** => len						Number of datas to write							**/
/*************************************************************************/

size_t vboxmodem_raw_write(struct vboxmodem *vbm, unsigned char *output, int len)
{
	return(write(vbm->fd, output, len));
}

/*************************************************************************/
/** vboxmodem_set_termio():	Sets the terminal io settings.				**/
/*************************************************************************/
/** => vbm							Pointer to modem structure						**/
/** => modemtio					Pointer to a terminal io structure			**/
/*************************************************************************/

int vboxmodem_set_termio(struct vboxmodem *vbm, TIO *modemtio)
{
	if (tcsetattr(vbm->fd, TCSANOW, modemtio) >= 0) return(0);

	return(-1);
}

/*************************************************************************/
/** vboxmodem_get_termio():	Gets the terminal io settings.				**/
/*************************************************************************/
/** => vbm							Pointer to modem structure						**/
/** => modemtio					Pointer to a terminal io structure			**/
/*************************************************************************/

int vboxmodem_get_termio(struct vboxmodem *vbm, TIO *modemtio)
{
	if (tcgetattr(vbm->fd, modemtio) >= 0) return(0);
	
	return(-1);
}

/*************************************************************************/
/** vboxmodem_sane_mode():	Sets sane mode.										**/
/*************************************************************************/
/** => modemtio				Pointer to a terminal io structure				**/
/*************************************************************************/

static void	vboxmodem_sane_mode(TIO *modemtio, int local)
{
	modemtio->c_iflag  =  (BRKINT|IGNPAR|IXON|IXANY);
	modemtio->c_oflag  =  (OPOST|TAB3);
	modemtio->c_cflag &= ~(CSIZE|CSTOPB|PARENB|PARODD|CLOCAL);
	modemtio->c_cflag |=  (CS8|CREAD|HUPCL|(local ? CLOCAL : 0));
	modemtio->c_lflag  =  (ECHOK|ECHOE|ECHO|ISIG|ICANON);
}

/*************************************************************************/
/** vboxmodem_raw_mode():	Sets raw mode.											**/
/*************************************************************************/
/** => modemtio				Pointer to a terminal io structure				**/
/*************************************************************************/

static void vboxmodem_raw_mode(TIO *modemtio)
{
	modemtio->c_iflag		 &= (IXON|IXOFF|IXANY);
	modemtio->c_oflag		  = 0;
	modemtio->c_lflag		  = 0;
	modemtio->c_cc[VMIN]   = 1;
	modemtio->c_cc[VTIME]  = 0;
}

/*************************************************************************/
/** vboxmodem_speed():	Sets speed to 38400.										**/
/*************************************************************************/
/** => modemtio			Pointer to a terminal io structure					**/
/*************************************************************************/

static void vboxmodem_speed(TIO *modemtio)
{
	cfsetospeed(modemtio, B38400);
	cfsetispeed(modemtio, B38400);
}

/*************************************************************************/
/** vboxmodem_flowcontrol():	Sets flow control to hardware.				**/
/*************************************************************************/
/** => modemtio					Pointer to a terminal io structure			**/
/*************************************************************************/

static void vboxmodem_flowcontrol(TIO *modemtio)
{
	modemtio->c_cflag &= ~(CRTSCTS);
	modemtio->c_iflag &= ~(IXON|IXOFF|IXANY);
	modemtio->c_cflag |=  (CRTSCTS);
}

/*************************************************************************/
/** vboxmodem_check_nocarrier(): Checks for carrier lost.					**/
/*************************************************************************/
/** =>									Pointer to the modem structure			**/
/** => c									Byte from modem input						**/
/*************************************************************************/

static void vboxmodem_check_nocarrier(struct vboxmodem *vbm, unsigned char c)
{
	if (c == vbm->nocarriertxt[vbm->nocarrierpos])
	{
		vbm->nocarrierpos++;

		if (vbm->nocarrierpos >= strlen(vbm->nocarriertxt))
		{
			vbm->nocarrier		= 1;
			vbm->nocarrierpos	= 0;
		}
	}
	else
	{
		vbm->nocarrierpos = 0;
		
		if (c == vbm->nocarriertxt[0]) vbm->nocarrierpos++;
	}
}
