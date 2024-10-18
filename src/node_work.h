#ifndef NODE_WORK_H
#define NODE_WORK_H

#include "node.h"

#include <cstdint>
#include <mutex>
#include <ostream>
#include <queue>

class NodeWork {
public:
    // we need to ensure that `send_segment` and `receive_packet` calls on `node`
    // are synchronised so that the extended implementation does not have to perform
    // locking on data structures
    std::mutex node_mt;
    Node* node;

    struct SegmentToSendInfo {
        IPAddress dest_ip;
        std::vector<uint8_t> segment;
        SegmentToSendInfo(IPAddress ip, std::vector<uint8_t> const& segment) : dest_ip(ip), segment(segment) { }
    };

private:
    struct PacketReceivedInfo {
        MACAddress src_mac;
        size_t dist;
        std::vector<uint8_t> packet;
        PacketReceivedInfo(MACAddress src_mac, size_t dist, std::vector<uint8_t> const& packet)
            : src_mac(src_mac), dist(dist), packet(packet) { }
    };

    std::mutex inbound_mt;
    std::queue<PacketReceivedInfo> inbound;

    std::vector<SegmentToSendInfo> outbound;
    std::mutex outbound_mt;

    bool on;

    std::mutex log_mt;
    std::ostream* logger;

public:
    NodeWork(Node* node, std::ostream* logger)
        : node(node), logger(logger)
    {
    }
    ~NodeWork();

    bool do_send();
    bool do_receive();
    void do_periodic();

    void send(std::vector<SegmentToSendInfo> const& outbound);

    void receive_frame(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t dist);
    void log(std::string logline);
};

#endif // NODE_WORK_H
