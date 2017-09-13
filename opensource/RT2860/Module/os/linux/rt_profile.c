#include "rt_config.h"

static void HTParametersHook(
	IN	PRTMP_ADAPTER pAd, 
	IN	CHAR		  *pValueStr,
	IN	CHAR		  *pInput);


#define ETH_MAC_ADDR_STR_LEN 17  // in format of xx:xx:xx:xx:xx:xx

// We assume the s1 is a sting, s2 is a memory space with 6 bytes. and content of s1 will be changed.
BOOLEAN rtstrmactohex(char *s1, char *s2)
{
	int i = 0;
	char *ptokS = s1, *ptokE = s1;

	if (strlen(s1) != ETH_MAC_ADDR_STR_LEN)
		return FALSE;

	while((*ptokS) != '\0')
	{
		if((ptokE = strchr(ptokS, ':')) != NULL)
			*ptokE++ = '\0';
		if ((strlen(ptokS) != 2) || (!isxdigit(*ptokS)) || (!isxdigit(*(ptokS+1))))
			break; // fail
		AtoH(ptokS, &s2[i++], 1);
		ptokS = ptokE;
		if (i == 6)
			break; // parsing finished
	}

	return ( i == 6 ? TRUE : FALSE);

}


// we assume the s1 and s2 both are strings.
BOOLEAN rtstrcasecmp(char *s1, char *s2)
{
	char *p1 = s1, *p2 = s2;
	
	if (strlen(s1) != strlen(s2))
		return FALSE;
	
	while(*p1 != '\0')
	{
		if((*p1 != *p2) && ((*p1 ^ *p2) != 0x20))
			return FALSE;
		p1++;
		p2++;
	}
	
	return TRUE;
}

// we assume the s1 (buffer) and s2 (key) both are strings.
char * rtstrstruncasecmp(char * s1, char * s2)
{
	INT l1, l2, i;
	char temp1, temp2;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;

	l1 = strlen(s1);

	while (l1 >= l2)
	{
		l1--;

		for(i=0; i<l2; i++)
		{
			temp1 = *(s1+i);
			temp2 = *(s2+i);

			if (('a' <= temp1) && (temp1 <= 'z'))
				temp1 = 'A'+(temp1-'a');
			if (('a' <= temp2) && (temp2 <= 'z'))
				temp2 = 'A'+(temp2-'a');

			if (temp1 != temp2)
				break;
		}

		if (i == l2)
			return (char *) s1;

		s1++;
	}
	
	return NULL; // not found
}

//add by kathy

 /**
  * strstr - Find the first substring in a %NUL terminated string
  * @s1: The string to be searched
  * @s2: The string to search for
  */
char * rtstrstr(const char * s1,const char * s2)
{
	INT l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;
	
	l1 = strlen(s1);
	
	while (l1 >= l2)
	{
		l1--;
		if (!memcmp(s1,s2,l2))
			return (char *) s1;
		s1++;
	}
	
	return NULL;
}
 
/**
 * rstrtok - Split a string into tokens
 * @s: The string to be searched
 * @ct: The characters to search for
 * * WARNING: strtok is deprecated, use strsep instead. However strsep is not compatible with old architecture.
 */
char * __rstrtok;
char * rstrtok(char * s,const char * ct)
{
	char *sbegin, *send;

	sbegin  = s ? s : __rstrtok;
	if (!sbegin)
	{
		return NULL;
	}

	sbegin += strspn(sbegin,ct);
	if (*sbegin == '\0')
	{
		__rstrtok = NULL;
		return( NULL );
	}

	send = strpbrk( sbegin, ct);
	if (send && *send != '\0')
		*send++ = '\0';

	__rstrtok = send;

	return (sbegin);
}

/**
 * delimitcnt - return the count of a given delimiter in a given string.
 * @s: The string to be searched.
 * @ct: The delimiter to search for.
 * Notice : We suppose the delimiter is a single-char string(for example : ";").
 */
INT delimitcnt(char * s,const char * ct)
{
	INT count = 0;
	/* point to the beginning of the line */
	const char *token = s; 

	for ( ;; )
	{
		token = strpbrk(token, ct); /* search for delimiters */

        if ( token == NULL )
		{
			/* advanced to the terminating null character */
			break; 
		}
		/* skip the delimiter */
	    ++token; 

		/*
		 * Print the found text: use len with %.*s to specify field width.
		 */
        DBGPRINT(RT_DEBUG_INFO, (" -> \"%.*s\"\n", (INT)(token - s), token));
		/* accumulate delimiter count */
	    ++count; 
	}
    return count;
}

#ifdef CONFIG_AP_SUPPORT
/*
  * converts the Internet host address from the standard numbers-and-dots notation
  * into binary data.
  * returns nonzero if the address is valid, zero if not.	
  */
static int rtinet_aton(const char *cp, unsigned int *addr)
{
	unsigned int 	val;
	int         	base, n;
	char        	c;
	unsigned int    parts[4];
	unsigned int    *pp = parts;

	for (;;)
    {
         /*
          * Collect number up to ``.''. 
          * Values are specified as for C: 
          *	0x=hex, 0=octal, other=decimal.
          */
         val = 0;
         base = 10;
         if (*cp == '0')
         {
             if (*++cp == 'x' || *cp == 'X')
                 base = 16, cp++;
             else
                 base = 8;
         }
         while ((c = *cp) != '\0')
         {
             if (isdigit((unsigned char) c))
             {
                 val = (val * base) + (c - '0');
                 cp++;
                 continue;
             }
             if (base == 16 && isxdigit((unsigned char) c))
             {
                 val = (val << 4) +
                     (c + 10 - (islower((unsigned char) c) ? 'a' : 'A'));
                 cp++;
                 continue;
             }
             break;
         }
         if (*cp == '.')
         {
             /*
              * Internet format: a.b.c.d a.b.c   (with c treated as 16-bits)
              * a.b     (with b treated as 24 bits)
              */
             if (pp >= parts + 3 || val > 0xff)
                 return 0;
             *pp++ = val, cp++;
         }
         else
             break;
     }
 
     /*
      * Check for trailing junk.
      */
     while (*cp)
         if (!isspace((unsigned char) *cp++))
             return 0;
 
     /*
      * Concoct the address according to the number of parts specified.
      */
     n = pp - parts + 1;
     switch (n)
     {
 
         case 1:         /* a -- 32 bits */
             break;
 
         case 2:         /* a.b -- 8.24 bits */
             if (val > 0xffffff)
                 return 0;
             val |= parts[0] << 24;
             break;
 
         case 3:         /* a.b.c -- 8.8.16 bits */
             if (val > 0xffff)
                 return 0;
             val |= (parts[0] << 24) | (parts[1] << 16);
             break;
 
         case 4:         /* a.b.c.d -- 8.8.8.8 bits */
             if (val > 0xff)
                 return 0;
             val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
             break;
     }
	      
     *addr = htonl(val);
     return 1;

}
#endif // CONFIG_AP_SUPPORT //
/*
    ========================================================================

    Routine Description:
        Find key section for Get key parameter.

    Arguments:
        buffer                      Pointer to the buffer to start find the key section
        section                     the key of the secion to be find

    Return Value:
        NULL                        Fail
        Others                      Success
    ========================================================================
*/
PUCHAR  RTMPFindSection(
    IN  PCHAR   buffer)
{
    CHAR temp_buf[32];
    PUCHAR  ptr;

    strcpy(temp_buf, "Default");

    if((ptr = rtstrstr(buffer, temp_buf)) != NULL)
            return (ptr+strlen("\n"));
        else
            return NULL;
}

