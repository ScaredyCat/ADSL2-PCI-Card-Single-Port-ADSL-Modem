#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

static int		tc_modified = 0;
static struct termios	oldset;

static void
echo_disable(void)
{
	struct termios newset;

	if (tcgetattr(STDIN_FILENO, &oldset) == 0 && (oldset.c_lflag & ECHO)) {
		tc_modified = 1;
		newset = oldset;
		newset.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
		(void) tcsetattr(STDIN_FILENO, TCSANOW, &newset);
	}
	return;
}

static void
echo_restore(void)
{
	if (tc_modified != 0) {
		tcsetattr(STDIN_FILENO, TCSANOW, &oldset);
		tc_modified = 0;
	}
}

static int
readnoecho(char* buf, int maxl)
{
	char c = 0;
	int n, i = 0;

	echo_disable();
	while (c != '\n') {
		n = read(STDIN_FILENO, &c, 1);
		if (n == -1 && (errno == EAGAIN || errno == EINTR))
			continue;
		if (n != 1)
			break;
		if (c == '\r')
			break;
		if (c == '\n')
			break;
		if (i < maxl)
			buf[i++] = c;
	}
	buf[i] = '\0';
	echo_restore();
	fprintf(stderr, "\n");
	fflush(stderr);
	return i;
}

int
readpassw(char *buf, int maxlen)
{
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, "password:");
	fflush(stderr);
	return(readnoecho(buf, maxlen));
}

