#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    if (arp_table.find(next_hop_ip) != arp_table.end()) {
        EthernetFrame ethfrm;
        ethfrm.header().src = _ethernet_address;
        ethfrm.header().dst = arp_table.find(next_hop_ip)->second.link_layer_addr;
        ethfrm.header().type = EthernetHeader::TYPE_IPv4;
        ethfrm.payload() = dgram.serialize();
        _frames_out.push(ethfrm);
    } else if (queried_arp.find(next_hop_ip) != queried_arp.end()) {
        queried_ip.push_back(IPSentInfo(next_hop, dgram));
    } else {
        ARPMessage arpmsg;
        arpmsg.opcode = ARPMessage::OPCODE_REQUEST;
        arpmsg.sender_ethernet_address = _ethernet_address;
        arpmsg.sender_ip_address = _ip_address.ipv4_numeric();
        arpmsg.target_ip_address = next_hop_ip;

        EthernetFrame ethfrm;
        ethfrm.header().dst = ETHERNET_BROADCAST;
        ethfrm.header().src = _ethernet_address;
        ethfrm.header().type = EthernetHeader::TYPE_ARP;
        ethfrm.payload() = arpmsg.serialize();

        _frames_out.push(ethfrm);
        queried_arp[next_hop_ip] = ARP_QUERY_TIME_TO_LIVE;
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if ((frame.header().dst != _ethernet_address) or (frame.header().dst != ETHERNET_BROADCAST)) {
        return {};
    }

    switch (frame.header().type) {
        case EthernetHeader::TYPE_IPv4: {
            InternetDatagram dgram;
            if (dgram.parse(frame.payload()) != ParseResult::NoError) {
                return {};
            }
            return dgram;
        }
        case EthernetHeader::TYPE_ARP: {
            ARPMessage arpmsg;
            if (arpmsg.parse(frame.payload()) != ParseResult::NoError) {
                return {};
            }
            bool arp_req = (arpmsg.opcode == ARPMessage::OPCODE_REQUEST) and
                           (arpmsg.target_ip_address == _ip_address.ipv4_numeric());
            bool arp_rsp =
                (arpmsg.opcode == ARPMessage::OPCODE_REPLY) and (arpmsg.target_ethernet_address == _ethernet_address);

            if (arp_req) {
                ARPMessage arprpl;
                arprpl.opcode = ARPMessage::OPCODE_REPLY;
                arprpl.sender_ethernet_address = _ethernet_address;
                arprpl.sender_ip_address = _ip_address.ipv4_numeric();
                arprpl.target_ethernet_address = arpmsg.sender_ethernet_address;
                arprpl.target_ip_address = arpmsg.target_ip_address;

                EthernetFrame ethfrm;
                ethfrm.header().src = _ethernet_address;
                ethfrm.header().dst = arpmsg.sender_ethernet_address;
                ethfrm.header().type = EthernetHeader::TYPE_ARP;
                ethfrm.payload() = arprpl.serialize();

                _frames_out.push(ethfrm);
            }
            if (arp_req or arp_rsp) {
                arp_table[arpmsg.sender_ip_address] =
                    ARPMapping(arpmsg.sender_ethernet_address, ARP_QUERY_TIME_TO_LIVE);

                for (NetworkInterface::IPSentInfo sentinfo : queried_ip) {
                    if (sentinfo.network_layer_addr.ipv4_numeric() == arpmsg.sender_ip_address) {
                        send_datagram(sentinfo.data_sent, sentinfo.network_layer_addr);
                    }
                }
                queried_ip.erase(std::remove_if(queried_ip.begin(), queried_ip.end(), [arpmsg](const IPSentInfo &info) {
                    return info.network_layer_addr.ipv4_numeric() == arpmsg.sender_ip_address;
                }));
                queried_arp.erase(arpmsg.sender_ip_address);
            }
            break;
        }
        default: {
            return {};
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }
