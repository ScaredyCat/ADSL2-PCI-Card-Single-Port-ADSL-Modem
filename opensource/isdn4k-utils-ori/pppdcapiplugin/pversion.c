#include <stdio.h>

int main(int ac, char *av[])
{
   char *s;
   int v1 = 0;
   int v2 = 0;
   int v3 = 0;
   int v4 = 0;

   if (ac != 2) return 1;
   switch (sscanf(av[1], "%d.%d.%db%d", &v1, &v2, &v3, &v4)) {
      case 3:
      case 4:
	 printf("%d\n",
		v1 * 1000000 +
		v2 * 10000 +
		v3 * 100 +
		v4);
	 return 0;
   }
   return 2;
}
