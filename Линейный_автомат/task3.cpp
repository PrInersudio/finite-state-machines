
#include <limits>
#include <exception>
#include <stdexcept>
#include <iostream>
extern "C" {
#include "Graph.h"
}
#include "LinearFSM.hpp"




void linearFSMToGraph(struct Graph *graph, const LinearFSM &lin) {
    uint64_t num_states = lin.numStates();
    uint64_t num_inputs = lin.numInputs();
    if (initGraph(graph, num_states))
        throw std::runtime_error("Не удалось иницилизировать граф.");
    for (uint64_t i = 0; i < num_states; ++i)
        for (uint64_t j = 0; j < num_inputs; ++j)
            setOrDeleteEdge(
                graph, i,
                uint64_t(lin.stateFunction(
                    GFMatrix(lin.getGF(), lin.stateLength(), i),
                    GFMatrix(lin.getGF(), lin.inputLength(), j)
            )), 1);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <файл конфигурации линейного автомата>" << std::endl;
        return -1;
    }
    LinearFSM lin = initLinearFSM(argv[1]);
    if (lin.isStronglyConnected()) {
        std::cout << "Линейный автомат сильно связанный." << std::endl;
        return 0;
    }
    std::cout << "Линейный автомат не сильно связанный." << std::endl;
    struct Graph graph;
    try {
        linearFSMToGraph(&graph, lin);
    }
    catch (std::overflow_error &e) {
        std::cout <<
            "Размерности линейного автомата привышают пределы реализации Graph. "
            "Проверка на слабую связанность проводиться не будет."
        << std::endl;
        return 0;
    }
    makeUnoriented(&graph);
    List components;
    int rc = getComponents(&graph, &components);
    freeGraph(&graph);
    if (rc) throw std::runtime_error("Не удалось построить компоненты связанности графа.");
    if (getListSize(&components) == 1)
        std::cout << "Линейный автомат связанный." << std::endl;
    else
        std::cout << "Линейный автомат не связанный." << std::endl;
    deepClearList(&components, reinterpret_cast<FreeValueFunction>(clearList));
    return 0;
}