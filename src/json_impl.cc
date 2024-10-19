#include "nlohmann/json.hpp"
#include "node_impl/naive.h"
#include "node_work.h"
#include "simulation.h"

#include <chrono>
#include <fstream>
#include <string>

Simulation::Simulation(bool log_enabled, std::string logfile_prefix, std::istream& net_spec)
    : log_enabled(log_enabled), logfile_prefix(logfile_prefix), tp_start(std::chrono::system_clock::now())
{
    auto j = nlohmann::json::parse(net_spec);
    if (!j.contains("node_type"))
        throw std::invalid_argument("Bad network file: No 'node_type' specified");
    if (!j.contains("network_info"))
        throw std::invalid_argument("Bad network file: No 'network_info' specified");

    auto const& nt_json = j["node_type"];

    enum class NT {
        NAIVE,
        // XXX add others
    } node_type;

    if (!nt_json.is_string())
        throw std::invalid_argument("Bad network file: Invalid 'node_type', must be a string!");

    auto nt_str = std::string(nt_json);
    // XXX add others
    if (nt_str == "naive")
        node_type = NT::NAIVE;
    else
        throw std::invalid_argument(std::string("Bad network file: Invalid 'node_type' value '") + nt_str + "'");

    auto const& ni_json = j["network_info"];
    if (!ni_json.is_array())
        throw std::invalid_argument("Bad network file: Invalid 'network_info', must be an array!");

    std::map<MACAddress, IPAddress> ips;
    std::map<std::pair<MACAddress, MACAddress>, size_t> distances;

    for (auto const& node_json : ni_json) {
        if (!node_json.contains("mac"))
            throw std::invalid_argument("Bad network file: Invalid node, no 'mac' specified");
        auto const& mac_json = node_json["mac"];
        if (!mac_json.is_number_integer())
            throw std::invalid_argument("Bad network file: Invalid node, 'mac' should be an integer");
        MACAddress mac(mac_json);
        if (ips.count(mac) > 0)
            throw std::invalid_argument("Bad network file: Invalid node, 'mac' should be unique across nodes");

        if (!node_json.contains("ip"))
            throw std::invalid_argument("Bad network file: Invalid node, no 'ip' specified");
        auto const& ip_json = node_json["ip"];
        if (!ip_json.is_number_integer())
            throw std::invalid_argument("Bad network file: Invalid node, 'ip' should be an integer");
        IPAddress ip(ip_json);
        ips[mac] = ip;
        if (ip_to_mac.count(ip) > 0)
            throw std::invalid_argument("Bad network file: Invalid node, 'ip' should be unique across nodes");
        ip_to_mac[ip] = mac;

        if (!node_json.contains("neighbours"))
            throw std::invalid_argument("Bad network file: Invalid node, no 'neighbours' specified");
        auto const& neighbours_json = node_json["neighbours"];
        if (!neighbours_json.is_array())
            throw std::invalid_argument("Bad network file: Invalid node, 'neighbours' should be an array");

        std::map<MACAddress, size_t> ni;
        for (auto const& neighbour_json : neighbours_json) {
            if (!neighbour_json.contains("mac"))
                throw std::invalid_argument("Bad network file: Invalid node, each entry of 'neighbours' should contain 'mac'");
            auto const& neighbour_mac_json = neighbour_json["mac"];
            if (!neighbour_mac_json.is_number_integer())
                throw std::invalid_argument("Bad network file: Invalid node, each entry of 'neighbours' should have 'mac' field as integer");
            MACAddress neighbour_mac(neighbour_mac_json);
            if (!neighbour_json.contains("distance"))
                throw std::invalid_argument("Bad network file: Invalid node, each entry of 'neighbours' should contain 'distance'");
            auto const& neighbour_distance_json = neighbour_json["distance"];
            if (!neighbour_distance_json.is_number_integer())
                throw std::invalid_argument("Bad network file: Invalid node, each entry of 'neighbours' should have 'distance' field as integer");
            size_t neighbour_distance(neighbour_distance_json);

            auto const& it = distances.find(std::minmax(mac, neighbour_mac));
            if (it != distances.end() && it->second != neighbour_distance)
                throw std::invalid_argument("Bad network file: Invalid node, inconsistent distances");
            else
                distances[std::minmax(mac, neighbour_mac)] = neighbour_distance;

            ni[neighbour_mac] = neighbour_distance;
        }
        adj[mac] = ni;
    }
    for (auto const& adj : distances) {
        if (ips.count(adj.first.first) == 0)
            throw std::invalid_argument("Bad network file: Invalid neighbour '" + std::to_string(adj.first.first) + "', not a MAC address of a node");
        if (ips.count(adj.first.second) == 0)
            throw std::invalid_argument("Bad network file: Invalid neighbour '" + std::to_string(adj.first.second) + "', not a MAC address of a node");
    }

    // TODO check that graph is connected
    // TODO compute width of graph

    for (auto const& i : ips) {
        MACAddress mac = i.first;
        IPAddress ip = i.second;
        Node* node = nullptr;
        switch (node_type) {
        case NT::NAIVE:
            node = new NaiveNode(this, mac, ip);
            break;
            // XXX add others
        }
        assert(node != nullptr);
        std::ostream* log_stream = nullptr;
        if (log_enabled) {
            log_stream = new std::ofstream(logfile_prefix + std::to_string(mac) + ".log");
            (*log_stream) << std::setprecision(2) << std::fixed;
        }
        nodes[mac] = new NodeWork(node, log_stream, *this);
    }
}
