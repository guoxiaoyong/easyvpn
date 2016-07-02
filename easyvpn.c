#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <dl_list.h>
#include <easyvpn.h>
#include <aux.h>
#include <debugmsg.h>
#include <evpn_frame.h>



struct evpn* evpn_init(  uint32_t ipv4,
                         uint16_t port,
                         char* tapdev,
                         enum evpn_mode mode,
                         encryptor data_enc,
                         decryptor data_dec ) {


    struct evpn* ev;
    ev = (struct evpn*)calloc(sizeof(struct evpn), 1);

    if (ev == NULL) {

        return NULL;
    }

    ev->signal = 0;
    ev->last_beacon = 0;

    strcpy(ev->tapdev, tapdev);
    ev->mode = mode;
    dl_list_init( &(ev->nodes) );


    if ( getmac(ev->tapdev, ev->tapmac) < 0 ) {

        free(ev);
        return NULL;
    }

    ev->server_ipv4 = ipv4;
    ev->server_port = port; // network order
    ev->my_ipv4     = 0;
    ev->my_port     = 0;

    ev->data_enc = data_enc;
    ev->data_dec = data_dec;

    ev->net_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if ( ev->net_sock < 0 ) {

        perror("create socket error!\n");
        free(ev);
        return NULL;
    }

    assert(mode == EVPN_SERVER || mode == EVPN_CLIENT);

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = 0;
    local_addr.sin_port = port; // network order

    if ( bind(ev->net_sock,
              (struct sockaddr*)&local_addr,
              sizeof(struct sockaddr_in) ) < 0 )
    {

        perror("bind failed!\n");
        close(ev->net_sock);
        free(ev);
        return NULL;
    }

    ev->tap_sock = tun_alloc(ev->tapdev, IFF_TAP|IFF_NO_PI);
    if (ev->tap_sock < 0) {

        perror("connect to tap interface error!");
        close(ev->net_sock);
        free(ev);
        return NULL;

    } else {

        fprintf( stderr,
                 "successfully connected to the interface %s\n",
                 ev->tapdev);
    }


    ev->maxfd = (ev->tap_sock > ev->net_sock) ? ev->tap_sock:ev->net_sock;
    return ev;
}



void evpn_destroy(struct evpn* ev) {

    struct node_data* item;
    struct node_data* temp;

    close(ev->net_sock);
    close(ev->tap_sock);

    dl_list_for_each_safe(item, temp, &(ev->nodes), struct node_data, list) {

        dl_list_del(&(item->list));
        free(item);
    }

    free(ev);
}


struct node_data*
evpn_find_node(struct evpn* ev, uint8_t* mac) {

    struct node_data* nd;

    dl_list_for_each(nd, &(ev->nodes), struct node_data, list) {

        if ( memcmp(nd->mac, mac, 6) == 0 ) {

            return nd;
        }
    }

    return NULL;
}


struct node_data*
evpn_find_node_from_mgt_frame(struct evpn* ev) {

    uint8_t* mac = mgt_message_get_mac(ev->data);
    return evpn_find_node(ev, mac);
}


struct node_data*
evpn_find_node_from_dst_mac(struct evpn* ev) {

    uint8_t* mac = data_frame_get_dst_mac(ev->data);
    return evpn_find_node(ev, mac);
}


struct node_data*
evpn_find_node_from_src_mac(struct evpn* ev) {

    uint8_t* mac = data_frame_get_src_mac(ev->data);
    return evpn_find_node(ev, mac);
}


int evpn_add_node( struct evpn* ev ) {

    time_t currtime = time(NULL);
    struct node_data* nd;

    nd = calloc(sizeof(struct node_data), 1);

    if ( nd == NULL ) {

        return -1;
    }

    nd->lastseen = currtime;
    nd->ipv4     = ev->data_ipv4;
    nd->port     = ev->data_port;

    switch ( frame_get_type(ev->data) ) {

    case FRAME_MGT_PROBE_REQUEST:
    case FRAME_MGT_PROBE_RESPONSE:
        maccpy(nd->mac, mgt_message_get_mac(ev->data));
        break;

    case FRAME_DATA:
        maccpy(nd->mac, data_frame_get_src_mac(ev->data));
        break;

    default:
        return -1;
    }

    dl_list_add( &(ev->nodes), &(nd->list) );

    return 0;
}


int evpn_delete_timeout_node(struct evpn* ev) {

    time_t currtime = time(NULL);
    struct node_data* nd;
    struct node_data* tmp;

    dl_list_for_each_safe(nd, tmp, &(ev->nodes), struct node_data, list) {

        if ( currtime - nd->lastseen > 20 ) {

            dl_list_del(&nd->list);
            free(nd);
        }
    }


    return 0;
}


