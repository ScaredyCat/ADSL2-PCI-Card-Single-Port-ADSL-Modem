/*
** $Id: libvbox.c,v 1.12 2001/03/01 14:59:16 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
              
#include "libvbox.h"                     

/** Variables ************************************************************/

char *compressions[] =
{
   "?", "?", "ADPCM-2", "ADPCM-3", "ADPCM-4", "ALAW", "ULAW"
};

static char  vboxd_message[VBOXD_MAX_LINE + 1];

int vboxd_r_fd = -1;
int vboxd_w_fd = -1;

/**************************************************************************/
/** vboxd_connect(): Connect to the vbox message server. After connect   **/
/**                  the function reads the server startup message!      **/
/**************************************************************************/
/** machine          String with the hostname or the ip address.         **/
/** port             Port number to connect.                             **/
/** <return>         0 on success; < 0 on error.                         **/
/**************************************************************************/

int vboxd_connect(char *machine, int port)
{
	struct sockaddr_in   sockp;
   struct hostent      *hostp;
	struct hostent       defaulthost;
	struct in_addr       defaultaddr;
	char                 defaultname[256];
	char                *defaultlist[1];
	char               **p;
	int                  c;
	int                  s;

	if (isdigit(*machine))
	{
		defaultlist[0] = NULL;

		if (inet_aton(machine, &defaultaddr) != 0)
		{
			xstrncpy(defaultname, machine, 255);

			defaulthost.h_name      = (char *)defaultname;
			defaulthost.h_addr_list = defaultlist;
			defaulthost.h_addr      = (char *)&defaultaddr;
			defaulthost.h_length    = sizeof(struct in_addr);
			defaulthost.h_addrtype  = AF_INET;
			defaulthost.h_aliases   = 0;

			hostp = &defaulthost;
		}
		else hostp = gethostbyname(machine);
	}
	else hostp = gethostbyname(machine);

	if (!hostp) return(VBOXC_ERR_UNKNOWNHOST);

	memset((char *)&sockp, '\0', sizeof(struct sockaddr_in));

   sockp.sin_family = hostp->h_addrtype;
   sockp.sin_port   = htons(port);

	c = -1;
	s = -1;

	errno = 0;

   for (p = hostp->h_addr_list; ((p) && (*p)); p++)
	{
      s = socket(hostp->h_addrtype, SOCK_STREAM, 0);

      if (s < 0) return(VBOXC_ERR_NOSOCKET);

      memcpy((char *)&sockp.sin_addr, *p, hostp->h_length);

		c = connect(s, (struct sockaddr *)&sockp, sizeof(struct sockaddr_in));

		if (c == 0) break;

		close(s);
	}

	if (c < 0) return(VBOXC_ERR_NOCONNECT);

	vboxd_r_fd = s;
	vboxd_w_fd = dup(vboxd_r_fd);

	if ((vboxd_w_fd < 0) || (vboxd_r_fd < 0))
	{
		vboxd_disconnect();

		return(VBOXC_ERR_NOFILEIO);
	}

	if (!vboxd_get_message())
	{
		vboxd_disconnect();

		return(VBOXC_ERR_GETMESSAGE);
	}

	if (!vboxd_test_response(VBOXD_VAL_SERVEROK))
	{
		vboxd_disconnect();

		return(VBOXC_ERR_GETMESSAGE);
	}

	return(VBOXC_ERR_OK);
}

/**************************************************************************/
/** vboxd_disconnect(): Sends "quit" and disconnect from the server.     **/
/**************************************************************************/

void vboxd_disconnect(void)
{
	if (vboxd_w_fd) vboxd_put_message("quit");

	if (vboxd_r_fd != -1) close_and_mone(vboxd_r_fd);
	if (vboxd_w_fd != -1) close_and_mone(vboxd_w_fd);
}

/**************************************************************************/
/** vboxd_login(): Login to the vboxd server.                            **/
/**************************************************************************/
/** username       Username to login.                                    **/
/** password       Password to login.                                    **/
/** <return>       0 on success; < 0 on error.                           **/
/**************************************************************************/

int vboxd_login(char *username, char *password)
{
	vboxd_put_message("login %s %s", username, password);

	if (vboxd_get_message())
	{
		if (vboxd_test_response(VBOXD_VAL_LOGINOK)) return(VBOXC_ERR_OK);
	}

	return(VBOXC_ERR_LOGIN);
}

/**************************************************************************/
/** vboxd_put_message(): Puts a message to the server.                   **/
/**************************************************************************/

void vboxd_put_message(char *fmt, ...)
{
   va_list arg;
	char    msgline[VBOXD_MAX_LINE + 1];

	va_start(arg, fmt);
	vnprintstring(msgline, VBOXD_MAX_LINE, fmt, arg);
	va_end(arg);

	write(vboxd_w_fd, msgline, strlen(msgline));
	write(vboxd_w_fd, "\r\n", 2);
}

