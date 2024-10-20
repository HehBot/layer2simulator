#include "node_work.h"
#include "simulation.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

void Simulation::run(std::istream& msgfile)
{
    std::string line;
    bool keep_going = (std::getline(msgfile, line) ? true : false);
    while (keep_going) {
        std::cout << std::string(50, '=') << '\n';

        total_nonperiodic_packets_transmitted = 0;
        total_nonperiodic_packets_distance = 0;
        total_packets_transmitted = 0;
        total_packets_distance = 0;

        for (auto g : nodes)
            g.second->launch_recv();
        for (auto g : nodes)
            g.second->launch_periodic();

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

        do {
            std::stringstream ss(line);
            std::string type;
            ss >> type;
            if (type == "MSG") {
                MACAddress src_mac;
                IPAddress dest_ip;
                size_t count = 1;
                std::string next;
                ss >> next;
                if (next == "REPE") {
                    ss >> count;
                    ss >> src_mac >> dest_ip;
                } else {
                    src_mac = std::stoi(next);
                    ss >> dest_ip;
                }
                std::string segment;
                std::getline(ss, segment);
                auto it = nodes.find(src_mac);
                if (it == nodes.end())
                    throw std::invalid_argument("Bad message file: Invalid MAC '" + std::to_string(src_mac) + "', not a MAC address of a node");
                auto it2 = ip_to_mac.find(dest_ip);
                if (it2 == ip_to_mac.end())
                    throw std::invalid_argument("Bad message file: Invalid IP '" + std::to_string(dest_ip) + "', not an IP address of a node");

                if (count == 1) {
                    segment_delivered[{ it2->second, segment }] = false;
                    nodes[src_mac]->add_to_send_segment_queue(NodeWork::SegmentToSendInfo(dest_ip, std::vector<uint8_t>(segment.begin(), segment.end())));
                } else {
                    std::vector<NodeWork::SegmentToSendInfo> v;
                    v.reserve(count);
                    for (size_t i = 0; i < count; ++i) {
                        std::string r = segment + "#" + std::to_string(i);
                        segment_delivered[{ it2->second, r }] = false;
                        auto s = NodeWork::SegmentToSendInfo(dest_ip, std::vector<uint8_t>(r.begin(), r.end()));
                        v.push_back(s);
                    }
                    nodes[src_mac]->add_to_send_segment_queue(v);
                }
            } else if (type == "UP" || type == "DOWN")
                break;
            else
                throw std::invalid_argument("Bad message file: Unknown type line '" + type + "'");
        } while ((keep_going = (std::getline(msgfile, line) ? true : false)));

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

        for (auto g : nodes)
            g.second->send_segments();

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

        for (auto g : nodes)
            g.second->end_periodic();

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

        for (auto g : nodes)
            g.second->end_recv();

        std::cout << std::string(50, '=') << '\n';

        log(LogLevel::INFO, "Total packets transmitted = " + std::to_string(total_nonperiodic_packets_transmitted));
        log(LogLevel::INFO, "Total packet distance     = " + std::to_string(total_nonperiodic_packets_distance));
        double avg_nonperiodic_packet_distance = total_nonperiodic_packets_distance * 1.0 / total_nonperiodic_packets_transmitted;
        log(LogLevel::INFO, "Average packet distance   = " + std::to_string(avg_nonperiodic_packet_distance));
        double avg_packet_distance = total_packets_distance * 1.0 / total_packets_transmitted;
        log(LogLevel::INFO, "Total packets transmitted (incl. periodic) = " + std::to_string(total_packets_transmitted));
        log(LogLevel::INFO, "Total packet distance     (incl. periodic) = " + std::to_string(total_packets_distance));
        log(LogLevel::INFO, "Average packet distance   (incl. periodic) = " + std::to_string(avg_packet_distance));

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
            log(LogLevel::WARNING, ss.str());
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
                log(LogLevel::INFO, log_prefix + std::to_string(mac) + ")");
                it->second->is_up = is_up;
            }
        } while ((keep_going = (std::getline(msgfile, line) ? true : false)));
    }
}
