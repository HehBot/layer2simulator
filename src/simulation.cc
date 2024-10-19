#include "node.h"
#include "node_work.h"
#include "simulation.h"

#include <cassert>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

enum class LogLevel {
    INFO,
    EVENT,
    WARNING,
    ERROR,
};
static void simul_log(LogLevel l, std::string logline)
{
    static std::mutex stdout_mt;
    stdout_mt.lock();
    std::string ll;
    switch (l) {
    case LogLevel::INFO:
        ll = "\x1b[34;1m[INFO] \x1b[0m";
        break;
    case LogLevel::EVENT:
        ll = "\x1b[32;1m[EVENT] \x1b[0m";
        break;
    case LogLevel::WARNING:
        ll = "\x1b[31;1m[WARNING] \x1b[0m";
        break;
    case LogLevel::ERROR:
        ll = "\x1b[31;1;5m[ERROR] \x1b[0m";
        break;
    }
    std::cout << ll << logline << '\n'
              << std::flush;
    stdout_mt.unlock();
}

void Node::send_packet(MACAddress dest_mac, std::vector<uint8_t> const& packet) const
{
    simul->send_packet(this->mac, dest_mac, packet);
}
void Node::broadcast_packet(std::vector<uint8_t> const& packet) const
{
    simul->broadcast_packet(this->mac, packet);
}
void Node::receive_segment(IPAddress src_ip, std::vector<uint8_t> const& segment) const
{
    simul->verify_received_segment(src_ip, this->mac, segment);
}
void Node::log(std::string logline) const
{
    simul->node_log(this->mac, logline);
}

