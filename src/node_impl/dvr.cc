#include "dvr.h"

#include <cstring>
#include <vector>

#define INITIAL_VALIDITY 70
#define MAX_TTL 4

void DVRNode::send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const
{
    PacketHeader pkt_header(ip, dest_ip, false, MAX_TTL);
    std::vector<uint8_t> packet(sizeof(PacketHeader) + segment.size());
    memcpy(&packet[0], &pkt_header, sizeof(PacketHeader));
    memcpy(&packet[sizeof(PacketHeader)], &segment[0], segment.size());

    if (distance_vector.find(dest_ip) == distance_vector.end()) {
        broadcast_packet(packet);
        return;
    }

    MACAddress neighbor_to_send = gateway.at(dest_ip);

    // log("Sending packet to " + std::to_string(dest_ip) + " via " + std::to_string(neighbor_to_send));
    send_packet(neighbor_to_send, packet);
}

void DVRNode::receive_packet(MACAddress src_mac, std::vector<uint8_t> packet, size_t distance)
{
    // log("Received packet from " + std::to_string(src_mac) + " with distance " + std::to_string(distance));
    PacketHeader pkt_header;
    memcpy(&pkt_header, &packet[0], sizeof(PacketHeader));

    if (pkt_header.is_arp) {
        // log("Received distance vector from " + std::to_string(src_mac));
        std::vector<uint8_t> segment(packet.begin() + sizeof(PacketHeader), packet.end());

        IPAddress neighbor_ip = *(IPAddress*)(&segment[0]);
        MACAddress neighbor_mac = *(MACAddress*)(&segment[sizeof(IPAddress)]);

        // log("Received distance vector from " + std::to_string(neighbor_ip) + " with distance " + std::to_string(distance) + " with mac " + std::to_string(neighbor_mac));

        gateway[neighbor_ip] = neighbor_mac;
        neighbor_distances[neighbor_mac] = distance;
        distance_vector[neighbor_ip] = distance;
        distance_vector_validity[neighbor_ip] = INITIAL_VALIDITY;
        neighbor_ips[neighbor_mac] = neighbor_ip;

        for (auto it = segment.begin() + sizeof(IPAddress) + sizeof(MACAddress); it != segment.end(); it += sizeof(IPAddress) + sizeof(size_t)) {
            IPAddress ip = *(IPAddress*)(&(*it));
            size_t dist = *(size_t*)(&(*(it + sizeof(IPAddress))));

            if (distance_vector.find(ip) == distance_vector.end() || distance_vector[ip] > dist + distance) {
                distance_vector[ip] = dist + distance;
                distance_vector_validity[ip] = INITIAL_VALIDITY;
                gateway[ip] = neighbor_mac;
            } else if (gateway[ip] == neighbor_mac) {
                distance_vector_validity[ip] = INITIAL_VALIDITY;
            }
        }

        return;
    }

    if (pkt_header.dest_ip != ip) {
        std::string log_msg = "Received packet with destination " + std::to_string(pkt_header.dest_ip) + " but my IP is " + std::to_string(ip);
        if (pkt_header.ttl == 0) {
            // log("Packet TTL expired");
            return;
        }

        IPAddress dest_ip = pkt_header.dest_ip;
        pkt_header.ttl--;
        memcpy(&packet[0], &pkt_header, sizeof(PacketHeader));

        if (distance_vector.find(dest_ip) == distance_vector.end()) {
            log_msg += " and I don't know how to reach it";
            broadcast_packet(packet);

            // log(log_msg);
            return;
        }

        log_msg += " and I know how to reach it and sending it to " + std::to_string(gateway.at(dest_ip));
        MACAddress neighbor_to_send = gateway.at(dest_ip);
        send_packet(neighbor_to_send, packet);
        // log(log_msg);
        return;
    }

    std::vector<uint8_t> segment(packet.begin() + sizeof(PacketHeader), packet.end());
    log("Received segment:\t" + std::string(segment.begin(), segment.end()));
    receive_segment(pkt_header.source_ip, segment);
}

void DVRNode::do_periodic()
{
    PacketHeader pkt_header(ip, 0, true, MAX_TTL);
    size_t size = sizeof(IPAddress) + sizeof(size_t);
    std::vector<uint8_t> packet(sizeof(PacketHeader) + sizeof(IPAddress) + sizeof(MACAddress) + size * distance_vector.size());

    memcpy(&packet[0], &pkt_header, sizeof(PacketHeader));
    memcpy(&packet[sizeof(PacketHeader)], &ip, sizeof(IPAddress));
    memcpy(&packet[sizeof(PacketHeader) + sizeof(IPAddress)], &mac, sizeof(MACAddress));

    std::vector<IPAddress> to_delete_from_distance_vector;
    for (auto it = distance_vector.begin(); it != distance_vector.end(); it++) {
        distance_vector_validity[it->first]--;
        if (distance_vector_validity[it->first] == 0)
            to_delete_from_distance_vector.push_back(it->first);
    }

    for (auto ip : to_delete_from_distance_vector) {
        if (neighbor_ips.find(gateway[ip]) != neighbor_ips.end() && neighbor_ips[gateway[ip]] == ip) {
            // log("Neighbor " + std::to_string(gateway[ip]) + " is down");
            neighbor_distances.erase(gateway[ip]);
            neighbor_ips.erase(gateway[ip]);
        }

        distance_vector.erase(ip);
        gateway.erase(ip);
        distance_vector_validity.erase(ip);
    }

    int counter = 0;
    for (auto it = distance_vector.begin(); it != distance_vector.end(); it++) {
        memcpy(&packet[sizeof(PacketHeader) + sizeof(IPAddress) + sizeof(MACAddress) + size * counter], &it->first, sizeof(IPAddress));
        memcpy(&packet[sizeof(PacketHeader) + sizeof(IPAddress) + sizeof(MACAddress) + size * counter + sizeof(IPAddress)], &it->second, sizeof(size_t));
        counter++;
    }

    broadcast_packet(packet);
    return;
}