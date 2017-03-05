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

typedef struct {
    int count;
    pthread_mutex_t count_lock;
    pthread_cond_t ok_to_proceed;
    struct node * root;
    struct node * L1Nodes;
    struct node * parent;
} mylib_treebarrier_t;

struct node {
	int num_children;
	int isL1Node; /* if true, this is L1 node */
	struct node * children[4];
	int threads[4];
    struct node * parent;
    int L1id;

    int count;
    pthread_mutex_t count_lock;
    pthread_cond_t ok_to_proceed;
};

void mylib_treebarrier_init(mylib_treebarrier_t *b, int num_threads) {
    b->count = 0;
    pthread_mutex_init(&(b->count_lock), NULL);
    pthread_cond_init(&(b->ok_to_proceed), NULL);

    /* init by creating the tree of threads */

    int num_L1Node = num_threads / 4;
    if (num_threads % 4){
        num_L1Node++;
    } 

    struct node * L1Nodes = malloc(sizeof(struct node) * num_L1Node);
    int i;
    int j = 0;
    int tid = 0;
    printf("num_L1Node: %d\n", num_L1Node);

    for (i = 0; i < num_L1Node; i++) {
    	if (num_threads % 4 && i == num_L1Node - 1){
            L1Nodes[i].num_children = num_threads % 4;
        } else {
            L1Nodes[i].num_children = 4;
        }

	    for (j = 0; j < L1Nodes[i].num_children; j++) {
		    L1Nodes[i].threads[j] = tid++;
	    }

        L1Nodes[i].parent = NULL;
        L1Nodes[i].isL1Node = 1;

        L1Nodes[i].count = 0;
        pthread_mutex_init(&(L1Nodes[i].count_lock), NULL);
        pthread_cond_init(&(L1Nodes[i].ok_to_proceed), NULL);

        printf("L1Nodes[%d].num_children: %d\n", i, L1Nodes[i].num_children);
        printf("L1Nodes[%d].isL1Node: %d\n", i, L1Nodes[i].isL1Node);
        printf("L1Nodes[%d].parent: %d\n", i, L1Nodes[i].parent == NULL);
    }
    b->L1Nodes = L1Nodes;

    int total_num_nodes = num_L1Node;
    struct node * nodes = L1Nodes;
    tid = 0;

    printf("total_num_nodes: %d\n", num_L1Node);
    /* each loop iteration add one more level up to the root if needed */
    while ( total_num_nodes > 1 ) {
    	int num_parentNodes = total_num_nodes / 4;
	    if (total_num_nodes % 4){
            num_parentNodes ++;
        }
        
    	struct node * parentNodes = malloc(sizeof(struct node) * num_parentNodes);
	    for(i = 0; i < num_parentNodes; i++) {
		    if (total_num_nodes % 4 && i == num_parentNodes - 1){
                parentNodes[i].num_children = total_num_nodes % 4;
            } else {
                parentNodes[i].num_children = 4;
            }

		    for (j = 0; j < parentNodes[i].num_children; j++) {
			    parentNodes[i].children[j] = &L1Nodes[tid++];
                parentNodes[i].children[j]->parent = &parentNodes[i];
		    }

            parentNodes[i].parent = NULL;
		    parentNodes[i].isL1Node = 0;

            parentNodes[i].count = 0;
            pthread_mutex_init(&(parentNodes[i].count_lock), NULL);
            pthread_cond_init(&(parentNodes[i].ok_to_proceed), NULL);
	    }	

	    total_num_nodes = num_parentNodes;
	    nodes = parentNodes;
    }

    b->root = nodes;
}

void mylib_treebarrier (mylib_treebarrier_t * b, int tid) {
    struct node * currentNode = NULL;

    struct node * parent = &(b->L1Nodes[tid / 4]);

	int num_threads = parent->num_children;

    // printf("tid: %d | num_threads: %d | L1Nodes: %d | Parent Id: %d\n", tid, num_threads, (int) tid / 4, parent->L1id);

	struct node * path[8];
	int last = -1;	
    int top = 0;
	/* to gather all threads' arrival hierarchically */
	while (1) {
		/* report to parent node of arrival */
		pthread_mutex_lock(&(parent->count_lock));
        
		parent->count ++;
        
        if(!parent->isL1Node){
            printf("count: %d\n", parent->count);
        }

		if (parent->count == num_threads) {/* this this the master/submaster to report to upper level */
            
			if (parent->parent == NULL) { /* The master on top */
                printf("ROOT!\n");
                //while(1);
        		pthread_cond_broadcast(&(parent->ok_to_proceed));

                /*TODO: broadcast all the nodes along the path */
                //int i;
                //for(i = 0; i < top; i++){
                //    pthread_cond_broadcast(&(path[top]->ok_to_proceed));
                //}
				
				pthread_mutex_unlock(&(parent->count_lock));
				break;
			} else { /* report to the upper level */
                pthread_mutex_unlock(&(parent->count_lock));
                
				
				/* record the path */
				// path[top++] = parent;
                
				currentNode = parent;
                parent = parent->parent;
				num_threads = parent->num_children;

                printf("L1 NODE! ok: %p\n", &parent->ok_to_proceed);
			}
		} else {
			while (pthread_cond_wait(&(parent->ok_to_proceed), &(parent->count_lock)) != 0);
            printf("FREEDOM!");
            while(1);

			if (currentNode != NULL ) {
        		pthread_cond_broadcast(&(currentNode->ok_to_proceed));

				/*TODO: broadcast all the nodes along the path */
                int i;
                for(i = 0; i < top; i++){
                    pthread_cond_broadcast(&(path[top]->ok_to_proceed));
                }

				// pthread_mutex_unlock(&(parent->count_lock));
			}
			// pthread_mutex_unlock(&(parent->count_lock));
			break;
		}
	}
}

/* a reduction operation (sum) that add all the local values of each thread together and store back to a global variable */
void mylib_sum(mylib_barrier_t * b, int num_threads, long local, long * global) {

}

int NUM_THREADS = 16;
mylib_barrier_t barrier;
mylib_treebarrier_t treebarrier;

void *barrier_test(void *thread_id) { /* thread func */
    long tid = ((long)thread_id);
    //printf("Entering barrier%ld!\n", tid);
//    mylib_barrier(&barrier, NUM_THREADS);
    mylib_treebarrier(&treebarrier, tid);
    //printf("Existing barrier%ld!\n", tid);
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
    mylib_treebarrier_init(&treebarrier, NUM_THREADS);

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

