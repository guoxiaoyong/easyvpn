#include <iostream>
#include <map>
#include <cstdint>

using namespace std;

struct MacAddr {

    uint8_t mac[6];     // ethernet hardware address
};

struct NodeInfo {

    MacAddr   mac;
    uint32_t  ipv4;       // IPv4 address
    uint16_t  port;       // stored in network order
    time_t lastseen;
};

enum evpn_mode {

    EVPN_SERVER = 0,
    EVPN_CLIENT,
};


class EasyVPN {

private:

    enum evpn_mode mode;
    std::map<MacAddr, NodeInfo> peers; 
    int signal;


/*====================*/

public:

    EasyVPN(uint32_t ipv4, uint16_t port, enum evpn_mode m) {

        signal = 0;
        mode = m;
    }

    ~EasyVPN() {

        cout << "EasyVPN stopped!" << endl;
    }

    void run() {
    }
};





