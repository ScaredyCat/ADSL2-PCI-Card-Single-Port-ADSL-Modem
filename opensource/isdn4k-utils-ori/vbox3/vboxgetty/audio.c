/*
** $Id: audio.c,v 1.5 1998/11/10 18:36:25 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "audio.h"
#include "log.h"
#include "voice.h"

/************************************************************************* 
 ** audio_open_dev():	�ffnet das Audiodevice und stellt es ein.			**
 *************************************************************************
 ** => name					Name des Devices.											**
 *************************************************************************/

int audio_open_dev(unsigned char *name)
{
	int desc;
	int mask;

	errno = 0;

	if ((desc = open(name, O_WRONLY)) == -1)
	{
		log(LOG_W, "Can't open \"%s\" (%s).\n", name, strerror(errno));

		return(audio_close_dev(desc));
	}

#ifdef VBOXAUDIO_SET_FRAGEMENTSIZE

		/* Fragmentgr��e und Anzahl der Fragmente einstellen. Die	*/
		/* h�heren 16 Bit sind die Anzahl der Fragmente, die nied-	*/
		/* eren die Fragmentgr��e. Hier 5 Fragmente und 64 Byte		*/
		/* Fragmentgr��e (Bit 5). Es darf nur *ein* Bit f�r die		*/
		/* Gr��e verwendet werden, sonst schmiert der Rechner eis-	*/
		/* kalt ab!																	*/

	mask = 0x00050005;

	if (ioctl(desc, SNDCTL_DSP_SETFRAGMENT, &mask) == -1)
	{
		log(LOG_W, "Error: SNDCTL_DSP_SETFRAGMENT (%s).\n", strerror(errno));

		return(audio_close_dev(desc));
	}

#else
	log(LOG_D, "Setting audio fragment size disabled at compile time!\n");
#endif

		/* OSS ab 3.6 gibt muLaw nur zur�ck, wenn die Audiohardware	*/
		/* das Format unterst�tzt. Ansonsten wird muLaw durch eine	*/
		/* Lookup-Table emuliert. Es wird hier nur gepr�ft ob U8		*/
		/* unterst�tzt wird. Wenn ja wird muLaw eingestellt, aber	*/
		/* keine weitere �berpr�fung mehr gemacht!						*/

	if (ioctl(desc, SNDCTL_DSP_GETFMTS, &mask) == -1)
	{
		log(LOG_W, "Error: SNDCTL_DSP_GETFMTS (%s).\n", strerror(errno));

		return(audio_close_dev(desc));
	}

	if (!(mask & AFMT_U8))
	{
 		log(LOG_E, "Audio device doesn't support U8 (disabled).\n");

		return(audio_close_dev(desc));
	}

	mask = AFMT_MU_LAW;

	if (ioctl(desc, SNDCTL_DSP_SETFMT, &mask) == -1)
	{
		log(LOG_W, "Error: SNDCTL_DSP_SETFMT (%s).\n", strerror(errno));

		return(audio_close_dev(desc));
	}

	if (!(mask & AFMT_MU_LAW))
	{
 		log(LOG_D, "Audio device doesn't support hardware muLaw.\n");
	}

		/* Stereo-Modus des Devices einstellen. 0 ist Mono und 1	*/
		/* ist Stereo. Da nur in Mono aufgezeichnet wird, wird	*/
		/* auch nur Mono eingestellt.										*/

	mask = 0;

	if (ioctl(desc, SNDCTL_DSP_STEREO, &mask) == -1)
	{
		log(LOG_W, "Error: SNDCTL_DSP_STEREO (%s).\n", strerror(errno));

		return(audio_close_dev(desc));
	}

		/* Sampling Rate des Devices einstellen. i4l liefert die	*/
		/* Daten in 8kHz, also werden sie auch damit wiedergege-	*/
		/*	ben.																	*/

	mask = 8000;

	if (ioctl(desc, SNDCTL_DSP_SPEED, &mask) == -1)
	{
		log(LOG_W, "Error: SNDCTL_DSP_SPEED (%s).\n", strerror(errno));

		return(audio_close_dev(desc));
	}

	if (mask != 8000)
	{
		log(LOG_E, "Audio device doesn't support 8kHz sampling rate (disabled).\n");

		return(audio_close_dev(desc));
	}

		/* Nach allen Einstellungen zur Sicherheit nochmal die	*/
		/* Gr��e der Fragmente �berpr�fen. Sie mu� mit der Gr��e	*/
		/* des Modembuffers �bereinstimmen (32 Byte).				*/

	if (ioctl(desc, SNDCTL_DSP_GETBLKSIZE, &mask) == -1)
	{
		log(LOG_W, "Error: SNDCTL_DSP_GETBLKSIZE (%s).\n", strerror(errno));

		return(audio_close_dev(desc));
	}

	log(LOG_D, "Audio fragment size is %d; voice buffer size is %d.\n", mask, VBOXVOICE_BUFSIZE);

#ifdef VBOXAUDIO_SET_FRAGEMENTSIZE

	if (mask != VBOXVOICE_BUFSIZE)
	{
		log(LOG_E, "Audio fragment size is not %d (audio disabled).\n", VBOXVOICE_BUFSIZE);

		return(audio_close_dev(desc));
	}

#endif

	return(desc);
}

/************************************************************************* 
 ** audio_close_dev():	Schlie�t das Audiodevice.								**
 *************************************************************************
 ** => desc					File Descriptor des Devices.							**
 *************************************************************************/

int audio_close_dev(int desc)
{
	if (desc != -1) close(desc);

	return(-1);
}
