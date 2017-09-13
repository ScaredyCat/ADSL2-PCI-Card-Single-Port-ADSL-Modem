#include "base64.h"

/**
* @Encode with base64
*
* @param buf   [out] save the result of encoding
* @param text  [in] the sting to be encoded
* @param size  [in] the length of text
*
* @return the length of encoded string.
**/
int Base64Enc(unsigned char *buf,const unsigned char*text,int size) 
{ 
   static char *base64_encoding = 
       "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   int buflen = 0; 

   while(size>0){
       *buf++ = base64_encoding[ (text[0] >> 2 ) & 0x3f];
       if(size>2){
           *buf++ = base64_encoding[((text[0] & 3) << 4) | (text[1] >> 4)];
           *buf++ = base64_encoding[((text[1] & 0xF) << 2) | (text[2] >> 6)];
           *buf++ = base64_encoding[text[2] & 0x3F];
       }else{
           switch(size){
               case 1:
                   *buf++ = base64_encoding[(text[0] & 3) << 4 ];
                   *buf++ = '=';
                   *buf++ = '=';
                   break;
               case 2: 
                   *buf++ = base64_encoding[((text[0] & 3) << 4) | (text[1] >> 4)]; 
                   *buf++ = base64_encoding[((text[1] & 0x0F) << 2) | (text[2] >> 6)]; 
                   *buf++ = '='; 
                   break; 
           } 
       } 

       text +=3; 
       size -=3; 
       buflen +=4; 
   } 

   *buf = 0; 
   return buflen; 
} 


