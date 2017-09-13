/*
** $Id: audio.h,v 1.3 1998/08/31 15:30:39 michael Exp $
**
** Copyright 1996-1998 Michael 'Ghandi' Herold <michael@abadonna.mayn.de>
*/

#ifndef _VBOX_AUDIO_H
#define _VBOX_AUDIO_H 1

/** Defines **************************************************************/

	/* Define the next line if you want to set the audio buffer size	*/
	/* to the same as the voice buffer size (this will reduce the		*/
	/* echo effect). But note, this can crash your machine (kernel		*/
	/* bug)!																				*/

#undef VBOXAUDIO_SET_FRAGEMENTSIZE

/** Prototypes ***********************************************************/

extern int audio_open_dev(unsigned char *);
extern int audio_close_dev(int);

#endif /* _VBOX_AUDIO_H */
