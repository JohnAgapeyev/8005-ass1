#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <omp.h>
#include <sys/wait.h>
#include <openssl/bn.h>
#include "main.h"

static const char *mixed_filename = ".input.txt";
static void init_mixed(long count);

/*
 * Calculates the nth root using Newton's method and the OpenSSL Bignum library
 */
BIGNUM *nth_root(BIGNUM *input, const unsigned long n) {
    int bit_count = BN_num_bits(input);

    int first_bit_index = -1;
    for (int i = bit_count; i > 0; --i) {
        if (BN_is_bit_set(input, i)) {
            first_bit_index = i;
            break;
        }
    }
    if (first_bit_index == -1) {
        return NULL;
    }

    BIGNUM *power_of_two = BN_new();
    BN_zero(power_of_two);
    BN_set_bit(power_of_two, first_bit_index);

    BN_CTX *ctx = BN_CTX_new();

    BIGNUM *x = BN_dup(power_of_two);
    BIGNUM *y = BN_new();

    BIGNUM *bn_n = BN_new();
    BN_set_word(bn_n, n);

    BIGNUM *n_minus_one = BN_new();
    BN_set_word(n_minus_one, n - 1);

    BIGNUM *temp_1 = BN_new();
    BIGNUM *temp_2 = BN_new();

    BIGNUM *rtn = BN_new();

    while (BN_cmp(x, BN_value_one()) == 1) {
        BN_exp(temp_1, x, n_minus_one, ctx);
        BN_div(temp_1, NULL, input, temp_1, ctx);

        BN_mul(temp_2, n_minus_one, x, ctx);

        BN_add(y, temp_1, temp_2);

        BN_div(y, NULL, y, bn_n, ctx);

        if (BN_cmp(y, x) != -1) {
            BN_copy(rtn, x);
            goto cleanup;
        }
        BN_copy(x, y);
    }
    BN_one(rtn);
cleanup:
    BN_free(power_of_two);
    BN_CTX_free(ctx);
    BN_free(x);
    BN_free(y);
    BN_free(bn_n);
    BN_free(n_minus_one);
    BN_free(temp_1);
    BN_free(temp_2);
    return rtn;
}

