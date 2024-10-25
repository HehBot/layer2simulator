#ifndef RP_H
#define RP_H

#include "../node.h"

#include <unordered_map>
#include <vector>

class RPNode : public Node {
private:
    struct RoutingInfo {
        MACAddress gateway;
        size_t distance;
        size_t validity;
    };
    std::unordered_map<IPAddress, RoutingInfo> dv_table;

    std::unordered_map<MACAddress, IPAddress> neighbor_ips;

    void send_packet_to(IPAddress dest_ip, std::vector<uint8_t> const& packet) const;

public:
    RPNode(Simulation* simul, MACAddress mac, IPAddress ip) : Node(simul, mac, ip) { }

    void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const override;
    void receive_packet(MACAddress src_mac, std::vector<uint8_t> packet, size_t distance) override;
    void do_periodic() override;
};

#endif
