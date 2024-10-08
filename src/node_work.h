#ifndef NODE_WORK_H
#define NODE_WORK_H

#include "nlohmann/json_fwd.hpp"
#include "node.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>

class NodeWork {
public:
    // we need to ensure that `send_segment` and `receive_packet` calls on `node`
    // are synchronised so that the extended implementation does not have to perform
    // locking on data structures
    std::mutex send_recv_mt;

    struct SegmentToSendInfo {
        IPAddress dest_ip;
        std::vector<uint8_t> segment;
        SegmentToSendInfo(nlohmann::json const& segment_to_send_info_json);
    };

private:
    Node* node;
    bool has_ended;

    struct PacketReceivedInfo {
        MACAddress src_mac;
        std::vector<uint8_t> packet;
        PacketReceivedInfo(MACAddress src_mac, std::vector<uint8_t> const& packet)
            : src_mac(src_mac), packet(packet) { }
    };

    std::queue<PacketReceivedInfo> inbound;
    std::vector<SegmentToSendInfo> outbound;

    // to synchronise push and pop to `inbound` queue
    std::mutex inbound_mt;
    std::condition_variable cv;

    std::thread receive_thread;
    std::thread send_thread;

    void receive_loop();
    void send_loop();

public:
    NodeWork(Node* node, std::vector<SegmentToSendInfo> const& outbound)
        : node(node), has_ended(false), outbound(outbound) { }
    ~NodeWork();
    // TODO interface is hacky but needed for correctness
    void end_send();
    void run();
    void receive_frame(MACAddress src_mac, std::vector<uint8_t> const& packet);
};

#endif // NODE_WORK_H
