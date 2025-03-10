#include "ShiftRegister.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Использование: %s <файл_настроек> [--print-search-log]\n", argv[0]);
        return 0;
    }
    uint8_t print_search_log = 0;
    if (
        (argc >= 3) &&
        !strncmp(argv[2], "--print-search-log", strlen("--print-search-log"))
    ) {
        print_search_log = 1;
        enablePrintSearchLog();
    }
    struct ShiftRegister reg;
    if (initShiftRegisterFromFile(&reg, argv[1])) return -1;
    struct Graph graph;
    if (shiftRegisterToGraph(&reg, &graph)) {
        freeShiftRegister(&reg);
        return -2;
    }
    if (print_search_log) printGraph(&graph);
    int rc = 0;
    List components;
    if (getStronglyConnectedComponents(&graph, &components)) {
        rc = -3;
        goto end;
    }
    if (getListSize(&components) != 1) {
        printf("Не является сильно связаным.\n");
        printListOfEquivalenceClasses(&components, (PrintValue)printState);
    } else {
        deepClearList(&components, (FreeValueFunction)clearList);
        printf("Является сильно связаным.\n");
        goto end;
    }
    deepClearList(&components, (FreeValueFunction)clearList);
    makeUnoriented(&graph);
    if (getComponents(&graph, &components)) {
        rc = -4;
        goto end;
    }
    if (getListSize(&components) != 1) {
        printf("Не является связаным.\n");
        printListOfEquivalenceClasses(&components, (PrintValue)printState);
    } else printf("Является связаным.\n");
    deepClearList(&components, (FreeValueFunction)clearList);
end:
    freeShiftRegister(&reg);
    freeGraph(&graph);
    return rc;
}