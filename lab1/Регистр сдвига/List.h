#ifndef LIST_H
#define LIST_H

typedef struct {
    void *head;
    void *tail;
    long long unsigned size;
} List;

void initList(List *list);
void *getListValue(List *list, long long unsigned i);
void setListValue(List *list, long long unsigned i, void *value);
long long unsigned getListSize(List *list);
int addListValue(List *list, void *value);
void *popListValue(List *list, long long unsigned i);
void freeList(List *list);
void transfer(List *src, List *dst);

struct ListIterator {
    void *node;
};

void initListIterator(List *list, struct ListIterator *it);
void incListIterator(struct ListIterator *it);
void *getListIteratorValue(struct ListIterator *it);

#endif