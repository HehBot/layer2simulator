#ifndef DVR_H
#define DVR_H

#include "../node.h"

#include <vector>

class DVRNode : public Node {
private:
    /*
     * XXX
     * Add any fields, helper functions etc here
     */

    void send_packet_to(IPAddress dest_ip, std::vector<uint8_t> const& packet) const;

public:
    /*
     * NOTE You may not modify the constructor of this class
     */
    DVRNode(Simulation* simul, MACAddress mac, IPAddress ip) : Node(simul, mac, ip) { }

    void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const override;
    void receive_packet(MACAddress src_mac, std::vector<uint8_t> packet, size_t distance) override;
    void do_periodic() override;
};

#endif
