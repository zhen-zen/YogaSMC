/*
    bmfdec.c - Decompress binary MOF file (BMF)
    Copyright (C) 2017  Pali Rohár <pali.rohar@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

//#include <stdint.h>
//#include <stdio.h>
//#include <string.h>
//#include <errno.h>
//#include <unistd.h>

#include <IOKit/IOLib.h>

#define INLINE static inline

typedef uint8_t __u8;
typedef uint32_t __u32;
typedef uint16_t __u16;

#ifdef DEBUG
#define LOG_DECOMP(...) do { IOLog("YSMC - Debug: YogaBMF: " __VA_ARGS__); } while (0)
//#define LOG_DECOMP(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG_DECOMP(...)
#endif

/*
dblspace_dec.c

DMSDOS CVF-FAT module: [dbl|drv]space cluster read and decompression routines.

******************************************************************************
DMSDOS (compressed MSDOS filesystem support) for Linux
written 1995-1998 by Frank Gockel and Pavel Pisa

    (C) Copyright 1995-1998 by Frank Gockel
    (C) Copyright 1996-1998 by Pavel Pisa

Some code of dmsdos has been copied from the msdos filesystem
so there are the following additional copyrights:

    (C) Copyright 1992,1993 by Werner Almesberger (msdos filesystem)
    (C) Copyright 1994,1995 by Jacques Gelinas (mmap code)
    (C) Copyright 1992-1995 by Linus Torvalds

DMSDOS was inspired by the THS filesystem (a simple doublespace
DS-0-2 compressed read-only filesystem) written 1994 by Thomas Scheuermann.

The DMSDOS code is distributed under the Gnu General Public Licence.
See file COPYING for details.
******************************************************************************

*/

#define M_MOVSB(D,S,C) for(;(C);(C)--) *((__u8*)(D)++)=*((__u8*)(S)++)

#if !defined(le16_to_cpu)
    /* for old kernel versions - works only on i386 */
    #define le16_to_cpu(v) (v)
#endif

/* for reading and writting from/to bitstream */
typedef
 struct {
   __u32 buf;	/* bit buffer */
     int pb;	/* already readed bits from buf */
   __u16 *pd;	/* first not readed input data */
   __u16 *pe;	/* after end of data */
 } bits_t;

const unsigned dblb_bmsk[]=
   {0x0,0x1,0x3,0x7,0xF,0x1F,0x3F,0x7F,0xFF,
    0x1FF,0x3FF,0x7FF,0xFFF,0x1FFF,0x3FFF,0x7FFF,0xFFFF};

/* read next 16 bits from input */
#define RDN_G16(bits) \
   { \
    (bits).buf>>=16; \
    (bits).pb-=16; \
    if((bits).pd<(bits).pe) \
    { \
     (bits).buf|=((__u32)(le16_to_cpu(*((bits).pd++))))<<16; \
    }; \
   }

/* prepares at least 16 bits for reading */
#define RDN_PR(bits,u) \
   { \
    if((bits).pb>=16) RDN_G16(bits); \
    u=(bits).buf>>(bits).pb; \
   }

/* initializes reading from bitstream */
INLINE void dblb_rdi(bits_t *pbits,void *pin,unsigned lin)
{
  pbits->pb=32;
  pbits->pd=(__u16*)pin;
  pbits->pe=pbits->pd+((lin+1)>>1);
  pbits->buf=0;
}

/* reads n<=16 bits from bitstream *pbits */
INLINE unsigned dblb_rdn(bits_t *pbits,int n)
{
  unsigned u;
  RDN_PR(*pbits,u);
  pbits->pb+=n;
  u&=dblb_bmsk[n];
  return u;
}

INLINE int dblb_rdlen(bits_t *pbits)
{ unsigned u;
  RDN_PR(*pbits,u);
  switch (u&15)
  { case  1: case  3: case  5: case  7:
    case  9: case 11: case 13: case 15:
      pbits->pb++;     return 3;
    case  2: case  6:
    case 10: case 14:
      pbits->pb+=2+1;  return (1&(u>>2))+4;
    case  4: case 12:
      pbits->pb+=3+2;  return (3&(u>>3))+6;
    case  8:
      pbits->pb+=4+3;  return (7&(u>>4))+10;
    case  0: ;
  }
  switch ((u>>4)&15)
  { case  1: case  3: case  5: case  7:
    case  9: case 11: case 13: case 15:
      pbits->pb+=5+4;  return (15&(u>>5))+18;
    case  2: case  6:
    case 10: case 14:
      pbits->pb+=6+5;  return (31&(u>>6))+34;
    case  4: case 12:
      pbits->pb+=7+6;  return (63&(u>>7))+66;
    case  8:
      pbits->pb+=8+7;  return (127&(u>>8))+130;
    case  0: ;
  }
  pbits->pb+=9;
  if(u&256) return dblb_rdn(pbits,8)+258;
  return -1;
}

