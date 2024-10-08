# Link Layer Simulator Framework for Custom Network Layer
## Build
```
make -j$(nproc)
```
This compiles an executable `bin/main`.

## Explanation
This program simulates a link layer network whose graph is specified in the given input file (see `example_spec.json` for reference on format). It spawns threads for sending and receiving at each node, while ensuring that all concurrency is abstracted away from node implementation logic.

## How to use
To implement a routing algorithm, create a new implementation in `node_impl` (see `node_impl/naive.h` for example). Add appropriate statements at places marked `XXX` in `src/json_impl.cc` to use it in the simulator.

Any implementation of the abstract class `Node` (in `src/node.h`) must implement the following methods:
 - `send_segment`
   - This method is used to send a (layer-4) segment from the node to the specified IP address.
   - Internally it must call the `send_packet` method to send any constructed (layer-3) packets over layer 2.
 - `receive_packet`
   - This method is invoked by the simulator for any (layer-3) packet that is received by this node.

For an example implementation see `node_impl/naive.h`.

The method `log` is provided for node-wise logging.

## Run
```
bin/main <json spec>
```
For example
```
bin/main ./example_spec.json
```
To enable logging for each node (which captures the outputs of `log` calls in the nodes), use `--log`
```
bin/main ./example_spec.json --log
```
