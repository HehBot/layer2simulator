#ifndef SIMULATION_H
#define SIMULATION_H

#include "node.h"

#include <chrono>
#include <map>
#include <string>

class NodeWork;

struct Simulation {
private:
    bool log_enabled;
    std::string logfile_prefix;
    std::chrono::system_clock::time_point tp_start;

    std::map<MACAddress, NodeWork*> nodes;
    std::map<IPAddress, MACAddress> ip_to_mac;
    std::map<MACAddress, std::map<MACAddress, size_t>> adj;

    std::map<std::pair<MACAddress, std::string>, bool> segment_delivered;

public:
    Simulation(bool log_enabled, std::string logfile_prefix, std::istream& net_spec);
    void run(std::istream& msg_file);
    ~Simulation();

    void send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet) const;
    void broadcast_packet(MACAddress src_mac, std::vector<uint8_t> const& packet) const;
    void verify_received_segment(IPAddress src_ip, MACAddress dest_mac, std::vector<uint8_t> const& segment);
    void node_log(MACAddress, std::string logline) const;
};

#endif // SIMULATION_H
