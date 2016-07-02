#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>


#include <easyvpn.h>
#include <evpn_option.h>
#include <debugmsg.h>
#include <aux.h>


void usage(char* progname) {

    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s -i <ifacename> [-s|-c <server ip>] [-p <port>] [-d]\n", progname);
    fprintf(stderr, "%s -h\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "-i <ifacename>: name of the tap interface to use (mandatory)\n");
    fprintf(stderr, "-s|-c <server ip>: run in server mode (-s), or specify server address (-c <server ip>) (mandatory)\n");
    fprintf(stderr, "-p <port>: port to listen on (if run in server mode) or to connect to (in client mode)\n");
    fprintf(stderr, "-d run in daemon mode\n");
    fprintf(stderr, "-h: prints this help text\n\n");

    fprintf(stderr, "author: Xiaoyong Guo (guo.xiaoyong@gmail.com)\n");

    return;
}

struct evpn* ev;

void signal_handler(int sig) {

    if ( sig == SIGINT ) {

        fprintf(stderr, "SIGINT signal caught!\n");
        ev->signal = SIGINT;
    }
}



int main (int argc, char* argv[]) {

    signal(SIGINT, signal_handler);

    struct evpn_options* evopts = evpn_options_init(argc, argv);

    if ( evopts == NULL ) {

        fprintf(stderr, "faild to allocate memory for evopts!\n");
        return 0;
    }

    if ( evopts->show_usage ) {

        usage(argv[0]);
        evpn_options_destroy(evopts);
        return 0;
    }

    debug_level_set(evopts->debug_level);

    if (evopts->mode == EVPN_CLIENT) {

        debug_output(9, "server address = %s:%d\n",
                     ip2str(evopts->server_ipv4),
                     ntohs(evopts->server_port));
    }


    if ( evopts->daemonized == 1 ) {

        pid_t pid = fork();
        if (pid != 0) {

            printf("pid = %d\n", pid);
            return 0;
        }

        signal(SIGHUP, SIG_IGN);
        setpgrp();
    }

    ev = evpn_init( evopts->server_ipv4,
                    evopts->server_port,
                    evopts->tapdev,
                    evopts->mode,
                    encdec,
                    encdec );

    if (ev != NULL) {

        evpn_run(ev);
        evpn_destroy(ev);
    }


    evpn_options_destroy(evopts);
    return 0;
}


