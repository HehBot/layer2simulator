#ifndef NAIVE_H
#define NAIVE_H

#include "../node.h"

#include <cstring>
#include <vector>

struct NaivePacketHeader {
    IPAddress dest_ip; // 0 for broadcast
    IPAddress src_ip;

    NaivePacketHeader() = default;
    NaivePacketHeader(IPAddress src_ip, IPAddress dest_ip)
        : dest_ip(dest_ip), src_ip(src_ip) { }
};

// DON'T DO THIS, this is just illustrative of how to extend the Node class
// This assumes (wrongly) that
//  - the link layer is a clique and
//  - that IP addresses are direct-mapped to MAC addresses as
//          IP = MAC * 1000
class NaiveNode : public Node {
public:
    NaiveNode(Simulation& simul, MACAddress mac, IPAddress ip) : Node(simul, mac, ip) { }

    void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const override
    {
        MACAddress dest_mac = dest_ip / 1000;

        auto ph = NaivePacketHeader(ip, dest_ip);

        std::vector<uint8_t> packet(sizeof(ph) + segment.size());

        memcpy(&packet[0], &ph, sizeof(ph));
        memcpy(&packet[sizeof(ph)], &segment[0], segment.size());

        send_packet(dest_mac, packet);
    }
    void receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t distance) override
    {
        NaivePacketHeader ph;
        ;
        memcpy(&ph, &packet[0], sizeof(ph));

        if (ph.dest_ip == 0) {
            log("Received broadcast from " + std::to_string(ph.src_ip));
            return;
        } else if (ph.dest_ip != ip) {
            log("Packet delivered to wrong node, intended for ip " + std::to_string(ph.dest_ip));
            return;
        }

        std::vector<uint8_t> segment(packet.begin() + sizeof(ph), packet.end());
        receive_segment(ph.src_ip, segment);
    }
    void do_periodic() override
    {
        log("Sending broadcast");
        std::string s = "BROADCAST FROM " + std::to_string(ip);
        NaivePacketHeader ph(ip, 0);
        std::vector<uint8_t> packet(sizeof(ph) + s.length());
        memcpy(&packet[0], &ph, sizeof(ph));
        memcpy(&packet[sizeof(ph)], &s[0], s.length());
        broadcast_packet(packet);
    }
};

#endif // NAIVE_H
