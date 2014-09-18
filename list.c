
#include "list.h"

list *list_init() {
    list *l = malloc(sizeof(list));
    l->head = NULL;
    l->tail = NULL;
    return l;
}

void list_free(list *l,void (*freeItem)(void *d)) {
    listNode *i = l->head;
    listNode *n;
    while (i != NULL) {
        if (freeItem != NULL) {
            freeItem(i->item);
        }
        n = i->next;
        free(i);
        i = n;
    }
    free(l);
}

void list_append(list *l, void *i) {
    listNode *n = (listNode *) malloc(sizeof(listNode));
    listNode *t = l->tail;
    n->item = i;
    if (t == NULL) {
        n->prev = NULL;
        n->next = NULL;
        l->head = n;
        l->tail = n;
    } else {
        n->prev = t;
        n->next = NULL;
        t->next = n;
        l->tail = n;
    }
}

void list_prepend(list *l, void *i) {
    listNode *n = (listNode *) malloc(sizeof(listNode));
    listNode *h = l->head;
    n->item = i;
    if (h == NULL) {
        n->prev = NULL;
        n->next = NULL;
        l->head = n;
        l->tail = n;
    } else {
        n->prev = NULL;
        n->next = h;
        h->prev = n;
        l->head = n;
    }
}

int list_count(list *l) {
    int n = 0;
    listNode *i = l->head;
    while (i != NULL) {
        ++n;
        i = i->next;
    }
    return n;
}

listNode *list_nth(list *l, int n) {
    listNode *i = l->head;
    int c = 0;
    while (i != NULL) {
        if (c == n) {
            return i;
        } 
        ++c;
        i = i->next;
    }
    return NULL;
}

void list_insert(list *l, listNode *after, void *item) {
    listNode *n = l->head;
    while (n != NULL) {
        if (n == after) {
            listNode *new = (listNode *) malloc(sizeof(listNode));
            new->item = item;
            new->prev = n;
            if (n->next) {
                new->next = n->next;
                n->next->prev = new;
            } else {
                new->next = NULL;
            }
            n->next = new;
            break;
        }
        n = n->next;
    }
}

listNode *list_find(list *l, void *target, int (*cmp)(void *a,void *b)) {
    listNode *n = l->head;
    while (n != NULL) {
        if (cmp(n->item,target) == 0) {
            return n;
        }
        n = n->next;
    }
    return NULL;
}

void list_apply(list *l, void (*f)(void *d)) {
    listNode *n = l->head;
    while (n != NULL) {
        f(n->item);
        n = n->next;
    }
}

list *list_filter(list *l, int (*f)(void *d)) {
    list *r = list_init();
    listNode *n = l->head;
    while (n != NULL) {
        if (f(n->item)) {
            list_append(r,n->item);
        }
        n = n->next;
    }
    return r;
}

list *list_map(list *l, void *(*f)(void *d)) {
    list *r = list_init();
    listNode *n = l->head;
    while (n != NULL) {
        list_append(r,f(n->item));
        n = n->next;
    }
    return r;
}

