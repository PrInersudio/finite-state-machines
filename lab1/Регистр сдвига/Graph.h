#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>
#include "BitArray.h"
#include "List.h"

struct Graph {
    BitArray *adjacency_matrix;
    uint64_t num_of_nodes;
};

void enablePrintSearchLog();
int initGraph(struct Graph *graph, uint64_t num_of_nodes);
void freeGraph(struct Graph *graph);
uint8_t edgeExists(struct Graph *graph, uint64_t node1, uint64_t node2);
void setOrDeleteEdge(
    struct Graph *graph,
    uint64_t node1, uint64_t node2, uint8_t set
);
int getComponents(struct Graph *graph, List *components);
void makeUnoriented(struct Graph *graph);
int reverseGraph(struct Graph *original, struct Graph *reversed);
int getStronglyConnectedComponents(struct Graph *graph, List *components);
void printGraph(struct Graph *graph);


#endif