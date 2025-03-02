#include "List.h"
#include <stdlib.h>

struct Node {
    void *value;
    struct Node *next;
};

static struct Node *newNode(void *value, struct Node *next) {
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    if (!node) return NULL;
    node->value = value;
    node->next = next;
    return node;
}

static void *getNodeValue(struct Node *node) {
    return node->value;
}

static void *setNodeValue(struct Node *node, void *value) {
    node->value = value;
}

static struct Node *getNodeNext(struct Node *node) {
    return node->next;
}

static void setNodeNext(struct Node *node, struct Node *next) {
    node->next = next;
}

static void freeNode(struct Node *node) {
    node->value = NULL;
    node->next = NULL;
    free(node);
}

void initList(List *list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void *getListValue(List *list, long long unsigned i) {
    struct Node *node = (struct Node *)list->head;
    while (i--) node = getNodeNext(node);
    return getNodeValue(node);
}

void setListValue(List *list, long long unsigned i, void *value) {
    struct Node *node = (struct Node *)list->head;
    while (i--) node = getNodeNext(node);
    setNodeValue(node, value);
}

long long unsigned getListSize(List *list) {
    return list->size;
}

static int addFirstListValue(List *list, void *value) {
    list->head = (void *)newNode(value, NULL);
    if (!list->head) return -1;
    setNodeNext(list->head, list->head);
    list->tail = list->head;
    ++list->size;
    return 0;
}

static int addNextListValue(List *list, void *value) {
    struct Node *node = newNode(value, list->head);
    if (!node) return -1;
    setNodeNext(list->tail, node);
    list->tail = node;
    ++list->size;
    return 0;
}

int addListValue(List *list, void *value) {
    if (list->size == 0)
        return addFirstListValue(list, value);
    return addNextListValue(list, value);
}

static void *popFirstListValue(List *list) {
    struct Node *prev_head = (struct Node *)list->head;
    if (list->size == 1) {
        list->head = NULL;
        list->tail = NULL;
    }
    else {
        list->head = (void *)getNodeNext(prev_head);
        setNodeNext(list->tail, list->head);
    }
    void *value = getNodeValue(prev_head);
    freeNode(prev_head);
    --list->size;
    return value;
}

static void *popNotFirstListValue(List *list, long long unsigned i) {
    struct Node *prev_node = (struct Node *)list->head;
    while (--i) prev_node = getNodeNext(prev_node);
    struct Node *node = getNodeNext(prev_node);
    setNodeNext(prev_node, getNodeNext(node));
    if (node == list->tail) list->tail = prev_node;
    void *value = getNodeValue(node);
    freeNode(node);
    --list->size;
    return value;
}

void *popListValue(List *list, long long unsigned i) {
    if (i == 0) return popFirstListValue(list);
    return popNotFirstListValue(list, i);
}

void freeList(List *list) {
    if (list->size == 0) return;
    struct Node *node = (struct Node *)list->head;
    struct Node *next;
    do {
        next = getNodeNext(node);
        freeNode(node);
        node = next;
    } while (node != list->head);
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void initListIterator(List *list, struct ListIterator *it) {
    it->node = list->head;
}

void incListIterator(struct ListIterator *it) {
    it->node = (void *)getNodeNext(it->node);
}

void *getListIteratorValue(struct ListIterator *it) {
    return getNodeValue(it->node);
}

void transfer(List *src, List *dst) {
    if(!src->size) return;
    if (!dst->size) {
        dst->head = src->head;
        dst->tail = src->tail;
        dst->size = src->size;
    }
    else {
        setNodeNext(dst->tail, src->head);
        dst->tail = src->tail;
        dst->size += src->size;
    }
    src->head = NULL;
    src->tail = NULL;
    src->size = 0;
}