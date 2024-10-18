#include "simulation.h"

#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    if ((argc < 3)
        || (argc >= 4 && std::string(argv[3]) != "--log")
        || (argc > 5)) {
        std::cerr << "Usage: " << argv[0] << " <json_spec> <msg_file> [--log [logfile_prefix]]\n";
        return 1;
    }

    char const* net_spec_name = argv[1];
    std::ifstream net_spec(net_spec_name);
    if (!net_spec.is_open()) {
        std::cerr << "Unable to open file '" << net_spec_name << "' for reading\n";
        return 1;
    }
    char const* msg_file_name = argv[2];
    std::ifstream msg_file(msg_file_name);

    bool log_enabled = (argc >= 4);
    char const* logfile_prefix = (argc == 5 ? argv[4] : "node-");

    Simulation s(log_enabled, logfile_prefix, net_spec);
    s.run(msg_file);
}