int main(int argc, char **argv) {
    long worker_count = 8;
    enum worker_type type = 0;
    int c;
    for (;;) {
        int option_index = 0;
        if ((c = getopt_long(argc, argv, "w:htpoa", long_options, &option_index)) == -1) {
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
            case 'a':
                type = ALL;
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
    init_mixed(worker_count);
    switch(type) {
        case ALL:
            thread_work(worker_count);
            process_work(worker_count);
            openmp_work(worker_count);
            break;
        case THREADS:
            thread_work(worker_count);
            break;
        case PROCESSES:
            process_work(worker_count);
            break;
        case OPENMP:
            openmp_work(worker_count);
            break;
        default:
            print_help();
            unlink(mixed_filename);
            return EXIT_SUCCESS;
    }
    unlink(mixed_filename);
    return EXIT_SUCCESS;
}

void thread_work(const long count) {
    printf("Thread start\n");
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
        pthread_join(threads[i], NULL);
    }
    printf("Thread end\n");
}

void process_work(const long count) {
    printf("Process start\n");
    signal(SIGQUIT, SIG_IGN);
    for (long i = 0; i < count; ++i) {
        switch (fork()) {
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
    //Spin until all children are gone
    while(wait(NULL) != -1);
    printf("Process end\n");
}

void openmp_work(const long count) {
    printf("OpenMP start\n");
    omp_set_dynamic(0);
#pragma omp parallel for schedule(static, 1) num_threads(count)
    for (long i = 0; i < count; ++i) {
        do_work(NULL);
    }
    printf("OpenMP end\n");
}

void *do_work(void *arg) {
    (void)arg;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    do_cpu_work();
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + ((end.tv_nsec - start.tv_nsec) / 1000);
    printf("CPU: %lu\n", delta_us);

#if 1
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    do_io_work();
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("I/O %lu\n", delta_us);
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    do_mixed_work();
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("Mixed: %lu\n", delta_us);
#endif
    return NULL;
}

void do_cpu_work(void) {
    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();
    BIGNUM *d = BN_new();
    BIGNUM *n = BN_new();

    BIGNUM *p = BN_new();
    BIGNUM *q = BN_new();
    BN_generate_prime_ex(p, 32, 1, NULL, NULL, NULL);
    BN_generate_prime_ex(q, 32, 1, NULL, NULL, NULL);
    BN_CTX *ctx = BN_CTX_new();

    BN_mul(n, p, q, ctx);

    BN_one(d);
    BN_set_word(x, 2);
    BN_set_word(y, 2);

    BIGNUM *zero = BN_new();
    BN_zero(zero);

    BIGNUM *neg_one = BN_new();
    BN_zero(neg_one);
    BN_sub_word(neg_one, 1);

    BIGNUM *diff = BN_new();

    while(BN_cmp(d, BN_value_one()) == 0) {
        //x = x^2 + 1 mod n
        BN_sqr(x, x, ctx);
        BN_mod_add(x, x, BN_value_one(), n, ctx);

        //y = g(g(y))
        BN_sqr(y, y, ctx);
        BN_mod_add(y, y, BN_value_one(), n, ctx);
        BN_sqr(y, y, ctx);
        BN_mod_add(y, y, BN_value_one(), n, ctx);

        BN_sub(diff, x, y);
        if (BN_cmp(diff, zero) < 0) {
            BN_mul(diff, diff, neg_one, ctx);
        }
        BN_gcd(d, diff, n, ctx);
    }
    BN_free(x);
    BN_free(y);
    BN_free(d);
    BN_free(n);
    BN_free(p);
    BN_free(q);
    BN_free(zero);
    BN_free(neg_one);
    BN_free(diff);
    BN_CTX_free(ctx);
}

void do_io_work(void) {
    const char *filename = ".tmp.txt";
    int file = open(filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    long file_size = 1 << 20;
    long read_count = 4096;
    //Set file size to 1 MB
    ftruncate(file, file_size);

    char buffer[read_count];
    for (long i = 0; i < file_size; ++i) {
        lseek(file, rand() % (file_size - read_count), SEEK_SET);
        read(file, buffer, read_count);
        lseek(file, rand() % (file_size - read_count), SEEK_SET);
        write(file, buffer, read_count);
    }
    close(file);
    unlink(filename);
}

void do_mixed_work(void) {
    int file = open(mixed_filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    FILE *file_p = fdopen(file, "rwb");

    BIGNUM *n = NULL;
    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();
    BIGNUM *d = BN_new();
    BIGNUM *diff = BN_new();

    BIGNUM *zero = BN_new();
    BN_zero(zero);

    BIGNUM *neg_one = BN_new();
    BN_zero(neg_one);
    BN_sub_word(neg_one, 1);

    BN_CTX *ctx = BN_CTX_new();

    char input[1024];
    memset(input, 0, 1024);
    while(fgets(input, 1023, file_p)) {
        input[strlen(input) - 1] = '\0';
        BN_hex2bn(&n, input);

        BN_one(d);
        BN_set_word(x, 2);
        BN_set_word(y, 2);

        while(BN_cmp(d, BN_value_one()) == 0) {
            //x = x^2 + 1 mod n
            BN_sqr(x, x, ctx);
            BN_mod_add(x, x, BN_value_one(), n, ctx);

            //y = g(g(y))
            BN_sqr(y, y, ctx);
            BN_mod_add(y, y, BN_value_one(), n, ctx);
            BN_sqr(y, y, ctx);
            BN_mod_add(y, y, BN_value_one(), n, ctx);

            BN_sub(diff, x, y);
            if (BN_cmp(diff, zero) < 0) {
                BN_mul(diff, diff, neg_one, ctx);
            }
            BN_gcd(d, diff, n, ctx);
        }
    }
    BN_free(x);
    BN_free(y);
    BN_free(d);
    BN_free(n);
    BN_free(zero);
    BN_free(neg_one);
    BN_free(diff);

    BN_CTX_free(ctx);

    close(file);
}

static void init_mixed(long count) {
    BIGNUM *p = BN_new();
    BIGNUM *q = BN_new();
    BIGNUM *n = BN_new();
    BN_CTX *ctx = BN_CTX_new();

    int file = open(mixed_filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    for (long i = 0; i < 100 * count; ++i) {
        BN_generate_prime_ex(p, 24, 1, NULL, NULL, NULL);
        BN_generate_prime_ex(q, 24, 1, NULL, NULL, NULL);
        BN_mul(n, p, q, ctx);

        char *n_hex = BN_bn2hex(n);
        write(file, n_hex, strlen(n_hex));
        write(file, "\n", 1);

        free(n_hex);
    }

    close(file);
}
