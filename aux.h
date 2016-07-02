#ifndef AUX_H
#define AUX_H

#include <stdint.h>

uint8_t* encdec(uint8_t* buf, int len);
uint8_t* id_encdec(uint8_t* buf, int len);
int tun_alloc(char *dev, int flags);
int getmac(char* dev, uint8_t* mac);
void hexdump(uint8_t* payload, int len, int offset);

int debug_output(int level, const char* format, ...);
int debug_level_set(int lvl);

#endif

