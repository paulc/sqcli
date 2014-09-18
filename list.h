
#include <stdlib.h>

typedef struct listNode {
    void *item;
    struct listNode *prev;
    struct listNode *next;
} listNode;

typedef struct list {
    struct listNode *head;
    struct listNode *tail;
} list;

list *list_init();
void list_free(list *l,void (*freeItem)(void *d));
void list_append(list *l, void *i);
void list_prepend(list *l, void *i);
int list_count(list *l);
listNode *list_nth(list *l, int n);
void list_insert(list *l, listNode *after, void *item);
listNode *list_find(list *l, void *target, int (*cmp)(void *a,void *b));
void list_apply(list *l, void (*f)(void *d));
list *list_filter(list *l, int (*f)(void *d));
list *list_map(list *l, void *(*f)(void *d));

