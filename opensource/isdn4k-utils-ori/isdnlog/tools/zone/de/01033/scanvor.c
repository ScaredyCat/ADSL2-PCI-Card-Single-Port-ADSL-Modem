#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct stName {
	unsigned short nVorwahl;
	char *szName;
} *pNamen = NULL;
int nNamen = 0;

char *getname(unsigned short nVorwahl)
{
	int i;

	if(!nNamen)
	{
		FILE *fp;
		char szBuffer[512];
		char *cp;
		fp = fopen("Verzonung.dat", "r");
		if(fp == NULL)
		{
			nNamen = -1;
			return "";
		}
		
		while(fgets(szBuffer, sizeof(szBuffer), fp) != NULL)
		{
			pNamen = (struct stName *)realloc(pNamen, (nNamen+1) * sizeof(struct stName));
			cp = strchr(szBuffer, ';');
			if(cp != NULL)
			{
				*cp++= '\0';
				pNamen[nNamen].nVorwahl = atoi(szBuffer);
				cp = strchr(cp, ';');
				if(cp != NULL)
				{
					cp = pNamen[nNamen].szName = strdup(++cp);
					while(*cp && *cp != '\n' && *cp != '\r')
						cp++;
					*cp = '\0';
					nNamen++;
				}
			}
		}
		fclose(fp);
	}
	
	for(i=0; i<nNamen; i++)
	{
		if(pNamen[i].nVorwahl == nVorwahl)
			return pNamen[i].szName;
	}
	
	return "unbekannt";
}

int main(int argc, char **argv)
{
	FILE *fp = fopen("/usr/lib/isdn/vorwahl.dat", "rb");
	unsigned short nAnzahl, nHilf;
	unsigned short nAnzCity, nAnzFern;
	unsigned short *pVorwahlen;
	unsigned long *lOffset;
	int i,j;
	
	if(fp == NULL)
	{
		printf("File not found\n");
		exit(0);
	}
	
	fread(&nAnzahl, sizeof(nAnzahl), 1, fp);
	pVorwahlen = (unsigned short *)malloc(nAnzahl * sizeof(unsigned short));
	lOffset = (unsigned long *)malloc(nAnzahl * sizeof(unsigned long));
	fseek(fp, 3, SEEK_SET);
	fread(lOffset, nAnzahl, sizeof(unsigned long), fp);
	fseek(fp, 3+4*nAnzahl, SEEK_SET);
	fread(pVorwahlen, nAnzahl, sizeof(unsigned short), fp);
	
	for(i=0; i<nAnzahl; i++)
	{
		pVorwahlen[i] += 32768;
		printf("V:%u", pVorwahlen[i]);
		printf(":%s", getname(pVorwahlen[i]));
		fseek(fp, lOffset[i]-1, SEEK_SET);
		fread(&nAnzCity, 1, sizeof(nAnzCity), fp);
		printf(":C:");
		for(j=0; j<nAnzCity; j++)
		{
			fread(&nHilf, 1, sizeof(nHilf), fp);
			nHilf += 32768;
			printf("%s%u", j?",":"", nHilf);
		}
		fread(&nAnzFern, 1, sizeof(nAnzFern), fp);
		printf(":R:");
		for(j=0; j<nAnzFern; j++)
		{
			fread(&nHilf, 1, sizeof(nHilf), fp);
			nHilf += 32768;
			printf("%s%u", j?",":"", nHilf);
		}
		printf("\n");
	}
	
	free(pVorwahlen);
	free(lOffset);
	fclose(fp);
	
	for(i=0; i<nNamen; i++)
		free(pNamen[i].szName);
	free(pNamen);
}