/*
    ========================================================================

    Routine Description:
        Get key parameter.

    Arguments:
        key                         Pointer to key string
        dest                        Pointer to destination      
        destsize                    The datasize of the destination
        buffer                      Pointer to the buffer to start find the key

    Return Value:
        TRUE                        Success
        FALSE                       Fail

    Note:
        This routine get the value with the matched key (case case-sensitive)
    ========================================================================
*/
INT RTMPGetKeyParameter(
    IN  PCHAR   key,
    OUT PCHAR   dest,   
    IN  INT     destsize,
    IN  PCHAR   buffer)
{
    UCHAR *temp_buf1 = NULL;
    UCHAR *temp_buf2 = NULL;
    CHAR *start_ptr;
    CHAR *end_ptr;
    CHAR *ptr;
    CHAR *offset = 0;
    INT  len;

	//temp_buf1 = kmalloc(MAX_PARAM_BUFFER_SIZE, MEM_ALLOC_FLAG);
	os_alloc_mem(NULL, &temp_buf1, MAX_PARAM_BUFFER_SIZE);

	if(temp_buf1 == NULL)
        return (FALSE);
	
	//temp_buf2 = kmalloc(MAX_PARAM_BUFFER_SIZE, MEM_ALLOC_FLAG);
	os_alloc_mem(NULL, &temp_buf2, MAX_PARAM_BUFFER_SIZE);
	if(temp_buf2 == NULL)
	{
		os_free_mem(NULL, temp_buf1);
        return (FALSE);
	}
	
    //find section
    if((offset = RTMPFindSection(buffer)) == NULL)
    {
    	os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    strcpy(temp_buf1, "\n");
    strcat(temp_buf1, key);
    strcat(temp_buf1, "=");

    //search key
    if((start_ptr=rtstrstr(offset, temp_buf1))==NULL)
    {
		os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    start_ptr+=strlen("\n");
    if((end_ptr=rtstrstr(start_ptr, "\n"))==NULL)
       end_ptr=start_ptr+strlen(start_ptr);

    if (end_ptr<start_ptr)
    {
		os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    NdisMoveMemory(temp_buf2, start_ptr, end_ptr-start_ptr);
    temp_buf2[end_ptr-start_ptr]='\0';
    len = strlen(temp_buf2);
    strcpy(temp_buf1, temp_buf2);
    if((start_ptr=rtstrstr(temp_buf1, "=")) == NULL)
    {
		os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    strcpy(temp_buf2, start_ptr+1);
    ptr = temp_buf2;
    //trim space or tab
    while(*ptr != 0x00)
    {
        if( (*ptr == ' ') || (*ptr == '\t') )
            ptr++;
        else
           break;
    }

    len = strlen(ptr);    
    memset(dest, 0x00, destsize);
    strncpy(dest, ptr, len >= destsize ?  destsize: len);

	os_free_mem(NULL, temp_buf1);
    os_free_mem(NULL, temp_buf2);
    return TRUE;
}

/*
    ========================================================================

    Routine Description:
        Get key parameter.

    Arguments:
        key                         Pointer to key string
        dest                        Pointer to destination      
        destsize                    The datasize of the destination
        buffer                      Pointer to the buffer to start find the key

    Return Value:
        TRUE                        Success
        FALSE                       Fail

    Note:
        This routine get the value with the matched key (case case-sensitive).
        It is called for parsing SSID and any key string.
    ========================================================================
*/
INT RTMPGetCriticalParameter(
    IN  PCHAR   key,
    OUT PCHAR   dest,   
    IN  INT     destsize,
    IN  PCHAR   buffer)
{
    UCHAR *temp_buf1 = NULL;
    UCHAR *temp_buf2 = NULL;
    CHAR *start_ptr;
    CHAR *end_ptr;
    CHAR *ptr;
    CHAR *offset = 0;
    INT  len;

	//temp_buf1 = kmalloc(MAX_PARAM_BUFFER_SIZE, MEM_ALLOC_FLAG);
	os_alloc_mem(NULL, &temp_buf1, MAX_PARAM_BUFFER_SIZE);

	if(temp_buf1 == NULL)
        return (FALSE);
	
	//temp_buf2 = kmalloc(MAX_PARAM_BUFFER_SIZE, MEM_ALLOC_FLAG);
	os_alloc_mem(NULL, &temp_buf2, MAX_PARAM_BUFFER_SIZE);
	if(temp_buf2 == NULL)
	{
		os_free_mem(NULL, temp_buf1);
        return (FALSE);
	}
	
    //find section
    if((offset = RTMPFindSection(buffer)) == NULL)
    {
    	os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    strcpy(temp_buf1, "\n");
    strcat(temp_buf1, key);
    strcat(temp_buf1, "=");

    //search key
    if((start_ptr=rtstrstr(offset, temp_buf1))==NULL)
    {
		os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    start_ptr+=strlen("\n");
    if((end_ptr=rtstrstr(start_ptr, "\n"))==NULL)
       end_ptr=start_ptr+strlen(start_ptr);

    if (end_ptr<start_ptr)
    {
		os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    NdisMoveMemory(temp_buf2, start_ptr, end_ptr-start_ptr);
    temp_buf2[end_ptr-start_ptr]='\0';
    len = strlen(temp_buf2);
    strcpy(temp_buf1, temp_buf2);
    if((start_ptr=rtstrstr(temp_buf1, "=")) == NULL)
    {
		os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    strcpy(temp_buf2, start_ptr+1);
    ptr = temp_buf2;

    //trim tab
    /* We cannot trim space(' ') for SSID and key string. */
    while(*ptr != 0x00)
    {
        //if( (*ptr == ' ') || (*ptr == '\t') )
        if( (*ptr == '\t') )
            ptr++;
        else
           break;
    }

    len = strlen(ptr);    
    memset(dest, 0x00, destsize);
    strncpy(dest, ptr, len >= destsize ?  destsize: len);

	os_free_mem(NULL, temp_buf1);
    os_free_mem(NULL, temp_buf2);
    return TRUE;
}

/*
    ========================================================================

    Routine Description:
        Get multiple key parameter.

    Arguments:
        key                         Pointer to key string
        dest                        Pointer to destination      
        destsize                    The datasize of the destination
        buffer                      Pointer to the buffer to start find the key

    Return Value:
        TRUE                        Success
        FALSE                       Fail

    Note:
        This routine get the value with the matched key (case case-sensitive)
    ========================================================================
*/
INT RTMPGetKeyParameterWithOffset(
    IN  PCHAR   key,
    OUT PCHAR   dest,   
    OUT	USHORT	*end_offset,		
    IN  INT     destsize,
    IN  PCHAR   buffer,
    IN	BOOLEAN	bTrimSpace)
{
    UCHAR *temp_buf1 = NULL;
    UCHAR *temp_buf2 = NULL;
    CHAR *start_ptr;
    CHAR *end_ptr;
    CHAR *ptr;
    CHAR *offset = 0;
    INT  len;

	if (*end_offset >= MAX_INI_BUFFER_SIZE)
		return (FALSE);
	
	os_alloc_mem(NULL, &temp_buf1, MAX_PARAM_BUFFER_SIZE);

	if(temp_buf1 == NULL)
        return (FALSE);
		
	os_alloc_mem(NULL, &temp_buf2, MAX_PARAM_BUFFER_SIZE);
	if(temp_buf2 == NULL)
	{
		os_free_mem(NULL, temp_buf1);
        return (FALSE);
	}
	
    //find section		
	if(*end_offset == 0)
    {
		if ((offset = RTMPFindSection(buffer)) == NULL)
		{
			os_free_mem(NULL, temp_buf1);
	    	os_free_mem(NULL, temp_buf2);
    	    return (FALSE);
		}
    }
	else
		offset = buffer + (*end_offset);	
		
    strcpy(temp_buf1, "\n");
    strcat(temp_buf1, key);
    strcat(temp_buf1, "=");

    //search key
    if((start_ptr=rtstrstr(offset, temp_buf1))==NULL)
    {
		os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    start_ptr+=strlen("\n");
    if((end_ptr=rtstrstr(start_ptr, "\n"))==NULL)
       end_ptr=start_ptr+strlen(start_ptr);
	
    if (end_ptr<start_ptr)
    {
		os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

	*end_offset = end_ptr - buffer;

    NdisMoveMemory(temp_buf2, start_ptr, end_ptr-start_ptr);
    temp_buf2[end_ptr-start_ptr]='\0';
    len = strlen(temp_buf2);
    strcpy(temp_buf1, temp_buf2);
    if((start_ptr=rtstrstr(temp_buf1, "=")) == NULL)
    {
		os_free_mem(NULL, temp_buf1);
    	os_free_mem(NULL, temp_buf2);
        return (FALSE);
    }

    strcpy(temp_buf2, start_ptr+1);
    ptr = temp_buf2;
    //trim space or tab
    while(*ptr != 0x00)
    {
        if((bTrimSpace && (*ptr == ' ')) || (*ptr == '\t') )
            ptr++;
        else
           break;
    }

    len = strlen(ptr);    
    memset(dest, 0x00, destsize);
    strncpy(dest, ptr, len >= destsize ?  destsize: len);

	os_free_mem(NULL, temp_buf1);
    os_free_mem(NULL, temp_buf2);
    return TRUE;
}


static int rtmp_parse_key_buffer_from_file(IN  PRTMP_ADAPTER pAd,IN  char *buffer,IN  ULONG KeyType,IN  INT BSSIdx,IN  INT KeyIdx)
{
	PUCHAR		keybuff;
	INT			i = BSSIdx, idx = KeyIdx;
	ULONG		KeyLen;
	UCHAR		CipherAlg = CIPHER_WEP64;
	
	keybuff = buffer;
	KeyLen = strlen(keybuff);

	if (KeyType == 1)
	{//Ascii								
		if( (KeyLen == 5) || (KeyLen == 13))
		{
			pAd->SharedKey[i][idx].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[i][idx].Key, keybuff, KeyLen);
			if (KeyLen == 5)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			pAd->SharedKey[i][idx].CipherAlg = CipherAlg;
	
			DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Key%dStr=%s and type=%s\n", i, idx+1, keybuff, (KeyType == 0) ? "Hex":"Ascii"));		
			return 1;
		}
		else
		{//Invalid key length
			DBGPRINT(RT_DEBUG_ERROR, ("Key%dStr is Invalid key length! KeyLen = %ld!\n", idx+1, KeyLen));
			return 0;
		}
	}
	else
	{//Hex type
		if( (KeyLen == 10) || (KeyLen == 26))
		{
			pAd->SharedKey[i][idx].KeyLen = KeyLen / 2;
			AtoH(keybuff, pAd->SharedKey[i][idx].Key, KeyLen / 2);
			if (KeyLen == 10)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			pAd->SharedKey[i][idx].CipherAlg = CipherAlg;

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Key%dStr=%s and type=%s\n", i, idx+1, keybuff, (KeyType == 0) ? "Hex":"Ascii"));
			return 1;
		}
		else
		{//Invalid key length
			DBGPRINT(RT_DEBUG_ERROR, ("I/F(ra%d) Key%dStr is Invalid key length! KeyLen = %ld!\n", i, idx+1, KeyLen));
			return 0;
		}
	}
}
static void rtmp_read_key_parms_from_file(IN  PRTMP_ADAPTER pAd, char *tmpbuf, char *buffer)
{
	char		tok_str[16];
	PUCHAR		macptr;						
	INT			i = 0, idx;
	ULONG		KeyType[MAX_MBSSID_NUM];
	ULONG		KeyIdx;

	NdisZeroMemory(KeyType, MAX_MBSSID_NUM);

	//DefaultKeyID
	if(RTMPGetKeyParameter("DefaultKeyID", tmpbuf, 25, buffer))
	{
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
			{
				if (i >= pAd->ApCfg.BssidNum)
				{
					break;
				}

				KeyIdx = simple_strtol(macptr, 0, 10);
				if((KeyIdx >= 1 ) && (KeyIdx <= 4))
					pAd->ApCfg.MBSSID[i].DefaultKeyId = (UCHAR) (KeyIdx - 1 );
				else
					pAd->ApCfg.MBSSID[i].DefaultKeyId = 0;

				DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) DefaultKeyID(0~3)=%d\n", i, pAd->ApCfg.MBSSID[i].DefaultKeyId));
			}
		}
#endif // CONFIG_AP_SUPPORT //

	}	   


	for (idx = 0; idx < 4; idx++)
	{
		sprintf(tok_str, "Key%dType", idx + 1);
		//Key1Type
		if (RTMPGetKeyParameter(tok_str, tmpbuf, 128, buffer))
		{
		    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
		    {
			    KeyType[i] = simple_strtol(macptr, 0, 10);
		    }
#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			{
				if (TRUE)
				{
					BOOLEAN bKeyxStryIsUsed = FALSE;
					DBGPRINT(RT_DEBUG_TRACE, ("pAd->ApCfg.BssidNum=%d\n", pAd->ApCfg.BssidNum));

					for (i = 0; i < pAd->ApCfg.BssidNum; i++)
			        	{
						sprintf(tok_str, "Key%dStr%d", idx + 1, i + 1);
					if (RTMPGetCriticalParameter(tok_str, tmpbuf, 128, buffer))
						{
							rtmp_parse_key_buffer_from_file(pAd, tmpbuf, KeyType[i], i, idx);

							if (bKeyxStryIsUsed == FALSE)
							{
								bKeyxStryIsUsed = TRUE;
							}						
						}
					}

					if (bKeyxStryIsUsed == FALSE)
					{
						sprintf(tok_str, "Key%dStr", idx + 1);
					if (RTMPGetCriticalParameter(tok_str, tmpbuf, 128, buffer))
						{
							if (pAd->ApCfg.BssidNum == 1)
							{
								rtmp_parse_key_buffer_from_file(pAd, tmpbuf, KeyType[BSS0], BSS0, idx);
							}
							else
							{
								// Anyway, we still do the legacy dissection of the whole KeyxStr string.
							    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
							    {
									rtmp_parse_key_buffer_from_file(pAd, macptr, KeyType[i], i, idx);
							    }
							}
						}
					}
				}
			}
#endif // CONFIG_AP_SUPPORT //

		}
	}
}

#ifdef CONFIG_AP_SUPPORT 

#ifdef APCLI_SUPPORT
static void rtmp_read_ap_client_from_file(
	IN PRTMP_ADAPTER pAd,
	IN char *tmpbuf,
	IN char *buffer)
{
	PUCHAR		macptr;
	INT			i=0, j=0, idx;
	UCHAR		macAddress[MAC_ADDR_LEN];
	UCHAR		keyMaterial[40];
	PAPCLI_STRUCT   pApCliEntry = NULL;
	ULONG		KeyIdx;
	char		tok_str[16];
	ULONG		KeyType[MAX_APCLI_NUM];
	ULONG		KeyLen;
	UCHAR		CipherAlg = CIPHER_WEP64;

	//ApCliEnable
	if(RTMPGetKeyParameter("ApCliEnable", tmpbuf, 128, buffer))
	{
		for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_APCLI_NUM); macptr = rstrtok(NULL,";"), i++)
		{
			pApCliEntry = &pAd->ApCfg.ApCliTab[i];
			if ((strncmp(macptr, "0", 1) == 0))
				pApCliEntry->Enable = FALSE;
			else if ((strncmp(macptr, "1", 1) == 0))
				pApCliEntry->Enable = TRUE;
	        else
				pApCliEntry->Enable = FALSE;

			if (pApCliEntry->Enable)
			{
				//pApCliEntry->WpaState = SS_NOTUSE;
				//pApCliEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
				//NdisZeroMemory(pApCliEntry->ReplayCounter, LEN_KEY_DESC_REPLAY); 
			}
			DBGPRINT(RT_DEBUG_TRACE, ("ApCliEntry[%d].Enable=%d\n", i, pApCliEntry->Enable));
	    }
	}

	//ApCliSsid
	if(RTMPGetCriticalParameter("ApCliSsid", tmpbuf, MAX_PARAM_BUFFER_SIZE, buffer))
	{
		for (i=0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_APCLI_NUM); macptr = rstrtok(NULL,";"), i++) 
		{
			pApCliEntry = &pAd->ApCfg.ApCliTab[i];

			//Ssid acceptable strlen must be less than 32 and bigger than 0.
			if((strlen(macptr) < 0) || (strlen(macptr) > 32))
				continue; 

			pApCliEntry->CfgSsidLen = strlen(macptr);
			if(pApCliEntry->CfgSsidLen > 0)
			{
				memcpy(&pApCliEntry->CfgSsid, macptr, pApCliEntry->CfgSsidLen);
				pApCliEntry->Valid = FALSE;// it should be set when successfuley association
			} else
			{
				NdisZeroMemory(&(pApCliEntry->CfgSsid), MAX_LEN_OF_SSID);
				continue;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("ApCliEntry[%d].CfgSsidLen=%d, CfgSsid=%s\n", i, pApCliEntry->CfgSsidLen, pApCliEntry->CfgSsid));
		}
	}

	//ApCliBssid
	if(RTMPGetKeyParameter("ApCliBssid", tmpbuf, MAX_PARAM_BUFFER_SIZE, buffer))
	{
		for (i=0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_APCLI_NUM); macptr = rstrtok(NULL,";"), i++) 
		{
			pApCliEntry = &pAd->ApCfg.ApCliTab[i];

			if(strlen(macptr) != 17)  //Mac address acceptable format 01:02:03:04:05:06 length 17
				continue; 
			if(strcmp(macptr,"00:00:00:00:00:00") == 0)
				continue; 
			if(i >= MAX_APCLI_NUM)
				break; 
			for (j=0; j<ETH_LENGTH_OF_ADDRESS; j++)
			{
				AtoH(macptr, &macAddress[j], 1);
				macptr=macptr+3;
			}	
			memcpy(pApCliEntry->CfgApCliBssid, &macAddress, ETH_LENGTH_OF_ADDRESS);
			pApCliEntry->Valid = FALSE;// it should be set when successfuley association
		}
	}

	//ApCliAuthMode
	if (RTMPGetKeyParameter("ApCliAuthMode", tmpbuf, 255, buffer))
	{
		for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_APCLI_NUM); macptr = rstrtok(NULL,";"), i++)
		{
			pApCliEntry = &pAd->ApCfg.ApCliTab[i];
			
			if ((strncmp(macptr, "WEPAUTO", 7) == 0) || (strncmp(macptr, "wepauto", 7) == 0))
				pApCliEntry->AuthMode = Ndis802_11AuthModeAutoSwitch;
			else if ((strncmp(macptr, "SHARED", 6) == 0) || (strncmp(macptr, "shared", 6) == 0))
				pApCliEntry->AuthMode = Ndis802_11AuthModeShared;
			else if ((strncmp(macptr, "WPAPSK", 6) == 0) || (strncmp(macptr, "wpapsk", 6) == 0))
				pApCliEntry->AuthMode = Ndis802_11AuthModeWPAPSK;
			else if ((strncmp(macptr, "WPA2PSK", 7) == 0) || (strncmp(macptr, "wpa2psk", 7) == 0))
				pApCliEntry->AuthMode = Ndis802_11AuthModeWPA2PSK;
			else
				pApCliEntry->AuthMode = Ndis802_11AuthModeOpen;

			//pApCliEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) ApCli_AuthMode=%d \n", i, pApCliEntry->AuthMode));
			RTMPMakeRSNIE(pAd, pApCliEntry->AuthMode, pApCliEntry->WepStatus, (i + MIN_NET_DEVICE_FOR_APCLI));
		}

	}

	//ApCliEncrypType
	if (RTMPGetKeyParameter("ApCliEncrypType", tmpbuf, 255, buffer))
	{
		for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_APCLI_NUM); macptr = rstrtok(NULL,";"), i++)
		{
			pApCliEntry = &pAd->ApCfg.ApCliTab[i];

			pApCliEntry->WepStatus = Ndis802_11WEPDisabled;
			if ((strncmp(macptr, "WEP", 3) == 0) || (strncmp(macptr, "wep", 3) == 0))
            {
				if (pApCliEntry->AuthMode < Ndis802_11AuthModeWPA)
					pApCliEntry->WepStatus = Ndis802_11WEPEnabled;				  
			}
			else if ((strncmp(macptr, "TKIP", 4) == 0) || (strncmp(macptr, "tkip", 4) == 0))
			{
				if (pApCliEntry->AuthMode >= Ndis802_11AuthModeWPA)
					pApCliEntry->WepStatus = Ndis802_11Encryption2Enabled;                       
            }
			else if ((strncmp(macptr, "AES", 3) == 0) || (strncmp(macptr, "aes", 3) == 0))
			{
				if (pApCliEntry->AuthMode >= Ndis802_11AuthModeWPA)
					pApCliEntry->WepStatus = Ndis802_11Encryption3Enabled;                            
			}    
			else
			{
				pApCliEntry->WepStatus      = Ndis802_11WEPDisabled;                 
			}

			pApCliEntry->PairCipher     = pApCliEntry->WepStatus;
			pApCliEntry->GroupCipher    = pApCliEntry->WepStatus;
			pApCliEntry->bMixCipher		= FALSE;
			
			//pApCliEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) APCli_EncrypType = %d \n", i, pApCliEntry->WepStatus));
			RTMPMakeRSNIE(pAd, pApCliEntry->AuthMode, pApCliEntry->WepStatus, (i + MIN_NET_DEVICE_FOR_APCLI));
		}

	}
	
	//ApCliWPAPSK
	if (RTMPGetCriticalParameter("ApCliWPAPSK", tmpbuf, 255, buffer))
	{
		for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_APCLI_NUM); macptr = rstrtok(NULL,";"), i++)
		{
			int err = 0;

			pApCliEntry = &pAd->ApCfg.ApCliTab[i];

			if((strlen(macptr) < 8) || (strlen(macptr) > 64))
			{
				DBGPRINT(RT_DEBUG_ERROR, ("APCli_WPAPSK_KEY, key string required 8 ~ 64 characters!!!\n"));
				continue; 
			}
			
			NdisMoveMemory(pApCliEntry->PSK, macptr, strlen(macptr));
			pApCliEntry->PSKLen = strlen(macptr);
			DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) APCli_WPAPSK_KEY=%s, Len=%d\n", i, pApCliEntry->PSK, pApCliEntry->PSKLen));

			if ((pApCliEntry->AuthMode != Ndis802_11AuthModeWPAPSK) &&
				(pApCliEntry->AuthMode != Ndis802_11AuthModeWPA2PSK))
			{
				err = 1;
			}
			
			if ((strlen(macptr) >= 8) && (strlen(macptr) < 64))
			{// ASCII mode
				PasswordHash((char *)macptr, pApCliEntry->CfgSsid, pApCliEntry->CfgSsidLen, keyMaterial);
				NdisMoveMemory(pApCliEntry->PMK, keyMaterial, 32);
			}
			else if (strlen(macptr) == 64)
			{// Hex mode
				AtoH(macptr, pApCliEntry->PMK, 32);
			}
	
			if (err == 0)
			{
				// Start STA supplicant WPA state machine
				DBGPRINT(RT_DEBUG_TRACE, ("Start AP-client WPAPSK state machine \n"));
				//pApCliEntry->WpaState = SS_START;				
			}

			//RTMPMakeRSNIE(pAd, pApCliEntry->AuthMode, pApCliEntry->WepStatus, (i + MIN_NET_DEVICE_FOR_APCLI));			
