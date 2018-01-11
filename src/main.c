#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <omp.h>
#include <sys/wait.h>
#include <openssl/bn.h>
#include "main.h"

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
    switch(type) {
        case ALL:
            thread_work(worker_count);
            process_work(worker_count);
            openmp_work(worker_count);
            break;
        case THREADS:
            //thread_work(worker_count);
            thread_work(1);
            break;
        case PROCESSES:
            process_work(worker_count);
            break;
        case OPENMP:
            openmp_work(worker_count);
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
        pthread_join(threads[i], NULL);
    }
}

void process_work(const long count) {
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
}

void openmp_work(const long count) {
    omp_set_dynamic(0);
#pragma omp parallel for schedule(static, 1) num_threads(count)
    for (long i = 0; i < count; ++i) {
        do_work(NULL);
    }
}

void *do_work(void *arg) {
#if 0
    BIGNUM *expo = BN_new();
    BN_set_word(expo, 65537);
    //BN_one(expo);
    BIGNUM *base = BN_new();
    BN_rand(base, 1024, BN_RAND_TOP_TWO, BN_RAND_BOTTOM_ANY);

    printf("Base and expo set\n");

    BIGNUM *p = BN_new();
    BIGNUM *q = BN_new();
    BN_generate_prime_ex(p, 1024, 1, NULL, NULL, NULL);
    printf("P set\n");
    BN_generate_prime_ex(q, 2048, 1, NULL, NULL, NULL);
    printf("Q set\n");

    BN_CTX *ctx = BN_CTX_new();

    BIGNUM *modulus = BN_new();
    BN_mul(modulus, p, q, ctx);
    printf("Modulus done\n");

    for (unsigned long i = 0; i < 100; ++i) {
        BN_mod_exp(base, base, expo, modulus, ctx);
        printf("%lu modexp done\n", i);
    }
#else
#if 0
    BIGNUM *p = BN_new();
    for (int i = 3; i <= 10; ++i) {
        BN_generate_prime_ex(p, 1 << i, 1, NULL, NULL, NULL);
        //BN_generate_prime_ex(p, 768, 1, NULL, NULL, NULL);
        printf("%d prime generated\n", 1 << i);
    }
#else
    BIGNUM *num = BN_new();
    BIGNUM *p = BN_new();
    BIGNUM *q = BN_new();
    BN_generate_prime_ex(p, 28, 1, NULL, NULL, NULL);
    BN_generate_prime_ex(q, 28, 1, NULL, NULL, NULL);
    BN_CTX *ctx = BN_CTX_new();

    BN_mul(num, p, q, ctx);

    BIGNUM *div = BN_new();
    BIGNUM *rem = BN_new();
    BIGNUM *i = BN_new();
    BIGNUM *root = nth_root(num, 2);
    BIGNUM *zero = BN_new();
    BN_zero(zero);

    char *numstr = BN_bn2dec(num);
    printf("Number: %s\n", numstr);

    char *rootstr = BN_bn2dec(root);
    printf("Root: %s\n", rootstr);

    for (BN_set_word(i, 2); BN_cmp(i, root) != 0; BN_add_word(i, 1)) {
        BN_div(div, rem, num, i, ctx);
        if (BN_cmp(rem, zero) == 0) {
            char *str = BN_bn2dec(div);
            char *istr = BN_bn2dec(i);
            printf("Factors: %s %s\n", istr, str);
            break;
        }
    }
#endif
#endif
    //Do slow stuff here
    //sleep(4);
    return NULL;
}
