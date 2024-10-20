#include "naive.h"

#include <cstring>

/*
 * DON'T DO THIS
 * This is just illustrative of how to extend the Node class
 * This assumes (wrongly) that
 *  - the link layer is a clique and
 *  - that IP addresses are direct-mapped to MAC addresses as
 *          IP = MAC * 1000
 */

struct PacketHeader {
private:
    PacketHeader() = default;

public:
    bool is_broadcast;
    IPAddress src_ip;
    IPAddress dest_ip;

    PacketHeader(IPAddress src_ip)
        : is_broadcast(true), src_ip(src_ip), dest_ip(0) { }
    PacketHeader(IPAddress src_ip, IPAddress dest_ip)
        : is_broadcast(false), src_ip(src_ip), dest_ip(dest_ip) { }
    static PacketHeader from_bytes(uint8_t const* bytes)
    {
        PacketHeader ph;
        memcpy(&ph, bytes, sizeof(ph));
        return ph;
    }
};

void NaiveNode::send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const
{
    MACAddress dest_mac = dest_ip / 1000;

    auto ph = PacketHeader(ip, dest_ip);

    std::vector<uint8_t> packet(sizeof(ph) + segment.size());

    memcpy(&packet[0], &ph, sizeof(ph));
    memcpy(&packet[sizeof(ph)], &segment[0], segment.size());

    send_packet(dest_mac, packet);
}
void NaiveNode::receive_packet(MACAddress src_mac, std::vector<uint8_t> const& packet, size_t distance)
{
    PacketHeader ph = PacketHeader::from_bytes(&packet[0]);

    if (ph.is_broadcast) {
        log("Received broadcast from " + std::to_string(ph.src_ip));
        return;
    } else if (ph.dest_ip != ip) {
        log("Packet delivered to wrong node, intended for ip " + std::to_string(ph.dest_ip));
        return;
    }

    std::vector<uint8_t> segment(packet.begin() + sizeof(ph), packet.end());
    receive_segment(ph.src_ip, segment);
}
void NaiveNode::do_periodic()
{
    log("Broadcasting");
    std::string s = "BROADCAST FROM " + std::to_string(ip);
    PacketHeader ph(ip, 0);
    std::vector<uint8_t> packet(sizeof(ph) + s.length());
    memcpy(&packet[0], &ph, sizeof(ph));
    memcpy(&packet[sizeof(ph)], &s[0], s.length());
    broadcast_packet(packet);
}