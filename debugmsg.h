#ifndef DEBUGMSG_H
#define DEBUGMSG_H

#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>

void   printmac(uint8_t* mac);
char*  mac2str(uint8_t* mac);
char*  ip2str(uint32_t ip);
char*  sockaddr_in_to_string(struct sockaddr_in* addr);
void   print_sockaddr_in(struct sockaddr_in* addr);


#endif


