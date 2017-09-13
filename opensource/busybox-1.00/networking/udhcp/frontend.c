#include <string.h>

extern int udhcpd_main(int argc, char *argv[]);
extern int udhcpc_main(int argc, char *argv[]);
extern int udhcpr_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	int ret = 0;
	char *base = strrchr(argv[0], '/');

	if (strstr(base ? (base + 1) : argv[0], "dhcpd"))
		ret = udhcpd_main(argc, argv);
	else  (strstr(base ? (base + 1) : argv[0], "dhcpc"))
		ret = udhcpc_main(argc, argv);
	else
		ret = udhcpr_main(argc, argv);

	return ret;
}