#ifdef DBG
			DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) PMK Material => \n", i));
			
			for (j = 0; j < 32; j++)
			{
				printk("%02x:", pApCliEntry->PMK[j]);
				if ((j%16) == 15)
					printk("\n");
			}
			printk("\n");
#endif
		}
	}

	//ApCliDefaultKeyID
	if (RTMPGetKeyParameter("ApCliDefaultKeyID", tmpbuf, 255, buffer))
	{
		for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_APCLI_NUM); macptr = rstrtok(NULL,";"), i++)
		{
			pApCliEntry = &pAd->ApCfg.ApCliTab[i];
			
			KeyIdx = simple_strtol(macptr, 0, 10);
			if((KeyIdx >= 1 ) && (KeyIdx <= 4))
				pApCliEntry->DefaultKeyId = (UCHAR) (KeyIdx - 1);
			else
				pApCliEntry->DefaultKeyId = 0;

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) DefaultKeyID(0~3)=%d\n", i, pApCliEntry->DefaultKeyId));
		}
	}

	//ApCliKeyXType, ApCliKeyXStr
	for (idx=0; idx<4; idx++)
	{
		sprintf(tok_str, "ApCliKey%dType", idx+1);
		//ApCliKey1Type
		if(RTMPGetKeyParameter(tok_str, tmpbuf, 128, buffer))
		{
		    for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_APCLI_NUM); macptr = rstrtok(NULL,";"), i++)
		    {
			    KeyType[i] = simple_strtol(macptr, 0, 10);
		    }

			sprintf(tok_str, "ApCliKey%dStr", idx+1);
			//ApCliKey1Str
			if(RTMPGetCriticalParameter(tok_str, tmpbuf, 512, buffer))
			{
			    for (i = 0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_APCLI_NUM); macptr = rstrtok(NULL,";"), i++)
		        {
		        	pApCliEntry = &pAd->ApCfg.ApCliTab[i];
					KeyLen = strlen(macptr);
					if(KeyType[i] == 0)
					{//Hex type
						if( (KeyLen == 10) || (KeyLen == 26))
						{
							pApCliEntry->SharedKey[idx].KeyLen = KeyLen / 2;
							AtoH(macptr, pApCliEntry->SharedKey[idx].Key, KeyLen / 2);
							if (KeyLen == 10)
								CipherAlg = CIPHER_WEP64;
							else
								CipherAlg = CIPHER_WEP128;
							pApCliEntry->SharedKey[idx].CipherAlg = CipherAlg;

							DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Key%dStr=%s and type=%s\n", i, idx+1, macptr, (KeyType[i]==0) ? "Hex":"Ascii"));
						}
						else
						{ //Invalid key length
							DBGPRINT(RT_DEBUG_ERROR, ("I/F(apcli%d) Key%dStr is Invalid key length!\n", i, idx+1));
						}
					}
					else
					{//Ascii								
						if( (KeyLen == 5) || (KeyLen == 13))
						{
							pApCliEntry->SharedKey[idx].KeyLen = KeyLen;
							NdisMoveMemory(pApCliEntry->SharedKey[idx].Key, macptr, KeyLen);
							if (KeyLen == 5)
								CipherAlg = CIPHER_WEP64;
							else
								CipherAlg = CIPHER_WEP128;
							pApCliEntry->SharedKey[idx].CipherAlg = CipherAlg;
					
							DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Key%dStr=%s and type=%s\n", i, idx+1, macptr, (KeyType[i]==0) ? "Hex":"Ascii"));		
						}
						else
						{ //Invalid key length
							DBGPRINT(RT_DEBUG_ERROR, ("I/F(apcli%d) Key%dStr is Invalid key length!\n", i, idx+1));
						}
					}
			    }
			}
		}
	}
	
#ifdef WSC_AP_SUPPORT
#if 0
    //WscPinCode
	if(RTMPGetCriticalParameter("ApCliWscPinCode", tmpbuf, 10, buffer))
	{
		pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode = (INT) simple_strtol(tmpbuf, 0, 10);
        if (ValidateChecksum(pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode) == FALSE)
        {
            pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode = 0;
            DBGPRINT(RT_DEBUG_TRACE, ("ApCliWscPinCode InValid!!\n"));
        }
        else
		    DBGPRINT(RT_DEBUG_TRACE, ("ApCliWscPinCode=%d\n", pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode));
	}
#endif // if 0    
#endif // WSC_AP_SUPPORT //	
}
#endif // APCLI_SUPPORT //

static void rtmp_read_acl_parms_from_file(IN  PRTMP_ADAPTER pAd, char *tmpbuf, char *buffer)
{
	char		tok_str[32];
	PUCHAR		macptr;						
	INT			i=0, j=0, idx;
	UCHAR		macAddress[MAC_ADDR_LEN];
											  
	for (idx=0; idx<MAX_MBSSID_NUM; idx++)
	{
		memset(&pAd->ApCfg.MBSSID[idx].AccessControlList, 0, sizeof(RT_802_11_ACL));
		// AccessPolicyX
		sprintf(tok_str, "AccessPolicy%d", idx);
		if (RTMPGetKeyParameter(tok_str, tmpbuf, 10, buffer))
		{
			switch (simple_strtol(tmpbuf, 0, 10))
			{
				case 1: // Allow All, and the AccessControlList is positive now.
					pAd->ApCfg.MBSSID[idx].AccessControlList.Policy = 1;
					break;
				case 2: // Reject All, and the AccessControlList is negative now.
					pAd->ApCfg.MBSSID[idx].AccessControlList.Policy = 2;
					break;
				case 0: // Disable, don't care the AccessControlList.
				default:
					pAd->ApCfg.MBSSID[idx].AccessControlList.Policy = 0;
					break;
			}
			DBGPRINT(RT_DEBUG_TRACE, ("%s=%ld\n", tok_str, pAd->ApCfg.MBSSID[idx].AccessControlList.Policy));
		}
		// AccessControlListX
		sprintf(tok_str, "AccessControlList%d", idx);
		if (RTMPGetKeyParameter(tok_str, tmpbuf, MAX_PARAM_BUFFER_SIZE, buffer))
		{
			for (i=0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++) 
			{
				if (strlen(macptr) != 17)  // Mac address acceptable format 01:02:03:04:05:06 length 17
					continue;

				ASSERT(pAd->ApCfg.MBSSID[idx].AccessControlList.Num <= MAX_NUM_OF_ACL_LIST);
				
				if (pAd->ApCfg.MBSSID[idx].AccessControlList.Num == MAX_NUM_OF_ACL_LIST)
				{
					DBGPRINT(RT_DEBUG_WARN, ("The AccessControlList is full, and no more entry can join the list!\n"));
        			DBGPRINT(RT_DEBUG_WARN, ("The last entry of ACL is %02x:%02x:%02x:%02x:%02x:%02x\n",
        				macAddress[0],macAddress[1],macAddress[2],macAddress[3],macAddress[4],macAddress[5]));

				    break;
				}
				for (j=0; j<ETH_LENGTH_OF_ADDRESS; j++)
				{
					AtoH(macptr, &macAddress[j], 1);
					macptr=macptr+3;
				}
				
				pAd->ApCfg.MBSSID[idx].AccessControlList.Num++;
				NdisMoveMemory(pAd->ApCfg.MBSSID[idx].AccessControlList.Entry[(pAd->ApCfg.MBSSID[idx].AccessControlList.Num - 1)].Addr, macAddress, ETH_LENGTH_OF_ADDRESS);				
			}
			DBGPRINT(RT_DEBUG_TRACE, ("%s=Get %ld Mac Address\n", tok_str, pAd->ApCfg.MBSSID[idx].AccessControlList.Num));
 		}
	}
}

/*
    ========================================================================

    Routine Description:
        In kernel mode read parameters from file

    Arguments:
        src                     the location of the file.
        dest                        put the parameters to the destination.
        Length                  size to read.

    Return Value:
        None

    Note:

    ========================================================================
*/
static void rtmp_read_ap_wmm_parms_from_file(IN  PRTMP_ADAPTER pAd, char *tmpbuf, char *buffer)
{
	PUCHAR					macptr;						
	INT						i=0;

	//WmmCapable
	if(RTMPGetKeyParameter("WmmCapable", tmpbuf, 32, buffer))
	{
	    BOOLEAN bEnableWmm = FALSE;
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			if (i >= pAd->ApCfg.BssidNum)
			{
				break;
			}

			if(simple_strtol(macptr, 0, 10) != 0)  //Enable
			{
				pAd->ApCfg.MBSSID[i].bWmmCapable = TRUE;
				bEnableWmm = TRUE;
			}
			else //Disable
			{
				pAd->ApCfg.MBSSID[i].bWmmCapable = FALSE;
			}
			if (bEnableWmm)
			{
				pAd->CommonCfg.APEdcaParm.bValid = TRUE;
				pAd->ApCfg.BssEdcaParm.bValid = TRUE;
			}
			else
				{
				pAd->CommonCfg.APEdcaParm.bValid = FALSE;
				pAd->ApCfg.BssEdcaParm.bValid = FALSE;
			}

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) WmmCapable=%d\n", i, pAd->ApCfg.MBSSID[i].bWmmCapable));
	    }
	}
	//DLSCapable
	if(RTMPGetKeyParameter("DLSCapable", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			if (i >= pAd->ApCfg.BssidNum)
			{
				break;
			}

			if(simple_strtol(macptr, 0, 10) != 0)  //Enable
			{
				pAd->ApCfg.MBSSID[i].bDLSCapable = TRUE;
			}
			else //Disable
			{
				pAd->ApCfg.MBSSID[i].bDLSCapable = FALSE;
			}

			DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) DLSCapable=%d\n", i, pAd->ApCfg.MBSSID[i].bDLSCapable));
	    }
	}
	//APAifsn
	if(RTMPGetKeyParameter("APAifsn", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->CommonCfg.APEdcaParm.Aifsn[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("APAifsn[%d]=%d\n", i, pAd->CommonCfg.APEdcaParm.Aifsn[i]));
	    }
	}
	//APCwmin
	if(RTMPGetKeyParameter("APCwmin", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->CommonCfg.APEdcaParm.Cwmin[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("APCwmin[%d]=%d\n", i, pAd->CommonCfg.APEdcaParm.Cwmin[i]));
	    }
	}
	//APCwmax
	if(RTMPGetKeyParameter("APCwmax", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->CommonCfg.APEdcaParm.Cwmax[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("APCwmax[%d]=%d\n", i, pAd->CommonCfg.APEdcaParm.Cwmax[i]));
	    }
	}
	//APTxop
	if(RTMPGetKeyParameter("APTxop", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->CommonCfg.APEdcaParm.Txop[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("APTxop[%d]=%d\n", i, pAd->CommonCfg.APEdcaParm.Txop[i]));
	    }
	}
	//APACM
	if(RTMPGetKeyParameter("APACM", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->CommonCfg.APEdcaParm.bACM[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("APACM[%d]=%d\n", i, pAd->CommonCfg.APEdcaParm.bACM[i]));
	    }
	}
	//BSSAifsn
	if(RTMPGetKeyParameter("BSSAifsn", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->ApCfg.BssEdcaParm.Aifsn[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("BSSAifsn[%d]=%d\n", i, pAd->ApCfg.BssEdcaParm.Aifsn[i]));
	    }
	}
	//BSSCwmin
	if(RTMPGetKeyParameter("BSSCwmin", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->ApCfg.BssEdcaParm.Cwmin[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("BSSCwmin[%d]=%d\n", i, pAd->ApCfg.BssEdcaParm.Cwmin[i]));
	    }
	}
	//BSSCwmax
	if(RTMPGetKeyParameter("BSSCwmax", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->ApCfg.BssEdcaParm.Cwmax[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("BSSCwmax[%d]=%d\n", i, pAd->ApCfg.BssEdcaParm.Cwmax[i]));
	    }
	}
	//BSSTxop
	if(RTMPGetKeyParameter("BSSTxop", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->ApCfg.BssEdcaParm.Txop[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("BSSTxop[%d]=%d\n", i, pAd->ApCfg.BssEdcaParm.Txop[i]));
	    }
	}
	//BSSACM
	if(RTMPGetKeyParameter("BSSACM", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->ApCfg.BssEdcaParm.bACM[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("BSSACM[%d]=%d\n", i, pAd->ApCfg.BssEdcaParm.bACM[i]));
	    }
	}
	//AckPolicy
	if(RTMPGetKeyParameter("AckPolicy", tmpbuf, 32, buffer))
	{
	    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
	    {
			pAd->CommonCfg.AckPolicy[i] = simple_strtol(macptr, 0, 10);;

			DBGPRINT(RT_DEBUG_TRACE, ("AckPolicy[%d]=%d\n", i, pAd->CommonCfg.AckPolicy[i]));
	    }
	}
#ifdef UAPSD_AP_SUPPORT
	//APSDCapable
	if(RTMPGetKeyParameter("APSDCapable", tmpbuf, 10, buffer))
	{
		if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
			pAd->CommonCfg.bAPSDCapable = TRUE;
		else
			pAd->CommonCfg.bAPSDCapable = FALSE;

		DBGPRINT(RT_DEBUG_TRACE, ("APSDCapable=%d\n", pAd->CommonCfg.bAPSDCapable));
	}
#endif // UAPSD_AP_SUPPORT //
#ifdef RTL865X_SOC
	//EthWithVLANTag
	if(RTMPGetKeyParameter("EthWithVLANTag", tmpbuf, 10, buffer))
	{
		if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
			pAd->CommonCfg.bEthWithVLANTag = TRUE;
		else
			pAd->CommonCfg.bEthWithVLANTag = FALSE;

		DBGPRINT(RT_DEBUG_TRACE, ("bEthWithVLANTag=%d\n", pAd->CommonCfg.bEthWithVLANTag));
	}
#endif // RTL865X_SOC //
}


