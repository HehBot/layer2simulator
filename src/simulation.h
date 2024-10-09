#ifndef SIMULATION_H
#define SIMULATION_H

#include "node.h"

#include <chrono>
#include <iosfwd>
#include <map>
#include <set>
#include <vector>

class NodeWork;

struct Simulation {
private:
    // for logging
    bool log_enabled;
    std::chrono::system_clock::time_point tp_start;
    std::map<MACAddress, std::ostream*> log_streams;

public:
    std::map<MACAddress, NodeWork*> nodes;
    std::map<MACAddress, std::set<MACAddress>> network_graph;

    Simulation(bool log_enabled, std::istream& i);
    void run();
    ~Simulation();

    void send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet);
    void log(MACAddress, IPAddress, std::string logline) const;
};

#endif // SIMULATION_H
