#ifndef DVR_H
#define DVR_H

#include "../node.h"

#include <unordered_map>
#include <vector>

struct PacketHeader {
    PacketHeader(IPAddress source_ip, IPAddress dest_ip, MACAddress source_mac, bool is_arp) : is_arp(is_arp), source_ip(source_ip), dest_ip(dest_ip), source_mac(source_mac) { }
    PacketHeader() { }

    bool is_arp;
    IPAddress source_ip;
    IPAddress dest_ip;
    MACAddress source_mac;
};

class DVRNode : public Node {
private:
    std::unordered_map<IPAddress, size_t> distance_vector;
    std::unordered_map<IPAddress, MACAddress> gateway;
    std::unordered_map<MACAddress, size_t> neighbor_distances;

public:
    DVRNode(Simulation* simul, MACAddress mac, IPAddress ip) : Node(simul, mac, ip) { }

    void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const override;
    void receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t distance) override;
    void do_periodic(size_t us) override;
};

#endif
