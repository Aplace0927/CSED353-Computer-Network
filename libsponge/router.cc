#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    uint32_t route_masked = route_prefix & ~(static_cast<uint32_t>(0xFFFFFFFF) >> prefix_length);
    if (routing_table.find(route_masked) == routing_table.end()) {  // Not in Routing table
        routing_table[route_masked] = {~(static_cast<uint32_t>(0xFFFFFFFF) >> prefix_length), next_hop, interface_num};
    }
    if (routing_table.find(route_masked)->second.prefix_mask <=
        ~(static_cast<uint32_t>(0xFFFFFFFF) >> prefix_length)) {  // Subset in Routing table
        routing_table[route_masked] = {~(static_cast<uint32_t>(0xFFFFFFFF) >> prefix_length), next_hop, interface_num};
    }
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    if (dgram.header().ttl <= 1) {
        return;
    }
    uint32_t mask = 0xFFFFFFFFU;
    while (true) {
        if (routing_table.find(dgram.header().dst & mask) != routing_table.end() and
            routing_table.find(dgram.header().dst & mask)->second.prefix_mask == mask) {
            RoutingTableElement rte = routing_table.find(dgram.header().dst & mask)->second;
            if (rte.next_hop.has_value()) {
                dgram.header().ttl--;
                _interfaces[rte.interface_num].send_datagram(dgram, rte.next_hop.value());
            } else {
                dgram.header().ttl--;
                _interfaces[rte.interface_num].send_datagram(dgram, Address::from_ipv4_numeric(dgram.header().dst));
            }
            break;
        }
        if (mask == 0U) {
            break;
        }
        mask <<= 1;  // 0xFFFFFFFF -> 0xFFFFFFFE -> ... -> 0x80000000 -> 0x00000000
    }
    return;
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
