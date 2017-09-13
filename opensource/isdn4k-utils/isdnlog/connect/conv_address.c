/*
 * ISDN accounting for isdn4linux. 
 *
 * Copyright 1996 by Stefan Luethje (luethje@sl-gw.lake.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*****************************************************************************/

#define _CONV_ADDRESS_C_

/*****************************************************************************/

#include "socket.h"

/*****************************************************************************/

#define ADR_DELIMITER ':'

/*****************************************************************************/

int Set_String(char **Target, char*Source);
int Set_Address(Addresses **Adr, int Anz, char **Array, int *Cnt2);
int Set_String_Field(char ***String, int Anz,char **Array, int *Cnt2);
int Set_PhoneNumber(PhoneNumber **String, int Anz,char **Array, int *Cnt2);
int Set_Date(time_t *NewTime, char *Ptr1);
int Append_Date(char** String, time_t NewTime);
int Append_Integer(char** String, int Append);
int Append_String(char** String, char *Append);

/*****************************************************************************/

#ifdef STANDALONE
int main (int argc, char* argv[])
{
	int len = 5000;
	char buf[5000];
	Address *Ptr;

	while(!feof(stdin))
	{
		fgets(buf,len,stdin);

		if ((Ptr = read_address(buf)) != NULL)
		{
			printf("%s\n",write_address(Ptr));
			free_Address(Ptr);
		}
	}

	return 0;
}
#endif

/*****************************************************************************/

Address* read_address(char* Ptr1)
{
	Address *APtr;
	char   **Array;
	int Cnt = 0;


	Array = String_to_Array(Ptr1, ADR_DELIMITER);

	if (Array == NULL)
	{
		del_Array(Array);
		return NULL;
	}

	if ((APtr = (Address*) calloc(1,sizeof(Address))) == NULL)
	{
		free_Address(APtr);
		del_Array(Array);
		return NULL;
	}

	if (Set_String(&(APtr->NName),Array[Cnt++]))
	{
		free_Address(APtr);
		del_Array(Array);
		return NULL;
	}

	if (Set_String(&(APtr->FName),Array[Cnt++]))
	{
		free_Address(APtr);
		del_Array(Array);
		return NULL;
	}

	if (Array[Cnt])
		APtr->NumAdr = atoi(Array[Cnt++]);
	else
	{
		free_Address(APtr);
		del_Array(Array);
		return NULL;
	}

	if (Set_Address(&(APtr->Adr),APtr->NumAdr,Array,&Cnt))
	{
		free_Address(APtr);
		del_Array(Array);
		return NULL;
	}

	/*
	if (Set_Date(&(APtr->Birthday),Ptr1))
	*/
	if (Set_String(&(APtr->Birthday),Array[Cnt++]))
	{
		free_Address(APtr);
		del_Array(Array);
		return NULL;
	}

	del_Array(Array);
	return APtr;
}

/*****************************************************************************/

char* write_address(Address* Ptr)
{
	int Cnt1, Cnt2;
	int len;
	char *RetCode = NULL;


	if (Append_String(&RetCode,Ptr->NName))
		return NULL;

	if (Append_String(&RetCode,Ptr->FName))
		return NULL;

	if (Append_Integer(&RetCode,Ptr->NumAdr))
		return NULL;

	for (Cnt1 = 0; Cnt1 < Ptr->NumAdr; Cnt1++)
	{
		if (Append_String(&RetCode,Ptr->Adr[Cnt1].Company))
			return NULL;

		if (Append_String(&RetCode,Ptr->Adr[Cnt1].Street))
			return NULL;

		if (Append_String(&RetCode,Ptr->Adr[Cnt1].Country))
			return NULL;

		if (Append_String(&RetCode,Ptr->Adr[Cnt1].PLZ))
			return NULL;

		if (Append_String(&RetCode,Ptr->Adr[Cnt1].City))
			return NULL;

		if (Append_Integer(&RetCode,Ptr->Adr[Cnt1].NumTel))
			return NULL;

		for (Cnt2 = 0; Cnt2 < Ptr->Adr[Cnt1].NumTel; Cnt2++)
		{
			if (Append_String(&RetCode,Ptr->Adr[Cnt1].Tel[Cnt2].Number))
				return NULL;

			if (Append_String(&RetCode,Ptr->Adr[Cnt1].Tel[Cnt2].Alias))
				return NULL;
		}

		if (Append_Integer(&RetCode,Ptr->Adr[Cnt1].NumFax))
			return NULL;

		for (Cnt2 = 0; Cnt2 < Ptr->Adr[Cnt1].NumFax; Cnt2++)
		{
			if (Append_String(&RetCode,Ptr->Adr[Cnt1].Fax[Cnt2].Number))
				return NULL;

			if (Append_String(&RetCode,Ptr->Adr[Cnt1].Fax[Cnt2].Alias))
				return NULL;
		}

		if (Append_Integer(&RetCode,Ptr->Adr[Cnt1].NumEmail))
			return NULL;

		for (Cnt2 = 0; Cnt2 < Ptr->Adr[Cnt1].NumEmail; Cnt2++)
			if (Append_String(&RetCode,Ptr->Adr[Cnt1].Email[Cnt2]))
				return NULL;
	}

	/*
	if (Append_Date(&RetCode,Ptr->Birthday))
	*/
	if (Append_String(&RetCode,Ptr->Birthday))
		return NULL;

	len = strlen(RetCode);
	if (len > 0)
		RetCode[len-1] = '\0';

	return RetCode;
}

