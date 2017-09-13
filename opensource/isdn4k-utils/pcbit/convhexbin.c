#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

/*
 * Copyright (c) 1993, 1994, 1995, INESC. All Rights Reserved.
 * Licensed Material-Property of INESC
 * 
 */

int hexbin(FILE *, unsigned char *, int, int);

int
convhexbin (char *filename, unsigned char *buf, int size)
{
   FILE *fpr;
   int retval;
   register int lastmem;
   char str[100];

   /* Abre ficheiro de entrada */
   if ((fpr=fopen (filename,"rb")) == NULL)
    return (-1);

   /* le primeira linha do ficheiro stpd */
   fgets(str, 80, fpr);

  /* Poe buf a 0x00 */
  for (lastmem=0 ; lastmem < size ; lastmem++)
    buf[lastmem]=0x00;

  /* Converte para binario */
  retval = hexbin(fpr, buf, 0x100000-1024, 0xfffff);

  fclose (fpr);
  return retval;
}

int
hexbin(FILE *fpr, unsigned char *ar256k, int firstend, int lastend)
{

   unsigned char str [80], i, cc;
   u_int linha, nn, xx, nnn, pp;
   int lastmem, ind, end, ss;

   ind = 0;

   linha=0; lastmem=0; ss=0;

   while (fgets((char *)str, 80, fpr) != NULL) {
      linha++;
      if (str[0] == 26)
         break;

      if (str[0] != ':' || str[7] != '0') {
         printf("Error in file input format\n");
         printf("%c - unknown format in line %u\n", str[0], linha);
         fclose(fpr);
         exit(2);
      }

      sscanf((char *)&str[1], "%2x", &nn);
      cc=nn;

      switch (str[8]) {
         case '0' :
            nnn=(nn<<1)+9;
            sscanf((char *)&str[3], "%4x", &pp);
            cc+=pp+(pp>>8);
            end=ss+pp;
            for (i=9 ; i<nnn; i+= 2) {
               sscanf((char *)&str[i], "%2x", &xx);
               if ((end >= firstend) && (end <= lastend)) {
                  ind=end-firstend;
                  if (ind > lastmem)
                     lastmem=ind;

                  ar256k[ind]=xx;
               }
               ind++;
               end++;
               cc+=xx;
            }
            sscanf((char *)&str[i], "%2x", &xx);
            cc+=xx;
            if (cc) {
               printf("CHECK SUM Error in line %u\n", linha);
               fclose(fpr);
               exit(2);
            }
            break;

         case '1' :
            for (i=3 ; i<11 ; i+=2) {
               sscanf((char *)&str[i],"%2x",&xx);
               cc+=xx;
            }
            if (cc) {
               printf("CHECK SUM Error in line %u\n", linha);
               fclose(fpr);
               exit(2);
            }
            break;

         case '2' :
            ss=0;
            sscanf((char *)&str[9], "%4X", &ss);
            ss<<=4;
            for (i=3; i<15; i+= 2) {
               sscanf((char *)&str[i], "%2x", &xx);
               cc+=xx;
            }
            if (cc) {
               printf("CHECK SUM Error in line %u\n",linha);
               fclose(fpr);
               exit(2);
            }
            break;

         case '3' :
            break;

         default :
            printf("Error in file input format\n");
            printf("Type %c - unknown type in line %u\n", str[8], linha);
            fclose(fpr);
            exit(2);
      }
   }
   return 0;
}




