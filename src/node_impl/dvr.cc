#include "dvr.h"

#include <cstring>
#include <vector>

#define INITIAL_VALIDITY 100
#define MAX_TTL 4

struct DVRPacketHeader {
private:
    DVRPacketHeader() = default;

public:
    static DVRPacketHeader payload_header(IPAddress src_ip, IPAddress dest_ip)
    {
        return DVRPacketHeader { false, src_ip, dest_ip, MAX_TTL };
    }
    static DVRPacketHeader dv_header(IPAddress my_ip)
    {
        return DVRPacketHeader { true, my_ip, 0, 0 };
    }
    static DVRPacketHeader from_bytes(uint8_t const* bytes)
    {
        DVRPacketHeader ph;
        memcpy(&ph, bytes, sizeof(ph));
        return ph;
    }

    bool is_dv_table;
    IPAddress src_ip;
    IPAddress dest_ip;
    size_t ttl;
};

struct DVEntry {
    IPAddress ip;
    MACAddress gateway;
    size_t distance;
    size_t validity;
};

// helper function that performs gateway lookup and sends packet appropriately
void DVRNode::send_packet_to(IPAddress dest_ip, std::vector<uint8_t> const& packet) const
{
    auto dv_entry = dv_table.find(dest_ip);
    if (dv_entry == dv_table.end() || dv_entry->second.validity == 0) {
        /*
         * if we do not have a route to `dest_ip` we broadcast it
         */
        broadcast_packet(packet);
    } else {
        /*
         * if we have a route to `dest_ip` we send it along that route
         */
        MACAddress neighbor_to_send = dv_entry->second.gateway;
        send_packet(neighbor_to_send, packet);
    }
}

void DVRNode::send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const
{
    DVRPacketHeader pkt_header = DVRPacketHeader::payload_header(ip, dest_ip);
    std::vector<uint8_t> packet(sizeof(DVRPacketHeader) + segment.size());
    memcpy(&packet[0], &pkt_header, sizeof(DVRPacketHeader));
    memcpy(&packet[sizeof(DVRPacketHeader)], &segment[0], segment.size());

    send_packet_to(dest_ip, packet);
}

void DVRNode::receive_packet(MACAddress src_mac, std::vector<uint8_t> packet, size_t distance)
{
    DVRPacketHeader pkt_header = DVRPacketHeader::from_bytes(&packet[0]);

    if (pkt_header.is_dv_table) {
        /*
         * the packet is a DV table sent by a neighbor
         */
        IPAddress neighbor_ip = pkt_header.src_ip;
        MACAddress neighbor_mac = src_mac;

        dv_table[neighbor_ip] = RoutingInfo { neighbor_mac, distance, INITIAL_VALIDITY };
        neighbor_ips[neighbor_mac] = neighbor_ip;

        for (DVEntry const* ptr = (DVEntry*)&packet[sizeof(DVRPacketHeader)]; ptr < (DVEntry*)&packet[packet.size()]; ptr++) {
            DVEntry const& neighbor_dve = *ptr;
            IPAddress dest_ip = neighbor_dve.ip;

            auto dv_entry = dv_table.find(dest_ip);
            if (neighbor_dve.gateway == mac) {
                /*
                 * disregard proposed routes that go through us (split horizon)
                 */
                if (dv_entry != dv_table.end() && dv_entry->second.gateway == neighbor_mac) {
                    /*
                     * also invalidate any resulting loop routes
                     */
                    dv_entry->second.validity = 0;
                }
                continue;
            }
            if (dv_entry == dv_table.end()
                || dv_entry->second.distance > neighbor_dve.distance + distance
                || (dv_entry->second.distance == neighbor_dve.distance + distance
                    && dv_entry->second.gateway != neighbor_mac
                    && dv_entry->second.validity < neighbor_dve.validity)) {
                /*
                 * switch to proposed_route if
                 *  - we don't have a route to `dest_ip`
                 *  - our route to `dest_ip` is longer than the proposed route
                 *  - our route is different from the proposed route
                 *      but has the same distance and lower validity
                 */
                dv_table[dest_ip] = RoutingInfo {
                    neighbor_mac,
                    neighbor_dve.distance + distance,
                    neighbor_dve.validity
                };
            } else if (dv_entry->second.gateway == neighbor_mac) {
                /*
                 * if our route is the same as the proposed route then update validity as received
                 */
                dv_entry->second.validity = neighbor_dve.validity;
            }
        }
    } else {
        /*
         * the packet contains a segment payload
         */
        if (pkt_header.dest_ip == ip) {
            /*
             * the segment was intended for us
             */
            std::vector<uint8_t> segment(packet.begin() + sizeof(DVRPacketHeader), packet.end());
            receive_segment(pkt_header.src_ip, segment);

        } else {
            /*
             * the segment was not intended for us, we need to forward it appropriately
             */
            if (pkt_header.ttl == 0) {
                /*
                 * packet TTL expired, dropped
                 */
                return;
            }
            IPAddress dest_ip = pkt_header.dest_ip;
            pkt_header.ttl--;
            memcpy(&packet[0], &pkt_header, sizeof(DVRPacketHeader));

            send_packet_to(dest_ip, packet);
        }
    }
}

void DVRNode::do_periodic()
{
    /*
     * decrement validity of `dv_table` entries
     */
    std::vector<IPAddress> to_delete_from_distance_vector;
    for (auto& dv_entry : dv_table) {
        if (dv_entry.second.validity == 0)
            to_delete_from_distance_vector.push_back(dv_entry.first);
        dv_entry.second.validity--;
    }

    /*
     * delete invalidated entries
     */
    for (auto ip : to_delete_from_distance_vector) {
        MACAddress gateway = dv_table[ip].gateway;

        /*
         * if an invalidated entry corresponds to that of a neighbour
         * it must mean that that neighbour is down
         */
        auto neighbor_ips_entry = neighbor_ips.find(gateway);
        if (neighbor_ips_entry != neighbor_ips.end() && neighbor_ips_entry->second == ip)
            neighbor_ips.erase(neighbor_ips_entry);

        dv_table.erase(ip);
    }

    /*
     * broadcast our DV table to neighbours
     */
    std::vector<uint8_t> packet(sizeof(DVRPacketHeader) + sizeof(DVEntry) * dv_table.size());
    DVRPacketHeader pkt_header = DVRPacketHeader::dv_header(ip);
    memcpy(&packet[0], &pkt_header, sizeof(DVRPacketHeader));
    size_t off = sizeof(DVRPacketHeader);
    for (auto dv_entry : dv_table) {
        DVEntry dve = { dv_entry.first, dv_entry.second.gateway, dv_entry.second.distance, dv_entry.second.validity };
        memcpy(&packet[off], &dve, sizeof(dve));
        off += sizeof(dve);
    }
    broadcast_packet(packet);
}
