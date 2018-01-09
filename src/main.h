#ifndef MAIN_H
#define MAIN_H

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

void thread_work(const long count);
void process_work(const long count);
void openmp_work(const long count);

void *do_work(void *arg);

#endif
