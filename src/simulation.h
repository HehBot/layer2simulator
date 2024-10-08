#ifndef SIMULATION_H
#define SIMULATION_H

#include "node.h"

#include <fstream>
#include <map>
#include <set>
#include <vector>

class NodeThread;

struct Simulation {
    std::map<MACAddress, NodeThread*> nodes;
    std::map<MACAddress, std::set<MACAddress>> network_graph;

    Simulation(std::ifstream& i);
    void run();
    ~Simulation();

    void send_packet(MACAddress src_mac, MACAddress dest_mac, std::vector<uint8_t> const& packet);
};

#endif // SIMULATION_H