/*****************************************************************************/

int Append_Integer(char** String, int Append)
{
	static char NewString[30];

	sprintf(NewString,"%d",Append);

	return Append_String(String,NewString);
}

/*****************************************************************************/

int Append_String(char** String, char *Append)
{
	int len1 = 0;
	int len2 = 0;


	if (Append == NULL)
		Append = "";
	else 
		len2 = strlen(Append);

	if (*String)
	{
		len1 = strlen(*String);
		*String = (char*) realloc(*String,(len1+len2+2)*sizeof(char));
	}
	else
		*String = (char*) calloc(len2+2,sizeof(char));

	if (*String == NULL)
		return -1;

	/*
	strcat(*String,Append);
	*/
	memcpy(*String+len1,Append,len2);
	(*String)[len1+len2] = ADR_DELIMITER;
	(*String)[len1+len2+1] = '\0';

	return 0;
}

/*****************************************************************************/

int Append_Date(char** String, time_t NewTime)
{
	static char RetString[20];
	struct tm *Time;
	
	Time = localtime(&NewTime);
	sprintf(RetString,"%d.%d.%d",Time->tm_mday,Time->tm_mon+1,Time->tm_year+1900);

	return Append_String(String,RetString);
}

/*****************************************************************************/

int Set_Date(time_t *NewTime, char *Ptr)
{
	int Day, Month, Year;
	time_t t_Time;
	struct tm Time;
	static struct tm *CurTime = NULL;


	if (*Ptr == '\0')
		return 0;

	if (sscanf(Ptr,"%d.%d.%d",&Day,&Month,&Year) == 3);
	else
	if (sscanf(Ptr,"%d/%d/%d",&Month,&Day,&Year) == 3);
	else
		return -1;

	Time.tm_sec = 0;
	Time.tm_min = 0;
	Time.tm_hour = 0;
	Time.tm_wday = 0;
	Time.tm_yday = 0;
	Time.tm_isdst = 0;
	Time.tm_mday = Day;
	Time.tm_mon  = Month - 1;

	if (Year < 100)
	{
		if (CurTime == NULL)
		{
			time(&t_Time);
			CurTime = localtime(&t_Time);
		}
		
		Time.tm_year = (CurTime->tm_year/100) * 100 + Year;
	}
	else
		Time.tm_year = Year - 1900;
	
	(*NewTime) = mktime(&Time);

	return 0;
}

/*****************************************************************************/

int Set_PhoneNumber(PhoneNumber **String, int Anz,char **Array, int *Cnt2)
{
	int Cnt = 0;

	if (!Anz)
		return 0;

	if((*String = (PhoneNumber*) calloc(Anz,sizeof(PhoneNumber))) == NULL)
		return -1;

	while (Cnt < Anz)
	{
		if (Set_String(&((*String)[Cnt].Number),Array[(*Cnt2)++]))
			return -1;

		if (Set_String(&((*String)[Cnt].Alias),Array[(*Cnt2)++]))
			return -1;

		Cnt++;
	}

	return 0;
}

/*****************************************************************************/

int Set_String_Field(char ***String, int Anz,char **Array, int *Cnt2)
{
	int Cnt = 0;

	if (!Anz)
		return 0;

	if((*String = (char**) calloc(Anz,sizeof(char*))) == NULL)
		return -1;

	while (Cnt < Anz)
	{
		if (Set_String(&((*String)[Cnt]),Array[(*Cnt2)++]))
			return -1;

		Cnt++;
	}

	return 0;
}

/*****************************************************************************/

