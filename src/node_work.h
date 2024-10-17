#ifndef NODE_WORK_H
#define NODE_WORK_H

#include "node.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <ostream>
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
        SegmentToSendInfo(IPAddress ip, std::vector<uint8_t> const& segment) : dest_ip(ip), segment(segment) { }
    };

    Node* node;

private:
    struct PacketReceivedInfo {
        MACAddress src_mac;
        std::vector<uint8_t> packet;
        PacketReceivedInfo(MACAddress src_mac, std::vector<uint8_t> const& packet)
            : src_mac(src_mac), packet(packet) { }
    };

    std::queue<PacketReceivedInfo> inbound;
    // to synchronise push and pop to `inbound` queue
    std::mutex inbound_mt;
    std::condition_variable inbound_cv;
    std::thread receive_thread;
    void receive_loop();
    bool recv_on;

    std::vector<SegmentToSendInfo> outbound;
    // to synchronise push and pop to `outbound` queue
    std::mutex outbound_mt;
    std::condition_variable outbound_cv, outbound_empty_cv;
    std::thread send_thread;
    void send_loop();

    std::chrono::system_clock::time_point last_periodic;
    std::thread periodic_thread;
    void periodic_loop();

    bool on;

    std::mutex log_mt;
    std::ostream* logger;

public:
    NodeWork(Node* node, std::ostream* logger);
    ~NodeWork();
    void start_recv();
    void send(std::vector<SegmentToSendInfo> const& outbound);
    void flush_send();
    void end_recv();
    void receive_frame(MACAddress src_mac, std::vector<uint8_t> const& packet);
    void log(std::string logline);
};

#endif // NODE_WORK_H
