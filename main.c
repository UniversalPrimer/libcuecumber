#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // uint8_t, uint16_t, etc.
#include <string.h> // strlen
#include <netinet/in.h> // hton, ntoh
#include <argp.h>

#include "cuecumber.h"

/* Program documentation. */
static char doc[] =
    "Cuecumber";
     
/* A description of the arguments we accept. */
static char args_doc[] = "Filename";

static struct argp_option options[] = {
    {"verbose",  'v', 0,      0,  "Produce verbose output" },
    {"quiet",    'q', 0,      0,  "Don't produce any output" },
    {"silent",   's', 0,      OPTION_ALIAS },
    {"input file", 'i', "FILE", 0, "FLV file to read"},
    {"write",     'w', 0, 0, "Write cuepoints to file"},
    {"output",   'o', "FILE", 0,
     "Output to FILE instead of standard output" },
    { 0 }
};

struct arguments
{
    int silent, verbose, write;
    char *output_file;
    FILE* in_file;
};


static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;
  
    switch (key)
    {
        case 'q': case 's':
            arguments->silent = 1;
            break;
        case 'v':
            arguments->verbose = 1;
            break;
        case 'o':
            arguments->output_file = arg;
            break;
        case 'w':
            arguments->write = 1;
            break;
        case 'i':
            arguments->in_file = fopen(arg, "rb+");
            break;
     
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

/* This function reads a test cuepoint from a file and injects it into
 * the stream. Created for testing purposes. */
void cuepoint_test() {
    FILE* cuepoint_file = fopen("cuepoint_gen.raw", "r");

    fseek(cuepoint_file, 0, SEEK_END);
    int cuepoint_size = ftell(cuepoint_file);
    void* cuepoint = malloc(cuepoint_size);
    rewind(cuepoint_file);

    cuecumber_init();
    
    sleep(120);
    cuecumber_stop();
    cuecumber_exit();
    printf("\nDone\n");
}

int main(int argc, char** argv) {  
    struct arguments arguments;

    arguments.silent = 0;
    arguments.verbose = 0;
    arguments.output_file = "-";
    arguments.write = 0;
  
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    cuepoint_test();
  
    return 0;
}
