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

struct node {
	int num_children;
	int isL1Node; /* if true, this is L1 node */
	struct node children[4];
	int threads[4];
}

/* fan-in and fan-out parameters are 4, https://6xq.net/barrier-intro/ */
typedef struct {
    int count;
    pthread_mutex_t count_lock;
    pthread_cond_t ok_to_proceed;
    struct node * root;
    struct node * L1Nodes;
} mylib_barrier_t;


typedef struct {
    int count;
    pthread_mutex_t count_lock;
    pthread_cond_t ok_to_proceed;
    struct node * root;
    struct node * L1Nodes;
} mylib_barrier_t;


void mylib_barrier_init(mylib_barrier_t *b, int num_threads) {
    
    b->count = 0;
    pthread_mutex_init(&(b->count_lock), NULL);
    pthread_cond_init(&(b->ok_to_proceed), NULL);

    /* init by creating the tree of threads */

    int num_L1Node = num_threads/4;
    if (num_threads%4) num_L1node ++;

    struct node * L1nodes = malloc(sizeof(struct node) * num_L1Node);
    int i;
    int j = 0;
    int tid = 0;
    for (i=0; i<num_L1Node; i++) {
    	if (num_threads%4 && i==num_L1Node-1) L1nodes[i].num_children = num_threads%4;
        else 	L1nodes[i].num_children = 4;
	for (j=0; j<L1nodes[i].num_children; j++) {
		L1Nodes[i].threads[j] = tid++;
	}
        L1nodes[i].isL1Node = 1;
    }
    b->L1Nodes = L1nods;

    int total_num_nodes = num_L1Node;
    struct node * nodes = L1nodes;
    int tid = 0;
    /* each loop iteration add one more level up to the root if needed */
    while ( total_num_nodes > 1 ) {
    	int num_parentNodes = total_num_nodes/4;
	if (total_num_nodes%4) num_parentNodes ++;
    	struct node * parentNodes = malloc(sizeof(struct node) * num_parentNodes);
	for(i=0;i<num_parentNodes;i++) {
		if (totol_num_nodes%4 && i==num_parentNodes-1) parentNodes[i].num_children = total_num_nodes%4;
		else 	parentNodes[i].num_children = 4;
		for (j=0; j<parentNodes[i].num_children;j++)
			parentNodes[i].children[j] = tid++;
		parentNodes[i].isL1Node = 0;
	}	

	total_num_nodes = num_parentNodes;
	nodes = parentNodes;
    }
    b->root = nodes;
    
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

void mylib_treebarrier (mylib_barrier_t *b, int tid, int num_threads) {
	struct node * L1node = b->L1Nodes[tid/4];
	int num_threads = L1node.num_children;
	
	/* to gather all threads' arrival hierarchically */
	while ( ) {

	}

	/* to signal all the threads to proceed hierarchically */
	while ( ) {

	}	
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

