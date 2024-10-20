# Network Layer Routing

In this lab, we would be implementing Network Layer over the Link Layer. We have provided you with the simulator that simulates Link Layer. Your mission, should you choose to accept it, is to implement a **Routing Protocol** with following specifications:

1. Packets should be routed ONLY through shortest path possible
2. Protocol should be able to detect nodes that have gone down or have come up recently, and should find an alternate shortest path
3. Protocol should be able to converge fairly quickly
4. In case of shortest path not being found *yet*, protocol should fall back to broadcasting to make sure packet reaches destination

Any routing protocol which satisfies above specification can be implemented.

## Description of the Simulator

Before jumping into the actual problem statement, let's understand how the simulator that we have provided you works.

Simulator comes with the Link Layer already implemented. That means, it comes with the functionality of sending a packet from a node to its neighbor. But of course, being Link Layer, it cannot send packets over multiple hops. This is what you have to implement - Network Layer Routing.

---

**Following are the various function implementations provided to you. You can use them directly in your implementations:**

### `send_packet`
#### Declaration
`void Node::send_packet(MACAddress dest_mac, std::vector<uint8_t> const& packet) const`
#### Description
`send_packet` is used to send a packet to one of the neighbor nodes. You need to specify the exact neighbor node using `dest_mac`. `packet` vector is a vector of bytes that you want to send. Content of this `packet` can be anything, and it is upto you how you want to structure it.
> Note: `dest_mac` **must** be the MAC address of one of the neighbors

### `broadcast_packet`
#### Declaration
`void Node::broadcast_packet(std::vector<uint8_t> const& packet) const`
#### Declaration
`broadcast_packet` is used to send a packet to **all** neighbors of the current node. `packet` vector is a vector of bytes that you want to send. Content of this `packet` can be anything, and it is upto you how you want to structure it.

---

**Following are the functions that you need to implement:**

### `send_segment`
#### Declaration
`void send_segment(IPAddress dest_ip, std::vector<uint8_t> const& segment) const`
#### Description
`send_segment` is used to send `segment` to node having the `dest_ip`. `segment` vector is a vector of bytes consisting of message that needs to be sent. This function is called by the simulator with appropriate arguments at appropriate times. You need to be concerned abbout the invocation of this function.
> Note: Node having `dest_ip` may or may not be a neighbor of the current node

### `receive_packet`
#### Declaration
`void receive_packet(MACAddress src_mac, std::vector<uint8_t> packet, size_t distance)`
#### Description
`receive_packet` is called when node receives a packet from one of its neighbors. `src_mac` is the MAC address of the neighbor from whom node received the packet. `packet` vector is a vector of bytes containing the data of the packet. `distance` is the distance of the neighbor from the current node.
> Hint: Use `distance` argument for your implementation of Distance Vector Routing

### `do_periodic`
#### Declaration
`void do_periodic()`
#### Description
`do_periodic` is called periodically by the simulator on each of the nodes. Tasks done by this function should be the ones that you want to be done periodically on every node.
> Think what kind of tasks are periodic in routing protocols

---

- Network topology is specified using a JSON file. You can check out `example_spec.json` to understand its structure.

- Messages that need to be sent are also specified in a file. You can check out `msg_file` to understand its structure. It also contains the instructions to bring down or bring up a specified node (using `DOWN` and `UP`).

## More on protocol specifications

1. Protocol should route the packets through shortest possible path. You need to use the `distance` argument in `receive_packet(...)` function to determine this shortest path. Implementations that route packets through sub-optimal paths will receive a penalty.
2. Simulator will make some nodes go down (and then bring them up) arbitrarily. Your protocol should be able to detect that a node has gone down, and then find an alternate shortest path. Similary, if the node comes back up, your protocol should be able to detect that node has become active and if it provides a shorter path, your protocol should make changes accordingly.
3. In rare cases, if your protocol hasn't yet determined the shortest path (due to not having converged yet), your protocol should fall back to broadcasting the packets. Your protocol should NOT drop packets in any case. In these rare cases, packets may take sub-optimal paths - this is allowed.
4. Simulator runs `do_periodic` function for a short period of time before the actual transmission of messages start. This period is deliberately provided for your protocol to converge. Your protocol MUST converge in the given time constraints and thereafter use the shortes paths for routing.
5. After the simulator brings down (or up) a certain node, it again runs `do_periodic` function for short period. Only after this does the actual transmission starts. This time perios is again provided for your protocol to converge (as your protocol will need to find alternate path).

## Running Instructions

## Submission Instructions