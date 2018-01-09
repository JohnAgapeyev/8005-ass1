#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <omp.h>
#include "main.h"

static sem_t semaphore;

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
            sem_init(&semaphore, 0, worker_count);
            thread_work(worker_count);
            sem_destroy(&semaphore);
            break;
        case PROCESSES:
            sem_init(&semaphore, 0, worker_count);
            process_work(worker_count);
            sem_destroy(&semaphore);
            break;
        case OPENMP:
            sem_init(&semaphore, 0, worker_count);
            openmp_work(worker_count);
            sem_destroy(&semaphore);
            break;
        default:
            print_help();
            return EXIT_SUCCESS;
    }
    return EXIT_SUCCESS;
}

void thread_work(const long count) {
    pthread_t threads[count];
    for (long i = 0; i < count; ++i) {
        if (pthread_create(threads + i, NULL, do_work, NULL) != 0) {
            for (long j = 0; j < i; ++j) {
                pthread_kill(threads[j], SIGQUIT);
            }
            break;
        }
    }
    for (long i = 0; i < count; ++i) {
        sem_wait(&semaphore);
    }
}

void process_work(const long count) {
    pid_t child_pid;
    signal(SIGQUIT, SIG_IGN);
    for (long i = 0; i < count; ++i) {
        switch ((child_pid = fork())) {
            case 0:
                //Child process
                do_work(NULL);
                return;
            case -1:
                kill(-getpid(), SIGQUIT);
                break;
            default:
                //Parent process
                break;
        }
    }
    for (long i = 0; i < count; ++i) {
        sem_wait(&semaphore);
    }
}

void openmp_work(const long count) {
    omp_set_dynamic(0);
#pragma omp parallel for schedule(static, 1) num_threads(count)
    for (long i = 0; i < count; ++i) {
        do_work(NULL);
    }
}

void *do_work(void *arg) {
    sem_wait(&semaphore);
    //Do slow stuff here
    sem_post(&semaphore);
    return NULL;
}
