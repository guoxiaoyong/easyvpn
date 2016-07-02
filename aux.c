#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <arpa/inet.h>

#include <aux.h>

/*###########################################
 *
 * very simple symmetric encryption
 * and decryption
 *
 *##########################################*/

uint8_t* encdec(uint8_t* buf, int len) {

    int n;
    uint8_t key = 0xD5; //  1101 0101

    for (n = 0; n < len; n++) {

        buf[n] ^= key;
    }

    return buf;
}


uint8_t* id_encdec(uint8_t* buf, int len) {

    len = len;

    return buf;
}


/*#############################################
 *
 * allocates or reconnects to a tun/tap device.
 * The caller needs to
 * reserve enough space in *dev.
 *
 *#############################################*/

int tun_alloc(char *dev, int flags) {

    struct ifreq ifr;
    int fd, err;

    if ( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {

        perror("opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;

    if (*dev) {

        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    strcpy(dev, ifr.ifr_name);

    return fd;
}


int getmac(char* dev, uint8_t* mac) {

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if ( sock < 0) {

        perror("create socket fail\n");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name)-1);

    if ( (ioctl(sock, SIOCGIFHWADDR, &ifr)) < 0 ) {

        printf("mac ioctl error\n");
        return -1;
    }

    close(sock);
    memcpy((void*)mac, (void*)ifr.ifr_hwaddr.sa_data, 6);

    return 0;
}


void hexdump(uint8_t* payload, int len, int offset) {

    int i, j;
    uint8_t *ch;
    int numIters = len / 16;
    int remain   = len % 16;

    ch  = &payload[offset];


    for (j = 0; j < numIters; ++j) {
        // print offset
        printf("%08d: ", offset);

        // print hex
        for (i = 0; i < 16; ++i) {

            printf("%02x ", ch[i]);
        }

        printf(" ");

        // print ascii
        for (i = 0; i < 16; ++i) {

            if ( isprint(ch[i]) ) {

                printf("%c", ch[i]);
            } else {

                printf(".");
            }
        }

        printf("\n");
        offset += 16;
        ch += 16;
    }

    // print offset
    printf("%08d: ", offset);

    // print hex
    for (i = 0; i < remain; ++i) {

        printf("%02x ", ch[i]);
    }

    for (i = remain; i < 16; ++i) {

        printf("   ");
    }

    printf(" ");

    // print ascii
    for (i = 0; i < remain; ++i) {

        if ( isprint(ch[i]) ) {

            printf("%c", ch[i]);
        } else {

            printf(".");
        }
    }

    printf("\n");
}




static int debug_level;

int debug_output(int level, const char* format, ...) {

    va_list args;

    va_start(args, format);

    if ( level <= debug_level ) {

        return vprintf(format, args);
    }

    va_end(args);

    return -1;
}

int debug_level_set(int lvl) {

    return debug_level = lvl;
}


