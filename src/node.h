#ifndef NODE_H
#define NODE_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct Simulation;

using MACAddress = uint32_t;
using IPAddress = uint32_t;

class Node {
private:
    Simulation const& simul;

public:
    virtual ~Node() = default;

    MACAddress const mac;
    IPAddress const ip;
    std::map<MACAddress, size_t> const neighbour_distances;
    Node(Simulation const& simul, MACAddress mac, IPAddress ip, std::map<MACAddress, size_t> nd)
        : simul(simul), mac(mac), ip(ip), neighbour_distances(nd) { }

    // XXX implement these
    virtual void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const = 0;
    virtual void receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet) = 0;
    // implement this if you need to
    virtual void do_periodic() { };

protected:
    // XXX use this in your implementation of send_segment
    void send_packet(MACAddress dest_mac, std::vector<uint8_t> const& packet) const;
    // XXX use this for debugging (writes logs to a file named "node-`mac`.log" )
    void log(std::string) const;
};

#endif // NODE_H
