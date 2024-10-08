#include "nlohmann/json.hpp"
#include "node_impl/naive.h"
#include "node_work.h"
#include "simulation.h"

#include <fstream>

NodeWork::SegmentToSendInfo::SegmentToSendInfo(nlohmann::json const& segment_to_send_info_json)
{
    if (!segment_to_send_info_json.contains("dest_ip"))
        throw std::invalid_argument("Bad network file, each entry of 'segments_to_send_info' should contain 'dest_ip'");
    auto const& dest_ip_json = segment_to_send_info_json["dest_ip"];
    if (!dest_ip_json.is_number_integer())
        throw std::invalid_argument("Bad network file, each entry of 'segments_to_send_info' should have 'dest_ip' field as integer");
    dest_ip = IPAddress(dest_ip_json);

    if (!segment_to_send_info_json.contains("segment"))
        throw std::invalid_argument("Bad network file, each entry of 'segments_to_send_info' should contain 'segment'");
    auto const& segment_json = segment_to_send_info_json["segment"];
    if (!segment_json.is_string())
        throw std::invalid_argument("Bad network file, each entry of 'segments_to_send_info' should have 'segment' field as string");
    std::string segment_str(segment_json);
    segment = std::vector<uint8_t>(segment_str.begin(), segment_str.end());
}

Simulation::Simulation(bool log_enabled, std::istream& i)
    : log_enabled(log_enabled)
{
    clock_gettime(CLOCK_MONOTONIC, &tp_start);

    auto j = nlohmann::json::parse(i);
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
        throw std::invalid_argument(std::string("Bad network file: Invalid 'node_type' '") + nt_str + "'");

    auto const& ni_json = j["network_info"];
    if (!ni_json.is_array())
        throw std::invalid_argument("Bad network file: Invalid 'network_info', must be an array!");

    std::map<MACAddress, IPAddress> ips;
    std::map<MACAddress, std::vector<NodeWork::SegmentToSendInfo>> segments_to_send_info;

    for (auto const& node_json : ni_json) {
        if (!node_json.contains("mac"))
            throw std::invalid_argument("Bad network file: Invalid node, no 'mac' specified");
        auto const& mac_json = node_json["mac"];
        if (!mac_json.is_number_integer())
            throw std::invalid_argument("Bad network file: Invalid node, 'mac' should be an integer");
        MACAddress mac(mac_json);
        if (network_graph.count(mac) > 0)
            throw std::invalid_argument("Bad network file: Invalid node, 'mac' repeated across nodes");

        if (!node_json.contains("ip"))
            throw std::invalid_argument("Bad network file: Invalid node, no 'ip' specified");
        auto const& ip_json = node_json["ip"];
        if (!ip_json.is_number_integer())
            throw std::invalid_argument("Bad network file: Invalid node, 'ip' should be an integer");
        ips[mac] = IPAddress(ip_json);

        if (!node_json.contains("neighbours"))
            throw std::invalid_argument("Bad network file: Invalid node, no 'neighbours' specified");
        auto const& neighbours_json = node_json["neighbours"];
        if (!neighbours_json.is_array())
            throw std::invalid_argument("Bad network file: Invalid node, 'neighbours' should be an array");
        std::set<MACAddress> neighbours;
        for (auto const& neighbour_mac_json : neighbours_json) {
            if (!neighbour_mac_json.is_number_integer())
                throw std::invalid_argument("Bad network file: Invalid node, each entry of 'neighbour' should be an integer");
            neighbours.insert(MACAddress(neighbour_mac_json));
        }
        if (neighbours.count(mac) > 0)
            throw std::invalid_argument("Bad network file: Invalid node, cannot be its own neighbour");
        network_graph[mac] = neighbours;

        if (!node_json.contains("segments_to_send"))
            throw std::invalid_argument("Bad network file: Invalid node, no 'segments_to_send' specified");
        auto const& segments_to_send_json = node_json["segments_to_send"];
        if (!segments_to_send_json.is_array())
            throw std::invalid_argument("Bad network file: Invalid node, 'segments_to_send' should be an array");
        std::vector<NodeWork::SegmentToSendInfo> v;
        for (auto const& segment_to_send_info_json : segments_to_send_json)
            v.push_back(NodeWork::SegmentToSendInfo(segment_to_send_info_json));
        segments_to_send_info[mac] = v;
    }
    for (auto const& adj : network_graph) {
        for (auto const& neighbour : adj.second)
            if (ips.count(neighbour) == 0)
                throw std::invalid_argument("Bad network file: Invalid neighbour, not a MAC address of a node");
        MACAddress mac = adj.first;
        Node* node;
        switch (node_type) {
        case NT::NAIVE:
            node = new NaiveNode(*this, mac, ips[mac]);
            break;
        // XXX add others
        default:
            __builtin_unreachable();
        }
        nodes[mac] = new NodeWork(node, segments_to_send_info[mac]);
    }

    if (log_enabled) {
        for (auto const& g : nodes) {
            MACAddress mac = g.first;
            log_streams[mac] = new std::ofstream(std::string("node-") + std::to_string(mac) + ".log");
        }
    }
}
