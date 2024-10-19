# DVR

In this lab, we would be implementing Network Layer over the Link Layer. We have provided you with the simulator that simulates Link Layer. Your mission, should you choose to accept it, is to implement **Distance Vector Routing**.

## Description of the Simulator

Before jumping into the actual problem statement, let's understand how the simulator that we have provided you works.

Simulator comes with the Link Layer already implemented. That means, it comes with the functionality of sending a packet from a node to its neighbor. But of course, being Link Layer, it cannot send packets over multiple hops. This is what you have to implement - Network Layer Routing.

Following are the various function implementations provided to you. You can use them directly in your implementations:

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

Following are the functions that you need to implement:

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
`do_periodic` is called periodically by the simulator on each of the nodes. Tasks done by this function should be the ones that you to be done periodically on every node.
> Think what kind of tasks are periodic in Distance Vector Routing

Network topology is specified using a JSON file. You can check out `example_spec.json` to understand its structure.

Messages that need to be sent are also specified in a file. You can check out `msg_file` to understand its structure. It also contains the instructions to bring down or bring up a specified node (using `DOWN` and `UP`).

## Your Task
You need to implement Distance Vector Routing. If you are unfamiliar with this routing algorithm, you can check its details [here](https://en.wikipedia.org/wiki/Distance-vector_routing_protocol#Count_to_infinity_problem).

In this algorithm, there is a possibility of a particular entry not being present because the algorithm has not converged yet. You can safely ignore this case as our simulator provides enough time for the algorithm to converge before it starts sending actual messages.

We will also bring down and then bring up certain nodes to test your implementation. Your algorithm must be able to detect that a node has gone down (how?) and must figure out an alternate path bypassing the node that has gone down. You need not worry about certain node being totally inaccessible. We will ensure that the graph remains connected at all times.