/*
    ========================================================================

    Routine Description:
        In kernel mode read parameters from file

    Arguments:
        src                     the location of the file.
        dest                        put the parameters to the destination.
        Length                  size to read.

    Return Value:
        None

    Note:

    ========================================================================
*/
static void rtmp_read_radius_parms_from_file(IN  PRTMP_ADAPTER pAd, char *tmpbuf, char *buffer)
{
	char					tok_str[16];
	PUCHAR					macptr;		
	UINT32					ip_addr;
	INT						i=0;
	BOOLEAN					bUsePrevFormat = FALSE;
	USHORT					offset;
	INT						count[MAX_MBSSID_NUM];

	// own_ip_addr
	if (RTMPGetKeyParameter("own_ip_addr", tmpbuf, 32, buffer))
	{
		if (rtinet_aton(tmpbuf, &ip_addr))
     	{
            pAd->ApCfg.own_ip_addr = ip_addr;  
			DBGPRINT(RT_DEBUG_TRACE, ("own_ip_addr=%s(%x)\n", tmpbuf, pAd->ApCfg.own_ip_addr));
		}	    
	}
	// radius_retry_primary_interval
	if (RTMPGetKeyParameter("radius_retry_primary_interval", tmpbuf, 32, buffer))
	{
		pAd->ApCfg.retry_interval = simple_strtol(tmpbuf, 0, 10); 
		DBGPRINT(RT_DEBUG_TRACE, ("radius_retry_primary_interval=%d\n", pAd->ApCfg.retry_interval));
	} 
	// session_timeout_interval
	if (RTMPGetKeyParameter("session_timeout_interval", tmpbuf, 32, buffer))
	{
		pAd->ApCfg.session_timeout_interval = simple_strtol(tmpbuf, 0, 10); 
		DBGPRINT(RT_DEBUG_TRACE, ("session_timeout_interval=%d\n", pAd->ApCfg.session_timeout_interval));
	} 
	// EAPifname
	if (RTMPGetKeyParameter("EAPifname", tmpbuf, 32, buffer))
	{
		if (strlen(tmpbuf) > 0)
		{
			pAd->ApCfg.EAPifname_len = strlen(tmpbuf); 
			NdisMoveMemory(pAd->ApCfg.EAPifname, tmpbuf, strlen(tmpbuf));
			DBGPRINT(RT_DEBUG_TRACE, ("EAPifname=%s, len=%d\n", 
														pAd->ApCfg.EAPifname, 
														pAd->ApCfg.EAPifname_len));
		}
	}
	// PreAuthifname
	if (RTMPGetKeyParameter("PreAuthifname", tmpbuf, 32, buffer))
	{
		if (strlen(tmpbuf) > 0)
		{
			pAd->ApCfg.PreAuthifname_len = strlen(tmpbuf); 
			NdisMoveMemory(pAd->ApCfg.PreAuthifname, tmpbuf, strlen(tmpbuf));
			DBGPRINT(RT_DEBUG_TRACE, ("PreAuthifname=%s, len=%d\n",
														pAd->ApCfg.PreAuthifname, 
														pAd->ApCfg.PreAuthifname_len));
		}
	}
	// RADIUS_Server
	offset = 0;
	//if (RTMPGetKeyParameter("RADIUS_Server", tmpbuf, 256, buffer))
	while (RTMPGetKeyParameterWithOffset("RADIUS_Server", tmpbuf, &offset, 256, buffer, TRUE))	
	{
		for (i=0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_MBSSID_NUM); macptr = rstrtok(NULL,";"), i++) 
		{
			if (rtinet_aton(macptr, &ip_addr) && pAd->ApCfg.MBSSID[i].radius_srv_num < MAX_RADIUS_SRV_NUM)
	     	{
				INT		srv_idx = pAd->ApCfg.MBSSID[i].radius_srv_num;
									
	            pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_ip = ip_addr;				
				pAd->ApCfg.MBSSID[i].radius_srv_num ++;	
				DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d), radius_ip(seq-%d)=%s(%x)\n", i, pAd->ApCfg.MBSSID[i].radius_srv_num, macptr, pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_ip));
			}	    
		}
	}
	// RADIUS_Port
	//if (RTMPGetKeyParameter("RADIUS_Port", tmpbuf, 128, buffer))
	offset = 0;
	for (i=0; i < MAX_MBSSID_NUM; i++)	
		count[i] = 0;
	while (RTMPGetKeyParameterWithOffset("RADIUS_Port", tmpbuf, &offset, 128, buffer, TRUE))
	{
		for (i=0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_MBSSID_NUM); macptr = rstrtok(NULL,";"), i++) 
		{	  
			if (count[i] < pAd->ApCfg.MBSSID[i].radius_srv_num)
			{		
				INT		srv_idx = count[i];
				
            	pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_port = simple_strtol(macptr, 0, 10); 
				count[i] ++;
				DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d), radius_port(seq-%d)=%d\n", i, count[i], pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_port));
			}
		}
	}
	// RADIUS_Key
	//if (RTMPGetCriticalParameter("RADIUS_Key", tmpbuf, 640, buffer))
	offset = 0;
	for (i=0; i < MAX_MBSSID_NUM; i++)	
		count[i] = 0;
	while (RTMPGetKeyParameterWithOffset("RADIUS_Key", tmpbuf, &offset, 640, buffer, FALSE))
	{
		if (strlen(tmpbuf) > 0)
			bUsePrevFormat = TRUE;
	
		for (i=0, macptr = rstrtok(tmpbuf,";"); (macptr && i < MAX_MBSSID_NUM); macptr = rstrtok(NULL,";"), i++) 
		{	  
			if (strlen(macptr) > 0 && (count[i] < pAd->ApCfg.MBSSID[i].radius_srv_num))
			{
				INT		srv_idx = count[i];
			
				pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_key_len = strlen(macptr); 
				NdisMoveMemory(pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_key, macptr, strlen(macptr));
				count[i] ++;
				DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d), radius_key(seq-%d)=%s, len=%d\n", i, 
															count[i],
															pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_key, 
															pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_key_len));
			}
		}
	}

	if (!bUsePrevFormat)
	{
		for (i = 0; i < MAX_MBSSID_NUM; i++)
		{
			INT	srv_idx = 0;
			
			sprintf(tok_str, "RADIUS_Key%d", i + 1);
			
			// RADIUS_KeyX (X=1~MAX_MBSSID_NUM)
			//if (RTMPGetCriticalParameter(tok_str, tmpbuf, 128, buffer))			
			offset = 0;
			while (RTMPGetKeyParameterWithOffset(tok_str, tmpbuf, &offset, 128, buffer, FALSE))
			{
				if (strlen(tmpbuf) > 0 && (srv_idx < pAd->ApCfg.MBSSID[i].radius_srv_num))
				{
					pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_key_len = strlen(tmpbuf); 
					NdisMoveMemory(pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_key, tmpbuf, strlen(tmpbuf));					
					DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d), update radius_key(seq-%d)=%s, len=%d\n", i, srv_idx+1,
																pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_key, 
																pAd->ApCfg.MBSSID[i].radius_srv_info[srv_idx].radius_key_len));
					srv_idx ++;
				}	
			}
		}
	}
}


#endif // CONFIG_AP_SUPPORT //


#ifdef CONFIG_AP_SUPPORT
static int rtmp_parse_wpapsk_buffer_from_file(IN  PRTMP_ADAPTER pAd,IN  char *buffer,IN  INT BSSIdx)
{
	PUCHAR		tmpbuf = buffer;
	INT			i = BSSIdx;
	UCHAR		keyMaterial[40];
	ULONG		len = strlen(tmpbuf);
	int         ret = 0;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) WPAPSK_KEY=%s\n", i, tmpbuf));
	if (len == 64)
	{// Hex mode
    	AtoH(tmpbuf, pAd->ApCfg.MBSSID[i].PMK, 32);
		ret = 1;
	}
	else if ((len >= 8) && (len < 64))
	{// ASCII mode
    	PasswordHash((char *)tmpbuf, pAd->ApCfg.MBSSID[i].Ssid, pAd->ApCfg.MBSSID[i].SsidLen, keyMaterial);
    	NdisMoveMemory(pAd->ApCfg.MBSSID[i].PMK, keyMaterial, 32);
		ret = 1;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("WPAPSK_KEY, key len (should be 8~64) incorrect!!!, your key len = %ld\n", len));
		ret = 0;
	}
#ifdef WSC_AP_SUPPORT
    NdisZeroMemory(pAd->ApCfg.MBSSID[i].WscControl.WpaPsk, 64);
    pAd->ApCfg.MBSSID[i].WscControl.WpaPskLen = 0;
    if ((len >= 8) && (len <= 64))
    {                                    
        NdisMoveMemory(pAd->ApCfg.MBSSID[i].WscControl.WpaPsk, tmpbuf, len);
        pAd->ApCfg.MBSSID[i].WscControl.WpaPskLen = len;
    }
#endif // WSC_AP_SUPPORT //
	return ret;
}
#endif // CONFIG_AP_SUPPORT //

NDIS_STATUS	RTMPReadParametersHook(
	IN	PRTMP_ADAPTER pAd)
{
	PUCHAR					src = NULL;
	struct file				*srcf;
	INT 					retval, orgfsuid, orgfsgid;
   	mm_segment_t			orgfs;
	CHAR					*buffer;
	CHAR					*tmpbuf;
	ULONG					RtsThresh;
	ULONG					FragThresh;


	PUCHAR					macptr;							
	INT						i = 0;

	buffer = kmalloc(MAX_INI_BUFFER_SIZE, MEM_ALLOC_FLAG);
	if(buffer == NULL)
        return NDIS_STATUS_FAILURE;

	tmpbuf = kmalloc(MAX_PARAM_BUFFER_SIZE, MEM_ALLOC_FLAG);
	if(tmpbuf == NULL)
	{
		kfree(buffer);
        return NDIS_STATUS_FAILURE;
	}
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		src = AP_PROFILE_PATH;
#endif // CONFIG_AP_SUPPORT //

#ifdef MULTIPLE_CARD_SUPPORT
	src = pAd->MC_FileName;
#endif // MULTIPLE_CARD_SUPPORT //

	// Save uid and gid used for filesystem access.
	// Set user and group to 0 (root)	
	orgfsuid = current->fsuid;
	orgfsgid = current->fsgid;
	current->fsuid=current->fsgid = 0;
    orgfs = get_fs();
    set_fs(KERNEL_DS);

	if (src && *src)
	{
		srcf = filp_open(src, O_RDONLY, 0);
		if (IS_ERR(srcf)) 
		{
			DBGPRINT(RT_DEBUG_TRACE, ("--> Error %ld opening %s\n", -PTR_ERR(srcf),src));
		}
		else 
		{
			// The object must have a read method
			if (srcf->f_op && srcf->f_op->read)
			{
				memset(buffer, 0x00, MAX_INI_BUFFER_SIZE);
				retval=srcf->f_op->read(srcf, buffer, MAX_INI_BUFFER_SIZE, &srcf->f_pos);
				if (retval < 0)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("--> Read %s error %d\n", src, -retval));
				}
				else
				{
					// set file parameter to portcfg
					//CountryRegion
					if(RTMPGetKeyParameter("CountryRegion", tmpbuf, 25, buffer))
					{
						pAd->CommonCfg.CountryRegion = (UCHAR) simple_strtol(tmpbuf, 0, 10);
						DBGPRINT(RT_DEBUG_TRACE, ("CountryRegion=%d\n", pAd->CommonCfg.CountryRegion));
					}
					//CountryRegionABand
					if(RTMPGetKeyParameter("CountryRegionABand", tmpbuf, 25, buffer))
					{
						pAd->CommonCfg.CountryRegionForABand= (UCHAR) simple_strtol(tmpbuf, 0, 10);
						DBGPRINT(RT_DEBUG_TRACE, ("CountryRegionABand=%d\n", pAd->CommonCfg.CountryRegionForABand));
					}
					//CountryCode
					if(RTMPGetKeyParameter("CountryCode", tmpbuf, 25, buffer))
					{
						NdisMoveMemory(pAd->CommonCfg.CountryCode, tmpbuf , 2);
						if (strlen(pAd->CommonCfg.CountryCode) != 0)
						{
							pAd->CommonCfg.bCountryFlag = TRUE;
						}
						DBGPRINT(RT_DEBUG_TRACE, ("CountryCode=%s\n", pAd->CommonCfg.CountryCode));
					}
					//ChannelGeography
					if(RTMPGetKeyParameter("ChannelGeography", tmpbuf, 25, buffer))
					{
						UCHAR Geography = (UCHAR) simple_strtol(tmpbuf, 0, 10);
						if (Geography <= BOTH)
						{
							pAd->CommonCfg.Geography = Geography;
							pAd->CommonCfg.CountryCode[2] =
								(pAd->CommonCfg.Geography == BOTH) ? ' ' : ((pAd->CommonCfg.Geography == IDOR) ? 'I' : 'O');
							DBGPRINT(RT_DEBUG_TRACE, ("ChannelGeography=%d\n", pAd->CommonCfg.Geography));
						}
					}
					else
					{
						pAd->CommonCfg.Geography = BOTH;
						pAd->CommonCfg.CountryCode[2] = ' ';
					}
#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
#ifdef MBSS_SUPPORT
						//BSSIDNum; This must read first of other multiSSID field, so list this field first in configuration file
						if(RTMPGetKeyParameter("BssidNum", tmpbuf, 25, buffer))
						{
							pAd->ApCfg.BssidNum = (UCHAR) simple_strtol(tmpbuf, 0, 10);
							if(pAd->ApCfg.BssidNum > MAX_MBSSID_NUM)
							{
								pAd->ApCfg.BssidNum = MAX_MBSSID_NUM;
								DBGPRINT(RT_DEBUG_TRACE, ("BssidNum=%d(MAX_MBSSID_NUM is %d)\n", pAd->ApCfg.BssidNum,MAX_MBSSID_NUM));
							}
							else
							DBGPRINT(RT_DEBUG_TRACE, ("BssidNum=%d\n", pAd->ApCfg.BssidNum));
						}

						if (HW_BEACON_OFFSET > (HW_BEACON_MAX_SIZE / pAd->ApCfg.BssidNum))
						{
							printk("mbss> fatal error! beacon offset is error in driver! "
									"Please re-assign HW_BEACON_OFFSET!\n");
						} /* End of if */
#else
						pAd->ApCfg.BssidNum = 1;
#endif // MBSS_SUPPORT //
					}
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
						// SSID
						if (TRUE)
						{
							CHAR tok_str[16];
							UCHAR BssidCountSupposed = 0;
							BOOLEAN bSSIDxIsUsed = FALSE;

							DBGPRINT(RT_DEBUG_TRACE, ("pAd->ApCfg.BssidNum=%d\n", pAd->ApCfg.BssidNum));

							for (i = 0; i < pAd->ApCfg.BssidNum; i++)
							{
								sprintf(tok_str, "SSID%d", i + 1);
								if(RTMPGetCriticalParameter(tok_str, tmpbuf, 33, buffer))
									{
										NdisMoveMemory(pAd->ApCfg.MBSSID[i].Ssid, tmpbuf , strlen(tmpbuf));
								    	pAd->ApCfg.MBSSID[i].Ssid[strlen(tmpbuf)] = '\0';
								    	pAd->ApCfg.MBSSID[i].SsidLen = strlen(pAd->ApCfg.MBSSID[i].Ssid);
										if (bSSIDxIsUsed == FALSE)
										{
											bSSIDxIsUsed = TRUE;
										}
								    	DBGPRINT(RT_DEBUG_TRACE, ("SSID[%d]=%s\n", i, pAd->ApCfg.MBSSID[i].Ssid));
									}
								}
							if (bSSIDxIsUsed == FALSE)
							{
								if(RTMPGetCriticalParameter("SSID", tmpbuf, 256, buffer))
								{			
									BssidCountSupposed = delimitcnt(tmpbuf, ";") + 1;
									if (pAd->ApCfg.BssidNum != BssidCountSupposed)
									{
										DBGPRINT_ERR(("Your no. of SSIDs( = %d) does not match your BssidNum( = %d)!\n", BssidCountSupposed, pAd->ApCfg.BssidNum));
									}
									if (pAd->ApCfg.BssidNum > 1)
									{
										// Anyway, we still do the legacy dissection of the whole SSID string.
									    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
										{
											int apidx = 0;

											if (i < pAd->ApCfg.BssidNum)
											{
												apidx = i;
											} 
											else
											{
												break;
											}

											NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].Ssid, macptr , strlen(macptr));
							    			pAd->ApCfg.MBSSID[apidx].Ssid[strlen(macptr)] = '\0';
							    			pAd->ApCfg.MBSSID[apidx].SsidLen = strlen(pAd->ApCfg.MBSSID[apidx].Ssid);

							    			DBGPRINT(RT_DEBUG_TRACE, ("SSID[%d]=%s\n", i, pAd->ApCfg.MBSSID[apidx].Ssid));
										}
									}
									else
									{
										if ((strlen(tmpbuf) > 0) && (strlen(tmpbuf) <= 32))
										{
											NdisMoveMemory(pAd->ApCfg.MBSSID[BSS0].Ssid, tmpbuf , strlen(tmpbuf));
									    	pAd->ApCfg.MBSSID[BSS0].Ssid[strlen(tmpbuf)] = '\0';
									    	pAd->ApCfg.MBSSID[BSS0].SsidLen = strlen(pAd->ApCfg.MBSSID[BSS0].Ssid);
											DBGPRINT(RT_DEBUG_TRACE, ("SSID=%s\n", pAd->ApCfg.MBSSID[BSS0].Ssid));
										}
									}
								}
							}
						}
					}
