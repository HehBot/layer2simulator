#ifndef NODE_H
#define NODE_H

#include <cstdint>
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
    Node(Simulation const& simul, MACAddress mac, IPAddress ip)
        : simul(simul), mac(mac), ip(ip) { }

    // XXX implement these
    virtual void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const = 0;
    virtual void receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t distance) = 0;

    // implement this if you need to do something periodically (eg heartbeats, ARP etc)
    // you get a timestamp in the argument that represents real time in ms
    virtual void do_periodic(double ms) { };

protected:
    // XXX use this in your implementation of send_segment
    void send_packet(MACAddress dest_mac, std::vector<uint8_t> const& packet) const;

    // XXX use this in your implementation of receive_packet when you receive a segment;
    // this will be used to verify that segments are being routed correctly
    void receive_segment(IPAddress src_ip, std::vector<uint8_t> const& segment) const;

    // XXX use this to broadcast to all neighbours
    void broadcast_packet(std::vector<uint8_t> const& packet) const;

    // XXX use this for debugging (writes logs to a file named "node-`mac`.log" )
    void log(std::string) const;
};

#endif // NODE_H
