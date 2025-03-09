#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "Hash.h"

typedef void (*FreeValueFunction)(void *value);
typedef uint8_t (*ValueComparator)(void *value1, void *value2);
typedef void (*PrintValue)(void *);

typedef struct {
    void *head;
    void *tail;
    uint64_t size;
} List;

void initList(List *list);
void *getListValue(List *list, uint64_t i);
int setListValue(List *list, uint64_t i, void *value, size_t value_size);
uint64_t getListSize(List *list);
int pushList(List *list, void *value, size_t value_size);
int pushForwardList(List *list, void *value, size_t value_size);
void *topList(List *list, uint8_t free_value);
void *popListAtIndex(List *list, uint64_t i, uint8_t free_value);
void *popList(List *list, uint8_t free_value);
void clearList(List *list);
void deepClearList(List *list, FreeValueFunction free_value);
void transfer(List *src, List *dst);
uint64_t indexOfList(List *list, void *value, size_t value_size);
uint64_t deepIndexOfList(List *list, void *value, ValueComparator compare);
uint8_t containsList(List *list, void *value, size_t value_size);
uint8_t deepContainsList(List *list, void *value, ValueComparator compare);
void* getListHead(List *list);
void* getListTail(List *list);
int copyList(List *dst, List *src);
uint64_t hashList(const List *list);
uint64_t deepHashList(const List *list, Hash hashValue);
uint8_t compareLists(List *list1, List *list2);
uint8_t deepCompareLists(List *list1, List *list2, ValueComparator compare);

struct ListIterator {
    void *node;
};

void setListIteratorNode(struct ListIterator *it, void *node);
uint8_t compareListIteratorNode(struct ListIterator *it, void *node);
void incListIterator(struct ListIterator *it);
void *getListIteratorValue(struct ListIterator *it);
void printList(List *list, PrintValue printValue);

#endif