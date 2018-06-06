#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <list/zm_scdlist.h>

#define TEST_NELEMTS 100
#define SHUFFLE_SIZE 10

#undef ZM_SCDL_DATA
#define ZM_SCDL_DATA int data

static inline void kfy_shuffle(int *array, int length) {
    for(int i = length - 1; i > 0; i--) {
        int trg =  rand() % (i + 1);
        int tmp = array[i];
        array[i] = array[trg];
        array[trg] = tmp;
    }
}

int main (int argc, char** argv) {
    int rand_array[TEST_NELEMTS], i, errs = 0;
    struct zm_scdlnode nodes[TEST_NELEMTS];
    struct zm_scdlnode *tmp_node;
    int nproducers = omp_get_max_threads() - 1;
    int chunk_size = TEST_NELEMTS/nproducers;

    for (i=0; i<TEST_NELEMTS; i++)
        rand_array[i] = rand();
    struct zm_scdlist list1, list2, list3;

    zm_scdlist_init(&list1);
    zm_scdlist_init(&list2);
    zm_scdlist_init(&list3);

    /* push the array's elements to list1 */
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        if (tid > 0) {
            int start = (tid - 1) * chunk_size;
            int end = ((tid - 1) < nproducers  - 1) ? (tid) * chunk_size : TEST_NELEMTS;
            for (i = start; i < end; i++) {
                nodes[i].data = rand_array[i];
                zm_scdlist_push_back(&list1, &nodes[i]);
            }
        } else {
            volatile int count = 0;
            /* pop the elements from list1 and push them to list2 */
            zm_scdlist_pop_front(&list1, &tmp_node);
            while (count < TEST_NELEMTS) {
                if(tmp_node != NULL)
                    count++;
                zm_scdlist_pop_front(&list1, &tmp_node);
            }
        }
    }

    /* remove windows of elements from list2. Select random elements inside
     * each window to remove; wait if element not pushed yet. */
    /* push the array's elements to list1 */
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        if (tid > 0) {
            int start = (tid - 1) * chunk_size;
            int end = ((tid - 1) < nproducers  - 1) ? (tid) * chunk_size : TEST_NELEMTS;
            for (i = start; i < end; i++) {
                nodes[i].data = rand_array[i];
                zm_scdlist_push_back(&list1, &nodes[i]);
            }
        } else {
            for(int i = 0; i < (TEST_NELEMTS/SHUFFLE_SIZE); i++) {
                int indices[SHUFFLE_SIZE];
                int window_idx = i*SHUFFLE_SIZE;

                for(int j = 0; j < SHUFFLE_SIZE; j++)
                    indices[j] = window_idx + j;

                kfy_shuffle(&indices[0], SHUFFLE_SIZE);

                for(int j = 0; j < SHUFFLE_SIZE; j++) {
                    int ret;
                    do {
                        ret = zm_scdlist_remove(&list1, &nodes[indices[j]]);
                    } while(ret == ZM_ENOTFOUND);
                }
            }
        }
    }
    if(!zm_scdlist_isempty(list2))
        errs++;

    if(errs > 0)    fprintf(stderr, "Failed with %d errors\n", errs);
    else            printf("Pass\n");

    return 0;
}