int Set_Address(Addresses **Adr, int Anz, char **Array, int *Cnt2)
{
	int Cnt = 0;


	if (!Anz)
		return 0;

	if((*Adr = (Addresses*) calloc(Anz,sizeof(Addresses))) == NULL)
		return -1;

	while (Cnt < Anz)
	{
		if (Set_String(&((*Adr)[Cnt].Company),Array[(*Cnt2)++]))
			return -1;

		if (Set_String(&((*Adr)[Cnt].Street),Array[(*Cnt2)++]))
			return -1;

		if (Set_String(&((*Adr)[Cnt].Country),Array[(*Cnt2)++]))
			return -1;

		if (Set_String(&((*Adr)[Cnt].PLZ),Array[(*Cnt2)++]))
			return -1;

		if (Set_String(&((*Adr)[Cnt].City),Array[(*Cnt2)++]))
			return -1;

		if (Array[*Cnt2])
			(*Adr)[Cnt].NumTel = atoi(Array[(*Cnt2)++]);
		else
			return -1;

		if (Set_PhoneNumber(&((*Adr)[Cnt].Tel),(*Adr)[Cnt].NumTel,Array,Cnt2))
			return -1;

		if (Array[*Cnt2])
			(*Adr)[Cnt].NumFax = atoi(Array[(*Cnt2)++]);
		else
			return -1;

		if (Set_PhoneNumber(&((*Adr)[Cnt].Fax),(*Adr)[Cnt].NumFax,Array,Cnt2))
			return -1;

		if (Array[*Cnt2])
			(*Adr)[Cnt].NumEmail = atoi(Array[(*Cnt2)++]);
		else
			return -1;

		if (Set_String_Field(&((*Adr)[Cnt].Email),(*Adr)[Cnt].NumEmail,Array,Cnt2))
			return -1;

		Cnt++;
	}

	return 0;
}

/*****************************************************************************/

int Set_String(char **Target, char*Source)
{
	int len = strlen(Source);

	if (len && Source)
	{
		if ((*Target = (char*) calloc(len+1,sizeof(char))) == NULL)
			return -1;

		strcpy(*Target,Source);

		if ((*Target)[len-1] == '\n')
			(*Target)[len-1] = '\0';
	}

	return 0;
}

/*****************************************************************************/

void free_Address(Address *APtr)
{
	int Cnt1, Cnt2;


	if (APtr->Adr)
	{
		for(Cnt1 = 0; Cnt1 < APtr->NumAdr; Cnt1++)
		{
			if (APtr->Adr[Cnt1].Tel)
			{
				for(Cnt2 = 0; Cnt2 < APtr->Adr[Cnt1].NumTel; Cnt2++)
				{
					if (APtr->Adr[Cnt1].Tel[Cnt2].Number)
						free(APtr->Adr[Cnt1].Tel[Cnt2].Number);

					if (APtr->Adr[Cnt1].Tel[Cnt2].Alias)
						free(APtr->Adr[Cnt1].Tel[Cnt2].Alias);
				}

				free(APtr->Adr[Cnt1].Tel);
			}

			if (APtr->Adr[Cnt1].Fax)
			{
				for(Cnt2 = 0; Cnt2 < APtr->Adr[Cnt1].NumFax; Cnt2++)
				{
					if (APtr->Adr[Cnt1].Fax[Cnt2].Number)
						free(APtr->Adr[Cnt1].Fax[Cnt2].Number);

					if (APtr->Adr[Cnt1].Fax[Cnt2].Alias)
						free(APtr->Adr[Cnt1].Fax[Cnt2].Alias);
				}

				free(APtr->Adr[Cnt1].Fax);
			}

			if (APtr->Adr[Cnt1].Email)
			{
				for(Cnt2 = 0; Cnt2 < APtr->Adr[Cnt1].NumEmail; Cnt2++)
					if (APtr->Adr[Cnt1].Email[Cnt2])
						free(APtr->Adr[Cnt1].Email[Cnt2]);
			}

			if (APtr->Adr[Cnt1].Company)
				free(APtr->Adr[Cnt1].Company);

			if (APtr->Adr[Cnt1].Street)
				free(APtr->Adr[Cnt1].Street);

			if (APtr->Adr[Cnt1].Country)
				free(APtr->Adr[Cnt1].Country);

			if (APtr->Adr[Cnt1].PLZ)
				free(APtr->Adr[Cnt1].PLZ);

			if (APtr->Adr[Cnt1].City)
				free(APtr->Adr[Cnt1].City);
		}

		free(APtr->Adr);
	}

	if (APtr->FName)
		free(APtr->FName);

	if (APtr->NName)
		free(APtr->NName);

	if (APtr->Birthday)
		free(APtr->Birthday);
}

/*****************************************************************************/