#endif // CONFIG_AP_SUPPORT //


					//Channel
					if(RTMPGetKeyParameter("Channel", tmpbuf, 10, buffer))
					{
						pAd->CommonCfg.Channel = (UCHAR) simple_strtol(tmpbuf, 0, 10);
						DBGPRINT(RT_DEBUG_TRACE, ("Channel=%d\n", pAd->CommonCfg.Channel));
					}
					//WirelessMode
					if(RTMPGetKeyParameter("WirelessMode", tmpbuf, 10, buffer))
					{
						int value  = 0;
#if 0
						switch (simple_strtol(tmpbuf, 0, 10))
						{
							case PHY_11A:
								pAd->CommonCfg.PhyMode = PHY_11A;
								break;
							case PHY_11B:
								pAd->CommonCfg.PhyMode = PHY_11B;
								break;
							case PHY_11G:
								pAd->CommonCfg.PhyMode = PHY_11G;
								break;
							case PHY_11BG_MIXED:
							case PHY_11BGN_MIXED:
							default:
								pAd->CommonCfg.PhyMode = PHY_11BGN_MIXED;
								break;
						}
#endif

						value = simple_strtol(tmpbuf, 0, 10);

						if (value <= PHY_11N_5G)
						{
							pAd->CommonCfg.PhyMode = value;
						}
						DBGPRINT(RT_DEBUG_TRACE, ("PhyMode=%d\n", pAd->CommonCfg.PhyMode));
					}
#if 0	// The parameter is ignored
					//TxRate
					if(RTMPGetKeyParameter("TxRate", tmpbuf, 10, buffer))
					{
						int rateindex;
						macptr = rstrtok(tmpbuf,";");
						//Set_TxRate_Proc(pAd, macptr);
		
						//pAd->ApCfg.MBSSID[apidx].Ssid[i].DesiredRatesIndex = simple_strtol(macptr, 0, 10);
						rateindex = simple_strtol(macptr, 0, 10);
						//RTMPBuildDesireRate(pAd, 0, rateindex);
						/*for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
						{
							pAd->ApCfg.MBSSID[i].DesiredRatesIndex = simple_strtol(macptr, 0, 10);
							RTMPBuildDesireRate(pAd, i,pAd->ApCfg.MBSSID[i].DesiredRatesIndex);
						}*/
					}
#endif					
                    //BasicRate
					if(RTMPGetKeyParameter("BasicRate", tmpbuf, 10, buffer))
					{
						pAd->CommonCfg.BasicRateBitmap = (ULONG) simple_strtol(tmpbuf, 0, 10);
						DBGPRINT(RT_DEBUG_TRACE, ("BasicRate=%ld\n", pAd->CommonCfg.BasicRateBitmap));
					}
					//BeaconPeriod
					if(RTMPGetKeyParameter("BeaconPeriod", tmpbuf, 10, buffer))
					{
						pAd->CommonCfg.BeaconPeriod = (USHORT) simple_strtol(tmpbuf, 0, 10);
						DBGPRINT(RT_DEBUG_TRACE, ("BeaconPeriod=%d\n", pAd->CommonCfg.BeaconPeriod));
					}
#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
						//DtimPeriod
						if(RTMPGetKeyParameter("DtimPeriod", tmpbuf, 10, buffer))
						{
							pAd->ApCfg.DtimPeriod = (UCHAR) simple_strtol(tmpbuf, 0, 10);
							DBGPRINT(RT_DEBUG_TRACE, ("DtimPeriod=%d\n", pAd->ApCfg.DtimPeriod));
						}
					}
#endif // CONFIG_AP_SUPPORT //					
                    //TxPower
					if(RTMPGetKeyParameter("TxPower", tmpbuf, 10, buffer))
					{
						pAd->CommonCfg.TxPowerPercentage = (ULONG) simple_strtol(tmpbuf, 0, 10);
						DBGPRINT(RT_DEBUG_TRACE, ("TxPower=%ld\n", pAd->CommonCfg.TxPowerPercentage));
					}
					//BGProtection
					if(RTMPGetKeyParameter("BGProtection", tmpbuf, 10, buffer))
					{
//#if 0	//#ifndef WIFI_TEST
//						pAd->CommonCfg.UseBGProtection = 2;// disable b/g protection for throughput test
//#else
						switch (simple_strtol(tmpbuf, 0, 10))
						{
							case 1: //Always On
								pAd->CommonCfg.UseBGProtection = 1;
								break;
							case 2: //Always OFF
								pAd->CommonCfg.UseBGProtection = 2;
								break;
							case 0: //AUTO
							default:
								pAd->CommonCfg.UseBGProtection = 0;
								break;
						}
//#endif
						DBGPRINT(RT_DEBUG_TRACE, ("BGProtection=%ld\n", pAd->CommonCfg.UseBGProtection));
					}
					//OLBCDetection
					if(RTMPGetKeyParameter("DisableOLBC", tmpbuf, 10, buffer))
					{
						switch (simple_strtol(tmpbuf, 0, 10))
						{
							case 1: //disable OLBC Detection
								pAd->CommonCfg.DisableOLBCDetect = 1;
								break;
							case 0: //enable OLBC Detection
								pAd->CommonCfg.DisableOLBCDetect = 0;
								break;
							default:
								pAd->CommonCfg.DisableOLBCDetect= 0;
								break;
						}
						DBGPRINT(RT_DEBUG_TRACE, ("OLBCDetection=%ld\n", pAd->CommonCfg.DisableOLBCDetect));
					}
					//TxPreamble
					if(RTMPGetKeyParameter("TxPreamble", tmpbuf, 10, buffer))
					{
						switch (simple_strtol(tmpbuf, 0, 10))
						{
							case Rt802_11PreambleShort:
								pAd->CommonCfg.TxPreamble = Rt802_11PreambleShort;
								break;
							case Rt802_11PreambleLong:
							default:
								pAd->CommonCfg.TxPreamble = Rt802_11PreambleLong;
								break;
						}
						DBGPRINT(RT_DEBUG_TRACE, ("TxPreamble=%ld\n", pAd->CommonCfg.TxPreamble));
					}
					//RTSThreshold
					if(RTMPGetKeyParameter("RTSThreshold", tmpbuf, 10, buffer))
					{
						RtsThresh = simple_strtol(tmpbuf, 0, 10);
						if( (RtsThresh >= 1) && (RtsThresh <= MAX_RTS_THRESHOLD) )
							pAd->CommonCfg.RtsThreshold  = (USHORT)RtsThresh;
						else
							pAd->CommonCfg.RtsThreshold = MAX_RTS_THRESHOLD;
						
						DBGPRINT(RT_DEBUG_TRACE, ("RTSThreshold=%d\n", pAd->CommonCfg.RtsThreshold));
					}
					//FragThreshold
					if(RTMPGetKeyParameter("FragThreshold", tmpbuf, 10, buffer))
					{		
						FragThresh = simple_strtol(tmpbuf, 0, 10);
						pAd->CommonCfg.bUseZeroToDisableFragment = FALSE;

						if (FragThresh > MAX_FRAG_THRESHOLD || FragThresh < MIN_FRAG_THRESHOLD)
						{ //illegal FragThresh so we set it to default
							pAd->CommonCfg.FragmentThreshold = MAX_FRAG_THRESHOLD;
							pAd->CommonCfg.bUseZeroToDisableFragment = TRUE;
						}
						else if (FragThresh % 2 == 1)
						{
							// The length of each fragment shall always be an even number of octets, except for the last fragment
							// of an MSDU or MMPDU, which may be either an even or an odd number of octets.
							pAd->CommonCfg.FragmentThreshold = (USHORT)(FragThresh - 1);
						}
						else
						{
							pAd->CommonCfg.FragmentThreshold = (USHORT)FragThresh;
						}
						//pAd->CommonCfg.AllowFragSize = (pAd->CommonCfg.FragmentThreshold) - LENGTH_802_11 - LENGTH_CRC;
						DBGPRINT(RT_DEBUG_TRACE, ("FragThreshold=%d\n", pAd->CommonCfg.FragmentThreshold));
					}
					//TxBurst
					if(RTMPGetKeyParameter("TxBurst", tmpbuf, 10, buffer))
					{
//#ifdef WIFI_TEST
//						pAd->CommonCfg.bEnableTxBurst = FALSE;
//#else
						if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
							pAd->CommonCfg.bEnableTxBurst = TRUE;
						else //Disable
							pAd->CommonCfg.bEnableTxBurst = FALSE;
//#endif
						DBGPRINT(RT_DEBUG_TRACE, ("TxBurst=%d\n", pAd->CommonCfg.bEnableTxBurst));
					}

#ifdef AGGREGATION_SUPPORT
					//PktAggregate
					if(RTMPGetKeyParameter("PktAggregate", tmpbuf, 10, buffer))
					{
						if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
							pAd->CommonCfg.bAggregationCapable = TRUE;
						else //Disable
							pAd->CommonCfg.bAggregationCapable = FALSE;
#ifdef PIGGYBACK_SUPPORT
						pAd->CommonCfg.bPiggyBackCapable = pAd->CommonCfg.bAggregationCapable;
#endif // PIGGYBACK_SUPPORT //
						DBGPRINT(RT_DEBUG_TRACE, ("PktAggregate=%d\n", pAd->CommonCfg.bAggregationCapable));
					}
#else
					pAd->CommonCfg.bAggregationCapable = FALSE;
					pAd->CommonCfg.bPiggyBackCapable = FALSE;
#endif // AGGREGATION_SUPPORT //

					// WmmCapable
#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
						rtmp_read_ap_wmm_parms_from_file(pAd, tmpbuf, buffer);
#endif // CONFIG_AP_SUPPORT //


#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
						//NoForwarding
						if(RTMPGetKeyParameter("NoForwarding", tmpbuf, 32, buffer))
						{
						    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
						    {
								if (i >= pAd->ApCfg.BssidNum)
									break;

	    						if(simple_strtol(macptr, 0, 10) != 0)  //Enable
	    							pAd->ApCfg.MBSSID[i].IsolateInterStaTraffic = TRUE;
	    						else //Disable
	    							pAd->ApCfg.MBSSID[i].IsolateInterStaTraffic = FALSE;

	    						DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) NoForwarding=%ld\n", i, pAd->ApCfg.MBSSID[i].IsolateInterStaTraffic));
						    }
						}
						//NoForwardingBTNBSSID
						if(RTMPGetKeyParameter("NoForwardingBTNBSSID", tmpbuf, 10, buffer))
						{
							if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
								pAd->ApCfg.IsolateInterStaTrafficBTNBSSID = TRUE;
							else //Disable
								pAd->ApCfg.IsolateInterStaTrafficBTNBSSID = FALSE;

							DBGPRINT(RT_DEBUG_TRACE, ("NoForwardingBTNBSSID=%ld\n", pAd->ApCfg.IsolateInterStaTrafficBTNBSSID));
						}
						//HideSSID
						if(RTMPGetKeyParameter("HideSSID", tmpbuf, 32, buffer))
						{
							for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
						    {
								int apidx = i;

								if (i >= pAd->ApCfg.BssidNum)
									break;

								if(simple_strtol(macptr, 0, 10) != 0)  //Enable
									pAd->ApCfg.MBSSID[apidx].bHideSsid = TRUE;								
								else //Disable
									pAd->ApCfg.MBSSID[apidx].bHideSsid = FALSE;								

								DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) HideSSID=%d\n", i, pAd->ApCfg.MBSSID[apidx].bHideSsid));
							}
						}

						//StationKeepAlive
						if(RTMPGetKeyParameter("StationKeepAlive", tmpbuf, 32, buffer))
						{
							for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
						    {
								int apidx = i;

								if (i >= pAd->ApCfg.BssidNum)
									break;

								pAd->ApCfg.MBSSID[apidx].StationKeepAliveTime = simple_strtol(macptr, 0, 10);
								DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) StationKeepAliveTime=%d\n", i, pAd->ApCfg.MBSSID[apidx].StationKeepAliveTime));
							}
						}
					}
#endif // CONFIG_AP_SUPPORT //
					//ShortSlot
					if(RTMPGetKeyParameter("ShortSlot", tmpbuf, 10, buffer))
					{
						if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
							pAd->CommonCfg.bUseShortSlotTime = TRUE;
						else //Disable
							pAd->CommonCfg.bUseShortSlotTime = FALSE;

						DBGPRINT(RT_DEBUG_TRACE, ("ShortSlot=%d\n", pAd->CommonCfg.bUseShortSlotTime));
					}
