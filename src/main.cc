#include "simulation.h"

#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    if ((argc != 2 && argc != 3)
        || (argc == 3 && std::string(argv[2]) != "--log")) {
        std::cerr << "Usage: " << argv[0] << " <json spec> [--log]\n";
        return 1;
    }
    bool log_enabled = (argc == 3);
    char const* json_spec_name = argv[1];
    std::ifstream i = std::ifstream(json_spec_name);
    if (!i.is_open()) {
        std::cerr << "Unable to open file '" << json_spec_name << "' for reading\n";
        return 1;
    }

    Simulation s(log_enabled, i);
    s.run();
}
