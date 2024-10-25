#include "rp.h"

#include <cstring>
#include <vector>

#define MAX_TTL 15

struct RPPacketHeader {
private:
    RPPacketHeader() = default;

public:
    static RPPacketHeader payload_header(IPAddress src_ip, IPAddress dest_ip)
    {
        return RPPacketHeader { false, src_ip, dest_ip, MAX_TTL };
    }
    static RPPacketHeader dv_header()
    {
        return RPPacketHeader { true, 0, 0, 0 };
    }
    static RPPacketHeader from_bytes(uint8_t const* bytes)
    {
        RPPacketHeader ph;
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

/*
 * helper function that performs gateway lookup and sends packet appropriately
 */
void RPNode::send_packet_to(IPAddress dest_ip, std::vector<uint8_t> const& packet) const
{
    auto dv_entry = dv_table.find(dest_ip);
    if (dv_entry == dv_table.end() || dv_entry->second.validity == 0) {
        /*
         * if we do not have a route to `dest_ip` we broadcast it
         */
        broadcast_packet_to_all_neighbors(packet);
    } else {
        /*
         * if we have a route to `dest_ip` we send it along that route
         */
        MACAddress neighbor_to_send = dv_entry->second.gateway;
        send_packet(neighbor_to_send, packet);
    }
}

void RPNode::send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const
{
    RPPacketHeader pkt_header = RPPacketHeader::payload_header(ip, dest_ip);
    std::vector<uint8_t> packet(sizeof(RPPacketHeader) + segment.size());
    memcpy(&packet[0], &pkt_header, sizeof(RPPacketHeader));
    memcpy(&packet[sizeof(RPPacketHeader)], &segment[0], segment.size());

    send_packet_to(dest_ip, packet);
}

void RPNode::receive_packet(MACAddress src_mac, std::vector<uint8_t> packet, size_t distance)
{
    RPPacketHeader pkt_header = RPPacketHeader::from_bytes(&packet[0]);

    if (pkt_header.is_dv_table) {
        /*
         * the packet is a DV table sent by a neighbor
         */
        MACAddress neighbor_mac = src_mac;

        for (DVEntry const* ptr = (DVEntry*)&packet[sizeof(RPPacketHeader)]; ptr < (DVEntry*)&packet[packet.size()]; ptr++) {
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
            std::vector<uint8_t> segment(packet.begin() + sizeof(RPPacketHeader), packet.end());
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
            memcpy(&packet[0], &pkt_header, sizeof(RPPacketHeader));

            send_packet_to(dest_ip, packet);
        }
    }
}

void RPNode::do_periodic()
{
    /*
     * decrement validity of `dv_table` entries
     */
    std::vector<IPAddress> to_delete_from_distance_vector;
    for (auto& dv_entry : dv_table) {
        if (dv_entry.second.validity == 0)
            to_delete_from_distance_vector.push_back(dv_entry.first);
        /*
         * don't decrement validity of self entry
         */
        if (dv_entry.first != ip)
            dv_entry.second.validity--;
    }

    /*
     * delete invalidated entries
     */
    for (auto ip : to_delete_from_distance_vector)
        dv_table.erase(ip);

    /*
     * broadcast our DV table to neighbours
     */
    std::vector<uint8_t> packet(sizeof(RPPacketHeader) + sizeof(DVEntry) * dv_table.size());
    RPPacketHeader pkt_header = RPPacketHeader::dv_header();
    memcpy(&packet[0], &pkt_header, sizeof(RPPacketHeader));
    size_t off = sizeof(RPPacketHeader);
    for (auto dv_entry : dv_table) {
        DVEntry dve = { dv_entry.first, dv_entry.second.gateway, dv_entry.second.distance, dv_entry.second.validity };
        memcpy(&packet[off], &dve, sizeof(dve));
        off += sizeof(dve);
    }
    broadcast_packet_to_all_neighbors(packet);
}
