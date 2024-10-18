#ifndef DVR_H
#define DVR_H

#include "../node.h"

#include <cstring>
#include <vector>

struct PacketHeader {
	PacketHeader(IPAddress source_ip, IPAddress dest_ip, MACAddress source_mac, bool is_arp): is_arp(is_arp), source_ip(source_ip), dest_ip(dest_ip), source_mac(source_mac) {}
	PacketHeader(){}

	bool is_arp;	
	IPAddress source_ip;
	IPAddress dest_ip;
	MACAddress source_mac;
};

class DVRNode : public Node {
private:
	std::unordered_map<IPAddress, size_t> distance_vector; 
	std::unordered_map<IPAddress, MACAddress> gateway;
	std::unordered_map<MACAddress, size_t> neighbor_distances;

public:
    DVRNode(Simulation& simul, MACAddress mac, IPAddress ip) : Node(simul, mac, ip) { }

    void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const override
	{
		if(distance_vector.find(dest_ip) == distance_vector.end()){
			// TODO: Add default gateway
			log("No route to " + std::to_string(dest_ip));
			assert(false);
		}

		MACAddress neighbor_to_send = gateway.at(dest_ip);
		PacketHeader pkt_header (ip, dest_ip, mac, false);
        std::vector<uint8_t> packet(sizeof(PacketHeader) + segment.size());

		memcpy(&packet[0], &pkt_header, sizeof(PacketHeader));
        memcpy(&packet[sizeof(PacketHeader)], &segment[0], segment.size());

		log("Sending packet to " + std::to_string(dest_ip) + " via " + std::to_string(neighbor_to_send));
        send_packet(neighbor_to_send, packet);
	}

    void receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t distance) override
    {
		log("Received packet from " + std::to_string(src_mac) + " with distance " + std::to_string(distance));
        PacketHeader pkt_header;
        memcpy(&pkt_header, &packet[0], sizeof(PacketHeader));

		if (pkt_header.is_arp) {
			log("Received distance vector from " + std::to_string(src_mac));
			std::vector<uint8_t> segment(packet.begin() + sizeof(PacketHeader), packet.end());		

			IPAddress neighbor_ip = *(IPAddress*)(&segment[0]);
			MACAddress neighbor_mac = *(MACAddress*)(&segment[sizeof(IPAddress)]);

			gateway[neighbor_ip] = neighbor_mac;
			neighbor_distances[neighbor_mac] = distance;
			distance_vector[neighbor_ip] = distance;

			for(auto it = segment.begin() + sizeof(IPAddress) + sizeof(MACAddress); it != segment.end(); it += sizeof(IPAddress) + sizeof(size_t)){
				IPAddress ip = *(IPAddress*)(&(*it));
				size_t dist = *(size_t*)(&(*(it + sizeof(IPAddress))));

				if(distance_vector.find(ip) == distance_vector.end() || distance_vector[ip] > dist + distance){
					distance_vector[ip] = dist + distance;
					gateway[ip] = neighbor_mac;
				}
			}

			return;
		}

        if (pkt_header.dest_ip != ip) {
			IPAddress dest_ip = pkt_header.dest_ip;

			if(distance_vector.find(dest_ip) == distance_vector.end()){
				// TODO: Add default gateway
				assert(false);
			}

			MACAddress neighbor_to_send = gateway[dest_ip];
			send_packet(neighbor_to_send, packet);
            return;
        }

		std::vector<uint8_t> segment(packet.begin() + sizeof(PacketHeader), packet.end());		
		receive_segment(pkt_header.source_ip, segment);
        log("Received segment:\t" + std::string(segment.begin(), segment.end()));
    }

	void do_periodic() override
	{
		log("Sending distance vector to neighbors");
		PacketHeader pkt_header (ip, 0, mac, true);
		size_t size = sizeof(IPAddress) + sizeof(size_t);
		std::vector<uint8_t> packet(sizeof(PacketHeader) + sizeof(IPAddress) + sizeof(MACAddress) + size * distance_vector.size());

		memcpy(&packet[0], &pkt_header, sizeof(PacketHeader));
		memcpy(&packet[sizeof(PacketHeader)], &ip, sizeof(IPAddress));
		memcpy(&packet[sizeof(PacketHeader) + sizeof(IPAddress)], &mac, sizeof(MACAddress));

		int counter = 0;
		for(auto it = distance_vector.begin(); it != distance_vector.end(); it++){
			memcpy(&packet[sizeof(PacketHeader) + sizeof(IPAddress) + sizeof(MACAddress) + size*counter], &it->first, sizeof(IPAddress));
			memcpy(&packet[sizeof(PacketHeader) + sizeof(IPAddress) + sizeof(MACAddress) + size*counter + sizeof(IPAddress)], &it->second, sizeof(size_t));
			counter++;
		}

		broadcast_packet(packet);

		return;
	}
};

#endif
