#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <string.h>

typedef uint8_t (*ValueComparator)(void *, void *);

typedef struct {
    void *head;
    void *tail;
    uint64_t size;
} List;

void initList(List *list);
void *getListValue(List *list, uint64_t i);
void setListValue(List *list, uint64_t i, void *value);
uint64_t getListSize(List *list);
int pushList(List *list, void *value);
int pushForwardList(List *list, void *value);
void *topList(List *list);
void *popListAtIndex(List *list, uint64_t i);
void *popList(List *list);
void freeList(List *list);
void transfer(List *src, List *dst);
uint64_t indexOfList(List *list, void *value, size_t value_size);
uint64_t deepIndexOfList(List *list, void *value, ValueComparator compare);
uint64_t containsList(List *list, void *value, size_t value_size);
uint64_t deepContainsList(List *list, void *value, ValueComparator compare);

struct ListIterator {
    void *node;
};

void initListIterator(List *list, struct ListIterator *it);
void incListIterator(struct ListIterator *it);
void *getListIteratorValue(struct ListIterator *it);

#endif