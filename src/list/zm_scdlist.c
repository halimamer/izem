/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "list/zm_scdlist.h"

void zm_scdlist_init(struct zm_scdlist *list) {
    STORE(&list->head, ZM_NULL);
    STORE(&list->tail, ZM_NULL);
}
