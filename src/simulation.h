#ifndef SIMULATION_H
#define SIMULATION_H

#include "node.h"

#include <chrono>
#include <iosfwd>
#include <map>
#include <vector>

class NodeWork;

struct Simulation {
private:
    bool log_enabled;
    std::chrono::system_clock::time_point tp_start;

    std::map<MACAddress, NodeWork*> nodes;
    std::map<MACAddress, std::map<MACAddress, size_t>> adj;

public:
    Simulation(bool log_enabled, std::istream& i);
    void run();
    ~Simulation();

    void send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet) const;
    void broadcast_packet(MACAddress src_mac, std::vector<uint8_t> const& packet) const;
    void log(MACAddress, std::string logline) const;
};

#endif // SIMULATION_H
