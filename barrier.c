/**
 * tree barrier implementation
 * compile: gcc barrier.c -o barrier
 * execute: ./barrier [num_of_threads (default 16)]
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/timeb.h>

/* read timer in second */
double read_timer() {
    struct timeb tm;
    ftime(&tm);
    return (double) tm.time + (double) tm.millitm / 1000.0;
}

/* read timer in ms */
double read_timer_ms() {
    struct timeb tm;
    ftime(&tm);
    return (double) tm.time * 1000.0 + (double) tm.millitm;
}

/* fan-in and fan-out parameters are 4, https://6xq.net/barrier-intro/ */
typedef struct {
    int count;
    pthread_mutex_t count_lock;
    pthread_cond_t ok_to_proceed;
} mylib_barrier_t;

void mylib_barrier_init(mylib_barrier_t *b) {
    b->count = 0;
    pthread_mutex_init(&(b->count_lock), NULL);
    pthread_cond_init(&(b->ok_to_proceed), NULL);
}

void mylib_barrier (mylib_barrier_t *b, int num_threads) {
    pthread_mutex_lock(&(b->count_lock));

    b->count ++;
    if (b->count == num_threads) {
        b->count = 0;
        pthread_cond_broadcast(&(b->ok_to_proceed));
    } else
        while (pthread_cond_wait(&(b->ok_to_proceed), &(b->count_lock)) != 0);

    pthread_mutex_unlock(&(b->count_lock));
}

void mylib_treebarrier (mylib_barrier_t *b, int num_threads) {

}

/* a reduction operation (sum) that add all the local values of each thread together and store back to a global variable */
void mylib_sum(mylib_barrier_t * b, int num_threads, long local, long * global) {

}

int NUM_THREADS = 16;
mylib_barrier_t barrier;

void *barrier_test(void *thread_id) { /* thread func */
    long tid = ((long)thread_id);
    printf("Entering barrier%ld!\n", tid);
    mylib_barrier(&barrier, NUM_THREADS);
//    mylib_treebarrier(&barrier, NUM_THREADS);
    printf("Existing barrier%ld!\n", tid);
    pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: a.out [NUM_THREADS(%d)]\n", NUM_THREADS);
    } else NUM_THREADS = atoi(argv[1]);
    pthread_t thread[NUM_THREADS];
    pthread_attr_t attr;
    long t;
    void *status;

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    mylib_barrier_init(&barrier);

    for(t=0; t<NUM_THREADS; t++) {
        pthread_create(&thread[t], &attr, barrier_test, (void *)t);
    }
    /* Free attribute and wait for the other threads */
    pthread_attr_destroy(&attr);
    for(t=0; t<NUM_THREADS; t++) {
        pthread_join(thread[t], &status);
    }
    pthread_exit(NULL);
}

