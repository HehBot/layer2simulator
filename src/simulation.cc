#include "node.h"
#include "node_work.h"
#include "simulation.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

void Node::send_packet(MACAddress dest_mac, std::vector<uint8_t> const& packet) const
{
    simul.send_packet(this->mac, dest_mac, packet);
}
void Node::log(std::string logline) const
{
    simul.log(this->mac, logline);
}
void Simulation::run()
{
    // placeholder messages
    for (auto const& g : nodes) {
        std::vector<NodeWork::SegmentToSendInfo> v {};
        std::string s = std::to_string(g.second->node->ip) + "->";
        for (auto const& r : g.second->node->neighbour_distances) {
            std::string t = s + std::to_string(nodes[r.first]->node->ip);
            v.emplace_back(nodes[r.first]->node->ip, std::vector<uint8_t>(t.begin(), t.end()));
        }
        g.second->send(v);
    }

    for (auto const& g : nodes)
        g.second->flush_send();
}
void Simulation::send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet) const
{
    assert(nodes.count(src_mac) > 0);
    if (nodes.count(dest_mac) == 0)
        throw std::invalid_argument(std::to_string(dest_mac) + " is not a valid MAC address");
    if (network_graph.count(std::minmax(src_mac, dest_mac)) == 0)
        throw std::invalid_argument(std::to_string(dest_mac) + " is not a MAC address of any neighbour of " + std::to_string(src_mac));

    NodeWork* src_nt = nodes.at(src_mac);
    NodeWork* dest_nt = nodes.at(dest_mac);

    // src_nt->send_recv_mt must be locked at this point
    src_nt->send_recv_mt.unlock();
    dest_nt->receive_frame(src_mac, packet);
    src_nt->send_recv_mt.lock();
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
