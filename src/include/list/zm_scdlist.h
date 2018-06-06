/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_SCDLIST_H_
#define _ZM_SCDLIST_H_

#include <stdlib.h>
#include <assert.h>
#include "common/zm_common.h"

#define LOAD(addr)                  zm_atomic_load(addr, zm_memord_acquire)
#define STORE(addr, val)            zm_atomic_store(addr, val, zm_memord_release)
#define SWAP(addr, desire)          zm_atomic_exchange(addr, desire, zm_memord_acq_rel)
#define CAS(addr, expect, desire)   zm_atomic_compare_exchange_strong(addr,\
                                                                      expect,\
                                                                      desire,\
                                                                      zm_memord_acq_rel,\
                                                                      zm_memord_acquire)

/* Non-catastrophic error code(s) */

#define ZM_SUCCESS 0
#define ZM_ENOTFOUND 1
#define ZM_SCDL_DATA int data /* user should redefine this */

/* Single-Consumer Doubly-linked List (SDList) */

struct zm_scdlist {
    zm_atomic_ptr_t head;
    zm_atomic_ptr_t tail;
};

struct zm_scdlnode {
    /* DCDL_DATA must be defined by the user of the list*/
    ZM_SCDL_DATA __attribute__((aligned(64)));
    zm_atomic_ptr_t next;
    zm_atomic_ptr_t prev;
};

void zm_scdlist_init(struct zm_scdlist *);

/* Concurrent push to the back (tail) of the list */
static inline void zm_scdlist_push_back(struct zm_scdlist *list, struct zm_scdlnode *node) {
    STORE(&node->next, ZM_NULL);
    STORE(&node->prev, ZM_NULL);

    zm_ptr_t tail = LOAD(&list->tail);
    SWAP(&list->tail, (zm_ptr_t)node);
    if(tail == ZM_NULL) {
        STORE(&list->head, (zm_ptr_t)node); /* invariant: only one producer arrives here */
    } else {
        STORE(&((struct zm_scdlnode*)tail)->next, node);
        STORE(&node->prev, tail); /* a deq might wait for this to be set */
    }
}

/* Sequential pop from the front (dequeue) of the list */
static inline int zm_scdlist_pop_front(struct zm_scdlist *list, struct zm_scdlnode **out_node) {
    zm_ptr_t head;
    zm_ptr_t next;
    *out_node = (struct zm_scdlnode *)ZM_NULL;
    if(LOAD(&list->head) == ZM_NULL) {
        if (LOAD(&list->tail) != ZM_NULL) {
            /* an enq is in progress */
            while(LOAD(&list->head) == ZM_NULL)
                ;/* wait for the enqueuer to update head */
        } else {
            return ZM_ENOTFOUND; /* empty list */
        }
    }
    head = LOAD(&list->head);
    next = LOAD(&((struct zm_scdlnode *)head)->next);
    STORE(&list->head, next);
    /* update tail if empty queue */
    if(next == ZM_NULL) {
        if(LOAD(&list->tail) == head)
            CAS(&list->tail, (zm_ptr_t *)&head, next);
    } else {
        STORE(&((struct zm_scdlnode*)next)->prev, ZM_NULL);
    }
    *out_node = (struct zm_scdlnode *)head;
    return ZM_SUCCESS;
}

/* This routine is O(1) and assumes node truly exists in list */
static inline int zm_scdlist_remove(struct zm_scdlist *list, struct zm_scdlnode *node) {
    zm_ptr_t head = LOAD(&list->head);
    zm_ptr_t tail = LOAD(&list->tail);
    zm_ptr_t prev;
    if((head == ZM_NULL) && (tail == ZM_NULL))
        return ZM_ENOTFOUND;
    if(node == (struct zm_scdlnode*)LOAD(&list->head)) {
        struct zm_scdlnode *tmp_node;
        int ret = zm_scdlist_pop_front(list, &tmp_node);
        assert(tmp_node == node);
        return ret;
    }
    if((zm_ptr_t)node == tail) {
        prev = LOAD(&((struct zm_scdlnode*)tail)->prev);
        if(CAS(&list->tail, (zm_ptr_t *)&tail, prev)) {
            CAS(&((struct zm_scdlnode*)tail)->prev, (zm_ptr_t *)&tail, ZM_NULL); /* CAS instead of store because
                                                another thread T2 might be
                                                enqueing, i.e., tail has moved
                                                again and prev would point to
                                                it */
        }
    } else { /* middle of the list; no races, so atomic stores are sufficient */
        STORE(&((struct zm_scdlnode *)node->prev)->next, node->next);
        STORE(&((struct zm_scdlnode *)node->next)->prev, node->prev);
        /* reset removed node links */
        STORE(&node->next, ZM_NULL);
        STORE(&node->prev, ZM_NULL);
    }
    return ZM_SUCCESS;
}

static inline int zm_scdlist_isempty(struct zm_scdlist list) {
   return (LOAD(&list.head) == ZM_NULL) && (LOAD(&list.tail) != ZM_NULL);
}

#endif /* _ZM_SCDLIST_H_ */
