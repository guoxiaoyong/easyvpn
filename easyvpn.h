#ifndef EASYVPN_H
#define EASYVPN_H

#include <stdint.h>
#include <sys/socket.h>
#include <linux/if.h>

#include <dl_list.h>


enum evpn_frame_type {

    FRAME_MGT_PROBE_REQUEST,
    FRAME_MGT_PROBE_RESPONSE,
    FRAME_MGT_NAT_PUNCH,
    FRAME_DATA,

    FRAME_MGT_GET_STATUS,
};


enum evpn_mode {

    EVPN_SERVER = 0,
    EVPN_CLIENT,
    EVPN_QUERY,
};


struct node_data {

    uint8_t   mac[6];                // ethernet hardware address
    uint32_t  ipv4;                  // IPv4 address
    uint16_t  port;                  // stored in network order

    uint32_t last_seqno; // not used!
    time_t lastseen;

    struct dl_list list;

};


typedef uint8_t* (*encryptor)(uint8_t*, int);
typedef uint8_t* (*decryptor)(uint8_t*, int);


#define MAX_MGT_FRAME_SIZE 32
#define MAX_DATA_FRAME_SIZE 2048

struct evpn {

    enum evpn_mode mode;

    int tap_sock;
    int net_sock;
    int maxfd;
    int tap_sock_ready;
    int net_sock_ready;

    char tapdev[IFNAMSIZ];
    uint8_t tapmac[6];

    uint8_t  data[MAX_DATA_FRAME_SIZE];
    ssize_t  datalen;
    uint32_t data_ipv4;
    uint16_t data_port;

    uint32_t server_ipv4;
    uint16_t server_port;

    uint32_t my_ipv4;
    uint16_t my_port;

    encryptor data_enc;
    decryptor data_dec;

    time_t last_beacon;

    int signal;

    uint32_t seqno;

    struct dl_list nodes;

};



struct evpn* evpn_init( uint32_t ipv4,
                        uint16_t port,
                        char* tapdev,
                        enum evpn_mode mode,
                        encryptor data_enc,
                        decryptor data_dec );

void evpn_destroy(struct evpn* ev);

int evpn_run(struct evpn* ev);




#endif


