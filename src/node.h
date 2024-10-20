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
    Simulation* simul;

public:
    virtual ~Node() = default;

    MACAddress const mac;
    IPAddress const ip;
    Node(Simulation* simul, MACAddress mac, IPAddress ip)
        : simul(simul), mac(mac), ip(ip) { }

    /*
     * XXX implement these
     */
    virtual void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const = 0;
    virtual void receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t distance) = 0;

    /*
     * XXX implement this if you need to do something periodically
     */
    virtual void do_periodic() { };

protected:
    /*
     * use this in your implementation of send_segment
     * XXX WARNING use it as send_packet(dest_mac, packet), do not provide any argument for metadata!
     * for reference see node_impl/naive.cc
     */
    void send_packet(MACAddress dest_mac, std::vector<uint8_t> const& packet, char const* metadata) const;

    /*
     * use this in your implementation of receive_packet when you receive a segment
     * this will be used to verify that segments are being routed correctly
     * for reference see node_impl/naive.cc
     */
    void receive_segment(IPAddress src_ip, std::vector<uint8_t> const& segment) const;

    /*
     * use this to broadcast to all neighbours
     * XXX WARNING use it as broadcast_packet(packet), do not provide any argument for metadata!
     * for reference see node_impl/naive.cc
     */
    void broadcast_packet(std::vector<uint8_t> const& packet, char const* metadata) const;

    /*
     * use this for debugging (writes logs to a file named "node-`mac`.log")
     */
    void log(std::string) const;
};

#define send_packet(X, Y) send_packet(X, Y, __func__)
#define broadcast_packet(X) broadcast_packet(X, __func__)

#endif // NODE_H
