#include "node_work.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

auto constexpr periodic_interval = std::chrono::microseconds(100);

void NodeWork::send_loop()
{
    while (true) {
        std::unique_lock<std::mutex> ul(outbound_mt);
        outbound_cv.wait(ul, [this]() { return outbound.size() > 0 || !on; });
        if (outbound.size() == 0 && !on)
            break;

        for (auto const& f : outbound) {
            send_recv_mt.lock();
            node->send_segment(f.dest_ip, f.segment);
            send_recv_mt.unlock();
        }
        outbound.clear();

        ul.unlock();
        outbound_empty_cv.notify_all();
    }
}

void NodeWork::receive_loop()
{
    while (true) {
        std::unique_lock<std::mutex> ul(inbound_mt);
        inbound_cv.wait(ul, [this]() { return inbound.size() > 0 || !recv_on; });
        if (inbound.size() == 0 && !recv_on)
            break;

        while (inbound.size() > 0) {
            PacketReceivedInfo const& f = inbound.front();
            send_recv_mt.lock();
            node->receive_packet(f.src_mac, f.packet);
            send_recv_mt.unlock();
            inbound.pop();
        }
    }
}

void NodeWork::periodic_loop()
{
    while (on) {
        auto d = std::chrono::system_clock::now() - last_periodic - periodic_interval;
        if (d.count() > 0) {
            node->do_periodic();
            last_periodic = std::chrono::system_clock::now();
        } else
            std::this_thread::sleep_for(-0.7 * d);
    }
}

NodeWork::NodeWork(Node* node, std::ostream* logger)
    : node(node), recv_on(false), last_periodic(std::chrono::system_clock::now()), on(true), logger(logger)
{
    start_recv();
    send_thread = std::thread(&NodeWork::send_loop, this);
    periodic_thread = std::thread(&NodeWork::periodic_loop, this);
}
NodeWork::~NodeWork()
{
    on = false;

    periodic_thread.join();

    outbound_cv.notify_one();
    send_thread.join();

    end_recv();

    delete node;
    delete logger;
}

void NodeWork::start_recv()
{
    if (!recv_on) {
        recv_on = true;
        receive_thread = std::thread(&NodeWork::receive_loop, this);
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

void NodeWork::send(std::vector<SegmentToSendInfo> const& o)
{
    std::unique_lock<std::mutex> ul(outbound_mt);
    outbound.insert(outbound.end(), o.begin(), o.end());
    ul.unlock();
    outbound_cv.notify_one();
}

void NodeWork::flush_send()
{
    std::unique_lock<std::mutex> ul(outbound_mt);
    outbound_empty_cv.wait(ul, [this] { return outbound.size() == 0; });
}

void NodeWork::receive_frame(MACAddress src_mac, std::vector<uint8_t> const& packet)
{
    std::unique_lock<std::mutex> ul(inbound_mt);
    inbound.push(PacketReceivedInfo { src_mac, packet });
    ul.unlock();
    inbound_cv.notify_one();
}

void NodeWork::log(std::string logline)
{
    std::unique_lock<std::mutex> ul(log_mt);
    (*logger) << logline << std::flush;
}
