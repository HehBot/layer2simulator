#include "node_work.h"

#include <condition_variable>
#include <mutex>
#include <queue>

void NodeWork::receive_loop()
{
    while (true) {
        std::unique_lock<std::mutex> ul(inbound_mt);
        cv.wait(ul, [this]() { return inbound.size() > 0 || has_ended; });
        if (inbound.size() == 0 && has_ended)
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

void NodeWork::send_loop()
{
    for (auto const& g : outbound) {
        send_recv_mt.lock();
        node->send_segment(g.dest_ip, g.segment);
        send_recv_mt.unlock();
    }
}

NodeWork::~NodeWork()
{
    std::unique_lock<std::mutex> ul(inbound_mt);
    has_ended = true;
    ul.unlock();
    cv.notify_one();
    receive_thread.join();
    delete node;
}

void NodeWork::end_send()
{
    send_thread.join();
}

void NodeWork::run()
{
    receive_thread = std::thread(&NodeWork::receive_loop, this);
    send_thread = std::thread(&NodeWork::send_loop, this);
}

void NodeWork::receive_frame(MACAddress src_mac, std::vector<uint8_t> const& packet)
{
    std::unique_lock<std::mutex> ul(inbound_mt);
    inbound.push(PacketReceivedInfo { src_mac, packet });
    ul.unlock();
    cv.notify_one();
}
