#ifndef SIMULATION_H
#define SIMULATION_H

#include "node.h"

#include <chrono>
#include <map>

class NodeWork;

struct Simulation {
private:
    bool log_enabled;
    std::string logfile_prefix;
    std::chrono::system_clock::time_point tp_start;

    std::map<MACAddress, NodeWork*> nodes;
    std::map<MACAddress, std::map<MACAddress, size_t>> adj;

    void recv_loop(bool& recv_flush);
    void periodic_loop(bool& on);
    void send_loop(bool& send_flush);

public:
    Simulation(bool log_enabled, std::string logfile_prefix, std::istream& net_spec);
    double time_ms() const;
    void run(std::istream& msg_file);
    ~Simulation();

    void send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet) const;
    void broadcast_packet(MACAddress src_mac, std::vector<uint8_t> const& packet) const;
    void verify_received_segment(IPAddress src_ip, MACAddress dest_mac, std::vector<uint8_t> const& segment) const;
    void log(MACAddress, std::string logline) const;
};

#endif // SIMULATION_H
