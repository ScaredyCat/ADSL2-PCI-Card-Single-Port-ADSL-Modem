#include "isdnlog.h"

void test_center (char* string)
{
#if 1
	printf("%d\n",CheckTime(string));
	exit(0);
#elif 0
	char File[256];
	int Cnt = 20000;

	sprintf(File,"%s%c%s",confdir(),C_SLASH,USERFILE);

	while(Cnt--)
	{
		utime(File,NULL);
		/* sleep(1); */

		if (read_user_access() != 0)
			Exit(-1);

		if (write_user_access() != 0)
			Exit(-1);
	}
#elif 0
	int Cnt = 200000;
	int Max = 0;
	int add = 0, del = 0;

	while (Cnt--)
	{
		if (socket_size(sockets) == 0 || rand() % 2)
		{
			add++;
			add_socket(&sockets,open("/dev/null",O_WRONLY));
			if (socket_size(sockets) > Max) Max = socket_size(sockets);
		}
		else
		{
			del++;
			del_socket(&sockets,rand() % socket_size(sockets));
		}
	}

	printf("Max: %d, del: %d, add: %d, RAND_MAX: %d\n",Max,del,add,RAND_MAX);

	Exit(-2);
#elif 0
	char User[256];
	char Host[256];

	if (read_user_access() != 0)
		Exit(-1);

	scanf("%s",User);
	scanf("%s",Host);
	printf("User:*%s*\n",User);
	printf("Host:*%s*\n",Host);

	if (user_has_access(User,Host) == -1)
		printf("Rejected\n");
	else
		printf("Accepted\n");

	Exit(-3);
	/* Problem: Wenn Hosts vom Internet in der user-Datei sind, werden
	            diese beim Start von isdnlog auf Gueltigkeit geprueft
	   Loesung: Eintrag dieser Hosts in die /etc/hosts                */
	/* Problem: Wenn Jeder Erdenbuerger von seiner Maschine Zugriff
	            auf meine Maschine haben soll                         */
#elif 0
	char User[256];
	char Host[256];
	char MSN[256];
	int  Flag;

	if (read_user_access() != 0)
		Exit(-1);

	scanf("%s",User);
	scanf("%s",Host);
	scanf("%s",MSN);
	scanf("%d",&Flag);
	printf("User:*%s*\n",User);
	printf("Host:*%s*\n",Host);
	printf("MSN :*%s*\n",MSN);
	printf("Flag:*%d*\n",Flag);

	if (User_Get_Message(User,Host,MSN,Flag) == -1)
		printf("Rejected\n");
	else
		printf("Accepted\n");

	Exit(-3);
#elif 0
	readconfig("isdnlog");
	Exit(-4);
#endif
}
