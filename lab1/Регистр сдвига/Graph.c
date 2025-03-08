#include "Graph.h"
#include <stdlib.h>
#include "EquivalenceClass.h"

static int noop(const char *__restrict__ __format, ...) {
    return 0;
}

static int (*printSearchLog)(const char *__restrict__ __format, ...) = noop;

void enablePrintSearchLog() {
    printSearchLog = printf;
}

static void freeAdjacencyMatrix(BitArray *matrix, uint64_t num_of_nodes) {
    for (uint32_t i = 0; i <= (uint32_t)(num_of_nodes - 1); ++i)
        freeBitArray(matrix + i);
    free(matrix);
}

int initGraph(struct Graph *graph, uint64_t num_of_nodes) {
    graph->adjacency_matrix = (BitArray *)malloc(num_of_nodes * sizeof(BitArray));
    if (!graph->adjacency_matrix) return -1;
    for (uint32_t i = 0; i <= (uint32_t)(num_of_nodes - 1); ++i)
        if (initBitArray(graph->adjacency_matrix + i, num_of_nodes)) {
            freeAdjacencyMatrix(graph->adjacency_matrix, i);
            return -2;
        }
    graph->num_of_nodes = num_of_nodes;
    return 0;
};

void freeGraph(struct Graph *graph) {
    freeAdjacencyMatrix(graph->adjacency_matrix, graph->num_of_nodes);
    graph->num_of_nodes = 0;
}

uint8_t edgeExists(struct Graph *graph, uint32_t node1, uint32_t node2) {
    return getBitArrayElement(graph->adjacency_matrix + node1, node2);
}

void setOrDeleteEdge(struct Graph *graph, uint32_t node1, uint32_t node2, uint8_t set) {
    setBitArrayElement(graph->adjacency_matrix + node1, node2, set);
}

static int deepFirstSearch(
    struct Graph *graph,
    List *component,
    BitArray *visited,
    uint32_t current_node
) {
    setBitArrayElement(visited, current_node, 1);
    uint32_t *node = (uint32_t *)malloc(sizeof(uint32_t));
    if (!node) return -1;
    *node = current_node;
    if (pushList(component, node)) {
        free(node);
        return -2;
    }
    printSearchLog("deepFirstSearch вход. Текущее состояение: %u\n", current_node);
    for (uint32_t i = 0; i <= (uint32_t)(graph->num_of_nodes - 1); ++i) {
        if (!getBitArrayElement(visited, i) && edgeExists(graph, current_node, i))
            if (deepFirstSearch(graph, component, visited, i)) return -3;
    }
    printSearchLog("deepFirstSearch выход. Текущее состояение: %u\n", current_node);
    return 0;
}

static int invertedDeepFirstSearch(
    struct Graph *graph,
    List *component,
    BitArray *visited,
    uint32_t current_node
) {
    setBitArrayElement(visited, current_node, 1);
    printSearchLog("invertedDeepFirstSearch вход. Текущее состояение: %u\n", current_node);
    for (uint32_t i = 0; i <= (uint32_t)(graph->num_of_nodes - 1); ++i)
        if (!getBitArrayElement(visited, i) && edgeExists(graph, current_node, i))
            if (invertedDeepFirstSearch(graph, component, visited, i)) return -1;
    printSearchLog("invertedDeepFirstSearch выход. Текущее состояение: %u\n", current_node);
    uint32_t *node = (uint32_t *)malloc(sizeof(uint32_t));
    if (!node) return -2;
    *node = current_node;
    if (pushList(component, node)) {
        free(node);
        return -3;
    }
    return 0;
}

void makeUnoriented(struct Graph *graph) {
    for (uint32_t i = 0; i <= (uint32_t)(graph->num_of_nodes - 1); ++i)
        for (uint32_t j = 0; j < i; ++j)
            if (edgeExists(graph, i, j) || edgeExists(graph, j, i)) {
                setOrDeleteEdge(graph, i, j, 1);
                setOrDeleteEdge(graph, j, i, 1);
            }
}

int getComponents(struct Graph *graph, List *components) {
    initList(components);
    BitArray visited;
    if (initBitArray(&visited, graph->num_of_nodes)) return -1;
    for (uint32_t i = 0; i <= (uint32_t)(graph->num_of_nodes - 1); ++i)
        if (!getBitArrayElement(&visited, i)) {
            List *component = (List *)malloc(sizeof(List));
            if (!component) goto err;
            initList(component);
            if (
                deepFirstSearch(graph, component, &visited, i) ||
                pushList(components, component)
            ) {
                freeEquivalenceClass(component, 1);
                goto err;
            }
        }
    freeBitArray(&visited);
    return 0;
err:
    freeBitArray(&visited);
    freeListOfEquivalenceClasses(components,  1);
    return -2;
}

int reverseGraph(struct Graph *original, struct Graph *reversed) {
    if (initGraph(reversed, original->num_of_nodes))
        return -1;
    for (uint32_t i = 0; i <= (uint32_t)(original->num_of_nodes - 1); ++i)
        for (uint32_t j = 0; j <= (uint32_t)(original->num_of_nodes - 1); ++j)
            setOrDeleteEdge(reversed, i , j, edgeExists(original, j, i));
    return 0;
}

static int topologicalSort(struct Graph *graph, List *sorted) {
    BitArray visited;
    if (initBitArray(&visited, graph->num_of_nodes)) return -1;
    initList(sorted);
    for (uint32_t i = 0; i <= (uint32_t)(graph->num_of_nodes - 1); ++i)
        if (!getBitArrayElement(&visited, i))
            if (invertedDeepFirstSearch(graph, sorted, &visited, i)) {
                freeBitArray(&visited);
                freeEquivalenceClass(sorted, 1);
                return -2;
            }
    freeBitArray(&visited);
    return 0;
}

int getStronglyConnectedComponents(struct Graph *graph, List *components) {
    BitArray visited;
    if (initBitArray(&visited, graph->num_of_nodes)) return -1;
    struct Graph reversed;
    if (reverseGraph(graph, &reversed)) {
        freeBitArray(&visited);
        return -2;
    }
    List topological_sort;
    int rc = 0;
    if (topologicalSort(graph, &topological_sort)) {
        rc = -3;
        goto end;
    }
    initList(components);
    while (getListSize(&topological_sort)) {
        uint32_t *node = (uint32_t *)topList(&topological_sort);
        printSearchLog("getStronglyConnectedComponents. Текущее состояние: %u\n", *node);
        if (!getBitArrayElement(&visited, *node)) {
            List *component = (List *)malloc(sizeof(List));
            if (!component) {
                free(node);
                freeListOfEquivalenceClasses(components, 1);
                rc = -4;
                goto end;
            }
            initList(component);
            if (
                deepFirstSearch(graph, component, &visited, *node) ||
                pushList(components, component)
            ) {
                free(node);
                freeEquivalenceClass(component, 1);
                freeListOfEquivalenceClasses(components, 1);
                rc = -5;
                goto end;
            }
        }
        free(node);
    }
end:
    freeBitArray(&visited);
    freeEquivalenceClass(&topological_sort, 1);
    freeGraph(&reversed);
    return rc;
}

void printGraph(struct Graph *graph) {
    for (uint32_t i = 0; i <= (uint32_t)(graph->num_of_nodes - 1); ++i) {
        for (uint32_t j = 0; j <= (uint32_t)(graph->num_of_nodes - 1); ++j)
            printf("%u ", edgeExists(graph, i, j));
        printf("\n");
    }
}