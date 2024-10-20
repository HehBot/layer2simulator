#ifndef SIMULATION_H
#define SIMULATION_H

#include "node.h"
#undef send_packet
#undef broadcast_packet

#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <unordered_map>

class NodeWork;

struct Simulation {
private:
    bool log_enabled;
    std::string logfile_prefix;
    std::chrono::system_clock::time_point tp_start;

    std::unordered_map<MACAddress, NodeWork*> nodes;
    std::unordered_map<IPAddress, MACAddress> ip_to_mac;
    std::unordered_map<MACAddress, std::unordered_map<MACAddress, size_t>> adj;

    std::map<std::pair<MACAddress, std::string>, bool> segment_delivered;

    std::atomic<size_t> total_packets_transmitted = 0;
    std::atomic<size_t> total_packet_distance = 0;

public:
    Simulation(bool log_enabled, std::string logfile_prefix, std::istream& net_spec);
    void run(std::istream& msg_file);
    ~Simulation();

    void send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet, bool inc);
    void broadcast_packet(MACAddress src_mac, std::vector<uint8_t> const& packet, bool inc);
    void verify_received_segment(IPAddress src_ip, MACAddress dest_mac, std::vector<uint8_t> const& segment);
    void node_log(MACAddress, std::string logline) const;
};

#endif // SIMULATION_H