char* evpn_print_nodelist(struct evpn* ev) {

    struct node_data* nd;
    int n = -1;

    char* result = (char*)(ev->data);
    char tmp[256];

    result[0] = 0;

    if ( dl_list_empty( &(ev->nodes) ) ) {

        ev->datalen = sprintf(result, "empty node list!\n");
        ev->datalen++;
        return result;
    }

    dl_list_for_each(nd, &(ev->nodes), struct node_data, list) {

        n++;
        sprintf(tmp, "%02d: MAC=%s, IP=%s, port=%d, lastseen=%s\n", n,
                mac2str(nd->mac),
                ip2str(nd->ipv4),
                ntohs(nd->port),
                asctime(localtime(&(nd->lastseen))) );

        strcat(result, tmp);
    }

    ev->datalen = strlen(result);
    ev->datalen++;

    return result;
}



int evpn_sendto( struct evpn* ev, uint32_t ipv4, uint16_t port) {

    struct sockaddr_in remote;

    remote.sin_family       = AF_INET;
    remote.sin_addr.s_addr  = ipv4;
    remote.sin_port         = port;

    int e = sendto( ev->net_sock,
                    ev->data,
                    ev->datalen, 0,
                    (struct sockaddr*)&remote,
                    sizeof(remote) );

    if (e < 0) {

        perror("sendto failed!");
    }


    return e;
}


int evpn_sendto_node( struct evpn* ev ) {

    return evpn_sendto(ev, ev->data_ipv4, ev->data_port);
}


int evpn_sendto_server(struct evpn* ev) {

    return evpn_sendto(ev, ev->server_ipv4, ev->server_port);
}


int evpn_sendto_nodelist( struct evpn* ev ) {

    struct node_data* nd;

    dl_list_for_each(nd, &(ev->nodes), struct node_data, list) {

        ev->data_ipv4 = nd->ipv4;
        ev->data_port = nd->port;
        evpn_sendto_node(ev);
    }

    return 0;
}


int evpn_recvfrom_node(struct evpn* ev) {


    struct sockaddr_in remote;
    socklen_t socklen = sizeof(struct sockaddr_in);

    ev->datalen = recvfrom( ev->net_sock, ev->data,
                            MAX_DATA_FRAME_SIZE, 0,
                            (struct sockaddr*)&remote,
                            &socklen );

    if (ev->datalen == -1) {

        perror("read net device error!");
        return -1;
    }

    ev->data_ipv4 = (uint32_t)remote.sin_addr.s_addr;
    ev->data_port = (uint16_t)remote.sin_port;

    return 0;
}



int evpn_select(struct evpn* ev) {

    fd_set rd_set;
    FD_ZERO(&rd_set);
    FD_SET(ev->tap_sock, &rd_set);
    FD_SET(ev->net_sock, &rd_set);

    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 10000;

    ev->tap_sock_ready = 0;
    ev->net_sock_ready = 0;

    int ret = select(ev->maxfd + 1, &rd_set, NULL, NULL, &timeout);

    if (ret < 0 && errno == EINTR) {

        return -1;
    }

    if (ret < 0) {

        perror("select error");
        return -2;
    }

    if ( FD_ISSET(ev->tap_sock, &rd_set) ) {

        ev->tap_sock_ready = 1;
    }

    if ( FD_ISSET(ev->net_sock, &rd_set) ) {

        ev->net_sock_ready = 1;
    }

    return 0;
}



