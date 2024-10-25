#ifndef RP_H
#define RP_H

#include "../node.h"

#include <unordered_map>
#include <vector>

#define INITIAL_VALIDITY 100

class RPNode : public Node {
private:
    struct RoutingInfo {
        MACAddress gateway;
        size_t distance;
        size_t validity;
    };
    std::unordered_map<IPAddress, RoutingInfo> dv_table;

    void send_packet_to(IPAddress dest_ip, std::vector<uint8_t> const& packet) const;

public:
    RPNode(Simulation* simul, MACAddress mac, IPAddress ip)
        : Node(simul, mac, ip)
    {
        /*
         * add self entry to advertise to others
         */
        dv_table[ip] = RoutingInfo { mac, 0, INITIAL_VALIDITY };
    }

    void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const override;
    void receive_packet(MACAddress src_mac, std::vector<uint8_t> packet, size_t distance) override;
    void do_periodic() override;
};

#endif
