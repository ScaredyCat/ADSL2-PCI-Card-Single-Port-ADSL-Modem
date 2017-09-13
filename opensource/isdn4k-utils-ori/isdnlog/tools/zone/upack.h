#ifndef _UPACK_H_
#define _UPACK_H_

void tools_pack32(unsigned char *buf, UL num);
UL tools_unpack32(unsigned char *buf);
void tools_pack16(unsigned char *buf, US num);
US tools_unpack16(unsigned char *buf);

#endif
