#ifndef EVPN_OPTION_H
#define EVPN_OPTION_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>

struct evpn_options {

    char tapdev[IFNAMSIZ];
    enum evpn_mode mode;
    uint32_t server_ipv4;
    uint16_t server_port;
    bool daemonized;
    bool show_usage;
    int debug_level;
};

struct evpn_options*
evpn_options_init(int argc, char* argv[]);

void evpn_options_destroy(struct evpn_options* evopts);


#endif