#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					//if (pAd->OpMode == OPMODE_AP)
					{
						//AutoChannelSelect
						if(RTMPGetKeyParameter("AutoChannelSelect", tmpbuf, 10, buffer))
						{
							if(simple_strtol(tmpbuf, 0, 10) != 0)  //Enable
								pAd->ApCfg.bAutoChannelAtBootup = TRUE;
							else
								pAd->ApCfg.bAutoChannelAtBootup = FALSE;

							DBGPRINT(RT_DEBUG_TRACE, ("AutoChannelAtBootup=%d\n", pAd->ApCfg.bAutoChannelAtBootup));
						}
						//IEEE8021X
						if(RTMPGetKeyParameter("IEEE8021X", tmpbuf, 10, buffer))
						{
						    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
						    {
								if (i >= pAd->ApCfg.BssidNum)
									break;

	    						if(simple_strtol(macptr, 0, 10) != 0)  //Enable
	    							pAd->ApCfg.MBSSID[i].IEEE8021X = TRUE;
	    						else //Disable
	    							pAd->ApCfg.MBSSID[i].IEEE8021X = FALSE;

	    						DBGPRINT(RT_DEBUG_TRACE, ("IEEE8021X=%d\n", pAd->ApCfg.MBSSID[i].IEEE8021X));
						    }
						}
					}
#endif // CONFIG_AP_SUPPORT //
					//IEEE80211H
					if(RTMPGetKeyParameter("IEEE80211H", tmpbuf, 10, buffer))
					{
					    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
					    {
    						if(simple_strtol(macptr, 0, 10) != 0)  //Enable
    							pAd->CommonCfg.bIEEE80211H = TRUE;
    						else //Disable
    							pAd->CommonCfg.bIEEE80211H = FALSE;

    						DBGPRINT(RT_DEBUG_TRACE, ("IEEE80211H=%d\n", pAd->CommonCfg.bIEEE80211H));
					    }
					}
					//CSPeriod
					if(RTMPGetKeyParameter("CSPeriod", tmpbuf, 10, buffer))
					{
					    if(simple_strtol(tmpbuf, 0, 10) != 0)
							pAd->CommonCfg.RadarDetect.CSPeriod = simple_strtol(tmpbuf, 0, 10);
						else
							pAd->CommonCfg.RadarDetect.CSPeriod = 0;

   						DBGPRINT(RT_DEBUG_TRACE, ("CSPeriod=%d\n", pAd->CommonCfg.RadarDetect.CSPeriod));
					}

					//RDRegion
					if(RTMPGetKeyParameter("RDRegion", tmpbuf, 128, buffer))
					{
						if ((strncmp(tmpbuf, "JAP_W53", 7) == 0) || (strncmp(tmpbuf, "jap_w53", 7) == 0))
						{
							pAd->CommonCfg.RadarDetect.RDDurRegion = JAP_W53;
							pAd->CommonCfg.RadarDetect.DfsSessionTime = 15;
						}
						else if ((strncmp(tmpbuf, "JAP_W56", 7) == 0) || (strncmp(tmpbuf, "jap_w56", 7) == 0))
						{
							pAd->CommonCfg.RadarDetect.RDDurRegion = JAP_W56;
							pAd->CommonCfg.RadarDetect.DfsSessionTime = 13;
						}
						else if ((strncmp(tmpbuf, "JAP", 3) == 0) || (strncmp(tmpbuf, "jap", 3) == 0))
						{
							pAd->CommonCfg.RadarDetect.RDDurRegion = JAP;
							pAd->CommonCfg.RadarDetect.DfsSessionTime = 5;
						}
						else  if ((strncmp(tmpbuf, "FCC", 3) == 0) || (strncmp(tmpbuf, "fcc", 3) == 0))
						{
							pAd->CommonCfg.RadarDetect.RDDurRegion = FCC;
							pAd->CommonCfg.RadarDetect.DfsSessionTime = 5;
						}
						else if ((strncmp(tmpbuf, "CE", 2) == 0) || (strncmp(tmpbuf, "ce", 2) == 0))
						{
							pAd->CommonCfg.RadarDetect.RDDurRegion = CE;
							pAd->CommonCfg.RadarDetect.DfsSessionTime = 13;
						}
						else
						{
							pAd->CommonCfg.RadarDetect.RDDurRegion = CE;
							pAd->CommonCfg.RadarDetect.DfsSessionTime = 13;
						}

						DBGPRINT(RT_DEBUG_TRACE, ("RDRegion=%d\n", pAd->CommonCfg.RadarDetect.RDDurRegion));
					}
					else
					{
						pAd->CommonCfg.RadarDetect.RDDurRegion = CE;
						pAd->CommonCfg.RadarDetect.DfsSessionTime = 13;
					}

					//WirelessEvent
					if(RTMPGetKeyParameter("WirelessEvent", tmpbuf, 10, buffer))
					{				
#if WIRELESS_EXT >= 15
					    if(simple_strtol(tmpbuf, 0, 10) != 0)
							pAd->CommonCfg.bWirelessEvent = simple_strtol(tmpbuf, 0, 10);
						else
							pAd->CommonCfg.bWirelessEvent = 0;	// disable
#else
						pAd->CommonCfg.bWirelessEvent = 0;	// disable
#endif
   						DBGPRINT(RT_DEBUG_TRACE, ("WirelessEvent=%d\n", pAd->CommonCfg.bWirelessEvent));
					}
					if(RTMPGetKeyParameter("WiFiTest", tmpbuf, 10, buffer))
					{				
					    if(simple_strtol(tmpbuf, 0, 10) != 0)
							pAd->CommonCfg.bWiFiTest= simple_strtol(tmpbuf, 0, 10);
						else
							pAd->CommonCfg.bWiFiTest = 0;	// disable

   						DBGPRINT(RT_DEBUG_TRACE, ("WiFiTest=%d\n", pAd->CommonCfg.bWiFiTest));
					}
#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
						//PreAuth
						if(RTMPGetKeyParameter("PreAuth", tmpbuf, 10, buffer))
						{
						    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
						    {
								if (i >= pAd->ApCfg.BssidNum)
									break;

	    						if(simple_strtol(macptr, 0, 10) != 0)  //Enable
	    							pAd->ApCfg.MBSSID[i].PreAuth = TRUE;
	    						else //Disable
	    							pAd->ApCfg.MBSSID[i].PreAuth = FALSE;

	    						DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) PreAuth=%d\n", i, pAd->ApCfg.MBSSID[i].PreAuth));
						    }
						}
					}
#endif // CONFIG_AP_SUPPORT //					
					//AuthMode
					if(RTMPGetKeyParameter("AuthMode", tmpbuf, 128, buffer))
					{
#ifdef CONFIG_AP_SUPPORT
						IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
						{
					   		 for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
					    		{
								int apidx;

								if (i < pAd->ApCfg.BssidNum)
								{							
									apidx = i;
								}
								else
								{
									break;
								}

								if ((strncmp(macptr, "WEPAUTO", 4) == 0) || (strncmp(macptr, "wepauto", 4) == 0))
					    				pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeAutoSwitch;
    								else if ((strncmp(macptr, "OPEN", 4) == 0) || (strncmp(macptr, "open", 4) == 0))
    									pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeOpen;
    								else if ((strncmp(macptr, "SHARED", 6) == 0) || (strncmp(macptr, "shared", 6) == 0))
    									pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeShared;
    								else if ((strncmp(macptr, "WPA2PSK", 7) == 0) || (strncmp(macptr, "wpa2psk", 7) == 0))
    									pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA2PSK;
    								else if ((strncmp(macptr, "WPA2", 4) == 0) || (strncmp(macptr, "wpa2", 4) == 0))
    									pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA2;
    								else if ((strncmp(macptr, "WPA1WPA2", 8) == 0) || (strncmp(macptr, "wpa1wpa2", 8) == 0))
    									pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA1WPA2;
    								else if ((strncmp(macptr, "WPAPSKWPA2PSK", 13) == 0) || (strncmp(macptr, "wpapskwpa2psk", 13) == 0))
    									pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA1PSKWPA2PSK;
    								else if ((strncmp(macptr, "WPAPSK", 6) == 0) || (strncmp(macptr, "wpapsk", 6) == 0))
    									pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPAPSK;
    								else if ((strncmp(macptr, "WPA", 3) == 0) || (strncmp(macptr, "wpa", 3) == 0))
    									pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA;
    								else //Default
    									pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeOpen;

    								RTMPMakeRSNIE(pAd, pAd->ApCfg.MBSSID[apidx].AuthMode, pAd->ApCfg.MBSSID[apidx].WepStatus, apidx);
    								DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) AuthMode=%d\n", i, pAd->ApCfg.MBSSID[apidx].AuthMode));
							}
						}
#endif // CONFIG_AP_SUPPORT //
					}
					//EncrypType
					if(RTMPGetKeyParameter("EncrypType", tmpbuf, 128, buffer))
					{
#ifdef CONFIG_AP_SUPPORT
						IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
						{
							//We need to reset the WepStatus of all interfaces as 1 (Ndis802_11WEPDisabled) first. 
							//		Or it may have problem when some interface enabled but didn't configure it.
							for ( i= 0; i<pAd->ApCfg.BssidNum; i++)
								pAd->ApCfg.MBSSID[i].WepStatus = Ndis802_11WEPDisabled;
					    		for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
					    		{
								int apidx;

								if (i<pAd->ApCfg.BssidNum)
								{							
									apidx = i;
								}
					        		else
								{
									break;
								}

								if ((strncmp(macptr, "NONE", 4) == 0) || (strncmp(macptr, "none", 4) == 0))
					            			pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11WEPDisabled;
					        		else if ((strncmp(macptr, "WEP", 3) == 0) || (strncmp(macptr, "wep", 3) == 0))
					            			pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11WEPEnabled;
					        		else if ((strncmp(macptr, "TKIPAES", 7) == 0) || (strncmp(macptr, "tkipaes", 7) == 0))
					            			pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11Encryption4Enabled;
					        		else if ((strncmp(macptr, "TKIP", 4) == 0) || (strncmp(macptr, "tkip", 4) == 0))
					            			pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11Encryption2Enabled;
					        		else if ((strncmp(macptr, "AES", 3) == 0) || (strncmp(macptr, "aes", 3) == 0))
					            			pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11Encryption3Enabled;
					        		else
					            			pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11WEPDisabled;

					        		RTMPMakeRSNIE(pAd, pAd->ApCfg.MBSSID[apidx].AuthMode, pAd->ApCfg.MBSSID[apidx].WepStatus, apidx);
					        		DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) EncrypType=%d\n", i, pAd->ApCfg.MBSSID[apidx].WepStatus));
					    		}
						}
#endif // CONFIG_AP_SUPPORT //

					}

#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
						//RekeyMethod
						if(RTMPGetKeyParameter("RekeyMethod", tmpbuf, 128, buffer))
						{
							if ((strcmp(tmpbuf, "TIME") == 0) || (strcmp(tmpbuf, "time") == 0))
								pAd->ApCfg.WPAREKEY.ReKeyMethod = TIME_REKEY;
							else if ((strcmp(tmpbuf, "PKT") == 0) || (strcmp(tmpbuf, "pkt") == 0))
								pAd->ApCfg.WPAREKEY.ReKeyMethod = PKT_REKEY;
							else if ((strcmp(tmpbuf, "DISABLE") == 0) || (strcmp(tmpbuf, "disable") == 0))
								pAd->ApCfg.WPAREKEY.ReKeyMethod = DISABLE_REKEY;
							else
								pAd->ApCfg.WPAREKEY.ReKeyMethod = DISABLE_REKEY;

							DBGPRINT(RT_DEBUG_TRACE, ("ReKeyMethod=%ld\n", pAd->ApCfg.WPAREKEY.ReKeyMethod));
						}
						//RekeyInterval
						if(RTMPGetKeyParameter("RekeyInterval", tmpbuf, 255, buffer))
						{
							if((simple_strtol(tmpbuf, 0, 10) >= 0) && (simple_strtol(tmpbuf, 0, 10) < MAX_REKEY_INTER))
								pAd->ApCfg.WPAREKEY.ReKeyInterval = simple_strtol(tmpbuf, 0, 10);
							else //Default
								pAd->ApCfg.WPAREKEY.ReKeyInterval = 10;

							DBGPRINT(RT_DEBUG_TRACE, ("ReKeyInterval=%ld\n", pAd->ApCfg.WPAREKEY.ReKeyInterval));
						}
						//PMKCachePeriod
						if(RTMPGetKeyParameter("PMKCachePeriod", tmpbuf, 255, buffer))
						{
							pAd->ApCfg.PMKCachePeriod = simple_strtol(tmpbuf, 0, 10) * 60 * HZ;

							DBGPRINT(RT_DEBUG_TRACE, ("PMKCachePeriod=%ld\n", pAd->ApCfg.PMKCachePeriod));
						}
					}
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
						//WPAPSK_KEY
						if(TRUE)
						{
							CHAR tok_str[16];
							BOOLEAN bWPAPSKxIsUsed = FALSE;

							DBGPRINT(RT_DEBUG_TRACE, ("pAd->ApCfg.BssidNum=%d\n", pAd->ApCfg.BssidNum));

							for (i = 0; i < pAd->ApCfg.BssidNum; i++)
							{
								sprintf(tok_str, "WPAPSK%d", i + 1);
							if(RTMPGetCriticalParameter(tok_str, tmpbuf, 65, buffer))
								{
									rtmp_parse_wpapsk_buffer_from_file(pAd, tmpbuf, i);
									
									if (bWPAPSKxIsUsed == FALSE)
									{
										bWPAPSKxIsUsed = TRUE;
									}
								}
							}
							if (bWPAPSKxIsUsed == FALSE)
							{
							if (RTMPGetCriticalParameter("WPAPSK", tmpbuf, 512, buffer))
								{
									if (pAd->ApCfg.BssidNum == 1)
									{
										rtmp_parse_wpapsk_buffer_from_file(pAd, tmpbuf, BSS0);
									}
									else
									{
										// Anyway, we still do the legacy dissection of the whole WPAPSK passphrase.
									    for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
									    {
											rtmp_parse_wpapsk_buffer_from_file(pAd, macptr, i);
									    }

									}
								}
							}

#ifdef DBG
							for (i = 0; i < pAd->ApCfg.BssidNum; i++)
							{
								int j;
								
	                            				DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) WPAPSK Key => \n", i));
	                            				for (j = 0; j < 32; j++)
	                            				{
	                                				DBGPRINT(RT_DEBUG_TRACE, ("%02x:", pAd->ApCfg.MBSSID[i].PMK[j]));
	                                				if ((j%16) == 15)
	                                    					DBGPRINT(RT_DEBUG_TRACE, ("\n"));
	                            				}
	                            				DBGPRINT(RT_DEBUG_TRACE, ("\n"));
							}
#endif
						}
					}
