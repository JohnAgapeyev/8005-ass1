#ifndef MAIN_H
#define MAIN_H

/*
 * HEADER FILE: main.h - Main application header
 *
 * PROGRAM: 8005-ass1
 *
 * DATE: Jan. 20, 2018
 *
 * FUNCTIONS:
 * void thread_work(const long count);
 * void process_work(const long count);
 * void openmp_work(const long count);
 * void *do_work(void *arg);
 * void do_cpu_work(void);
 * void do_io_work(void);
 * void do_mixed_work(void);
 *
 * DESIGNER: John Agapeyev
 *
 * PROGRAMMER: John Agapeyev
 */

static struct option long_options[] = {
    {"workers",    required_argument, 0, 'w'},
    {"help",       no_argument,       0, 'h'},
    {"thread",     no_argument,       0, 't'},
    {"process",    no_argument,       0, 'p'},
    {"openmp",     no_argument,       0, 'o'},
    {"all",        no_argument,       0, 'a'},
    {0,            0,                 0,  0}
};

#define print_help() \
    do { \
        printf("usage options:\n"\
                "\t [w]orkers <5-256>       - the number of workers to use, default: 8\n"\
                "\t [t]hread                - Use threads as worker type\n"\
                "\t [p]rocess               - Use processes as worker type\n"\
                "\t [o]penmp                - Use OpenMP as worker type\n"\
                "\t [a]all                  - Test all worker types\n"\
                "\t [h]elp                  - this message\n"\
                );\
    } while(0)

enum worker_type {
    THREADS = 1,
    PROCESSES = 2,
    OPENMP = 3,
    ALL = 4
};

void thread_work(const long count);
void process_work(const long count);
void openmp_work(const long count);

void *do_work(void *arg);
void do_cpu_work(void);
void do_io_work(void);
void do_mixed_work(void);

#endif
