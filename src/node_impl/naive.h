#ifndef NAIVE_H
#define NAIVE_H

#include "../node.h"

#include <cstring>
#include <vector>

struct NaivePacketHeader {
private:
    NaivePacketHeader() = default;

public:
    bool is_broadcast;
    IPAddress src_ip;
    IPAddress dest_ip;

    NaivePacketHeader(IPAddress src_ip)
        : is_broadcast(true), src_ip(src_ip), dest_ip(0) { }
    NaivePacketHeader(IPAddress src_ip, IPAddress dest_ip)
        : is_broadcast(false), src_ip(src_ip), dest_ip(dest_ip) { }
    static NaivePacketHeader from_bytes(uint8_t const* bytes)
    {
        NaivePacketHeader ph;
        memcpy(&ph, bytes, sizeof(ph));
        return ph;
    }
};

// DON'T DO THIS, this is just illustrative of how to extend the Node class
// This assumes (wrongly) that
//  - the link layer is a clique and
//  - that IP addresses are direct-mapped to MAC addresses as
//          IP = MAC * 1000
class NaiveNode : public Node {
public:
    NaiveNode(Simulation* simul, MACAddress mac, IPAddress ip) : Node(simul, mac, ip) { }

    void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const override;
    void receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t distance) override;
    void do_periodic() override;
};

#endif // NAIVE_H
