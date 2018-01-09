#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

static struct option long_options[] = {
    {"workers",    required_argument, 0, 'w'},
    {"help",       no_argument,       0, 'h'},
    {"thread",     no_argument,       0, 't'},
    {"process",    no_argument,       0, 'p'},
    {"openmp",     no_argument,       0, 'o'},
    {0,            0,                 0,  0}
};

#define print_help() \
    do { \
        printf("usage options:\n"\
                "\t [w]orkers <5-256>       - the number of workers to use, default: 8\n"\
                "\t [t]hread                - Use threads as worker type\n"\
                "\t [p]rocess               - Use processes as worker type\n"\
                "\t [o]penmp                - Use OpenMP as worker type\n"\
                "\t [h]elp                  - this message\n"\
                );\
    } while(0)

enum worker_type {
    THREADS = 1,
    PROCESSES = 2,
    OPENMP = 3
};

int main(int argc, char **argv) {
    long worker_count = 8;
    enum worker_type type = 0;
    int c;
    for (;;) {
        int option_index = 0;
        if ((c = getopt_long(argc, argv, "w:htpo", long_options, &option_index)) == -1) {
            break;
        }
        switch (c) {
            case 'w':
                worker_count = strtol(optarg, NULL, 10);
                if (worker_count < 5 || worker_count > 256) {
                    printf("Worker count must be bewteen 5 and 256\n");
                    print_help();
                }
                break;
            case 't':
                type = THREADS;
                break;
            case 'p':
                type = PROCESSES;
                break;
            case 'o':
                type = OPENMP;
                break;
            case 'h':
                //Intentional fallthrough
            case '?':
                //Intentional fallthrough
            default:
                print_help();
                return EXIT_SUCCESS;
        }
    }
    switch(type) {
        case THREADS:
            break;
        case PROCESSES:
            break;
        case OPENMP:
            break;
        default:
            print_help();
            return EXIT_SUCCESS;
    }
}
