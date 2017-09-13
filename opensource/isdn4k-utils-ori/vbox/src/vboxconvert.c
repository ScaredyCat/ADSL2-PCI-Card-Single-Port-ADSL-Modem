/*
** $Id: vboxconvert.c,v 1.10 2001/03/01 14:59:16 paul Exp $
**
** Copyright (C) 1996, 1997 Michael 'Ghandi' Herold
**
** The most converting routines are taken from the mgetty/vgetty v0.98
** package - this routines are copyright by there autors!
*/

#include "config.h"

#if TIME_WITH_SYS_TIME
#   include <sys/time.h>
#   include <time.h>
#else
#   if HAVE_SYS_TIME_H
#      include <sys/time.h>
#   else
#      include <time.h>
#   endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/utsname.h>

#include "libvbox.h"
#include "vboxconvert.h"

/** Variables ************************************************************/
                
static state_t InitState = { 0x0000, 0 };
                
static int mx[3][8] =
{
	{ 0x3800, 0x5600, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
	{ 0x399a, 0x3a9f, 0x4d14, 0x6607, 0x0000, 0x0000, 0x0000, 0x0000 },
	{ 0x3556, 0x3556, 0x399A, 0x3A9F, 0x4200, 0x4D14, 0x6607, 0x6607 },
};

static int bitmask[9] =
{
	0, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
}; 

static char *sun_format_names[] =
{
	"unspecified format",
	"8-bit mu-law samples",
	"8-bit linear samples",
	"16-bit linear samples",
	"24-bit linear samples",
	"32-bit linear samples",
	"floating-point samples",
	"double-precision float samples",
	"fragmented sampled data",
	"?",
	"dsp program",
	"8-bit fixed-point samples",
	"16-bit fixed-point samples",
	"24-bit fixed-point samples",
	"32-bit fixed-point samples",
	"?",
	"non-audio display data",
	"?",
	"16-bit linear with emphasis",
	"16-bit linear with compression",
	"16-bit linear with emphasis & compression",
	"music Kit DSP commands",
	"?"
};

static struct option args_vboxtoau[] =
{
	{ "help"      , no_argument      , NULL, 'h' },
	{ "version"   , no_argument      , NULL, 'v' },
	{ "samplerate", required_argument, NULL, 'r' },
	{ "ulaw"      , no_argument      , NULL, 'u' },
	{ "linear8"   , no_argument      , NULL, '1' },
	{ "linear16"  , no_argument      , NULL, '2' },
	{ NULL        , 0                , NULL,  0  }
};

static struct option args_vboxmode[] =
{
	{ "help"      , no_argument      , NULL, 'h' },
	{ "version"   , no_argument      , NULL, 'v' },
	{ "quiet"     , no_argument      , NULL, 'q' },
	{ NULL        , 0                , NULL,  0  }
};

static struct option args_autovbox[] =
{
	{ "help"      , no_argument      , NULL, 'h' },
	{ "version"   , no_argument      , NULL, 'v' },
	{ "adpcm-2"	,	no_argument			, NULL, '2' },
	{ "adpcm-3" ,  no_argument			, NULL, '3' },
	{ "adpcm-4" ,  no_argument       , NULL, '4' },
	{ "ulaw"    ,  no_argument       , NULL, 'u' },
	{ "name"		, required_argument	, NULL, 'n' },
	{ "callerid", required_argument  , NULL, 'c' },
	{ "phone"   , required_argument	, NULL, 'p' },
	{ "location", required_argument	, NULL, 'l'	},
	
	{ NULL        , 0                , NULL,  0  }
};

static char *vbasename = NULL;
static FILE *vboxtmpfile   = NULL;
static char perrormsg[256];

/** Prototypes ***********************************************************/

static int				start_vboxmode(char *, int);
static void			   start_vboxtoau(int, int);
static void				start_autovbox(int, char *, char *, char *, char *);
static void				usage_vboxtoau(void);
static void				usage_vboxmode(void);
static void				usage_autovbox(void);
static void          leave_vboxtoau(int);
static void          leave_autovbox(int);
static void				version(void);

static int				convert_adpcm_to_pvf(int, FILE *, FILE *);
static int				convert_pvf_to_adpcm(int, FILE *, FILE *);
static int				convert_au_to_pvf(FILE *, FILE *);
static int				convert_pvf_to_au(int, int, FILE *, FILE *);
static int				convert_ulaw_to_pvf(FILE *, FILE *);
static int				convert_pvf_to_ulaw(FILE *, FILE *);
static unsigned char	byte_linear_to_ulaw(int);
static int				byte_ulaw_to_linear(unsigned char);

static int				test_sample_is_vbox(char *, int);
static int				test_sample_is_au(char *, int);
static void				write_one_word(int, FILE *);
static int				read_one_word(FILE *);
static int				zget(FILE *);
static void				zput(int, FILE *);
static int				get_bits(int, state_t *, FILE *);
static void				put_bits(int, int, state_t *, FILE *);
static int				check_io_error(FILE *, FILE *);

/*************************************************************************/
/** The magic main...																	**/
/*************************************************************************/

int main(int argc, char **argv)
{
	struct utsname  utsname;
	struct passwd	*passwd;

	char	realname[VAH_MAX_NAME + 1];
	int	opts;
	int	rate;
	int	mode;
	char *name;
	char *phone;
	char *location;
	int	i;

	if (!(vbasename = rindex(argv[0], '/')))
		vbasename = argv[0];
	else
		vbasename++;

		/* Called as 'vboxtoau' converts a messages saved with vbox to	*/
		/* sun's au format.															*/

	if (strcasecmp(vbasename, "vboxtoau") == 0)
	{
		rate = 8000;
		mode = SND_FORMAT_LINEAR_16;

		while ((opts = getopt_long(argc, argv, "vhu12r:", args_vboxtoau, (int *)0)) != EOF)
		{
			switch (opts)
			{
				case 'r':
					rate = atoi(optarg);
					break;
					
				case 'u':
					mode = SND_FORMAT_MULAW_8;
					break;
					
				case '1':
					mode = SND_FORMAT_LINEAR_8;
					break;
					
				case '2':
					mode = SND_FORMAT_LINEAR_16;
					break;

				case 'v':
					version();
					break;

				case 'h':
				default:
					usage_vboxtoau();
					break;
			}
		}

		start_vboxtoau(mode, rate);
	}

		/* Called as 'autovbox' converts a sun au sample to any of the	*/
		/* vbox message formats.													*/

	if (strcasecmp(vbasename, "autovbox") == 0)
	{
		name		= "*** Unknown ***";
		phone		= "*** Unknown ***";
		location	= "*** Unknown ***";
		mode		= 4;

		if (uname(&utsname) == 0) location = utsname.nodename;

		if ((passwd = getpwuid(getuid())))
		{
			xstrncpy(realname, passwd->pw_gecos, VAH_MAX_NAME);
			
			for (i = 0; i < strlen(realname); i++)
			{
				if ((realname[i] == ',') || (realname[i] == ';'))
				{
					realname[i] = '\0';
					
					break;
				}
			}

			name = realname;
		}

		while ((opts = getopt_long(argc, argv, "hv234un:c:p:l:", args_autovbox, (int *)0)) != EOF)
		{
			switch (opts)
			{
				case '2':
					mode = 2;
					break;
					
				case '3':
					mode = 3;
					break;
					
				case '4':
					mode = 4;
					break;
					
				case 'u':
					mode = 6;
					break;
					
				case 'n':
					name = optarg;
					break;
					
				case 'p':
					phone = optarg;
					break;
					
				case 'l':
					location = optarg;
					break;

				case 'v':
					version();
					break;

				case 'h':
				default:
					usage_autovbox();
					break;
			}
		}

		start_autovbox(mode, name, "0", phone, location);
	}

		/* Called as 'vboxmode' displays information about the sample	*/
		/* format.																		*/

	if (strcasecmp(vbasename, "vboxmode") == 0)
	{
		mode = FALSE;
		name = "";

		while ((opts = getopt_long(argc, argv, "vhq", args_vboxmode, (int *)0)) != EOF)
		{
			switch (opts)
			{
				case 'v':
					version();
					break;
					
				case 'q':
					mode = TRUE;
					break;
					
				case 'h':
				default:
					usage_vboxmode();
					break;
			}
		}

		for (i = 1; i < argc; i++)
		{
			if (argv[i][0] != '-') name = argv[i];
		}

		if ((argc < 2) || (!*name)) usage_vboxmode();

		exit(start_vboxmode(name, mode));
	};

	fprintf(stderr, "\n");
	fprintf(stderr, "The vbox converter can be called as:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "vboxtoau - to convert vbox messages to au format.\n");
	fprintf(stderr, "autovbox - to convert au format samples to vbox messages.\n");
	fprintf(stderr, "vboxmode - to display message information (works also with au).\n");
	fprintf(stderr, "\n");

	exit(255);
}

/*************************************************************************/
/** start_vboxtoau(): Converts vbox messages to sun's au format.		   **/
/*************************************************************************/

static void start_vboxtoau(int samplemode, int samplerate)
{
	vaheader_t	header;
	int	      compression;
	int 	      result;

	vboxtmpfile = NULL;
	
	signal(SIGINT , leave_vboxtoau);
	signal(SIGQUIT, leave_vboxtoau);
	signal(SIGTERM, leave_vboxtoau);
	signal(SIGHUP , leave_vboxtoau);
	signal(SIGPIPE, leave_vboxtoau);
	
	if (!(vboxtmpfile = tmpfile()))
	{
		sprintf(perrormsg, "%s: can't create tmpfile", vbasename);
		perror(perrormsg);
		leave_vboxtoau(255);
	}

	if (fread(&header, sizeof(vaheader_t), 1, stdin) != 1)
	{
		sprintf(perrormsg, "%s: can't read vbox audio header", vbasename);
		perror(perrormsg);
		leave_vboxtoau(255);
	}

	if (strncmp(header.magic, VAH_MAGIC, VAH_MAX_MAGIC) != 0)
	{
		fprintf(stderr, "%s: sample contains no vbox audio header.\n", vbasename);
		
		leave_vboxtoau(255);
	}

	compression = (int)ntohl(header.compression);

	result = 255;

	switch (compression)
	{
		case 2:
		case 3:
		case 4:
			result = convert_adpcm_to_pvf(compression, stdin, vboxtmpfile);
			break;

		case 6:
			result = convert_ulaw_to_pvf(stdin, vboxtmpfile);
			break;

		default:
			fprintf(stderr, "%s: unknown/unsupported compression %d.\n", vbasename, compression);
			break;
	}

	if (result == 0)
	{
		result = convert_pvf_to_au(samplemode, samplerate, vboxtmpfile, stdout);
	}

	leave_vboxtoau(result);
}

/**************************************************************************/
/** leave_vboxtoau(): Leave vboxtoau mode.                               **/
/**************************************************************************/

static void leave_vboxtoau(int sig)
{
	if (vboxtmpfile)
		fclose(vboxtmpfile);
	exit(sig);
}

/*************************************************************************/
/** usage_vboxtoau(): Usage message for "vboxtoau".                     **/
/*************************************************************************/

static void usage_vboxtoau(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s OPTION [ OPTION ] [ ... ] <INFILE >OUTFILE\n", vbasename);
	fprintf(stderr, "\n");
	fprintf(stderr, "-r, --samplerate RATE   Header sampling rate (default 8000).\n");
	fprintf(stderr, "-u, --ulaw              Use 8-bit uLaw output.\n");
	fprintf(stderr, "-1, --linear8           Use 8-bit linear output.\n");
	fprintf(stderr, "-2, --linear16          Use 16-Bit linear output (default).\n");
	fprintf(stderr, "-h, --help              Prints this usage message.\n");
	fprintf(stderr, "-v, --version           Prints the package version.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Note: The sampling rate is written into the file header only, it\n      doesn't do any sampling rate conversion!\n");
	fprintf(stderr, "\n");

	exit(255);
}

/*************************************************************************/
/** start_vboxmode(): Displays sample information.							   **/
/*************************************************************************/

static int start_vboxmode(char *name, int quiet)
{
	int result;

	result = test_sample_is_vbox(name, quiet);
	
	if (result == 255)
	{
		result = test_sample_is_au(name, quiet);
	}

	if (result == 255)
	{
		fprintf(stderr, "%s: sample contains no vbox or sun au header.\n", vbasename);
	}

	return(result);
}

/*************************************************************************/
/** usage_vboxmode(): Usage message for "vboxmode".							**/
/*************************************************************************/

static void usage_vboxmode(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s OPTION [ OPTION ] [ ... ] SAMPLENAME\n", vbasename);
	fprintf(stderr, "\n");
	fprintf(stderr, "-q, --quiet             Returns only the errorcode (no descriptions).\n");
	fprintf(stderr, "-h, --help              Prints this usage message.\n");
	fprintf(stderr, "-v, --version           Prints the package version.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "The program returns the sample format as errorcode:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "- For au samples it returns %d-%d,\n", SUN_FORMAT_MIN + 128, SUN_FORMAT_MAX + 128);
	fprintf(stderr, "- For vbox messages it returns 2-6,\n");
	fprintf(stderr, "- For unknown formats or on errors it returns 255.\n");
	fprintf(stderr, "\n");

	exit(255);
}

/*************************************************************************/
/** start_autovbox(): Converts au samples to vbox messages.				   **/
/*************************************************************************/

static void start_autovbox(int compression, char *name, char *id, char *phone, char *location)
{
	vaheader_t	header;
	int	      result;

	vboxtmpfile = NULL;

	signal(SIGINT , leave_autovbox);
	signal(SIGQUIT, leave_autovbox);
	signal(SIGTERM, leave_autovbox);
	signal(SIGHUP , leave_autovbox);
	signal(SIGPIPE, leave_autovbox);
	
	if (!(vboxtmpfile = tmpfile()))
	{
		sprintf(perrormsg, "%s: can't create tmpfile", vbasename);
		perror(perrormsg);
		leave_autovbox(255);
	}

	xstrncpy(header.magic	, VAH_MAGIC	, VAH_MAX_MAGIC	);
	xstrncpy(header.callerid, id			, VAH_MAX_CALLERID);
	xstrncpy(header.name		, name		, VAH_MAX_NAME		);
	xstrncpy(header.phone	, phone		, VAH_MAX_PHONE	);
	xstrncpy(header.location, location	, VAH_MAX_LOCATION);
	
	header.time				= htonl(time(NULL));
	header.compression	= htonl(compression);
	
	if (fwrite(&header, sizeof(vaheader_t), 1, stdout) != 1)
	{
		fprintf(stderr, "%s: can't write vbox audio header.\n", vbasename);
		
		leave_autovbox(255);
	}

	result = convert_au_to_pvf(stdin, vboxtmpfile);

	if (result == 0)
	{
		result = 255;

		switch (compression)
		{
			case 2:
			case 3:
			case 4:
				result = convert_pvf_to_adpcm(compression, vboxtmpfile, stdout);
				break;

			case 6:
				result = convert_pvf_to_ulaw(vboxtmpfile, stdout);
				break;
				
			default:
				fprintf(stderr, "%s: unsupported compression type.\n", vbasename);
				break;

		}
	}

	leave_autovbox(result);
}

/**************************************************************************/
/** leave_autovbox(): Leave autovbox mode.                               **/
/**************************************************************************/

static void leave_autovbox(int sig)
{
	if (vboxtmpfile)
		fclose(vboxtmpfile);
	exit(sig);
}

/*************************************************************************/
/** usage_autovbox(): Usage message for "autovbox".							**/
/*************************************************************************/

static void usage_autovbox(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s OPTION [ OPTION ] [ ... ] <INFILE >OUTFILE\n", vbasename);
	fprintf(stderr, "\n");
	fprintf(stderr, "-2, --adpcm-2             Converts sample to adpcm-2.\n");
	fprintf(stderr, "-3, --adpcm-3             Converts sample to adpcm-3.\n");
	fprintf(stderr, "-4, --adpcm-4             Converts sample to adpcm-4 (default).\n");
	fprintf(stderr, "-u, --ulaw                Converts sample to ulaw.\n");
	fprintf(stderr, "-n, --name NAME           Sets header information.\n");
	fprintf(stderr, "-p, --phone PHONE         Sets header information.\n");
	fprintf(stderr, "-l, --location LOCATION   Sets header information.\n");
	fprintf(stderr, "-h, --help                Prints this usage message.\n");
	fprintf(stderr, "-v, --version             Prints the package version.\n");
	fprintf(stderr, "\n");

	exit(255);
}

/*************************************************************************/
/** convert_pvf_to_adpcm(): Converts protable voice format to adpcm-2	**/
/**								 adpcm-3 or adpcm-4.								   **/
/*************************************************************************/

static int convert_pvf_to_adpcm(int nbits, FILE *in, FILE *out)
{
	state_t	s;
	int		a;
	int		d;
	int		e;
	int		nmax;
	int		sign;
	int		delta;

	rewind(in);
	clearerr(in);
	clearerr(out);

	a = 0;
	d = 5;
	s = InitState;

	while (1)
	{
		e = 0;
		nmax = 1 << (nbits-1);
	
		delta = (zget(in) >> 2) - a;

		if (feof(in)) break;
	
		if (delta < 0)
		{
			e = nmax;
			delta = -delta;
		}

		while(--nmax && delta > d)
		{
			delta -= d;
			e++;
		}
	
		if (nbits==4 && ((e & 0x0F) == 0)) e = 0x08;

		put_bits( e, nbits, &s, out);
	
		a = (a * 4093 + 2048) >> 12;
	
		sign = (e >> (nbits - 1)) ? -1 : 1;
		e = (e & bitmask[nbits - 1]);
	
		a += sign * ((e<<1)+1) * d >>1;

		if ((d & 1)) a++;
	
		d = (d * mx[nbits - 2][e] + 0x2000) >> 14;

		if (d < 5) d = 5;
	}

	if (s.nleft) put_bits(0, 8-s.nleft, &s, out);

	return(check_io_error(in, out));
}

/*************************************************************************/
/** convert_adpcm_to_pvf(): Converts adpcm-2, adpcm-3 or adpcm-4 to	   **/
/**								 portable voice format.							   **/
/*************************************************************************/

static int convert_adpcm_to_pvf(int nbits, FILE *in, FILE *out)
{
	state_t	state;
	int		a;
	int		d;
	int		e;
	int		sign;

	state = InitState;
	a		= 0;
	d		= 5;

	clearerr(in);
	clearerr(out);

	while (1)
	{
		if ((e = get_bits(nbits, &state, in)) == EOF) break;

		if ((nbits == 4) && (e == 0)) d = 4;
	
		sign = (e >> (nbits - 1)) ? -1 : 1;

		e = e & bitmask[nbits - 1];
		a = (a * 4093 + 2048) >> 12;
	
		a += sign * ((e << 1) + 1) * d >> 1;

		if ((d & 1)) a++;

		zput(a << 2, out);
	
		d = (d * mx[nbits - 2][e] + 0x2000) >> 14;

		if (d < 5) d = 5;	    
	}

	return(check_io_error(in, out));
}

/*************************************************************************/
/** convert_au_to_pvf(): Converts sun's au format to portable voice	   **/
/**							 format.													   **/
/*************************************************************************/

static int convert_au_to_pvf(FILE *in, FILE *out)
{
	SNDSoundStruct hdr;
	int i;
	int c;

	rewind(in);
	clearerr(in);
	clearerr(out);
         
	hdr.magic			= read_one_word(in);
	hdr.dataLocation	= read_one_word(in);
	hdr.dataSize		= read_one_word(in);
	hdr.dataFormat		= read_one_word(in);
	hdr.samplingRate	= read_one_word(in);
	hdr.channelCount	= read_one_word(in);
    
	if (hdr.magic != SND_MAGIC)
	{
		fprintf(stderr, "%s: illegal magic number for an au file.\n", vbasename);
		
		return(255);
	}

	if (hdr.channelCount != 1)
	{
		fprintf(stderr, "%s: number of channels != 1 (only mono supported).\n", vbasename);
		
		return(255);
	}

	for (i = (SND_HEADER_SIZE - 4); i < hdr.dataLocation; i++)
	{
		if (getc(in) == EOF)
		{
			fprintf(stderr, "%s: unexpected EOF.\n", vbasename);
			
			return(255);
		}
	}
    
	switch (hdr.dataFormat)
	{
		case SND_FORMAT_MULAW_8:
			while(1)
			{
				c = getc(in);
				if (feof(in)) break;
				zput(byte_ulaw_to_linear(c), out);
			}
			break;

		case SND_FORMAT_LINEAR_8:
			while(1)
			{
				c = (getc(in) & 0xFF);
				if (c >= 0x80) c -= 0x100;
				if (feof(in)) break;
				zput(c<<8, out);
			}
			break;

			case SND_FORMAT_LINEAR_16:
				while(1)
				{
					c = zget(in);
					if(feof(in)) break;
					zput(c, out);
				}
				break;

			default:
				fprintf(stderr, "%s: unsupported or illegal sound encoding.\n", vbasename);
				return(255);
	}

	return(check_io_error(in, out));
}

/*************************************************************************/
/** convert_pvf_to_au(): Converts portable voice format to sun's au	   **/
/**							 format.													   **/
/*************************************************************************/

static int convert_pvf_to_au(int mode, int rate, FILE *in, FILE *out)
{
	SNDSoundStruct Snd;
	int s;

	rewind(in);
	clearerr(in);
	clearerr(out);

	Snd.dataFormat		= mode;
	Snd.samplingRate	= rate;
	Snd.magic			= SND_MAGIC;
	Snd.dataLocation	= SND_HEADER_SIZE;
	Snd.dataSize		= SND_UNKNOWN_SIZE;
	Snd.channelCount	= 1;
	Snd.info[0]			= 0;
    
	write_one_word((int)Snd.magic,        out);
	write_one_word((int)Snd.dataLocation, out);
	write_one_word((int)Snd.dataSize,     out);
	write_one_word((int)Snd.dataFormat,   out);
	write_one_word((int)Snd.samplingRate, out);
	write_one_word((int)Snd.channelCount, out);
	write_one_word(*((int *)Snd.info),    out);

	while (1)
	{
		s = zget(in);
		
		if ((feof(in)) || (ferror(in))) break;

		switch (Snd.dataFormat)
		{
			case SND_FORMAT_MULAW_8:
				putc(byte_linear_to_ulaw(s), out);
				break;

			case SND_FORMAT_LINEAR_8:
				putc(s >> 8, out);
				break;

			case SND_FORMAT_LINEAR_16:
				zput(s, out);
				break;
		}
	}

	return(check_io_error(in, out));
}

/*************************************************************************/
/** convert_pvf_to_ulaw():	Converts portable voice format to ulaw.		**/
/*************************************************************************/

static int convert_pvf_to_ulaw(FILE *in, FILE *out)
{
	int sample;

	rewind(in);
	clearerr(in);
	clearerr(out);

	while (1)
	{
		sample = zget(in);
		if(feof(in)) break;
		putc(byte_linear_to_ulaw(sample), out);
	}

	return(check_io_error(in, out));
}

/*************************************************************************/
/** convert_ulaw_to_pvf():	Converts ulaw to portable voice format.		**/
/*************************************************************************/

static int convert_ulaw_to_pvf(FILE *in, FILE *out)
{
	int c;

	clearerr(in);
	clearerr(out);

	while (1)
	{
		if ((c = getc(in)) == EOF) break;

		zput(byte_ulaw_to_linear(c), out);
	}

	return(check_io_error(in, out));
}

/*************************************************************************/
/** put_bits():																			**/
/*************************************************************************/

static void put_bits(int data, int nbits, state_t *s, FILE *out)
{
	int d;

	s->word = (s->word << nbits) | (data & bitmask[nbits]);
	s->nleft += nbits;

	while (s->nleft >= 8)
	{
		d = (s->word >> (s->nleft - 8));
		putc(d & 255, out);
		s->nleft -= 8;
	}
}

/*************************************************************************/
/** get_bits():																			**/
/*************************************************************************/

static int get_bits(int nbits, state_t *state, FILE *in)
{
	int d;

	while (state->nleft < nbits)
	{
		if ((d = getc(in)) == EOF) return(EOF);
		
		state->word   = (state->word << 8) | d;
		state->nleft += 8;
	}

	state->nleft -= nbits;

	return(state->word >> state->nleft) & bitmask[nbits];
}

/*************************************************************************/
/** zput():																					**/
/*************************************************************************/

static void zput(int d, FILE *out)
{
    if (d >  0x7fff) d =  0x7fff;
    if (d < -0x8000) d = -0x8000;
    
    putc((d >> 8) & 0xFF, out);
    putc(d & 0xFF, out);
}

/*************************************************************************/
/** zget():																					**/
/*************************************************************************/

static int zget(FILE *in)
{
	int d;

	d  = (getc(in) & 0xFF) << 8;
	d |= (getc(in) & 0xFF);

	if (d >= 0x8000) d -= 0x10000;

	return(d);
}

/*************************************************************************/
/** write_one_word():																	**/
/*************************************************************************/

static void write_one_word(int w, FILE *out)
{
    putc((w & 0xFF000000) >> 24, out);
    putc((w & 0x00FF0000) >> 16, out);
    putc((w & 0x0000FF00) >>  8, out);
    putc((w & 0x000000FF)      , out);
}

/*************************************************************************/
/** read_one_word():																		**/
/*************************************************************************/

static int read_one_word(FILE *in)
{
	int w;

	w =            getc(in);
	w = (w << 8) | getc(in);
	w = (w << 8) | getc(in);
	w = (w << 8) | getc(in);

	return(w);
}

/*************************************************************************/
/** byte_ulaw_to_linear():	Converts 8 bit ulaw sample to 16 bit linear	**/
/**								sample.													**/
/*************************************************************************/

static int byte_ulaw_to_linear(unsigned char ulawbyte)
{
	static int exp[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };

	int	sign;
	int	exponent;
	int	mantissa;
	int	sample;

	ulawbyte = ~ulawbyte;
	sign		= (ulawbyte & 0x80);
	exponent = (ulawbyte >> 4) & 0x07;
	mantissa = ulawbyte & 0x0F;
	sample	= exp[exponent] + (mantissa << (exponent + 3));

	if (sign != 0) sample = -sample;

	return(sample);
}

/*************************************************************************/
/** byte_linear_to_ulaw():	Converts from signed 16 bit linear to 8 bit	**/
/**								ulaw.														**/
/*************************************************************************/
/** Sample						16 bit linear sample.								**/
/** <RC>							8 bit ulaw sample.									**/
/*************************************************************************/

#define BIAS 0x84				/* Define the add-in bias for 16 bit samples	*/
#define CLIP 32635

static unsigned char byte_linear_to_ulaw(int sample)
{
	static int exp[256] =
	{
		0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
		4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
	};

	unsigned char	ulawbyte;
	int				sign;
	int				exponent;
	int				mantissa;
	
	sign = (sample >> 8) & 0x80;

	if (sign != 0) sample = -sample;

	if (sample > CLIP) sample = CLIP;

	sample	= sample + BIAS;
	exponent = exp[(sample >> 7) & 0xFF];
	mantissa = (sample >> (exponent + 3)) & 0x0F;
	ulawbyte = ~(sign | (exponent << 4) | mantissa);

	return(ulawbyte);
}

/*************************************************************************/
/** test_sample_is_vbox():	Tests if the sample is in vbox format.			**/
/*************************************************************************/

static int test_sample_is_vbox(char *name, int quiet)
{
	struct tm   *timelocal;
	struct stat  status;
	vaheader_t	 header;
	time_t		 timestamp;
	long int		 compression;
	int			 fd;
	char         timestring[256];

	compression = 255;
	
	if ((fd = open(name, O_RDONLY)) != -1)
	{
		if (header_get(fd, &header))
		{
			compression = ntohl(header.compression);

			if ((compression < 2) || (compression > 6)) compression = 0;

			if (fstat(fd, &status) == 0)
			{
				timestamp = ntohl(header.time);
				timelocal = localtime(&timestamp);
				
				if (strftime(timestring, 255, "%c\n", timelocal) == 255)
				{
					printstring(timestring, "???\n");
				}

				if (!quiet)
				{
					fprintf(stdout, "\n");
					fprintf(stdout, "Creation time...........: %s", timestring);
					fprintf(stdout, "Compression.............: %s\n", compressions[compression]);
					fprintf(stdout, "Length..................: %d seconds\n", get_message_ptime(compression, (status.st_size - sizeof(vaheader_t))));
					fprintf(stdout, "Speaker name............: %s\n", header.name);
					fprintf(stdout, "Speaker caller number...: %s\n", header.callerid);
					fprintf(stdout, "Speaker phone number....: %s\n", header.phone);
					fprintf(stdout, "Speaker location........: %s\n", header.location);
					fprintf(stdout, "\n");
				}
			}
		}

		close(fd);
	}
	else fprintf(stderr, "%s: can't open \"%s\" (vbox sample test).\n", vbasename, name);

	return(compression);
}

/*************************************************************************/
/** test_sample_is_au(): Tests if the sample is in sun's au format.	   **/
/*************************************************************************/

static int test_sample_is_au(char *name, int quiet)
{
	SNDSoundStruct snd;

	FILE *in;

	if (!(in = fopen(name, "r")))
	{
		fprintf(stderr, "%s: can't open \"%s\" (au sample test).\n", vbasename, name);
		
		return(255);
	}

	clearerr(in);

	snd.magic			= read_one_word(in);
	snd.dataLocation	= read_one_word(in);
	snd.dataSize		= read_one_word(in);
	snd.dataFormat		= read_one_word(in);
	snd.samplingRate	= read_one_word(in);
	snd.channelCount	= read_one_word(in);

	if ((snd.magic != SND_MAGIC) || (ferror(in) != 0))
	{
		fclose(in);
		return(255);
	}

	fclose(in);

	if ((snd.dataFormat < SUN_FORMAT_MIN) || (snd.dataFormat > SUN_FORMAT_MAX))
	{
		return(255);
	}

	if (!quiet)
	{
		fprintf(stdout, "\n");
		fprintf(stdout, "Format..........: %s\n", sun_format_names[snd.dataFormat]);
		fprintf(stdout, "Sampling rate...: %d\n", snd.samplingRate);
		fprintf(stdout, "Channels........: %d\n", snd.channelCount);
		fprintf(stdout, "Size............: %d %s\n", snd.dataSize, snd.dataSize == -1 ? "(unknown size)" : "bytes");
		fprintf(stdout, "Data location...: %d\n", snd.dataLocation);
		fprintf(stdout, "\n");
	}

	return(128 + snd.dataFormat);
}

/*************************************************************************/
/** version():	Package version.														**/
/*************************************************************************/

static void version(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "%s version %s (%s)\n", vbasename, VERSION, VERDATE);
	fprintf(stderr, "\n");
	
	exit(255);
}

/*************************************************************************/
/** check_io_error():																	**/
/*************************************************************************/

static int check_io_error(FILE *in, FILE *out)
{
	if (ferror(in) != 0)
	{
		fprintf(stderr, "%s: can't read input.\n", vbasename);
		
		return(255);
	}

	if (ferror(out) != 0)
	{
		fprintf(stderr, "%s: can't write output.\n", vbasename);
		
		return(255);
	}

	return(0);
}
