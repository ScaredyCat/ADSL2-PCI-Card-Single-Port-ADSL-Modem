#include "config.h"
#include "common.h"

void tools_pack32(unsigned char *buf, UL num)
{
  *buf++ = num; num >>= 8;
  *buf++ = num; num >>= 8;
  *buf++ = num; num >>= 8;
  *buf = num;
}

UL tools_unpack32(unsigned char *buf)
{
  UL num;
  num = buf[3]; num <<= 8;
  num += buf[2]; num <<= 8;
  num += buf[1]; num <<= 8;
  num += buf[0];
  return num;
}

void tools_pack16(unsigned char *buf, US num)
{
  *buf++ = num; num >>= 8;
  *buf = num;
}

US tools_unpack16(unsigned char *buf)
{
  US num;
  num = buf[1]; num <<= 8;
  num += buf[0];
  return num;
}
