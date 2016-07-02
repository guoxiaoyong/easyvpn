# EasyVPN

**Xiaoyong Guo**

I wrote EasyVPN about two years ago to bypass GFW. EasyVPN is inspired by [simpletun](http://backreference.org/2010/03/26/tuntap-interface-tutorial/). EasyVPN runs on Linux only. 

EasyVPN chooses UDP to tunnel ethernet frames instead of using TCP as simpletun. The reason for this choise is that most application level network protocols are based on TCP, and [TCP over TCP is a bad ideal](http://sites.inka.de/~W1011/devel/tcp-tcp.html). The tun/tap device is set to tap mode so that we can do things like NIC bridging, or run non-IP protocols over internet.

EasyVPN can run as a server or as a client. All clients connected to the same server can see each other as if they are in the same LAN. 

## Build
Simply type `make` to compile.

## Usage
`easyvpn -h` print out help text.


