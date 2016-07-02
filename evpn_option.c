#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <easyvpn.h>
#include <evpn_option.h>


struct evpn_options*
evpn_options_init(int argc, char* argv[]) {

    int option;
    struct evpn_options* evopts;

    evopts = (struct evpn_options*)calloc(sizeof(struct evpn_options), 1);

    if (evopts == NULL) {

        fprintf(stderr, "failed to allocate memory!\n");
        return NULL;
    }

    if (argc == 1) evopts->show_usage = true;

    /* check command line options */
    while ( (option = getopt(argc, argv, "hi:sqc:p:dl:")) > 0 ) {

        switch (option) {

        case 'h':
            evopts->show_usage = true;
            break;

        case 'i':
            strncpy(evopts->tapdev, optarg, IFNAMSIZ-1);
            break;

        case 's':
            evopts->mode = EVPN_SERVER;
            evopts->server_ipv4 = 0;
            break;

        case 'q':
            evopts->mode = EVPN_QUERY;
            break;

        case 'c':
            evopts->mode = EVPN_CLIENT;
            evopts->server_ipv4 = (uint32_t)inet_addr(optarg);
            break;

        case 'p':
            evopts->server_port = htons((uint16_t)atoi(optarg));
            break;

        case 'd':
            evopts->daemonized = 1;
            break;

        case 'l':
            evopts->debug_level = atoi(optarg);
            break;

        default:
            fprintf(stderr, "unknown option %c\n", option);
            evopts->show_usage = true;
            return NULL;
        }
    }

    argv += optind;
    argc -= optind;

    if (argc > 0) {

        fprintf(stderr, "too many options!, argc =%d\n", argc);
        evopts->show_usage = true;
    }

    return evopts;
}


void evpn_options_destroy(struct evpn_options* evopts) {

    free(evopts);
}

