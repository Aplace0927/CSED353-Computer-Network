#ifndef SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
#define SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH

#include "ethernet_frame.hh"
#include "tcp_over_ip.hh"
#include "tun.hh"

#include <functional>
#include <map>
#include <optional>
#include <queue>
#include <vector>
//! \brief A "network interface" that connects IP (the internet layer, or network layer)
//! with Ethernet (the network access layer, or link layer).

//! This module is the lowest layer of a TCP/IP stack
//! (connecting IP with the lower-layer network protocol,
//! e.g. Ethernet). But the same module is also used repeatedly
//! as part of a router: a router generally has many network
//! interfaces, and the router's job is to route Internet datagrams
//! between the different interfaces.

//! The network interface translates datagrams (coming from the
//! "customer," e.g. a TCP/IP stack or router) into Ethernet
//! frames. To fill in the Ethernet destination address, it looks up
//! the Ethernet address of the next IP hop of each datagram, making
//! requests with the [Address Resolution Protocol](\ref rfc::rfc826).
//! In the opposite direction, the network interface accepts Ethernet
//! frames, checks if they are intended for it, and if so, processes
//! the the payload depending on its type. If it's an IPv4 datagram,
//! the network interface passes it up the stack. If it's an ARP
//! request or reply, the network interface processes the frame
//! and learns or replies as necessary.
class NetworkInterface {
  private:
    //! Ethernet (known as hardware, network-access-layer, or link-layer) address of the interface
    EthernetAddress _ethernet_address;

    //! IP (known as internet-layer or network-layer) address of the interface
    Address _ip_address;

    //! outbound queue of Ethernet frames that the NetworkInterface wants sent
    std::queue<EthernetFrame> _frames_out{};

    const time_t ARP_QUERY_TIME_TO_LIVE = 30'000;
    const time_t ARP_RESPN_TIME_TO_LIVE = 5'000;

    class ARPMapping {
      public:
        EthernetAddress link_layer_addr;
        time_t addr_ttl;

        ARPMapping() = default;
        ARPMapping(EthernetAddress addr, size_t ttl) : link_layer_addr(addr), addr_ttl(ttl){};
        bool expired() { return addr_ttl < 0; }
    };

    class IPSentInfo {
      public:
        Address network_layer_addr;
        InternetDatagram data_sent;

        IPSentInfo() = default;
        IPSentInfo(const Address addr, const InternetDatagram &dgram) : network_layer_addr(addr), data_sent(dgram){};
    };

    struct ARPMessage arp_craft(uint16_t opcode,
                                EthernetAddress send_eth,
                                uint32_t send_ip,
                                EthernetAddress recv_eth,
                                uint32_t recv_ip);
    EthernetFrame eth_craft(uint16_t type, EthernetAddress src, EthernetAddress dst, BufferList payload);

    // ARP table
    std::map<uint32_t, ARPMapping> arp_table = {};
    // ARP Packets queried, waiting IP address response until timeout
    std::map<uint32_t, time_t> queried_arp = {};
    // IP Packets queried, waiting ARP packet recieved.
    std::vector<IPSentInfo> queried_ip = {};

  public:
    //! \brief Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer) addresses
    NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address);

    //! \brief Access queue of Ethernet frames awaiting transmission
    std::queue<EthernetFrame> &frames_out() { return _frames_out; }

    //! \brief Sends an IPv4 datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination address).

    //! Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address for the next hop
    //! ("Sending" is accomplished by pushing the frame onto the frames_out queue.)
    void send_datagram(const InternetDatagram &dgram, const Address &next_hop);

    //! \brief Receives an Ethernet frame and responds appropriately.

    //! If type is IPv4, returns the datagram.
    //! If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
    //! If type is ARP reply, learn a mapping from the "sender" fields.
    std::optional<InternetDatagram> recv_frame(const EthernetFrame &frame);

    //! \brief Called periodically when time elapses
    void tick(const size_t ms_since_last_tick);
};

#endif  // SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
