#ifndef SIMULATION_H
#define SIMULATION_H

#include "node.h"

#include <chrono>
#include <map>
#include <set>
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

    std::set<std::pair<MACAddress, std::string>> pending_segments;

    void recv_loop(bool& recv_flush);
    void periodic_loop(bool& on);
    void send_loop(bool& send_flush);

public:
    Simulation(bool log_enabled, std::string logfile_prefix, std::istream& net_spec);
    size_t time_us() const;
    void run(std::istream& msg_file);
    ~Simulation();

    void send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet) const;
    void broadcast_packet(MACAddress src_mac, std::vector<uint8_t> const& packet) const;
    void verify_received_segment(IPAddress src_ip, MACAddress dest_mac, std::vector<uint8_t> const& segment);
    void log(MACAddress, std::string logline) const;
};

#endif // SIMULATION_H
