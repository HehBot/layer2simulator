#include "simulation.h"

#include <fstream>
#include <iostream>

extern "C" bool log_enabled;
extern "C" char const* logfile_prefix;
extern "C" char const* args[2];
extern "C" size_t delay_ms;

extern "C" void parse(int argc, char** argv);

int main(int argc, char** argv)
{
    parse(argc, argv);

    std::ifstream net_spec_file(args[0]);
    if (!net_spec_file.is_open()) {
        std::cerr << "Unable to open file '" << args[0] << "' for reading\n";
        return 1;
    }
    std::ifstream msg_file(args[1]);
    if (!msg_file.is_open()) {
        std::cerr << "Unable to open file '" << args[1] << "' for reading\n";
        return 1;
    }

    Simulation s(log_enabled, logfile_prefix, net_spec_file, delay_ms);
    s.run(msg_file);
}
