#include "node_work.h"
#include "simulation.h"

#include <iostream>
#include <mutex>
#include <queue>

bool NodeWork::do_send()
{
    std::unique_lock<std::mutex> ul(outbound_mt);

    if (outbound.size() == 0)
        return false;

    for (auto const& f : outbound) {
        node_mt.lock();
        node->send_segment(f.dest_ip, f.segment);
        node_mt.unlock();
    }
    outbound.clear();

    return true;
}

bool NodeWork::do_receive()
{
    std::unique_lock<std::mutex> ul(inbound_mt);

    if (inbound.size() == 0)
        return false;

    while (inbound.size() > 0) {
        PacketReceivedInfo const& f = inbound.front();
        node_mt.lock();
        node->receive_packet(f.src_mac, f.packet, f.dist);
        node_mt.unlock();
        inbound.pop();
    }

    return true;
}

void NodeWork::do_periodic()
{
    node_mt.lock();
    node->do_periodic(simul.time_ms());
    node_mt.unlock();
}

NodeWork::~NodeWork()
{
    delete node;
    delete logger;
}

void NodeWork::send(std::vector<SegmentToSendInfo> const& o)
{
    std::unique_lock<std::mutex> ul(outbound_mt);
    outbound.insert(outbound.end(), o.begin(), o.end());
}
void NodeWork::send(SegmentToSendInfo o)
{
    std::unique_lock<std::mutex> ul(outbound_mt);
    outbound.push_back(o);
}

void NodeWork::receive_frame(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t dist)
{
    std::unique_lock<std::mutex> ul(inbound_mt);
    inbound.push(PacketReceivedInfo { src_mac, dist, packet });
}

void NodeWork::log(std::string logline)
{
    std::unique_lock<std::mutex> ul(log_mt);
    (*logger) << '[' << loglineno++ << "] " << logline << '\n'
              << std::flush;
}
