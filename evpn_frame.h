#ifndef EVPN_FRAME_H
#define EVPN_FRAME_H

#include <stdint.h>
#include <string.h>
#include <easyvpn.h>


struct mgt_message {

      uint8_t   type;
      uint32_t  seqno;
      uint8_t   mac[6];
      uint32_t  ipv4;
      uint16_t  port;

} __attribute__((packed));



struct data_frame {

     uint8_t   type;
     uint16_t  payload_len;
     uint8_t   dst_mac[6];
     uint8_t   src_mac[6];
     uint8_t   payload[0];

} __attribute__((packed));



inline int 
is_mac_equal(uint8_t* mac1, uint8_t* mac2) {

    return (memcmp(mac1, mac2, 6) == 0);
}

inline int
is_mac_broadcast(uint8_t* mac) {

    static const uint8_t brdmac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
    return (memcmp(mac, brdmac, 6) == 0);
}

inline int
is_mac_all_zero(uint8_t* mac) {

    static const uint8_t brdmac[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
    return (memcmp(mac, brdmac, 6) == 0);
}

inline uint8_t*
maccpy(uint8_t* mac_dst, uint8_t* mac_src) {

     return memcpy(mac_dst, mac_src, 6);
}


inline uint8_t* 
frame_set_type(uint8_t* frame, enum evpn_frame_type type) {

     frame[0] = (uint8_t)type;
     return frame;
}


inline enum evpn_frame_type
frame_get_type(uint8_t* frame) {

     return frame[0];
}


inline uint8_t*
data_frame_get_payload(uint8_t* frame) {

    struct data_frame* tmp = (struct data_frame*)frame;
    return tmp->payload;
}


inline uint16_t
data_frame_get_payload_len(uint8_t* frame) {

    struct data_frame* tmp = (struct data_frame*)frame;
    return htons(tmp->payload_len);
}



inline uint8_t*
data_frame_set_payload_len(uint8_t* frame, uint16_t len) {

    struct data_frame* tmp = (struct data_frame*)frame;
    tmp->payload_len = htons(len);

    return frame;
}



inline uint8_t*
data_frame_get_dst_mac(uint8_t* frame) {

    struct data_frame* tmp = (struct data_frame*)frame;
    return tmp->dst_mac;
}


inline uint8_t*
data_frame_get_src_mac(uint8_t* frame) {

    struct data_frame* tmp = (struct data_frame*)frame;
    return tmp->src_mac;
}

inline uint8_t*
data_frame_set(uint8_t* frame) {

    struct data_frame* tmp = (struct data_frame*)frame;
    frame_set_type(frame, FRAME_DATA);
    maccpy(tmp->dst_mac, data_frame_get_payload(frame)+0);
    maccpy(tmp->src_mac, data_frame_get_payload(frame)+6);
    
    return frame;
}


inline uint8_t*
mgt_message_set_mac(uint8_t* frame, uint8_t* mac){

    struct mgt_message* tmp = (struct mgt_message*)frame;
    maccpy(tmp->mac, mac);
    
    return frame;
}


inline uint8_t*
mgt_message_get_mac(uint8_t* frame){

    struct mgt_message* tmp = (struct mgt_message*)frame;
    return tmp->mac;
}


inline uint8_t*
mgt_message_set_seqno(uint8_t* frame, uint32_t seqno) {

    struct mgt_message* tmp = (struct mgt_message*)frame;
    tmp->seqno = seqno;

    return frame;
}



inline uint32_t
mgt_message_get_seqno(uint8_t* frame) {

    struct mgt_message* tmp = (struct mgt_message*)frame;
    return tmp->seqno;
}

inline uint32_t
mgt_message_get_ipv4(uint8_t* frame) {

    struct mgt_message* tmp = (struct mgt_message*)frame;
    return tmp->ipv4;

}

inline uint8_t*
mgt_message_set_ipv4(uint8_t* frame, uint32_t ipv4) {

    struct mgt_message* tmp = (struct mgt_message*)frame;
    tmp->ipv4= ipv4;

    return frame;
}


inline uint16_t
mgt_message_get_port(uint8_t* frame) {

    struct mgt_message* tmp = (struct mgt_message*)frame;
    return tmp->port;

}

inline uint8_t*
mgt_message_set_port(uint8_t* frame, uint16_t port) {

    struct mgt_message* tmp = (struct mgt_message*)frame;
    tmp->port = port;

    return frame;
}




inline uint8_t*
mgt_message_set(uint8_t* frame, 
                uint8_t  type,
                uint8_t* mac, 
                uint32_t ipv4, 
                uint16_t port, 
                uint32_t seqno) {

    struct mgt_message* tmp = (struct mgt_message*)frame;

    memcpy(tmp->mac, mac, 6);
    tmp->type  = type;
    tmp->ipv4  = ipv4;
    tmp->port  = port;
    tmp->seqno = seqno;
    
    return frame;
}


#endif