#endif // CONFIG_AP_SUPPORT //


    							
					//DefaultKeyID, KeyType, KeyStr
					rtmp_read_key_parms_from_file(pAd, tmpbuf, buffer);

					//HSCounter
					/*if(RTMPGetKeyParameter("HSCounter", tmpbuf, 10, buffer))
					{
						switch (simple_strtol(tmpbuf, 0, 10))
						{
							case 1: //Enable
								pAd->CommonCfg.bEnableHSCounter = TRUE;
								break;
							case 0: //Disable
							default:
								pAd->CommonCfg.bEnableHSCounter = FALSE;
								break;
						}
						DBGPRINT(RT_DEBUG_TRACE, "HSCounter=%d\n", pAd->CommonCfg.bEnableHSCounter);
					}*/
#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
						//Access Control List
						rtmp_read_acl_parms_from_file(pAd, tmpbuf, buffer);

#ifdef APCLI_SUPPORT					
						rtmp_read_ap_client_from_file(pAd, tmpbuf, buffer);
#endif // APCLI_SUPPORT //

#ifdef IGMP_SNOOP_SUPPORT
						// Igmp Snooping information
						rtmp_read_igmp_snoop_from_file(pAd, tmpbuf, buffer);
#endif // IGMP_SNOOP_SUPPORT //
	
#ifdef WDS_SUPPORT
						rtmp_read_wds_from_file(pAd, tmpbuf, buffer);
#else // WDS_SUPPORT //
						pAd->WdsTab.Mode = WDS_DISABLE_MODE;
#endif // WDS_SUPPORT //

						rtmp_read_radius_parms_from_file(pAd, tmpbuf, buffer);

#ifdef IDS_SUPPORT
						rtmp_read_ids_from_file(pAd, tmpbuf, buffer);
#endif // IDS_SUPPORT //
					}

#endif // CONFIG_AP_SUPPORT //
					HTParametersHook(pAd, tmpbuf, buffer);

#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
#ifdef WSC_AP_SUPPORT
						//WscConfMode
						if(RTMPGetKeyParameter("WscConfMode", tmpbuf, 10, buffer))
						{
							INT WscConfMode = simple_strtol(tmpbuf, 0, 10);

							if (WscConfMode >= 0 && WscConfMode < 8)
							{
								pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfMode = WscConfMode;
							}
							else
							{
								pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfMode = WSC_DISABLE;
							}

							DBGPRINT(RT_DEBUG_TRACE, ("WscConfMode=%d\n", pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfMode));
						}

						//WscConfStatus
						if(RTMPGetKeyParameter("WscConfStatus", tmpbuf, 10, buffer))
						{
							pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfStatus = (INT) simple_strtol(tmpbuf, 0, 10);

							DBGPRINT(RT_DEBUG_TRACE, ("WscConfStatus=%d\n", pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfStatus));
						}
#if 0
                    //WscPinCode
					if(RTMPGetCriticalParameter("WscPinCode", tmpbuf, 10, buffer))
					{
						pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscEnrolleePinCode = (INT) simple_strtol(tmpbuf, 0, 10);
                        if (ValidateChecksum(pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscEnrolleePinCode) == FALSE)
                        {
                            pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscEnrolleePinCode = 0;
                            DBGPRINT(RT_DEBUG_TRACE, ("ApCliWscPinCode InValid!!\n"));
                        }
                        else
						    DBGPRINT(RT_DEBUG_TRACE, ("WscPinCode=%d\n", pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscEnrolleePinCode));
					}
#endif // if 0
#endif // WSC_AP_SUPPORT //

#ifdef CARRIER_DETECTION_SUPPORT
						//CarrierDetect
						if(RTMPGetKeyParameter("CarrierDetect", tmpbuf, 128, buffer))
						{
							if ((strncmp(tmpbuf, "0", 1) == 0))
								pAd->CommonCfg.CarrierDetect.Enable = FALSE;
							else if ((strncmp(tmpbuf, "1", 1) == 0))
								pAd->CommonCfg.CarrierDetect.Enable = TRUE;
							else
								pAd->CommonCfg.CarrierDetect.Enable = FALSE;

							DBGPRINT(RT_DEBUG_TRACE, ("CarrierDetect.Enable=%d\n", pAd->CommonCfg.CarrierDetect.Enable));
						}
						else
							pAd->CommonCfg.CarrierDetect.Enable = FALSE;
#endif // CARRIER_DETECTION_SUPPORT //
					}
#endif // CONFIG_AP_SUPPORT //


#ifdef ETH_CONVERT_SUPPORT
					// Ethernet Converter Operation Mode.
					if (RTMPGetKeyParameter("EthConvertMode", tmpbuf, 32, buffer))
					{	
						Set_EthConvertMode_Proc(pAd, tmpbuf);
						DBGPRINT(RT_DEBUG_TRACE, ("EthConvertMode=%d\n", pAd->EthConvert.ECMode));
					}

					// Ethernet Converter Operation Mode.
					if (RTMPGetKeyParameter("EthCloneMac", tmpbuf, 32, buffer))
					{
						Set_EthCloneMac_Proc(pAd, tmpbuf);
						DBGPRINT(RT_DEBUG_TRACE, ("EthCloneMac=%02x:%02x:%02x:%02x:%02x:%02x\n", 
								pAd->EthConvert.EthCloneMac[0], pAd->EthConvert.EthCloneMac[1], pAd->EthConvert.EthCloneMac[2],
								pAd->EthConvert.EthCloneMac[3], pAd->EthConvert.EthCloneMac[4], pAd->EthConvert.EthCloneMac[5]));
					}
#endif // ETH_CONVERT_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
#ifdef MCAST_RATE_SPECIFIC
						// McastPhyMode
						if (RTMPGetKeyParameter("McastPhyMode", tmpbuf, 32, buffer))
						{	
							UCHAR PhyMode = simple_strtol(tmpbuf, 0, 10);
							switch (PhyMode)
							{
								case 0: // disable
									pAd->CommonCfg.McastTransmitPhyMode = MCAST_DISABLE;
									break;
								case 1:	// CCK
									pAd->CommonCfg.McastTransmitPhyMode = MCAST_CCK;
									break;
								case 2:	// CCK
									pAd->CommonCfg.McastTransmitPhyMode = MCAST_OFDM;
									break;
								default:
									printk("unknow Muticast PhyMode %d.\n", PhyMode);
									printk("0:Disable 1:CCK, 2:OFDM.\n");
									break;
							}
						}
						else
							pAd->CommonCfg.McastTransmitPhyMode = MCAST_DISABLE;

						// McastMcs
						if (RTMPGetKeyParameter("McastMcs", tmpbuf, 32, buffer))
						{
							UCHAR Mcs = simple_strtol(tmpbuf, 0, 10);
							if (Mcs <= 15)
								pAd->CommonCfg.McastTransmitMcs = Mcs;
							else
								pAd->CommonCfg.McastTransmitMcs = 0;
						}
						else
							pAd->CommonCfg.McastTransmitMcs = 0;
#endif // MCAST_RATE_SPECIFIC //

#ifdef DOT11N_DRAFT3
						if (RTMPGetKeyParameter("OBSSScanParam", tmpbuf, 32, buffer))
						{	int ObssScanValue;
							for (i = 0, macptr = rstrtok(tmpbuf,";"); macptr; macptr = rstrtok(NULL,";"), i++)
							{
								ObssScanValue = simple_strtol(macptr, 0, 10);
								switch (i)
								{
									case 0:
										if (ObssScanValue < 5 || ObssScanValue > 1000)
										{
											DBGPRINT(RT_DEBUG_ERROR, ("Invalid OBSSScanParam for Dot11OBssScanPassiveDwell(%d), should in range 5~1000\n", ObssScanValue));
										}
										else
										{
											pAd->CommonCfg.Dot11OBssScanPassiveDwell = ObssScanValue;	// Unit : TU. 5~1000
											DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11OBssScanPassiveDwell=%d\n", ObssScanValue));
										}
										break;
									case 1:
										if (ObssScanValue < 10 || ObssScanValue > 1000)
										{
											DBGPRINT(RT_DEBUG_ERROR, ("Invalid OBSSScanParam for Dot11OBssScanActiveDwell(%d), should in range 10~1000\n", ObssScanValue));
										}
										else
										{
											pAd->CommonCfg.Dot11OBssScanActiveDwell = ObssScanValue;	// Unit : TU. 10~1000
											DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11OBssScanActiveDwell=%d\n", ObssScanValue));
										}
										break;
									case 2:
										pAd->CommonCfg.Dot11BssWidthTriggerScanInt = ObssScanValue;	// Unit : Second
										DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11BssWidthTriggerScanInt=%d\n", ObssScanValue));
										break;
									case 3:
										if (ObssScanValue < 200 || ObssScanValue > 10000)
										{
											DBGPRINT(RT_DEBUG_ERROR, ("Invalid OBSSScanParam for Dot11OBssScanPassiveTotalPerChannel(%d), should in range 200~10000\n", ObssScanValue));
										}
										else
										{
											pAd->CommonCfg.Dot11OBssScanPassiveTotalPerChannel = ObssScanValue;	// Unit : TU. 200~10000
											DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11OBssScanPassiveTotalPerChannel=%d\n", ObssScanValue));
										}
										break;
									case 4:
										if (ObssScanValue < 20 || ObssScanValue > 10000)
										{
											DBGPRINT(RT_DEBUG_ERROR, ("Invalid OBSSScanParam for Dot11OBssScanActiveTotalPerChannel(%d), should in range 20~10000\n", ObssScanValue));
										}
										else
										{
											pAd->CommonCfg.Dot11OBssScanActiveTotalPerChannel = ObssScanValue;	// Unit : TU. 20~10000
											DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11OBssScanActiveTotalPerChannel=%d\n", ObssScanValue));
										}
										break;
									case 5:
										pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor = ObssScanValue;
										DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11BssWidthChanTranDelayFactor=%d\n", ObssScanValue));
										break;
									case 6:
										pAd->CommonCfg.Dot11OBssScanActivityThre = ObssScanValue;	// Unit : percentage
										DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11BssWidthChanTranDelayFactor=%d\n", ObssScanValue));
										break;
								}			
							}
							
							if (i != 7)
							{
								DBGPRINT(RT_DEBUG_ERROR, ("Wrong OBSSScanParamtetrs format in dat file!!!!! Use default value.\n"));

								pAd->CommonCfg.Dot11OBssScanPassiveDwell = dot11OBSSScanPassiveDwell;	// Unit : TU. 5~1000
								pAd->CommonCfg.Dot11OBssScanActiveDwell = dot11OBSSScanActiveDwell;	// Unit : TU. 10~1000
								pAd->CommonCfg.Dot11BssWidthTriggerScanInt = dot11BSSWidthTriggerScanInterval;	// Unit : Second	
								pAd->CommonCfg.Dot11OBssScanPassiveTotalPerChannel = dot11OBSSScanPassiveTotalPerChannel;	// Unit : TU. 200~10000
								pAd->CommonCfg.Dot11OBssScanActiveTotalPerChannel = dot11OBSSScanActiveTotalPerChannel;	// Unit : TU. 20~10000
								pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor = dot11BSSWidthChannelTransactionDelayFactor;
								pAd->CommonCfg.Dot11OBssScanActivityThre = dot11BSSScanActivityThreshold;	// Unit : percentage
							}

							pAd->CommonCfg.Dot11BssWidthChanTranDelay = (pAd->CommonCfg.Dot11BssWidthTriggerScanInt * pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor);
							DBGPRINT(RT_DEBUG_TRACE, ("OBSSScanParam for Dot11BssWidthChanTranDelay=%d\n", pAd->CommonCfg.Dot11BssWidthChanTranDelay));
						}
#endif // DOT11N_DRAFT3 //
					}
#endif // CONFIG_AP_SUPPORT //
#ifdef WSC_INCLUDED
                    if (RTMPGetKeyParameter("WscVendorPinCode", tmpbuf, 256, buffer))
                    {
                    	BOOLEAN     validatePin;

                        int value = simple_strtol(tmpbuf, 0, 10);

						if (strlen(tmpbuf) == 4)
							validatePin = TRUE;
						else
							validatePin = ValidateChecksum(value);

						if (validatePin)
                        {
#ifdef CONFIG_AP_SUPPORT
							IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
							{
								pAd->ApCfg.MBSSID[BSS0].WscControl.WscEnrolleePinCode = value;
								if (strlen(tmpbuf) == 4)
									pAd->ApCfg.MBSSID[BSS0].WscControl.WscEnrolleePinCodeLen = 4;
								else
									pAd->ApCfg.MBSSID[BSS0].WscControl.WscEnrolleePinCodeLen = 8;
							}
#endif // CONFIG_AP_SUPPORT //
                        DBGPRINT(RT_DEBUG_TRACE, ("%s - WscVendorPinCode= (%d)\n", __FUNCTION__, value));
                        }
                        else
                        {
                            DBGPRINT(RT_DEBUG_ERROR, ("%s - WscVendorPinCode: invalid pin code (%d)\n", __FUNCTION__, value));
                        }

                    }
#endif // WSC_INCLUDED //
				}
			}
			else
			{
				DBGPRINT(RT_DEBUG_TRACE, ("--> %s does not have a write method\n", src));
			}
			
			retval=filp_close(srcf,NULL);
			
			if (retval)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("--> Error %d closing %s\n", -retval, src));
			}
		}
	}

	set_fs(orgfs);
	current->fsuid = orgfsuid;
	current->fsgid = orgfsgid;

	kfree(buffer);
	kfree(tmpbuf);

	return (NDIS_STATUS_SUCCESS);	
}


#if 0
	//Init Ba CApability parameters.
	pAd->CommonCfg.DesiredHtPhy.MpduDensity = (UCHAR)pAd->CommonCfg.BACapability.field.MpduDensity;
	pAd->CommonCfg.DesiredHtPhy.AmsduEnable = (USHORT)pAd->CommonCfg.BACapability.field.AmsduEnable;
	pAd->CommonCfg.DesiredHtPhy.AmsduSize= (USHORT)pAd->CommonCfg.BACapability.field.AmsduSize;
	pAd->CommonCfg.DesiredHtPhy.MimoPs= (USHORT)pAd->CommonCfg.BACapability.field.MMPSmode;
	// UPdata to HT IE
	pAd->CommonCfg.HtCapability.HtCapInfo.MimoPs = (USHORT)pAd->CommonCfg.BACapability.field.MMPSmode;
	pAd->CommonCfg.HtCapability.HtCapInfo.AMsduSize = (USHORT)pAd->CommonCfg.BACapability.field.AmsduSize;
	pAd->CommonCfg.HtCapability.HtCapParm.MpduDensity = (UCHAR)pAd->CommonCfg.BACapability.field.MpduDensity;

   	pAd->CommonCfg.BACapability.field.MpduDensity = 0;
	pAd->CommonCfg.BACapability.field.Policy = IMMED_BA;
	pAd->CommonCfg.BACapability.field.RxBAWinLimit = 32;
	pAd->CommonCfg.BACapability.field.TxBAWinLimit = 32;
	pAd->CommonCfg.HTPhyMode.field.BW = BW_c40;
	pAd->CommonCfg.HTPhyMode.field.MCS = MCS_15;
	pAd->CommonCfg.HTPhyMode.field.ShortGI = GI_400;
	pAd->CommonCfg.HTPhyMode.field.STBC = STBC_NONE;
	pAd->CommonCfg.TxRate = RATE_6;

