/*
** $Id: streamio.c,v 1.4 1997/10/22 20:47:15 fritz Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "streamio.h"
#include "libvbox.h"

/*************************************************************************
 ** streamio_open():	Loads a file into memory.									**
 *************************************************************************/

streamio_t *streamio_open(char *name)
{
	struct stat status;

	streamio_t	*streamio;
	char			*file;
	char			*save;
	long			 i;
	int			 fd;

	if ((fd = open(name, O_RDONLY)) != -1)
	{
		if (stat(name, &status) == 0)
		{
			if ((streamio = malloc(sizeof(streamio_t))))
			{
				if ((file = malloc(status.st_size + 3)))
				{
					if ((save = malloc(strlen(name) + 1)))
					{
						xstrncpy(save, name, strlen(name));

						if (read(fd, file, status.st_size) == status.st_size)
						{
							file[status.st_size + 0] = 1;
							file[status.st_size + 1] = 1;
							file[status.st_size + 2] = 1;

							for (i = 0; i < status.st_size; i++)
							{
								if (file[i] == 13) file[i] = 32;
								if (file[i] == 10) file[i] = 0;
							}
						
							streamio->fd	= fd;
							streamio->name	= save;
							streamio->file	= file;
							streamio->next	= file;
							streamio->time	= status.st_mtime;
							streamio->line	= 0;

							return(streamio);
						}

						free(save);
					}

					free(file);
				}

				free(streamio);
			}
		}

		close(fd);
	}
	
	return(NULL);
}

/*************************************************************************
 ** streamio_reopen():	Reloads a file into memory if changes was made	**
 **							since the last streamio_open(). If not the old	**
 **							file is returned.											**
 *************************************************************************/

streamio_t *streamio_reopen(streamio_t *streamio)
{
	struct stat status;

	streamio_t *newstreamio;

	if (streamio)
	{
		streamio->next = streamio->file;
		streamio->line = 0;

		if (stat(streamio->name, &status) == 0)
		{
			if (status.st_mtime != streamio->time)
			{
				if ((newstreamio = streamio_open(streamio->name)))
				{
					streamio_close(streamio);
					
					return(newstreamio);
				}
			}
		}
	}

	return(streamio);
}

/*************************************************************************
 ** streamio_close():	Close a file opened with streamio_open():			**
 *************************************************************************/

void streamio_close(streamio_t *streamio)
{
	if (streamio)
	{
		if (streamio->file) free(streamio->file);
		if (streamio->name) free(streamio->name);

		if (streamio->fd != -1) close(streamio->fd);
		
		free(streamio);
	}
}

/*************************************************************************
 ** streamio_gets():	Returns a line from a file opened with streamio_-	**
 **						open().															**
 *************************************************************************/

char *streamio_gets(char *temp, int max, streamio_t *streamio)
{
	char *stop;
	char *line;

	if (streamio)
	{
		while (streamio->next)
		{
			line = streamio->next;

			while (*(streamio->next) != 0) streamio->next++;

			streamio->next++;
			streamio->line++;

			if (*(streamio->next) == 1) streamio->next = NULL;

			while (isspace(*line)) line++;

			xstrncpy(temp, line, max);

			if ((stop = rindex(temp, '#'))) *stop = '\0';
      
			while (strlen(temp) > 0)
			{
				if (!isspace(temp[strlen(temp) - 1])) break;
                  
				temp[strlen(temp) - 1] = '\0';
			}

			if ((*temp != '\0') && (*temp != '#')) return(temp);
		} 
	}

	return(NULL);
}
