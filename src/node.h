#ifndef NODE_H
#define NODE_H

#include <cstdint>
#include <vector>

struct Simulation;

using MACAddress = uint32_t;
using IPAddress = uint32_t;

class Node {
private:
    Simulation& simul;

public:
    virtual ~Node() = default;

    MACAddress const mac;
    IPAddress const ip;
    Node(Simulation& simul, MACAddress mac, IPAddress ip) : simul(simul), mac(mac), ip(ip) { }

    // XXX implement these
    virtual void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const = 0;
    virtual void receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet) = 0;

protected:
    // XXX use this in your implementation of send_segment
    void send_packet(MACAddress dest_mac, std::vector<uint8_t> const& packet) const;
};

#endif // NODE_H