#endif


static void	HTParametersHook(
	IN	PRTMP_ADAPTER pAd, 
	IN	CHAR		  *pValueStr,
	IN	CHAR		  *pInput)
{

	INT Value;
#ifdef CONFIG_AP_SUPPORT	
	INT			i=0;
	PUCHAR		Bufptr;
#endif // CONFIG_AP_SUPPORT //

    if (RTMPGetKeyParameter("HT_PROTECT", pValueStr, 25, pInput))
    {
        Value = simple_strtol(pValueStr, 0, 10);
        if (Value == 0)
        {
            pAd->CommonCfg.bHTProtect = FALSE;
        }
        else
        {
            pAd->CommonCfg.bHTProtect = TRUE;
        }
        DBGPRINT(RT_DEBUG_TRACE, ("HT: Protection  = %s\n", (Value==0) ? "Disable" : "Enable"));
    }

    if (RTMPGetKeyParameter("HT_MIMOPSEnable", pValueStr, 25, pInput))
    {
        Value = simple_strtol(pValueStr, 0, 10);
        if (Value == 0)
        {
            pAd->CommonCfg.bMIMOPSEnable = FALSE;
        }
        else
        {
            pAd->CommonCfg.bMIMOPSEnable = TRUE;
        }
        DBGPRINT(RT_DEBUG_TRACE, ("HT: MIMOPSEnable  = %s\n", (Value==0) ? "Disable" : "Enable"));
    }


    if (RTMPGetKeyParameter("HT_MIMOPSMode", pValueStr, 25, pInput))
    {
#if 0	// unused variable
		char sMIMOPS[4][20]={"static MIMO", 
					  "dynamic MIMO",
					  "NA",
					  "No limitation"};
#endif
        Value = simple_strtol(pValueStr, 0, 10);
        if (Value > MMPS_ENABLE)
        {
			pAd->CommonCfg.BACapability.field.MMPSmode = MMPS_ENABLE;
        }
        else
        {
            //TODO: add mimo power saving mechanism
            pAd->CommonCfg.BACapability.field.MMPSmode = MMPS_ENABLE;
			//pAd->CommonCfg.BACapability.field.MMPSmode = Value;
        }
        DBGPRINT(RT_DEBUG_TRACE, ("HT: MIMOPS Mode  = %d\n", Value));
    }

    if (RTMPGetKeyParameter("HT_BADecline", pValueStr, 25, pInput))
    {
        Value = simple_strtol(pValueStr, 0, 10);
        if (Value == 0)
        {
            pAd->CommonCfg.bBADecline = FALSE;
        }
        else
        {
            pAd->CommonCfg.bBADecline = TRUE;
        }
        DBGPRINT(RT_DEBUG_TRACE, ("HT: BA Decline  = %s\n", (Value==0) ? "Disable" : "Enable"));
    }


    if (RTMPGetKeyParameter("HT_DisableReordering", pValueStr, 25, pInput))
    {
        Value = simple_strtol(pValueStr, 0, 10);
        if (Value == 0)
        {
            pAd->CommonCfg.bDisableReordering = FALSE;
        }
        else
        {
            pAd->CommonCfg.bDisableReordering = TRUE;
        }
        DBGPRINT(RT_DEBUG_TRACE, ("HT: DisableReordering  = %s\n", (Value==0) ? "Disable" : "Enable"));
    }

    if (RTMPGetKeyParameter("HT_AutoBA", pValueStr, 25, pInput))
    {
        Value = simple_strtol(pValueStr, 0, 10);
        if (Value == 0)
        {
            pAd->CommonCfg.BACapability.field.AutoBA = FALSE;
        }
        else
        {
            pAd->CommonCfg.BACapability.field.AutoBA = TRUE;
        }
        pAd->CommonCfg.REGBACapability.field.AutoBA = pAd->CommonCfg.BACapability.field.AutoBA;
        DBGPRINT(RT_DEBUG_TRACE, ("HT: Auto BA  = %s\n", (Value==0) ? "Disable" : "Enable"));
    }

	// Tx_+HTC frame
    if (RTMPGetKeyParameter("HT_HTC", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);
		if (Value == 0)
		{
			pAd->HTCEnable = FALSE;
		}
		else
		{
            pAd->HTCEnable = TRUE;
		}
		DBGPRINT(RT_DEBUG_TRACE, ("HT: Tx +HTC frame = %s\n", (Value==0) ? "Disable" : "Enable"));
	}

	// Enable HT Link Adaptation Control
	if (RTMPGetKeyParameter("HT_LinkAdapt", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);
		if (Value == 0)
		{
			pAd->bLinkAdapt = FALSE;
		}
		else
		{
			pAd->HTCEnable = TRUE;
			pAd->bLinkAdapt = TRUE;
		}
		DBGPRINT(RT_DEBUG_TRACE, ("HT: Link Adaptation Control = %s\n", (Value==0) ? "Disable" : "Enable(+HTC)"));
	}

	// Reverse Direction Mechanism
    if (RTMPGetKeyParameter("HT_RDG", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);
		if (Value == 0)
		{			
			pAd->CommonCfg.bRdg = FALSE;
		}
		else
		{
			pAd->HTCEnable = TRUE;
            pAd->CommonCfg.bRdg = TRUE;
		}
		DBGPRINT(RT_DEBUG_TRACE, ("HT: RDG = %s\n", (Value==0) ? "Disable" : "Enable(+HTC)"));
	}




	// Tx A-MSUD ?
    if (RTMPGetKeyParameter("HT_AMSDU", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);
		if (Value == 0)
		{
			pAd->CommonCfg.BACapability.field.AmsduEnable = FALSE;
		}
		else
		{
            pAd->CommonCfg.BACapability.field.AmsduEnable = TRUE;
		}
		DBGPRINT(RT_DEBUG_TRACE, ("HT: Tx A-MSDU = %s\n", (Value==0) ? "Disable" : "Enable"));
	}

	// MPDU Density
    if (RTMPGetKeyParameter("HT_MpduDensity", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);
		if (Value <=7 && Value >= 0)
		{		
			pAd->CommonCfg.BACapability.field.MpduDensity = Value;
			DBGPRINT(RT_DEBUG_TRACE, ("HT: MPDU Density = %d\n", Value));
		}
		else
		{
			pAd->CommonCfg.BACapability.field.MpduDensity = 4;
			DBGPRINT(RT_DEBUG_TRACE, ("HT: MPDU Density = %d (Default)\n", 4));
		}
	}

	// Max Rx BA Window Size
    if (RTMPGetKeyParameter("HT_BAWinSize", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);

		// Intel IOT
		Value = 64;
		if (Value >=1 && Value <= 64)
		{		
			pAd->CommonCfg.REGBACapability.field.RxBAWinLimit = Value;
			pAd->CommonCfg.BACapability.field.RxBAWinLimit = Value;
			DBGPRINT(RT_DEBUG_TRACE, ("HT: BA Windw Size = %d\n", Value));
		}
		else
		{
            pAd->CommonCfg.REGBACapability.field.RxBAWinLimit = 64;
			pAd->CommonCfg.BACapability.field.RxBAWinLimit = 64;
			DBGPRINT(RT_DEBUG_TRACE, ("HT: BA Windw Size = 64 (Defualt)\n"));
		}

	}

	// Guard Interval
	if (RTMPGetKeyParameter("HT_GI", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);

		if (Value == GI_400)
		{
			pAd->CommonCfg.RegTransmitSetting.field.ShortGI = GI_400;
		}
		else
		{
			pAd->CommonCfg.RegTransmitSetting.field.ShortGI = GI_800;
		}
		
		DBGPRINT(RT_DEBUG_TRACE, ("HT: Guard Interval = %s\n", (Value==GI_400) ? "400" : "800" ));
	}

	// HT Operation Mode : Mixed Mode , Green Field
	if (RTMPGetKeyParameter("HT_OpMode", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);

		if (Value == HTMODE_GF)
		{

			pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_GF;
		}
		else
		{
			pAd->CommonCfg.RegTransmitSetting.field.HTMODE  = HTMODE_MM;
		}		

		DBGPRINT(RT_DEBUG_TRACE, ("HT: Operate Mode = %s\n", (Value==HTMODE_GF) ? "Green Field" : "Mixed Mode" ));
	}

	// Fixed Tx mode : CCK, OFDM
	if (RTMPGetKeyParameter("FixedTxMode", pValueStr, 25, pInput))
	{
		UCHAR	fix_tx_mode;
	
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			for (i = 0, Bufptr = rstrtok(pValueStr,";"); (Bufptr && i < MAX_MBSSID_NUM); Bufptr = rstrtok(NULL,";"), i++) 	
			{
				fix_tx_mode = FIXED_TXMODE_HT;

				if (strcmp(Bufptr, "OFDM") == 0 || strcmp(Bufptr, "ofdm") == 0)
				{
					fix_tx_mode = FIXED_TXMODE_OFDM;
				}	
				else if (strcmp(Bufptr, "CCK") == 0 || strcmp(Bufptr, "cck") == 0)
				{
			        fix_tx_mode = FIXED_TXMODE_CCK;
				}
				else if (strcmp(Bufptr, "HT") == 0 || strcmp(Bufptr, "ht") == 0)
				{
			        fix_tx_mode = FIXED_TXMODE_HT;
				}
				else
				{
					Value = simple_strtol(Bufptr, 0, 10);
					// 1 : CCK
					// 2 : OFDM
					// otherwise : HT
					if (Value == FIXED_TXMODE_CCK || Value == FIXED_TXMODE_OFDM)
						fix_tx_mode = Value;	
					else
						fix_tx_mode = FIXED_TXMODE_HT;
				}

				pAd->ApCfg.MBSSID[i].DesiredTransmitSetting.field.FixedTxMode = fix_tx_mode;																	
				DBGPRINT(RT_DEBUG_TRACE, ("(IF-ra%d) Fixed Tx Mode = %d\n", i, fix_tx_mode));
							
			}
		}
#endif // CONFIG_AP_SUPPORT //
	}


	// Channel Width
	if (RTMPGetKeyParameter("HT_BW", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);

		if (Value == BW_40)
		{
			pAd->CommonCfg.RegTransmitSetting.field.BW  = BW_40;
		}
		else
		{
            pAd->CommonCfg.RegTransmitSetting.field.BW  = BW_20;
		}		

		DBGPRINT(RT_DEBUG_TRACE, ("HT: Channel Width = %s\n", (Value==BW_40) ? "40 MHz" : "20 MHz" ));
	}

	if (RTMPGetKeyParameter("HT_EXTCHA", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);

		if (Value == 0)
		{
			
			pAd->CommonCfg.RegTransmitSetting.field.EXTCHA  = EXTCHA_BELOW;
		}
		else
		{
            pAd->CommonCfg.RegTransmitSetting.field.EXTCHA = EXTCHA_ABOVE;
		}		

		DBGPRINT(RT_DEBUG_TRACE, ("HT: Ext Channel = %s\n", (Value==0) ? "BELOW" : "ABOVE" ));
	}

	// MSC
	if (RTMPGetKeyParameter("HT_MCS", pValueStr, 50, pInput))
	{
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			for (i = 0, Bufptr = rstrtok(pValueStr,";"); (Bufptr && i < MAX_MBSSID_NUM); Bufptr = rstrtok(NULL,";"), i++) 	
			{
				Value = simple_strtol(Bufptr, 0, 10);			
//				if ((Value >= 0 && Value <= 15) || (Value == 32))
				if ((Value >= 0 && Value <= 23) || (Value == 32)) // 3*3
				{
					pAd->ApCfg.MBSSID[i].DesiredTransmitSetting.field.MCS  = Value;
					DBGPRINT(RT_DEBUG_TRACE, ("(IF-ra%d) HT: MCS = %d\n", i, pAd->ApCfg.MBSSID[i].DesiredTransmitSetting.field.MCS));							
				}
				else
				{
					pAd->ApCfg.MBSSID[i].DesiredTransmitSetting.field.MCS  = MCS_AUTO;
					DBGPRINT(RT_DEBUG_TRACE, ("(IF-ra%d) HT: MCS is AUTO\n", i));							
				}		
			}		
		}
#endif // CONFIG_AP_SUPPORT //

	}

	// STBC 
    if (RTMPGetKeyParameter("HT_STBC", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);
		if (Value == STBC_USE)
		{		
			pAd->CommonCfg.RegTransmitSetting.field.STBC = STBC_USE;
		}
		else
		{
			pAd->CommonCfg.RegTransmitSetting.field.STBC = STBC_NONE;
		}
		DBGPRINT(RT_DEBUG_TRACE, ("HT: STBC = %d\n", pAd->CommonCfg.RegTransmitSetting.field.STBC));
	}

	// 40_Mhz_Intolerant
	if (RTMPGetKeyParameter("HT_40MHZ_INTOLERANT", pValueStr, 25, pInput))
	{
		Value = simple_strtol(pValueStr, 0, 10);
		if (Value == 0)
		{		
			pAd->CommonCfg.bForty_Mhz_Intolerant = FALSE;
		}
		else
		{
			pAd->CommonCfg.bForty_Mhz_Intolerant = TRUE;
		}
		DBGPRINT(RT_DEBUG_TRACE, ("HT: 40MHZ INTOLERANT = %d\n", pAd->CommonCfg.bForty_Mhz_Intolerant));
	}
	//HT_TxStream
	if(RTMPGetKeyParameter("HT_TxStream", pValueStr, 10, pInput))
	{
		switch (simple_strtol(pValueStr, 0, 10))
		{
			case 1:
				pAd->CommonCfg.TxStream = 1;
				break;
			case 2:
				pAd->CommonCfg.TxStream = 2;
				break;
			case 3: // 3*3
			default:
				pAd->CommonCfg.TxStream = 3;

				if (pAd->MACVersion < RALINK_2883_VERSION)
					pAd->CommonCfg.TxStream = 2; // only 2 tx streams for RT2860 series
				break;
		}
		DBGPRINT(RT_DEBUG_TRACE, ("HT: Tx Stream = %d\n", pAd->CommonCfg.TxStream));
	}
	//HT_RxStream
	if(RTMPGetKeyParameter("HT_RxStream", pValueStr, 10, pInput))
	{
		switch (simple_strtol(pValueStr, 0, 10))
		{
			case 1:
				pAd->CommonCfg.RxStream = 1;
				break;
			case 2:
				pAd->CommonCfg.RxStream = 2;
				break;
			case 3:
			default:
				pAd->CommonCfg.RxStream = 3;

				if (pAd->MACVersion < RALINK_2883_VERSION)
					pAd->CommonCfg.RxStream = 2; // only 2 rx streams for RT2860 series
				break;
		}
		DBGPRINT(RT_DEBUG_TRACE, ("HT: Rx Stream = %d\n", pAd->CommonCfg.RxStream));
	}

}