/**************************************************************************/
/** vboxd_get_message(): Try to get a message from the server. The func- **/
/**                      tion us a timeout of VBOXD_GET_MSG_TIMEOUT sec- **/
/**                      ends to get the message.                        **/
/**************************************************************************/
/** <return>             Pointer to the message or NULL on error.        **/
/**************************************************************************/

char *vboxd_get_message(void)
{
	struct timeval timeval;
	fd_set         rmask;
	char          *stop;
	int            p;
	int            c;
	int            rc;

	*vboxd_message = '\0';

	p = 0;
	c = 0;

	while (TRUE)
	{
		VBOX_ONE_FD_MASK(&rmask, vboxd_r_fd);

		timeval.tv_sec  = VBOXD_GET_MSG_TIMEOUT;
		timeval.tv_usec = 0;

		rc = select((vboxd_r_fd + 1), &rmask, NULL, NULL, &timeval);

		if (rc <= 0)
		{
			if ((rc < 0) && (errno == EINTR)) continue;

			break;
		}

		if (!FD_ISSET(vboxd_r_fd, &rmask)) break;

		rc = read(vboxd_r_fd, &c, 1);

		if (rc <= 0)
		{
			if ((rc < 0) && (errno == EINTR)) continue;

			break;
		}

		if (c == '\n')
		{
			if ((stop = rindex(vboxd_message, '\r'))) *stop = '\0';

			return(vboxd_message);
		}

		vboxd_message[p + 0] = c;
		vboxd_message[p + 1] = '\0';

		if (p++ >= VBOXD_MAX_LINE) break;
	}

	return(NULL);
}

/**************************************************************************/
/** vboxd_test_response(): Test response of a vboxd message.             **/
/**************************************************************************/
/** response               Needed response code to check.                **/
/** <return>               TRUE (1) on success, FALSE (0) on error.      **/
/**************************************************************************/

int vboxd_test_response(char *response)
{
	if (strlen(vboxd_message) > (strlen(response) + 1))
	{
		if (strncmp(response, vboxd_message, strlen(response)) == 0)
		{
			if (vboxd_message[strlen(response)] == ' ') returnok();
		}
	}

	returnerror();
}

/*************************************************************************/
/** get_message_ptime():	Returns the vbox message length in seconds.	**/
/**								The length is calculated from the size & the	**/
/**								compression mode.										**/
/*************************************************************************/
/** compression				Compression mode of the sample.					**/
/** size							Size of the sample.									**/
/** <return>					Sample length in seconds.							**/
/*************************************************************************/

int get_message_ptime(int compression, int size)
{
	if ((compression >= 2) && (compression <= 4))
	{
		size = ((size * 8) / compression);
	}
                           
	return((size / KERNEL_SAMPLE_FREQ));
}

/*************************************************************************/
/** get_nr_messages():	Returns number of files in directory <path>. If	**/
/**							<countnew> is set only files with modification	**/
/**							time greater 0 are counted. Theres no check if	**/
/**							the files is a vbox message!							**/
/*************************************************************************/
/** path						Directory with files to count.						**/
/** countnew				0 to count all or 1 to count only new files.		**/
/** <return>				Number of files found.									**/
/*************************************************************************/

int get_nr_messages(char *path, int countnew)
{
	struct dirent *entry;
	struct stat		status;

	char	temp[PATH_MAX + 1];
	DIR  *dir;
	int	messages;

	messages = 0;

	if ((dir = opendir(path)))
	{
		while ((entry = readdir(dir)))
		{
			if (countnew)
			{
				if (strcmp(entry->d_name, "." ) == 0) continue;
				if (strcmp(entry->d_name, "..") == 0) continue;

				xstrncpy(temp, path			 , PATH_MAX);
				xstrncat(temp, "/"			 , PATH_MAX);
				xstrncat(temp, entry->d_name, PATH_MAX);

				if (stat(temp, &status) == 0)
				{
					if (status.st_mtime > 0) messages++;
				}
			}
			else messages++;
		}
		
		closedir(dir);
	}

	return(messages);
}

/*************************************************************************/
/** ctrl_create():	Creates a vbox control file. The file is created	**/
/**						with the permissions -rw-rw-rw-.							**/
/*************************************************************************/
/** path					Path to the spool directory of the current user.	**/
/** file					Name of the control file to create.						**/
/** <return>			FALSE (0) on error; TRUE (1) on success.           **/
/*************************************************************************/

