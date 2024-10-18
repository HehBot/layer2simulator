#include "node.h"
#include "node_work.h"
#include "simulation.h"

#include <cassert>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

void Node::send_packet(MACAddress dest_mac, std::vector<uint8_t> const& packet) const
{
    simul.send_packet(this->mac, dest_mac, packet);
}
void Node::broadcast_packet(std::vector<uint8_t> const& packet) const
{
    simul.broadcast_packet(this->mac, packet);
}
void Node::log(std::string logline) const
{
    simul.log(this->mac, logline);
}
void Simulation::run()
{
    bool pt_on = true, recv_flush = false, send_flush = false;
    std::thread rt = std::thread([&recv_flush, this] {
        while (true) {
            bool did = false;
            for (auto g : nodes)
                did = did || g.second->do_receive();
            if (!did && recv_flush)
                break;
        }
    });
    std::thread pt = std::thread([&pt_on, this] {
        while (pt_on) {
            for (auto g : nodes)
                g.second->do_periodic();
        }
    });
    std::thread st = std::thread([&send_flush, this] {
        while (true) {
            bool did = false;
            for (auto g : nodes)
                did = did || g.second->do_send();
            if (!did && send_flush)
                break;
        }
    });

    // placeholder messages
    for (auto const& g : nodes) {
        std::vector<NodeWork::SegmentToSendInfo> v {};
        std::string s = std::to_string(g.second->node->ip) + "->";
        for (auto const& r : adj[g.first]) {
            std::string t = s + std::to_string(r.first * 1000);
            v.emplace_back(r.first * 1000, std::vector<uint8_t>(t.begin(), t.end()));
        }
        g.second->send(v);
    }

    send_flush = true;
    st.join();

    pt_on = false;
    pt.join();

    recv_flush = true;
    rt.join();
}
void Simulation::send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet) const
{
    assert(nodes.count(src_mac) > 0);
    if (nodes.count(dest_mac) == 0)
        throw std::invalid_argument(std::to_string(dest_mac) + " is not a valid MAC address");
    auto it = adj.at(src_mac).find(dest_mac);
    if (it == adj.at(src_mac).end())
        throw std::invalid_argument(std::to_string(dest_mac) + " is not a MAC address of any neighbour of " + std::to_string(src_mac));

    NodeWork* src_nt = nodes.at(src_mac);
    NodeWork* dest_nt = nodes.at(dest_mac);

    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string s = ss.str();

    // src_nt->send_recv_mt must be locked at this point
    src_nt->node_mt.unlock();
    dest_nt->receive_frame(src_mac, packet, it->second);
    src_nt->node_mt.lock();
}
void Simulation::broadcast_packet(MACAddress src_mac, std::vector<uint8_t> const& packet) const
{
    assert(nodes.count(src_mac) > 0);
    NodeWork* src_nt = nodes.at(src_mac);

    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string s = ss.str();

    for (auto r : adj.at(src_mac)) {
        src_nt->node_mt.unlock();
        nodes.at(r.first)->receive_frame(src_mac, packet, r.second);
        src_nt->node_mt.lock();
    }
}
Simulation::~Simulation()
{
    for (auto const& g : nodes)
        delete g.second;
}

void Simulation::log(MACAddress mac, std::string logline) const
{
    if (log_enabled) {
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
        double dur = std::chrono::duration_cast<std::chrono::microseconds>(tp - tp_start).count() / 1000.0;
        std::string rawlogline = std::string("[") + std::to_string(dur) + "ms] " + logline + "\n";
        nodes.at(mac)->log(rawlogline);
    }
}