void Simulation::recv_loop(bool& recv_flush)
{
    while (true) {
        bool did = false;
        for (auto g : nodes)
            did = did || g.second->do_receive();
        if (!did && recv_flush)
            break;
    }
}
void Simulation::periodic_loop(bool& on)
{
    while (on) {
        for (auto g : nodes)
            g.second->do_periodic();
    }
}
void Simulation::send_loop(bool& send_flush)
{
    while (true) {
        bool did = false;
        for (auto g : nodes)
            did = did || g.second->do_send();
        if (!did && send_flush)
            break;
    }
}
void Simulation::run(std::istream& msgfile)
{
    std::string line;
    bool keep_going = (std::getline(msgfile, line) ? true : false);
    while (keep_going) {
        std::cout << std::string(50, '=') << '\n';

        bool on = true, recv_flush = false, send_flush = false;

        std::thread rt = std::thread(&Simulation::recv_loop, this, std::ref(recv_flush));
        std::thread pt = std::thread(&Simulation::periodic_loop, this, std::ref(on));

        do {
            std::stringstream ss(line);
            std::string type;
            ss >> type;
            if (type == "MSG") {
                MACAddress src_mac;
                IPAddress dest_ip;
                ss >> src_mac >> dest_ip;
                std::string segment;
                std::getline(ss, segment);
                auto it = nodes.find(src_mac);
                if (it == nodes.end())
                    throw std::invalid_argument("Bad message file: Invalid node '" + std::to_string(src_mac) + "', not a MAC address of a node");
                auto it2 = ip_to_mac.find(dest_ip);
                if (it2 == ip_to_mac.end())
                    throw std::invalid_argument("Bad message file: Invalid node '" + std::to_string(src_mac) + "', not a MAC address of a node");

                segment_delivered[{ it2->second, segment }] = false;

                nodes[src_mac]->send(NodeWork::SegmentToSendInfo(dest_ip, std::vector<uint8_t>(segment.begin(), segment.end())));
            } else if (type == "UP" || type == "DOWN")
                break;
            else
                throw std::invalid_argument("Bad message file: Unknown type line '" + type + "'");
        } while ((keep_going = (std::getline(msgfile, line) ? true : false)));

        std::thread st = std::thread(&Simulation::send_loop, this, std::ref(send_flush));

        send_flush = true;
        st.join();
        on = false;
        pt.join();
        recv_flush = true;
        rt.join();

        std::cout << std::string(50, '=') << '\n';

        bool found_undelivered = false;
        std::stringstream ss;
        ss << "Some segment(s) not delivered:\n";
        for (auto i : segment_delivered) {
            if (!i.second) {
                found_undelivered = true;
                ss << "\tAt (mac:" << i.first.first << ") with contents:\n\t\t" << i.first.second << '\n';
            }
        }
        if (found_undelivered)
            simul_log(LogLevel::WARNING, ss.str());
        segment_delivered.clear();

        // up/down nodes
        do {
            std::stringstream ss(line);
            std::string type;
            ss >> type;
            bool is_up;
            std::string log_prefix;
            if (type == "DOWN") {
                is_up = false;
                log_prefix = "Bringing down (mac:";
            } else if (type == "UP") {
                is_up = true;
                log_prefix = "Bringing up (mac:";
            } else
                break;
            MACAddress mac;
            while (ss >> mac) {
                auto it = nodes.find(mac);
                if (it == nodes.end())
                    throw std::invalid_argument("Bad message file: Invalid node '" + std::to_string(mac) + "', not a MAC address of a node");
                simul_log(LogLevel::INFO, log_prefix + std::to_string(mac) + ")");
                it->second->is_up = is_up;
            }
        } while ((keep_going = (std::getline(msgfile, line) ? true : false)));
    }
}
void Simulation::send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet) const
{
    assert(nodes.count(src_mac) > 0);
    if (nodes.count(dest_mac) == 0) {
        simul_log(LogLevel::ERROR, "Attempted to send to MAC address '" + std::to_string(dest_mac) + "' which is not a MAC address of any node");
        return;
    }

    auto it = adj.at(src_mac).find(dest_mac);
    if (it == adj.at(src_mac).end()) {
        simul_log(LogLevel::ERROR, "Attempted to send to MAC address '" + std::to_string(dest_mac) + "' which is not a MAC address of any neighbour of (mac:" + std::to_string(src_mac) + ")");
        return;
    }

    NodeWork* src_nt = nodes.at(src_mac);
    NodeWork* dest_nt = nodes.at(dest_mac);

    if (!dest_nt->is_up) {
        simul_log(LogLevel::ERROR, "Attempted to send to (mac:" + std::to_string(dest_mac) + ") which is down");
        return;
    }

    // src_nt->send_recv_mt must be locked at this point
    src_nt->node_mt.unlock();
    dest_nt->receive_frame(src_mac, packet, it->second);
    src_nt->node_mt.lock();
}
void Simulation::broadcast_packet(MACAddress src_mac, std::vector<uint8_t> const& packet) const
{
    assert(nodes.count(src_mac) > 0);
    NodeWork* src_nt = nodes.at(src_mac);

    for (auto r : adj.at(src_mac)) {
        src_nt->node_mt.unlock();
        nodes.at(r.first)->receive_frame(src_mac, packet, r.second);
        src_nt->node_mt.lock();
    }
}
void Simulation::verify_received_segment(IPAddress src_ip, MACAddress dest_mac, std::vector<uint8_t> const& segment)
{
    std::string segment_str(segment.begin(), segment.end());

    auto it = segment_delivered.find({ dest_mac, segment_str });
    if (it == segment_delivered.end())
        simul_log(LogLevel::ERROR, "Segment from (ip:" + std::to_string(src_ip) + ") wrongly delivered to (mac:" + std::to_string(dest_mac) + " with contents:\n\t" + segment_str);
    else {
        std::string logline = "(mac:" + std::to_string(dest_mac) + ") received segment from (ip:" + std::to_string(src_ip) + ") with contents:\n\t" + segment_str;
        if (it->second)
            logline = "{Duplicate delivery} " + logline;
        it->second = true;
        simul_log(LogLevel::EVENT, logline);
    }
}
Simulation::~Simulation()
{
    for (auto const& g : nodes)
        delete g.second;
}

size_t Simulation::time_us() const
{
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(tp - tp_start).count();
}

void Simulation::node_log(MACAddress mac, std::string logline) const
{
    auto p = nodes.at(mac);
    if (log_enabled && p->log_enabled() && !p->log(logline))
        simul_log(LogLevel::WARNING, "Too many logs emitted at (mac:" + std::to_string(mac) + "), no more logs will be written");
}
