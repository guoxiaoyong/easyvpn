#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debugmsg.h>


void printmac(uint8_t* mac) {

    printf( "%02X:%02X:%02X:%02X:%02X:%02X\n",
            mac[0], mac[1], mac[2],
            mac[3], mac[4], mac[5]);
}


char* mac2str(uint8_t* mac) {

    static char str[64];

    sprintf( str, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);

    return str;
}

char* ip2str(uint32_t ip) {

    static char str[64];

    uint8_t* ptr = (uint8_t*)&ip;

    sprintf( str, "%d.%d.%d.%d", ptr[0], ptr[1], ptr[2], ptr[3]);

    return str;
}


char* sockaddr_in_to_string(struct sockaddr_in* addr) {

    static char str[64];

    sprintf(str, "IP: %s\n", inet_ntoa(addr->sin_addr));
    sprintf(str, "port: %u\n", (unsigned int)ntohs(addr->sin_port));

    return str;
}


void print_sockaddr_in(struct sockaddr_in* addr) {

    printf("IP: %s\n", inet_ntoa(addr->sin_addr));
    printf("port: %u\n", (unsigned int)ntohs(addr->sin_port));
}



