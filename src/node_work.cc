#include "node_work.h"
#include "simulation.h"

#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

enum class LogLevel {
    DEBUG,
    INFO,
    EVENT,
    WARNING,
    ERROR,
};
void simul_log(LogLevel l, std::string logline);
size_t constexpr MAXLOGLINES = 20000;

bool NodeWork::send_segments()
{
    if (!is_up)
        return false;

    for (auto const& f : outbound) {
        node_mt.lock();
        node->send_segment(f.dest_ip, f.segment);
        node_mt.unlock();
    }
    outbound.clear();

    return true;
}

void NodeWork::receive_loop()
{
    if (!is_up)
        return;
    while (true) {
        node_mt.lock();
        inbound_mt.lock();

        // inbound_cv.wait(ul, [this]{ return inbound.size() > 0 || !recv_on; });

        if (inbound.size() == 0 && !recv_on) {
            inbound_mt.unlock();
            node_mt.unlock();
            break;
        }

        while (inbound.size() > 0) {
            PacketReceivedInfo const& f = inbound.front();
            inbound_mt.unlock();
            node->receive_packet(f.src_mac, f.packet, f.dist);
            inbound_mt.lock();
            inbound.pop();
        }
        inbound_mt.unlock();
        node_mt.unlock();
    }
}

void NodeWork::periodic_loop()
{
    if (!is_up)
        return;
    while (periodic_on) {
        node_mt.lock();
        node->do_periodic();
        node_mt.unlock();
    }
}

void NodeWork::launch_recv()
{
    if (!recv_on) {
        recv_on = true;
        inbound = std::queue<PacketReceivedInfo>();
        receive_thread = std::thread(&NodeWork::receive_loop, this);
    }
}
void NodeWork::launch_periodic()
{
    if (!periodic_on) {
        periodic_on = true;
        periodic_thread = std::thread(&NodeWork::periodic_loop, this);
    }
}

void NodeWork::end_recv()
{
    if (recv_on) {
        std::unique_lock<std::mutex> ul(inbound_mt);
        recv_on = false;
        ul.unlock();
        inbound_cv.notify_one();
        receive_thread.join();
    }
}
void NodeWork::end_periodic()
{
    if (periodic_on) {
        periodic_on = false;
        periodic_thread.join();
    }
}

NodeWork::~NodeWork()
{
    end_periodic();
    end_recv();
    delete node;
    delete logger;
}

void NodeWork::add_to_send_segment_queue(std::vector<SegmentToSendInfo> const& o)
{
    outbound.insert(outbound.end(), o.begin(), o.end());
}
void NodeWork::add_to_send_segment_queue(SegmentToSendInfo o)
{
    outbound.push_back(o);
}

void NodeWork::receive_frame(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t dist)
{
    std::unique_lock<std::mutex> ul(inbound_mt);
    inbound.push(PacketReceivedInfo { src_mac, dist, packet });
    ul.unlock();
    inbound_cv.notify_one();
}

bool NodeWork::log(std::string logline)
{
    std::unique_lock<std::mutex> ul(log_mt);
    if (loglineno >= MAXLOGLINES) {
        delete logger;
        logger = nullptr;
        return false;
    }
    (*logger) << '[' << loglineno++ << "] " << logline << '\n'
              << std::flush;
    return true;
}
bool NodeWork::log_enabled() const
{
    return logger != nullptr;
}
