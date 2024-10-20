#include <argp.h>
#include <stdbool.h>
#include <stdlib.h>

static char const* doc = "layer2simulator - A Layer 2 Network Simulator";
static char const* args_doc = "NETWORK_SPEC_FILE MSG_FILE";
static struct argp_option options[] = {
    { "log", 'l', "LOGFILE_PREFIX", OPTION_ARG_OPTIONAL, "Emit nodewise logs" },
    { "delay", 'd', "DELAY", 0, "Add delay (in ms)" },
    { 0 }
};

bool log_enabled = false;
char const* logfile_prefix = "node-";
char const* args[2] = { 0 };
size_t delay_ms = 10;

static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
    switch (key) {
    case 'l':
        log_enabled = true;
        if (arg != NULL)
            logfile_prefix = arg;
        break;
    case 'd':
        delay_ms = atoi(arg);
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num >= 2)
            argp_usage(state);
        args[state->arg_num] = arg;
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 1)
            argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

void parse(int argc, char** argv)
{
    struct argp a = { options, parse_opt, args_doc, doc };
    argp_parse(&a, argc, argv, 0, 0, NULL);
}