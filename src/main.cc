#include "simulation.h"

#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <json spec>\n";
        return 1;
    }
    char const* json_spec_name = argv[1];
    std::ifstream i = std::ifstream(json_spec_name);
    if (!i.is_open()) {
        std::cerr << "Unable to open file '" << json_spec_name << "' for reading\n";
        return 1;
    }

    Simulation s(i);
    s.run();
}
