/*
 *
 * mkcountries.c  1.00  09-Apr-99  14:56
 *
 * (c) 1999 Andreas Kool <akool@isdn4linux.de>
 *
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define COUNTRIES "/usr/lib/isdn/countries.dat"
#define MAXLAND   248

#define  WMAX    64
#define  P        1
#define  Q        1
#define  R        1
#define  DISTANCE 2


typedef struct {
  char *vorwahl;
  char *bez;
  char *ubez;
  char *hint;
  int   used;
} LAND;


static LAND Land[MAXLAND];
static int  nLand = 0;


static int min3(register int x, register int y, register int z)
{
  if (x < y)
    y = x;
  if (y < z)
    z = y;

  return(z);
} /* min */


static int wld(register char *nadel, register char *heuhaufen) /* weighted Levenshtein distance */
{
  register int i, j;
  auto     int l1 = strlen(nadel);
  auto     int l2 = strlen(heuhaufen);
  auto     int dw[WMAX + 1][WMAX + 1];


  dw[0][0] = 0;

  for (j = 1; j <= WMAX; j++)
    dw[0][j] = dw[0][j - 1] + Q;

  for (i = 1; i <= WMAX; i++)
    dw[i][0] = dw[i - 1][0] + R;

  for (i = 1; i <= l1; i++)
    for (j = 1; j <= l2; j++)
      dw[i][j] = min3(dw[i - 1][j - 1] + ((nadel[i - 1] == heuhaufen[j - 1]) ? 0 : P), dw[i][j - 1] + Q, dw[i - 1][j] + R);

  return(dw[l1][l2]);
} /* wld */


static char *deb(register char *s)
{
  register char *p = strchr(s, 0);


  while (*--p == ' ')
    *p = 0;

  p = s;

  while (*p == ' ')
    p++;

  return(p);
} /* deb */


static char *down(register char *s)
{
  register char *p = s;


  while (*p) {
    *p = tolower(*p);
    p++;
  } /* while */

  return(s);
} /* down */


main(int argc, char *argv[], char *envp[])
{
  register char *p, *p1;
  register int   i, match, l, unused = 0;
  auto 	   FILE *f;
  auto 	   char  s[BUFSIZ], s1[BUFSIZ], us[BUFSIZ];


  if (argc > 1) {
    if (!strcmp(argv[1], "-u"))
      unused++;
    else {
      fprintf(stderr, "mkcountries: Usage: mkcountries [ -u ] < Laendertabelle | sort -u >> rate-xx.dat\n");
      exit(1);
    } /* else */
  } /* if */

  if ((f = fopen(COUNTRIES, "r")) != (FILE *)NULL) {
    while (fgets(s, BUFSIZ, f)) {
      if (p = strchr(s, ' ')) {
        *p++ = 0;
        Land[nLand].vorwahl = strdup(s + 2);

        if (p1 = strchr(p, '\n'))
          *p1 = 0;

        if (p1 = strchr(p, '#')) {
          *p1 = 0;
          Land[nLand].hint = down(deb(strdup(p1 + 1)));
        } /* if */

        Land[nLand].bez = deb(strdup(p));
        Land[nLand].used = 0;
        Land[nLand++].ubez = down(deb(strdup(p)));
      } /* if */
    } /* while */

    fclose(f);

    while (gets(s)) {

      strcpy(s1, deb(s));

      if (*s1 && (*s != '#')) {
        strcpy(us, s1);
	(void)down(us);

        match = 0;

        for (i = 0; i < nLand; i++)
       	  if (strstr(Land[i].ubez, us)) {
            match = 1;
            break;
       	  } /* if */

        if (!match)
          for (i = 0; i < nLand; i++)
            if ((l = wld(us, Land[i].ubez)) <= DISTANCE) {
              match = 2;
	      break;
       	    } /* if */

        if (!match)
          for (i = 0; i < nLand; i++)
            if (Land[i].hint && strstr(Land[i].hint, us)) {
              match = 3;
	      break;
       	    } /* if */

        if (match) {
          Land[i].used++;

          printf("A:+%s%*s# %s", Land[i].vorwahl, 6 - strlen(Land[i].vorwahl), "", Land[i].bez);

          if (match == 2)
            printf(" (FUZZ %d:``%s'' ~ ``%s'')", l, s1, Land[i].bez);

          printf("\n");
        }
        else
          printf("# UNKNOWN: %s\n", s1);
      } /* if */
    } /* while */

    if (unused) {
      printf("\n\n# UNUSED country-codes:\n");

      for (i = 0; i < nLand; i++)
        if (!Land[i].used)
          printf("A:+%s%*s# %s\n", Land[i].vorwahl, 6 - strlen(Land[i].vorwahl), "", Land[i].bez);

      printf("\n\n# more than once used country-codes:\n");

      for (i = 0; i < nLand; i++)
        if (Land[i].used > 1)
          printf("A:+%s%*s# %s (%d times)\n", Land[i].vorwahl, 6 - strlen(Land[i].vorwahl), "", Land[i].bez, Land[i].used);
    } /* if */
  } /* if */

  exit(0);
} /* main */

#if 0
  Problemf„lle:

  Grossbritannien          -> Großbritannien und Nordirland
  USA/Hawaii 		   -> Vereinigte Staaten (USA)
  Färöer Inseln		   -> Färöer
  Tschechische Republik    -> Tschechien
  Moldawien    		   -> Moldau Republik
  Russland		   -> GUS (Russische Föderation)
  Weissrussland	           -> Weißrußland (Belarus)
  Frz. Polynesien          -> Französisch-Polynesien
  Diego Garcia	           -> ???
  Guantanamo	           -> ???
  Jungferninseln (Am.)     -> Amerikanische Jungferninseln
  Jungferninseln (Brit.)   -> Britische Jungferninseln
  Korea (Nord)	 	   -> Korea (Republik) ::: Korea (Demokratische Republik)
  Norfolk Inseln	   -> Norfolkinseln (Australien)
  Solomon Inseln	   -> ???
  Turks und Caicos	   -> Turks und Caicos
  Wallis und Futuna Inseln -> ???
  Slowakische Republik	   -> Slowakei
  Jugoslawien (Serbien und Montenegro) -> Jugoslawien Montenegro
  Mazedonien (ehem. jugoslawische Republik) -> Mazedonien
#endif
