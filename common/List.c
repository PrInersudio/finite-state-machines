#include "List.h"
#include <stdlib.h>

struct Node {
    void *value;
    size_t value_size;
    struct Node *next;
};

static struct Node *newNode(void *value, size_t value_size) {
    struct Node *node = malloc(sizeof(struct Node));
    if (!node) return NULL;
    node->value = malloc(value_size);
    if (!node->value) {
        free(node);
        return NULL;
    }
    memcpy(node->value, value, value_size);
    node->value_size = value_size;
    node->next = NULL;
    return node;
}

static void *freeNode(struct Node *node, uint8_t free_value) {
    void *value;
    if (free_value) {
        value = NULL;
        free(node->value);
    } else value = node->value;
    free(node);
    return value;
}

static void deepFreeNode(struct Node *node, FreeValueFunction free_value) {
    free_value(node->value);
    free(node->value);
    free(node);
}

static void *getNodeValue(struct Node *node) {
    return node->value;
}

static int setNodeValue(struct Node *node, void *value, size_t value_size) {
    void *new_value = malloc(value_size);
    if (!new_value) return -1;
    memcpy(new_value, value, value_size);
    free(node->value);
    node->value = new_value;
    node->value_size = value_size;
    return 0;
}

static size_t getNodeValueSize(struct Node *node) {
    return node->value_size;
}

static struct Node *getNodeNext(struct Node *node) {
    return node->next;
}

static void setNodeNext(struct Node *node, struct Node *next) {
    node->next = next;
}

static struct Node *copyNode(struct Node *src) {
    return newNode(src->value, src->value_size);
}