// port in network order
void evpn_process_net_frame( struct evpn* ev ) {

    struct node_data* nd = NULL;
    time_t currtime = time(NULL);
    uint8_t* dst_mac;
    uint8_t  type = frame_get_type(ev->data);


    switch ( type ) {

    case FRAME_MGT_PROBE_REQUEST:

        nd = evpn_find_node_from_mgt_frame(ev);

        if (nd == NULL) {

            evpn_add_node(ev);

        } else {

            nd->lastseen = currtime;
            nd->ipv4     = ev->data_ipv4;
            nd->port     = ev->data_port;
        }

        mgt_message_set( ev->data,
                         FRAME_MGT_PROBE_RESPONSE,
                         ev->tapmac,
                         ev->data_ipv4,
                         ev->data_port,
                         ev->seqno++ ); // seqno is not used!

        ev->datalen = sizeof(struct mgt_message);

        evpn_sendto_node(ev);

        break;


    //=================================================================
    case FRAME_MGT_PROBE_RESPONSE:

        if (ev->my_ipv4 != 0) {

            assert(ev->my_ipv4 == mgt_message_get_ipv4(ev->data));
            assert(ev->my_port == mgt_message_get_port(ev->data));

        } else {

            ev->my_ipv4 = mgt_message_get_ipv4(ev->data);
            ev->my_port = mgt_message_get_port(ev->data);
        }


        nd = evpn_find_node_from_mgt_frame(ev);

        if (nd == NULL) {

            evpn_add_node(ev);

        } else {

            nd->lastseen = currtime;
        }

        break;


    case FRAME_MGT_NAT_PUNCH:

        debug_output(0, "not implemented yet!\n");
        break;

    case FRAME_MGT_GET_STATUS:

        evpn_print_nodelist(ev);
        evpn_sendto_node(ev);
        break;

    case FRAME_DATA:

        dst_mac = data_frame_get_dst_mac(ev->data);

        if ( is_mac_equal(dst_mac, ev->tapmac) ) {

            ev->data_dec(  data_frame_get_payload(ev->data),
                           data_frame_get_payload_len(ev->data));

            int e = write(  ev->tap_sock,
                            data_frame_get_payload(ev->data),
                            data_frame_get_payload_len(ev->data));

            assert(e == data_frame_get_payload_len(ev->data));

        } else if ( is_mac_broadcast(dst_mac) ) { // all broadcast are send to server

            if (ev->mode == EVPN_SERVER) {

                evpn_sendto_nodelist(ev);
            }

            ev->data_dec(  data_frame_get_payload(ev->data),
                           data_frame_get_payload_len(ev->data));

            int e = write(  ev->tap_sock,
                            data_frame_get_payload(ev->data),
                            data_frame_get_payload_len(ev->data));

            assert(e == data_frame_get_payload_len(ev->data));


        } else {

            assert(ev->mode == EVPN_SERVER);
            nd = evpn_find_node_from_dst_mac(ev);

            if ( nd != NULL ) {

                ev->data_ipv4 = nd->ipv4;
                ev->data_port = nd->port;
                evpn_sendto_node(ev);
            }
        }

        break;

    default:

        debug_output(0, "unknown type byte = %d\n", frame_get_type(ev->data));
        break;
    }
}



int evpn_run( struct evpn* ev ) {

    while (1) {

        if ( ev->signal == SIGINT ) break;

        if ( evpn_select(ev) < 0 ) continue;

        if ( ev->net_sock_ready ) {

            if ( evpn_recvfrom_node(ev) == 0 ) {

                evpn_process_net_frame(ev);
            }
        }


        if ( ev->tap_sock_ready )  {

            ev->datalen = read( ev->tap_sock,
                                data_frame_get_payload(ev->data),
                                MAX_DATA_FRAME_SIZE );

            assert(ev->datalen < MAX_DATA_FRAME_SIZE-100);

            if (ev->datalen == -1) {

                perror("read tap device error!");
                goto TAP_SOCK_EXIT;

            }

            data_frame_set_payload_len(ev->data, (uint16_t)ev->datalen);
            data_frame_set(ev->data);

            ev->data_enc(data_frame_get_payload(ev->data), ev->datalen);

            ev->datalen += sizeof(struct data_frame);

            if ( is_mac_broadcast(data_frame_get_dst_mac(ev->data)) ) {

                if ( ev->mode == EVPN_CLIENT ) {

                    evpn_sendto_server(ev);

                } else {

                    evpn_sendto_nodelist(ev);
                }

            } else {

                struct node_data* nd = evpn_find_node_from_dst_mac(ev);

                if (nd == NULL) {

                    if (ev->mode == EVPN_CLIENT) {

                        evpn_sendto_server(ev);
                    }

                } else {

                    evpn_sendto(ev, nd->ipv4, nd->port);
                    //debug_output(15, "data frame send to %s %s %d\n", mac2str(nd->mac), ip2str(nd->ipv4), (int)htons(nd->port));
                }
            }

        }


TAP_SOCK_EXIT:

        if ( ev->mode == EVPN_CLIENT && time(NULL) - ev->last_beacon > 5 ) {

            ev->last_beacon = time(NULL);

            mgt_message_set( ev->data,
                             FRAME_MGT_PROBE_REQUEST,
                             ev->tapmac,
                             ev->my_ipv4,
                             ev->my_port,
                             ev->seqno++ );

            ev->datalen = sizeof(struct mgt_message);
            evpn_sendto_server(ev);
        }

        evpn_delete_timeout_node(ev);




    }


    return 0;
}




