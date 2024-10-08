#include "node.h"
#include "node_work.h"
#include "simulation.h"

#include <cassert>
#include <ctime>
#include <iostream>
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
    simul.log(this->mac, this->ip, logline);
}
void Simulation::run()
{
    for (auto const& g : nodes)
        g.second->run();
}
void Simulation::send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet)
{
    assert(nodes.count(src_mac) > 0);
    if (nodes.count(dest_mac) == 0)
        throw std::invalid_argument(std::to_string(dest_mac) + " is not a valid MAC address");
    if (network_graph[src_mac].count(dest_mac) == 0)
        throw std::invalid_argument(std::to_string(dest_mac) + " is not a MAC address of any neighbour of " + std::to_string(src_mac));

    NodeWork* src_nt = nodes[src_mac];
    NodeWork* dest_nt = nodes[dest_mac];

    // src_nt->send_recv_mt must be locked at this point
    src_nt->send_recv_mt.unlock();
    dest_nt->receive_frame(src_mac, packet);
    src_nt->send_recv_mt.lock();
}
Simulation::~Simulation()
{
    for (auto const& g : nodes)
        g.second->end_send();
    for (auto const& g : nodes) {
        delete g.second;
        delete log_streams[g.first];
    }
}

void Simulation::log(MACAddress mac, IPAddress ip, std::string logline) const
{
    if (log_enabled) {
        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC, &tp);
        double dur = 1000 * ((tp.tv_sec + 1e-9 * tp.tv_nsec) - (tp_start.tv_sec + 1e-9 * tp_start.tv_nsec));
        (*log_streams.at(mac)) << "[" << dur << "ms] [MAC:" << mac << ",IP:" << ip << "]\t" << logline << '\n';
    }
}
