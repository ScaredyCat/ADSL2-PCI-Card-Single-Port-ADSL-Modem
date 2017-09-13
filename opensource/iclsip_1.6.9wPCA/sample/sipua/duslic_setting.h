/*
** Copyright (C) 2005 infineon <Qi-Ming.Wu@infineon.com>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
//-----------------------------------------------------------------------
//Description:	
// register setting for internel loop back
//-----------------------------------------------------------------------
//Author:	Qi-Ming.Wu@infineon.com
//Created:	24-Augest-2005
//-----------------------------------------------------------------------*/


unsigned char A_setting[]={ 
          0x15,// REVISION   0x0
          0x08,// CHIPID1    0x01
          0x15,// CHIPID2    0x02
          0x10,// CHIPID3    0x03
          0x00,// FUSE       0x04
          0x00,// PCMC1      0x05 
          0x00,// XCR        0x06
          0x04,// INTREG1    0x07
          0x4f,// INTREG2    0x08
          0x00,// INTREG3    0x09
          0x00,// INTREG4    0x0A
          0xcd,// CHKR1      0x0B
          0xae,// CHKR2      0x0C
          0x00,// LMRES1     0x0D
          0x00,// LMRES2     0x0E
          0x00,// FUSE2      0x0F
          0x00,// FUSE3      0x10
          0xbf,// MASK       0x11 
          0x0f,// IOCTL1     0x12
          0x00,// IOCTL2     0x13
          0x90,// IOCTL3     0x14
          0x00,// BCR1       0x15
          0x00,// BCR2       0x16
          0x10,// BCR3       0x17
          0x00,// BCR4       0x18
          0x00,// BCR5       0x19
	  0x00,// DSCR       0x1A 
          0x00,// CTR        0x1B 
	  0x22,// LMCR1      0x1C
          0x00,// LMCR2      0x1D
          0x00,// LMCR3      0x1E
          0x00,// OFR1       0x1F
          0x00,// OFR2       0x20
          0x01,// PCMR1      0x21
	  0x00,// PCMR2      0x22  
          0x00,// PCMR3      0x23
	  0x00,// PCMR4      0x24
	  0x01,// PCMX1      0x25
          0x00,// PCMX2      0x26
	  0x00,// PCMX3      0x27
	  0x00,// PCMX4      0x28
	  0x00,// TSTR1      0x29  
          0x00,// TSTR2      0x2A
          0x00,// TSTR3      0x2B
          0x00,// TSTR4      0x2C
          0x00 // TSTR5      0x2D
};
   


unsigned char B_setting[]={ 
          0x15,// REVISION   0x0
          0x08,// CHIPID1    0x01
          0x15,// CHIPID2    0x02
          0x10,// CHIPID3    0x03
          0x00,// FUSE       0x04
          0x00,// PCMC1      0x05 
          0x00,// XCR        0x06
          0x04,// INTREG1    0x07
          0x4f,// INTREG2    0x08
          0x00,// INTREG3    0x09
          0x00,// INTREG4    0x0A
          0x8c,// CHKR1      0x0B
          0x88,// CHKR2      0x0C
          0x00,// LMRES1     0x0D
          0x00,// LMRES2     0x0E
          0x00,// FUSE2      0x0F
          0x00,// FUSE3      0x10
          0xbf,// MASK       0x11 
          0x0f,// IOCTL1     0x12
          0x00,// IOCTL2     0x13
          0x90,// IOCTL3     0x14
          0x00,// BCR1       0x15
          0x00,// BCR2       0x16
          0x00,// BCR3       0x17
          0x00,// BCR4       0x18
          0x00,// BCR5       0x19
	  0x00,// DSCR       0x1A 
          0x00,// CTR        0x1B 
	  0x22,// LMCR1      0x1C
          0x00,// LMCR2      0x1D
          0x00,// LMCR3      0x1E
          0x00,// OFR1       0x1F
          0x00,// OFR2       0x20
          0x00,// PCMR1      0x21
	  0x00,// PCMR2      0x22  
          0x00,// PCMR3      0x23
	  0x00,// PCMR4      0x24
	  0x00,// PCMX1      0x25
          0x00,// PCMX2      0x26
	  0x00,// PCMX3      0x27
	  0x00,// PCMX4      0x28
	  0x00,// TSTR1      0x29  
          0x00,// TSTR2      0x2A
          0x00,// TSTR3      0x2B
          0x00,// TSTR4      0x2C
          0x00 // TSTR5      0x2D
};   
      
	 
	       
   
unsigned char A_setting_size=sizeof(A_setting);   
unsigned char B_setting_size=sizeof(B_setting);   
   
   
   
   
   
   
   
   
   
   
   
   
   
   


