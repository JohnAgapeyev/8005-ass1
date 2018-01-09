#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <omp.h>
#include "main.h"

static sem_t *semaphore;
static const char *shmem_name = "/8005";

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
    semaphore = malloc(sizeof(sem_t));
    if (semaphore == NULL) {
        abort();
    }
    switch(type) {
        case THREADS:
            sem_init(semaphore, 0, worker_count);
            thread_work(worker_count);
            sem_destroy(semaphore);
            free(semaphore);
            break;
        case PROCESSES:
            {
                int shm;
                if ((shm = shm_open(shmem_name, O_RDWR | O_CREAT, S_IRWXU)) == -1) {
                    perror("shm_open");
                    exit(EXIT_FAILURE);
                }
                if (ftruncate(shm, sizeof(sem_t)) < 0 ) {
                    shm_unlink(shmem_name);
                    perror("ftruncate");
                    exit(EXIT_FAILURE);
                }
                if ((semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0)) == MAP_FAILED) {
                    shm_unlink(shmem_name);
                    perror("mmap");
                    exit(EXIT_FAILURE);
                }
                sem_init(semaphore, 1, worker_count);
                process_work(worker_count);
                if (munmap(semaphore, sizeof(sem_t)) == -1) {
                    shm_unlink(shmem_name);
                    perror("munmap");
                    exit(EXIT_FAILURE);
                }
                sem_destroy(semaphore);
                shm_unlink(shmem_name);
            }
            break;
        case OPENMP:
            sem_init(semaphore, 0, worker_count);
            openmp_work(worker_count);
            sem_destroy(semaphore);
            free(semaphore);
            break;
        default:
            print_help();
            free(semaphore);
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
    for (unsigned long i = 0; i < 1000; ++i) {
        sched_yield();
    }
    for (long i = 0; i < count; ++i) {
        sem_wait(semaphore);
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
                exit(EXIT_SUCCESS);
            case -1:
                kill(0, SIGQUIT);
                break;
            default:
                //Parent process
                break;
        }
    }
    for (unsigned long i = 0; i < 1000; ++i) {
        sched_yield();
    }
    for (long i = 0; i < count; ++i) {
        sem_wait(semaphore);
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
    sem_wait(semaphore);
    //Do slow stuff here
    sleep(4);
    sem_post(semaphore);
    return NULL;
}