int ctrl_create(char *path, char *file)
{
	char	location[PATH_MAX + 1];
	int	fd;
	
	xstrncpy(location, path, PATH_MAX);
	xstrncat(location, "/" , PATH_MAX);
	xstrncat(location, file, PATH_MAX);

	if (strncmp(file, CTRL_NAME_MAGIC, strlen(CTRL_NAME_MAGIC)) == 0)
	{
		if ((fd = open(location, O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) != -1)
		{
			chmod(location, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
			chmod(location, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

			close(fd);

			returnok();
		}
	}

	returnerror();
}

/*************************************************************************/
/** ctrl_remove():	Removes a vbox control file.								**/
/*************************************************************************/
/** path					Path to the spool directory of the current user.	**/
/** file					Name of the control file to remove.						**/
/** <return>			FALSE (0) on error; TRUE (1) on success.		 	   **/
/*************************************************************************/

int ctrl_remove(char *path, char *file)
{
	char	location[PATH_MAX + 1];
	int	loop;
	
	xstrncpy(location, path, PATH_MAX);
	xstrncat(location, "/" , PATH_MAX);
	xstrncat(location, file, PATH_MAX);

	if (strncmp(file, CTRL_NAME_MAGIC, strlen(CTRL_NAME_MAGIC)) == 0)
	{
		loop = 5;

		while (loop > 0)
		{
			unlink(location);
			unlink(location);
		
			if (!ctrl_ishere(path, file)) returnok();

			xpause(1000);
		
			loop--;
		}
	}

	returnerror();
}

/*************************************************************************/
/** ctrl_ishere():	Checks if a control file exists.							**/
/*************************************************************************/
/** path					Path to the spool directory of the current user.	**/
/** file					Name of the control file to check.						**/
/** <return>			FALSE (0) doesn't exist; TRUE (1) exists.			   **/
/*************************************************************************/

int ctrl_ishere(char *path, char *file)
{
	char location[PATH_MAX + 1];
	
	xstrncpy(location, path, PATH_MAX);
	xstrncat(location, "/" , PATH_MAX);
	xstrncat(location, file, PATH_MAX);

	if (strncmp(file, CTRL_NAME_MAGIC, strlen(CTRL_NAME_MAGIC)) == 0)
	{
		if (access(location, F_OK) == 0) returnok();
	}

	returnerror();
}

/*************************************************************************/
/** xstrncpy():	Copys one string to another.									**/
/*************************************************************************/
/** dest				Pointer to the destination.									**/
/** source			Pointer to the source.											**/
/** max				Max length of destination.										**/
/*************************************************************************/

void xstrncpy(char *dest, char *source, int max)
{
	strncpy(dest, source, max);
	
	dest[max] = '\0';
}

/*************************************************************************/
/** xstrncat():	Cats one string to another.									**/
/*************************************************************************/
/** dest				Pointer to the destination.									**/
/** source			Pointer to the source.											**/
/** max				Max length of destination.										**/
/*************************************************************************/

void xstrncat(char *dest, char *source, int max)
{
	if ((max - strlen(dest)) > 0) strncat(dest, source, max - strlen(dest));

	dest[max] = '\0';
}

/*************************************************************************/
/** xpause():	Waits some miliseconds.												**/
/*************************************************************************/
/** ms			Miliseconds to wait.													**/
/*************************************************************************/

void xpause(unsigned long ms)
{
	usleep(ms * 1000);
}

/*************************************************************************/
/** xstrtol():	Converts a string to a long number, using a default on	**/
/**				error.																	**/
/*************************************************************************/
/** str			String to convert to long.											**/
/** use			Default value if string can't converted.						**/
/** <return>	Converted string value on success; default on error.		**/
/*************************************************************************/

long xstrtol(char *str, long use)
{
	char *stop;
	long	line;

	line = strtol(str, &stop, 10);
	
	if ((line < 0) || (*stop != 0)) line = use;

	return(line);
}

/*************************************************************************/
/** xstrtoul(): Converts a string to a unsigned long number, using a    **/
/**             default on error.                                       **/
/*************************************************************************/
/** str			 String to convert to unsigned long.							**/
/** use			 Default value if string can't converted.						**/
/** <return>	 Converted string value on success; default on error.		**/
/*************************************************************************/

unsigned long xstrtoul(char *str, unsigned long use)
{
	char          *stop;
	unsigned long  line;

	line = strtoul(str, &stop, 10);
	
	if ((line < 0) || (*stop != 0)) line = use;

	return(line);
}

/*************************************************************************/
/** header_put():	Writes the vbox audio header.									**/
/*************************************************************************/
/** fd				File descriptor used to write.								**/
/** header			Pointer to a filled vbox audio header.						**/
/** <return>		0 on error; 1 on success.										**/
/*************************************************************************/

int header_put(int fd, vaheader_t *header)
{
	if (write(fd, header, sizeof(vaheader_t)) != sizeof(vaheader_t))
	{
		returnerror();
	}

	returnok();
}

/*************************************************************************/
/** header_get():	Reads a vbox audio header.										**/
/*************************************************************************/
/** fd				File descriptor used to read.									**/
/** header			Pointer to a vbox audio header.								**/
/** <return>		0 on error; 1 on success.										**/
/*************************************************************************/

int header_get(int fd, vaheader_t *header)
{
	vaheader_t dummy;

	if (read(fd, &dummy, VAH_MAX_MAGIC) == VAH_MAX_MAGIC)
	{
		if (strncmp(dummy.magic, VAH_MAGIC, VAH_MAX_MAGIC) == 0)
		{
			if (lseek(fd, 0, SEEK_SET) == 0)
			{
				if (read(fd, header, sizeof(vaheader_t)) == sizeof(vaheader_t))
				{
					returnok();
				}
			}
		}
	}
	
	returnerror();
}
