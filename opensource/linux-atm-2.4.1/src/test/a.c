
#include <stdio.h>
#include <signal.h>

void sigint(int);

static int abort_flag=0;

void f1(void)
{
	static int i=0;

	printf("Loop Pass #%d\n", ++i);
}

int main(int argc,char **argv)
{

    signal(SIGINT, sigint);

    while (abort_flag == 0) {
	    f1();
	    sleep(5);
    }
    printf("\nSummary Test ==> OK\n");
}


void sigint(int signo)
{
	abort_flag = 1;
}

