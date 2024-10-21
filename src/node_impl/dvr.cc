#include "dvr.h"

#include <cassert>

void DVRNode::send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const
{
    /*
     * XXX
     * Implement this function
     */
    assert(false && "Unimplemented");
}

void DVRNode::receive_packet(MACAddress src_mac, std::vector<uint8_t> packet, size_t distance)
{
    /*
     * XXX
     * Implement this function
     */
    assert(false && "Unimplemented");
}

void DVRNode::do_periodic()
{
    /*
     * XXX
     * Implement this function
     */
    assert(false && "Unimplemented");
}