INLINE int dblb_decrep(bits_t *pbits, __u8 **p, void *pout, __u8 *pend,
		 int repoffs, int k, int flg)
{ int replen;
  __u8 *r;

  if(repoffs==0){LOG_DECOMP("DMSDOS: decrb: zero offset ?\n");return -2;}
  if(repoffs==0x113f)
  { 
    int pos=(int)(*p-(__u8*)pout);
    LOG_DECOMP("DMSDOS: decrb: 0x113f sync found.\n");
    if((pos%512) && !(flg&0x4000))
    { LOG_DECOMP("DMSDOS: decrb: sync at decompressed pos %d ?\n",pos);
      return -2;
    }
    return 0;
  }
  replen=dblb_rdlen(pbits)+k;

  if(replen<=0)
    {LOG_DECOMP("DMSDOS: decrb: illegal count ?\n");return -2;}
  if((__u8*)pout+repoffs>*p)
    {LOG_DECOMP("DMSDOS: decrb: of>pos ?\n");return -2;}
  if(*p+replen>pend)
    {LOG_DECOMP("DMSDOS: decrb: output overfill ?\n");return -2;}
  r=*p-repoffs;
  M_MOVSB(*p,r,replen);
  return 0;
}

/* DS decompression */
/* flg=0x4000 is used, when called from stacker_dec.c, because of
   stacker does not store original cluster size and it can mean,
   that last cluster in file can be ended by garbage */
int ds_dec(void* pin,int lin, void* pout, int lout, int flg)
{ 
  __u8 *p, *pend;
  unsigned u, repoffs;
  int r;
  bits_t bits;

  dblb_rdi(&bits,pin,lin);
  p=(__u8*)pout;pend=p+lout;
  if((dblb_rdn(&bits,16))!=0x5344) return -1;
    
  u=dblb_rdn(&bits,16);
  u=((u&0xff)<<8)|((u>>8)&0xff);
  LOG_DECOMP("DMSDOS: DS decompression version %d\n",u);
  
  do
  { r=0;
    RDN_PR(bits,u);
    switch(u&3)
    {
      case 0:
	bits.pb+=2+6;
	repoffs=(u>>2)&63;
	r=dblb_decrep(&bits,&p,pout,pend,repoffs,-1,flg);
	break;
      case 1:
	bits.pb+=2+7;
	*(p++)=(u>>2)|128;
	break;
      case 2:
	bits.pb+=2+7;
	*(p++)=(u>>2)&127;
	break;
      case 3:
	if(u&4) {  bits.pb+=3+12; repoffs=((u>>3)&4095)+320; }
	else  {  bits.pb+=3+8;  repoffs=((u>>3)&255)+64; };
	r=dblb_decrep(&bits,&p,pout,pend,repoffs,-1,flg);
	break;
    }
  }while((r==0)&&(p<pend)&&(bits.pd<bits.pe||(bits.pd==bits.pe&&bits.pb<16)));
  
  if(r<0) return r;

  if(!(flg&0x4000))
  { 
    u=dblb_rdn(&bits,3);if(u==7) u=dblb_rdn(&bits,12)+320;
    if(u!=0x113f)
    { LOG_DECOMP("DMSDOS: decrb: final sync not found?\n");
      return -2;
    }
  }

  return (int)(p-(__u8*)pout);
}

/*
 * BMF file is compressed by DS-01 algorithm with additional header:
 * 4 bytes: 46 4f 4d 42 - 'F' 'O' 'M' 'B'
 * 4 bytes: 01 00 00 00 - version 0x01
 * 4 bytes: size of compressed data (low endian) without this header
 * 4 bytes: size of decompressed data (low endian) without this header
 */

/*
 * Decompressed part of BMF file contains:
 * 4 bytes: 46 4f 4d 42 - 'F' 'O' 'M' 'B'
 * 4 bytes: N = size of first part (low endian) since beginning
 * N-8 bytes: first part data
 * 16 bytes: 42 4d 4f 46 51 55 41 4c 46 4c 41 56 4f 52 31 31 - "BMOFQUALFLAVOR11"
 * 4 bytes: M = count of 4 bytes (low endian) after
 * M*8 bytes: second part data
 */

//static int process_data(char *data, uint32_t size) {
//  ssize_t ret = write(1, data, size);
//  return (ret > 0 && ret == size) ? 0 : 1;
//}
//
//#undef process_data
//static int process_data(char *data, uint32_t size);
//
//int main() {
//  static uint32_t pin[0x10000/4];
//  static char pout[0x40000];
//  ssize_t lin;
//  int lout;
//  lin = read(0, pin, sizeof(pin));
//  if (lin < 0) {
//    fprintf(stderr, "Failed to read data: %s\n", strerror(errno));
//    return 1;
//  } else if (lin == sizeof(pin)) {
//    fprintf(stderr, "Failed to read data: %s\n", strerror(EFBIG));
//    return 1;
//  }
//  if (lin <= 16 || pin[0] != 0x424D4F46 || pin[1] != 0x01 || pin[2] != (uint32_t)lin-16 || pin[3] > sizeof(pout)) {
//    fprintf(stderr, "Invalid input\n");
//    return 1;
//  }
//  lout = pin[3];
//  if (ds_dec((char *)pin+16, lin-16, pout, lout, 0) != lout) {
//    fprintf(stderr, "Decompress failed\n");
//    return 1;
//  }
//  return process_data(pout, lout);
//}
