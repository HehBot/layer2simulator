#ifndef NAIVE_H
#define NAIVE_H

#include "../node.h"

#include <cstring>
#include <vector>

// DON'T DO THIS, this is just illustrative of how to extend the Node class
class NaiveNode : public Node {
public:
    NaiveNode(Simulation& simul, MACAddress mac, IPAddress ip, std::map<MACAddress, size_t> nd) : Node(simul, mac, ip, nd) { }

    void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const override
    {
        log("Sending segment:\t" + std::string(segment.begin(), segment.end()));
        MACAddress dest_mac = dest_ip / 1000;

        std::vector<uint8_t> packet(sizeof(dest_ip) + segment.size());

        memcpy(&packet[0], &dest_ip, sizeof(dest_ip));
        memcpy(&packet[sizeof(IPAddress)], &segment[0], segment.size());

        send_packet(dest_mac, packet);
    }
    void receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet) override
    {
        IPAddress dest_ip;
        memcpy(&dest_ip, &packet[0], sizeof(dest_ip));

        if (dest_ip != ip) {
            log("Packet delivered to wrong node, intended for ip " + std::to_string(dest_ip));
            return;
        }

        std::vector<uint8_t> segment(packet.begin() + sizeof(dest_ip), packet.end());
        log("Received segment:\t" + std::string(segment.begin(), segment.end()));
    }
};

#endif // NAIVE_H
