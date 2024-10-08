#ifndef NAIVE_H
#define NAIVE_H

#include "../node.h"

#include <cstring>
#include <iostream>
#include <vector>

// DON'T DO THIS, this is just illustrative of how to extend the Node class
class NaiveNode : public Node {
public:
    NaiveNode(Simulation& simul, MACAddress mac, IPAddress ip) : Node(simul, mac, ip) { }

    void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const override
    {
        std::cout << '[' << mac << "] Sending segment\n";
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
            std::cout << '[' << mac << "] Packet delivered to wrong node!\n";
            return;
        }

        std::vector<uint8_t> segment(packet.begin() + 4, packet.end());
        std::cout << '[' << mac << "] Received packet:\n\t";
        for (auto const& c : packet) {
            std::cout << c;
        }
        std::cout << '\n';
    }
};

#endif // NAIVE_H