void initList(List *list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void *getListValue(List *list, uint64_t i) {
    struct Node *node = list->head;
    while (i--) node = getNodeNext(node);
    return getNodeValue(node);
}

int setListValue(List *list, uint64_t i, void *value, size_t value_size) {
    struct Node *node = list->head;
    while (i--) node = getNodeNext(node);
    return setNodeValue(node, value, value_size);
}

uint64_t getListSize(List *list) {
    return list->size;
}

static void addListNode(List *list, struct Node *node, uint8_t forward) {
    if (list->size == 0) {
        setNodeNext(node, node);
        list->head = node;
        list->tail = node;
    }
    else {
        setNodeNext(node, list->head);
        setNodeNext(list->tail, node);
        if (forward) list->head = node;
        else list->tail = node;
    }
    ++list->size;
}

static int push(List *list, void *value, size_t value_size, uint8_t forward) {
    struct Node *node = newNode(value, value_size);
    if (!node) return -1;
    addListNode(list, node, forward);
    return 0;
}

int pushList(List *list, void *value, size_t value_size) {
    return push(list, value, value_size, 0);
}

int pushForwardList(List *list, void *value, size_t value_size) {
    return push(list, value, value_size, 1);
}

void *topList(List *list, uint8_t free_value) {
    struct Node *prev_head = list->head;
    if (list->size == 1) {
        list->head = NULL;
        list->tail = NULL;
    }
    else {
        list->head = getNodeNext(prev_head);
        setNodeNext(list->tail, list->head);
    }
    --list->size;
    return freeNode(prev_head, free_value);
}

void *popListAtIndex(List *list, uint64_t i, uint8_t free_value) {
    if (i == 0) return topList(list, free_value);
    struct Node *prev_node = list->head;
    while (--i) prev_node = getNodeNext(prev_node);
    struct Node *node = getNodeNext(prev_node);
    setNodeNext(prev_node, getNodeNext(node));
    if (node == list->tail) list->tail = prev_node;
    --list->size;
    return freeNode(node, free_value);
}

void *popList(List *list, uint8_t free_value) {
    return popListAtIndex(list, list->size - 1, free_value);
}

void clearList(List *list) {
    if (list->size == 0) return;
    struct Node *node = list->head;
    do {
        struct Node *next = getNodeNext(node);
        freeNode(node, 1);
        node = next;
    } while (node != list->head);
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void deepClearList(List *list, FreeValueFunction free_value) {
    if (list->size == 0) return;
    struct Node *node = list->head;
    do {
        struct Node *next = getNodeNext(node);
        deepFreeNode(node, free_value);
        node = next;
    } while (node != list->head);
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void setListIteratorNode(struct ListIterator *it, void *node) {
    it->node = node;
}

uint8_t compareListIteratorNode(struct ListIterator *it, void *node) {
    return it->node == node;
}

void incListIterator(struct ListIterator *it) {
    it->node = getNodeNext(it->node);
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
        setNodeNext(src->tail, dst->head);
        dst->tail = src->tail;
        dst->size += src->size;
    }
    src->head = NULL;
    src->tail = NULL;
    src->size = 0;
}

uint64_t indexOfList(List *list, void *value, size_t value_size) {
    if (!list->size) return 0;
    uint64_t i = 0;
    struct Node *node = list->head;
    do {
        if (!memcmp(getNodeValue(node), value, value_size)) break;
        node = getNodeNext(node);
        ++i;
    } while (node != list->head);
    return i;
}

uint64_t deepIndexOfList(List *list, void *value, ValueComparator compare) {
    if (!list->size) return 0;
    uint64_t i = 0;
    struct Node *node = list->head;
    do {
        if (!compare(getNodeValue(node), value)) break;
        node = getNodeNext(node);
        ++i;
    } while (node != list->head);
    return i;
}

uint8_t containsList(List *list, void *value, size_t value_size) {
    return indexOfList(list, value, value_size) < list->size;
}

uint8_t deepContainsList(List *list, void *value, ValueComparator compare) {
    return deepIndexOfList(list, value, compare) < list->size;
}

void* getListHead(List *list) {
    return list->head;
}

void* getListTail(List *list) {
    return list->tail;
}

int copyList(List *dst, List *src) {
    initList(dst);
    struct Node *node = src->head;
    do {
        struct Node *node_copy = copyNode(node);
        if (!node_copy) {
            clearList(dst);
            return -1;
        }
        addListNode(dst, node_copy, 0);
        node = getNodeNext(node);
    } while (node != src->head);
    return 0;
}

uint64_t hashList(const List *list) {
    if (!list->size) return 0;
    uint64_t hash = 0;
    struct Node* node = list->head;
    do {
        hash += hashBytes(getNodeValue(node), getNodeValueSize(node));
        hash *= 31;
        node = getNodeNext(node);
    } while(node != list->head);
    return hash;
}

uint64_t deepHashList(const List *list, Hash hashValue) {
    if (!list->size) return 0;
    uint64_t hash = 0;
    struct Node* node = list->head;
    do {
        hash += hashValue(getNodeValue(node));
        hash *= 31;
        node = getNodeNext(node);
    } while(node != list->head);
    return hash;
}

uint8_t compareLists(List *list1, List *list2) {
    if (list1->size != list2->size) return 0;
    if (!list1->size) return 1;
    struct Node *node1 = list1->head;
    struct Node *node2 = list2->head;
    do {
        if (
            getNodeValueSize(node1) != getNodeValueSize(node2) ||
            memcmp(getNodeValue(node1), getNodeValue(node2), getNodeValueSize(node1))
        ) return 0;
        node1 = getNodeNext(node1);
        node2 = getNodeNext(node2);
    } while (node1 != list1->head);
    return 1;
}

uint8_t deepCompareLists(List *list1, List *list2, ValueComparator compare) {
    if (list1->size != list2->size) return 0;
    if (!list1->size) return 1;
    struct Node *node1 = list1->head;
    struct Node *node2 = list2->head;
    do {
        if (!compare(getNodeValue(node1), getNodeValue(node2)))
            return 0;
        node1 = getNodeNext(node1);
        node2 = getNodeNext(node2);
    } while (node1 != list1->head);
    return 1;
}

void printList(List *list, PrintValue printValue) {
    if (!list->size) return;
    struct Node *node = list->head;
    do {
        printValue(getNodeValue(node));
        printf(" ");
        node = getNodeNext(node);
    } while (node != list->head);
}